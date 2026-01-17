#include "comm.h"
#include "db.h"
#include "event.h"
#include "handler.h"
#include "interpreter.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * This file attempts to concentrate most of the code which must be changed
 * in order for new classes to be added.  If you're adding a new class,
 * you should go through this entire file from beginning to end and add
 * the appropriate new special cases for your new class.
 */

extern struct spell_info_type *spells;
extern struct room_data *world;
extern struct zone_data *zone_table;
/* Names first */

const char *class_abbrevs[] = {"War", "Rog", "Thi", "Sor", "Wiz", "Enc", "Con", "Nec", "Cle",
                               "Pri", "Sha", "Mon", "Dru", "Asn", "Bar", "Ran", "Mer", "\n"};

const char *pc_class_types[] = {"Warrior",  "Rogue",       "Thief",  "Sorcerer", "Wizard",    "Enchanter",
                                "Conjurer", "Necromancer", "Cleric", "Priest",   "Shaman",    "Monk",
                                "Druid",    "Assassin",    "Bard",   "Ranger",   "Mercenary", "\n"};

const char *pc_race_types_color[] = {"{RUndefined", "{wHuman",    "{gTroll", "{yOgre",     "{YDwarf",
                                     "{GElf",       "{CHalf-Elf", "{RGnome", "{mHalfling", "\n"};

const char *pc_race_types[] = {"Undefined", "Human",    "Troll", "Ogre",     "Dwarf",
                               "Elf",       "Half-Elf", "Gnome", "Halfling", "\n"};

const char *pc_race_names[] = {"the Undefined", "the Human",      "the Troll",   "the Ogre",     "the Dwarven",
                               "the Elven",     "the Half-Elven", "the Gnomish", "the Halfling", "\n"};

/* classes allowed to a given race */
const int pc_racial_classes[NUM_RACES][NUM_CLASSES] = {
    /* war, rog, thi, sor, wiz, enc, con, nec, cle, pri, sha, mon, dru, ass, bar, ran, mer */
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},  /*RACE_UNDEFINED*/
    {1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1},  /*RACE_HUMAN*/
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1},  /*RACE_TROLL*/
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1},  /*RACE_OGRE*/
    {1, 0, 1, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1},  /*RACE_DWARF*/
    {1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0},  /*RACE_ELF*/
    {1, 1, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1},  /*RACE_HALFELF*/
    {1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0},  /*RACE_GNOME*/
    {1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0}}; /*RACE_HALFLING*/

/* minimum attributes for a given class */
const int pc_class_attrib[NUM_CLASSES][8] = {
    /* STR, INT, WIS, DEX, CON, AGI */
    {55, 0, 0, 0, 40, 35}, /* Warrior */
    {0, 0, 0, 60, 0, 55},  /* Rogue */
    {0, 0, 0, 60, 0, 55},  /* Thief */
    {0, 65, 0, 55, 0, 0},  /* Sorcerer */
    {0, 65, 0, 55, 0, 0},  /* Wizard */
    {0, 65, 0, 0, 45, 0},  /* Enchanter */
    {0, 65, 0, 0, 45, 0},  /* Conjurer */
    {910, 910, 910, 910, 910, 910},
    /* Necromancer was 0, 70, 60, 0, 40, 0*/ /* set back to original when implemented */
    {0, 45, 60, 0, 0, 0},                    /* Cleric */
    {910, 910, 910, 910, 910, 910},          /* Priest was 0, 45, 60, 0, 0, 0*/
    {0, 40, 45, 0, 0, 0},                    /* Shaman  */
    {0, 0, 0, 55, 40, 60},                   /* Monk was 0, 0, 0, 55, 40, 70*/
    {0, 50, 60, 0, 20, 0},                   /* Druid */
    {0, 0, 0, 60, 30, 60},                   /* Assassin */
    {910, 910, 910, 910, 910, 910},          /* Bard was 0, 65, 0, 60, 0, 0 */
    {910, 910, 910, 910, 910, 910},          /* Ranger was 50, 45, 0, 40, 0, 35*/
    {50, 0, 0, 50, 40, 0}                    /* Mercenary  */
};

/* The menu for choosing a race in interpreter.c: */
const char *race_menu = "\r\n"
                        "Select a race (allowed classes):\r\n"
                        "  [1] Human            Warrior, Rogue, Sorcerer,\r\n"
                        "                       Cleric, Conjurer, Monk, Druid,\r\n"
                        "                       Assassin, Mercenary, Enchanter\r\n"
                        "  [2] Troll            Warrior, Shaman, Mercenary\r\n"
                        "  [3] Ogre             Warrior, Shaman, Mercenary\r\n"
                        "  [4] Dwarf            Warrior, Thief, Conjurer, Cleric,\r\n"
                        "                       Mercenary\r\n"
                        "  [5] Elf              Warrior, Rogue, Wizard, Enchanter,\r\n"
                        "                       Cleric\r\n"
                        "  [6] Half-Elf         Warrior, Rogue, Sorcerer, Enchanter,\r\n"
                        "                       Cleric, Monk, Druid, Assassin,\r\n"
                        "                       Mercenary\r\n"
                        "  [7] Gnome            Warrior, Thief, Wizard, Conjurer\r\n"
                        "  [8] Halfling         Warrior, Thief, Wizard, Assassin\r\n";

bool check_class(struct char_data *ch, int class) {
  if (!pc_racial_classes[GET_RACE(ch)][class])
    return FALSE;
  if (GET_STR(ch) < pc_class_attrib[class][0])
    return FALSE;
  if (GET_INT(ch) < pc_class_attrib[class][1])
    return FALSE;
  if (GET_WIS(ch) < pc_class_attrib[class][2])
    return FALSE;
  if (GET_DEX(ch) < pc_class_attrib[class][3])
    return FALSE;
  if (GET_CON(ch) < pc_class_attrib[class][4])
    return FALSE;
  if (GET_AGI(ch) < pc_class_attrib[class][5])
    return FALSE;

  return TRUE;
}

void disp_class_menu(struct descriptor_data *d) {

  SEND_TO_Q("These are the classes available to you.\r\n\r\n", d);
  if (check_class(d->character, CLASS_WARRIOR))
    SEND_TO_Q("   [Wa] Warrior\r\n", d);
  if (check_class(d->character, CLASS_ROGUE))
    SEND_TO_Q("   [R]  Rogue\r\n", d);
  if (check_class(d->character, CLASS_THIEF))
    SEND_TO_Q("   [T]  Thief\r\n", d);
  if (check_class(d->character, CLASS_SORCERER))
    SEND_TO_Q("   [S]  Sorcerer\r\n", d);
  if (check_class(d->character, CLASS_WIZARD))
    SEND_TO_Q("   [W]  Wizard\r\n", d);
  if (check_class(d->character, CLASS_ENCHANTER))
    SEND_TO_Q("   [E]  Enchanter\r\n", d);
  if (check_class(d->character, CLASS_CONJURER))
    SEND_TO_Q("   [Co] Conjurer\r\n", d);
  if (check_class(d->character, CLASS_NECROMANCER))
    SEND_TO_Q("   [N]  Necromancer\r\n", d);
  if (check_class(d->character, CLASS_CLERIC))
    SEND_TO_Q("   [C]  Cleric\r\n", d);
  if (check_class(d->character, CLASS_PRIEST))
    SEND_TO_Q("   [P]  Priest\r\n", d);
  if (check_class(d->character, CLASS_SHAMAN))
    SEND_TO_Q("   [Sh] Shaman\r\n", d);
  if (check_class(d->character, CLASS_MONK))
    SEND_TO_Q("   [Mo] Monk\r\n", d);
  if (check_class(d->character, CLASS_DRUID))
    SEND_TO_Q("   [D]  Druid\r\n", d);
  if (check_class(d->character, CLASS_ASSASSIN))
    SEND_TO_Q("   [A]  Assassin\r\n", d);
  if (check_class(d->character, CLASS_BARD))
    SEND_TO_Q("   [B]  Bard\r\n", d);
  if (check_class(d->character, CLASS_RANGER))
    SEND_TO_Q("   [Ra] Ranger\r\n", d);
  if (check_class(d->character, CLASS_MERCENARY))
    SEND_TO_Q("   [M]  Mercenary\r\n", d);
}

/*
 * The code to interpret a class letter (used in interpreter.c when a
 * new character is selecting a class).
 */
int parse_class_spec(char *arg) {
  int a;

  a = arg[0];
  a = LOWER(a);

  switch (a) {
  case 'c':
    if (arg[1] == 'o' || arg[1] == 'O')
      return CLASS_CONJURER;
    else
      return CLASS_CLERIC;
  case 'w':
    if (arg[1] == 'a' || arg[1] == 'A')
      return CLASS_WARRIOR;
    else
      return CLASS_WIZARD;
  case 't':
    return CLASS_THIEF;
  case 'r':
    if (arg[1] == 'a' || arg[1] == 'A')
      return CLASS_RANGER;
    else
      return CLASS_ROGUE;
  case 'b':
    return CLASS_BARD;
  case 'a':
    return CLASS_ASSASSIN;
  case 's':
    if (arg[1] == 'h' || arg[1] == 'H')
      return CLASS_SHAMAN;
    else
      return CLASS_SORCERER;
  case 'n':
    return CLASS_NECROMANCER;
  case 'e':
    return CLASS_ENCHANTER;
  case 'd':
    return CLASS_DRUID;
  case 'p':
    return CLASS_PRIEST;
  case 'm':
    if (arg[1] == 'o' || arg[1] == 'O')
      return CLASS_MONK;
    else
      return CLASS_MERCENARY;
  default:
    return CLASS_UNDEFINED;
  }
}

int parse_race_spec(char *arg) {
  int a;

  a = arg[0];
  a = LOWER(a);

  switch (a) {
  case 'd':
    return RACE_DWARF;
  case 'e':
    return RACE_ELF;
  case 'g':
    return RACE_GNOME;
  case 'h':
    if (arg[1] == 'u' || arg[1] == 'U')
      return RACE_HUMAN;
    else if (arg[4] == 'l' || arg[4] == 'L')
      return RACE_HALFLING;
    else
      return RACE_HALFELF;
  case 'o':
    return RACE_OGRE;
  case 't':
    return RACE_TROLL;
  default:
    return RACE_UNDEFINED;
  }
}

int is_class(char *arg) {

  if (strncasecmp(pc_class_types[CLASS_CONJURER], arg, strlen(arg)) == 0)
    return CLASS_CONJURER;
  else if (strncasecmp(pc_class_types[CLASS_CLERIC], arg, strlen(arg)) == 0)
    return CLASS_CLERIC;
  else if (strncasecmp(pc_class_types[CLASS_WARRIOR], arg, strlen(arg)) == 0)
    return CLASS_WARRIOR;
  else if (strncasecmp(pc_class_types[CLASS_WIZARD], arg, strlen(arg)) == 0)
    return CLASS_WIZARD;
  else if (strncasecmp(pc_class_types[CLASS_THIEF], arg, strlen(arg)) == 0)
    return CLASS_THIEF;
  else if (strncasecmp(pc_class_types[CLASS_RANGER], arg, strlen(arg)) == 0)
    return CLASS_RANGER;
  else if (strncasecmp(pc_class_types[CLASS_ROGUE], arg, strlen(arg)) == 0)
    return CLASS_ROGUE;
  else if (strncasecmp(pc_class_types[CLASS_BARD], arg, strlen(arg)) == 0)
    return CLASS_BARD;
  else if (strncasecmp(pc_class_types[CLASS_ASSASSIN], arg, strlen(arg)) == 0)
    return CLASS_ASSASSIN;
  else if (strncasecmp(pc_class_types[CLASS_SHAMAN], arg, strlen(arg)) == 0)
    return CLASS_SHAMAN;
  else if (strncasecmp(pc_class_types[CLASS_SORCERER], arg, strlen(arg)) == 0)
    return CLASS_SORCERER;
  else if (strncasecmp(pc_class_types[CLASS_NECROMANCER], arg, strlen(arg)) == 0)
    return CLASS_NECROMANCER;
  else if (strncasecmp(pc_class_types[CLASS_ENCHANTER], arg, strlen(arg)) == 0)
    return CLASS_ENCHANTER;
  else if (strncasecmp(pc_class_types[CLASS_DRUID], arg, strlen(arg)) == 0)
    return CLASS_DRUID;
  else if (strncasecmp(pc_class_types[CLASS_PRIEST], arg, strlen(arg)) == 0)
    return CLASS_PRIEST;
  else if (strncasecmp(pc_class_types[CLASS_MONK], arg, strlen(arg)) == 0)
    return CLASS_MONK;
  else if (strncasecmp(pc_class_types[CLASS_MERCENARY], arg, strlen(arg)) == 0)
    return CLASS_MERCENARY;

  return CLASS_UNDEFINED;
}

int is_race(char *arg) {

  if (strncasecmp(pc_race_types[RACE_HUMAN], arg, strlen(arg)) == 0)
    return RACE_HUMAN;
  else if (strncasecmp(pc_race_types[RACE_TROLL], arg, strlen(arg)) == 0)
    return RACE_TROLL;
  else if (strncasecmp(pc_race_types[RACE_OGRE], arg, strlen(arg)) == 0)
    return RACE_OGRE;
  else if (strncasecmp(pc_race_types[RACE_DWARF], arg, strlen(arg)) == 0)
    return RACE_DWARF;
  else if (strncasecmp(pc_race_types[RACE_ELF], arg, strlen(arg)) == 0)
    return RACE_ELF;
  else if (strncasecmp(pc_race_types[RACE_HALFELF], arg, strlen(arg)) == 0)
    return RACE_HALFELF;
  else if (strncasecmp(pc_race_types[RACE_GNOME], arg, strlen(arg)) == 0)
    return RACE_GNOME;
  else if (strncasecmp(pc_race_types[RACE_HALFLING], arg, strlen(arg)) == 0)
    return RACE_HALFLING;

  return RACE_UNDEFINED;
}

/*
 * These are definitions which control the guildmasters for each class.
 *
 * The first field (top line) controls the highest percentage skill level
 * a character of the class is allowed to attain in any skill.  (After
 * this level, attempts to practice will say "You are already learned in
 * this area."
 *
 * The second line controls the maximum percent gain in learnedness a
 * character is allowed per practice -- in other words, if the random
 * die throw comes out higher than this number, the gain will only be
 * this number instead.
 *
 * The third line controls the minimu percent gain in learnedness a
 * charcter is allowed per practice -- in other words, if the random
 * die throw comes out below this number, the gain will be set up to
 * this number.
 *
 * The fourth line simply sets whether the character knows 'spells'
 * or 'skills'.  This does not affect anything except the message given
 * to the character when trying to practice (i.e. "You know of the
 * following spells" vs. "You know of the following skills"
 */

#define SP 0
#define SK 1

/* #define LEARNED_LEVEL  0  % known which is considered "learned" */
/* #define MAX_PER_PRAC    1  max percent gain in skill per practice */
/* #define MIN_PER_PRAC    2  min percent gain in skill per practice */
/* #define PRAC_TYPE    3  should it say 'spell' or 'skill'?  */

int prac_params[4][NUM_CLASSES] = {
    /* wa   ro   th   so   wi   en   co   ne   cl   pr   sh   mo   dr   as   ba   ra   me*/
    {95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95}, /* learned level */
    {5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5},                  /* max per practice */
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},                  /* min per practice */
    {SK, SK, SK, SP, SP, SP, SP, SP, SP, SP, SP, SK, SP, SK, SP, SK, SK}  /* prac name */
};

int max_lvl_skill[2][5]
                 [52] = {{
                             /* max learns by prac per lvl */
                             {0,  15, 19, 23, 26, 30, 33, 37, 40, 44, 47, 51, 54, 58, 61, 65, 68, 72,
                              76, 79, 83, 87, 90, 93, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95,
                              95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 100}, /* EASY skills */
                             {0,  15, 17, 18, 20, 22, 23, 25, 27, 30, 32, 35, 37, 40, 42, 45, 47, 50,
                              52, 55, 57, 60, 62, 65, 67, 70, 72, 75, 77, 80, 82, 85, 87, 90, 92, 95,
                              95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 100}, /* AVERAGE skills */
                             {0,  15, 16, 17, 18, 19, 20, 21, 22, 23, 25, 27, 29, 31, 33, 35, 37, 39,
                              41, 43, 45, 47, 49, 51, 53, 55, 57, 59, 61, 63, 65, 67, 69, 71, 73, 75,
                              77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 85, 85, 86, 87, 88, 100}, /* HARD skills */
                             {0,  15, 16, 17, 18, 19, 20, 21, 22, 23, 25, 27, 29, 31, 33, 35, 37, 39,
                              41, 43, 45, 47, 49, 51, 53, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65,
                              66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 100}, /* VRHARD skills */
                             {0,  10, 13, 15, 17, 19, 20, 21, 22, 23, 24, 26, 28, 29, 31, 32, 35, 37,
                              40, 42, 43, 44, 46, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
                              61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 100} /* EXHARD skills */
                         },
                         {
                             /* max learns by use per lvl */
                             {0,  20, 24, 28, 31, 35, 38, 42, 45, 49, 52, 56, 59, 63, 66,  70, 73, 77,
                              81, 84, 88, 92, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95,  95, 95, 95,
                              95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 100, 100}, /*EASY skills */
                             {0,  20, 22, 23, 25, 27, 28, 30, 32, 35, 37, 40, 42, 45, 47,  50, 52, 55,
                              57, 60, 62, 65, 67, 70, 72, 75, 77, 80, 82, 85, 87, 90, 92,  95, 95, 95,
                              95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 95, 100, 100}, /* AVERAGE skills */
                             {0,  20, 21, 22, 23, 24, 25, 26, 27, 28, 30, 32, 34, 36, 38,  40, 42, 44,
                              46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70, 72, 74,  76, 78, 80,
                              82, 84, 86, 88, 90, 92, 94, 95, 95, 95, 95, 95, 95, 95, 100, 100}, /* HARD skills */
                             {0,  15, 16, 17, 18, 19, 20, 21, 22, 23, 25, 27, 29, 31, 33,  35, 37, 39,
                              41, 43, 45, 47, 49, 51, 53, 55, 57, 59, 61, 63, 65, 67, 69,  71, 73, 75,
                              77, 79, 81, 83, 85, 86, 85, 86, 87, 88, 90, 92, 93, 94, 100, 100}, /* VRHARD skills */
                             {0,  15, 16, 17, 18, 19, 20, 21, 22, 23, 25, 27, 29, 31, 33,  35, 37, 39,
                              41, 43, 45, 46, 48, 50, 50, 51, 52, 53, 54, 55, 56, 57, 59,  61, 63, 65,
                              67, 69, 71, 73, 75, 76, 75, 76, 77, 78, 80, 82, 83, 84, 100, 100} /* EXHARD skills */
                         }};

/* ...And the appropriate rooms for each guildmaster/guildguard */
int guild_info[][3] = {

    /* Weirvane */
    /* Guild_info has the following format.

     the class that will be allowed to pass by a guild guard, the room vnum
     the guild guard will sit,   the direction that the guard will watch.
     in this format

     {CLASS_<insert here>, vnum,   SCMD_<direction>},

     as example, the following line will designate that if a mob has spec of
     guild_guard, it will only allow magic users to pass south out of room 3000

     {CLASS_WIZARD,    3000,   SCMD_SOUTH},

     */

    {CLASS_WARRIOR, 3140, SCMD_EAST},
    {CLASS_WARRIOR, 3987, SCMD_SOUTH},
    {CLASS_ROGUE, 3073, SCMD_NORTH},
    {CLASS_THIEF, 3073, SCMD_NORTH},
    {CLASS_ASSASSIN, 3073, SCMD_NORTH},
    {CLASS_BARD, 3073, SCMD_NORTH},
    {CLASS_MERCENARY, 3073, SCMD_NORTH},
    {CLASS_ROGUE, 3981, SCMD_SOUTH},
    {CLASS_THIEF, 3981, SCMD_SOUTH},
    {CLASS_ASSASSIN, 3981, SCMD_SOUTH},
    {CLASS_BARD, 3990, SCMD_WEST},
    {CLASS_MERCENARY, 3981, SCMD_SOUTH},
    {CLASS_SORCERER, 3087, SCMD_EAST},
    {CLASS_WIZARD, 3087, SCMD_EAST},
    {CLASS_ENCHANTER, 3087, SCMD_EAST},
    {CLASS_CONJURER, 3087, SCMD_EAST},
    {CLASS_NECROMANCER, 3087, SCMD_EAST},
    {CLASS_CLERIC, 3149, SCMD_EAST},
    {CLASS_PRIEST, 3149, SCMD_EAST},
    {CLASS_SHAMAN, 3149, SCMD_EAST},
    {CLASS_MONK, 3140, SCMD_EAST},
    {CLASS_DRUID, 3149, SCMD_EAST},
    {CLASS_CLERIC, 3984, SCMD_EAST},
    {CLASS_PRIEST, 3984, SCMD_EAST},
    {CLASS_SHAMAN, 3984, SCMD_EAST},
    {CLASS_MONK, 3987, SCMD_SOUTH},
    {CLASS_DRUID, 3984, SCMD_EAST},
    {CLASS_RANGER, 3140, SCMD_EAST},

    /* Brass Dragon */
    /* {-999,               5065,  SCMD_WEST}, */

    /* -999 = all */
    /* The Ferry Master, Zone 34, and 37 */
    {-999, 3499, SCMD_EAST},
    {-999, 3720, SCMD_WEST},
    {-999, 3730, SCMD_WEST},

    /* The Ferry Master, Zone 120, and 59 */
    {-999, 12013, SCMD_SOUTH},
    {-999, 5962, SCMD_EAST},

    /* lizardmen swamps of dispair */
    {-999, 14006, SCMD_EAST},
    {-999, 14007, SCMD_SOUTH},

    /* The surly gateguard, Zone 37 */
    {-999, 3778, SCMD_SOUTH},

    /* this must go last -- add new guards above */
    {-1, -1, -1}};

/* Saving Throws for classes and levels */

/* only need to list saving throw for lvl 1, cuz it
 just gets decremented every 4 levels... lvl 4, 8, 12, etc. */

const byte class_saving_throws[NUM_CLASSES][5] = {
    {13, 14, 15, 15, 16}, /* war */
    {16, 15, 18, 20, 20}, /* rog */
    {14, 15, 13, 17, 15}, /* thi */
    {15, 17, 11, 16, 13}, /* sor */
    {16, 12, 15, 17, 17}, /* wiz */
    {16, 14, 13, 17, 15}, /* enc */
    {13, 16, 16, 18, 18}, /* con */
    {16, 14, 18, 15, 20}, /* nec */
    {14, 18, 13, 13, 15}, /* cle */
    {13, 19, 12, 12, 14}, /* pri */
    {13, 14, 14, 15, 15}, /* sha */
    {12, 16, 15, 16, 16}, /* mon */
    {12, 14, 13, 14, 15}, /* dru */
    {15, 13, 14, 17, 16}, /* ass */
    {15, 16, 15, 16, 14}, /* bar */
    {13, 16, 15, 13, 17}, /* ran */
    {14, 14, 15, 16, 17}  /* mer */
};

/* this modifies the saving throw of the class. */
const byte race_saving_throws[NUM_RACES][5] = {
    {00, 00, 00, 00, 00}, /* hum */
    {+1, +1, 00, -1, -1}, /* tro */
    {-1, 00, +1, 00, +1}, /* ogr */
    {00, -1, 00, 00, -1}, /* dwa */
    {+1, 00, 00, -1, -2}, /* elf */
    {+1, 00, -1, 00, -1}, /* hel */
    {+1, +1, -1, 00, -1}, /* gno */
    {+1, +2, 00, -1, 00}  /* hal */
};

const byte mob_race_saving_throws[NUM_NPC_RACES][5] = {
    {00, 00, 00, 00, 00},      /* undefined */
    {00, 00, 00, 00, 00},      /* human */
    {+1, +1, 00, -1, -1},      /* troll */
    {-1, 00, +1, 00, +1},      /* ogre */
    {00, -1, 00, 00, -1},      /* elf */
    {+1, 00, 00, -1, -2},      /* half_elf */
    {+1, 00, -1, 00, -1},      /* gnome */
    {+1, +1, -1, 00, -1},      /* halfling */
    {+1, +2, 00, -1, 00},      /* drow */
    {+1, +2, 00, -1, 00},      /* draconian */
    {-10, -10, -10, -10, -10}, /* dragon */
    {+1, +2, 00, -1, 00},      /* minotaur */
    {+1, +2, 00, -1, 00},      /* orc */
    {+1, +2, 00, -1, 00},      /* animal */
    {+1, +2, 00, -1, 00},      /* insect */
    {+1, +2, 00, -1, 00}       /* plant */
};

/* THAC0 for classes and levels.  (To Hit Armor Class 0) */
/* initial thac0, how many levels before change */

const int thaco[NUM_CLASSES][2] = {
    {18, 04}, /* war */
    {18, 04}, /* rog */
    {18, 04}, /* thi */
    {20, 06}, /* sor */
    {20, 06}, /* wiz */
    {20, 06}, /* enc */
    {20, 06}, /* con */
    {20, 06}, /* nec */
    {19, 05}, /* cle */
    {19, 05}, /* pri */
    {19, 05}, /* sha */
    {18, 03}, /* mon */
    {19, 05}, /* dru */
    {18, 04}, /* ass */
    {18, 04}, /* bar */
    {18, 04}, /* ran */
    {18, 04}  /* mer */
};

int race_mod[NUM_RACES][8] = {
    /* str, int, wis, dex, con, agi, wgt, hgt */
    {0, 0, 0, 0, 0, 0, 2, 3},          /* undefined */
    {0, 0, 0, 0, 0, 0, 2, 3},          /* human     */
    {20, -25, -20, -5, 10, 0, 3, 4},   /* troll     */
    {25, -20, -15, -10, 15, -5, 8, 5}, /* ogre      */
    {5, -5, 5, -5, 5, -10, 2, 2},      /* dwarf     */
    {-12, 13, 0, 5, -15, 7, 1, 3},     /* elf       */
    {-5, 5, 0, 5, -5, 5, 2, 3},        /* half-elf  */
    {-15, 20, -5, 15, -10, 0, 1, 2},   /* gnome     */
    {-10, 0, 0, 10, -10, 10, 1, 2}     /* halfling  */
};

/* Rewritten according to Meith's specifications */

#define STR   0
#define INTEL 1
#define WIS   2
#define DEX   3
#define CON   4
#define AGI   5

void roll_real_abils(struct char_data *ch) {
  int i, j, k, ctr, ctr1, ctr2, tmpnum;
  int r[6];
  int table[8];
  int rolls[81];
  bool class = FALSE;

  while (class == FALSE) {

    /* okay, roll 6 5d15 + 25 dices, to generate from 30-75 stats */

    /* roll 6d15, dropping the lowest d15, then add them, 6 times */
    for (i = 0; i < 6; i++) {
      table[i] = 0;
      for (k = 0; k < 6; k++) {
        r[k] = dice(1, 15);
      }
      /* bubble sort */
      for (ctr1 = 0; ctr1 < 5; ctr1++) {
        for (ctr2 = (ctr1 + 1); ctr2 < 6; ctr2++) {
          if (r[ctr1] > r[ctr2]) {
            tmpnum = r[ctr1];
            r[ctr1] = r[ctr2];
            r[ctr2] = tmpnum;
          }
        }
      }
      /* add up the dice, skipping first one (add remaining 5)*/
      for (ctr1 = 1; ctr1 < 6; ctr1++) {
        table[i] += r[ctr1];
      }
      table[i] += 25;
    }

    /* bubble sort */
    for (ctr1 = 0; ctr1 < 5; ctr1++) {
      for (ctr2 = (ctr1 + 1); ctr2 < 6; ctr2++) {
        if (table[ctr1] > table[ctr2]) {
          tmpnum = table[ctr1];
          table[ctr1] = table[ctr2];
          table[ctr2] = tmpnum;
        }
      }
    }

    /* make sure the sorting table is all zeroes */
    for (i = 0; i < 81; i++) {
      rolls[i] = 0;
    }

    /* sort the values
    for (i = 0; i < 6; i++) {
      rolls[table[i]]++;
    }
    j = 0;
    for (i = 80; i > 19; i--) {
      while (rolls[i]) {
        table[j] = i;
        j++;
        rolls[i]--;
      }
    }
    */

    /* okay, table has been sorted with the highest value first */
    /* make it the primary stat */
    rolls[0] = table[0];

    /* distribute the rest of the stats randomly */
    for (i = 1; i < 6; i++) {
      j = number(1, 5);
      while (rolls[j]) {
        j = number(1, 5);
      }
      rolls[j] = table[i];
    }

    /* add race applies */

    for (i = 0; i < 6; i++) {
      rolls[i] += race_mod[GET_RACE(ch)][i];
    }

    /* no stat lower than 1 or higher than 100 */
    for (i = 0; i < 6; i++) {
      if (rolls[i] > 100) {
        rolls[i] = 100;
      }
      if (rolls[i] < 1) {
        rolls[i] = 1;
      }
    }

    ch->real_abils.str = rolls[STR];
    ch->real_abils.intel = rolls[INTEL];
    ch->real_abils.wis = rolls[WIS];
    ch->real_abils.dex = rolls[DEX];
    ch->real_abils.con = rolls[CON];
    ch->real_abils.agi = rolls[AGI];
    ch->aff_abils = ch->real_abils;

    /* WEIGHT/HEIGHT */

    GET_WEIGHT(ch) = ((dice(1, 20) + GET_STR(ch) + 20) * race_mod[GET_RACE(ch)][6]);
    GET_HEIGHT(ch) = ((20 + dice(1, 8)) * race_mod[GET_RACE(ch)][7]);

    if (GET_RACE(ch) == RACE_ELF && GET_WEIGHT(ch) > 25) {
      GET_WEIGHT(ch) = GET_WEIGHT(ch) - 25;
    }

    if (GET_HEIGHT(ch) > 12 && ((GET_RACE(ch) == RACE_GNOME) || (GET_RACE(ch) == RACE_HALFLING))) {
      GET_HEIGHT(ch) -= 12;
    }
    if (GET_SEX(ch) == SEX_FEMALE && GET_RACE(ch) != RACE_OGRE) {
      GET_WEIGHT(ch) = ((GET_WEIGHT(ch) * 8) / 10);
      GET_HEIGHT(ch) = ((GET_HEIGHT(ch) * 9) / 10);
    }

    for (ctr = 0; ctr < NUM_CLASSES; ctr++) {
      if (check_class(ch, ctr))
        class = TRUE;
    }
  } /* end of while loop */
}

char *stat_msg(sbyte stat) {
  if (stat < 40 && stat >= 0)
    return "POOR    ";
  if (stat <= 75 && stat >= 40)
    return "AVERAGE ";
  if (stat <= 100 && stat > 75)
    return "GOOD    ";

  return "UNDEF   ";
}

void display_stats(struct char_data *ch) {
  char buf1[2046];
  size_t buflen;

  buflen = safe_snprintf(buf1, sizeof(buf1), "\r\nYour stats are:\r\n\r\n");
  buflen += safe_snprintf(buf1 + buflen, sizeof(buf1) - buflen, "STR: %s       ", stat_msg(GET_STR(ch)));
  buflen += safe_snprintf(buf1 + buflen, sizeof(buf1) - buflen, "CON: %s\r\n", stat_msg(GET_CON(ch)));
  buflen += safe_snprintf(buf1 + buflen, sizeof(buf1) - buflen, "DEX: %s       ", stat_msg(GET_DEX(ch)));
  buflen += safe_snprintf(buf1 + buflen, sizeof(buf1) - buflen, "WIS: %s\r\n", stat_msg(GET_WIS(ch)));
  buflen += safe_snprintf(buf1 + buflen, sizeof(buf1) - buflen, "AGI: %s       ", stat_msg(GET_AGI(ch)));
  safe_snprintf(buf1 + buflen, sizeof(buf1) - buflen, "INT: %s\r\n\r\n", stat_msg(GET_INT(ch)));

  send_to_char(buf1, ch);
}

void init_resists(struct char_data *ch) {
  switch (GET_RACE(ch)) {
  case RACE_TROLL:
    GET_RESIST(ch, DAM_FIRE) = -100;
    GET_RESIST(ch, DAM_SLASH) = 10;
    break;
  case RACE_DWARF:
    GET_RESIST(ch, DAM_MAGIC) = 15;
    GET_RESIST(ch, DAM_POISON) = 50;
    break;
  case RACE_ELF:
    GET_RESIST(ch, DAM_CHARM) = 90;
    GET_RESIST(ch, DAM_SLEEP) = 90;
    break;
  case RACE_HALFELF:
    GET_RESIST(ch, DAM_CHARM) = 30;
    GET_RESIST(ch, DAM_SLEEP) = 30;
    break;
  case RACE_GNOME:
    GET_RESIST(ch, DAM_MAGIC) = 15;
    break;
  case RACE_HALFLING:
    GET_RESIST(ch, DAM_MAGIC) = 15;
    break;
  default:
    break;
  }
  return;
}

/* Some initializations for characters, including initial skills */
void do_start(struct char_data *ch) {
  void advance_level(struct char_data * ch);
  int index = 1;

  set_title(ch, NULL);
  ch->points.max_mana = 100;
  ch->points.max_move = 82;

  if (!GET_LEVEL(ch)) {
    switch (GET_CLASS(ch)) {
    case CLASS_WARRIOR:
    case CLASS_MONK:
    case CLASS_RANGER:
      ch->real_abils.str += 5;
      ch->real_abils.dex += 3;
      if (ch->real_abils.str > 100)
        ch->real_abils.str = 100;
      if (ch->real_abils.dex > 100)
        ch->real_abils.dex = 100;
      break;
    case CLASS_THIEF:
    case CLASS_ROGUE:
    case CLASS_ASSASSIN:
    case CLASS_BARD:
    case CLASS_MERCENARY:
      ch->real_abils.dex += 5;
      ch->real_abils.str += 3;
      if (ch->real_abils.str > 100)
        ch->real_abils.str = 100;
      if (ch->real_abils.dex > 100)
        ch->real_abils.str = 100;
      break;
    case CLASS_CLERIC:
    case CLASS_PRIEST:
    case CLASS_SHAMAN:
    case CLASS_DRUID:
      ch->real_abils.wis += 5;
      ch->real_abils.intel += 3;
      if (ch->real_abils.wis > 100)
        ch->real_abils.wis = 100;
      if (ch->real_abils.intel > 100)
        ch->real_abils.intel = 100;
      break;
    default:
      ch->real_abils.intel += 5;
      ch->real_abils.wis += 3;
      if (ch->real_abils.intel > 100)
        ch->real_abils.intel = 100;
      if (ch->real_abils.wis > 100)
        ch->real_abils.wis = 100;
      break;
    }
  }

  init_resists(ch);

  GET_LEVEL(ch) = 1;
  advance_level(ch);

  index = 0;
  while (spells[index].command[0] != '\n') {
    if (spells[index].min_level[(int)GET_CLASS(ch)] == 1) {
      SET_SKILL(ch, spells[index].spellindex, 15);
    } else {
      SET_SKILL(ch, spells[index].spellindex, 0);
    }
    GET_PRACS(ch, spells[index].spellindex) = 0;
    index++;
  }

  GET_EXP(ch) = 1;
  GET_GOLD(ch) = 100;
  GET_TEMP_GOLD(ch) = 10000; /* 100 gold in copper */
  GET_MAX_HIT(ch) = (GET_CON(ch) / 2);
  GET_HIT(ch) = GET_MAX_HIT(ch);
  GET_MANA(ch) = GET_MAX_MANA(ch);
  GET_MOVE(ch) = GET_MAX_MOVE(ch);

  GET_COND(ch, THIRST) = 24;
  GET_COND(ch, FULL) = 24;
  GET_COND(ch, DRUNK) = 0;

  ch->player.time.played = 0;
  ch->player.time.logon = time(0);
}

/* Lose maximum in various points */
void lose_level(struct char_data *ch) {
  void set_magic_memory(struct char_data * ch);
  int add_hp = 0, add_mana = 0, add_move = 0, i;

  extern sh_int stats[12][101];
  extern const sh_int monk_stat[LVL_IMMORT + 1][5];

  add_hp = stats[CON_HITP][GET_CON(ch)];
  switch (GET_CLASS(ch)) {
  case CLASS_NECROMANCER:
  case CLASS_SORCERER:
  case CLASS_ENCHANTER:
  case CLASS_WIZARD:
  case CLASS_CONJURER:
    add_hp += number(3, 8);
    add_mana = number(GET_LEVEL(ch), (int)(1.5 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana, 10);
    add_move = number(0, 2);
    break;
  case CLASS_SHAMAN:
  case CLASS_PRIEST:
  case CLASS_CLERIC:
  case CLASS_DRUID:
    add_hp += number(5, 10);
    add_mana = number(GET_LEVEL(ch), (int)(1.5 * GET_LEVEL(ch)));
    add_mana = MIN(add_mana, 10);
    add_move = number(0, 2);
    break;
  case CLASS_ASSASSIN:
  case CLASS_THIEF:
  case CLASS_ROGUE:
  case CLASS_BARD:
  case CLASS_MERCENARY:
    add_hp += number(7, 13);
    add_hp = (add_hp * 9) / 10;
    add_mana = 0;
    add_move = number(1, 3);
    break;
  case CLASS_RANGER:
  case CLASS_WARRIOR:
  case CLASS_MONK:
    add_hp += number(10, 15);
    add_mana = 0;
    add_move = number(1, 3);
    break;
  }
  if (GET_LEVEL(ch) > 1) {
    ch->points.max_hit -= MAX(1, add_hp);
    ch->points.max_move -= MAX(1, add_move);
    ch->points.max_mana -= MAX(0, add_mana);
  }
  if (IS_MAGE(ch) || IS_PRI(ch)) {
    GET_PRACTICES(ch) -= MAX(2, stats[WIS_PRAC][GET_WIS(ch)]);
    if (GET_PRACTICES(ch) < 0)
      GET_PRACTICES(ch) = 0;
    GET_TRAINING(ch) -= BOUNDED(1, stats[WIS_PRAC][GET_WIS(ch)], 2);
    if (GET_TRAINING(ch) < 0)
      GET_TRAINING(ch) = 0;
    set_magic_memory(ch);
  } else {
    GET_PRACTICES(ch) -= BOUNDED(1, stats[WIS_PRAC][GET_WIS(ch)], 2);
    if (GET_PRACTICES(ch) < 0)
      GET_PRACTICES(ch) = 0;
    GET_TRAINING(ch) -= MAX(2, stats[WIS_PRAC][GET_WIS(ch)]);
    if (GET_TRAINING(ch) < 0)
      GET_TRAINING(ch) = 0;
  }

  if (IS_MONK(ch))
    GET_AC(ch) = monk_stat[(sh_int)GET_LEVEL(ch)][3];

  for (i = 1; spells[i].command[0] != '\n'; i++)
    if (spells[i].min_level[(int)GET_CLASS(ch)] >= GET_LEVEL(ch))
      SET_SKILL(ch, spells[i].spellindex, 0);
  save_char_text(ch, NOWHERE);
  safe_snprintf(buf, MAX_STRING_LENGTH, "%s lost level %d", GET_NAME(ch), GET_LEVEL(ch));
  mudlog(buf, 'A', COM_IMMORT, TRUE);
}

/* Gain maximum in various points */
void advance_level(struct char_data *ch) {
  void set_magic_memory(struct char_data * ch);
  struct obj_data *obj, *next_obj;
  int add_hp = 0, add_mana = 0, add_move = 0, index = 0, i;

  extern sh_int stats[12][101];
  extern const sh_int monk_stat[LVL_IMMORT + 1][5];

  if (IS_MAGE(ch) || IS_PRI(ch))
    add_hp = stats[CON_HITP][GET_CON(ch)] / 4;
  else
    add_hp = stats[CON_HITP][GET_CON(ch)];

  switch (GET_CLASS(ch)) {

  case CLASS_NECROMANCER:
  case CLASS_SORCERER:
  case CLASS_ENCHANTER:
  case CLASS_WIZARD:
  case CLASS_CONJURER:
    add_hp += number(2, 3);
    add_mana = (number(4, 8) + (GET_INT(ch) / 25));
    add_move = number(0, 2);
    break;

  case CLASS_SHAMAN:
  case CLASS_PRIEST:
  case CLASS_CLERIC:
  case CLASS_DRUID:
    add_hp += number(5, 9);
    add_mana = (number(3, 7) + (GET_WIS(ch) / 25));
    add_move = number(0, 2);
    break;

  case CLASS_ASSASSIN:
  case CLASS_THIEF:
  case CLASS_ROGUE:
  case CLASS_BARD:
  case CLASS_MERCENARY:
    add_hp += number(6, 10);
    add_mana = 0;
    add_move = number(1, 3);
    break;

  case CLASS_RANGER:
  case CLASS_WARRIOR:
  case CLASS_MONK:
    add_hp += number(8, 14);
    add_mana = 0;
    add_move = number(1, 3);
    break;
  }

  switch (GET_RACE(ch)) {
  case RACE_OGRE:
    add_hp += dice(4, 2);
    break;
  }

  ch->points.max_hit += MAX(1, add_hp);
  ch->points.max_move += MAX(1, add_move);

  if (GET_LEVEL(ch) > 1)
    ch->points.max_mana += add_mana;

  if (IS_MAGE(ch) || IS_PRI(ch)) {
    GET_PRACTICES(ch) += MAX(2, stats[WIS_PRAC][GET_WIS(ch)]);
    GET_TRAINING(ch) += BOUNDED(1, stats[WIS_PRAC][GET_WIS(ch)], 2);
    set_magic_memory(ch);
  } else {
    GET_PRACTICES(ch) += BOUNDED(1, stats[WIS_PRAC][GET_WIS(ch)], 2);
    GET_TRAINING(ch) += MAX(2, stats[WIS_PRAC][GET_WIS(ch)]);
  }

  if (GET_LEVEL(ch) > 1) {
    while (spells[index].command[0] != '\n') {
      if (spells[index].min_level[(int)GET_CLASS(ch)] < GET_LEVEL(ch))
        GET_PRACS(ch, spells[index].spellindex)++;
      if (spells[index].min_level[(int)GET_CLASS(ch)] == GET_LEVEL(ch))
        GET_PRACS(ch, spells[index].spellindex) = (GET_LEVEL(ch) / 4);
      index++;
    }
  }

  if (IS_MONK(ch)) {
    GET_AC(ch) = monk_stat[(sh_int)GET_LEVEL(ch)][2];
    for (i = 0; i < NUM_WEARS; i++) {
      if (GET_EQ(ch, i) && (GET_OBJ_WEIGHT(GET_EQ(ch, i)) > monk_stat[(int)GET_LEVEL(ch)][3])) {
        safe_snprintf(buf, MAX_STRING_LENGTH, "%s becomes too heavy for you to wear.\r\n", OBJN(GET_EQ(ch, i), ch));
        send_to_char(buf, ch);
        obj_to_char(unequip_char(ch, i), ch);
      }
    }
  }

  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    for (i = 0; i < 3; i++)
      GET_COND(ch, i) = (char)-1;
    SET_BIT(PRF_FLAGS(ch), PRF_HOLYLIGHT);
    GET_GOLD(ch) = 0;

    /* sac chars equip */
    for (i = 0; i < NUM_WEARS; i++)
      if (ch->equipment[i]) {
        extract_obj(unequip_char(ch, i));
      }

    for (obj = ch->carrying; obj != NULL; obj = next_obj) {
      next_obj = obj->next_content;
      extract_obj(obj);
    }
  }

  save_char_text(ch, NOWHERE);

  safe_snprintf(buf, MAX_STRING_LENGTH, "%s advanced to level %d", GET_NAME(ch), GET_LEVEL(ch));
  mudlog(buf, 'A', COM_IMMORT, TRUE);
  safe_snprintf(buf, MAX_STRING_LENGTH, "LEVELGAIN:%s gained %d hp %d m %d v : int %d wis %d con %d", GET_NAME(ch),
                add_hp, add_mana, add_move, GET_INT(ch), GET_WIS(ch), GET_CON(ch));

  mudlog(buf, 'D', COM_IMMORT, TRUE);
}

/* exp required for each level */

const float ClassExpMod[NUM_CLASSES] = {
    1.0, /* War */
    1.0, /* Rog */
    1.0, /* Thi */
    1.0, /* Sor */
    1.0, /* Wiz */
    1.0, /* Enc */
    1.0, /* Con */
    1.0, /* Nec */
    1.0, /* Cle */
    1.0, /* Pri */
    1.0, /* Sha */
    1.0, /* Mon */
    1.0, /* Dru */
    1.0, /* Ass */
    1.0, /* Bar */
    1.0, /* Ran */
    1.0  /* Mer */
};

const float LvlExpMod[LVL_IMMORT + 2] = {
    1.0,  /* 00 */
    1.0,  /* 01 */
    2.0,  /* 02 */
    2.0,  /* 03 */
    3.0,  /* 04 */
    3.0,  /* 05 */
    4.0,  /* 06 */
    4.0,  /* 07 */
    5.0,  /* 08 */
    5.0,  /* 09 */
    5.0,  /* 10 */
    6.0,  /* 11 */
    6.0,  /* 12 */
    6.0,  /* 13 */
    7.0,  /* 14 */
    7.0,  /* 15 */
    7.0,  /* 16 */
    8.0,  /* 16 */
    8.0,  /* 18 */
    8.0,  /* 19 */
    9.0,  /* 20 */
    9.0,  /* 21 */
    9.0,  /* 22 */
    10.0, /* 23 */
    10.0, /* 24 */
    10.0, /* 25 */
    11.0, /* 26 */
    11.0, /* 27 */
    11.0, /* 28 */
    12.0, /* 29 */
    12.0, /* 30 */
    12.0, /* 31 */
    13.0, /* 32 */
    13.0, /* 33 */
    13.0, /* 34 */
    14.0, /* 35 */
    14.0, /* 36 */
    14.0, /* 37 */
    15.0, /* 38 */
    15.0, /* 39 */
    15.0, /* 40 */
    16.0, /* 41 */
    16.0, /* 42 */
    16.0, /* 43 */
    17.0, /* 44 */
    17.0, /* 45 */
    17.0, /* 46 */
    18.0, /* 47 */
    18.0, /* 48 */
    18.0, /* 49 */
    19.0, /* 50 */
    40.0, /* 51 */
    50.0  /* 52 */
};

const unsigned int MaxExperience[LVL_IMMORT + 2] = {
    0, /* 00 */
    1,
    /* 01 */ /* 1st circle */
    200,     /* 02 */
    800,     /* 03 */
    1250,    /* 04 */
    2000,    /* 05 */
    9000,
    /* 06 */ /* 2nd circle */
    12000,   /* 07 */
    18000,   /* 08 */
    25000,   /* 09 */
    37000,   /* 10 */
    51750,
    /* 11 */ /* 3rd circle */
    90000,   /* 12 */
    129446,  /* 13 */
    341000,  /* 14 */
    530920,  /* 15 */
    988830,
    /* 16 */ /* 4th circle */
    1061880, /* 17 */
    1370210, /* 18 */
    2737490, /* 19 */
    3991550, /* 20 */
    7465070,
    /* 21 */  /* 5th circle */
    9196500,  /* 22 */
    12230800, /* 23 */
    14621200, /* 24 */
    17430000, /* 25 */
    20730200,
    /* 26 */  /* 6th circle */
    24608000, /* 27 */
    29164400, /* 28 */
    34518100, /* 29 */
    40808800, /* 30 */
    48970560,
    /* 31 */  /* 7th circle */
    53867616, /* 32 */
    59254377, /* 33 */
    65179815, /* 34 */
    71697796, /* 35 */
    78867576,
    /* 36 */   /* 8th circle */
    86754334,  /* 37 */
    95429767,  /* 38 */
    104972744, /* 39 */
    115470018, /* 40 */
    127017020,
    /* 41 */   /* 9th circle */
    139718722, /* 42 */
    153690595, /* 43 */
    169059654, /* 44 */
    185965620, /* 45 */
    204562182,
    /* 46 */   /* 10th circle */
    225018400, /* 47 */
    247520240, /* 48 */
    272272264, /* 49 */
    299499490, /* 50 */
    1000000000,
    /* 51 */   /* 11th circle */
    1999999999 /* 52 */
};

int invalid_class(struct char_data *ch, struct obj_data *obj) {
#define IS_MATERIAL(obj, val) (GET_OBJ_VAL((obj), 4) == (val))
  if ((IS_OBJ_STAT(obj, ITEM_ANTI_MAGIC_USER) && (IS_MAGE(ch))) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_CLERIC) && (IS_PRI(ch))) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_WARRIOR) && (IS_WARRIOR(ch) || IS_MONK(ch) || IS_RANGER(ch))) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_THIEF) &&
       (IS_THIEF(ch) || IS_ROGUE(ch) || IS_ASSASSIN(ch) || IS_BARD(ch) || IS_MERCENARY(ch))) ||
      (GET_OBJ_TYPE(obj) != ITEM_WEAPON &&
       (IS_MATERIAL(obj, TYPE_IRON) || IS_MATERIAL(obj, TYPE_STEEL) || IS_MATERIAL(obj, TYPE_MITHRIL) ||
        IS_MATERIAL(obj, TYPE_TITANIUM) || IS_MATERIAL(obj, TYPE_COPPER) || IS_MATERIAL(obj, TYPE_BRONZE) ||
        IS_MATERIAL(obj, TYPE_SILVER) || IS_MATERIAL(obj, TYPE_GOLD)) &&
       (GET_CLASS(ch) == CLASS_RANGER)))
    return 1;
  else
    return 0;
}

const sh_int monk_stat[LVL_IMMORT + 1][5] = {
    /*numdice sizdice  ac item wgt max tot wgt*/
    {0, 0, 100, 0, 0},    {1, 4, 100, 20, 0},                                                          /*  1 */
    {1, 4, 98, 20, 0},    {1, 4, 94, 20, 0},  {1, 4, 92, 20, 0}, {1, 4, 90, 16, 0}, {1, 6, 88, 16, 0}, /* 6 */
    {1, 6, 86, 16, 0},    {1, 6, 84, 16, 0},  {1, 6, 82, 16, 0}, {1, 6, 80, 12, 0}, {2, 4, 78, 12, 0}, /* 11 */
    {2, 4, 76, 12, 0},    {2, 4, 74, 12, 0},  {2, 4, 72, 12, 0}, {2, 4, 70, 8, 0},  {1, 10, 68, 8, 0}, /* 16 */
    {1, 10, 66, 8, 0},    {1, 10, 64, 8, 0},  {1, 10, 62, 8, 0}, {1, 10, 60, 5, 0}, {2, 6, 58, 5, 0},  /* 21 */
    {2, 6, 56, 5, 0},     {2, 6, 54, 5, 0},   {2, 6, 52, 5, 0},  {2, 6, 50, 5, 0},  {2, 7, 48, 5, 0},  /* 26 */
    {2, 7, 46, 5, 0},     {2, 7, 44, 5, 0},   {2, 7, 42, 5, 0},  {2, 7, 40, 4, 0},  {3, 6, 38, 4, 0},  /* 31 */
    {3, 6, 36, 4, 0},     {3, 6, 34, 4, 0},   {3, 6, 32, 4, 0},  {3, 6, 30, 4, 0},  {3, 7, 28, 4, 0},  /* 36 */
    {3, 7, 26, 4, 0},     {3, 7, 24, 4, 0},   {3, 7, 22, 4, 0},  {3, 7, 20, 3, 0},  {4, 6, 18, 3, 0},  /* 41 */
    {4, 6, 16, 3, 0},     {4, 6, 14, 3, 0},   {4, 6, 12, 3, 0},  {4, 6, 10, 3, 0},  {5, 6, 8, 3, 0},   /* 46 */
    {5, 6, 6, 3, 0},      {5, 6, 4, 3, 0},    {5, 6, 2, 3, 0},   {6, 6, 0, 2, 0},                      /* 50 */
    {10, 10, 100, 999, 0}};
