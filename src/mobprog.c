/**************************************************************************
 * MOBProgram ported for CircleMUD 3.0 by Mattias Larsson               *
 * Traveller@AnotherWorld (ml@eniac.campus.luth.se 4000)                *
 **************************************************************************/

/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *  The MOBprograms have been contributed by N'Atas-ha.  Any support for   *
 *  these routines should not be expected from Merc Industries.  However,  *
 *  under no circumstances should the blame for bugs, etc be placed on     *
 *  Merc Industries.  They are not guaranteed to work on all systems due   *
 *  to their frequent use of strxxx functions.  They are also not the most *
 *  efficient way to perform their tasks, but hopefully should be in the   *
 *  easiest possible way to install and begin using. Documentation for     *
 *  such installation can be found in INSTALL.  Enjoy...         N'Atas-Ha *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"

extern char buf2[MAX_STRING_LENGTH];

struct number_list_type {
  int number;
  struct number_list_type *next;
};

extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct room_data *world;
extern struct char_data *character_list;
extern struct time_info_data time_info;

extern void death_cry(struct char_data *ch);
extern bool str_prefix(const char *astr, const char *bstr);
extern int number_percent(void);
extern int number_range(int from, int to);

#define bug(x, y) { sprintf(buf2, (x), (y)); stderr_log(buf2); }

/*
 * local file scope variables
 */

struct delayed_mprog_type *delayed_mprog = NULL;

/*
 * Local function prototypes
 */

char * mprog_next_command(char* clist);
int mprog_seval(char *lhs, char *opr, char *rhs);
int mprog_veval(int lhs, char* opr, int rhs);
int mprog_do_ifchck(char* ifchck, struct char_data* mob, struct char_data* actor, struct obj_data* obj, void* vo, struct char_data* rndm);
char * mprog_process_if(char* ifchck, char* com_list, struct char_data* mob, struct char_data* actor, struct obj_data* obj, void* vo, struct char_data* rndm);
void mprog_translate(char ch, char* t, struct char_data* mob, struct char_data* actor, struct obj_data* obj, void* vo, struct char_data* rndm);
void mprog_process_cmnd(char* cmnd, struct char_data* mob, struct char_data* actor, struct obj_data* obj, void* vo, struct char_data* rndm);
void mprog_driver(char* com_list, struct char_data* mob, struct char_data* actor, struct obj_data* obj, void* vo, struct char_data* rndm);

void mprog_pulse();

char* find_endif(char* com_list, struct char_data *mob);
char* find_else(char* com_list, struct char_data *mob);

int scan_time(char* timelist, struct number_list_type **yearlist, struct number_list_type **monthlist, struct number_list_type **weeklist, struct number_list_type **daywlist, struct number_list_type **daymlist, struct number_list_type **hourlist);

void free_time(struct number_list_type *yearlist, struct number_list_type *monthlist, struct number_list_type *weeklist, struct number_list_type *daywlist, struct number_list_type *daymlist, struct number_list_type *hourlist);

int match_time(struct number_list_type *list, int date_segment);

int istime(char *time_string);

/* This is here, even though it is an 'mp' command. We need
 * more data than the ACMD macro provides, so it will not
 * appear in the master command list; it will be handled
 * locally by mprog_process_cmnd as a special case (let it be
 * known that I HATE special cases, but couldn't find any
 * other way to do this -- brr)
 */
void handle_mpdelay(char* delay, char* cmnd, struct char_data* mob, struct char_data* actor, struct obj_data* obj, void* vo, struct char_data* rndm);

void extract_mobprog(struct delayed_mprog_type* mprog);

void mob_delay_purge(struct char_data* ch);

void mprog_mpextract(struct char_data* mob);

void mprog_mppurge(char* morebuf, char* cmnd, struct char_data* mob, struct char_data *actor, struct obj_data* obj, void* vo, struct char_data* rndm);

/***************************************************************************
 * Local function code and brief comments.
 */

/* Used in mprog_process_if to determine the end of the if-block so that
 * other subsequent commands are not skipped in the event of an error or
 * breaks in nested if statements. This will inprove the logical flow of
 * mob_prog processing to what it should be.
 */

char* find_endif(char* com_list, struct char_data* mob)
{

  int if_scope = 1;

  char* cmnd = '\0';

  char buf[MAX_INPUT_LENGTH];

  while (if_scope > 0) {
    cmnd = com_list;

    while (*com_list != '\n' && *com_list != '\r' && *com_list != '\0')
      com_list++;
    com_list++;

    while (isspace(*cmnd))
      cmnd++;

    if (*cmnd == '\0') {
      bug("Mob: %d missing else or endif in find_endif", mob_index[mob->nr].virtual);
      return '\0';
    }

    one_argument(cmnd, buf);

    if (!str_cmp(buf, "if"))
      if_scope++;
    if (!str_cmp(buf, "endif"))
      if_scope--;
  } /* while */

  return com_list;
} /* find_endif */

/* Used in mprog_process_if to determine the proper 'else' of the if-block so that
 * other subsequent commands are not skipped in the event of and error or
 * breaks in nested if statements. This will inprove the logical flow of
 * mob_prog processing to what it should be.
 */

char* find_else(char* com_list, struct char_data *mob)
{
  int if_scope = 1;

  char* cmnd = '\0';

  char buf[MAX_INPUT_LENGTH];

  while (if_scope > 0) {
    cmnd = com_list;

    while (*com_list != '\n' && *com_list != '\r' && *com_list != '\0')
      com_list++;
    com_list++;

    while (isspace(*cmnd))
      cmnd++;

    if (*cmnd == '\0') {
      bug("Mob: %d missing else or endif in find_else", mob_index[mob->nr].virtual);
      return '\0';
    }

    one_argument(cmnd, buf);

    if (!str_cmp(buf, "if"))
      if_scope++;
    if (!str_cmp(buf, "endif"))
      if_scope--;
    if ((!str_cmp(buf, "else")) && (if_scope == 1)) {
      return cmnd;
    }
  } /* while */

  return cmnd;
} /* find_else */

/* Used to get sequential lines of a multi line string (separated by "\n\r")
 * Thus its like one_argument(), but a trifle different. It is destructive
 * to the multi line string argument, and thus clist must not be shared.
 */

char *mprog_next_command(char *clist)
{

  char *pointer = clist;

  /* a little error checking here, if we are sent a null pointer
   * dereferencing it would cause a seg fault and crash the mud
   * this is defined as 'bad'. for a complete description of 'bad'
   * check your local dictionary :)
   */
  if (!clist)
    return NULL;

  if (*pointer == '\r')
    pointer++;
  if (*pointer == '\n')
    pointer++;

  while (*pointer != '\n' && *pointer != '\0' && *pointer != '\r')
    pointer++;
  if (*pointer == '\n') {
    *pointer = '\0';
    pointer++;
  }
  if (*pointer == '\r') {
    *pointer = '\0';
    pointer++;
  }

  return (pointer);
}

/* we need str_infix here because strstr is not case insensitive */

bool str_infix(const char *astr, const char *bstr)
{
  int sstr1;
  int sstr2;
  int ichar;
  char c0;

  if ((c0 = LOWER(astr[0])) == '\0')
    return FALSE;

  sstr1 = strlen(astr);
  sstr2 = strlen(bstr);

  for (ichar = 0; ichar <= sstr2 - sstr1; ichar++) {
    if (c0 == LOWER(bstr[ichar]) && !str_prefix(astr, bstr + ichar))
      return FALSE;
  }

  return TRUE;
}

/* These two functions do the basic evaluation of ifcheck operators.
 *  It is important to note that the string operations are not what
 *  you probably expect.  Equality is exact and division is substring.
 *  remember that lhs has been stripped of leading space, but can
 *  still have trailing spaces so be careful when editing since:
 *  "guard" and "guard " are not equal.
 */
int mprog_seval(char *lhs, char *opr, char *rhs)
{

  if (!str_cmp(opr, "=="))
    return (!str_cmp(lhs, rhs));
  if (!str_cmp(opr, "!="))
    return (str_cmp(lhs, rhs));
  if (!str_cmp(opr, "/"))
    return (!str_infix(rhs, lhs));
  if (!str_cmp(opr, "!/"))
    return (str_infix(rhs, lhs));

  sprintf(buf, "Improper MOBprog operator: %s", opr);
  stderr_log(buf);
  return '\0';

}

int mprog_veval(int lhs, char *opr, int rhs)
{

  if (!str_cmp(opr, "=="))
    return (lhs == rhs);
  if (!str_cmp(opr, "!="))
    return (lhs != rhs);
  if (!str_cmp(opr, ">"))
    return (lhs > rhs);
  if (!str_cmp(opr, "<"))
    return (lhs < rhs);
  if (!str_cmp(opr, "<="))
    return (lhs <= rhs);
  if (!str_cmp(opr, ">="))
    return (lhs >= rhs);
  if (!str_cmp(opr, "&"))
    return (lhs & rhs);
  if (!str_cmp(opr, "|"))
    return (lhs | rhs);

  sprintf(buf, "Improper MOBprog operator: %s", opr);
  stderr_log(buf);
  return 0;

}

/* This function performs the evaluation of the if checks.  It is
 * here that you can add any ifchecks which you so desire. Hopefully
 * it is clear from what follows how one would go about adding your
 * own. The syntax for an if check is: ifchck (arg) [opr val]
 * where the parenthesis are required and the opr and val fields are
 * optional but if one is there then both must be. The spaces are all
 * optional. The evaluation of the opr expressions is farmed out
 * to reduce the redundancy of the mammoth if statement list.
 * If there are errors, then return -1 otherwise return boolean 1,0
 */
int mprog_do_ifchck(char *ifchck, struct char_data *mob, struct char_data *actor, struct obj_data *obj, void *vo, struct char_data *rndm)
{
  char buf[MAX_INPUT_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  char opr[MAX_INPUT_LENGTH];
  char val[MAX_INPUT_LENGTH];
  struct char_data *vict = (struct char_data *) vo;
  struct obj_data *v_obj = (struct obj_data *) vo;
  char *bufpt = buf;
  char *argpt = arg;
  char *oprpt = opr;
  char *valpt = val;
  char *point = ifchck;
  int lhsvl;
  int rhsvl;

  if (*point == '\0') {
    bug("Mob: %d null ifchck", (int)mob_index[mob->nr].virtual);
    return -1;
  }
  /* skip leading spaces */
  while (*point == ' ')
    point++;

  /* get whatever comes before the left paren.. ignore spaces */
  while (*point != '(')
    if (*point == '\0') {
      bug("Mob: %d ifchck syntax error", mob_index[mob->nr].virtual);
      return -1;
    } else if (*point == ' ')
      point++;
    else
      *bufpt++ = *point++;

  *bufpt = '\0';
  point++;

  /* get whatever is in between the parens.. ignore spaces */
  while (*point != ')')
    if (*point == '\0') {
      bug("Mob: %d ifchck syntax error", mob_index[mob->nr].virtual);
      return -1;
    } else if (*point == ' ')
      point++;
    else
      *argpt++ = *point++;

  *argpt = '\0';
  point++;

  /* check to see if there is an operator */
  while (*point == ' ')
    point++;
  if (*point == '\0') {
    *opr = '\0';
    *val = '\0';
  } else /* there should be an operator and value, so get them */
  {
    while ((*point != ' ') && (!isalnum(*point)))
      if (*point == '\0') {
        bug("Mob: %d ifchck operator without value", mob_index[mob->nr].virtual);
        return -1;
      } else
        *oprpt++ = *point++;

    *oprpt = '\0';

    /* finished with operator, skip spaces and then get the value */
    while (*point == ' ')
      point++;
    for (;;) {
      if ((*point != ' ') && (*point == '\0'))
        break;
      else
        *valpt++ = *point++;
    }

    *valpt = '\0';
  }
  bufpt = buf;
  argpt = arg;
  oprpt = opr;
  valpt = val;

  /* Ok... now buf contains the ifchck, arg contains the inside of the
   *  parentheses, opr contains an operator if one is present, and val
   *  has the value if an operator was present.
   *  So.. basically use if statements and run over all known ifchecks
   *  Once inside, use the argument and expand the lhs. Then if need be
   *  send the lhs,opr,rhs off to be evaluated.
   */

  if (!str_cmp(buf, "istime")) {
    return istime(arg);
  }

  if (!str_cmp(buf, "rand")) {
    return (number_percent() <= atoi(arg));
  }

  if (!str_cmp(buf, "ispc")) {
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
      case 'i':
        return 0;
      case 'n':
        if (actor)
          return (!IS_NPC(actor));
        else
          return -1;
      case 't':
        if (vict)
          return (!IS_NPC(vict));
        else
          return -1;
      case 'r':
        if (rndm)
          return (!IS_NPC(rndm));
        else
          return -1;
      default:
        bug("Mob: %d bad argument to 'ispc'", mob_index[mob->nr].virtual);
        return -1;
    }
  }

  if (!str_cmp(buf, "isnpc")) {
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
      case 'i':
        return 1;
      case 'n':
        if (actor)
          return IS_NPC(actor);
        else
          return -1;
      case 't':
        if (vict)
          return IS_NPC(vict);
        else
          return -1;
      case 'r':
        if (rndm)
          return IS_NPC(rndm);
        else
          return -1;
      default:
        bug("Mob: %d bad argument to 'isnpc'", mob_index[mob->nr].virtual);
        return -1;
    }
  }

  if (!str_cmp(buf, "isgood")) {
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
      case 'i':
        return IS_GOOD(mob);
      case 'n':
        if (actor)
          return IS_GOOD(actor);
        else {
          return -1;
        }
      case 't':
        if (vict)
          return IS_GOOD(vict);
        else
          return -1;
      case 'r':
        if (rndm)
          return IS_GOOD(rndm);
        else
          return -1;
      default:
        bug("Mob: %d bad argument to 'isgood'", mob_index[mob->nr].virtual);
        return -1;
    }
  }

  if (!str_cmp(buf, "isfight")) {
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
      case 'i':
        return (FIGHTING(mob)) ? 1 : 0;
      case 'n':
        if (actor)
          return (FIGHTING(actor)) ? 1 : 0;
        else
          return -1;
      case 't':
        if (vict)
          return (FIGHTING(vict)) ? 1 : 0;
        else
          return -1;
      case 'r':
        if (rndm)
          return (FIGHTING(rndm)) ? 1 : 0;
        else
          return -1;
      default:
        bug("Mob: %d bad argument to 'isfight'", mob_index[mob->nr].virtual);
        return -1;
    }
  }

  if (!str_cmp(buf, "isimmort")) {
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
      case 'i':
        return (GET_LEVEL(mob) > LVL_IMMORT);
      case 'n':
        if (actor)
          return (GET_LEVEL(actor) > LVL_IMMORT);
        else
          return -1;
      case 't':
        if (vict)
          return (GET_LEVEL(vict) > LVL_IMMORT);
        else
          return -1;
      case 'r':
        if (rndm)
          return (GET_LEVEL(rndm) > LVL_IMMORT);
        else
          return -1;
      default:
        bug("Mob: %d bad argument to 'isimmort'", mob_index[mob->nr].virtual);
        return -1;
    }
  }

  if (!str_cmp(buf, "ischarmed")) {
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
      case 'i':
        return IS_AFFECTED(mob, AFF_CHARM);
      case 'n':
        if (actor)
          return IS_AFFECTED(actor, AFF_CHARM);
        else
          return -1;
      case 't':
        if (vict)
          return IS_AFFECTED(vict, AFF_CHARM);
        else
          return -1;
      case 'r':
        if (rndm)
          return IS_AFFECTED(rndm, AFF_CHARM);
        else
          return -1;
      default:
        bug("Mob: %d bad argument to 'ischarmed'", mob_index[mob->nr].virtual);
        return -1;
    }
  }

  if (!str_cmp(buf, "isfollow")) {
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
      case 'i':
        return (mob->master != NULL && mob->master->in_room == mob->in_room);
      case 'n':
        if (actor)
          return (actor->master != NULL && actor->master->in_room == actor->in_room);
        else
          return -1;
      case 't':
        if (vict)
          return (vict->master != NULL && vict->master->in_room == vict->in_room);
        else
          return -1;
      case 'r':
        if (rndm)
          return (rndm->master != NULL && rndm->master->in_room == rndm->in_room);
        else
          return -1;
      default:
        bug("Mob: %d bad argument to 'isfollow'", mob_index[mob->nr].virtual);
        return -1;
    }
  }

  if (!str_cmp(buf, "isaffected")) {
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
      case 'i':
        lhsvl = (1 << atoi(val));
        return IS_AFFECTED(mob, lhsvl);
      case 'n':
        if (actor) {
          lhsvl = (1 << atoi(val));
          return IS_AFFECTED(actor, lhsvl);
        } else
          return -1;
      case 't':
        if (vict) {
          lhsvl = (1 << atoi(val));
          return IS_AFFECTED(vict, lhsvl);
        } else
          return -1;
      case 'r':
        if (rndm) {
          lhsvl = (1 << atoi(val));
          return IS_AFFECTED(rndm, lhsvl);
        } else
          return -1;
      default:
        bug("Mob: %d bad argument to 'isaffected'", mob_index[mob->nr].virtual);
        return -1;
    }
  }

  if (!str_cmp(buf, "hitprcnt")) {
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
      case 'i':
        lhsvl = mob->points.hit / mob->points.max_hit;
        rhsvl = atoi(val);
        return mprog_veval(lhsvl, opr, rhsvl);
      case 'n':
        if (actor) {
          lhsvl = actor->points.hit / actor->points.max_hit;
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      case 't':
        if (vict) {
          lhsvl = vict->points.hit / vict->points.max_hit;
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      case 'r':
        if (rndm) {
          lhsvl = rndm->points.hit / rndm->points.max_hit;
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      default:
        bug("Mob: %d bad argument to 'hitprcnt'", mob_index[mob->nr].virtual);
        return -1;
    }
  }

  if (!str_cmp(buf, "inroom")) {
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
      case 'i':
        lhsvl = mob->in_room;
        rhsvl = real_room(atoi(val));
        return mprog_veval(lhsvl, opr, rhsvl);
      case 'n':
        if (actor) {
          lhsvl = actor->in_room;
          rhsvl = real_room(atoi(val));
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      case 't':
        if (vict) {
          lhsvl = vict->in_room;
          rhsvl = real_room(atoi(val));
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      case 'r':
        if (rndm) {
          lhsvl = rndm->in_room;
          rhsvl = real_room(atoi(val));
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      default:
        bug("Mob: %d bad argument to 'inroom'", mob_index[mob->nr].virtual);
        return -1;
    }
  }

  if (!str_cmp(buf, "sex")) {
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
      case 'i':
        lhsvl = mob->player.sex;
        rhsvl = atoi(val);
        return mprog_veval(lhsvl, opr, rhsvl);
      case 'n':
        if (actor) {
          lhsvl = actor->player.sex;
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      case 't':
        if (vict) {
          lhsvl = vict->player.sex;
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      case 'r':
        if (rndm) {
          lhsvl = rndm->player.sex;
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      default:
        bug("Mob: %d bad argument to 'sex'", mob_index[mob->nr].virtual);
        return -1;
    }
  }

  if (!str_cmp(buf, "position")) {
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
      case 'i':
        lhsvl = mob->char_specials.position;
        rhsvl = atoi(val);
        return mprog_veval(lhsvl, opr, rhsvl);
      case 'n':
        if (actor) {
          lhsvl = actor->char_specials.position;
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      case 't':
        if (vict) {
          lhsvl = vict->char_specials.position;
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      case 'r':
        if (rndm) {
          lhsvl = rndm->char_specials.position;
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      default:
        bug("Mob: %d bad argument to 'position'", mob_index[mob->nr].virtual);
        return -1;
    }
  }

  if (!str_cmp(buf, "level")) {
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
      case 'i':
        lhsvl = GET_LEVEL(mob);
        rhsvl = atoi(val);
        return mprog_veval(lhsvl, opr, rhsvl);
      case 'n':
        if (actor) {
          lhsvl = GET_LEVEL(actor);
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      case 't':
        if (vict) {
          lhsvl = GET_LEVEL(vict);
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      case 'r':
        if (rndm) {
          lhsvl = GET_LEVEL(rndm);
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      default:
        bug("Mob: %d bad argument to 'level'", mob_index[mob->nr].virtual);
        return -1;
    }
  }

  if (!str_cmp(buf, "class")) {
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
      case 'i':
        lhsvl = mob->player.class;
        rhsvl = atoi(val);
        return mprog_veval(lhsvl, opr, rhsvl);
      case 'n':
        if (actor) {
          lhsvl = actor->player.class;
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      case 't':
        if (vict) {
          lhsvl = vict->player.class;
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      case 'r':
        if (rndm) {
          lhsvl = rndm->player.class;
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      default:
        bug("Mob: %d bad argument to 'class'", mob_index[mob->nr].virtual);
        return -1;
    }
  }

  if (!str_cmp(buf, "goldamt")) {
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
      case 'i':
        lhsvl = mob->points.gold;
        rhsvl = atoi(val);
        return mprog_veval(lhsvl, opr, rhsvl);
      case 'n':
        if (actor) {
          lhsvl = actor->points.gold;
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      case 't':
        if (vict) {
          lhsvl = vict->points.gold;
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      case 'r':
        if (rndm) {
          lhsvl = rndm->points.gold;
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      default:
        bug("Mob: %d bad argument to 'goldamt'", mob_index[mob->nr].virtual);
        return -1;
    }
  }

  if (!str_cmp(buf, "objtype")) {
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
      case 'o':
        if (obj) {
          lhsvl = obj->obj_flags.type_flag;
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      case 'p':
        if (v_obj) {
          lhsvl = v_obj->obj_flags.type_flag;
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      default:
        bug("Mob: %d bad argument to 'objtype'", mob_index[mob->nr].virtual);
        return -1;
    }
  }

  if (!str_cmp(buf, "objval0")) {
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
      case 'o':
        if (obj) {
          lhsvl = obj->obj_flags.value[0];
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      case 'p':
        if (v_obj) {
          lhsvl = v_obj->obj_flags.value[0];
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      default:
        bug("Mob: %d bad argument to 'objval0'", mob_index[mob->nr].virtual);
        return -1;
    }
  }

  if (!str_cmp(buf, "objval1")) {
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
      case 'o':
        if (obj) {
          lhsvl = obj->obj_flags.value[1];
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      case 'p':
        if (v_obj) {
          lhsvl = v_obj->obj_flags.value[1];
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      default:
        bug("Mob: %d bad argument to 'objval1'", mob_index[mob->nr].virtual);
        return -1;
    }
  }

  if (!str_cmp(buf, "objval2")) {
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
      case 'o':
        if (obj) {
          lhsvl = obj->obj_flags.value[2];
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      case 'p':
        if (v_obj) {
          lhsvl = v_obj->obj_flags.value[2];
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      default:
        bug("Mob: %d bad argument to 'objval2'", mob_index[mob->nr].virtual);
        return -1;
    }
  }

  if (!str_cmp(buf, "objval3")) {
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
      case 'o':
        if (obj) {
          lhsvl = obj->obj_flags.value[3];
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      case 'p':
        if (v_obj) {
          lhsvl = v_obj->obj_flags.value[3];
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else
          return -1;
      default:
        bug("Mob: %d bad argument to 'objval3'", mob_index[mob->nr].virtual);
        return -1;
    }
  }

  if (!str_cmp(buf, "number")) {
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
      case 'i':
        lhsvl = mob_index[mob->nr].virtual;
        rhsvl = atoi(val);
        return mprog_veval(lhsvl, opr, rhsvl);
      case 'n':
        if (actor) {
          if (IS_NPC(actor)) {
            lhsvl = mob_index[actor->nr].virtual;
            rhsvl = atoi(val);
            return mprog_veval(lhsvl, opr, rhsvl);
          }
        } else {
          return -1;
        }
        break;
      case 't':
        if (vict) {
          if (IS_NPC(actor)) {
            lhsvl = mob_index[vict->nr].virtual;
            rhsvl = atoi(val);
            return mprog_veval(lhsvl, opr, rhsvl);
          }
        } else {
          return -1;
        }
        break;
      case 'r':
        if (rndm) {
          if (IS_NPC(actor)) {
            lhsvl = mob_index[rndm->nr].virtual;
            rhsvl = atoi(val);
            return mprog_veval(lhsvl, opr, rhsvl);
          }
        } else {
          return -1;
        }
        break;
      case 'o':
        if (obj) {
          lhsvl = obj_index[obj->item_number].virtual;
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else {
          return -1;
        }
        break;
      case 'p':
        if (v_obj) {
          lhsvl = obj_index[v_obj->item_number].virtual;
          rhsvl = atoi(val);
          return mprog_veval(lhsvl, opr, rhsvl);
        } else {
          return -1;
        }
        break;
      default:
        bug("Mob: %d bad argument to 'number'", mob_index[mob->nr].virtual);
        return -1;
        break;
    }
  }

  if (!str_cmp(buf, "name")) {
    switch (arg[1]) /* arg should be "$*" so just get the letter */
    {
      case 'i':
        return mprog_seval(mob->player.name, opr, val);
      case 'n':
        if (actor)
          return mprog_seval(actor->player.name, opr, val);
        else
          return -1;
      case 't':
        if (vict)
          return mprog_seval(vict->player.name, opr, val);
        else
          return -1;
      case 'r':
        if (rndm)
          return mprog_seval(rndm->player.name, opr, val);
        else
          return -1;
      case 'o':
        if (obj)
          return mprog_seval(obj->name, opr, val);
        else
          return -1;
      case 'p':
        if (v_obj)
          return mprog_seval(v_obj->name, opr, val);
        else
          return -1;
      default:
        bug("Mob: %d bad argument to 'name'", mob_index[mob->nr].virtual);
        return -1;
    }
  }

  /* Ok... all the ifchcks are done, so if we didnt find ours then something
   * odd happened.  So report the bug and abort the MOBprogram (return error)
   */
  bug("Mob: %d unknown ifchck", mob_index[mob->nr].virtual);
  return -1;

}
/* Quite a long and arduous function, this guy handles the control
 * flow part of MOBprograms.  Basicially once the driver sees an
 * 'if' attention shifts to here.  While many syntax errors are
 * caught, some will still get through due to the handling of break
 * and errors in the same fashion.  The desire to break out of the
 * recursion without catastrophe in the event of a mis-parse was
 * believed to be high. Thus, if an error is found, it is bugged and
 * the parser acts as though a break were issued and just bails out
 * at that point. I havent tested all the possibilites, so I'm speaking
 * in theory, but it is 'guaranteed' to work on syntactically correct
 * MOBprograms, so if the mud crashes here, check the mob carefully!
 */

char null[1];

char *mprog_process_if(char *ifchck, char *com_list, struct char_data *mob, struct char_data *actor, struct obj_data *obj, void *vo, struct char_data *rndm)
{

  char buf[MAX_INPUT_LENGTH*2];
  char buf2[MAX_INPUT_LENGTH*2];
  char *morebuf = '\0';
  char *cmnd = '\0';
  int loopdone = FALSE;
  int flag = FALSE;
  int legal;

  char *end_list = '\0';
  char *else_list = '\0';

  *null = '\0';

  /* find the end of the list if-block for returning instead of
   * completely dropping out the mob-prog
   */

  end_list = find_endif(com_list, mob);

  /* skip all of the if-block to the proper 'else' segment
   * if there is no 'else' segment, this function will skip
   * to the proper 'endif' -- it's amazingly cool :) brr
   */

  else_list = find_else(com_list, mob);

  /* check for trueness of the ifcheck */
  if ((legal = mprog_do_ifchck(ifchck, mob, actor, obj, vo, rndm)))
    flag = TRUE;

  while (loopdone == FALSE) /*scan over any existing or statements */
  {
    cmnd = com_list;
    com_list = mprog_next_command(com_list);
    while (*cmnd == ' ')
      cmnd++;
    if (*cmnd == '\0') {
      bug("Mob: %d no commands after IF/OR", mob_index[mob->nr].virtual);
      return null;
    }
    morebuf = one_argument(cmnd, buf);
    if (!str_cmp(buf, "or")) {
      if ((legal = mprog_do_ifchck(morebuf, mob, actor, obj, vo, rndm)))
        flag = TRUE;
    } else
      loopdone = TRUE;
  }

  if (flag)
    for (;;) /*ifcheck was true, do commands but ignore else to endif*/
    {
      if (!str_cmp(buf, "if")) {
        com_list = mprog_process_if(morebuf, com_list, mob, actor, obj, vo, rndm);
        while (*cmnd == ' ')
          cmnd++;
        if (*com_list == '\0')
          return null;
        cmnd = com_list;
        com_list = mprog_next_command(com_list);
        morebuf = one_argument(cmnd, buf);
        continue;
      }
      if ((!str_cmp(buf, "break")) || (!str_cmp(buf, "endif")) || (!str_cmp(buf, "else")))
        return end_list;

      strcpy(buf2, cmnd);
      strcat(buf2, com_list);
      mprog_process_cmnd(buf2, mob, actor, obj, vo, rndm);
      if (!(*buf2))
        com_list[0] = '\0';
      cmnd = com_list;
      com_list = mprog_next_command(com_list);
      while (*cmnd == ' ')
        cmnd++;
      if (*cmnd == '\0') {
        bug("Mob: %d missing else or endif", mob_index[mob->nr].virtual);
        return null;
      }
      morebuf = one_argument(cmnd, buf);
    }
  else /*false ifcheck, find else and do existing commands or quit at endif*/
  {
    com_list = else_list;
    cmnd = com_list;
    com_list = mprog_next_command(com_list);

    morebuf = one_argument(cmnd, buf);

    /* found either an else or an endif.. act accordingly */
    if (!str_cmp(buf, "endif")) {
      return com_list;
    }
    cmnd = com_list;
    com_list = mprog_next_command(com_list);
    if (!cmnd) {
      bug("Mob: %d missing endif", mob_index[mob->nr].virtual);
      return null;
    }
    while (*cmnd == ' ')
      cmnd++;
    if (*cmnd == '\0') {
      bug("Mob: %d missing endif", mob_index[mob->nr].virtual);
      return null;
    }
    morebuf = one_argument(cmnd, buf);

    for (;;) /*process the post-else commands until an endif is found.*/
    {
      if (!str_cmp(buf, "if")) {
        com_list = mprog_process_if(morebuf, com_list, mob, actor, obj, vo, rndm);
        while (*cmnd == ' ')
          cmnd++;
        if (*com_list == '\0')
          return null;
        cmnd = com_list;
        com_list = mprog_next_command(com_list);
        morebuf = one_argument(cmnd, buf);
        continue;
      }
      if (!str_cmp(buf, "else")) {
        bug("Mob: %d found else in an else section", mob_index[mob->nr].virtual);
        return end_list;
      }
      if ((!str_cmp(buf, "break")) || (!str_cmp(buf, "endif")))
        return end_list;

      strcpy(buf2, cmnd);
      strcat(buf2, com_list);
      mprog_process_cmnd(buf2, mob, actor, obj, vo, rndm);
      if (!(*buf2))
        com_list[0] = '\0';
      cmnd = com_list;
      com_list = mprog_next_command(com_list);
      while (*cmnd == ' ')
        cmnd++;
      if (*cmnd == '\0') {
        bug("Mob:%d missing endif in else section", mob_index[mob->nr].virtual);
        return null;
      }
      morebuf = one_argument(cmnd, buf);
    }
  }
  return null;
}

/* This routine handles the variables for command expansion.
 * If you want to add any go right ahead, it should be fairly
 * clear how it is done and they are quite easy to do, so you
 * can be as creative as you want. The only catch is to check
 * that your variables exist before you use them. At the moment,
 * using $t when the secondary target refers to an object
 * i.e. >prog_act drops~<nl>if ispc($t)<nl>sigh<nl>endif<nl>~<nl>
 * probably makes the mud crash (vice versa as well) The cure
 * would be to change act() so that vo becomes vict & v_obj.
 * but this would require a lot of small changes all over the code.
 */
void mprog_translate(char ch, char *t, struct char_data *mob, struct char_data *actor, struct obj_data *obj, void *vo, struct char_data *rndm)
{
  static char *he_she[] = {"it", "he", "she"};
  static char *him_her[] = {"it", "him", "her"};
  static char *his_her[] = {"its", "his", "her"};
  struct char_data *vict = (struct char_data *) vo;
  struct obj_data *v_obj = (struct obj_data *) vo;

  *t = '\0';
  switch (ch) {
    case 'i':
      one_argument(mob->player.name, t);
      break;

    case 'I':
      strcpy(t, mob->player.short_descr);
      break;

    case 'n':
      if (actor) {
        if (CAN_SEE(mob, actor)) {
          if (!IS_NPC(actor)) {
            strcpy(t, actor->player.name);
          } else
            one_argument(actor->player.name, t);
        } else
          strcpy(t, "Someone");
      }
      break;

    case 'N':
      if (actor) {
        if (CAN_SEE(mob, actor)) {
          if (IS_NPC(actor)) {
            strcpy(t, actor->player.short_descr);
          } else {
            strcpy(t, actor->player.name);
            strcat(t, " ");
            strcat(t, actor->player.title);
          }
        } else {
          strcpy(t, "someone");
        }
      }
      break;

    case 't':
      if (vict) {
        if (CAN_SEE(mob, vict)) {
          if (!IS_NPC(vict)) {
            strcpy(t, vict->player.name);
          } else {
            one_argument(vict->player.name, t);
          }
        } else {
          strcpy(t, "Someone");
        }
      }
      break;

    case 'T':
      if (vict) {
        if (CAN_SEE(mob, vict)) {
          if (IS_NPC(vict)) {
            strcpy(t, vict->player.short_descr);
          } else {
            strcpy(t, vict->player.name);
            strcat(t, " ");
            strcat(t, vict->player.title);
          }
        } else {
          strcpy(t, "someone");
        }
      }
      break;

    case 'r':
      if (rndm) {
        if (CAN_SEE(mob, rndm)) {
          if (!IS_NPC(rndm)) {
            strcpy(t, rndm->player.name);
          } else {
            one_argument(rndm->player.name, t);
          }
        } else {
          strcpy(t, "Someone");
        }
      }
      break;

    case 'R':
      if (rndm) {
        if (CAN_SEE(mob, rndm)) {
          if (IS_NPC(rndm)) {
            strcpy(t, rndm->player.short_descr);
          } else {
            strcpy(t, rndm->player.name);
            strcat(t, " ");
            strcat(t, rndm->player.title);
          }
        } else {
          strcpy(t, "someone");
        }
      }
      break;

    case 'e':
      if (actor) {
        CAN_SEE(mob, actor) ? strcpy(t, he_she[(int) actor->player.sex]) : strcpy(t, "someone");
      }
      break;

    case 'm':
      if (actor) {
        CAN_SEE(mob, actor) ? strcpy(t, him_her[(int) actor->player.sex]) : strcpy(t, "someone");
      }
      break;

    case 's':
      if (actor) {
        CAN_SEE(mob, actor) ? strcpy(t, his_her[(int) actor->player.sex]) : strcpy(t, "someone's");
      }
      break;

    case 'E':
      if (vict) {
        CAN_SEE(mob, vict) ? strcpy(t, he_she[(int) vict->player.sex]) : strcpy(t, "someone");
      }
      break;

    case 'M':
      if (vict) {
        CAN_SEE(mob, vict) ? strcpy(t, him_her[(int) vict->player.sex]) : strcpy(t, "someone");
      }
      break;

    case 'S':
      if (vict) {
        CAN_SEE(mob, vict) ? strcpy(t, his_her[(int) vict->player.sex]) : strcpy(t, "someone's");
      }
      break;

    case 'j':
      strcpy(t, he_she[(int) mob->player.sex]);
      break;

    case 'k':
      strcpy(t, him_her[(int) mob->player.sex]);
      break;

    case 'l':
      strcpy(t, his_her[(int) mob->player.sex]);
      break;

    case 'J':
      if (rndm) {
        CAN_SEE(mob, rndm) ? strcpy(t, he_she[(int) rndm->player.sex]) : strcpy(t, "someone");
      }
      break;

    case 'K':
      if (rndm) {
        CAN_SEE(mob, rndm) ? strcpy(t, him_her[(int) rndm->player.sex]) : strcpy(t, "someone");
      }
      break;

    case 'L':
      if (rndm) {
        CAN_SEE(mob, rndm) ? strcpy(t, his_her[(int) rndm->player.sex]) : strcpy(t, "someone's");
      }
      break;

    case 'o':
      if (obj) {
        CAN_SEE_OBJ(mob, obj) ? one_argument(obj->name, t) : strcpy(t, "something");
      }
      break;

    case 'O':
      if (obj) {
        CAN_SEE_OBJ(mob, obj) ? strcpy(t, obj->short_description) : strcpy(t, "something");
      }
      break;

    case 'p':
      if (v_obj) {
        CAN_SEE_OBJ(mob, v_obj) ? one_argument(v_obj->name, t) : strcpy(t, "something");
      }
      break;

    case 'P':
      if (v_obj) {
        CAN_SEE_OBJ(mob, v_obj) ? strcpy(t, v_obj->short_description) : strcpy(t, "something");
      }
      break;

    case 'a':
      if (obj)
        switch (*(obj->name)) {
          case 'a':
          case 'e':
          case 'i':
          case 'o':
          case 'u':
            strcpy(t, "an");
            break;
          default:
            strcpy(t, "a");
        }
      break;

    case 'A':
      if (v_obj)
        switch (*(v_obj->name)) {
          case 'a':
          case 'e':
          case 'i':
          case 'o':
          case 'u':
            strcpy(t, "an");
            break;
          default:
            strcpy(t, "a");
        }
      break;

    case '$':
      strcpy(t, "$");
      break;

    default:
      bug("Mob: %d bad $var", mob_index[mob->nr].virtual);
      break;
  }

  return;

}

/* This procedure simply copies the cmnd to a buffer while expanding
 * any variables by calling the translate procedure.  The observant
 * code scrutinizer will notice that this is taken from act()
 */
void mprog_process_cmnd(char *cmnd, struct char_data *mob, struct char_data *actor, struct obj_data *obj, void *vo, struct char_data *rndm)
{
  char buf[MAX_INPUT_LENGTH];
  char* command_list;
  char *morebuf;
  char tmp[MAX_INPUT_LENGTH * 2];
  char *str;
  char *i;
  char *point;
  int j;

  point = buf;
  str = cmnd;
  /* brr */

  /* This is that damn special case I was talking about. mpdelay
   * cannot be done as an ACMD function. It is implemented here as
   * a special case function called handle_mpdelay. There is an
   * ACMD(do_mpdelay) defined and implemented as an empty function
   * just for good form. -- brr
   */

  /* Same thing goes for mpextract and mppurge.  Possible bad things can happen
   if mobs have been triggered multiple times before any triggers begin. This
   litte work around will pull the mob out of whatever room it's in, give it
   time to work out all the triggers it's received, send the triggered progs
   into the mpdelay list, then purge the mob, and the progs from the delayed
   list, hopefully avoiding nastiness.
   */
  command_list = cmnd;
  command_list = mprog_next_command(command_list);
  morebuf = one_argument(cmnd, buf);

  if (!str_cmp(buf, "mpdelay")) {
    handle_mpdelay(morebuf, command_list, mob, actor, obj, vo, rndm);
    cmnd[0] = '\0';
    return;
  }

  if (!str_cmp(buf, "mpextract")) {
    mudlog("Performing actual extraction now", 'Q', COM_QUEST, FALSE);
    mprog_mpextract(mob);
    cmnd[0] = '\0';
    return;
  }

  if (!str_cmp(buf, "mppurge")) {
    mprog_mppurge(morebuf, cmnd, mob, actor, obj, vo, rndm);
    cmnd[0] = '\0';
    return;
  }

  /* and back to your regularly schedule programming ... btw
   * I handle the mpdelay above this section of code, because
   * I know that the parameters of mpdelay do not have any
   * variables in them ... also ... I HATE special cases -- brr
   */

  while (*str != '\0') {
    if (*str != '$') {
      *point++ = *str++;
      continue;
    }
    str++;
    mprog_translate(*str, tmp, mob, actor, obj, vo, rndm);
    i = tmp;
    ++str;
    while ((*point = *i) != '\0')
      ++point, ++i;
  }
  *point = '\0';
  str = buf;
  j = 1;
  while (j < MAX_INPUT_LENGTH - 2) {
    if (str[j] == '\n') {
      str[j] = '\0';
      break;
    }
    if (str[j] == '\r') {
      str[j] = '\0';
      break;
    }
    if (str[j] == '\0')
      break;
    j++;
  }

  command_interpreter(mob, buf);

  return;

}

/* The main focus of the MOBprograms.  This routine is called
 *  whenever a trigger is successful.  It is responsible for parsing
 *  the command list and figuring out what to do. However, like all
 *  complex procedures, everything is farmed out to the other guys.
 */
void mprog_driver(char *com_list, struct char_data *mob, struct char_data *actor, struct obj_data *obj, void *vo, struct char_data *rndm)
{

  char tmpcmndlst[MAX_STRING_LENGTH];
  char buf[MAX_INPUT_LENGTH*2];
  char buf2[MAX_INPUT_LENGTH*2];
  char *morebuf;
  char *command_list;
  char *cmnd;
  struct char_data *vch = NULL;
  int count = 0;

  if (IS_AFFECTED(mob, AFF_CHARM)) {
    return;
  }

  if (!rndm) {
    /* get a random visable mortal player who is in the room with the mob */
    for (vch = world[mob->in_room].people; vch; vch = vch->next_in_room)
      if (!IS_NPC(vch) && vch->player.level < LVL_IMMORT && CAN_SEE(mob, vch)) {
        if (number_range(0, count) == 0)
          rndm = vch;
        count++;
      }
  }

  strcpy(tmpcmndlst, com_list);
  command_list = tmpcmndlst;
  cmnd = command_list;
  command_list = mprog_next_command(command_list);
  while (*cmnd != '\0') {
    morebuf = one_argument(cmnd, buf);
    if (!str_cmp(buf, "if"))
      command_list = mprog_process_if(morebuf, command_list, mob, actor, obj, vo, rndm);

    /* The following two 'else if's ("else" and "endif") are
     * for re-entrant mob-progs. These will be needed for the
     * mpdelay function being added.
     */
    else if (!str_cmp(buf, "else")) {
      command_list = find_endif(command_list, mob);
      mprog_process_cmnd(command_list, mob, actor, obj, vo, rndm);
      /* we may have to do something else here, but I'm too tired to
       * trace it down now :) -- brr
       */
    } else if (!str_cmp(buf, "endif")) {
      /* really do nothing */
    }

    /* back to our originally scheduled programming */
    else {
      /* ... or maybe not. Okay, here's the deal. mpdelay needs all
       * of the remaining commands sent to it, and not just the line
       * it occupies (as was previously sent). So instead of sending
       * a single line, we'll send the whole damn thing. That means
       * mprog_process_cmnd is responsible for breaking it back up.
       */

      /* this also needs to happen in mprog_process_if */
      strcpy(buf2, cmnd);
      strcat(buf2, command_list);
      mprog_process_cmnd(buf2, mob, actor, obj, vo, rndm);
      if (!(*buf2))
        command_list[0] = '\0';
    }
    cmnd = command_list;
    command_list = mprog_next_command(command_list);
  }

  return;

}

/* mprog_pulse is the counter for mpdelay. This function
 * will get called at each game pulse (currently, could
 * change) and process all (if any) delayed mobprogs
 */

void mprog_pulse()
{

  struct delayed_mprog_type *current;
  struct delayed_mprog_type *next;

  if (delayed_mprog) {
    for (current = delayed_mprog; current; current = next) {
      next = current->next;
      (current->delay)--;

      if (current->delay <= 0) {
        mprog_driver(current->remaining_cmnds, current->mob, current->actor, current->obj, current->vo, current->rndm);

        extract_mobprog(current);
      }
    }
  }
}

/***************************************************************************
 * Global function code and brief comments.
 */

/* The next two routines are the basic trigger types. Either trigger
 *  on a certain percent, or trigger on a keyword or word phrase.
 *  To see how this works, look at the various trigger routines..
 */
void mprog_wordlist_check(char *arg, struct char_data *mob, struct char_data *actor, struct obj_data *obj, void *vo, int type)
{

  char temp1[MAX_STRING_LENGTH];
  char temp2[MAX_INPUT_LENGTH];
  char word[MAX_INPUT_LENGTH];
  MPROG_DATA *mprg;
  char *list;
  char *start;
  char *dupl;
  char *end;
  int i;

  for (mprg = mob_index[mob->nr].mobprogs; mprg != NULL; mprg = mprg->next)
    if (mprg->type & type) {
      strcpy(temp1, mprg->arglist);
      list = temp1;
      while (isspace(*list))
        list++;
      for (i = 0; i < strlen(list); i++)
        list[i] = LOWER(list[i]);
      strcpy(temp2, arg);
      dupl = temp2;
      for (i = 0; i < strlen(dupl); i++)
        dupl[i] = LOWER(dupl[i]);
      if ((list[0] == 'p') && (list[1] == ' ')) {
        list += 2;
        while ((start = strstr(dupl, list)))
          if ((start == dupl || *(start - 1) == ' ') && (*(end = start + strlen(list)) == ' ' || *end == '\n' || *end == '\r' || *end == '\0')) {
            mprog_driver(mprg->comlist, mob, actor, obj, vo, NULL);
            break;
          } else
            dupl = start + 1;
      } else {
        list = one_argument(list, word);
        for (; word[0] != '\0'; list = one_argument(list, word))
          while ((start = strstr(dupl, word)))
            if ((start == dupl || *(start - 1) == ' ') && (*(end = start + strlen(word)) == ' ' || *end == '\n' || *end == '\r' || *end == '\0')) {
              mprog_driver(mprg->comlist, mob, actor, obj, vo, NULL);
              break;
            } else
              dupl = start + 1;
      }
    }

  return;

}

void mprog_percent_check(struct char_data *mob, struct char_data *actor, struct obj_data *obj, void *vo, int type)
{
  MPROG_DATA * mprg;

  for (mprg = mob_index[mob->nr].mobprogs; mprg != NULL; mprg = mprg->next)
    if ((mprg->type & type) && (number_percent() < atoi(mprg->arglist)) && mob->mob_specials.mp_toggle == FALSE) {
      mprog_driver(mprg->comlist, mob, actor, obj, vo, NULL);
      if (type != GREET_PROG && type != ALL_GREET_PROG)
        break;
    }

  return;

}

/* The triggers.. These are really basic, and since most appear only
 * once in the code (hmm. i think they all do) it would be more efficient
 * to substitute the code in and make the mprog_xxx_check routines global.
 * However, they are all here in one nice place at the moment to make it
 * easier to see what they look like. If you do substitute them back in,
 * make sure you remember to modify the variable names to the ones in the
 * trigger calls.
 */
void mprog_act_trigger(char *buf, struct char_data *mob, struct char_data *ch, struct obj_data *obj, void *vo)
{

  MPROG_ACT_LIST * tmp_act;

  if (IS_NPC(mob) && (mob_index[mob->nr].progtypes & ACT_PROG) && (mob->mob_specials.mp_toggle == FALSE) && (mob != ch)) {
    tmp_act = malloc(sizeof(MPROG_ACT_LIST));
    if (mob->mpactnum > 0)
      tmp_act->next = mob->mpact;
    else
      tmp_act->next = NULL;

    mob->mpact = tmp_act;
    mob->mpact->buf = strdup(buf);
    mob->mpact->ch = ch;
    mob->mpact->obj = obj;
    mob->mpact->vo = vo;
    mob->mpactnum++;

  }
  return;

}

void mprog_bribe_trigger(struct char_data *mob, struct char_data *ch, int amount)
{

  MPROG_DATA *mprg;
  int plat;
  int gold;
  int silver;
  int copper;
  int temp;

  if (IS_NPC(mob) && (mob->mob_specials.mp_toggle == FALSE) && (mob_index[mob->nr].progtypes & BRIBE_PROG)) {
    temp = amount;
    plat = temp / 1000;
    temp -= plat * 1000;
    gold = temp / 100;
    temp -= gold * 100;
    silver = temp / 10;
    temp -= silver * 10;
    copper = temp;

    mob->points.plat -= plat;
    mob->points.gold -= gold;
    mob->points.silver -= silver;
    mob->points.copper -= copper;

    for (mprg = mob_index[mob->nr].mobprogs; mprg != NULL; mprg = mprg->next)
      if ((mprg->type & BRIBE_PROG) && (amount >= atoi(mprg->arglist))) {
        mprog_driver(mprg->comlist, mob, ch, NULL, NULL, NULL);
        break;
      }
  }

  return;

}

void mprog_death_trigger(struct char_data *mob, struct char_data *killer)
{

  if (IS_NPC(mob) && (mob->mob_specials.mp_toggle == FALSE) && (mob_index[mob->nr].progtypes & DEATH_PROG)) {
    mprog_percent_check(mob, killer, NULL, NULL, DEATH_PROG);
  }

  death_cry(mob);
  return;

}

void mprog_entry_trigger(struct char_data *mob)
{

  if (IS_NPC(mob) && (mob->mob_specials.mp_toggle == FALSE) && (mob_index[mob->nr].progtypes & ENTRY_PROG))
    mprog_percent_check(mob, NULL, NULL, NULL, ENTRY_PROG);

  return;

}

void mprog_fight_trigger(struct char_data *mob, struct char_data *ch)
{

  if (IS_NPC(mob) && (mob->mob_specials.mp_toggle == FALSE) && (mob_index[mob->nr].progtypes & FIGHT_PROG))
    mprog_percent_check(mob, ch, NULL, NULL, FIGHT_PROG);

  return;

}

void mprog_give_trigger(struct char_data *mob, struct char_data *ch, struct obj_data *obj)
{

  char buf[MAX_INPUT_LENGTH];
  MPROG_DATA *mprg;

  if (IS_NPC(mob) && (mob->mob_specials.mp_toggle == FALSE) && (mob_index[mob->nr].progtypes & GIVE_PROG))
    for (mprg = mob_index[mob->nr].mobprogs; mprg != NULL; mprg = mprg->next) {
      one_argument(mprg->arglist, buf);
      if ((mprg->type & GIVE_PROG) && ((!str_infix(obj->name, mprg->arglist)) || (!str_cmp("all", buf)))) {
        mprog_driver(mprg->comlist, mob, ch, obj, NULL, NULL);
        break;
      }
    }

  return;

}

void mprog_greet_trigger(struct char_data *ch)
{

  struct char_data *vmob;

  for (vmob = world[ch->in_room].people; vmob != NULL; vmob = vmob->next_in_room) {
    if (IS_NPC(vmob) && (ch->mob_specials.mp_toggle == FALSE) && ch != vmob && CAN_SEE(vmob, ch) && (vmob->char_specials.fighting == NULL) && AWAKE(vmob) && (mob_index[vmob->nr].progtypes & GREET_PROG)) {
      mprog_percent_check(vmob, ch, NULL, NULL, GREET_PROG);
    } else {
      if (IS_NPC(vmob) && (vmob->char_specials.fighting == NULL) && AWAKE(vmob) && (mob_index[vmob->nr].progtypes & ALL_GREET_PROG)) {
        mprog_percent_check(vmob, ch, NULL, NULL, ALL_GREET_PROG);
      }
    }
  }
}

void mprog_hitprcnt_trigger(struct char_data *mob, struct char_data *ch)
{

  MPROG_DATA *mprg;

  if (IS_NPC(mob) && (mob->mob_specials.mp_toggle == FALSE) && (mob_index[mob->nr].progtypes & HITPRCNT_PROG))
    for (mprg = mob_index[mob->nr].mobprogs; mprg != NULL; mprg = mprg->next)
      if ((mprg->type & HITPRCNT_PROG) && ((100 * mob->points.hit / mob->points.max_hit) < atoi(mprg->arglist))) {
        mprog_driver(mprg->comlist, mob, ch, NULL, NULL, NULL);
        break;
      }

  return;

}

void mprog_random_trigger(struct char_data *mob)
{
  if ((mob_index[mob->nr].progtypes & RAND_PROG) && (mob->mob_specials.mp_toggle == FALSE))
    mprog_percent_check(mob, NULL, NULL, NULL, RAND_PROG);

  return;

}

void mprog_speech_trigger(char *txt, struct char_data *mob)
{

  struct char_data *vmob;

  for (vmob = world[mob->in_room].people; vmob != NULL; vmob = vmob->next_in_room)
    if ((mob != vmob) && IS_NPC(vmob) && (mob->mob_specials.mp_toggle == FALSE) && (mob_index[vmob->nr].progtypes & SPEECH_PROG))
      mprog_wordlist_check(txt, vmob, mob, NULL, NULL, SPEECH_PROG);

  return;

}

void mprog_shout_trigger(char *txt, struct char_data *mob)
{

  struct char_data *vmob;

  for (vmob = character_list; vmob != NULL; vmob = vmob->next) {
    if ((world[mob->in_room].zone == world[vmob->in_room].zone) && (mob->mob_specials.mp_toggle == FALSE) && (GET_POS (vmob) > POS_RESTING)) {
      if ((mob != vmob) && IS_NPC(vmob) && (mob_index[vmob->nr].progtypes & SHOUT_PROG)) {
        mprog_wordlist_check(txt, vmob, mob, NULL, NULL, SHOUT_PROG);
      }
    }
  }

  return;

}
/*  HOLLER REMOVED -- Meith
 void mprog_holler_trigger(char *txt, struct char_data *mob)
 {

 struct char_data *vmob;

 for (vmob = character_list; vmob != NULL; vmob = vmob->next)
 {
 if (GET_POS (vmob) > POS_RESTING)
 {
 if ((mob != vmob) && IS_NPC(vmob) && (mob_index[vmob->nr].progtypes & HOLLER_PROG ))
 {
 mprog_wordlist_check(txt, vmob, mob, NULL, NULL, HOLLER_PROG);
 }
 }
 }

 return;

 }
 */
void mprog_tell_trigger(char *txt, struct char_data *mob, struct char_data *vmob)
{

  if ((mob != vmob) && IS_NPC(vmob) && (mob->mob_specials.mp_toggle == FALSE) && (mob_index[vmob->nr].progtypes & TELL_PROG))
    mprog_wordlist_check(txt, vmob, mob, NULL, NULL, TELL_PROG);

  return;

}

void mprog_ask_trigger(char *txt, struct char_data *mob, struct char_data *vmob)
{

  if ((mob != vmob) && IS_NPC(vmob) && (mob->mob_specials.mp_toggle == FALSE) && (mob_index[vmob->nr].progtypes & ASK_PROG))
    mprog_wordlist_check(txt, vmob, mob, NULL, NULL, ASK_PROG);

  return;

}

void mprog_time_trigger(struct time_info_data time)

{

  char buf[MAX_INPUT_LENGTH];

  struct char_data *list;

  char error[64];

  int year, month, week, dayw, daym, hour;
  struct number_list_type *pyear, *pmonth, *pweek, *pdayw, *pdaym, *phour;

  MPROG_DATA *mprg;

  year = time.year;
  month = time.month + 1;
  daym = time.day + 1;
  hour = time.hours;

  dayw = 1 + ((daym - 1) % 7);
  week = 1 + (daym / 7);

  for (list = character_list; list; list = list->next) {
    if (IS_NPC(list) && (list->mob_specials.mp_toggle == FALSE) && (mob_index[list->nr].progtypes & TIME_PROG)) {
      for (mprg = mob_index[list->nr].mobprogs; mprg; mprg = mprg->next) {
        pyear = NULL;
        pmonth = NULL;
        pweek = NULL;
        pdayw = NULL;
        pdaym = NULL;
        phour = NULL;

        strcpy(buf, mprg->arglist);
        if ((mprg->type & TIME_PROG)) {
          if (scan_time(buf, &pyear, &pmonth, &pweek, &pdayw, &pdaym, &phour) == -2) {
            sprintf(error, "time_prog error: bad time format in mob %d", mob_index[list->nr].virtual);
            mudlog(error, 'E', COM_IMMORT, TRUE);
            continue;
          }

          if (!match_time(pyear, year)) {
            free_time(pyear, pmonth, pweek, pdayw, pdaym, phour);
            continue;
          }

          if (!match_time(pmonth, month)) {
            free_time(pyear, pmonth, pweek, pdayw, pdaym, phour);
            continue;
          }

          if (!match_time(pweek, week)) {
            free_time(pyear, pmonth, pweek, pdayw, pdaym, phour);
            continue;
          }

          if (!match_time(pdayw, dayw)) {
            free_time(pyear, pmonth, pweek, pdayw, pdaym, phour);
            continue;
          }

          if (!match_time(pdaym, daym)) {
            free_time(pyear, pmonth, pweek, pdayw, pdaym, phour);
            continue;
          }

          if (!match_time(phour, hour)) {
            free_time(pyear, pmonth, pweek, pdayw, pdaym, phour);
            continue;
          }

          free_time(pyear, pmonth, pweek, pdayw, pdaym, phour);

          mprog_driver(mprg->comlist, list, NULL, NULL, NULL, NULL);
        }
      }
    }
  }

  return;
}

int match_time(struct number_list_type *list, int date_segment)
{
  int match;

  match = 0;
  while (list && !match) {
    if (list->number == -1)
      match = 1;
    if (list->number == date_segment)
      match = 1;
    list = list->next;
  }

  return match;

}

int scan_time(char* data, struct number_list_type **yearlist, struct number_list_type **monthlist, struct number_list_type **weeklist, struct number_list_type **daywlist, struct number_list_type **daymlist, struct number_list_type **hourlist)
{
  char* p;
  int number, last, done, i;
  struct number_list_type *temp;

  if (data == NULL)
    return -2;

  p = data;

  while (isspace(*p))
    p++;

  /* parse the year */

  done = 0;
  while (!done) {
    if (!(*p)) {
      return -2;
    }

    switch (*p) {
      case '*':
      case '-':
        number = -1;
        done = 1;
        while (*p && (*p != '/'))
          p++;
        break;

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        sscanf(p, "%d", &number);
        while ((*p) && (*p != '.') && (*p != ',') && (*p != '/'))
          p++;
        if (!(*p)) {
          return -2;
        }
        break;

      default:
        return -2;
    }

    temp = (struct number_list_type*) malloc(sizeof(struct number_list_type));

    temp->number = number;
    temp->next = *yearlist;
    *yearlist = temp;

    if (*p == ',')
      p++;
    else if ((p[0] == '.') && (p[1] == '.')) {
      p = &(p[2]);
      if (isdigit(*p))
        sscanf(p, "%d", &last);
      else {
        return -2;
      }

      for (i = number + 1; i <= last; i++) {
        temp = (struct number_list_type*) malloc(sizeof(struct number_list_type));

        temp->number = i;
        temp->next = *yearlist;
        *yearlist = temp;
      }

      while (*p && (*p != ',') && (*p != '/'))
        p++;

      if (!(*p)) {
        return -2;
      } else if (*p == '/') {
        done = 1;
        p++;
      } else {
        p++;
      }
    } else if (*p == '/') {
      done = 1;
      p++;
    } else {
      return -2;
    }
  }

  /* parse the month */

  done = 0;
  while (!done) {
    if (!(*p)) {
      return -2;
    }

    switch (*p) {
      case '*':
      case '-':
        number = -1;
        done = 1;
        while (*p && (*p != '/'))
          p++;
        break;

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        sscanf(p, "%d", &number);
        while ((*p) && (*p != '.') && (*p != ',') && (*p != '/'))
          p++;
        if (!(*p)) {
          return -2;
        }
        break;

      default:
        return -2;
    }

    temp = (struct number_list_type*) malloc(sizeof(struct number_list_type));

    temp->number = number;
    temp->next = *monthlist;
    *monthlist = temp;

    if (*p == ',')
      p++;
    else if ((p[0] == '.') && (p[1] == '.')) {
      p = &(p[2]);
      if (isdigit(*p))
        sscanf(p, "%d", &last);
      else {
        return -2;
      }

      for (i = number + 1; i <= last; i++) {
        temp = (struct number_list_type*) malloc(sizeof(struct number_list_type));

        temp->number = i;
        temp->next = *monthlist;
        *monthlist = temp;
      }

      while (*p && (*p != ',') && (*p != '/'))
        p++;

      if (!(*p)) {
        return -2;
      } else if (*p == '/') {
        done = 1;
        p++;
      } else {
        p++;
      }
    } else if (*p == '/') {
      done = 1;
      p++;
    } else {
      return -2;
    }
  }

  /* parse the week */

  done = 0;
  while (!done) {
    if (!(*p)) {
      return -2;
    }
    switch (*p) {
      case '*':
      case '-':
        number = -1;
        done = 1;
        while (*p && (*p != '/'))
          p++;
        break;

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        sscanf(p, "%d", &number);
        while ((*p) && (*p != '.') && (*p != ',') && (*p != '/'))
          p++;
        if (!(*p)) {
          return -2;
        }
        break;

      default:
        return -2;
    }

    temp = (struct number_list_type*) malloc(sizeof(struct number_list_type));

    temp->number = number;
    temp->next = *weeklist;
    *weeklist = temp;

    if (*p == ',')
      p++;
    else if ((p[0] == '.') && (p[1] == '.')) {
      p = &(p[2]);
      if (isdigit(*p))
        sscanf(p, "%d", &last);
      else {
        return -2;
      }

      for (i = number + 1; i <= last; i++) {
        temp = (struct number_list_type*) malloc(sizeof(struct number_list_type));

        temp->number = i;
        temp->next = *weeklist;
        *weeklist = temp;
      }

      while (*p && (*p != ',') && (*p != '/'))
        p++;

      if (!(*p)) {
        return -2;
      } else if (*p == '/') {
        done = 1;
        p++;
      } else {
        p++;
      }
    } else if (*p == '/') {
      done = 1;
      p++;
    } else {
      return -2;
    }
  }

  /* parse the day of the week */

  done = 0;
  while (!done) {
    if (!(*p)) {
      return -2;
    }

    switch (*p) {
      case '*':
      case '-':
        number = -1;
        done = 1;
        while (*p && (*p != '/'))
          p++;
        break;

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        sscanf(p, "%d", &number);
        while ((*p) && (*p != '.') && (*p != ',') && (*p != '/'))
          p++;
        if (!(*p)) {
          return -2;
        }
        break;

      default:
        return -2;
    }

    temp = (struct number_list_type*) malloc(sizeof(struct number_list_type));

    temp->number = number;
    temp->next = *daywlist;
    *daywlist = temp;

    if (*p == ',')
      p++;
    else if ((p[0] == '.') && (p[1] == '.')) {
      p = &(p[2]);
      if (isdigit(*p))
        sscanf(p, "%d", &last);
      else {
        return -2;
      }

      for (i = number + 1; i <= last; i++) {
        temp = (struct number_list_type*) malloc(sizeof(struct number_list_type));

        temp->number = i;
        temp->next = *daywlist;
        *daywlist = temp;
      }

      while (*p && (*p != ',') && (*p != '/'))
        p++;

      if (!(*p)) {
        return -2;
      } else if (*p == '/') {
        done = 1;
        p++;
      } else {
        p++;
      }
    } else if (*p == '/') {
      done = 1;
      p++;
    } else {
      return -2;
    }

  }

  /* parse the day of the month */

  done = 0;
  while (!done) {
    if (!(*p)) {
      return -2;
    }
    switch (*p) {
      case '*':
      case '-':
        number = -1;
        done = 1;
        while (*p && (*p != '/'))
          p++;
        break;

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        sscanf(p, "%d", &number);
        while ((*p) && (*p != '.') && (*p != ',') && (*p != '/'))
          p++;
        if (!(*p)) {
          return -2;
        }
        break;

      default:
        return -2;
    }

    temp = (struct number_list_type*) malloc(sizeof(struct number_list_type));

    temp->number = number;
    temp->next = *daymlist;
    *daymlist = temp;

    if (*p == ',')
      p++;
    else if ((p[0] == '.') && (p[1] == '.')) {
      p = &(p[2]);
      if (isdigit(*p))
        sscanf(p, "%d", &last);
      else {
        return -2;
      }

      for (i = number + 1; i <= last; i++) {
        temp = (struct number_list_type*) malloc(sizeof(struct number_list_type));

        temp->number = i;
        temp->next = *daymlist;
        *daymlist = temp;
      }

      while (*p && (*p != ',') && (*p != '/'))
        p++;

      if (!(*p)) {
        return -2;
      } else if (*p == '/') {
        done = 1;
        p++;
      } else {
        p++;
      }
    } else if (*p == '/') {
      done = 1;
      p++;
    } else {
      return -2;
    }

  }

  /* parse the hour */

  done = 0;
  while (!done) {
    if (!(*p)) {
      return -2;
    }
    switch (*p) {
      case '*':
      case '-':
        number = -1;
        done = 1;
        while (*p && !isspace(*p))
          p++;
        break;

      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        sscanf(p, "%d", &number);
        while ((*p) && (*p != '.') && (*p != ',') && !(isspace(*p)))
          p++;
        break;

      default:
        return -2;
    }

    temp = (struct number_list_type*) malloc(sizeof(struct number_list_type));

    temp->number = number;
    temp->next = *hourlist;
    *hourlist = temp;

    if ((p == (void*) NULL) || (*p == '\0') || isspace(*p))
      done = 1;
    else if (*p == ',')
      p++;
    else if ((p[0] == '.') && (p[1] == '.')) {
      p = &(p[2]);
      if (isdigit(*p)) {
        sscanf(p, "%d", &last);
      } else {
        return -2;
      }
      i = 0;

      for (i = number + 1; i <= last; i++) {
        temp = (struct number_list_type*) malloc(sizeof(struct number_list_type));

        temp->number = i;
        temp->next = *hourlist;
        *hourlist = temp;
      }
      while (*p && (*p != ',') && !isspace(*p))
        p++;

      if (*p) {
        p++;
      } else {
        done = 1;
      }
    } else {
      return -2;
    }

  }

  return 0;
}

void free_time(struct number_list_type *yearlist, struct number_list_type *monthlist, struct number_list_type *weeklist, struct number_list_type *daywlist, struct number_list_type *daymlist, struct number_list_type *hourlist)
{

  struct number_list_type* next;

  while (yearlist) {
    next = yearlist->next;
    free(yearlist);
    yearlist = next;
  }

  while (monthlist) {
    next = monthlist->next;
    free(monthlist);
    monthlist = next;
  }

  while (weeklist) {
    next = weeklist->next;
    free(weeklist);
    weeklist = next;
  }

  while (daywlist) {
    next = daywlist->next;
    free(daywlist);
    daywlist = next;
  }

  while (daymlist) {
    next = daymlist->next;
    free(daymlist);
    daymlist = next;
  }

  while (hourlist) {
    next = hourlist->next;
    free(hourlist);
    hourlist = next;
  }

}

int istime(char* time_string)

{

  char error[64];

  int year, month, week, dayw, daym, hour;
  struct number_list_type *pyear, *pmonth, *pweek, *pdayw, *pdaym, *phour;

  year = time_info.year;
  month = time_info.month + 1;
  daym = time_info.day + 1;
  hour = time_info.hours;

  dayw = 1 + ((daym - 1) % 7);
  week = 1 + (daym / 7);

  pyear = NULL;
  pmonth = NULL;
  pweek = NULL;
  pdayw = NULL;
  pdaym = NULL;
  phour = NULL;

  if (scan_time(time_string, &pyear, &pmonth, &pweek, &pdayw, &pdaym, &phour) == -2) {
    sprintf(error, "istime error: bad time format if check on %s", time_string);
    mudlog(error, 'E', COM_IMMORT, TRUE);
    return 0;
  }

  if (!match_time(pyear, year)) {
    free_time(pyear, pmonth, pweek, pdayw, pdaym, phour);
    return 0;
  }

  if (!match_time(pmonth, month)) {
    free_time(pyear, pmonth, pweek, pdayw, pdaym, phour);
    return 0;
  }

  if (!match_time(pweek, week)) {
    free_time(pyear, pmonth, pweek, pdayw, pdaym, phour);
    return 0;
  }

  if (!match_time(pdayw, dayw)) {
    free_time(pyear, pmonth, pweek, pdayw, pdaym, phour);
    return 0;
  }

  if (!match_time(pdaym, daym)) {
    free_time(pyear, pmonth, pweek, pdayw, pdaym, phour);
    return 0;
  }

  if (!match_time(phour, hour)) {
    free_time(pyear, pmonth, pweek, pdayw, pdaym, phour);
    return 0;
  }

  free_time(pyear, pmonth, pweek, pdayw, pdaym, phour);

  return 1;
}

void handle_mpdelay(char* delay, char* cmnd, struct char_data* mob, struct char_data* actor, struct obj_data* obj, void* vo, struct char_data* rndm)
{

  struct delayed_mprog_type *temp;

  temp = (struct delayed_mprog_type*) malloc(sizeof(struct delayed_mprog_type));

  temp->remaining_cmnds = (char*) malloc(strlen(cmnd) + 1);

  if (delayed_mprog) {
    temp->next = delayed_mprog;
    temp->prev = NULL;

    delayed_mprog->prev = temp;

    delayed_mprog = temp;
  } else {
    delayed_mprog = temp;
    temp->next = NULL;
    temp->prev = NULL;
  }

  temp->mob = mob;
  temp->actor = actor;
  temp->obj = obj;
  temp->vo = vo;
  temp->rndm = rndm;

  /* brr */

  /* already know it's an mpdelay, move on to the argument */

  if (*buf != '\0') {
    temp->delay = atoi(delay);
  } else {
    temp->delay = 5;
  }

  strcpy(temp->remaining_cmnds, cmnd);

}

/* Function used to remove delayed mob_progs from the delayed list if the
 mob has been extracted.  Called from extract_char in handler.c
 */

void mob_delay_purge(struct char_data* ch)
{
  struct delayed_mprog_type *temp_mprog;

  if (!ch) {
    mudlog("Error in removing delayed mob_progs, nonexistant char", 'E', COM_IMMORT, TRUE);
    return;
  }

  for (temp_mprog = delayed_mprog; temp_mprog; temp_mprog = temp_mprog->next)
    if (temp_mprog->mob == ch)
      extract_mobprog(temp_mprog);
}

/* Function used to physically extract delayed mob_progs from the list.  Called
 from mob_delay_purge, and mprog_pulse
 */

void extract_mobprog(struct delayed_mprog_type* mprog)
{

  if (mprog == NULL) {
    mudlog("tried to remove a non-existant mob prog", 'E', COM_IMMORT, TRUE);
    return;
  }

  if (mprog == delayed_mprog)
    delayed_mprog = mprog->next;
  if (mprog->prev)
    mprog->prev->next = mprog->next;
  if (mprog->next)
    mprog->next->prev = mprog->prev;

  free(mprog->remaining_cmnds);
  free(mprog);
}
