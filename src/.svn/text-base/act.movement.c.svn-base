/* ************************************************************************
 *   File: act.movement.c                                Part of CircleMUD *
 *  Usage: movement commands, door handling, & sleep/rest/etc state        *
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

/* external vars  */
extern struct room_data *world;
extern struct zone_data *zone_table;
extern struct index_data *obj_index;
extern int rev_dir[];
extern char *dirs[];
extern int movement_loss[];
extern char *arrived_from[];
extern struct obj_data *object_list;

/* external functs */
void death_cry(struct char_data * ch);
void mprog_greet_trigger(struct char_data * ch);
void mprog_entry_trigger(struct char_data * mob);
#define MOB_AGGR_TO_ALIGN MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL | MOB_AGGR_GOOD

/* do_simple_move assumes
 *  1. That there is no master and no followers.
 *  2. That the direction exists.
 *
 *   Returns :
 *   1 : If succes.
 *   0 : If fail
 */
int do_simple_move(struct char_data * ch, int dir, int need_specials_check)
{
  int was_in, need_movement, has_boat = 0, found, i;
  struct char_data *vict, *fakech, *ch_list;
  struct obj_data *obj;
  extern struct char_data *character_list;

  int special(struct char_data * ch, int cmd, char *arg);

  /*
   * Check for special routines (North is 1 in command list, but 0 here) Note
   * -- only check if following; this avoids 'double spec-proc' bug
   */
  if (need_specials_check && special(ch, dir + 1, ""))
    return 0;

  if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_PRIVATE)) {
    if (world[EXIT(ch, dir)->to_room].people && world[EXIT(ch, dir)->to_room].people->next_in_room)
      if (!COM_FLAGGED(ch, COM_ADMIN)) {
        send_to_char("There is a private conversation in that room.\r\n", ch);
        return 0;
      }
  }
  if (ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_GODROOM)) {
    for (ch_list = character_list; ch_list; ch_list = ch_list->next)
      if (IN_ROOM(ch_list) == EXIT(ch, dir)->to_room && !IS_NPC(ch_list)) {
        if (!COM_FLAGGED(ch, COM_ADMIN)) {
          send_to_char("That room is restricted.\r\n", ch);
          return 0;
        }
      }
  }
  if (!IS_NPC(ch) && ROOM_FLAGGED(EXIT(ch, dir)->to_room, ROOM_TUNNEL)) {
    /* check to see if another PC is in the room... */
    for (ch_list = character_list; ch_list; ch_list = ch_list->next)
      if (IN_ROOM(ch_list) == EXIT(ch, dir)->to_room && !IS_NPC(ch_list)) {
        if (GET_LEVEL(ch) < LVL_IMMORT) {
          send_to_char("It is a little too crowded in that direction.\r\n", ch);
          return 0;
        }
      }
  }
  /* if this room or the one we're going to needs a boat, check for one */

  if ((world[ch->in_room].sector_type == SECT_WATER_NOSWIM) || (world[EXIT(ch, dir)->to_room].sector_type == SECT_WATER_NOSWIM)) {
    if (AFF2_FLAGGED(ch, AFF2_WRAITHFORM))
      has_boat = TRUE;
    for (obj = ch->carrying; obj; obj = obj->next_content)
      if (GET_OBJ_TYPE(obj) == ITEM_BOAT)
        has_boat = TRUE;
    if (!IS_AFFECTED(ch,AFF_WATERWALK) && !has_boat && !IS_AFFECTED(ch,AFF_FLY) && (GET_LEVEL(ch) < LVL_IMMORT)) {
      send_to_char("You need a boat to go there.\r\n", ch);
      return 0;
    }
  } else if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_WATERONLY)) {
    send_to_char("You should not move out of the water.\r\n", ch);
    return 0;
  }

  need_movement = ((movement_loss[IN_SECTOR(IN_ROOM(ch))] + movement_loss[IN_SECTOR(EXIT(ch, dir)->to_room)]) >> 1) + EXIT(ch, dir)->add_move;
  if (IS_AFFECTED(ch,AFF_FLY) || MOUNTING(ch))
    need_movement >>= 1;
  if (COM_FLAGGED(ch, COM_IMMORT))
    need_movement = 0;
  if (GET_WINDSPEED(IN_ZONE(ch)) > 70 && GET_WINDDIR(IN_ZONE(ch)) == dir) {
    need_movement += GET_WINDSPEED(IN_ZONE(ch)) / 10;
    send_to_char("The wind requires great force to move against it.\r\n", ch);
  }
  if (GET_WINDSPEED(IN_ZONE(ch)) > 70 && GET_WINDDIR(IN_ZONE(ch)) == rev_dir[dir]) {
    need_movement -= GET_WINDSPEED(IN_ZONE(ch)) / 10;
    send_to_char("The wind at your back eases your movement.\r\n", ch);
    if (need_movement < 1)
      need_movement = 1;
  }

  if (AFF2_FLAGGED(ch, AFF2_WRAITHFORM))
    need_movement = 0;

  if (GET_MOVE(ch) < need_movement && !IS_NPC(ch)) {
    if (need_specials_check && ch->master)
      send_to_char("You are too exhausted to follow.\r\n", ch);
    else
      send_to_char("You are too exhausted.\r\n", ch);

    return 0;
  }

  if (!check_access(ch, EXIT(ch, dir)->to_room)) {
    send_to_char("That zone is restricted, go away!\r\n", ch);
    return 0;
  }

  if (GET_LEVEL(ch) < LVL_IMMORT && !IS_NPC(ch)) {
    GET_MOVE(ch) -= need_movement;
    /*  WAIT_STATE(ch, (need_movement-1)*4); */
  }

  if (!IS_AFFECTED(ch, AFF_SNEAK) && !IS_AFFECTED(ch,AFF_FLY) && !MOUNTING(ch) && !MOUNTED_BY(ch)) {
    sprintf(buf2, "$n walks %s.", dirs[dir]);
    act(buf2, TRUE, ch, 0, 0, TO_ROOM);
  }

  /* If player is flying, show different message */
  if (!IS_AFFECTED(ch, AFF_SNEAK) && IS_AFFECTED(ch,AFF_FLY) && !MOUNTING(ch) && !MOUNTED_BY(ch)) {
    sprintf(buf2, "$n flies %s.", dirs[dir]);
    act(buf2, TRUE, ch, 0, 0, TO_ROOM);
  }

  if (MOUNTING(ch) && !IS_AFFECTED(MOUNTING(ch), AFF_SNEAK)) {
    sprintf(buf2, "$n rides %s.", dirs[dir]);
    act(buf2, TRUE, ch, 0, 0, TO_ROOM);
  }

  was_in = ch->in_room;
  char_from_room(ch);
  char_to_room(ch, world[was_in].dir_option[dir]->to_room);

  if (!IS_AFFECTED(ch, AFF_SNEAK) && !IS_AFFECTED(ch, AFF_FLY) && !MOUNTING(ch) && !MOUNTED_BY(ch)) {
    sprintf(buf, "$n walks in from ");
    sprinttype(dir, arrived_from, buf2);
    strcat(buf, buf2);
    act(buf, TRUE, ch, 0, 0, TO_ROOM);
  } else if (!IS_AFFECTED(ch, AFF_SNEAK) && IS_AFFECTED(ch, AFF_FLY) && !MOUNTING(ch) && !MOUNTED_BY(ch)) {
    sprintf(buf, "$n flies in from ");
    sprinttype(dir, arrived_from, buf2);
    strcat(buf, buf2);
    act(buf, TRUE, ch, 0, 0, TO_ROOM);
  } else if (MOUNTING(ch) && !IS_AFFECTED(MOUNTING(ch), AFF_SNEAK)) {
    sprintf(buf, "$n rides in from ");
    sprinttype(dir, arrived_from, buf2);
    strcat(buf, buf2);
    act(buf, TRUE, ch, 0, 0, TO_ROOM);
  }

  if (PRF_FLAGGED(ch, PRF_BRIEF))
    look_at_room(ch, 0);
  else
    look_at_room(ch, 1);
  if (!AFF2_FLAGGED(ch, AFF2_WRAITHFORM) && ROOM_FLAGGED(ch->in_room, ROOM_UNAFFECT)) {
    if (ch->affected) {
      while (ch->affected)
        affect_remove(ch, ch->affected);
      send_to_char("There is a brief flash of light\r\n"
          "You feel slightly different.\r\n", ch);
    }
  }

  if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_DEATH) && GET_LEVEL(ch) < LVL_IMMORT) {
    log_death_trap(ch);
    death_cry(ch);
    extract_char(ch, 1);
    return 0;
  }
  found = FALSE;

  /* Aggressive on entry */

  if (!AFF2_FLAGGED(ch, AFF2_WRAITHFORM) && !IS_NPC(ch) && !IS_AFFECTED(ch, AFF_SNEAK)) {
    /* add !found to fakech in first loop to have intelligen attacking */
    for (fakech = world[ch->in_room].people; fakech; fakech = fakech->next_in_room) {
      if (IS_NPC(fakech) && MOB_FLAGGED(fakech, MOB_AGGRESSIVE | MOB_AGGR_TO_ALIGN) && AWAKE(fakech) && !FIGHTING(fakech) && !HUNTING(fakech)) {
        for (vict = world[fakech->in_room].people; vict && !found; vict = vict->next_in_room) {
          if (!CAN_SEE(fakech, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE))
            continue;
          if (IS_AFFECTED(vict, AFF_CHARM) && IS_NPC(vict->master))
            continue;
          if (IS_AFFECTED(vict, AFF_PROTECT_EVIL) && IS_EVIL(fakech))
            continue;
          if (MOB_FLAGGED(fakech, MOB_WIMPY) && AWAKE(vict))
            continue;
          if (IS_NPC(vict) && !IS_AFFECTED(vict, AFF_CHARM))
            continue;
          if (!MOB_FLAGGED(fakech, MOB_AGGR_TO_ALIGN) || (MOB_FLAGGED(fakech, MOB_AGGR_EVIL) && IS_EVIL(vict)) || (MOB_FLAGGED(fakech, MOB_AGGR_NEUTRAL) && IS_NEUTRAL(vict)) || (MOB_FLAGGED(fakech, MOB_AGGR_GOOD) && IS_GOOD(vict))) {
            for (i = 0; (i < 10); i++) {
              if (vict->player_specials->hunters[i] == NULL && vict->player_specials->hunters[i] != fakech) {
                vict->player_specials->hunters[i] = fakech;
                vict->player_specials->huntstate[i] = 0; /* don't give the player some free time to flee */
                break;
              }
            }
            HUNTING(fakech) = GET_IDNUM(vict);
            act("$N charges towards you!", TRUE, ch, 0, fakech, TO_CHAR);
            act("$N charges towards $n.", TRUE, ch, 0, fakech, TO_ROOM);
            found = TRUE;
          }
        }
      }
    }
  }
  mprog_entry_trigger(ch);
  mprog_greet_trigger(ch);
  return 1;
}

int perform_move(struct char_data * ch, int dir, int need_specials_check)
{
  int was_in;
  struct follow_type *k, *next;

  if (ch == NULL || dir < 0 || dir >= NUM_OF_DIRS)
    return 0;
  else if (!EXIT(ch, dir) || EXIT(ch, dir)->to_room == NOWHERE || (!(AFF2_FLAGGED(ch, AFF2_WRAITHFORM) && !IS_SET(EXIT(ch, dir)->exit_info, EX_HIDDEN)) && IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED))) {
    switch (number(0, 6)) {
      case 0:
        send_to_char("Alas, you cannot go that way...\r\n", ch);
        break;
      case 1:
        send_to_char("Your path is obstructed.\r\n", ch);
        break;
      case 2:
        send_to_char("Excuse me, you cant go there!!.\r\n", ch);
        break;
      case 3:
        send_to_char("Try as you might, the gods refuse to let you go that way.\r\n", ch);
        break;
      case 4:
        send_to_char("OOMPH! Was that a wall?\r\n", ch);
        break;
      case 5:
        send_to_char("After further analysis, you decide that there is no exit in that direction.\r\n", ch);
        break;
      default:
        send_to_char("You cannot walk through walls... yet!\r\n", ch);
        break;
    }
  } else {

    if (!ch->followers && !MOUNTING(ch)) {
      return (do_simple_move(ch, dir, need_specials_check));
    }

    was_in = ch->in_room;
    if (!do_simple_move(ch, dir, need_specials_check))
      return 0;

    if (MOUNTING(ch))
      perform_move(MOUNTING(ch), dir, 1);

    for (k = ch->followers; k; k = next) {
      next = k->next;
      if (CAN_SEE(k->follower, ch) && (was_in == k->follower->in_room) && (GET_POS(k->follower) >= POS_STANDING) && !AFF2_FLAGGED(k->follower, AFF2_CASTING)) {
        act("You follow $N.\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
        perform_move(k->follower, dir, 1);
      } else if (CAN_SEE(k->follower, ch) && (was_in == k->follower->in_room)) {
        switch (GET_POS(k->follower)) {
          case POS_SITTING:
            act("You cannot follow $N while sitting.\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
            break;
          case POS_RESTING:
            act("You are too comfortable to follow $N.\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
            break;
          case POS_FIGHTING:
            act("You are too engaged in combat to follow $N.\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
            break;
          default:
            act("You are unable to follow $N.\r\n", FALSE, k->follower, 0, ch, TO_CHAR);
            break;
        }
      }
    }
    return 1;
  }
  return 0;
}

ACMD(do_move)
{
  extern sh_int stats[11][101];
  /*
   * This is basically a mapping of cmd numbers to perform_move indexes.
   * It cannot be done in perform_move because perform_move is called
   * by other functions which do not require the remapping.
   */
  if (MOUNTED_BY(ch))
    return;
  if (GET_DRAGGING(ch)) {
    if (perform_move(ch, cmd - 1, 0)) {
      sprintf(buf, "%s drags $p along.", GET_NAME(ch));
      act(buf, TRUE, 0, GET_DRAGGING(ch), 0, TO_ROOM);

      obj_from_room(GET_DRAGGING(ch));
      obj_to_room(GET_DRAGGING(ch), ch->in_room);

      sprintf(buf, "You drag $p along with you.");
      act(buf, FALSE, ch, GET_DRAGGING(ch), 0, TO_CHAR);
      sprintf(buf, "%s drags $p along with $m.", GET_NAME(ch));
      act(buf, TRUE, ch, GET_DRAGGING(ch), 0, TO_ROOM);
    } else
      GET_MOVE(ch) -= ((GET_OBJ_WEIGHT(GET_DRAGGING(ch)) * 15) / CAN_CARRY_W(ch));
  } else
    perform_move(ch, cmd - 1, 0);
}

int find_door(struct char_data * ch, char *type, char *dir)
{
  int door;

  if (*dir) { /* a direction was specified */
    if ((door = search_block(dir, dirs, FALSE)) == -1) { /* Partial Match */
      send_to_char("That's not a direction.\r\n", ch);
      return -1;
    }
    if (EXIT(ch, door) && EXIT(ch, door)->keyword && isname(type, EXIT(ch, door)->keyword) && !IS_SET(EXIT(ch, door)->exit_info, EX_HIDDEN))
      return door;
    else if (EXIT(ch, door) && EXIT(ch, door)->keyword) {
      sprintf(buf2, "I see no %s there.\r\n", type);
      send_to_char(buf2, ch);
      return -1;
    } else if (EXIT(ch, door) && !IS_SET(EXIT(ch, door)->exit_info, EX_HIDDEN))
      return door;
    else {
      send_to_char("I really don't see how you can close anything there.\r\n", ch);
      return -1;
    }
  } else { /* try to locate the keyword */
    for (door = 0; door < NUM_OF_DIRS; door++)
      if (EXIT(ch, door) && EXIT(ch, door)->keyword && !IS_SET(EXIT(ch, door)->exit_info, EX_HIDDEN) && isname(type, EXIT(ch, door)->keyword))
        return door;

    sprintf(buf2, "There doesn't seem to be %s %s here.\r\n", AN(type), type);
    send_to_char(buf2, ch);
  }
  return -1;
}

ACMD(do_open)
{
  int door, other_room;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct room_direction_data *back;
  struct obj_data *obj;
  struct char_data *victim;

  two_arguments(argument, type, dir);

  if (!*type) {
    send_to_char("Open what?\r\n", ch);
  } else if (generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj)) {
    /* this is an object */
    if (GET_OBJ_TYPE(obj) != ITEM_CONTAINER) {
      send_to_char("That's not a container.\r\n", ch);
    } else if (!IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) {
      send_to_char("But it's already open!\r\n", ch);
    } else if (!IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSEABLE)) {
      send_to_char("You can't do that.\r\n", ch);
    } else if (IS_SET(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) {
      send_to_char("It seems to be locked.\r\n", ch);
    } else {
      REMOVE_BIT(GET_OBJ_VAL(obj, 1), CONT_CLOSED);
      act("You open $p.", FALSE, ch, obj, 0, TO_CHAR);
      act("$n opens $p.", FALSE, ch, obj, 0, TO_ROOM);
    }
  } else if ((door = find_door(ch, type, dir)) >= 0) {
    /* perhaps it is a door */
    if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR)) {
      send_to_char("That's impossible, I'm afraid.\r\n", ch);
    } else if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)) {
      send_to_char("It's already open!\r\n", ch);
    } else if (IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED)) {
      send_to_char("It seems to be locked.\r\n", ch);
    } else {
      REMOVE_BIT(EXIT(ch, door)->exit_info, EX_CLOSED);
      if (EXIT(ch, door)->keyword) {
        act("$n opens the $F.", FALSE, ch, 0, EXIT(ch, door)->keyword, TO_ROOM);
      } else {
        act("$n opens the door.", FALSE, ch, 0, 0, TO_ROOM);
      }
      act("You open the $F.", FALSE, ch, 0, EXIT(ch, door)->keyword, TO_CHAR);
      /* now for opening the OTHER side of the door! */
      if ((other_room = EXIT(ch, door)->to_room) != NOWHERE) {
        if ((back = world[other_room].dir_option[rev_dir[door]])) {
          if (back->to_room == ch->in_room) {
            REMOVE_BIT(back->exit_info, EX_CLOSED);
            if (back->keyword) {
              sprintf(buf, "The %s is opened from the other side.", fname(back->keyword));
              /* send_to_room(buf, EXIT(ch, door)->to_room); */
              act(buf, 0, ch, NULL, (void*) EXIT(ch, door)->to_room, TO_EXIT);
            } else {
              /* send_to_room("The door is opened from the other side.\r\n", EXIT(ch, door)->to_room); */
              act("The door is opened from the other side.", 0, ch, NULL, (void*) EXIT(ch, door)->to_room, TO_EXIT);
            }
          }
        }
      }
    }
  }
}

ACMD(do_close)
{
  int door, other_room;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct room_direction_data *back;
  struct obj_data *obj;
  struct char_data *victim;

  two_arguments(argument, type, dir);

  if (!*type)
    send_to_char("Close what?\r\n", ch);
  else if (generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj)) {
    /* this is an object */
    if (GET_OBJ_TYPE(obj) != ITEM_CONTAINER) {
      send_to_char("That's not a container.\r\n", ch);
    } else if (IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) {
      send_to_char("But it's already closed!\r\n", ch);
    } else if (!IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSEABLE)) {
      send_to_char("That's impossible.\r\n", ch);
    } else {
      SET_BIT(GET_OBJ_VAL(obj, 1), CONT_CLOSED);
      act("You close $p.", FALSE, ch, obj, 0, TO_CHAR);
      act("$n closes $p.", FALSE, ch, obj, 0, TO_ROOM);
    }
  } else if ((door = find_door(ch, type, dir)) >= 0) {
    /* Or a door */
    if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR)) {
      send_to_char("That's absurd.\r\n", ch);
    } else if (IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)) {
      send_to_char("It's already closed!\r\n", ch);
    } else {
      SET_BIT(EXIT(ch, door)->exit_info, EX_CLOSED);
      if (EXIT(ch, door)->keyword) {
        act("$n closes the $F.", 0, ch, 0, EXIT(ch, door)->keyword, TO_ROOM);
      } else {
        act("$n closes the door.", FALSE, ch, 0, 0, TO_ROOM);
      }
      act("You close the $F.", FALSE, ch, 0, EXIT(ch, door)->keyword, TO_CHAR);
      /* now for closing the other side, too */
      if ((other_room = EXIT(ch, door)->to_room) != NOWHERE) {
        if ((back = world[other_room].dir_option[rev_dir[door]])) {
          if (back->to_room == ch->in_room) {
            SET_BIT(back->exit_info, EX_CLOSED);
            if (back->keyword) {
              sprintf(buf, "The %s closes quietly.", fname(back->keyword));
              /* send_to_room(buf, EXIT(ch, door)->to_room); */
              act(buf, 0, ch, NULL, (void*) EXIT(ch, door)->to_room, TO_EXIT);
            } else {
              /* send_to_room("The door closes quietly.\r\n", EXIT(ch, door)->to_room); */
              act("The door closes quietly.", 0, ch, NULL, (void*) EXIT(ch, door)->to_room, TO_EXIT);
            }
          }
        }
      }
    }
  }
}

int has_key(struct char_data * ch, int key)
{
  struct obj_data *o;

  for (o = ch->carrying; o; o = o->next_content)
    if (GET_OBJ_VNUM(o) == key)
      return 1;

  if (ch->equipment[WEAR_HOLD])
    if (GET_OBJ_VNUM(ch->equipment[WEAR_HOLD]) == key)
      return 1;

  return 0;
}

ACMD(do_lock)
{
  int door, other_room;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct room_direction_data *back;
  struct obj_data *obj;
  struct char_data *victim;

  two_arguments(argument, type, dir);

  if (!*type) {
    send_to_char("Lock what?\r\n", ch);
  } else if (generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj)) {
    /* this is an object */
    if (GET_OBJ_TYPE(obj) != ITEM_CONTAINER) {
      send_to_char("That's not a container.\r\n", ch);
    } else if (!IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) {
      send_to_char("Maybe you should close it first...\r\n", ch);
    } else if (GET_OBJ_VAL(obj, 2) < 0) {
      send_to_char("That thing can't be locked.\r\n", ch);
    } else if (!has_key(ch, GET_OBJ_VAL(obj, 2))) {
      send_to_char("You don't seem to have the proper key.\r\n", ch);
    } else if (IS_SET(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) {
      send_to_char("It is locked already.\r\n", ch);
    } else {
      SET_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED);
      send_to_char("*click*\r\n", ch);
      act("$n locks $p - 'click', it says.", FALSE, ch, obj, 0, TO_ROOM);
    }
  } else if ((door = find_door(ch, type, dir)) >= 0) {
    /* a door, perhaps */
    if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR)) {
      send_to_char("That's absurd.\r\n", ch);
    } else if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)) {
      send_to_char("You have to close it first, I'm afraid.\r\n", ch);
    } else if (EXIT(ch, door)->key < 0) {
      send_to_char("There does not seem to be a keyhole.\r\n", ch);
    } else if (!has_key(ch, EXIT(ch, door)->key) && GET_LEVEL(ch) < LVL_IMMORT) {
      send_to_char("You don't have the proper key.\r\n", ch);
    } else if (IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED)) {
      send_to_char("It's already locked!\r\n", ch);
    } else {
      SET_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
      if (EXIT(ch, door)->keyword) {
        act("$n locks the $F.", 0, ch, 0, EXIT(ch, door)->keyword, TO_ROOM);
      } else {
        act("$n locks the door.", FALSE, ch, 0, 0, TO_ROOM);
      }
      send_to_char("*click*\r\n", ch);
      /* now for locking the other side, too */
      if ((other_room = EXIT(ch, door)->to_room) != NOWHERE) {
        if ((back = world[other_room].dir_option[rev_dir[door]])) {
          if (back->to_room == ch->in_room) {
            SET_BIT(back->exit_info, EX_LOCKED);
          }
        }
      }
    }
  }
}

ACMD(do_unlock)
{
  int door, other_room;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct room_direction_data *back;
  struct obj_data *obj;
  struct char_data *victim;

  two_arguments(argument, type, dir);

  if (!*type) {
    send_to_char("Unlock what?\r\n", ch);
  } else if (generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &victim, &obj)) {
    /* this is an object */
    if (GET_OBJ_TYPE(obj) != ITEM_CONTAINER) {
      send_to_char("That's not a container.\r\n", ch);
    } else if (!IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) {
      send_to_char("Silly - it ain't even closed!\r\n", ch);
    } else if (GET_OBJ_VAL(obj, 2) < 0) {
      send_to_char("Odd - you can't seem to find a keyhole.\r\n", ch);
    } else if (!has_key(ch, GET_OBJ_VAL(obj, 2))) {
      send_to_char("You don't seem to have the proper key.\r\n", ch);
    } else if (!IS_SET(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) {
      send_to_char("Oh.. it wasn't locked, after all.\r\n", ch);
    } else {
      REMOVE_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED);
      send_to_char("*click*\r\n", ch);
      act("$n unlocks $p.", FALSE, ch, obj, 0, TO_ROOM);
    }
  } else if ((door = find_door(ch, type, dir)) >= 0) {
    /* it is a door */
    if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR)) {
      send_to_char("That's absurd.\r\n", ch);
    } else if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)) {
      send_to_char("Heck.. it ain't even closed!\r\n", ch);
    } else if (EXIT(ch, door)->key < 0) {
      send_to_char("You can't seem to spot any keyholes.\r\n", ch);
    } else if (!has_key(ch, EXIT(ch, door)->key) && GET_LEVEL(ch) < LVL_IMMORT) {
      send_to_char("You do not have the proper key for that.\r\n", ch);
    } else if (!IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED)) {
      send_to_char("It's already unlocked, it seems.\r\n", ch);
    } else {
      REMOVE_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
      if (EXIT(ch, door)->keyword) {
        act("$n unlocks the $F.", 0, ch, 0, EXIT(ch, door)->keyword, TO_ROOM);
      } else {
        act("$n unlocks the door.", FALSE, ch, 0, 0, TO_ROOM);
      }
      send_to_char("*click*\r\n", ch);
      /* now for unlocking the other side, too */
      if ((other_room = EXIT(ch, door)->to_room) != NOWHERE) {
        if ((back = world[other_room].dir_option[rev_dir[door]])) {
          if (back->to_room == ch->in_room) {
            REMOVE_BIT(back->exit_info, EX_LOCKED);
          }
        }
      }
    }
  }
}

ACMD(do_enter)
{
  extern struct transport_data transport_list[MAX_TRANSPORT];
  extern ACMD(do_look);
  register struct obj_data *k;
  int transport = -1;
  int i = 0;
  bool exists = FALSE;
  bool valid = FALSE;

  one_argument(argument, buf);

  if (!*buf) {
    send_to_char("You must specify a target for that action.\r\n", ch);
    return;
  }

  for (i = 0; i < MAX_TRANSPORT; i++) {
    for (k = object_list; k; k = k->next) {
      if (CAN_SEE_OBJ(ch, k) && isname(buf, k->name)) {
        if (obj_index[k->item_number].virtual == transport_list[i].obj_vnum && (real_room(transport_list[i].enter_vnum) == ch->in_room || real_room(transport_list[i].exit_vnum) == ch->in_room)) {
          valid = TRUE;
          transport = i;
        }
        exists = TRUE;
      }
      if (valid && exists)
        break;
    }
    if (valid && exists)
      break;
  }

  if (!valid) {
    send_to_char("You don't see that here.\r\n", ch);
    return;
  }

  if (!exists && valid) {
    send_to_char("You can't enter that.\r\n", ch);
    return;
  }

  act(transport_list[transport].enter_room_msg, FALSE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);
  send_to_char(transport_list[transport].enter_char_msg, ch);
  if (GET_DRAGGING(ch)) {
    sprintf(buf, "%s drags $p along.", GET_NAME(ch));
    act(buf, TRUE, 0, GET_DRAGGING(ch), 0, TO_ROOM);
  }
  char_to_room(ch, real_room(transport_list[transport].room_vnum));
  act(transport_list[transport].dest_room_msg, FALSE, ch, 0, 0, TO_ROOM);
  do_look(ch, "room", 0, 0);
  if (GET_DRAGGING(ch)) {
    obj_from_room(GET_DRAGGING(ch));
    obj_to_room(GET_DRAGGING(ch), ch->in_room);
    sprintf(buf, "You drag $p along with you.");
    act(buf, FALSE, ch, GET_DRAGGING(ch), 0, TO_CHAR);
    sprintf(buf, "%s drags $p along with $m.", GET_NAME(ch));
    act(buf, TRUE, ch, GET_DRAGGING(ch), 0, TO_ROOM);
  }
}

ACMD(do_leave)
{
  extern struct transport_data transport_list[MAX_TRANSPORT];
  extern ACMD(do_look);
  register struct obj_data *k, *obj = NULL;
  int transport = -1;
  int i = 0;
  bool ontransport = FALSE;
  bool found = FALSE;

  one_argument(argument, arg);
  if (*arg) {
    if (GET_DRAGGING(ch) && isname(arg, GET_DRAGGING(ch)->name)) {
      GET_DRAGGING(ch)->dragged_by = NULL;
      GET_DRAGGING(ch) = NULL;
      sprintf(buf, "You leave %s behind.\r\n", arg);
      send_to_char(buf, ch);
    } else {
      send_to_char("Leave what behind?\r\n", ch);
    }
    return;
  }
  for (i = 0; i < MAX_TRANSPORT; i++) {
    if (transport_list[i].room_vnum == world[ch->in_room].number) {
      ontransport = TRUE;
      transport = i;
      break;
    }
  }

  if (!ontransport) {
    send_to_char("You aren't on anything you can leave.\r\n", ch);
    return;
  }

  for (k = object_list; k; k = k->next) {
    if (obj_index[k->item_number].virtual == transport_list[i].obj_vnum) {
      obj = k;
      found = TRUE;
      break;
    }
  }

  if (!found) {
    send_to_char(transport_list[transport].transport_mv_msg, ch);
    return;
  }

  act(transport_list[transport].enter_room_msg2, FALSE, ch, 0, 0, TO_ROOM);
  if (GET_DRAGGING(ch)) {
    sprintf(buf, "%s drags $p along.", GET_NAME(ch));
    act(buf, TRUE, 0, GET_DRAGGING(ch), 0, TO_ROOM);
  }
  char_from_room(ch);
  send_to_char(transport_list[transport].enter_char_msg2, ch);
  char_to_room(ch, obj->in_room);
  if (GET_DRAGGING(ch)) {
    sprintf(buf, "You drag $p along with you.");
    act(buf, FALSE, ch, GET_DRAGGING(ch), 0, TO_CHAR);
    obj_from_room(GET_DRAGGING(ch));
    obj_to_room(GET_DRAGGING(ch), ch->in_room);
    sprintf(buf, "%s drags $p along with $m.", GET_NAME(ch));
    act(buf, TRUE, ch, GET_DRAGGING(ch), 0, TO_ROOM);
  }
  act(transport_list[transport].dest_room_msg2, FALSE, ch, 0, 0, TO_ROOM);
  do_look(ch, "room", 0, 0);
}

ACMD(do_stand)
{
  switch (GET_POS(ch)) {
    case POS_STANDING:
      act("You are already standing.", FALSE, ch, 0, 0, TO_CHAR);
      break;
    case POS_SITTING:
      act("You stand up.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n clambers onto $s feet.", TRUE, ch, 0, 0, TO_ROOM);
      GET_POS(ch) = POS_STANDING;
      break;
    case POS_RESTING:
      act("You stop resting, and stand up.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n stops resting, and clambers onto $s feet.", TRUE, ch, 0, 0, TO_ROOM);
      GET_POS(ch) = POS_STANDING;
      break;
    case POS_SLEEPING:
      act("You have to wake up first!", FALSE, ch, 0, 0, TO_CHAR);
      break;
    case POS_FIGHTING:
      act("Do you not consider fighting as standing?", FALSE, ch, 0, 0, TO_CHAR);
      break;
    default:
      act("You stop floating around, and put your feet on the ground.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n stops floating around, and puts $s feet on the ground.", TRUE, ch, 0, 0, TO_ROOM);
      GET_POS(ch) = POS_STANDING;
      break;
  }
}

ACMD(do_sit)
{
  switch (GET_POS(ch)) {
    case POS_STANDING:
      act("You sit down.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n sits down.", FALSE, ch, 0, 0, TO_ROOM);
      GET_POS(ch) = POS_SITTING;
      break;
    case POS_SITTING:
      send_to_char("You're sitting already.\r\n", ch);
      break;
    case POS_RESTING:
      act("You stop resting, and sit up.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n stops resting.", TRUE, ch, 0, 0, TO_ROOM);
      GET_POS(ch) = POS_SITTING;
      break;
    case POS_SLEEPING:
      act("You have to wake up first.", FALSE, ch, 0, 0, TO_CHAR);
      break;
    case POS_FIGHTING:
      act("Sit down while fighting? are you MAD?", FALSE, ch, 0, 0, TO_CHAR);
      break;
    default:
      act("You stop floating around, and sit down.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n stops floating around, and sits down.", TRUE, ch, 0, 0, TO_ROOM);
      GET_POS(ch) = POS_SITTING;
      break;
  }
}

ACMD(do_rest)
{
  switch (GET_POS(ch)) {
    case POS_STANDING:
      act("You sit down and rest your tired bones.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n sits down and rests.", TRUE, ch, 0, 0, TO_ROOM);
      GET_POS(ch) = POS_RESTING;
      break;
    case POS_SITTING:
      act("You rest your tired bones.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n rests.", TRUE, ch, 0, 0, TO_ROOM);
      GET_POS(ch) = POS_RESTING;
      break;
    case POS_RESTING:
      act("You are already resting.", FALSE, ch, 0, 0, TO_CHAR);
      break;
    case POS_SLEEPING:
      act("You have to wake up first.", FALSE, ch, 0, 0, TO_CHAR);
      break;
    case POS_FIGHTING:
      act("Rest while fighting?  Are you MAD?", FALSE, ch, 0, 0, TO_CHAR);
      break;
    default:
      act("You stop floating around, and stop to rest your tired bones.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n stops floating around, and rests.", FALSE, ch, 0, 0, TO_ROOM);
      GET_POS(ch) = POS_SITTING;
      break;
  }
}

ACMD(do_sleep)
{
  switch (GET_POS(ch)) {
    case POS_STANDING:
    case POS_SITTING:
    case POS_RESTING:
      send_to_char("You go to sleep.\r\n", ch);
      act("$n lies down and falls asleep.", TRUE, ch, 0, 0, TO_ROOM);
      GET_POS(ch) = POS_SLEEPING;
      break;
    case POS_SLEEPING:
      send_to_char("You are already sound asleep.\r\n", ch);
      break;
    case POS_FIGHTING:
      send_to_char("Sleep while fighting?  Are you MAD?\r\n", ch);
      break;
    default:
      act("You stop floating around, and lie down to sleep.", FALSE, ch, 0, 0, TO_CHAR);
      act("$n stops floating around, and lie down to sleep.", TRUE, ch, 0, 0, TO_ROOM);
      GET_POS(ch) = POS_SLEEPING;
      break;
  }
}

ACMD(do_wake)
{
  struct char_data *vict;
  int self = 0;

  one_argument(argument, arg);
  if (*arg) {
    if (GET_POS(ch) == POS_SLEEPING)
      send_to_char("You can't wake people up if you're asleep yourself!\r\n", ch);
    else if ((vict = get_char_room_vis(ch, arg)) == NULL)
      send_to_char(NOPERSON, ch);
    else if (vict == ch)
      self = 1;
    else if (GET_POS(vict) > POS_SLEEPING)
      act("$E is already awake.", FALSE, ch, 0, vict, TO_CHAR);
    else if (IS_AFFECTED(vict, AFF_SLEEP))
      act("You can't wake $M up!", FALSE, ch, 0, vict, TO_CHAR);
    else if (AFF2_FLAGGED(vict, AFF2_KNOCKEDOUT))
      act("You can't wake $M up!", FALSE, ch, 0, vict, TO_CHAR);
    else if (GET_POS(vict) < POS_SLEEPING)
      act("You can't wake $M up!", FALSE, ch, 0, vict, TO_CHAR);
    else {
      act("You wake $M up.", FALSE, ch, 0, vict, TO_CHAR);
      act("You are awakened by $n.", FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);
      GET_POS(vict) = POS_SITTING;
    }
    if (!self)
      return;
  }
  if (IS_AFFECTED(ch, AFF_SLEEP))
    send_to_char("You can't wake up!\r\n", ch);
  else if (GET_POS(ch) > POS_SLEEPING)
    send_to_char("You are already awake...\r\n", ch);
  else {
    send_to_char("You awaken, and sit up.\r\n", ch);
    act("$n awakens.", TRUE, ch, 0, 0, TO_ROOM);
    GET_POS(ch) = POS_SITTING;
  }
}

ACMD(do_follow)
{
  struct char_data *leader;

  void stop_follower(struct char_data * ch);
  void add_follower(struct char_data * ch, struct char_data * leader);

  one_argument(argument, buf);

  if (*buf) {
    if (!(leader = get_char_room_vis(ch, buf))) {
      send_to_char(NOPERSON, ch);
      return;
    }
  } else {
    send_to_char("Whom do you wish to follow?\r\n", ch);
    return;
  }

  if (ch->master == leader) {
    act("You are already following $M.", FALSE, ch, 0, leader, TO_CHAR);
    return;
  }
  if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master)) {
    act("But you only feel like following $N!", FALSE, ch, 0, ch->master, TO_CHAR);
  } else { /* Not Charmed follow person */
    if (leader == ch) {
      if (!ch->master) {
        send_to_char("You are already following yourself.\r\n", ch);
        return;
      }
      stop_follower(ch);
    } else {
      if (circle_follow(ch, leader)) {
        act("Sorry, but following in loops is not allowed.", FALSE, ch, 0, 0, TO_CHAR);
        return;
      }
      if (ch->master)
        stop_follower(ch);
      REMOVE_BIT(AFF_FLAGS(ch), AFF_GROUP);
      add_follower(ch, leader);
    }
  }
}
