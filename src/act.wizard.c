/* ************************************************************************
 *   File: act.wizard.c                                  Part of CircleMUD *
 *  Usage: Player-level god commands and other goodies                     *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "event.h"
#include "spells.h"
#include "screen.h"
#include "maze.h"
#include "olc.h"

#define PC   1
#define NPC  2
#define BOTH 3

#define MISC  0
#define BINARY  1
#define NUMBER  2

/*   external vars  */
extern int mob_idnum; /* mobs have a negative idnum */
extern int file_to_string_alloc(char *name, char **buf);
extern struct room_data *world;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct descriptor_data *descriptor_list;
extern int MaxExperience[LVL_IMMORT + 2];
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct char_data *mob_proto;
extern struct obj_data *obj_proto;
extern sh_int stats[11][101];
extern struct zone_data *zone_table;
extern struct player_special_data *dummy_mob;
extern int top_of_zone_table;
extern int restrict_game_lvl;
extern int top_of_world;
extern int top_of_mobt;
extern int top_of_objt;
extern int top_of_p_table;
extern int new_top_mobt;
extern int new_top_objt;
extern char *weapon_handed[];
extern int pk_allowed;
extern int pt_allowed;
extern int level_can_shout;
extern int level_can_color;
extern int max_exp_gain;
extern int max_exp_loss;
extern int max_npc_corpse_time;
extern int max_pc_corpse_time;
extern int max_obj_time;
extern int free_rent;
extern int max_obj_save;
extern int min_rent_cost;
extern int auto_save;
extern int autosave_time;
extern int crash_file_timeout;
extern int rent_file_timeout;
extern int PRACS_COST;
extern int NODECAY;

extern void show_string(struct descriptor_data * d, char *input);
/* for objects */
extern char *item_types[];
extern char *wear_bits[];
extern char *extra_bits[];
extern char *drinks[];
extern char *material_types[];
extern char *weapon_types[];
char *get_spell_name(int spellnum);

/* for rooms */
extern char *dirs[];
extern char *room_bits[];
extern char *exit_bits[];
extern char *sector_types[];

/* for chars */
extern struct spell_info_type *spells;
extern char *equipment_types[];
extern char *worn_bits[];
extern char *affected_bits[];
extern char *affected_bits2[];
extern char *affected_bits3[];
extern char *difficulty[];
extern char *apply_types[];
extern char *pc_class_types[];
extern char *pc_race_types[];
extern char *npc_race_names[];
extern char *npc_class_types[];
extern char *action_bits[];
extern char *player_bits[];
extern char *com_bits[];
extern char *preference_bits[];
extern char *position_types[];
extern char *connected_types[];
extern char *size_names[];
extern int death_count;
extern char *resists_names[];
extern char *saving_throws[];
extern char *resist_short_name[];

extern char *mprog_type_to_name(int type);

int find_spell_num(char *spellname);
int find_skill_num_def(int define);
int find_skill_num(char *skillname);
extern int find_name(char *name);
extern int has_mail(int id);
extern void load_text(struct char_data * vict);
extern void Crash_save(struct char_data * vict, int type);
extern void save_text(struct char_data * vict);
extern void Crash_delete_text(char *name);
void reread_wizlists(void);
extern void create_maze(maze *m);
extern void print_maze(maze *m, FILE *f);
extern void reboot_maze(maze *m, int zone);
extern void Read_Invalid_List();
int load_char_text(char *name, struct char_data *char_element);
extern int SECS_PER_MUD_HOUR;
extern int SECS_PER_MUD_DAY;
extern int SECS_PER_MUD_MONTH;
extern int SECS_PER_MUD_YEAR;
extern int num_months;
extern int num_days;
extern int num_hours;

char *find_hunted_name(int idnum)
{
  struct char_data *vict;
  for (vict = character_list; vict; vict = vict->next) {
    if (GET_IDNUM(vict) == idnum) {
      return GET_NAME(vict);
    }
  }
  return NULL;
}

ACMD(do_config)
{
  int l = 0;
  char abuf[MAX_STRING_LENGTH];
  char field[MAX_INPUT_LENGTH], val_arg[MAX_INPUT_LENGTH];
  int on = 0, off = 0, value = 0;
  extern int max_players;
  struct config_struct {
    char *cmd;
    char level;
    char type;
  } fields[] = {
      {"ticsize", COM_ADMIN, NUMBER}, /* 0 */
      {"num_months", COM_ADMIN, NUMBER}, /* 1 */
      {"num_days", COM_ADMIN, NUMBER}, /* 2 */
      {"num_hours", COM_ADMIN, NUMBER}, /* 3 */
      {"pk_allowed", COM_ADMIN, BINARY}, /* 4 */
      {"pt_allowed", COM_ADMIN, BINARY}, /* 5 */
      {"lvl_can_shout", COM_ADMIN, NUMBER}, /* 6 */
      {"lvl_can_color", COM_ADMIN, NUMBER}, /* 7 */
      {"max_exp_gain", COM_ADMIN, NUMBER}, /* 8 */
      {"max_exp_loss", COM_ADMIN, NUMBER}, /* 9 */
      {"max_npc_corpse_time", COM_ADMIN, NUMBER}, /* 10 */
      {"max_pc_corpse_time", COM_ADMIN, NUMBER}, /* 11 */
      {"max_obj_time", COM_ADMIN, NUMBER}, /* 12 */
      {"free_rent", COM_ADMIN, BINARY}, /* 13 */
      {"max_obj_save", COM_ADMIN, NUMBER}, /* 14 */
      {"max_rent_cost", COM_ADMIN, NUMBER}, /* 15 */
      {"auto_save", COM_ADMIN, BINARY}, /* 16 */
      {"autosave_time", COM_ADMIN, NUMBER}, /* 17 */
      {"crash_file_timeout", COM_ADMIN, NUMBER}, /* 18 */
      {"rent_file_timeout", COM_ADMIN, NUMBER}, /* 19 */
      {"maxconnect", COM_ADMIN, NUMBER}, /* 20 */
      {"pracs_cost", COM_ADMIN, BINARY}, /* 21 */
      {"nodecay", COM_ADMIN, BINARY}, /* 22 */
      {"\n", 0, MISC}
  };

  half_chop(argument, field, buf);

  if (!*field) {
    sprintf(abuf, "Config Options\r\n\r\n");
    sprintf(abuf + strlen(abuf), "ticsize (in secs)            :  %d\r\n", SECS_PER_MUD_HOUR);
    sprintf(abuf + strlen(abuf), "num_months (per year)        :  %d\r\n", num_months);
    sprintf(abuf + strlen(abuf), "num_days (per month)         :  %d\r\n", num_days);
    sprintf(abuf + strlen(abuf), "num_hours (per day)          :  %d\r\n", num_hours);
    sprintf(abuf + strlen(abuf), "pk_allowed (yes no)          :  %s\r\n", YESNO(pk_allowed));
    sprintf(abuf + strlen(abuf), "pt_allowed (yes no)          :  %s\r\n", YESNO(pt_allowed));
    sprintf(abuf + strlen(abuf), "level_can_shout (0-51)       :  %d\r\n", level_can_shout);
    sprintf(abuf + strlen(abuf), "level_can_color (0-51)       :  %d\r\n", level_can_color);
    sprintf(abuf + strlen(abuf), "max_exp_gain                 :  %d\r\n", max_exp_gain);
    sprintf(abuf + strlen(abuf), "max_exp_loss                 :  %d\r\n", max_exp_loss);
    sprintf(abuf + strlen(abuf), "max_npc_corpse_time (in tics):  %d\r\n", max_npc_corpse_time);
    sprintf(abuf + strlen(abuf), "max_pc_corpse_time (in tics) :  %d\r\n", max_pc_corpse_time);
    sprintf(abuf + strlen(abuf), "max_obj_time (in tics)       :  %d\r\n", max_obj_time);
    sprintf(abuf + strlen(abuf), "free_rent (yes no)           :  %s\r\n", YESNO(free_rent));
    sprintf(abuf + strlen(abuf), "max_obj_save                 :  %d\r\n", max_obj_save);
    sprintf(abuf + strlen(abuf), "min_rent_cost                :  %d\r\n", min_rent_cost);
    sprintf(abuf + strlen(abuf), "auto_save (yes no)           :  %s\r\n", YESNO(auto_save));
    sprintf(abuf + strlen(abuf), "autosave_time (in mins)      :  %d\r\n", autosave_time);
    sprintf(abuf + strlen(abuf), "crash_file_timeout (in days) :  %d\r\n", crash_file_timeout);
    sprintf(abuf + strlen(abuf), "rent_file_timeout (in days)  :  %d\r\n", rent_file_timeout);
    sprintf(abuf + strlen(abuf), "maxconnect                   :  %d\r\n", max_players);
    sprintf(abuf + strlen(abuf), "pracs_cost (yes no)          :  %s\r\n", YESNO(PRACS_COST));
    sprintf(abuf + strlen(abuf), "nodecay (yes no)             :  %s\r\n", YESNO(NODECAY));

    sprintf(abuf + strlen(abuf), "\r\nUsage: config <field> <value>\r\n");
    page_string(ch->desc, abuf, 1);
    return;
  }

  strcpy(val_arg, buf);
  if (!*val_arg) {
    send_to_char("Usage: config <field> <value>\r\n", ch);
    return;
  }

  if (!COM_FLAGGED(ch, COM_ADMIN)) {
    send_to_char("Maybe that's not such a great idea...\r\n", ch);
    return;
  }

  for (l = 0; *(fields[l].cmd) != '\n'; l++) {
    if (!strncmp(field, fields[l].cmd, strlen(field))) {
      break;
    }
  }

  if (*fields[l].cmd == '\n') {
    send_to_char("No such option. Type config for a list of options\r\n", ch);
    return;
  }

  if (!COM_FLAGGED(ch, fields[l].level)) {
    send_to_char("You are not godly enough for that!\r\n", ch);
    return;
  }

  if (fields[l].type == BINARY) {
    if (!strcmp(val_arg, "on") || !strcmp(val_arg, "yes")) {
      on = 1;
    } else if (!strcmp(val_arg, "off") || !strcmp(val_arg, "no")) {
      off = 1;
    }
    if (!(on || off)) {
      send_to_char("Value must be (on or off) or (yes or no).\r\n", ch);
      return;
    }
    value = on ? 1 : 0;
  } else if (fields[l].type == NUMBER) {
    value = atoi(val_arg);
  }

  switch (l) {
    case 0:
      SECS_PER_MUD_HOUR = MAX(1, value);
      SECS_PER_MUD_DAY = num_hours * SECS_PER_MUD_HOUR;
      SECS_PER_MUD_MONTH = num_days * SECS_PER_MUD_DAY;
      SECS_PER_MUD_YEAR = num_months * SECS_PER_MUD_MONTH;
      break;
    case 1:
      num_months = MAX(1, value);
      SECS_PER_MUD_DAY = num_hours * SECS_PER_MUD_HOUR;
      SECS_PER_MUD_MONTH = num_days * SECS_PER_MUD_DAY;
      SECS_PER_MUD_YEAR = num_months * SECS_PER_MUD_MONTH;
      break;
    case 2:
      num_days = MAX(1, value);
      SECS_PER_MUD_DAY = num_hours * SECS_PER_MUD_HOUR;
      SECS_PER_MUD_MONTH = num_days * SECS_PER_MUD_DAY;
      SECS_PER_MUD_YEAR = num_months * SECS_PER_MUD_MONTH;
      break;
    case 3:
      num_hours = MAX(1, value);
      SECS_PER_MUD_DAY = num_hours * SECS_PER_MUD_HOUR;
      SECS_PER_MUD_MONTH = num_days * SECS_PER_MUD_DAY;
      SECS_PER_MUD_YEAR = num_months * SECS_PER_MUD_MONTH;
      break;
    case 4:
      pk_allowed = value;
      break;
    case 5:
      pt_allowed = value;
      break;
    case 6:
      level_can_shout = BOUNDED(1, value, 51);
      break;
    case 7:
      level_can_color = BOUNDED(1, value, 51);
      break;
    case 8:
      max_exp_gain = MAX(1, value);
      break;
    case 9:
      max_exp_loss = MAX(1, value);
      break;
    case 10:
      max_npc_corpse_time = MAX(1, value);
      break;
    case 11:
      max_pc_corpse_time = MAX(1, value);
      break;
    case 12:
      max_obj_time = MAX(1, value);
      break;
    case 13:
      free_rent = value;
      break;
    case 14:
      max_obj_save = MAX(1, value);
      break;
    case 15:
      min_rent_cost = MAX(0, value);
      break;
    case 16:
      auto_save = value;
      break;
    case 17:
      autosave_time = MAX(1, value);
      break;
    case 18:
      crash_file_timeout = MAX(1, value);
      break;
    case 19:
      rent_file_timeout = MAX(1, value);
      break;
    case 20:
      max_players = MAX(1, value);
      break;
    case 21:
      PRACS_COST = value;
      break;
    case 22:
      NODECAY = value;
      break;
  }
}

ACMD (do_xname)
{
  char tempname[MAX_INPUT_LENGTH];
  char *achrp = NULL;
  int i = 0;
  FILE *fp = NULL;
  *buf = '\0';
  *buf1 = '\0';

  if (!COM_FLAGGED(ch, COM_ADMIN)) {
    send_to_char("Go Away.\r\n", ch);
    return;
  }

  one_argument(argument, buf);

  if (!*buf) {
    send_to_char("Xname who?\r\n", ch);
  }

  if (!(fp = fopen(DECLINED_FILE, "a"))) {
    perror("Cannot open declined file for update");
    return;
  }
  strcpy(tempname, buf);
  tempname[MAX_NAME_LENGTH] = '\0';
  achrp = strchr(tempname, '\n');
  if (achrp) {
    *achrp = '\0';
  }
  for (i = 0; tempname[i]; i++) {
    tempname[i] = LOWER(tempname[i]);
  }
  fprintf(fp, "%s\n", tempname);
  fclose(fp);
  sprintf(buf1, "%s has been xnamed!", tempname);
  mudlog(buf1, 'G', COM_ADMIN, TRUE);
  sprintf(buf1, "%s has been xnamed!\n", tempname);
  send_to_char(buf1, ch);
  Read_Invalid_List();
}

ACMD(do_bootmaze)
{
  char log_file[80];
  char index_file[80];
  maze m;
  FILE* f;
  FILE* index;
  int found = 0;
  int zone_num = 0;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("You must specify a zone number!\r\n", ch);
    return;
  }

  zone_num = atoi(arg);

  sprintf(log_file, "%s/%s.maz", MAZE_PREFIX, arg);
  sprintf(index_file, "%s/%s", MAZE_PREFIX, INDEX_FILE);
  index = fopen(index_file, "r");
  if (index == NULL) {
    sprintf(buf, "Could not open %s.\r\n", index_file);
    send_to_char(buf, ch);
    return;
  }

  fgets(buf, 80, index);
  while (!feof(index)) {
    buf[strlen(buf) - 1] = '\0';
    if (strcmp(buf, log_file + strlen(MAZE_PREFIX) + 1) == 0)
      found = 1;
    fgets(buf, 80, index);
  }

  fclose(index);
  if (!found) {
    sprintf(buf, "Sorry, but zone %s is not a maze.\r\n", arg);
    send_to_char(buf, ch);
    return;
  }

  f = fopen(log_file, "w");
  if (f == NULL) {
    sprintf(buf, "Could not open %s.\r\n", log_file);
    send_to_char(buf, ch);
    return;
  }

  sprintf(buf, "%s has rebuilt maze #%s.", GET_NAME(ch), arg);
  mudlog(buf, 'G', COM_QUEST, TRUE);

  create_maze(&m);
  print_maze(&m, f);
  reboot_maze(&m, zone_num);
  send_to_char("Done!\r\n", ch);
}

ACMD(do_vlist)
{
  int i, j, k, oktosee;

  two_arguments(argument, arg, buf);
  if (!*arg) {
    send_to_char("Usage: vlist <o | m | r> <zone>\r\n", ch);
    return;
  }
  if (!*buf) {
    send_to_char("You have to supply a zone number.\r\n", ch);
    return;
  }

  if (is_abbrev(arg, "room")) {
    for (j = atoi(buf), i = 0; zone_table[i].number != j && i <= top_of_zone_table; i++) {
      ;
    }
    if (i <= top_of_zone_table) {
      if (zone_table[i].number != 12 && zone_table[i].number != 30 && zone_table[i].number != 31 && !COM_FLAGGED(ch, COM_ADMIN)) {
        for (j = 0; j < 4; j++) {
          if (ch->olc_zones[j] == zone_table[i].number) {
            oktosee = 1;
          }
        }
        if (!oktosee || zone_table[i].number == 0) {
          send_to_char("You are not authorized to see info in this zone.\r\n", ch);
          return;
        }
      }
      sprintf(string_buf, "Room vnums in zone (#%d) %s:\r\n", zone_table[i].number, zone_table[i].name);
      for (j = (i ? zone_table[i - 1].top : -1) + 1; j < zone_table[i].top + 1; j++) {
        if ((k = real_room(j)) > 0) {
          sprintf(buf, "%s#%s%d%s: %s%s\r\n", CBBLU(ch, C_CMP), CBWHT(ch, C_CMP), world[k].number, CCCYN(ch, C_CMP), world[k].name, CCNRM(ch, C_NRM));
          strcat(string_buf, buf);
        }
        if (strlen(string_buf) > 39918) {
          break;
        }
      }
      page_string(ch->desc, string_buf, 1);
    } else {
      send_to_char("That is not a valid zone.\r\n", ch);
      return;
    }
  } else if (is_abbrev(arg, "mobile")) {
    for (j = atoi(buf), i = 0; zone_table[i].number != j && i <= top_of_zone_table; i++) {
      ;
    }
    if (i <= top_of_zone_table) {
      if (zone_table[i].number != 12 && zone_table[i].number != 30 && zone_table[i].number != 31 && !COM_FLAGGED(ch, COM_ADMIN)) {
        for (j = 0; j < 4; j++) {
          if (ch->olc_zones[j] == zone_table[i].number) {
            oktosee = 1;
          }
        }
        if (!oktosee || zone_table[i].number == 0) {
          send_to_char("You are not authorized to see info in this zone.\r\n", ch);
          return;
        }
      }
      sprintf(string_buf, "Mobile vnums in zone (#%d) %s:\r\n", zone_table[i].number, zone_table[i].name);
      for (j = (i ? zone_table[i - 1].top : -1) + 1; j < zone_table[i].top + 1; j++) {
        if ((k = real_mobile(j)) > 0) {
          sprintf(buf, "%s#%s%d%s: %s%s\r\n", CBBLU(ch, C_CMP), CBWHT(ch, C_CMP), mob_index[k].virtual, CCCYN(ch, C_CMP), mob_proto[k].player.short_descr, CCNRM(ch, C_NRM));
          strcat(string_buf, buf);
        }
        if (strlen(string_buf) > 39918) {
          break;
        }
      }
      page_string(ch->desc, string_buf, 1);
    } else {
      send_to_char("That is not a valid zone.\r\n", ch);
      return;
    }
  } else if (is_abbrev(arg, "object")) {
    for (j = atoi(buf), i = 0; zone_table[i].number != j && i <= top_of_zone_table; i++) {
      ;
    }
    if (i <= top_of_zone_table) {
      if (zone_table[i].number != 12 && zone_table[i].number != 30 && zone_table[i].number != 31 && !COM_FLAGGED(ch, COM_ADMIN)) {
        for (j = 0; j < 4; j++) {
          if (ch->olc_zones[j] == zone_table[i].number) {
            oktosee = 1;
          }
        }
        if (!oktosee || zone_table[i].number == 0) {
          send_to_char("You are not authorized to see info in this zone.\r\n", ch);
          return;
        }
      }
      sprintf(string_buf, "Object vnums in zone (#%d) %s:\r\n", zone_table[i].number, zone_table[i].name);
      for (j = (i ? zone_table[i - 1].top : -1) + 1; j < zone_table[i].top + 1; j++) {
        if ((k = real_object(j)) > 0) {
          sprintf(buf, "%s#%s%d%s: %s%s\r\n", CBBLU(ch, C_CMP), CBWHT(ch, C_CMP), obj_index[k].virtual, CCCYN(ch, C_CMP), obj_proto[k].short_description, CCNRM(ch, C_NRM));
          strcat(string_buf, buf);
        }
        if (strlen(string_buf) > 39918) {
          break;
        }
      }
      page_string(ch->desc, string_buf, 1);
    } else {
      send_to_char("That is not a valid zone.\r\n", ch);
      return;
    }
  } else {
    send_to_char("You have to chose either 'mobile', 'object' or 'room'\r\n", ch);
  }

  return;

}

ACMD(do_objtochar)
{

}

ACMD(do_wizupdate)
{
  char buf[100];
  sprintf(buf, "../bin/autowiz %d text/wizlist %d text/immlist 1 &", LVL_IMMORT, LVL_IMMORT);
  system(buf);
  reread_wizlists();
  sprintf(buf, "%s initiated autowiz", GET_NAME(ch));
  mudlog(buf, 'M', COM_ADMIN, FALSE);
}

ACMD(do_logcheck)
{
  FILE *fil = NULL;
  char fname[MAX_INPUT_LENGTH];
  int lvl, i;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Usage: plog <player>\r\n", ch);
    return;
  }

  if (!get_filename(arg, fname, PLOG_FILE)) {
    return;
  }

  if (!(fil = fopen(fname, "r"))) {
    if (errno != ENOENT) { /* if it fails, NOT because of no file */
      sprintf(buf1, "SYSERR: READING PLOG FILE %s (5)", fname);
      perror(buf1);
      return;
    }
    send_to_char("There is no such logfile.\r\n", ch);
    return;
  }

  sprintf(logbuffer, "%s logchecked %s", GET_NAME(ch), arg);
  mudlog(logbuffer, 'G', COM_QUEST, FALSE);
  plog(logbuffer, ch, MAX(LVL_IMMORT, GET_LEVEL(ch)));

  *string_buf = '\0';
  while (!feof(fil)) {
    *buf = '\0';
    fscanf(fil, "%d %d ", &lvl, &i);
    fgets(buf, MAX_INPUT_LENGTH, fil);
    if (GET_LEVEL(ch) >= lvl) {
      strcat(string_buf, buf);
    }
    if (strlen(string_buf) > 39500) {
      page_string(ch->desc, string_buf, 1);
      string_buf[0] = '\0';
    }
  }
  fclose(fil);
  page_string(ch->desc, string_buf, 1);
}

ACMD(do_fpurge)
{
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Purge what files??\r\n", ch);
    return;
  }

  switch (subcmd) {
    case SCMD_PRENT:
      Crash_delete_file(arg);
      sprintf(buf, "%s purged %s's rentfile.", GET_NAME(ch), arg);
      mudlog(buf, 'G', COM_ADMIN, TRUE);
      plog(buf, ch, MAX(LVL_IMMORT, GET_LEVEL(ch)));
      send_to_char("Rentfile purged.\r\n", ch);
      break;
    case SCMD_PTEXT:
      Crash_delete_text(arg);
      sprintf(buf, "%s purged %s's textfile.", GET_NAME(ch), arg);
      mudlog(buf, 'G', COM_ADMIN, TRUE);
      plog(buf, ch, MAX(LVL_IMMORT, GET_LEVEL(ch)));
      send_to_char("Textfile purged.\r\n", ch);
      break;
    default:
      return;
  }
}

ACMD(do_strings) /* shows some basic strings for immos */
{
  struct char_data *tch;
  one_argument(argument, arg);

  if (*arg) {
    tch = get_char_vis(ch, arg);
    if (!tch || IS_NPC(tch)) {
      send_to_char("Show strings of who?\r\n", ch);
      return;
    }
  } else {
    tch = ch;
  }
  sprintf(buf, "Who setting: %s\r\n"
      "Poof-in    : %s\r\n"
      "Poof-out   : %s\r\n"
      "Title      : %s\r\n"
      "Description: %s\r\n", WHOSPEC(tch) ? WHOSPEC(tch) : "NONE", POOFIN(tch) ? POOFIN(tch) : "NONE", POOFOUT(tch) ? POOFOUT(tch) : "NONE", GET_TITLE(tch) ? GET_TITLE(tch) : "NONE", GET_DESC(tch) ? GET_DESC(tch) : "NONE");
  send_to_char(buf, ch);
}

ACMD(do_frent) /* force a player to rent, for use with animate */
{
  struct char_data *vict;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Force-rent who??\r\n", ch);
    return;
  }

  if (!(vict = get_char_vis(ch, arg)) || GET_MOB_RNUM(vict) != -1) {
    send_to_char("Who is that?\r\n", ch);
    return;
  }

  if (!GET_INVIS_LEV(vict)) {
    act("$n has left the game.", TRUE, vict, 0, 0, TO_ROOM);
  }
  sprintf(buf, "%s was force-rented by %s.", GET_NAME(vict), GET_NAME(ch));
  mudlog(buf, 'R', COM_ADMIN, TRUE);
  plog(buf, vict, 0);
  Crash_save(vict, RENT_FORCED);
  extract_char(vict, 0); /* Char is saved in extract char */
}

ACMD(do_animate)
{

  struct char_data *vict;

  one_argument(argument, arg);
  if (!*arg) {
    send_to_char("Which player do you want to animate?\r\n", ch);
    return;
  }

  CREATE(vict, struct char_data, 1);
  clear_char(vict);
  if (load_char_text(arg, vict) > 0) { /* load player */
    vict->desc = NULL;
    GET_MOB_RNUM(vict) = -1; /* this is not a regular NPC */
    reset_char(vict);
    vict->next = character_list;
    character_list = vict;
    Crash_load(vict);
    /*
     if (has_mail(GET_IDNUM(vict)))
     act("$N has mail waiting.", FALSE, ch, 0, vict, TO_CHAR);
     */
    char_to_room(vict, ch->in_room);
    act("$n gestures and a ghost of $N appears.", TRUE, ch, 0, vict, TO_ROOM);
    act("You have created a ghost of $N.", FALSE, ch, 0, vict, TO_CHAR);
    sprintf(buf, "%s animated %s", GET_NAME(ch), GET_NAME(vict));
    mudlog(buf, 'G', COM_ADMIN, TRUE);
    plog(buf, ch, MAX(LVL_IMMORT, GET_LEVEL(ch)));
    plog(buf, vict, MAX(LVL_IMMORT, GET_LEVEL(ch)));
  } else { /* no such player */
    send_to_char("Cannot find that player in the database.\r\n", ch);
    FREE(vict);
  }
}
/*
 ACMD(do_clone)
 {
 char **msg;
 struct obj_data *item = NULL;
 struct char_data *vict = NULL;
 struct char_data *mob;
 struct obj_data *obj;

 two_arguments(argument, buf, arg);

 if (!*arg) {
 send_to_char("Usage: clone { obj | mob } <name>\r\n", ch);
 return; }

 if (!*buf) {
 send_to_char("What do you want to clone?\r\n", ch);
 return; }

 generic_find(arg, FIND_OBJ_WORLD | FIND_OBJ_ROOM | FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_CHAR_ROOM | FIND_CHAR_WORLD, ch, &vict, &item);

 if (is_abbrev(buf, "mob")) {
 if (vict == NULL) {
 send_to_char("No mobiles by that name.\r\n", ch);
 return; }
 if (IS_NPC(vict) && GET_MOB_SPEC(vict)) {
 send_to_char("Cloning of mobiles with spec_procs has been temporarily disabled.\r\n",ch);
 return; }
 if (!IS_NPC(vict)) {
 send_to_char("You are not allowed to clone players.\r\n", ch);
 return; }
 mob = read_mobile(vict->nr, REAL);
 if (mob->player.name) {
 msg = &mob->player.name;
 *msg = strdup(vict->player.name); }
 if (mob->player.short_descr) {
 msg = &mob->player.short_descr;
 *msg = strdup(vict->player.short_descr); }
 if (mob->player.long_descr) {
 msg = &mob->player.long_descr;
 *msg  = strdup(vict->player.long_descr); }
 if (GET_DESC(mob)) {
 msg = &(GET_DESC(mob));
 *msg = strdup(vict->player.description); }
 if (mob->player.title) {
 msg = &mob->player.title;
 *msg = strdup(vict->player.title); }
 ** add more cloning stuff here later, according to medit **
 char_to_room(mob, ch->in_room);
 act("$n makes some magical gestures with $s hands.", TRUE, ch,
 0, 0, TO_ROOM);
 act("$n has cloned $N!", FALSE, ch, 0, mob, TO_ROOM);
 act("You clone $N.", FALSE, ch, 0, mob, TO_CHAR);
 sprintf(buf, "%s cloned %s at %s", GET_NAME(ch), GET_NAME(mob), world[ch->in_room].name);
 mudlog(buf, 'L', COM_QUEST, FALSE);
 plog(buf, ch, MAX(LVL_IMMORT, GET_LEVEL(ch)));
 return;
 }

 if (is_abbrev(buf, "obj")) {
 if (item == NULL) {
 send_to_char("No objects by that name.\r\n", ch);
 return; }
 if (GET_OBJ_SPEC(item)) {
 send_to_char("Cloning of objects with spec_procs has been temporarily disabled.\r\n",ch);
 return; }
 obj = read_object(item->item_number, REAL);
 if(obj == NULL) {
 send_to_char("Cloning failure, original is probably a QIC over the limit.\r\n",ch);
 return; }
 if (obj->name) {
 msg = &obj->name;
 *msg = strdup(item->name); }
 if (obj->description) {
 msg = &obj->description;
 *msg = strdup(item->description); }
 if (obj->short_description) {
 msg = &obj->short_description;
 *msg = strdup(item->short_description); }
 if (obj->action_description) {
 msg = &obj->action_description;
 *msg = strdup(item->action_description); }
 obj_to_room(obj, ch->in_room);
 act("$n makes a strange, fascinating gesture.", TRUE, ch, 0, 0, TO_ROOM);
 act("$n has cloned $p!", FALSE, ch, obj, 0, TO_ROOM);
 act("You cloned $p.", FALSE, ch, obj, 0, TO_CHAR);
 sprintf(buf, "%s cloned %s at %s", GET_NAME(ch), obj->short_description, world[ch->in_room].name);
 mudlog(buf, 'L', COM_QUEST, FALSE);
 plog(buf, ch, MAX(LVL_IMMORT, GET_LEVEL(ch)));
 return;
 }

 send_to_char("You have to chose either 'obj' or 'mob'.\r\n", ch);

 }
 */

ACMD(do_kit)
{
  /* okay.. 11 items for each class...
   weapon, shield, legs, head, body, feet, water, food, food, food, bag
   */

  int kits[NUM_CLASSES][11] = {
      {3022, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032}, /* Warrior */
      {3020, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032}, /* Rogue */
      {3020, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032}, /* Thief */
      {3058, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032}, /* Sorcerer*/
      {3058, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032}, /* Wizard */
      {3058, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032}, /* Enchanter */
      {3058, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032}, /* Conjurer */
      {3058, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032}, /* Necromancer */
      {3058, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032}, /* Cleric */
      {3058, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032}, /* Priest */
      {3058, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032}, /* Shaman */
      {3058, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032}, /* Monk */
      {3058, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032}, /* Druid */
      {3020, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032}, /* Assassian */
      {3058, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032}, /* Bard */
      {3022, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032}, /* Ranger */
      {3020, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032}, /* Mercenary */
  };
  int i;
  struct char_data *vict;
  struct obj_data *obj;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Give a newbie kit to who?\r\n", ch);
    return;
  }

  if (!(vict = get_char_vis(ch, arg)) || IS_NPC(vict)) {
    send_to_char("Who is that?\r\n", ch);
    return;
  }

  if (PLR_FLAGGED(vict, PLR_KIT)) {
    send_to_char("Someone has already given that player a newbie kit.\r\n", ch);
    return;
  } else {
    SET_BIT(PLR_FLAGS(vict), PLR_KIT);
  }

  /* Okay... time to give the kit to the player.. */
  for (i = 0; i < 11; i++) {
    obj = read_object(kits[(int) GET_CLASS(vict)][i], VIRTUAL);
    if (obj == NULL) {
      mudlog("Error! QIC/buggy object in newbie kit!", 'E', COM_IMMORT, FALSE);
      return;
    }
    SET_BIT(GET_OBJ_EXTRA(obj), ITEM_CARRIED);
    /* newbie items flagged as carried */
    obj_to_char(obj, vict);
  }

  act("You give $N a newbie kit.", FALSE, ch, 0, vict, TO_CHAR);
  act("$n gives you a newbie kit.", TRUE, ch, 0, vict, TO_VICT);
  act("$N gives $n a newbie kit.", TRUE, vict, 0, ch, TO_NOTVICT);
  sprintf(buf, "%s gave a newbie kit to %s", GET_NAME(ch), GET_NAME(vict));
  mudlog(buf, 'G', COM_IMMORT, FALSE);
  plog(buf, ch, LVL_IMMORT);
}

ACMD(do_echo)
{
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char("Yes.. but what?\r\n", ch);
  } else {
    if (subcmd == SCMD_EMOTE) {
      if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_NOEMOTE)) {
        send_to_char("You are so full of emotions, if you only knew how to express them.\r\n.", ch);
        return;
      }
      if (GET_LEVEL(ch) < LVL_IMMORT && strlen(argument) > 78) {
        send_to_char("That's too long... Try something shorter.\r\n", ch);
        return;
      } else {
        sprintf(buf, "$n %s", argument);
      }
    } else {
      strcpy(buf, argument);
    }
    MOBTrigger = FALSE;
    act(buf, FALSE, ch, 0, 0, TO_ROOM);
    if (PRF_FLAGGED(ch, PRF_NOREPEAT)) {
      send_to_char(OK, ch);
    } else {
      MOBTrigger = FALSE;
      act(buf, FALSE, ch, 0, 0, TO_CHAR);
    }
  }
}

ACMD(do_send)
{
  struct char_data *vict;

  half_chop(argument, arg, buf);

  if (!*arg) {
    send_to_char("Send what to who?\r\n", ch);
    return;
  }
  if (!(vict = get_char_vis(ch, arg))) {
    send_to_char(NOPERSON, ch);
    return;
  }
  send_to_char(buf, vict);
  send_to_char("\r\n", vict);
  if (PRF_FLAGGED(ch, PRF_NOREPEAT)) {
    send_to_char("Sent.\r\n", ch);
  } else {
    sprintf(buf2, "You send '%s' to %s.\r\n", buf, GET_NAME(vict));
    send_to_char(buf2, ch);
  }
}

/* take a string, and return an rnum.. used for goto, at, etc.  -je 4/6/93 */
sh_int find_target_room(struct char_data * ch, char *rawroomstr)
{
  int tmp;
  sh_int location;
  struct char_data *target_mob;
  struct obj_data *target_obj;
  char roomstr[MAX_INPUT_LENGTH];

  one_argument(rawroomstr, roomstr);

  if (!*roomstr) {
    send_to_char("You must supply a room number or name.\r\n", ch);
    return NOWHERE;
  }
  if (isdigit(*roomstr) && !strchr(roomstr, '.')) {
    tmp = atoi(roomstr);
    if ((location = real_room(tmp)) < 0) {
      send_to_char("No room exists with that number.\r\n", ch);
      return NOWHERE;
    }
  } else if ((target_mob = get_char_vis(ch, roomstr))) {
    location = target_mob->in_room;
  } else if ((target_obj = get_obj_vis(ch, roomstr))) {
    if (target_obj->in_room != NOWHERE) {
      location = target_obj->in_room;
    } else {
      send_to_char("That object is not available.\r\n", ch);
      return NOWHERE;
    }
  } else {
    send_to_char("No such creature or object around.\r\n", ch);
    return NOWHERE;
  }

  /* a location has been found -- if you're < GRGOD, check restrictions. */
  if (!COM_FLAGGED(ch, COM_ADMIN)) {
    if (ROOM_FLAGGED(location, ROOM_GODROOM)) {
      send_to_char("You are not godly enough to use that room!\r\n", ch);
      return NOWHERE;
    }
    if (ROOM_FLAGGED(location, ROOM_PRIVATE) && world[location].people && world[location].people->next_in_room) {
      send_to_char("There's a private conversation going on in that room.\r\n", ch);
      return NOWHERE;
    }
  }
  return location;
}

ACMD(do_at)
{
  char command[MAX_INPUT_LENGTH];
  int location, original_loc;

  half_chop(argument, buf, command);
  if (!*buf) {
    send_to_char("You must supply a room number or a name.\r\n", ch);
    return;
  }

  if (!*command) {
    send_to_char("What do you want to do there?\r\n", ch);
    return;
  }

  if ((location = find_target_room(ch, buf)) < 0) {
    return;
  }

  if (!check_access(ch, location)) {
    send_to_char("That is a restricted zone, no peeking!\r\n", ch);
    return;
  }

  /* a location has been found. */
  original_loc = ch->in_room;
  char_from_room(ch);
  char_to_room(ch, location);
  command_interpreter(ch, command);

  /* check if the char is still there */
  if (ch->in_room == location) {
    char_from_room(ch);
    char_to_room(ch, original_loc);
  }
}

ACMD(do_goto)
{
  sh_int location;

  one_argument(argument, arg);
  if (PLR_FLAGGED(ch, PLR_LOADROOM) && strcasecmp(arg, "home") == 0) {
    location = real_room(GET_LOADROOM(ch));
  } else if ((location = find_target_room(ch, arg)) < 0) {
    return;
  }

  if (!check_access(ch, location)) {
    send_to_char("That is a restricted zone, stay out!\r\n", ch);
    return;
  }

  if (POOFOUT(ch)) {
    sprintf(buf, "%s", POOFOUT(ch));
  } else {
    strcpy(buf, "$n disappears in a puff of smoke.");
  }

  act(buf, TRUE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, location);

  if (POOFIN(ch)) {
    sprintf(buf, "%s", POOFIN(ch));
  } else {
    strcpy(buf, "$n appears with an ear-splitting bang.");
  }

  act(buf, TRUE, ch, 0, 0, TO_ROOM);
  look_at_room(ch, 0);
}

ACMD(do_trans)
{
  struct descriptor_data *i;
  struct char_data *victim;
  struct char_data *member;
  struct char_data *member2;
  struct follow_type *ftype;
  struct follow_type *ftype2;
  char *buf2;
  char *buf;

  buf = argument;
  buf2 = one_argument(argument, buf);
  if (!*buf) {
    send_to_char("Whom do you wish to transfer?\r\n", ch);
  } else if (*buf == '-') {
    buf++;
    switch (*buf) {
      case 'g':
        if (!*buf2) {
          send_to_char("You must specify a group leader or member or a mob with followers.\r\n", ch);
          return;
        }
        buf2++;
        if (!(victim = get_char_vis(ch, buf2))) {
          send_to_char(NOPERSON, ch);
          return;
        }

        if ((victim->master && (victim->master == ch->master)) || (ch == victim->master) || (victim == ch->master)) {
          send_to_char("You can't transfer your own group./r/n", ch);
          return;
        }

        if (IS_NPC(victim)) {
          int oktosee = 0;
          int zonenum = 0;
          int n;

          zonenum = (GET_MOB_VNUM(victim) - (GET_MOB_VNUM(victim) % 100)) / 100;
          if (zonenum != 12 && zonenum != 30 && zonenum != 31 && !COM_FLAGGED(ch, COM_ADMIN)) {
            for (n = 0; n < 4; n++) {
              if (ch->olc_zones[n] == zonenum) {
                oktosee = 1;
              }
            }
            if (!oktosee || zonenum == 0) {
              send_to_char("You are not authorized to see info in this zone.\r\n", ch);
              return;
            }
          }
          if (victim->followers == NULL) {
            send_to_char("That mob has no followers.\r\n", ch);
            return;
          }
          act("$n and $s followers disappear in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
          char_from_room(victim);
          char_to_room(victim, ch->in_room);
          act("$n appears in a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
          for (ftype = victim->followers; ftype; ftype = ftype->next) {
            member = ftype->follower;
            if (IS_NPC(member)) {
              char_from_room(member);
              char_to_room(member, ch->in_room);
              act("$n appears in a puff of smoke.", FALSE, member, 0, 0, TO_ROOM);
            }
          }
          sprintf(buf, "%s transferred %s and followers to %s", GET_NAME(ch), GET_NAME(victim), world[ch->in_room].name);
          mudlog(buf, 'G', COM_QUEST, FALSE);
          plog(buf, ch, MAX(LVL_IMMORT, GET_LEVEL(ch)));
          return;
        }
        if (IS_IMMO(victim)) {
          int oktosee = 0;
          int n;

          for (n = 0; n < 4; n++) {
            if (IN_ZONE(ch) == 12 || IN_ZONE(ch) == 30 || IN_ZONE(ch) == 31 || victim->olc_zones[n] == IN_ZONE(ch)) {
              oktosee = 1;
            }
          }
          if (!oktosee) {
            send_to_char("They are not authorized to see info in this zone.\r\n", ch);
            return;
          }
        }

        if (victim->master && IS_AFFECTED(victim, AFF_GROUP)) {
          victim = victim->master;
        }
        if ((victim->followers && !victim->master) || (IS_AFFECTED(victim, AFF_GROUP) && !victim->master)) {
          act("$n and $s group disappear in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
          send_to_room(victim->player.name, (int) ch->in_room);
          send_to_room(" and their group arrive in a puff of smoke.\r\n", (int) ch->in_room);
          char_from_room(victim);
          char_to_room(victim, ch->in_room);
          act("$n has transferred you and your group!\r\n", FALSE, ch, 0, victim, TO_VICT);
          look_at_room(victim, 0);
          for (ftype = victim->followers; ftype; ftype = ftype->next) {
            member = ftype->follower;
            if (IS_NPC(member) || IS_AFFECTED(member, AFF_GROUP)) {
              char_from_room(member);
              char_to_room(member, ch->in_room);
              send_to_char("You are pulled along!\r\n\r\n", member);
              look_at_room(member, 0);
              if (!IS_NPC(member)) {
                for (ftype2 = member->followers; ftype2; ftype2 = ftype2->next) {
                  member2 = ftype2->follower;
                  if (IS_NPC(member2)) {
                    char_from_room(member2);
                    char_to_room(member2, ch->in_room);
                  }
                }
              }
            }
          }
          sprintf(buf, "%s transferred %s and their group to %s", GET_NAME(ch), GET_NAME(victim), world[ch->in_room].name);
          mudlog(buf, 'G', COM_QUEST, FALSE);
          plog(buf, ch, MAX(LVL_IMMORT, GET_LEVEL(ch)));
        } else {
          send_to_char("That person is not grouped and has no followers.\r\n", ch);
        }
        return;

      default:
        send_to_char("Unknown option.\r\n", ch);
        return;
    }
  } else if (str_cmp("all", buf)) {
    if (!(victim = get_char_vis(ch, buf))) {
      send_to_char(NOPERSON, ch);
    } else if (victim == ch) {
      send_to_char("That doesn't make much sense, does it?\r\n", ch);
    } else {
      int oktosee = 0;
      int zonenum = 0;
      int n;

      if (IS_NPC(victim)) {
        zonenum = (GET_MOB_VNUM(victim) - (GET_MOB_VNUM(victim) % 100)) / 100;
        if (zonenum != 12 && zonenum != 30 && zonenum != 31 && !COM_FLAGGED(ch, COM_ADMIN)) {
          for (n = 0; n < 4; n++) {
            if (ch->olc_zones[n] == zonenum) {
              oktosee = 1;
            }
          }
          if (!oktosee || zonenum == 0) {
            send_to_char("You are not authorized to see info in this zone.\r\n", ch);
            return;
          }
        }
      }
      if ((GET_LEVEL(ch) < GET_LEVEL(victim)) && !IS_NPC(victim)) {
        send_to_char("Go transfer someone your own size.\r\n", ch);
        return;
      }
      act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
      char_from_room(victim);
      char_to_room(victim, ch->in_room);
      act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
      act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
      sprintf(buf, "%s transferred %s to %s", GET_NAME(ch), GET_NAME(victim), world[ch->in_room].name);
      mudlog(buf, 'G', COM_QUEST, FALSE);
      plog(buf, ch, MAX(LVL_IMMORT, GET_LEVEL(ch)));
      look_at_room(victim, 0);
    }
  } else { /* Trans All */
    if (!COM_FLAGGED(ch, COM_BUILDER)) {
      send_to_char("I think not.\r\n", ch);
      return;
    }
    sprintf(buf, "%s transferred all to %s", GET_NAME(ch), world[ch->in_room].name);
    mudlog(buf, 'G', COM_QUEST, FALSE);
    plog(buf, ch, MAX(LVL_IMMORT, GET_LEVEL(ch)));
    for (i = descriptor_list; i; i = i->next)
      if (!i->connected && i->character && i->character != ch) {
        victim = i->character;
        if (GET_LEVEL(victim) >= GET_LEVEL(ch)) {
          continue;
        }
        act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
        char_from_room(victim);
        char_to_room(victim, ch->in_room);
        act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
        act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
        look_at_room(victim, 0);
      }
    send_to_char(OK, ch);
  }
}

ACMD(do_teleport)
{
  struct char_data *victim;
  sh_int target;

  two_arguments(argument, buf, buf2);

  if (!*buf) {
    send_to_char("Whom do you wish to teleport?\r\n", ch);
  } else if (!(victim = get_char_vis(ch, buf))) {
    send_to_char(NOPERSON, ch);
  } else if (victim == ch) {
    send_to_char("Use 'goto' to teleport yourself.\r\n", ch);
  } else if ((GET_LEVEL(victim) >= GET_LEVEL(ch)) && !COM_FLAGGED(ch, COM_ADMIN) && !IS_NPC(victim)) {
    send_to_char("Maybe you shouldn't do that.\r\n", ch);
  } else if (!*buf2) {
    send_to_char("Where do you wish to send this person?\r\n", ch);
  } else if ((target = find_target_room(ch, buf2)) >= 0) {
    if (!check_access(ch, target)) {
      send_to_char("You are not allowed to teleport people into restricted zones.\r\n", ch);
      return;
    }
    send_to_char(OK, ch);
    act("$n disappears in a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
    char_from_room(victim);
    char_to_room(victim, target);
    act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
    act("$n has teleported you!", FALSE, ch, 0, (char *) victim, TO_VICT);
    look_at_room(victim, 0);
    sprintf(logbuffer, "%s teleported %s to %s", GET_NAME(ch), GET_NAME(victim), world[victim->in_room].name);
    mudlog(logbuffer, 'G', COM_QUEST, TRUE);
  }
}

ACMD(do_vnum)
{
  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2 || (!is_abbrev(buf, "type") && (!is_abbrev(buf, "mob") && !is_abbrev(buf, "obj")))) {
    send_to_char("Usage: vnum {{ obj | mob } <name>\r\n", ch);
    return;
  }
  if (is_abbrev(buf, "mob")) {
    if (!vnum_mobile(buf2, ch)) {
      send_to_char("No mobiles by that name.\r\n", ch);
    } else {
      sprintf(logbuffer, "%s did a 'vnum m %s'", GET_NAME(ch), buf2);
      mudlog(logbuffer, 'X', COM_QUEST, FALSE);
    }
  }

  if (is_abbrev(buf, "obj")) {
    if (!vnum_object(buf2, ch)) {
      send_to_char("No objects by that name.\r\n", ch);
    } else {
      sprintf(logbuffer, "%s did a 'vnum o %s'", GET_NAME(ch), buf2);
      mudlog(logbuffer, 'X', COM_QUEST, FALSE);
    }
  }

  if (is_abbrev(buf, "type")) {
    if (!COM_FLAGGED(ch, COM_ADMIN)) {
      send_to_char("Usage: vnum { obj | mob } <name>\r\n", ch);
      return;
    }
    if (!vnum_type(buf2, ch)) {
      send_to_char("No objects by that type.\r\n", ch);
    } else {
      sprintf(logbuffer, "%s did a 'vnum t %s'", GET_NAME(ch), buf2);
      mudlog(logbuffer, 'X', COM_QUEST, FALSE);
    }
  }
}

void do_stat_room(struct char_data * ch)
{
  struct extra_descr_data *desc;
  struct room_data *rm = &world[ch->in_room];
  int i, found = 0;
  struct obj_data *j = 0;
  struct char_data *k = 0;
  int oktosee = 0;
  int zonenum = 0;

  zonenum = (rm->number - (rm->number % 100)) / 100;
  if (zonenum != 12 && zonenum != 30 && zonenum != 31 && !COM_FLAGGED(ch, COM_ADMIN)) {
    for (i = 0; i < 4; i++) {
      if (ch->olc_zones[i] == zonenum) {
        oktosee = 1;
      }
    }
    if (!oktosee || zonenum == 0) {
      send_to_char("You are not authorized to see info in this zone.\r\n", ch);
      return;
    }
  }
  sprintf(logbuffer, "%s did a 'stat room' on #%d (%s)", GET_NAME(ch), world[ch->in_room].number, world[ch->in_room].name);
  mudlog(logbuffer, 'X', COM_QUEST, FALSE);

  sprintf(buf, "Room name: %s%s%s\r\n", CCCYN(ch, C_NRM), rm->name, CCNRM(ch, C_NRM));
  send_to_char(buf, ch);

  sprinttype(rm->sector_type, sector_types, buf2);
  sprintf(buf, "Zone: [%3d], VNum: [%s%5d%s], RNum: [%5d], Type: %s, Access: ", rm->zone, CCGRN(ch, C_NRM), rm->number, CCNRM(ch, C_NRM), ch->in_room, buf2);
  if (IS_SET(zone_table[world[ch->in_room].zone].bits, ZONE_RESTRICTED)) {
    strcat(buf, "DENIED\r\n");
  } else {
    strcat(buf, "FREE\r\n");
  }

  send_to_char(buf, ch);

  sprintbit((long) rm->room_flags, room_bits, buf2);
  sprintf(buf, "SpecProc: %s, Flags: %s\r\n", (rm->func == NULL) ? "None" : "Exists", buf2);
  send_to_char(buf, ch);
  sprintf(buf, "Light: %d, Ambient Light: %d\r\n", rm->light, GET_ZONE_LIGHT(rm->zone));
  send_to_char(buf, ch);

  send_to_char("Description:\r\n", ch);
  if (rm->description) {
    send_to_char(rm->description, ch);
  } else {
    send_to_char("  None.\r\n", ch);
  }

  if (rm->ex_description) {
    sprintf(buf, "Extra descs:%s", CCCYN(ch, C_NRM));
    for (desc = rm->ex_description; desc; desc = desc->next) {
      strcat(buf, " ");
      strcat(buf, desc->keyword);
    }
    strcat(buf, CCNRM(ch, C_NRM));
    send_to_char(strcat(buf, "\r\n"), ch);
  }

  if (!COM_FLAGGED(ch, COM_QUEST)) { /* limit room stat for lowlevels */
    return;
  }

  sprintf(buf, "Chars present:%s", CCYEL(ch, C_NRM));
  for (found = 0, k = rm->people; k; k = k->next_in_room) {
    if (!CAN_SEE(ch, k)) {
      continue;
    }
    sprintf(buf2, "%s %s(%s)", found++ ? "," : "", GET_NAME(k), (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")));
    strcat(buf, buf2);
    if (strlen(buf) >= 62) {
      if (k->next_in_room) {
        send_to_char(strcat(buf, ",\r\n"), ch);
      } else {
        send_to_char(strcat(buf, "\r\n"), ch);
      }
      *buf = found = 0;
    }
  }

  if (*buf) {
    send_to_char(strcat(buf, "\r\n"), ch);
  }
  send_to_char(CCNRM(ch, C_NRM), ch);

  if (rm->contents) {
    sprintf(buf, "Contents:%s", CCGRN(ch, C_NRM));
    for (found = 0, j = rm->contents; j; j = j->next_content) {
      if (!CAN_SEE_OBJ(ch, j)) {
        continue;
      }
      sprintf(buf2, "%s %s", found++ ? "," : "", j->short_description);
      strcat(buf, buf2);
      if (strlen(buf) >= 62) {
        if (j->next_content) {
          send_to_char(strcat(buf, ",\r\n"), ch);
        } else {
          send_to_char(strcat(buf, "\r\n"), ch);
        }
        *buf = found = 0;
      }
    }

    if (*buf) {
      send_to_char(strcat(buf, "\r\n"), ch);
    }
    send_to_char(CCNRM(ch, C_NRM), ch);
  }
  for (i = 0; i < NUM_OF_DIRS; i++) {
    if (rm->dir_option[i]) {
      if (rm->dir_option[i]->to_room == NOWHERE) {
        sprintf(buf1, " %sNONE%s", CCCYN(ch, C_NRM), CCNRM(ch, C_NRM));
      } else {
        sprintf(buf1, "%s%5d%s", CCCYN(ch, C_NRM), world[rm->dir_option[i]->to_room].number, CCNRM(ch, C_NRM));
      }
      sprintbit(rm->dir_option[i]->exit_info, exit_bits, buf2);
      sprintf(buf, "Exit %s%-5s%s:  To: [%s], Key: [%5d], Keywrd: %s, Type: %s\r\n ", CCCYN(ch, C_NRM), dirs[i], CCNRM(ch, C_NRM), buf1, rm->dir_option[i]->key, rm->dir_option[i]->keyword ? rm->dir_option[i]->keyword : "None", buf2);
      send_to_char(buf, ch);
      if (rm->dir_option[i]->general_description) {
        strcpy(buf, rm->dir_option[i]->general_description);
      } else {
        strcpy(buf, "  No exit description.\r\n");
      }
      send_to_char(buf, ch);
    }
  }
}

void do_stat_object(struct char_data * ch, struct obj_data * j)
{
  int i, virtual, found;
  struct obj_data *j2;
  struct extra_descr_data *desc;
  char tname[50];
  int oktosee = 0;
  int zonenum = 0;

  virtual = GET_OBJ_VNUM(j);

  zonenum = (virtual - (virtual % 100)) / 100;
  if (zonenum != 12 && zonenum != 30 && zonenum != 31 && !COM_FLAGGED(ch, COM_ADMIN)) {
    for (i = 0; i < 4; i++) {
      if (ch->olc_zones[i] == zonenum) {
        oktosee = 1;
      }
    }
    if (!oktosee || zonenum == 0) {
      send_to_char("You are not authorized to see info in this zone.\r\n", ch);
      return;
    }
  }

  sprintf(logbuffer, "%s did a 'stat' on object #%d (%s)", GET_NAME(ch), virtual, (j->cshort_description ? j->cshort_description : j->short_description));
  mudlog(logbuffer, 'X', COM_QUEST, FALSE);

  sprintf(buf, "{BName: '%s%s%s'{B, Aliases: %s\r\n{x", CCYEL(ch, C_NRM), (j->cshort_description ? j->short_description : ((j->short_description) ? j->short_description : "<None>")), CCNRM(ch, C_NRM), j->name);
  send_to_char(buf, ch);
  sprinttype(GET_OBJ_TYPE(j), item_types, buf1);
  if (GET_OBJ_RNUM(j) >= 0) {
    strcpy(buf2, (obj_index[GET_OBJ_RNUM(j)].func ? "Exists" : "None"));
  } else {
    strcpy(buf2, "None");
  }
  sprintf(buf, "{BVNum: [%s%5d%s{B], RNum: [{g%5d{B], Type: {g%s{B, SpecProc: {g%s", CCGRN(ch, C_NRM), virtual, CCNRM(ch, C_NRM), GET_OBJ_RNUM(j), buf1, buf2);
  if (GET_OBJ_RNUM(j) >= 0 && obj_index[GET_OBJ_RNUM(j)].qic != NULL && COM_FLAGGED(ch, COM_ADMIN)) {
    sprintf(buf2, " {RQIC: {W%d({w%d){x\r\n", obj_index[GET_OBJ_RNUM(j)].qic->items, obj_index[GET_OBJ_RNUM(j)].qic->limit);
    strcat(buf, buf2);
  } else {
    strcat(buf, "\r\n");
  }
  send_to_char(buf, ch);
  sprintf(buf, "{BL-Des: {x%s\r\n", ((j->description) ? j->description : "None"));
  send_to_char(buf, ch);

  if (j->ex_description) {
    sprintf(buf, "{BExtra descs:{c");
    for (desc = j->ex_description; desc; desc = desc->next) {
      sprintf(buf + strlen(buf), " %s", (desc->keyword ? desc->keyword : "NONE"));
    }
    sprintf(buf + strlen(buf), "{x\r\n");
    send_to_char(buf, ch);
  }
  send_to_char("{BCan be worn on:{c ", ch);
  sprintbit(GET_OBJ_WEAR(j), wear_bits, buf);
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  send_to_char("{BSet char bits :{c ", ch);
  sprintbit(GET_OBJ_BITV(j), affected_bits, buf);
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  send_to_char("{BSet char bits2 :{c ", ch);
  sprintbit(GET_OBJ_BITV2(j), affected_bits2, buf);
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  send_to_char("{BExtra flags   :{c ", ch);
  sprintbit(GET_OBJ_EXTRA(j), extra_bits, buf);
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  sprintf(buf, "{BWeight: {c%d{B, Value: {c%d{B, Timer: {c%d{B, Material:{c", GET_OBJ_WEIGHT(j), GET_OBJ_COST(j), GET_OBJ_TIMER(j));
  sprinttype(GET_OBJ_VAL(j, 4), material_types, buf2);
  sprintf(buf + strlen(buf), " %s\r\n", buf2);
  send_to_char(buf, ch);

  strcpy(buf, "{BIn room:{c ");
  if (j->in_room == NOWHERE) {
    strcat(buf, "Nowhere");
  } else {
    sprintf(buf + strlen(buf), "%d", world[j->in_room].number);
  }
  sprintf(buf + strlen(buf),
      "{B, In object:{c %s"
      "{B, Carried by:{c %s"
      "{B, Worn by:{c %s\r\n", (j->in_obj ? j->in_obj->short_description : "None"), (j->carried_by ? GET_NAME(j->carried_by) : "Nobody"), (j->worn_by ? GET_NAME(j->worn_by) : "Nobody"));
  send_to_char(buf, ch);
  sprintf(buf, "{BItems in game: {c%d{B, Items in rent at boot: {c%d{B, Spec vals: [{c%d{B] [{c%d{B] [{c%d{B]{x\r\n", obj_index[GET_OBJ_RNUM(j)].number, obj_index[GET_OBJ_RNUM(j)].rent,

  j->spec_vars[0], j->spec_vars[1], j->spec_vars[2]);
  send_to_char(buf, ch);

  switch (GET_OBJ_TYPE(j)) {
    case ITEM_LIGHT:
      sprintf(buf, "{BColor: [{c%d{B], Type: [{c%d{B], Hours: [{c%d{B]{x", GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2));
      break;
    case ITEM_SCROLL:
    case ITEM_POTION:
      sprintf(buf, "{BSpells ({blvl-%d{B):", GET_OBJ_VAL(j, 0));
      for (i = 1; i < 4; i++) {
        if (GET_OBJ_VAL(j, i)) {
          sprintf(buf + strlen(buf), "\r\n   {c%s{x", get_spell_name(GET_OBJ_VAL(j,i)));
        } else {
          sprintf(buf + strlen(buf), "\r\n   {c<NONE>{x");
        }
      }
      break;
    case ITEM_WAND:
    case ITEM_STAFF:
      sprintf(buf, "{BSpell: {c%s{B, Charges: {c%d{B({C%d{B){x", get_spell_name(GET_OBJ_VAL(j, 3)), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 1));
      break;
    case ITEM_FIREWEAPON:
    case ITEM_WEAPON:
      sprintf(buf, "{BSkill: {c");
      sprinttype(GET_OBJ_VAL(j, 0), weapon_handed, buf2);
      strcat(buf, buf2);
      sprintf(buf2, "{B, Todam: {R%d{Wd{R%d{B, Type:{c ", GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2));
      strcat(buf, buf2);
      sprinttype(GET_OBJ_VAL(j, 3), weapon_types, buf2);
      strcat(buf, buf2);
      break;
    case ITEM_MISSILE:
      sprintf(buf, "{BTohit: {c%d{B, Todam: {c%d{B, Type: {c%d", GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 3));
      break;
    case ITEM_ARMOR:
      sprintf(buf, "{BAC-apply: [{c%d{B]{x", GET_OBJ_VAL(j, 0));
      break;
    case ITEM_TRAP:
      sprintf(buf, "{BSpell: {c%s{B, - Hitpoints: {c%d{x", get_spell_name(GET_OBJ_VAL(j, 0)), GET_OBJ_VAL(j, 1));
      break;
    case ITEM_CONTAINER:
    case ITEM_PCORPSE:
      sprintf(buf, "{BMax-contains: {c%d{B, Locktype: {c%d{B, Corpse: {c%s{x", GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 3) ? "Yes" : "No");
      break;
    case ITEM_DRINKCON:
    case ITEM_FOUNTAIN:
      sprinttype(GET_OBJ_VAL(j, 2), drinks, buf2);
      sprintf(buf, "{BMax-contains: {c%d{B, Contains: {c%d{B, Poisoned: {c%s{B, Liquid: {c%s{x", GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 3) ? "Yes" : "No", buf2);
      break;
    case ITEM_NOTE:
      sprintf(buf, "{BTongue: {c%d{x", GET_OBJ_VAL(j, 0));
      break;
    case ITEM_KEY:
      sprintf(buf, "{BKeytype: {c%d{x", GET_OBJ_VAL(j, 0));
      break;
    case ITEM_FOOD:
      sprintf(buf, "{BMakes full: {c%d{B, Poisoned: {R%d{x", GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 3));
      break;
    default:
      sprintf(buf, "{BValues 0-4: [{c%d{B] [{c%d{B] [{c%d{B] [{c%d{B] [{c%d{B]{x", GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1), GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3), GET_OBJ_VAL(j, 4));
      break;
  }
  send_to_char(buf, ch);
  if (GET_OBJ_SLOTS(j)) {
    buf2[0] = '\0';
    sprintbit(GET_OBJ_SLOTS(j), worn_bits, buf2);
    sprintf(buf, "\r\n{BEquipment Slots:{c %s{x\r\n", buf2);
  } else {
    sprintf(buf, "\r\n{BEquipment Slots:{c UNDEFINED{x\r\n");
  }
  send_to_char(buf, ch);

  strcpy(buf, "{BEquipment Status:{c ");
  if (!j->carried_by) {
    strcat(buf, "None");
  } else {
    found = FALSE;
    for (i = 0; i < NUM_WEARS; i++) {
      if (j->carried_by->equipment[i] == j) {
        sprinttype(i, equipment_types, buf2);
        strcat(buf, buf2);
        found = TRUE;
      }
    }
    if (!found) {
      strcat(buf, "Inventory");
    }
  }
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  if (j->contains) {
    sprintf(buf, "{BContents:{x%s", CCGRN(ch, C_NRM));
    for (found = 0, j2 = j->contains; j2; j2 = j2->next_content) {
      sprintf(buf2, "%s %s", found++ ? "," : "", j2->short_description);
      strcat(buf, buf2);
      if (strlen(buf) >= 62) {
        if (j2->next_content) {
          strcat(buf, ",\r\n");
          send_to_char(buf, ch);
        } else {
          strcat(buf, "\r\n");
          send_to_char(buf, ch);
        }
        *buf = found = 0;
      }
    }

    if (*buf) {
      strcat(buf, "\r\n");
      send_to_char(buf, ch);
    }
    send_to_char(CCNRM(ch, C_NRM), ch);
  }
  found = 0;
  send_to_char("{BAffections:{x", ch);
  for (i = 0; i < MAX_OBJ_AFFECT; i++) {
    if (j->affected[i].modifier && j->affected[i].location) {
      sprinttype(j->affected[i].location, apply_types, buf2);
      sprintf(buf, "%s %+d to %s", found++ ? "," : "", j->affected[i].modifier, buf2);
      send_to_char(buf, ch);
    }
  }
  if (!found) {
    send_to_char(" None", ch);
  }
  send_to_char("\r\n", ch);

  found = 0;
  send_to_char("{BResistances: {x", ch);
  for (i = 0; i < MAX_DAM_TYPE; i++) {
    if (j->resists[i] && !found) {
      found = 1;
      sprinttype(i, resists_names, tname);
      sprintf(buf, " {c%s  {w%d\r\n", tname, j->resists[i]);
      send_to_char(buf, ch);
    } else if (j->resists[i] && found) {
      sprinttype(i, resists_names, tname);
      sprintf(buf, "              {c%s  {W%d\r\n", tname, j->resists[i]);
      send_to_char(buf, ch);
    }
  }
  if (!found) {
    send_to_char("{c<{CNone{c>{x\r\n", ch);
  }
}

void do_stat_character(struct char_data * ch, struct char_data * k)
{
  int i, i2, found = 0;
  int count;
  struct obj_data *j;
  struct follow_type *fol;
  struct affected_type *aff;
  MPROG_DATA *mprg;
  extern struct attack_hit_type attack_hit_text[];
  struct mob_attacks_data *next_one, *this;
  char rtype[50];
  int oktosee = 0;
  int zonenum = 0;
  int virtual;

  if (IS_NPC(k)) {
    virtual = GET_MOB_VNUM(k);

    zonenum = (virtual - (virtual % 100)) / 100;
    if (zonenum != 12 && zonenum != 30 && zonenum != 31 && !COM_FLAGGED(ch, COM_ADMIN)) {
      for (i = 0; i < 4; i++) {
        if (ch->olc_zones[i] == zonenum) {
          oktosee = 1;
        }
      }
      if (!oktosee || zonenum == 0) {
        send_to_char("You are not authorized to see info in this zone.\r\n", ch);
        return;
      }
    }
  }

  sprintf(logbuffer, "%s did a 'stat' on %s #%ld (%s)", GET_NAME(ch), IS_NPC(k) ? "mob" : "player", IS_NPC(k) ? GET_MOB_VNUM(k) : GET_IDNUM(k), GET_NAME(k));
  mudlog(logbuffer, 'X', COM_IMMORT, FALSE);

  switch (k->player.sex) {
    case SEX_NEUTRAL:
      strcpy(buf, "{cNEUTRAL-SEX{x");
      break;
    case SEX_MALE:
      strcpy(buf, "{cMALE{x");
      break;
    case SEX_FEMALE:
      strcpy(buf, "{cFEMALE{x");
      break;
    default:
      strcpy(buf, "{RILLEGAL-SEX!!{x");
      break;
  }

  sprintf(buf2, " %s '%s'  {BIDNum: [{c%5ld{B], In room [{c%5d{B]{x\r\n", (!IS_NPC(k) ? "{CPC{x" : (!IS_MOB(k) ? "{RNPC{x" : "{RMOB{x")), GET_NAME(k), GET_IDNUM(k), world[k->in_room].number);
  send_to_char(strcat(buf, buf2), ch);
  if (IS_MOB(k)) {
    sprintf(buf, "{BAlias: {c%s{B, VNum: [{c%5d{B], RNum: [{c%5d{B]{x\r\n", k->player.name, GET_MOB_VNUM(k), GET_MOB_RNUM(k));
    send_to_char(buf, ch);
  }
  sprintf(buf, "{BTitle:{x %s\r\n", (k->player.title ? k->player.title : "<None>"));
  send_to_char(buf, ch);

  sprintf(buf, "{BL-Des: {c%s{x", (k->player.long_descr ? k->player.long_descr : "<None>\r\n"));
  send_to_char(buf, ch);

  if (!IS_NPC(k)) {
    strcpy(buf, "{BRace:{c ");
    sprinttype(GET_RACE(k), pc_race_types, buf2);
    strcat(buf, buf2);
    send_to_char(buf, ch);
  } else {
    strcpy(buf, "{BRace:{c ");
    sprinttype(GET_MOB_RACE(k), npc_race_names, buf2);
    strcat(buf, buf2);
    send_to_char(buf, ch);
  }

  if (IS_NPC(k)) {
    strcpy(buf, "{B, {BMonster Class: {c");
    sprinttype(k->player.class, npc_class_types, buf2);
  } else {
    strcpy(buf, "{B, {BClass: {y");
    sprinttype(k->player.class, pc_class_types, buf2);
  }
  strcat(buf2, "\r\n");
  strcat(buf, buf2);

  sprintf(buf2, "{BLev: [{y%s%2d%s{B], XP: [{y%s%7d%s{B], XPNeed: [{y%7d{B], Align: [{y%4d{B]{x\r\n", CCYEL(ch, C_NRM), GET_LEVEL(k), CCNRM(ch, C_NRM), CCYEL(ch, C_NRM), GET_EXP(k), CCNRM(ch, C_NRM), (!IS_NPC(k) ? MaxExperience[(int) (GET_LEVEL(k) + 1)] : 0), GET_ALIGNMENT(k));
  strcat(buf, buf2);
  send_to_char(buf, ch);

  if (!IS_NPC(k)) {
    strcpy(buf1, (char *) asctime(localtime(&(k->player.time.birth))));
    strcpy(buf2, (char *) asctime(localtime(&(k->player.time.logon))));
    buf1[10] = buf2[10] = '\0';

    sprintf(buf, "{BCreated: [{c%s{B], Last Logon: [{c%s{B], Played [{c%dh %dm{B], Age [{c%d{B]{x\r\n", buf1, buf2, k->player.time.played / 3600, ((k->player.time.played / 3600) % 60), age(k).year);
    send_to_char(buf, ch);

    sprintf(buf, "{BHometown: [{c%d{B], (STL[{c%d{B]/per[{c%d{B]/NSTL[{c%d{B]), Race: [{c%d{B], WS[{c%d{B]{x\r\n", k->player.hometown, GET_PRACTICES(k), stats[INT_LEARN][GET_INT(k)], stats[WIS_PRAC][GET_WIS(k)], k->player_specials->saved.race, GET_TRAINING(ch));
    send_to_char(buf, ch);

  }

  if (COM_FLAGGED(ch, COM_QUEST)) {
    sprintf(buf, "{BStr: [%s%d%s{B]  Int: [%s%d%s{B]  Wis: [%s%d%s{B]  "
        "{BDex: [%s%d%s{B]  Con: [%s%d%s{B]  Agi: [%s%d%s{B]{x\r\n", CCCYN(ch, C_NRM), GET_STR(k), CCNRM(ch, C_NRM), CCCYN(ch, C_NRM), GET_INT(k), CCNRM(ch, C_NRM), CCCYN(ch, C_NRM), GET_WIS(k), CCNRM(ch, C_NRM), CCCYN(ch, C_NRM), GET_DEX(k), CCNRM(ch, C_NRM), CCCYN(ch, C_NRM), GET_CON(k), CCNRM(ch, C_NRM), CCCYN(ch, C_NRM), GET_AGI(k), CCNRM(ch, C_NRM));
    send_to_char(buf, ch);

    if (!IS_NPC(ch)) {
      sprintf(buf, "{BWeight: [{c%d{B] Height: [{c%d{B]{x\r\n", GET_WEIGHT(k), GET_HEIGHT(k));
      send_to_char(buf, ch);
    }

    sprintf(buf, "{BHit p.:[%s%d/%d+%d%s{B]  Mana p.:[%s%d/%d+%d%s{B]  Move p.:[%s%d/%d+%d%s{B]{x\r\n", CCGRN(ch, C_NRM), GET_HIT(k), GET_MAX_HIT(k), hit_gain(k, 0), CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), GET_MANA(k), GET_MAX_MANA(k), mana_gain(k, 0), CCNRM(ch, C_NRM), CCGRN(ch, C_NRM), GET_MOVE(k), GET_MAX_MOVE(k), move_gain(k, 0), CCNRM(ch, C_NRM));
    send_to_char(buf, ch);

    sprintf(buf, "{BCoins: [{c%d{Wp {c%d{Yg {c%d{ws {c%d{yc{B], Bank: [{c%d{Wp {c%d{Yg {c%d{ws {c%d{yc{B] (Total: {c%d{Wp {c%d{Yg {c%d{ws {c%d{yc{B){x\r\n", GET_PLAT(k), GET_GOLD(k), GET_SILVER(k), GET_COPPER(k), GET_BANK_PLAT(k), GET_BANK_GOLD(k), GET_BANK_SILVER(k), GET_BANK_COPPER(k), GET_PLAT(k) + GET_BANK_PLAT(k), GET_GOLD(k) + GET_BANK_GOLD(k), GET_SILVER(k) + GET_BANK_SILVER(k), GET_COPPER(k) + GET_BANK_COPPER(k));
    send_to_char(buf, ch);

    if (IS_NPC(k)) {
      sprintf(buf, "{BAC: [{c%d/10{B], Thac0: [{c%2d{B], Damroll: [{c%2d{B], Saving throws: [{c%d{B/{c%d{B/{c%d{B/{c%d{B/{c%d{B]{x\r\n", GET_AC(k), k->points.hitroll, k->points.damroll, GET_SAVE(k, 0), GET_SAVE(k, 1), GET_SAVE(k, 2), GET_SAVE(k, 3), GET_SAVE(k, 4));
    } else {
      sprintf(buf, "{BAC: [{c%d/10{B], Hitroll: [{c%2d{B], Damroll: [{c%2d{B], Saving throws: [{c%d{B/{c%d{B/{c%d{B/{c%d{B/{c%d{B]{x\r\n", GET_AC(k), k->points.hitroll, k->points.damroll, GET_SAVE(k, 0), GET_SAVE(k, 1), GET_SAVE(k, 2), GET_SAVE(k, 3), GET_SAVE(k, 4));
    }
    send_to_char(buf, ch);
    sprintf(buf, "{BDeaths: [{c%3d{B], Pk's [{c%3d{B]{x\r\n", GET_DEATHCOUNT(k), GET_PKCOUNT(k));
    send_to_char(buf, ch);
  }

  sprinttype(GET_POS(k), position_types, buf2);
  sprintf(buf, "{BPos: {c%s{B, Fighting: {c%s", buf2, (FIGHTING(k) ? GET_NAME(FIGHTING(k)) : "Nobody"));

  if (IS_NPC(k)) {
    strcat(buf, ", {BMob size: {c");
    sprinttype(k->mob_specials.size, size_names, buf2);
    strcat(buf, buf2);
  }
  if (k->desc) {
    sprinttype(k->desc->connected, connected_types, buf2);
    strcat(buf, ", {BConnected: {c");
    strcat(buf, buf2);
  }
  send_to_char(strcat(buf, "\r\n"), ch);

  if (COM_FLAGGED(ch, COM_QUEST)) {
    if (IS_NPC(k) && k->mob_specials.mob_attacks) {
      send_to_char("{BMob attacks:{x\r\n", ch);
      for (this = k->mob_specials.mob_attacks; this; this = next_one) {
        next_one = this->next;
        sprintf(buf, "  {R%d{Wd{R%d{W+{R%d{W, {Wspeed: {R%d%%{W, type: {R", this->nodice, this->sizedice, this->damroll, this->attacks);
        strcat(buf, attack_hit_text[(int) this->attack_type].singular);
        send_to_char(strcat(buf, "\r\n"), ch);
      }
      send_to_char("{x\r\n", ch);
    }
  }

  strcpy(buf, "{BDefault position: {c");
  sprinttype((k->mob_specials.default_pos), position_types, buf2);
  strcat(buf, buf2);

  sprintf(buf2, "{B, Idle Timer (in tics) [{c%d{B]{x\r\n", k->char_specials.timer);
  strcat(buf, buf2);
  send_to_char(buf, ch);

  if (COM_FLAGGED(ch, COM_BUILDER)) {
    if (IS_NPC(k)) {
      if (mob_index[k->nr].progtypes) {
        send_to_char("{BMobile programs:{x\r\n", ch);
        for (mprg = mob_index[k->nr].mobprogs; mprg != NULL; mprg = mprg->next) {
          sprintf(buf, ">%s %s\n\r%s\n\r", mprog_type_to_name(mprg->type), mprg->arglist, mprg->comlist);
          send_to_char(buf, ch);
        }
      }
    }
  } else if (COM_FLAGGED(ch, COM_QUEST)) {
    send_to_char("{BMobile programs: EXISTS{x\r\n", ch);
  }

  if (IS_NPC(k)) {
    sprintbit(MOB_FLAGS(k), action_bits, buf2);
    sprintf(buf, "{BNPC flags:{x %s%s%s\r\n", CCYEL(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
    send_to_char(buf, ch);
  } else if (COM_FLAGGED(ch, COM_QUEST)) {
    sprintbit(PLR_FLAGS(k), player_bits, buf2);
    sprintf(buf, "{BPLR:{x %s%s%s\r\n", CCCYN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
    send_to_char(buf, ch);
    sprintbit(PRF_FLAGS(k), preference_bits, buf2);
    sprintf(buf, "{BPRF:{x %s%s%s\r\n", CCGRN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
    send_to_char(buf, ch);
    if (COM_FLAGGED(ch, COM_BUILDER)) {
      sprintbit(COM_FLAGS(k), com_bits, buf2);
      sprintf(buf, "{BCOM:{x %s%s%s\r\n", CCGRN(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
      send_to_char(buf, ch);
    }
  }

  sprintf(buf, "{BRES:{x ");
  for (i = 0; i < MAX_DAM_TYPE; i++) {
    if (GET_RESIST(k, i) && !found) {
      found = 1;
      sprinttype(i, resists_names, rtype);
      sprintf(buf2, "{C%s  {W%d\r\n", rtype, GET_RESIST(k, i));
      strcat(buf, buf2);
    } else if (GET_RESIST(k, i) && found) {
      sprinttype(i, resists_names, rtype);
      sprintf(buf2, "     {C%s  {W%d\r\n", rtype, GET_RESIST(k,i));
      strcat(buf, buf2);
    }
  }
  if (!found) {
    strcat(buf, "{c<none>{x\r\n");
  }
  send_to_char(buf, ch);
  found = 0;

  if (IS_MOB(k)) {
    sprintf(buf, "{BMob Spec-Proc: {c%s{x\r\n", (mob_index[GET_MOB_RNUM(k)].func ? "Exists" : "None"));
    send_to_char(buf, ch);
  }
  sprintf(buf, "{BCarried: weight: {c%d{B, items: {c%d{B; ", IS_CARRYING_W(k), IS_CARRYING_N(k));

  for (i = 0, j = k->carrying; j; j = j->next_content, i++) {
    ;
  }
  sprintf(buf + strlen(buf), "{BItems in: inventory: {c%d{B, ", i);

  for (i = 0, i2 = 0; i < NUM_WEARS; i++) {
    if (k->equipment[i]) {
      i2++;
    }
  }
  sprintf(buf2, "{Beq: {c%d{x\r\n", i2);
  strcat(buf, buf2);
  send_to_char(buf, ch);

  sprintf(buf, "{BHunger: {c%d{B, Thirst: {c%d{B, Drunk: {c%d{x\r\n", GET_COND(k, FULL), GET_COND(k, THIRST), GET_COND(k, DRUNK));
  send_to_char(buf, ch);

  if (IS_MOB(k)) {
    if (HUNTINGRM(k) == -1) { /* not hunting a room */
      sprintf(buf, "{MHunting: {W%s{M, Hunting Room: {W<none>{x", ((HUNTING(k)) ? ((find_hunted_name(HUNTING(k))) ? find_hunted_name(HUNTING(k)) : "<unknown>") : "<none>"));
    } else {
      sprintf(buf, "{MHunting: {W<none>{M, Hunting Room: {W%d{x", HUNTINGRM(k));
    }

    send_to_char(strcat(buf, "\r\n"), ch);
  }

  sprintf(buf, "{BMaster is: {c%s{B, Followers are:{c", ((k->master) ? GET_NAME(k->master) : "<none>"));

  for (fol = k->followers; fol; fol = fol->next) {
    sprintf(buf2, "%s %s", found++ ? "," : "", PERS(fol->follower, ch));
    strcat(buf, buf2);
    if (strlen(buf) >= 62) {
      if (fol->next) {
        send_to_char(strcat(buf, ",{x\r\n"), ch);
      } else {
        send_to_char(strcat(buf, "\r\n"), ch);
      }
      *buf = found = 0;
    }
  }

  if (*buf) {
    send_to_char(strcat(buf, "\r\n"), ch);
  }

  /* Showing the bitvector */
  sprintbit(AFF_FLAGS(k), affected_bits, buf2);
  sprintf(buf, "{BAFF:{x %s%s%s\r\n", CCYEL(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
  send_to_char(buf, ch);
  sprintbit(AFF2_FLAGS(k), affected_bits2, buf2);
  sprintf(buf, "{BAFF2:{x %s%s%s\r\n", CCYEL(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
  send_to_char(buf, ch);
  sprintbit(AFF3_FLAGS(k), affected_bits3, buf2);
  sprintf(buf, "{BAFF3:{x %s%s%s\r\n", CCYEL(ch, C_NRM), buf2, CCNRM(ch, C_NRM));
  send_to_char(buf, ch);

  /* Routine to show what spells a char is affected by */
  if (k->affected) {
    for (aff = k->affected; aff; aff = aff->next) {
      *buf2 = '\0';
      count = 0;
      sprintf(buf, "{BSPL: ({c%3dsec{B){x %s%-21s%s\r\n", aff->duration + 1, CCCYN(ch, C_NRM), get_spell_name(aff->type), CCNRM(ch, C_NRM));
      for (i = 0; i < NUM_MODIFY; i++) {
        if (aff->modifier[i] && aff->location[i]) {
          sprintf(buf2, "    %+3d to %s\r\n", aff->modifier[i], apply_types[aff->location[i]]);
          count++;
          strcat(buf, buf2);
        }
      }
      if (aff->bitvector) {
        if (*buf2) {
          strcat(buf, ", {Bsets{x ");
        } else {
          strcat(buf, "{Bsets{x ");
        }
        sprintbit(aff->bitvector, affected_bits, buf2);
        strcat(buf, buf2);
      }
      if (aff->bitvector2) {
        if (*buf2) {
          strcat(buf, ", {Bsets{x ");
        } else {
          strcat(buf, "{Bsets{x ");
        }
        sprintbit(aff->bitvector2, affected_bits2, buf2);
        strcat(buf, buf2);
      }
      if (aff->bitvector3) {
        if (*buf2) {
          strcat(buf, ", {Bsets{x ");
        } else {
          strcat(buf, "{Bsets{x ");
        }
        sprintbit(aff->bitvector3, affected_bits3, buf2);
        strcat(buf, buf2);
      }
      send_to_char(strcat(buf, "\r\n"), ch);
    }
  }
}

ACMD(do_stat)
{
  struct char_data *victim = 0;
  struct obj_data *object = 0;
  int tmp;

  half_chop(argument, buf1, buf2);

  if (!*buf1) {
    send_to_char("Stats on who or what?\r\n", ch);
    return;
  } else if (is_abbrev(buf1, "room")) {
    do_stat_room(ch);
  } else if (is_abbrev(buf1, "mob")) {
    if (!*buf2) {
      send_to_char("Stats on which mobile?\r\n", ch);
    } else {
      if ((victim = get_char_vis(ch, buf2))) {
        do_stat_character(ch, victim);
      } else {
        send_to_char("No such mobile around.\r\n", ch);
      }
    }
  } else if (is_abbrev(buf1, "player")) {
    if (!*buf2) {
      send_to_char("Stats on which player?\r\n", ch);
    } else {
      if ((victim = get_player_vis(ch, buf2, 0))) {
        do_stat_character(ch, victim);
      } else {
        CREATE(victim, struct char_data, 1);
        clear_char(victim);
        if (load_char_text(buf2, victim) > 0) {
          do_stat_character(ch, victim);
          free_char(victim);
        } else {
          send_to_char("No such player.\r\n", ch);
          FREE(victim);
        }
      }
    }
  } else if (is_abbrev(buf1, "file")) {
    if (!*buf2) {
      send_to_char("Stats on which player?\r\n", ch);
    } else {
      CREATE(victim, struct char_data, 1);
      clear_char(victim);
      if (load_char_text(buf2, victim) > 0) {
        do_stat_character(ch, victim);
        free_char(victim);
      } else {
        send_to_char("There is no such player.\r\n", ch);
        FREE(victim);
      }
    }
  } else if (is_abbrev(buf1, "object")) {
    if (!*buf2) {
      send_to_char("Stats on which object?\r\n", ch);
    } else {
      if ((object = get_obj_vis(ch, buf2))) {
        do_stat_object(ch, object);
      } else {
        send_to_char("No such object around.\r\n", ch);
      }
    }
  } else {
    if ((object = get_object_in_equip_vis(ch, buf1, ch->equipment, &tmp))) {
      do_stat_object(ch, object);
    } else if ((object = get_obj_in_list_vis(ch, buf1, ch->carrying))) {
      do_stat_object(ch, object);
    } else if ((victim = get_char_room_vis(ch, buf1))) {
      do_stat_character(ch, victim);
    } else if ((object = get_obj_in_list_vis(ch, buf1, world[ch->in_room].contents))) {
      do_stat_object(ch, object);
    } else if ((victim = get_char_vis(ch, buf1))) {
      do_stat_character(ch, victim);
    } else if ((object = get_obj_vis(ch, buf1))) {
      do_stat_object(ch, object);
    } else {
      CREATE(victim, struct char_data, 1);
      clear_char(victim);
      if (load_char_text(buf1, victim) > 0) {
        do_stat_character(ch, victim);
        free_char(victim);
      } else {
        send_to_char("Nothing around by that name.\r\n", ch);
        FREE(victim);
      }
    }
  }
}

ACMD(do_shutdown)
{
  extern int circle_shutdown, circle_reboot;

  if (subcmd != SCMD_SHUTDOWN) {
    send_to_char("Quit bothering me!!\r\n", ch);
    return;
  }
  one_argument(argument, arg);

  if (!*arg) {
    sprintf(buf, "(GC) Shutdown by %s.", GET_NAME(ch));
    stderr_log(buf);
    sprintf(buf, "\r\nOh, lets hope %s didnt have a wittle typo..", GET_NAME(ch));
    send_to_all(buf);
    circle_shutdown = 1;
  } else if (!str_cmp(arg, "reboot")) {
    sprintf(buf, "(GC) Reboot by %s.", GET_NAME(ch));
    stderr_log(buf);
    sprintf(buf, "Reboot by %s.. come back in 10-20 seconds...\r\n", GET_NAME(ch));
    send_to_all(buf);
    touch("../.fastboot");
    circle_shutdown = circle_reboot = 1;
  } else if (!str_cmp(arg, "die")) {
    sprintf(buf, "(GC) Shutdown by %s.", GET_NAME(ch));
    stderr_log(buf);
    sprintf(buf, "\r\nWith one final blow to the skull %s has killed ShadowWind...\r\nShadowWind is DEAD.  R.I.P. ", GET_NAME(ch));
    send_to_all(buf);
    touch("../.killscript");
    circle_shutdown = 1;
  } else if (!str_cmp(arg, "pause")) {
    sprintf(buf, "(GC) Shutdown by %s.", GET_NAME(ch));
    stderr_log(buf);
    sprintf(buf, "\r\n%s steps through a rift and with a sudden jesture all time is frozen\r\nReturning into the vast nothingness of time, the world colapses and ends.", GET_NAME(ch));
    send_to_all(buf);
    touch("../pause");
    circle_shutdown = 1;
  } else {
    send_to_char("QUIT POKING ME!!\r\n", ch);
  }
}

void stop_snooping(struct char_data * ch)
{
  if (!ch->desc->snooping) {
    send_to_char("You aren't snooping anyone.\r\n", ch);
  } else {
    sprintf(logbuffer, "%s stopped snooping %s", GET_NAME(ch), GET_NAME(ch->desc->snooping->character));
    mudlog(logbuffer, 'X', COM_ADMIN, TRUE);
    send_to_char("You stop snooping.\r\n", ch);
    ch->desc->snooping->snoop_by = NULL;
    ch->desc->snooping = NULL;
  }
}

ACMD(do_snoop)
{
  struct char_data *victim, *tch;

  if (!ch->desc) {
    return;
  }

  one_argument(argument, arg);

  if (!*arg) {
    stop_snooping(ch);
  } else if (!(victim = get_char_vis(ch, arg))) {
    send_to_char("No such person around.\r\n", ch);
  } else if (!victim->desc) {
    send_to_char("There's no link.. nothing to snoop.\r\n", ch);
  } else if (victim == ch) {
    stop_snooping(ch);
  } else if (victim->desc->snoop_by) {
    send_to_char("Busy already. \r\n", ch);
  } else {
    if (victim->desc->original) {
      tch = victim->desc->original;
    } else {
      tch = victim;
    }

    if (IS_SET(PLR_FLAGS(tch), PLR_EDITING)) {
      send_to_char("Maybe it isn't such a good idea to snoop them while they are editing.\r\n", ch);
      return;
    }
    send_to_char(OK, ch);

    sprintf(logbuffer, "%s snooping %s", GET_NAME(ch), GET_NAME(victim));
    mudlog(logbuffer, 'X', COM_ADMIN, TRUE);
    plog(logbuffer, ch, MAX(LVL_IMMORT, GET_LEVEL(ch)));

    if (ch->desc->snooping) {
      ch->desc->snooping->snoop_by = NULL;
    }

    ch->desc->snooping = victim->desc;
    victim->desc->snoop_by = ch->desc;
  }
}

ACMD(do_switch)
{
  struct char_data *victim;
  sh_int location;

  one_argument(argument, arg);

  if (ch->desc->original) {
    send_to_char("You're already switched.\r\n", ch);
  } else if (!*arg) {
    send_to_char("Switch with who?\r\n", ch);
  } else if (!(victim = get_char_vis(ch, arg))) {
    send_to_char("No such character.\r\n", ch);
  } else if (ch == victim) {
    send_to_char("Hee hee... we are jolly funny today, eh?\r\n", ch);
  } else if (victim->desc) {
    send_to_char("You can't do that, the body is already in use!\r\n", ch);
  } else if ((location = find_target_room(ch, arg)) < 0) {
    return;
  } else if ((!COM_FLAGGED(ch, COM_ADMIN)) && !IS_NPC(victim)) {
    send_to_char("You aren't holy enough to use a mortal's body.\r\n", ch);
  } else if (!check_access(ch, location)) {
    send_to_char("That is a restricted zone, stay out!\r\n", ch);
  } else {
    act("Okay, you are now $N.", TRUE, ch, 0, victim, TO_CHAR);
    sprintf(buf, "%s switched to %s at %s", GET_NAME(ch), GET_NAME(victim), world[victim->in_room].name);
    mudlog(buf, 'Q', COM_QUEST, FALSE);
    plog(buf, ch, MAX(LVL_IMMORT, GET_LEVEL(ch)));
    ch->desc->character = victim;
    ch->desc->original = ch;

    victim->desc = ch->desc;
    ch->desc = NULL;
  }
}

ACMD(do_return)
{
  if (ch->desc && ch->desc->original) {
    send_to_char("You return to your original body.\r\n", ch);

    sprintf(buf, "%s returned to the original body.", GET_NAME(ch->desc->original));
    mudlog(buf, 'Q', COM_QUEST, FALSE);
    plog(buf, ch->desc->original, MAX(LVL_IMMORT, GET_LEVEL(ch->desc->original)));
    ch->desc->character = ch->desc->original;
    ch->desc->original = NULL;

    ch->desc->character->desc = ch->desc;
    ch->desc = NULL;
  }
}

ACMD(do_load)
{
  struct char_data *mob;
  struct obj_data *obj;
  int number, r_num;

  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2 || !isdigit(*buf2)) {
    send_to_char("Usage: load [ obj | mob ] <virt num>\r\n", ch);
    return;
  }
  if ((number = atoi(buf2)) < 0) {
    send_to_char("A NEGATIVE number??\r\n", ch);
    return;
  }
  if (is_abbrev(buf, "mob")) {
    int oktosee = 0;
    int zonenum = 0;
    int n;

    if ((r_num = real_mobile(number)) < 0) {
      send_to_char("There is no monster with that number.\r\n", ch);
      return;
    }
    mob = read_mobile(r_num, REAL);

    zonenum = (GET_MOB_VNUM(mob) - (GET_MOB_VNUM(mob) % 100)) / 100;
    if (zonenum != 12 && zonenum != 30 && zonenum != 31 && !COM_FLAGGED(ch, COM_ADMIN)) {
      for (n = 0; n < 4; n++) {
        if (ch->olc_zones[n] == zonenum) {
          oktosee = 1;
        }
      }
      if (!oktosee || zonenum == 0) {
        send_to_char("You are not authorized to see info in this zone.\r\n", ch);
        return;
      }
    }
    GET_IDNUM(mob) = mob_idnum;
    mob_idnum--;
    char_to_room(mob, ch->in_room);

    act("$n makes a quaint, magical gesture with one hand.", TRUE, ch, 0, 0, TO_ROOM);
    act("$n has created $N!", FALSE, ch, 0, mob, TO_ROOM);
    act("You create $N.", FALSE, ch, 0, mob, TO_CHAR);
    sprintf(buf, "%s created %s at %s", GET_NAME(ch), GET_NAME(mob), world[ch->in_room].name);
    mudlog(buf, 'L', COM_QUEST, FALSE);
    plog(buf, ch, MAX(LVL_IMMORT, GET_LEVEL(ch)));
  } else if (is_abbrev(buf, "obj")) {
    int oktosee = 0;
    int zonenum = 0;
    int i;

    if ((r_num = real_object(number)) < 0) {
      send_to_char("There is no object with that number.\r\n", ch);
      return;
    }
    if (obj_index[r_num].qic != NULL && !COM_FLAGGED(ch, COM_ADMIN)) {
      send_to_char("You are not allowed to load QIC.\r\n", ch);
      return;
    }
    obj = read_object(r_num, REAL);
    zonenum = (GET_OBJ_VNUM(obj) - (GET_OBJ_VNUM(obj) % 100)) / 100;
    if (zonenum != 12 && zonenum != 30 && zonenum != 31 && !COM_FLAGGED(ch, COM_ADMIN)) {
      for (i = 0; i < 4; i++) {
        if (ch->olc_zones[i] == zonenum) {
          oktosee = 1;
        }
      }
      if (!oktosee || zonenum == 0) {
        send_to_char("You are not authorized to see info in this zone.\r\n", ch);
        return;
      }
    }
    if (obj == NULL) {
      send_to_char("You are not allowed to load QIC's over the limit.\r\n", ch);
      return;
    }

    /* if there is a TAKE flag on the object, put it in the
     * inventory of the creator, otherwise call obj_to_room
     * like normal
     */
    obj_to_room(obj, ch->in_room);
    act("$n makes a strange magical gesture.", TRUE, ch, 0, 0, TO_ROOM);
    act("$n has created $p!", FALSE, ch, obj, 0, TO_ROOM);
    act("You create $p.", FALSE, ch, obj, 0, TO_CHAR);
    sprintf(buf, "%s created %s at %s", GET_NAME(ch), obj->short_description, world[ch->in_room].name);
    mudlog(buf, 'L', COM_QUEST, FALSE);
    plog(buf, ch, MAX(LVL_IMMORT, GET_LEVEL(ch)));
  } else {
    send_to_char("That'll have to be either 'obj' or 'mob'.\r\n", ch);
  }
}

ACMD(do_rload)
{
  struct char_data *mob;
  struct obj_data *obj;
  int number;

  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2 || !isdigit(*buf2)) {
    send_to_char("Usage: rload { obj | mob } <real num>\r\n", ch);
    return;
  }
  if ((number = atoi(buf2)) < 0) {
    send_to_char("A NEGATIVE number??\r\n", ch);
    return;
  }
  if (is_abbrev(buf, "mob")) {
    if (number < 0 || number > new_top_mobt) {
      send_to_char("There is no monster with that number.\r\n", ch);
      return;
    }
    mob = read_mobile(number, REAL);
    char_to_room(mob, ch->in_room);

    act("$n makes a quaint, magical gesture with one hand.", TRUE, ch, 0, 0, TO_ROOM);
    act("$n has created $N!", FALSE, ch, 0, mob, TO_ROOM);
    act("You create $N.", FALSE, ch, 0, mob, TO_CHAR);
    sprintf(buf, "%s created %s", GET_NAME(ch), GET_NAME(mob));
    mudlog(buf, 'L', COM_QUEST, FALSE);
    plog(buf, ch, MAX(LVL_IMMORT, GET_LEVEL(ch)));
  } else if (is_abbrev(buf, "obj")) {
    if (number < 0 || number > new_top_objt) {
      send_to_char("There is no object with that number.\r\n", ch);
      return;
    }
    if (obj_index[number].qic != NULL && !COM_FLAGGED(ch, COM_ADMIN)) {
      send_to_char("You are not allowed to load QIC.\r\n", ch);
      return;
    }
    obj = read_object(number, REAL);
    if (obj == NULL) {
      send_to_char("You are not allowed to load objects over the QIC limit.\r\n", ch);
      return;
    }
    obj_to_room(obj, ch->in_room);
    act("$n makes a strange magical gesture.", TRUE, ch, 0, 0, TO_ROOM);
    act("$n has created $p!", FALSE, ch, obj, 0, TO_ROOM);
    act("You create $p.", FALSE, ch, obj, 0, TO_CHAR);
    sprintf(buf, "%s created %s", GET_NAME(ch), obj->short_description);
    mudlog(buf, 'L', COM_QUEST, FALSE);
    plog(buf, ch, MAX(LVL_IMMORT, GET_LEVEL(ch)));
  } else {
    send_to_char("That'll have to be either 'obj' or 'mob'.\r\n", ch);
  }
}

ACMD(do_vstat)
{
  struct char_data *mob;
  struct obj_data *obj;
  int number, r_num;

  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2 || !isdigit(*buf2)) {
    send_to_char("Usage: vstat [ obj | mob ] <number>\r\n", ch);
    return;
  }
  if ((number = atoi(buf2)) < 0) {
    send_to_char("A NEGATIVE number??\r\n", ch);
    return;
  }
  if (is_abbrev(buf, "mob")) {
    if ((r_num = real_mobile(number)) < 0) {
      send_to_char("There is no monster with that number.\r\n", ch);
      return;
    }
    mob = read_mobile(r_num, REAL | NOEQUIP);
    char_to_room(mob, 0);
    do_stat_character(ch, mob);
    extract_char(mob, 0);
  } else if (is_abbrev(buf, "obj")) {
    if ((r_num = real_object(number)) < 0) {
      send_to_char("There is no object with that number.\r\n", ch);
      return;
    }
    obj = read_object_q(r_num, REAL);
    obj_index[r_num].number--;
    do_stat_object(ch, obj);
    obj_index[r_num].number++;
    extract_obj_q(obj);
    /* The above changes the index numbers of the objects in quest is the easiest
     work-around to vstat loading an object, and then stating it.  All this does
     is try to keep the number existing accurate.
     */
  } else {
    send_to_char("That'll have to be either 'obj' or 'mob'.\r\n", ch);
  }
}

/* clean a room of all mobiles and objects */
ACMD(do_purge)
{
  struct char_data *vict, *next_v;
  struct obj_data *obj, *next_o;

  one_argument(argument, buf);

  if (*buf) { /* argument supplied. destroy single object or char */
    if ((vict = get_char_room_vis(ch, buf))) {
      if (!IS_NPC(vict) && (GET_LEVEL(ch) < GET_LEVEL(vict))) {
        send_to_char("Fuuuuuuuuu!\r\n", ch);
        return;
      }
      if (!IS_NPC(vict) && !COM_FLAGGED(ch, COM_ADMIN) && COM_FLAGGED(vict, COM_ADMIN)) {
        send_to_char("Bah, fat chance.\r\n", ch);
        return;
      }
      act("$n disintegrates $N.", FALSE, ch, 0, vict, TO_NOTVICT);

      if (!IS_NPC(vict)) {
        sprintf(buf, "%s has purged %s.", GET_NAME(ch), GET_NAME(vict));
        mudlog(buf, 'G', COM_BUILDER, TRUE);
        plog(buf, ch, LVL_IMMORT);
        if (vict->desc) {
          close_socket(vict->desc);
          vict->desc = NULL;
        }
      }
      extract_char(vict, 1);
    } else if ((obj = get_obj_in_list_vis(ch, buf, world[ch->in_room].contents))) {
      act("$n destroys $p.", FALSE, ch, obj, 0, TO_ROOM);
      extract_obj(obj);
    } else {
      send_to_char("Nothing here by that name.\r\n", ch);
      return;
    }

    send_to_char(OK, ch);
  } else { /* no argument. clean out the room */
    act("$n gestures... You are surrounded by scorching flames!", FALSE, ch, 0, 0, TO_ROOM);
    send_to_room("The world seems a little cleaner.\r\n", ch->in_room);

    for (vict = world[ch->in_room].people; vict != NULL; vict = next_v) {
      next_v = vict->next_in_room;
      if (IS_NPC(vict)) {
        extract_char(vict, 1);
      }
    }

    for (obj = world[ch->in_room].contents; obj != NULL; obj = next_o) {
      next_o = obj->next_content;
      extract_obj(obj);
    }
  }
}

ACMD(do_advance)
{
  struct char_data *victim;
  char name[100], level[100];
  int newlevel;
  void do_start(struct char_data *ch);

  two_arguments(argument, name, level);

  if (*name) {
    if (!(victim = get_char_vis(ch, name))) {
      send_to_char("That player is not here.\r\n", ch);
      return;
    }
  } else {
    send_to_char("Advance who?\r\n", ch);
    return;
  }

  if (GET_LEVEL(ch) <= GET_LEVEL(victim)) {
    send_to_char("Maybe that's not such a great idea.\r\n", ch);
    return;
  }
  if (IS_NPC(victim)) {
    send_to_char("NO!  Not on NPC's.\r\n", ch);
    return;
  }
  if (!*level || (newlevel = atoi(level)) <= 0) {
    send_to_char("That's not a level!\r\n", ch);
    return;
  }
  if (newlevel > LVL_IMMORT) {
    sprintf(buf, "%d is the highest possible level.\r\n", LVL_IMMORT);
    send_to_char(buf, ch);
    return;
  }
  if (newlevel > GET_LEVEL(ch)) {
    send_to_char("Yeah, right.\r\n", ch);
    return;
  }
  if (newlevel < GET_LEVEL(victim)) {
    do_start(victim);
    GET_LEVEL(victim) = newlevel;
  } else {
    act("$n makes some strange gestures.\r\n"
        "A strange feeling comes upon you,\r\n"
        "Like a giant hand, light comes down\r\n"
        "from above, grabbing your body, that\r\n"
        "begins to pulse with colored lights\r\n"
        "from inside.\r\n\r\n"
        "Your head seems to be filled with demons\r\n"
        "from another plane as your body dissolves\r\n"
        "to the elements of time and space itself.\r\n"
        "Suddenly a silent explosion of light\r\n"
        "snaps you back to reality.\r\n\r\n"
        "You feel slightly different.", FALSE, ch, 0, victim, TO_VICT);
  }

  send_to_char(OK, ch);

  sprintf(buf, "%s has advanced %s to level %d (from %d)", GET_NAME(ch), GET_NAME(victim), newlevel, GET_LEVEL(victim));
  mudlog(buf, 'A', COM_ADMIN, TRUE);
  plog(buf, ch, MAX(LVL_IMMORT, GET_LEVEL(ch)));
  gain_exp_regardless(victim, (MaxExperience[newlevel]) - GET_EXP(victim));
  save_char_text(victim, NOWHERE);
}

ACMD(do_tedit)
{
  int l, i;
  char field[MAX_INPUT_LENGTH];
  extern char *credits;
  extern char *news;
  extern char *motd;
  extern char *imotd;
  extern char *help;
  extern char *info;
  extern char *background;
  extern char *handbook;
  extern char *policies;
  extern char *namepol;
  extern char *signoff;
  extern char *todolist;
  extern char *buglist;
  extern char *idealist;
  extern char *typolist;
  extern char *nmotd;
  extern char *MENU;
  extern char *GREET1;
  extern char *GREET2;

  struct editor_struct {
    char *cmd;
    char level;
    char *buffer;
    int size;
    char *filename;
  };
  struct editor_struct fields[] = {
      /* edit the lvls to your own needs */
      {"credits", COM_ADMIN, credits, MAX_STRING_LENGTH, CREDITS_FILE},
      {"news", COM_BUILDER, news, MAX_STRING_LENGTH, NEWS_FILE},
      {"motd", COM_BUILDER, motd, MAX_STRING_LENGTH, MOTD_FILE},
      {"imotd", COM_ADMIN, imotd, MAX_STRING_LENGTH, IMOTD_FILE},
      {"help", COM_BUILDER, help, MAX_STRING_LENGTH, HELP_PAGE_FILE},
      {"info", COM_BUILDER, info, MAX_STRING_LENGTH, INFO_FILE},
      {"background", COM_ADMIN, background, MAX_STRING_LENGTH, BACKGROUND_FILE},
      {"handbook", COM_ADMIN, handbook, MAX_STRING_LENGTH, HANDBOOK_FILE},
      {"policies", COM_ADMIN, policies, MAX_STRING_LENGTH, POLICIES_FILE},
      {"namepol", COM_ADMIN, namepol, MAX_STRING_LENGTH, NAMEPOL_FILE},
      {"signoff", COM_ADMIN, signoff, MAX_STRING_LENGTH, SIGNOFF_FILE},
      {"todo", COM_BUILDER, todolist, MAX_STRING_LENGTH, TODO_FILE},
      {"bugs", COM_BUILDER, buglist, MAX_STRING_LENGTH, BUG_FILE},
      {"ideas", COM_BUILDER, idealist, MAX_STRING_LENGTH, IDEA_FILE},
      {"typos", COM_BUILDER, typolist, MAX_STRING_LENGTH, TYPO_FILE},
      {"nmotd", COM_BUILDER, nmotd, MAX_STRING_LENGTH, NMOTD_FILE},
      {"menu", COM_ADMIN, MENU, MAX_STRING_LENGTH, MENU_FILE},
      {"greet1", COM_ADMIN, GREET1, MAX_STRING_LENGTH, GREET1_FILE},
      {"greet2", COM_ADMIN, GREET2, MAX_STRING_LENGTH, GREET2_FILE},
      {"\n", 0, NULL, 0, NULL}
  };

  if (ch->desc == NULL) {
    send_to_char("Get outta here you linkdead head!\r\n", ch);
    return;
  }

  if (!COM_FLAGGED(ch, COM_BUILDER)) {
    send_to_char("You do not have text editor permissions.\r\n", ch);
    return;
  }

  half_chop(argument, field, buf);

  if (!*field) {
    strcpy(buf, "Files available to be edited:\r\n");
    i = 1;
    for (l = 0; *fields[l].cmd != '\n'; l++) {
      if (COM_FLAGGED(ch, fields[l].level)) {
        sprintf(buf + strlen(buf), "%-11.11s", fields[l].cmd);
        if (!(i % 7)) {
          strcat(buf, "\r\n");
        }
        i++;
      }
    }
    if (--i % 7) {
      strcat(buf, "\r\n");
    }
    if (i == 0) {
      strcat(buf, "None.\r\n");
    }
    send_to_char(buf, ch);
    return;
  }
  for (l = 0; *(fields[l].cmd) != '\n'; l++) {
    if (!strncmp(field, fields[l].cmd, strlen(field))) {
      break;
    }
  }

  if (*fields[l].cmd == '\n') {
    send_to_char("Invalid text editor option.\r\n", ch);
    return;
  }

  if (!COM_FLAGGED(ch, fields[l].level)) {
    send_to_char("You don't have permission to do that!\r\n", ch);
    return;
  }

  switch (l) {
    case 0:
      ch->desc->str = &credits;
      ch->desc->max_str = strlen(credits) + fields[l].size;
      break;
    case 1:
      ch->desc->str = &news;
      ch->desc->max_str = strlen(news) + fields[l].size;
      break;
    case 2:
      ch->desc->str = &motd;
      ch->desc->max_str = strlen(motd) + fields[l].size;
      break;
    case 3:
      ch->desc->str = &imotd;
      ch->desc->max_str = strlen(imotd) + fields[l].size;
      break;
    case 4:
      ch->desc->str = &help;
      ch->desc->max_str = strlen(help) + fields[l].size;
      break;
    case 5:
      ch->desc->str = &info;
      ch->desc->max_str = strlen(info) + fields[l].size;
      break;
    case 6:
      ch->desc->str = &background;
      ch->desc->max_str = strlen(background) + fields[l].size;
      break;
    case 7:
      ch->desc->str = &handbook;
      ch->desc->max_str = strlen(handbook) + fields[l].size;
      break;
    case 8:
      ch->desc->str = &policies;
      ch->desc->max_str = strlen(policies) + fields[l].size;
      break;
    case 9:
      ch->desc->str = &namepol;
      ch->desc->max_str = strlen(namepol) + fields[l].size;
      break;
    case 10:
      ch->desc->str = &signoff;
      ch->desc->max_str = strlen(signoff) + fields[l].size;
      break;
    case 11:
      ch->desc->str = &todolist;
      ch->desc->max_str = strlen(todolist) + fields[l].size;
      break;
    case 12:
      ch->desc->str = &buglist;
      ch->desc->max_str = strlen(buglist) + fields[l].size;
      break;
    case 13:
      ch->desc->str = &idealist;
      ch->desc->max_str = strlen(idealist) + fields[l].size;
      break;
    case 14:
      ch->desc->str = &typolist;
      ch->desc->max_str = strlen(typolist) + fields[l].size;
      break;
    case 15:
      ch->desc->str = &nmotd;
      ch->desc->max_str = strlen(nmotd) + fields[l].size;
      break;
    default:
      send_to_char("Invalid text editor option.\r\n", ch);
      return;
  }

  /* set up editor stats */
  send_to_char("\x1B[H\x1B[J", ch);
  send_to_char("Edit file below: (/s saves /h for help)\r\n", ch);
  ch->desc->backstr = NULL;
  if (fields[l].buffer) {
    send_to_char(fields[l].buffer, ch);
    ch->desc->backstr = strdup(fields[l].buffer);
  }
  ch->desc->mail_to = 0;
  ch->desc->storage = strdup(fields[l].filename);
  SET_BIT(PLR_FLAGS(ch), PLR_WRITING);
  sprintf(buf, "OLC: %s begins editing %s", GET_NAME(ch), fields[l].filename);
  mudlog(buf, 'G', COM_BUILDER, TRUE);
  act("$n begins editing a scroll.", TRUE, ch, 0, 0, TO_ROOM);
  STATE(ch->desc) = CON_TEXTED;
}

ACMD(do_restore)
{

  struct descriptor_data *in, *next_desc;
  struct char_data *vict;
  int i;

  one_argument(argument, buf);

  if (!*buf) {
    send_to_char("Whom do you wish to restore?\r\n", ch);
  } else if (!(vict = get_char_vis(ch, buf)) && (str_cmp("all", buf))) {
    send_to_char(NOPERSON, ch);
  } else if (!str_cmp("all", buf) && COM_FLAGGED(ch, COM_BUILDER)) {
    send_to_char(OK, ch);
    sprintf(buf, "%s has restored all", GET_NAME(ch));
    mudlog(buf, 'G', COM_IMMORT, TRUE);
    plog(buf, ch, LVL_IMMORT);

    for (in = descriptor_list; in; in = next_desc) {
      next_desc = in->next;

      if (in->connected || !(vict = in->character)) {
        continue;
      }
      if (GET_LEVEL(in->character) >= LVL_IMMORT) {
        continue;
      }
      GET_HIT(vict) = GET_MAX_HIT(vict);
      GET_MANA(vict) = GET_MAX_MANA(vict);
      GET_MOVE(vict) = GET_MAX_MOVE(vict);
      act("{DA {r*HUGE* {Dhand reaches down from the {Cheavens {Dseemingly to crush the very life\r\n{Dfrom you, leaving as quickly as it came, you feel fully healed.{x", FALSE, vict, 0, ch, TO_CHAR);
      update_pos(vict);
    }
  } else if (str_cmp("all", buf)) {
    GET_HIT(vict) = GET_MAX_HIT(vict);
    GET_MANA(vict) = GET_MAX_MANA(vict);
    GET_MOVE(vict) = GET_MAX_MOVE(vict);

    if ((COM_FLAGGED(ch, COM_BUILDER) && (GET_LEVEL(vict) >= LVL_IMMORT))) {
      for (i = 1; spells[i].command[0] != '\n'; i++) {
        SET_SKILL(vict, spells[i].spellindex, 100);
      }

      if (GET_LEVEL(vict) >= LVL_IMMORT) {
        vict->real_abils.intel = 100;
        vict->real_abils.wis = 100;
        vict->real_abils.dex = 100;
        vict->real_abils.str = 100;
        vict->real_abils.con = 100;
        vict->real_abils.agi = 100;
      }
      vict->aff_abils = vict->real_abils;
    }
    update_pos(vict);
    send_to_char(OK, ch);
    act("{DA {r*HUGE* {Dhand reaches down from the {Cheavens {Dseemingly to crush the very life\r\n{Dfrom you, leaving as quickly as it came, you feel fully healed.{x", FALSE, vict, 0, ch, TO_CHAR);
    sprintf(logbuffer, "%s restored %s", GET_NAME(ch), GET_NAME(vict));
    mudlog(logbuffer, 'G', COM_IMMORT, TRUE);
    plog(logbuffer, ch, MAX(LVL_IMMORT, GET_LEVEL(ch)));
  } else {
    send_to_char("You are not godly enough to do that.\r\n", ch);
  }
}

ACMD(do_invis)
{
  int level = 0;

  one_argument(argument, arg);
  if (!*arg) {
    if (GET_INVIS_LEV(ch) > 0) {
      if (IS_NPC(ch)) {
        GET_MOB_INVIS_LEV(ch) = 0;
      } else {
        GET_PLR_INVIS_LEV(ch) = 0;
      }
      sprintf(buf, "You are now fully visible.\r\n");
    } else {
      if (IS_NPC(ch)) {
        GET_MOB_INVIS_LEV(ch) = GET_LEVEL(ch);
      } else {
        GET_PLR_INVIS_LEV(ch) = GET_LEVEL(ch);
      }
      sprintf(buf, "Your invisibility level is %d.\r\n", GET_LEVEL(ch));
    }
  } else {
    if (!strcmp(arg, "admin") && COM_FLAGGED(ch, COM_ADMIN)) {
      if (IS_NPC(ch)) {
        GET_MOB_INVIS_LEV(ch) = 52;
      } else {
        GET_PLR_INVIS_LEV(ch) = 52;
      }
      sprintf(buf, "You are now invisible to all but admins.\r\n");
    } else {
      if (IS_NPC(ch)) {
        GET_MOB_INVIS_LEV(ch) = 0;
      } else {
        GET_PLR_INVIS_LEV(ch) = 0;
      }
      level = atoi(arg);
      if (level > GET_LEVEL(ch)) {
        sprintf(buf, "You are not high enough level.\r\n");
      } else {
        if (IS_NPC(ch)) {
          GET_MOB_INVIS_LEV(ch) = level;
        } else {
          GET_PLR_INVIS_LEV(ch) = level;
        }
        sprintf(buf, "Your invisibility level is %d.\r\n", GET_INVIS_LEV(ch));
      }
    }
  }
  send_to_char(buf, ch);
}

ACMD(do_gecho)
{
  struct descriptor_data *pt;

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char("That must be a mistake...\r\n", ch);
  } else {
    sprintf(buf, "%s\r\n", argument);
    for (pt = descriptor_list; pt; pt = pt->next) {
      if (!pt->connected && pt->character && pt->character != ch) {
        send_to_char(buf, pt->character);
      }
    }
    if (PRF_FLAGGED(ch, PRF_NOREPEAT)) {
      send_to_char(OK, ch);
    } else {
      send_to_char(buf, ch);
    }
    sprintf(buf, "%s gechoed '%s'", GET_NAME(ch), argument);
    mudlog(buf, 'G', COM_QUEST, FALSE);
    plog(buf, ch, LVL_IMMORT);
  }
}

ACMD(do_zecho)
{
  struct descriptor_data *pt;

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char("That must be a mistake...\r\n", ch);
  } else {
    sprintf(buf, "%s\r\n", argument);
    for (pt = descriptor_list; pt; pt = pt->next) {
      if (!pt->connected && pt->character && pt->character != ch && world[pt->character->in_room].zone == world[ch->in_room].zone) {
        send_to_char(buf, pt->character);
      }
    }
    if (PRF_FLAGGED(ch, PRF_NOREPEAT)) {
      send_to_char(OK, ch);
    } else {
      send_to_char(buf, ch);
    }
    sprintf(buf, "%s zechoed '%s'", GET_NAME(ch), argument);
    mudlog(buf, 'G', COM_QUEST, FALSE);
    plog(buf, ch, LVL_IMMORT);
  }
}

ACMD(do_poofset)
{
  char **msg;

  switch (subcmd) {
    case SCMD_NAMECOLOR:
      msg = &(NAMECOLOR(ch));
      break;
    case SCMD_POOFIN:
      msg = &(POOFIN(ch));
      break;
    case SCMD_POOFOUT:
      msg = &(POOFOUT(ch));
      break;
    case SCMD_WHOIS:
      msg = &(WHOSTR(ch));
      break;
    default:
      return;
  }

  skip_spaces(&argument);

  if (*msg) {
    FREE(*msg);
  }

  if (!*argument) {
    *msg = NULL;
  } else if (subcmd == SCMD_NAMECOLOR) {
    *msg = strdup(colorf(*argument, ch));
  } else {
    *msg = strdup(argument);
  }

  send_to_char(OK, ch);
}

ACMD(do_whoset)
{
  char **msg;

  msg = &(WHOSPEC(ch));

  /*  skip_spaces(&argument);  This one might it impossible to center.. */

  if (*msg) {
    FREE(*msg);
  }

  if (!*argument) {
    *msg = NULL;
  } else {
    *msg = strdup(argument + 1);
  }

  send_to_char(OK, ch);
}

ACMD(do_dc)
{
  struct descriptor_data *d;
  int num_to_dc;

  one_argument(argument, arg);
  if (!(num_to_dc = atoi(arg)) && str_cmp("*", arg)) {
    send_to_char("Usage: DC <connection number> (type USERS for a list)\r\n", ch);
    return;
  }

  if (str_cmp("*", arg)) {
    for (d = descriptor_list; d && d->desc_num != num_to_dc; d = d->next) {
      ;
    }

    if (!d) {
      send_to_char("No such connection.\r\n", ch);
      return;
    }
    if (d->character && (GET_LEVEL(d->character) >= GET_LEVEL(ch) && !COM_FLAGGED(ch, COM_ADMIN))) {
      send_to_char("Umm.. maybe that's not such a good idea...\r\n", ch);
      return;
    }
    close_socket(d);
    sprintf(buf, "Connection #%d closed.\r\n", num_to_dc);
    send_to_char(buf, ch);
    sprintf(buf, "Connection closed by %s.", GET_NAME(ch));
    mudlog(buf, 'J', COM_BUILDER, TRUE);
    stderr_log(buf);
  } else { /* Cut all undefined sessions */
    for (d = descriptor_list; d; d = d->next) {
      if (d->connected != CON_PLAYING && d->connected != CON_REDIT && d->connected != CON_OEDIT && d->connected != CON_MEDIT && d->connected != CON_EXDESC) {
        close_socket(d);
        d = descriptor_list;
      }
    }
    send_to_char("All undefined connections have been closed.\r\n", ch);
    sprintf(buf, "%s closed all undefined connections.", GET_NAME(ch));
    mudlog(buf, 'J', COM_BUILDER, FALSE);
    plog(buf, ch, LVL_IMMORT);
  }
}

ACMD(do_wizlock)
{
  int value;
  char *when;

  one_argument(argument, arg);
  if (*arg) {
    value = atoi(arg);
    if (value < 0 || value > GET_LEVEL(ch)) {
      send_to_char("Invalid wizlock value.\r\n", ch);
      return;
    }
    restrict_game_lvl = value;
    when = "now";
  } else {
    when = "currently";
  }

  switch (restrict_game_lvl) {
    case 0:
      sprintf(buf, "The game is %s completely open.\r\n", when);
      send_to_char(buf, ch);
      buf[strlen(buf)-2]='\0';
      mudlog (buf, 'J', COM_ADMIN, TRUE);
      break;
    case 1:
      sprintf(buf, "The game is %s closed to new players.\r\n", when);
      send_to_char(buf, ch);
      buf[strlen(buf)-2]='\0';
      mudlog (buf, 'J', COM_ADMIN, TRUE);
      break;
    default:
      sprintf(buf, "Only level %d and above may enter the game %s.\r\n",
      restrict_game_lvl, when);
      send_to_char(buf, ch);
      buf[strlen(buf)-2]='\0';
      mudlog (buf, 'J', COM_ADMIN, TRUE);
      break;
  }
}

ACMD(do_date)
{
  char *tmstr;
  time_t mytime;
  int d, h, m;
  extern time_t boot_time;

  if (subcmd == SCMD_DATE) {
    mytime = time(0);
  } else {
    mytime = boot_time;
  }

  tmstr = (char *) asctime(localtime(&mytime));
  *(tmstr + strlen(tmstr) - 1) = '\0';

  if (subcmd == SCMD_DATE) {
    sprintf(buf, "Current machine time: %s\r\n", tmstr);
  } else {
    mytime = time(0) - boot_time;
    d = mytime / 86400;
    h = (mytime / 3600) % 24;
    m = (mytime / 60) % 60;

    sprintf(buf, "Up since %s: %d day%s, %d:%02d\r\n", tmstr, d, ((d == 1) ? "" : "s"), h, m);
  }

  send_to_char(buf, ch);
}

ACMD(do_last)
{
  struct char_data *chdata;
  extern char *class_abbrevs[];
  extern struct lastinfo *LastInfo;
  struct lastinfo *tmpLast;
  char tempname[MAX_INPUT_LENGTH];
  char *tmpc;

  one_argument(argument, arg);
  CREATE(chdata, struct char_data, 1);

  if (!*arg) {
    *buf = '\0';
    tmpLast = LastInfo;
    send_to_char("{CPlr Num    Plr Name            Num Connects    Last On{x\r\n", ch);
    while (tmpLast) {
      memset(tempname, 0, MAX_INPUT_LENGTH);
      sprintf(tempname, "%s", tmpLast->Name);
      if (!load_char_text(tmpLast->Name, chdata)) {
        strcat(tempname, " (deleted)");
      }
      if (*buf != '\0') {
        sprintf(buf + strlen(buf), "{c%5d      %-20s%7d         %20s", tmpLast->PlayerNum, tempname, tmpLast->NumberConnects, asctime(localtime((time_t*) &(tmpLast->Time))));
      } else {
        sprintf(buf, "{c%5d      %-20s%7d         %20s", tmpLast->PlayerNum, tempname, tmpLast->NumberConnects, asctime(localtime((time_t*) &(tmpLast->Time))));
      }
      tmpc = strrchr(buf, '\n');
      if (tmpc) {
        sprintf(tmpc, "\r\n");
      } else {
        strcat(buf, "\r\n");
      }
      tmpLast = tmpLast->next;
    }
    sprintf(buf + strlen(buf), "{x");
    page_string(ch->desc, buf, 1);
    free_char(chdata);
    return;
  }

  if (load_char_text(arg, chdata) < 1) {
    free_char(chdata);
    send_to_char("There is no such player.\r\n", ch);
    return;
  }
  if (GET_LEVEL(ch) < LVL_IMMORT) {
    free_char(chdata);
    send_to_char("You are not allowed to get information about that player.\r\n", ch);
    return;
  }

  if (GET_LEVEL(chdata) > GET_LEVEL(ch)) {
    send_to_char("You are not sufficiently godly for that!\r\n", ch);
    free_char(chdata);
    return;
  }
  sprintf(buf, "{B[{W%5ld{B] [{W%2d %s{B] {c%-12s {w: {c%-18s {w: {c%-20s{x\r\n", GET_IDNUM(chdata), (int) GET_LEVEL(chdata), class_abbrevs[(int) GET_CLASS(chdata)], GET_NAME(chdata), GET_HOST(chdata), ctime(&(GET_LOGON(chdata))));
  send_to_char(buf, ch);
  free_char(chdata);
}

ACMD(do_force)
{
  struct descriptor_data *i, *next_desc;
  struct char_data *vict, *next_force;
  char to_force[MAX_INPUT_LENGTH + 2];
  char buf1[256];

  half_chop(argument, arg, to_force);

  sprintf(buf1, "$n has forced you to '%s'.", to_force);

  if (!*arg || !*to_force) {
    send_to_char("Whom do you wish to force do what?\r\n", ch);
  } else if ((COM_FLAGGED(ch, COM_QUEST)) && (str_cmp("all", arg) && str_cmp("room", arg))) {
    if (!(vict = get_char_vis(ch, arg))) {
      send_to_char(NOPERSON, ch);
    } else if (GET_LEVEL(ch) < GET_LEVEL(vict) && !IS_NPC(vict)) {
      send_to_char("No, no, no!\r\n", ch);
    } else if (IS_IMMO(ch) && IS_IMMO(vict) && !COM_FLAGGED(ch, COM_ADMIN)) {
      send_to_char("No, no, no!\r\n", ch);
    } else if (!COM_FLAGGED(ch, COM_ADMIN) && COM_FLAGGED(vict, COM_ADMIN)) {
      send_to_char("You are not able to force an admin.\r\n", ch);
      sprintf(buf, "{Y%s attempted to force you to, '%s'{x\r\n", GET_NAME(ch), to_force);
      send_to_char(buf, vict);
    } else {
      int oktosee = 0;
      int zonenum = 0;
      int n;

      if (IS_NPC(vict)) {
        zonenum = (GET_MOB_VNUM(vict) - (GET_MOB_VNUM(vict) % 100)) / 100;
        if (zonenum != 12 && zonenum != 30 && zonenum != 31 && !COM_FLAGGED(ch, COM_ADMIN)) {
          for (n = 0; n < 4; n++) {
            if (ch->olc_zones[n] == zonenum) {
              oktosee = 1;
            }
          }
          if (!oktosee || zonenum == 0) {
            send_to_char("You are not authorized to see info in this zone.\r\n", ch);
            return;
          }
        }
      }
      if (!COM_FLAGGED(ch, COM_QUEST) && !IS_NPC(vict)) {
        send_to_char("No, no, no! You are only allowed to force mobs!\r\n", ch);
      } else {
        send_to_char(OK, ch);
        act(buf1, TRUE, ch, NULL, vict, TO_VICT);
        sprintf(buf, "%s forced %s to %s", GET_NAME(ch), GET_NAME(vict), to_force);
        mudlog(buf, 'G', COM_QUEST, TRUE);
        plog(buf, ch, LVL_IMMORT);
        command_interpreter(vict, to_force);
      }
    }
  } else if ((!str_cmp("room", arg)) && COM_FLAGGED(ch, COM_BUILDER)) {
    send_to_char(OK, ch);
    sprintf(buf, "%s forced room %d to %s", GET_NAME(ch), world[ch->in_room].number, to_force);
    mudlog(buf, 'G', COM_QUEST, TRUE);
    plog(buf, ch, LVL_IMMORT);
    for (vict = world[ch->in_room].people; vict; vict = next_force) {
      next_force = vict->next_in_room;
      if (GET_LEVEL(vict) > GET_LEVEL(ch)) {
        continue;
      }
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      command_interpreter(vict, to_force);
    }
  } else if ((!str_cmp("all", arg)) && COM_FLAGGED(ch, COM_ADMIN)) {
    send_to_char(OK, ch);
    sprintf(buf, "%s forced all to %s", GET_NAME(ch), to_force);
    mudlog(buf, 'G', COM_QUEST, TRUE);
    plog(buf, ch, LVL_IMMORT);
    for (i = descriptor_list; i; i = next_desc) {
      next_desc = i->next;

      if (i->connected || !(vict = i->character) || GET_LEVEL(vict) > GET_LEVEL(ch)) {
        continue;
      }
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      command_interpreter(vict, to_force);
    }
  }
}

ACMD(do_wiznet)
{
  struct descriptor_data *d;
  char emote = FALSE;
  char any = FALSE;
  int level = COM_IMMORT;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument && IS_IMMO(ch)) {
    send_to_char("Usage: wiznet <text> | #<level> <text> | wiz @\r\n", ch);
    return;
  } else if (!*argument) {
    send_to_char("Huh?\r\n", ch);
    return;
  }

  switch (*argument) {
    /*  case '*':
     emote = TRUE; */
    case '#':
      one_argument(argument + 1, buf1);
      if (*buf1) {
        half_chop(argument + 1, buf1, argument);

        if (!strcmp(buf1, "im")) {
          level = COM_IMMORT;
        } else if (!strcmp(buf1, "qu")) {
          level = COM_QUEST;
        } else if (!strcmp(buf1, "bu")) {
          level = COM_BUILDER;
        } else if (!strcmp(buf1, "ad")) {
          level = COM_ADMIN;
        }

        if (!COM_FLAGGED(ch, level)) {
          send_to_char("You can't wiznet above your own level.\r\n", ch);
          return;
        }
      } else if (emote) {
        argument++;
      }
      break;
    case '@':
      for (d = descriptor_list; d; d = d->next) {
        if (!d->connected && GET_LEVEL(d->character) >= LVL_IMMORT && !PRF_FLAGGED(d->character, PRF_NOWIZ) && (CAN_SEE(ch, d->character) || COM_FLAGGED(ch, COM_ADMIN))) {
          if (!any) {
            sprintf(buf1, "Gods online:\r\n");
            any = TRUE;
          }
          sprintf(buf1 + strlen(buf1), "  %s", GET_NAME(d->character));
          if (PLR_FLAGGED(d->character, PLR_WRITING)) {
            sprintf(buf1 + strlen(buf1), " (Writing)\r\n");
          } else if (PLR_FLAGGED(d->character, PLR_MAILING)) {
            sprintf(buf1 + strlen(buf1), " (Writing mail)\r\n");
          } else {
            sprintf(buf1 + strlen(buf1), "\r\n");
          }
        }
      }
      any = FALSE;
      for (d = descriptor_list; d; d = d->next) {
        if (!d->connected && GET_LEVEL(d->character) >= LVL_IMMORT && PRF_FLAGGED(d->character, PRF_NOWIZ) && CAN_SEE(ch, d->character)) {
          if (!any) {
            sprintf(buf1 + strlen(buf1), "Gods offline:\r\n");
            any = TRUE;
          }
          sprintf(buf1 + strlen(buf1), "  %s\r\n", GET_NAME(d->character));
        }
      }
      send_to_char(buf1, ch);
      return;
    case '\\':
      ++argument;
      break;
    default:
      break;
  }
  if (PRF_FLAGGED(ch, PRF_NOWIZ)) {
    send_to_char("You are offline!\r\n", ch);
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char("Don't bother the gods like that!\r\n", ch);
    return;
  }
  if (level > COM_IMMORT) {
    char channel[5];

    strcpy(channel, buf1);
    sprintf(buf1, "{W::{C%s{W:: <{C%s{W> {c%s%s{x\r\n", GET_NAME(ch), channel, emote ? "<--- " : "", argument);
    sprintf(buf2, "{W::{CSomeone{W:: <{C%s{W> {c%s%s{x\r\n", channel, emote ? "<--- " : "", argument);
  } else {
    sprintf(buf1, "{W::{C%s{W:: {c%s%s{x\r\n", GET_NAME(ch), emote ? "<--- " : "", argument);
    sprintf(buf2, "{W::{CSomeone{W:: {c%s%s{x\r\n", emote ? "<--- " : "", argument);
  }

  for (d = descriptor_list; d; d = d->next) {
    if ((!d->connected) && (COM_FLAGGED(d->character, level)) && (!PRF_FLAGGED(d->character, PRF_NOWIZ)) && !IS_NPC(d->character) && (!PLR_FLAGGED(d->character, PLR_WRITING | PLR_MAILING)) && (d != ch->desc || !(PRF_FLAGGED(d->character, PRF_NOREPEAT)))) {
      send_to_char(CCCYN(d->character, C_NRM), d->character);
      if (CAN_SEE(d->character, ch)) {
        send_to_char(buf1, d->character);
      } else {
        send_to_char(buf2, d->character);
      }
      send_to_char(CCNRM(d->character, C_NRM), d->character);
    }
  }

  if (PRF_FLAGGED(ch, PRF_NOREPEAT)) {
    send_to_char(OK, ch);
  }
}

ACMD(do_iquest)
{
  struct descriptor_data *d;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument) {
    send_to_char("Usage: iquest <text>\r\n", ch);
    return;
  }

  if (PRF_FLAGGED(ch, PRF_NOIMMQUEST)) {
    send_to_char("You are offline!\r\n", ch);
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char("Don't bother the gods like that!\r\n", ch);
    return;
  }

  sprintf(buf1, "{m<<{W%s{m>> {W%s{x\r\n", GET_NAME(ch), argument);
  sprintf(buf2, "{m<<{WSomeone{m>> {W%s{x\r\n", argument);

  for (d = descriptor_list; d; d = d->next) {
    if ((!d->connected) && (COM_FLAGGED(d->character, COM_QUEST)) && (!PRF_FLAGGED(d->character, PRF_NOWIZ)) && !IS_NPC(d->character) && (!PLR_FLAGGED(d->character, PLR_WRITING | PLR_MAILING)) && (d != ch->desc || !(PRF_FLAGGED(d->character, PRF_NOREPEAT)))) {
      send_to_char(CBYEL(d->character, C_NRM), d->character);
      if (CAN_SEE(d->character, ch)) {
        send_to_char(buf1, d->character);
      } else {
        send_to_char(buf2, d->character);
      }
      send_to_char(CCNRM(d->character, C_NRM), d->character);
    }
  }

  if (PRF_FLAGGED(ch, PRF_NOREPEAT)) {
    send_to_char(OK, ch);
  }
}

ACMD(do_zreset)
{
  void reset_zone(int zone);
  int i, j;

  one_argument(argument, arg);
  if (!*arg) {
    send_to_char("You must specify a zone.\r\n", ch);
    return;
  }
  if (*arg == '*') {
    for (i = 0; i <= top_of_zone_table; i++) {
      reset_zone(i);
    }
    send_to_char("Reset world.\r\n", ch);
    return;
  } else if (*arg == '.') {
    i = world[ch->in_room].zone;
  } else {
    j = atoi(arg);
    for (i = 0; i <= top_of_zone_table; i++) {
      if (zone_table[i].number == j) {
        break;
      }
    }
  }
  if (i >= 0 && i <= top_of_zone_table) {
    reset_zone(i);
    sprintf(buf, "Reset zone %d (#%d): %s.\r\n", i, zone_table[i].number, zone_table[i].name);
    send_to_char(buf, ch);
    sprintf(buf, "%s reset zone #%d (%s)", GET_NAME(ch), zone_table[i].number, zone_table[i].name);
    mudlog(buf, 'G', COM_BUILDER, TRUE);
    plog(buf, ch, MAX(LVL_IMMORT, GET_LEVEL(ch)));
  } else {
    send_to_char("Invalid zone number.\r\n", ch);
  }
}

/*
 *  General fn for wizcommands of the sort: cmd <player>
 */

ACMD(do_wizutil)
{
  struct char_data *vict;
  long result;
  void roll_real_abils(struct char_data *ch);

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Yes, but for whom?!?\r\n", ch);
  } else if (!(vict = get_char_vis(ch, arg))) {
    send_to_char("There is no such player.\r\n", ch);
  } else if (IS_NPC(vict)) {
    send_to_char("You can't do that to a mob!\r\n", ch);
  } else if (GET_LEVEL(vict) > GET_LEVEL(ch)) {
    send_to_char("Hmmm...you'd better not.\r\n", ch);
  } else {
    switch (subcmd) {
      case SCMD_REROLL:
        send_to_char("Rerolled...\r\n", ch);
        roll_real_abils(vict);
        sprintf(buf, "%s has rerolled %s.", GET_NAME(ch), GET_NAME(vict));
        mudlog(buf, 'X', COM_ADMIN, TRUE);
        stderr_log(buf);
        sprintf(buf, "Stats: Str %d, Int %d, Wis %d, Dex %d, Con %d, Agi %d, Wgt %d, Hgt %d\r\n", GET_STR(vict), GET_INT(vict), GET_WIS(vict), GET_DEX(vict), GET_CON(vict), GET_AGI(vict), GET_WEIGHT(vict), GET_HEIGHT(vict));
        send_to_char(buf, ch);
        break;
      case SCMD_PARDON:
        if (!PLR_FLAGGED(vict, PLR_THIEF | PLR_KILLER)) {
          send_to_char("Your victim is not flagged.\r\n", ch);
          return;
        }
        REMOVE_BIT(PLR_FLAGS(vict), PLR_THIEF | PLR_KILLER);
        send_to_char("Pardoned.\r\n", ch);
        send_to_char("You have been pardoned by the Gods!\r\n", vict);
        sprintf(buf, "%s pardoned by %s", GET_NAME(vict), GET_NAME(ch));
        mudlog(buf, 'G', COM_IMMORT, TRUE);
        plog(buf, ch, LVL_IMMORT);
        break;
      case SCMD_NOTITLE:
        result = PLR_TOG_CHK(vict, PLR_NOTITLE);
        sprintf(buf, "Notitle %s for %s by %s.", ONOFF(result), GET_NAME(vict), GET_NAME(ch));
        mudlog(buf, 'G', COM_BUILDER, TRUE);
        plog(buf, ch, LVL_IMMORT);
        plog(buf, vict, LVL_IMMORT);
        strcat(buf, "\r\n");
        send_to_char(buf, ch);
        break;
      case SCMD_NOEMOTE:
        result = PLR_TOG_CHK(vict, PLR_NOEMOTE);
        sprintf(buf, "Noemote %s for %s by %s.", ONOFF(result), GET_NAME(vict), GET_NAME(ch));
        mudlog(buf, 'G', COM_QUEST, TRUE);
        plog(buf, ch, LVL_IMMORT);
        plog(buf, vict, LVL_IMMORT);
        strcat(buf, "\r\n");
        send_to_char(buf, ch);
        break;
      case SCMD_NOSOCIAL:
        result = PLR_TOG_CHK(vict, PLR_NOSOCIAL);
        sprintf(buf, "Nosocial %s for %s by %s.", ONOFF(result), GET_NAME(vict), GET_NAME(ch));
        mudlog(buf, 'G', COM_QUEST, TRUE);
        plog(buf, ch, LVL_IMMORT);
        plog(buf, vict, LVL_IMMORT);
        strcat(buf, "\r\n");
        send_to_char(buf, ch);
        break;
      case SCMD_SQUELCH:
        result = PLR_TOG_CHK(vict, PLR_NOSHOUT);
        sprintf(buf, "Mute %s for %s by %s.", ONOFF(result), GET_NAME(vict), GET_NAME(ch));
        mudlog(buf, 'G', COM_QUEST, TRUE);
        plog(buf, ch, LVL_IMMORT);
        plog(buf, vict, LVL_IMMORT);
        strcat(buf, "\r\n");
        send_to_char(buf, ch);
        break;
      case SCMD_FREEZE:
        if (ch == vict) {
          send_to_char("Oh, yeah, THAT'S real smart...\r\n", ch);
          return;
        }
        if (PLR_FLAGGED(vict, PLR_FROZEN)) {
          send_to_char("Your victim is already pretty cold.\r\n", ch);
          return;
        }
        SET_BIT(PLR_FLAGS(vict), PLR_FROZEN);
        GET_FREEZE_LEV(vict) = GET_LEVEL(ch);
        send_to_char("A bitter wind suddenly rises and drains every erg of heat from your body!\r\nYou feel frozen!\r\n", vict);
        send_to_char("Frozen.\r\n", ch);
        act("A sudden cold wind conjured from nowhere freezes $n!", FALSE, vict, 0, 0, TO_ROOM);
        sprintf(buf, "%s frozen by %s.", GET_NAME(vict), GET_NAME(ch));
        mudlog(buf, 'G', COM_BUILDER, TRUE);
        plog(buf, ch, LVL_IMMORT);
        plog(buf, vict, LVL_IMMORT);
        break;
      case SCMD_THAW:
        if (!PLR_FLAGGED(vict, PLR_FROZEN)) {
          send_to_char("Sorry, your victim is not morbidly encased in ice at the moment.\r\n", ch);
          return;
        }
        if (GET_FREEZE_LEV(vict) > GET_LEVEL(ch)) {
          sprintf(buf, "Sorry, a level %d God froze %s... you can't unfreeze %s.\r\n", GET_FREEZE_LEV(vict), GET_NAME(vict), HMHR(vict));
          send_to_char(buf, ch);
          return;
        }
        sprintf(buf, "%s un-frozen by %s.", GET_NAME(vict), GET_NAME(ch));
        mudlog(buf, 'G', COM_BUILDER, TRUE);
        plog(buf, ch, LVL_IMMORT);
        plog(buf, vict, LVL_IMMORT);
        REMOVE_BIT(PLR_FLAGS(vict), PLR_FROZEN);
        send_to_char("A fireball suddenly explodes in front of you, melting the ice!\r\nYou feel thawed.\r\n", vict);
        send_to_char("Thawed.\r\n", ch);
        act("A sudden fireball conjured from nowhere thaws $n!", FALSE, vict, 0, 0, TO_ROOM);
        break;
      case SCMD_UNAFFECT:
        if (vict->affected) {
          while (vict->affected)
            affect_remove(vict, vict->affected);
          send_to_char("There is a brief flash of light!\r\n"
              "You feel slightly different.\r\n", vict);
          send_to_char("All spells removed.\r\n", ch);
        } else {
          send_to_char("Your victim does not have any affections!\r\n", ch);
          return;
        }
        sprintf(buf, "%s's affections removed by %s", GET_NAME(vict), GET_NAME(ch));
        mudlog(buf, 'G', COM_QUEST, TRUE);
        break;
      case SCMD_OUTLAW:
        result = PLR_TOG_CHK(vict, PLR_OUTLAW);
        sprintf(buf, "Outlaw %s for %s by %s.", ONOFF(result), GET_NAME(vict), GET_NAME(ch));
        mudlog(buf, 'G', COM_ADMIN, TRUE);
        plog(buf, ch, LVL_IMMORT);
        plog(buf, vict, LVL_IMMORT);
        strcat(buf, "\r\n");
        send_to_char(buf, ch);
        break;
      default:
        stderr_log("SYSERR: Unknown subcmd passed to do_wizutil (act.wizard.c)");
        break;
    }
    save_char_text(vict, NOWHERE);
  }
}

/* single zone printing fn used by "show zone" so it's not repeated in the
 code 3 times ... -je, 4/6/93 */

void print_zone_to_buf(char *bufptr, int zone)
{
  sprintf(bufptr + strlen(bufptr), "%3d %-30.30s Age: %3d; Reset: %3d (%1d); Top: %5d\r\n", zone_table[zone].number, zone_table[zone].name, zone_table[zone].age, zone_table[zone].lifespan, zone_table[zone].reset_mode, zone_table[zone].top);
}

char* get_spell_argument(spell* spell_pointer)
{
  if (spell_pointer == spell_char) {
    return "It requires a char type argument.\r\n";
  } else if (spell_pointer == spell_general) {
    return "The argument is either ignored or is parsed specially.\r\n";
  } else if (spell_pointer == spell_obj_char) {
    return "It requires a char or an obj type argument.\r\n";
  } else if (spell_pointer == spell_obj) {
    return "It requires an obj type argument.\r\n";
  } else if (spell_pointer == spell_obj_room) {
    return "It requires an obj type argument or no argument (for rooms).\r\n";
  }
  return "This could be a skill, or a bad spell define.\r\n";
}

char* get_event_type(event* event_pointer)
{
  if (event_pointer == spell_teleport_event) {
    return "It is a Teleport spell.\r\n";
  } else if (event_pointer == spell_magic_missile_event) {
    return "It is a Magic Missile spell.\r\n";
  } else if (event_pointer == spell_word_recall_event) {
    return "It is a Word of Recall spell.\r\n";
  } else if (event_pointer == spell_dam_event) {
    return "It is a spell that damages a char.\r\n";
  } else if (event_pointer == spell_char_event) {
    return "It is a spell that affects a char.\r\n";
  } else if (event_pointer == spell_points_event) {
    return "It is a spell that changes a char points.\r\n";
  } else if (event_pointer == spell_obj_event) {
    return "It is a spell that affects a object.\r\n";
  } else if (event_pointer == spell_obj_room_event) {
    return "It is a spell that affects a object or room.\r\n";
  } else if (event_pointer == spell_room_event) {
    return "It is a spell that affects a room.\r\n";
  } else if (event_pointer == spell_area_event) {
    return "It is a spell that affects every char in room.\r\n";
  } else if (event_pointer == spell_dimdoor_event) {
    return "It is a dimdoor spell.\r\n";
  } else if (event_pointer == spell_locate_obj_event) {
    return "This spell will find an object.\r\n";
  } else if (event_pointer == spell_create_obj_event) {
    return "This spell will create an object.\r\n";
  } else if (event_pointer == spell_area_points_event) {
    return "It is a spell that changes the points of every char in room.\r\n";
  } else if (event_pointer == spell_summon_event) {
    return "This spell will summon a player.\r\n";
  } else if (event_pointer == spell_create_mob_event) {
    return "This spell will create a mobile.\r\n";
  } else if (event_pointer == spell_group_event) {
    return "This spell will affect all in group.\r\n";
  } else if (event_pointer == spell_obj_char_event) {
    return "This spell will affect either an object or a char.\r\n";
  } else if (event_pointer == spell_area_dam_event) {
    return "This spell will damage all in room.\r\n";
  } else if (event_pointer == spell_confusion_event) {
    return "This is the confusion spell.\r\n";
  } else if (event_pointer == spell_charm_event) {
    return "This spell will charm a mobile.\r\n";
  } else if (event_pointer == spell_telekinesis_event) {
    return "This spell will raise an object out of the water.\r\n";
  } else if (event_pointer == spell_turn_undead_event) {
    return "This is the turn undead spell.\r\n";
  } else if (event_pointer == spell_identify_event) {
    return "This spell will identify an object.\r\n";
  } else if (event_pointer == spell_dispel_magic_event) {
    return "This spell will remove some of a char's magic affections.\r\n";
  }
  return "Dunno what this spell does.\r\n";
}

void show_spell_info(struct char_data *ch, char *spellname)
{
  int spellnum;
  int i;
  extern char *class_abbrevs[];

  if (!spellname) {
    send_to_char("You must provide a spell to look up.\r\n", ch);
    return;
  }

  spellnum = find_spell_num(spellname);
  if (spellnum == -1) {
    spellnum = find_skill_num(spellname);
    if (spellnum == -1) {
      send_to_char("You must provide a valid spell to look up.\r\n", ch);
      return;
    }
  }

  sprintf(buf, "%d) %s is a %s\r\n", spells[spellnum].spellindex, spells[spellnum].command, (spells[spellnum].event_pointer ? "SPELL" : "SKILL"));

  if (spells[spellnum].event_pointer) {
    sprintf(buf + strlen(buf), "%s", get_spell_argument(spells[spellnum].spell_pointer));
    sprintf(buf + strlen(buf), "%s", get_event_type(spells[spellnum].event_pointer));
    sprintf(buf + strlen(buf), "You must be in at least %s position to use this spell.\r\n", position_types[spells[spellnum].min_position]);
    sprintf(buf + strlen(buf), "Maximum mana: %d\tMinimum mana: %d\t Amount change per level: %d\r\n", spells[spellnum].mana_max, spells[spellnum].mana_min, spells[spellnum].mana_change);
    sprintf(buf + strlen(buf), "It is a %s spell and is %s to learn.\r\n", (spells[spellnum].aggressive ? "aggressive" : "non-aggressive"), difficulty[spells[find_skill_num_def(spells[spellnum].realm)].difficulty]);
    sprintf(buf + strlen(buf), "This spell is in the %s.\r\n", spells[find_skill_num_def(spells[spellnum].realm)].command);
    if (spells[spellnum].unaffect) {
      sprintf(buf + strlen(buf), "This spell removes %s.\r\n", spells[spellnum].unaffect);
    }
    sprintf(buf + strlen(buf), "Invisible: %s\r\n", YESNO(spells[spellnum].invisible));
    if (spells[spellnum].num_dice || spells[spellnum].size_dice) {
      sprintf(buf + strlen(buf), "Num dice: %d, Size dice: %d, Avg dice %d\r\n", spells[spellnum].num_dice, spells[spellnum].size_dice, ((spells[spellnum].num_dice + (spells[spellnum].size_dice * spells[spellnum].num_dice)) / 2));
    }
    sprintf(buf + strlen(buf), "Resist: %s\r\n", resist_short_name[spells[spellnum].resist_type - 1]);
    sprintf(buf + strlen(buf), "Saving Throw: %s\r\n", saving_throws[spells[spellnum].saving_throw]);
  } else {
    sprintf(buf + strlen(buf), "You must be in at least %s position to use this skill.\r\n", position_types[spells[spellnum].min_position]);
    sprintf(buf + strlen(buf), "It is a %s skill and is %s to learn.\r\n", (spells[spellnum].aggressive ? "aggressive" : "non-aggressive"), difficulty[spells[find_skill_num_def(spells[spellnum].realm)].difficulty]);
  }
  sprintf(buf + strlen(buf), "\r\nLevels assigned to each class:\r\n");
  for (i = 0; i < NUM_CLASSES; i++) {
    if (i == 9) {
      sprintf(buf + strlen(buf), " %s:%2d\r\n", class_abbrevs[i], spells[spellnum].min_level[i]);
    } else if (i == 0 || i == 10) {
      sprintf(buf + strlen(buf), "%s:%2d", class_abbrevs[i], spells[spellnum].min_level[i]);
    } else {
      sprintf(buf + strlen(buf), " %s:%2d", class_abbrevs[i], spells[spellnum].min_level[i]);
    }
  }
  strcat(buf, "\r\n");
  page_string(ch->desc, buf, 1);
}

ACMD(do_show)
{
  struct char_data *vbuf;
  int i, j, k, l, con;
  char self = 0;
  struct char_data *vict;
  struct obj_data *obj;
  char field[40], value[40], birth[80], spell[80];
  extern char *todolist;
  extern char *buglist;
  extern char *idealist;
  extern char *typolist;
  extern char *helplist;
  extern char *class_abbrevs[];
  extern char *genders[];
  extern int buf_switches, buf_largecount, buf_overflows;
  void show_shops(struct char_data * ch, char *value);

  struct show_struct {
    char *cmd;
    char level;
  } fields[] = {
      {"nothing", 1}, /* 0 */
      {"stats", 1},
      {"zones", COM_IMMORT}, /* 1 */
      {"player", COM_BUILDER},
      {"rent", COM_BUILDER},
      {"errors", COM_ADMIN}, /* 5 */
      {"death", COM_BUILDER},
      {"godrooms", COM_BUILDER},
      {"shops", COM_IMMORT},
      {"todo", COM_BUILDER},
      {"bugs", COM_BUILDER}, /* 10 */
      {"ideas", COM_IMMORT},
      {"typos", COM_IMMORT},
      {"heal", COM_BUILDER},
      {"connections", COM_BUILDER},
      {"islands", COM_BUILDER}, /* 15 */
      {"sinfo", COM_BUILDER},
      {"help", COM_IMMORT},
      {"\n", 0} /* 18 */
  };

  skip_spaces(&argument);

  if (!*argument) {
    strcpy(buf, "Show options:\r\n");
    for (j = 0, i = 0; fields[i].level; i++) {
      if (i < 2 || COM_FLAGGED(ch, fields[i].level)) {
        sprintf(buf + strlen(buf), "%-15s%s", fields[i].cmd, (!(++j % 5) ? "\r\n" : ""));
      }
    }
    strcat(buf, "\r\n");
    send_to_char(buf, ch);
    return;
  }

  strcpy(arg, two_arguments(argument, field, value));

  if (!strcmp(field, "sinfo")) {
    if (*arg) {
      sprintf(spell, "%s%s", value, arg);
    } else {
      strcpy(spell, value);
    }
  }

  for (l = 0; *(fields[l].cmd) != '\n'; l++) {
    if (!strncmp(field, fields[l].cmd, strlen(field))) {
      break;
    }
  }

  if (*fields[l].cmd == '\n') {
    send_to_char("There is no such option!\r\n", ch);
    return;
  }

  if (l > 1 && !COM_FLAGGED(ch, fields[l].level)) {
    send_to_char("You are not godly enough for that!\r\n", ch);
    return;
  }
  if (!strcmp(value, ".")) {
    self = 1;
  }
  buf[0] = '\0';
  switch (l) {
    case 0:
      send_to_char("You see what you expected... nothing.\r\n", ch);
      return;
    case 1:
      i = 0;
      j = 0;
      k = 0;
      con = 0;
      for (vict = character_list; vict; vict = vict->next) {
        if (IS_NPC(vict)) {
          j++;
        } else if (CAN_SEE(ch, vict)) {
          i++;
          if (vict->desc) {
            con++;
          }
        }
      }
      for (obj = object_list; obj; obj = obj->next) {
        k++;
      }
      sprintf(buf, "Current stats:\r\n");
      sprintf(buf + strlen(buf), "  %5d players in game  %5d connected\r\n", i, con);
      sprintf(buf + strlen(buf), "  %5d registered\r\n", top_of_p_table + 1);
      sprintf(buf + strlen(buf), "  %5d mobiles          %5d prototypes\r\n", j, top_of_mobt + 1);
      sprintf(buf + strlen(buf), "  %5d objects          %5d prototypes\r\n", k, top_of_objt + 1);
      sprintf(buf + strlen(buf), "  %5d rooms            %5d zones\r\n", top_of_world + 1, top_of_zone_table + 1);
      sprintf(buf + strlen(buf), "  %5d large bufs\r\n", buf_largecount);
      sprintf(buf + strlen(buf), "  %5d buf switches     %5d overflows\r\n", buf_switches, buf_overflows);
      sprintf(buf + strlen(buf), "  %5d deaths\r\n", death_count);
      send_to_char(buf, ch);
      break;
    case 2: /* zone */
      /* tightened up by JE 4/6/93 */
      if (self) {
        print_zone_to_buf(buf, world[ch->in_room].zone);
      } else if (*value && is_number(value)) {
        for (j = atoi(value), i = 0; zone_table[i].number != j && i <= top_of_zone_table; i++) {
          ;
        }
        if (i <= top_of_zone_table) {
          print_zone_to_buf(buf, i);
        } else {
          send_to_char("That is not a valid zone.\r\n", ch);
          return;
        }
      } else {
        for (i = 0; i <= top_of_zone_table; i++) {
          print_zone_to_buf(buf, i);
        }
      }
      page_string(ch->desc, buf, 1);
      break;
    case 3: /* player */
      if (!*value) {
        send_to_char("A name would help.\r\n", ch);
        return;
      }

      CREATE(vbuf, struct char_data, 1);
      if (load_char_text(value, vbuf) < 1) {
        send_to_char("There is no such player.\r\n", ch);
        free_char(vbuf);
        return;
      }
      sprintf(buf, "Player: %-12s (%s) [%2d %s]\r\n", GET_NAME(vbuf), genders[(int) GET_SEX(vbuf)], GET_LEVEL(vbuf), class_abbrevs[(int) GET_CLASS(vbuf)]);
      sprintf(buf + strlen(buf), "Au: %-8d  Bal: %-8d  Exp: %-8d  Align: %-5d  Lessons: %-3d\r\n", GET_GOLD(vbuf), GET_BANK_GOLD(vbuf), GET_EXP(vbuf), GET_ALIGNMENT(vbuf), GET_PRACTICES(vbuf));
      strcpy(birth, (char*) ctime(&GET_BIRTH(vbuf)));
      sprintf(buf + strlen(buf), "Started: %-20.16s  Last: %-20.16s  Played: %3dh %2dm\r\n", birth, ctime(&GET_LOGON(vbuf)), (int) (GET_PLAYED(vbuf) / 3600), (int) ((GET_PLAYED(vbuf) / 60) % 60));
      free_char(vbuf);
      send_to_char(buf, ch);
      break;
    case 4:
      Crash_listrent(ch, value);
      break;
    case 5:
      strcpy(buf, "Errant Rooms\r\n------------\r\n");
      for (i = 0, k = 0; i <= top_of_world; i++) {
        for (j = 0; j < NUM_OF_DIRS; j++) {
          if (world[i].dir_option[j] && world[i].dir_option[j]->to_room == 0) {
            sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n", ++k, world[i].number, world[i].name);
          }
        }
      }
      send_to_char(buf, ch);
      break;
    case 6:
      strcpy(buf, "Death Traps\r\n-----------\r\n");
      for (i = 0, j = 0; i <= top_of_world; i++) {
        if (IS_SET(ROOM_FLAGS(i), ROOM_DEATH)) {
          sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n", ++j, world[i].number, world[i].name);
        }
      }
      send_to_char(buf, ch);
      break;
    case 7:
#define GOD_ROOMS_ZONE 1
      strcpy(buf, "Godrooms\r\n--------------------------\r\n");
      for (i = 0, j = 0; i < top_of_world; i++) {
        if (world[i].zone == GOD_ROOMS_ZONE) {
          sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n", j++, world[i].number, world[i].name);
        }
      }
      send_to_char(buf, ch);
      break;
    case 8:
      show_shops(ch, value);
      break;
    case 9:
      if (todolist != NULL) {
        page_string_no_color(ch->desc, todolist, 1);
      }
      break;
    case 10:
      if (buglist != NULL) {
        page_string_no_color(ch->desc, buglist, 1);
      }
      break;
    case 11:
      if (idealist != NULL) {
        page_string_no_color(ch->desc, idealist, 1);
      }
      break;
    case 12:
      if (typolist != NULL) {
        page_string_no_color(ch->desc, typolist, 1);
      }
      break;
    case 13:
      strcpy(buf, "Fastheal Rooms\r\n-----------\r\n");
      for (i = 0, j = 0; i <= top_of_world; i++) {
        if (IS_SET(ROOM_FLAGS(i), ROOM_FASTHEAL)) {
          sprintf(buf + strlen(buf), "%2d: [%5d] %s\r\n", ++j, world[i].number, world[i].name);
        }
      }
      send_to_char(buf, ch);
      break;
    case 14:
      for (j = (self ? zone_table[world[ch->in_room].zone].number : atoi(value)), i = 0; zone_table[i].number != j && i <= top_of_zone_table; i++) {
        ;
      }
      if (i > top_of_zone_table) {
        sprintf(buf, "That is not a valid zone.\r\n");
      } else {
        sprintf(buf, "Connections in zone %d.\r\n", j);
        strcat(buf, "========================\r\n");
        for (i = 0; i <= top_of_world; i++) {
          if (zone_table[world[i].zone].number == j) {
            for (k = 0; k < NUM_OF_DIRS; k++) {
              if (world[i].dir_option[k] && world[i].dir_option[k]->to_room != NOWHERE && zone_table[world[world[i].dir_option[k]->to_room].zone].number != j) {
                sprintf(buf + strlen(buf), "%5d exit %s leads to %-5d.\r\n", world[i].number, dirs[k], world[world[i].dir_option[k]->to_room].number);
              }
            }
          }
        }
      }
      page_string(ch->desc, buf, 1);
      break;
    case 15:
      {
        int counter;

        sprintf(buf, "Rooms with no valid exits.\r\n");
        strcat(buf, "==========================\r\n");
        for (i = 0; i <= top_of_world; i++) {
          for (k = 0, counter = 0; k < NUM_OF_DIRS; k++) {
            if (world[i].dir_option[k] && world[i].dir_option[k]->to_room != i && world[i].dir_option[k]->to_room != NOWHERE) {
              counter = 1;
            }
          }
          if (!counter) {
            sprintf(buf + strlen(buf), "%5d has no valid exits.\r\n", world[i].number);
          }
        }
        page_string(ch->desc, buf, 1);
      }
      break;
    case 16:
      show_spell_info(ch, spell);
      break;
    case 17:
      if (helplist != NULL) {
        page_string_no_color(ch->desc, helplist, 1);
      }
      break;
    default:
      send_to_char("Sorry, I don't understand that.\r\n", ch);
      break;
  }
}

#define SET_OR_REMOVE(flagset, flags) { \
  if (on) SET_BIT(flagset, flags); \
  else if (off) REMOVE_BIT(flagset, flags); }

#define RANGE(low, high) (value = BOUNDED((low), (value), (high)))

ACMD(do_set)
{

#define NUMLOG(str, num) sprintf(logbuffer, str, GET_NAME(ch), num, GET_NAME(vict)); mudlog(logbuffer, 'G', COM_BUILDER, TRUE); plog(logbuffer, ch, MAX(LVL_IMMORT, GET_LEVEL(ch)));
#define FLAGLOG(str) sprintf(logbuffer, "%s set flag %s %s for %s", GET_NAME(ch), str, ONOFF(on), GET_NAME(vict)); mudlog(logbuffer, 'F', COM_ADMIN, TRUE); plog(logbuffer, ch, MAX(LVL_IMMORT, GET_LEVEL(ch)));
  int i, l, j;
  struct char_data *vict;
  struct char_data *cbuf;
  char field[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], val_arg[MAX_INPUT_LENGTH];
  int on = 0, off = 0, value = 0;
  char is_file = 0, is_player = 0;
  int parse_class_spec(char *arg);
  int parse_race_spec(char *arg);

  struct set_struct {
    char *cmd;
    char level;
    char pcnpc;
    char type;
  } fields[] = {
      {"brief", COM_QUEST, PC, BINARY}, /* 0 */
      {"invstart", COM_QUEST, PC, BINARY}, /* 1 */
      {"title", COM_IMMORT, PC, MISC},
      {"nosummon", COM_BUILDER, PC, BINARY},
      {"maxhit", COM_BUILDER, BOTH, NUMBER},
      {"maxmana", COM_BUILDER, BOTH, NUMBER}, /* 5 */
      {"maxmove", COM_BUILDER, BOTH, NUMBER},
      {"hit", COM_BUILDER, BOTH, NUMBER},
      {"mana", COM_BUILDER, BOTH, NUMBER},
      {"move", COM_BUILDER, BOTH, NUMBER},
      {"align", COM_QUEST, BOTH, NUMBER}, /* 10 */
      {"str", COM_BUILDER, BOTH, NUMBER},
      {"stradd", COM_BUILDER, BOTH, NUMBER},
      {"int", COM_BUILDER, BOTH, NUMBER},
      {"wis", COM_BUILDER, BOTH, NUMBER},
      {"dex", COM_BUILDER, BOTH, NUMBER}, /* 15 */
      {"con", COM_BUILDER, BOTH, NUMBER},
      {"sex", COM_BUILDER, BOTH, MISC},
      {"ac", COM_BUILDER, BOTH, NUMBER},
      {"gold", COM_QUEST, BOTH, NUMBER},
      {"bankg", COM_QUEST, PC, NUMBER}, /* 20 */
      {"exp", COM_BUILDER, BOTH, NUMBER},
      {"hitroll", COM_BUILDER, BOTH, NUMBER},
      {"damroll", COM_BUILDER, BOTH, NUMBER},
      {"invis", COM_ADMIN, PC, NUMBER},
      {"nohassle", COM_BUILDER, PC, BINARY}, /* 25 */
      {"frozen", COM_ADMIN, PC, BINARY},
      {"practices", COM_BUILDER, PC, NUMBER},
      {"lessons", COM_BUILDER, PC, NUMBER},
      {"drunk", COM_BUILDER, BOTH, MISC},
      {"hunger", COM_QUEST, BOTH, MISC}, /* 30 */
      {"thirst", COM_QUEST, BOTH, MISC},
      {"killer", COM_QUEST, PC, BINARY},
      {"thief", COM_QUEST, PC, BINARY},
      {"level", COM_ADMIN, BOTH, NUMBER},
      {"room", COM_ADMIN, BOTH, NUMBER}, /* 35 */
      {"roomflag", COM_BUILDER, PC, BINARY},
      {"siteok", COM_BUILDER, PC, BINARY},
      {"deleted", COM_ADMIN, PC, BINARY},
      {"class", COM_BUILDER, BOTH, MISC},
      {"nowizlist", COM_QUEST, PC, BINARY}, /* 40 */
      {"accepted", COM_IMMORT, PC, BINARY},
      {"loadroom", COM_QUEST, PC, MISC},
      {"color", COM_QUEST, PC, BINARY},
      {"idnum", COM_ADMIN, PC, NUMBER},
      {"passwd", COM_ADMIN, PC, MISC}, /* 45 */
      {"nodelete", COM_QUEST, PC, BINARY},
      {"agi", COM_BUILDER, BOTH, NUMBER},
      {"race", COM_BUILDER, PC, MISC},
      {"casting", COM_BUILDER, PC, BINARY},
      {"weapon", COM_BUILDER, PC, NUMBER}, /* 50 */
      {"married", COM_ADMIN, PC, NUMBER}, /* 51 */
      {"immort", COM_ADMIN, PC, BINARY},
      {"quest", COM_ADMIN, PC, BINARY},
      {"builder", COM_ADMIN, PC, BINARY}, /* 54 */
      {"admin", COM_ADMIN, PC, BINARY}, /* 55 */
      {"mob", COM_ADMIN, PC, BINARY},
      {"kit", COM_BUILDER, PC, BINARY}, /* 57 */
      {"outlaw", COM_BUILDER, PC, BINARY}, /* 58 */
      {"specialize", COM_ADMIN, PC, BINARY}, /* 59 */
      {"zoneok", COM_BUILDER, PC, BINARY}, /* 60 */
      {"staygod", COM_ADMIN, PC, BINARY}, /* 61 */
      {"unrestrict", COM_BUILDER, PC, BINARY}, /* 62 */
      {"noemote", COM_BUILDER, PC, BINARY}, /* 63 */
      {"nosocial", COM_BUILDER, PC, BINARY}, /* 64 */
      {"mptoggle", COM_QUEST, NPC, MISC}, /* 65 */
      {"age", COM_QUEST, PC, NUMBER}, /* 66 */
      {"plat", COM_QUEST, BOTH, NUMBER}, /* 67 */
      {"silver", COM_QUEST, BOTH, NUMBER}, /* 68 */
      {"copper", COM_QUEST, BOTH, NUMBER}, /* 69 */
      {"bankp", COM_QUEST, PC, NUMBER}, /* 70 */
      {"banks", COM_QUEST, PC, NUMBER}, /* 71 */
      {"bankc", COM_QUEST, PC, NUMBER}, /* 72 */
      {"name", COM_ADMIN, PC, MISC}, /* 73 */
      {"knockedout", COM_IMMORT, PC, BINARY}, /* 74 */
      {"\n", 0, BOTH, MISC}
  };

  vict = NULL;
  cbuf = NULL;

  half_chop(argument, name, buf);
  if (!strcmp(name, "file")) {
    is_file = 1;
    half_chop(buf, name, buf);
  } else if (!str_cmp(name, "player")) {
    is_player = 1;
    half_chop(buf, name, buf);
  } else if (!str_cmp(name, "mob")) {
    half_chop(buf, name, buf);
  }
  half_chop(buf, field, buf);
  strcpy(val_arg, buf);

  if (!*name || !*field) {
    send_to_char("Usage: set <victim> <field> <value>\r\n", ch);
    sprintf(buf, "Possible fields for you are:\r\n");
    l = 0;
    for (j = 0; *(fields[j].cmd) != '\n'; j++) {
      if (COM_FLAGGED(ch, fields[j].level)) {
        memset(buf2, 0, MAX_STRING_LENGTH);
        sprintf(buf2, "%-13s", fields[j].cmd);
        if (++l == 5) {
          l = 0;
          strcat(buf2, "\r\n");
        }
        strcat(buf, buf2);
      }
    }
    if (l) {
      strcat(buf, "\r\n");
    }
    send_to_char(buf, ch);
    return;
  }

  if (!is_file) {
    if (is_player) {
      if (!(vict = get_player_vis(ch, name, 0))) {
        CREATE(cbuf, struct char_data, 1);
        clear_char(cbuf);
        if (load_char_text(name, cbuf) > 0) {
          if ((GET_LEVEL(cbuf) >= GET_LEVEL(ch)) && (!COM_FLAGGED(ch, COM_ADMIN))) {
            free_char(cbuf);
            send_to_char("Sorry, you can't do that.\r\n", ch);
            return;
          }
          vict = cbuf;
          is_file = 1;
        } else {
          FREE(cbuf);
          send_to_char("There is no such player.\r\n", ch);
          return;
        }
      }
    } else {
      if (!(vict = get_char_vis(ch, name))) {
        CREATE(cbuf, struct char_data, 1);
        clear_char(cbuf);
        if (load_char_text(name, cbuf) > 0) {
          if ((GET_LEVEL(cbuf) >= GET_LEVEL(ch)) && (!COM_FLAGGED(ch, COM_ADMIN))) {
            free_char(cbuf);
            send_to_char("Sorry, you can't do that.\r\n", ch);
            return;
          }
          vict = cbuf;
          is_file = 1;
        } else {
          FREE(cbuf);
          send_to_char("There is no such creature.\r\n", ch);
          return;
        }
      }
    }
  } else {
    CREATE(cbuf, struct char_data, 1);
    clear_char(cbuf);
    if (load_char_text(name, cbuf) > 0) {
      if ((GET_LEVEL(cbuf) >= GET_LEVEL(ch)) && (!COM_FLAGGED(ch, COM_ADMIN))) {
        free_char(cbuf);
        send_to_char("Sorry, you can't do that.\r\n", ch);
        return;
      }
      vict = cbuf;
    } else {
      FREE(cbuf);
      send_to_char("There is no such player.\r\n", ch);
      return;
    }
  }
  if (!COM_FLAGGED(ch, COM_ADMIN)) {
    if (!IS_NPC(vict) && GET_LEVEL(ch) <= GET_LEVEL(vict) && vict != ch) {
      send_to_char("Maybe that's not such a great idea...\r\n", ch);
      return;
    }
  }
  for (l = 0; *(fields[l].cmd) != '\n'; l++) {
    if (!strncmp(field, fields[l].cmd, strlen(field))) {
      break;
    }
  }

  if (!COM_FLAGGED(ch, fields[l].level)) {
    send_to_char("You are not godly enough for that!\r\n", ch);
    return;
  }
  if (IS_NPC(vict) && !(fields[l].pcnpc && NPC)) {
    send_to_char("You can't do that to a beast!\r\n", ch);
    return;
  } else if (!IS_NPC(vict) && !(fields[l].pcnpc && PC)) {
    send_to_char("That can only be done to a beast!\r\n", ch);
    return;
  }
  if (fields[l].type == BINARY) {
    if (!strcmp(val_arg, "on") || !strcmp(val_arg, "yes")) {
      on = 1;
    } else if (!strcmp(val_arg, "off") || !strcmp(val_arg, "no")) {
      off = 1;
    } if (!(on || off)) {
      send_to_char("Value must be on or off.\r\n", ch);
      return;
    }
  } else if (fields[l].type == NUMBER) {
    value = atoi(val_arg);
  }
  strcpy(buf, "Okay.");
  switch (l) {
    case 0:
      SET_OR_REMOVE(PRF_FLAGS(vict), PRF_BRIEF);
      FLAGLOG("PRF_BRIEF");
      break;
    case 1:
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_INVSTART);
      FLAGLOG("PLR_INVSTART");
      break;
    case 2:
      set_title(vict, val_arg);
      sprintf(buf, "%s's title is now: %s", GET_NAME(vict), GET_TITLE(vict));
      break;
    case 3:
      break;
    case 4:
      vict->points.max_hit = RANGE(1, 30000);
      affect_total(vict);
      NUMLOG("%s changed MAXHIT to %d on %s", value);
      break;
    case 5:
      vict->points.max_mana = RANGE(1, 30000);
      affect_total(vict);
      NUMLOG("%s changed MAXMANA to %d on %s", value);
      break;
    case 6:
      vict->points.max_move = RANGE(1, 30000);
      affect_total(vict);
      NUMLOG("%s changed MAXMOVE to %d on %s", value);
      break;
    case 7:
      vict->points.hit = RANGE(-9, vict->points.max_hit);
      affect_total(vict);
      NUMLOG("%s changed HIT to %d on %s", value);
      break;
    case 8:
      vict->points.mana = RANGE(0, vict->points.max_mana);
      affect_total(vict);
      NUMLOG("%s changed MANA to %d on %s", value);
      break;
    case 9:
      vict->points.move = RANGE(0, vict->points.max_move);
      affect_total(vict);
      NUMLOG("%s changed MOVE to %d on %s", value);
      break;
    case 10:
      GET_ALIGNMENT(vict) = RANGE(-1000, 1000);
      affect_total(vict);
      NUMLOG("%s changed ALIGN to %d on %s", value);
      break;
    case 11:
      RANGE(1, 100);
      vict->real_abils.str = value;
      affect_total(vict);
      NUMLOG("%s changed STR to %d on %s", value);
      break;
    case 12:
      break;
    case 13:
      RANGE(1, 100);
      vict->real_abils.intel = value;
      affect_total(vict);
      NUMLOG("%s changed INT to %d on %s", value);
      break;
    case 14:
      RANGE(1, 100);
      vict->real_abils.wis = value;
      affect_total(vict);
      NUMLOG("%s changed WIS to %d on %s", value);
      break;
    case 15:
      RANGE(1, 100);
      vict->real_abils.dex = value;
      affect_total(vict);
      NUMLOG("%s changed DEX to %d on %s", value);
      break;
    case 16:
      RANGE(1, 100);
      vict->real_abils.con = value;
      affect_total(vict);
      NUMLOG("%s changed CON to %d on %s", value);
      break;
    case 17:
      if (!str_cmp(val_arg, "male")) {
        vict->player.sex = SEX_MALE;
      } else if (!str_cmp(val_arg, "female")) {
        vict->player.sex = SEX_FEMALE;
      } else if (!str_cmp(val_arg, "neutral")) {
        vict->player.sex = SEX_NEUTRAL;
      } else {
        send_to_char("Must be 'male', 'female', or 'neutral'.\r\n", ch);
        return;
      }
      break;
    case 18:
      vict->points.armor = RANGE(-100, 100);
      affect_total(vict);
      NUMLOG("%s changed ARMOR to %d on %s", value);
      break;
    case 19:
      GET_GOLD(vict) = RANGE(0, 400000);
      GET_TEMP_GOLD(vict) = (1000 * GET_PLAT(vict)) + (100 * GET_GOLD(vict)) + (10 * GET_SILVER(vict)) + GET_COPPER(vict);
      NUMLOG("%s changed GOLD to %d on %s", value);
      break;
    case 20:
      GET_BANK_GOLD(vict) = RANGE(0, 400000);
      NUMLOG("%s changed BANK_GOLD to %d on %s", value);
      break;
    case 21:
      vict->points.exp = RANGE(0, 400000000);
      NUMLOG("%s changed EXP to %d on %s", value);
      break;
    case 22:
      vict->points.hitroll = RANGE(-120, 120);
      affect_total(vict);
      NUMLOG("%s changed HITROLL to %d on %s", value);
      break;
    case 23:
      vict->points.damroll = RANGE(-120, 120);
      affect_total(vict);
      NUMLOG("%s changed DAMROLL to %d on %s", value);
      break;
    case 24:
      if (!COM_FLAGGED(ch, COM_ADMIN) && ch != vict) {
        send_to_char("You aren't godly enough for that!\r\n", ch);
        return;
      }
      if (IS_NPC(vict)) {
        GET_MOB_INVIS_LEV(vict) = RANGE(0, GET_LEVEL(vict));
      } else {
        GET_PLR_INVIS_LEV(vict) = RANGE(0, GET_LEVEL(vict));
      }
      NUMLOG("%s changed INVIS to %d on %s", value);
      break;
    case 25:
      if (!COM_FLAGGED(ch, COM_ADMIN) && ch != vict) {
        send_to_char("You aren't godly enough for that!\r\n", ch);
        return;
      }
      SET_OR_REMOVE(PRF_FLAGS(vict), PRF_NOHASSLE);
      FLAGLOG("PRF_NOHASSLE");
      break;
    case 26:
      if (ch == vict) {
        send_to_char("Better not -- could be a long winter!\r\n", ch);
        return;
      }
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_FROZEN);
      FLAGLOG("PLR_FROZEN");
      break;
    case 27:
    case 28:
      GET_PRACTICES(vict) = RANGE(0, 100);
      NUMLOG("%s changed PRACS to %d on %s", value);
      break;
    case 29:
    case 30:
    case 31:
      if (!str_cmp(val_arg, "off")) {
        GET_COND(vict, (l - 29)) = (char) -1;
        sprintf(buf, "%s's %s now off.", GET_NAME(vict), fields[l].cmd);
      } else if (is_number(val_arg)) {
        value = atoi(val_arg);
        RANGE(0, 24);
        GET_COND(vict, (l - 29)) = (char) value;
        sprintf(buf, "%s's %s set to %d.", GET_NAME(vict), fields[l].cmd, value);
      } else {
        send_to_char("Must be 'off' or a value from 0 to 24.\r\n", ch);
        return;
      }
      break;
    case 32:
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_KILLER);
      FLAGLOG("PLR_KILLER");
      break;
    case 33:
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_THIEF);
      FLAGLOG("PLR_THIEF");
      break;
    case 34:
      if (value > GET_LEVEL(ch)) {
        send_to_char("You can't do that.\r\n", ch);
        return;
      }
      RANGE(0, LVL_IMMORT);
      vict->player.level = (byte) value;
      NUMLOG("%s changed LEVEL to %d on %s", value);
      break;
    case 35:
      if ((i = real_room(value)) < 0) {
        send_to_char("No room exists with that number.\r\n", ch);
        return;
      }
      char_from_room(vict);
      char_to_room(vict, i);
      break;
    case 36:
      SET_OR_REMOVE(PRF_FLAGS(vict), PRF_ROOMFLAGS);
      FLAGLOG("PRF_ROOMFLAGS");
      break;
    case 37:
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_SITEOK);
      FLAGLOG("PLR_SITEOK");
      break;
    case 38:
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_DELETED);
      REMOVE_BIT(PRF_FLAGS(vict), PRF_DELETED);
      FLAGLOG("PLR_DELETED");
      break;
    case 39:
      if ((i = parse_class_spec(val_arg)) == CLASS_UNDEFINED) {
        send_to_char("That is not a class.\r\n", ch);
      } else {
        GET_CLASS(vict) = i;
        send_to_char(OK, ch);
        NUMLOG("%s changed CLASS to %d on %s", i);
      }
      break;
    case 40:
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOWIZLIST);
      FLAGLOG("PLR_NOWIZLIST");
      break;
    case 41:
      SET_OR_REMOVE(PRF_FLAGS(vict), PRF_ACCEPTED);
      FLAGLOG("PRF_ACCEPTED");
      break;
    case 42:
      if (!str_cmp(val_arg, "on")) {
        SET_BIT(PLR_FLAGS(vict), PLR_LOADROOM);
      } else if (!str_cmp(val_arg, "off")) {
        REMOVE_BIT(PLR_FLAGS(vict), PLR_LOADROOM);
      } else {
        if (real_room(i = atoi(val_arg)) > -1) {
          GET_LOADROOM(vict) = i;
          sprintf(buf, "%s will enter at %d.", GET_NAME(vict), GET_LOADROOM(vict));
        } else {
          sprintf(buf, "That room does not exist!");
        }
      }
      break;
    case 43:
      SET_OR_REMOVE(PRF_FLAGS(vict), (PRF_COLOR_1 | PRF_COLOR_2));
      FLAGLOG("PRF_COLOR");
      break;
    case 44:
      /*  if (!IS_NPC(vict))
       return; */
      GET_IDNUM(vict) = value;
      NUMLOG("%s changed IDNUM to %d on %s", value);
      break;
    case 45:
      if (!is_file) {
        return;
      }
      /*  if (GET_LEVEL(vict) >= LVL_GRGOD) {
       send_to_char("You cannot change that.\r\n", ch);
       return;
       } */
      if (crypto_pwhash_str(GET_ENCPASSWD(cbuf), val_arg, strlen(val_arg), crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE) == 0) {
        sprintf(buf, "Password changed to '%s'.", val_arg);
      }
      break;
    case 46:
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NODELETE);
      FLAGLOG("PLR_NODELETE");
      break;
    case 47:
      RANGE(1, 100);
      vict->real_abils.agi = value;
      NUMLOG("%s changed AGI to %d on %s", value);
      affect_total(vict);
      break;
    case 48:
      if ((i = parse_race_spec(val_arg)) == RACE_UNDEFINED) {
        send_to_char("That is not a race.\r\n", ch);
      } else {
        vict->player_specials->saved.race = i;
        send_to_char(OK, ch);
        NUMLOG("%s changed RACE to %d on %s", i);
      }
      break;
    case 49:
      SET_OR_REMOVE(AFF2_FLAGS(vict), AFF2_CASTING);
      FLAGLOG("AFF2_CASTING");
      break;
    case 50:
      GET_TRAINING(vict) = value;
      NUMLOG("%s changed TRAIN to %d on %s", value);
      break;
    case 51:
      break;
    case 52:
      SET_OR_REMOVE(COM_FLAGS(vict), COM_IMMORT);
      FLAGLOG("COM_IMMORT");
      break;
    case 53:
      SET_OR_REMOVE(COM_FLAGS(vict), COM_QUEST);
      FLAGLOG("COM_QUEST");
      break;
    case 54:
      SET_OR_REMOVE(COM_FLAGS(vict), COM_BUILDER);
      FLAGLOG("COM_BUILDER");
      break;
    case 55:
      SET_OR_REMOVE(COM_FLAGS(vict), COM_ADMIN);
      FLAGLOG("COM_ADMIN");
      break;
    case 56:
      SET_OR_REMOVE(COM_FLAGS(vict), COM_MOB);
      FLAGLOG("COM_MOB");
      break;
    case 57:
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_KIT);
      FLAGLOG("PLR_KIT");
      break;
    case 58:
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_OUTLAW);
      FLAGLOG("PLR_OUTLAW");
      break;
    case 59:
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_SPECIALIZED);
      FLAGLOG("PLR_SPECIALIZED");
      break;
    case 60:
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_ZONEOK);
      FLAGLOG("PLR_ZONEOK");
      break;
    case 61:
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_STAYGOD);
      FLAGLOG("PLR_STAYGOD");
      break;
    case 62:
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_UNRESTRICT);
      FLAGLOG("PLR_UNRESTRICT");
      break;
    case 63:
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOEMOTE);
      FLAGLOG("PLR_NOEMOTE");
      break;
    case 64:
      SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOSOCIAL);
      FLAGLOG("PLR_NOSOCIAL");
      break;
    case 65:
      if (ch->mob_specials.mp_toggle) {
        ch->mob_specials.mp_toggle = FALSE;
        send_to_char("Mob Prog toggled on.\r\n", ch);
      } else {
        ch->mob_specials.mp_toggle = TRUE;
        send_to_char("Mob Prog toggled off.\r\n", ch);
      }
      break;
    case 66:
      vict->player.time.birth = (time(0) - ((value - 17) * SECS_PER_MUD_YEAR));
      NUMLOG("%s changed AGE to %d on %s", value);
      break;
    case 67:
      GET_PLAT(vict) = RANGE(0, 400000);
      GET_TEMP_GOLD(vict) = (1000 * GET_PLAT(vict)) + (100 * GET_GOLD(vict)) + (10 * GET_SILVER(vict)) + GET_COPPER(vict);
      NUMLOG("%s changed PLAT to %d on %s", value);
      break;
    case 68:
      GET_SILVER(vict) = RANGE(0, 400000);
      GET_TEMP_GOLD(vict) = (1000 * GET_PLAT(vict)) + (100 * GET_GOLD(vict)) + (10 * GET_SILVER(vict)) + GET_COPPER(vict);
      NUMLOG("%s changed SILVER to %d on %s", value);
      break;
    case 69:
      GET_COPPER(vict) = RANGE(0, 400000);
      GET_TEMP_GOLD(vict) = (1000 * GET_PLAT(vict)) + (100 * GET_GOLD(vict)) + (10 * GET_SILVER(vict)) + GET_COPPER(vict);
      NUMLOG("%s changed COPPER to %d on %s", value);
      break;
    case 70:
      GET_BANK_PLAT(vict) = RANGE(0, 400000000);
      NUMLOG("%s changed BANK PLAT to %d on %s", value);
      break;
    case 71:
      GET_BANK_SILVER(vict) = RANGE(0, 400000000);
      NUMLOG("%s changed BANK SILVER to %d on %s", value);
      break;
    case 72:
      GET_BANK_COPPER(vict) = RANGE(0, 400000000);
      NUMLOG("%s changed BANK COPPER to %d on %s", value);
      break;
    case 73:
      FREE(GET_NAME(cbuf));
      strncpy(GET_NAME(cbuf), val_arg, MAX_NAME_LENGTH);
      GET_NAME(cbuf)[MAX_NAME_LENGTH] = '\0';
      NUMLOG("%s changed NAME to %s on %s", GET_NAME(cbuf));
      sprintf(buf, "Name changed to '%s'.", GET_NAME(cbuf));
      break;
    case 74:
      SET_OR_REMOVE(AFF2_FLAGS(vict), AFF2_KNOCKEDOUT);
      FLAGLOG("AFF2_KNOCKEDOUT");
      break;
    default:
      sprintf(buf, "Can't set that!");
      break;
  }

  if (fields[l].type == BINARY) {
    sprintf(buf, "%s %s for %s.\r\n", fields[l].cmd, ONOFF(on), GET_NAME(vict));
    CAP(buf);
  } else if (fields[l].type == NUMBER) {
    sprintf(buf, "%s's %s set to %d.\r\n", GET_NAME(vict), fields[l].cmd, value);
  } else {
    strcat(buf, "\r\n");
  }
  send_to_char(CAP(buf), ch);

  if (!is_file && !IS_NPC(vict)) {
    save_char_text(vict, NOWHERE);
  }

  if (is_file) {
    save_char_text(vict, NOWHERE);
    free_char(cbuf);
    send_to_char("Saved in file.\r\n", ch);
  }
}

ACMD(do_syslog)
{
  extern long asciiflag_conv(char *flag);
  int tp, nr, vektor;
  const char *log_text[] = {"A Advancement", "B Boards", "C Connections", "D Code Debug Messages", "E Errors", "F Flags", "G God commands", "\0", "I QIC transactions", "J Extensive QIC", "K Kills", "L Load", "M Messages", "N Skills", "O Other kills", "P Password", "Q Quest", "R Rent", "S Server", "T Ticks", "U Auction", "\0", "W Warnings", "X Extensive GC", "Y Player steal/kill", "Z Zone resets", "\n", };

  one_argument(argument, arg);

  if (!*arg) {
    TOGGLE_BIT(PRF_FLAGS(ch), PRF_LOG);
    sprintf(buf, "Your syslog is now %s.\r\n", PRF_FLAGGED(ch, PRF_LOG) ? "ON" : "OFF");
    send_to_char(buf, ch);
    return;
  }

  if (*arg == '+') {
    tp = TRUE;
  } else if (*arg == '-') {
    tp = FALSE;
  } else if (*arg == 'l') { /* list */
    sprintf(buf, "Current syslog(%s) flag settings:\r\n", PRF_FLAGGED(ch, PRF_LOG) ? "ON" : "OFF");
    send_to_char(buf, ch);
    *buf = '\0';
    vektor = LOG_FLAGS(ch);
    for (nr = 0; vektor; vektor >>= 1) {
      if (IS_SET(1, vektor)) {
        if (*log_text[nr] != '\n' && *log_text[nr] != '\0') {
          strcat(buf, log_text[nr]);
          strcat(buf, "\r\n");
        }
      }
      if (*log_text[nr] != '\n') {
        nr++;
      }
      if (nr > 30) {
        break;
      }
    }
    send_to_char(buf, ch);
    return;
  } else {
    send_to_char("Usage: syslog [+/-][flags] (example +acz)\r\n"
        "       syslog list  (list current status)\r\n"
        "       syslog       (toggles syslog on/off)\r\n", ch);
    return;
  }
  *arg = ' ';

  if (arg[1] == '*') {
    if (tp) {
      LOG_FLAGS(ch) = 268435455;
    } else {
      LOG_FLAGS(ch) = 0;
    }
  } else {
    if (tp) {
      SET_BIT(LOG_FLAGS(ch), asciiflag_conv(arg));
    } else {
      REMOVE_BIT(LOG_FLAGS(ch), asciiflag_conv(arg));
    }
  }

  send_to_char("SYSLOG operation performed.\r\n", ch);
}

ACMD(do_copyto)
{
  /* Only works if you have Oasis OLC */
  void redit_save_to_disk(int real_zone);

  char buf2[10];
  char buf[80];
  int iroom = 0, rroom = 0;

  one_argument(argument, buf2);
  /* buf2 is room to copy to */

  iroom = atoi(buf2);
  rroom = real_room(atoi(buf2));

  if (!*buf2) {
    send_to_char("Format: copyto <room number>\r\n", ch);
    return;
  }
  if (rroom <= 0) {
    sprintf(buf, "There is no room with the number %d.\r\n", iroom);
    send_to_char(buf, ch);
    return;
  }

  /* Main stuff */

  if (world[ch->in_room].description) {
    if (world[rroom].description) {
      FREE(world[rroom].description);
    }
    world[rroom].description = strdup(world[ch->in_room].description);

    /* Only works if you have Oasis OLC */
    CREATE(ch->desc->olc, struct olc_data, 1);
    OLC_ZNUM(ch->desc) = world[rroom].zone;
    redit_save_to_disk(OLC_ZNUM(ch->desc));
    FREE(ch->desc->olc);

    sprintf(buf, "You copy the description to room %d.\r\n", iroom);
    send_to_char(buf, ch);
  } else {
    send_to_char("This room has no description!\r\n", ch);
  }
}

ACMD(do_dig)
{
  /* Only works if you have Oasis OLC */
  void redit_save_to_disk(int real_zone);
  extern void redit_save_internally(struct descriptor_data *d);
  extern const int rev_dir[];
  int real_zone(int number);

  char buf2[10];
  char buf3[10];
  char buf[80];
  int iroom = 0, rroom = 0;
  int dir = 0;
  int new = 0;
  /*  struct room_data *room; */

  two_arguments(argument, buf2, buf3);
  /* buf2 is the direction, buf3 is the room */

  iroom = atoi(buf3);
  rroom = real_room(iroom);

  if (!*buf2) {
    send_to_char("Format: dig <dir> <room number>\r\n", ch);
    return;
  } else if (!*buf3) {
    send_to_char("Format: dig <dir> <room number>\r\n", ch);
    return;
  } else if (!isdigit(*buf3)) {
    send_to_char("Format: dig <dir> <room number>\r\n", ch);
    return;
  }

  if (real_zone(iroom) == -1) {
    send_to_char("There is no zone for that number!\r\n", ch);
    return;
  }

  CREATE(ch->desc->olc, struct olc_data, 1);

  if (rroom <= 0) {
    CREATE(OLC_ROOM(ch->desc), struct room_data, 1);
    OLC_ROOM(ch->desc)->name = strdup("An unfinished room");
    OLC_ROOM(ch->desc)->description = strdup("An unfinished room\n");
    OLC_NUM(ch->desc) = iroom;
    new = 1;
  }

  /* Main stuff */
  switch (*buf2) {
    case 'n':
    case 'N':
      dir = NORTH;
      break;
    case 'e':
    case 'E':
      dir = EAST;
      break;
    case 's':
    case 'S':
      dir = SOUTH;
      break;
    case 'w':
    case 'W':
      dir = WEST;
      break;
    case 'u':
    case 'U':
      dir = UP;
      break;
    case 'd':
    case 'D':
      dir = DOWN;
      break;
  }

  if (world[ch->in_room].dir_option[dir]) {
    FREE(world[ch->in_room].dir_option[dir]);
  }

  CREATE(world[ch->in_room].dir_option[dir], struct room_direction_data, 1);

  world[ch->in_room].dir_option[dir]->general_description = NULL;
  world[ch->in_room].dir_option[dir]->keyword = NULL;
  world[ch->in_room].dir_option[dir]->to_room_vnum = iroom;

  if (!new) {
    OLC_ZNUM(ch->desc) = world[rroom].zone;
    if (world[rroom].dir_option[rev_dir[dir]]) {
      FREE(world[rroom].dir_option[rev_dir[dir]]);
    }

    CREATE(world[rroom].dir_option[rev_dir[dir]], struct room_direction_data, 1);

    world[rroom].dir_option[rev_dir[dir]]->general_description = NULL;
    world[rroom].dir_option[rev_dir[dir]]->keyword = NULL;
    world[rroom].dir_option[rev_dir[dir]]->to_room = ch->in_room;
    world[rroom].dir_option[rev_dir[dir]]->to_room_vnum = world[ch->in_room].number;

  } else {
    OLC_ZNUM(ch->desc) = real_zone(iroom);
    CREATE(OLC_ROOM(ch->desc)->dir_option[rev_dir[dir]], struct room_direction_data, 1);
    OLC_ROOM(ch->desc)->dir_option[rev_dir[dir]]->general_description = NULL;
    OLC_ROOM(ch->desc)->dir_option[rev_dir[dir]]->keyword = NULL;
    OLC_ROOM(ch->desc)->dir_option[rev_dir[dir]]->to_room = ch->in_room;
    OLC_ROOM(ch->desc)->dir_option[rev_dir[dir]]->to_room_vnum = world[ch->in_room].number;
  }

  /* Only works if you have Oasis OLC */
  if (new) {
    redit_save_internally(ch->desc);
  }

  world[ch->in_room].dir_option[dir]->to_room = real_room(iroom);
  /* save world data for destination room */
  redit_save_to_disk(OLC_ZNUM(ch->desc));

  /* save world data for starting room */OLC_ZNUM(ch->desc) = world[ch->in_room].zone;
  redit_save_to_disk(OLC_ZNUM(ch->desc));

  FREE(OLC_ROOM(ch->desc));
  FREE(ch->desc->olc);
  sprintf(buf, "You make an exit %s to room %d.\r\n", buf2, iroom);
  send_to_char(buf, ch);
}

ACMD(do_nuke)
{
  struct descriptor_data *d;
  struct descriptor_data *next_d;
  one_argument(argument, arg);
  if (!strcasecmp(arg, GET_NAME(ch))) {
    send_to_char("Just do that from the menu stupid.\r\n", ch);
    return;
  }
  for (d = descriptor_list; d; d = next_d) {
    next_d = d->next;
    if (d->character && !strcasecmp(GET_NAME(d->character), arg)) {
      SET_BIT(PRF_FLAGS(d->character), PRF_DELETED);
      extract_char(d->character, 0);
      STATE(d) = CON_CLOSE;
    }
  }
  Delete_file(arg, PTDAT_FILE);
  Delete_file(arg, CRASH_FILE);
  Delete_file(arg, ETEXT_FILE);
  Delete_file(arg, PLOG_FILE);
  sprintf(buf, "Character '%s' deleted!\r\n", arg);
  send_to_char(buf, ch);
  sprintf(buf, "Character '%s' deleted by %s", arg, GET_NAME(ch));
  mudlog(buf, 'X', COM_ADMIN, TRUE);
  plog(buf, ch, LVL_IMMORT);
  stderr_log(buf);
}

ACMD(do_approve)
{
  struct descriptor_data *d = NULL;
  struct descriptor_data *next_d = NULL;
  extern char* nmotd;
  int found = 0;
  one_argument(argument, arg);
  for (d = descriptor_list; d; d = next_d) {
    next_d = d->next;
    if (d->character && GET_NAME(d->character) && !strcasecmp(GET_NAME(d->character), arg) && STATE(d) == CON_ACCEPT) {
      found = 1;
      REMOVE_BIT(PRF_FLAGS(d->character), PRF_DELETED);
      SET_BIT(PRF_FLAGS(d->character), PRF_ACCEPTED);
      SEND_TO_Q("\r\nYour character has been approved.\r\n", d);
      SEND_TO_Q_COLOR(nmotd, d);
      SEND_TO_Q("\r\n\n*** PRESS RETURN: ", d);
      STATE(d) = CON_RMOTD;
      d->idle_cnt = 0;
      sprintf(buf, "%s [%s] new player. (approved by %s)", GET_NAME(d->character), GET_HOST(d->character), GET_NAME(ch));
      mudlog(buf, 'C', COM_IMMORT, TRUE);
    }
  }
  if (!found) {
    send_to_char("Could not find that person.\r\n", ch);
  }
}

ACMD(do_decline)
{
  struct descriptor_data *d = NULL;
  struct descriptor_data *next_d = NULL;
  int found = 0;
  char name[128];
  char reason[256];
  char *denyreason[] = {"\r\nNo obscene, insulting, threatening or degrading names will be tolerated.\r\n\r\n", "\r\nNo modern trademarks, cartoon characters, product names, etc.\r\n"
      "Examples: Bevis, Bacardi, Seinfeld\r\n\r\n", "\r\nNo incoherent collection of random letters is allowed.\r\n"
      "Examples: Wqzlks, Xeyplw\r\n\r\n", "\r\nNo names that imply rank or privilage.\r\n"
      "Examples: King, Ladyhawk\r\n\r\n", "\r\nNo names implying any religious reference or preference.\r\n"
      "Examples: God, Buddha\r\n\r\n", "\r\nNo names refering to historical figures, living or dead.\r\n"
      "Examples: Stalin, Churchill\r\n\r\n", "\r\nNo characters out of well known works of fiction, myth or legend.\r\n"
      "Examples: Merlin, Aragorn, Zeus\r\n\r\n", "\r\nNo everyday names, or everyday words, including cities, universities,\r\n"
      "etc. (nor misspellings of).\r\n"
      "Examples: Mike, Angela, Slammer, Psycho\r\n\r\n", "\r\nNo stupid names.\r\n"
      "Examples:  Dorkyboy, Darkangle, etc.\r\n\r\n", "\r\nThat name doesn't fit with Shadowwind's medieval theme.\r\n\r\n", "\r\n- HUMANS\r\n"
      "\r\nHumans have the widest variety of names, however they very rarely possess\r\n"
      "the sharpness or tone of, for example, a troll or ogre.\r\n"
      "Examples: Valsar, Morvul, Taloran, Telseen, Luthar\r\n\r\n", "\r\n- TROLLS\r\n"
      "\r\nTrolls are horrible creatures, of particularly low intelligence. Their\r\n"
      "names are short, badly spelt and often use odd combinations of many of\r\n"
      "the oft unused letters of the alphabet, eg. z,k,g.\r\n"
      "Examples: Voraak, Blzrog, Gorakk, Arkkz\r\n\r\n", "\r\n- OGRES\r\n"
      "\r\nLarge, hulking beasts, Ogres are far dumber than trolls and have similar\r\n"
      "names. Harsh, but not sharp gutteral sounds, with an emphasis on phlegm.\r\n"
      "Examples: Bolag, Falrag, Moggo, Gluog\r\n\r\n", "\r\n- DWARVES\r\n"
      "\r\nDwarves believe they are fine, strong warriors, unmatched in combat. Their\r\n"
      "names will represent this by having a strong, sharp sound.\r\n"
      "Examples: Tumal, Naruk, Gandark\r\n\r\n", "\r\n- ELVES\r\n"
      "\r\nAs elves believe strongly in the powers of music, their names tend to be\r\n"
      "soft, and almost song-like.\r\n"
      "Examples: Velnarin, Myrethan, Lanthalin\r\n\r\n", "\r\n- HALF-ELVES\r\n"
      "\r\nBeing the offspring of Humans and Elves, Half-Elves can have either a\r\n"
      "human or an elven name. Human names contain lots of variety, but not too\r\n"
      "harsh, while elven names tend to be almost musical.\r\n"
      "Examples: Valryn, Kealaria, Soavedna, Qupal\r\n\r\n", "\r\n- GNOMES\r\n"
      "\r\nGnomes are odd creatures, and therefore have odd names. They are vibrant\r\n"
      "and bouncy, and their names reflect this.\r\n"
      "Examples: Lippble, Nufbby, Twypplb, Mulmble\r\n\r\n", "\r\n- HALFLINGS\r\n"
      "\r\nHalflings are almost as odd as Gnomes, however their names can be even\r\n"
      "sillier, Almost anything that is light and stupid will be acceptable\r\n"
      "for a halfling.\r\n"
      "Examples: Biffle, Moopal, Wolpah, Rathop\r\n\r\n", "\n"};

  char *reasonlist[] = {
      "obscene", /* 0 */
      "trademark", /* 1 */
      "random", /* 2 */
      "rank", /* 3 */
      "religious", /* 4 */
      "historical", /* 5 */
      "myth", /* 6 */
      "everyday", /* 7 */
      "stupid", /* 8 */
      "theme", /* 9 */
      "race",
      "\n"
  };

  ACMD(do_xname);

  reason[0] = '\0';
  name[0] = '\0';
  two_arguments(argument, name, reason);

  if (!reason[0]) {
    send_to_char("You must specify a reason, wizhelp deny for options.\r\n", ch);
    return;
  }
  if (search_block(reason, reasonlist, 1) == -1) {
    send_to_char("You must specify a valid reason, wizhelp deny for options.\r\n", ch);
    return;
  }
  for (d = descriptor_list; d; d = next_d) {
    next_d = d->next;
    if (d->character && GET_NAME(d->character) && !strcasecmp(GET_NAME(d->character), name) && !PLR_FLAGGED(d->character, PLR_DENIED) && STATE(d) == CON_ACCEPT) {
      found = 1;
      SET_BIT(PLR_FLAGS(d->character), PLR_DENIED);
      SEND_TO_Q("\r\nYour character has been declined.\r\n", d);
      SEND_TO_Q("Reason:\r\n", d);
      switch (search_block(reason, reasonlist, 1)) {
        case 0:
          SEND_TO_Q(denyreason[0], d);
          break;
        case 1:
          SEND_TO_Q(denyreason[1], d);
          break;
        case 2:
          SEND_TO_Q(denyreason[2], d);
          break;
        case 3:
          SEND_TO_Q(denyreason[3], d);
          break;
        case 4:
          SEND_TO_Q(denyreason[4], d);
          break;
        case 5:
          SEND_TO_Q(denyreason[5], d);
          break;
        case 6:
          SEND_TO_Q(denyreason[6], d);
          break;
        case 7:
          SEND_TO_Q(denyreason[7], d);
          break;
        case 8:
          SEND_TO_Q(denyreason[8], d);
          break;
        case 9:
          SEND_TO_Q(denyreason[9], d);
          break;
        default:
          SEND_TO_Q(denyreason[GET_RACE(d->character) + 9], d);
          break;
      }

      SEND_TO_Q("If you wish to start over, simply disconnect and reconnect.\r\n", d);
      SEND_TO_Q("Please choose a more acceptable name.\r\n", d);
      SEND_TO_Q("Name: ", d);
      d->idle_cnt = 0;
      STATE(d) = CON_NEWNAME;
      sprintf(buf, "%s [%s] new player. (declined by %s reason %s)", GET_NAME(d->character), GET_HOST(d->character), GET_NAME(ch), reason);
      mudlog(buf, 'C', COM_IMMORT, TRUE);
      do_xname(ch, argument, 0, 0);
    }
  }
  if (!found) {
    send_to_char("Could not find that person.\r\n", ch);
  }
}

ACMD(do_rename)
{
  struct descriptor_data *d;
  struct descriptor_data *next_d;
  char to_force[5];
  char orig_name[64];
  char new_name[64];

  two_arguments(argument, orig_name, new_name);
  strcpy(to_force, "save");

  for (d = descriptor_list; d; d = next_d) {
    next_d = d->next;
    if (d->character && !strcasecmp(GET_NAME(d->character), orig_name)) {
      FREE(GET_NAME(d->character));
      GET_PLR_NAME(d->character) = strdup(CAP(new_name));
      command_interpreter(d->character, to_force);
      Delete_file(orig_name, PTDAT_FILE);
      Delete_file(orig_name, CRASH_FILE);
      Delete_file(orig_name, ETEXT_FILE);
      Delete_file(orig_name, PLOG_FILE);
      sprintf(buf, "Character '%s' renamed to %s!\r\n", orig_name, new_name);
      send_to_char(buf, ch);
      sprintf(buf, "Character '%s' renamed to %s by %s", orig_name, new_name, GET_NAME(ch));
      mudlog(buf, 'X', COM_ADMIN, TRUE);
      plog(buf, ch, LVL_IMMORT);
      stderr_log(buf);
    }
  }
}

ACMD(do_copyover)
{
  extern int circle_shutdown, circle_reboot;
  struct descriptor_data *d;
  struct descriptor_data *dnext;

  for (d = descriptor_list; d; d = dnext) {
    dnext = d->next;
    if (d->character && GET_LEVEL(d->character) < LVL_IMMORT) {
      SET_BIT(PLR_FLAGS(d->character), PLR_CAMP);
      SET_BIT(PLR_FLAGS(d->character), PLR_LOADROOM);
      GET_LOADROOM(d->character) = world[d->character->in_room].number;
      Crash_save(d->character, RENT_CAMPED);
    }
  }
  sprintf(buf, "(GC) Copyover by %s.", GET_NAME(ch));
  stderr_log(buf);
  sprintf(buf, "Copyover by %s.. come back in 10-20 seconds...\r\n", GET_NAME(ch));
  send_to_all(buf);
  touch("../.fastboot");
  circle_shutdown = circle_reboot = 2;
}

