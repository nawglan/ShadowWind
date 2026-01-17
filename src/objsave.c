/* ************************************************************************
 *   File: objsave.c                                     Part of CircleMUD *
 *  Usage: loading/saving player objects for rent and crash-save           *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>

#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"
#include "event.h"
#include "spells.h"

/* these factors should be unique integers */
#define RENT_FACTOR       1
#define CRYO_FACTOR       2

/* File version of rent file */
#define RENT_VERSION    3

extern struct room_data *world;
extern struct index_data *obj_index;
extern struct descriptor_data *descriptor_list;
extern struct obj_data *object_list;

/* Extern functions */
void clear_magic_memory(struct char_data *ch);
int find_spell_num(char *name);
int find_skill_num_def(int spellindex);
extern struct spell_info_type *spells;
ACMD(do_tell);
SPECIAL(receptionist);
SPECIAL(cryogenicist);
void parse_pline(char *line, char *field, char *value);
long asciiflag_conv(char *flag);
struct obj_data *unequip_char(struct char_data * ch, int pos);
void weight_change_object(struct obj_data *obj, int amount);

bool doneload = FALSE;

/* Text saving (alias/poofs/whoset) - Modified by Mattias Larsson */

void load_text(struct char_data * ch)
{
  FILE *fil = NULL;
  char fname[MAX_INPUT_LENGTH];
  struct alias *al;
  int i, cnt;

  void free_alias(struct alias * a);

  if (IS_NPC(ch))
    return;

  if (!get_filename(GET_NAME(ch), fname, ETEXT_FILE))
    return;

  if (!(fil = fopen(fname, "r"))) {
    if (errno != ENOENT) { /* if it fails, NOT because of no file */
      safe_snprintf(buf1, MAX_STRING_LENGTH, "SYSERR: READING TEXT FILE %s (5)", fname);
      perror(buf1);
      send_to_char("\r\n***** NOTICE - THERE WAS A PROBLEM READING YOUR ALIAS FILE *****\r\n", ch);
      return;
    }
    return;
  }

  while ((al = GET_ALIASES(ch)) != NULL) { /* get rid of alias multiplying problem */
    GET_ALIASES(ch) = (GET_ALIASES(ch))->next;
    free_alias(al);
  }

  cnt = 200; /* security counter to make memory alloc bombing impossible :) */
  fscanf(fil, "%d\n", &i); /* get first type */
  while (i != -1 && cnt != 0) {
    CREATE(al, struct alias, 1);
    al->type = i;
    fgets(buf, MAX_INPUT_LENGTH, fil);
    buf[strlen(buf) - 1] = '\0';
    al->alias = strdup(buf);
    fgets(buf, MAX_INPUT_LENGTH, fil);
    buf[strlen(buf) - 1] = '\0';
    al->replacement = strdup(buf);
    al->next = ch->player_specials->aliases;
    ch->player_specials->aliases = al;
    fscanf(fil, "%d\n", &i);
    cnt--;
  }
  /* aliases has been loaded */
  if (WHOSPEC(ch)) {
    FREE(WHOSPEC(ch));
  }
  if (POOFIN(ch)) {
    FREE(POOFIN(ch));
  }
  if (POOFOUT(ch)) {
    FREE(POOFOUT(ch));
  }
  if (WHOSTR(ch)) {
    FREE(WHOSTR(ch));
  }
  if (NAMECOLOR(ch)) {
    FREE(NAMECOLOR(ch));
  }

  fgets(buf, MAX_INPUT_LENGTH, fil);
  buf[strlen(buf) - 1] = '\0';
  if (str_cmp("!UNUSED!", buf)) {
    WHOSPEC(ch) = strdup(buf);
  }
  fgets(buf, SMALL_BUFSIZE, fil);
  buf[strlen(buf) - 1] = '\0';
  if (str_cmp("!UNUSED!", buf)) {
    POOFIN(ch) = strdup(buf);
  }
  fgets(buf, SMALL_BUFSIZE, fil);
  buf[strlen(buf) - 1] = '\0';
  if (str_cmp("!UNUSED!", buf)) {
    POOFOUT(ch) = strdup(buf);
  }
  fgets(buf, MAX_INPUT_LENGTH, fil);
  buf[strlen(buf) - 1] = '\0';
  if (str_cmp("!UNUSED!", buf)) {
    WHOSTR(ch) = strdup(buf);
  }
  if (fgets(buf, SMALL_BUFSIZE, fil) > (char*) NULL) {
    buf[strlen(buf) - 1] = '\0';
    if (str_cmp("!UNUSED!", buf)) {
      NAMECOLOR(ch) = strdup(buf);
    }
  }

  if (cnt == 0) {
    send_to_char("Undetermined error when reading your alias file ...\r\n", ch);
    safe_snprintf(buf, MAX_STRING_LENGTH, "ERROR when reading %s's alias file.", GET_NAME(ch));
    mudlog(buf, 'W', COM_IMMORT, TRUE);
    plog(buf, ch, 0);
  }
  fclose(fil);
  return;
}

void save_text(struct char_data * ch)
{
  FILE *fil = NULL;
  char fname[MAX_INPUT_LENGTH];
  struct alias *al;

  if (IS_NPC(ch)) {
    return;
  }

  if (!get_filename(GET_NAME(ch), fname, ETEXT_FILE)) {
    return;
  }

  if (!(fil = fopen(fname, "w"))) {
    return;
  }

  al = ch->player_specials->aliases;
  while (al) {
    fprintf(fil, "%d\n", al->type);
    fprintf(fil, "%s\n", al->alias);
    fprintf(fil, "%s\n", al->replacement);
    al = al->next;
  }
  fprintf(fil, "-1\n"); /* End of alias segment */
  if (WHOSPEC(ch)) {
    fprintf(fil, "%s\n", WHOSPEC(ch));
  } else {
    fprintf(fil, "!UNUSED!\n");
  }
  if (POOFIN(ch)) {
    fprintf(fil, "%s\n", POOFIN(ch));
  } else {
    fprintf(fil, "!UNUSED!\n");
  }
  if (POOFOUT(ch)) {
    fprintf(fil, "%s\n", POOFOUT(ch));
  } else {
    fprintf(fil, "!UNUSED!\n");
  }
  if (WHOSTR(ch)) {
    fprintf(fil, "%s\n", WHOSTR(ch));
  } else {
    fprintf(fil, "!UNUSED!\n");
  }
  if (ch->desc && NAMECOLOR(ch)) {
    fprintf(fil, "%s\n", NAMECOLOR(ch));
  } else {
    fprintf(fil, "!UNUSED!\n");
  }
  fclose(fil);
  return;
}

void Obj_to_store(struct obj_data * obj, int objnum, int obj_pos, FILE * fl)
{
  void olc_print_bitvectors(FILE *f, long bitvector, long max);
  int j;

  fprintf(fl, "-obj_begin-\n");
  fprintf(fl, "-obj_number- %d\n", GET_OBJ_VNUM(obj));
  fprintf(fl, "-obj_tnum- %d\n", objnum);
  fprintf(fl, "-obj_pos- %d\n", obj_pos);
  if (GET_OBJ_VAL(obj, 0)) {
    fprintf(fl, "-obj_value0- %d\n", GET_OBJ_VAL(obj, 0));
  }
  if (GET_OBJ_VAL(obj, 1)) {
    fprintf(fl, "-obj_value1- %d\n", GET_OBJ_VAL(obj, 1));
  }
  if (GET_OBJ_VAL(obj, 2)) {
    fprintf(fl, "-obj_value2- %d\n", GET_OBJ_VAL(obj, 2));
  }
  if (GET_OBJ_VAL(obj, 3)) {
    fprintf(fl, "-obj_value3- %d\n", GET_OBJ_VAL(obj, 3));
  }
  if (GET_OBJ_VAL(obj, 4)) {
    fprintf(fl, "-obj_value4- %d\n", GET_OBJ_VAL(obj, 4));
  }
  if (GET_OBJ_TIMER(obj)) {
    fprintf(fl, "-obj_timer- %d\n", GET_OBJ_TIMER(obj));
  }
  if (GET_OBJ_EXTRA(obj)) {
    fprintf(fl, "-obj_extra_flags- ");
    olc_print_bitvectors(fl, GET_OBJ_EXTRA(obj), NUM_ITEM_FLAGS);
    fprintf(fl, "\n");
  }
  if (GET_OBJ_BITV(obj)) {
    fprintf(fl, "-obj_bitvector- ");
    olc_print_bitvectors(fl, GET_OBJ_BITV(obj), NUM_AFF_FLAGS);
    fprintf(fl, "\n");
  }
  if (GET_OBJ_BITV2(obj)) {
    fprintf(fl, "-obj_bitvector- ");
    olc_print_bitvectors(fl, GET_OBJ_BITV2(obj), NUM_AFF2_FLAGS);
    fprintf(fl, "\n");
  }
  for (j = 0; j < MAX_OBJ_AFFECT; j++) {
    if (obj->affected[j].location) {
      fprintf(fl, "-obj_affect_loc- %d\n", obj->affected[j].location);
      fprintf(fl, "-obj_affect_mod- %d\n", obj->affected[j].modifier);
    }
  }
  switch (GET_OBJ_TYPE(obj)) {
    case ITEM_SPELLBOOK:
      if (GET_OBJ_SPELLLIST(obj)) {
        for (j = 0; j < GET_OBJ_VAL(obj, 0); j++) {
          if (GET_OBJ_SPELLLISTNUM(obj, j)) {
            fprintf(fl, "-obj_spellbook_entry- %s\n", spells[find_skill_num_def(GET_OBJ_SPELLLISTNUM(obj, j))].command);
          }
        }
      }
      break;
    case ITEM_POTION:
    case ITEM_SCROLL:
      if (GET_OBJ_VAL(obj, 1)) {
        fprintf(fl, "-obj_spell1- %s\n", spells[find_skill_num_def(GET_OBJ_VAL(obj, 1))].command);
      }
      if (GET_OBJ_VAL(obj, 2)) {
        fprintf(fl, "-obj_spell2- %s\n", spells[find_skill_num_def(GET_OBJ_VAL(obj, 2))].command);
      }
      if (GET_OBJ_VAL(obj, 3)) {
        fprintf(fl, "-obj_spell3- %s\n", spells[find_skill_num_def(GET_OBJ_VAL(obj, 3))].command);
      }
      break;
    case ITEM_WAND:
    case ITEM_STAFF:
      if (GET_OBJ_VAL(obj, 1)) {
        fprintf(fl, "-obj_spell1- %s\n", spells[find_skill_num_def(GET_OBJ_VAL(obj, 1))].command);
      }
      break;
  }
  fprintf(fl, "-obj_end-\n");
}

int Crash_delete_text(char *name)
{
  char filename[50];
  FILE *fl;

  if (!get_filename(name, filename, ETEXT_FILE)) {
    return 0;
  }
  if (!(fl = fopen(filename, "r"))) {
    if (errno != ENOENT) { /* if it fails but NOT because of no file */
      safe_snprintf(buf1, MAX_STRING_LENGTH, "SYSERR: deleting text file %s (1)", filename);
      perror(buf1);
    }
    return 0;
  }
  fclose(fl);

  if (unlink(filename) < 0) {
    if (errno != ENOENT) { /* if it fails, NOT because of no file */
      safe_snprintf(buf1, MAX_STRING_LENGTH, "SYSERR: deleting text file %s (2)", filename);
      perror(buf1);
    }
  }
  return (1);
}

int Crash_delete_file(char *name)
{
  char filename[50];
  FILE *fl;

  if (!get_filename(name, filename, CRASH_FILE)) {
    return 0;
  }
  if (!(fl = fopen(filename, "r"))) {
    if (errno != ENOENT) { /* if it fails but NOT because of no file */
      safe_snprintf(buf1, MAX_STRING_LENGTH, "SYSERR: deleting crash file %s (1)", filename);
      perror(buf1);
    }
    return 0;
  }
  fclose(fl);

  if (unlink(filename) < 0) {
    if (errno != ENOENT) { /* if it fails, NOT because of no file */
      safe_snprintf(buf1, MAX_STRING_LENGTH, "SYSERR: deleting crash file %s (2)", filename);
      perror(buf1);
    }
  }
  return (1);
}

int Delete_file(char *name, int mode)
{
  char filename[50];
  FILE *fl;

  if (!get_filename(name, filename, mode)) {
    return 0;
  }
  if (!(fl = fopen(filename, "r"))) {
    if (errno != ENOENT) { /* if it fails but NOT because of no file */
      safe_snprintf(buf1, MAX_STRING_LENGTH, "SYSERR: deleting crash file %s (1)", filename);
      perror(buf1);
    }
    return 0;
  }
  fclose(fl);

  if (unlink(filename) < 0) {
    if (errno != ENOENT) { /* if it fails, NOT because of no file */
      safe_snprintf(buf1, MAX_STRING_LENGTH, "SYSERR: deleting crash file %s (2)", filename);
      perror(buf1);
    }
  }
  return (1);
}

int Crash_delete_crashfile(struct char_data * ch)
{
  char fname[MAX_INPUT_LENGTH];
  FILE *fl;
  char input[MAX_INPUT_LENGTH + 1];
  char tag[MAX_INPUT_LENGTH + 1];
  char tag_arguments[MAX_INPUT_LENGTH + 1];
  char *p;
  int val;

  if (!get_filename(GET_NAME(ch), fname, CRASH_FILE)) {
    return 0;
  }
  if (!(fl = fopen(fname, "r"))) {
    if (errno != ENOENT) { /* if it fails, NOT because of no file */
      safe_snprintf(buf1, MAX_STRING_LENGTH, "SYSERR: checking for crash file %s (3)", fname);
      perror(buf1);
    }
    return 0;
  }

  while (get_line(fl, input)) {
    parse_pline(input, tag, tag_arguments);
    while ((p = strrchr(tag_arguments, '\n')) != NULL) {
      *p = '\0';
    }

    val = atoi(tag_arguments);
    switch (tag[0]) {
      case 'r':
      case 'R':
        if (strcasecmp(tag, "rent_type") == 0) {
          fclose(fl);
          if (val == RENT_CRASH) {
            Crash_delete_file(GET_NAME(ch));
          }
          return 1;
        }
        break;
    }
  }
  return 0;
}

int Crash_clean_file(char *name)
{
  char fname[MAX_STRING_LENGTH];
  char input[MAX_INPUT_LENGTH + 1];
  char tag[MAX_INPUT_LENGTH + 1];
  char tag_arguments[MAX_INPUT_LENGTH + 1];
  int val;
  extern int rent_file_timeout, crash_file_timeout;
  FILE *fl;
  time_t rent_time = time(NULL);
  char *p;

  if (!get_filename(name, fname, CRASH_FILE)) {
    return 0;
  }
  if (!(fl = fopen(fname, "r"))) {
    if (errno != ENOENT) { /* if it fails, NOT because of no file */
      safe_snprintf(buf1, MAX_STRING_LENGTH, "SYSERR: OPENING OBJECT FILE %s (4)", fname);
      perror(buf1);
    }
    return 0;
  }

  while (get_line(fl, input)) {
    parse_pline(input, tag, tag_arguments);
    while ((p = strrchr(tag_arguments, '\n')) != NULL) {
      *p = '\0';
    }

    val = atoi(tag_arguments);
    switch (tag[0]) {
      case 'r':
      case 'R':
        if (strcasecmp(tag, "rent_time") == 0)
          rent_time = val;
        else if (strcasecmp(tag, "rent_type") == 0) {
          fclose(fl);
          switch (val) {
            case RENT_CRASH:
            case RENT_FORCED:
            case RENT_TIMEDOUT:
              if (rent_time < (time(NULL) - (crash_file_timeout * SECS_PER_REAL_DAY))) {
                Crash_delete_file(name);
                switch (val) {
                  case RENT_CRASH:
                    safe_snprintf(buf, MAX_STRING_LENGTH, "    Deleting %s's crash file.", name);
                    break;
                  case RENT_FORCED:
                    safe_snprintf(buf, MAX_STRING_LENGTH, "    Deleting %s's forced rent file.", name);
                    break;
                  case RENT_TIMEDOUT:
                    safe_snprintf(buf, MAX_STRING_LENGTH, "    Deleting %s's idlesave file.", name);
                    break;
                }
                stderr_log(buf);
                return 1;
              }
              break;
            case RENT_RENTED:
            case RENT_CAMPED:
              if (rent_time < (time(0) - (rent_file_timeout * SECS_PER_REAL_DAY))) {
                Crash_delete_file(name);
                switch (val) {
                  case RENT_RENTED:
                    safe_snprintf(buf, MAX_STRING_LENGTH, "    Deleting %s's rent file.", name);
                    break;
                  case RENT_CAMPED:
                    safe_snprintf(buf, MAX_STRING_LENGTH, "    Deleting %s's camp file.", name);
                    break;
                }
                stderr_log(buf);
                return 1;
              }
          }
          return 1;
        }
        break;
    }
  }
  return (0);
}

void update_obj_file(void)
{
  extern struct char_data *character_list;
  struct char_data *ch = character_list;

  for (; ch; ch = ch->next) {
    Crash_clean_file(GET_NAME(ch));
  }
  return;
}

void Crash_listrent(struct char_data * ch, char *name)
{
  FILE *fl;
  char fname[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char input[MAX_INPUT_LENGTH + 1];
  char tag[MAX_INPUT_LENGTH + 1];
  char tag_arguments[MAX_INPUT_LENGTH + 1];
  char *p;
  struct obj_data *obj;
  int val = 0;
  time_t rent_time;
  int d;
  int h;
  int m;
  size_t buflen;

  if (!get_filename(name, fname, CRASH_FILE)) {
    return;
  }
  if (!(fl = fopen(fname, "r"))) {
    safe_snprintf(buf, sizeof(buf), "%s has no rent file.\r\n", name);
    send_to_char(buf, ch);
    return;
  }

  buflen = safe_snprintf(buf, sizeof(buf), "%s\r\n", fname);
  while (get_line(fl, input)) {
    parse_pline(input, tag, tag_arguments);
    while ((p = strrchr(tag_arguments, '\n')) != NULL) {
      *p = '\0';
    }

    val = atoi(tag_arguments);
    switch (tag[0]) {
      case 'o':
      case 'O':
        if (strcasecmp(tag, "obj_number") == 0) {
          if (real_object(val) > -1) {
            obj = read_object_q(val, VIRTUAL);
            buflen += safe_snprintf(buf + buflen, sizeof(buf) - buflen, " [%5d] %-20s\r\n", val, obj->short_description);
            extract_obj_q(obj);
          }
        }
        break;
      case 'r':
      case 'R':
        if (strcasecmp(tag, "rent_version") == 0) {
          buflen += safe_snprintf(buf + buflen, sizeof(buf) - buflen, "Version: %d\r\n", val);
        } else if (strcasecmp(tag, "rent_time") == 0) {
          rent_time = time(0) - val;
          d = rent_time / 86400;
          h = (rent_time / 3600) % 24;
          m = (rent_time / 60) % 60;
          buflen += safe_snprintf(buf + buflen, sizeof(buf) - buflen, "Rented %d day%s, %d hour%s, %d minute%s ago.\r\n", d, ((d == 1) ? "" : "s"), h, ((h == 1) ? "" : "s"), m, ((m == 1) ? "" : "s"));
        } else if (strcasecmp(tag, "rent_type") == 0)
          switch (val) {
            case RENT_RENTED:
              buflen += safe_snprintf(buf + buflen, sizeof(buf) - buflen, "Type: Rent\r\n");
              break;
            case RENT_CRASH:
              buflen += safe_snprintf(buf + buflen, sizeof(buf) - buflen, "Type: Crash\r\n");
              break;
            case RENT_CRYO:
              buflen += safe_snprintf(buf + buflen, sizeof(buf) - buflen, "Type: Cryo\r\n");
              break;
            case RENT_TIMEDOUT:
            case RENT_FORCED:
              buflen += safe_snprintf(buf + buflen, sizeof(buf) - buflen, "Type: TimedOut\r\n");
              break;
            case RENT_CAMPED:
              buflen += safe_snprintf(buf + buflen, sizeof(buf) - buflen, "Type: Camped\r\n");
              break;
            default:
              buflen += safe_snprintf(buf + buflen, sizeof(buf) - buflen, "Type: Undef\r\n");
              break;
          }
        break;
    }
  }
  page_string(ch->desc, buf, 1);
  fclose(fl);
}

/* Modified by Rasmus Bertelsen */
/* return values:
 0 - successful load, keep char in rent room.
 1 - load failure or load of crash items -- put char in temple.
 2 - rented equipment lost (no $)
 */
int Crash_load(struct char_data * ch)
{
  void Crash_save(struct char_data * ch, int type);
  FILE *fl;
  char fname[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char input[MAX_INPUT_LENGTH + 1];
  char tag[MAX_INPUT_LENGTH + 1];
  char tag_arguments[MAX_INPUT_LENGTH + 1];
  int val;
  struct obj_data *tmpobj = NULL;
  struct obj_data *tmpobj2;
  struct obj_data *next_obj;
  int found_begin = 0;
  int affect_counter = 0;
  int found;
  char *p;
  struct corpse_obj_save *crash_load_stack = NULL;
  struct corpse_obj_save *temp_stack = NULL;
  struct corpse_obj_save *temp_stack_next = NULL;
  int chg;
  int ovnum = 0;
  int j;

  if (!get_filename(GET_NAME(ch), fname, CRASH_FILE)) {
    return 1;
  }
  if (!(fl = fopen(fname, "r"))) {
    if (errno != ENOENT) { /* if it fails, NOT because of no file */
      safe_snprintf(buf1, MAX_STRING_LENGTH, "SYSERR: READING OBJECT FILE %s (5)", fname);
      perror(buf1);
      send_to_char("\r\n********************* NOTICE *********************\r\n"
          "There was a problem loading your objects from disk.\r\n"
          "Contact a God for assistance.\r\n", ch);
    }
    safe_snprintf(buf, MAX_STRING_LENGTH, "%s entering game with no equipment.", GET_NAME(ch));
    mudlog(buf, 'R', COM_IMMORT, TRUE);
    plog(buf, ch, 0);
    return 1;
  }

  while (get_line(fl, input)) {
    parse_pline(input, tag, tag_arguments);
    while ((p = strrchr(tag_arguments, '\n')) != NULL) {
      *p = '\0';
    }

    val = atoi(tag_arguments);
    switch (tag[4]) {
      case 'a':
      case 'A':
        if (strcasecmp(tag, "obj_affect_loc") == 0 && affect_counter < MAX_OBJ_AFFECT) {
          tmpobj->affected[affect_counter].location = val;
        } else if (strcasecmp(tag, "obj_affect_mod") == 0 && affect_counter < MAX_OBJ_AFFECT) {
          tmpobj->affected[affect_counter].modifier = val;
          affect_counter++;
        } else {
          safe_snprintf(buf, MAX_STRING_LENGTH, "Unknown Rent-File Tag: %s", tag);
          stderr_log(buf);
        }
        break;
      case 'b':
      case 'B':
        if (strcasecmp(tag, "obj_begin") == 0) {
          if (found_begin) {
            send_to_char("\r\n********************* NOTICE *********************\r\n"
                "There was a problem loading your objects from disk.\r\n"
                "Contact a God for assistance.\r\n", ch);
            return 1;
          }
          found_begin = 1;
        } else if (strcasecmp(tag, "obj_bitvector") == 0) {
          GET_OBJ_BITV(tmpobj) = asciiflag_conv(tag_arguments);
        } else if (strcasecmp(tag, "obj_bitvector2") == 0) {
          GET_OBJ_BITV2(tmpobj) = asciiflag_conv(tag_arguments);
        } else {
          safe_snprintf(buf, MAX_STRING_LENGTH, "Unknown Rent-File Tag: %s", tag);
          stderr_log(buf);
        }
        break;
      case 'e':
      case 'E':
        if (strcasecmp(tag, "obj_extra_flags") == 0) {
          GET_OBJ_EXTRA(tmpobj) = asciiflag_conv(tag_arguments);
        } else if (strcasecmp(tag, "obj_end") == 0) {
          if (ovnum >= 0) {
            if (!found_begin) {
              send_to_char("\r\n********************* NOTICE *********************\r\n"
                  "There was a problem loading your objects from disk.\r\n"
                  "Contact a God for assistance.\r\n", ch);
              return 1;
            }
            found_begin = 0;
            affect_counter = 0;

            CREATE(crash_load_stack, struct corpse_obj_save, 1);
            crash_load_stack->level = 0;
            crash_load_stack->prev = NULL;

            for (tmpobj2 = ch->carrying, found = 0; tmpobj2 && !found; tmpobj2 = next_obj) {
              next_obj = tmpobj2->next_content;
              if (tmpobj2->objnum == tmpobj->inobj) {
                obj_to_obj(tmpobj, tmpobj2);
                found = 1;
              }
              if (tmpobj2->contains) {
                crash_load_stack->next_obj = next_obj;
                CREATE(temp_stack, struct corpse_obj_save, 1);
                temp_stack->level = crash_load_stack->level + 1;
                temp_stack->prev = crash_load_stack;
                crash_load_stack = temp_stack;
                next_obj = tmpobj2->contains;
                if (tmpobj2 == next_obj) {
                  /* infinite loop */
                  next_obj = NULL;
                  continue;
                }
              } else if (next_obj == NULL && crash_load_stack->level > 0) {
                temp_stack = crash_load_stack;
                crash_load_stack = crash_load_stack->prev;
                FREE(temp_stack);
                next_obj = crash_load_stack->next_obj;
              }
            }
            for (temp_stack = crash_load_stack; temp_stack; temp_stack = temp_stack_next) {
              temp_stack_next = temp_stack->prev;
              FREE(temp_stack);
            }
            if (!found) {
              obj_to_char(tmpobj, ch);
            }
          } else {
            extract_obj(tmpobj);
            found_begin = 0;
            affect_counter = 0;
            ovnum = 0;
          }
        } else {
          safe_snprintf(buf, MAX_STRING_LENGTH, "Unknown Rent-File Tag: %s", tag);
          stderr_log(buf);
        }
        break;
      case 'n':
      case 'N':
        if (strcasecmp(tag, "obj_number") == 0) {
          tmpobj = read_object(val, VIRTUAL);
          ovnum = val;
          if (!tmpobj) {
            tmpobj = create_obj();
          }
        } else {
          safe_snprintf(buf, MAX_STRING_LENGTH, "Unknown Rent-File Tag: %s", tag);
          stderr_log(buf);
        }
        break;
      case 'p':
      case 'P':
        if (strcasecmp(tag, "obj_pos") == 0) {
          tmpobj->inobj = val;
        } else {
          safe_snprintf(buf, MAX_STRING_LENGTH, "Unknown Rent-File Tag: %s", tag);
          stderr_log(buf);
        }
        break;
      case 's':
      case 'S':
        if (strcasecmp(tag, "obj_spellbook_entry") == 0) {
          for (j = 0; j < GET_OBJ_VAL(tmpobj, 0); j++) {
            if (GET_OBJ_SPELLLISTNUM(tmpobj, j) == 0) {
              GET_OBJ_SPELLLISTNUM(tmpobj, j) = spells[find_spell_num(tag_arguments)].spellindex;
              break;
            }
          }
        } else if (strcasecmp(tag, "obj_spell1") == 0) {
          GET_OBJ_VAL(tmpobj, 1) = spells[find_spell_num(tag_arguments)].spellindex;
        } else if (strcasecmp(tag, "obj_spell2") == 0) {
          GET_OBJ_VAL(tmpobj, 2) = spells[find_spell_num(tag_arguments)].spellindex;
        } else if (strcasecmp(tag, "obj_spell3") == 0) {
          GET_OBJ_VAL(tmpobj, 3) = spells[find_spell_num(tag_arguments)].spellindex;
        } else {
          safe_snprintf(buf, MAX_STRING_LENGTH, "Unknown Rent-File Tag: %s", tag);
          stderr_log(buf);
        }
        break;
      case 't':
      case 'T':
        if (strcasecmp(tag, "obj_tnum") == 0) {
          tmpobj->objnum = val;
        } else if (strcasecmp(tag, "obj_timer") == 0) {
          GET_OBJ_TIMER(tmpobj) = val;
        } else {
          safe_snprintf(buf, MAX_STRING_LENGTH, "Unknown Rent-File Tag: %s", tag);
          stderr_log(buf);
        }
        break;
      case 'v':
      case 'V':
        if (strcasecmp(tag, "obj_value0") == 0) {
          GET_OBJ_VAL(tmpobj, 0) = val;
        } else if (strcasecmp(tag, "obj_value1") == 0) {
          if (GET_OBJ_TYPE(tmpobj) == ITEM_DRINKCON) {
            chg = val - GET_OBJ_VAL(tmpobj, 1);
            weight_change_object(tmpobj, chg);
            GET_OBJ_VAL(tmpobj, 1) = val;
          } else {
            GET_OBJ_VAL(tmpobj, 1) = val;
          }
        } else if (strcasecmp(tag, "obj_value2") == 0) {
          GET_OBJ_VAL(tmpobj, 2) = val;
        } else if (strcasecmp(tag, "obj_value3") == 0) {
          GET_OBJ_VAL(tmpobj, 3) = val;
        } else if (strcasecmp(tag, "obj_value4") == 0) {
          GET_OBJ_VAL(tmpobj, 4) = val;
        } else {
          safe_snprintf(buf, MAX_STRING_LENGTH, "Unknown Rent-File Tag: %s", tag);
          stderr_log(buf);
        }
        break;
      case 'w':
      case 'W':
        if (strcasecmp(tag, "obj_weight") == 0) {
          GET_OBJ_WEIGHT(tmpobj) = val;
        } else {
          safe_snprintf(buf, MAX_STRING_LENGTH, "Unknown Rent-File Tag: %s", tag);
          stderr_log(buf);
        }
        break;
      case '_':
        if (strcasecmp(tag, "rent_type") == 0) {
          switch (val) {
            case RENT_RENTED:
              safe_snprintf(buf, MAX_STRING_LENGTH, "%s un-renting and entering game.", GET_NAME(ch));
              mudlog(buf, 'R', COM_IMMORT, TRUE);
              break;
            case RENT_CRASH:
              safe_snprintf(buf, MAX_STRING_LENGTH, "%s retrieving crash-saved items and entering game.", GET_NAME(ch));
              mudlog(buf, 'R', COM_IMMORT, TRUE);
              break;
            case RENT_CRYO:
              safe_snprintf(buf, MAX_STRING_LENGTH, "%s un-cryo'ing and entering game.", GET_NAME(ch));
              mudlog(buf, 'R', COM_IMMORT, TRUE);
              break;
            case RENT_CAMPED:
              safe_snprintf(buf, MAX_STRING_LENGTH, "%s un-camping and entering game.", GET_NAME(ch));
              mudlog(buf, 'R', COM_IMMORT, TRUE);
              break;
            case RENT_FORCED:
            case RENT_TIMEDOUT:
              safe_snprintf(buf, MAX_STRING_LENGTH, "%s retrieving force-saved items and entering game.", GET_NAME(ch));
              mudlog(buf, 'R', COM_IMMORT, TRUE);
              break;
            default:
              safe_snprintf(buf, MAX_STRING_LENGTH, "WARNING: %s entering game with undefined rent code.", GET_NAME(ch));
              mudlog(buf, 'W', COM_IMMORT, TRUE);
              break;
          }
          plog(buf, ch, 0);
        } else {
          safe_snprintf(buf, MAX_STRING_LENGTH, "Unknown Rent-File Tag: %s", tag);
          stderr_log(buf);
        }
        break;
    }
  }
  tmpobj = ch->carrying;
  while (tmpobj) {
    next_obj = tmpobj->next_content;
    if (tmpobj->inobj < 0) {
      obj_from_char(tmpobj);
      equip_char(ch, tmpobj, -(tmpobj->inobj));
      CHAR_WEARING(ch) |= GET_OBJ_SLOTS(tmpobj);
    }
    tmpobj->objnum = -1;
    tmpobj = next_obj;
  }

  fclose(fl);

  /* Crash save char -Rasmus */

  Crash_save(ch, RENT_CRASH);

  return 0;
}

int Crash_is_unrentable(struct obj_data * obj)
{
  if (!obj) {
    return 0;
  }

  if (IS_OBJ_STAT(obj, ITEM_NORENT) || GET_OBJ_RNUM(obj) <= NOTHING || GET_OBJ_TYPE(obj) == ITEM_KEY) {
    return 1;
  }

  return 0;
}

void Crash_save(struct char_data * ch, int type)
{
  char buf[MAX_INPUT_LENGTH];
  struct obj_data *tmpobj = NULL;
  struct obj_data *next_obj = NULL;
  struct corpse_obj_save *crash_save_stack = NULL;
  struct corpse_obj_save *temp_stack = NULL;
  struct corpse_obj_save *temp_stack_next = NULL;
  int j;
  int objnum = 0;
  extern int top_of_objt;
  FILE *fp = NULL;

  if (IS_NPC(ch)) {
    return;
  }

  if (!get_filename(GET_NAME(ch), buf, CRASH_FILE)) {
    return;
  }
  if (!(fp = fopen(buf, "w"))) {
    return;
  }

  CREATE(crash_save_stack, struct corpse_obj_save, 1);
  crash_save_stack->level = 0;
  crash_save_stack->prev = NULL;
  crash_save_stack->containernum = objnum;

  fprintf(fp, "-rent_version- %d\n", RENT_VERSION);
  fprintf(fp, "-rent_time- %d\n", (int) time(0));
  fprintf(fp, "-rent_type- %d\n", type);

  for (tmpobj = ch->carrying, objnum++; tmpobj; tmpobj = next_obj, objnum++) {
    next_obj = tmpobj->next_content;
    if (tmpobj == next_obj) {
      /* infinite loop */
      next_obj = NULL;
      continue;
    }
    if ((tmpobj->item_number <= top_of_objt) && (!Crash_is_unrentable(tmpobj) || type == RENT_CRASH)) {
      Obj_to_store(tmpobj, objnum, crash_save_stack->containernum, fp);
      if (tmpobj->contains) {
        crash_save_stack->next_obj = next_obj;
        CREATE(temp_stack, struct corpse_obj_save, 1);
        temp_stack->level = crash_save_stack->level + 1;
        temp_stack->prev = crash_save_stack;
        temp_stack->containernum = objnum;
        crash_save_stack = temp_stack;
        next_obj = tmpobj->contains;
        if (tmpobj == next_obj) {
          /* infinite loop */
          next_obj = NULL;
          continue;
        }
      } else if (next_obj == NULL && crash_save_stack->level > 0) {
        temp_stack = crash_save_stack;
        crash_save_stack = crash_save_stack->prev;
        FREE(temp_stack);
        next_obj = crash_save_stack->next_obj;
      }
    }
  }

  for (temp_stack = crash_save_stack; temp_stack; temp_stack = temp_stack_next) {
    temp_stack_next = temp_stack->prev;
    FREE(temp_stack);
  }

  for (j = 0; j < NUM_WEARS; j++, objnum++) {
    if (ch->equipment[j]) {
      if (!Crash_is_unrentable(tmpobj) || type == RENT_CRASH) {
        Obj_to_store(ch->equipment[j], objnum, -j, fp);
        if (ch->equipment[j]->contains) {
          CREATE(crash_save_stack, struct corpse_obj_save, 1);
          crash_save_stack->level = 0;
          crash_save_stack->prev = NULL;
          crash_save_stack->containernum = objnum;
          for (tmpobj = ch->equipment[j]->contains; tmpobj; tmpobj = next_obj, objnum++) {
            next_obj = tmpobj->next_content;
            if (tmpobj == next_obj) {
              /* infinite loop */
              next_obj = NULL;
              continue;
            }
            if (!Crash_is_unrentable(tmpobj) || type == RENT_CRASH) {
              Obj_to_store(tmpobj, objnum, crash_save_stack->containernum, fp);
              if (tmpobj->contains) {
                crash_save_stack->next_obj = next_obj;
                CREATE(temp_stack, struct corpse_obj_save, 1);
                temp_stack->level = crash_save_stack->level + 1;
                temp_stack->prev = crash_save_stack;
                temp_stack->containernum = objnum;
                crash_save_stack = temp_stack;
                next_obj = tmpobj->contains;
                if (tmpobj == next_obj) {
                  /* infinite loop */
                  next_obj = NULL;
                  continue;
                }
              } else if (next_obj == NULL && crash_save_stack->level > 0) {
                temp_stack = crash_save_stack;
                crash_save_stack = crash_save_stack->prev;
                FREE(temp_stack);
                next_obj = crash_save_stack->next_obj;
              }
            }
          }
          for (temp_stack = crash_save_stack; temp_stack; temp_stack = temp_stack_next) {
            temp_stack_next = temp_stack->prev;
            FREE(temp_stack);
          }
        }
      }
    }
  }
  fprintf(fp, "-end_rent-\n");
  fclose(fp);
  REMOVE_BIT(PLR_FLAGS(ch), PLR_CRASH);
}

/* ************************************************************************
 * Routines used for the receptionist                                *
 ************************************************************************* */

int Crash_report_unrentables(struct char_data * ch, struct char_data * recep, struct obj_data * obj)
{
  char buf[128];
  int has_norents = 0;

  if (obj) {
    if (Crash_is_unrentable(obj)) {
      has_norents = 1;
      if (recep) {
        safe_snprintf(buf, MAX_STRING_LENGTH, "$n tells you, 'You cannot store %s.'", OBJS(obj, ch));
        act(buf, FALSE, recep, 0, ch, TO_VICT);
      } else {
        safe_snprintf(buf, MAX_STRING_LENGTH, "You cannot store %s\n", OBJS(obj, ch));
        send_to_char(buf, ch);
      }
    }
    has_norents += Crash_report_unrentables(ch, recep, obj->contains);
    has_norents += Crash_report_unrentables(ch, recep, obj->next_content);
  }
  return (has_norents);
}

int gen_receptionist(struct char_data * ch, struct char_data * recep, int cmd, char *arg, int mode)
{
  int i;
  sh_int save_room;
  int unrentables = 0;
  char *action_table[] = {"smile", "dance", "sigh", "blush", "burp", "cough", "fart", "twiddle", "yawn"};

  ACMD(do_action);

  if (!ch->desc || IS_NPC(ch))
    return FALSE;

  if (!cmd && !number(0, 5)) {
    do_action(recep, "", find_command(action_table[number(0, 8)]), 0);
    return FALSE;
  }
  if (!CMD_IS("rent")) {
    return FALSE;
  }
  if (!AWAKE(recep)) {
    send_to_char("She is unable to talk to you...\r\n", ch);
    return TRUE;
  }
  if (!CAN_SEE(recep, ch)) {
    act("$n says, 'I don't deal with people I can't see!'", FALSE, recep, 0, 0, TO_ROOM);
    return TRUE;
  }
  if (CMD_IS("rent")) {

    unrentables = Crash_report_unrentables(ch, recep, ch->carrying);
    for (i = 0; i < NUM_WEARS; i++) {
      unrentables = Crash_report_unrentables(ch, recep, GET_EQ(ch, i));
    }
    if (unrentables) {
      return TRUE;
    }
    safe_snprintf(buf, MAX_STRING_LENGTH, "{r$n tells you, '{REnjoy your stay.{r'{x");
    act(buf, FALSE, recep, 0, ch, TO_VICT);

    if (mode == RENT_FACTOR) {
      act("{W$n stores your belongings and helps you into your private chamber.{x", FALSE, recep, 0, ch, TO_VICT);
      Crash_save(ch, RENT_RENTED);
      SET_BIT(PLR_FLAGS(ch), PLR_RENT);
      safe_snprintf(buf, MAX_STRING_LENGTH, "%s has rented in room #%d", GET_NAME(ch), world[IN_ROOM(ch)].number);
      mudlog(buf, 'R', COM_IMMORT, TRUE);
    } else { /* cryo */
      act("$n stores your belongings and helps you into your private chamber.\r\n"
          "A white mist appears in the room, chilling you to the bone...\r\n"
          "You begin to lose consciousness...", FALSE, recep, 0, ch, TO_VICT);
      Crash_save(ch, RENT_CRYO);
      SET_BIT(PLR_FLAGS(ch), PLR_CRYO);
      safe_snprintf(buf, MAX_STRING_LENGTH, "%s has cryo-rented in room #%d.", GET_NAME(ch), world[IN_ROOM(ch)].number);
      mudlog(buf, 'R', COM_IMMORT, TRUE);
    }

    act("$n helps $N into $S private chamber.", FALSE, recep, 0, ch, TO_NOTVICT);
    save_room = ch->in_room;
    SET_BIT(PLR_FLAGS(ch), PLR_LOADROOM);
    GET_LOADROOM(ch) = world[ch->in_room].number;
    extract_char(ch, 0);
    ch->in_room = real_room(world[save_room].number);
    save_char_text(ch, ch->in_room);
    clear_magic_memory(ch);
    for (i = 0; i < NUM_WEARS; i++) {
      if (GET_EQ(ch, i)) {
        obj_to_char(unequip_char(ch, i), ch);
      }
    }
  }
  return TRUE;
}

SPECIAL(receptionist)
{
  return (gen_receptionist(ch, me, cmd, argument, RENT_FACTOR));
}

SPECIAL(cryogenicist)
{
  return (gen_receptionist(ch, me, cmd, argument, CRYO_FACTOR));
}

void Crash_save_all(void)
{
  struct descriptor_data *d;
  for (d = descriptor_list; d; d = d->next) {
    if ((d->connected == CON_PLAYING) && !IS_NPC(d->character)) {
      Crash_save(d->character, RENT_CRASH);
      save_char_text(d->character, NOWHERE);
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_CRASH);
    }
  }
}

void all_crashsave(void)
{
  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next) {
    if ((d->connected == CON_PLAYING) && !IS_NPC(d->character)) {
      if (PLR_FLAGGED(d->character, PLR_CRASH)) {
        Crash_save(d->character, RENT_CRASH);
        REMOVE_BIT(PLR_FLAGS(d->character), PLR_CRASH);
      }
    }
  }
}

void corpsesaveall()
{
  FILE *tofile = NULL;
  struct obj_data *obj;
  int objnum = 0;
  int tmpobjnum = 0;
  bool written = FALSE;

  tofile = fopen(CORPSE_FILE, "w");

  for (obj = object_list; obj; obj = obj->next) {
    if (GET_OBJ_TYPE(obj) == ITEM_PCORPSE && !obj->carried_by && !obj->worn_by && !obj->in_obj) {
      tmpobjnum = corpsesave(obj, objnum, tofile);
      objnum = tmpobjnum;
      written = TRUE;
    }
  }
  fseek(tofile, -2, SEEK_CUR);
  fprintf(tofile, "$");
  fclose(tofile);
  if (!written) {
    unlink(CORPSE_FILE);
  }
  return;
}

int corpsesave(struct obj_data *obj, int objnum, FILE *tofile)
{
  struct corpse_obj_save *corpse_container_stack;
  struct corpse_obj_save *temp_stack;
  struct corpse_obj_save *temp_stack_next;
  struct obj_data *tmpobj;
  struct obj_data *next_obj;

  CREATE(corpse_container_stack, struct corpse_obj_save, 1);
  corpse_container_stack->level = 0;
  corpse_container_stack->prev = NULL;
  corpse_container_stack->containernum = objnum;

  corpsesaveobj(obj, objnum, -1, tofile);
  for (tmpobj = obj->contains; tmpobj && corpse_container_stack->level < 8; tmpobj = next_obj) {
    next_obj = tmpobj->next_content;
    if (tmpobj == next_obj) {
      /* infinite loop */
      next_obj = NULL;
      continue;
    }
    corpsesaveobj(tmpobj, ++objnum, corpse_container_stack->containernum, tofile);
    if (tmpobj->contains) {
      corpse_container_stack->next_obj = next_obj;
      CREATE(temp_stack, struct corpse_obj_save, 1);
      temp_stack->level = corpse_container_stack->level + 1;
      temp_stack->prev = corpse_container_stack;
      temp_stack->containernum = objnum;
      corpse_container_stack = temp_stack;
      next_obj = tmpobj->contains;
    } else if (next_obj == NULL && corpse_container_stack->level > 0) {
      /* pop off of stack and set next_obj */
      temp_stack = corpse_container_stack;
      corpse_container_stack = corpse_container_stack->prev;
      FREE(temp_stack);
      next_obj = corpse_container_stack->next_obj;
    }
  }

  for (temp_stack = corpse_container_stack; temp_stack; temp_stack = temp_stack_next) {
    temp_stack_next = temp_stack->prev;
    FREE(temp_stack);
  }

  return ++objnum;
}

void corpsesaveobj(struct obj_data *obj, int objnum, int inobj, FILE *tofile)
{
  struct extra_descr_data *ex_desc;
  int counter = 0;

  fprintf(tofile, "%d ", objnum); /* index in save file */
  fprintf(tofile, "%d ", obj->in_room); /* room obj is in */
  fprintf(tofile, "%d\n", inobj); /* index # of container */

  fprintf(tofile, "%d\n", GET_OBJ_VNUM(obj));

  fprintf(tofile, "%s~\n", obj->name); /* obj name */
  fprintf(tofile, "%s~\n", obj->description); /* long desc */
  fprintf(tofile, "%s~\n", obj->short_description); /* short desc */
  if (obj->action_description) {
    fprintf(tofile, "%s~\n", obj->action_description); /* use desc */
  } else {
    fprintf(tofile, "~\n");
  }
  if (obj->cname) {
    fprintf(tofile, "%s~\n", obj->cname); /* obj name */
  } else {
    fprintf(tofile, "~\n");
  }
  if (obj->cdescription) {
    fprintf(tofile, "%s~\n", obj->cdescription); /* long desc */
  } else {
    fprintf(tofile, "~\n");
  }
  if (obj->cshort_description) {
    fprintf(tofile, "%s~\n", obj->cshort_description); /* short desc */
  } else {
    fprintf(tofile, "~\n");
  }

  /* obj type, extra flags and wear positions */
  fprintf(tofile, "%d %d %d\n", GET_OBJ_TYPE(obj), GET_OBJ_EXTRA(obj), GET_OBJ_WEAR(obj));

  /* object values */
  if (GET_OBJ_VAL(obj, 4)) {
    fprintf(tofile, "%d %d %d %d %d\n", GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 3), GET_OBJ_VAL(obj, 4));
  } else {
    fprintf(tofile, "%d %d %d %d\n", GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 3));
  }

  /* object weight, cost and rent cost */
  fprintf(tofile, "%d %d\n", GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj));

  /* extra descs, if any */
  if (obj->ex_description) {
    for (ex_desc = obj->ex_description; ex_desc; ex_desc = ex_desc->next) {
      fprintf(tofile, "E\n");
      fprintf(tofile, "%s~\n", ex_desc->keyword);
      fprintf(tofile, "%s~\n", ex_desc->description);
    }
  }

  /* affects, if any */
  for (counter = 0; counter < MAX_OBJ_AFFECT; counter++) {
    if (obj->affected[counter].modifier) {
      fprintf(tofile, "A\n");
      fprintf(tofile, "%d %d\n", obj->affected[counter].location, obj->affected[counter].modifier);
    }
  }

  /* perm affect bitvector, if any <grin> */
  if (obj->obj_flags.bitvector) {
    fprintf(tofile, "B\n");
    fprintf(tofile, "%ld\n", obj->obj_flags.bitvector);
  }

  /* S vals, if any */
  if (GET_OBJ_SVAL(obj, 0) || GET_OBJ_SVAL(obj, 1) || GET_OBJ_SVAL(obj, 1)) {
    fprintf(tofile, "S\n");
    fprintf(tofile, "%d %d %d\n", GET_OBJ_SVAL(obj, 0), GET_OBJ_SVAL(obj, 1), GET_OBJ_SVAL(obj, 2));
  }

  /* corpse duration left, if applicable */
  if (GET_OBJ_TYPE(obj) == ITEM_PCORPSE) {
    fprintf(tofile, "P\n");
    fprintf(tofile, "%d\n", GET_OBJ_TIMER(obj));
    fprintf(tofile, "%s~\n", obj->owner);
  }

  fprintf(tofile, "#\n");

  return;
}

void corpseloadall()
{
  FILE *fromfile = NULL;
  struct obj_data *obj;
  struct obj_data *obj2 = NULL;

  doneload = FALSE;
  if (!(fromfile = fopen(CORPSE_FILE, "r"))) {
    return;
  }
  while (!doneload) {
    obj = corpseloadobj(fromfile);
    if (obj->in_room > NOWHERE) {
      obj_to_room(obj, obj->in_room);
      if (GET_OBJ_TYPE(obj) == ITEM_PCORPSE) {
        REMOVE_BIT(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
      }
    } else if (obj->inobj > -1) {
      for (obj2 = object_list; obj2; obj2 = obj2->next) {
        if (obj2->objnum == obj->inobj) {
          obj_to_obj2(obj, obj2);
        }
      }
    }
  }
  fclose(fromfile);

  return;
}

struct obj_data *corpseloadobj(FILE *fromfile)
{
  struct obj_data *obj;
  struct extra_descr_data *new_descr;
  static char line[256];
  extern int top_of_objt;
  extern struct index_data *obj_index;
  int t[10], temp;
  int j = 0;
  int i = 0;

  obj = create_obj();

  if (fscanf(fromfile, "%d %d %d\n", &(obj->objnum), &(obj->in_room), &(obj->inobj)) != 3) {
    stderr_log("SYSERR: Format error reading object header from corpse file");
    extract_obj(obj);
    return NULL;
  }

  if (fscanf(fromfile, "%d\n", &i) != 1) {
    stderr_log("SYSERR: Format error reading item number from corpse file");
    extract_obj(obj);
    return NULL;
  }
  if (i > -1 && i <= obj_index[top_of_objt].virtual) {
    obj->item_number = (real_object(i));
  } else {
    obj->item_number = -1;
  }

  obj->name = fread_string(fromfile, buf2);
  obj->description = fread_string(fromfile, buf2);
  obj->short_description = fread_string(fromfile, buf2);
  obj->action_description = fread_string(fromfile, buf2);
  if (obj->cname) {
    FREE(obj->cname);
  }
  obj->cname = fread_string(fromfile, buf2);
  if (obj->cdescription) {
    FREE(obj->cdescription);
  }
  obj->cdescription = fread_string(fromfile, buf2);
  if (obj->cshort_description) {
    FREE(obj->cshort_description);
  }
  obj->cshort_description = fread_string(fromfile, buf2);

  if (fscanf(fromfile, "%d %d %d\n", &(obj->obj_flags.type_flag), &(obj->obj_flags.extra_flags), &(obj->obj_flags.wear_flags)) != 3) {
    stderr_log("SYSERR: Format error reading object flags from corpse file");
    extract_obj(obj);
    return NULL;
  }

  get_line(fromfile, line);

  temp = sscanf(line, "%d %d %d %d %d\n", t, t + 1, t + 2, t + 3, t + 4);
  if (temp == 4) {
    obj->obj_flags.value[0] = t[0];
    obj->obj_flags.value[1] = t[1];
    obj->obj_flags.value[2] = t[2];
    obj->obj_flags.value[3] = t[3];
  } else if (temp == 5) {
    obj->obj_flags.value[0] = t[0];
    obj->obj_flags.value[1] = t[1];
    obj->obj_flags.value[2] = t[2];
    obj->obj_flags.value[3] = t[3];
    obj->obj_flags.value[4] = t[4];
  } else if (temp < 4) {
    stderr_log("SYSERR: Format error reading object values from corpse file");
    extract_obj(obj);
    return NULL;
  }

  if (fscanf(fromfile, "%d %d\n", &(obj->obj_flags.weight), &(obj->obj_flags.cost)) != 2) {
    stderr_log("SYSERR: Format error reading object weight/cost from corpse file");
    extract_obj(obj);
    return NULL;
  }

  for (;;) {
    get_line(fromfile, line);
    switch (*line) {
      case 'B':
        fscanf(fromfile, "%ld\n", &(obj->obj_flags.bitvector));
        break;
      case 'S':
        fscanf(fromfile, "%d %d %d\n", t, t + 1, t + 2);
        obj->spec_vars[0] = t[0];
        obj->spec_vars[1] = t[1];
        obj->spec_vars[2] = t[2];
        break;
      case 'E':
        CREATE(new_descr, struct extra_descr_data, 1);
        new_descr->keyword = fread_string(fromfile, buf2);
        new_descr->description = fread_string(fromfile, buf2);
        new_descr->next = obj->ex_description;
        obj->ex_description = new_descr;
        break;
      case 'A':
        fscanf(fromfile, "%d %d\n", t, t + 1);
        obj->affected[j].location = t[0];
        obj->affected[j].modifier = t[1];
        j++;
        break;
      case 'P':
        fscanf(fromfile, "%d\n", &(obj->obj_flags.timer));
        obj->owner = fread_string(fromfile, buf2);
        break;
      case '$':
        doneload = TRUE;
        return obj;
      case '#':
        return obj;
      default:
        fprintf(stderr, "Format error in corpse file.\n");
        fflush(NULL);
        exit(1);
        break;
    }
  }
  return NULL;
}

