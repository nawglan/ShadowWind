/* ************************************************************************
 *   File: fixmobs.c                                    Part of ShadowWind *
 *  Usage: Resets mobs to "standard"                                       *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../db.h"
#include "../structs.h"
#include "../utils.h"

#define GET_ALIAS(mob)      ((mob)->player.name)
#define GET_SDESC(mob)      ((mob)->player.short_descr)
#define GET_DDESC(mob)      ((mob)->player.description)
#define GET_SIZE(mob)       ((mob)->mob_specials.size)
#define GET_ATTACKS(mob)    ((mob)->mob_specials.mob_attacks)
#define GET_EQUIP(mob)      ((mob)->mob_specials.mob_equip)
#define GET_ACTION(mob)     ((mob)->mob_specials.mob_action)
#define GET_ATTACK(mob)     ((mob)->mob_specials.mob_attacks->attack_type)
#define GET_ST_HPD(level)   (mob_stats[(int)(level)].hp_dice)
#define GET_ST_HPS(level)   (mob_stats[(int)(level)].hp_sides)
#define GET_ST_HPB(level)   (mob_stats[(int)(level)].hp_bonus)
#define GET_ST_EXP(level)   (mob_stats[(int)(level)].experience)
#define GET_ST_GOLD(level)  (mob_stats[(int)(level)].gold)
#define GET_ST_THAC0(level) (mob_stats[(int)(level)].thac0)
#define GET_ST_AC(level)    (mob_stats[(int)(level)].ac)
#define GET_MPROG(mob)      (mob_index[(mob)->nr].mobprogs)

char buf1[MAX_STRING_LENGTH];
char buf2[MAX_STRING_LENGTH];
int num_mobs_in_zone = 0;

const struct max_mob_stats mob_stats[] = {
    /* level, hp_dice, hp_sides, hp_bonus, exp, gold, thac0, ac, damage */
    {0, 1, 1, 5, 20, 10, 20, 10, 1},
    {1, 1, 1, 10, 150, 50, 19, 9, 1},
    {2, 1, 1, 20, 300, 150, 19, 9, 2},
    {3, 2, 2, 30, 600, 190, 18, 8, 3},
    {4, 2, 2, 40, 1000, 270, 18, 8, 4},
    {5, 3, 3, 50, 1600, 350, 17, 7, 5},
    {6, 3, 3, 60, 2400, 450, 17, 7, 6},
    {7, 4, 4, 70, 2900, 540, 16, 6, 7},
    {8, 4, 4, 80, 3500, 600, 16, 6, 8},
    {9, 5, 5, 90, 4200, 690, 15, 5, 9},
    {10, 5, 5, 100, 5000, 800, 15, 5, 10},
    {11, 6, 6, 110, 6900, 880, 14, 4, 11},
    {12, 6, 6, 120, 7500, 950, 14, 4, 12},
    {13, 7, 7, 130, 8700, 1000, 14, 3, 13},
    {14, 7, 7, 140, 9900, 1080, 14, 3, 14},
    {15, 8, 8, 150, 11000, 1150, 14, 2, 15},
    {16, 8, 8, 160, 12500, 1200, 13, 2, 16},
    {17, 9, 9, 170, 14300, 1280, 13, 1, 17},
    {18, 9, 9, 180, 16800, 1340, 13, 1, 18},
    {19, 10, 10, 190, 20000, 1430, 12, 0, 19},
    {20, 10, 10, 200, 25000, 1500, 12, 0, 20},
    {21, 11, 11, 210, 27500, 1600, 12, -1, 21},
    {22, 11, 11, 220, 31000, 1750, 11, -1, 22},
    {23, 12, 12, 230, 35000, 2000, 11, -2, 23},
    {24, 12, 12, 240, 40000, 2200, 11, -2, 24},
    {25, 13, 13, 250, 45500, 2650, 10, -3, 25},
    {26, 13, 13, 260, 51000, 3000, 10, -3, 26},
    {27, 14, 14, 270, 55500, 3400, 10, -4, 27},
    {28, 14, 14, 280, 61000, 3900, 9, -4, 28},
    {29, 15, 15, 290, 70000, 4400, 9, -5, 29},
    {30, 15, 15, 300, 80000, 5000, 9, -5, 30},
    {31, 16, 16, 310, 85000, 5100, 8, -6, 31},
    {32, 16, 16, 320, 92000, 5250, 8, -6, 32},
    {33, 17, 17, 330, 99000, 5500, 8, -7, 33},
    {34, 17, 17, 340, 106000, 5800, 7, -7, 34},
    {35, 18, 18, 350, 112000, 6100, 7, -8, 35},
    {36, 18, 18, 360, 118000, 6500, 7, -8, 36},
    {37, 19, 19, 370, 124000, 6900, 6, -9, 37},
    {38, 19, 19, 380, 132000, 7200, 6, -9, 38},
    {39, 20, 20, 390, 140000, 7600, 6, -10, 39},
    {40, 20, 20, 400, 150000, 8000, 5, -10, 40},
    {41, 21, 21, 410, 165000, 8100, 5, -10, 41},
    {42, 21, 21, 420, 190000, 8200, 5, -10, 42},
    {43, 22, 22, 430, 220000, 8300, 4, -10, 43},
    {44, 22, 22, 440, 250000, 8500, 4, -10, 44},
    {45, 23, 23, 450, 280000, 8700, 4, -10, 45},
    {46, 23, 23, 460, 310000, 8900, 3, -10, 46},
    {47, 24, 24, 470, 350000, 9100, 3, -10, 47},
    {48, 24, 24, 480, 390000, 9400, 3, -10, 48},
    {49, 25, 25, 490, 440000, 9700, 2, -10, 49},
    {50, 25, 25, 500, 500000, 10000, 2, -10, 50},
    {51, 26, 26, 1510, 1500000, 20000, 2, -10, 99999},
    {52, 26, 26, 2520, 2600000, 20000, 1, -10, 99999},
    {53, 27, 27, 3530, 3700000, 20000, 1, -10, 99999},
    {54, 27, 27, 4540, 4800000, 20000, 1, -10, 99999},
    {55, 28, 28, 5550, 6000000, 20000, 0, -10, 99999},
    {56, 28, 28, 7650, 7000000, 20000, 0, -10, 99999},
    {57, 29, 29, 9650, 8000000, 20000, 0, -10, 99999},
    {58, 29, 29, 13580, 9000000, 20000, 0, -10, 99999},
    {59, 30, 30, 19000, 9500000, 20000, 0, -10, 99999},
    {60, 30, 30, 30000, 10000000, 20000, 0, -10, 99999}};

const struct balance_type balance_table[] = {
    /* hitnum,hitsize,hitadd,exp,thac0,ac,damnum,damsize,damadd,gold */
    {1, 9, 1, 25, 20, 10, 1, 4, 0, 0}, /* 0 */
    {1, 11, 11, 100, 20, 9, 1, 5, 0, 10},
    {1, 12, 23, 200, 19, 8, 1, 6, 0, 50},
    {1, 11, 36, 350, 18, 7, 1, 7, 0, 100},
    {1, 12, 48, 600, 17, 6, 1, 8, 0, 200},
    {1, 11, 61, 900, 16, 5, 2, 4, 0, 300}, /* 5 */
    {1, 12, 73, 1500, 15, 4, 1, 8, 1, 400},
    {1, 11, 86, 2250, 14, 4, 2, 4, 1, 500},
    {1, 12, 98, 3750, 13, 3, 2, 5, 1, 600},
    {1, 11, 111, 6000, 12, 3, 2, 5, 1, 800},
    {1, 12, 123, 9000, 11, 2, 2, 6, 1, 1000}, /* 10 */
    {1, 11, 136, 11000, 10, 2, 2, 6, 1, 1200},
    {1, 12, 148, 13000, 9, 2, 2, 7, 1, 1400},
    {1, 11, 161, 16000, 8, 2, 2, 7, 1, 1600},
    {1, 12, 173, 18000, 7, 1, 2, 8, 1, 1800},
    {1, 11, 186, 21000, 6, 1, 2, 8, 2, 2000}, /* 15 */
    {1, 12, 198, 24000, 5, 1, 2, 8, 2, 2400},
    {1, 11, 211, 28000, 4, 1, 3, 6, 2, 2800},
    {1, 12, 223, 30000, 3, 0, 3, 6, 2, 3200},
    {1, 11, 236, 35000, 2, 0, 3, 6, 3, 3600},
    {1, 12, 248, 40000, 1, 0, 3, 6, 4, 4000}, /* 20 */
    {1, 11, 261, 50000, 0, -1, 3, 7, 5, 5000},
    {1, 12, 274, 60000, 0, -1, 3, 8, 4, 6000},
    {1, 11, 286, 80000, 0, -2, 3, 8, 4, 7000},
    {1, 12, 299, 100000, 0, -2, 3, 8, 4, 8000},
    {1, 11, 311, 110000, 0, -3, 4, 6, 4, 9000}, /* 25 */
    {1, 12, 324, 120000, 0, -3, 4, 6, 4, 10000},
    {1, 11, 336, 130000, 0, -4, 4, 6, 4, 11000},
    {1, 12, 349, 140000, 0, -4, 4, 6, 5, 12000},
    {1, 11, 361, 150000, 0, -5, 4, 7, 5, 13000},
    {1, 12, 374, 160000, 0, -5, 4, 8, 5, 14000}, /* 30 */
    {1, 15, 385, 180000, 0, -6, 4, 8, 5, 15000},
    {1, 50, 401, 200000, 0, -6, 4, 8, 5, 16000},
    {1, 50, 451, 220000, 0, -7, 4, 8, 6, 17000},
    {1, 50, 501, 240000, 0, -7, 4, 8, 6, 18000},
    {1, 50, 551, 260000, 0, -8, 4, 8, 6, 19000}, /* 35 */
    {1, 100, 601, 280000, 0, -8, 4, 8, 6, 20000},
    {1, 100, 701, 300000, 0, -9, 4, 9, 6, 21000},
    {1, 100, 801, 320000, 0, -9, 4, 9, 6, 22000},
    {1, 200, 900, 340000, 0, -10, 4, 9, 6, 23000},
    {1, 400, 1101, 360000, 0, -10, 5, 9, 6, 240000} /* 40 */
};

const char *resist_short_name[] = {"ResLight",   "ResDark",  "ResFire",  "ResCold",        "ResAcid",   "ResPoison",
                                   "ResDisease", "ResCharm", "ResSleep", "ResSlash",       "ResPierce", "ResBludgeon",
                                   "ResNWeap",   "ResMWeap", "ResMagic", "ResElectricity", "\n"};

void discrete_save(FILE *mob_file, int zone);
void mprog_read_programs(FILE *fp, struct index_data *pMobIndex);
void strip_string(char *string);

/**************************************************************************
 *  declarations of most of the 'global' variables                         *
 ************************************************************************ */

struct char_data *character_list = NULL; /* global linked list of
                                          * chars         */
struct index_data *mob_index;            /* index table for mobile file         */
struct char_data *mob_proto;             /* prototypes for mobs                 */
int top_of_mobt = 0;                     /* top of mobile index table         */
int new_top_mobt = 0;                    /* new top, used for OLC         */
int mob_idnum = -1;                      /* mobs have a negative idnum    */
struct player_special_data dummy_mob;

/* local functions */
void setup_dir(FILE *fl, int room, int dir);
void index_boot(int mode);
void discrete_load(FILE *fl, int mode, int reset);
void parse_mobile(FILE *mob_f, int nr, int reset);
int is_empty(int zone_nr);
void clear_char(struct char_data *ch);
void parse_pline(char *line, char *field, char *value);

int MIN(int a, int b) {
  return a < b ? a : b;
}

int MAX(int a, int b) {
  return a > b ? a : b;
}

/* returns: 0 if equal, 1 if arg1 > arg2, -1 if arg1 < arg2  */
/* scan 'till found different or end of both                 */
int str_cmp(char *arg1, char *arg2) {
  if (arg1 == NULL || arg2 == NULL) {
    printf("NULL argument in str_cmp\n");
    return (0);
  }

  return (strcasecmp(arg1, arg2));
}

/*. This procedure removes the '\r' from a string so that it may be
 saved to a file.  Use it only on buffers, not on the oringinal
 strings.*/

void strip_string(char *buffer) {
  char *pointer;

  pointer = buffer;
  /*. Hardly elegant, but it does the job .*/
  while ((pointer = strchr(pointer, '\r')))
    strcpy(pointer, pointer + 1);
}

void olc_print_bitvectors(FILE *f, long bitvector, long max) {
  int i;
  int counter = 0;

  for (i = 0; i < max; i++)
    if (bitvector & (1 << i)) {
      if (i <= 25)
        fprintf(f, "%c", i + 'a');
      else
        fprintf(f, "%c", i - 26 + 'A');
      counter++;
    }
  if (counter == 0)
    fprintf(f, "0 ");
  else
    fprintf(f, " ");
}

/*
 * Get the type of MobProg.
 */
const char *medit_get_mprog_type(struct mob_prog_data *mprog) {
  switch (mprog->type) {
  case IN_FILE_PROG:
    return ">in_file_prog";
  case ACT_PROG:
    return ">act_prog";
  case SPEECH_PROG:
    return ">speech_prog";
  case RAND_PROG:
    return ">rand_prog";
  case FIGHT_PROG:
    return ">fight_prog";
  case HITPRCNT_PROG:
    return ">hitprcnt_prog";
  case DEATH_PROG:
    return ">death_prog";
  case ENTRY_PROG:
    return ">entry_prog";
  case GREET_PROG:
    return ">greet_prog";
  case ALL_GREET_PROG:
    return ">all_greet_prog";
  case GIVE_PROG:
    return ">give_prog";
  case BRIBE_PROG:
    return ">bribe_prog";
  case SHOUT_PROG:
    return ">shout_prog";
  case HOLLER_PROG:
    return ">holler_prog";
  case TELL_PROG:
    return ">tell_prog";
  case TIME_PROG:
    return ">time_prog";
  case ASK_PROG:
    return ">ask_prog";
  }
  return ">ERROR_PROG";
}

char *fname(char *namelist) {
  static char holder[30];
  register char *point;

  for (point = holder; isalpha(*namelist); namelist++, point++)
    *point = *namelist;

  *point = '\0';

  return (holder);
}

/* returns the real number of the monster with given virtual number */
int real_mobile(int virtual) {
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

/*
 * get_line reads the next non-blank line off of the input stream.
 * The newline character is removed from the input.  Lines which begin
 * with '*' are considered to be comments.
 *
 * Returns the number of lines advanced in the file.
 */

int get_line(FILE *fl, char *buf) {
  char temp[256];
  int lines = 0;

  *temp = '\0';
  do {
    lines++;
    fgets(temp, 256, fl);
    if (*temp)
      temp[strlen(temp) - 1] = '\0';
  } while (!feof(fl) && (*temp == '*' || !*temp));

  if (feof(fl))
    return 0;
  else {
    strcpy(buf, temp);
    return lines;
  }
}

/*
 * unget_line rewinds one line in the file
 */

void unget_line(FILE *fl) {
  do {
    fseek(fl, -2, SEEK_CUR);
  } while (fgetc(fl) != '\n');
}

/*************************************************************************
 *  routines for booting the system                                       *
 *********************************************************************** */

/* body of the booting system */
void boot_db(void) {
  printf("Fixmobs -- BEGIN.\n");
  index_boot(DB_BOOT_MOB);
  printf("Fixmobs -- END.\n");
}

/* function to count how many hash-mark delimited records exist in a file */
int count_hash_records(FILE *fl) {
  char buf[128];
  int count = 0;

  while (fgets(buf, 128, fl))
    if (*buf == '#')
      count++;

  return count;
}

void index_boot(int mode) {
  char *index_filename, *prefix;
  FILE *index, *db_file, *new_db_file;
  int zone;

  prefix = MOB_PREFIX;
  index_filename = INDEX_FILE;

  sprintf(buf2, "%s/%s", prefix, index_filename);

  if (!(index = fopen(buf2, "r"))) {
    printf("Error opening index file '%s'\n", buf2);
    fflush(NULL);
    exit(1);
  }

  fscanf(index, "%s\n", buf1);
  while (*buf1 != '$') {

    sprintf(buf2, "%s/%s", prefix, buf1);
    sscanf(buf1, "%d.mob", &zone);

    if (!(db_file = fopen(buf2, "r"))) {
      perror(buf2);
      fflush(NULL);
      exit(1);
    }
    sprintf(buf2, "%s/%s.new", prefix, buf1);
    if (!(new_db_file = fopen(buf2, "w"))) {
      perror(buf2);
      fflush(NULL);
      exit(1);
    }

    num_mobs_in_zone = count_hash_records(db_file);
    fprintf(stderr, "Num mobs found = %d, zone = %d\n", num_mobs_in_zone, zone);
    CREATE(mob_proto, struct char_data, num_mobs_in_zone);
    CREATE(mob_index, struct index_data, num_mobs_in_zone);

    rewind(db_file);
    discrete_load(db_file, mode, 1);
    fclose(db_file);
    discrete_save(new_db_file, zone);
    FREE(mob_proto);
    FREE(mob_index);
    mob_proto = NULL;
    mob_index = NULL;
    num_mobs_in_zone = 0;
    fscanf(index, "%s\n", buf1);
  }

  new_top_mobt = top_of_mobt;
  fclose(index);
}

void discrete_load(FILE *fl, int mode, int reset) {
  int nr = -1, last = 0;
  char line[256];

  for (;;) {
    /*
     * we have to do special processing with the obj files because they have
     * no end-of-record marker :(
     */
    if (mode != DB_BOOT_OBJ || nr < 0)
      if (!get_line(fl, line)) {
        return;
        /* fprintf(stderr, "Format error after mob #%d\n", nr);
         exit(1); */
      }
    if (*line == '$')
      return;

    if (*line == '#') {
      last = nr;
      if (sscanf(line, "#%d", &nr) != 1) {
        fprintf(stderr, "Format error after mob #%d\n", last);
        fflush(NULL);
        exit(1);
      }
      if (nr >= 999999)
        return;
      else
        parse_mobile(fl, nr, reset);
    } else {
      /*      fprintf(stderr, "Format error in %s file near %s #%d\n",
       modes[mode], modes[mode], nr);
       fprintf(stderr, "Offending line: '%s'\n", line);
       exit(1); */
      return;
    }
    reset = 0;
  }
}

long asciiflag_conv(char *flag) {
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

char fread_letter(FILE *fp) {
  char c;
  do {
    c = getc(fp);
  } while (isspace(c));
  return c;
}

void parse_simple_mob(FILE *mob_f, int i, int nr) {
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
    fprintf(stderr,
            "Format error in mob #%d, first line after S flag\n"
            "...expecting line of form '# # # #d#+# #d#+#'\n",
            nr);
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

void parse_another_mob(FILE *mob_f, int i, int nr) {
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
    fprintf(stderr,
            "Format error in mob #%d, first line after A flag\n"
            "...expecting line of form '# # # #d#+# #d#+#'\n",
            nr);
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
      printf("CRITICAL ERROR - 0 attacks\n");
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

#define CASE(test)       if (!matched && !str_cmp(keyword, test) && (matched = 1))
#define RANGE(low, high) (num_arg = MAX((low), MIN((high), (num_arg))))

void interpret_espec(char *keyword, char *value, int i, int nr) {
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

void parse_espec(char *buf, int i, int nr) {
  char *ptr;

  if ((ptr = strchr(buf, ':')) != NULL) {
    *(ptr++) = '\0';
    while (isspace(*ptr))
      ptr++;
  } else
    ptr = "";

  interpret_espec(buf, ptr, i, nr);
}

void parse_enhanced_mob(FILE *mob_f, int i, int nr) {
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

void parse_extensions(FILE *mob_f, int i, int nr) {
  struct mob_attacks_data *new_attack;
  struct mob_equipment_data *new_equip;
  struct mob_action_data *new_action;
  char line[256];
  char *scanner;
  int t[10], cnt;

  while (get_line(mob_f, line)) {
    if (*line == '#' || *line == '>') { /* end of extension fields */
      unget_line(mob_f);                /* unget the line.. arhem? */
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

    } else if (line[0] == 'Q' && line[1] == ' ') { /* quest number */
      if (sscanf(line, "Q %d", t) != 1) {
        fprintf(stderr, "Incorrect Q field in mob #%d\n", nr);
        fflush(NULL);
        exit(1);
      }
      GET_MOB_QUEST(mob_proto + i) = t[0];

    } else
      /* well.. assume it's an E-Spec */
      parse_espec(line, i, nr);
  }
  return; /* end of file */
}

void parse_balanced_mob(FILE *mob_f, int i, int nr) {
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
    fprintf(stderr,
            "Format error in mob #%d, first line after B flag\n"
            "...expecting line of form '# # # #'\n",
            nr);
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

void parse_extended_mob(FILE *mob_f, int i, int nr) {
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
    fprintf(stderr,
            "Format error in mob #%d, first line after X flag\n"
            "...expecting line of form '# # # #d#+#'\n",
            nr);
    fflush(NULL);
    exit(1);
  }
  GET_LEVEL(mob_proto + i) = t[0];
  mob_proto[i].points.hitroll = t[1];
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

  get_line(mob_f, line);
  sscanf(line, " %d %d ", t, t + 1);
  GET_TEMP_GOLD(mob_proto + i) = t[0];
  GET_EXP(mob_proto + i) = t[1];

  get_line(mob_f, line);
  if (sscanf(line, " %d %d %d %d %d %d ", t, t + 1, t + 2, t + 3, t + 4, t + 5) != 6)
    fprintf(stderr,
            "Format error in mob #%d, third line after X flag\n"
            "...expecting line of form '# # # # # #'\n",
            nr);

  mob_proto[i].char_specials.position = t[0];
  mob_proto[i].mob_specials.default_pos = t[1];
  mob_proto[i].player.sex = t[2];
  mob_proto[i].player.class = t[3];
  mob_proto[i].mob_specials.race = t[4];
  mob_proto[i].mob_specials.size = t[5];

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

  parse_extensions(mob_f, i, nr); /* parse extra fields */

  if (mob_proto[i].mob_specials.mob_attacks == NULL) { /* no attack fields was found, add the default */
    CREATE(new_attack, struct mob_attacks_data, 1);
    /* new combat system */
    new_attack->next = mob_proto[i].mob_specials.mob_attacks;
    mob_proto[i].mob_specials.mob_attacks = new_attack;
    new_attack->nodice = balance_table[(int)GET_LEVEL(mob_proto + i)].dam_dicenum;
    new_attack->sizedice = balance_table[(int)GET_LEVEL(mob_proto + i)].dam_dicesize;
    new_attack->damroll = balance_table[(int)GET_LEVEL(mob_proto + i)].dam_add;
    new_attack->attack_type = 0;
    new_attack->attacks = 100;
  }
}

void parse_mobile(FILE *mob_f, int nr, int reset) {
  static int i = 0;
  int g, j, t[10];
  char line[256], *tmpptr, letter;
  char f1[128], f2[128], f3[128];

  if (reset)
    i = 0;

  mob_index[i].virtual = nr;
  mob_index[i].number = 0;
  mob_index[i].func = NULL;

  clear_char(mob_proto + i);

  (mob_proto + i)->player_specials = &dummy_mob;

  sprintf(buf2, "mob vnum %d", nr);

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
  case 'A': /* AnotherWorld monsters */
    parse_another_mob(mob_f, i, nr);
    break;
  case 'E': /* CircleMUD 3.00 E-Spec monsters */
    parse_enhanced_mob(mob_f, i, nr);
    break;
  case 'B': /* Balanced AnotherWorld extended monsters */
    parse_balanced_mob(mob_f, i, nr);
    break;
  case 'X': /* Standard AnotherWorld extended monsters */
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
    (void)mprog_read_programs(mob_f, &mob_index[i]);
  } else
    ungetc(letter, mob_f);

  top_of_mobt = i++;
}

/************************************************************************
 *  procs of a (more or less) general utility nature                        *
 ********************************************************************** */

/* read and allocate space for a '~'-terminated string from a given file */
char *fread_string(FILE *fl, char *error) {
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
      printf("SYSERR: fread_string: string too large (db.c)\n");
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

/* clear ALL the working variables of a char; do NOT free any space alloc'ed */
void clear_char(struct char_data *ch) {
  memset((char *)ch, 0, sizeof(struct char_data));

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

/* the functions */

/* This routine transfers between alpha and numeric forms of the
 *  mob_prog bitvector types. This allows the use of the words in the
 *  mob/script files.
 */

int mprog_name_to_type(char *name) {
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
int fread_number(FILE *fp) {
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
    printf("Fread_number: bad format.\n");
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
void fread_to_eol(FILE *fp) {
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
char *fread_word(FILE *fp) {
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

  printf("SYSERR: Fread_word: word too long.\n");
  fflush(NULL);
  exit(1);
  return NULL;
}

/* This routine reads in scripts of MOBprograms from a file */

MPROG_DATA *mprog_file_read(char *f, MPROG_DATA *mprg, struct index_data *pMobIndex) {

  char MOBProgfile[MAX_INPUT_LENGTH];
  MPROG_DATA *mprg2;
  FILE *progfile;
  bool done = FALSE;

  sprintf(MOBProgfile, "%s/%s", MOB_DIR, f);

  progfile = fopen(MOBProgfile, "r");
  if (!progfile) {
    printf("Mob: %d couldnt open mobprog file\n", pMobIndex->virtual);
    fflush(NULL);
    exit(1);
  }

  mprg2 = mprg;
  switch (fread_letter(progfile)) {
  case '>':
    break;
  case '|':
    printf("empty mobprog file.\n");
    fflush(NULL);
    exit(1);
    break;
  default:
    printf("in mobprog file syntax error.\n");
    fflush(NULL);
    exit(1);
    break;
  }

  while (!done) {
    mprg2->type = mprog_name_to_type(fread_word(progfile));
    switch (mprg2->type) {
    case ERROR_PROG:
      printf("mobprog file type error\n");
      fflush(NULL);
      exit(1);
      break;
    case IN_FILE_PROG:
      printf("mprog file contains a call to file.\n");
      fflush(NULL);
      exit(1);
      break;
    default:
      sprintf(buf2, "Error in file %s\n", f);
      pMobIndex->progtypes = pMobIndex->progtypes | mprg2->type;
      mprg2->arglist = fread_string(progfile, buf2);
      mprg2->comlist = fread_string(progfile, buf2);
      switch (fread_letter(progfile)) {
      case '>':
        mprg2->next = (MPROG_DATA *)malloc(sizeof(MPROG_DATA));
        mprg2 = mprg2->next;
        mprg2->next = NULL;
        break;
      case '|':
        done = TRUE;
        break;
      default:
        printf("in mobprog file %s syntax error.\n", f);
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

/* This procedure is responsible for reading any in_file MOBprograms.
 */

void mprog_read_programs(FILE *fp, struct index_data *pMobIndex) {
  MPROG_DATA *mprg;
  char letter;
  bool done = FALSE;

  if ((letter = fread_letter(fp)) != '>') {
    printf("Load_mobiles: vnum %d MOBPROG char\n", pMobIndex->virtual);
    fflush(NULL);
    exit(1);
  }
  pMobIndex->mobprogs = (MPROG_DATA *)malloc(sizeof(MPROG_DATA));
  mprg = pMobIndex->mobprogs;

  while (!done) {
    mprg->type = mprog_name_to_type(fread_word(fp));
    switch (mprg->type) {
    case ERROR_PROG:
      printf("Load_mobiles: vnum %d MOBPROG type.\n", pMobIndex->virtual);
      fflush(NULL);
      exit(1);
      break;
    case IN_FILE_PROG:
      sprintf(buf2, "Mobprog for mob #%d", pMobIndex->virtual);
      mprg = mprog_file_read(fread_word(fp), mprg, pMobIndex);
      fread_to_eol(fp); /* need to strip off that silly ~*/
      switch (letter = fread_letter(fp)) {
      case '>':
        mprg->next = (MPROG_DATA *)malloc(sizeof(MPROG_DATA));
        mprg = mprg->next;
        mprg->next = NULL;
        break;
      case '|':
        mprg->next = NULL;
        fread_to_eol(fp);
        done = TRUE;
        break;
      default:
        printf("Load_mobiles: vnum %d bad MOBPROG.\n", pMobIndex->virtual);
        fflush(NULL);
        exit(1);
        break;
      }
      break;
    default:
      sprintf(buf2, "Mobprog for mob #%d", pMobIndex->virtual);
      pMobIndex->progtypes = pMobIndex->progtypes | mprg->type;
      mprg->arglist = fread_string(fp, buf2);
      mprg->comlist = fread_string(fp, buf2);
      switch (letter = fread_letter(fp)) {
      case '>':
        mprg->next = (MPROG_DATA *)malloc(sizeof(MPROG_DATA));
        mprg = mprg->next;
        mprg->next = NULL;
        break;
      case '|':
        mprg->next = NULL;
        fread_to_eol(fp);
        done = TRUE;
        break;
      default:
        printf("Load_mobiles: vnum %d bad MOBPROG (%c).\n", pMobIndex->virtual, letter);
        fflush(NULL);
        exit(1);
        break;
      }
      break;
    }
  }

  return;
}
/*-------------------------------------------------------------------*/

/*
 * Save ALL mobiles for a zone to their .mob file, mobs are all
 * saved in Extended format, regardless of whether they have any
 * extended fields.  Thanks to Sammy for ideas on this bit of code.
 */
void discrete_save(FILE *mob_file, int zone) {
  int i, rmob_num, top, counter;
  struct char_data *mob;
  struct mob_attacks_data *attack;
  struct mob_equipment_data *equipment;
  struct mob_action_data *action;
  MPROG_DATA *mob_prog = NULL;

  /*
   * Seach the database for mobs in this zone and save them.
   */
  sprintf(buf1, "%d99", zone);
  top = atoi(buf1);
  for (i = zone * 100; i <= top; i++) {
    fflush(mob_file);
    if ((rmob_num = real_mobile(i)) != -1) {
      if (fprintf(mob_file, "#%d\n", i) < 0) {
        printf("SYSERR: OLC: Cannot write mob file!\n");
        fclose(mob_file);
        return;
      }
      mob = (mob_proto + rmob_num);
      attack = GET_ATTACKS(mob);
      action = GET_ACTION(mob);
      equipment = GET_EQUIP(mob);
      GET_HITROLL(mob) = GET_ST_THAC0(GET_LEVEL(mob));
      GET_AC(mob) = 10 * GET_ST_AC(GET_LEVEL(mob));
      GET_EXP(mob) = GET_ST_EXP(GET_LEVEL(mob));
      GET_HIT(mob) = GET_ST_HPD(GET_LEVEL(mob));
      GET_MANA(mob) = GET_ST_HPS(GET_LEVEL(mob));
      GET_MOVE(mob) = GET_ST_HPB(GET_LEVEL(mob));

      /*
       * Clean up strings.
       */
      strcpy(buf1, ((GET_LDESC(mob) && *GET_LDESC(mob)) ? GET_LDESC(mob) : "undefined"));
      strip_string(buf1);
      strcpy(buf2, ((GET_DDESC(mob) && *GET_DDESC(mob)) ? GET_DDESC(mob) : "undefined"));
      strip_string(buf2);

      fprintf(mob_file,
              "%s~\n"
              "%s~\n"
              "%s~\n"
              "%s~\n",
              ((GET_ALIAS(mob) && *GET_ALIAS(mob)) ? GET_ALIAS(mob) : "undefined"),
              ((GET_SDESC(mob) && *GET_SDESC(mob)) ? GET_SDESC(mob) : "undefined"), buf1, buf2);
      olc_print_bitvectors(mob_file, MOB_FLAGS(mob), NUM_MOB_FLAGS);
      olc_print_bitvectors(mob_file, AFF_FLAGS(mob), NUM_AFF_FLAGS);

      fprintf(mob_file, "%d X ", GET_ALIGNMENT(mob));
      olc_print_bitvectors(mob_file, AFF2_FLAGS(mob), NUM_AFF2_FLAGS);
      fprintf(mob_file,
              "\n"
              "%d %d %d %dd%d+%d\n"
              "%d %d\n"
              "%d %d %d %d %d %d\n",
              GET_LEVEL(mob), GET_HITROLL(mob),                                /* Hitroll -> THAC0 */
              GET_AC(mob) / 10, GET_HIT(mob), GET_MANA(mob), GET_MOVE(mob), 0, /* all gold reset to 0 */
              GET_EXP(mob), GET_POS(mob), GET_DEFAULT_POS(mob), GET_SEX(mob), GET_CLASS(mob), GET_MOB_RACE(mob),
              GET_SIZE(mob));

      while (attack) {
        fprintf(mob_file, "T %d %dd%d+%d %d\n", attack->attacks, attack->nodice, attack->sizedice, attack->damroll,
                attack->attack_type);
        attack = attack->next;
      }

      while (action) {
        fprintf(mob_file, "A %d %d %s\n", action->chance, action->minpos, action->action);
        action = action->next;
      }

      while (equipment) {
        fprintf(mob_file, "E %d %d %d %d\n", equipment->pos, equipment->chance, equipment->vnum, equipment->max);
        equipment = equipment->next;
      }

      if (GET_MOB_QUEST(mob))
        fprintf(mob_file, "Q %d\n", GET_MOB_QUEST(mob));

      /*
       * Deal with Extra stats in case they are there.
       */
      if (GET_STR(mob) != 50)
        fprintf(mob_file, "Str: %d\n", GET_STR(mob));
      if (GET_DEX(mob) != 50)
        fprintf(mob_file, "Dex: %d\n", GET_DEX(mob));
      if (GET_AGI(mob) != 50)
        fprintf(mob_file, "Agi: %d\n", GET_AGI(mob));
      if (GET_INT(mob) != 50)
        fprintf(mob_file, "Int: %d\n", GET_INT(mob));
      if (GET_WIS(mob) != 50)
        fprintf(mob_file, "Wis: %d\n", GET_WIS(mob));
      if (GET_CON(mob) != 50)
        fprintf(mob_file, "Con: %d\n", GET_CON(mob));

      /*. Deal with Resists in case they are there .*/
      for (counter = 1; counter <= NUM_RESISTS; counter++)
        if (GET_RESIST(mob, counter))
          fprintf(mob_file, "%s: %d\n", resist_short_name[counter - 1], GET_RESIST(mob, counter));

      /*
       * Write out the MobProgs.
       */
      mob_prog = GET_MPROG(mob);
      while (mob_prog) {
        strcpy(buf1, mob_prog->arglist);
        strip_string(buf1);
        strcpy(buf2, mob_prog->comlist);
        strip_string(buf2);
        fprintf(mob_file, "%s %s~\n%s", medit_get_mprog_type(mob_prog), buf1, buf2);
        mob_prog = mob_prog->next;
        fprintf(mob_file, "~\n%s", (!mob_prog ? "|\n" : ""));
      }
    }
  }
  fclose(mob_file);
}

int main(void) {
  boot_db();
  return 0;
}
