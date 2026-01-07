/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  _TwyliteMud_ by Rv.                          Based on CircleMud3.0bpl9 *
 *    				                                          *
 *  OasisOLC - olc.c 		                                          *
 *    				                                          *
 *  Copyright 1996 Harvey Gilpin.                                          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define _RV_OLC_

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "structs.h"
#include "comm.h"
#include "utils.h"
#include "interpreter.h"
#include "db.h"
#include "olc.h"
#include "screen.h"
#include "handler.h"

/*. External data structures .*/
extern struct room_data *world;
extern int top_of_zone_table;
extern struct zone_data *zone_table;
extern struct descriptor_data *descriptor_list;

/*. External functions .*/
extern void zedit_setup(struct descriptor_data *d, int room_num);
extern void zedit_new_zone(struct char_data *ch, int new_zone);
extern void medit_setup_new(struct descriptor_data *d);
extern void medit_setup_existing(struct descriptor_data *d, int rmob_num);
extern void redit_setup_new(struct descriptor_data *d);
extern void redit_setup_existing(struct descriptor_data *d, int rroom_num);
extern void oedit_setup_new(struct descriptor_data *d);
extern void oedit_setup_existing(struct descriptor_data *d, int robj_num);
extern void sedit_setup_new(struct descriptor_data *d);
extern void sedit_setup_existing(struct descriptor_data *d, int robj_num);
extern int real_shop(int vnum);
extern void free_shop(struct shop_data *shop);
extern void free_room(struct room_data *room);

/*. Internal function prototypes .*/
int real_zone(int number);
void olc_saveinfo(struct char_data *ch);

/*. Internal data .*/

struct olc_scmd_data {
  char *text;
  int con_type;
};

struct olc_scmd_data olc_scmd_info[5] = { {"room", CON_REDIT}, {"object", CON_OEDIT}, {"room", CON_ZEDIT}, {"mobile", CON_MEDIT}, {"shop", CON_SEDIT}};

/*------------------------------------------------------------*\
 Eported ACMD do_olc function

 This function is the OLC interface.  It deals with all the
 generic OLC stuff, then passes control to the sub-olc sections.
 \*------------------------------------------------------------*/

ACMD(do_olc)
{
  int number = -1, save = 0, real_num;
  struct descriptor_data *d;
  int zone1, zone2;
  int oktoedit;
  int i;
  char tbuf[256];

  if (IS_NPC(ch))
    /*. No screwing arround .*/
    return;

  if (subcmd == SCMD_OLC_SAVEINFO) {
    olc_saveinfo(ch);
    return;
  }

  /*. Parse any arguments .*/
  two_arguments(argument, buf1, buf2);
  if (!*buf1) { /* No argument given .*/
    switch (subcmd) {
      case SCMD_OLC_ZEDIT:
      case SCMD_OLC_REDIT:
        number = world[IN_ROOM(ch)].number;
        break;
      case SCMD_OLC_OEDIT:
      case SCMD_OLC_MEDIT:
      case SCMD_OLC_SEDIT:
        snprintf(buf, MAX_STRING_LENGTH, "Specify a %s VNUM to edit.\r\n", olc_scmd_info[subcmd].text);
        send_to_char(buf, ch);
        return;
    }
  } else if (!isdigit (*buf1)) {
    if (strncmp("save", buf1, 4) == 0) {
      if (!*buf2) {
        send_to_char("Save which zone?\r\n", ch);
        return;
      } else {
        save = 1;
        number = atoi(buf2) * 100;
      }
    } else if (subcmd == SCMD_OLC_ZEDIT && COM_FLAGGED(ch, COM_ADMIN)) {
      if ((strncmp("new", buf1, 3) == 0) && *buf2)
        zedit_new_zone(ch, atoi(buf2));
      else
        send_to_char("Specify a new zone number.\r\n", ch);
      return;
    } else {
      send_to_char("Yikes!  Stop that, someone will get hurt!\r\n", ch);
      return;
    }
  }

  /*. If a numeric argument was given, get it .*/
  if (number == -1) {
    number = atoi(buf1);
  }

  oktoedit = 0;
  /*. Check number to see if it is an acceptable zone for them to edit .*/
  if (!COM_FLAGGED(ch, COM_ADMIN)) {
    zone1 = (number - (number % 100)) / 100;
    for (i = 0; i < 4; i++) {
      sprintf(tbuf, "Checking %d <==> %d", zone1, ch->olc_zones[i]);
      mudlog(tbuf, 'D', COM_BUILDER, TRUE);
      if (ch->olc_zones[i] == zone1) {
        oktoedit = 1;
        break;
      }
    }
    if (!oktoedit || zone1 == 0) {
      send_to_char("You are not authorized to edit that zone.\r\n", ch);
      return;
    }
  }

  /*. Check whatever it is isn't already being edited .*/
  for (d = descriptor_list; d; d = d->next) {
    if (d->connected == olc_scmd_info[subcmd].con_type) {
      if (d->olc) {
        zone1 = OLC_NUM(d) - (OLC_NUM(d) % 100);
        zone2 = number - (number % 100);
        if (zone1 == zone2) {
          snprintf(buf, MAX_STRING_LENGTH, "That %s is currently being edited by %s.\r\n", olc_scmd_info[subcmd].text, GET_NAME(d->character));
          send_to_char(buf, ch);
          return;
        }
      }
    }
  }

  d = ch->desc;

  /*. Give descriptor an OLC struct .*/
  CREATE(d->olc, struct olc_data, 1);

  /*. Find the zone .*/OLC_ZNUM(d) = real_zone(number);
  if (OLC_ZNUM(d) == -1) {
    send_to_char("Sorry, there is no zone for that number!\r\n", ch);
    FREE(d->olc);
    return;
  }

  if (save) {
    return;
  }

  OLC_NUM(d) = number;
  SET_BIT(PLR_FLAGS (ch), PLR_EDITING);

  /*. Steal players descriptor start up subcommands .*/
  switch (subcmd) {
    case SCMD_OLC_REDIT:
      real_num = real_room(number);
      if (real_num >= 0)
        redit_setup_existing(d, real_num);
      else
        redit_setup_new(d);
      snprintf(buf, MAX_STRING_LENGTH, "OLC: %s edits a room in zone %d (%d)", GET_NAME(ch), zone_table[OLC_ZNUM(d)].number, number);
      mudlog(buf, 'G', COM_BUILDER, TRUE);
      STATE(d) = CON_REDIT;
      break;
    case SCMD_OLC_ZEDIT:
      real_num = real_room(number);
      if (real_num < 0) {
        send_to_char("That room does not exist.\r\n", ch);
        FREE(d->olc);
        return;
      }
      zedit_setup(d, real_num);
      snprintf(buf, MAX_STRING_LENGTH, "OLC: %s edits zone info for zone %d", GET_NAME(ch), zone_table[OLC_ZNUM(d)].number);
      mudlog(buf, 'G', COM_BUILDER, TRUE);
      STATE(d) = CON_ZEDIT;
      break;
    case SCMD_OLC_MEDIT:
      real_num = real_mobile(number);
      if (real_num < 0) {
        medit_setup_new(d);
        /*
         REMOVE_BIT(PLR_FLAGS (ch), PLR_EDITING);
         send_to_char("Medit for new mobs is temporarily disabled.\r\n", ch);
         return;
         */
      } else
        medit_setup_existing(d, real_num);
      snprintf(buf, MAX_STRING_LENGTH, "OLC: %s edits a mob in zone %d (%d)", GET_NAME(ch), zone_table[OLC_ZNUM(d)].number, number);
      mudlog(buf, 'G', COM_BUILDER, TRUE);
      STATE(d) = CON_MEDIT;
      break;
    case SCMD_OLC_OEDIT:
      real_num = real_object(number);
      if (real_num >= 0)
        oedit_setup_existing(d, real_num);
      else {
        oedit_setup_new(d);
        /*
         REMOVE_BIT(PLR_FLAGS (ch), PLR_EDITING);
         send_to_char("Oedit for new objects is temporarily disabled.\r\n", ch);
         return;
         */
      }
      snprintf(buf, MAX_STRING_LENGTH, "OLC: %s edits a object in zone %d (%d)", GET_NAME(ch), zone_table[OLC_ZNUM(d)].number, number);
      mudlog(buf, 'G', COM_BUILDER, TRUE);
      STATE(d) = CON_OEDIT;
      break;
    case SCMD_OLC_SEDIT:
      real_num = real_shop(number);
      if (real_num >= 0)
        sedit_setup_existing(d, real_num);
      else
        sedit_setup_new(d);
      snprintf(buf, MAX_STRING_LENGTH, "OLC: %s edits a shop in zone %d (%d)", GET_NAME(ch), zone_table[OLC_ZNUM(d)].number, number);
      mudlog(buf, 'G', COM_BUILDER, TRUE);
      STATE(d) = CON_SEDIT;
      break;
  }
  act("$n starts using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
}
/*------------------------------------------------------------*\
 Internal utlities
 \*------------------------------------------------------------*/

void olc_saveinfo(struct char_data *ch)
{
  struct olc_save_info *entry;
  static char *save_info_msg[5] = {"Rooms", "Objects", "Zone info", "Mobiles", "Shops"};

  if (olc_save_list)
    send_to_char("The following OLC components need saving:-\r\n", ch);
  else
    send_to_char("The database is up to date.\r\n", ch);

  for (entry = olc_save_list; entry; entry = entry->next) {
    snprintf(buf, MAX_STRING_LENGTH, " - %s for zone %d.\r\n", save_info_msg[(int) entry->type], entry->zone);
    send_to_char(buf, ch);
  }
}

int real_zone(int number)
{
  int counter;
  for (counter = 0; counter <= top_of_zone_table; counter++)
    if ((number >= (zone_table[counter].number * 100)) && (number <= (zone_table[counter].top)))
      return counter;

  return -1;
}

/*------------------------------------------------------------*\
 Exported utlities
 \*------------------------------------------------------------*/

/*. Add an entry to the 'to be saved' list .*/

void olc_add_to_save_list(int zone, byte type)
{
  struct olc_save_info *new;

  /*. Return if it's already in the list .*/
  for (new = olc_save_list; new; new = new->next)
    if ((new->zone == zone) && (new->type == type))
      return;

  CREATE(new, struct olc_save_info, 1);
  new->zone = zone;
  new->type = type;
  new->next = olc_save_list;
  olc_save_list = new;
}

/*. Remove an entry from the 'to be saved' list .*/

void olc_remove_from_save_list(int zone, byte type)
{
  struct olc_save_info **entry;
  struct olc_save_info *temp;

  for (entry = &olc_save_list; *entry; entry = &(*entry)->next)
    if (((*entry)->zone == zone) && ((*entry)->type == type)) {
      temp = *entry;
      *entry = temp->next;
      FREE(temp);
      return;
    }
}

/*. Set the colour string pointers for that which this char will
 see at color level NRM.  Changing the entries here will change
 the colour scheme throught the OLC.*/

void get_char_cols(struct char_data *ch)
{
  nrm = CCNRM(ch, C_NRM);
  grn = CCGRN(ch, C_NRM);
  cyn = CCCYN(ch, C_NRM);
  yel = CCYEL(ch, C_NRM);
}

/*. This procedure removes the '\r' from a string so that it may be
 saved to a file.  Use it only on buffers, not on the oringinal
 strings.*/

void strip_string(char *buffer)
{
  char *pointer;

  pointer = buffer;
  while ((pointer = strchr(pointer, '\r')))
    /*. Hardly elegant, but it does the job .*/
    strcpy(pointer, pointer + 1);
}

/*. This procdure frees up the strings and/or the structures
 attatched to a descriptor, sets all flags back to how they
 should be .*/

void cleanup_olc(struct descriptor_data *d, byte cleanup_type)
{
  if (d->olc) {
    /*. Check for room .*/
    if (OLC_ROOM(d)) { /*. free_room performs no sanity checks, must be carefull here .*/
      switch (cleanup_type) {
        case CLEANUP_ALL:
          free_room(OLC_ROOM(d));
          FREE(OLC_ROOM(d));
          break;
        case CLEANUP_STRUCTS:
          FREE(OLC_ROOM(d));
          break;
        default:
          /*. Caller has screwed up .*/
          break;
      }
    }

    /*. Check for object .*/
    if (OLC_OBJ(d)) { /*. free_obj checks strings aren't part of proto .*/
      free_obj(OLC_OBJ(d));
    }

    /*. Check for mob .*/
    if (OLC_MOB(d)) { /*. free_char checks strings aren't part of proto .*/
      MPROG_DATA *temp;

      free_char(OLC_MOB(d));
      for (temp = OLC_MPROGL(d); temp; OLC_MPROGL(d) = temp) {
        temp = temp->next;
        FREE(OLC_MPROGL(d));
      }
    }

    /*. Check for zone .*/
    if (OLC_ZONE(d)) { /*. cleanup_type is irrelivent here, free everything .*/
      FREE(OLC_ZONE(d)->name);
      FREE(OLC_ZONE(d)->cmd);
      FREE(OLC_ZONE(d));
    }

    /*. Check for shop .*/
    if (OLC_SHOP(d)) { /*. free_shop performs no sanity checks, must be carefull here .*/
      switch (cleanup_type) {
        case CLEANUP_ALL:
          free_shop(OLC_SHOP(d));
          break;
        case CLEANUP_STRUCTS:
          FREE(OLC_SHOP(d));
          break;
        default:
          /*. Caller has screwed up .*/
          break;
      }
    }

    /*. Restore desciptor playing status .*/
    if (d->character) {
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_EDITING);
      STATE(d) = CON_PLAYING;
      act("$n stops using OLC.", TRUE, d->character, 0, 0, TO_ROOM);
    }
    FREE(d->olc);
  }
}

void olc_print_bitvectors(FILE* f, long bitvector, long max)
{
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

ACMD(do_assign)
{
  char tbuf[256];
  char *buf2;
  struct char_data *vict;
  int found;
  int znum[4];
  int i;

  buf[0] = '\0';
  if (!COM_FLAGGED(ch, COM_ADMIN)) {
    send_to_char("You are not authorized to assign zones to another player.\r\n", ch);
  }

  buf2 = one_argument(argument, buf);

  if (!*buf) {
    send_to_char("Usage: assign <player> [zone num]\r\n", ch);
    return;
  }

  if (!*buf2) {
    if ((vict = get_char_vis(ch, buf)) == NULL) {
      send_to_char("That player isn't online.\r\n", ch);
      return;
    }
    sprintf(tbuf, "%s has been assigned these zones: %d %d %d %d\r\n", GET_NAME(vict), vict->olc_zones[0], vict->olc_zones[1], vict->olc_zones[2], vict->olc_zones[3]);
    send_to_char(tbuf, ch);
  } else {
    if ((vict = get_char_vis(ch, buf)) == NULL) {
      send_to_char("That player isn't online.\r\n", ch);
      return;
    }
    found = sscanf(buf2, "%d %d %d %d", &znum[0], &znum[1], &znum[2], &znum[3]);
    for (i = 0; i < found; i++) {
      vict->olc_zones[i] = znum[i];
    }
    for (i = found; i < 4; i++) {
      vict->olc_zones[i] = 0;
    }
    sprintf(tbuf, "%s has been assigned these zones: %d %d %d %d\r\n", GET_NAME(vict), vict->olc_zones[0], vict->olc_zones[1], vict->olc_zones[2], vict->olc_zones[3]);
    send_to_char(tbuf, ch);
  }
}
