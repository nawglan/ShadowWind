/* QIC system, written by Mattias Larsson 1995 ml@eniac.campus.luth.se */

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "screen.h"
#include "structs.h"
#include "utils.h"

/* External Structures */
extern struct descriptor_data *descriptor_list;
extern struct index_data *obj_index;
extern struct obj_data *object_list;
extern struct obj_data *obj_proto;
extern struct player_index_element *player_table;
extern int top_of_objt;
extern int top_of_p_table;

/* Global variables for qic system */

FILE *qic_fl = NULL; /* File identification for qic file */
int qic_items;       /* Number of items in database */

int qic_vnums[500];

void save_record(int j) { /* save one record */
  struct qic_data q;

  q.vnum = obj_index[j].virtual;
  q.limit = obj_index[j].qic->limit;
  q.items = obj_index[j].qic->items;
  fseek(qic_fl, obj_index[j].qic->vnum * sizeof(struct qic_data), SEEK_SET); /* search for the record number */
  if (fwrite(&q, sizeof(struct qic_data), 1, qic_fl) < 1) {
    perror("Error writing QIC record (full save)");
    return;
  }
}

void load_qic(void) {
  int size, i, nr, j;
  struct qic_data q;

  if (!(qic_fl = fopen(QIC_FILE, "r+b"))) {
    if (errno != ENOENT) {
      perror("1. fatal error opening qic database");
      fflush(NULL);
      exit(1);
    } else {
      stderr_log("No QIC database. Creating a new one.");
      touch(QIC_FILE);
      if (!(qic_fl = fopen(QIC_FILE, "r+b"))) {
        perror("2. fatal error opening qic database");
        fflush(NULL);
        exit(1);
      }
    }
  }

  /* add code here to load database :) */

  fseek(qic_fl, 0L, SEEK_END);
  size = ftell(qic_fl);
  rewind(qic_fl);
  if (size % sizeof(struct qic_data))
    fprintf(stderr, "WARNING:  QIC DATABASE IS PROBABLY CORRUPT!\n");
  qic_items = size / sizeof(struct qic_data);
  if (qic_items) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "   %d records in QIC database.", qic_items);
    stderr_log(buf);
  } else {
    qic_items = 0;
    return;
  }
  i = 0;
  for (; !feof(qic_fl);) {
    fread(&q, sizeof(struct qic_data), 1, qic_fl);
    if ((nr = real_object(q.vnum)) < 0) {
      i++;
      safe_snprintf(buf, MAX_STRING_LENGTH, "Invalid vnum [%5d] in QIC database!", q.vnum);
      stderr_log(buf);
    } else {
      CREATE(obj_index[nr].qic, struct qic_data, 1);
      qic_vnums[i] = q.vnum;
      obj_index[nr].qic->vnum = i;
      i++;
      obj_index[nr].qic->limit = q.limit;
      obj_index[nr].qic->items = q.items;
      for (j = 0; j < QIC_OWNERS; j++) /* make sure it's reset to 0... hmm */
        obj_index[nr].qic->owners[j][0] = '\0';
    }
  }

} /* end of load thing */

void save_qic(void) { /* save the whole qic database */
  struct qic_data q;
  int j;

  fclose(qic_fl);

  if (!(qic_fl = fopen(QIC_FILE, "w+b"))) {
    if (errno != ENOENT) {
      perror("fatal error opening qicfile");
      fflush(NULL);
      exit(1);
    }
  }

  for (j = 0; j <= top_of_objt; j++) {
    if (obj_index[j].qic != NULL) {
      q.vnum = obj_index[j].virtual;
      q.limit = obj_index[j].qic->limit;
      q.items = obj_index[j].qic->items;
      if (fwrite(&q, sizeof(struct qic_data), 1, qic_fl) < 1) {
        perror("Error writing QIC record (full save)");
        return;
      }
    }
  }
  fclose(qic_fl); /* close QIC database file */
} /* end of save rec database full blaha.. i'm so tired */

/* Check if it's okay to load a QIC. If yes, it increase the num loaded
 QIC's and then returns TRUE to the caller. Otherwise it returns FALSE,
 and doesnt touch any counter at all. */

int load_qic_check(int rnum) {
  if (obj_index[rnum].qic == NULL)
    return 1; /* okay to load, go ahead, no QIC record */
  if (obj_index[rnum].qic->items >= obj_index[rnum].qic->limit)
    return 0;                   /* too many items in the game already */
  obj_index[rnum].qic->items++; /* okay.. increase items counter */
  return 1;                     /* and return true */
}

void purge_qic(int rnum) {
  if (obj_index[rnum].qic == NULL) {
    return;
  } else {
    if (obj_index[rnum].qic->items > 0)
      obj_index[rnum].qic->items--;
  }
}

void qic_load(int rnum) {
  if (obj_index[rnum].qic == NULL) {
    return;
  } else {
    obj_index[rnum].qic->items++;
  }
}

void add_owner(int nr, char *name) {
  int i;

  if (name == NULL)
    return;

  for (i = 0; i < QIC_OWNERS; i++)
    if (!strcmp(obj_index[nr].qic->owners[i], name))
      return;

  for (i = 0; i < QIC_OWNERS; i++)
    if (obj_index[nr].qic->owners[i][0] == '\0') {
      safe_snprintf(obj_index[nr].qic->owners[i], sizeof(obj_index[nr].qic->owners[i]), "%s", name);
      return;
    }
}

/* Scan a single rent database file for QIC's */
void qic_scan_file(char *name, long id) {
  char fname[MAX_STRING_LENGTH];
  int j, nr, vnum;
  FILE *fl;
  char line[1024];
  char tag[256];
  char value[256];

  if (!get_filename(name, fname, CRASH_FILE))
    return;

  if (!(fl = fopen(fname, "r"))) {
    if (errno != ENOENT) { /* if it fails, NOT because of no file */
      safe_snprintf(buf1, MAX_STRING_LENGTH, "SYSERR: OPENING OBJECT FILE %s (4)", fname);
      perror(buf1);
    }
    return;
  }

  if (!feof(fl))
    fgets(line, 1024, fl);

  while (!feof(fl)) {
    sscanf(line, "%s %s", tag, value);
    if (strcmp(tag, "-obj_number-") == 0) {
      vnum = atoi(value);
      if ((nr = real_object(vnum)) >= 0)
        obj_index[nr].rent++;
      for (j = 0; j < qic_items; j++) {
        if (vnum == qic_vnums[j]) { /* okay, found a qic */
          if ((nr = real_object(vnum)) >= 0) {
            obj_index[nr].qic->items++; /* maybe a check to add for NULL qic? */
            add_owner(nr, name);        /* add owner to owner list */
          } /* end real_obj */
        }
      } /* for */
    }
    fgets(line, 1024, fl);
  }

  fclose(fl);
}

/* Scans through the whole rent database and sets the current QIC values */
void qic_scan_rent(void) {
  int i;
  DIR *rp;
  struct dirent *dirp;
  char *cp;
  char buf[80];

  /* okay, first of all, clean out the item counters loaded by load_qic() */

  for (i = 0; i < top_of_objt; i++)
    if (obj_index[i].qic != NULL)
      obj_index[i].qic->items = 0;

  rp = opendir("plrobjs/A-E");
  while ((dirp = readdir(rp)) != NULL) {
    safe_snprintf(buf, sizeof(buf), "%s", dirp->d_name);
    if (strstr(buf, "objs")) {
      cp = strrchr(buf, '.');
      if (cp) {
        *cp = '\0';
        qic_scan_file(buf, 0);
      }
    }
  }
  closedir(rp);
  rp = opendir("plrobjs/F-J");
  while ((dirp = readdir(rp)) != NULL) {
    safe_snprintf(buf, sizeof(buf), "%s", dirp->d_name);
    if (strstr(buf, "objs")) {
      cp = strrchr(buf, '.');
      if (cp) {
        *cp = '\0';
        qic_scan_file(buf, 0);
      }
    }
  }
  closedir(rp);
  rp = opendir("plrobjs/K-O");
  while ((dirp = readdir(rp)) != NULL) {
    safe_snprintf(buf, sizeof(buf), "%s", dirp->d_name);
    if (strstr(buf, "objs")) {
      cp = strrchr(buf, '.');
      if (cp) {
        *cp = '\0';
        qic_scan_file(buf, 0);
      }
    }
  }
  closedir(rp);
  rp = opendir("plrobjs/P-T");
  while ((dirp = readdir(rp)) != NULL) {
    safe_snprintf(buf, sizeof(buf), "%s", dirp->d_name);
    if (strstr(buf, "objs")) {
      cp = strrchr(buf, '.');
      if (cp) {
        *cp = '\0';
        qic_scan_file(buf, 0);
      }
    }
  }
  closedir(rp);
  rp = opendir("plrobjs/U-Z");
  while ((dirp = readdir(rp)) != NULL) {
    safe_snprintf(buf, sizeof(buf), "%s", dirp->d_name);
  }
  closedir(rp);
  rp = opendir("plrobjs/ZZZ");
  while ((dirp = readdir(rp)) != NULL) {
    safe_snprintf(buf, sizeof(buf), "%s", dirp->d_name);
    if (strstr(buf, "objs")) {
      cp = strrchr(buf, '.');
      if (cp) {
        *cp = '\0';
        qic_scan_file(buf, 0);
      }
    }
  }
  closedir(rp);

  return;
}

ACMD(do_setqic) {
  struct qic_data *q;
  int i, j;

  if (!*argument) {
    send_to_char("Usage: setqic [vnum] [limit]\r\n", ch);
    return;
  }

  two_arguments(argument, buf, arg);

  if ((i = real_object(atoi(buf))) < 0) {
    send_to_char("There is no object with that vnum.\r\n", ch);
    return;
  }

  if ((j = atoi(arg)) == 0) { /* remove QIC */
    if (obj_index[i].qic == NULL) {
      send_to_char("No QIC record to remove.\r\n", ch);
      return;
    }
    FREE(obj_index[i].qic);
    /* set QIC record to NULL */
    obj_index[i].qic = NULL;
    save_qic(); /* rewrite the whole QIC database (yuck!) */
    load_qic(); /* and load it again .. pfffft */
    send_to_char("QIC record removed.\r\n", ch);
    return;
  }

  if (obj_index[i].qic == NULL) {
    CREATE(q, struct qic_data, 1);
    obj_index[i].qic = q;
    q->vnum = qic_items; /* rec in database file */
    q->items = 0;        /* since it's a new QIC, no items loaded */
    q->limit = j;        /* the QIC limit */
    qic_items++;         /* increase max pointer */
  } else {
    q = obj_index[i].qic; /* we already have a QIC record */
    q->limit = j;         /* set the new limit */
  }
  save_record(i); /* save the QIC record */
  send_to_char(OK, ch);
}

ACMD(do_qicsave) {

  send_to_char("Forcing save and reload of QIC item database.\r\n", ch);
  save_qic();
  load_qic();
}

ACMD(do_qicinfo) {
  int i;
  size_t sblen;

  sblen = safe_snprintf(string_buf, MAX_STRING_LENGTH * 2, "Currently defined QICs:\r\n");

  for (i = 0; i < top_of_objt; i++) {
    if (obj_index[i].qic != NULL) {
      sblen += safe_snprintf(string_buf + sblen, MAX_STRING_LENGTH * 2 - sblen,
                             "%s[%s%5d%s]%s %-50s %sIn:%s %2d%s, Lim: %s%2d%s\r\n", CBBLU(ch, C_CMP), CBWHT(ch, C_CMP),
                             obj_index[i].virtual, CBBLU(ch, C_CMP), CCCYN(ch, C_CMP), obj_proto[i].short_description,
                             CBBLU(ch, C_CMP), CBWHT(ch, C_CMP), obj_index[i].qic->items, CBBLU(ch, C_CMP),
                             CBWHT(ch, C_CMP), obj_index[i].qic->limit, CCNRM(ch, C_NRM));
    }
    if (sblen > MAX_STRING_LENGTH * 2 - 256)
      break;
  }

  page_string(ch->desc, string_buf, 0);
}

/* List owners of a QIC item */ ACMD(do_owners) {
  int i, j;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Usage: owners <vnum>\r\n", ch);
    return;
  }

  if ((i = real_object(atoi(arg))) < 0) {
    send_to_char("There is no object with that vnum.\r\n", ch);
    return;
  }

  if (obj_index[i].qic == NULL) {
    send_to_char("That item is not QIC.\r\n", ch);
    return;
  }

  safe_snprintf(buf, MAX_STRING_LENGTH, "Registered owners at boot (item #%d - %s):\r\n", obj_index[i].virtual,
                obj_proto[i].short_description);
  send_to_char(buf, ch);

  for (j = 0; j < QIC_OWNERS; j += 2) {
    if (obj_index[i].qic->owners[j][0] == '\0')
      break;
    safe_snprintf(buf, MAX_STRING_LENGTH, "%20.20s  %20.20s\r\n", obj_index[i].qic->owners[j],
                  obj_index[i].qic->owners[j + 1] ? obj_index[i].qic->owners[j + 1] : "");
    send_to_char(buf, ch);
  }
  send_to_char("\r\n", ch);
}

ACMD(do_qload) {
  struct obj_data *obj;
  int number, r_num;

  one_argument(argument, buf);

  if (!*buf || !isdigit(*buf)) {
    send_to_char("Usage: qload <virt num>\r\n", ch);
    return;
  }
  if ((number = atoi(buf)) < 0) {
    send_to_char("A NEGATIVE number??\r\n", ch);
    return;
  }

  if ((r_num = real_object(number)) < 0) {
    send_to_char("There is no object with that number.\r\n", ch);
    return;
  }
  obj = read_object_q(r_num, REAL);
  if (obj == NULL) {
    send_to_char("Error loading QIC - report asap!\r\n", ch);
    return;
  }
  obj_to_room(obj, ch->in_room);
  qic_load(r_num);
  act("$n makes a strange powerful gesture.", TRUE, ch, 0, 0, TO_ROOM);
  act("$n has created $p!", FALSE, ch, obj, 0, TO_ROOM);
  act("You create $p.", FALSE, ch, obj, 0, TO_CHAR);
  safe_snprintf(buf, MAX_STRING_LENGTH, "%s QIC created %s", GET_NAME(ch), obj->short_description);
  mudlog(buf, 'L', COM_ADMIN, FALSE);
  plog(buf, ch, LVL_IMMORT);
}
