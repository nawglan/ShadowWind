#include "comm.h"
#include "db.h"
#include "interpreter.h"
#include "screen.h"
#include "structs.h"
#include "utils.h"
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int count_help_records(FILE *fl);
struct help_index_element *build_help_index(int *num, int type);
int hash_keyword(char *s, int size);
char *command_level(int level);
ACMD(do_help);
ACMD(do_wiz_help);
ACMD(do_gen_write);

char *command_level(int level) {
  switch (level) {
  case 51:
  case 52:
    return "IM";
  case 53:
    return "QU";
  case 54:
    return "BU";
  case 55:
    return "AD";
  default:
    return "";
  }
}

ACMD(do_wiz_help) {
  extern int top_of_wiz_helpt;
  extern struct help_index_element *wiz_help_index;
  FILE *wiz_help_fl;
  extern const struct command_info cmd_info[];
  int i, no;
  int chk, bot, top, mid, minlen;
  char nohelpbug[MAX_STRING_LENGTH];
  int bug_cmd;

  wiz_help_fl = fopen(WIZ_HELP_KWRD_FILE, "r");
  if (!wiz_help_fl)
    return;

  if (!ch->desc) {
    fclose(wiz_help_fl);
    return;
  }

  skip_spaces(&argument);

  if (!*argument) {
    size_t buflen = 0;
    g_buf[0] = '\0';
    for (no = 1, i = 1; cmd_info[i].command[0] != '\n'; i++) {
      if (((GET_LEVEL(ch) >= cmd_info[i].minimum_level) || COM_FLAGGED(ch, cmd_info[i].flag)) &&
          (cmd_info[i].minimum_level >= LVL_IMMORT)) {
        buflen += safe_snprintf(g_buf + buflen, MAX_STRING_LENGTH - buflen, "%s[%s%s%s] %s%s%-10s%s", CCCYN(ch, C_SPR),
                                CBWHT(ch, C_SPR), command_level(cmd_info[i].minimum_level), CCCYN(ch, C_SPR),
                                CCNRM(ch, C_SPR), CBBLU(ch, C_SPR), cmd_info[i].command, CCNRM(ch, C_SPR));
        if (!(no % 5))
          buflen += safe_snprintf(g_buf + buflen, MAX_STRING_LENGTH - buflen, "\r\n");
        no++;
      }
    }
    page_string(ch->desc, g_buf, 1);
    fclose(wiz_help_fl);
    return;
  }

  /* make sure that you cant retrieve help on immo commands */
  chk = find_command(argument);
  if (chk != -1 && !COM_FLAGGED(ch, cmd_info[chk].flag)) {
    send_to_char("{YThere is currently no help on that subject. Request Logged.{x\r\n", ch);
    fclose(wiz_help_fl);
    return;
  }

  if (!wiz_help_index) {
    send_to_char("No help available.\r\n", ch);
    fclose(wiz_help_fl);
    return;
  }
  bot = 0;
  top = top_of_wiz_helpt;

  for (;;) {
    mid = (bot + top) >> 1;
    minlen = strlen(argument);

    if (!(chk = strncasecmp(argument, wiz_help_index[mid].keyword, minlen))) {

      /* trace backwards to find first matching entry. Thanks Jeff Fink! */
      while ((mid > 0) && (!(chk = strncasecmp(argument, wiz_help_index[mid - 1].keyword, minlen))))
        mid--;
      fseek(wiz_help_fl, wiz_help_index[mid].pos, SEEK_SET);
      *g_buf2 = '\0';
      for (;;) {
        if (fgets(g_buf, 128, wiz_help_fl) == NULL)
          break;
        if (*g_buf == '#')
          break;
        size_t len = strlen(g_buf);
        if (len > 0 && g_buf[len - 1] == '\n')
          g_buf[len - 1] = '\0'; /* cleave off the trailing \n */
        if (g_buf[0] != '@')
          safe_snprintf(g_buf2 + strlen(g_buf2), MAX_STRING_LENGTH - strlen(g_buf2), "%s\r\n", g_buf);
      }
      page_string(ch->desc, g_buf2, 1);
      fclose(wiz_help_fl);
      return;
    } else if (bot >= top) {
      send_to_char("{YThere is currently no help on that subject. Request Logged.{x\r\n", ch);
      safe_snprintf(nohelpbug, sizeof(nohelpbug), "Wizhelp needed on: %s", argument);
      for (bug_cmd = 0; strcmp(cmd_info[bug_cmd].command, "helpn"); bug_cmd++)
        ;
      do_gen_write(ch, nohelpbug, bug_cmd, SCMD_WHELPN);
      fclose(wiz_help_fl);
      return;
    } else if (chk > 0)
      bot = ++mid;
    else
      top = --mid;
  }
}

ACMD(do_help) {
  extern int top_of_helpt;
  extern struct help_index_element *help_index;
  FILE *help_fl;
  extern char *help;
  char nohelpbug[MAX_STRING_LENGTH];
  int bug_cmd;

  int chk, bot, top, mid, minlen;

  help_fl = fopen(HELP_KWRD_FILE, "r");
  if (!help_fl)
    return;

  if (!ch->desc) {
    fclose(help_fl);
    return;
  }

  skip_spaces(&argument);

  if (!*argument) {
    page_string(ch->desc, help, 1);
    fclose(help_fl);
    return;
  }

  if (!help_index) {
    send_to_char("No help available.\r\n", ch);
    fclose(help_fl);
    return;
  }
  bot = 0;
  top = top_of_helpt;

  for (;;) {
    mid = (bot + top) >> 1;
    minlen = strlen(argument);

    if (!(chk = strncasecmp(argument, help_index[mid].keyword, minlen))) {

      /* trace backwards to find first matching entry. Thanks Jeff Fink! */
      while ((mid > 0) && (!(chk = strncasecmp(argument, help_index[mid - 1].keyword, minlen))))
        mid--;
      fseek(help_fl, help_index[mid].pos, SEEK_SET);
      *g_buf2 = '\0';
      for (;;) {
        if (fgets(g_buf, 128, help_fl) == NULL)
          break;
        if (*g_buf == '#')
          break;
        size_t len = strlen(g_buf);
        if (len > 0 && g_buf[len - 1] == '\n')
          g_buf[len - 1] = '\0'; /* cleave off the trailing \n */
        if (g_buf[0] != '@')
          safe_snprintf(g_buf2 + strlen(g_buf2), MAX_STRING_LENGTH - strlen(g_buf2), "%s\r\n", g_buf);
      }
      page_string(ch->desc, g_buf2, 1);
      fclose(help_fl);
      return;
    } else if (bot >= top) {
      send_to_char("{YThere is currently no help on that subject. Request Logged.{x\r\n", ch);
      safe_snprintf(nohelpbug, sizeof(nohelpbug), "Help needed on: %s", argument);
      for (bug_cmd = 0; strcmp(cmd_info[bug_cmd].command, "helpn"); bug_cmd++)
        ;
      do_gen_write(ch, nohelpbug, bug_cmd, SCMD_HELPN);
      fclose(help_fl);
      return;
    } else if (chk > 0)
      bot = ++mid;
    else
      top = --mid;
  }
}

/* function to count how many hash-mark delimited records exist in a file */
int count_help_records(FILE *fl) {
  char buf[128];
  int count = 0;

  while (fgets(g_buf, 128, fl))
    if (*g_buf == '#') {
      if (g_buf[1] == '~')
        break;
      if (fgets(g_buf, 128, fl) == NULL)
        break;
      while (*g_buf == '@') {
        count++;
        if (fgets(g_buf, 128, fl) == NULL)
          break;
        if (g_buf[1] == '~')
          break;
      }
    }

  return count;
}

struct help_index_element *build_help_index(int *num, int type) {
  int nr = -1, issorted, i;
  struct help_index_element *list = 0, mem;
  char buf[128], *scan;
  long pos;
  int tablesize;
  FILE *fl;

  if (type == HELP_WIZHELP) {
    fl = fopen(WIZ_HELP_KWRD_FILE, "r");
    if (!fl) {
      stderr_log("Wizhelp not found");
      return 0;
    }
  } else {
    fl = fopen(HELP_KWRD_FILE, "r");
    if (!fl) {
      stderr_log("Help not found");
      return 0;
    }
  }
  tablesize = count_help_records(fl);
  safe_snprintf(g_buf, sizeof(g_buf), "   Num Help records: %d", tablesize);
  stderr_log(g_buf);
  rewind(fl);
  CREATE(list, struct help_index_element, tablesize);

  for (;;) {
    size_t len;

    /* skip the text */
    do {
      if (fgets(g_buf, 128, fl) == NULL)
        goto done_parsing;
    } while (*g_buf != '#');
    if (g_buf[1] == '~')
      break;

    pos = ftell(fl);
    if (fgets(g_buf, 128, fl) == NULL)
      break;
    len = strlen(g_buf);
    if (len > 0 && g_buf[len - 1] == '\n')
      g_buf[len - 1] = '\0';
    for (;;) {
      /* extract the keywords */

      scan = g_buf;
      if (scan[1] == '~')
        break;

      for (i = 0; i < (int)strlen(scan); i++)
        scan[i] = tolower(scan[i]);

      nr++;

      list[nr].pos = pos;
      CREATE(list[nr].keyword, char, strlen(scan));
      memcpy(list[nr].keyword, scan + 1, strlen(scan));
      if (fgets(g_buf, 128, fl) == NULL)
        break;
      len = strlen(g_buf);
      if (len > 0 && g_buf[len - 1] == '\n')
        g_buf[len - 1] = '\0';
    }
  }
done_parsing:

  /* we might as well sort the stuff */
  do {
    issorted = 1;
    for (i = 0; i < nr; i++)
      if (str_cmp(list[i].keyword, list[i + 1].keyword) > 0) {
        mem = list[i];
        list[i] = list[i + 1];
        list[i + 1] = mem;
        issorted = 0;
      }
  } while (!issorted);

  *num = nr;
  fclose(fl);
  return (list);
}
