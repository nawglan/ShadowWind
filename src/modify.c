/* ************************************************************************
 *   File: modify.c                                      Part of CircleMUD *
 *  Usage: Run-time modification of game variables                         *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "comm.h"
#include "event.h"
#include "spells.h"
#include "mail.h"
#include "boards.h"
#include "olc.h"

void show_string(struct descriptor_data *d, char *input);
void show_skills(struct char_data *ch, struct char_data *vict);

/* action modes for parse_action */
#define PARSE_FORMAT		0 
#define PARSE_REPLACE		1 
#define PARSE_HELP		2 
#define PARSE_DELETE		3
#define PARSE_INSERT		4
#define PARSE_LIST_NORM		5
#define PARSE_LIST_NUM		6
#define PARSE_EDIT		7

/* maximum length for text field x+1 */
int length[] = {15, 60, 256, 240, 60};

/*. This procedure replaces the '\r\n' with a space so that we can have
 modifiable screensizes.
 .*/

void convert_linefeed(char *buffer)
{
  char *pointer;

  pointer = buffer;
  while ((pointer = strchr(pointer, '\r')))
    *pointer = ' ';
  pointer = buffer;
  while ((pointer = strchr(pointer, '\n')))
    *pointer = ' ';
}

/*. This procedure places the '\r\n' in the string at the desired location.
 .*/

char *reformat_string(char *buffer, struct char_data *ch)
{
  char *oldbuffer = buffer;

  ch->charnum = 0;
  ch->pagesize = GET_SCREEN_WIDTH(ch) * GET_SCREEN_HEIGHT(ch);

  if (ch->pagesize < strlen(buffer))
    ch->pointer = (char*) malloc(strlen(buffer) * 4);
  else
    ch->pointer = (char*) malloc(strlen(buffer) * 2);

  ch->temp = ch->pointer;
  while (*buffer) {
    *ch->temp = *buffer;
    if (!*buffer)
      break;
    if (isprint(*buffer)) {
      if (*buffer == '{')
        ch->charnum -= 2;
      else
        ch->charnum++;
    }
    if (*buffer == '\r' || *buffer == '\n') {
      ch->charnum = 0;
    }
    if (ch->charnum == GET_SCREEN_WIDTH(ch) || !*buffer) {
      ch->charnum = 0;
      if (*ch->temp == ' ' || ispunct(*ch->temp)) {
        if (*ch->temp != ' ')
          ch->temp++;
        *ch->temp = '\r';
        ch->temp++;
        *ch->temp = '\n';
      } else {
        ch->wordp = strrchr(ch->temp, ' ');
        if (ch->wordp) {
          ch->wordp++;
          strcpy(ch->wordwrap, ch->wordp);
          ch->wordp--;
          *ch->wordp = '\r';
          ch->wordp++;
          *ch->wordp = '\n';
          ch->temp = ch->wordp;
          strcpy(ch->temp, ch->wordwrap);
          ch->temp += strlen(ch->wordwrap);
          ch->charnum += strlen(ch->wordwrap);
        }
      }
    }
    ch->temp++;
    buffer++;
    *ch->temp = '\0'; /* ensure that string is null terminated */
  }
  buffer = oldbuffer;
  return ch->pointer;
}

/* ************************************************************************
 *  modification of malloc'ed strings                                      *
 ************************************************************************ */

/*  handle some editor commands */
void parse_action(int command, char *string, struct descriptor_data *d)
{
  int indent = 0, rep_all = 0, flags = 0, total_len, replaced;
  register int j = 0;
  int i, line_low, line_high;
  char *s, *t, temp;

  switch (command) {
    case PARSE_HELP:
      safe_snprintf(buf, MAX_STRING_LENGTH, "Editor command formats: /<letter>\r\n\r\n"
          "/a         -  aborts editor\r\n"
          "/c         -  clears buffer\r\n"
          "/d#        -  deletes a line #\r\n"
          "/e# <text> -  changes the line at # with <text>\r\n"
          "/f         -  formats text\r\n"
          "/fi        -  indented formatting of text\r\n"
          "/h         -  list text editor commands\r\n"
          "/i# <text> -  inserts <text> before line #\r\n"
          "/l         -  lists buffer\r\n"
          "/n         -  lists buffer with line numbers\r\n"
          "/r 'a' 'b' -  replace 1st occurance of text <a> in buffer with text <b>\r\n"
          "/ra 'a' 'b'-  replace all occurances of text <a> within buffer with text <b>\r\n"
          "              usage: /r[a] 'pattern' 'replacement'\r\n"
          "/s         -  saves text\r\n");
      SEND_TO_Q(buf, d);
      break;
    case PARSE_FORMAT:
      while (isalpha(string[j]) && j < 2) {
        switch (string[j]) {
          case 'i':
            if (!indent) {
              indent = 1;
              flags += FORMAT_INDENT;
            }
            break;
          default:
            break;
        }
        j++;
      }
      format_text(d->str, flags, d, d->max_str);
      safe_snprintf(buf, MAX_STRING_LENGTH, "Text formarted with%s indent.\r\n", (indent ? "" : "out"));
      SEND_TO_Q(buf, d);
      break;
    case PARSE_REPLACE:
      while (isalpha(string[j]) && j < 2) {
        switch (string[j]) {
          case 'a':
            if (!indent) {
              rep_all = 1;
            }
            break;
          default:
            break;
        }
        j++;
      }
      s = strtok(string, "'");
      if (s == NULL) {
        SEND_TO_Q("Invalid format.\r\n", d);
        return;
      }
      s = strtok(NULL, "'");
      if (s == NULL) {
        SEND_TO_Q("Target string must be enclosed in single quotes.\r\n", d);
        return;
      }
      t = strtok(NULL, "'");
      if (t == NULL) {
        SEND_TO_Q("No replacement string.\r\n", d);
        return;
      }
      t = strtok(NULL, "'");
      if (t == NULL) {
        SEND_TO_Q("Replacement string must be enclosed in single quotes.\r\n", d);
        return;
      }
      total_len = ((strlen(t) - strlen(s)) + strlen(*d->str));
      if (total_len <= d->max_str) {
        if ((replaced = replace_str(d->str, s, t, rep_all, d->max_str)) > 0) {
          safe_snprintf(buf, MAX_STRING_LENGTH, "Replaced %d occurance%sof '%s' with '%s'.\r\n", replaced, ((replaced != 1) ? "s " : " "), s, t);
          SEND_TO_Q(buf, d);
        } else if (replaced == 0) {
          safe_snprintf(buf, MAX_STRING_LENGTH, "String '%s' not found.\r\n", s);
          SEND_TO_Q(buf, d);
        } else {
          SEND_TO_Q("ERROR: Replacement string causes buffer overflow, aborted replace.\r\n", d);
        }
      } else
        SEND_TO_Q("Not enough space left in buffer.\r\n", d);
      break;
    case PARSE_DELETE:
      switch (sscanf(string, " %d - %d ", &line_low, &line_high)) {
        case 0:
          SEND_TO_Q("You must specify a line number or range to delete.\r\n", d);
          return;
        case 1:
          line_high = line_low;
          break;
        case 2:
          if (line_high < line_low) {
            SEND_TO_Q("That range is invalid.\r\n", d);
            return;
          }
          break;
      }

      i = 1;
      total_len = 1;
      if ((s = *d->str) == NULL) {
        SEND_TO_Q("Buffer is empty.\r\n", d);
        return;
      }
      if (line_low > 0) {
        while (s && (i < line_low))
          if ((s = strchr(s, '\n')) != NULL) {
            i++;
            s++;
          }
        if ((i < line_low) || (s == NULL)) {
          SEND_TO_Q("Line(s) out of range; not deleting.\r\n", d);
          return;
        }

        t = s;
        while (s && (i < line_high))
          if ((s = strchr(s, '\n')) != NULL) {
            i++;
            total_len++;
            s++;
          }
        if ((s) && ((s = strchr(s, '\n')) != NULL)) {
          s++;
          while (*s != '\0')
            *(t++) = *(s++);
        } else
          total_len--;
        *t = '\0';
        RECREATE(*d->str, char, strlen(*d->str) + 3);
        safe_snprintf(buf, MAX_STRING_LENGTH, "%d line%sdeleted.\r\n", total_len, ((total_len != 1) ? "s " : " "));
        SEND_TO_Q(buf, d);
      } else {
        SEND_TO_Q("Invalid line numbers to delete must be higher than 0.\r\n", d);
        return;
      }
      break;
    case PARSE_LIST_NORM:
      /* note: my buf,buf1,buf2 vars are defined at 32k sizes so they
       * are prolly ok fer what i want to do here. */
      *buf = '\0';
      if (*string != '\0')
        switch (sscanf(string, " %d - %d ", &line_low, &line_high)) {
          case 0:
            line_low = 1;
            line_high = 999999;
            break;
          case 1:
            line_high = line_low;
            break;
        }
      else {
        line_low = 1;
        line_high = 999999;
      }

      if (line_low < 1) {
        SEND_TO_Q("Line numbers must be greater than 0.\r\n", d);
        return;
      }
      if (line_high < line_low) {
        SEND_TO_Q("That range is invalid.\r\n", d);
        return;
      }
      *buf = '\0';
      if ((line_high < 999999) || (line_low > 1)) {
        safe_snprintf(buf, MAX_STRING_LENGTH, "Current buffer range [%d - %d]:\r\n", line_low, line_high);
      }
      i = 1;
      total_len = 0;
      s = *d->str;
      while (s && (i < line_low))
        if ((s = strchr(s, '\n')) != NULL) {
          i++;
          s++;
        }
      if ((i < line_low) || (s == NULL)) {
        SEND_TO_Q("Line(s) out of range; no buffer listing.\r\n", d);
        return;
      }

      t = s;
      while (s && (i <= line_high))
        if ((s = strchr(s, '\n')) != NULL) {
          i++;
          total_len++;
          s++;
        }
      if (s) {
        temp = *s;
        *s = '\0';
        strcat(buf, t);
        *s = temp;
      } else
        strcat(buf, t);
      /* this is kind of annoying.. will have to take a poll and see..
       sprintf(buf + strlen(buf), "\r\n%d line%sshown.\r\n", total_len,
       ((total_len != 1)?"s ":" "));
       */
      page_string(d, buf, TRUE);
      break;
    case PARSE_LIST_NUM:
      /* note: my buf,buf1,buf2 vars are defined at 32k sizes so they
       * are prolly ok fer what i want to do here. */
      *buf = '\0';
      if (*string != '\0')
        switch (sscanf(string, " %d - %d ", &line_low, &line_high)) {
          case 0:
            line_low = 1;
            line_high = 999999;
            break;
          case 1:
            line_high = line_low;
            break;
        }
      else {
        line_low = 1;
        line_high = 999999;
      }

      if (line_low < 1) {
        SEND_TO_Q("Line numbers must be greater than 0.\r\n", d);
        return;
      }
      if (line_high < line_low) {
        SEND_TO_Q("That range is invalid.\r\n", d);
        return;
      }
      *buf = '\0';
      i = 1;
      total_len = 0;
      s = *d->str;
      while (s && (i < line_low))
        if ((s = strchr(s, '\n')) != NULL) {
          i++;
          s++;
        }
      if ((i < line_low) || (s == NULL)) {
        SEND_TO_Q("Line(s) out of range; no buffer listing.\r\n", d);
        return;
      }

      t = s;
      while (s && (i <= line_high))
        if ((s = strchr(s, '\n')) != NULL) {
          i++;
          total_len++;
          s++;
          temp = *s;
          *s = '\0';
          safe_snprintf(buf + strlen(buf), MAX_STRING_LENGTH - strlen(buf), "%4d: ", (i - 1));
          strcat(buf, t);
          *s = temp;
          t = s;
        }
      if (s && t) {
        temp = *s;
        *s = '\0';
        strcat(buf, t);
        *s = temp;
      } else if (t)
        strcat(buf, t);
      /* this is kind of annoying .. seeing as the lines are #ed
       sprintf(buf + strlen(buf), "\r\n%d numbered line%slisted.\r\n", total_len,
       ((total_len != 1)?"s ":" "));
       */
      page_string(d, buf, TRUE);
      break;

    case PARSE_INSERT:
      half_chop(string, buf, buf2);
      if (*buf == '\0') {
        SEND_TO_Q("You must specify a line number before which to insert text.\r\n", d);
        return;
      }
      line_low = atoi(buf);
      strcat(buf2, "\r\n");

      i = 1;
      *buf = '\0';
      if ((s = *d->str) == NULL) {
        SEND_TO_Q("Buffer is empty, nowhere to insert.\r\n", d);
        return;
      }
      if (line_low > 0) {
        while (s && (i < line_low))
          if ((s = strchr(s, '\n')) != NULL) {
            i++;
            s++;
          }
        if ((i < line_low) || (s == NULL)) {
          SEND_TO_Q("Line number out of range; insert aborted.\r\n", d);
          return;
        }
        temp = *s;
        *s = '\0';
        if ((strlen(*d->str) + strlen(buf2) + strlen(s + 1) + 3) > d->max_str) {
          *s = temp;
          SEND_TO_Q("Insert text pushes buffer over maximum size, insert aborted.\r\n", d);
          return;
        }
        if (*d->str && (**d->str != '\0'))
          strcat(buf, *d->str);
        *s = temp;
        strcat(buf, buf2);
        if (s && (*s != '\0'))
          strcat(buf, s);
        RECREATE(*d->str, char, strlen(buf) + 3);
        strcpy(*d->str, buf);
        SEND_TO_Q("Line inserted.\r\n", d);
      } else {
        SEND_TO_Q("Line number must be higher than 0.\r\n", d);
        return;
      }
      break;

    case PARSE_EDIT:
      half_chop(string, buf, buf2);
      if (*buf == '\0') {
        SEND_TO_Q("You must specify a line number at which to change text.\r\n", d);
        return;
      }
      line_low = atoi(buf);
      strcat(buf2, "\r\n");

      i = 1;
      *buf = '\0';
      if ((s = *d->str) == NULL) {
        SEND_TO_Q("Buffer is empty, nothing to change.\r\n", d);
        return;
      }
      if (line_low > 0) {
        /* loop through the text counting /n chars till we get to the line */
        while (s && (i < line_low))
          if ((s = strchr(s, '\n')) != NULL) {
            i++;
            s++;
          }
        /* make sure that there was a THAT line in the text */
        if ((i < line_low) || (s == NULL)) {
          SEND_TO_Q("Line number out of range; change aborted.\r\n", d);
          return;
        }
        /* if s is the same as *d->str that means im at the beginning of the
         * message text and i dont need to put that into the changed buffer */
        if (s != *d->str) {
          /* first things first .. we get this part into buf. */
          temp = *s;
          *s = '\0';
          /* put the first 'good' half of the text into storage */
          strcat(buf, *d->str);
          *s = temp;
        }
        /* put the new 'good' line into place. */
        strcat(buf, buf2);
        if ((s = strchr(s, '\n')) != NULL) {
          /* this means that we are at the END of the line we want outta there. */
          /* BUT we want s to point to the beginning of the line AFTER
           * the line we want edited */
          s++;
          /* now put the last 'good' half of buffer into storage */
          strcat(buf, s);
        }
        /* check for buffer overflow */
        if (strlen(buf) > d->max_str) {
          SEND_TO_Q("Change causes new length to exceed buffer maximum size, aborted.\r\n", d);
          return;
        }
        /* change the size of the REAL buffer to fit the new text */
        RECREATE(*d->str, char, strlen(buf) + 3);
        strcpy(*d->str, buf);
        SEND_TO_Q("Line changed.\r\n", d);
      } else {
        SEND_TO_Q("Line number must be higher than 0.\r\n", d);
        return;
      }
      break;
    default:
      SEND_TO_Q("Invalid option.\r\n", d);
      mudlog("SYSERR: invalid command passed to parse_action", 'E', LVL_IMMORT, TRUE);
      return;
  }
}

/* Add user input to the 'current' string (as defined by d->str) */
void string_add(struct descriptor_data *d, char *str)
{
  FILE *fl;
  int terminator = 0, action = 0;
  register int i = 2, j = 0;
  char actions[MAX_INPUT_LENGTH];
  extern char *MENU;
  struct char_data *mailto = NULL;
  extern struct char_data *character_list;

  /* determine if this is the terminal string, and truncate if so */
  /* changed to accept '/<letter>' style editing commands - instead */
  /* of solitary '@' to end - (modification of improved_edit patch) */
  /*   M. Scott 10/15/96 */

  if (!*str)
    return;

  delete_doubledollar(str);

  /* removed old handling of '@' char */
  /* if ((terminator = (*str == '@'))) *str = '\0'; */

  if ((action = (*str == '/'))) {
    while (str[i] != '\0') {
      actions[j] = str[i];
      i++;
      j++;
    }
    actions[j] = '\0';
    *str = '\0';
    switch (str[1]) {
      case 'a':
        terminator = 2; /* working on an abort message */
        break;
      case 'c':
        if (*(d->str)) {
          FREE(*d->str);
          *(d->str) = NULL;
          SEND_TO_Q("Current buffer cleared.\r\n", d);
        } else
          SEND_TO_Q("Current buffer empty.\r\n", d);
        break;
      case 'd':
        parse_action(PARSE_DELETE, actions, d);
        break;
      case 'e':
        parse_action(PARSE_EDIT, actions, d);
        break;
      case 'f':
        if (*(d->str))
          parse_action(PARSE_FORMAT, actions, d);
        else
          SEND_TO_Q("Current buffer empty.\r\n", d);
        break;
      case 'i':
        if (*(d->str))
          parse_action(PARSE_INSERT, actions, d);
        else
          SEND_TO_Q("Current buffer empty.\r\n", d);
        break;
      case 'h':
        parse_action(PARSE_HELP, actions, d);
        break;
      case 'l':
        if (*d->str)
          parse_action(PARSE_LIST_NORM, actions, d);
        else
          SEND_TO_Q("Current buffer empty.\r\n", d);
        break;
      case 'n':
        if (*d->str)
          parse_action(PARSE_LIST_NUM, actions, d);
        else
          SEND_TO_Q("Current buffer empty.\r\n", d);
        break;
      case 'r':
        parse_action(PARSE_REPLACE, actions, d);
        break;
      case 's':
        terminator = 1;
        *str = '\0';
        break;
      default:
        SEND_TO_Q("Invalid option.\r\n", d);
        break;
    }
  }

  if (!(*d->str)) {
    if (strlen(str) > d->max_str) {
      send_to_char("String too long - Truncated.\r\n", d->character);
      *(str + d->max_str) = '\0';
      /* changed this to NOT abort out.. just give warning. */
      /* terminator = 1; */
    }
    CREATE(*d->str, char, strlen(str) + 3);
    strcpy(*d->str, str);
  } else {
    if (strlen(str) + strlen(*d->str) > d->max_str) {
      send_to_char("String too long, limit reached on message.  Last line ignored.\r\n", d->character);
      /* terminator = 1; */
    } else {
      if (!(*d->str = (char *) realloc(*d->str, strlen(*d->str) + strlen(str) + 3))) {
        perror("string_add");
        fflush(NULL);
        exit(1);
      }
      strcat(*d->str, str);
    }
  }

  if (terminator) {
    /*. OLC Edits .*/
    extern void oedit_disp_menu(struct descriptor_data *d);
    extern void oedit_disp_extradesc_menu(struct descriptor_data *d);
    extern void redit_disp_menu(struct descriptor_data *d);
    extern void redit_disp_extradesc_menu(struct descriptor_data *d);
    extern void redit_disp_exit_menu(struct descriptor_data *d);
    extern void medit_disp_menu(struct descriptor_data *d);
    extern void medit_change_mprog(struct descriptor_data *d);

    /* here we check for the abort option and reset the pointers */
    if ((terminator == 2) && ((STATE(d) == CON_REDIT) || (STATE(d) == CON_MEDIT) || (STATE(d) == CON_OEDIT) || (STATE(d) == CON_EXDESC) || (STATE(d) == CON_TEXTED))) {
      FREE(*d->str);
      if (d->backstr) {
        *d->str = d->backstr;
      } else
        *d->str = NULL;
      d->backstr = NULL;
      d->str = NULL;
    }
    /* this fix causes the editor to NULL out empty messages -- M. Scott */
    else if ((d->str) && (*d->str) && (**d->str == '\0')) {
      FREE(*d->str);
      *d->str = NULL;
    }

    if (STATE(d) == CON_MEDIT) {
      switch (OLC_MODE(d)) {
        case MEDIT_D_DESC:
          medit_disp_menu(d);
          break;
        case MEDIT_MPROG_COMLIST:
          medit_change_mprog(d);
          break;
      }
    }

    if (STATE(d) == CON_OEDIT) {
      switch (OLC_MODE(d)) {
        case OEDIT_ACTDESC:
          oedit_disp_menu(d);
          break;
        case OEDIT_EXTRADESC_DESCRIPTION:
          oedit_disp_extradesc_menu(d);
      }
    } else if (STATE(d) == CON_REDIT) {
      switch (OLC_MODE(d)) {
        case REDIT_DESC:
          redit_disp_menu(d);
          break;
        case REDIT_EXIT_DESCRIPTION:
          redit_disp_exit_menu(d);
          break;
        case REDIT_EXTRADESC_DESCRIPTION:
          redit_disp_extradesc_menu(d);
          break;
      }
    } else if (!d->connected && (PLR_FLAGGED(d->character, PLR_MAILING))) {
      if (terminator == 1 && *d->str) {
        store_mail(d->mail_to, GET_NAME(d->character), *d->str);
        SEND_TO_Q("Message sent!\r\n", d);
        for (mailto = character_list; mailto; mailto = mailto->next)
          if (!strcasecmp(GET_NAME(mailto), d->mail_to)) {
            send_to_char("{WYou have {Rnew mail {Wwaiting.{x\r\n", mailto);
            break;
          }
      } else
        SEND_TO_Q("Mail aborted.\r\n", d);
      d->mail_to = 0;
      FREE(*d->str);
      FREE(d->str);
      safe_snprintf(buf, MAX_STRING_LENGTH, "%s finished writing mail message.", GET_NAME(d->character));
      mudlog(buf, 'G', COM_ADMIN, TRUE);
    } else if ((long) d->mail_to >= BOARD_MAGIC) {
      Board_save_board((long) d->mail_to - BOARD_MAGIC);
      SEND_TO_Q("Post not aborted, use REMOVE <post #>.\r\n", d);
      d->mail_to = 0;
      safe_snprintf(buf, MAX_STRING_LENGTH, "%s finished writing message on board.", GET_NAME(d->character));
      mudlog(buf, 'G', COM_BUILDER, TRUE);
    } else if (STATE(d) == CON_EXDESC) {
      if (terminator != 1)
        SEND_TO_Q("Description aborted.\r\n", d);
      if (d->color) {
        SEND_TO_Q_COLOR(MENU, d);
        SEND_TO_Q_COLOR("{WMake your choice{G:{x ", d);
      } else {
        SEND_TO_Q(MENU, d);
        SEND_TO_Q("Make your choice: ", d);
      }
      d->connected = CON_MENU;
    } else if (STATE(d) == CON_TEXTED) {
      if (terminator == 1) {
        if (!(fl = fopen((char *) d->storage, "w"))) {
          safe_snprintf(buf, MAX_STRING_LENGTH, "SYSERR: Can't write file '%s'.", d->storage);
          mudlog(buf, 'G', COM_BUILDER, TRUE);
        } else {
          if (*d->str) {
            strip_string(*d->str);
            fputs(*d->str, fl);
          }
          fclose(fl);
          safe_snprintf(buf, MAX_STRING_LENGTH, "OLC: %s saves '%s'.", GET_NAME(d->character), d->storage);
          mudlog(buf, 'G', COM_BUILDER, TRUE);
          SEND_TO_Q("Saved.\r\n", d);
          act("$n stops editing some scrolls.", TRUE, d->character, 0, 0, TO_ROOM);
          if (d->character && !IS_NPC(d->character))
            REMOVE_BIT(PLR_FLAGS(d->character), PLR_WRITING | PLR_MAILING);
          if (d->backstr)
            FREE(d->backstr);
          if (*d->str)
            FREE(*d->str);
          if (d->str)
            FREE(d->str);
          d->backstr = NULL;
          d->str = NULL;
          STATE(d) = CON_PLAYING;
        }
      } else {
        SEND_TO_Q("Edit aborted.\r\n", d);
        safe_snprintf(buf, MAX_STRING_LENGTH, "OLC: %s stops editing '%s'.", GET_NAME(d->character), d->storage);
        mudlog(buf, 'G', COM_BUILDER, TRUE);
        act("$n stops editing some scrolls.", TRUE, d->character, 0, 0, TO_ROOM);
        FREE(d->storage);
        d->storage = NULL;
        STATE(d) = CON_PLAYING;
      }
    } else if (!d->connected && d->character && !IS_NPC(d->character)) {
      if (terminator == 1) {
        if (strlen(*d->str) == 0) {
          FREE(*d->str);
          *d->str = NULL;
        }
      } else {
        FREE(*d->str);
        if (d->backstr) {
          *d->str = d->backstr;
        } else
          *d->str = NULL;
        d->backstr = NULL;
        SEND_TO_Q("Message aborted.\r\n", d);
      }
    }

    if (d->character && !IS_NPC(d->character))
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_WRITING | PLR_MAILING);
    if (d->backstr)
      FREE(d->backstr);
    d->backstr = NULL;
    d->str = NULL;
  } else if (!action)
    strcat(*d->str, "\r\n");
}

/* **********************************************************************
 *  Modification of character skills                                     *
 ********************************************************************** */

ACMD(do_skillset)
{
  extern struct spell_info_type *spells;
  struct char_data *vict;
  struct char_data *victim = 0;
  char name[100], buf2[100], buf[100], help[MAX_STRING_LENGTH];
  int skill, value, i, qend;

  argument = one_argument(argument, name);

  if (!*name) { /* no arguments. print an informative text */
    send_to_char("Syntax: skillset <name> '<skill>' <value>\r\n", ch);
    safe_snprintf(help, sizeof(help), "Skill being one of the following:\n\r");
    for (i = 1; spells[i].command[0] != '\n'; i++) {
      safe_snprintf(help + strlen(help), sizeof(help) - strlen(help), "%18s", spells[i].command);
      if (i % 4 == 3) {
        safe_snprintf(help + strlen(help), sizeof(help) - strlen(help), "\r\n");
        send_to_char(help, ch);
        *help = '\0';
      }
    }
    if (*help)
      send_to_char(help, ch);
    send_to_char("\n\r", ch);
    return;
  }

  if (!(vict = get_char_vis(ch, name))) {
    CREATE(victim, struct char_data, 1);
    clear_char(victim);
    if (load_char_text(name, victim) > 0) {
      if (GET_LEVEL(victim) > GET_LEVEL(ch)) {
        send_to_char("Can't touch or see those skills.\r\n", ch);
        free_char(victim);
        return;
      }
      vict = victim;
    } else {
      send_to_char(NOPERSON, ch);
      FREE(victim);
      return;
    }
  }

  skip_spaces(&argument);

  /* If there is no chars in argument */
  if (!*argument) {
    show_skills(ch, vict);
    if (victim) {
      vict = NULL;
      free_char(victim);
    }
    return;
  }
  if (*argument != '\'') {
    send_to_char("Skill must be enclosed in: ''\n\r", ch);
    if (victim) {
      vict = NULL;
      free_char(victim);
    }
    return;
  }
  /* Locate the last quote && lowercase the magic words (if any) */

  for (qend = 1; *(argument + qend) && (*(argument + qend) != '\''); qend++)
    *(argument + qend) = LOWER(*(argument + qend));

  if (*(argument + qend) != '\'') {
    send_to_char("Skill must be enclosed in: ''\n\r", ch);
    if (victim) {
      vict = NULL;
      free_char(victim);
    }
    return;
  }
  strcpy(help, (argument + 1));
  help[qend - 1] = '\0';
  if ((skill = find_skill_num(help)) <= 0) {
    send_to_char("Unrecognized skill.\n\r", ch);
    if (victim) {
      vict = NULL;
      free_char(victim);
    }
    return;
  }
  argument += qend + 1; /* skip to next parameter */
  argument = one_argument(argument, buf);

  if (!*buf) {
    send_to_char("Learned value expected.\n\r", ch);
    if (victim) {
      vict = NULL;
      free_char(victim);
    }
    return;
  }
  value = atoi(buf);
  if (value < 0) {
    send_to_char("Minimum value for learned is 0.\n\r", ch);
    if (victim) {
      vict = NULL;
      free_char(victim);
    }
    return;
  }
  if (value > 100) {
    send_to_char("Max value for learned is 100.\n\r", ch);
    if (victim) {
      vict = NULL;
      free_char(victim);
    }
    return;
  }
  if (IS_NPC(vict)) {
    send_to_char("You can't set NPC skills.\n\r", ch);
    if (victim) {
      vict = NULL;
      free_char(victim);
    }
    return;
  }
  safe_snprintf(buf2, MAX_STRING_LENGTH, "%s changed %s's %s to %d.", GET_NAME(ch), GET_NAME(vict), spells[skill].command, value);
  mudlog(buf2, 'G', COM_BUILDER, TRUE);

  SET_SKILL(vict, spells[skill].spellindex, value);

  safe_snprintf(buf2, MAX_STRING_LENGTH, "You change %s's %s to %d.\n\r", GET_NAME(vict), spells[skill].command, value);
  send_to_char(buf2, ch);

  save_char_text(vict, NOWHERE);
  if (victim) {
    vict = NULL;
    free_char(victim);
  }

}

/*********************************************************************
 * New Pagination Code
 * Michael Buselli submitted the following code for an enhanced pager
 * for CircleMUD.  All functions below are his.  --JE 8 Mar 96
 *
 *********************************************************************/

#define PAGE_LENGTH     22
#define PAGE_WIDTH      80

/* Traverse down the string until the begining of the next page has been
 * reached.  Return NULL if this is the last page of the string.
 */
char *next_page(char *str, struct char_data *ch)
{
  int col = 1, line = 1, spec_code = FALSE;

  for (; *str; str++) {
    /* If we're at the start of the next page, return this fact. */
    if (GET_SCREEN_HEIGHT(ch) == 0) {
      GET_SCREEN_WIDTH(ch) = PAGE_WIDTH;
      GET_SCREEN_HEIGHT(ch) = PAGE_LENGTH;
      if (line > PAGE_LENGTH)
        return str;
    } else if (ch->desc && ch->desc->original && line > GET_SCREEN_HEIGHT(ch->desc->original))
      return str;
    else if (line > GET_SCREEN_HEIGHT(ch))
      return str;

    switch (*str) {
      case '\x1B':
        spec_code = TRUE;
        break;
      case 'm':
        spec_code = FALSE;
        break;
      case '\r':
        col = 1;
        break;
      case '\n':
        line++;
        break;
      default:
        if ((!spec_code) && (++col > GET_SCREEN_WIDTH(ch))) {
          col = 1;
          line++;
        }
    }
  }
  return NULL;
}

/* Function that returns the number of pages in the string. */
int count_pages(char *str, struct char_data *ch)
{
  int pages;

  for (pages = 1; (str = next_page(str, ch)); pages++)
    ;
  return pages;
}

/* This function assigns all the pointers for showstr_vector for the
 * page_string function, after showstr_vector has been allocated and
 * showstr_count set.
 */
void paginate_string(char *str, struct descriptor_data *d)
{
  int i;

  if (d->showstr_count)
    *(d->showstr_vector) = str;

  for (i = 1; i < d->showstr_count && str; i++)
    str = d->showstr_vector[i] = next_page(str, d->character);

  d->showstr_page = 0;
}

/* The call that gets the paging ball rolling... */
void page_string(struct descriptor_data *d, char *str, int keep_internal)
{
  const char *point;
  char *point2;
  char buf[MAX_STRING_LENGTH * 8];
  /* use with wordwrap
   char *tmpstr;
   */

  buf[0] = '\0';
  point2 = buf;

  if (!d)
    return;

  if (!str || !*str) {
    send_to_char("", d->character);
    return;
  }

  /* removed until i can get the wordwrap to work. Dez.
   tmpstr = reformat_string(str, d->character);
   str = tmpstr;
   */
  if (PRF_FLAGGED(d->character, PRF_COLOR_2) || d->color) {
    for (point = str; *point; point++) {
      if (*point == '{') {
        point++;
        strcpy(point2, colorf(*point, d->character));
        for (; *point2; point2++)
          ;
        continue;
      }
      *point2 = *point;
      *++point2 = '\0';
    }
    *point2 = '\0';
    str = buf;
  } else {
    for (point = str; *point; point++) {
      if (*point == '{') {
        point++;
        continue;
      }
      *point2 = *point;
      *++point2 = '\0';
    }
    *point2 = '\0';
    str = buf;
  }

  /*convert_linefeed(str);*/

  /* use with wordwrap
   CREATE(d->showstr_vector, char *, d->showstr_count = count_pages(tmpstr, d->character));
   */
  CREATE(d->showstr_vector, char *, d->showstr_count = count_pages(str, d->character));

  if (keep_internal) {
    /* use with wordwrap
     d->showstr_head = strdup(tmpstr);
     */
    d->showstr_head = strdup(str);
    paginate_string(d->showstr_head, d);
  } else
    /* use with wordwrap
     paginate_string(tmpstr, d);
     */
    paginate_string(str, d);

  show_string(d, "");
  /* use with wordwrap
   free(tmpstr);
   */
}

/* The call that gets the paging ball rolling... */
void page_string_no_color(struct descriptor_data *d, char *str, int keep_internal)
{
  const char *point;
  char *point2;
  char buf[MAX_STRING_LENGTH * 8];
  /* use with wordwrap
   char *tmpstr;
   */

  buf[0] = '\0';
  point2 = buf;

  if (!d)
    return;

  if (!str || !*str) {
    send_to_char("", d->character);
    return;
  }

  /* removed until i can get the wordwrap to work. Dez.
   tmpstr = reformat_string(str, d->character);
   str = tmpstr;
   */
  for (point = str; *point; point++) {
    if (*point == '{') {
      point++;
      continue;
    }
    *point2 = *point;
    *++point2 = '\0';
  }
  *point2 = '\0';
  str = buf;
  /*convert_linefeed(str);*/
  /* use with wordwrap
   CREATE(d->showstr_vector, char *, d->showstr_count = count_pages(tmpstr, d->character));
   */
  CREATE(d->showstr_vector, char *, d->showstr_count = count_pages(str, d->character));

  if (keep_internal) {
    /* use with wordwrap
     d->showstr_head = strdup(tmpstr);
     */
    d->showstr_head = strdup(str);
    paginate_string(d->showstr_head, d);
  } else
    /* use with wordwrap
     paginate_string(tmpstr, d);
     */
    paginate_string(str, d);

  show_string(d, "");
  /* use with wordwrap
   free(tmpstr);
   */
}

/* The call that displays the next page. */
void show_string(struct descriptor_data *d, char *input)
{
  char buffer[MAX_STRING_LENGTH];
  int diff;

  one_argument(input, buf);

  /* Q is for quit. :) */
  if (STATE(d) != CON_NAME_CNFRM && STATE(d) != CON_GET_NAME && STATE(d) != CON_POLICY && LOWER(*buf) == 'q') {
    FREE(d->showstr_vector);
    d->showstr_count = 0;
    if (d->showstr_head) {
      FREE(d->showstr_head);
      d->showstr_head = 0;
    }
    return;
  }
  /* R is for refresh, so back up one page internally so we can display
   * it again.
   */
  else if (LOWER(*buf) == 'r')
    d->showstr_page = MAX(0, d->showstr_page - 1);

  /* B is for back, so back up two pages internally so we can display the
   * correct page here.
   */
  else if (LOWER(*buf) == 'b')
    d->showstr_page = MAX(0, d->showstr_page - 2);

  /* Feature to 'goto' a page.  Just type the number of the page and you
   * are there!
   */
  else if (STATE(d) != CON_NAME_CNFRM && STATE(d) != CON_GET_NAME && STATE(d) != CON_POLICY && isdigit(*buf))
    d->showstr_page = BOUNDED(0, atoi(buf) - 1, d->showstr_count - 1);

  else if (*buf) {
    send_to_char("Valid commands while paging are RETURN, Q, R, B, or a numeric value.\r\n", d->character);
    return;
  }
  /* If we're displaying the last page, just send it to the character, and
   * then free up the space we used.
   */
  if (d->showstr_page + 1 >= d->showstr_count) {
    if (STATE(d) == CON_GET_NAME || STATE(d) == CON_NAME_CNFRM) {
      if (d->color)
        SEND_TO_Q_COLOR(d->showstr_vector[d->showstr_page], d);
      else
        SEND_TO_Q(d->showstr_vector[d->showstr_page], d);
    } else {
      if (STATE(d) == CON_GET_NAME || STATE(d) == CON_NAME_CNFRM) {
        if (d->color)
          SEND_TO_Q_COLOR(d->showstr_vector[d->showstr_page], d);
        else
          SEND_TO_Q(d->showstr_vector[d->showstr_page], d);
      } else
        send_to_char(d->showstr_vector[d->showstr_page], d->character);
    }
    FREE(d->showstr_vector);
    d->showstr_count = 0;
    if (d->showstr_head) {
      FREE(d->showstr_head);
      d->showstr_head = NULL;
    }
  }
  /* Or if we have more to show.... */
  else {
    strncpy(buffer, d->showstr_vector[d->showstr_page], diff = ((long) d->showstr_vector[d->showstr_page + 1]) - ((long) d->showstr_vector[d->showstr_page]));
    buffer[diff] = '\0';
    if (STATE(d) == CON_GET_NAME || STATE(d) == CON_NAME_CNFRM) {
      if (d->color)
        SEND_TO_Q_COLOR(buffer, d);
      else
        SEND_TO_Q(buffer, d);
    } else
      send_to_char(buffer, d->character);
    d->showstr_page++;
  }
}
