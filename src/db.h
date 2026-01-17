/* ************************************************************************
 *   File: db.h                                          Part of CircleMUD *
 *  Usage: header file for database handling                               *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

/* arbitrary constants used by index_boot() (must be unique) */
#define DB_BOOT_WLD 0
#define DB_BOOT_MOB 1
#define DB_BOOT_OBJ 2
#define DB_BOOT_ZON 3
#define DB_BOOT_SHP 4
#define DB_BOOT_QST 5

/* defines for the help system */
#define HELP_HELP    1
#define HELP_WIZHELP 2

/* names of various files and directories */
#define INDEX_FILE  "index"      /* index of world files		*/
#define MINDEX_FILE "index.mini" /* ... and for mini-mud-mode	*/
#define WLD_PREFIX  "world/wld"  /* room definitions		*/
#define MOB_PREFIX  "world/mob"  /* monster prototypes		*/
#define OBJ_PREFIX  "world/obj"  /* object prototypes		*/
#define ZON_PREFIX  "world/zon"  /* zon defs & command tables	*/
#define SHP_PREFIX  "world/shp"  /* shop definitions		*/
#define MAZE_PREFIX "world/maz"  /* maze definitions		*/
#define QST_PREFIX  "world/qst"  /* quest definitions		*/
#define MOB_DIR     "world/prg"  /* Mob programs dir             */

#define CREDITS_FILE       "text/credits"        /* for the 'credits' command	*/
#define NEWS_FILE          "text/news"           /* for the 'news' command	*/
#define MOTD_FILE          "text/motd"           /* messages of the day / mortal	*/
#define NMOTD_FILE         "text/nmotd"          /* messages of the day / newbie	*/
#define IMOTD_FILE         "text/imotd"          /* messages of the day / immort	*/
#define HELP_KWRD_FILE     "text/help_table"     /* for HELP <keywrd>		*/
#define WIZ_HELP_KWRD_FILE "text/wiz_help_table" /* for HELP <keywrd>*/
#define HELP_PAGE_FILE     "text/help"           /* for HELP <CR>		*/
#define INFO_FILE          "text/info"           /* for INFO			*/
#define WIZLIST_FILE       "text/wizlist"        /* for WIZLIST			*/
#define IMMLIST_FILE       "text/immlist"        /* for IMMLIST			*/
#define BACKGROUND_FILE    "text/background"     /* for the background story	*/
#define POLICIES_FILE      "text/policies"       /* player policies/rules	*/
#define NAMEPOL_FILE       "text/namepol"        /* name policies                */
#define SIGNOFF_FILE       "text/signoff"        /* logout message               */
#define HANDBOOK_FILE      "text/handbook"       /* handbook for new immorts	*/
#define QUEST_FILE         "text/quest"          /* List of quest masters        */

#define HELPN_FILE    "text/helpneeded" /* for the 'show help'-command	*/
#define IDEA_FILE     "text/ideas"      /* for the 'idea'-command	*/
#define TYPO_FILE     "text/typos"      /*         'typo'		*/
#define BUG_FILE      "text/bugs"       /*         'bug'		*/
#define TODO_FILE     "text/todo"       /* list containing items left to do */
#define MESS_FILE     "misc/messages"   /* damage messages		*/
#define ACTD_FILE     "misc/actd"       /* ACTD messages                */
#define SOCMESS_FILE  "misc/socials"    /* messgs for social acts	*/
#define XNAME_FILE    "misc/xnames"     /* invalid name substrings	*/
#define DECLINED_FILE "misc/declined"   /* declined name substrings	*/
#define SPELL_FILE    "misc/spells"     /* spell definitions		*/

#define PLAYER_FILE   "etc/players"   /* the player database		*/
#define MAIL_FILE     "etc/plrmail"   /* for the mudmail system	*/
#define BAN_FILE      "etc/badsites"  /* for the siteban system	*/
#define HCONTROL_FILE "etc/hcontrol"  /* for the house system		*/
#define AUCTION_FILE  "etc/auction"   /* for the auctioning system    */
#define QIC_FILE      "etc/qicdb"     /* for the QIC system           */
#define CORPSE_FILE   "etc/corpses"   /* corpse save file             */
#define ID_FILE       "plrdata/plrid" /* Next Player Id stored here   */
#define GREET1_FILE   "text/greet1"   /* This is the greet file ANSI  */
#define GREET2_FILE   "text/greet2"   /* This is the greet file !ANSI */
#define MENU_FILE     "text/menu"     /* This is the menu             */

/* public procedures in db.c */
void boot_db(void);
int create_entry(char *name);
void zone_update(void);
int real_room(int virtual);
char *fread_string(FILE *fl, char *error);
long get_id_by_name(char *name);
char *get_name_by_id(long id);

int load_char_text(char *name, struct char_data *char_element);
void save_char_text(struct char_data *ch, sh_int load_room);
void save_text(struct char_data *ch);
void init_char(struct char_data *ch);
struct char_data *create_char(void);
struct char_data *read_mobile(int nr, int type);
int real_mobile(int virtual);
int vnum_mobile(char *searchname, struct char_data *ch);
void clear_char(struct char_data *ch);
void reset_char(struct char_data *ch);
void free_char(struct char_data *ch);

struct obj_data *create_obj(void);
void clear_object(struct obj_data *obj);
void free_obj(struct obj_data *obj);
void free_room(struct room_data *room);
void free_obj_q(struct obj_data *obj);
int real_object(int virtual);
struct obj_data *read_object(int nr, int type);
struct obj_data *read_object_q(int nr, int type);
int vnum_object(char *searchname, struct char_data *ch);
int vnum_type(char *searchtype, struct char_data *ch);

#define REAL    0
#define VIRTUAL 1
#define NOEQUIP (1 << 5) /* if bit 5 is set, dont equip char */

/* for queueing zones for update   */
struct reset_q_element {
  int zone_to_reset; /* ref to zone_data */
  struct reset_q_element *next;
};

/* structure for the update queue     */
struct reset_q_type {
  struct reset_q_element *head;
  struct reset_q_element *tail;
};

struct player_index_element {
  char *name;
  long id;
};

struct help_index_element {
  char *keyword;
  long pos;
};

/* don't change these */
#define BAN_NOT    0
#define BAN_NEW    1
#define BAN_SELECT 2
#define BAN_ALL    3
#define BAN_OUTLAW 4
#define BAN_FROZEN 5

#define BANNED_SITE_LENGTH 50
struct ban_list_element {
  char site[BANNED_SITE_LENGTH + 1];
  int type;
  time_t date;
  char name[MAX_NAME_LENGTH + 1];
  struct ban_list_element *next;
};

/* global buffering system */

#ifdef __DB_C__
char buf[MAX_STRING_LENGTH];
char buf1[MAX_STRING_LENGTH];
char buf2[MAX_STRING_LENGTH];
char buf3[MAX_STRING_LENGTH];
char arg[MAX_STRING_LENGTH];
char string_buf[MAX_STRING_LENGTH * 2];
char logbuffer[256];
#else
extern char string_buf[MAX_STRING_LENGTH * 2];
extern char buf[MAX_STRING_LENGTH];
extern char buf1[MAX_STRING_LENGTH];
extern char buf2[MAX_STRING_LENGTH];
extern char buf3[MAX_STRING_LENGTH];
extern char arg[MAX_STRING_LENGTH];
extern char logbuffer[256];
#endif

#ifndef __CONFIG_C__
extern char *OK;
extern char *NOPERSON;
#endif
