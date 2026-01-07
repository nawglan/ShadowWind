/* ************************************************************************
 *   File: db.c                                          Part of CircleMUD *
 *  Usage: Loading/saving chars, booting/resetting world, internal funcs   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#define __DB_C__

#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "handler.h"
#include "event.h"
#include "spells.h"
#include "mail.h"
#include "interpreter.h"

void file_list_skills(struct char_data *ch, FILE *f);
void mprog_read_programs(FILE * fp, struct index_data * pMobIndex);
void init_mm(void);
extern void parse_quest(FILE* f, int vnum);
extern int load_qic_check(int rnum);
extern void qic_scan_rent(void);
extern void purge_qic(int rnum);
extern void qic_load(int rnum);
extern void load_actd(void);
extern void update_log_file(void);
extern void start_main_threads(void);
extern int boot_mazes(void);
extern struct balance_type balance_table[];
extern char *resists_names[];
void strip_string(char *string);
void skip_spaces(char **string);
char *get_spell_name(int spellnum);
int spell_comp(const void* element1, const void* element2);
int find_spell_num(char *name);
extern const sh_int monk_stat[LVL_IMMORT + 1][5];

char err_buf[MAX_STRING_LENGTH];

/**************************************************************************
 *  declarations of most of the 'global' variables                         *
 ************************************************************************ */

struct room_data *world = NULL; /* array of rooms     */
int top_of_world = 0; /* ref to top element of world   */

struct char_data *character_list = NULL; /* global linked list of
 * chars   */
struct index_data *mob_index; /* index table for mobile file   */
struct char_data *mob_proto; /* prototypes for mobs     */
int top_of_mobt = 0; /* top of mobile index table   */
int new_top_mobt = 0; /* new top, used for OLC   */
int mob_idnum = -1; /* mobs have a negative idnum    */
/* Mob Quests */
struct quest_data *mob_quests;

struct obj_data *object_list = NULL; /* global linked list of objs   */
struct index_data *obj_index; /* index table for object file   */
struct obj_data *obj_proto; /* prototypes for objs     */
int top_of_objt = 0; /* top of object index table   */
int new_top_objt = 0; /* New top, used for OLC   */

struct zone_data *zone_table; /* zone table       */
int top_of_zone_table = 0; /* top element of zone tab   */
struct message_list fight_messages[MAX_MESSAGES]; /* fighting messages   */

int top_of_p_table = 0; /* ref to top of table     */
int top_of_p_file = 0; /* ref of size of p file   */
long top_idnum = 0; /* highest idnum in use     */

int no_mail = 0; /* mail disabled?     */
int mini_mud = 0; /* mini-mud mode?     */
int no_rent_check = 0; /* skip rent check on boot?   */
time_t boot_time = 0; /* time of mud boot     */
int restrict_game_lvl = 0; /* level of game restriction   */
sh_int r_mortal_start_room; /* rnum of mortal start room   */
sh_int r_immort_start_room; /* rnum of immort start room   */
sh_int r_frozen_start_room; /* rnum of frozen start room   */
char *credits = NULL; /* game credits       */
char *news = NULL; /* mud news       */
char *motd = NULL; /* message of the day - mortals */
char *nmotd = NULL; /* message of the day - newbie */
char *MENU = NULL; /* menu */
char *GREET1 = NULL; /* splash screen color */
char *GREET2 = NULL; /* splash screen no-color */
char *todolist;
char *helplist;
char *buglist;
char *idealist;
char *typolist;
char *imotd = NULL; /* message of the day - immorts */
char *help = NULL; /* help screen       */
char *info = NULL; /* info page       */
char *wizlist = NULL; /* list of higher gods     */
char *immlist = NULL; /* list of peon gods     */
char *background = NULL; /* background story     */
char *handbook = NULL; /* handbook for new immortals   */
char *policies = NULL;
char *namepol = NULL; /* policies page     */
char *signoff = NULL; /* signoff message     */
char *quest = NULL; /* quest runners     */

struct help_index_element *help_index = 0; /* the help table*/
struct help_index_element *wiz_help_index = 0; /* the help table*/
int top_of_helpt; /* top of help index table   */
int top_of_wiz_helpt; /* top of wiz_help index table   */
int max_players = 0; /* max players so far     */
int numspells = 0; /* holds the number of spells read */
int NumSpellsDefined = 0; /* number of spells/skills in spells file */

int death_count = 0;

struct time_info_data time_info;/* the infomation about the time    */
struct reset_q_type reset_q; /* queue of zones to be reset   */
struct player_special_data dummy_mob;
struct spell_info_type *spells;

/* local functions */
void load_spells(void);
void setup_dir(FILE * fl, int room, int dir);
void index_boot(int mode);
void discrete_load(FILE * fl, int mode);
void parse_room(FILE * fl, int virtual_nr);
void parse_mobile(FILE * mob_f, int nr);
char *parse_object(FILE * obj_f, int nr);
void load_zones(FILE * fl, char *zonename);
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void assign_the_shopkeepers(void);
void build_player_index(void);
int is_empty(int zone_nr);
void reset_zone(int zone);
int file_to_string(char *name, char *buf);
int file_to_string_alloc(char *name, char **buf);
void check_start_rooms(void);
void renum_world(void);
void renum_zone_table(void);
void log_zone_error(int zone, int cmd_no, char *message);
void reset_time(void);
void clear_char(struct char_data * ch);
void parse_pline(char *line, char *field, char *value);

/* external functions */
extern struct descriptor_data *descriptor_list;
void load_messages(void);
void weather_and_time(int mode);
void boot_social_messages(void);
void update_obj_file(void); /* In objsave.c */
void load_auction(void); /* In auction.c */
void load_qic(void); /* In qic.c */
void load_banned(void);
void Read_Invalid_List(void);
void boot_the_shops(FILE * shop_f, char *filename, int rec_count);
struct help_index_element *build_help_index(int *num, int type);

/*************************************************************************
 *  routines for booting the system                                       *
 *********************************************************************** */

/* this is necessary for the autowiz system */
void reboot_wizlists(void)
{
  file_to_string_alloc(WIZLIST_FILE, &wizlist);
  file_to_string_alloc(IMMLIST_FILE, &immlist);
}

ACMD(do_reboot)
{
  int i;
  one_argument(argument, arg);

  if (!str_cmp(arg, "all") || *arg == '*') {
    file_to_string_alloc(NEWS_FILE, &news);
    file_to_string_alloc(CREDITS_FILE, &credits);
    file_to_string_alloc(MOTD_FILE, &motd);
    file_to_string_alloc(NMOTD_FILE, &nmotd);
    file_to_string_alloc(IMOTD_FILE, &imotd);
    file_to_string_alloc(MENU_FILE, &MENU);
    file_to_string_alloc(GREET1_FILE, &GREET1);
    file_to_string_alloc(GREET2_FILE, &GREET2);
    file_to_string_alloc(HELP_PAGE_FILE, &help);
    file_to_string_alloc(INFO_FILE, &info);
    file_to_string_alloc(WIZLIST_FILE, &wizlist);
    file_to_string_alloc(IMMLIST_FILE, &immlist);
    file_to_string_alloc(POLICIES_FILE, &policies);
    file_to_string_alloc(NAMEPOL_FILE, &namepol);
    file_to_string_alloc(HANDBOOK_FILE, &handbook);
    file_to_string_alloc(BACKGROUND_FILE, &background);
    file_to_string_alloc(QUEST_FILE, &quest);
    file_to_string_alloc(HELPN_FILE, &helplist);
    file_to_string_alloc(TODO_FILE, &todolist);
    file_to_string_alloc(BUG_FILE, &buglist);
    file_to_string_alloc(IDEA_FILE, &idealist);
    file_to_string_alloc(TYPO_FILE, &typolist);
    file_to_string_alloc(SIGNOFF_FILE, &signoff);
  } else if (!str_cmp(arg, "wizlist")) {
    file_to_string_alloc(WIZLIST_FILE, &wizlist);
    file_to_string_alloc(IMMLIST_FILE, &immlist);
  } else if (!str_cmp(arg, "immlist"))
    file_to_string_alloc(IMMLIST_FILE, &immlist);
  else if (!str_cmp(arg, "todo"))
    file_to_string_alloc(TODO_FILE, &todolist);
  else if (!str_cmp(arg, "bugs"))
    file_to_string_alloc(BUG_FILE, &buglist);
  else if (!str_cmp(arg, "helplist"))
    file_to_string_alloc(HELPN_FILE, &helplist);
  else if (!str_cmp(arg, "ideas"))
    file_to_string_alloc(IDEA_FILE, &idealist);
  else if (!str_cmp(arg, "typos"))
    file_to_string_alloc(TYPO_FILE, &typolist);
  else if (!str_cmp(arg, "news"))
    file_to_string_alloc(NEWS_FILE, &news);
  else if (!str_cmp(arg, "credits"))
    file_to_string_alloc(CREDITS_FILE, &credits);
  else if (!str_cmp(arg, "motd"))
    file_to_string_alloc(MOTD_FILE, &motd);
  else if (!str_cmp(arg, "nmotd"))
    file_to_string_alloc(NMOTD_FILE, &nmotd);
  else if (!str_cmp(arg, "imotd"))
    file_to_string_alloc(IMOTD_FILE, &imotd);
  else if (!str_cmp(arg, "greet1"))
    file_to_string_alloc(GREET1_FILE, &GREET1);
  else if (!str_cmp(arg, "greet2"))
    file_to_string_alloc(GREET2_FILE, &GREET2);
  else if (!str_cmp(arg, "menu"))
    file_to_string_alloc(MENU_FILE, &MENU);
  else if (!str_cmp(arg, "help"))
    file_to_string_alloc(HELP_PAGE_FILE, &help);
  else if (!str_cmp(arg, "info"))
    file_to_string_alloc(INFO_FILE, &info);
  else if (!str_cmp(arg, "policy"))
    file_to_string_alloc(POLICIES_FILE, &policies);
  else if (!str_cmp(arg, "namepol"))
    file_to_string_alloc(NAMEPOL_FILE, &namepol);
  else if (!str_cmp(arg, "signoff"))
    file_to_string_alloc(SIGNOFF_FILE, &signoff);
  else if (!str_cmp(arg, "handbook"))
    file_to_string_alloc(HANDBOOK_FILE, &handbook);
  else if (!str_cmp(arg, "background"))
    file_to_string_alloc(BACKGROUND_FILE, &background);
  else if (!str_cmp(arg, "quest"))
    file_to_string_alloc(QUEST_FILE, &quest);
  else if (!str_cmp(arg, "xhelp")) {
    for (i = 0; i < top_of_wiz_helpt; i++)
      FREE(wiz_help_index[i].keyword);
    FREE(wiz_help_index);
    wiz_help_index = build_help_index(&top_of_wiz_helpt, HELP_WIZHELP);
    for (i = 0; i < top_of_helpt; i++)
      FREE(help_index[i].keyword);
    FREE(help_index);
    help_index = build_help_index(&top_of_helpt, HELP_HELP);
  } else {
    send_to_char("Unknown reboot option.\r\n", ch);
    return;
  }

  if (str_cmp(arg, "helplist") != 0)
    send_to_char(OK, ch);
}

/* body of the booting system */
void boot_db(void)
{
  int i;
  extern int no_specials;

  stderr_log("Boot db -- BEGIN.");

  stderr_log("Resetting the game time:");
  reset_time();

  /* init random number generator */
  init_mm();

  stderr_log("Reading news, credits, help, bground, info & motds.");
  file_to_string_alloc(NEWS_FILE, &news);
  file_to_string_alloc(CREDITS_FILE, &credits);
  file_to_string_alloc(MOTD_FILE, &motd);
  file_to_string_alloc(NMOTD_FILE, &nmotd);
  file_to_string_alloc(IMOTD_FILE, &imotd);
  file_to_string_alloc(MENU_FILE, &MENU);
  file_to_string_alloc(GREET1_FILE, &GREET1);
  file_to_string_alloc(GREET2_FILE, &GREET2);
  file_to_string_alloc(HELP_PAGE_FILE, &help);
  file_to_string_alloc(INFO_FILE, &info);
  file_to_string_alloc(WIZLIST_FILE, &wizlist);
  file_to_string_alloc(IMMLIST_FILE, &immlist);
  file_to_string_alloc(POLICIES_FILE, &policies);
  file_to_string_alloc(NAMEPOL_FILE, &namepol);
  file_to_string_alloc(SIGNOFF_FILE, &signoff);
  file_to_string_alloc(HANDBOOK_FILE, &handbook);
  file_to_string_alloc(BACKGROUND_FILE, &background);
  file_to_string_alloc(QUEST_FILE, &quest);
  file_to_string_alloc(TODO_FILE, &todolist);
  file_to_string_alloc(HELPN_FILE, &helplist);
  file_to_string_alloc(BUG_FILE, &buglist);
  file_to_string_alloc(IDEA_FILE, &idealist);
  file_to_string_alloc(TYPO_FILE, &typolist);

  stderr_log("Opening help file.");
  help_index = build_help_index(&top_of_helpt, HELP_HELP);

  stderr_log("Opening wiz_help file.");
  wiz_help_index = build_help_index(&top_of_wiz_helpt, HELP_WIZHELP);

  stderr_log("Loading and sorting spells.");
  load_spells();

  stderr_log("Loading fight messages.");
  load_messages();

  stderr_log("Loading zone table.");
  index_boot(DB_BOOT_ZON);

  stderr_log("Loading rooms.");
  index_boot(DB_BOOT_WLD);

  stderr_log("Generating random mazes.");
  boot_mazes();
  stderr_log("Finished making random mazes.");

  stderr_log("Renumbering rooms.");
  renum_world();

  stderr_log("Checking start rooms.");
  check_start_rooms();

  stderr_log("Loading objs and generating index.");
  index_boot(DB_BOOT_OBJ);

  stderr_log("Loading mobs and generating index.");
  index_boot(DB_BOOT_MOB);

  stderr_log("Loading Mob Quests.");
  index_boot(DB_BOOT_QST);

  stderr_log("Renumbering zone table.");
  renum_zone_table();

  stderr_log("Loading ACTD messages.");
  load_actd();

  stderr_log("Loading social messages.");
  boot_social_messages();

  stderr_log("Loading QIC database.");
  load_qic();

  stderr_log("Scanning rent files for QIC items.");
  qic_scan_rent();

  stderr_log("Loading and cleaning auction database.");
  load_auction();

  if (!no_specials) {
    stderr_log("Loading shops.");
    index_boot(DB_BOOT_SHP);
  }
  stderr_log("Assigning function pointers:");

  if (!no_specials) {
    stderr_log("   Mobiles.");
    assign_mobiles();
    stderr_log("   Shopkeepers.");
    assign_the_shopkeepers();
    stderr_log("   Objects.");
    assign_objects();
    stderr_log("   Rooms.");
    assign_rooms();
  }
  stderr_log("Booting mail system.");
  if (!scan_file()) {
    stderr_log("    Mail boot failed -- Mail system disabled");
    no_mail = 1;
  }
  stderr_log("Reading banned site and invalid-name list.");
  load_banned();
  Read_Invalid_List();

  if (!no_rent_check) {
    stderr_log("Deleting timed-out crash and rent files:");
    update_obj_file();
    stderr_log("Done.");
  }

  stderr_log("Deleting timed-out personal log entrys.");
  update_log_file();

  for (i = 0; i <= top_of_zone_table; i++) {
    snprintf(buf2, MAX_STRING_LENGTH, "Resetting %s (rooms %d-%d).", zone_table[i].name, (i ? (zone_table[i].bottom) : 0), zone_table[i].top);
    stderr_log(buf2);
    reset_zone(i);
  }

  reset_q.head = reset_q.tail = NULL;

  boot_time = time(0);

  MOBTrigger = TRUE;

  stderr_log("Boot db -- DONE.");

}

/* reset the time in the game from file */
void reset_time(void)
{
  long beginning_of_time = 650336715;
  struct time_info_data mud_time_passed(time_t t2, time_t t1);

  time_info = mud_time_passed(time(0), beginning_of_time);

  snprintf(buf, MAX_STRING_LENGTH, "   Current Gametime: %dH %dD %dM %dY.", time_info.hours, time_info.day, time_info.month, time_info.year);
  stderr_log(buf);
}

/* function to count how many hash-mark delimited records exist in a file */
int count_hash_records(FILE * fl)
{
  char buf[128];
  int count = 0;

  while (fgets(buf, 128, fl))
    if (*buf == '#')
      count++;

  return count;
}

void index_boot(int mode)
{
  char *index_filename, *prefix;
  FILE *index, *db_file;
  int rec_count = 0;

  switch (mode) {
    case DB_BOOT_WLD:
      prefix = WLD_PREFIX;
      break;
    case DB_BOOT_MOB:
      prefix = MOB_PREFIX;
      break;
    case DB_BOOT_OBJ:
      prefix = OBJ_PREFIX;
      break;
    case DB_BOOT_ZON:
      prefix = ZON_PREFIX;
      break;
    case DB_BOOT_SHP:
      prefix = SHP_PREFIX;
      break;
    case DB_BOOT_QST:
      prefix = QST_PREFIX;
      break;
    default:
      stderr_log("SYSERR: Unknown subcommand to index_boot!");
      fflush(NULL);
      exit(1);
      break;
  }

  if (mini_mud)
    index_filename = MINDEX_FILE;
  else
    index_filename = INDEX_FILE;

  snprintf(buf2, MAX_STRING_LENGTH, "%s/%s", prefix, index_filename);

  if (!(index = fopen(buf2, "r"))) {
    snprintf(buf1, MAX_STRING_LENGTH, "Error opening index file '%s'", buf2);
    perror(buf1);
    fflush(NULL);
    exit(1);
  }
  /* first, count the number of records in the file so we can malloc */
  fscanf(index, "%s\n", buf1);
  while (*buf1 != '$') {
    snprintf(buf2, MAX_STRING_LENGTH, "%s/%s", prefix, buf1);
    if (!(db_file = fopen(buf2, "r"))) {
      perror(buf2);
      fflush(NULL);
      exit(1);
    } else {
      if (mode == DB_BOOT_ZON)
        rec_count++;
      else
        rec_count += count_hash_records(db_file);
    }

    fclose(db_file);
    fscanf(index, "%s\n", buf1);
  }

  if (!rec_count && mode != DB_BOOT_QST) {
    stderr_log("SYSERR: boot error - 0 records counted");
    fflush(NULL);
    exit(1);
  }
  rec_count++;

  switch (mode) {
    case DB_BOOT_WLD:
      CREATE(world, struct room_data, rec_count);
      break;
    case DB_BOOT_MOB:
      CREATE(mob_proto, struct char_data, rec_count);
      CREATE(mob_index, struct index_data, rec_count);
      break;
    case DB_BOOT_OBJ:
      CREATE(obj_proto, struct obj_data, rec_count);
      CREATE(obj_index, struct index_data, rec_count);
      break;
    case DB_BOOT_ZON:
      CREATE(zone_table, struct zone_data, rec_count);
      break;
    case DB_BOOT_QST:
      CREATE(mob_quests, struct quest_data, rec_count + 1);
      break;
  }

  rewind(index);
  fscanf(index, "%s\n", buf1);
  while (*buf1 != '$') {
    snprintf(buf2, MAX_STRING_LENGTH, "%s/%s", prefix, buf1);
    if (!(db_file = fopen(buf2, "r"))) {
      perror(buf2);
      fflush(NULL);
      exit(1);
    }
    switch (mode) {
      case DB_BOOT_WLD:
      case DB_BOOT_OBJ:
      case DB_BOOT_MOB:
      case DB_BOOT_QST:
        discrete_load(db_file, mode);
        break;
      case DB_BOOT_ZON:
        load_zones(db_file, buf2);
        break;
      case DB_BOOT_SHP:
        boot_the_shops(db_file, buf2, rec_count);
        break;
    }

    fclose(db_file);
    fscanf(index, "%s\n", buf1);
  }

  new_top_objt = top_of_objt;
  new_top_mobt = top_of_mobt;
  fclose(index);

}

void discrete_load(FILE * fl, int mode)
{
  int nr = -1, last = 0;
  char line[256];

  char *modes[] = {"world", "mob", "obj", "zone", "shop", "quest"};

  for (;;) {
    /*
     * we have to do special processing with the obj files because they have
     * no end-of-record marker :(
     */
    if (mode != DB_BOOT_OBJ || nr < 0)
      if (!get_line(fl, line)) {
        return;
        /* fprintf(stderr, "Format error after %s #%d\n", modes[mode], nr);
         exit(1); */
      }
    if (*line == '$')
      return;

    if (*line == '#') {
      last = nr;
      if (sscanf(line, "#%d", &nr) != 1) {
        fprintf(stderr, "Format error after %s #%d\n", modes[mode], last);
        fflush(NULL);
        exit(1);
      }
      if (nr >= 999999)
        return;
      else
        switch (mode) {
          case DB_BOOT_WLD:
            parse_room(fl, nr);
            break;
          case DB_BOOT_MOB:
            parse_mobile(fl, nr);
            break;
          case DB_BOOT_QST:
            parse_quest(fl, nr);
            break;
          case DB_BOOT_OBJ:
            strcpy(line, parse_object(fl, nr));
            break;
        }
    } else {
      /*      fprintf(stderr, "Format error in %s file near %s #%d\n",
       modes[mode], modes[mode], nr);
       fprintf(stderr, "Offending line: '%s'\n", line);
       exit(1); */
      return;
    }
  }
}

long asciiflag_conv(char *flag)
{
  long flags = 0;
  int is_number = 1;
  register char *p;

  for (p = flag; *p; p++) {
    if (islower(*p))
      flags |= 1 << (*p - 'a');
    else if (isupper(*p))
      flags |= 1 << (26 + (*p - 'A'));

    if (!isdigit(*p))
      is_number = 0;
  }

  if (is_number)
    flags = atol(flag);

  return flags;
}

/* load the rooms */
void parse_room(FILE * fl, int virtual_nr)
{
  static int room_nr = 0, zone = 0;
  int t[10], i;
  char line[256], flags[128];
  struct extra_descr_data *new_descr;

  snprintf(buf2, MAX_STRING_LENGTH, "room #%d", virtual_nr);

  if (virtual_nr <= (zone ? zone_table[zone - 1].top : -1)) {
    fprintf(stderr, "Room #%d is below zone %d.\n", virtual_nr, zone);
    fflush(NULL);
    exit(1);
  }
  if (zone_table[zone].bottom == -1)
    zone_table[zone].bottom = virtual_nr;
  while (virtual_nr > zone_table[zone].top)
    if (++zone > top_of_zone_table) {
      fprintf(stderr, "Room %d is outside of any zone.\n", virtual_nr);
      fflush(NULL);
      exit(1);
    }
  world[room_nr].zone = zone;
  world[room_nr].number = virtual_nr;
  world[room_nr].name = fread_string(fl, buf2);
  world[room_nr].description = fread_string(fl, buf2);

  if (!get_line(fl, line) || sscanf(line, " %d %s %d ", t, flags, t + 2) != 3) {
    fprintf(stderr, "Format error in room #%d\n", virtual_nr);
    fflush(NULL);
    exit(1);
  }
  /* t[0] is the zone number; ignored with the zone-file system */
  world[room_nr].room_flags = asciiflag_conv(flags);
  world[room_nr].sector_type = t[2];

  world[room_nr].func = NULL;
  world[room_nr].contents = NULL;
  world[room_nr].people = NULL;
  world[room_nr].light = 0; /* Zero light sources */

  for (i = 0; i < NUM_OF_DIRS; i++)
    world[room_nr].dir_option[i] = NULL;

  world[room_nr].ex_description = NULL;

  snprintf(buf, MAX_STRING_LENGTH, "Format error in room #%d (expecting D/E/S)", virtual_nr);

  for (;;) {
    if (!get_line(fl, line)) {
      fprintf(stderr, "%s\n", buf);
      fflush(NULL);
      exit(1);
    }
    switch (*line) {
      case 'D':
        setup_dir(fl, room_nr, atoi(line + 1));
        break;
      case 'E':
        CREATE(new_descr, struct extra_descr_data, 1);
        new_descr->keyword = fread_string(fl, buf2);
        new_descr->description = fread_string(fl, buf2);
        new_descr->next = world[room_nr].ex_description;
        world[room_nr].ex_description = new_descr;
        break;
      case 'S': /* end of room */
        top_of_world = room_nr++;
        return;
      case '#':
        top_of_world = room_nr++;
        unget_line(fl);
        return;
      default:
        fprintf(stderr, "%s\n", buf);
        fflush(NULL);
        exit(1);
        break;
    }
  }
}

/* read direction data */
void setup_dir(FILE * fl, int room, int dir)
{
  int ctr;
  int t[5];
  char line[256];

  snprintf(buf2, MAX_STRING_LENGTH, "room #%d, direction D%d", world[room].number, dir);

  CREATE(world[room].dir_option[dir], struct room_direction_data, 1);
  world[room].dir_option[dir]->general_description = fread_string(fl, buf2);
  world[room].dir_option[dir]->keyword = fread_string(fl, buf2);

  if (!get_line(fl, line)) {
    fprintf(stderr, "Format error, %s\n", buf2);
    fflush(NULL);
    exit(1);
  }

  for (ctr = 0; ctr < 5; ctr++)
    t[ctr] = 0;

  if (sscanf(line, " %d %d %d %d", t, t + 1, t + 2, t + 3) < 3) {
    fprintf(stderr, "Format error, %s\n", buf2);
    fflush(NULL);
    exit(1);
  }
  if (t[0] == 1)
    world[room].dir_option[dir]->exit_info = EX_ISDOOR;
  else if (t[0] == 2)
    world[room].dir_option[dir]->exit_info = EX_ISDOOR | EX_PICKPROOF;
  else
    world[room].dir_option[dir]->exit_info = 0;

  world[room].dir_option[dir]->key = t[1];
  world[room].dir_option[dir]->to_room = t[2];
}

/* make sure the start rooms exist & resolve their vnums to rnums */
void check_start_rooms(void)
{
  extern sh_int mortal_start_room;
  extern sh_int immort_start_room;
  extern sh_int frozen_start_room;

  if ((r_mortal_start_room = real_room(mortal_start_room)) < 0) {
    stderr_log("SYSERR:  Mortal start room does not exist.  Change in config.c.");
    fflush(NULL);
    exit(1);
  }
  if ((r_immort_start_room = real_room(immort_start_room)) < 0) {
    if (!mini_mud)
      stderr_log("SYSERR:  Warning: Immort start room does not exist.  Change in config.c.");
    r_immort_start_room = r_mortal_start_room;
  }
  if ((r_frozen_start_room = real_room(frozen_start_room)) < 0) {
    if (!mini_mud)
      stderr_log("SYSERR:  Warning: Frozen start room does not exist.  Change in config.c.");
    r_frozen_start_room = r_mortal_start_room;
  }
}

/* resolve all vnums into rnums in the world */
void renum_world(void)
{
  register int room, door;

  /* before renumbering the exits, copy them to to_room_vnum */
  for (room = 0; room <= top_of_world; room++)
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (world[room].dir_option[door]) {
        /* copy */
        world[room].dir_option[door]->to_room_vnum = world[room].dir_option[door]->to_room;
        if (world[room].dir_option[door]->to_room != NOWHERE)
          world[room].dir_option[door]->to_room = real_room(world[room].dir_option[door]->to_room);
      }
}

#define ZCMD zone_table[zone].cmd[cmd_no]

/* resulve vnums into rnums in the zone reset tables */
void renum_zone_table(void)
{
  int zone, cmd_no, a, b;

  for (zone = 0; zone <= top_of_zone_table; zone++) {
    for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {
      a = b = 0;
      switch (ZCMD.command) {
        case 'M':
          a = ZCMD.arg1 = real_mobile(ZCMD.arg1);
          b = ZCMD.arg3 = real_room(ZCMD.arg3);
          break;
        case 'O':
          a = ZCMD.arg1 = real_object(ZCMD.arg1);
          if (ZCMD.arg3 != NOWHERE)
            b = ZCMD.arg3 = real_room(ZCMD.arg3);
          break;
        case 'G':
          a = ZCMD.arg1 = real_object(ZCMD.arg1);
          break;
        case 'E':
          a = ZCMD.arg1 = real_object(ZCMD.arg1);
          break;
        case 'P':
          a = ZCMD.arg1 = real_object(ZCMD.arg1);
          b = ZCMD.arg3 = real_object(ZCMD.arg3);
          break;
        case 'R': /* rem obj from room */
          a = ZCMD.arg1 = real_room(ZCMD.arg1);
          b = ZCMD.arg2 = real_object(ZCMD.arg2);
          break;
        case 'D':
          a = ZCMD.arg1 = real_room(ZCMD.arg1);
          break;
      }

      if (a < 0 || b < 0) {
        if (!mini_mud)
          log_zone_error(zone, cmd_no, "Invalid vnum, cmd disabled");
#if 0
        fprintf(stderr, "Invalid vnum in zone reset cmd: zone #%d, cmd %d .. command disabled.\n",
            zone_table[zone].number, cmd_no + 1);
#endif
        ZCMD.command = '*';
      }
    }
  }
}

char fread_letter(FILE *fp)
{
  char c;
  do {
    c = getc(fp);
  } while (isspace(c));
  return c;
}

void parse_simple_mob(FILE *mob_f, int i, int nr)
{
  int t[10], j;
  char line[256];
  struct mob_attacks_data *new_attack;

  mob_proto[i].real_abils.str = 50;
  mob_proto[i].real_abils.intel = 50;
  mob_proto[i].real_abils.wis = 50;
  mob_proto[i].real_abils.dex = 50;
  mob_proto[i].real_abils.con = 50;
  mob_proto[i].real_abils.agi = 50;

  get_line(mob_f, line);
  if (sscanf(line, " %d %d %d %dd%d+%d %dd%d+%d ", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8) != 9) {
    fprintf(stderr, "Format error in mob #%d, first line after S flag\n"
        "...expecting line of form '# # # #d#+# #d#+#'\n", nr);
    fflush(NULL);
    exit(1);
  }
  GET_LEVEL(mob_proto + i) = t[0];
  mob_proto[i].points.hitroll = 20 - t[1];
  mob_proto[i].points.armor = 10 * t[2];
  if (mob_proto[i].points.armor < -100)
    mob_proto[i].points.armor = -100;
  if (mob_proto[i].points.armor > 100)
    mob_proto[i].points.armor = 100;

  /* max hit = 0 is a flag that H, M, V is xdy+z */
  mob_proto[i].points.max_hit = 0;
  mob_proto[i].points.hit = t[3];
  mob_proto[i].points.mana = t[4];
  mob_proto[i].points.move = t[5];

  mob_proto[i].points.max_mana = 10;
  mob_proto[i].points.max_move = 50;

  CREATE(new_attack, struct mob_attacks_data, 1);
  /* new combat system */
  new_attack->next = mob_proto[i].mob_specials.mob_attacks;
  mob_proto[i].mob_specials.mob_attacks = new_attack;

  new_attack->nodice = t[6];
  new_attack->sizedice = t[7];
  new_attack->damroll = t[8];

  get_line(mob_f, line);
  sscanf(line, " %d %d ", t, t + 1);
  GET_GOLD(mob_proto + i) = t[0];
  GET_EXP(mob_proto + i) = t[1];

  get_line(mob_f, line);
  if (sscanf(line, " %d %d %d %d ", t, t + 1, t + 2, t + 3) == 4)
    new_attack->attack_type = t[3];
  else
    new_attack->attack_type = 0;

  new_attack->attacks = 100;

  mob_proto[i].char_specials.position = t[0];
  mob_proto[i].mob_specials.default_pos = t[1];
  mob_proto[i].player.sex = t[2];

  mob_proto[i].player.class = 0;
  mob_proto[i].player.weight = 200;
  mob_proto[i].player.height = 198;

  for (j = 0; j < 3; j++)
    GET_COND(mob_proto + i, j) = -1;

  /*
   * these are now save applies; base save numbers for MOBs are now from
   * the warrior save table.
   */
  for (j = 0; j < 5; j++)
    GET_SAVE(mob_proto + i, j) = 0;

}

void parse_another_mob(FILE * mob_f, int i, int nr)
{
  int j, t[10], tmp;
  char line[256], letter;
  struct mob_attacks_data *new_attack;

  mob_proto[i].real_abils.str = 50;
  mob_proto[i].real_abils.intel = 50;
  mob_proto[i].real_abils.wis = 50;
  mob_proto[i].real_abils.dex = 50;
  mob_proto[i].real_abils.con = 50;
  mob_proto[i].real_abils.agi = 50;

  get_line(mob_f, line);
  if (sscanf(line, " %d %d %d %dd%d+%d %dd%d+%d ", t, t + 1, t + 2, t + 3, t + 4, t + 5, t + 6, t + 7, t + 8) != 9) {
    fprintf(stderr, "Format error in mob #%d, first line after A flag\n"
        "...expecting line of form '# # # #d#+# #d#+#'\n", nr);
    fflush(NULL);
    exit(1);
  }
  GET_LEVEL(mob_proto + i) = t[0];
  mob_proto[i].points.hitroll = 20 - t[1];
  mob_proto[i].points.armor = 10 * t[2];
  if (mob_proto[i].points.armor < -100)
    mob_proto[i].points.armor = -100;
  if (mob_proto[i].points.armor > 100)
    mob_proto[i].points.armor = 100;

  /* max hit = 0 is a flag that H, M, V is xdy+z */
  mob_proto[i].points.max_hit = 0;
  mob_proto[i].points.hit = t[3];
  mob_proto[i].points.mana = t[4];
  mob_proto[i].points.move = t[5];

  mob_proto[i].points.max_mana = 10;
  mob_proto[i].points.max_move = 50;

  CREATE(new_attack, struct mob_attacks_data, 1);
  /* new combat system */
  new_attack->next = mob_proto[i].mob_specials.mob_attacks;
  mob_proto[i].mob_specials.mob_attacks = new_attack;

  new_attack->nodice = t[6];
  new_attack->sizedice = t[7];
  new_attack->damroll = t[8];

  get_line(mob_f, line);
  sscanf(line, " %d %d ", t, t + 1);
  GET_GOLD(mob_proto + i) = t[0];
  GET_EXP(mob_proto + i) = t[1];

  get_line(mob_f, line);
  if (sscanf(line, " %d %d %d %d ", t, t + 1, t + 2, t + 3) == 4)
    new_attack->attack_type = t[3];
  else
    new_attack->attack_type = 0;

  mob_proto[i].char_specials.position = t[0];
  mob_proto[i].mob_specials.default_pos = t[1];
  mob_proto[i].player.sex = t[2];

  get_line(mob_f, line); /* new for A format */
  tmp = sscanf(line, " %d %d %d %d %c", t, t + 1, t + 2, t + 3, &letter);

  mob_proto[i].player.class = t[0];
  mob_proto[i].mob_specials.race = t[1];
  if (t[3] < 9)
    new_attack->attacks = 100 / (t[3] + 1);
  else
    new_attack->attacks = t[3] + 1;

  mob_proto[i].mob_specials.size = t[2];

  if (tmp == 5)
    tmp++;

  while (tmp == 6) {
    get_line(mob_f, line); /* read a line */
    tmp = sscanf(line, " %d %dd%d+%d %d %c ", t, t + 1, t + 2, t + 3, t + 4, &letter);

    CREATE(new_attack, struct mob_attacks_data, 1);
    if (t[0] == 0)
      stderr_log("CRITICAL ERROR - 0 attacks");
    if (t[0] < 10)
      new_attack->attacks = 100 / t[0];
    else
      new_attack->attacks = t[0];
    new_attack->nodice = t[1];
    new_attack->sizedice = t[2];
    new_attack->damroll = t[3];
    new_attack->attack_type = t[4];
    new_attack->next = mob_proto[i].mob_specials.mob_attacks;
    mob_proto[i].mob_specials.mob_attacks = new_attack;

  };

  mob_proto[i].player.weight = 200;
  mob_proto[i].player.height = 198;

  for (j = 0; j < 3; j++)
    GET_COND(mob_proto + i, j) = -1;

  /*
   * these are now save applies; base save numbers for MOBs are now from
   * the warrior save table.
   */
  for (j = 0; j < 5; j++)
    GET_SAVE(mob_proto + i, j) = 0;

}

/*
 * interpret_espec is the function that takes espec keywords and values
 * and assigns the correct value to the mob as appropriate.  Adding new
 * e-specs is absurdly easy -- just add a new CASE statement to this
 * function!  No other changes need to be made anywhere in the code.
 */

#define CASE(test) if (!matched && !str_cmp(keyword, test) && (matched = 1))
#define RANGE(low, high) (num_arg = BOUNDED((low), (num_arg), (high)))

void interpret_espec(char *keyword, char *value, int i, int nr)
{
  int num_arg, matched = 0;

  num_arg = atoi(value);

  CASE("BareHandAttack") {
    RANGE(0, 99);
    mob_proto[i].mob_specials.mob_attacks->attack_type = num_arg;
  }

  CASE("Str") {
    RANGE(1, 100);
    mob_proto[i].real_abils.str = num_arg;
  }

  CASE("Int") {
    RANGE(1, 100);
    mob_proto[i].real_abils.intel = num_arg;
  }

  CASE("Wis") {
    RANGE(1, 100);
    mob_proto[i].real_abils.wis = num_arg;
  }

  CASE("Dex") {
    RANGE(1, 100);
    mob_proto[i].real_abils.dex = num_arg;
  }

  CASE("Con") {
    RANGE(1, 100);
    mob_proto[i].real_abils.con = num_arg;
  }

  CASE("Agi") {
    RANGE(1, 100);
    mob_proto[i].real_abils.agi = num_arg;
  }

  CASE("Gold") {
    RANGE(0, 10000000);
    GET_TEMP_GOLD(mob_proto + i) = num_arg;

  }

  CASE("Exp") {
    RANGE(0, 100000000);
    GET_EXP(mob_proto + i) = num_arg;
  }

  CASE("ResLight") {
    RANGE(-1000, 1000);
    GET_RESIST((mob_proto + i), DAM_LIGHT) = num_arg;
  }

  CASE("ResDark") {
    RANGE(-1000, 1000);
    GET_RESIST((mob_proto + i), DAM_DARK) = num_arg;
  }

  CASE("ResFire") {
    RANGE(-1000, 1000);
    GET_RESIST((mob_proto + i), DAM_FIRE) = num_arg;
  }

  CASE("ResCold") {
    RANGE(-1000, 1000);
    GET_RESIST((mob_proto + i), DAM_COLD) = num_arg;
  }

  CASE("ResAcid") {
    RANGE(-1000, 1000);
    GET_RESIST((mob_proto + i), DAM_ACID) = num_arg;
  }

  CASE("ResPoison") {
    RANGE(-1000, 1000);
    GET_RESIST((mob_proto + i), DAM_POISON) = num_arg;
  }

  CASE("ResDisease") {
    RANGE(-1000, 1000);
    GET_RESIST((mob_proto + i), DAM_DISEASE) = num_arg;
  }

  CASE("ResCharm") {
    RANGE(-1000, 1000);
    GET_RESIST((mob_proto + i), DAM_CHARM) = num_arg;
  }

  CASE("ResSleep") {
    RANGE(-1000, 1000);
    GET_RESIST((mob_proto + i), DAM_SLEEP) = num_arg;
  }

  CASE("ResSlash") {
    RANGE(-1000, 1000);
    GET_RESIST((mob_proto + i), DAM_SLASH) = num_arg;
  }

  CASE("ResPierce") {
    RANGE(-1000, 1000);
    GET_RESIST((mob_proto + i), DAM_PIERCE) = num_arg;
  }

  CASE("ResBludgeon") {
    RANGE(-1000, 1000);
    GET_RESIST((mob_proto + i), DAM_BLUDGEON) = num_arg;
  }

  CASE("ResNWeap") {
    RANGE(-1000, 1000);
    GET_RESIST((mob_proto + i), DAM_NWEAP) = num_arg;
  }

  CASE("ResMWeap") {
    RANGE(-1000, 1000);
    GET_RESIST((mob_proto + i), DAM_MWEAP) = num_arg;
  }

  CASE("ResMagic") {
    RANGE(-1000, 1000);
    GET_RESIST((mob_proto + i), DAM_MAGIC) = num_arg;
  }

  CASE("ResElectricity") {
    RANGE(-1000, 1000);
    GET_RESIST((mob_proto + i), DAM_ELECTRICITY) = num_arg;
  }

  if (!matched) {
    fprintf(stderr, "Warning: unrecognized espec keyword %s in mob #%d\n", keyword, nr);
  }
}

#undef CASE
#undef RANGE

void parse_espec(char *buf, int i, int nr)
{
  char *ptr;

  if ((ptr = strchr(buf, ':')) != NULL) {
    *(ptr++) = '\0';
    while (isspace(*ptr))
      ptr++;
  } else
    ptr = "";

  interpret_espec(buf, ptr, i, nr);
}

void parse_enhanced_mob(FILE *mob_f, int i, int nr)
{
  char line[256];

  parse_simple_mob(mob_f, i, nr);

  while (get_line(mob_f, line)) {
    if (!strcmp(line, "E")) /* end of the ehanced section */
      return;
    else if (*line == '#') { /* we've hit the next mob, maybe? */
      fprintf(stderr, "Unterminated E section in mob #%d\n", nr);
      fflush(NULL);
      exit(1);
    } else
      parse_espec(line, i, nr);
  }

  fprintf(stderr, "Unexpected end of file reached after mob #%d\n", nr);
  fflush(NULL);
  exit(1);
}

void parse_extensions(FILE *mob_f, int i, int nr)
{
  struct mob_attacks_data *new_attack;
  struct mob_equipment_data *new_equip;
  struct mob_action_data *new_action;
  char line[256];
  char *scanner;
  int t[10], cnt;

  while (get_line(mob_f, line)) {
    if (*line == '#' || *line == '>') { /* end of extension fields */
      unget_line(mob_f); /* unget the line.. arhem? */
      return;
    }
    if (line[0] == 'T' && line[1] == ' ') { /* parse attacks field */
      if (sscanf(line, "T %d %dd%d+%d %d", t, t + 1, t + 2, t + 3, t + 4) != 5) {
        fprintf(stderr, "Incorrect T field in mob #%d\n", nr);
        fflush(NULL);
        exit(1);
      }
      CREATE(new_attack, struct mob_attacks_data, 1);
      new_attack->next = mob_proto[i].mob_specials.mob_attacks;
      mob_proto[i].mob_specials.mob_attacks = new_attack;
      new_attack->nodice = t[1];
      new_attack->sizedice = t[2];
      new_attack->damroll = t[3];
      new_attack->attack_type = t[4];
      new_attack->attacks = t[0];
    } else if (line[0] == 'E' && line[1] == ' ') { /* parse equipment field */
      if (sscanf(line, "E %d %d %d %d", t, t + 1, t + 2, t + 3) != 4) {
        fprintf(stderr, "Incorrect E field in mob #%d\n", nr);
        fflush(NULL);
        exit(1);
      }
      CREATE(new_equip, struct mob_equipment_data, 1);
      new_equip->next = mob_proto[i].mob_specials.mob_equip;
      mob_proto[i].mob_specials.mob_equip = new_equip;
      new_equip->pos = t[0];
      new_equip->chance = t[1];
      new_equip->vnum = t[2];
      new_equip->rnum = real_object(t[2]);
      new_equip->max = t[3];
    } else if (line[0] == 'A' && line[1] == ' ') { /* parse action field */
      if (sscanf(line, "A %d %d ", t, t + 1) != 2) {
        fprintf(stderr, "Incorrect A field in mob #%d\n", nr);
        fflush(NULL);
        exit(1);
      }
      CREATE(new_action, struct mob_action_data, 1);
      new_action->next = mob_proto[i].mob_specials.mob_action;
      mob_proto[i].mob_specials.mob_action = new_action;
      new_action->chance = t[0];
      new_action->minpos = t[1];
      scanner = line;
      cnt = 0;
      while (cnt < 3) {
        if (*scanner == ' ')
          cnt++;
        scanner++;
      }
      new_action->action = strdup(scanner);
    } else if (line[0] == 'S' && line[1] == ' ') { /* spec proc fields */
      if (sscanf(line, "S %d %d %d", t, t + 1, t + 2) != 3) {
        fprintf(stderr, "Incorrect S field in mob #%d\n", nr);
        fflush(NULL);
        exit(1);
      }
      GET_MOB_SVAL(mob_proto + i, 0) = t[0];
      GET_MOB_SVAL(mob_proto + i, 1) = t[1];
      GET_MOB_SVAL(mob_proto + i, 2) = t[2];

    } else
      /* well.. assume it's an E-Spec */
      parse_espec(line, i, nr);
  }
  return; /* end of file */

}

void parse_balanced_mob(FILE *mob_f, int i, int nr)
{
  int t[10], j;
  char line[256];
  struct mob_attacks_data *new_attack;

  mob_proto[i].real_abils.str = 50;
  mob_proto[i].real_abils.intel = 50;
  mob_proto[i].real_abils.wis = 50;
  mob_proto[i].real_abils.dex = 50;
  mob_proto[i].real_abils.con = 50;
  mob_proto[i].real_abils.agi = 50;

  get_line(mob_f, line);
  if (sscanf(line, " %d %d %d %d ", t, t + 1, t + 2, t + 3) != 4) {
    fprintf(stderr, "Format error in mob #%d, first line after B flag\n"
        "...expecting line of form '# # # #'\n", nr);
    fflush(NULL);
    exit(1);
  }
  GET_LEVEL(mob_proto + i) = t[0];
  mob_proto[i].char_specials.position = t[1];
  mob_proto[i].mob_specials.default_pos = t[2];
  mob_proto[i].player.sex = t[3];
  j = t[0];

  mob_proto[i].points.hitroll = 20 - balance_table[j].thaco;
  mob_proto[i].points.armor = 10 * balance_table[j].ac;
  if (mob_proto[i].points.armor < -100)
    mob_proto[i].points.armor = -100;
  if (mob_proto[i].points.armor > 100)
    mob_proto[i].points.armor = 100;

  mob_proto[i].points.max_hit = 0;
  mob_proto[i].points.hit = balance_table[j].hit_dicenum;
  mob_proto[i].points.mana = balance_table[j].hit_dicesize;
  mob_proto[i].points.move = balance_table[j].hit_add;
  mob_proto[i].points.max_mana = 10;
  mob_proto[i].points.max_move = 50;

  GET_TEMP_GOLD(mob_proto + i) = balance_table[j].gold;
  GET_EXP(mob_proto + i) = balance_table[j].exp;

  mob_proto[i].player.class = 0;
  mob_proto[i].player.weight = 200;
  mob_proto[i].player.height = 198;

  parse_extensions(mob_f, i, nr); /* parse extra fields */

  if (mob_proto[i].mob_specials.mob_attacks == NULL) { /* no attack fields was found, add the default */
    CREATE(new_attack, struct mob_attacks_data, 1);
    /* new combat system */
    new_attack->next = mob_proto[i].mob_specials.mob_attacks;
    mob_proto[i].mob_specials.mob_attacks = new_attack;
    new_attack->nodice = balance_table[j].dam_dicenum;
    new_attack->sizedice = balance_table[j].dam_dicesize;
    new_attack->damroll = balance_table[j].dam_add;
    new_attack->attack_type = 0;
    new_attack->attacks = 100;
  }

  for (j = 0; j < 3; j++)
    GET_COND(mob_proto + i, j) = -1;

  for (j = 0; j < 5; j++)
    GET_SAVE(mob_proto + i, j) = 0;

}

void parse_extended_mob(FILE *mob_f, int i, int nr)
{
  int t[10], j;
  char line[256];
  struct mob_attacks_data *new_attack;

  mob_proto[i].real_abils.str = 50;
  mob_proto[i].real_abils.intel = 50;
  mob_proto[i].real_abils.wis = 50;
  mob_proto[i].real_abils.dex = 50;
  mob_proto[i].real_abils.con = 50;
  mob_proto[i].real_abils.agi = 50;

  get_line(mob_f, line);
  if (sscanf(line, " %d %d %d %dd%d+%d", t, t + 1, t + 2, t + 3, t + 4, t + 5) != 6) {
    fprintf(stderr, "Format error in mob #%d, first line after X flag\n"
        "...expecting line of form '# # # #d#+#'\n", nr);
    fflush(NULL);
    exit(1);
  }
  GET_LEVEL(mob_proto + i) = t[0];
  mob_proto[i].points.hitroll = t[1];
  mob_proto[i].points.armor = 10 * t[2];
  if (mob_proto[i].points.armor < -100) {
    mob_proto[i].points.armor = -100;
  }
  if (mob_proto[i].points.armor > 100) {
    mob_proto[i].points.armor = 100;
  }

  /* max hit = 0 is a flag that H, M, V is xdy+z */
  mob_proto[i].points.max_hit = 0;
  mob_proto[i].points.hit = t[3];
  mob_proto[i].points.mana = t[4];
  mob_proto[i].points.move = t[5];

  mob_proto[i].points.max_mana = 10;
  mob_proto[i].points.max_move = 50;

  get_line(mob_f, line);
  j = sscanf(line, " %d %d %d ", t, t + 1, t + 2);
  GET_TEMP_GOLD(mob_proto + i) = t[0];
  GET_EXP(mob_proto + i) = t[1];
  if (j == 3) {
    GET_MOB_QUEST_NUM(mob_proto + i) = t[2];
  } else {
    GET_MOB_QUEST_NUM(mob_proto + i) = -1;
  }

  get_line(mob_f, line);
  if (sscanf(line, " %d %d %d %d %d %d ", t, t + 1, t + 2, t + 3, t + 4, t + 5) != 6) {
    fprintf(stderr, "Format error in mob #%d, third line after X flag\n"
        "...expecting line of form '# # # # # #'\n", nr);
  }

  mob_proto[i].char_specials.position = t[0];
  mob_proto[i].mob_specials.default_pos = t[1];
  mob_proto[i].player.sex = t[2];
  mob_proto[i].player.class = t[3];
  mob_proto[i].mob_specials.race = t[4];
  mob_proto[i].mob_specials.size = t[5];

  mob_proto[i].player.weight = 200;
  mob_proto[i].player.height = 198;

  for (j = 0; j < 3; j++) {
    GET_COND(mob_proto + i, j) = -1;
  }

  /*
   * these are now save applies; base save numbers for MOBs are now from
   * the warrior save table.
   */
  for (j = 0; j < 5; j++) {
    GET_SAVE(mob_proto + i, j) = 0;
  }

  parse_extensions(mob_f, i, nr); /* parse extra fields */

  if (mob_proto[i].mob_specials.mob_attacks == NULL) { /* no attack fields was found, add the default */
    CREATE(new_attack, struct mob_attacks_data, 1);
    /* new combat system */
    new_attack->next = mob_proto[i].mob_specials.mob_attacks;
    mob_proto[i].mob_specials.mob_attacks = new_attack;
    new_attack->nodice = balance_table[(int) GET_LEVEL(mob_proto + i)].dam_dicenum;
    new_attack->sizedice = balance_table[(int) GET_LEVEL(mob_proto + i)].dam_dicesize;
    new_attack->damroll = balance_table[(int) GET_LEVEL(mob_proto + i)].dam_add;
    new_attack->attack_type = 0;
    new_attack->attacks = 100;
  }

}

void parse_mobile(FILE * mob_f, int nr)
{
  static int i = 0;
  int g, j, t[10];
  char line[256], *tmpptr, letter;
  char f1[128], f2[128], f3[128];

  mob_index[i].virtual = nr;
  mob_index[i].number = 0;
  mob_index[i].func = NULL;

  clear_char(mob_proto + i);

  (mob_proto + i)->player_specials = &dummy_mob;

  snprintf(buf2, MAX_STRING_LENGTH, "mob vnum %d", nr);

  /***** String data *** */
  mob_proto[i].player.name = fread_string(mob_f, buf2);
  tmpptr = mob_proto[i].player.short_descr = fread_string(mob_f, buf2);
  if (tmpptr && *tmpptr)
    if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") || !str_cmp(fname(tmpptr), "the"))
      *tmpptr = LOWER(*tmpptr);
  mob_proto[i].player.long_descr = fread_string(mob_f, buf2);
  mob_proto[i].player.description = fread_string(mob_f, buf2);
  mob_proto[i].player.title = NULL;

  /* *** Numeric data *** */
  get_line(mob_f, line);
  g = sscanf(line, "%s %s %d %c %s", f1, f2, t + 2, &letter, f3);
  MOB_FLAGS(mob_proto + i) = 0;
  MOB_FLAGS(mob_proto + i) = asciiflag_conv(f1);
  SET_BIT(MOB_FLAGS(mob_proto + i), MOB_ISNPC);
  AFF_FLAGS(mob_proto + i) = 0;
  AFF_FLAGS(mob_proto + i) = asciiflag_conv(f2);
  if (g == 5) {
    AFF2_FLAGS(mob_proto + i) = 0;
    AFF2_FLAGS(mob_proto + i) = asciiflag_conv(f3);
  }
  GET_ALIGNMENT(mob_proto + i) = t[2];
  GET_MOB_INVIS_LEV(mob_proto + i) = 0;
  if (IS_SET(AFF_FLAGS(mob_proto + i), AFF_SUPERINV))
    GET_MOB_INVIS_LEV(mob_proto + i) = 51;

  switch (letter) {
    case 'S': /* Simple monsters */
      parse_simple_mob(mob_f, i, nr);
      break;
    case 'A': /* ShadowWind monsters */
      parse_another_mob(mob_f, i, nr);
      break;
    case 'E': /* CircleMUD 3.00 E-Spec monsters */
      parse_enhanced_mob(mob_f, i, nr);
      break;
    case 'B': /* Balanced ShadowWind extended monsters */
      parse_balanced_mob(mob_f, i, nr);
      break;
    case 'X': /* Standard ShadowWind extended monsters */
      parse_extended_mob(mob_f, i, nr);
      break;
    default:
      fprintf(stderr, "Unsupported mob type %c in mob #%d\n", letter, nr);
      fflush(NULL);
      exit(1);
      break;
  }

  mob_proto[i].aff_abils = mob_proto[i].real_abils;

  for (j = 0; j < NUM_WEARS; j++)
    mob_proto[i].equipment[j] = NULL;

  mob_proto[i].nr = i;
  mob_proto[i].desc = NULL;

  letter = fread_letter(mob_f);
  if (letter == '>') {
    ungetc(letter, mob_f);
    (void) mprog_read_programs(mob_f, &mob_index[i]);
  } else
    ungetc(letter, mob_f);

  top_of_mobt = i++;
}

/* read all objects from obj file; generate index and prototypes */
char *parse_object(FILE * obj_f, int nr)
{
  static int i = 0;
  int spellnum = 1;
  static char line[256];
  int t[10], j, temp, rtype, ramt;
  char *tmpptr;
  char f1[128], f2[128], f3[128];
  char abuff[320];
  struct extra_descr_data *new_descr;
  int aspell = 0;

  memset(f1, 0, (sizeof(char) * 128));
  memset(f2, 0, (sizeof(char) * 128));
  memset(f3, 0, (sizeof(char) * 128));

  obj_index[i].virtual = nr;
  obj_index[i].number = 0;
  obj_index[i].func = NULL;

  clear_object(obj_proto + i);
  obj_proto[i].in_room = NOWHERE;
  obj_proto[i].item_number = i;
  obj_proto[i].cname = NULL;
  obj_proto[i].cshort_description = NULL;
  obj_proto[i].cdescription = NULL;

  /* *** string data *** */
  if ((obj_proto[i].name = fread_string(obj_f, buf2)) == NULL) {
    fprintf(stderr, "Null obj name or format error at or near %s\n", buf2);
    fflush(NULL);
    exit(1);
  }
  tmpptr = obj_proto[i].short_description = fread_string(obj_f, buf2);
  if (*tmpptr)
    if (!str_cmp(fname(tmpptr), "a") || !str_cmp(fname(tmpptr), "an") || !str_cmp(fname(tmpptr), "the"))
      *tmpptr = LOWER(*tmpptr);

  tmpptr = obj_proto[i].description = fread_string(obj_f, buf2);
  if (tmpptr && *tmpptr)
    *tmpptr = UPPER(*tmpptr);
  obj_proto[i].action_description = fread_string(obj_f, buf2);

  /* *** numeric data *** */
  if (!get_line(obj_f, line) || (temp = sscanf(line, " %d %s %s %s", t, f1, f2, f3)) < 3) {
    fprintf(stderr, "Format error in first numeric line, %s, obj#%d\r\n", buf2, nr);
    fflush(NULL);
    exit(1);
  }
  obj_proto[i].obj_flags.type_flag = t[0];
  obj_proto[i].obj_flags.extra_flags = asciiflag_conv(f1);
  obj_proto[i].obj_flags.wear_flags = asciiflag_conv(f2);
  obj_proto[i].obj_flags.wear_slots = asciiflag_conv(f3);

  if (temp == 3) {
    GET_OBJ_SLOTS(obj_proto +i) = 0;
    temp = GET_OBJ_WEAR(obj_proto + i);
    REMOVE_BIT(temp, ITEM_WEAR_TAKE);
    switch (temp) {
      case ITEM_WEAR_BADGE:
        GET_OBJ_SLOTS(obj_proto + i) = WORN_BADGE;
        break;
      case ITEM_WEAR_BODY:
        GET_OBJ_SLOTS(obj_proto + i) = WORN_BODY;
        break;
      case ITEM_WEAR_HEAD:
        GET_OBJ_SLOTS(obj_proto + i) = WORN_HEAD;
        break;
      case ITEM_WEAR_LEGS:
        GET_OBJ_SLOTS(obj_proto + i) = WORN_LEGS;
        break;
      case ITEM_WEAR_FEET:
        GET_OBJ_SLOTS(obj_proto + i) = WORN_FEET;
        break;
      case ITEM_WEAR_HANDS:
        GET_OBJ_SLOTS(obj_proto + i) = WORN_HANDS;
        break;
      case ITEM_WEAR_ARMS:
        GET_OBJ_SLOTS(obj_proto + i) = WORN_ARMS;
        break;
      case ITEM_WEAR_SHIELD:
        GET_OBJ_SLOTS(obj_proto + i) = WORN_SHIELD;
        break;
      case ITEM_WEAR_ABOUT:
        GET_OBJ_SLOTS(obj_proto + i) = WORN_ABOUT;
        break;
      case ITEM_WEAR_FACE:
        GET_OBJ_SLOTS(obj_proto + i) = WORN_FACE;
        break;
      case ITEM_WEAR_EYES:
        GET_OBJ_SLOTS(obj_proto + i) = WORN_EYES;
        break;
      case ITEM_WEAR_2HANDED:
        GET_OBJ_SLOTS(obj_proto + i) = WORN_2HANDED | WORN_WIELD | WORN_WIELD_2 | WORN_HOLD | WORN_HOLD_2 | WORN_SHIELD;
        break;
    }
  }
  if (!get_line(obj_f, line)) {
    fprintf(stderr, "Format error in second numeric line, %s\n", buf2);
    fflush(NULL);
    exit(1);
  }

  temp = sscanf(line, "%d %d %d %d %d", t, t + 1, t + 2, t + 3, t + 4);
  if (temp == 4) {
    obj_proto[i].obj_flags.value[0] = t[0];
    obj_proto[i].obj_flags.value[1] = t[1];
    obj_proto[i].obj_flags.value[2] = t[2];
    obj_proto[i].obj_flags.value[3] = t[3];
  } else if (temp == 5) {
    obj_proto[i].obj_flags.value[0] = t[0];
    obj_proto[i].obj_flags.value[1] = t[1];
    obj_proto[i].obj_flags.value[2] = t[2];
    obj_proto[i].obj_flags.value[3] = t[3];
    obj_proto[i].obj_flags.value[4] = t[4];
  } else {
    fprintf(stderr, "Format error in second numeric line, %s\n", buf2);
    fflush(NULL);
    exit(1);
  }

  if (!get_line(obj_f, line) || sscanf(line, "%d %d %d", t, t + 1, t + 2) < 2) {
    fprintf(stderr, "Format error in third numeric line, %s\n", buf2);
    fflush(NULL);
    exit(1);
  }
  obj_proto[i].obj_flags.weight = t[0];
  obj_proto[i].obj_flags.cost = t[1];

  /* check to make sure that weight of containers exceeds curr. quantity */
  if (obj_proto[i].obj_flags.type_flag == ITEM_DRINKCON || obj_proto[i].obj_flags.type_flag == ITEM_FOUNTAIN) {
    if (obj_proto[i].obj_flags.weight < obj_proto[i].obj_flags.value[1])
      obj_proto[i].obj_flags.weight = obj_proto[i].obj_flags.value[1] + 5;
  }

  /* *** extra descriptions and affect fields *** */

  for (j = 0; j < MAX_OBJ_AFFECT; j++) {
    obj_proto[i].affected[j].location = APPLY_NONE;
    obj_proto[i].affected[j].modifier = 0;
  }

  for (j = 0; j < MAX_DAM_TYPE; j++) {
    obj_proto[i].resists[j] = 0;
  }

  strcat(buf2, ", after numeric constants (expecting E/A/#xxx)");
  j = 0;

  spellnum = 1;
  for (;;) {
    if (!get_line(obj_f, line)) {
      top_of_objt = i++;
      return line;
    }
    switch (*line) {
      case 'B':
        get_line(obj_f, line);
        sscanf(line, "%s", f1);
        obj_proto[i].obj_flags.bitvector = asciiflag_conv(f1); /*sets AFF flags*/
        break;
      case 'C':
        get_line(obj_f, line);
        sscanf(line, "%s", f1);
        obj_proto[i].obj_flags.bitvector2 = asciiflag_conv(f1); /*sets AFF2 flags*/
        break;
      case 'S':
        get_line(obj_f, line);
        sscanf(line, " %d %d %d ", t, t + 1, t + 2);
        obj_proto[i].spec_vars[0] = t[0];
        obj_proto[i].spec_vars[1] = t[1];
        obj_proto[i].spec_vars[2] = t[2];
        break;
      case 'E':
        CREATE(new_descr, struct extra_descr_data, 1);
        new_descr->keyword = fread_string(obj_f, buf2);
        new_descr->description = fread_string(obj_f, buf2);
        new_descr->next = obj_proto[i].ex_description;
        obj_proto[i].ex_description = new_descr;
        break;
      case 'A':
        if (j >= MAX_OBJ_AFFECT) {
          fprintf(stderr, "Too many A fields: %d max, %s\n", MAX_OBJ_AFFECT, buf2);
          fflush(NULL);
          exit(1);
        }
        get_line(obj_f, line);
        sscanf(line, " %d %d ", t, t + 1);
        obj_proto[i].affected[j].location = t[0];
        obj_proto[i].affected[j].modifier = t[1];
        j++;
        break;
      case 'R':
        get_line(obj_f, line);
        sscanf(line, " %d %d ", &rtype, &ramt);
        if (rtype <= 0 || rtype > MAX_DAM_TYPE) {
          fprintf(stderr, "Invalid resist field (%d max type), %s\n", MAX_DAM_TYPE, buf2);
          fflush(NULL);
          exit(1);
        }
        obj_proto[i].resists[rtype] = ramt;
        break;
      case 'P':
        get_line(obj_f, line);
        if (((GET_OBJ_TYPE(obj_proto + i) == ITEM_POTION) || (GET_OBJ_TYPE(obj_proto + i) == ITEM_SCROLL)) && spellnum > 3)
          break;
        if (((GET_OBJ_TYPE(obj_proto + i) == ITEM_WAND) || (GET_OBJ_TYPE(obj_proto + i) == ITEM_STAFF)) && spellnum > 1)
          break;
        if ((GET_OBJ_TYPE(obj_proto + i) == ITEM_POTION) || (GET_OBJ_TYPE(obj_proto + i) == ITEM_SCROLL)) {
          aspell = find_spell_num(line);
          if (aspell < 0) {
            snprintf(abuff, sizeof(abuff), "OBJ# %d: Invalid spell field (%s)", nr, line);
            stderr_log(abuff);
            GET_OBJ_VAL(obj_proto + i, spellnum) = 0;
          } else {
            GET_OBJ_VAL(obj_proto + i, spellnum) = spells[aspell].spellindex;
          }
        }
        if ((GET_OBJ_TYPE(obj_proto + i) == ITEM_WAND) || (GET_OBJ_TYPE(obj_proto + i) == ITEM_STAFF)) {
          aspell = find_spell_num(line);
          if (aspell < 0) {
            snprintf(abuff, sizeof(abuff), "OBJ# %d: Invalid spell field (%s)", nr, line);
            stderr_log(abuff);
            GET_OBJ_VAL(obj_proto + i, 3) = 0;
          } else {
            GET_OBJ_VAL(obj_proto + i, 3) = spells[aspell].spellindex;
          }
        }
        spellnum++;
        break;
      case '$':
      case '#':
        top_of_objt = i++;
        return line;
      default:
        fprintf(stderr, "Format error in %s\n", buf2);
        fflush(NULL);
        exit(1);
        break;
    }
  }
}

#define Z  zone_table[zone]

/* load the zone table and command tables */
void load_zones(FILE * fl, char *zonename)
{
  static int zone = 0;
  int cmd_no = 0, num_of_cmds = 0, line_num = 0, atmp, tmp, error, i;
  char *ptr, buf[256], zname[256], flags[128];

  strcpy(zname, zonename);

  while (get_line(fl, buf))
    num_of_cmds++; /* this should be correct within 3 or so */
  rewind(fl);

  if (num_of_cmds == 0) {
    fprintf(stderr, "%s is empty!\n", zname);
    fflush(NULL);
    exit(0);
  } else
    CREATE(Z.cmd, struct reset_com, num_of_cmds);

  Z.bottom = -1;
  line_num += get_line(fl, buf);

  if (sscanf(buf, "#%d", &Z.number) != 1) {
    fprintf(stderr, "Format error in %s, line %d\n", zname, line_num);
    fflush(NULL);
    exit(0);
  }

  line_num += get_line(fl, buf);
  if ((ptr = strchr(buf, '~')) != NULL) /* take off the '~' if it's there */
    *ptr = '\0';
  Z.name = strdup(buf);

  line_num += get_line(fl, buf);
  i = sscanf(buf, " %d %d %d %s ", &Z.top, &Z.lifespan, &Z.reset_mode, flags);
  if (i != 4 && i != 3) {
    fprintf(stderr, "Format error in 3/4 constant line - %s", zname);
    fflush(NULL);
    exit(0);
  }

  if (i == 3)
    flags[0] = '\0';

  Z.bits = asciiflag_conv(flags);
  line_num += get_line(fl, buf);
  i = sscanf(buf, " %d %s %d ", &(Z.climate.season_pattern), flags, &(Z.climate.energy_add));
  if (i != 3) {
    fprintf(stderr, "Format error in climate constant line - %s", zname);
    fflush(NULL);
    exit(0);
  }
  Z.climate.flags = asciiflag_conv(flags);
  line_num += get_line(fl, buf);
  i = sscanf(buf, " %d %d %d %d ", &(Z.climate.season_wind[0]), &(Z.climate.season_wind[1]), &(Z.climate.season_wind[2]), &(Z.climate.season_wind[3]));
  if (i != 4) {
    fprintf(stderr, "Format error in wind constant line - %s", zname);
    fflush(NULL);
    exit(0);
  }
  line_num += get_line(fl, buf);
  i = sscanf(buf, " %d %d %d %d ", &(Z.climate.season_wind_dir[0]), &(Z.climate.season_wind_dir[1]), &(Z.climate.season_wind_dir[2]), &(Z.climate.season_wind_dir[3]));
  if (i != 4) {
    fprintf(stderr, "Format error in wind_dir constant line - %s", zname);
    fflush(NULL);
    exit(0);
  }
  line_num += get_line(fl, buf);
  i = sscanf(buf, " %d %d %d %d ", &(Z.climate.season_wind_variance[0]), &(Z.climate.season_wind_variance[1]), &(Z.climate.season_wind_variance[2]), &(Z.climate.season_wind_variance[3]));
  if (i != 4) {
    fprintf(stderr, "Format error in wind_variance constant line - %s", zname);
    fflush(NULL);
    exit(0);
  }
  line_num += get_line(fl, buf);
  i = sscanf(buf, " %d %d %d %d ", &(Z.climate.season_precip[0]), &(Z.climate.season_precip[1]), &(Z.climate.season_precip[2]), &(Z.climate.season_precip[3]));
  if (i != 4) {
    fprintf(stderr, "Format error in precip constant line - %s", zname);
    fflush(NULL);
    exit(0);
  }
  line_num += get_line(fl, buf);
  i = sscanf(buf, " %d %d %d %d ", &(Z.climate.season_temp[0]), &(Z.climate.season_temp[1]), &(Z.climate.season_temp[2]), &(Z.climate.season_temp[3]));
  if (i != 4) {
    fprintf(stderr, "Format error in temp constant line - %s", zname);
    fflush(NULL);
    exit(0);
  }

  cmd_no = 0;

  for (;;) {
    if ((tmp = get_line(fl, buf)) == 0) {
      fprintf(stderr, "Format error in %s - premature end of file\n", zname);
      fflush(NULL);
      exit(0);
    }
    line_num += tmp;
    ptr = buf;
    skip_spaces(&ptr);

    if ((ZCMD.command = *ptr) == '*')
      continue;

    ptr++;

    if (ZCMD.command == 'S' || ZCMD.command == '$') {
      ZCMD.command = 'S';
      break;
    }
    error = 0;
    if (strchr("MOEPD", ZCMD.command) == NULL) { /* a 3-arg command */
      if (sscanf(ptr, " %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2) != 3)
        error = 1;
    } else {
      atmp = sscanf(ptr, " %d %d %d %d %d ", &tmp, &ZCMD.arg1, &ZCMD.arg2, &ZCMD.arg3, &ZCMD.arg4);
      if (atmp == 4)
        ZCMD.arg4 = 100;
      else if (atmp < 4)
        error = 1;
    }

    ZCMD.if_flag = tmp;

    if (error) {
      fprintf(stderr, "Format error in %s, line %d: '%s'\n", zname, line_num, buf);
      fflush(NULL);
      exit(0);
    }
    ZCMD.line = line_num;
    cmd_no++;
  }

  top_of_zone_table = zone++;
}

#undef Z

/*************************************************************************
 *  procedures for resetting, both play-time and boot-time      *
 *********************************************************************** */

int vnum_mobile(char *searchname, struct char_data * ch)
{
  int nr, found = 0;
  char mybuf[LARGE_BUFSIZE];

  mybuf[0] = '\0';
  for (nr = 0; nr <= top_of_mobt; nr++) {
    if (isname(searchname, mob_proto[nr].player.name)) {
      int oktosee = 0;
      int zonenum = 0;
      int virtual;
      int i;

      virtual = GET_MOB_VNUM((mob_proto + nr));

      zonenum = (virtual - (virtual % 100)) / 100;
      if ((zonenum == 12) || (zonenum == 30) || (zonenum == 31) || COM_FLAGGED(ch, COM_ADMIN)) {
        oktosee = 1;
      } else {
        for (i = 0; i < 4; i++) {
          if (ch->olc_zones[i] == zonenum) {
            oktosee = 1;
          }
        }
      }
      if (oktosee && zonenum != 0) {
        snprintf(buf, MAX_STRING_LENGTH, "%3d. [%5d] %s\r\n", ++found, mob_index[nr].virtual, mob_proto[nr].player.short_descr);
        strcat(mybuf, buf);
      }
    }
  }
  page_string(ch->desc, mybuf, 1);
  return (found);
}

int vnum_object(char *searchname, struct char_data * ch)
{
  int nr, found = 0;
  char mybuf[LARGE_BUFSIZE];

  mybuf[0] = '\0';
  for (nr = 0; nr <= top_of_objt; nr++) {
    if (isname(searchname, obj_proto[nr].name)) {
      int oktosee = 0;
      int zonenum = 0;
      int virtual;
      int i;

      virtual = GET_OBJ_VNUM((obj_proto + nr));

      zonenum = (virtual - (virtual % 100)) / 100;
      if ((zonenum == 12) || (zonenum == 30) || (zonenum == 31) || COM_FLAGGED(ch, COM_ADMIN)) {
        oktosee = 1;
      } else {
        for (i = 0; i < 4; i++) {
          if (ch->olc_zones[i] == zonenum) {
            oktosee = 1;
          }
        }
      }
      if (oktosee && zonenum != 0) {
        snprintf(buf, MAX_STRING_LENGTH, "%3d. [%5d] %s\r\n", ++found, obj_index[nr].virtual, obj_proto[nr].short_description);
        strcat(mybuf, buf);
      }
    }
  }
  page_string(ch->desc, mybuf, 1);
  return (found);
}

int vnum_type(char *searchtype, struct char_data * ch)
{
  int nr, found = 0;
  int type;
  char mybuf[LARGE_BUFSIZE];
  char *types[] = {"!RESERVED!", "light", "scroll", "wand", "staff", "weapon", "fireweapon", "missile", "treasure", "armor", "potion", "worn", "other", "trash", "trap", "container", "note", "drinkcontainer", "key", "food", "money", "pen", "boat", "fountain", "instrument", "badge", "\n"};

  mybuf[0] = '\0';

  for (type = 0; types[type][0] != '\n'; type++)
    if (!strncmp(searchtype, types[type], strlen(searchtype)))
      break;

  for (nr = 0; nr <= top_of_objt; nr++) {
    if (GET_OBJ_TYPE(obj_proto+nr) == type) {
      snprintf(buf, MAX_STRING_LENGTH, "%3d. [%5d] %s\r\n", ++found, obj_index[nr].virtual, obj_proto[nr].short_description);
      strcat(mybuf, buf);
    }
  }
  page_string(ch->desc, mybuf, 1);
  return (found);
}

/* create a character, and add it to the char list */
struct char_data *create_char(void)
{
  struct char_data *ch;

  CREATE(ch, struct char_data, 1);
  clear_char(ch);
  ch->next = character_list;
  character_list = ch;

  return ch;
}

/* create a new mobile from a prototype */
struct char_data *read_mobile(int nr, int type)
{
  int i, j;
  struct obj_data *obj;
  struct char_data *mob;
  struct mob_equipment_data *mob_equip;

  if (IS_SET(type, NOEQUIP)) {
    REMOVE_BIT(type, NOEQUIP);
    j = FALSE;
  } else
    j = TRUE;

  if (type == VIRTUAL) {
    if ((i = real_mobile(nr)) < 0) {
      snprintf(buf, MAX_STRING_LENGTH, "Mobile (V) %d does not exist in database.", nr);
      return (0);
    }
  } else
    i = nr;

  CREATE(mob, struct char_data, 1);
  clear_char(mob);
  *mob = mob_proto[i];
  mob->next = character_list;
  character_list = mob;

  if (!mob->points.max_hit) {
    mob->points.max_hit = dice(mob->points.hit, mob->points.mana) + mob->points.move;
  } else
    mob->points.max_hit = number(mob->points.hit, mob->points.mana);

  mob->points.hit = mob->points.max_hit;
  mob->points.mana = mob->points.max_mana;
  mob->points.move = mob->points.max_move;

  mob->player.time.birth = time(0);
  mob->player.time.played = 0;
  mob->player.time.logon = time(0);
  mob_index[i].number++;

  GET_VSTR(mob) = GET_STR(mob);
  GET_VDEX(mob) = GET_DEX(mob);
  GET_VINT(mob) = GET_INT(mob);
  GET_VWIS(mob) = GET_WIS(mob);
  GET_VCON(mob) = GET_CON(mob);
  GET_VAGI(mob) = GET_AGI(mob);

  /* randomize gold, with a +-10% factor */
  i = GET_TEMP_GOLD(mob) / 10;
  GET_TEMP_GOLD(mob) = (GET_TEMP_GOLD(mob) - i) + number(0, i * 2);

  if (j && mob->mob_specials.mob_equip) {
    for (mob_equip = mob->mob_specials.mob_equip; mob_equip; mob_equip = mob_equip->next)
      if (mob_equip->pos < 0) {
        if (number(1, 99) <= mob_equip->chance) {
          if (mob_equip->rnum >= 0) {
            if (obj_index[(int) mob_equip->rnum].number < mob_equip->max || mob_equip->max < 0) {
              obj = read_object(mob_equip->rnum, REAL);
              if (obj)
                obj_to_char(obj, mob);
            }
          } else {
            snprintf(buf, MAX_STRING_LENGTH, "SYSERR: trying to equip mob with negative rnum (vnum=%d)!", mob_equip->vnum);
            stderr_log(buf);
          }
        }
      } else if (!mob->equipment[(int) mob_equip->pos]) {
        if (number(1, 99) <= mob_equip->chance) {
          if (obj_index[(int) mob_equip->rnum].number < mob_equip->max || mob_equip->max < 0) {
            obj = read_object(mob_equip->rnum, REAL);
            if (obj)
              equip_char(mob, obj, mob_equip->pos);
          }
        }
      }
  }

  return mob;
}

/* create an object, and add it to the object list */
struct obj_data *create_obj(void)
{
  struct obj_data *obj;

  CREATE(obj, struct obj_data, 1);
  clear_object(obj);
  obj->next = object_list;
  object_list = obj;

  return obj;
}

/* create a new object from a prototype */
struct obj_data *read_object(int nr, int type)
{
  struct obj_data *obj;
  int i;

  if (nr < 0) {
    stderr_log("SYSERR: trying to create obj with negative num!");
    return NULL;
  }
  if (type == VIRTUAL) {
    if ((i = real_object(nr)) < 0) {
      snprintf(buf, MAX_STRING_LENGTH, "Object (V) %d does not exist in database.", nr);
      stderr_log(buf);
      return NULL;
    }
  } else
    i = nr;

  if (load_qic_check(i) == 0) {
    return NULL;
  }

  CREATE(obj, struct obj_data, 1);
  clear_object(obj);
  *obj = obj_proto[i];
  obj->next = object_list;
  object_list = obj;
  GET_OBJ_SLOTS(obj) = GET_OBJ_SLOTS(obj_proto + i);

  if (GET_OBJ_TYPE(obj) == ITEM_KEY) { /* make keys NODON, NOAUC and NOSELL */
    SET_BIT(GET_OBJ_EXTRA(obj), ITEM_NORENT);
    SET_BIT(GET_OBJ_EXTRA(obj), ITEM_NOAUCTION);
    SET_BIT(GET_OBJ_EXTRA(obj), ITEM_NODONATE);
    SET_BIT(GET_OBJ_EXTRA(obj), ITEM_NOSELL);
  }

  if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK) {
    CREATE(obj->spell_list, int, GET_OBJ_VAL(obj, 0));
  }

  if (obj_index[i].qic) {
    snprintf(logbuffer, sizeof(logbuffer), "%s created", obj->short_description);
    mudlog(logbuffer, 'J', COM_QUEST, FALSE);
  }

  obj_index[i].number++;

  return obj;
}

/* create a new object from a prototype (without QIC check) */
struct obj_data *read_object_q(int nr, int type)
{
  struct obj_data *obj;
  int i;

  if (nr < 0) {
    stderr_log("SYSERR: trying to create obj with negative num!");
    return NULL;
  }
  if (type == VIRTUAL) {
    if ((i = real_object(nr)) < 0) {
      snprintf(buf, MAX_STRING_LENGTH, "Object (V) %d does not exist in database.", nr);
      return NULL;
    }
  } else
    i = nr;

  /*  qic_load(i); */
  CREATE(obj, struct obj_data, 1);
  clear_object(obj);
  *obj = obj_proto[i];
  obj->next = object_list;
  object_list = obj;

  GET_OBJ_SLOTS(obj) = GET_OBJ_SLOTS(obj_proto + i);

  if (GET_OBJ_TYPE(obj) == ITEM_KEY) { /* make keys NODON, NOAUC and NOSELL */
    SET_BIT(GET_OBJ_EXTRA(obj), ITEM_NORENT);
    SET_BIT(GET_OBJ_EXTRA(obj), ITEM_NOAUCTION);
    SET_BIT(GET_OBJ_EXTRA(obj), ITEM_NODONATE);
    SET_BIT(GET_OBJ_EXTRA(obj), ITEM_NOSELL);
  }

  if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK) {
    CREATE(obj->spell_list, int, GET_OBJ_VAL(obj, 0));
  }

  obj_index[i].number++;

  return obj;
}

#define ZO_DEAD  999

/* update zone ages, queue for reset if necessary, and dequeue when possible */
void zone_update(void)
{
  int i;
  struct reset_q_element *update_u, *temp;
  static int timer = 0;
  char buf[128];

  /* jelson 10/22/92 */
  if (((++timer * PULSE_ZONE) / PASSES_PER_SEC) >= 60) {
    /* one minute has passed */
    /*
     * NOT accurate unless PULSE_ZONE is a multiple of PASSES_PER_SEC or a
     * factor of 60
     */

    timer = 0;

    /* since one minute has passed, increment zone ages */
    for (i = 0; i <= top_of_zone_table; i++) {
      if (zone_table[i].age < zone_table[i].lifespan && zone_table[i].reset_mode)
        (zone_table[i].age)++;

      if (zone_table[i].age >= zone_table[i].lifespan && zone_table[i].age < ZO_DEAD && zone_table[i].reset_mode) {
        /* enqueue zone */

        CREATE(update_u, struct reset_q_element, 1);

        update_u->zone_to_reset = i;
        update_u->next = 0;

        if (!reset_q.head)
          reset_q.head = reset_q.tail = update_u;
        else {
          reset_q.tail->next = update_u;
          reset_q.tail = update_u;
        }

        zone_table[i].age = ZO_DEAD;
      }
    }
  } /* end - one minute has passed */
  /* dequeue zones (if possible) and reset */
  /* this code is executed every 10 seconds (i.e. PULSE_ZONE) */
  for (update_u = reset_q.head; update_u; update_u = update_u->next)
    if (zone_table[update_u->zone_to_reset].reset_mode == 2 || is_empty(update_u->zone_to_reset)) {
      reset_zone(update_u->zone_to_reset);
      snprintf(buf, MAX_STRING_LENGTH, "Auto zone reset: %s", zone_table[update_u->zone_to_reset].name);
      mudlog(buf, 'Z', COM_QUEST, FALSE);
      /* dequeue */
      if (update_u == reset_q.head)
        reset_q.head = reset_q.head->next;
      else {
        for (temp = reset_q.head; temp->next != update_u; temp = temp->next)
          ;

        if (!update_u->next)
          reset_q.tail = temp;

        temp->next = update_u->next;
      }

      FREE(update_u);
      break;
    }
}

void log_zone_error(int zone, int cmd_no, char *message)
{
  char buf[256];

  snprintf(buf, MAX_STRING_LENGTH, "SYSERR: error in zone file: %s", message);
  mudlog(buf, 'E', COM_IMMORT, TRUE);

  snprintf(buf, MAX_STRING_LENGTH, "SYSERR: ...offending cmd: '%c' cmd in zone #%d, line %d", ZCMD.command, zone_table[zone].number, ZCMD.line);
  mudlog(buf, 'E', COM_IMMORT, TRUE);
  snprintf(buf, MAX_STRING_LENGTH, "SYSERR: ...arg1: %5d arg2: %5d arg3: %5d", ZCMD.arg1, ZCMD.arg2, ZCMD.arg3);
  mudlog(buf, 'E', COM_IMMORT, TRUE);
}

#define ZONE_ERROR(message) \
  { log_zone_error(zone, cmd_no, message); last_cmd = 0; }

/* execute the reset command table of a given zone */
void reset_zone(int zone)
{
  int cmd_no, last_cmd = 0, percent = 0;
  struct char_data *mob = NULL;
  struct obj_data *obj, *obj_to;
  int temp_gold = 0;

  for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++) {

    if (ZCMD.if_flag && !last_cmd)
      continue;

    switch (ZCMD.command) {
      case '*': /* ignore command */
        last_cmd = 0;
        break;

      case 'M': /* read a mobile */
        percent = number(1, 100);
        if (ZCMD.arg4 >= percent) {
          if (mob_index[ZCMD.arg1].number < ZCMD.arg2 || ZCMD.arg2 == 0) {
            mob = read_mobile(ZCMD.arg1, REAL);
            GET_IDNUM(mob) = mob_idnum;
            mob_idnum--;
            if (GET_TEMP_GOLD(mob)) {
              temp_gold = GET_TEMP_GOLD(mob) / 10;
              temp_gold = (GET_TEMP_GOLD(mob) - temp_gold) + number(0, temp_gold * 2);
              if (number(0, 1)) {
                GET_PLAT(mob) = (temp_gold / 1000);
                temp_gold -= (GET_PLAT(mob) * 1000);
              }
              if (number(0, 2)) {
                GET_GOLD(mob) = (temp_gold / 100);
                temp_gold -= (GET_GOLD(mob) * 100);
              }
              if (number(0, 3)) {
                GET_SILVER(mob) = (temp_gold / 10);
                temp_gold -= (GET_SILVER(mob) * 10);
              }
              GET_COPPER(mob) = temp_gold;
            }
            char_to_room(mob, ZCMD.arg3);
            mob->loadin = ZCMD.arg3;
            last_cmd = 1;
          } else
            last_cmd = 0;
        } else
          last_cmd = 0;
        break;

      case 'O': /* read an object */
        percent = number(1, 100);
        if (ZCMD.arg4 >= percent) {
          if (obj_index[ZCMD.arg1].number < ZCMD.arg2 || ZCMD.arg2 == 0)
            if (ZCMD.arg3 >= 0) {
              if (!get_obj_in_list_num(ZCMD.arg1, world[ZCMD.arg3].contents)) {
                obj = read_object(ZCMD.arg1, REAL);
                if (obj)
                  obj_to_room(obj, ZCMD.arg3);
                last_cmd = 1;
              } else
                last_cmd = 0;
            } else {
              obj = read_object(ZCMD.arg1, REAL);
              if (obj)
                obj->in_room = NOWHERE;
              last_cmd = 1;
            }
          else
            last_cmd = 0;
        } else
          last_cmd = 0;
        break;

      case 'P': /* object to object */
        if (obj_index[ZCMD.arg1].number < ZCMD.arg2 || ZCMD.arg2 == 0) {
          obj = read_object(ZCMD.arg1, REAL);
          if (!(obj_to = get_obj_num(ZCMD.arg3))) {
            ZONE_ERROR("target obj not found");
            break;
          }
          if (obj)
            obj_to_obj(obj, obj_to);
          last_cmd = 1;
        } else
          last_cmd = 0;
        break;

      case 'G': /* obj_to_char */
        if (!mob) {
          ZONE_ERROR("attempt to give obj to non-existant mob");
          break;
        }
        if (obj_index[ZCMD.arg1].number < ZCMD.arg2 || ZCMD.arg2 == 0) {
          obj = read_object(ZCMD.arg1, REAL);
          if (obj)
            obj_to_char(obj, mob);
          last_cmd = 1;
        } else
          last_cmd = 0;
        break;

      case 'E': /* object to equipment list */
        if (!mob) {
          ZONE_ERROR("trying to equip non-existant mob");
          break;
        }
        if (obj_index[ZCMD.arg1].number < ZCMD.arg2 || ZCMD.arg2 == 0) {
          if (ZCMD.arg3 < 0 || ZCMD.arg3 >= NUM_WEARS) {
            ZONE_ERROR("invalid equipment pos number");
          } else {
            obj = read_object(ZCMD.arg1, REAL);
            if (obj)
              equip_char(mob, obj, ZCMD.arg3);
            last_cmd = 1;
          }
        } else
          last_cmd = 0;
        break;

      case 'R': /* rem obj from room */
        if ((obj = get_obj_in_list_num(ZCMD.arg2, world[ZCMD.arg1].contents)) != NULL) {
          obj_from_room(obj);
          extract_obj(obj);
        }
        last_cmd = 1;
        break;

      case 'D': /* set state of door */
        if (ZCMD.arg2 < 0 || ZCMD.arg2 >= NUM_OF_DIRS || (world[ZCMD.arg1].dir_option[ZCMD.arg2] == NULL)) {
          ZONE_ERROR("door does not exist");
        } else
          switch (ZCMD.arg3) {
            case 0:
              REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info, EX_LOCKED);
              REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info, EX_CLOSED);
              REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info, EX_HIDDEN);
              break;
            case 1:
              SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info, EX_CLOSED);
              REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info, EX_LOCKED);
              REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info, EX_HIDDEN);
              break;
            case 2:
              SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info, EX_LOCKED);
              SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info, EX_CLOSED);
              REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info, EX_HIDDEN);
              break;
            case 3:
              REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info, EX_LOCKED);
              SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info, EX_CLOSED);
              SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info, EX_HIDDEN);
              break;
            case 4:
              SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info, EX_LOCKED);
              SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info, EX_CLOSED);
              SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info, EX_HIDDEN);
              break;
            case 5:
              REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info, EX_LOCKED);
              REMOVE_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info, EX_CLOSED);
              SET_BIT(world[ZCMD.arg1].dir_option[ZCMD.arg2]->exit_info, EX_HIDDEN);
              break;
          }
        last_cmd = 1;
        break;

      default:
        ZONE_ERROR("unknown cmd in reset table!");
        break;
    }
  }

  zone_table[zone].age = 0;
}

/* for use in reset_zone; return TRUE if zone 'nr' is free of PC's  */
int is_empty(int zone_nr)
{
  struct descriptor_data *i;

  for (i = descriptor_list; i; i = i->next)
    if (!i->connected)
      if (world[i->character->in_room].zone == zone_nr)
        return (0);

  return (1);
}

/*************************************************************************
 *  stuff related to the save/load player system         *
 *********************************************************************** */

int get_idnum(void)
{
  FILE *idfile;

  if ((idfile = fopen(ID_FILE, "r")) == NULL) {
    fclose(idfile);
    idfile = fopen(ID_FILE, "w");
    fprintf(idfile, "2");
    fclose(idfile);
    top_of_p_table = 1;
  } else {
    fscanf(idfile, "%d", &top_of_p_table);
    if (top_of_p_table < 0)
      top_of_p_table = 1;
    fclose(idfile);
    idfile = fopen(ID_FILE, "w");
    fprintf(idfile, "%d", top_of_p_table + 1);
    fclose(idfile);
  }
  return top_of_p_table;
}

long get_id_by_name(char *name)
{
  char filename[80];
  char abuf[MAX_INPUT_LENGTH * 8];
  char field[MAX_INPUT_LENGTH * 8];
  char value[MAX_INPUT_LENGTH * 8];
  FILE *f;

  one_argument(name, arg);
  get_filename(arg, filename, PTDAT_FILE);

  if ((f = fopen(filename, "r")) != NULL) {
    get_line(f, abuf);
    while (!feof(f)) {
      parse_pline(abuf, field, value);
      if (strcmp(field, "idnum") == 0)
        return atoi(value);
      get_line(f, abuf);
    }
  }
  return -1;
}

/* Load a char, TRUE if loaded, FALSE if not */

void parse_pline(char *line, char *field, char *value)
{
  char *linetmp = (line + 1);
  char *fieldtmp = field;
  char *valuetmp = value;

  while (*linetmp != '-')
    *(fieldtmp++) = *(linetmp++);
  *fieldtmp = '\0';

  linetmp++;
  while (*linetmp == ' ')
    linetmp++;

  while (*linetmp)
    *(valuetmp++) = *(linetmp++);
  *valuetmp = '\0';
}

void add_innates(struct char_data *ch)
{
  switch (GET_RACE(ch)) {
    /* infravision alone*/
    case RACE_HALFELF:
    case RACE_OGRE:
    case RACE_TROLL:
      mag_affect_char(NULL, AFF_INFRAVISION, ch, ch, -1);
      break;
      /* infravision and sneak */
    case RACE_ELF:
      mag_affect_char(NULL, AFF_INFRAVISION, ch, ch, -1);
      mag_affect_char(NULL, AFF_SNEAK, ch, ch, -1);
      break;
      /* infravision and detect align */
    case RACE_DWARF:
      mag_affect_char(NULL, AFF_INFRAVISION, ch, ch, -1);
      mag_affect_char(NULL, AFF_DETECT_ALIGN, ch, ch, -1);
      break;
      /* sneak alone */
    case RACE_HALFLING:
      mag_affect_char(NULL, AFF_SNEAK, ch, ch, -1);
      break;
    default:
      break;
  };
}

/* Load a char, TRUE if loaded, FALSE if not */
int load_char_text(char *name, struct char_data * char_element)
{
  int numval = 0, i = 0, j = 0, affnum = 0, num1 = 0, num2 = 0;
  char field[40];
  char resname[30];
  char line[MAX_INPUT_LENGTH + 21];
  char line2[MAX_INPUT_LENGTH + 21];
  char value[MAX_INPUT_LENGTH + 1];
  char filename[80];
  char buf2[80];
  char *p;
  char *cp;
  struct char_data *ctmp = char_element;
  struct affected_type affected[MAX_AFFECT];
  FILE *pfile;

  if (char_element->player_specials) {
    FREE(char_element->player_specials);
  }
  memset((struct char_data *) ctmp, 0, sizeof(struct char_data));
  memset((struct affected_type *) affected, 0, MAX_AFFECT * sizeof(struct affected_type));
  get_filename(name, filename, PTDAT_FILE);

  *GET_PASSWD(ctmp) = '\0';
  *GET_ENCPASSWD(ctmp) = '\0';

  if ((pfile = fopen(filename, "r")) == NULL) {
    return 0;
  }

  if (ctmp->player_specials == NULL) {
    CREATE(ctmp->player_specials, struct player_special_data, 1);
  }

  GET_DESC(ctmp) = strdup(" ");
  GET_TITLE(ctmp) = strdup(" ");
  GET_SCREEN_WIDTH(ctmp) = 80;
  GET_SCREEN_HEIGHT(ctmp) = 22;
  GET_TEMP_GOLD(ctmp) = 0;
  ctmp->consent = NULL;

  for (i = 0; i < MAX_SKILLS + 1; i++) {
    SET_SKILL(ctmp, i, 0);
  }

  i = 0;
  while (get_line(pfile, line)) {
    parse_pline(line, field, value);
    numval = atoi(value);

    switch (UPPER(*field)) {
      case 'A':
        if (!strcmp(field, "alignment")) {
          GET_ALIGNMENT(ctmp) = numval;
        } else if (!strcmp(field, "act")) {
          PLR_FLAGS(ctmp) = asciiflag_conv(value);
        } else if (!strcmp(field, "affby1")) {
          AFF_FLAGS(ctmp) = asciiflag_conv(value);
        } else if (!strcmp(field, "affby2")) {
          AFF2_FLAGS(ctmp) = asciiflag_conv(value);
        } else if (!strcmp(field, "affby3")) {
          AFF3_FLAGS(ctmp) = asciiflag_conv(value);
        } else if (!strcmp(field, "armor")) {
          GET_AC(ctmp) = numval;
        } else if (!strcmp(field, "agi")) {
          GET_AGI(ctmp) = numval;
        } else if (!strcmp(field, "Affected")) {
          for (;;) {
            get_line(pfile, line);
            parse_pline(line, field, value);
            numval = atoi(value);
            if (!strcmp(field, "EndAffected")) {
              break;
            }
            if (!strcmp(field, "affname")) {
              affected[affnum].type = spells[find_skill_num(value)].spellindex;
              for (i = 0; i < 4; i++) {
                get_line(pfile, line);
                parse_pline(line, field, value);
                numval = atoi(value);
                if (!strcmp(field, "duration")) {
                  affected[affnum].duration = numval;
                } else if (!strcmp(field, "modifier")) {
                  p = value;
                  j = 0;
                  while ((cp = strtok(p, " ")) != NULL) {
                    affected[affnum].modifier[j] = atoi(cp);
                    p = NULL;
                    j++;
                  }
                } else if (!strcmp(field, "location")) {
                  p = value;
                  j = 0;
                  while ((cp = strtok(p, " ")) != NULL) {
                    affected[affnum].location[j] = atoi(cp);
                    p = NULL;
                    j++;
                  }
                } else if (!strcmp(field, "bitvector")) {
                  affected[affnum].bitvector = numval;
                } else if (!strcmp(field, "bitvector2")) {
                  affected[affnum].bitvector2 = numval;
                } else if (!strcmp(field, "bitvector3")) {
                  affected[affnum].bitvector3 = numval;
                }
              }
              affnum++;
            }
          }
        } else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown field [%s]", field);
          stderr_log(buf2);
        }
        break;
      case 'B':
        if (!strcmp(field, "birth")) {
          GET_BIRTH(ctmp) = numval;
        } else if (!strcmp(field, "bad_pw")) {
          GET_BAD_PWS(ctmp) = numval;
        } else if (!strcmp(field, "bank_plat")) {
          GET_BANK_PLAT(ctmp) = numval;
        } else if (!strcmp(field, "bank_gold")) {
          GET_BANK_GOLD(ctmp) = numval;
        } else if (!strcmp(field, "bank_silver")) {
          GET_BANK_SILVER(ctmp) = numval;
        } else if (!strcmp(field, "bank_copper")) {
          GET_BANK_COPPER(ctmp) = numval;
        } else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown field [%s]", field);
          stderr_log(buf2);
        }
        break;
      case 'C':
        if (!strcmp(field, "class")) {
          GET_CLASS(ctmp) = numval;
        } else if (!strcmp(field, "commands")) {
          COM_FLAGS(ctmp) = asciiflag_conv(value);
        } else if (!strcmp(field, "copper")) {
          GET_COPPER(ctmp) = numval;
          GET_TEMP_GOLD(ctmp) += numval;
        } else if (!strcmp(field, "con")) {
          GET_CON(ctmp) = numval;
        } else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown field [%s]", field);
          stderr_log(buf2);
        }
        break;
      case 'D':
        if (!strcmp(field, "description")) {
          *line2 = '\0';
          if (GET_DESC(ctmp)) {
            FREE(GET_DESC(ctmp));
          }
          GET_DESC(ctmp) = fread_string(pfile, line2);
        } else if (!strcmp(field, "drunk")) {
          GET_COND(ctmp, 2) = numval;
        } else if (!strcmp(field, "death_count")) {
          GET_DEATHCOUNT(ctmp) = numval;
        } else if (!strcmp(field, "damroll")) {
          GET_DAMROLL(ctmp) = numval;
        } else if (!strcmp(field, "dex")) {
          GET_DEX(ctmp) = numval;
        } else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown field [%s]", field);
          stderr_log(buf2);
        }
        break;
      case 'E':
        if (!strcmp(field, "exp")) {
          GET_EXP(ctmp) = numval;
        } else if (!strcmp(field, "enc_password")) {
          strncpy(GET_ENCPASSWD(ctmp), value, strlen(value));
        } else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown field [%s]", field);
          stderr_log(buf2);
        }
        break;
      case 'F':
        if (!strcmp(field, "freeze_level")) {
          GET_FREEZE_LEV(ctmp) = numval;
        } else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown field [%s]", field);
          stderr_log(buf2);
        }
        break;
      case 'G':
        if (!strcmp(field, "gold")) {
          GET_GOLD(ctmp) = numval;
          GET_TEMP_GOLD(ctmp) += numval * 100;
        } else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown field [%s]", field);
          stderr_log(buf2);
        }
        break;
      case 'H':
        if (!strcmp(field, "host")) {
          strcpy(GET_HOST(ctmp), value);
        } else if (!strcmp(field, "hometown")) {
          GET_HOME(ctmp) = numval;
        } else if (!strcmp(field, "height")) {
          GET_HEIGHT(ctmp) = numval;
        } else if (!strcmp(field, "hungry")) {
          GET_COND(ctmp, 0) = numval;
        } else if (!strcmp(field, "hit")) {
          GET_HIT(ctmp) = numval;
        } else if (!strcmp(field, "hitroll")) {
          GET_HITROLL(ctmp) = numval;
        } else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown field [%s]", field);
          stderr_log(buf2);
        }
        break;
      case 'I':
        if (!strcmp(field, "idnum")) {
          GET_IDNUM(ctmp) = numval;
        } else if (!strcmp(field, "invis_level")) {
          GET_PLR_INVIS_LEV(ctmp) = numval;
        } else if (!strcmp(field, "int")) {
          GET_INT(ctmp) = numval;
        } else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown field [%s]", field);
          stderr_log(buf2);
        }
        break;
      case 'J':
        snprintf(buf2, MAX_STRING_LENGTH, "Unknown field [%s]", field);
        stderr_log(buf2);
        break;
      case 'K':
        if (!strcmp(field, "kills")) {
          GET_KILLS(ctmp) = numval;
        } else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown field [%s]", field);
          stderr_log(buf2);
        }
        break;
      case 'L':
        if (!strcmp(field, "last_logon")) {
          GET_LOGON(ctmp) = numval;
        } else if (!strcmp(field, "level")) {
          GET_LEVEL(ctmp) = numval;
        } else if (!strcmp(field, "log")) {
          LOG_FLAGS(ctmp) = asciiflag_conv(value);
        } else if (!strcmp(field, "lessons")) {
          GET_PRACTICES(ctmp) = numval;
        } else if (!strcmp(field, "load_room")) {
          GET_LOADROOM(ctmp) = numval;
        } else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown field [%s]", field);
          stderr_log(buf2);
        }
        break;
      case 'M':
        if (!strcmp(field, "mana")) {
          GET_MANA(ctmp) = numval;
        } else if (!strcmp(field, "max_mana")) {
          GET_MAX_MANA(ctmp) = numval;
        } else if (!strcmp(field, "max_hit")) {
          GET_MAX_HIT(ctmp) = numval;
        } else if (!strcmp(field, "move")) {
          GET_MOVE(ctmp) = numval;
        } else if (!strcmp(field, "max_move")) {
          GET_MAX_MOVE(ctmp) = numval;
        } else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown field [%s]", field);
          stderr_log(buf2);
        }
        break;
      case 'N':
        if (!strcmp(field, "name")) {
          if (GET_PLR_NAME(ctmp)) {
            FREE(GET_PLR_NAME(ctmp));
          }
          GET_PLR_NAME(ctmp) = strdup(value);
        } else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown field [%s]", field);
          stderr_log(buf2);
        }
        break;
      case 'O':
        if (!strcmp(field, "olc_zone1")) {
          ctmp->olc_zones[0] = numval;
        } else if (!strcmp(field, "olc_zone2")) {
          ctmp->olc_zones[1] = numval;
        } else if (!strcmp(field, "olc_zone3")) {
          ctmp->olc_zones[2] = numval;
        } else if (!strcmp(field, "olc_zone4")) {
          ctmp->olc_zones[3] = numval;
        } else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown field [%s]", field);
          stderr_log(buf2);
        }
        break;
      case 'P':
        if (!strcmp(field, "played")) {
          GET_PLAYED(ctmp) = numval;
        } else if (!strcmp(field, "password")) {
          strcpy(GET_PASSWD(ctmp), value);
        } else if (!strcmp(field, "plat")) {
          GET_PLAT(ctmp) = numval;
          GET_TEMP_GOLD(ctmp) += numval * 1000;
        } else if (!strcmp(field, "prompt")) {
          GET_PROMPT(ctmp) = asciiflag_conv(value);
        } else if (!strcmp(field, "pref")) {
          PRF_FLAGS(ctmp) = asciiflag_conv(value);
        } else if (!strcmp(field, "pk_count")) {
          GET_PKCOUNT(ctmp) = numval;
        } else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown field [%s]", field);
          stderr_log(buf2);
        }
        break;
      case 'Q':
        if (!strcmp(field, "quest_complete0")) {
          ctmp->player_specials->saved.questcompleted[0] = numval;
        } else if (!strcmp(field, "quest_complete1")) {
          ctmp->player_specials->saved.questcompleted[0] = numval;
        } else if (!strcmp(field, "quest_complete2")) {
          ctmp->player_specials->saved.questcompleted[0] = numval;
        } else if (!strcmp(field, "quest_complete3")) {
          ctmp->player_specials->saved.questcompleted[0] = numval;
        } else if (!strcmp(field, "quest_complete4")) {
          ctmp->player_specials->saved.questcompleted[0] = numval;
        } else if (!strcmp(field, "quest_complete5")) {
          ctmp->player_specials->saved.questcompleted[0] = numval;
        } else if (!strcmp(field, "quest_complete6")) {
          ctmp->player_specials->saved.questcompleted[0] = numval;
        } else if (!strcmp(field, "quest_complete7")) {
          ctmp->player_specials->saved.questcompleted[0] = numval;
        } else if (!strcmp(field, "quest_complete8")) {
          ctmp->player_specials->saved.questcompleted[0] = numval;
        } else if (!strcmp(field, "quest_complete9")) {
          ctmp->player_specials->saved.questcompleted[0] = numval;
        } else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown field [%s]", field);
          stderr_log(buf2);
        }
        break;
      case 'R':
        if (!strcmp(field, "race")) {
          GET_RACE(ctmp) = numval;
        } else if (!strcmp(field, "Resists")) {
          for (;;) {
            get_line(pfile, line);
            parse_pline(line, field, value);
            numval = atoi(value);
            if (!strcmp(field, "EndResists")) {
              break;
            }
            for (j = 0; j < MAX_DAM_TYPE; j++) {
              sprinttype(j, resists_names, resname);
              if (!strcmp(field, resname)) {
                GET_RESIST(ctmp, j) = numval;
              }
            }
          }
        } else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown field [%s]", field);
          stderr_log(buf2);
        }
        break;
      case 'S':
        if (!strcmp(field, "sex")) {
          GET_SEX(ctmp) = numval;
        } else if (!strcmp(field, "specialized")) {
          GET_SPECIALIZED(ctmp) = numval;
        } else if (!strcmp(field, "save_para")) {
          GET_SAVE(ctmp, 0) = numval;
        } else if (!strcmp(field, "save_rod")) {
          GET_SAVE(ctmp, 1) = numval;
        } else if (!strcmp(field, "save_petri")) {
          GET_SAVE(ctmp, 2) = numval;
        } else if (!strcmp(field, "save_breath")) {
          GET_SAVE(ctmp, 3) = numval;
        } else if (!strcmp(field, "save_spell")) {
          GET_SAVE(ctmp, 4) = numval;
        } else if (!strcmp(field, "screen_height")) {
          GET_SCREEN_HEIGHT(ctmp) = numval;
        } else if (!strcmp(field, "screen_width")) {
          GET_SCREEN_WIDTH(ctmp) = numval;
        } else if (!strcmp(field, "silver")) {
          GET_SILVER(ctmp) = numval;
          GET_TEMP_GOLD(ctmp) += numval * 10;
        } else if (!strcmp(field, "str")) {
          GET_STR(ctmp) = numval;
        } else if (!strcmp(field, "Skills")) {
          for (;;) {
            get_line(pfile, line);
            parse_pline(line, field, value);
            numval = atoi(value);
            if (!strcmp(field, "EndSkills")) {
              break;
            }
            for (j = 1; spells[j].command[0] != '\n'; j++) {
              if (!strcmp(field, spells[j].command)) {
                sscanf(value, "%d %d", &num1, &num2);
                GET_SKILL(ctmp, spells[j].spellindex) = num1;
                GET_PRACS(ctmp, spells[j].spellindex) = num2;
              }
            }
          }
        } else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown field [%s]", field);
          stderr_log(buf2);
        }
        break;
      case 'T':
        if (!strcmp(field, "title")) {
          if (GET_TITLE(ctmp)) {
            FREE(GET_TITLE(ctmp));
          }
          GET_TITLE(ctmp) = strdup(value);
        } else if (!strcmp(field, "thirsty")) {
          GET_COND(ctmp, 1) = numval;
        } else if (!strcmp(field, "timer")) {
          GET_TIMER(ctmp) = numval;
        } else if (!strcmp(field, "train_sess")) {
          GET_TRAINING(ctmp) = numval;
        } else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown field [%s]", field);
          stderr_log(buf2);
        }
        break;
      case 'U':
        snprintf(buf2, MAX_STRING_LENGTH, "Unknown field [%s]", field);
        stderr_log(buf2);
        break;
      case 'V':
        snprintf(buf2, MAX_STRING_LENGTH, "Unknown field [%s]", field);
        stderr_log(buf2);
        break;
      case 'W':
        if (!strcmp(field, "weapontimer")) {
          ctmp->player_specials->saved.weapontimer = numval;
        } else if (!strcmp(field, "weight")) {
          GET_WEIGHT(ctmp) = numval;
        } else if (!strcmp(field, "wimp_level")) {
          GET_WIMP_LEV(ctmp) = numval;
        } else if (!strcmp(field, "wis")) {
          GET_WIS(ctmp) = numval;
        } else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown field [%s]", field);
          stderr_log(buf2);
        }
        break;
      case 'X':
        snprintf(buf2, MAX_STRING_LENGTH, "Unknown field [%s]", field);
        stderr_log(buf2);
        break;
      case 'Y':
        snprintf(buf2, MAX_STRING_LENGTH, "Unknown field [%s]", field);
        stderr_log(buf2);
        break;
      case 'Z':
        snprintf(buf2, MAX_STRING_LENGTH, "Unknown field [%s]", field);
        stderr_log(buf2);
        break;
      default:
        snprintf(buf2, MAX_STRING_LENGTH, "Unknown field [%s]", field);
        stderr_log(buf2);
        break;
    }
  }

  GET_VSTR(ctmp) = GET_STR(ctmp);
  GET_VDEX(ctmp) = GET_DEX(ctmp);
  GET_VINT(ctmp) = GET_INT(ctmp);
  GET_VWIS(ctmp) = GET_WIS(ctmp);
  GET_VCON(ctmp) = GET_CON(ctmp);
  GET_VAGI(ctmp) = GET_AGI(ctmp);

  for (i = 0; i < MAX_AFFECT; i++) {
    if (affected[i].type) {
      affect_to_char(ctmp, (affected + i));
    }
  }

  POOFIN(ctmp) = NULL;
  POOFOUT(ctmp) = NULL;
  WHOSPEC(ctmp) = NULL;
  WHOSTR(ctmp) = NULL;
  ctmp->player.short_descr = NULL;
  ctmp->player.long_descr = NULL;

  ctmp->real_abils = ctmp->aff_abils;
  ctmp->char_specials.carry_weight = 0;
  ctmp->char_specials.carry_items = 0;

  ctmp->in_room = real_room(GET_LOADROOM(ctmp));
  GET_WAS_IN(ctmp) = NOWHERE;

  if (!IS_AFFECTED(ctmp, AFF_POISON) && (((long) (time(0) - GET_LOGON(ctmp))) >= SECS_PER_REAL_HOUR)) {
    GET_HIT(ctmp) = GET_MAX_HIT(ctmp);
    GET_MOVE(ctmp) = GET_MAX_MOVE(ctmp);
    GET_MANA(ctmp) = GET_MAX_MANA(ctmp);
  }

  if (ctmp->desc) {
    GET_LOGON(ctmp) = time(0);
  }

  add_innates(ctmp);
  fclose(pfile);
  return 1;
}

/* write the vital data of a player to the player file */
void save_char_text(struct char_data * ch, sh_int load_room)
{
  extern void olc_print_bitvectors(FILE *f, long bitvector, long max);

  int i, j;
  FILE *pfile;
  char tname[50];
  char filename[80];
  char descript[EXDSCR_LENGTH];
  char *p;
  struct affected_type affected[MAX_AFFECT];
  struct affected_type *af;
  struct obj_data *char_eq[NUM_WEARS];

  if (IS_NPC(ch))
    return;

  /* Unaffect everything a character can be affected by */
  for (i = 0; i < NUM_WEARS; i++) {
    if (ch->equipment[i]) {
      char_eq[i] = unequip_char(ch, i);
    } else {
      char_eq[i] = NULL;
    }
  }

  for (af = ch->affected, i = 0; i < MAX_AFFECT; i++) {
    if (af) {
      affected[i] = *af;
      affected[i].next = 0;
      af = af->next;
    } else {
      affected[i].type = 0; /* Zero signifies not used */
      affected[i].duration = 0;
      affected[i].modifier[0] = 0;
      affected[i].location[0] = 0;
      affected[i].bitvector = 0;
      affected[i].bitvector2 = 0;
      affected[i].bitvector3 = 0;
      affected[i].next = 0;
    }
  }

  if ((i >= MAX_AFFECT) && af && af->next) {
    stderr_log("SYSERR: WARNING: OUT OF STORE ROOM FOR AFFECTED TYPES!!!");
  }

  /*
   * remove the affections so that the raw values are stored; otherwise the
   * effects are doubled when the char logs back in.
   */

  while (ch->affected) {
    affect_remove(ch, ch->affected);
  }

  ch->aff_abils = ch->real_abils;

  GET_PLAYED(ch) += (long) (time(0) - ch->player.time.logon);
  GET_LOGON(ch) = time(0);

  get_filename(GET_NAME(ch), filename, PTDAT_FILE);

  pfile = fopen(filename, "w");

  if (GET_DESC(ch)) {
    memset(descript, 0, EXDSCR_LENGTH);
    strcpy(descript, GET_DESC(ch));
    strip_string(descript);
    while ((p = strchr(descript, '~')) != NULL) {
      *p = ' ';
    }
    if (!IS_IMMO(ch)) {
      while ((p = strchr(descript, '{')) != NULL) {
        *p = ' ';
        if (*(p + 1)) {
          *(p + 1) = ' ';
        }
      }
    }
    fprintf(pfile, "-description-\n%s~\n", descript);
  }

  fprintf(pfile, "-name- %s\n", GET_NAME(ch));

  if (GET_TITLE(ch)) {
    fprintf(pfile, "-title- %s\n", GET_TITLE(ch));
  }
  if (GET_SEX(ch)) {
    fprintf(pfile, "-sex- %d\n", GET_SEX(ch));
  }
  if (GET_CLASS(ch)) {
    fprintf(pfile, "-class- %d\n", GET_CLASS(ch));
  }
  if (GET_LEVEL(ch)) {
    fprintf(pfile, "-level- %d\n", GET_LEVEL(ch));
  }
  if (GET_HOME(ch)) {
    fprintf(pfile, "-hometown- %d\n", GET_HOME(ch));
  }
  if (GET_BIRTH(ch)) {
    fprintf(pfile, "-birth- %d\n", (int) GET_BIRTH(ch));
  }
  if (GET_PLAYED(ch)) {
    fprintf(pfile, "-played- %d\n", GET_PLAYED(ch));
  }
  if (GET_LOGON(ch)) {
    fprintf(pfile, "-last_logon- %d\n", (int) GET_LOGON(ch));
  }
  if (GET_HOST(ch)) {
    fprintf(pfile, "-host- %s\n", GET_HOST(ch));
  }
  if (GET_WEIGHT(ch)) {
    fprintf(pfile, "-weight- %d\n", GET_WEIGHT(ch));
  }
  if (GET_HEIGHT(ch)) {
    fprintf(pfile, "-height- %d\n", GET_HEIGHT(ch));
  }
  /* don't print out the password, just update encrypted password instead */
  if (*GET_PASSWD(ch) != '\0') {
    if (crypto_pwhash_str(GET_ENCPASSWD(ch), GET_PASSWD(ch), strlen(GET_PASSWD(ch)), crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE) == 0) {
        /* no op */
    }
  }
  if (*GET_ENCPASSWD(ch) != '\0') {
    fprintf(pfile, "-enc_password- %s\n", GET_ENCPASSWD(ch));
  }
  if (GET_SCREEN_HEIGHT(ch)) {
    fprintf(pfile, "-screen_height- %d\n", GET_SCREEN_HEIGHT(ch));
  }
  if (GET_SCREEN_WIDTH(ch)) {
    fprintf(pfile, "-screen_width- %d\n", GET_SCREEN_WIDTH(ch));
  }
  if (GET_ALIGNMENT(ch)) {
    fprintf(pfile, "-alignment- %d\n", GET_ALIGNMENT(ch));
  }
  if (GET_IDNUM(ch)) {
    fprintf(pfile, "-idnum- %ld\n", GET_IDNUM(ch));
  }
  if (GET_SPECIALIZED(ch)) {
    fprintf(pfile, "-specialized- %d\n", GET_SPECIALIZED(ch));
  }
  if (GET_SAVE(ch, 0)) {
    fprintf(pfile, "-save_para- %d\n", GET_SAVE(ch, 0));
  }
  if (GET_SAVE(ch, 1)) {
    fprintf(pfile, "-save_rod- %d\n", GET_SAVE(ch, 1));
  }
  if (GET_SAVE(ch, 2)) {
    fprintf(pfile, "-save_petri- %d\n", GET_SAVE(ch, 2));
  }
  if (GET_SAVE(ch, 3)) {
    fprintf(pfile, "-save_breath- %d\n", GET_SAVE(ch, 3));
  }
  if (GET_SAVE(ch, 4)) {
    fprintf(pfile, "-save_spell- %d\n", GET_SAVE(ch, 4));
  }
  if (ch->olc_zones[0]) {
    fprintf(pfile, "-olc_zone1- %d\n", ch->olc_zones[0]);
  }
  if (ch->olc_zones[1]) {
    fprintf(pfile, "-olc_zone2- %d\n", ch->olc_zones[1]);
  }
  if (ch->olc_zones[2]) {
    fprintf(pfile, "-olc_zone3- %d\n", ch->olc_zones[2]);
  }
  if (ch->olc_zones[3]) {
    fprintf(pfile, "-olc_zone4- %d\n", ch->olc_zones[3]);
  }
  for (i = 0; i < 10; i++) {
    if (ch->player_specials->saved.questcompleted[i]) {
      fprintf(pfile, "-quest_complete%d- %d\n", i, ch->player_specials->saved.questcompleted[i]);
    }
  }

  if (PLR_FLAGS(ch)) {
    fprintf(pfile, "-act- ");
    olc_print_bitvectors(pfile, PLR_FLAGS(ch), PLR_MAX);
    fprintf(pfile, "\n");
  }
  if (AFF_FLAGS(ch)) {
    fprintf(pfile, "-affby1- ");
    olc_print_bitvectors(pfile, AFF_FLAGS(ch), NUM_AFF_FLAGS);
    fprintf(pfile, "\n");
  }
  if (AFF2_FLAGS(ch)) {
    fprintf(pfile, "-affby2- ");
    olc_print_bitvectors(pfile, AFF2_FLAGS(ch), NUM_AFF2_FLAGS);
    fprintf(pfile, "\n");
  }
  if (AFF3_FLAGS(ch)) {
    fprintf(pfile, "-affby3- ");
    olc_print_bitvectors(pfile, AFF3_FLAGS(ch), NUM_AFF3_FLAGS);
    fprintf(pfile, "\n");
  }
  if (ch->player_specials->saved.weapontimer) {
    fprintf(pfile, "-weapontimer- %d\n", ch->player_specials->saved.weapontimer);
  }
  if (COM_FLAGS(ch)) {
    fprintf(pfile, "-commands- ");
    olc_print_bitvectors(pfile, COM_FLAGS(ch), COM_MAX);
    fprintf(pfile, "\n");
  }
  if (GET_PROMPT(ch)) {
    fprintf(pfile, "-prompt- ");
    olc_print_bitvectors(pfile, GET_PROMPT(ch), PRM_MAX);
    fprintf(pfile, "\n");
  }
  if (PRF_FLAGS(ch)) {
    fprintf(pfile, "-pref- ");
    olc_print_bitvectors(pfile, PRF_FLAGS(ch), PRF_MAX);
    fprintf(pfile, "\n");
  }
  if (LOG_FLAGS(ch)) {
    fprintf(pfile, "-log- ");
    olc_print_bitvectors(pfile, LOG_FLAGS(ch), 26);
    fprintf(pfile, "\n");
  }
  if (GET_PRACTICES(ch)) {
    fprintf(pfile, "-lessons- %d\n", GET_PRACTICES(ch));
  }
  if (GET_WIMP_LEV(ch)) {
    fprintf(pfile, "-wimp_level- %d\n", GET_WIMP_LEV(ch));
  }
  if (GET_FREEZE_LEV(ch)) {
    fprintf(pfile, "-freeze_level- %d\n", GET_FREEZE_LEV(ch));
  }
  if (GET_INVIS_LEV(ch)) {
    fprintf(pfile, "-invis_level- %d\n", GET_INVIS_LEV(ch));
  }
  if (PLR_FLAGGED(ch, PLR_LOADROOM)) {
    fprintf(pfile, "-load_room- %d\n", GET_LOADROOM(ch));
  } else {
    fprintf(pfile, "-load_room- %d\n", load_room);
  }
  if (GET_BAD_PWS(ch)) {
    fprintf(pfile, "-bad_pw- %d\n", GET_BAD_PWS(ch));
  }
  if (GET_COND(ch, 0)) {
    fprintf(pfile, "-hungry- %d\n", GET_COND(ch, 0));
  }
  if (GET_COND(ch, 1)) {
    fprintf(pfile, "-thirsty- %d\n", GET_COND(ch, 1));
  }
  if (GET_COND(ch, 2)) {
    fprintf(pfile, "-drunk- %d\n", GET_COND(ch, 2));
  }
  if (GET_RACE(ch)) {
    fprintf(pfile, "-race- %d\n", GET_RACE(ch));
  }
  if (GET_PKCOUNT(ch)) {
    fprintf(pfile, "-pk_count- %d\n", GET_PKCOUNT(ch));
  }
  if (GET_TIMER(ch)) {
    fprintf(pfile, "-timer- %d\n", GET_TIMER(ch));
  }
  if (GET_DEATHCOUNT(ch)) {
    fprintf(pfile, "-death_count- %d\n", GET_DEATHCOUNT(ch));
  }
  if (GET_KILLS(ch)) {
    fprintf(pfile, "-kills- %d\n", GET_KILLS(ch));
  }
  if (GET_TRAINING(ch)) {
    fprintf(pfile, "-train_sess- %d\n", GET_TRAINING(ch));
  }
  if (GET_MANA(ch)) {
    fprintf(pfile, "-mana- %d\n", GET_MANA(ch));
  }
  if (GET_MAX_MANA(ch)) {
    fprintf(pfile, "-max_mana- %d\n", GET_MAX_MANA(ch));
  }
  if (GET_HIT(ch)) {
    fprintf(pfile, "-hit- %d\n", GET_HIT(ch));
  }
  if (GET_MAX_HIT(ch)) {
    fprintf(pfile, "-max_hit- %d\n", GET_MAX_HIT(ch));
  }
  if (GET_MOVE(ch)) {
    fprintf(pfile, "-move- %d\n", GET_MOVE(ch));
  }
  if (GET_MAX_MOVE(ch)) {
    fprintf(pfile, "-max_move- %d\n", GET_MAX_MOVE(ch));
  }
  if (IS_MONK(ch)) {
    fprintf(pfile, "-armor- %d\n", GET_AC(ch));
  } else {
    fprintf(pfile, "-armor- 100\n");
  }
  if (GET_PLAT(ch)) {
    fprintf(pfile, "-plat- %d\n", GET_PLAT(ch));
  }
  if (GET_GOLD(ch)) {
    fprintf(pfile, "-gold- %d\n", GET_GOLD(ch));
  }
  if (GET_SILVER(ch)) {
    fprintf(pfile, "-silver- %d\n", GET_SILVER(ch));
  }
  if (GET_COPPER(ch)) {
    fprintf(pfile, "-copper- %d\n", GET_COPPER(ch));
  }
  if (GET_BANK_PLAT(ch)) {
    fprintf(pfile, "-bank_plat- %d\n", GET_BANK_PLAT(ch));
  }
  if (GET_BANK_GOLD(ch)) {
    fprintf(pfile, "-bank_gold- %d\n", GET_BANK_GOLD(ch));
  }
  if (GET_BANK_SILVER(ch)) {
    fprintf(pfile, "-bank_silver- %d\n", GET_BANK_SILVER(ch));
  }
  if (GET_BANK_COPPER(ch)) {
    fprintf(pfile, "-bank_copper- %d\n", GET_BANK_COPPER(ch));
  }
  if (GET_EXP(ch)) {
    fprintf(pfile, "-exp- %d\n", GET_EXP(ch));
  }
  if (GET_STR(ch)) {
    fprintf(pfile, "-str- %d\n", GET_STR(ch));
  }
  if (GET_INT(ch)) {
    fprintf(pfile, "-int- %d\n", GET_INT(ch));
  }
  if (GET_WIS(ch)) {
    fprintf(pfile, "-wis- %d\n", GET_WIS(ch));
  }
  if (GET_DEX(ch)) {
    fprintf(pfile, "-dex- %d\n", GET_DEX(ch));
  }
  if (GET_CON(ch)) {
    fprintf(pfile, "-con- %d\n", GET_CON(ch));
  }
  if (GET_AGI(ch)) {
    fprintf(pfile, "-agi- %d\n", GET_AGI(ch));
  }

  fprintf(pfile, "-Resists-\n");
  for (i = 0; i < MAX_DAM_TYPE; i++) {
    if (GET_RESIST(ch, i)) {
      sprinttype(i, resists_names, tname);
      fprintf(pfile, "-%s- %d\n", tname, GET_RESIST(ch, i));
    }
  }
  fprintf(pfile, "-EndResists-\n");

  fprintf(pfile, "-Skills-\n");
  file_list_skills(ch, pfile);
  fprintf(pfile, "-EndSkills-\n");

  fprintf(pfile, "-Affected-\n");
  for (i = 0; i < MAX_AFFECT; i++) {
    if (affected[i].type) {
      fprintf(pfile, "-name- %s\n"
          "-duration- %d\n"
          "-bitvector- %ld\n"
          "-bitvector2- %ld\n"
          "-bitvector3- %ld\n", get_spell_name(affected[i].type), affected[i].duration, affected[i].bitvector, affected[i].bitvector2, affected[i].bitvector3);
      fprintf(pfile, "-location- ");
      for (j = 0; j < NUM_MODIFY; j++) {
        fprintf(pfile, "%d ", affected[i].location[j]);
      }
      fprintf(pfile, "\n-modifier- ");
      for (j = 0; j < NUM_MODIFY; j++) {
        fprintf(pfile, "%d ", affected[i].modifier[j]);
      }
      fprintf(pfile, "\n");
    }
  }
  fprintf(pfile, "-EndAffected-\n");

  /* add spell and eq affections back in now */
  for (i = 0; i < MAX_AFFECT; i++) {
    if (affected[i].type) {
      affect_to_char(ch, (affected + i));
    }
  }

  for (i = 0; i < NUM_WEARS; i++) {
    if (char_eq[i]) {
      equip_char(ch, char_eq[i], i);
    }
  }

  fclose(pfile);
}

/************************************************************************
 *  procs of a (more or less) general utility nature      *
 ********************************************************************** */

/* read and allocate space for a '~'-terminated string from a given file */
char *fread_string(FILE * fl, char *error)
{
  char buf[MAX_STRING_LENGTH], tmp[512], *rslt;
  register char *point;
  int done = 0, length = 0, templength = 0;
  int i;

  *buf = '\0';

  do {
    if (!fgets(tmp, 512, fl)) {
      fprintf(stderr, "SYSERR: fread_string: format error at or near %s\n", error);
      fflush(NULL);
      exit(1);
    }
    /* If there is a '~', end the string; else an "\r\n" over the '\n'. */
    templength = strlen(tmp);
    for (i = 0; i < 4; i++) {
      if ((templength - i) >= 0 && tmp[templength - i] == '~') {
        tmp[templength - i] = '\0';
        done = 1;
      }
    }
    if (!done) {
      point = tmp + templength - 1;
      *(point++) = '\r';
      *(point++) = '\n';
      *point = '\0';
      templength++;
    }

    if (length + templength >= MAX_STRING_LENGTH) {
      stderr_log("SYSERR: fread_string: string too large (db.c)");
      fflush(NULL);
      exit(1);
    } else {
      strcpy(buf + length, tmp);
      length += templength;
    }
  } while (!done);

  /* allocate space for the new string and copy it */
  if (length > 0) {
    CREATE(rslt, char, length + 1);
    strcpy(rslt, buf);
    rslt[length] = '\0';
  } else
    rslt = NULL;

  return rslt;
}

/* release memory allocated for a char struct */
void free_char(struct char_data * ch)
{
  int i;
  struct alias *a;
  struct mob_attacks_data *this, *next_one;
  struct mob_equipment_data *equip, *next_equip;
  struct mob_action_data *action, *next_action;
  void free_alias(struct alias * a);

  if (!IS_NPC(ch) && ch->player_specials != NULL) {
    while ((a = GET_ALIASES(ch)) != NULL) {
      GET_ALIASES(ch) = (GET_ALIASES(ch))->next;
      free_alias(a);
    }
    GET_ALIASES(ch) = NULL;
    if (ch->player_specials->poofin) {
      FREE(ch->player_specials->poofin);
      ch->player_specials->poofin = NULL;
    }
    if (ch->player_specials->poofout) {
      FREE(ch->player_specials->poofout);
      ch->player_specials->poofout = NULL;
    }
    if (ch->player_specials->whospec) {
      FREE(ch->player_specials->whospec);
      ch->player_specials->whospec = NULL;
    }
    if (ch->player_specials->whois) {
      FREE(ch->player_specials->whois);
      ch->player_specials->whois = NULL;
    }
    if (ch && ch->desc && ch->desc->namecolor) {
      FREE(ch->desc->namecolor);
      ch->desc->namecolor = NULL;
    }
    FREE(ch->player_specials);
    ch->player_specials = NULL;
  }
  if (!IS_NPC(ch) || (IS_NPC(ch) && GET_MOB_RNUM(ch) == -1)) {
    /* if this is a player, or a non-prototyped non-player, free all */
    if (IS_ANIMATED(ch)) {
      if (GET_NAME(ch))
        FREE(GET_NAME(ch));
      IS_ANIMATED(ch) = 0;
    }
    if (GET_NAME(ch)) {
      FREE(GET_NAME(ch));
    }
    if (ch->player.title) {
      FREE(ch->player.title);
      ch->player.title = NULL;
    }
    if (ch->player.short_descr) {
      FREE(ch->player.short_descr);
      ch->player.short_descr = NULL;
    }
    if (ch->player.long_descr) {
      FREE(ch->player.long_descr);
      ch->player.long_descr = NULL;
    }
    if (GET_DESC(ch)) {
      FREE(GET_DESC(ch));
      GET_DESC(ch) = NULL;
    }
  } else if ((i = GET_MOB_RNUM(ch)) > -1) {
    /* otherwise, free strings only if the string is not pointing at proto */
    if (ch->player.name && ch->player.name != mob_proto[i].player.name) {
      FREE(ch->player.name);
      ch->player.name = NULL;
    }
    if (ch->player.title && ch->player.title != mob_proto[i].player.title) {
      FREE(ch->player.title);
      ch->player.title = NULL;
    }
    if (ch->player.short_descr && ch->player.short_descr != mob_proto[i].player.short_descr) {
      FREE(ch->player.short_descr);
      ch->player.short_descr = NULL;
    }
    if (ch->player.long_descr && ch->player.long_descr != mob_proto[i].player.long_descr) {
      FREE(ch->player.long_descr);
      ch->player.long_descr = NULL;
    }
    if (GET_DESC(ch) && GET_DESC(ch) != mob_proto[i].player.description) {
      FREE(GET_DESC(ch));
      GET_DESC(ch) = NULL;
    }
    /* free extra stuff */
    if (ch->mob_specials.mob_attacks && ch->mob_specials.mob_attacks != mob_proto[i].mob_specials.mob_attacks) {
      for (this = ch->mob_specials.mob_attacks; this; this = next_one) {
        next_one = this->next;
        FREE(this);
      }
      ch->mob_specials.mob_attacks = NULL;
    }
    if (ch->mob_specials.mob_equip && ch->mob_specials.mob_equip != mob_proto[i].mob_specials.mob_equip) {
      for (equip = ch->mob_specials.mob_equip; equip; equip = next_equip) {
        next_equip = equip->next;
        FREE(equip);
      }
      ch->mob_specials.mob_equip = NULL;
    }
    if (ch->mob_specials.mob_action && ch->mob_specials.mob_action != mob_proto[i].mob_specials.mob_action) {
      for (action = ch->mob_specials.mob_action; action; action = next_action) {
        next_action = action->next;
        if (action->action)
          FREE(action->action);
        FREE(action);
      }
      ch->mob_specials.mob_action = NULL;
    }
  }
  while (ch->affected)
    affect_remove(ch, ch->affected);

  FREE(ch);
  ch = NULL;
}

/* release memory allocated for an obj struct */
void free_obj(struct obj_data * obj)
{
  int nr;
  struct extra_descr_data *this, *next_one;

  if (obj->spell_list) {
    FREE(obj->spell_list);
    obj->spell_list = NULL;
  }
  if ((nr = GET_OBJ_RNUM(obj)) == -1) {
    if (obj->owner) {
      FREE(obj->owner);
      obj->owner = NULL;
    }
    if (obj->name) {
      FREE(obj->name);
      obj->name = NULL;
    }
    if (obj->description) {
      FREE(obj->description);
      obj->description = NULL;
    }
    if (obj->short_description) {
      FREE(obj->short_description);
      obj->short_description = NULL;
    }
    if (obj->cname) {
      FREE(obj->cname);
      obj->cname = NULL;
    }
    if (obj->cdescription) {
      FREE(obj->cdescription);
      obj->cdescription = NULL;
    }
    if (obj->cshort_description) {
      FREE(obj->cshort_description);
      obj->cshort_description = NULL;
    }
    if (obj->action_description) {
      FREE(obj->action_description);
      obj->action_description = NULL;
    }
    if (obj->ex_description) {
      for (this = obj->ex_description; this; this = next_one) {
        next_one = this->next;
        if (this->keyword) {
          FREE(this->keyword);
        }
        if (this->description) {
          FREE(this->description);
        }
        FREE(this);
      }
      obj->ex_description = NULL;
    }
  } else {
    purge_qic(nr);

    if (obj->name && obj->name != obj_proto[nr].name) {
      FREE(obj->name);
      obj->name = NULL;
    }
    if (obj->description && obj->description != obj_proto[nr].description) {
      FREE(obj->description);
      obj->description = NULL;
    }
    if (obj->short_description && obj->short_description != obj_proto[nr].short_description) {
      FREE(obj->short_description);
      obj->short_description = NULL;
    }
    if (obj->action_description && obj->action_description != obj_proto[nr].action_description) {
      FREE(obj->action_description);
      obj->action_description = NULL;
    }

    if (obj->ex_description && obj->ex_description != obj_proto[nr].ex_description) {
      for (this = obj->ex_description; this; this = next_one) {
        next_one = this->next;
        if (this->keyword) {
          FREE(this->keyword);
        }
        if (this->description) {
          FREE(this->description);
        }
        FREE(this);
      }
      obj->ex_description = NULL;
    }
  }

  FREE(obj);
  obj = NULL;
}

/* release memory allocated for an obj struct */
void free_obj_q(struct obj_data * obj)
{
  int nr;
  struct extra_descr_data *this, *next_one;

  if (obj->spell_list) {
    FREE(obj->spell_list);
    obj->spell_list = NULL;
  }
  if ((nr = GET_OBJ_RNUM(obj)) == -1) {
    if (obj->name) {
      FREE(obj->name);
      obj->name = NULL;
    }
    if (obj->description) {
      FREE(obj->description);
      obj->description = NULL;
    }
    if (obj->short_description) {
      FREE(obj->short_description);
      obj->short_description = NULL;
    }
    if (obj->action_description) {
      FREE(obj->action_description);
      obj->action_description = NULL;
    }
    if (obj->ex_description) {
      for (this = obj->ex_description; this; this = next_one) {
        next_one = this->next;
        if (this->keyword) {
          FREE(this->keyword);
        }
        if (this->description) {
          FREE(this->description);
        }
        FREE(this);
      }
      obj->ex_description = NULL;
    }
  } else {
    if (obj->name && obj->name != obj_proto[nr].name) {
      FREE(obj->name);
      obj->name = NULL;
    }
    if (obj->description && obj->description != obj_proto[nr].description) {
      FREE(obj->description);
      obj->description = NULL;
    }
    if (obj->short_description && obj->short_description != obj_proto[nr].short_description) {
      FREE(obj->short_description);
      obj->short_description = NULL;
    }
    if (obj->action_description && obj->action_description != obj_proto[nr].action_description) {
      FREE(obj->action_description);
      obj->action_description = NULL;
    }

    if (obj->ex_description && obj->ex_description != obj_proto[nr].ex_description) {
      for (this = obj->ex_description; this; this = next_one) {
        next_one = this->next;
        if (this->keyword) {
          FREE(this->keyword);
        }
        if (this->description) {
          FREE(this->description);
        }
        FREE(this);
      }
      obj->ex_description = NULL;
    }
  }

  FREE(obj);
  obj = NULL;
}

/* read contets of a text file, alloc space, point buf to it */
int file_to_string_alloc(char *name, char **buf)
{
  char temp[2 * MAX_STRING_LENGTH];

  memset(temp, 0, 2 * MAX_STRING_LENGTH);
  if (file_to_string(name, temp) < 0)
    return -1;

  if (*buf)
    FREE(*buf);

  if (*temp)
    *buf = strdup(temp);

  return 0;
}

/* read contents of a text file, and place in buf */
int file_to_string(char *name, char *buf)
{
  FILE *fl;
  char tmp[MAX_STRING_LENGTH + 1024];
  int buflength = 0, templength = 0;

  *buf = '\0';

  if (!(fl = fopen(name, "r"))) {
    sprintf(tmp, "Error reading %s", name);
    perror(tmp);
    return (-1);
  }
  memset(tmp, 0, MAX_STRING_LENGTH + 1024);
  do {
    fgets(tmp, MAX_STRING_LENGTH, fl);
    tmp[strlen(tmp) - 1] = '\0';/* take off the trailing \n */
    strcat(tmp, "\r\n");
    templength = strlen(tmp);

    if (!feof(fl)) {
      if (buflength + templength + 1 > MAX_STRING_LENGTH) {
        stderr_log("SYSERR: fl->strng: string too big (db.c, file_to_string)");
        *buf = '\0';
        return (-1);
      }
      strcpy(buf + buflength, tmp);
      buflength += templength;
    }
  } while (!feof(fl));

  fclose(fl);

  return (0);
}

/* clear some of the the working variables of a char */
void reset_char(struct char_data * ch)
{
  int i;
  struct consent_data *consent;
  struct consent_data *consent_next;

  for (i = 0; i < NUM_WEARS; i++)
    ch->equipment[i] = NULL;

  GET_DRAGGING(ch) = NULL;
  ch->followers = NULL;
  ch->master = NULL;
  /* ch->in_room = NOWHERE; Used for start in room */
  ch->carrying = NULL;
  if (ch->consent) {
    for (consent = ch->consent; consent; consent = consent_next) {
      consent_next = consent->next;
      FREE(consent);
    }
  }
  ch->next = NULL;
  ch->next_fighting = NULL;
  ch->next_in_room = NULL;
  FIGHTING(ch) = NULL;
  ch->char_specials.position = POS_STANDING;
  ch->mob_specials.default_pos = POS_STANDING;
  ch->char_specials.carry_weight = 0;
  ch->char_specials.carry_items = 0;
  SET_BIT(PRF_FLAGS(ch), PRF_NOCITIZEN);
  GET_SCREEN_WIDTH(ch) = 78;
  GET_SCREEN_HEIGHT(ch) = 22;

  if (GET_HIT(ch) <= 0)
    GET_HIT(ch) = 1;
  if (GET_MOVE(ch) <= 0)
    GET_MOVE(ch) = 1;
  if (GET_MANA(ch) <= 0)
    GET_MANA(ch) = 1;
}

/* clear ALL the working variables of a char; do NOT free any space alloc'ed */
void clear_char(struct char_data * ch)
{
  memset((char *) ch, 0, sizeof(struct char_data));

  GET_DRAGGING(ch) = NULL;
  ch->in_room = NOWHERE;
  GET_PFILEPOS(ch) = -1;
  GET_WAS_IN(ch) = NOWHERE;
  GET_POS(ch) = POS_STANDING;
  ch->mob_specials.default_pos = POS_STANDING;
  GET_AC(ch) = 100; /* Basic Armor */
  HUNTINGRM(ch) = NOWHERE;
  CHAR_WEARING(ch) = 0;
  HUNTING(ch) = 0;
  GET_SCREEN_WIDTH(ch) = 78;
  GET_SCREEN_HEIGHT(ch) = 22;
}

void clear_object(struct obj_data * obj)
{
  memset((char *) obj, 0, sizeof(struct obj_data));

  obj->item_number = NOTHING;
  obj->in_room = NOWHERE;
  obj->objnum = -1;
  obj->inobj = -1;
  obj->contains = NULL;
  obj->next_content = NULL;
  obj->in_obj = NULL;

}

/* initialize a new character only if class is set */
void init_char(struct char_data * ch)
{
  int i;

  /* create a player_special structure */
  if (ch->player_specials == NULL)
    CREATE(ch->player_specials, struct player_special_data, 1);

  /* *** if this is our first player --- he be God *** */

  if (GET_IDNUM(ch) == 1) {
    GET_EXP(ch) = 7000000;
    GET_LEVEL(ch) = LVL_IMMORT;

    ch->points.max_hit = 500;
    ch->points.max_mana = 100;
    ch->points.max_move = 82;
  }
  set_title(ch, NULL);

  SKILL_TIMER(ch) = 0;
  ch->player.short_descr = NULL;
  GET_LDESC(ch) = NULL;
  GET_DESC(ch) = NULL;
  GET_SCREEN_HEIGHT(ch) = 22;
  GET_SCREEN_WIDTH(ch) = 80;

  ch->player.hometown = 1;

  ch->player.time.birth = time(0);
  ch->player.time.played = 0;
  ch->player.time.logon = time(0);

  ch->points.max_mana = 100;
  ch->points.mana = GET_MAX_MANA(ch);
  ch->points.hit = GET_MAX_HIT(ch);
  ch->points.max_move = 82;
  ch->points.move = GET_MAX_MOVE(ch);
  ch->points.armor = 100;

  for (i = 1; spells[i].command[0] != '\n'; i++) {
    if (GET_LEVEL(ch) < LVL_IMMORT)
      SET_SKILL(ch, spells[i].spellindex, 0)
    else
      SET_SKILL(ch, spells[i].spellindex, 100);
  }

  ch->char_specials.saved.affected_by = 0;

  for (i = 0; i < 5; i++)
    GET_SAVE(ch, i) = 0;

  for (i = 0; i < MAX_DAM_TYPE; i++)
    GET_RESIST(ch, i) = 0;

  for (i = 0; i < 3; i++)
    GET_COND(ch, i) = (GET_LEVEL(ch) == LVL_IMMORT ? -1 : 24);
}

/* returns the real number of the room with given virtual number */
int real_room(int virtual)
{
  int bot, top, mid;

  bot = 0;
  top = top_of_world;

  /* perform binary search on world-table */
  for (;;) {
    mid = (bot + top) >> 1;

    if (bot > top)
      return -1;
    if ((world + mid)->number == virtual)
      return mid;
    if ((world + mid)->number > virtual)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}

/* returns the real number of the monster with given virtual number */
int real_mobile(int virtual)
{
  int bot, top, mid;

  bot = 0;
  top = top_of_mobt;

  /* perform binary search on mob-table */
  for (;;) {
    mid = (bot + top) / 2;

    if ((mob_index + mid)->virtual == virtual)
      return (mid);
    if (bot >= top)
      return (-1);
    if ((mob_index + mid)->virtual > virtual)
      top = mid - 1;
    else
      bot = mid + 1;
  }
}

/* returns the real number of the object with given virtual number */
int real_object(int virtual)
{
  int bot, top, mid;

  bot = 0;
  top = top_of_objt;

  /* perform binary search on obj-table */
  for (;;) {
    mid = (bot + top) / 2;

    if ((obj_index + mid)->virtual == virtual)
      return (mid);
    if (bot >= top)
      break; /* return (-1); but we wanna check for new objects */
    if ((obj_index + mid)->virtual > virtual)
      top = mid - 1;
    else
      bot = mid + 1;
  }

  for (bot = new_top_objt; bot >= top_of_objt; bot--) {
    if ((obj_index + bot)->virtual == virtual)
      return (bot);
  };

  return (-1); /* nopes, not even a new object could be found.. */
}

/* the functions */

/* This routine transfers between alpha and numeric forms of the
 *  mob_prog bitvector types. This allows the use of the words in the
 *  mob/script files.
 */

int mprog_name_to_type(char *name)
{
  if (!str_cmp(name, "in_file_prog"))
    return IN_FILE_PROG;
  if (!str_cmp(name, "act_prog"))
    return ACT_PROG;
  if (!str_cmp(name, "speech_prog"))
    return SPEECH_PROG;
  if (!str_cmp(name, "rand_prog"))
    return RAND_PROG;
  if (!str_cmp(name, "fight_prog"))
    return FIGHT_PROG;
  if (!str_cmp(name, "hitprcnt_prog"))
    return HITPRCNT_PROG;
  if (!str_cmp(name, "death_prog"))
    return DEATH_PROG;
  if (!str_cmp(name, "entry_prog"))
    return ENTRY_PROG;
  if (!str_cmp(name, "greet_prog"))
    return GREET_PROG;
  if (!str_cmp(name, "all_greet_prog"))
    return ALL_GREET_PROG;
  if (!str_cmp(name, "give_prog"))
    return GIVE_PROG;
  if (!str_cmp(name, "bribe_prog"))
    return BRIBE_PROG;
  if (!str_cmp(name, "shout_prog"))
    return SHOUT_PROG;
  if (!str_cmp(name, "holler_prog"))
    return HOLLER_PROG;
  if (!str_cmp(name, "tell_prog"))
    return TELL_PROG;
  if (!str_cmp(name, "time_prog"))
    return TIME_PROG;
  if (!str_cmp(name, "ask_prog"))
    return ASK_PROG;

  return (ERROR_PROG);
}

/*
 * Read a number from a file.
 */
int fread_number(FILE *fp)
{
  int number;
  bool sign;
  char c;

  do {
    c = getc(fp);
  } while (isspace(c));

  number = 0;

  sign = FALSE;
  if (c == '+') {
    c = getc(fp);
  } else if (c == '-') {
    sign = TRUE;
    c = getc(fp);
  }

  if (!isdigit(c)) {
    stderr_log("Fread_number: bad format.");
    fflush(NULL);
    exit(1);
  }

  while (isdigit(c)) {
    number = number * 10 + c - '0';
    c = getc(fp);
  }

  if (sign)
    number = 0 - number;

  if (c == '|')
    number += fread_number(fp);
  else if (c != ' ')
    ungetc(c, fp);

  return number;
}

/*
 * Read to end of line (for comments).
 */
void fread_to_eol(FILE *fp)
{
  char c;

  do {
    c = getc(fp);
  } while (c != '\n' && c != '\r');

  do {
    c = getc(fp);
  } while (c == '\n' || c == '\r');

  ungetc(c, fp);
  return;
}

/*
 * Read one word (into static buffer).
 */
char *fread_word(FILE *fp)
{
  static char word[MAX_INPUT_LENGTH];
  char *pword;
  char cEnd;

  do {
    cEnd = getc(fp);
  } while (isspace(cEnd));

  if (cEnd == '\'' || cEnd == '"') {
    pword = word;
  } else {
    word[0] = cEnd;
    pword = word + 1;
    cEnd = ' ';
  }

  for (; pword < word + MAX_INPUT_LENGTH; pword++) {
    *pword = getc(fp);
    if (cEnd == ' ' ? isspace(*pword) || *pword == '~' : *pword == cEnd) {
      if (cEnd == ' ' || cEnd == '~')
        ungetc(*pword, fp);
      *pword = '\0';
      return word;
    }
  }

  stderr_log("SYSERR: Fread_word: word too long.");
  fflush(NULL);
  exit(1);
  return NULL;
}

/* This routine reads in scripts of MOBprograms from a file */

MPROG_DATA* mprog_file_read(char *f, MPROG_DATA *mprg, struct index_data *pMobIndex)
{

  char MOBProgfile[MAX_INPUT_LENGTH];
  MPROG_DATA *mprg2;
  FILE *progfile;
  bool done = FALSE;

  sprintf(MOBProgfile, "%s/%s", MOB_DIR, f);

  progfile = fopen(MOBProgfile, "r");
  if (!progfile) {
    sprintf(err_buf, "Mob: %d couldnt open mobprog file", pMobIndex->virtual);
    stderr_log(err_buf);
    fflush(NULL);
    exit(1);
  }

  mprg2 = mprg;
  switch (fread_letter(progfile)) {
    case '>':
      break;
    case '|':
      stderr_log("empty mobprog file.");
      fflush(NULL);
      exit(1);
      break;
    default:
      stderr_log("in mobprog file syntax error.");
      fflush(NULL);
      exit(1);
      break;
  }

  while (!done) {
    mprg2->type = mprog_name_to_type(fread_word(progfile));
    switch (mprg2->type) {
      case ERROR_PROG:
        stderr_log("mobprog file type error");
        fflush(NULL);
        exit(1);
        break;
      case IN_FILE_PROG:
        stderr_log("mprog file contains a call to file.");
        fflush(NULL);
        exit(1);
        break;
      default:
        snprintf(buf2, MAX_STRING_LENGTH, "Error in file %s", f);
        pMobIndex->progtypes = pMobIndex->progtypes | mprg2->type;
        mprg2->arglist = fread_string(progfile, buf2);
        mprg2->comlist = fread_string(progfile, buf2);
        switch (fread_letter(progfile)) {
          case '>':
            mprg2->next = (MPROG_DATA *) malloc(sizeof(MPROG_DATA));
            mprg2 = mprg2->next;
            mprg2->next = NULL;
            break;
          case '|':
            done = TRUE;
            break;
          default:
            sprintf(err_buf, "in mobprog file %s syntax error.", f);
            stderr_log(err_buf);
            fflush(NULL);
            exit(1);
            break;
        }
        break;
    }
  }
  fclose(progfile);
  return mprg2;
}

struct index_data *get_obj_index(int vnum)
{
  int nr;
  for (nr = 0; nr <= top_of_objt; nr++) {
    if (obj_index[nr].virtual == vnum)
      return &obj_index[nr];
  }
  return NULL;
}

struct index_data *get_mob_index(int vnum)
{
  int nr;
  for (nr = 0; nr <= top_of_mobt; nr++) {
    if (mob_index[nr].virtual == vnum)
      return &mob_index[nr];
  }
  return NULL;
}

/* This procedure is responsible for reading any in_file MOBprograms.
 */

void mprog_read_programs(FILE *fp, struct index_data *pMobIndex)
{
  MPROG_DATA *mprg;
  char letter;
  bool done = FALSE;

  if ((letter = fread_letter(fp)) != '>') {
    sprintf(err_buf, "Load_mobiles: vnum %d MOBPROG char", pMobIndex->virtual);
    stderr_log(err_buf);
    fflush(NULL);
    exit(1);
  }
  pMobIndex->mobprogs = (MPROG_DATA *) malloc(sizeof(MPROG_DATA));
  mprg = pMobIndex->mobprogs;

  while (!done) {
    mprg->type = mprog_name_to_type(fread_word(fp));
    switch (mprg->type) {
      case ERROR_PROG:
        sprintf(err_buf, "Load_mobiles: vnum %d MOBPROG type.", pMobIndex->virtual);
        stderr_log(err_buf);
        fflush(NULL);
        exit(1);
        break;
      case IN_FILE_PROG:
        snprintf(buf2, MAX_STRING_LENGTH, "Mobprog for mob #%d", pMobIndex->virtual);
        mprg = mprog_file_read(fread_word(fp), mprg, pMobIndex);
        fread_to_eol(fp); /* need to strip off that silly ~*/
        switch (letter = fread_letter(fp)) {
          case '>':
            mprg->next = (MPROG_DATA *) malloc(sizeof(MPROG_DATA));
            mprg = mprg->next;
            mprg->next = NULL;
            break;
          case '|':
            mprg->next = NULL;
            fread_to_eol(fp);
            done = TRUE;
            break;
          default:
            sprintf(err_buf, "Load_mobiles: vnum %d bad MOBPROG.", pMobIndex->virtual);
            stderr_log(err_buf);
            fflush(NULL);
            exit(1);
            break;
        }
        break;
      default:
        snprintf(buf2, MAX_STRING_LENGTH, "Mobprog for mob #%d", pMobIndex->virtual);
        pMobIndex->progtypes = pMobIndex->progtypes | mprg->type;
        mprg->arglist = fread_string(fp, buf2);
        mprg->comlist = fread_string(fp, buf2);
        switch (letter = fread_letter(fp)) {
          case '>':
            mprg->next = (MPROG_DATA *) malloc(sizeof(MPROG_DATA));
            mprg = mprg->next;
            mprg->next = NULL;
            break;
          case '|':
            mprg->next = NULL;
            fread_to_eol(fp);
            done = TRUE;
            break;
          default:
            sprintf(err_buf, "Load_mobiles: vnum %d bad MOBPROG (%c).", pMobIndex->virtual, letter);
            stderr_log(err_buf);
            fflush(NULL);
            exit(1);
            break;
        }
        break;
    }
  }

  return;
}

event* get_spell_event(char *spell_event)
{
  char buf2[80];

  switch (spell_event[0]) {
    case 'A':
    case 'a':
      if (strcasecmp(spell_event, "area_dam") == 0)
        return spell_area_dam_event;
      else if (strcasecmp(spell_event, "area") == 0)
        return spell_area_event;
      else if (strcasecmp(spell_event, "area_points") == 0)
        return spell_area_points_event;
      else if (strcasecmp(spell_event, "add_dam_event") == 0)
        return spell_add_dam_event;
      else {
        snprintf(buf2, MAX_STRING_LENGTH, "Unknown event_type [%s]", spell_event);
        stderr_log(buf2);
      }
      break;
    case 'C':
    case 'c':
      if (strcasecmp(spell_event, "charm") == 0)
        return spell_charm_event;
      else if (strcasecmp(spell_event, "char") == 0)
        return spell_char_event;
      else if (strcasecmp(spell_event, "confusion") == 0)
        return spell_confusion_event;
      else if (strcasecmp(spell_event, "create_obj") == 0)
        return spell_create_obj_event;
      else if (strcasecmp(spell_event, "create_mob") == 0)
        return spell_create_mob_event;
      else if (strcasecmp(spell_event, "create_water") == 0)
        return spell_create_water_event;
      else {
        snprintf(buf2, MAX_STRING_LENGTH, "Unknown event_type [%s]", spell_event);
        stderr_log(buf2);
      }
      break;
    case 'D':
    case 'd':
      if (strcasecmp(spell_event, "dam") == 0)
        return spell_dam_event;
      else if (strcasecmp(spell_event, "dimdoor") == 0)
        return spell_dimdoor_event;
      else if (strcasecmp(spell_event, "disintegrate") == 0)
        return spell_disintegrate_event;
      else if (strcasecmp(spell_event, "dispel_magic") == 0)
        return spell_dispel_magic_event;
      else if (strcasecmp(spell_event, "destroy_inventory") == 0)
        return spell_destroy_inventory_event;
      else if (strcasecmp(spell_event, "destroy_equipment") == 0)
        return spell_destroy_equipment_event;
      else {
        snprintf(buf2, MAX_STRING_LENGTH, "Unknown event_type [%s]", spell_event);
        stderr_log(buf2);
      }
      break;
    case 'G':
    case 'g':
      if (strcasecmp(spell_event, "group") == 0)
        return spell_group_event;
      else if (strcasecmp(spell_event, "group_points") == 0)
        return spell_group_points_event;
      else {
        snprintf(buf2, MAX_STRING_LENGTH, "Unknown event_type [%s]", spell_event);
        stderr_log(buf2);
      }
      break;
    case 'I':
    case 'i':
      if (strcasecmp(spell_event, "identify") == 0)
        return spell_identify_event;
      else {
        snprintf(buf2, MAX_STRING_LENGTH, "Unknown event_type [%s]", spell_event);
        stderr_log(buf2);
      }
      break;
    case 'L':
    case 'l':
      if (strcasecmp(spell_event, "locate_obj") == 0)
        return spell_locate_obj_event;
      else {
        snprintf(buf2, MAX_STRING_LENGTH, "Unknown event_type [%s]", spell_event);
        stderr_log(buf2);
      }
      break;
    case 'M':
    case 'm':
      if (strcasecmp(spell_event, "magic_missile") == 0)
        return spell_magic_missile_event;
      else if (strcasecmp(spell_event, "magical_lock") == 0)
        return spell_magical_lock_event;
      else if (strcasecmp(spell_event, "magical_unlock") == 0)
        return spell_magical_unlock_event;
      else {
        snprintf(buf2, MAX_STRING_LENGTH, "Unknown event_type [%s]", spell_event);
        stderr_log(buf2);
      }
      break;
    case 'O':
    case 'o':
      if (strcasecmp(spell_event, "obj_char") == 0)
        return spell_obj_char_event;
      else if (strcasecmp(spell_event, "obj") == 0)
        return spell_obj_event;
      else if (strcasecmp(spell_event, "obj_room") == 0)
        return spell_obj_room_event;
      else {
        snprintf(buf2, MAX_STRING_LENGTH, "Unknown event_type [%s]", spell_event);
        stderr_log(buf2);
      }
      break;
    case 'P':
    case 'p':
      if (strcasecmp(spell_event, "prismatic_spray") == 0)
        return spell_prismatic_spray_event;
      else if (strcasecmp(spell_event, "points") == 0)
        return spell_points_event;
      else {
        snprintf(buf2, MAX_STRING_LENGTH, "Unknown event_type [%s]", spell_event);
        stderr_log(buf2);
      }
      break;
    case 'R':
    case 'r':
      if (strcasecmp(spell_event, "room") == 0)
        return spell_room_event;
      else if (strcasecmp(spell_event, "resurrection") == 0)
        return spell_resurrection_event;
      else {
        snprintf(buf2, MAX_STRING_LENGTH, "Unknown event_type [%s]", spell_event);
        stderr_log(buf2);
      }
      break;
    case 'S':
    case 's':
      if (strcasecmp(spell_event, "summon") == 0)
        return spell_summon_event;
      else {
        snprintf(buf2, MAX_STRING_LENGTH, "Unknown event_type [%s]", spell_event);
        stderr_log(buf2);
      }
      break;
    case 'T':
    case 't':
      if (strcasecmp(spell_event, "teleport") == 0)
        return spell_teleport_event;
      else if (strcasecmp(spell_event, "turn_undead") == 0)
        return spell_turn_undead_event;
      else if (strcasecmp(spell_event, "telekinesis") == 0)
        return spell_telekinesis_event;
      else {
        snprintf(buf2, MAX_STRING_LENGTH, "Unknown event_type [%s]", spell_event);
        stderr_log(buf2);
      }
      break;
    case 'W':
    case 'w':
      if (strcasecmp(spell_event, "word_recall") == 0)
        return spell_word_recall_event;
      else {
        snprintf(buf2, MAX_STRING_LENGTH, "Unknown event_type [%s]", spell_event);
        stderr_log(buf2);
      }
      break;
    default:
      snprintf(buf2, MAX_STRING_LENGTH, "Unknown event_type [%s]", spell_event);
      stderr_log(buf2);
      break;
  }

  return NULL;
}

spell* get_spell_type(char *spell_type)
{
  char buf2[80];
  ASPELL(spell_general);
  ASPELL(spell_char);
  ASPELL(spell_dam);
  ASPELL(spell_obj);
  ASPELL(spell_obj_char);
  ASPELL(spell_obj_room);

  if (strcasecmp(spell_type, "general") == 0)
    return spell_general;
  else if (strcasecmp(spell_type, "char") == 0)
    return spell_char;
  else if (strcasecmp(spell_type, "dam") == 0)
    return spell_dam;
  else if (strcasecmp(spell_type, "obj") == 0)
    return spell_obj;
  else if (strcasecmp(spell_type, "obj_char") == 0)
    return spell_obj_char;
  else if (strcasecmp(spell_type, "obj_room") == 0)
    return spell_obj_room;
  else {
    snprintf(buf2, MAX_STRING_LENGTH, "Unknown spell_type [%s]", spell_type);
    stderr_log(buf2);
  }

  return NULL;
}

void load_spells(void)
{
  FILE *f = fopen(SPELL_FILE, "r");
  char buf[256];
  char buf2[256];
  extern int numspells;
  char *p;
  char *cp;
  char tag[MAX_INPUT_LENGTH + 1];
  char tag_arguments[MAX_INPUT_LENGTH + 1];
  int spellnumber = -1;
  int i = 0;
  int val = 0;

  if (!f) {
    stderr_log("ERROR loading spells (file not found)");
    fflush(NULL);
    exit(1);
  }

  fgets(buf, 256, f);
  while (!feof(f)) {
    if (strstr(buf, "spell_begin"))
      numspells++;
    fgets(buf, 256, f);
  }
  rewind(f);

  snprintf(buf, MAX_STRING_LENGTH, "   Found %d spells/skills.", numspells - 2);
  stderr_log(buf);
  NumSpellsDefined = numspells - 2;

  CREATE(spells, struct spell_info_type, numspells);
  /*
   memset(spells, 0, numspells*sizeof(struct spell_info_type));
   */

  while (get_line(f, buf)) {
    parse_pline(buf, tag, tag_arguments);
    while ((p = strrchr(tag_arguments, '\n')) != NULL)
      *p = '\0';

    val = atoi(tag_arguments);
    switch (tag[0]) {
      case 'a':
        if (strcasecmp(tag, "aggressive") == 0)
          spells[spellnumber].aggressive = val;
        else if (strcasecmp(tag, "accum_affect") == 0)
          spells[spellnumber].accum_affect = val;
        else if (strcasecmp(tag, "avg_affect") == 0)
          spells[spellnumber].avg_affect = val;
        else if (strcasecmp(tag, "accum_duration") == 0)
          spells[spellnumber].accum_duration = val;
        else if (strcasecmp(tag, "avg_duration") == 0)
          spells[spellnumber].avg_duration = val;
        else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown tag [%s]", tag);
          stderr_log(buf2);
        }
        break;
      case 'c':
        if (strcasecmp(tag, "class_lvl_list") == 0) {
          p = tag_arguments;
          i = 0;
          while ((cp = strtok(p, " ")) != NULL) {
            val = atoi(cp);
            spells[spellnumber].min_level[i] = val ? val : 51;
            i++;
            p = NULL;
          }
        } else if (strcasecmp(tag, "cost_multiplier") == 0) {
          spells[spellnumber].cost_multiplier = val;
        } else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown tag [%s]", tag);
          stderr_log(buf2);
        }
        break;
      case 'd':
        if (strcasecmp(tag, "dice_add") == 0)
          spells[spellnumber].dice_add = val;
        else if (strcasecmp(tag, "dice_add2") == 0)
          spells[spellnumber].dice_add2 = val;
        else if (strcasecmp(tag, "dice_limit") == 0)
          spells[spellnumber].dice_limit = val;
        else if (strcasecmp(tag, "dice_limit2") == 0)
          spells[spellnumber].dice_limit2 = val;
        else if (strcasecmp(tag, "difficulty") == 0)
          spells[spellnumber].difficulty = val;
        else if (strcasecmp(tag, "delay") == 0) {
          spells[spellnumber].delay = val;
        } else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown tag [%s]", tag);
          stderr_log(buf2);
        }
        break;
      case 'i':
        if (strcasecmp(tag, "invisible") == 0)
          spells[spellnumber].invisible = val;
        else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown tag [%s]", tag);
          stderr_log(buf2);
        }
        break;
      case 'm':
        if (strcasecmp(tag, "min_position") == 0)
          spells[spellnumber].min_position = val;
        else if (strcasecmp(tag, "max_mana") == 0)
          spells[spellnumber].mana_max = val;
        else if (strcasecmp(tag, "min_mana") == 0)
          spells[spellnumber].mana_min = val;
        else if (strcasecmp(tag, "mana_chg") == 0)
          spells[spellnumber].mana_change = val;
        else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown tag [%s]", tag);
          stderr_log(buf2);
        }
        break;
      case 'n':
        if (strcasecmp(tag, "num_dice") == 0)
          spells[spellnumber].num_dice = val;
        else if (strcasecmp(tag, "num_dice2") == 0)
          spells[spellnumber].num_dice2 = val;
        else if (strcasecmp(tag, "npc_defense_flags") == 0)
          spells[spellnumber].npc_defense_flags = asciiflag_conv(tag_arguments);
        else if (strcasecmp(tag, "npc_offense_flags") == 0)
          spells[spellnumber].npc_offense_flags = asciiflag_conv(tag_arguments);
        else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown tag [%s]", tag);
          stderr_log(buf2);
        }
        break;
      case 'o':
        if (strcasecmp(tag, "obj_affs_loc") == 0) {
          p = tag_arguments;
          i = 0;
          while ((cp = strtok(p, " ")) != NULL) {
            spells[spellnumber].obj_aff[i].location = atoi(cp);
            i++;
            spells[spellnumber].num_obj_aff = i;
            p = NULL;
          }
        } else if (strcasecmp(tag, "obj_affs_mod") == 0) {
          p = tag_arguments;
          i = 0;
          while ((cp = strtok(p, " ")) != NULL) {
            spells[spellnumber].obj_aff[i].modifier = atoi(cp);
            i++;
            p = NULL;
          }
        } else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown tag [%s]", tag);
          stderr_log(buf2);
        }
        break;
      case 'p':
        if (strcasecmp(tag, "prime_stat") == 0) {
          spells[spellnumber].prime_stat = val;
        } else if (strcasecmp(tag, "plr_affs_loc") == 0) {
          p = tag_arguments;
          i = 0;
          while ((cp = strtok(p, " ")) != NULL) {
            spells[spellnumber].plr_aff[i].location = atoi(cp);
            i++;
            spells[spellnumber].num_plr_aff = i;
            p = NULL;
          }
        } else if (strcasecmp(tag, "plr_affs_mod") == 0) {
          p = tag_arguments;
          i = 0;
          while ((cp = strtok(p, " ")) != NULL) {
            spells[spellnumber].plr_aff[i].modifier = atoi(cp);
            i++;
            p = NULL;
          }
        } else if (strcasecmp(tag, "point_loc") == 0)
          spells[spellnumber].point_loc = val;
        else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown tag [%s]", tag);
          stderr_log(buf2);
        }
        break;
      case 'q':
        if (strcasecmp(tag, "quest_only") == 0)
          spells[spellnumber].quest_only = val;
        else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown tag [%s]", tag);
          stderr_log(buf2);
        }
        break;
      case 'r':
        if (strcasecmp(tag, "resist_type") == 0)
          spells[spellnumber].resist_type = val;
        else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown tag [%s]", tag);
          stderr_log(buf2);
        }
        break;
      case 's':
        if (strcasecmp(tag, "second_stat") == 0) {
          spells[spellnumber].second_stat = val;
        } else if (strcasecmp(tag, "spell_begin") == 0) {
          spellnumber++;
          spells[spellnumber].spellindex = spellnumber;
        } else if (strcasecmp(tag, "spell_name") == 0) {
          if (strcasecmp(tag_arguments, "<RETURN>") == 0)
            spells[spellnumber].command = strdup("\n");
          else
            spells[spellnumber].command = strdup(tag_arguments);
        } else if (strcasecmp(tag, "spell_type") == 0)
          spells[spellnumber].spell_pointer = get_spell_type(tag_arguments);
        else if (strcasecmp(tag, "spell_event") == 0)
          spells[spellnumber].event_pointer = get_spell_event(tag_arguments);
        else if (strcasecmp(tag, "spell_realm") == 0)
          spells[spellnumber].realm = val;
        else if (strcasecmp(tag, "size_dice") == 0)
          spells[spellnumber].size_dice = val;
        else if (strcasecmp(tag, "size_dice2") == 0)
          spells[spellnumber].size_dice2 = val;
        else if (strcasecmp(tag, "size_limit") == 0)
          spells[spellnumber].size_limit = val;
        else if (strcasecmp(tag, "size_limit2") == 0)
          spells[spellnumber].size_limit2 = val;
        else if (strcasecmp(tag, "spell_duration") == 0)
          spells[spellnumber].spell_duration = val;
        else if (strcasecmp(tag, "saving_throw") == 0)
          spells[spellnumber].saving_throw = val;
        else if (strcasecmp(tag, "spell_obj_bit") == 0)
          spells[spellnumber].spell_obj_bit = asciiflag_conv(tag_arguments);
        else if (strcasecmp(tag, "spell_plr_bit") == 0)
          spells[spellnumber].spell_plr_bit = asciiflag_conv(tag_arguments);
        else if (strcasecmp(tag, "spell_plr_bit2") == 0)
          spells[spellnumber].spell_plr_bit2 = asciiflag_conv(tag_arguments);
        else if (strcasecmp(tag, "spell_plr_bit3") == 0)
          spells[spellnumber].spell_plr_bit3 = asciiflag_conv(tag_arguments);
        else if (strcasecmp(tag, "spell_room_bit") == 0)
          spells[spellnumber].spell_room_bit = asciiflag_conv(tag_arguments);
        else if (strcasecmp(tag, "send_to_char") == 0)
          spells[spellnumber].send_to_char = strdup(tag_arguments);
        else if (strcasecmp(tag, "send_to_vict") == 0)
          spells[spellnumber].send_to_vict = strdup(tag_arguments);
        else if (strcasecmp(tag, "send_to_room") == 0)
          spells[spellnumber].send_to_room = strdup(tag_arguments);
        else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown tag [%s]", tag);
          stderr_log(buf2);
        }
        break;
      case 'u':
        if (strcasecmp(tag, "unaffect") == 0)
          spells[spellnumber].unaffect = strdup(tag_arguments);
        else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown tag [%s]", tag);
          stderr_log(buf2);
        }
        break;
      case 'v':
        if (strcasecmp(tag, "vnum_list") == 0) {
          p = tag_arguments;
          i = 0;
          while ((cp = strtok(p, " ")) != NULL) {
            spells[spellnumber].vnum_list[i] = atoi(cp);
            i++;
            p = NULL;
          }
        } else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown tag [%s]", tag);
          stderr_log(buf2);
        }
        break;
      case 'w':
        if (strcasecmp(tag, "wear_off_msg") == 0)
          spells[spellnumber].wear_off = strdup(tag_arguments);
        else {
          snprintf(buf2, MAX_STRING_LENGTH, "Unknown tag [%s]", tag);
          stderr_log(buf2);
        }
        break;
      default:
        snprintf(buf2, MAX_STRING_LENGTH, "Unknown tag [%s]", tag);
        stderr_log(buf2);
        break;
    }
  }

  fclose(f);
  qsort(((struct spell_info_type *) spells + 1), numspells - 2, sizeof(struct spell_info_type), spell_comp);
  for (i = 0;; i++) {
    if (spells[i].command[0] != '\n') {
      /* convert values for spell realms */
      switch (spells[i].realm) {
        case 0:
          spells[i].realm = spells[find_skill_num("general realm")].spellindex;
          break;
        case 1:
          spells[i].realm = spells[find_skill_num("elemental realm")].spellindex;
          break;
        case 2:
          spells[i].realm = spells[find_skill_num("summoning realm")].spellindex;
          break;
        case 3:
          spells[i].realm = spells[find_skill_num("healing realm")].spellindex;
          break;
        case 4:
          spells[i].realm = spells[find_skill_num("divination realm")].spellindex;
          break;
        case 5:
          spells[i].realm = spells[find_skill_num("protection realm")].spellindex;
          break;
        case 6:
          spells[i].realm = spells[find_skill_num("creation realm")].spellindex;
          break;
        case 7:
          spells[i].realm = spells[find_skill_num("enchantment realm")].spellindex;
          break;
        default:
          spells[i].realm = spells[find_skill_num("general realm")].spellindex;
          break;
      }
    } else
      break;
  }
}

int spell_comp(const void* element1, const void* element2)
{
  return strcasecmp(((struct spell_info_type *) element1)->command, ((struct spell_info_type *) element2)->command);
}
