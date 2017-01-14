/***************************************************************************
 * MOBProgram ported for CircleMUD 3.0 by Mattias Larsson		           *
 * Traveller@AnotherWorld (ml@eniac.campus.luth.se 4000) 		           *
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
 *  such installation can be found in INSTALL.  Enjoy........    N'Atas-Ha *
 ***************************************************************************/

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "structs.h"
#include "db.h"
#include "utils.h"
#include "handler.h"
#include "interpreter.h"
#include "comm.h"

extern int mob_idnum; /* mob idnums are negative */
extern int top_of_world;
extern struct index_data *mob_index;
extern struct room_data *world;
extern struct char_data *character_list;
extern struct index_data *obj_index;
extern struct descriptor_data *descriptor_list;

extern struct index_data *get_mob_index(int vnum);
extern struct index_data *get_obj_index(int vnum);

/* This would have been needed if do_mpdelay could
 * have been implemented here. I believe I have left
 * sufficient documentation as to why it has not.
 * look around, you may find it :) -- brr
 */

extern sh_int find_target_room(struct char_data * ch, char *rawroomstr);

extern void handle_mpdelay(char* delay, char* cmnd, struct char_data* mob, struct char_data* actor, struct obj_data* obj, void* vo, struct char_data* rndm);

#define bug(x, y) { sprintf(buf2, (x), (y)); stderr_log(buf2); }

/*
 * Local functions.
 */

char * mprog_type_to_name(int type);

void mprog_mppurge(char* argument, char* cmnd, struct char_data* ch, struct char_data *actor, struct obj_data* obj, void* vo, struct char_data* rndm);

void mprog_mpextract(struct char_data* mob);

/* This routine transfers between alpha and numeric forms of the
 *  mob_prog bitvector types. It allows the words to show up in mpstat to
 *  make it just a hair bit easier to see what a mob should be doing.
 */

char *mprog_type_to_name(int type)
{
  switch (type) {
    case IN_FILE_PROG:
      return "in_file_prog";
    case ACT_PROG:
      return "act_prog";
    case SPEECH_PROG:
      return "speech_prog";
    case RAND_PROG:
      return "rand_prog";
    case FIGHT_PROG:
      return "fight_prog";
    case HITPRCNT_PROG:
      return "hitprcnt_prog";
    case DEATH_PROG:
      return "death_prog";
    case ENTRY_PROG:
      return "entry_prog";
    case GREET_PROG:
      return "greet_prog";
    case ALL_GREET_PROG:
      return "all_greet_prog";
    case GIVE_PROG:
      return "give_prog";
    case BRIBE_PROG:
      return "bribe_prog";
    case SHOUT_PROG:
      return "shout_prog";
    case HOLLER_PROG:
      return "holler_prog";
    case TELL_PROG:
      return "tell_prog";
    case TIME_PROG:
      return "time_prog";
    case ASK_PROG:
      return "ask_prog";
    default:
      return "ERROR_PROG";
  }
}

/* string prefix routine */

bool str_prefix(const char *astr, const char *bstr)
{
  if (!astr) {
    stderr_log("Strn_cmp: null astr.");
    return TRUE;
  }
  if (!bstr) {
    stderr_log("Strn_cmp: null astr.");
    return TRUE;
  }
  for (; *astr; astr++, bstr++) {
    if (LOWER(*astr) != LOWER(*bstr))
      return TRUE;
  }
  return FALSE;
}

/* logs a message on the Q channel */

ACMD (do_mplog)
{
  char *p;

  if (!IS_NPC(ch)) {
    send_to_char("That command is for mobs only. :)\r\n", ch);
    return;
  }

  p = argument;

  while (isspace(*p))
    p++;

  mudlog(p, 'Q', COM_QUEST, TRUE);

}

/* sets up the necessary info so that a mobprog
 * may re-enter its execution after the delay
 * has been effected.
 */

/* do_mpdelay SHOULD have gone here; however, the ACMD macro
 * did not contain sufficient data in it's argument list, to
 * support this function. mpdelay is being implemented in
 * mobprog.c, and is being handled as a special case in
 * the mprog_process_cmnd function. We are adding an empty
 * implementation here just to keep form. The function that
 * will actually be used is handle_mpdelay.
 */

ACMD(do_mpdelay)
{
  if (!IS_NPC(ch)) {
    send_to_char("You're not a mob.... technically.\r\n", ch);
    return;
  }

  mudlog("This error needs to be reported immediately: mpdelay failed", 'E', COM_IMMORT, TRUE);
}

/* prints the argument to all the rooms aroud the mobile */

ACMD(do_mpasound)
{
  char *p;
  room_num was_in_room;
  int door;

  if (!ch) {
    bug("Mpasound - no character %d", 1);
    return;
  }

  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  if (argument[0] == '\0') {
    bug("Mpasound - No argument: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }
  p = argument;

  while (isspace(*p))
    p++;

  was_in_room = ch->in_room;
  for (door = 0; door <= 5; door++) {
    struct room_direction_data *pexit;

    if ((pexit = world[was_in_room].dir_option[door]) != NULL && pexit->to_room != NOWHERE && pexit->to_room != was_in_room) {
      ch->in_room = pexit->to_room;
      MOBTrigger = FALSE;
      act(p, FALSE, ch, NULL, NULL, TO_ROOM);
    }
  }

  ch->in_room = was_in_room;
  return;

}

/* lets the mobile kill any player or mobile without murder*/

ACMD(do_mprawkill)
{
  char arg[MAX_INPUT_LENGTH];
  void raw_kill(struct char_data *vict, struct char_data *ch);
  struct char_data *victim;

  if (!ch) {
    bug("Mprawkill - no character %d", 1);
    return;
  }
  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  one_argument(argument, arg);

  if (arg[0] == '\0') {
    bug("MprawKill - no argument: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if ((victim = get_char_room_vis(ch, arg)) == NULL) {
    bug("MprawKill - Victim not in room: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if (victim == ch) {
    bug("MprawKill - Bad victim to attack: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) {
    bug("MprawKill - Charmed mob attacking master: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if (ch->char_specials.position == POS_FIGHTING) {
    bug("MprawKill - Already fighting: vnum %d", mob_index[ch->nr].virtual);
    return;
  }

  raw_kill(victim, ch);
  return;
}

ACMD(do_mpkill)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *victim;

  if (!ch) {
    bug("Mpkill - no character %d", 1);
    return;
  }
  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  one_argument(argument, arg);

  if (arg[0] == '\0') {
    bug("MpKill - no argument: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if ((victim = get_char_room_vis(ch, arg)) == NULL) {
    bug("MpKill - Victim not in room: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if (victim == ch) {
    bug("MpKill - Bad victim to attack: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) {
    bug("MpKill - Charmed mob attacking master: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if (ch->char_specials.position == POS_FIGHTING) {
    bug("MpKill - Already fighting: vnum %d", mob_index[ch->nr].virtual);
    return;
  }
  if (!ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL))
    hit(ch, victim, -1);
  return;
}

ACMD(do_mphunt)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *victim;
  int i = 0;

  if (!ch) {
    bug("Mphunt - no character %d", 1);
    return;
  }
  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  one_argument(argument, arg);

  if (arg[0] == '\0') {
    bug("Mphunt - no argument: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if ((victim = get_char_vis(ch, arg)) == NULL) {
    bug("Mphunt - Victim not found/not visible: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if (victim == ch) {
    /* we don't care, just return. */
    /*bug("Mphunt - Bad victim to hunt: vnum %d.", mob_index[ch->nr].virtual);*/
    return;
  }
  for (i = 0; i < 10; i++) {
    if (victim->player_specials->hunters[i] == NULL && victim->player_specials->hunters[i] != ch) {
      victim->player_specials->hunters[i] = ch;
      victim->player_specials->huntstate[i] = 2;
      break;
    }
  }
  HUNTINGRM(ch) = -1;
  HUNTING(ch) = GET_IDNUM(victim);
}

ACMD(do_mphuntrm)
{
  char arg[MAX_INPUT_LENGTH];
  sh_int location;

  if (!ch) {
    bug("Mphuntrm - no character %d", 1);
    return;
  }

  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  one_argument(argument, arg);
  if (arg[0] == '\0') {
    bug("Mphuntrm - No argument: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if ((location = find_target_room(ch, arg)) < 0) {
    bug("Mphunt - No such location: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  HUNTING(ch) = 0;
  HUNTINGRM(ch) = location;
}

/* lets the mobile destroy an object in its inventory
 it can also destroy a worn object and it can destroy
 items using all.xxxxx or just plain all of them */

ACMD(do_mpjunk)
{
  char arg[MAX_INPUT_LENGTH];
  int pos;
  struct obj_data *obj;
  struct obj_data *obj_next;

  if (!ch) {
    bug("Mpjunk - no character %d", 1);
    return;
  }

  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  one_argument(argument, arg);

  if (arg[0] == '\0') {
    bug("Mpjunk - No argument: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if (str_cmp(arg, "all") && str_prefix("all.", arg)) {
    if ((obj = get_object_in_equip_vis(ch, arg, ch->equipment, &pos)) != NULL) {
      unequip_char(ch, pos);
      extract_obj(obj);
      return;
    }
    if ((obj = get_obj_in_list_vis(ch, arg, ch->carrying)) != NULL)
      extract_obj(obj);
    return;
  } else {
    for (obj = ch->carrying; obj != NULL; obj = obj_next) {
      obj_next = obj->next_content;
      if (arg[3] == '\0' || isname(arg + 4, obj->name)) {
        extract_obj(obj);
      }
    }
    while ((obj = get_object_in_equip_vis(ch, arg, ch->equipment, &pos)) != NULL) {
      unequip_char(ch, pos);
      extract_obj(obj);
    }
  }
  return;
}

/* prints the message to everyone in the room other than the mob and victim */

ACMD(do_mpechoaround)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *victim;
  char *p;

  if (!ch) {
    bug("Mpechoaround - no character %d", 1);
    return;
  }

  if ((!IS_NPC(ch) && !COM_FLAGGED(ch, COM_MOB)) || IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  p = one_argument(argument, arg);
  while (isspace(*p))
    p++; /* skip over leading space */

  if (arg[0] == '\0') {
    bug("Mpechoaround - No argument:  vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if (!(victim = get_char_room_vis(ch, arg))) {
    bug("Mpechoaround - victim does not exist: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  delete_doubledollar(p);
  act(p, FALSE, ch, NULL, victim, TO_NOTVICT);
  return;
}

/* prints the message to only the victim */

ACMD(do_mpechoat)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *victim;
  char *p;

  if (!ch) {
    bug("Mpechoat - no character %d", 1);
    return;
  }

  if ((!IS_NPC(ch) && !COM_FLAGGED(ch, COM_MOB)) || IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  p = one_argument(argument, arg);
  while (isspace(*p))
    p++; /* skip over leading space */

  if (arg[0] == '\0') {
    bug("Mpechoat - No argument:  vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if (!(victim = get_char_room_vis(ch, arg))) {
    bug("Mpechoat - victim does not exist: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  delete_doubledollar(p);
  act(p, FALSE, ch, NULL, victim, TO_VICT);
  return;
}

/* prints the message to the room at large */

ACMD(do_mpecho)
{
  char *p;

  if (!ch) {
    bug("Mpecho - no character %d", 1);
    return;
  }

  if ((!IS_NPC(ch) && !COM_FLAGGED(ch, COM_MOB)) || IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  if (argument[0] == '\0') {
    bug("Mpecho - called w/o argument: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }
  p = argument;
  delete_doubledollar(p);
  while (isspace(*p))
    p++;

  act(p, FALSE, ch, NULL, NULL, TO_ROOM);
  return;
}

/* lets the mobile load an item or mobile.  All items
 are loaded into inventory.  you can specify a level with
 the load object portion as well. */

ACMD(do_mpmload)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *victim;

  if (!ch) {
    bug("Mpmload - no character %d", 1);
    return;
  }

  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  one_argument(argument, arg);

  if (arg[0] == '\0' || !is_number(arg)) {
    bug("Mpmload - Bad vnum as arg: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if (get_mob_index(atoi(arg)) == NULL) {
    bug("Mpmload - Bad mob vnum: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  victim = read_mobile(atoi(arg), VIRTUAL);
  GET_IDNUM(victim) = mob_idnum;
  mob_idnum--;
  char_to_room(victim, ch->in_room);
  return;
}

ACMD(do_mpoload)
{
  char arg1[MAX_INPUT_LENGTH];
  struct obj_data *obj;

  if (!ch) {
    bug("Mpoload - no character %d", 1);
    return;
  }

  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  argument = one_argument(argument, arg1);

  if (arg1[0] == '\0' || !is_number(arg1)) {
    bug("Mpoload - Bad syntax: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if (get_obj_index(atoi(arg1)) == NULL) {
    bug("Mpoload - Bad vnum arg: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  obj = read_object(atoi(arg1), VIRTUAL);
  if (obj == NULL)
    return;
  if (obj_index[obj->item_number].qic != NULL) {
    bug("Mpoload - trying to load qic: mob #%d.", mob_index[ch->nr].virtual);
    return;
  }
  if (CAN_WEAR(obj, ITEM_WEAR_TAKE)) {
    obj_to_char(obj, ch);
  } else {
    obj_to_room(obj, ch->in_room);
  }

  return;
}

/* lets the mobile purge all objects and other npcs in the room,
 or purge a specified object or mob in the room.  It can purge
 itself, but this had best be the last command in the MOBprogram
 otherwise ugly stuff will happen */

ACMD(do_mppurge)
{
  struct char_data *vict, *next_v;
  struct obj_data *obj, *next_o;

  if ((!IS_NPC(ch) && !COM_FLAGGED(ch, COM_MOB)) || IS_AFFECTED(ch, AFF_CHARM))
    return;

  one_argument(argument, buf);

  if (*buf) { /* argument supplied. destroy single object
   * or char */
    if ((vict = get_char_room_vis(ch, buf))) {
      if (!IS_NPC(vict) || (vict == ch)) {
        return;
      }
      extract_char(vict, 1);
    } else if ((obj = get_obj_in_list_vis(ch, buf, world[ch->in_room].contents))) {
      extract_obj(obj);
    } else {
      send_to_char("Nothing here by that name.\r\n", ch);
      return;
    }

  } else { /* no argument. clean out the room */
    for (vict = world[ch->in_room].people; vict && ch->in_room >= 0 && ch->in_room <= top_of_world; vict = next_v) {
      if (!vict)
        break;
      next_v = vict->next_in_room;
      if (IS_NPC(vict) && (vict != ch))
        extract_char(vict, 1);
    }

    for (obj = world[ch->in_room].contents; obj && ch->in_room >= 0 && ch->in_room <= top_of_world; obj = next_o) {
      if (!obj)
        break;
      next_o = obj->next_content;
      extract_obj(obj);
    }
  }
}

void mprog_mpextract(struct char_data* mob)
{
  if (!(mob)) {
    mudlog("ERROR: Attempting to extract non-existant mob.", 'E', COM_IMMORT, TRUE);
    return;
  } else
    extract_char(mob, 0);
}

void mprog_mppurge(char* argument, char* cmnd, struct char_data* ch, struct char_data *actor, struct obj_data* object, void* vo, struct char_data* rndm)
{
  struct char_data *victim;
  struct obj_data *obj;

  if (!ch) {
    bug("Mppurge - no character %d", 1);
    return;
  }

  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  one_argument(argument, arg);

  if (arg[0] == '\0') {
    /* 'purge' */
    struct char_data *vnext;
    struct obj_data *obj_next;

    for (victim = world[ch->in_room].people; victim != NULL; victim = vnext) {
      vnext = victim->next_in_room;
      if (IS_NPC(victim) && victim != ch)
        extract_char(victim, 0);
    }

    for (obj = world[ch->in_room].contents; obj != NULL; obj = obj_next) {
      obj_next = obj->next_content;
      extract_obj(obj);
    }

    return;
  }

  if (!(victim = get_char_room_vis(ch, arg))) {
    if ((obj = get_obj_vis(ch, arg))) {
      extract_obj(obj);
    } else {
      bug("Mppurge - Bad argument: vnum %d.", mob_index[ch->nr].virtual);
    }
    return;
  }

  if (!IS_NPC(victim)) {
    bug("Mppurge - Purging a PC: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if (victim == ch) {
    mudlog("Extracting mob with mppurge self", 'Q', COM_QUEST, FALSE);

    strcpy(cmnd, "mpextract\n");

    handle_mpdelay("20", cmnd, ch, actor, object, vo, rndm);

    char_from_room(ch);
    char_to_room(ch, 0);
    return;
  }

  extract_char(victim, 0);
  return;
}

/* lets the mobile goto any location it wishes that is not private */

ACMD(do_mpgoto)
{
  char arg[MAX_INPUT_LENGTH];
  sh_int location;

  if (!ch) {
    bug("Mpgoto - no character %d", 1);
    return;
  }

  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  one_argument(argument, arg);
  if (arg[0] == '\0') {
    bug("Mpgoto - No argument: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if ((location = find_target_room(ch, arg)) < 0) {
    bug("Mpgoto - No such location: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if (FIGHTING(ch) != NULL)
    stop_fighting(ch);

  char_from_room(ch);
  char_to_room(ch, location);

  return;
}

/* lets the mobile do a command at another location. Very useful */

ACMD(do_mpat)
{
  char arg[MAX_INPUT_LENGTH];
  sh_int location;
  sh_int original;
  /*    struct char_data       *wch; */

  if (!ch) {
    bug("Mpat - no character %d", 1);
    return;
  }

  if (!IS_NPC(ch) || IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  argument = one_argument(argument, arg);

  if (arg[0] == '\0' || argument[0] == '\0') {
    bug("Mpat - Bad argument: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if ((location = find_target_room(ch, arg)) < 0) {
    bug("Mpat - No such location: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  original = ch->in_room;
  char_from_room(ch);
  char_to_room(ch, location);
  command_interpreter(ch, argument);

  /*
   * See if 'ch' still exists before continuing!
   * Handles 'at XXXX quit' case.
   */
  if (ch->in_room == location) {
    char_from_room(ch);
    char_to_room(ch, original);
  }

  return;
}

/* lets the mobile transfer people.  the all argument transfers
 everyone in the current room to the specified location */

ACMD(do_mptransfer)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  sh_int location;
  struct char_data *victim, *to;

  if (!ch) {
    bug("Mptransfer - no character %d", 1);
    return;
  }

  if ((!IS_NPC(ch) && !COM_FLAGGED(ch, COM_MOB)) || IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }
  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);

  if (arg1[0] == '\0') {
    bug("Mptransfer - Bad syntax: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if (!str_cmp(arg1, "all")) {
    to = character_list;

    for (; to != NULL; to = to->next) {
      if (arg2[0] == '\0') {
        location = ch->in_room;
      } else {
        if ((location = find_target_room(ch, arg2)) < 0) {
          bug("Mptransfer - no such location: vnum %d.", mob_index[ch->nr].virtual);
          return;
        }
        if (IS_SET (world[location].room_flags, ROOM_PRIVATE)) {
          bug("Mptransfer - Private room: vnum %d.", mob_index[ch->nr].virtual);
          return;
        }
      }

      if ((victim = to) == NULL) {
        bug("Mptransfer - no such person: vnum %d.", mob_index[ch->nr].virtual);
        continue;
      }

      if (victim->in_room != ch->in_room) {
        continue;
      }

      if (!CAN_SEE(ch, victim)) {
        continue;
      }
      if (victim == ch) {
        continue;
      }

      if (FIGHTING(victim) != NULL)
        stop_fighting(victim);

      if (GET_DRAGGING(victim)) {
        sprintf(buf, "%s drags $p along.", GET_NAME(victim));
        act(buf, TRUE, 0, GET_DRAGGING(victim), 0, TO_ROOM);
      }
      char_from_room(victim);
      char_to_room(victim, location);
      if (GET_DRAGGING(victim)) {
        obj_from_room(GET_DRAGGING(victim));
        obj_to_room(GET_DRAGGING(victim), victim->in_room);
        sprintf(buf, "You drag $p along with you.");
        act(buf, FALSE, victim, GET_DRAGGING(victim), 0, TO_CHAR);
        sprintf(buf, "%s drags $p along with $m.", GET_NAME(victim));
        act(buf, TRUE, victim, GET_DRAGGING(victim), 0, TO_ROOM);
      }

    }
    return;
  }

  /*
   * Thanks to Grodyn for the optional location parameter.
   */
  if (arg2[0] == '\0') {
    location = ch->in_room;
  } else {
    if ((location = find_target_room(ch, arg2)) < 0) {
      bug("Mptransfer - No such location: vnum %d.", mob_index[ch->nr].virtual);
      return;
    }

    if (IS_SET(world[location].room_flags, ROOM_PRIVATE)) {
      bug("Mptransfer - Private room: vnum %d.", mob_index[ch->nr].virtual);
      return;
    }
  }

  if ((victim = get_char_vis(ch, arg1)) == NULL) {
    bug("Mptransfer - No such person: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if (victim->in_room == 0) {
    bug("Mptransfer - Victim in Limbo: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if (FIGHTING(victim) != NULL)
    stop_fighting(victim);

  char_from_room(victim);
  char_to_room(victim, location);

  return;
}

/* Allows mobs to perform training activities without using
 specprocs. This should only be used with implementor approval.
 This will have a security parameter added and will not activate
 the training if the security is incorrect. This security will
 be unique to the mob so that zone designers cannot stat an
 existing mob with mptrain and get a valid train code.

 security = (mob->vnum * 7) - 2

 this will be 100% unique and just enough off the pattern
 that someone clever will not recognize it.

 syntax of mptrain is:
 mptrain 'skill' 'class list' min_skill max_skill min_char_level security

 'skill'      is any valid skill in the MUD
 'class list' is any of the following:

 FIGHTERS  -- any of the general class of warriors
 KNIGHT
 WARRIOR
 RANGER
 BERZERKER
 THIEVES   -- any of the general class of theives
 THIEF
 ASSASSIN
 ROGUE
 BARD      -- BARD is not yet in the game, but will be
 MAGES     -- any of the general class of mages
 WIZARD
 SORCERER
 NECROMANCER
 ENCHANTER
 PRIESTS   -- any of the general class of clerics
 CLERIC
 CRUSADER
 HEALER
 SHAMAN    -- SHAMAN is not yet in the game, but will be
 ALL       -- anyone may be trained (who uses skills)
 -- immos and if a social class is created
 -- will not be able to be trained.

 */
ACMD(do_mptrain)
{

  /*   unsigned long    security; */
  /*   int              max_skill_level,
   min_skill_level,
   min_char_level; */
  char *arg;
  int runner;
  int foundskill;

  arg = argument;

  while (isspace(*arg))
    arg++;

  if (*arg != '\'') {
    bug("Mptrain - bad syntax: skill not enclosed in single quote - mob: %d", mob_index[ch->nr].virtual);
    return;
  }

  /* copy the skill into the skill variable, this will be sent to the
   * do_skillset function, so include the single quote (')
   */

  for (runner = 1; arg[runner] != '\''; runner++) {
    if (arg[runner] == '\0') {
      bug("Mptrain - bad syntax: no ending quote on skill - mob: %d", mob_index[ch->nr].virtual);
      return;
    }

  }

  runner++;
  runner++;

  /* put arg at the beginning of the class list */
  while (isspace(arg[runner]))
    runner++;
  arg = &(arg[runner]);

  /* check to see if the character is of the appropriate class */

  runner = 1;
  foundskill = 0;

  if (*arg != '\'') {
    bug("Mptrain - bad syntax: classes not enclosed in single quote - mob: %d", mob_index[ch->nr].virtual);
    return;
  }

  arg++;

  for (runner = 1; arg[runner] |= '\''; runner++) {
    if (arg[runner] == '\0') {
      bug("Mptrain - bad syntax: no ending quote on skill - mob: %d", mob_index[ch->nr].virtual);
      return;
    }
  }

  while (!foundskill) {
  }
}

/* lets the mobile force someone to do something.  must be mortal level
 and the all argument only affects those in the room with the mobile */

ACMD(do_mpforce)
{
  char arg[MAX_INPUT_LENGTH];

  if (!ch) {
    bug("Mpforce - no character %d", 1);
    return;
  }
  if ((!IS_NPC(ch) && !COM_FLAGGED(ch, COM_MOB)) || IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("Huh?\n\r", ch);
    return;
  }

  argument = one_argument(argument, arg);

  if (arg[0] == '\0' || argument[0] == '\0') {
    bug("Mpforce - Bad syntax: vnum %d.", mob_index[ch->nr].virtual);
    return;
  }

  if (!str_cmp(arg, "room") || !str_cmp(arg, "all")) {
    struct descriptor_data *i;
    struct char_data *vch;

    for (i = descriptor_list; i; i = i->next) {
      if (i->character != ch && !i->connected && i->character->in_room == ch->in_room) {
        vch = i->character;
        if (!COM_FLAGGED(vch, COM_IMMORT) && CAN_SEE(ch, vch)) {
          command_interpreter(vch, argument);
        }
      }
    }
  } else {
    struct char_data *victim;

    if ((victim = get_char_vis(ch, arg)) == NULL) {
      bug("Mpforce - No such victim: vnum %d.", mob_index[ch->nr].virtual);
      return;
    }

    if (victim == ch) {
      bug("Mpforce - Forcing oneself: vnum %d.", mob_index[ch->nr].virtual);
      return;
    }
    if ((GET_LEVEL(victim) >= LVL_IMMORT) && (!IS_NPC(victim))) {
      return;
    }
    command_interpreter(victim, argument);
  }

  return;
}

