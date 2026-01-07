/* ************************************************************************
 *   File: utility.c                                     Part of CircleMUD *
 *  Usage: various internal functions of a utility nature                  *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <arpa/telnet.h>
#include <netinet/in.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "screen.h"
#include "event.h"
#include "spells.h"
#include "db.h"
#include "handler.h"

extern struct spell_info_type *spells;
extern struct zone_data *zone_table;
extern struct time_data time_info;
extern struct room_data *world;
extern int number_range(int from, int to);
extern int top_of_p_table;
extern struct player_index_element *player_table;
extern int SECS_PER_MUD_HOUR;
extern int SECS_PER_MUD_DAY;
extern int SECS_PER_MUD_MONTH;
extern int SECS_PER_MUD_YEAR;
int find_spell_num(char *name);
char *strip_color(char *from, char *to, int length);

int MIN(int a, int b)
{
  return ((a < b) ? a : b);
}

int MAX(int a, int b)
{
  return ((a > b) ? a : b);
}

int BOUNDED(int a, int b, int c)
{
  return (MAX(a, MIN(b, c)));
}

/* creates a random number in interval [from;to] */
int number(int from, int to)
{
  return (number_range(from, to));
}

/* simulates dice roll */
int dice(int number, int size)
{
  /* changed to use the mitchell-moore algorith. */
  int idice;
  int sum;

  switch (size) {
    case 0:
      return 0;
    case 1:
      return number;
  }

  for (idice = 0, sum = 0; idice < number; idice++)
    sum += number_range(1, size);

  return sum;
}

char *str_dup(char *arg1)
{
  if (arg1 == NULL)
    return NULL;
  else
    return strdup(arg1);
}

/* returns: 0 if equal, 1 if arg1 > arg2, -1 if arg1 < arg2  */
/* scan 'till found different or end of both                 */
int str_cmp(char *arg1, char *arg2)
{
  static int retval;
  if (arg1 == NULL || arg2 == NULL) {
    mudlog("NULL argument in str_cmp", 'E', COM_IMMORT, TRUE);
    return -1;
  }

  retval = strcasecmp(arg1, arg2);
  return retval;
}

/* log a death trap hit */
void log_death_trap(struct char_data * ch)
{
  char buf[150];
  extern struct room_data *world;

  snprintf(buf, MAX_STRING_LENGTH, "%s hit death trap #%d (%s)", GET_NAME(ch), world[ch->in_room].number, world[ch->in_room].name);
  mudlog(buf, 'K', COM_IMMORT, TRUE);
  plog(buf, ch, 0);
}

/* writes a string to the log */
void stderr_log(char *str)
{
  time_t ct;
  char newlog[MAX_STRING_LENGTH];
  char *tmstr = NULL;

  memset(newlog, 0, MAX_STRING_LENGTH);
  ct = time(0);
  tmstr = asctime(localtime(&ct));
  *(tmstr + strlen(tmstr) - 1) = '\0';

  strip_color(str, newlog, strlen(str));
  fprintf(stderr, "%-19.19s :: %s\n", tmstr, newlog);
}

/* the "touch" command, essentially. */
int touch(char *path)
{
  FILE *fl;

  if (!(fl = fopen(path, "a"))) {
    perror(path);
    return -1;
  } else {
    fclose(fl);
    return 0;
  }
}

/* New PROC: syslog by Fen Jul 3, 1992 */
void mudlog(char *str, char type, sbyte level, byte file)
{
  char buf[8 * MAX_STRING_LENGTH], buf1[8 * MAX_STRING_LENGTH + 6];
  char temp[8 * MAX_STRING_LENGTH];
  char newlog[8 * MAX_STRING_LENGTH];
  extern struct descriptor_data *descriptor_list;
  extern long asciiflag_conv(char *flag);
  struct descriptor_data *i;
  struct descriptor_data *next_i;
  char *tmp;
  time_t ct;

  ct = time(0);
  tmp = asctime(localtime(&ct));
  arg[0] = type;
  arg[1] = '\0';

  snprintf(buf, MAX_STRING_LENGTH, ":%s: %s", arg, str);

  snprintf(temp, sizeof(temp), "%-19.19s :: %s\n", tmp, buf);
  memset(newlog, 0, 1024);
  strip_color(temp, newlog, strlen(temp));

  fprintf(stderr, "%s", newlog);

  if (level < 0)
    return;

  snprintf(buf1, MAX_STRING_LENGTH, "[ %s ]\r\n", buf);
  arg[0] = LOWER(type);

  for (i = descriptor_list; i; i = next_i) {
    next_i = i->next;
    if (!i->connected && !PLR_FLAGGED(i->character, PLR_WRITING) && !PLR_FLAGGED(i->character, PLR_EDITING)) {

      if ((COM_FLAGGED(i->character, level) && PRF_FLAGGED(i->character, PRF_LOG) && LOG_FLAGGED(i->character, asciiflag_conv(arg)))) {
        send_to_char(CCGRN(i->character, C_NRM), i->character);
        send_to_char(buf1, i->character);
        send_to_char(CCNRM(i->character, C_NRM), i->character);
      }
    }
  }
}

/* End of Modification */

void sprintbit(unsigned long vektor, char *names[], char *result)
{
  long nr;

  *result = '\0';

  for (nr = 0; vektor; vektor >>= 1) {
    if (IS_SET(1, vektor)) {
      if (*names[nr] != '\n') {
        strcat(result, names[nr]);
        strcat(result, " ");
      } else
        strcat(result, "UNDEFINED ");
    }
    if (*names[nr] != '\n')
      nr++;
  }

  if (!*result)
    strcat(result, "NOBITS ");
}

void sprinttype(int type, char *names[], char *result)
{
  int nr;

  for (nr = 0; (*names[nr] != '\n'); nr++)
    ;
  if (type < nr)
    strcpy(result, names[type]);
  else
    strcpy(result, "UNDEFINED");
}

/* Calculate the REAL time passed over the last t2-t1 centuries (secs) */
struct time_info_data real_time_passed(time_t t2, time_t t1)
{
  long secs;
  struct time_info_data now;

  secs = (long) (t2 - t1);

  now.hours = (secs / SECS_PER_REAL_HOUR) % 24; /* 0..23 hours */
  secs -= SECS_PER_REAL_HOUR * now.hours;

  now.day = (secs / SECS_PER_REAL_DAY); /* 0..34 days  */
  secs -= SECS_PER_REAL_DAY * now.day;

  now.month = -1;
  now.year = -1;

  return now;
}

/* Calculate the MUD time passed over the last t2-t1 centuries (secs) */
struct time_info_data mud_time_passed(time_t t2, time_t t1)
{
  long secs;
  static struct time_info_data now;

  secs = (long) (t2 - t1);

  now.hours = (secs / SECS_PER_MUD_HOUR) % 24; /* 0..23 hours */
  secs -= SECS_PER_MUD_HOUR * now.hours;

  now.day = (secs / SECS_PER_MUD_DAY) % 35; /* 0..34 days  */
  secs -= SECS_PER_MUD_DAY * now.day;

  now.month = (secs / SECS_PER_MUD_MONTH) % 17; /* 0..16 months */
  secs -= SECS_PER_MUD_MONTH * now.month;

  now.year = (secs / SECS_PER_MUD_YEAR); /* 0..XX? years */

  return now;
}

struct time_info_data age(struct char_data * ch)
{
  struct time_info_data player_age;

  player_age = mud_time_passed(time(0), ch->player.time.birth);

  player_age.year += 17; /* All players start at 17 */

  return player_age;
}

/*
 * Turn off echoing (specific to telnet client)
 */
void echo_off(struct descriptor_data *d)
{
  char off_string[] = {(char) IAC, (char) WILL, (char) TELOPT_ECHO, (char) 0, };

  SEND_TO_Q(off_string, d);
}

/*
 * Turn on echoing (specific to telnet client)
 */
void echo_on(struct descriptor_data *d)
{
  char on_string[] = {(char) IAC, (char) WONT, (char) TELOPT_ECHO, (char) TELOPT_NAOFFD, (char) TELOPT_NAOCRD, (char) 0, };

  SEND_TO_Q(on_string, d);
}

/* Check if making CH follow VICTIM will create an illegal */
/* Follow "Loop/circle"                                    */
bool circle_follow(struct char_data * ch, struct char_data * victim)
{
  struct char_data *k;

  for (k = victim; k; k = k->master) {
    if (k == ch)
      return TRUE;
  }

  return FALSE;
}

/* Called when stop following persons, or stopping charm */
/* This will NOT do if a character quits/dies!!          */
void stop_follower(struct char_data * ch)
{
  struct follow_type *j, *k;
  int temp = find_spell_num("charm");
  int spellnum = 0;

  assert(ch->master);
  if (temp >= 0)
    spellnum = spells[temp].spellindex;

  if (IS_AFFECTED(ch, AFF_CHARM)) {
    act("You realize that $N is a jerk!", FALSE, ch, 0, ch->master, TO_CHAR);
    act("$n realizes that $N is a jerk!", FALSE, ch, 0, ch->master, TO_NOTVICT);
    act("$n hates your guts!", FALSE, ch, 0, ch->master, TO_VICT);
    if (affected_by_spell(ch, spellnum))
      affect_from_char(ch, spellnum);
  } else {
    act("You stop following $N.", FALSE, ch, 0, ch->master, TO_CHAR);
    if (CAN_SEE(ch->master, ch)) {
      act("$n stops following $N.", TRUE, ch, 0, ch->master, TO_NOTVICT);
      act("$n stops following you.", TRUE, ch, 0, ch->master, TO_VICT);
    }
  }

  if (ch->master->followers->follower == ch) { /* Head of follower-list? */
    k = ch->master->followers;
    ch->master->followers = k->next;
    FREE(k);
  } else { /* locate follower who is not head of list */
    for (k = ch->master->followers; k->next->follower != ch; k = k->next)
      ;

    j = k->next;
    k->next = j->next;
    FREE(j);
  }

  ch->master = NULL;
  REMOVE_BIT(AFF_FLAGS(ch), AFF_CHARM | AFF_GROUP);
}

/* Called when a character that follows/is followed dies */
void die_follower(struct char_data * ch)
{
  struct follow_type *j, *k;

  if (ch->master)
    stop_follower(ch);

  for (k = ch->followers; k; k = j) {
    j = k->next;
    stop_follower(k->follower);
  }
}

/* Do NOT call this before having checked if a circle of followers */
/* will arise. CH will follow leader                               */
void add_follower(struct char_data * ch, struct char_data * leader)
{
  struct follow_type *k;

  assert(!ch->master);

  ch->master = leader;

  CREATE(k, struct follow_type, 1);

  k->follower = ch;
  k->next = leader->followers;
  leader->followers = k;

  act("You now follow $N.", FALSE, ch, 0, leader, TO_CHAR);
  act("$n starts following you.", TRUE, ch, 0, leader, TO_VICT);
  act("$n starts to follow $N.", TRUE, ch, 0, leader, TO_NOTVICT);
}

/*
 * get_line reads the next non-blank line off of the input stream.
 * The newline character is removed from the input.  Lines which begin
 * with '*' are considered to be comments.
 *
 * Returns the number of lines advanced in the file.
 */
int get_line(FILE * fl, char *buf)
{
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

void unget_line(FILE * fl)
{

  do {
    fseek(fl, -2, SEEK_CUR);
  } while (fgetc(fl) != '\n');

}

/* string manipulation fucntion originally by Darren Wilson */
/* (wilson@shark.cc.cc.ca.us) improved and bug fixed by Chris (zero@cnw.com) */
/* completely re-written again by M. Scott 10/15/96 (scottm@workcommn.net), */
/* substitute appearances of 'pattern' with 'replacement' in string */
/* and return the # of replacements */
int replace_str(char **string, char *pattern, char *replacement, int rep_all, int max_size)
{
  char *replace_buffer = NULL;
  char *flow, *jetsam, temp;
  int len, i;

  if ((strlen(*string) - strlen(pattern)) + strlen(replacement) > max_size)
    return -1;

  CREATE(replace_buffer, char, max_size);
  i = 0;
  jetsam = *string;
  flow = *string;
  *replace_buffer = '\0';
  if (rep_all) {
    while ((flow = (char *) strstr(flow, pattern)) != NULL) {
      i++;
      temp = *flow;
      *flow = '\0';
      if ((strlen(replace_buffer) + strlen(jetsam) + strlen(replacement)) > max_size) {
        i = -1;
        break;
      }
      strcat(replace_buffer, jetsam);
      strcat(replace_buffer, replacement);
      *flow = temp;
      flow += strlen(pattern);
      jetsam = flow;
    }
    strcat(replace_buffer, jetsam);
  } else {
    if ((flow = (char *) strstr(*string, pattern)) != NULL) {
      i++;
      flow += strlen(pattern);
      len = ((char *) flow - (char *) *string) - strlen(pattern);

      strncpy(replace_buffer, *string, len);
      strcat(replace_buffer, replacement);
      strcat(replace_buffer, flow);
    }
  }
  if (i == 0)
    return 0;
  if (i > 0) {
    RECREATE(*string, char, strlen(replace_buffer) + 3);
    strcpy(*string, replace_buffer);
  }
  FREE(replace_buffer);
  return i;
}

/* re-formats message type formatted char * */
/* (for strings edited with d->str) (mostly olc and mail)     */
void format_text(char **ptr_string, int mode, struct descriptor_data *d, int maxlen)
{
  int total_chars, cap_next = TRUE, cap_next_next = FALSE;
  char *flow, *start = NULL, temp;
  /* warning: do not edit messages with max_str's of over this value */
  char formated[MAX_STRING_LENGTH];

  flow = *ptr_string;
  if (!flow)
    return;

  if (IS_SET(mode, FORMAT_INDENT)) {
    strcpy(formated, "   ");
    total_chars = 3;
  } else {
    *formated = '\0';
    total_chars = 0;
  }

  while (*flow != '\0') {
    while ((*flow == '\n') || (*flow == '\r') || (*flow == '\f') || (*flow == '\t') || (*flow == '\v') || (*flow == ' '))
      flow++;

    if (*flow != '\0') {

      start = flow++;
      while ((*flow != '\0') && (*flow != '\n') && (*flow != '\r') && (*flow != '\f') && (*flow != '\t') && (*flow != '\v') && (*flow != ' ') && (*flow != '.') && (*flow != '?') && (*flow != '!'))
        flow++;

      if (cap_next_next) {
        cap_next_next = FALSE;
        cap_next = TRUE;
      }

      /* this is so that if we stopped on a sentance .. we move off the sentance delim. */
      while ((*flow == '.') || (*flow == '!') || (*flow == '?')) {
        cap_next_next = TRUE;
        flow++;
      }

      temp = *flow;
      *flow = '\0';

      if ((total_chars + strlen(start) + 1) > 76) {
        strcat(formated, "\r\n");
        total_chars = 0;
      }

      if (!cap_next) {
        if (total_chars > 0) {
          strcat(formated, " ");
          total_chars++;
        }
      } else {
        cap_next = FALSE;
        *start = UPPER(*start);
      }

      total_chars += strlen(start);
      strcat(formated, start);

      *flow = temp;
    }

    if (cap_next_next) {
      if ((total_chars + 3) > 76) {
        strcat(formated, "\r\n");
        total_chars = 0;
      } else {
        strcat(formated, " ");
        total_chars += 1;
      }
    }
  }
  strcat(formated, "\r\n");

  if (strlen(formated) > maxlen)
    formated[maxlen] = '\0';
  RECREATE(*ptr_string, char, MIN(maxlen, strlen(formated)+3));
  strcpy(*ptr_string, formated);
}

int get_filename(char *orig_name, char *filename, int mode)
{
  char *prefix, *middle, *suffix, *ptr, name[64];

  switch (mode) {
    case CRASH_FILE:
      prefix = "plrobjs";
      suffix = "objs";
      break;
    case ETEXT_FILE:
      prefix = "plrtext";
      suffix = "text";
      break;
    case PLOG_FILE:
      prefix = "plrlogs";
      suffix = "log";
      break;
    case PDAT_FILE:
      prefix = "plrdata";
      suffix = "dat";
      break;
    case PTDAT_FILE:
      prefix = "plrdata";
      suffix = "tdat";
      break;
    default:
      return 0;
  }

  if (!orig_name || !*orig_name)
    return 0;

  strcpy(name, orig_name);
  for (ptr = name; *ptr; ptr++)
    *ptr = LOWER(*ptr);

  switch (LOWER(*name)) {
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
      middle = "A-E";
      break;
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
      middle = "F-J";
      break;
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
      middle = "K-O";
      break;
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
      middle = "P-T";
      break;
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
      middle = "U-Z";
      break;
    default:
      middle = "ZZZ";
      break;
  }

  sprintf(filename, "%s/%s/%s.%s", prefix, middle, name, suffix);
  return 1;
}

void plog(char *logtext, struct char_data * ch, int level)
{
  time_t ltime;
  FILE *fil = NULL;
  char *tmstr;
  char fname[MAX_INPUT_LENGTH];
  char newlog[MAX_STRING_LENGTH];

  if (IS_NPC(ch))
    return;

  if (!get_filename(GET_NAME(ch), fname, PLOG_FILE)) {
    stderr_log("SYSERR: Error when getting filename, plog()");
    return;
  }

  if (!(fil = fopen(fname, "a"))) {
    stderr_log("SYSERR: Error when opening file, plog()");
    return;
  }

  memset(newlog, 0, MAX_STRING_LENGTH);
  strip_color(logtext, newlog, strlen(logtext));
  ltime = time(0);
  tmstr = asctime(localtime(&ltime));
  *(tmstr + strlen(tmstr) - 1) = '\0';
  fprintf(fil, "%d %ld %-19.19s :: %s\n", level, ltime, tmstr, newlog);

  fclose(fil);
  return;

}

/* clean a plog file, remove old records */

void clean_log_file(char *name)
{
  time_t ltime;
  FILE *fil = NULL;
  FILE *temp = NULL;
  char fname[MAX_INPUT_LENGTH];
  long i;
  int lvl;

  if (!get_filename(name, fname, PLOG_FILE))
    return;

  if (!(fil = fopen(fname, "r"))) {
    if (errno != ENOENT) { /* if it fails, NOT because of no file */
      snprintf(buf1, MAX_STRING_LENGTH, "SYSERR: READING PLOG FILE %s (5)", fname);
      perror(buf1);
      return;
    }
    return;
  }

  if (!(temp = fopen("temp.plog", "w"))) {
    if (errno != ENOENT) { /* if it fails, NOT because of no file */
      snprintf(buf1, MAX_STRING_LENGTH, "SYSERR: READING PLOG FILE %s (5)", fname);
      perror(buf1);
      return;
    }
    return;
  }

  ltime = time(0); /* current UNIX sys time */
  while (!feof(fil)) {
    *buf = '\0';
    i = 0;
    fscanf(fil, "%d %ld ", &lvl, &i);
    fgets(buf, MAX_INPUT_LENGTH, fil);
    if (i + (7 * 24 * 60 * 60) > ltime && *buf != '\0') /* record is not timed-out yet */
      fprintf(temp, "%d %ld %s", lvl, i, buf);
  }
  fclose(fil);
  fclose(temp);

  /* now copy the info back to the playerlog file (must be a better way
   * to do this???						       */

  if (!(fil = fopen(fname, "w"))) {
    if (errno != ENOENT) { /* if it fails, NOT because of no file */
      snprintf(buf1, MAX_STRING_LENGTH, "SYSERR: WRITING PLOG FILE %s (5)", fname);
      perror(buf1);
      return;
    }
    return;
  }

  if (!(temp = fopen("temp.plog", "r"))) {
    if (errno != ENOENT) { /* if it fails, NOT because of no file */
      snprintf(buf1, MAX_STRING_LENGTH, "SYSERR: READING TEMP.PLOG FILE (5)");
      perror(buf1);
      return;
    }
    return;
  }

  while (!feof(temp)) {
    *buf = '\0';
    fscanf(temp, "%d %ld ", &lvl, &i);
    fgets(buf, MAX_INPUT_LENGTH, temp);
    if (*buf != '\0')
      fprintf(fil, "%d %ld %s", lvl, i, buf);
  }
  fclose(fil);
  fclose(temp);

}

/* Scan through the plog files and delete timed-out entries */
void update_log_file(void)
{
  extern struct char_data *character_list;
  struct char_data *ch = character_list;

  for (; ch; ch = ch->next)
    clean_log_file(GET_NAME(ch));
  return;
}

char *strip_color(char *from, char *to, int length)
{
  char *tmp = NULL;
  char *origto = to;
  int i;

  for (tmp = from, i = 0; tmp && i < length; *to = *tmp, tmp++, to++, i++)
    if (*tmp == '{') {
      if (*(tmp + 1) != '\0' && *(tmp + 1) != '{')
        tmp += 2;
      else
        tmp++;
    }
  *to = '\0';

  return origto;
}

char *make_money_text(int coins)
{
  int p, g, s;
  static char buf[MAX_INPUT_LENGTH];

  buf[0] = 0;
  if (!coins) {
    snprintf(buf, MAX_STRING_LENGTH, "free");
    return (buf);
  }
  p = coins / 1000;
  coins -= p * 1000;
  g = coins / 100;
  coins -= g * 100;
  s = coins / 10;
  coins -= s * 10;

  if (p) {
    snprintf(buf, MAX_STRING_LENGTH, "{x%d {Wp{x", p);
  }

  if (g) {
    if (p && !s && !coins)
      sprintf(buf + strlen(buf), ", and ");
    else if (p)
      sprintf(buf + strlen(buf), ", ");
    sprintf(buf + strlen(buf), "{x%d {Yg{x", g);
  }
  if (s) {
    if ((p || g) && !coins)
      sprintf(buf + strlen(buf), ", and ");
    else if (p || g)
      sprintf(buf + strlen(buf), ", ");
    sprintf(buf + strlen(buf), "{x%d s", s);
  }
  if (coins) {
    if (p || g || s)
      sprintf(buf + strlen(buf), ", and ");
    sprintf(buf + strlen(buf), "{x%d {yc{x", coins);
  }
  sprintf(buf + strlen(buf), " coin%s{x", ((p + g + s + coins) > 1) ? "s" : "");

  return buf;
}

char *make_money_text_nocolor(int coins)
{
  int p, g, s;
  static char buf[MAX_INPUT_LENGTH];

  buf[0] = 0;
  if (!coins)
    return (buf);
  p = coins / 1000;
  coins -= p * 1000;
  g = coins / 100;
  coins -= g * 100;
  s = coins / 10;
  coins -= s * 10;

  if (p) {
    snprintf(buf, MAX_STRING_LENGTH, "%d p", p);
  }

  if (g) {
    if (p && !s && !coins)
      sprintf(buf + strlen(buf), ", and ");
    else if (p)
      sprintf(buf + strlen(buf), ", ");
    sprintf(buf + strlen(buf), "%d g", g);
  }
  if (s) {
    if ((p || g) && !coins)
      sprintf(buf + strlen(buf), ", and ");
    else if (p || g)
      sprintf(buf + strlen(buf), ", ");
    sprintf(buf + strlen(buf), "%d s", s);
  }
  if (coins) {
    if (p || g || s)
      sprintf(buf + strlen(buf), ", and ");
    sprintf(buf + strlen(buf), "%d c", coins);
  }
  sprintf(buf + strlen(buf), " coin%s", ((p + g + s + coins) > 1) ? "s" : "");

  return buf;
}

int IS_DARK(int room)
{
  int retval = 0;

  if (!IS_SET(world[room].room_flags, ROOM_LIGHT)) {
    retval = 1;
  }
  if (IS_SET(world[room].room_flags, ROOM_DARK) || GET_SECT(room) == SECT_UNDERGROUND) {
    retval = 1;
  }
  if (GET_SECT(room) != SECT_INSIDE && GET_SECT(room) != SECT_CITY) {
    if (!IS_SET(SEASON_FLAGS(world[room].zone), MOON_VISIBLE | SUN_VISIBLE)) {
      retval = 1;
      if (!(world[room].light) && !GET_ZONE_LIGHT(world[room].zone)) {
        retval = 1;
      } else {
        retval = 0;
      }
    } else {
      retval = 0;
    }
  } else {
    retval = 0;
  }
  return retval;
}

int CAN_SEE_IN_DARK(struct char_data *ch)
{
  int retval = 0;

  if (IS_NPC(ch)) {
    if (AFF_FLAGGED(ch, AFF_NIGHTVISION | AFF_INFRAVISION)) {
      retval = 1;
    }
  } else {
    if (AFF_FLAGGED(ch, AFF_NIGHTVISION)) {
      retval = 1;
    }
    if (PRF_FLAGGED(ch, PRF_HOLYLIGHT) || !IS_DARK(ch->in_room)) {
      retval = 1;
    }
  }

  if (AFF_FLAGGED(ch, AFF_BLIND)) {
    retval = 0;
  }

  return retval;
}

int LIGHT_OK(struct char_data *ch)
{
  int retval = 1;

  if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch)) {
    retval = 0;
  }
  if (AFF_FLAGGED(ch, AFF_BLIND)) {
    retval = 0;
  }
  return retval;
}

int INVIS_OK(struct char_data *ch, struct char_data *vict)
{
  int retval = 1;

  if (IS_AFFECTED(vict, AFF_INVISIBLE) && !AFF_FLAGGED(ch, AFF_DETECT_INVIS | AFF_SEEING)) {
    retval = 0;
  }
  if (IS_AFFECTED(vict, AFF_HIDE) && !AFF_FLAGGED(ch, AFF_SENSE_LIFE | AFF_SEEING)) {
    retval = 0;
  }
  if (GET_LEVEL(ch) < GET_INVIS_LEV(vict)) {
    retval = 0;
  }

  return retval;
}

int CAN_SEE(struct char_data *ch, struct char_data *vict)
{
  int retval = 0;

  if (ch == vict) {
    retval = 1;
  }

  if (LIGHT_OK(ch) && INVIS_OK(ch, vict)) {
    retval = 1;
  }

  if (!IS_NPC(ch) && COM_FLAGGED(ch, COM_ADMIN)) {
    retval = 1;
  }

  return retval;
}

#define MAGE 0
#define CLERIC 1

int best_class(int type, struct spell_info_type *sinfo)
{
  int retval = 0;

  if (type == MAGE) {
    retval = CLASS_SORCERER;
    if (sinfo->min_level[CLASS_WIZARD] < sinfo->min_level[retval]) {
      retval = CLASS_WIZARD;
    }
    if (sinfo->min_level[CLASS_ENCHANTER] < sinfo->min_level[retval]) {
      retval = CLASS_ENCHANTER;
    }
    if (sinfo->min_level[CLASS_CONJURER] < sinfo->min_level[retval]) {
      retval = CLASS_CONJURER;
    }
    if (sinfo->min_level[CLASS_NECROMANCER] < sinfo->min_level[retval]) {
      retval = CLASS_NECROMANCER;
    }
  } else if (type == CLERIC) {
    retval = CLASS_PRIEST;
    if (sinfo->min_level[CLASS_CLERIC] < sinfo->min_level[retval]) {
      retval = CLASS_CLERIC;
    }
    if (sinfo->min_level[CLASS_DRUID] < sinfo->min_level[retval]) {
      retval = CLASS_DRUID;
    }
    if (sinfo->min_level[CLASS_SHAMAN] < sinfo->min_level[retval]) {
      retval = CLASS_SHAMAN;
    }
  }
  return retval;
}

int GET_CIRCLE_DIFF(struct char_data *ch, struct spell_info_type *sinfo)
{
  int retval = 0;

  if (IS_MOB(ch)) {
    if (MOB_FLAGGED(ch, MOB_HAS_MAGE)) {
      retval = (((GET_LEVEL(ch) + 4) / 5) - ((sinfo->min_level[best_class(MAGE, sinfo)] + 4) / 5));
    } else if (MOB_FLAGGED(ch, MOB_HAS_CLERIC)) {
      retval = (((GET_LEVEL(ch) + 4) / 5) - ((sinfo->min_level[best_class(CLERIC, sinfo)] + 4) / 5));
    } else {
      retval = (((GET_LEVEL(ch) + 4) / 5) - ((sinfo->min_level[CLASS_WARRIOR] + 4) / 5));
    }
    if (MOB_FLAGGED(ch, MOB_HAS_MAGE) && MOB_FLAGGED(ch, MOB_HAS_CLERIC)) {
      if (sinfo->min_level[best_class(MAGE, sinfo)] <= sinfo->min_level[best_class(CLERIC, sinfo)]) {
        retval = (((GET_LEVEL(ch) + 4) / 5) - ((sinfo->min_level[best_class(MAGE, sinfo)] + 4) / 5));
      } else {
        retval = (((GET_LEVEL(ch) + 4) / 5) - ((sinfo->min_level[best_class(CLERIC, sinfo)] + 4) / 5));
      }
    }
  } else {
    retval = (((GET_LEVEL(ch) + 4) / 5) - ((sinfo->min_level[(int) GET_CLASS(ch)] + 4) / 5));
  }

  return retval;
}

int GET_SPELL_CIRCLE(struct char_data *ch, struct spell_info_type *sinfo)
{
  int retval = 0;

  if (IS_MOB(ch)) {
    if (MOB_FLAGGED(ch, MOB_HAS_MAGE)) {
      retval = (sinfo->min_level[best_class(MAGE, sinfo)] + 4) / 5;
    } else if (MOB_FLAGGED(ch, MOB_HAS_CLERIC)) {
      retval = (sinfo->min_level[best_class(CLERIC, sinfo)] + 4) / 5;
    } else {
      retval = 11;
    }
    if (MOB_FLAGGED(ch, MOB_HAS_MAGE) && MOB_FLAGGED(ch, MOB_HAS_CLERIC)) {
      if (sinfo->min_level[best_class(MAGE, sinfo)] <= sinfo->min_level[best_class(CLERIC, sinfo)]) {
        retval = (sinfo->min_level[best_class(MAGE, sinfo)] + 4) / 5;
      } else {
        retval = (sinfo->min_level[best_class(CLERIC, sinfo)] + 4) / 5;
      }
    }
  } else {
    retval = (sinfo->min_level[(int) GET_CLASS(ch)] + 4) / 5;
  }

  return retval;
}
