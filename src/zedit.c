/************************************************************************
 *  OasisOLC - zedit.c						v1.5	*
 *									*
 *  Copyright 1996 Harvey Gilpin.					*
 ************************************************************************/

#include "comm.h"
#include "db.h"
#include "olc.h"
#include "structs.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * Turn on zedit debugging.  Please mail log results to greerga@van.ml.org
 * This will attempt to find the problem with replacing other zedit commands
 * when you add unrelated ones.
 */
#if 0
#define DEBUG 1
#endif

/*-------------------------------------------------------------------*/

/*
 * External data structures.
 */
extern struct zone_data *zone_table;
extern struct room_data *world;
extern int top_of_zone_table;
extern struct char_data *mob_proto;
extern struct index_data *mob_index;
extern struct obj_data *obj_proto;
extern struct index_data *obj_index;
extern char *equipment_types[];
extern char *dirs[];
extern char *zone_extras[];     /*. constants.c .*/
extern char *wind_types[];      /*. constants.c .*/
extern char *precip_types[];    /*. constants.c .*/
extern char *temp_types[];      /*. constants.c .*/
extern char *season_patterns[]; /*. constants.c .*/
extern char *season_flags[];    /*. constants.c .*/
extern char *season_variance[]; /*. constants.c .*/
int real_zone(int number);
char *strip_color(char *from, char *to, int length);

/*-------------------------------------------------------------------*/

/*
 * Function prototypes.
 */
void zedit_disp_menu(struct descriptor_data *d);
void zedit_setup(struct descriptor_data *d, int room_num);
void add_cmd_to_list(struct reset_com **list, struct reset_com *newcmd, int pos);
void remove_cmd_from_list(struct reset_com **list, int pos);
void delete_command(struct descriptor_data *d, int pos);
int new_command(struct descriptor_data *d, int pos);
int start_change_command(struct descriptor_data *d, int pos);
void zedit_disp_comtype(struct descriptor_data *d);
void zedit_disp_arg1(struct descriptor_data *d);
void zedit_disp_arg2(struct descriptor_data *d);
void zedit_disp_arg3(struct descriptor_data *d);
void zedit_save_internally(struct descriptor_data *d);
void zedit_save_to_disk(int zone_num);
void zedit_create_index(int znum, char *type);
void zedit_new_zone(struct char_data *ch, int vzone_num);
void zedit_disp_zone_extras(struct descriptor_data *d);
void olc_print_bitvectors(FILE *f, long bitvector, long max);

/*-------------------------------------------------------------------*/

/*
 * Nasty internal macros to clean up the code.
 */
#define ZCMD              (zone_table[OLC_ZNUM(d)].cmd[subcmd])
#define MYCMD             (OLC_ZONE(d)->cmd[subcmd])
#define OLC_CMD(d)        (OLC_ZONE(d)->cmd[OLC_VAL(d)])
#define ZON_EXTRAS(zone)  ((zone)->bits)
#define ZON_CLIMATE(zone) ((zone)->climate)

/*-------------------------------------------------------------------*/

/*
 * Utility functions.
 */

/*-------------------------------------------------------------------*/

void zedit_setup(struct descriptor_data *d, int room_num) {
  struct zone_data *zone;
  int i, subcmd = 0, count = 0, cmd_room = -1;

  /*
   * Allocate one scratch zone structure.
   */
  CREATE(zone, struct zone_data, 1);

  /*
   * Copy all the zone header information over.
   */
  zone->name = strdup(zone_table[OLC_ZNUM(d)].name);
  zone->lifespan = zone_table[OLC_ZNUM(d)].lifespan;
  zone->top = zone_table[OLC_ZNUM(d)].top;
  zone->reset_mode = zone_table[OLC_ZNUM(d)].reset_mode;
  /*
   * The remaining fields are used as a 'has been modified' flag
   */
  zone->number = 0; /* Header information has changed.	*/
  zone->age = 0;    /* The commands have changed.		*/
  zone->climate.season_pattern = SEASON_PATTERN(OLC_ZNUM(d));
  for (i = 0; i < MAX_SEASONS; i++)
    zone->climate.season_wind[i] = SEASON_WIND(OLC_ZNUM(d), i);
  for (i = 0; i < MAX_SEASONS; i++)
    zone->climate.season_wind_dir[i] = SEASON_WIND_DIR(OLC_ZNUM(d), i);
  for (i = 0; i < MAX_SEASONS; i++)
    zone->climate.season_wind_variance[i] = SEASON_WIND_VAR(OLC_ZNUM(d), i);
  for (i = 0; i < MAX_SEASONS; i++)
    zone->climate.season_precip[i] = SEASON_PRECIP(OLC_ZNUM(d), i);
  for (i = 0; i < MAX_SEASONS; i++)
    zone->climate.season_temp[i] = SEASON_TEMP(OLC_ZNUM(d), i);
  zone->climate.energy_add = SEASON_ENERGY_ADD(OLC_ZNUM(d));
  zone->climate.flags = SEASON_FLAGS(OLC_ZNUM(d));

  /*
   * Start the reset command list with a terminator.
   */
  CREATE(zone->cmd, struct reset_com, 1);
  zone->cmd[0].command = 'S';

  /*
   * Add all entries in zone_table that relate to this room.
   */
  while (ZCMD.command != 'S') {
    switch (ZCMD.command) {
    case 'M':
    case 'O':
      cmd_room = ZCMD.arg3;
      break;
    case 'D':
    case 'R':
      cmd_room = ZCMD.arg1;
      break;
    default:
      break;
    }
    if (cmd_room == room_num) {
#if defined(DEBUG)
      stderr_log("zedit_setup called add_cmd_to_list.");
#endif
      add_cmd_to_list(&(zone->cmd), &ZCMD, count);
      count++;
    }
    subcmd++;
  }

  OLC_ZONE(d) = zone;
  /*
   * Display main menu.
   */
  zedit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

/*
 * Create a new zone.
 */

void zedit_new_zone(struct char_data *ch, int vzone_num) {
  FILE *fp;
  struct zone_data *new_table;
  int i, j, room;

  if (vzone_num < 0) {
    send_to_char("You can't make negative zones.\r\n", ch);
    return;
  } else if (vzone_num > 999) {
    send_to_char("999 is the highest zone allowed.\r\n", ch);
    return;
  }

  /*
   * Make sure the zone does not exist.
   */
  room = vzone_num * 100;
  for (i = 0; i <= top_of_zone_table; i++)
    if ((zone_table[i].number * 100 <= room) && (zone_table[i].top >= room)) {
      send_to_char("A zone already covers that area.\r\n", ch);
      return;
    }

  /*
   * Create the zone file.
   */
  safe_snprintf(buf, MAX_STRING_LENGTH, "%s/%d.zon", ZON_PREFIX, vzone_num);
  if (!(fp = fopen(buf, "w"))) {
    mudlog("SYSERR: OLC: Can't write new zone file", 'G', COM_BUILDER, TRUE);
    send_to_char("Could not write zone file.\r\n", ch);
    return;
  }
  fprintf(fp,
          "#%d\n"
          "New Zone~\n"
          "%d 30 2\n"
          "1 0 25\n"
          "1 1 1 1\n"
          "0 0 0 0\n"
          "1 1 1 1\n"
          "1 1 1 1\n"
          "6 6 6 6\n"
          "S\n"
          "$\n",
          vzone_num, (vzone_num * 100) + 99);
  fclose(fp);

  /*
   * Create the room file.
   */
  safe_snprintf(buf, MAX_STRING_LENGTH, "%s/%d.wld", WLD_PREFIX, vzone_num);
  if (!(fp = fopen(buf, "w"))) {
    mudlog("SYSERR: OLC: Can't write new world file", 'G', COM_BUILDER, TRUE);
    send_to_char("Could not write world file.\r\n", ch);
    return;
  }
  fprintf(fp, "#%d\nThe Beginning~\nNot much here.\n~\n%d 0 0\nS\n$\n", vzone_num * 100, vzone_num);
  fclose(fp);

  /*
   * Create the mobile file.
   */
  safe_snprintf(buf, MAX_STRING_LENGTH, "%s/%d.mob", MOB_PREFIX, vzone_num);
  if (!(fp = fopen(buf, "w"))) {
    mudlog("SYSERR: OLC: Can't write new mob file", 'G', COM_BUILDER, TRUE);
    send_to_char("Could not write mobile file.\r\n", ch);
    return;
  }
  fprintf(fp, "$\n");
  fclose(fp);

  /*
   * Create the object file.
   */
  safe_snprintf(buf, MAX_STRING_LENGTH, "%s/%d.obj", OBJ_PREFIX, vzone_num);
  if (!(fp = fopen(buf, "w"))) {
    mudlog("SYSERR: OLC: Can't write new obj file", 'G', COM_BUILDER, TRUE);
    send_to_char("Could not write object file.\r\n", ch);
    return;
  }
  fprintf(fp, "$\n");
  fclose(fp);

  /*
   * Create the shop file.
   */
  safe_snprintf(buf, MAX_STRING_LENGTH, "%s/%d.shp", SHP_PREFIX, vzone_num);
  if (!(fp = fopen(buf, "w"))) {
    mudlog("SYSERR: OLC: Can't write new shop file", 'G', COM_BUILDER, TRUE);
    send_to_char("Could not write shop file.\r\n", ch);
    return;
  }
  fprintf(fp, "$~\n");
  fclose(fp);

  /*
   * Update index files.
   */
  zedit_create_index(vzone_num, "zon");
  zedit_create_index(vzone_num, "wld");
  zedit_create_index(vzone_num, "mob");
  zedit_create_index(vzone_num, "obj");
  zedit_create_index(vzone_num, "shp");

  /*
   * Make a new zone in memory. This was the source of all the zedit new
   * crashes reported to the CircleMUD list. It was happily overwriting
   * the stack.  This new loop by Andrew Helm fixes that problem and is
   * more understandable at the same time.
   *
   * The variable is 'top_of_zone_table_table + 2' because we need record 0
   * through top_of_zone (top_of_zone_table + 1 items) and a new one which
   * makes it top_of_zone_table + 2 elements large.
   */
  CREATE(new_table, struct zone_data, top_of_zone_table + 2);
  new_table[top_of_zone_table + 1].number = 99000;

  if (vzone_num > zone_table[top_of_zone_table].number) {
    /*
     * We're adding to the end of the zone table, copy all of the current
     * top_of_zone_table + 1 items over and set write point to before the
     * the last record for the for() loop below.
     */
    memcpy(new_table, zone_table, (sizeof(struct zone_data) * (top_of_zone_table + 1)));
    i = top_of_zone_table + 1;
  } else
    /*
     * Copy over all the zones that are before this zone.
     */
    for (i = 0; vzone_num > zone_table[i].number; i++)
      new_table[i] = zone_table[i];

  /*
   * Ok, insert the new zone here.
   */
  new_table[i].name = strdup("New Zone");
  new_table[i].number = vzone_num;
  new_table[i].top = (vzone_num * 100) + 99;
  new_table[i].lifespan = 30;
  new_table[i].age = 0;
  new_table[i].reset_mode = 2;
  new_table[i].climate.season_pattern = 1;
  new_table[i].climate.flags = 0;
  new_table[i].climate.energy_add = 25;
  for (j = 0; j < MAX_SEASONS; j++) {
    new_table[i].climate.season_wind[j] = 1;
    new_table[i].climate.season_wind_variance[j] = 0;
    new_table[i].climate.season_wind_dir[j] = 1;
    new_table[i].climate.season_precip[j] = 1;
    new_table[i].climate.season_temp[j] = 6;
  }

  /*
   * No zone commands, just terminate it with an 'S'
   */
  CREATE(new_table[i].cmd, struct reset_com, 1);
  new_table[i].cmd[0].command = 'S';

  /*
   * Copy remaining zones into the table one higher, unless of course we
   * are appending to the end in which case this loop will not be used.
   */
  for (; i <= top_of_zone_table; i++)
    new_table[i + 1] = zone_table[i];

  /*
   * Look Ma, no memory leak!
   */
  FREE(zone_table);
  zone_table = new_table;
  top_of_zone_table++;

  /*
   * Previously, creating a new zone while invisible gave you away.
   * That quirk has been fixed with the MAX() statement.
   */
  safe_snprintf(buf, MAX_STRING_LENGTH, "OLC: %s creates new zone #%d", GET_NAME(ch), vzone_num);
  mudlog(buf, 'G', COM_BUILDER, TRUE);
  send_to_char("Zone created successfully.\r\n", ch);

  return;
}

/*-------------------------------------------------------------------*/

void zedit_create_index(int znum, char *type) {
  FILE *newfile, *oldfile;
  char new_name[32], old_name[32], *prefix;
  int num, found = FALSE;

  switch (*type) {
  case 'z':
    prefix = ZON_PREFIX;
    break;
  case 'w':
    prefix = WLD_PREFIX;
    break;
  case 'o':
    prefix = OBJ_PREFIX;
    break;
  case 'm':
    prefix = MOB_PREFIX;
    break;
  case 's':
    prefix = SHP_PREFIX;
    break;
  default:
    /*
     * Caller messed up
     */
    return;
  }

  safe_snprintf(old_name, sizeof(old_name), "%s/index", prefix);
  safe_snprintf(new_name, sizeof(new_name), "%s/newindex", prefix);

  if (!(oldfile = fopen(old_name, "r"))) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "SYSERR: OLC: Failed to open %s", old_name);
    mudlog(buf, 'G', COM_BUILDER, TRUE);
    return;
  } else if (!(newfile = fopen(new_name, "w"))) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "SYSERR: OLC: Failed to open %s", new_name);
    mudlog(buf, 'G', COM_BUILDER, TRUE);
    return;
  }

  /*
   * Index contents must be in order: search through the old file for the
   * right place, insert the new file, then copy the rest over.
   */
  safe_snprintf(buf1, MAX_STRING_LENGTH, "%d.%s", znum, type);
  while (get_line(oldfile, buf)) {
    if (*buf == '$') {
      fprintf(newfile, "%s\n$\n", (!found ? buf1 : ""));
      break;
    } else if (!found) {
      sscanf(buf, "%d", &num);
      if (num > znum) {
        found = TRUE;
        fprintf(newfile, "%s\n", buf1);
      }
    }
    fprintf(newfile, "%s\n", buf);
  }

  fclose(newfile);
  fclose(oldfile);
  /*
   * Out with the old, in with the new.
   */
  remove(old_name);
  rename(new_name, old_name);
}

/*-------------------------------------------------------------------*/

/*
 * Save all the information in the player's temporary buffer back into
 * the current zone table.
 */
void zedit_save_internally(struct descriptor_data *d) {
  int subcmd = 0, cmd_room = -2, room_num = real_room(OLC_NUM(d));
  int i;

  /*
   * Delete all entries in zone_table that relate to this room so we
   * can add all the ones we have in their place.
   */
  while (ZCMD.command != 'S') {
    switch (ZCMD.command) {
    case 'M':
    case 'O':
      cmd_room = ZCMD.arg3;
      break;
    case 'D':
    case 'R':
      cmd_room = ZCMD.arg1;
      break;
    default:
      break;
    }
    if (cmd_room == room_num) {
#if defined(DEBUG)
      stderr_log("zedit_save_internally called remove_cmd_from_list.");
#endif
      remove_cmd_from_list(&(zone_table[OLC_ZNUM(d)].cmd), subcmd);
    } else
      subcmd++;
  }

  /*
   * Now add all the entries in the players descriptor list
   */
  subcmd = 0;
  while (MYCMD.command != 'S') {
#if defined(DEBUG)
    stderr_log("zedit_save_internally called add_cmd_to_list.");
#endif
    add_cmd_to_list(&(zone_table[OLC_ZNUM(d)].cmd), &MYCMD, subcmd);
    subcmd++;
  }

  /*
   * Finally, if zone headers have been changed, copy over
   */
  if ((OLC_ZONE(d)->number) || OLC_VAL(d)) {
    FREE(zone_table[OLC_ZNUM(d)].name);
    zone_table[OLC_ZNUM(d)].name = strdup(OLC_ZONE(d)->name);
    zone_table[OLC_ZNUM(d)].top = OLC_ZONE(d)->top;
    zone_table[OLC_ZNUM(d)].reset_mode = OLC_ZONE(d)->reset_mode;
    zone_table[OLC_ZNUM(d)].lifespan = OLC_ZONE(d)->lifespan;
    SEASON_PATTERN(OLC_ZNUM(d)) = OLC_ZONE(d)->climate.season_pattern;
    SEASON_FLAGS(OLC_ZNUM(d)) = OLC_ZONE(d)->climate.flags;
    SEASON_ENERGY_ADD(OLC_ZNUM(d)) = OLC_ZONE(d)->climate.energy_add;
    for (i = 0; i < MAX_SEASONS; i++) {
      SEASON_WIND(OLC_ZNUM(d), i) = OLC_ZONE(d)->climate.season_wind[i];
      SEASON_WIND_DIR(OLC_ZNUM(d), i) = OLC_ZONE(d)->climate.season_wind_dir[i];
      SEASON_WIND_VAR(OLC_ZNUM(d), i) = OLC_ZONE(d)->climate.season_wind_variance[i];
      SEASON_PRECIP(OLC_ZNUM(d), i) = OLC_ZONE(d)->climate.season_precip[i];
      SEASON_TEMP(OLC_ZNUM(d), i) = OLC_ZONE(d)->climate.season_temp[i];
    }
  }
  zedit_save_to_disk(OLC_ZNUM(d));
}

/*-------------------------------------------------------------------*/

/*
 * Save all the zone_table for this zone to disk.  This function now
 * writes simple comments in the form of (<name>) to each record.  A
 * header for each field is also there.
 */
#undef ZCMD
#define ZCMD (zone_table[zone_num].cmd[subcmd])
void zedit_save_to_disk(int zone_num) {
  int subcmd, arg1 = -1, arg2 = -1, arg3 = -1, arg4 = 100;
  char fname[64];
  char *comment = NULL;
  char tempcomment[1024];
  FILE *zfile;
  int i;

  safe_snprintf(fname, sizeof(fname), "%s/%d.new", ZON_PREFIX, zone_table[zone_num].number);
  if (!(zfile = fopen(fname, "w"))) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "SYSERR: OLC: zedit_save_to_disk:  Can't write zone %d.",
                  zone_table[zone_num].number);
    mudlog(buf, 'G', COM_BUILDER, TRUE);
    return;
  }

  /*
   * Print zone header to file
   */
  fprintf(zfile,
          "#%d\n"
          "%s~\n"
          "%d %d %d ",
          zone_table[zone_num].number,
          (zone_table[zone_num].name && *zone_table[zone_num].name) ? zone_table[zone_num].name : "undefined",
          zone_table[zone_num].top, zone_table[zone_num].lifespan, zone_table[zone_num].reset_mode);

  olc_print_bitvectors(zfile, zone_table[zone_num].bits, NUM_ZONE_EXTRAS);
  fprintf(zfile, "\n");
  fprintf(zfile, "\n* Help for weather:\n"
                 "* <Season Pattern> <season flags> <energy to add on reset>\n"
                 "* <Wind 1> <Wind 2> <Wind 3> <Wind 4>\n"
                 "* <Wind-dir 1> <Wind-dir 2> <Wind-dir 3> <Wind-dir 4>\n"
                 "* <Wind-varience 1> <Wind-varience 2> <Wind-varience 3> <Wind-varience 4>\n"
                 "* <Precip 1> <Precip 2> <Precip 3> <Precip 4>\n"
                 "* <Temp 1> <Temp 2> <Temp 3> <Temp 4>\n"
                 "\n");
  fprintf(zfile, "%d ", SEASON_PATTERN(zone_num));
  olc_print_bitvectors(zfile, SEASON_FLAGS(zone_num), NUM_ZONE_SEASON_FLAGS);
  fprintf(zfile, "%d\n", SEASON_ENERGY_ADD(zone_num));
  for (i = 0; i < MAX_SEASONS; i++)
    fprintf(zfile, "%d ", SEASON_WIND(zone_num, i));
  fprintf(zfile, "\n");
  for (i = 0; i < MAX_SEASONS; i++)
    fprintf(zfile, "%d ", SEASON_WIND_DIR(zone_num, i));
  fprintf(zfile, "\n");
  for (i = 0; i < MAX_SEASONS; i++)
    fprintf(zfile, "%d ", SEASON_WIND_VAR(zone_num, i));
  fprintf(zfile, "\n");
  for (i = 0; i < MAX_SEASONS; i++)
    fprintf(zfile, "%d ", SEASON_PRECIP(zone_num, i));
  fprintf(zfile, "\n");
  for (i = 0; i < MAX_SEASONS; i++)
    fprintf(zfile, "%d ", SEASON_TEMP(zone_num, i));
  fprintf(zfile, "\n");

  fprintf(zfile, "\n* Help for commands:\n"
                 "* Command     Arg #1     Arg #2    Arg #3           Arg #4\n"
                 "* M (Mobile)  Mob-Vnum   Wld-Max   Room-Vnum        Percent-Load\n"
                 "* O (Object)  Obj-Vnum   Wld-Max   Room-Vnum        Percent-Load\n"
                 "* G (Give)    Obj-Vnum   Wld-Max   Unused           Unused\n"
                 "* E (Equip)   Obj-Vnum   Wld-Max   EQ-Position      Unused\n"
                 "* P (Put)     Obj-Vnum   Wld-Max   Target-Obj-Vnum  Unused\n"
                 "* D (Door)    Room-Vnum  Door-Dir  Door-State       Unused\n"
                 "* R (Remove)  Room-Vnum  Obj-Vnum  Unused           Unused\n"
                 "* a comment\n\n");

  for (subcmd = 0; ZCMD.command != 'S'; subcmd++) {
    memset(tempcomment, 0, 1024);
    switch (ZCMD.command) {
    case 'M':
      arg1 = mob_index[ZCMD.arg1].virtual;
      arg2 = ZCMD.arg2;
      arg3 = world[ZCMD.arg3].number;
      arg4 = ZCMD.arg4;
      comment = mob_proto[ZCMD.arg1].player.short_descr;
      strip_color(comment, tempcomment, strlen(comment));
      break;
    case 'O':
      arg1 = obj_index[ZCMD.arg1].virtual;
      arg2 = ZCMD.arg2;
      arg3 = world[ZCMD.arg3].number;
      arg4 = ZCMD.arg4;
      comment = obj_proto[ZCMD.arg1].short_description;
      strip_color(comment, tempcomment, strlen(comment));
      break;
    case 'G':
      arg1 = obj_index[ZCMD.arg1].virtual;
      arg2 = ZCMD.arg2;
      arg3 = -1;
      arg4 = 0;
      comment = obj_proto[ZCMD.arg1].short_description;
      strip_color(comment, tempcomment, strlen(comment));
      break;
    case 'E':
      arg1 = obj_index[ZCMD.arg1].virtual;
      arg2 = ZCMD.arg2;
      arg3 = ZCMD.arg3;
      arg4 = 0;
      comment = obj_proto[ZCMD.arg1].short_description;
      strip_color(comment, tempcomment, strlen(comment));
      break;
    case 'P':
      arg1 = obj_index[ZCMD.arg1].virtual;
      arg2 = ZCMD.arg2;
      arg3 = obj_index[ZCMD.arg3].virtual;
      arg4 = 0;
      comment = obj_proto[ZCMD.arg1].short_description;
      strip_color(comment, tempcomment, strlen(comment));
      break;
    case 'D':
      arg1 = world[ZCMD.arg1].number;
      arg2 = ZCMD.arg2;
      arg3 = ZCMD.arg3;
      arg4 = 0;
      comment = world[ZCMD.arg1].name;
      strip_color(comment, tempcomment, strlen(comment));
      break;
    case 'R':
      arg1 = world[ZCMD.arg1].number;
      arg2 = obj_index[ZCMD.arg2].virtual;
      comment = obj_proto[ZCMD.arg2].short_description;
      strip_color(comment, tempcomment, strlen(comment));
      arg3 = -1;
      arg4 = 0;
      break;
    case '*':
      /*
       * Invalid commands are replaced with '*' - Ignore them.
       */
      continue;
    default:
      safe_snprintf(buf, MAX_STRING_LENGTH, "SYSERR: OLC: z_save_to_disk(): Unknown cmd '%c' - NOT saving",
                    ZCMD.command);
      mudlog(buf, 'G', COM_BUILDER, TRUE);
      continue;
    }
    if (arg4)
      fprintf(zfile, "%c %d %d %d %d %d\t* %s\n", ZCMD.command, ZCMD.if_flag, arg1, arg2, arg3, arg4, tempcomment);
    else
      fprintf(zfile, "%c %d %d %d %d\t* %s\n", ZCMD.command, ZCMD.if_flag, arg1, arg2, arg3, tempcomment);
  }
  fprintf(zfile, "S\n$\n");
  fclose(zfile);
  safe_snprintf(buf2, MAX_STRING_LENGTH, "%s/%d.zon", ZON_PREFIX, zone_table[zone_num].number);
  /*
   * We're fubar'd if we crash between the two lines below.
   */
  remove(buf2);
  rename(fname, buf2);
}

/*-------------------------------------------------------------------*/

/*
 * Some common code to count the number of comands in the list.
 */
int count_commands(struct reset_com *list) {
  int count = 0;

  while (list[count].command != 'S')
    count++;

  return count;
}

/*-------------------------------------------------------------------*/

/*
 * Adds a new reset command into a list.  Takes a pointer to the list
 * so that it may play with the memory locations.
 */
void add_cmd_to_list(struct reset_com **list, struct reset_com *newcmd, int pos) {
  int count, i, l;
  struct reset_com *newlist;

  /*
   * Count number of commands (not including terminator).
   */
  count = count_commands(*list);

  /*
   * Value is +2 for the terminator and new field to add.
   */
  CREATE(newlist, struct reset_com, count + 2);

  /*
   * Even tighter loop to copy the old list and insert a new command.
   */
  for (i = 0, l = 0; i <= count; i++) {
    newlist[i] = ((i == pos) ? *newcmd : (*list)[l++]);
#if defined(DEBUG)
    safe_snprintf(buf, MAX_STRING_LENGTH, "add_cmd_to_list: added %c %d %d %d %d", newlist[i].command, newlist[i].arg1,
                  newlist[i].arg2, newlist[i].arg3, newlist[i].line);
    stderr_log(buf);
#endif
  }

  /*
   * Add terminator, then insert new list.
   */
  newlist[count + 1].command = 'S';
  FREE(*list);
  *list = newlist;
}

/*-------------------------------------------------------------------*/

/*
 * Remove a reset command from a list.  Takes a pointer to the list
 * so that it may play with the memory locations.
 */
void remove_cmd_from_list(struct reset_com **list, int pos) {
  int count, i, l;
  struct reset_com *newlist;

  /*
   * Count number of commands (not including terminator)
   */
  count = count_commands(*list);

  /*
   * Value is 'count' because we didn't include the terminator above
   * but since we're deleting one thing anyway we want one less.
   */
  CREATE(newlist, struct reset_com, count);

  /*
   * Even tighter loop to copy old list and skip unwanted command.
   */
  for (i = 0, l = 0; i < count; i++) {
    if (i != pos) {
#if defined(DEBUG)
      safe_snprintf(buf, MAX_STRING_LENGTH, "remove_cmd_from_list: kept %c %d %d %d %d", (*list)[i].command,
                    (*list)[i].arg1, (*list)[i].arg2, (*list)[i].arg3, (*list)[i].line);
#endif
      newlist[l++] = (*list)[i];
    }
#if defined(DEBUG)
    else
      safe_snprintf(buf, MAX_STRING_LENGTH, "remove_cmd_from_list: deleted %c %d %d %d %d", (*list)[i].command,
                    (*list)[i].arg1, (*list)[i].arg2, (*list)[i].arg3, (*list)[i].line);
    stderr_log(buf);
#endif
  }
  /*
   * Add the terminator, then insert the new list.
   */
  newlist[count - 1].command = 'S';
  FREE(*list);
  *list = newlist;
}

/*-------------------------------------------------------------------*/
void zedit_disp_season_flags(struct descriptor_data *d) {
  int i;
  int columns = 0;

  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  for (i = 0; *season_flags[i] != '\n'; i++) {
    size_t blen = safe_snprintf(buf, MAX_STRING_LENGTH, "%s%2d%s) %s\r\n", grn, i + 1, nrm, season_flags[i]);
    if (!(++columns % 2))
      safe_snprintf(buf + blen, MAX_STRING_LENGTH - blen, "\r\n");
    send_to_char(buf, d->character);
  }
  sprintbit(ZON_CLIMATE(OLC_ZONE(d)).flags, season_flags, buf1);
  safe_snprintf(buf, MAX_STRING_LENGTH,
                "\r\n"
                "Current settings  : %s%s%s\r\n"
                "Enter zone setting (0 to quit) : ",
                cyn, buf1, nrm);
  send_to_char(buf, d->character);
}

/*-------------------------------------------------------------------*/
void zedit_disp_zone_extras(struct descriptor_data *d) {
  int i;
  int columns = 0;

  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  for (i = 0; *zone_extras[i] != '\n'; i++) {
    size_t blen = safe_snprintf(buf, MAX_STRING_LENGTH, "%s%2d%s) %s\r\n", grn, i + 1, nrm, zone_extras[i]);
    if (!(++columns % 2))
      safe_snprintf(buf + blen, MAX_STRING_LENGTH - blen, "\r\n");
    send_to_char(buf, d->character);
  }
  sprintbit(ZON_EXTRAS(OLC_ZONE(d)), zone_extras, buf1);
  safe_snprintf(buf, MAX_STRING_LENGTH,
                "\r\n"
                "Current settings  : %s%s%s\r\n"
                "Enter zone setting (0 to quit) : ",
                cyn, buf1, nrm);
  send_to_char(buf, d->character);
}

/*-------------------------------------------------------------------*/
/*. Climate menu .*/
void zedit_disp_climate_info(struct descriptor_data *d) {
  int i;
  size_t len;
  char abuf[MAX_STRING_LENGTH];
  send_to_char("[H[J", d->character);
  safe_snprintf(abuf, sizeof(abuf), "1) Season Pattern: %s\r\n",
                season_patterns[ZON_CLIMATE(OLC_ZONE(d)).season_pattern - 1]);
  send_to_char(abuf, d->character);
  sprintbit(ZON_CLIMATE(OLC_ZONE(d)).flags, season_flags, buf1);
  safe_snprintf(abuf, sizeof(abuf),
                "2) Season Flags: %s\r\n"
                "3) Season Energy: %d\r\n",
                buf1, ZON_CLIMATE(OLC_ZONE(d)).energy_add);
  send_to_char(abuf, d->character);
  len = safe_snprintf(abuf, sizeof(abuf), "4) Season Winds: ");
  for (i = 0; i < MAX_SEASONS; i++)
    if (ZON_CLIMATE(OLC_ZONE(d)).season_wind[i])
      len += safe_snprintf(abuf + len, sizeof(abuf) - len, " %s,",
                           wind_types[ZON_CLIMATE(OLC_ZONE(d)).season_wind[i] - 1]);
    else
      len += safe_snprintf(abuf + len, sizeof(abuf) - len, " NONE,");
  safe_snprintf(abuf + len, sizeof(abuf) - len, "\r\n");
  send_to_char(abuf, d->character);
  len = safe_snprintf(abuf, sizeof(abuf), "5) Season Variance: ");
  for (i = 0; i < MAX_SEASONS; i++)
    len += safe_snprintf(abuf + len, sizeof(abuf) - len, " %s,",
                         season_variance[ZON_CLIMATE(OLC_ZONE(d)).season_wind_variance[i]]);
  safe_snprintf(abuf + len, sizeof(abuf) - len, "\r\n");
  send_to_char(abuf, d->character);
  len = safe_snprintf(abuf, sizeof(abuf), "6) Season Wind Direction: ");
  for (i = 0; i < MAX_SEASONS; i++)
    len += safe_snprintf(abuf + len, sizeof(abuf) - len, " %s,", dirs[ZON_CLIMATE(OLC_ZONE(d)).season_wind_dir[i]]);
  safe_snprintf(abuf + len, sizeof(abuf) - len, "\r\n");
  send_to_char(abuf, d->character);
  len = safe_snprintf(abuf, sizeof(abuf), "7) Season Precipitation: ");
  for (i = 0; i < MAX_SEASONS; i++)
    if (ZON_CLIMATE(OLC_ZONE(d)).season_precip[i])
      len += safe_snprintf(abuf + len, sizeof(abuf) - len, " %s,",
                           precip_types[ZON_CLIMATE(OLC_ZONE(d)).season_precip[i] - 1]);
    else
      len += safe_snprintf(abuf + len, sizeof(abuf) - len, " NONE,");
  safe_snprintf(abuf + len, sizeof(abuf) - len, "\r\n");
  send_to_char(abuf, d->character);
  len = safe_snprintf(abuf, sizeof(abuf), "8) Season Temperature: ");
  for (i = 0; i < MAX_SEASONS; i++)
    len +=
        safe_snprintf(abuf + len, sizeof(abuf) - len, " %s,", temp_types[ZON_CLIMATE(OLC_ZONE(d)).season_temp[i] - 1]);
  safe_snprintf(abuf + len, sizeof(abuf) - len, "\r\n");
  send_to_char(abuf, d->character);
  send_to_char("Enter number of option to edit [0 to exit]: ", d->character);
}

/*-------------------------------------------------------------------*/

/*
 * Error check user input and then add new (blank) command
 */
int new_command(struct descriptor_data *d, int pos) {
  int subcmd = 0;
  struct reset_com *new_com;

  /*
   * Error check to ensure users hasn't given too large an index
   */
  while (MYCMD.command != 'S')
    subcmd++;

  if ((pos > subcmd) || (pos < 0))
    return 0;

  /*
   * Ok, let's add a new (blank) command
   */
  CREATE(new_com, struct reset_com, 1);
  new_com->command = 'N';
#if defined(DEBUG)
  stderr_log("new_command called add_cmd_to_list.");
#endif
  add_cmd_to_list(&OLC_ZONE(d)->cmd, new_com, pos);
  return 1;
}

/*-------------------------------------------------------------------*/
/*
 * Error check user input and then remove command
 */

void delete_command(struct descriptor_data *d, int pos) {
  int subcmd = 0;

  /*
   * Error check to ensure users hasn't given too large an index
   */
  while (MYCMD.command != 'S')
    subcmd++;

  if ((pos >= subcmd) || (pos < 0))
    return;

    /*
     * Ok, let's zap it
     */
#if defined(DEBUG)
  stderr_log("delete_command called remove_cmd_from_list.");
#endif
  remove_cmd_from_list(&OLC_ZONE(d)->cmd, pos);
}

/*-------------------------------------------------------------------*/
/*
 * Error check user input and then setup change
 */

int start_change_command(struct descriptor_data *d, int pos) {
  int subcmd = 0;

  /*
   * Error check to ensure users hasn't given too large an index
   */
  while (MYCMD.command != 'S')
    subcmd++;

  if ((pos >= subcmd) || (pos < 0))
    return 0;

  /*
   * Ok, let's get editing
   */
  OLC_VAL(d) = pos;
  return 1;
}

/**************************************************************************
 Menu functions
 **************************************************************************/

/*
 * the main menu
 */
void zedit_disp_menu(struct descriptor_data *d) {
  int subcmd = 0, counter = 0;
  size_t buflen;

  get_char_cols(d->character);
  sprintbit(ZON_EXTRAS(OLC_ZONE(d)), zone_extras, buf1);

  /*
   * Menu header
   */
  buflen = safe_snprintf(buf, MAX_STRING_LENGTH,
#if defined(CLEAR_SCREEN)
                         "[H[J"
#endif
                         "Room number: %s%d%s		Room zone: %s%d\r\n"
                         "%sZ%s) Zone name   : %s%s\r\n"
                         "%sL%s) Lifespan    : %s%d minutes\r\n"
                         "%sT%s) Top of zone : %s%d\r\n"
                         "%sR%s) Reset Mode  : %s%s%s\r\n"
                         "%sS%s) Extra Settings  : %s%s%s\r\n"
                         "%sC%s) Climate\r\n"
                         "[Command list]\r\n",

                         cyn, OLC_NUM(d), nrm, cyn, zone_table[OLC_ZNUM(d)].number, grn, nrm, nrm,
                         OLC_ZONE(d)->name ? OLC_ZONE(d)->name : "<NONE!>", grn, nrm, yel, OLC_ZONE(d)->lifespan, grn,
                         nrm, yel, OLC_ZONE(d)->top, grn, nrm, yel,
                         OLC_ZONE(d)->reset_mode
                             ? ((OLC_ZONE(d)->reset_mode == 1) ? "Reset when no players are in zone." : "Normal reset.")
                             : "Never reset",
                         nrm, grn, nrm, yel, buf1, nrm, grn, nrm);

  /*
   * Print the commands for this room into display buffer.
   */
  while (MYCMD.command != 'S') {
    /*
     * Translate what the command means.
     */
    switch (MYCMD.command) {
    case 'M':
      safe_snprintf(buf2, MAX_STRING_LENGTH, "%sLoad %s [%s%d%s], Max : %d, %%load : %d", MYCMD.if_flag ? " then " : "",
                    mob_proto[MYCMD.arg1].player.short_descr, cyn, mob_index[MYCMD.arg1].virtual, yel, MYCMD.arg2,
                    MYCMD.arg4);
      break;
    case 'G':
      safe_snprintf(buf2, MAX_STRING_LENGTH, "%sGive it %s [%s%d%s], Max : %d", MYCMD.if_flag ? " then " : "",
                    obj_proto[MYCMD.arg1].short_description, cyn, obj_index[MYCMD.arg1].virtual, yel, MYCMD.arg2);
      break;
    case 'O':
      safe_snprintf(buf2, MAX_STRING_LENGTH, "%sLoad %s [%s%d%s], Max : %d, %%load : %d", MYCMD.if_flag ? " then " : "",
                    obj_proto[MYCMD.arg1].short_description, cyn, obj_index[MYCMD.arg1].virtual, yel, MYCMD.arg2,
                    MYCMD.arg4);
      break;
    case 'E':
      safe_snprintf(buf2, MAX_STRING_LENGTH, "%sEquip with %s [%s%d%s], %s, Max : %d", MYCMD.if_flag ? " then " : "",
                    obj_proto[MYCMD.arg1].short_description, cyn, obj_index[MYCMD.arg1].virtual, yel,
                    equipment_types[MYCMD.arg3], MYCMD.arg2);
      break;
    case 'P':
      safe_snprintf(buf2, MAX_STRING_LENGTH, "%sPut %s [%s%d%s] in %s [%s%d%s], Max : %d",
                    MYCMD.if_flag ? " then " : "", obj_proto[MYCMD.arg1].short_description, cyn,
                    obj_index[MYCMD.arg1].virtual, yel, obj_proto[MYCMD.arg3].short_description, cyn,
                    obj_index[MYCMD.arg3].virtual, yel, MYCMD.arg2);
      break;
    case 'R':
      safe_snprintf(buf2, MAX_STRING_LENGTH, "%sRemove %s [%s%d%s] from room.", MYCMD.if_flag ? " then " : "",
                    obj_proto[MYCMD.arg2].short_description, cyn, obj_index[MYCMD.arg2].virtual, yel);
      break;
    case 'D':
      safe_snprintf(
          buf2, MAX_STRING_LENGTH, "%sSet door %s as %s.", MYCMD.if_flag ? " then " : "", dirs[MYCMD.arg2],
          MYCMD.arg3 ? ((MYCMD.arg3 == 1)
                            ? "closed"
                            : ((MYCMD.arg3 == 2)
                                   ? "locked"
                                   : ((MYCMD.arg3 == 3) ? "hidden and closed"
                                                        : ((MYCMD.arg3 == 4) ? "locked and hidden" : "hidden only"))))
                     : "open"

      );
      break;
    default:
      safe_snprintf(buf2, MAX_STRING_LENGTH, "<Unknown Command>");
      break;
    }
    /*
     * Build the display buffer for this command
     */
    safe_snprintf(buf1, MAX_STRING_LENGTH, "%s%d - %s%s\r\n", nrm, counter++, yel, buf2);
    buflen += safe_snprintf(buf + buflen, MAX_STRING_LENGTH - buflen, "%s", buf1);
    subcmd++;
  }
  /*
   * Finish off menu
   */
  safe_snprintf(buf + buflen, MAX_STRING_LENGTH - buflen,
                "%s%d - <END OF LIST>\r\n"
                "%sN%s) New command.\r\n"
                "%sE%s) Edit a command.\r\n"
                "%sD%s) Delete a command.\r\n"
                "%sQ%s) Quit\r\nEnter your choice : ",
                nrm, counter, grn, nrm, grn, nrm, grn, nrm, grn, nrm);
  send_to_char(buf, d->character);

  OLC_MODE(d) = ZEDIT_MAIN_MENU;
}

/*-------------------------------------------------------------------*/

/*
 * Print the command type menu and setup response catch.
 */
void zedit_disp_comtype(struct descriptor_data *d) {
  get_char_cols(d->character);
  safe_snprintf(buf, MAX_STRING_LENGTH,
#if defined(CLEAR_SCREEN)
                "[H[J"
#endif
                "%sM%s) Load Mobile to room             %sO%s) Load Object to room\r\n"
                "%sE%s) Equip mobile with object        %sG%s) Give an object to a mobile\r\n"
                "%sP%s) Put object in another object    %sD%s) Open/Close/Lock/Hide a Door\r\n"
                "%sR%s) Remove an object from the room\r\n"
                "What sort of command will this be? : ",
                grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm, grn, nrm);
  send_to_char(buf, d->character);
  OLC_MODE(d) = ZEDIT_COMMAND_TYPE;
}

/*-------------------------------------------------------------------*/

/*
 * Print the appropriate message for the command type for arg1 and set
 * up the input catch clause
 */
void zedit_disp_arg1(struct descriptor_data *d) {
  switch (OLC_CMD(d).command) {
  case 'M':
    send_to_char("Input mob's vnum : ", d->character);
    OLC_MODE(d) = ZEDIT_ARG1;
    break;
  case 'O':
  case 'E':
  case 'P':
  case 'G':
    send_to_char("Input object vnum : ", d->character);
    OLC_MODE(d) = ZEDIT_ARG1;
    break;
  case 'D':
  case 'R':
    /*
     * Arg1 for these is the room number, skip to arg2
     */
    OLC_CMD(d).arg1 = real_room(OLC_NUM(d));
    zedit_disp_arg2(d);
    break;
  default:
    /*
     * We should never get here  .
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog("SYSERR: OLC: zedit_disp_arg1(): Help!", 'G', COM_BUILDER, TRUE);
    send_to_char("Oops...\r\n", d->character);
    return;
  }
}

/*-------------------------------------------------------------------*/

/*
 * Print the appropriate message for the command type for arg2 and set
 * up the input catch clause.
 */
void zedit_disp_arg2(struct descriptor_data *d) {
  int i = 0;

  switch (OLC_CMD(d).command) {
  case 'M':
  case 'O':
  case 'E':
  case 'P':
  case 'G':
    send_to_char("Input the maximum number that can exist on the mud : ", d->character);
    break;
  case 'D':
    while (*dirs[i] != '\n') {
      safe_snprintf(buf, MAX_STRING_LENGTH, "%d) Exit %s.\r\n", i, dirs[i]);
      send_to_char(buf, d->character);
      i++;
    }
    send_to_char("Enter exit number for door : ", d->character);
    break;
  case 'R':
    send_to_char("Input object's vnum : ", d->character);
    break;
  default:
    /*
     * We should never get here, but just in case...
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog("SYSERR: OLC: zedit_disp_arg2(): Help!", 'G', COM_BUILDER, TRUE);
    send_to_char("Oops...\r\n", d->character);
    return;
  }
  OLC_MODE(d) = ZEDIT_ARG2;
}

/*-------------------------------------------------------------------*/

/*
 * Print the appropriate message for the command type for arg3 and set
 * up the input catch clause.
 */
void zedit_disp_arg3(struct descriptor_data *d) {
  int i = 0;

  switch (OLC_CMD(d).command) {
  case 'E':
    while (*equipment_types[i] != '\n') {
      safe_snprintf(buf, MAX_STRING_LENGTH, "%2d) %26.26s %2d) %26.26s\r\n", i, equipment_types[i], i + 1,
                    (*equipment_types[i + 1] != '\n') ? equipment_types[i + 1] : "");
      send_to_char(buf, d->character);
      if (*equipment_types[i + 1] != '\n')
        i += 2;
      else
        break;
    }
    send_to_char("Location to equip : ", d->character);
    break;
  case 'P':
    send_to_char("Vnum of the container : ", d->character);
    break;
  case 'D':
    send_to_char("0)  Door open\r\n"
                 "1)  Door closed\r\n"
                 "2)  Door locked\r\n"
                 "3)  Door hidden and closed\r\n"
                 "4)  Door locked and hidden\r\n"
                 "5)  Door hidden only\r\n"
                 "Enter state of the door : ",
                 d->character);
    break;
  case 'M':
  case 'O':
  case 'R':
  case 'G':
  default:
    /*
     * We should never get here, just in case.
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog("SYSERR: OLC: zedit_disp_arg3(): Help!", 'G', COM_BUILDER, TRUE);
    send_to_char("Oops...\r\n", d->character);
    return;
  }
  OLC_MODE(d) = ZEDIT_ARG3;
}

/*-------------------------------------------------------------------*/
/*. Print the appropriate message for the command type for arg4 and set
 up the input catch clause .*/

void zedit_disp_arg4(struct descriptor_data *d) {
  switch (OLC_CMD(d).command) {
  case 'M':
  case 'O':
    send_to_char("Input the percent chance to load : ", d->character);
    break;
  default:
    /*. We should never get here .*/
    cleanup_olc(d, CLEANUP_ALL);
    mudlog("SYSERR: OLC: zedit_disp_arg2(): Help!", 'G', COM_BUILDER, TRUE);
    return;
  }
  OLC_MODE(d) = ZEDIT_ARG4;
}

/**************************************************************************
 The GARGANTAUN event handler
 **************************************************************************/

void zedit_parse(struct descriptor_data *d, char *arg) {
  int pos, i = 0;

  switch (OLC_MODE(d)) {
  /*-------------------------------------------------------------------*/
  case ZEDIT_CONFIRM_SAVESTRING:
    switch (*arg) {
    case 'y':
    case 'Y':
      /*
       * Save the zone in memory, hiding invisible people.
       */
      send_to_char("Saving zone info in memory and to disk.\r\n", d->character);
      zedit_save_internally(d);
      /* FALL THROUGH */
    case 'n':
    case 'N':
      safe_snprintf(buf, MAX_STRING_LENGTH, "OLC: %s finished editing zone info for room %d.", GET_NAME(d->character),
                    OLC_NUM(d));
      mudlog(buf, 'G', COM_BUILDER, TRUE);
      cleanup_olc(d, CLEANUP_ALL);
      break;
    default:
      send_to_char("Invalid choice!\r\n", d->character);
      send_to_char("Do you wish to save the zone info? : ", d->character);
      break;
    }
    break;
    /* End of ZEDIT_CONFIRM_SAVESTRING */

    /*-------------------------------------------------------------------*/
  case ZEDIT_MAIN_MENU:
    switch (*arg) {
    case 'q':
    case 'Q':
      if (OLC_ZONE(d)->age || OLC_ZONE(d)->number || OLC_VAL(d)) {
        send_to_char("Do you wish to save the changes to the zone info? (y/n) : ", d->character);
        OLC_MODE(d) = ZEDIT_CONFIRM_SAVESTRING;
      } else {
        send_to_char("No changes made.\r\n", d->character);
        safe_snprintf(buf, MAX_STRING_LENGTH, "OLC: %s finished editing zone info for room %d.", GET_NAME(d->character),
                      OLC_NUM(d));
        mudlog(buf, 'G', COM_BUILDER, TRUE);
        cleanup_olc(d, CLEANUP_ALL);
      }
      break;
    case 'n':
    case 'N':
      /*
       * New entry.
       */
      send_to_char("What number in the list should the new command be? : ", d->character);
      OLC_MODE(d) = ZEDIT_NEW_ENTRY;
      break;
    case 'e':
    case 'E':
      /*
       * Change an entry.
       */
      send_to_char("Which command do you wish to change? : ", d->character);
      OLC_MODE(d) = ZEDIT_CHANGE_ENTRY;
      break;
    case 'd':
    case 'D':
      /*
       * Delete an entry.
       */
      send_to_char("Which command do you wish to delete? : ", d->character);
      OLC_MODE(d) = ZEDIT_DELETE_ENTRY;
      break;
    case 'z':
    case 'Z':
      /*
       * Edit zone name.
       */
      send_to_char("Enter new zone name : ", d->character);
      OLC_MODE(d) = ZEDIT_ZONE_NAME;
      break;
    case 't':
    case 'T':
      /*
       * Edit top of zone.
       */
      if (!COM_FLAGGED(d->character, COM_ADMIN))
        zedit_disp_menu(d);
      else {
        send_to_char("Enter new top of zone : ", d->character);
        OLC_MODE(d) = ZEDIT_ZONE_TOP;
      }
      break;
    case 'c':
    case 'C':
      /*. Edit zone climate .*/
      zedit_disp_climate_info(d);
      OLC_MODE(d) = ZEDIT_ZONE_CLIMATE;
      break;
    case 'l':
    case 'L':
      /*
       * Edit zone lifespan.
       */
      send_to_char("Enter new zone lifespan : ", d->character);
      OLC_MODE(d) = ZEDIT_ZONE_LIFE;
      break;
    case 'r':
    case 'R':
      /*
       * Edit zone reset mode.
       */
      send_to_char("\r\n"
                   "0) Never reset\r\n"
                   "1) Reset only when no players in zone\r\n"
                   "2) Normal reset\r\n"
                   "Enter new zone reset type : ",
                   d->character);
      OLC_MODE(d) = ZEDIT_ZONE_RESET;
      break;
    case 's':
    case 'S':
      OLC_MODE(d) = ZEDIT_ZONE_EXTRAS;
      zedit_disp_zone_extras(d);
      break;
    default:
      zedit_disp_menu(d);
      break;
    }
    break;
    /* End of ZEDIT_MAIN_MENU */

    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_CLIMATE:
    pos = atoi(arg);
    switch (pos) {
    case 1:
      *buf1 = '\0';
      for (i = 0; *season_patterns[i] != '\n'; i++)
        safe_snprintf(buf1 + strlen(buf1), MAX_STRING_LENGTH - strlen(buf1), "%d) %s\r\n", i + 1, season_patterns[i]);
      send_to_char(buf1, d->character);
      send_to_char("Enter new season pattern: ", d->character);
      OLC_VAL(d) = 1;
      OLC_MODE(d) = ZEDIT_ZONE_SEASON_PATTERN;
      break;
    case 2:
      zedit_disp_season_flags(d);
      OLC_MODE(d) = ZEDIT_ZONE_SEASON_FLAGS;
      OLC_VAL(d) = 1;
      break;
    case 3:
      send_to_char("Enter new energy for zone: ", d->character);
      OLC_MODE(d) = ZEDIT_ZONE_SEASON_ENERGY;
      OLC_VAL(d) = 1;
      break;
    case 4:
      *buf1 = '\0';
      for (i = 0; i < MAX_SEASONS; i++)
        safe_snprintf(buf1 + strlen(buf1), MAX_STRING_LENGTH - strlen(buf1), "%d) %s\r\n", i + 1,
                      wind_types[ZON_CLIMATE(OLC_ZONE(d)).season_wind[i] - 1]);
      send_to_char(buf1, d->character);
      send_to_char("Enter number to change: ", d->character);
      OLC_MODE(d) = ZEDIT_ZONE_SEASON_WIND;
      OLC_VAL(d) = 1;
      break;
    case 5:
      *buf1 = '\0';
      for (i = 0; i < MAX_SEASONS; i++)
        safe_snprintf(buf1 + strlen(buf1), MAX_STRING_LENGTH - strlen(buf1), "%d) %s\r\n", i + 1,
                      season_variance[ZON_CLIMATE(OLC_ZONE(d)).season_wind_variance[i]]);
      send_to_char(buf1, d->character);
      send_to_char("Enter number to change: ", d->character);
      OLC_MODE(d) = ZEDIT_ZONE_SEASON_VAR;
      OLC_VAL(d) = 1;
      break;
    case 6:
      *buf1 = '\0';
      for (i = 0; i < MAX_SEASONS; i++)
        safe_snprintf(buf1 + strlen(buf1), MAX_STRING_LENGTH - strlen(buf1), "%d) %s\r\n", i + 1,
                      dirs[ZON_CLIMATE(OLC_ZONE(d)).season_wind_dir[i]]);
      send_to_char(buf1, d->character);
      send_to_char("Enter number to change: ", d->character);
      OLC_MODE(d) = ZEDIT_ZONE_SEASON_WINDDIR;
      OLC_VAL(d) = 1;
      break;
    case 7:
      *buf1 = '\0';
      for (i = 0; i < MAX_SEASONS; i++)
        safe_snprintf(buf1 + strlen(buf1), MAX_STRING_LENGTH - strlen(buf1), "%d) %s\r\n", i + 1,
                      precip_types[ZON_CLIMATE(OLC_ZONE(d)).season_precip[i] - 1]);
      send_to_char(buf1, d->character);
      send_to_char("Enter number to change: ", d->character);
      OLC_MODE(d) = ZEDIT_ZONE_SEASON_PRECIP;
      OLC_VAL(d) = 1;
      break;
    case 8:
      *buf1 = '\0';
      for (i = 0; i < MAX_SEASONS; i++)
        safe_snprintf(buf1 + strlen(buf1), MAX_STRING_LENGTH - strlen(buf1), "%d) %s\r\n", i + 1,
                      temp_types[ZON_CLIMATE(OLC_ZONE(d)).season_temp[i] - 1]);
      send_to_char(buf1, d->character);
      send_to_char("Enter number to change: ", d->character);
      OLC_MODE(d) = ZEDIT_ZONE_SEASON_TEMP;
      OLC_VAL(d) = 1;
      break;
    default:
      zedit_disp_menu(d);
      break;
    }
    break;
    /*-------------------------------------------------------------------*/
  case ZEDIT_NEW_ENTRY:
    /*
     * Get the line number and insert the new line.
     */
    pos = atoi(arg);
    if (isdigit(*arg) && new_command(d, pos)) {
      if (start_change_command(d, pos)) {
        zedit_disp_comtype(d);
        OLC_ZONE(d)->age = 1;
      }
    } else
      zedit_disp_menu(d);
    break;

    /*-------------------------------------------------------------------*/
  case ZEDIT_DELETE_ENTRY:
    /*
     * Get the line number and delete the line.
     */
    pos = atoi(arg);
    if (isdigit(*arg)) {
      delete_command(d, pos);
      OLC_ZONE(d)->age = 1;
    }
    zedit_disp_menu(d);
    break;

    /*-------------------------------------------------------------------*/
  case ZEDIT_CHANGE_ENTRY:
    /*
     * Parse the input for which line to edit, and goto next quiz.
     */
    pos = atoi(arg);
    if (isdigit(*arg) && start_change_command(d, pos)) {
      zedit_disp_comtype(d);
      OLC_ZONE(d)->age = 1;
    } else
      zedit_disp_menu(d);
    break;

    /*-------------------------------------------------------------------*/
  case ZEDIT_COMMAND_TYPE:
    /*
     * Parse the input for which type of command this is, and goto next
     * quiz.
     */
    OLC_CMD(d).command = toupper(*arg);
    if (!OLC_CMD(d).command || (strchr("MOPEDGR", OLC_CMD(d).command) == NULL)) {
      send_to_char("Invalid choice, try again : ", d->character);
    } else {
      if (OLC_VAL(d)) { /* If there was a previous command. */
        send_to_char("Is this command dependent on the success of the previous one? (y/n)\r\n", d->character);
        OLC_MODE(d) = ZEDIT_IF_FLAG;
      } else { /* 'if-flag' not appropriate. */
        OLC_CMD(d).if_flag = 0;
        zedit_disp_arg1(d);
      }
    }
    break;

    /*-------------------------------------------------------------------*/
  case ZEDIT_IF_FLAG:
    /*
     * Parse the input for the if flag, and goto next quiz.
     */
    switch (*arg) {
    case 'y':
    case 'Y':
      OLC_CMD(d).if_flag = 1;
      break;
    case 'n':
    case 'N':
      OLC_CMD(d).if_flag = 0;
      break;
    default:
      send_to_char("Try again : ", d->character);
      return;
    }
    zedit_disp_arg1(d);
    break;

    /*-------------------------------------------------------------------*/
  case ZEDIT_ARG1:
    /*
     * Parse the input for arg1, and goto next quiz.
     */
    if (!isdigit(*arg)) {
      send_to_char("Must be a numeric value, try again : ", d->character);
      return;
    }
    switch (OLC_CMD(d).command) {
    case 'M':
      if ((pos = real_mobile(atoi(arg))) >= 0) {
        OLC_CMD(d).arg1 = pos;
        zedit_disp_arg2(d);
      } else
        send_to_char("That mobile does not exist, try again : ", d->character);
      break;
    case 'O':
    case 'P':
    case 'E':
    case 'G':
      if ((pos = real_object(atoi(arg))) >= 0) {
        OLC_CMD(d).arg1 = pos;
        zedit_disp_arg2(d);
      } else
        send_to_char("That object does not exist, try again : ", d->character);
      break;
    case 'D':
    case 'R':
    default:
      /*
       * We should never get here.
       */
      cleanup_olc(d, CLEANUP_ALL);
      mudlog("SYSERR: OLC: zedit_parse(): case ARG1: Ack!", 'G', COM_BUILDER, TRUE);
      send_to_char("Oops...\r\n", d->character);
      break;
    }
    break;

    /*-------------------------------------------------------------------*/
  case ZEDIT_ARG2:
    /*
     * Parse the input for arg2, and goto next quiz.
     */
    if (!isdigit(*arg)) {
      send_to_char("Must be a numeric value, try again : ", d->character);
      return;
    }
    switch (OLC_CMD(d).command) {
    case 'M':
    case 'O':
      OLC_CMD(d).arg2 = atoi(arg);
      OLC_CMD(d).arg3 = real_room(OLC_NUM(d));
      zedit_disp_arg4(d);
      break;
    case 'G':
      OLC_CMD(d).arg2 = atoi(arg);
      zedit_disp_menu(d);
      break;
    case 'P':
    case 'E':
      OLC_CMD(d).arg2 = atoi(arg);
      zedit_disp_arg3(d);
      break;
    case 'D':
      pos = atoi(arg);
      /*
       * Count directions.
       */
      while (*dirs[i] != '\n')
        i++;
      if ((pos < 0) || (pos > i))
        send_to_char("Try again : ", d->character);
      else {
        OLC_CMD(d).arg2 = pos;
        zedit_disp_arg3(d);
      }
      break;
    case 'R':
      if ((pos = real_object(atoi(arg))) >= 0) {
        OLC_CMD(d).arg2 = pos;
        zedit_disp_menu(d);
      } else
        send_to_char("That object does not exist, try again : ", d->character);
      break;
    default:
      /*
       * We should never get here, but just in case...
       */
      cleanup_olc(d, CLEANUP_ALL);
      mudlog("SYSERR: OLC: zedit_parse(): case ARG2: Ack!", 'G', COM_BUILDER, TRUE);
      send_to_char("Oops...\r\n", d->character);
      break;
    }
    break;

    /*-------------------------------------------------------------------*/
  case ZEDIT_ARG3:
    /*
     * Parse the input for arg3, and go back to main menu.
     */
    if (!isdigit(*arg)) {
      send_to_char("Must be a numeric value, try again : ", d->character);
      return;
    }
    switch (OLC_CMD(d).command) {
    case 'E':
      pos = atoi(arg);
      /*
       * Count number of wear positions.  We could use NUM_WEARS, this is
       * more reliable.
       */
      while (*equipment_types[i] != '\n')
        i++;
      if ((pos < 0) || (pos > i))
        send_to_char("Try again : ", d->character);
      else {
        OLC_CMD(d).arg3 = pos;
        zedit_disp_menu(d);
      }
      break;
    case 'P':
      if ((pos = real_object(atoi(arg))) >= 0) {
        OLC_CMD(d).arg3 = pos;
        zedit_disp_menu(d);
      } else
        send_to_char("That object does not exist, try again : ", d->character);
      break;
    case 'D':
      pos = atoi(arg);
      if ((pos < 0) || (pos > 5))
        send_to_char("Try again : ", d->character);
      else {
        OLC_CMD(d).arg3 = pos;
        zedit_disp_menu(d);
      }
      break;
    case 'M':
    case 'O':
    case 'G':
    case 'R':
    default:
      /*
       * We should never get here, but just in case...
       */
      cleanup_olc(d, CLEANUP_ALL);
      mudlog("SYSERR: OLC: zedit_parse(): case ARG3: Ack!", 'G', COM_BUILDER, TRUE);
      send_to_char("Oops...\r\n", d->character);
      break;
    }
    break;

    /*-------------------------------------------------------------------*/
  case ZEDIT_ARG4:
    /*. Parse the input for arg4, and goto next quiz .*/
    if (!isdigit(*arg) || atoi(arg) < 0 || atoi(arg) > 100) {
      send_to_char("Must be a numeric value, try again : ", d->character);
      return;
    }
    switch (OLC_CMD(d).command) {
    case 'M':
    case 'O':
      OLC_CMD(d).arg4 = atoi(arg);
      zedit_disp_menu(d);
      break;
    default:
      /*. We should never get here .*/
      cleanup_olc(d, CLEANUP_ALL);
      mudlog("SYSERR: OLC: zedit_parse(): case ARG4: Ack!", 'G', COM_BUILDER, TRUE);
    }
    break;

    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_NAME:
    /*
     * Add new name and return to main menu.
     */
    if (OLC_ZONE(d)->name) {
      FREE(OLC_ZONE(d)->name);
    } else {
      stderr_log("SYSERR: OLC: ZEDIT_ZONE_NAME: no name to free!");
    }
    OLC_ZONE(d)->name = strdup(arg);
    OLC_ZONE(d)->number = 1;
    zedit_disp_menu(d);
    break;
    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_SEASON_PATTERN:
    pos = atoi(arg);
    if (pos < 1 || pos > 8) {
      *buf1 = '\0';
      for (i = 0; *season_patterns[i] != '\n'; i++)
        safe_snprintf(buf1 + strlen(buf1), MAX_STRING_LENGTH - strlen(buf1), "%d) %s\r\n", i + 1, season_patterns[i]);
      send_to_char(buf1, d->character);
      send_to_char("Enter new season pattern: ", d->character);
    } else {
      ZON_CLIMATE(OLC_ZONE(d)).season_pattern = pos;
      zedit_disp_climate_info(d);
    }
    OLC_MODE(d) = ZEDIT_ZONE_CLIMATE;
    break;
    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_SEASON_FLAGS:
    i = atoi(arg);
    if (i == 0) {
      zedit_disp_climate_info(d);
      OLC_MODE(d) = ZEDIT_ZONE_CLIMATE;
      break;
    }
    if (!((i < 0) || (i > NUM_ZONE_SEASON_FLAGS))) {
      i = 1 << (i - 1);
      if (IS_SET(ZON_CLIMATE(OLC_ZONE(d)).flags, i))
        REMOVE_BIT(ZON_CLIMATE(OLC_ZONE(d)).flags, i);
      else
        SET_BIT(ZON_CLIMATE(OLC_ZONE(d)).flags, i);
      OLC_VAL(d) = 1;
    }
    zedit_disp_season_flags(d);
    return;
    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_SEASON_ENERGY:
    pos = atoi(arg);
    if (pos < 0)
      pos = 0;
    ZON_CLIMATE(OLC_ZONE(d)).energy_add = pos;
    zedit_disp_climate_info(d);
    OLC_MODE(d) = ZEDIT_ZONE_CLIMATE;
    break;
    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_SEASON_WIND:
    *buf1 = '\0';
    pos = atoi(arg);
    for (i = 0; *wind_types[i] != '\n'; i++)
      safe_snprintf(buf1 + strlen(buf1), MAX_STRING_LENGTH - strlen(buf1), "%d) %s\r\n", i + 1, wind_types[i]);
    send_to_char(buf1, d->character);
    send_to_char("Enter new wind type: ", d->character);
    switch (pos) {
    case 1:
      OLC_MODE(d) = ZEDIT_ZONE_SEASON_WIND1;
      break;
    case 2:
      OLC_MODE(d) = ZEDIT_ZONE_SEASON_WIND2;
      break;
    case 3:
      OLC_MODE(d) = ZEDIT_ZONE_SEASON_WIND3;
      break;
    case 4:
      OLC_MODE(d) = ZEDIT_ZONE_SEASON_WIND4;
      break;
    }
    break;
    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_SEASON_VAR:
    *buf1 = '\0';
    pos = atoi(arg);
    for (i = 0; *season_variance[i] != '\n'; i++)
      safe_snprintf(buf1 + strlen(buf1), MAX_STRING_LENGTH - strlen(buf1), "%d) %s\r\n", i, season_variance[i]);
    send_to_char(buf1, d->character);
    send_to_char("Enter new variance type: ", d->character);
    switch (pos) {
    case 1:
      OLC_MODE(d) = ZEDIT_ZONE_SEASON_VAR1;
      break;
    case 2:
      OLC_MODE(d) = ZEDIT_ZONE_SEASON_VAR2;
      break;
    case 3:
      OLC_MODE(d) = ZEDIT_ZONE_SEASON_VAR3;
      break;
    case 4:
      OLC_MODE(d) = ZEDIT_ZONE_SEASON_VAR4;
      break;
    }
    break;
    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_SEASON_VAR1:
    pos = atoi(arg);
    ZON_CLIMATE(OLC_ZONE(d)).season_wind_variance[0] = pos;
    zedit_disp_climate_info(d);
    OLC_MODE(d) = ZEDIT_ZONE_CLIMATE;
    break;
    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_SEASON_VAR2:
    pos = atoi(arg);
    ZON_CLIMATE(OLC_ZONE(d)).season_wind_variance[1] = pos;
    zedit_disp_climate_info(d);
    OLC_MODE(d) = ZEDIT_ZONE_CLIMATE;
    break;
    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_SEASON_VAR3:
    pos = atoi(arg);
    ZON_CLIMATE(OLC_ZONE(d)).season_wind_variance[2] = pos;
    zedit_disp_climate_info(d);
    OLC_MODE(d) = ZEDIT_ZONE_CLIMATE;
    break;
    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_SEASON_VAR4:
    pos = atoi(arg);
    ZON_CLIMATE(OLC_ZONE(d)).season_wind_variance[3] = pos;
    zedit_disp_climate_info(d);
    OLC_MODE(d) = ZEDIT_ZONE_CLIMATE;
    break;
    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_SEASON_WIND1:
    pos = atoi(arg);
    ZON_CLIMATE(OLC_ZONE(d)).season_wind[0] = pos;
    zedit_disp_climate_info(d);
    OLC_MODE(d) = ZEDIT_ZONE_CLIMATE;
    break;
    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_SEASON_WIND2:
    pos = atoi(arg);
    ZON_CLIMATE(OLC_ZONE(d)).season_wind[1] = pos;
    zedit_disp_climate_info(d);
    OLC_MODE(d) = ZEDIT_ZONE_CLIMATE;
    break;
    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_SEASON_WIND3:
    pos = atoi(arg);
    ZON_CLIMATE(OLC_ZONE(d)).season_wind[2] = pos;
    zedit_disp_climate_info(d);
    OLC_MODE(d) = ZEDIT_ZONE_CLIMATE;
    break;
    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_SEASON_WIND4:
    pos = atoi(arg);
    ZON_CLIMATE(OLC_ZONE(d)).season_wind[3] = pos;
    zedit_disp_climate_info(d);
    OLC_MODE(d) = ZEDIT_ZONE_CLIMATE;
    break;
    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_SEASON_WINDDIR:
    pos = atoi(arg);
    send_to_char("0) North\r\n"
                 "1) East\r\n"
                 "2) South\r\n"
                 "3) West\r\n"
                 "4) Up\r\n"
                 "5) Down\r\n"
                 "\r\nEnter Choice: ",
                 d->character);
    switch (pos) {
    case 1:
      OLC_MODE(d) = ZEDIT_ZONE_SEASON_WINDDIR1;
      break;
    case 2:
      OLC_MODE(d) = ZEDIT_ZONE_SEASON_WINDDIR2;
      break;
    case 3:
      OLC_MODE(d) = ZEDIT_ZONE_SEASON_WINDDIR3;
      break;
    case 4:
      OLC_MODE(d) = ZEDIT_ZONE_SEASON_WINDDIR4;
      break;
    }
    break;
    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_SEASON_WINDDIR1:
    pos = atoi(arg);
    ZON_CLIMATE(OLC_ZONE(d)).season_wind_dir[0] = pos;
    zedit_disp_climate_info(d);
    OLC_MODE(d) = ZEDIT_ZONE_CLIMATE;
    break;
    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_SEASON_WINDDIR2:
    pos = atoi(arg);
    ZON_CLIMATE(OLC_ZONE(d)).season_wind_dir[1] = pos;
    zedit_disp_climate_info(d);
    OLC_MODE(d) = ZEDIT_ZONE_CLIMATE;
    break;
    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_SEASON_WINDDIR3:
    pos = atoi(arg);
    ZON_CLIMATE(OLC_ZONE(d)).season_wind_dir[2] = pos;
    zedit_disp_climate_info(d);
    OLC_MODE(d) = ZEDIT_ZONE_CLIMATE;
    break;
    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_SEASON_WINDDIR4:
    pos = atoi(arg);
    ZON_CLIMATE(OLC_ZONE(d)).season_wind_dir[3] = pos;
    zedit_disp_climate_info(d);
    OLC_MODE(d) = ZEDIT_ZONE_CLIMATE;
    break;
    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_SEASON_PRECIP:
    *buf1 = '\0';
    pos = atoi(arg);
    for (i = 0; *precip_types[i] != '\n'; i++)
      safe_snprintf(buf1 + strlen(buf1), MAX_STRING_LENGTH - strlen(buf1), "%d) %s\r\n", i + 1, precip_types[i]);
    send_to_char(buf1, d->character);
    send_to_char("Enter new precip type: ", d->character);
    switch (pos) {
    case 1:
      OLC_MODE(d) = ZEDIT_ZONE_SEASON_PRECIP1;
      break;
    case 2:
      OLC_MODE(d) = ZEDIT_ZONE_SEASON_PRECIP2;
      break;
    case 3:
      OLC_MODE(d) = ZEDIT_ZONE_SEASON_PRECIP3;
      break;
    case 4:
      OLC_MODE(d) = ZEDIT_ZONE_SEASON_PRECIP4;
      break;
    }
    break;
    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_SEASON_PRECIP1:
    pos = atoi(arg);
    ZON_CLIMATE(OLC_ZONE(d)).season_precip[0] = pos;
    zedit_disp_climate_info(d);
    OLC_MODE(d) = ZEDIT_ZONE_CLIMATE;
    break;
    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_SEASON_PRECIP2:
    pos = atoi(arg);
    ZON_CLIMATE(OLC_ZONE(d)).season_precip[1] = pos;
    zedit_disp_climate_info(d);
    OLC_MODE(d) = ZEDIT_ZONE_CLIMATE;
    break;
    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_SEASON_PRECIP3:
    pos = atoi(arg);
    ZON_CLIMATE(OLC_ZONE(d)).season_precip[2] = pos;
    zedit_disp_climate_info(d);
    OLC_MODE(d) = ZEDIT_ZONE_CLIMATE;
    break;
    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_SEASON_PRECIP4:
    pos = atoi(arg);
    ZON_CLIMATE(OLC_ZONE(d)).season_precip[3] = pos;
    zedit_disp_climate_info(d);
    OLC_MODE(d) = ZEDIT_ZONE_CLIMATE;
    break;
    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_SEASON_TEMP:
    *buf1 = '\0';
    pos = atoi(arg);
    for (i = 0; *temp_types[i] != '\n'; i++)
      safe_snprintf(buf1 + strlen(buf1), MAX_STRING_LENGTH - strlen(buf1), "%d) %s\r\n", i + 1, temp_types[i]);
    send_to_char(buf1, d->character);
    send_to_char("Enter new temp type: ", d->character);
    switch (pos) {
    case 1:
      OLC_MODE(d) = ZEDIT_ZONE_SEASON_TEMP1;
      break;
    case 2:
      OLC_MODE(d) = ZEDIT_ZONE_SEASON_TEMP2;
      break;
    case 3:
      OLC_MODE(d) = ZEDIT_ZONE_SEASON_TEMP3;
      break;
    case 4:
      OLC_MODE(d) = ZEDIT_ZONE_SEASON_TEMP4;
      break;
    }
    break;
    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_SEASON_TEMP1:
    pos = atoi(arg);
    ZON_CLIMATE(OLC_ZONE(d)).season_temp[0] = pos;
    zedit_disp_climate_info(d);
    OLC_MODE(d) = ZEDIT_ZONE_CLIMATE;
    break;
    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_SEASON_TEMP2:
    pos = atoi(arg);
    ZON_CLIMATE(OLC_ZONE(d)).season_temp[1] = pos;
    zedit_disp_climate_info(d);
    OLC_MODE(d) = ZEDIT_ZONE_CLIMATE;
    break;
    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_SEASON_TEMP3:
    pos = atoi(arg);
    ZON_CLIMATE(OLC_ZONE(d)).season_temp[2] = pos;
    zedit_disp_climate_info(d);
    OLC_MODE(d) = ZEDIT_ZONE_CLIMATE;
    break;
    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_SEASON_TEMP4:
    pos = atoi(arg);
    ZON_CLIMATE(OLC_ZONE(d)).season_temp[3] = pos;
    zedit_disp_climate_info(d);
    OLC_MODE(d) = ZEDIT_ZONE_CLIMATE;
    break;

    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_RESET:
    /*
     * Parse and add new reset_mode and return to main menu.
     */
    pos = atoi(arg);
    if (!isdigit(*arg) || (pos < 0) || (pos > 2))
      send_to_char("Try again (0-2) : ", d->character);
    else {
      OLC_ZONE(d)->reset_mode = pos;
      OLC_ZONE(d)->number = 1;
      zedit_disp_menu(d);
    }
    break;

    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_EXTRAS:
    i = atoi(arg);

    if (i == 0) {
      zedit_disp_menu(d);
      break;
    }
    if (!((i < 0) || (i > NUM_ZONE_EXTRAS))) {
      i = 1 << (i - 1);
      if (IS_SET(ZON_EXTRAS(OLC_ZONE(d)), i))
        REMOVE_BIT(ZON_EXTRAS(OLC_ZONE(d)), i);
      else
        SET_BIT(ZON_EXTRAS(OLC_ZONE(d)), i);
      OLC_VAL(d) = 1;
    }
    zedit_disp_zone_extras(d);
    return;

    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_LIFE:
    /*
     * Parse and add new lifespan and return to main menu.
     */
    pos = atoi(arg);
    if (!isdigit(*arg) || (pos < 0) || (pos > 240))
      send_to_char("Try again (0-240) : ", d->character);
    else {
      OLC_ZONE(d)->lifespan = pos;
      OLC_ZONE(d)->number = 1;
      zedit_disp_menu(d);
    }
    break;

    /*-------------------------------------------------------------------*/
  case ZEDIT_ZONE_TOP:
    /*
     * Parse and add new top room in zone and return to main menu.
     */
    if (OLC_ZNUM(d) == top_of_zone_table)
      OLC_ZONE(d)->top = BOUNDED(OLC_ZNUM(d) * 100, atoi(arg), 99900);
    else
      OLC_ZONE(d)->top = BOUNDED(OLC_ZNUM(d) * 100, atoi(arg), zone_table[OLC_ZNUM(d) + 1].number * 100);
    zedit_disp_menu(d);
    break;

    /*-------------------------------------------------------------------*/
  default:
    /*
     * We should never get here, but just in case...
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog("SYSERR: OLC: zedit_parse(): Reached default case!", 'G', COM_BUILDER, TRUE);
    send_to_char("Oops...\r\n", d->character);
    break;
  }
}

/*
 * End of parse_zedit()
 */
