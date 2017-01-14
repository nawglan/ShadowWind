/* ************************************************************************
 *   File: act.other.c                                   Part of CircleMUD *
 *  Usage: Miscellaneous player-level commands                             *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"
#include "event.h"
#include "spells.h"
#include "ident.h"

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct char_data* character_list;
extern struct room_data *world;
extern struct index_data *obj_index;
extern char *class_abbrevs[];
extern void save_text(struct char_data * ch);
extern struct actd_msg *get_actd(int vnum);
char *todolist = NULL;
EVENT(camp);
ACMD(do_reboot);
void Crash_save(struct char_data *ch, int type);
extern const int rev_dir[];

/* extern procedures */
SPECIAL(shop_keeper);

ACMD(do_search)
{
  int door, other_room;
  int found = 0;
  struct room_direction_data *back;

  for (door = 0; door < NUM_OF_DIRS; door++) {
    if (EXIT(ch, door) && IS_SET(EXIT(ch, door)->exit_info, EX_HIDDEN)) {
      if (number(1, 101) <= GET_INT(ch)) {
        REMOVE_BIT(EXIT(ch, door)->exit_info, EX_HIDDEN);
        if ((other_room = EXIT(ch, door)->to_room) != NOWHERE) {
          if ((back = world[other_room].dir_option[rev_dir[door]])) {
            if (back->to_room == ch->in_room) {
              REMOVE_BIT(back->exit_info, EX_HIDDEN);
            }
          }
        }
        send_to_char("You find a secret exit!\r\n", ch);
        found = 1;
        break;
      }
    }
  }
  if (!found) {
    send_to_char("Your search avails nothing.\r\n", ch);
  }
}

void give_consent(struct char_data *ch, struct char_data *to)
{
  struct consent_data *consent;

  CREATE(consent, struct consent_data, 1);
  consent->name = strdup(GET_NAME(ch));

  if (to->consent) {
    to->consent->prev = consent;
  }
  consent->next = to->consent;
  to->consent = consent;
}

void remove_consent(struct char_data *ch)
{
  struct char_data *tch;
  struct char_data *tch_next;
  struct consent_data *consent;
  struct consent_data *consent_next;

  for (tch = character_list; tch; tch = tch_next) {
    tch_next = tch->next;
    if (IS_NPC(tch) || tch == ch) {
      continue;
    }
    consent = tch->consent;
    if (consent && consent->name) {
      if (strcasecmp(consent->name, GET_NAME(ch)) == 0) {
        tch->consent = tch->consent->next;
        sprintf(buf, "You no longer have %s's consent.\r\n", GET_NAME(ch));
        send_to_char(buf, tch);
        if (GET_DRAGGING(tch) && strcasecmp(GET_DRAGGING(tch)->owner, GET_NAME(ch)) == 0) {
          GET_DRAGGING(tch)->dragged_by = NULL;
          GET_DRAGGING(tch) = NULL;
        }
        FREE(consent->name);
        FREE(consent);
      } else {
        for (consent = tch->consent; consent; consent = consent_next) {
          consent_next = consent->next;
          if (strcasecmp(consent->name, GET_NAME(ch)) == 0) {
            consent->prev->next = consent->next;
            if (consent->next) {
              consent->next->prev = consent->prev;
            }
            sprintf(buf, "You no longer have %s's consent.\r\n", GET_NAME(ch));
            send_to_char(buf, tch);
            if (GET_DRAGGING(tch) && strcasecmp(GET_DRAGGING(tch)->owner, GET_NAME(ch)) == 0) {
              GET_DRAGGING(tch)->dragged_by = NULL;
              GET_DRAGGING(tch) = NULL;
            }
            FREE(consent->name);
            FREE(consent);
          }
        }
      }
    }
  }
}

int check_consent(struct char_data *ch, char *vict)
{
  struct consent_data *consent;
  struct consent_data *consent_next;

  for (consent = ch->consent; consent; consent = consent_next) {
    consent_next = consent->next;
    if (strcasecmp(consent->name, vict) == 0) {
      return 1;
    }
  }
  return 0;
}

ACMD(do_consent)
{
  struct char_data* tch;
  char gender[4];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Consent who?\r\n", ch);
    return;
  }

  if (IS_NPC(ch)) {
    return;
  }

  if (strcasecmp(arg, "me") == 0 || strcasecmp(arg, GET_NAME(ch)) == 0) {
    remove_consent(ch);
    send_to_char("You decide that only you can be trusted.\r\n", ch);
    return;
  }

  if (GET_SEX(ch) == SEX_MALE) {
    strcpy(gender, "his");
  } else if (GET_SEX(ch) == SEX_FEMALE) {
    strcpy(gender, "her");
  }
  tch = get_char_vis(ch, arg);
  if (!tch) {
    send_to_char("Consent who?\r\n", ch);
    return;
  }
  sprintf(buf, "%s has entrusted you with %s life!\r\n", GET_NAME(ch), gender);
  send_to_char(buf, tch);
  sprintf(buf, "You entrust %s with your life!\r\n", GET_NAME(tch));
  send_to_char(buf, ch);
  give_consent(ch, tch);
}

ACMD(do_specialize)
{
  if (IS_NPC(ch)) {
    send_to_char("Go away!\r\n", ch);
    return;
  }

  if (PLR_FLAGGED(ch, PLR_SPECIALIZED)) {
    send_to_char("You are already specialized!\r\n", ch);
    return;
  }

  if (GET_LEVEL(ch) != 10) {
    send_to_char("You need to be level 10 to specialize.\r\n", ch);
    return;
  }

  send_to_char("You can only specialize in your guild.\r\n", ch);
  return;

}

ACMD(do_quit)
{
  void die(struct char_data * ch, struct char_data * killer);
  void clear_magic_memory(struct char_data *ch);
  struct descriptor_data *d, *next_d;
  extern int free_rent;

  if (IS_NPC(ch) || !ch->desc) {
    return;
  }

  one_argument(argument, arg);

  if (subcmd == 0 && GET_LEVEL(ch) < LVL_IMMORT) {
    send_to_char("You have to type quit - no less, to quit!\r\n", ch);
  } else if (GET_POS(ch) == POS_FIGHTING) {
    send_to_char("No way!  You're fighting for your life!\r\n", ch);
  } else if (GET_POS(ch) < POS_STUNNED && (*arg == 'y' || *arg == 'Y')) {
    send_to_char("You die before your time...\r\n", ch);
    sprintf(buf, "%s has quit the game in room #%d. (died)", GET_NAME(ch), world[IN_ROOM(ch)].number);
    mudlog(buf, 'C', COM_IMMORT, TRUE);
    plog(buf, ch, 0);
    clear_magic_memory(ch);
    die(ch, NULL);
  } else if ((*arg != 'y' && *arg != 'Y') && GET_LEVEL(ch) < LVL_IMMORT && !free_rent) {
    send_to_char("If you type quit, all your equipment will be dropped to the ground.\r\n"
        "If you really want to quit, type 'quit y', otherwise rent in the inn.\r\n", ch);
  } else {
    if (!GET_INVIS_LEV(ch)) {
      act("$n has left the game.", TRUE, ch, 0, 0, TO_ROOM);
    }
    sprintf(buf, "%s has quit the game in room #%d.", GET_NAME(ch), world[IN_ROOM(ch)].number);
    mudlog(buf, 'C', COM_IMMORT, TRUE);
    plog(buf, ch, 0);
    sprintf(buf, "Goodbye, %s... Come back soon!\r\n", ch->player.name);
    send_to_char(buf, ch);

    /*
     * kill off all sockets connected to the same player as the one who is
     * trying to quit.  Helps to maintain sanity as well as prevent duping.
     */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (d == ch->desc) {
        continue;
      }
      if (d->character && (GET_IDNUM(d->character) == GET_IDNUM(ch))) {
        close_socket(d);
      }
    }

    save_text(ch); /* save aliases etc */
    clear_magic_memory(ch);
    if ((free_rent || (GET_LEVEL(ch) >= LVL_IMMORT)) && subcmd == SCMD_QUIT) {
      Crash_save(ch, RENT_RENTED);
      extract_char(ch, 0); /* Char is saved in extract char */
    } else {
      extract_char(ch, 1); /* Char is saved in extract char */
    }
  }
}

ACMD(do_save)
{
  if (IS_NPC(ch) || !ch->desc) {
    return;
  }

  if (cmd) {
    send_to_char("Done.\r\n", ch);
    save_text(ch);
    save_char_text(ch, NOWHERE);
    Crash_save(ch, RENT_CRASH);
  } else {
    save_text(ch);
    save_char_text(ch, NOWHERE);
    Crash_save(ch, RENT_CRASH);
  }
}

/* generic function for commands which are normally overridden by
 special procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here)
{
  send_to_char("Sorry, but you cannot do that here!\r\n", ch);
}

ACMD(do_practice)
{
  void list_skills(struct char_data * ch);

  one_argument(argument, arg);

  if (*arg) {
    send_to_char("You can only practice skills in your guild.\r\n", ch);
  } else if (CMD_IS("specialize")) {
    if (IS_MAGE(ch) || IS_PRI(ch)) {
      send_to_char("You can only specialize in a realm of magic in your guild.\r\n", ch);
    } else {
      send_to_char("You have no need to specialize in a realm of magic.\r\n", ch);
    }
  } else {
    list_skills(ch);
  }
}

ACMD(do_train)
{
  one_argument(argument, arg);

  if (*arg) {
    send_to_char("You can only train combat techniques with your master.\r\n", ch);
  }
}

ACMD(do_visible)
{
  void appear(struct char_data * ch);

  if (IS_AFFECTED(ch, AFF_INVISIBLE)) {
    appear(ch);
    send_to_char("You break the spell of invisibility.\r\n", ch);
  } else {
    send_to_char("You are already visible.\r\n", ch);
  }
}

int perform_group(struct char_data *ch, struct char_data *vict)
{
  if (IS_AFFECTED(vict, AFF_GROUP) || !CAN_SEE(ch, vict) || IS_NPC(vict)) {
    return 0;
  }

  if (vict->in_room != ch->in_room) {
    return 0;
  }
  SET_BIT(AFF_FLAGS(vict), AFF_GROUP);
  if (ch != vict) {
    act("$N is now a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
  }
  act("You are now a member of $n's group.", FALSE, ch, 0, vict, TO_VICT);
  act("$N is now a member of $n's group.", FALSE, ch, 0, vict, TO_NOTVICT);

  return 1;
}

void print_group(struct char_data *ch)
{
  struct char_data *k;
  struct follow_type *f;

  if (!IS_AFFECTED(ch, AFF_GROUP)) {
    send_to_char("But you are not the member of a group!\r\n", ch);
  } else {
    send_to_char("{cYour group consists of{C:{x\r\n", ch);

    k = (ch->master ? ch->master : ch);

    if (IS_AFFECTED(k, AFF_GROUP)) {
      if (IN_ROOM(k) != IN_ROOM(ch)) {
        act("                                            $N {R({WHead of group{R){x", FALSE, ch, 0, k, TO_CHAR);
      } else {
        sprintf(buf, "     {c[{W%3d{w/{W%3d{wH {G%3d{g/{G%3d{gM {B%3d{b/{B%3d{bV{c] {c[{W%2d %s{c]{x $N {R({WHead of group{R){x", GET_HIT(k), GET_MAX_HIT(k), GET_MANA(k), GET_MAX_MANA(k), GET_MOVE(k), GET_MAX_MOVE(k), GET_LEVEL(k), CLASS_ABBR(k));
        act(buf, FALSE, ch, 0, k, TO_CHAR);
      }
    }

    for (f = k->followers; f; f = f->next) {
      if (!IS_AFFECTED(f->follower, AFF_GROUP)) {
        continue;
      }
      if (IN_ROOM(f->follower) != IN_ROOM(ch)) {
        act("                                            {x $N", FALSE, ch, 0, f->follower, TO_CHAR);
      } else {
        sprintf(buf, "     {c[{W%3d{w/{W%3d{wH {G%3d{g/{G%3d{gM {B%3d{b/{B%3d{bV{c] {c[{W%2d %s{c]{x $N", GET_HIT(f->follower), GET_MAX_HIT(f->follower), GET_MANA(f->follower), GET_MAX_MANA(f->follower), GET_MOVE(f->follower), GET_MAX_MOVE(f->follower), GET_LEVEL(f->follower), CLASS_ABBR(f->follower));
        act(buf, FALSE, ch, 0, f->follower, TO_CHAR);
      }
    }
  }
}

ACMD(do_group)
{
  struct char_data *vict;
  struct follow_type *f;
  int found;

  one_argument(argument, buf);

  if (!*buf) {
    print_group(ch);
    return;
  }

  if (IS_NPC(ch)) {
    send_to_char("Drop dead, Fred.\r\n", ch);
    return;
  }

  if (ch->master) {
    act("You can not enroll group members without being head of a group.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if (!str_cmp(buf, "all")) {
    perform_group(ch, ch);
    for (found = 0, f = ch->followers; f; f = f->next) {
      found += perform_group(ch, f->follower);
    }
    if (!found) {
      send_to_char("Everyone following you is already in your group.\r\n", ch);
    }
    return;
  }

  if (!(vict = get_char_room_vis(ch, buf))) {
    send_to_char(NOPERSON, ch);
  } else if ((vict->master != ch) && (vict != ch)) {
    act("$N must follow you to enter your group.", FALSE, ch, 0, vict, TO_CHAR);
  } else {
    if (!IS_AFFECTED(vict, AFF_GROUP)) {
      perform_group(ch, vict);
    } else {
      if (ch != vict) {
        act("$N is no longer a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
      }
      act("You have been kicked out of $n's group!", FALSE, ch, 0, vict, TO_VICT);
      act("$N has been kicked out of $n's group!", FALSE, ch, 0, vict, TO_NOTVICT);
      REMOVE_BIT(AFF_FLAGS(vict), AFF_GROUP);
    }
  }
}

ACMD(do_ungroup)
{
  struct follow_type *f, *next_fol;
  struct char_data *tch;
  void stop_follower(struct char_data * ch);

  one_argument(argument, buf);

  if (!*buf) {
    if (ch->master || !(IS_AFFECTED(ch, AFF_GROUP))) {
      send_to_char("But you lead no group!\r\n", ch);
      return;
    }
    sprintf(buf2, "%s has disbanded the group.\r\n", GET_NAME(ch));
    for (f = ch->followers; f; f = next_fol) {
      next_fol = f->next;
      if (IS_AFFECTED(f->follower, AFF_GROUP)) {
        REMOVE_BIT(AFF_FLAGS(f->follower), AFF_GROUP);
        send_to_char(buf2, f->follower);
        if (!IS_AFFECTED(f->follower, AFF_CHARM)) {
          stop_follower(f->follower);
        }
      }
    }

    REMOVE_BIT(AFF_FLAGS(ch), AFF_GROUP);
    send_to_char("You disband the group.\r\n", ch);
    return;
  }
  if (!(tch = get_char_room_vis(ch, buf))) {
    send_to_char("There is no such person!\r\n", ch);
    return;
  }
  if (tch->master != ch) {
    send_to_char("That person is not following you!\r\n", ch);
    return;
  }

  if (!IS_AFFECTED(tch, AFF_GROUP)) {
    send_to_char("That person isn't in your group.\r\n", ch);
    return;
  }

  REMOVE_BIT(AFF_FLAGS(tch), AFF_GROUP);

  act("$N is no longer a member of your group.", FALSE, ch, 0, tch, TO_CHAR);
  act("You have been kicked out of $n's group!", FALSE, ch, 0, tch, TO_VICT);
  act("$N has been kicked out of $n's group!", FALSE, ch, 0, tch, TO_NOTVICT);

  if (!IS_AFFECTED(tch, AFF_CHARM)) {
    stop_follower(tch);
  }
}

ACMD(do_report)
{
  char arg[80];
  char buff[80];
  char buff2[80];
  ACMD(do_say);
  ACMD(do_gsay);
  ACMD(do_tell);

  one_argument(argument, arg);
  sprintf(buff, "{wI have: {y%d{w/{y%dH{w, {y%d{w/{y%dM{w, {y%d{w/{y%dV{x", GET_HIT(ch), GET_MAX_HIT(ch), GET_MANA(ch), GET_MAX_MANA(ch), GET_MOVE(ch), GET_MAX_MOVE(ch));

  if (!*arg) {
    do_say(ch, buff, 0, 0);
    return;
  }

  if (strcasecmp(arg, "group") == 0) {
    do_gsay(ch, buff, 0, 0);
    return;
  }

  sprintf(buff2, "%s %s", arg, buff);
  do_tell(ch, buff2, 0, 0);
}

ACMD(do_split)
{
  int amount, num, share;
  struct char_data *k;
  struct follow_type *f;
  char value[256];
  char type[256];
  char *p;
  int is_plat = 0, is_gold = 0, is_silver = 0, is_copper = 0;

  if (IS_NPC(ch)) {
    return;
  }

  p = two_arguments(argument, value, type);

  if (is_number(value)) {
    amount = atoi(value);
    if (amount <= 0) {
      send_to_char("Sorry, you can't do that.\r\n", ch);
      return;
    }
    switch (type[0]) {
      case 'P':
      case 'p':
        if (amount > GET_PLAT(ch)) {
          send_to_char("You don't seem to have that much platinum to split.\r\n", ch);
          return;
        } else {
          is_plat = 1;
        }
        break;
      case 'G':
      case 'g':
        if (amount > GET_GOLD(ch)) {
          send_to_char("You don't seem to have that much gold to split.\r\n", ch);
          return;
        } else {
          is_gold = 1;
        }
        break;
      case 'S':
      case 's':
        if (amount > GET_SILVER(ch)) {
          send_to_char("You don't seem to have that much silver to split.\r\n", ch);
          return;
        } else {
          is_silver = 1;
        }
        break;
      case 'C':
      case 'c':
        if (amount > GET_COPPER(ch)) {
          send_to_char("You don't seem to have that much copper to split.\r\n", ch);
          return;
        } else {
          is_copper = 1;
        }
        break;
    }
    k = (ch->master ? ch->master : ch);

    if (IS_AFFECTED(k, AFF_GROUP) && (k->in_room == ch->in_room)) {
      num = 1;
    } else {
      num = 0;
    }

    for (f = k->followers; f; f = f->next) {
      if (IS_AFFECTED(f->follower, AFF_GROUP) && (!IS_NPC(f->follower)) && (f->follower->in_room == ch->in_room)) {
        num++;
      }
    }

    if (num && IS_AFFECTED(ch, AFF_GROUP)) {
      share = amount / num;
    } else {
      send_to_char("With whom do you wish to share your money?\r\n", ch);
      return;
    }

    if (is_plat) {
      GET_PLAT(ch) -= share * (num - 1);
      GET_TEMP_GOLD(ch) -= share * (num - 1) * 1000;
    } else if (is_gold) {
      GET_GOLD(ch) -= share * (num - 1);
      GET_TEMP_GOLD(ch) -= share * (num - 1) * 100;
    } else if (is_silver) {
      GET_SILVER(ch) -= share * (num - 1);
      GET_TEMP_GOLD(ch) -= share * (num - 1) * 10;
    } else if (is_copper) {
      GET_COPPER(ch) -= share * (num - 1);
      GET_TEMP_GOLD(ch) -= share * (num - 1);
    }

    if (IS_AFFECTED(k, AFF_GROUP) && (k->in_room == ch->in_room) && !(IS_NPC(k)) && k != ch) {
      if (is_plat) {
        GET_PLAT(k) += share;
        GET_TEMP_GOLD(k) += share * 1000;
      } else if (is_gold) {
        GET_GOLD(k) += share;
        GET_TEMP_GOLD(k) += share * 100;
      } else if (is_silver) {
        GET_SILVER(k) += share;
        GET_TEMP_GOLD(k) += share * 10;
      } else if (is_copper) {
        GET_COPPER(k) += share;
        GET_TEMP_GOLD(k) += share;
      }
      sprintf(buf, "{w%s splits %s%d{x %s, you receive %s%d{w.{x\r\n", GET_NAME(ch), (is_plat ? "{W" : (is_gold ? "{Y" : (is_silver ? "{w" : "{y"))), amount, (is_plat ? "{Wplatinum {wcoins" : (is_gold ? "{Ygold {wcoins" : (is_silver ? "{wsilver coins" : "{ycopper {wcoins"))), (is_plat ? "{W" : (is_gold ? "{Y" : (is_silver ? "{w" : "{y"))), share);
      send_to_char(buf, k);
    }
    for (f = k->followers; f; f = f->next) {
      if (IS_AFFECTED(f->follower, AFF_GROUP) && (!IS_NPC(f->follower)) && (f->follower->in_room == ch->in_room) && f->follower != ch) {
        if (is_plat) {
          GET_PLAT(f->follower) += share;
          GET_TEMP_GOLD(f->follower) += share * 1000;
        } else if (is_gold) {
          GET_GOLD(f->follower) += share;
          GET_TEMP_GOLD(f->follower) += share * 100;
        } else if (is_silver) {
          GET_SILVER(f->follower) += share;
          GET_TEMP_GOLD(f->follower) += share * 10;
        } else if (is_copper) {
          GET_COPPER(f->follower) += share;
          GET_TEMP_GOLD(f->follower) += share;
        }
        sprintf(buf, "{w%s splits %s%d{x %s, you receive %s%d{w.{x\r\n", GET_NAME(ch), (is_plat ? "{W" : (is_gold ? "{Y" : (is_silver ? "{w" : "{y"))), amount, (is_plat ? "{Wplatinum {wcoins" : (is_gold ? "{Ygold {wcoins" : (is_silver ? "{wsilver coins" : "{ycopper {wcoins"))), (is_plat ? "{W" : (is_gold ? "{Y" : (is_silver ? "{w" : "{y"))), share);
        send_to_char(buf, f->follower);
      }
    }
    sprintf(buf, "{wYou split %s%d %s {wamong %d members -- %s%d %s {weach.{x\r\n", (is_plat ? "{W" : (is_gold ? "{Y" : (is_silver ? "{w" : "{y"))), amount, (is_plat ? "{Wplatinum {wcoins" : (is_gold ? "{Ygold {wcoins" : (is_silver ? "{wsilver coins" : "{ycopper {wcoins"))), num, (is_plat ? "{W" : (is_gold ? "{Y" : (is_silver ? "{w" : "{y"))), share, (is_plat ? "{Wplatinum {wcoins" : (is_gold ? "{Ygold {wcoins" : (is_silver ? "{wsilver coins" : "{ycopper {wcoins"))));
    send_to_char(buf, ch);
    if (*p) {
      do_split(ch, p, 0, 0);
    }
  } else {
    send_to_char("How many coins do you wish to split with your group?\r\n", ch);
    return;
  }
}

ACMD(do_afk)
{
  if (!IS_NPC(ch)) {
    if (PRF_FLAGGED(ch, PRF_AFK)) {
      send_to_char("Okay, you are nolonger afk.\r\n", ch);
    } else {
      send_to_char("Okay, you are now afk.\r\n", ch);
    }

    TOGGLE_BIT(PRF_FLAGS(ch), PRF_AFK);
    TOGGLE_BIT(GET_PROMPT(ch), PRM_AFK);
  } else {
    send_to_char("Go away you ugly monster.\r\n", ch);
  }
}

ACMD(do_use)
{
  struct actd_msg *actd;
  struct obj_data *mag_item;
  struct char_data *tch = NULL;
  struct obj_data *tobj = NULL;

  half_chop(argument, arg, buf);
  if (!*arg) {
    sprintf(buf2, "What do you want to %s?\r\n", CMD_NAME);
    send_to_char(buf2, ch);
    return;
  }
  mag_item = ch->equipment[WEAR_HOLD];

  if (!IS_NPC(ch) && GET_LEVEL(ch) == LVL_IMMORT && !PLR_FLAGGED(ch, PLR_UNRESTRICT)) {
    send_to_char("You cant figure out how to use that thing.\r\n", ch);
    return;
  }

  if (!mag_item || !isname(arg, mag_item->name)) {
    mag_item = ch->equipment[WEAR_HOLD_2];
    if (!mag_item || !isname(arg, mag_item->name)) {
      switch (subcmd) {
        case SCMD_RECITE:
        case SCMD_QUAFF:
          if (!(mag_item = get_obj_in_list_vis(ch, arg, ch->carrying))) {
            sprintf(buf2, "You don't seem to have %s %s.\r\n", AN(arg), arg);
            send_to_char(buf2, ch);
            return;
          }
          break;
        case SCMD_USE:
          sprintf(buf2, "You don't seem to be holding %s %s.\r\n", AN(arg), arg);
          send_to_char(buf2, ch);
          return;
        default:
          stderr_log("SYSERR: Unknown subcmd passed to do_use");
          return;
      }
    }
  }
  switch (subcmd) {
    case SCMD_QUAFF:
      if (GET_OBJ_TYPE(mag_item) != ITEM_POTION) {
        send_to_char("You can only quaff potions.", ch);
        return;
      }
      break;
    case SCMD_RECITE:
      if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL) {
        send_to_char("You can only recite scrolls.", ch);
        return;
      }
      break;
    case SCMD_USE:
      actd = get_actd(GET_OBJ_VNUM(mag_item));
      if (((GET_OBJ_TYPE(mag_item) != ITEM_WAND) && (GET_OBJ_TYPE(mag_item) != ITEM_STAFF) && actd == NULL) || GET_OBJ_TYPE(mag_item) == ITEM_FOOD || GET_OBJ_TYPE(mag_item) == ITEM_DRINKCON) {
        send_to_char("You can't seem to figure out how to use it.\r\n", ch);
        return;
      }
      if (actd && GET_OBJ_TYPE(mag_item) != ITEM_WAND && GET_OBJ_TYPE(mag_item) != ITEM_STAFF) {
        /* this one is for "real" objects only, magic actds in spell parser */
        one_argument(buf, arg);
        generic_find(arg, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &tch, &tobj);
        if (!*arg) { /* No argument */
          act(actd->char_no_arg, FALSE, ch, mag_item, 0, TO_CHAR);
          act(actd->others_no_arg, TRUE, ch, mag_item, 0, TO_ROOM);
          return;
        } else if (tch == NULL && tobj == NULL) { /* Target not found */
          act(actd->not_found, FALSE, ch, mag_item, 0, TO_CHAR);
          return;
        } else if (tch == ch) { /* target is player */
          act(actd->char_auto, FALSE, ch, mag_item, tch, TO_CHAR);
          act(actd->others_auto, TRUE, ch, mag_item, tch, TO_ROOM);
          return;
        } else if (tobj != NULL) { /* target is object */
          act(actd->char_object, FALSE, ch, mag_item, tobj, TO_CHAR);
          act(actd->others_object, TRUE, ch, mag_item, tobj, TO_ROOM);
          return;
        } else { /* target found, etc */
          act(actd->char_found, FALSE, ch, mag_item, tch, TO_CHAR);
          act(actd->others_found, TRUE, ch, mag_item, tch, TO_NOTVICT);
          act(actd->vict_found, FALSE, ch, mag_item, tch, TO_VICT);
          return;
        }
      }
      break;
  }

  mag_objectmagic(ch, mag_item, buf);
}

ACMD(do_wimpy)
{
  int wimp_lev;

  one_argument(argument, arg);

  if (!*arg) {
    if (GET_WIMP_LEV(ch)) {
      sprintf(buf, "Your current wimp level is %d hit points.\r\n", GET_WIMP_LEV(ch));
      send_to_char(buf, ch);
      return;
    } else {
      send_to_char("At the moment, you're not a wimp.  (sure, sure...)\r\n", ch);
      return;
    }
  }
  if (isdigit(*arg)) {
    if ((wimp_lev = atoi(arg))) {
      if (wimp_lev < 0) {
        send_to_char("Heh, heh, heh.. we are jolly funny today, eh?\r\n", ch);
      } else if (wimp_lev > GET_MAX_HIT(ch)) {
        send_to_char("That doesn't make much sense, now does it?\r\n", ch);
      } else if (wimp_lev > (GET_MAX_HIT(ch) >> 1)) {
        send_to_char("You can't set your wimp level above half your hit points.\r\n", ch);
      } else {
        sprintf(buf, "Okay, you'll wimp out if you drop below %d hit points.\r\n", wimp_lev);
        send_to_char(buf, ch);
        GET_WIMP_LEV(ch) = wimp_lev;
      }
    } else {
      send_to_char("Okay, you'll now tough out fights to the bitter end.\r\n", ch);
      GET_WIMP_LEV(ch) = 0;
    }
  } else {
    send_to_char("Specify at how many hit points you want to wimp out at.  (0 to disable)\r\n", ch);
  }

  return;
}

ACMD(do_display)
{
  send_to_char("Use the toggle command please.\r\n", ch);
}

ACMD(do_gen_write)
{
  FILE *fl;
  char *tmp, *filename;
  struct stat fbuf;
  extern int max_filesize;
  time_t ct;
  char buf[8 * MAX_STRING_LENGTH];

  switch (subcmd) {
    case SCMD_BUG:
      filename = BUG_FILE;
      break;
    case SCMD_TYPO:
      filename = TYPO_FILE;
      break;
    case SCMD_IDEA:
      filename = IDEA_FILE;
      break;
    case SCMD_TODO:
      filename = TODO_FILE;
      break;
    case SCMD_HELPN:
    case SCMD_WHELPN:
      filename = HELPN_FILE;
      break;
    default:
      return;
  }

  ct = time(0);
  tmp = asctime(localtime(&ct));

  if (IS_NPC(ch)) {
    send_to_char("Monsters can't have ideas - Go away.\r\n", ch);
    return;
  }

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument) {
    send_to_char("That must be a mistake...\r\n", ch);
    return;
  }
  if (strlen(argument) > MAX_STRING_LENGTH) {
    send_to_char("Ack! I'm afraid that is too much text!\r\n", ch);
    return;
  }
  if (subcmd == SCMD_WHELPN) {
    sprintf(buf, "%s wizhelp: %s", GET_NAME(ch), argument);
  } else {
    sprintf(buf, "%s %s: %s", GET_NAME(ch), CMD_NAME, argument);
  }
  mudlog(buf, 'M', COM_IMMORT, FALSE);

  if (stat(filename, &fbuf) < 0) {
    perror("Error stating file");
    return;
  }
  if (fbuf.st_size >= max_filesize) {
    send_to_char("Sorry, the file is full right now.. try again later.\r\n", ch);
    return;
  }
  if (!(fl = fopen(filename, "a"))) {
    perror("do_gen_write");
    send_to_char("Could not open the file.  Sorry.\r\n", ch);
    return;
  }
  fprintf(fl, "%-8s (%6.6s) [%5d] %s\n", GET_NAME(ch), (tmp + 4), world[ch->in_room].number, argument);
  fclose(fl);
  /*
   switch(subcmd) {
   case SCMD_TODO:
   file_to_string_alloc(TODO_FILE, &todolist);
   break;
   default:
   break;
   }
   */
  switch (subcmd) {
    case SCMD_BUG:
      do_reboot(ch, " bugs", 0, 0);
      break;
    case SCMD_TYPO:
      do_reboot(ch, " typos", 0, 0);
      break;
    case SCMD_IDEA:
      do_reboot(ch, " ideas", 0, 0);
      break;
    case SCMD_TODO:
      do_reboot(ch, " todo", 0, 0);
      break;
    case SCMD_HELPN:
    case SCMD_WHELPN:
      do_reboot(ch, " helplist", 0, 0);
      break;
    default:
      return;
  }

}

#define TOG_OFF 0
#define TOG_ON  1

#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF_FLAGS(ch), (flag))) & (flag))

ACMD(do_gen_tog)
{
  long result;
  extern int nameserver_is_slow;

  char *tog_messages[][2] = {
      {"This was for summon protect... has been removed.\r\n", "This was for summon protect... has been removed.\r\n"},
      {"Nohassle disabled.\r\n", "Nohassle enabled.\r\n"},
      {"Brief mode level 1 off.\r\n", "Brief mode level 1 on.\r\n"},
      {"Compact mode off.\r\n", "Compact mode on.\r\n"},
      {"You can now hear tells.\r\n", "You are now deaf to tells.\r\n"},
      {"You can now hear auctions.\r\n", "You are now deaf to auctions.\r\n"},
      {"You can now hear shouts.\r\n", "You are now deaf to shouts.\r\n"},
      {"You can now hear chat...\r\n{RThis channel is for Non-Mud Related Topics ONLY!{x\r\n", "You are now deaf to chat.\r\n"},
      {"You can now hear the congratulation messages.\r\n", "You are now deaf to the congratulation messages.\r\n"},
      {"You can now hear the Wiz-channel.\r\n", "You are now deaf to the Wiz-channel.\r\n"},
      {"You are no longer part of the Quest.\r\n", "Okay, you are part of the Quest!\r\n"},
      {"You will no longer see the room flags.\r\n", "You will now see the room flags.\r\n"},
      {"You will now have your communication repeated.\r\n", "You will no longer have your communication repeated.\r\n"},
      {"HolyLight mode off.\r\n", "HolyLight mode on.\r\n"},
      {"Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n", "Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"},
      {"Autoexits disabled.\r\n", "Autoexits enabled.\r\n"},
      {"WHOIS information is no longer visible.\r\n", "WHOIS information is now visible.\r\n"},
      {"The ticker has now been turned off.\r\n", "The ticker has now been turned on.\r\n"},
      {"Your citizen status is now shown in the who list.\r\n", "Your citizen status is no longer shown in the who list.\r\n"},
      {"You are no longer auto-looting.\r\n", "You are now auto-looting.\r\n"},
      {"You are no longer auto-looting gold.\r\n", "You are now auto-looting gold.\r\n"},
      {"You are no longer auto-splitting gold.\r\n", "You are now auto-splitting gold.\r\n"},
      {"You are no longer anonymous.\r\n", "You are now anonymous.\r\n"},
      {"IDENT set to NO, username lookups no longer attempted.\r\n", "IDENT set to YES, username lookups will be attempted.\r\n"},
      {"You can now hear the immquest channel.\r\n", "You are now deaf to the immquest channel.\r\n"},
      {"You can now hear mob shouts.\r\n", "You are now deaf to mob shouts.\r\n"},
      {"Brief mode level 2 off.\r\n", "Brief mode level 2 on.\r\n"},
      {"Color is now off.\r\n", "Color is now on.\r\n"}
  };

  if (IS_NPC(ch)) {
    return;
  }

  one_argument(argument, arg);
  switch (subcmd) {
    case SCMD_NOHASSLE:
      result = PRF_TOG_CHK(ch, PRF_NOHASSLE);
      break;
    case SCMD_BRIEF:
      if (!*arg || *arg == '1') {
        result = PRF_TOG_CHK(ch, PRF_BRIEF);
      } else {
        subcmd = SCMD_BRIEF2;
        result = PRF_TOG_CHK(ch, PRF_BRIEF2);
      }
      break;
    case SCMD_COMPACT:
      result = PRF_TOG_CHK(ch, PRF_COMPACT);
      break;
    case SCMD_NOTELL:
      result = PRF_TOG_CHK(ch, PRF_NOTELL);
      break;
    case SCMD_NOAUCTION:
      result = PRF_TOG_CHK(ch, PRF_NOAUCT);
      break;
    case SCMD_DEAF:
      result = PRF_TOG_CHK(ch, PRF_DEAF);
      break;
    case SCMD_NOCHAT:
      result = PRF_TOG_CHK(ch, PRF_NOCHAT);
      break;
    case SCMD_NOGRATZ:
      result = PRF_TOG_CHK(ch, PRF_NOGRATZ);
      break;
    case SCMD_NOWIZ:
      result = PRF_TOG_CHK(ch, PRF_NOWIZ);
      break;
    case SCMD_ROOMFLAGS:
      result = PRF_TOG_CHK(ch, PRF_ROOMFLAGS);
      break;
    case SCMD_NOREPEAT:
      result = PRF_TOG_CHK(ch, PRF_NOREPEAT);
      break;
    case SCMD_HOLYLIGHT:
      result = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
      break;
    case SCMD_SLOWNS:
      result = (nameserver_is_slow = !nameserver_is_slow);
      break;
    case SCMD_AUTOEXIT:
      result = PRF_TOG_CHK(ch, PRF_AUTOEXIT);
      break;
    case SCMD_NOWHOIS:
      result = PRF_TOG_CHK(ch, PRF_WHOIS);
      break;
    case SCMD_TICKER:
      result = PRF_TOG_CHK(ch, PRF_TICKER);
      break;
    case SCMD_CITIZEN:
      result = PRF_TOG_CHK(ch, PRF_NOCITIZEN);
      break;
    case SCMD_AUTOLOOT:
      result = PRF_TOG_CHK(ch, PRF_AUTOLOOT);
      break;
    case SCMD_AUTOGOLD:
      result = PRF_TOG_CHK(ch, PRF_AUTOGOLD);
      break;
    case SCMD_AUTOSPLIT:
      result = PRF_TOG_CHK(ch, PRF_AUTOSPLIT);
      break;
    case SCMD_ANONYMOUS:
      result = PRF_TOG_CHK(ch, PRF_ANONYMOUS);
      break;
    case SCMD_IDENT:
      result = (ident = !ident);
      break;
    case SCMD_IMMQUEST:
      result = PRF_TOG_CHK(ch, PRF_NOIMMQUEST);
      break;
    case SCMD_MOBDEAF:
      result = PRF_TOG_CHK(ch, PRF_MOBDEAF);
      break;
    case SCMD_COLOR:
      result = PRF_TOG_CHK(ch, PRF_COLOR_2);
      break;
    default:
      stderr_log("SYSERR: Unknown subcmd in do_gen_toggle");
      return;
  }

  if (result) {
    send_to_char(tog_messages[subcmd][TOG_ON], ch);
  } else {
    send_to_char(tog_messages[subcmd][TOG_OFF], ch);
  }

  return;
}

