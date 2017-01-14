/* actd support */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "screen.h"

struct actd_msg *actd_list = NULL;

struct actd_msg *get_actd(int vnum)
{
  struct actd_msg *actd;

  actd = actd_list;

  while (actd) {
    if (vnum == actd->actd_nr) {
      return actd;
    }
    actd = actd->next;
  }
  return NULL;
}

void load_actd(void)
{
  FILE *fl;
  int type;
  struct actd_msg *actd;
  char chk[128];

  actd_list = NULL;

  if (!(fl = fopen(ACTD_FILE, "r"))) {
    sprintf(buf2, "Error reading ACTD file %s", ACTD_FILE);
    perror(buf2);
    fflush(NULL);
    exit(1);
  }

  fgets(chk, 128, fl);
  while (!feof(fl) && (*chk == '\n' || *chk == '*')) {
    fgets(chk, 128, fl);
  }

  while (*chk == 'A') {
    fgets(chk, 128, fl);
    sscanf(chk, " %d\n", &type);
    CREATE(actd, struct actd_msg, 1);
    actd->actd_nr = type;
    actd->next = actd_list;
    actd_list = actd;

    actd->char_no_arg = fread_string(fl, buf2);
    actd->others_no_arg = fread_string(fl, buf2);
    actd->char_found = fread_string(fl, buf2);
    actd->others_found = fread_string(fl, buf2);
    actd->vict_found = fread_string(fl, buf2);
    actd->not_found = fread_string(fl, buf2);
    actd->char_auto = fread_string(fl, buf2);
    actd->others_auto = fread_string(fl, buf2);
    actd->char_object = fread_string(fl, buf2);
    actd->others_object = fread_string(fl, buf2);

    fgets(chk, 128, fl);
    while (!feof(fl) && (*chk == '\n' || *chk == '*')) {
      fgets(chk, 128, fl);
    }
  }

  fclose(fl);
}

ACMD(do_actd)
{
  int j;
  struct actd_msg *actd;

  actd = actd_list;

  if (!*argument) {
    send_to_char("Currently defined ACTDs:\r\n", ch);
    while (actd) {
      j = 0;
      while (actd && j < 20) {
        j++;
        sprintf(buf, "%5d ", actd->actd_nr);
        send_to_char(buf, ch);
        actd = actd->next;
      }
      send_to_char("\r\n", ch);
    }
  } else {
    j = atoi(argument);
    while (actd) {
      if (j == actd->actd_nr) {
        break;
      }
      actd = actd->next;
    }

    if (actd != NULL) {
      sprintf(buf, "Definition of ACTD [%5d]:\r\n", actd->actd_nr);
      send_to_char(buf, ch);
      send_to_char("[CHAR] No argument - \r\n", ch);
      send_to_char(actd->char_no_arg, ch);
      send_to_char("\r\n[OTHERS] No argument - \r\n", ch);
      send_to_char(actd->others_no_arg, ch);
      send_to_char("\r\n[CHAR] Target found - \r\n", ch);
      send_to_char(actd->char_found, ch);
      send_to_char("\r\n[OTHERS] Target found - \r\n", ch);
      send_to_char(actd->others_found, ch);
      send_to_char("\r\n[TARGET] Target found - \r\n", ch);
      send_to_char(actd->vict_found, ch);
      send_to_char("\r\n[CHAR] Target not found - \r\n", ch);
      send_to_char(actd->not_found, ch);
      send_to_char("\r\n[CHAR] Target is player - \r\n", ch);
      send_to_char(actd->char_auto, ch);
      send_to_char("\r\n[OTHERS] Target is player - \r\n", ch);
      send_to_char(actd->others_auto, ch);
      send_to_char("\r\n[CHAR] Target is object - \r\n", ch);
      send_to_char(actd->char_auto, ch);
      send_to_char("\r\n[OTHERS] Target is object - \r\n", ch);
      send_to_char(actd->others_auto, ch);
    }
    send_to_char("\r\n", ch);
  }
}

