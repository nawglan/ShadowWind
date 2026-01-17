/* ************************************************************************
 *   File: house.c                                       Part of CircleMUD *
 *  Usage: Handling of player houses                                       *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "comm.h"
#include "db.h"
#include "handler.h"
#include "house.h"
#include "interpreter.h"
#include "structs.h"
#include "utils.h"

extern char *dirs[];
extern struct room_data *world;
extern int top_of_world;
extern const int rev_dir[];
extern struct index_data *obj_index;
extern int load_qic_check(int rnum);

struct obj_data *Obj_from_store(struct obj_file_elem object);
int Obj_to_store(struct obj_data *obj, int pos, FILE *fl);

struct house_control_rec house_control[MAX_HOUSES];
int num_of_houses = 0;

/* Modified by Rasmus Bertelsen */
int obj_count;

/* First, the basics: finding the filename; loading/saving objects */

/* Return a filename given a house vnum */
int House_get_filename(int vnum, char *filename) {
  if (vnum < 0)
    return 0;

  safe_snprintf(filename, MAX_STRING_LENGTH, "house/%d.house", vnum);
  return 1;
}

/* Modified by Rasmus Bertelsen */
/* Load all objects for a house */
int House_load(int vnum) {
  FILE *fl;
  char fname[MAX_STRING_LENGTH];
  struct obj_file_elem object;
  struct obj_data *tmp, *tmp2, *next;
  int rnum, pos, i;

  if ((rnum = real_room(vnum)) == -1)
    return 0;
  if (!House_get_filename(vnum, fname))
    return 0;
  if (!(fl = fopen(fname, "r+b"))) {
    /* no file found */
    return 0;
  }
  while (!feof(fl)) {
    fread(&object, sizeof(struct obj_file_elem), 1, fl);
    if (ferror(fl)) {
      perror("Reading house file: House_load.");
      fclose(fl);
      return 0;
    }
    if (!feof(fl))
      obj_to_room(Obj_from_store(object), rnum);
  }

  /* Sort out where to put the junk -Rasmus */

  tmp = world[real_room(vnum)].contents;
  while (tmp) {
    pos = tmp->worn_on;
    tmp->worn_on = 0;
    next = tmp->next_content;
    if (pos > 100) { /* item is inside another item */
      tmp2 = tmp;
      for (i = 100; i < pos; i++)
        tmp2 = tmp2->next;
      if (tmp2->item_number != NOTHING) /* Don't store in dummies */
      {
        obj_from_room(tmp);
        obj_to_obj(tmp, tmp2);
      }
    } else if (tmp->item_number == NOTHING) /* Throw away used dummies */
      extract_obj(tmp);
    tmp = next;
  }

  tmp = world[real_room(vnum)].contents;
  while (tmp) { /* count QIC in the room */
    next = tmp->next_content;
    load_qic_check(GET_OBJ_RNUM(tmp));
    tmp = tmp->next_content;
  }

  fclose(fl);
  return 1;
}

/* Modified by Rasmus Bertelsen */
/* Save all objects for a house (recursive; initial call must be followed
 by a call to House_restore_weight)  Assumes file is open already. */
int House_save(struct obj_data *obj, int pos, FILE *fp) {
  int Crash_obj_weight(struct obj_data * obj);
  int result;

  if (obj) {
    obj_count++;
    /* GET_OBJ_WEIGHT(obj) = Crash_obj_weight(obj); */

    if (pos == 0)
      result = Obj_to_store(obj, 100, fp);
    else
      result = Obj_to_store(obj, 100 + obj_count - pos, fp);

    if (!result)
      return 0;

    House_save(obj->contains, obj_count, fp);
    House_save(obj->next_content, pos, fp);
  }
  return 1;
}

/* restore weight of containers after House_save has changed them for saving */
void House_restore_weight(struct obj_data *obj) {
  if (obj) {
    House_restore_weight(obj->contains);
    House_restore_weight(obj->next_content);
    if (obj->in_obj)
      GET_OBJ_WEIGHT(obj->in_obj) += GET_OBJ_WEIGHT(obj);
  }
}

/* Modified by Rasmus Bertelsen */
/* Save all objects in a house */
void House_crashsave(int vnum) {
  int rnum;
  char buf[MAX_STRING_LENGTH];
  FILE *fp;

  obj_count = 0;

  if ((rnum = real_room(vnum)) == -1)
    return;
  if (!House_get_filename(vnum, buf))
    return;
  if (!(fp = fopen(buf, "wb"))) {
    perror("SYSERR: Error saving house file");
    return;
  }
  if (!House_save(world[rnum].contents, 0, fp)) {
    fclose(fp);
    return;
  }
  fclose(fp);
  House_restore_weight(world[rnum].contents);
  REMOVE_BIT(ROOM_FLAGS(rnum), ROOM_HOUSE_CRASH);
}

/* Delete a house save file */
void House_delete_file(int vnum) {
  char buf[MAX_INPUT_LENGTH], fname[MAX_INPUT_LENGTH];
  FILE *fl;

  if (!House_get_filename(vnum, fname))
    return;
  if (!(fl = fopen(fname, "rb"))) {
    if (errno != ENOENT) {
      safe_snprintf(buf, sizeof(buf), "SYSERR: Error deleting house file #%d. (1)", vnum);
      perror(buf);
    }
    return;
  }
  fclose(fl);
  if (unlink(fname) < 0) {
    safe_snprintf(buf, sizeof(buf), "SYSERR: Error deleting house file #%d. (2)", vnum);
    perror(buf);
  }
}

/* List all objects in a house file */
void House_listrent(struct char_data *ch, int vnum) {
  FILE *fl;
  char fname[MAX_STRING_LENGTH];
  char buf[MAX_STRING_LENGTH];
  struct obj_file_elem object;
  struct obj_data *obj;

  if (!House_get_filename(vnum, fname))
    return;
  if (!(fl = fopen(fname, "rb"))) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "No objects on file for house #%d.\r\n", vnum);
    send_to_char(buf, ch);
    return;
  }
  *buf = '\0';
  while (!feof(fl)) {
    fread(&object, sizeof(struct obj_file_elem), 1, fl);
    if (ferror(fl)) {
      fclose(fl);
      return;
    }
    if (!feof(fl) && (obj = Obj_from_store(object)) != NULL) {
      safe_snprintf(buf + strlen(buf), MAX_STRING_LENGTH - strlen(buf), " [%5d] (%5dau) %s\r\n", GET_OBJ_VNUM(obj),
                    GET_OBJ_RENT(obj), obj->short_description);
      free_obj(obj);
    }
  }

  send_to_char(buf, ch);
  fclose(fl);
}

/******************************************************************
 *  Functions for house administration (creation, deletion, etc.  *
 *****************************************************************/

int find_house(sh_int vnum) {
  int i;

  for (i = 0; i < num_of_houses; i++)
    if (house_control[i].vnum == vnum)
      return i;

  return -1;
}

char *House_find_owner(sh_int vnum) {
  int i;

  for (i = 0; i < num_of_houses; i++)
    if (house_control[i].vnum == vnum)
      return house_control[i].owner;

  return NULL;
}

/* Save the house control information */
void House_save_control(void) {
  FILE *fl;

  if (!(fl = fopen(HCONTROL_FILE, "wb"))) {
    perror("SYSERR: Unable to open house control file");
    return;
  }
  /* write all the house control recs in one fell swoop.  Pretty nifty, eh? */
  fwrite(house_control, sizeof(struct house_control_rec), num_of_houses, fl);

  fclose(fl);
}

/* call from boot_db - will load control recs, load objs, set atrium bits */
/* should do sanity checks on vnums & remove invalid records */
void House_boot(void) {
  struct house_control_rec temp_house;
  sh_int real_house, real_atrium;
  FILE *fl;

  memset((char *)house_control, 0, sizeof(struct house_control_rec) * MAX_HOUSES);

  if (!(fl = fopen(HCONTROL_FILE, "rb"))) {
    log("House control file does not exist.");
    return;
  }
  while (!feof(fl) && num_of_houses < MAX_HOUSES) {
    fread(&temp_house, sizeof(struct house_control_rec), 1, fl);

    if (feof(fl))
      break;

    if ((real_house = real_room(temp_house.vnum)) < 0)
      continue; /* this vnum doesn't exist -- skip */

    if ((find_house(temp_house.vnum)) >= 0)
      continue; /* this vnum is already a hosue -- skip */

    if ((real_atrium = real_room(temp_house.atrium)) < 0)
      continue; /* house doesn't have an atrium -- skip */

    if (temp_house.exit_num < 0 || temp_house.exit_num >= NUM_OF_DIRS)
      continue; /* invalid exit num -- skip */

    if (TOROOM(real_house, temp_house.exit_num) != real_atrium)
      continue; /* exit num mismatch -- skip */

    house_control[num_of_houses++] = temp_house;

    SET_BIT(ROOM_FLAGS(real_house), ROOM_HOUSE | ROOM_PRIVATE);
    SET_BIT(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);
    House_load(temp_house.vnum);
  }

  fclose(fl);
  House_save_control();
}

/* "House Control" functions */

char *HCONTROL_FORMAT = "Usage: hcontrol build <house vnum> <exit direction> <player name>\r\n"
                        "       hcontrol destroy <house vnum>\r\n"
                        "       hcontrol pay <house vnum>\r\n";

void hcontrol_list_houses(struct char_data *ch) {
  int i, j;
  char *timestr;
  char built_on[50], last_pay[50], own_name[50];

  if (!num_of_houses) {
    send_to_char("No houses have been defined.\r\n", ch);
    return;
  }
  size_t len =
      safe_snprintf(buf, MAX_STRING_LENGTH, "Address  Atrium  Build Date  Guests  Owner        Last Paymt\r\n");
  len += safe_snprintf(buf + len, MAX_STRING_LENGTH - len,
                       "-------  ------  ----------  ------  ------------ ----------\r\n");

  for (i = 0; i < num_of_houses; i++) {
    if (house_control[i].built_on) {
      timestr = asctime(localtime(&(house_control[i].built_on)));
      *(timestr + 10) = 0;
      safe_snprintf(built_on, sizeof(built_on), "%s", timestr);
    } else
      safe_snprintf(built_on, sizeof(built_on), "Unknown");

    if (house_control[i].last_payment) {
      timestr = asctime(localtime(&(house_control[i].last_payment)));
      *(timestr + 10) = 0;
      safe_snprintf(last_pay, sizeof(last_pay), "%s", timestr);
    } else
      safe_snprintf(last_pay, sizeof(last_pay), "None");

    safe_snprintf(own_name, sizeof(own_name), "%s", house_control[i].owner);

    len += safe_snprintf(buf + len, MAX_STRING_LENGTH - len, "%7d %7d  %-10s    %2d    %-12s %s\r\n",
                         house_control[i].vnum, house_control[i].atrium, built_on, house_control[i].num_of_guests,
                         CAP(own_name), last_pay);

    if (house_control[i].num_of_guests) {
      len += safe_snprintf(buf + len, MAX_STRING_LENGTH - len, "     Guests: ");
      for (j = 0; j < house_control[i].num_of_guests; j++) {
        safe_snprintf(buf2, MAX_STRING_LENGTH, "%s ", house_control[i].guests[j]);
        len += safe_snprintf(buf + len, MAX_STRING_LENGTH - len, "%s", CAP(buf2));
      }
      len += safe_snprintf(buf + len, MAX_STRING_LENGTH - len, "\r\n");
    }
  }
  send_to_char(buf, ch);
}

void hcontrol_build_house(struct char_data *ch, char *arg) {
  char arg1[MAX_INPUT_LENGTH];
  struct house_control_rec temp_house;
  sh_int virt_house, real_house, real_atrium, virt_atrium, exit_num;

  if (num_of_houses >= MAX_HOUSES) {
    send_to_char("Max houses already defined.\r\n", ch);
    return;
  }

  /* first arg: house's vnum */
  arg = one_argument(arg, arg1);
  if (!*arg1) {
    send_to_char(HCONTROL_FORMAT, ch);
    return;
  }
  virt_house = atoi(arg1);
  if ((real_house = real_room(virt_house)) < 0) {
    send_to_char("No such room exists.\r\n", ch);
    return;
  }
  if ((find_house(virt_house)) >= 0) {
    send_to_char("House already exists.\r\n", ch);
    return;
  }

  /* second arg: direction of house's exit */
  arg = one_argument(arg, arg1);
  if (!*arg1) {
    send_to_char(HCONTROL_FORMAT, ch);
    return;
  }
  if ((exit_num = search_block(arg1, dirs, FALSE)) < 0) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "'%s' is not a valid direction.\r\n", arg1);
    send_to_char(buf, ch);
    return;
  }
  if (TOROOM(real_house, exit_num) == NOWHERE) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "There is no exit %s from room %d.\r\n", dirs[exit_num], virt_house);
    send_to_char(buf, ch);
    return;
  }

  real_atrium = TOROOM(real_house, exit_num);
  virt_atrium = world[real_atrium].number;

  if (TOROOM(real_atrium, rev_dir[exit_num]) != real_house) {
    send_to_char("A house's exit must be a two-way door.\r\n", ch);
    return;
  }

  /* third arg: player's name */
  arg = one_argument(arg, arg1);
  if (!*arg1) {
    send_to_char(HCONTROL_FORMAT, ch);
    return;
  }
  if (get_id_by_name(arg1) < 0) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "Unknown player '%s'.\r\n", arg1);
    send_to_char(buf, ch);
    return;
  }

  temp_house.mode = HOUSE_PRIVATE;
  temp_house.vnum = virt_house;
  temp_house.atrium = virt_atrium;
  temp_house.exit_num = exit_num;
  temp_house.built_on = time(0);
  temp_house.last_payment = 0;
  safe_snprintf(temp_house.owner, sizeof(temp_house.owner), "%s", arg1);
  temp_house.num_of_guests = 0;

  house_control[num_of_houses++] = temp_house;

  SET_BIT(ROOM_FLAGS(real_house), ROOM_HOUSE | ROOM_PRIVATE);
  SET_BIT(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);
  House_crashsave(virt_house);

  send_to_char("House built.\r\n", ch);
  House_save_control();
}

void hcontrol_destroy_house(struct char_data *ch, char *arg) {
  int i, j;
  sh_int real_atrium, real_house;

  if (!*arg) {
    send_to_char(HCONTROL_FORMAT, ch);
    return;
  }
  if ((i = find_house(atoi(arg))) < 0) {
    send_to_char("Unknown house.\r\n", ch);
    return;
  }
  if ((real_atrium = real_room(house_control[i].atrium)) < 0)
    log("SYSERR: House had invalid atrium!");
  else
    REMOVE_BIT(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);

  if ((real_house = real_room(house_control[i].vnum)) < 0)
    log("SYSERR: House had invalid vnum!");
  else
    REMOVE_BIT(ROOM_FLAGS(real_house), ROOM_HOUSE | ROOM_PRIVATE | ROOM_HOUSE_CRASH);

  House_delete_file(house_control[i].vnum);

  for (j = i; j < num_of_houses - 1; j++)
    house_control[j] = house_control[j + 1];

  num_of_houses--;

  send_to_char("House deleted.\r\n", ch);
  House_save_control();

  /*
   * Now, reset the ROOM_ATRIUM flag on all existing houses' atriums,
   * just in case the house we just deleted shared an atrium with another
   * house.  --JE 9/19/94
   */
  for (i = 0; i < num_of_houses; i++)
    if ((real_atrium = real_room(house_control[i].atrium)) >= 0)
      SET_BIT(ROOM_FLAGS(real_atrium), ROOM_ATRIUM);
}

void hcontrol_pay_house(struct char_data *ch, char *arg) {
  int i;

  if (!*arg)
    send_to_char(HCONTROL_FORMAT, ch);
  else if ((i = find_house(atoi(arg))) < 0)
    send_to_char("Unknown house.\r\n", ch);
  else {
    safe_snprintf(buf, MAX_STRING_LENGTH, "Payment for house %s collected by %s.", arg, GET_NAME(ch));
    mudlog(buf, 'H', COM_IMMORT, TRUE);

    house_control[i].last_payment = time(0);
    House_save_control();
    send_to_char("Payment recorded.\r\n", ch);
  }
}

/* The hcontrol command itself, used by imms to create/destroy houses */ ACMD(do_hcontrol) {
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

  half_chop(argument, arg1, arg2);

  if (!*arg1)
    hcontrol_list_houses(ch);
  else if (is_abbrev(arg1, "build"))
    hcontrol_build_house(ch, arg2);
  else if (is_abbrev(arg1, "destroy"))
    hcontrol_destroy_house(ch, arg2);
  else if (is_abbrev(arg1, "pay"))
    hcontrol_pay_house(ch, arg2);
  else
    send_to_char(HCONTROL_FORMAT, ch);
}

/* The house command, used by mortal house owners to assign guests */ ACMD(do_house) {
  int i, j;

  one_argument(argument, arg);

  if (!IS_SET(ROOM_FLAGS(ch->in_room), ROOM_HOUSE))
    send_to_char("You must be in your house to set guests.\r\n", ch);
  else if ((i = find_house(world[ch->in_room].number)) < 0)
    send_to_char("Um.. this house seems to be screwed up.\r\n", ch);
  else if (strcmp(GET_NAME(ch), house_control[i].owner))
    send_to_char("Only the primary owner can set guests.\r\n", ch);
  else if (!*arg) {
    send_to_char("Guests of your house:\r\n", ch);
    if (house_control[i].num_of_guests == 0)
      send_to_char("  None.\r\n", ch);
    else
      for (j = 0; j < house_control[i].num_of_guests; j++) {
        safe_snprintf(buf, MAX_STRING_LENGTH, "%s\r\n", house_control[i].guests[j]);
        send_to_char(CAP(buf), ch);
      }
  } else if (get_id_by_name(arg) < 0)
    send_to_char("No such player.\r\n", ch);
  else {
    for (j = 0; j < house_control[i].num_of_guests; j++)
      if (!strcmp(house_control[i].guests[j], arg)) {
        for (; j < house_control[i].num_of_guests; j++)
          safe_snprintf(house_control[i].guests[j], MAX_NAME_LENGTH, "%s", house_control[i].guests[j + 1]);
        house_control[i].num_of_guests--;
        House_save_control();
        send_to_char("Guest deleted.\r\n", ch);
        return;
      }
    j = house_control[i].num_of_guests++;
    safe_snprintf(house_control[i].guests[j], MAX_NAME_LENGTH, "%s", arg);
    House_save_control();
    send_to_char("Guest added.\r\n", ch);
  }
}

/* Misc. administrative functions */

/* crash-save all the houses */
void House_save_all(void) {
  int i;
  sh_int real_house;

  for (i = 0; i < num_of_houses; i++)
    if ((real_house = real_room(house_control[i].vnum)) != NOWHERE)
      if (IS_SET(ROOM_FLAGS(real_house), ROOM_HOUSE_CRASH))
        House_crashsave(house_control[i].vnum);
}

/* note: arg passed must be house vnum, so there. */
int House_can_enter(struct char_data *ch, sh_int house) {
  int i, j;

  if (COM_FLAGGED(ch, COM_BUILDER) || (i = find_house(house)) < 0)
    return 1;

  switch (house_control[i].mode) {
  case HOUSE_PRIVATE:
    if (!strcmp(GET_NAME(ch), house_control[i].owner))
      return 1;
    for (j = 0; j < house_control[i].num_of_guests; j++)
      if (!strcmp(GET_NAME(ch), house_control[i].guests[j]))
        return 1;
    return 0;
    break;
  }

  return 0;
}
