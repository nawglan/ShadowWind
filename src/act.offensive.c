/* ************************************************************************
 *   File: act.offensive.c                               Part of CircleMUD *
 *  Usage: player-level commands of an offensive nature                    *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#include <stdio.h>
#include <string.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "event.h"
#include "spells.h"

/* extern variables */
extern struct zone_data *zone_table;
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct room_data *world;
extern int pk_allowed;
extern struct spell_info_type *spells;
extern int MaxExperience[LVL_IMMORT + 2];

/* extern functions */
void raw_kill(struct char_data * ch, struct char_data * killer);

int check_state(struct char_data * ch)
{
  if (ch->char_specials.fightwait > 0) {
    return 1;
  }
  return 0;
}

void fight_wait(struct char_data * ch, int waittime)
{
  ch->char_specials.fight_timer = waittime;
}

ACMD(do_assist)
{
  struct char_data *helpee, *opponent;

  if (FIGHTING(ch)) {
    send_to_char("You're already fighting!  How can you assist someone else?\r\n", ch);
    return;
  }
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Whom do you wish to assist?\r\n", ch);
  } else if (!(helpee = get_char_room_vis(ch, arg))) {
    send_to_char(NOPERSON, ch);
  } else if (helpee == ch) {
    send_to_char("You can't help yourself any more than this!\r\n", ch);
  } else {
    /*
     for (opponent = world[ch->in_room].people; opponent &&
     (FIGHTING(opponent) != helpee); opponent = opponent->next_in_room);
     */
    opponent = FIGHTING(helpee);

    if (!opponent) {
      act("But nobody is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
    } else if (!CAN_SEE(ch, opponent)) {
      act("You can't see who is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
    } else if (!pk_allowed && !IS_NPC(opponent)) { /* prevent accidental pkill */ 
      act("Use 'murder' if you really want to attack $N.", FALSE, ch, 0, opponent, TO_CHAR);
    } else {
      send_to_char("You join the fight!\r\n", ch);
      act("$N assists you!", 0, helpee, 0, ch, TO_CHAR);
      act("$n assists $N.", FALSE, ch, 0, helpee, TO_NOTVICT);
      hit(ch, opponent, TYPE_UNDEFINED);
    }
  }
}

ACMD(do_hit)
{
  struct char_data *vict;

  one_argument(argument, arg);
  vict = get_char_room_vis(ch, arg);

  if (!*arg) {
    send_to_char("Hit who?\r\n", ch);
  } else if (!vict) {
    send_to_char("They don't seem to be here.\r\n", ch);
  } else if (vict == ch) {
    send_to_char("You hit yourself...OUCH!.\r\n", ch);
    act("$n hits $mself, and says OUCH!", FALSE, ch, 0, vict, TO_ROOM);
  } else if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master == vict)) {
    act("$N is just such a good friend, you simply can't hit $M.", FALSE, ch, 0, vict, TO_CHAR);
  } else {
    if (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_NOKILL)) {
      send_to_char("That's not a good idea.\r\n", ch);
      return;
    }
    if (!pk_allowed) {
      if (!IS_NPC(vict) && !IS_NPC(ch) && (subcmd != SCMD_MURDER)) {
        send_to_char("Use 'murder' to hit another player.\r\n", ch);
        return;
      }
      if (IS_AFFECTED(ch, AFF_CHARM) && !IS_NPC(ch->master) && !IS_NPC(vict)) {
        return; /* you can't order a charmed pet to attack a player */
      }
    }
    if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
      send_to_char("You feel ashamed disturbing the tranquility of this place\r\n", ch);
      return;
    }
    if ((GET_POS(ch) == POS_STANDING) && (vict != FIGHTING(ch))) {
      hit(ch, vict, TYPE_UNDEFINED);
      WAIT_STATE(ch, 20);
    } else {
      send_to_char("You do the best you can!\r\n", ch);
    }
  }
}

ACMD(do_kill)
{
  struct char_data *vict;
  struct mob_attacks_data *this;

  if ((!COM_FLAGGED(ch, COM_ADMIN)) || IS_NPC(ch)) {
    if (IS_NPC(ch)) {
      this = ch->mob_specials.mob_attacks;
      this->fight_timer = 0;
    }
    do_hit(ch, argument, cmd, subcmd);
    return;
  }
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Kill who?\r\n", ch);
  } else {
    if (!(vict = get_char_room_vis(ch, arg))) {
      send_to_char("They arne't here.\r\n", ch);
    } else if (ch == vict) {
      send_to_char("Your mother would be so sad.. :(\r\n", ch);
    } else {
      act("{DYou chop {w$M {Dto pieces!  Ah!  The {rblood{D!{x", FALSE, ch, 0, vict, TO_CHAR);
      act("\r\n{D$N {Dchops you to {rpieces{D.{x\r\n{DYou are dead. {rR{D.{rI{D.{rP{D.{x\r\n{x",

      FALSE, vict, 0, ch, TO_CHAR);
      act("\r\n{D$n {Dchops {w$N {Dto pieces!  Ah!  The {rblood{D!{x",

      FALSE, ch, 0, vict, TO_NOTVICT);
      sprintf(buf, "%s rawkilled by %s", GET_NAME(vict), GET_NAME(ch));
      mudlog(buf, 'X', COM_ADMIN, TRUE);
      plog(buf, ch, LVL_IMMORT);
      raw_kill(vict, ch);
    }
  }
}

ACMD(do_order)
{
  char name[100], message[256];
  char buf[256];
  bool found = FALSE;
  int org_room;
  struct char_data *vict;
  struct follow_type *k;

  half_chop(argument, name, message);

  if (!*name || !*message) {
    send_to_char("Order who to do what?\r\n", ch);
  } else if (!(vict = get_char_room_vis(ch, name)) && !is_abbrev(name, "followers")) {
    send_to_char("That person isn't here.\r\n", ch);
  } else if (ch == vict) {
    send_to_char("You obviously suffer from skitzofrenia.\r\n", ch);
  } else {
    if (IS_AFFECTED(ch, AFF_CHARM)) {
      send_to_char("Your superior would not aprove of you giving orders.\r\n", ch);
      return;
    }
    if (vict) {
      sprintf(buf, "$N orders you to '%s'", message);
      act(buf, FALSE, vict, 0, ch, TO_CHAR);
      /* act("$n gives $N an order.", FALSE, ch, 0, vict, TO_ROOM);*/

      if ((vict->master != ch) || !IS_AFFECTED(vict, AFF_CHARM)) {
        act("$n has an indifferent look.", FALSE, vict, 0, 0, TO_ROOM);
      } else {
        sprintf(buf, "You order $N to '%s'", message);
        act(buf, FALSE, ch, 0, vict, TO_CHAR);
        command_interpreter(vict, message);
      }
    } else { /* This is order "followers" */
      /*act("$n gives $s followers an order.", FALSE, ch, 0, 0, TO_ROOM);*/

      org_room = ch->in_room;

      for (k = ch->followers; k; k = k->next) {
        if (org_room == k->follower->in_room) {
          if (IS_AFFECTED(k->follower, AFF_CHARM)) {
            found = TRUE;
            sprintf(buf, "You order your followers '%s'", message);
            act(buf, FALSE, ch, 0, vict, TO_CHAR);
            command_interpreter(k->follower, message);
          }
        }
      }
      if (!found) {
        send_to_char("Nobody here is a loyal subject of yours!\r\n", ch);
      }
    }
  }
}

ACMD(do_flee)
{
  int i, attempt;
  /* not needed.
   int loss;
   */
  extern char *dirs[];
  struct char_data *tch;

  if (CHECK_FIGHT(ch) && FIGHTING(ch)) {
    send_to_char("You are too engaged in combat to flee now!\r\n", ch);
    return;
  }

  for (i = 0; i < 6; i++) {
    attempt = number(0, NUM_OF_DIRS - 1); /* Select a random direction */
    if (CAN_GO(ch, attempt) && !IS_SET(ROOM_FLAGS(EXIT(ch, attempt)->to_room), ROOM_DEATH)) {
      act("$n panics, and attempts to flee!", TRUE, ch, 0, 0, TO_ROOM);
      if (do_simple_move(ch, attempt, TRUE)) {
        sprintf(buf, "You flee head over heels, heading %s.\r\n", dirs[attempt]);
        send_to_char(buf, ch);
        if (FIGHTING(ch)) {
          /* Disabled by Novo....
           if (!IS_NPC(ch)) {
           loss = GET_MAX_HIT(FIGHTING(ch)) - GET_HIT(FIGHTING(ch));
           loss *= (GET_LEVEL(FIGHTING(ch)) - GET_LEVEL(ch));
           if (loss < 0) loss = -loss;
           loss = MIN(loss, (MaxExperience[(int)GET_LEVEL(ch)+1] - MaxExperience[(int)GET_LEVEL(ch)]) / 2);
           gain_exp(ch, -loss);
           gain_exp(FIGHTING(ch), loss);
           }
           */
          for (tch = world[FIGHTING(ch)->in_room].people; tch; tch = tch->next_in_room) {
            if (FIGHTING(tch) == ch) {
              stop_fighting(tch);
            }
          }
          stop_fighting(ch);
        }
      } else {
        act("$n tries to flee, but can't!", TRUE, ch, 0, 0, TO_ROOM);
      }
      return;
    }
  }
  send_to_char("PANIC!  You couldn't escape!\r\n", ch);
}

ACMD(do_disengage)
{
  if (FIGHTING(ch)) {
    send_to_char("You stop fighting.\r\n", ch);
    act("$n stops fighting.", TRUE, ch, 0, 0, TO_ROOM);
    stop_fighting(ch);
    if (IS_NPC(ch)) {
      GET_MOB_WAIT(ch) = 2;
    } else {
      WAIT_STATE(ch, 2*PULSE_VIOLENCE);
    }
  } else {
    send_to_char("You're not fighting!\r\n", ch);
  }
}

