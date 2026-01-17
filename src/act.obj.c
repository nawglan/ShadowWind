/* ************************************************************************
 *   File: act.obj1.c                                    Part of CircleMUD *
 *  Usage: object handling routines -- get/drop and container handling     *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "comm.h"
#include "db.h"
#include "event.h"
#include "handler.h"
#include "interpreter.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"

/* extern variables */
extern int RaceFull[NUM_RACES];
extern char *weapon_handed[];
extern sh_int stats[12][101];
extern struct room_data *world;
extern struct index_data *obj_index;
extern char *drinks[];
extern int drink_aff[][3];
extern struct actd_msg *get_actd(int vnum);
extern int max_obj_time;
extern struct spell_info_type *spells;
extern const sh_int monk_stat[LVL_IMMORT + 1][5];

int check_consent(struct char_data *ch, char *name);
int find_skill_num(char *name);
int find_spell_num(char *name);
void mprog_give_trigger(struct char_data *mob, struct char_data *ch, struct obj_data *obj);
void mprog_bribe_trigger(struct char_data *mob, struct char_data *ch, int amount);
int quest_give_trigger(struct char_data *mob, struct char_data *ch, struct obj_data *obj);
int quest_bribe_trigger(struct char_data *mob, struct char_data *ch, int amount);
void Crash_save(struct char_data *ch, int type);

/* Sacrifice corpse command, empties the corpse, puts all the stuff on
 the ground, remove the corpse */

ACMD(do_sac) {
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *sac_obj, *temp, *next_obj;

  argument = one_argument(argument, g_arg);

  if (!(sac_obj = get_obj_in_list_vis(ch, g_arg, world[ch->in_room].contents))) {
    send_to_char("Which corpse do you want to sacrifice?\r\n", ch);
    return;
  }

  if (((GET_OBJ_TYPE(sac_obj) == ITEM_PCORPSE) || (GET_OBJ_TYPE(sac_obj) == ITEM_CONTAINER)) &&
      GET_OBJ_VAL(sac_obj, 3)) {
    if (GET_OBJ_TYPE(sac_obj) == ITEM_PCORPSE) {
      send_to_char("Player's corpses cannot be sacrificed!\r\n", ch);
      return;
    } else {
      act("$n sacrifices a corpse to the gods.", FALSE, ch, 0, 0, TO_ROOM);
      send_to_char("You sacrifice a corpse to the gods.\r\n", ch);
      for (temp = sac_obj->contains; temp; temp = next_obj) {
        next_obj = temp->next_content;
        obj_from_obj(temp);
        obj_to_room(temp, ch->in_room);
      }
      extract_obj(sac_obj);
      return;
    }
  }
  send_to_char("You can only sacrifice corpses!\r\n", ch);
  return;
}

void perform_put(struct char_data *ch, struct obj_data *obj, struct obj_data *cont) {
  if (GET_OBJ_WEIGHT(cont) + GET_OBJ_WEIGHT(obj) > GET_OBJ_VAL(cont, 0)) {
    act("$p won't fit in $P.", FALSE, ch, obj, cont, TO_CHAR);
  } else {
    obj_from_char(obj);
    obj_to_obj(obj, cont);
    act("You put $p in $P.", FALSE, ch, obj, cont, TO_CHAR);
    act("$n puts $p in $P.", TRUE, ch, obj, cont, TO_ROOM);
  }
}

/* The following put modes are supported by the code below:

 1) put <object> <container>
 2) put all.<object> <container>
 3) put all <container>

 <container> must be in inventory or on ground.
 all objects to be put into container must be in inventory.
 */

ACMD(do_put) {
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  struct obj_data *obj, *next_obj, *cont;
  struct char_data *tmp_char;
  int obj_dotmode, cont_dotmode, found = 0;

  two_arguments(argument, arg1, arg2);
  obj_dotmode = find_all_dots(arg1);
  cont_dotmode = find_all_dots(arg2);

  if (!*arg1) {
    send_to_char("Put what in what?\r\n", ch);
  } else if (!IS_NPC(ch) && GET_LEVEL(ch) == LVL_IMMORT && !PLR_FLAGGED(ch, PLR_UNRESTRICT)) {
    send_to_char("You dont feel a need to put things.\r\n", ch);
  } else if (cont_dotmode != FIND_INDIV) {
    send_to_char("You can only put things into one container at a time.\r\n", ch);
  } else if (!*arg2) {
    safe_snprintf(g_buf, MAX_STRING_LENGTH, "What do you want to put %s in?\r\n",
                  ((obj_dotmode == FIND_INDIV) ? "it" : "them"));
    send_to_char(g_buf, ch);
  } else {
    generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &tmp_char, &cont);
    if (!cont) {
      safe_snprintf(g_buf, MAX_STRING_LENGTH, "You don't see %s %s here.\r\n", AN(arg2), arg2);
      send_to_char(g_buf, ch);
    } else if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER && GET_OBJ_TYPE(cont) != ITEM_PCORPSE) {
      act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
    } else if (IS_SET(GET_OBJ_VAL(cont, 1), CONT_CLOSED)) {
      send_to_char("You'd better open it first!\r\n", ch);
    } else {
      if (obj_dotmode == FIND_INDIV) { /* put <obj> <container> */
        if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
          safe_snprintf(g_buf, MAX_STRING_LENGTH, "You aren't carrying %s %s.\r\n", AN(arg1), arg1);
          send_to_char(g_buf, ch);
        } else if (obj == cont) {
          send_to_char("You attempt to fold it into itself, but fail.\r\n", ch);
        } else {
          perform_put(ch, obj, cont);
        }
      } else {
        for (obj = ch->carrying; obj; obj = next_obj) {
          next_obj = obj->next_content;
          if (obj != cont && CAN_SEE_OBJ(ch, obj) && (obj_dotmode == FIND_ALL || isname(arg1, obj->name))) {
            found = 1;
            perform_put(ch, obj, cont);
          }
        }
        if (!found) {
          if (obj_dotmode == FIND_ALL) {
            send_to_char("You don't seem to have anything to put in it.\r\n", ch);
          } else {
            safe_snprintf(g_buf, MAX_STRING_LENGTH, "You don't seem to have any %ss.\r\n", arg1);
            send_to_char(g_buf, ch);
          }
        }
      }
    }
  }
}

int can_take_obj(struct char_data *ch, struct obj_data *obj) {

  if (!COM_FLAGGED(ch, COM_ADMIN) && IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
    act("$p: you can't carry that many items.", FALSE, ch, obj, 0, TO_CHAR);
    return 0;
  } else if (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch)) && !COM_FLAGGED(ch, COM_BUILDER)) {
    act("$p: you can't carry that much weight.", FALSE, ch, obj, 0, TO_CHAR);
    return 0;
  } else if (!(CAN_WEAR(obj, ITEM_WEAR_TAKE)) && !COM_FLAGGED(ch, COM_BUILDER)) {
    act("$p: you can't take that!", FALSE, ch, obj, 0, TO_CHAR);
    return 0;
  } else if (obj->in_obj) {
    if ((GET_OBJ_TYPE(obj->in_obj) == ITEM_PCORPSE) && !strstr(obj->in_obj->owner, GET_NAME(ch)) &&
        !check_consent(ch, obj->in_obj->owner) && !IS_IMMO(ch)) {
      act("$p: you do not have permission to get that!", FALSE, ch, obj, 0, TO_CHAR);
      return 0;
    }
  } else if (obj->dragged_by) {
    GET_DRAGGING(obj->dragged_by) = NULL;
    obj->dragged_by = NULL;
  }
  return 1;
}

void get_check_money(struct char_data *ch, struct obj_data *obj) {
  char tbuf[512];

  ACMD(do_split);
  if ((GET_OBJ_TYPE(obj) == ITEM_MONEY)) {
    obj_from_char(obj);
    safe_snprintf(g_buf, MAX_STRING_LENGTH, "There were {W%d p {Y%d g {w%d s {y%d c{x.\r\n", GET_OBJ_VAL(obj, 0),
                  GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2), GET_OBJ_VAL(obj, 3));
    send_to_char(g_buf, ch);
    GET_PLAT(ch) += GET_OBJ_VAL(obj, 0);
    GET_GOLD(ch) += GET_OBJ_VAL(obj, 1);
    GET_SILVER(ch) += GET_OBJ_VAL(obj, 2);
    GET_COPPER(ch) += GET_OBJ_VAL(obj, 3);
    GET_TEMP_GOLD(ch) +=
        GET_OBJ_VAL(obj, 0) * 1000 + GET_OBJ_VAL(obj, 1) * 100 + GET_OBJ_VAL(obj, 2) * 10 + GET_OBJ_VAL(obj, 3);
    if (PRF_FLAGGED(ch, PRF_AUTOSPLIT) && IS_AFFECTED(ch, AFF_GROUP)) {
      size_t tlen = 0;
      tbuf[0] = ' ';
      tbuf[1] = '\0';
      tlen = 1;
      if (GET_OBJ_VAL(obj, 0)) {
        tlen += safe_snprintf(tbuf + tlen, sizeof(tbuf) - tlen, " %d p", GET_OBJ_VAL(obj, 0));
      }
      if (GET_OBJ_VAL(obj, 1)) {
        tlen += safe_snprintf(tbuf + tlen, sizeof(tbuf) - tlen, " %d g", GET_OBJ_VAL(obj, 1));
      }
      if (GET_OBJ_VAL(obj, 2)) {
        tlen += safe_snprintf(tbuf + tlen, sizeof(tbuf) - tlen, " %d s", GET_OBJ_VAL(obj, 2));
      }
      if (GET_OBJ_VAL(obj, 3)) {
        tlen += safe_snprintf(tbuf + tlen, sizeof(tbuf) - tlen, " %d c", GET_OBJ_VAL(obj, 3));
      }

      do_split(ch, tbuf, 0, 0);
    }
    extract_obj(obj);
  }
}

void perform_get_from_container(struct char_data *ch, struct obj_data *obj, struct obj_data *cont, int mode) {
  if (mode == FIND_OBJ_INV || can_take_obj(ch, obj)) {
    if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
      act("$p: you can't hold any more items.", FALSE, ch, obj, 0, TO_CHAR);
    } else {
      obj_from_obj(obj);
      SET_BIT(GET_OBJ_EXTRA(obj), ITEM_CARRIED);
      /* item is in circulation, enable timer */ GET_OBJ_TIMER(obj) = max_obj_time;
      obj_to_char(obj, ch);
      act("You get $p from $P.", FALSE, ch, obj, cont, TO_CHAR);
      act("$n gets $p from $P.", TRUE, ch, obj, cont, TO_ROOM);
      get_check_money(ch, obj);
    }
  }
}

void get_from_container(struct char_data *ch, struct obj_data *cont, char *g_arg, int mode) {
  struct obj_data *obj, *next_obj;
  int obj_dotmode, found = 0;

  obj_dotmode = find_all_dots(g_arg);

  if (IS_SET(GET_OBJ_VAL(cont, 1), CONT_CLOSED)) {
    act("$p is closed.", FALSE, ch, cont, 0, TO_CHAR);
  } else if (obj_dotmode == FIND_INDIV) {
    if (!(obj = get_obj_in_list_vis(ch, g_arg, cont->contains))) {
      safe_snprintf(g_buf, MAX_STRING_LENGTH, "There doesn't seem to be %s %s in $p.", AN(g_arg), g_arg);
      act(g_buf, FALSE, ch, cont, 0, TO_CHAR);
    } else {
      perform_get_from_container(ch, obj, cont, mode);
    }
  } else {
    if (obj_dotmode == FIND_ALLDOT && !*g_arg) {
      send_to_char("Get all of what?\r\n", ch);
      return;
    }
    for (obj = cont->contains; obj; obj = next_obj) {
      next_obj = obj->next_content;
      if (CAN_SEE_OBJ(ch, obj) && (obj_dotmode == FIND_ALL || isname(g_arg, obj->name))) {
        found = 1;
        perform_get_from_container(ch, obj, cont, mode);
      }
    }
    if (!found) {
      if (obj_dotmode == FIND_ALL) {
        act("$p seems to be empty.", FALSE, ch, cont, 0, TO_CHAR);
      } else {
        safe_snprintf(g_buf, MAX_STRING_LENGTH, "You can't seem to find any %ss in $p.", g_arg);
        act(g_buf, FALSE, ch, cont, 0, TO_CHAR);
      }
    }
  }
  if (GET_OBJ_TYPE(cont) == ITEM_PCORPSE) {
    corpsesaveall();
  }
}

int perform_get_from_room(struct char_data *ch, struct obj_data *obj) {
  if (can_take_obj(ch, obj)) {
    obj_from_room(obj);
    SET_BIT(GET_OBJ_EXTRA(obj), ITEM_CARRIED);
    /* item is in circulation, enable timer */ GET_OBJ_TIMER(obj) = max_obj_time;
    obj_to_char(obj, ch);
    act("You get $p.", FALSE, ch, obj, 0, TO_CHAR);
    act("$n gets $p.", TRUE, ch, obj, 0, TO_ROOM);
    get_check_money(ch, obj);
    return 1;
  }
  return 0;
}

void get_from_room(struct char_data *ch, char *g_arg) {
  struct obj_data *obj, *next_obj;
  int dotmode, found = 0;

  dotmode = find_all_dots(g_arg);

  if (dotmode == FIND_INDIV) {
    if (!(obj = get_obj_in_list_vis(ch, g_arg, world[ch->in_room].contents))) {
      safe_snprintf(g_buf, MAX_STRING_LENGTH, "You don't see %s %s here.\r\n", AN(g_arg), g_arg);
      send_to_char(g_buf, ch);
    } else {
      if ((GET_OBJ_TYPE(obj) == ITEM_PCORPSE) && !IS_IMMO(ch)) {
        safe_snprintf(g_buf, MAX_STRING_LENGTH, "You can't take %s's corpse!", obj->owner);
        send_to_char(g_buf, ch);
        return;
      }
      perform_get_from_room(ch, obj);
    }
  } else {
    if (dotmode == FIND_ALLDOT && !*g_arg) {
      send_to_char("Get all of what?\r\n", ch);
      return;
    }
    for (obj = world[ch->in_room].contents; obj; obj = next_obj) {
      next_obj = obj->next_content;
      if (CAN_SEE_OBJ(ch, obj) && (GET_OBJ_TYPE(obj) != ITEM_PCORPSE) &&
          (dotmode == FIND_ALL || isname(g_arg, obj->name))) {
        found = 1;
        perform_get_from_room(ch, obj);
      }
    }
    if (!found) {
      if (dotmode == FIND_ALL) {
        send_to_char("There doesn't seem to be anything here.\r\n", ch);
      } else {
        safe_snprintf(g_buf, MAX_STRING_LENGTH, "You don't see any %ss here.\r\n", g_arg);
        send_to_char(g_buf, ch);
      }
    }
  }
}

ACMD(do_get) {
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];

  int cont_dotmode, found = 0, mode;
  struct obj_data *cont;
  struct char_data *tmp_char;

  two_arguments(argument, arg1, arg2);

  if (!COM_FLAGGED(ch, COM_ADMIN) && IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
    send_to_char("Your arms are already full!\r\n", ch);
  } else if (!*arg1) {
    send_to_char("Get what?\r\n", ch);
  } else if (!*arg2) {
    get_from_room(ch, arg1);
  } else {
    cont_dotmode = find_all_dots(arg2);
    if (cont_dotmode == FIND_INDIV) {
      mode = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &tmp_char, &cont);
      if (!cont) {
        safe_snprintf(g_buf, MAX_STRING_LENGTH, "You don't have %s %s.\r\n", AN(arg2), arg2);
        send_to_char(g_buf, ch);
      } else if (GET_OBJ_TYPE(cont) != ITEM_CONTAINER && GET_OBJ_TYPE(cont) != ITEM_PCORPSE) {
        act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
      } else {
        get_from_container(ch, cont, arg1, mode);
      }
    } else {
      if (cont_dotmode == FIND_ALLDOT && !*arg2) {
        send_to_char("Get from all of what?\r\n", ch);
        return;
      }
      for (cont = ch->carrying; cont; cont = cont->next_content) {
        if (CAN_SEE_OBJ(ch, cont) && (cont_dotmode == FIND_ALL || isname(arg2, cont->name))) {
          if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER || GET_OBJ_TYPE(cont) == ITEM_PCORPSE) {
            found = 1;
            get_from_container(ch, cont, arg1, FIND_OBJ_INV);
          } else if (cont_dotmode == FIND_ALLDOT) {
            found = 1;
            act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
          }
        }
      }
      for (cont = world[ch->in_room].contents; cont; cont = cont->next_content) {
        if (CAN_SEE_OBJ(ch, cont) && (cont_dotmode == FIND_ALL || isname(arg2, cont->name))) {
          if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER || GET_OBJ_TYPE(cont) == ITEM_PCORPSE) {
            get_from_container(ch, cont, arg1, FIND_OBJ_ROOM);
            found = 1;
          } else if (cont_dotmode == FIND_ALLDOT) {
            act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
            found = 1;
          }
        }
      }
      if (!found) {
        if (cont_dotmode == FIND_ALL) {
          send_to_char("You can't seem to find any containers.\r\n", ch);
        } else {
          safe_snprintf(g_buf, MAX_STRING_LENGTH, "You can't seem to find any %ss here.\r\n", arg2);
          send_to_char(g_buf, ch);
        }
      }
    }
  }
  if (!IS_NPC(ch)) {
    Crash_save(ch, RENT_CRASH);
  }
}

ACMD(do_drag) {
  char arg1[MAX_INPUT_LENGTH];
  struct obj_data *obj;

  one_argument(argument, arg1);

  if (!*arg1) {
    send_to_char("Drag what??\r\n", ch);
    return;
  }
  if (!(obj = get_obj_in_list_vis(ch, arg1, world[ch->in_room].contents))) {
    send_to_char("Couldn't find that.\r\n", ch);
    return;
  }

  if (!(CAN_WEAR(obj, ITEM_WEAR_TAKE)) && (!COM_FLAGGED(ch, COM_BUILDER)) && (GET_OBJ_TYPE(obj) != ITEM_PCORPSE)) {
    send_to_char("You can't drag that!\r\n", ch);
    return;
  }

  if ((GET_OBJ_TYPE(obj) == ITEM_PCORPSE) && !strstr(obj->owner, GET_NAME(ch)) && !check_consent(ch, obj->owner) &&
      !IS_IMMO(ch)) {
    send_to_char("You don't have permission to move that.\r\n", ch);
    return;
  }

  if ((GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(ch)) > (CAN_CARRY_W(ch) * 3)) {
    send_to_char("You can't drag that much weight.\r\n", ch);
    return;
  }

  GET_DRAGGING(ch) = obj;
  obj->dragged_by = ch;
  safe_snprintf(g_buf, MAX_STRING_LENGTH, "You begin dragging %s.\r\n", OBJS(obj, ch));
  send_to_char(g_buf, ch);
  return;
}

void perform_drop_plat(struct char_data *ch, int amount, byte mode, sh_int RDR) {
  struct obj_data *obj;

  if (amount <= 0) {
    send_to_char("Heh heh heh.. we are jolly funny today, eh?\r\n", ch);
  } else if (GET_PLAT(ch) < amount) {
    send_to_char("You don't have that many coins!\r\n", ch);
  } else {
    if (mode != SCMD_JUNK) {
      WAIT_STATE(ch, PULSE_VIOLENCE);
      /* to prevent coin-bombing */
      obj = create_money(amount, 0, 0, 0);
      if (mode == SCMD_DONATE) {
        send_to_char("You throw some platinum into the air where it disappears in a puff of smoke!\r\n", ch);
        act("$n throws some platinum into the air where it disappears in a puff of smoke!", FALSE, ch, 0, 0, TO_ROOM);
        obj_to_room(obj, RDR);
        act("$p suddenly appears in a puff of orange smoke!", 0, 0, obj, 0, TO_ROOM);
      } else {
        send_to_char("You drop some platinum.\r\n", ch);
        safe_snprintf(g_buf, MAX_STRING_LENGTH, "$n drops %s.", money_desc(amount * 1000));
        act(g_buf, FALSE, ch, 0, 0, TO_ROOM);
        obj_to_room(obj, ch->in_room);
      }
    } else {
      safe_snprintf(g_buf, MAX_STRING_LENGTH, "$n drops %s which disappears in a puff of smoke!",
                    money_desc(amount * 1000));
      act(g_buf, FALSE, ch, 0, 0, TO_ROOM);
      send_to_char("You drop some platinum which disappears in a puff of smoke!\r\n", ch);
    }
    GET_PLAT(ch) -= amount;
    GET_TEMP_GOLD(ch) -= amount * 1000;
  }
}

void perform_drop_gold(struct char_data *ch, int amount, byte mode, sh_int RDR) {
  struct obj_data *obj;

  if (amount <= 0) {
    send_to_char("Heh heh heh.. we are jolly funny today, eh?\r\n", ch);
  } else if (GET_GOLD(ch) < amount) {
    send_to_char("You don't have that many coins!\r\n", ch);
  } else {
    if (mode != SCMD_JUNK) {
      WAIT_STATE(ch, PULSE_VIOLENCE);
      /* to prevent coin-bombing */
      obj = create_money(0, amount, 0, 0);
      if (mode == SCMD_DONATE) {
        send_to_char("You throw some gold into the air where it disappears in a puff of smoke!\r\n", ch);
        act("$n throws some gold into the air where it disappears in a puff of smoke!", FALSE, ch, 0, 0, TO_ROOM);
        obj_to_room(obj, RDR);
        act("$p suddenly appears in a puff of orange smoke!", 0, 0, obj, 0, TO_ROOM);
      } else {
        send_to_char("You drop some gold.\r\n", ch);
        safe_snprintf(g_buf, MAX_STRING_LENGTH, "$n drops %s.", money_desc(amount * 100));
        act(g_buf, FALSE, ch, 0, 0, TO_ROOM);
        obj_to_room(obj, ch->in_room);
      }
    } else {
      safe_snprintf(g_buf, MAX_STRING_LENGTH, "$n drops %s which disappears in a puff of smoke!",
                    money_desc(amount * 100));
      act(g_buf, FALSE, ch, 0, 0, TO_ROOM);
      send_to_char("You drop some gold which disappears in a puff of smoke!\r\n", ch);
    }
    GET_GOLD(ch) -= amount;
    GET_TEMP_GOLD(ch) -= amount * 100;
  }
}

void perform_drop_silver(struct char_data *ch, int amount, byte mode, sh_int RDR) {
  struct obj_data *obj;

  if (amount <= 0) {
    send_to_char("Heh heh heh.. we are jolly funny today, eh?\r\n", ch);
  } else if (GET_SILVER(ch) < amount) {
    send_to_char("You don't have that many coins!\r\n", ch);
  } else {
    if (mode != SCMD_JUNK) {
      WAIT_STATE(ch, PULSE_VIOLENCE);
      /* to prevent coin-bombing */
      obj = create_money(0, 0, amount, 0);
      if (mode == SCMD_DONATE) {
        send_to_char("You throw some silver into the air where it disappears in a puff of smoke!\r\n", ch);
        act("$n throws some silver into the air where it disappears in a puff of smoke!", FALSE, ch, 0, 0, TO_ROOM);
        obj_to_room(obj, RDR);
        act("$p suddenly appears in a puff of orange smoke!", 0, 0, obj, 0, TO_ROOM);
      } else {
        send_to_char("You drop some silver.\r\n", ch);
        safe_snprintf(g_buf, MAX_STRING_LENGTH, "$n drops %s.", money_desc(amount * 10));
        act(g_buf, FALSE, ch, 0, 0, TO_ROOM);
        obj_to_room(obj, ch->in_room);
      }
    } else {
      safe_snprintf(g_buf, MAX_STRING_LENGTH, "$n drops %s which disappears in a puff of smoke!",
                    money_desc(amount * 10));
      act(g_buf, FALSE, ch, 0, 0, TO_ROOM);
      send_to_char("You drop some silver which disappears in a puff of smoke!\r\n", ch);
    }
    GET_SILVER(ch) -= amount;
    GET_TEMP_GOLD(ch) -= amount * 10;
  }
}

void perform_drop_copper(struct char_data *ch, int amount, byte mode, sh_int RDR) {
  struct obj_data *obj;

  if (amount <= 0) {
    send_to_char("Heh heh heh.. we are jolly funny today, eh?\r\n", ch);
  } else if (GET_COPPER(ch) < amount) {
    send_to_char("You don't have that many coins!\r\n", ch);
  } else {
    if (mode != SCMD_JUNK) {
      WAIT_STATE(ch, PULSE_VIOLENCE);
      /* to prevent coin-bombing */
      obj = create_money(0, 0, 0, amount);
      if (mode == SCMD_DONATE) {
        send_to_char("You throw some copper into the air where it disappears in a puff of smoke!\r\n", ch);
        act("$n throws some copper into the air where it disappears in a puff of smoke!", FALSE, ch, 0, 0, TO_ROOM);
        obj_to_room(obj, RDR);
        act("$p suddenly appears in a puff of orange smoke!", 0, 0, obj, 0, TO_ROOM);
      } else {
        send_to_char("You drop some copper.\r\n", ch);
        safe_snprintf(g_buf, MAX_STRING_LENGTH, "$n drops %s.", money_desc(amount));
        act(g_buf, FALSE, ch, 0, 0, TO_ROOM);
        obj_to_room(obj, ch->in_room);
      }
    } else {
      safe_snprintf(g_buf, MAX_STRING_LENGTH, "$n drops %s which disappears in a puff of smoke!", money_desc(amount));
      act(g_buf, FALSE, ch, 0, 0, TO_ROOM);
      send_to_char("You drop some copper which disappears in a puff of smoke!\r\n", ch);
    }
    GET_COPPER(ch) -= amount;
    GET_TEMP_GOLD(ch) -= amount;
  }
}

#define VANISH(mode) ((mode == SCMD_DONATE || mode == SCMD_JUNK) ? "  It vanishes in a puff of smoke!" : "")

int perform_drop(struct char_data *ch, struct obj_data *obj, byte mode, char *sname, sh_int RDR) {
  int value;

  if (IS_OBJ_STAT(obj, ITEM_NODROP)) {
    safe_snprintf(g_buf, MAX_STRING_LENGTH, "You can't %s $p, it must be CURSED!", sname);
    act(g_buf, FALSE, ch, obj, 0, TO_CHAR);
    return 0;
  }
  SET_BIT(GET_OBJ_EXTRA(obj), ITEM_CARRIED);
  /* item is in circulation, enable timer */
  if (GET_OBJ_TYPE(obj) == ITEM_KEY) {
    GET_OBJ_TIMER(obj) = 1;
  } else {
    GET_OBJ_TIMER(obj) = max_obj_time;
  }
  if (mode == SCMD_JUNK) {
    switch (GET_OBJ_TYPE(obj)) {
    default:
      act("As you toss $p into the air, a corrosive gas consumes it.", FALSE, ch, obj, 0, TO_CHAR);
      act("A corrosive gas consumes $p as $n tosses it into the air.", TRUE, ch, obj, 0, TO_ROOM);
      break;
    }
  } else {
    safe_snprintf(g_buf, MAX_STRING_LENGTH, "You %s $p.%s", sname, VANISH(mode));
    act(g_buf, FALSE, ch, obj, 0, TO_CHAR);
    safe_snprintf(g_buf, MAX_STRING_LENGTH, "$n %ss $p.%s", sname, VANISH(mode));
    act(g_buf, TRUE, ch, obj, 0, TO_ROOM);
    obj_from_char(obj);
  }

  if ((mode == SCMD_DONATE) && IS_OBJ_STAT(obj, ITEM_NODONATE)) {
    mode = SCMD_JUNK;
  }

  switch (mode) {
  case SCMD_DROP:
    if (GET_OBJ_TYPE(obj) == ITEM_NOTE || GET_OBJ_TYPE(obj) == ITEM_KEY) {
      act("As you drop $p it crumbles into dust.", FALSE, ch, obj, 0, TO_CHAR);
      act("$p crumbles into dust.", TRUE, 0, obj, 0, TO_ROOM);
      extract_obj(obj);
    } else {
      obj_to_room(obj, ch->in_room);
    }
    return 0;
  case SCMD_DONATE:
    obj_to_room(obj, RDR);
    SET_BIT(GET_OBJ_EXTRA(obj), ITEM_DONATED);
    /* you cant sell donated stuff */
    act("$p suddenly appears in a puff a smoke!", FALSE, 0, obj, 0, TO_ROOM);
    return 0;
  case SCMD_JUNK:
    value = BOUNDED(1, 200, GET_OBJ_COST(obj) >> 4);
    extract_obj(obj);
    return value;
  default:
    stderr_log("SYSERR: Incorrect argument passed to perform_drop");
    break;
  }

  return 0;
}

ACMD(do_drop) {
  extern sh_int donation_room_1;
#if 0
  extern sh_int donation_room_2; /* uncomment if needed! */
  extern sh_int donation_room_3; /* uncomment if needed! */
#endif
  struct obj_data *obj, *next_obj;
  sh_int RDR = 0;
  byte mode = SCMD_DROP;
  int dotmode, amount = 0;
  char *sname;

  if (!IS_NPC(ch) && GET_LEVEL(ch) == LVL_IMMORT && !COM_FLAGGED(ch, COM_IMMORT)) {
    send_to_char("You really should not be doing that.\r\n", ch);
    return;
  }

  switch (subcmd) {
  case SCMD_JUNK:
    /* taking junk out of the game to help promote selling to shops.
     sname = "junk";
     mode = SCMD_JUNK;
     break;
     */
    send_to_char("Junk has been disabled.  Please sell the item to a shop or just leave it on the ground.\r\n", ch);
    return;
  case SCMD_DONATE:
    sname = "donate";
    mode = SCMD_DONATE;
    switch (number(0, 2)) {
    case 0:
      /*      mode = SCMD_JUNK;
       break;
       */
    case 1:
    case 2:
      RDR = real_room(donation_room_1);
      break;
      /*
       case 3: RDR = real_room(donation_room_2); break;
       case 4: RDR = real_room(donation_room_3); break;
       */
    }
    if (RDR == NOWHERE) {
      send_to_char("Sorry, you can't donate anything right now.\r\n", ch);
      return;
    }
    break;
  default:
    sname = "drop";
    break;
  }

  argument = one_argument(argument, g_arg);

  if (!*g_arg) {
    safe_snprintf(g_buf, MAX_STRING_LENGTH, "What do you want to %s?\r\n", sname);
    send_to_char(g_buf, ch);
    return;
  } else if (is_number(g_arg)) {
    amount = atoi(g_arg);
    argument = one_argument(argument, g_arg);
    if (!strncmp("platinum", g_arg, strlen(g_arg))) {
      perform_drop_plat(ch, amount, mode, RDR);
    } else if (!strncmp("gold", g_arg, strlen(g_arg))) {
      perform_drop_gold(ch, amount, mode, RDR);
    } else if (!strncmp("silver", g_arg, strlen(g_arg))) {
      perform_drop_silver(ch, amount, mode, RDR);
    } else if (!strncmp("copper", g_arg, strlen(g_arg))) {
      perform_drop_copper(ch, amount, mode, RDR);
    } else {
      /* code to drop multiple items.  anyone want to write it? -je */
      send_to_char("Sorry, you can't do that to more than one item at a time.\r\n", ch);
    }
    return;
  } else {
    dotmode = find_all_dots(g_arg);

    /* Can't junk or donate all */
    if ((dotmode == FIND_ALL) && (subcmd == SCMD_JUNK || subcmd == SCMD_DONATE)) {
      if (subcmd == SCMD_JUNK) {

        /* sac chars equip
         for (i = 0; i < NUM_WEARS; i++) {
         if (ch->equipment[i]) {
         extract_obj(unequip_char(ch, i));
         }
         }
         */

        for (obj = ch->carrying; obj != NULL; obj = next_obj) {
          next_obj = obj->next_content;
          extract_obj(obj);
        }
        send_to_char("You junk your inventory.\r\n", ch);
        act("$n junks everything.", FALSE, ch, 0, 0, TO_ROOM);
      } else {
        send_to_char("Go do the donation room if you want to donate EVERYTHING!\r\n", ch);
      }
      return;
    }
    if (dotmode == FIND_ALL) {
      if (!ch->carrying) {
        send_to_char("You don't seem to be carrying anything.\r\n", ch);
      } else {
        for (obj = ch->carrying; obj; obj = next_obj) {
          next_obj = obj->next_content;
          amount += perform_drop(ch, obj, mode, sname, RDR);
        }
      }
    } else if (dotmode == FIND_ALLDOT) {
      if (!*g_arg) {
        safe_snprintf(g_buf, MAX_STRING_LENGTH, "What do you want to %s all of?\r\n", sname);
        send_to_char(g_buf, ch);
        return;
      }
      if (!(obj = get_obj_in_list_vis(ch, g_arg, ch->carrying))) {
        safe_snprintf(g_buf, MAX_STRING_LENGTH, "You don't seem to have any %ss.\r\n", g_arg);
        send_to_char(g_buf, ch);
      }
      while (obj) {
        next_obj = get_obj_in_list_vis(ch, g_arg, obj->next_content);
        if (!IS_OBJ_STAT(obj, ITEM_DONATED) || SCMD_JUNK != subcmd) {
          amount += perform_drop(ch, obj, mode, sname, RDR);
        } else {
          send_to_char("The gods chides you for junking donated items!\r\n", ch);
          act("The gods chides $n for junking donated items!\r\n", TRUE, ch, 0, 0, TO_ROOM);
          perform_drop(ch, obj, mode, sname, RDR);
        }
        obj = next_obj;
      }
    } else {
      if (!(obj = get_obj_in_list_vis(ch, g_arg, ch->carrying))) {
        safe_snprintf(g_buf, MAX_STRING_LENGTH, "You don't seem to have %s %s.\r\n", AN(g_arg), g_arg);
        send_to_char(g_buf, ch);
      } else {
        if (!IS_OBJ_STAT(obj, ITEM_DONATED) || SCMD_JUNK != subcmd) {
          amount += perform_drop(ch, obj, mode, sname, RDR);
        } else {
          send_to_char("The gods chides you for junking donated items!\r\n", ch);
          act("The gods chides $n for junking donated items!\r\n", TRUE, ch, 0, 0, TO_ROOM);
          perform_drop(ch, obj, mode, sname, RDR);
        }
      }
    }
  }

  /* Why give gold or any money? I don't see the need. *dez*
   if (amount && (subcmd == SCMD_JUNK)) {
   send_to_char("You have been rewarded by the gods!\r\n", ch);
   act("$n has been rewarded by the gods!", TRUE, ch, 0, 0, TO_ROOM);
   GET_GOLD(ch) += amount;
   }
   */
  if (!IS_NPC(ch)) {
    Crash_save(ch, RENT_CRASH);
  }
}

void perform_give(struct char_data *ch, struct char_data *vict, struct obj_data *obj) {
  if (IS_OBJ_STAT(obj, ITEM_NODROP)) {
    act("You can't let go of $p!!  Yeech!", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }
  if (IS_CARRYING_N(vict) >= CAN_CARRY_N(vict)) {
    act("$N seems to have $S hands full.", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }
  if (GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(vict) > CAN_CARRY_W(vict)) {
    act("$E can't carry that much weight.", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }
  obj_from_char(obj);
  obj_to_char(obj, vict);
  MOBTrigger = FALSE;
  act("You give $p to $N.", FALSE, ch, obj, vict, TO_CHAR);
  MOBTrigger = FALSE;
  act("$n gives you $p.", FALSE, ch, obj, vict, TO_VICT);
  MOBTrigger = FALSE;
  act("$n gives $p to $N.", TRUE, ch, obj, vict, TO_NOTVICT);
  if (IS_NPC(vict)) {
    if (!quest_give_trigger(vict, ch, obj)) {
      mprog_give_trigger(vict, ch, obj);
    }
  }
}

/* utility function for give */
struct char_data *give_find_vict(struct char_data *ch, char *g_arg) {
  struct char_data *vict;

  if (!*g_arg) {
    send_to_char("To who?\r\n", ch);
    return NULL;
  } else if (!(vict = get_char_room_vis(ch, g_arg))) {
    send_to_char(NOPERSON, ch);
    return NULL;
  } else if (vict == ch) {
    send_to_char("What's the point of that?\r\n", ch);
    return NULL;
  } else
    return vict;
}

void perform_give_plat(struct char_data *ch, struct char_data *vict, int amount) {
  if (amount <= 0) {
    send_to_char("Heh heh heh ... we are jolly funny today, eh?\r\n", ch);
    return;
  }
  if ((GET_PLAT(ch) < amount) && (IS_NPC(ch) || (!COM_FLAGGED(ch, COM_QUEST)))) {
    send_to_char("You don't have that many coins!\r\n", ch);
    return;
  }
  send_to_char(OK, ch);
  if (IS_NPC(vict)) {
    if (!quest_bribe_trigger(vict, ch, amount * 1000)) {
      mprog_bribe_trigger(vict, ch, amount * 1000);
    }
  }
  safe_snprintf(g_buf, MAX_STRING_LENGTH, "$n gives you %d {Wplatinum{x coins.", amount);
  MOBTrigger = FALSE;
  act(g_buf, FALSE, ch, 0, vict, TO_VICT);
  safe_snprintf(g_buf, MAX_STRING_LENGTH, "$n gives %s to $N.", money_desc(amount * 1000));
  MOBTrigger = FALSE;
  act(g_buf, TRUE, ch, 0, vict, TO_NOTVICT);
  if (IS_NPC(ch) || (!COM_FLAGGED(ch, COM_QUEST))) {
    GET_PLAT(ch) -= amount;
  }
  GET_TEMP_GOLD(ch) -= amount * 1000;
  GET_PLAT(vict) += amount;
  GET_TEMP_GOLD(vict) += amount * 1000;
}

void perform_give_gold(struct char_data *ch, struct char_data *vict, int amount) {
  if (amount <= 0) {
    send_to_char("Heh heh heh ... we are jolly funny today, eh?\r\n", ch);
    return;
  }
  if ((GET_GOLD(ch) < amount) && (IS_NPC(ch) || (!COM_FLAGGED(ch, COM_QUEST)))) {
    send_to_char("You don't have that many coins!\r\n", ch);
    return;
  }
  send_to_char(OK, ch);
  if (IS_NPC(vict)) {
    mprog_bribe_trigger(vict, ch, amount * 100);
    quest_bribe_trigger(vict, ch, amount * 100);
  }
  safe_snprintf(g_buf, MAX_STRING_LENGTH, "$n gives you %d {Ygold{x coins.", amount);
  MOBTrigger = FALSE;
  act(g_buf, FALSE, ch, 0, vict, TO_VICT);
  safe_snprintf(g_buf, MAX_STRING_LENGTH, "$n gives %s to $N.", money_desc(amount * 100));
  MOBTrigger = FALSE;
  act(g_buf, TRUE, ch, 0, vict, TO_NOTVICT);
  if (IS_NPC(ch) || (!COM_FLAGGED(ch, COM_QUEST))) {
    GET_GOLD(ch) -= amount;
    GET_TEMP_GOLD(ch) -= amount * 100;
  }
  GET_GOLD(vict) += amount;
  GET_TEMP_GOLD(vict) += amount * 100;
}

void perform_give_silver(struct char_data *ch, struct char_data *vict, int amount) {
  if (amount <= 0) {
    send_to_char("Heh heh heh ... we are jolly funny today, eh?\r\n", ch);
    return;
  }
  if ((GET_SILVER(ch) < amount) && (IS_NPC(ch) || (!COM_FLAGGED(ch, COM_QUEST)))) {
    send_to_char("You don't have that many coins!\r\n", ch);
    return;
  }
  send_to_char(OK, ch);
  if (IS_NPC(vict)) {
    mprog_bribe_trigger(vict, ch, amount * 10);
    quest_bribe_trigger(vict, ch, amount * 10);
  }
  safe_snprintf(g_buf, MAX_STRING_LENGTH, "$n gives you %d silver coins.", amount);
  MOBTrigger = FALSE;
  act(g_buf, FALSE, ch, 0, vict, TO_VICT);
  safe_snprintf(g_buf, MAX_STRING_LENGTH, "$n gives %s to $N.", money_desc(amount * 10));
  MOBTrigger = FALSE;
  act(g_buf, TRUE, ch, 0, vict, TO_NOTVICT);
  if (IS_NPC(ch) || (!COM_FLAGGED(ch, COM_QUEST))) {
    GET_SILVER(ch) -= amount;
    GET_TEMP_GOLD(ch) -= amount * 10;
  }
  GET_SILVER(vict) += amount;
  GET_TEMP_GOLD(vict) += amount * 10;
}

void perform_give_copper(struct char_data *ch, struct char_data *vict, int amount) {
  if (amount <= 0) {
    send_to_char("Heh heh heh ... we are jolly funny today, eh?\r\n", ch);
    return;
  }
  if ((GET_COPPER(ch) < amount) && (IS_NPC(ch) || (!COM_FLAGGED(ch, COM_QUEST)))) {
    send_to_char("You don't have that many coins!\r\n", ch);
    return;
  }
  send_to_char(OK, ch);
  if (IS_NPC(vict)) {
    mprog_bribe_trigger(vict, ch, amount);
    quest_bribe_trigger(vict, ch, amount);
  }
  safe_snprintf(g_buf, MAX_STRING_LENGTH, "$n gives you %d {ycopper{x coins.", amount);
  MOBTrigger = FALSE;
  act(g_buf, FALSE, ch, 0, vict, TO_VICT);
  safe_snprintf(g_buf, MAX_STRING_LENGTH, "$n gives %s to $N.", money_desc(amount));
  MOBTrigger = FALSE;
  act(g_buf, TRUE, ch, 0, vict, TO_NOTVICT);
  if (IS_NPC(ch) || (!COM_FLAGGED(ch, COM_QUEST))) {
    GET_COPPER(ch) -= amount;
    GET_TEMP_GOLD(ch) -= amount;
  }
  GET_COPPER(vict) += amount;
  GET_TEMP_GOLD(vict) += amount;
}

ACMD(do_give) {
  int amount, dotmode;
  struct char_data *vict = NULL;
  struct obj_data *obj, *next_obj;
  argument = one_argument(argument, g_arg);

  if (!*g_arg)
    send_to_char("Give what to who?\r\n", ch);
  else if (!IS_NPC(ch) && GET_LEVEL(ch) == LVL_IMMORT && !PLR_FLAGGED(ch, PLR_UNRESTRICT)) {
    send_to_char("Being generous today? I do not think so.\r\n", ch);
    return;
  } else if (is_number(g_arg)) {
    amount = atoi(g_arg);
    argument = one_argument(argument, g_arg);
    if (!strncmp("platinum", g_arg, strlen(g_arg))) {
      argument = one_argument(argument, g_arg);
      if ((vict = give_find_vict(ch, g_arg))) {
        perform_give_plat(ch, vict, amount);
      }
      return;
    } else if (!strncmp("gold", g_arg, strlen(g_arg))) {
      argument = one_argument(argument, g_arg);
      if ((vict = give_find_vict(ch, g_arg))) {
        perform_give_gold(ch, vict, amount);
      }
      return;
    } else if (!strncmp("silver", g_arg, strlen(g_arg))) {
      argument = one_argument(argument, g_arg);
      if ((vict = give_find_vict(ch, g_arg))) {
        perform_give_silver(ch, vict, amount);
      }
      return;
    } else if (!strncmp("copper", g_arg, strlen(g_arg))) {
      argument = one_argument(argument, g_arg);
      if ((vict = give_find_vict(ch, g_arg))) {
        perform_give_copper(ch, vict, amount);
      }
      return;
    } else {
      /* code to give multiple items.  anyone want to write it? -je */
      send_to_char("You can't give more than one item at a time.\r\n", ch);
      return;
    }
  } else {
    int found = 0;
    struct obj_data *temp;

    one_argument(argument, g_buf1);
    if (!(vict = give_find_vict(ch, g_buf1))) {
      return;
    }
    dotmode = find_all_dots(g_arg); /*PROBLEM?*/
    if (dotmode == FIND_INDIV) {
      if (!ch->carrying) {
        send_to_char("You don't seem to be holding anything.\r\n", ch);
        return;
      }
      for (temp = ch->carrying; temp; temp = temp->next_content) {
        if (strstr(temp->name, g_arg)) {
          found = 1;
        }
      }
      if (!found) {
        safe_snprintf(g_buf, MAX_STRING_LENGTH, "You don't seem to have %s %s.\r\n", AN(g_arg), g_arg);
        send_to_char(g_buf, ch);
        return;
      }
      if (!(obj = get_obj_in_list_vis(ch, g_arg, ch->carrying))) {
        safe_snprintf(g_buf, MAX_STRING_LENGTH, "You don't seem to have %s %s.\r\n", AN(g_arg), g_arg);
        send_to_char(g_buf, ch);
        return;
      } else {
        /* This keeps characters from giving keys to npcs VAR */
        if (IS_NPC(vict) && GET_OBJ_TYPE(obj) == ITEM_KEY) {
          send_to_char("I don't think that's a very good idea.\r\n", ch);
          return;
        }
      }
      perform_give(ch, vict, obj);
    } else {
      if (dotmode == FIND_ALLDOT && !*g_arg) {
        send_to_char("All of what?\r\n", ch);
        return;
      }
      if (!ch->carrying) {
        send_to_char("You don't seem to be holding anything.\r\n", ch);
      } else {
        for (obj = ch->carrying; obj; obj = next_obj) {
          next_obj = obj->next_content;
          if (CAN_SEE_OBJ(ch, obj) && ((dotmode == FIND_ALL || isname(g_arg, obj->name)))) {
            /* This keeps characters from giving keys to npcs VAR */
            if (IS_NPC(vict) && GET_OBJ_TYPE(obj) == ITEM_KEY) {
              send_to_char("I don't think that's a very good idea.\r\n", ch);
              return;
            }
            perform_give(ch, vict, obj);
          }
        }
      }
    }
  }
  all_crashsave();
}

/* Everything from here down is what was formerly act.obj2.c */

void weight_change_object(struct obj_data *obj, int weight) {
  struct obj_data *tmp_obj;
  struct char_data *tmp_ch;
  int done = 0;

  if (weight == 0) {
    return;
  }
  if ((tmp_ch = obj->carried_by)) {
    obj_from_char(obj);
    GET_OBJ_WEIGHT(obj) += weight;
    obj_to_char(obj, tmp_ch);
    done = 1;
  } else if ((tmp_obj = obj->in_obj)) {
    obj_from_obj(obj);
    GET_OBJ_WEIGHT(obj) += weight;
    obj_to_obj(obj, tmp_obj);
    done = 1;
  } else {
    GET_OBJ_WEIGHT(obj) += weight;
    done = 1;
  }
  if (!done) {
    stderr_log("SYSERR: Unknown attempt to subtract weight from an object.");
  }
}

void name_from_drinkcon(struct obj_data *obj) {
  int i;
  char *new_name;
  extern struct obj_data *obj_proto;

  for (i = 0; (*((obj->name) + i) != ' ') && (*((obj->name) + i) != '\0'); i++) {
    ;
  }

  if (*((obj->name) + i) == ' ') {
    new_name = strdup((obj->name) + i + 1);
    if (GET_OBJ_RNUM(obj) < 0 || obj->name != obj_proto[GET_OBJ_RNUM(obj)].name) {
      FREE(obj->name);
    }
    obj->name = new_name;
  }
}

void name_to_drinkcon(struct obj_data *obj, int type) {
  char *new_name;
  extern struct obj_data *obj_proto;
  extern char *drinknames[];

  CREATE(new_name, char, strlen(obj->name) + strlen(drinknames[type]) + 2);
  safe_snprintf(new_name, strlen(obj->name) + strlen(drinknames[type]) + 2, "%s %s", drinknames[type], obj->name);
  if (GET_OBJ_RNUM(obj) < 0 || obj->name != obj_proto[GET_OBJ_RNUM(obj)].name) {
    FREE(obj->name);
  }
  obj->name = new_name;
}

ACMD(do_drink) {
  struct obj_data *temp;
  struct affected_type af;
  struct actd_msg *actd;
  int amount, weight;
  int on_ground = 0;
  int spellnum = spells[find_spell_num("poison")].spellindex;

  one_argument(argument, g_arg);

  if (!*g_arg) {
    send_to_char("Drink from what?\r\n", ch);
    return;
  }
  if (!(temp = get_obj_in_list_vis(ch, g_arg, ch->carrying))) {
    if (!(temp = get_obj_in_list_vis(ch, g_arg, world[ch->in_room].contents))) {
      act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
      return;
    } else {
      on_ground = 1;
    }
  }
  if ((GET_OBJ_TYPE(temp) != ITEM_DRINKCON) && (GET_OBJ_TYPE(temp) != ITEM_FOUNTAIN)) {
    send_to_char("You can't drink from that!\r\n", ch);
    return;
  }
  if (on_ground && (GET_OBJ_TYPE(temp) == ITEM_DRINKCON)) {
    send_to_char("You have to be holding that to drink from it.\r\n", ch);
    return;
  }
  if ((GET_COND(ch, DRUNK) > 10) && (GET_COND(ch, THIRST) > 0)) {
    /* The pig is drunk */
    send_to_char("You can't seem to get close enough to your mouth.\r\n", ch);
    act("$n tries to drink but misses $s mouth!", TRUE, ch, 0, 0, TO_ROOM);
    return;
  }
  if (!IS_NPC(ch)) {
    if (GET_COND(ch, FULL) > RaceFull[GET_RACE(ch)] - 4 && (GET_COND(ch, THIRST) > 0)) {
      send_to_char("Your stomach is too full right now.\r\n", ch);
      return;
    }
  } else {
    if (GET_COND(ch, FULL) > 20 && (GET_COND(ch, THIRST) > 0)) {
      send_to_char("Your stomach is too full right now.\r\n", ch);
      return;
    }
  }
  if (GET_COND(ch, THIRST) > 20) {
    send_to_char("You feel like you'll burst if you drink anymore.\r\n", ch);
    return;
  }
  if (!GET_OBJ_VAL(temp, 1)) {
    send_to_char("It's empty.\r\n", ch);
    return;
  }
  actd = get_actd(GET_OBJ_VNUM(temp));
  if (subcmd == SCMD_DRINK) {
    if (actd) {
      act(actd->char_no_arg, FALSE, ch, temp, 0, TO_CHAR);
      act(actd->others_no_arg, TRUE, ch, temp, 0, TO_ROOM);
    } else {
      safe_snprintf(g_buf, MAX_STRING_LENGTH, "You drink the %s.\r\n", drinks[GET_OBJ_VAL(temp, 2)]);
      send_to_char(g_buf, ch);
    }

    if (drink_aff[GET_OBJ_VAL(temp, 2)][THIRST] > 0) {
      amount = (25 - GET_COND(ch, THIRST)) / drink_aff[GET_OBJ_VAL(temp, 2)][THIRST];
    } else {
      amount = number(3, 10);
    }
  } else {
    safe_snprintf(g_buf, MAX_STRING_LENGTH, "It tastes like %s.\r\n", drinks[GET_OBJ_VAL(temp, 2)]);
    send_to_char(g_buf, ch);
    amount = 1;
  }

  if (GET_OBJ_TYPE(temp) != ITEM_FOUNTAIN) {
    amount = MIN(amount, GET_OBJ_VAL(temp, 1));
    /* You can't subtract more than the object weighs */
    weight = MIN(amount, GET_OBJ_WEIGHT(temp));
    weight_change_object(temp, -weight); /* Subtract amount */
  }

  gain_condition(ch, DRUNK, (int)(drink_aff[GET_OBJ_VAL(temp, 2)][DRUNK] * amount));

  gain_condition(ch, THIRST, (int)(drink_aff[GET_OBJ_VAL(temp, 2)][THIRST] * amount));

  if (GET_COND(ch, DRUNK) > 10) {
    send_to_char("You feel drunk.\r\n", ch);
  }

  if (GET_COND(ch, THIRST) > 20) {
    send_to_char("You don't feel thirsty any more.\r\n", ch);
  }

  if (GET_OBJ_VAL(temp, 3)) { /* The shit was poisoned ! */
    send_to_char("Oops, it tasted rather strange!\r\n", ch);

    af.type = spellnum;
    af.duration = amount * 3;
    af.modifier[0] = 0;
    af.location[0] = APPLY_NONE;
    af.bitvector = AFF_POISON;
    affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
  }

  /* empty the container, and no longer poison. */
  if (GET_OBJ_TYPE(temp) != ITEM_FOUNTAIN) {
    GET_OBJ_VAL(temp, 1) -= amount;
    if (!GET_OBJ_VAL(temp, 1)) { /* The last bit */
      GET_OBJ_VAL(temp, 2) = 0;
      GET_OBJ_VAL(temp, 3) = 0;
      name_from_drinkcon(temp);
    }
    return;
  }
}

ACMD(do_eat) {
  struct obj_data *food;
  struct affected_type af;
  struct actd_msg *actd;
  int amount;
  int spellnum = spells[find_spell_num("poison")].spellindex;

  one_argument(argument, g_arg);

  if (!*g_arg) {
    send_to_char("Eat what?\r\n", ch);
    return;
  }
  if (!(food = get_obj_in_list_vis(ch, g_arg, ch->carrying))) {
    safe_snprintf(g_buf, MAX_STRING_LENGTH, "You don't seem to have %s %s.\r\n", AN(g_arg), g_arg);
    send_to_char(g_buf, ch);
    return;
  }
  if (subcmd == SCMD_TASTE && ((GET_OBJ_TYPE(food) == ITEM_DRINKCON) || (GET_OBJ_TYPE(food) == ITEM_FOUNTAIN))) {
    do_drink(ch, argument, 0, SCMD_SIP);
    return;
  }
  if ((GET_OBJ_TYPE(food) != ITEM_FOOD) && (!COM_FLAGGED(ch, COM_QUEST))) {
    send_to_char("You can't eat THAT!\r\n", ch);
    return;
  }
  if (!IS_NPC(ch)) {
    if (GET_COND(ch, FULL) > RaceFull[GET_RACE(ch)] - 3) {
      act("You are too full to eat more!", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
  } else {
    if (GET_COND(ch, FULL) > 20) { /* Stomach full */
      act("You are too full to eat more!", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
  }
  actd = get_actd(GET_OBJ_VNUM(food));
  if (subcmd == SCMD_EAT) {
    if (actd && GET_OBJ_TYPE(food) == ITEM_FOOD) {
      act(actd->char_no_arg, FALSE, ch, food, 0, TO_CHAR);
    } else {
      act("You eat the $o.", FALSE, ch, food, 0, TO_CHAR);
    }
  } else {
    act("You nibble a little bit of the $o.", FALSE, ch, food, 0, TO_CHAR);
  }

  amount = (subcmd == SCMD_EAT ? GET_OBJ_VAL(food, 0) : 1);

  gain_condition(ch, FULL, amount);

  if (!IS_NPC(ch)) {
    if (GET_COND(ch, FULL) > RaceFull[GET_RACE(ch)] - 3) {
      act("You are full.", FALSE, ch, 0, 0, TO_CHAR);
    }
  } else {
    if (GET_COND(ch, FULL) > 20) {
      act("You are full.", FALSE, ch, 0, 0, TO_CHAR);
    }
  }

  if (GET_OBJ_VAL(food, 3) && (GET_LEVEL(ch) < LVL_IMMORT)) {
    /* The shit was poisoned ! */
    send_to_char("Oops, that tasted rather strange!\r\n", ch);

    af.type = spellnum;
    af.duration = amount * 2;
    af.modifier[0] = 0;
    af.location[0] = APPLY_NONE;
    af.bitvector = AFF_POISON;
    affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
  }
  if (subcmd == SCMD_EAT) {
    extract_obj(food);
  } else {
    if (!(--GET_OBJ_VAL(food, 0))) {
      send_to_char("There's nothing left now.\r\n", ch);
      extract_obj(food);
    }
  }
}

ACMD(do_pour) {
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  struct obj_data *from_obj;
  struct obj_data *to_obj;
  int amount;

  from_obj = NULL;
  to_obj = NULL;

  two_arguments(argument, arg1, arg2);

  if (subcmd == SCMD_POUR) {
    if (!*arg1) { /* No arguments */
      act("What do you want to pour from?", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
    if (!(from_obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
      act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
    if (GET_OBJ_TYPE(from_obj) != ITEM_DRINKCON) {
      act("You can't pour from that!", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
  }
  if (subcmd == SCMD_FILL) {
    if (!*arg1) { /* no arguments */
      send_to_char("What do you want to fill?  And what are you filling it from?\r\n", ch);
      return;
    }
    if (!(to_obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
      send_to_char("You can't find it!", ch);
      return;
    }
    if (GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) {
      act("You can't fill $p!", FALSE, ch, to_obj, 0, TO_CHAR);
      return;
    }
    if (!*arg2) { /* no 2nd argument */
      act("What do you want to fill $p from?", FALSE, ch, to_obj, 0, TO_CHAR);
      return;
    }
    if (!(from_obj = get_obj_in_list_vis(ch, arg2, world[ch->in_room].contents))) {
      safe_snprintf(g_buf, MAX_STRING_LENGTH, "There doesn't seem to be %s %s here.\r\n", AN(arg2), arg2);
      send_to_char(g_buf, ch);
      return;
    }
    if (GET_OBJ_TYPE(from_obj) != ITEM_FOUNTAIN) {
      act("You can't fill something from $p.", FALSE, ch, from_obj, 0, TO_CHAR);
      return;
    }
  }
  if (GET_OBJ_VAL(from_obj, 1) == 0) {
    act("The $p is empty.", FALSE, ch, from_obj, 0, TO_CHAR);
    return;
  }
  if (subcmd == SCMD_POUR) { /* pour */
    if (!*arg2) {
      act("Where do you want it?  Out or in what?", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
    if (!str_cmp(arg2, "out")) {
      act("You empty $p.", FALSE, ch, from_obj, 0, TO_CHAR);

      weight_change_object(from_obj, -GET_OBJ_VAL(from_obj, 1)); /* Empty */

      GET_OBJ_VAL(from_obj, 1) = 0;
      GET_OBJ_VAL(from_obj, 2) = 0;
      GET_OBJ_VAL(from_obj, 3) = 0;
      name_from_drinkcon(from_obj);

      return;
    }
    if (!(to_obj = get_obj_in_list_vis(ch, arg2, ch->carrying))) {
      act("You can't find it!", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
    if ((GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) && (GET_OBJ_TYPE(to_obj) != ITEM_FOUNTAIN)) {
      act("You can't pour anything into that.", FALSE, ch, 0, 0, TO_CHAR);
      return;
    }
  }
  if (to_obj == from_obj) {
    act("A most unproductive effort.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if ((GET_OBJ_VAL(to_obj, 1) != 0) && (GET_OBJ_VAL(to_obj, 2) != GET_OBJ_VAL(from_obj, 2))) {
    act("There is already another liquid in it!", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if (!(GET_OBJ_VAL(to_obj, 1) < GET_OBJ_VAL(to_obj, 0))) {
    act("There is no room for more.", FALSE, ch, 0, 0, TO_CHAR);
    return;
  }
  if (subcmd == SCMD_POUR) {
    safe_snprintf(g_buf, MAX_STRING_LENGTH, "You pour the %s into the %s.", drinks[GET_OBJ_VAL(from_obj, 2)], arg2);
    send_to_char(g_buf, ch);
  }
  if (subcmd == SCMD_FILL) {
    act("You gently fill $p from $P.", FALSE, ch, to_obj, from_obj, TO_CHAR);
  }
  /* New alias */
  if (GET_OBJ_VAL(to_obj, 1) == 0) {
    name_to_drinkcon(to_obj, GET_OBJ_VAL(from_obj, 2));
  }

  /* First same type liq. */ GET_OBJ_VAL(to_obj, 2) = GET_OBJ_VAL(from_obj, 2);

  /* Then how much to pour */ GET_OBJ_VAL(from_obj, 1) -= (amount = (GET_OBJ_VAL(to_obj, 0) - GET_OBJ_VAL(to_obj, 1)));

  GET_OBJ_VAL(to_obj, 1) = GET_OBJ_VAL(to_obj, 0);

  if (GET_OBJ_VAL(from_obj, 1) < 0) { /* There was too little */
    GET_OBJ_VAL(to_obj, 1) += GET_OBJ_VAL(from_obj, 1);
    amount += GET_OBJ_VAL(from_obj, 1);
    GET_OBJ_VAL(from_obj, 1) = 0;
    GET_OBJ_VAL(from_obj, 2) = 0;
    GET_OBJ_VAL(from_obj, 3) = 0;
    name_from_drinkcon(from_obj);
  }
  /* Then the poison boogie */ GET_OBJ_VAL(to_obj, 3) = (GET_OBJ_VAL(to_obj, 3) || GET_OBJ_VAL(from_obj, 3));

  /* And the weight boogie */
  weight_change_object(from_obj, -amount);
  weight_change_object(to_obj, amount); /* Add weight */

  return;
}

void wear_message(struct char_data *ch, struct obj_data *obj, int where) {
  char *wear_messages[][2] = {
      {"$n wears $p on $s chest.", "You wear $p on your chest."},
      {"$n slides $p on to $s right ring finger.", "You slide $p on to your right ring finger."},
      {"$n slides $p on to $s left ring finger.", "You slide $p on to your left ring finger."},
      {"$n wears $p around $s neck.", "You wear $p around your neck."},
      {"$n wears $p around $s neck.", "You wear $p around your neck."},
      {
          "$n wears $p on $s body.",
          "You wear $p on your body.",
      },
      {"$n wears $p on $s head.", "You wear $p on your head."},
      {"$n puts $p on $s legs.", "You put $p on your legs."},
      {"$n wears $p on $s feet.", "You wear $p on your feet."},
      {"$n puts $p on $s hands.", "You put $p on your hands."},
      {"$n wears $p on $s arms.", "You wear $p on your arms."},
      {"$n straps $p around $s arm as a shield.", "You start to use $p as a shield."},
      {"$n wears $p about $s body.", "You wear $p around your body."},
      {"$n wears $p around $s waist.", "You wear $p around your waist."},
      {"$n puts $p around $s right wrist.", "You put $p around your right wrist."},
      {"$n puts $p around $s left wrist.", "You put $p around your left wrist."},
      {"$n wields $p.", "You wield $p."},
      {"$n grabs $p.", "You grab $p."},
      {"$n puts $p on $s face.", "You put $p on your face."},
      {"$n sticks $p in $s left ear.", "You stick $p in your left ear."},
      {"$n sticks $p in $s right ear.", "You stick $p in your right ear."},
      {"$n wears $p over $s eyes.", "You wear $p over your eyes."},
      {"$n wears $p around $s left ankle.", "You wear $p around your left ankle."},
      {"$n wears $p around $s right ankle.", "You wear $p around your right ankle."},
      {"$n wields $p.", "You wield $p."},
      {"$n grabs $p.", "You grab $p."},
      {"$n wields $p.", "You wield $p."},
      {"$n lights $p and holds it.", "You light $p and hold it."}};

  if (GET_OBJ_TYPE(obj) == ITEM_LIGHT) {
    act(wear_messages[where + 10][0], TRUE, ch, obj, 0, TO_ROOM);
    act(wear_messages[where + 10][1], FALSE, ch, obj, 0, TO_CHAR);
  } else {
    act(wear_messages[where][0], TRUE, ch, obj, 0, TO_ROOM);
    act(wear_messages[where][1], FALSE, ch, obj, 0, TO_CHAR);
  }
}

void perform_wear(struct char_data *ch, struct obj_data *obj, int where) {
  int skillnum = spells[find_skill_num("dual wield")].spellindex;

  int wear_bitvectors[] = {ITEM_WEAR_BADGE, ITEM_WEAR_FINGER, ITEM_WEAR_FINGER, ITEM_WEAR_NECK,   ITEM_WEAR_NECK,
                           ITEM_WEAR_BODY,  ITEM_WEAR_HEAD,   ITEM_WEAR_LEGS,   ITEM_WEAR_FEET,   ITEM_WEAR_HANDS,
                           ITEM_WEAR_ARMS,  ITEM_WEAR_SHIELD, ITEM_WEAR_ABOUT,  ITEM_WEAR_WAIST,  ITEM_WEAR_WRIST,
                           ITEM_WEAR_WRIST, ITEM_WEAR_WIELD,  ITEM_WEAR_TAKE,   ITEM_WEAR_FACE,   ITEM_WEAR_EAR,
                           ITEM_WEAR_EAR,   ITEM_WEAR_EYES,   ITEM_WEAR_ANKLES, ITEM_WEAR_ANKLES, ITEM_WEAR_WIELD,
                           ITEM_WEAR_HOLD,  ITEM_WEAR_2HANDED};

  char *already_wearing[] = {"You're already wearing a badge.\r\n",
                             "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
                             "You're already wearing something on both of your ring fingers.\r\n",
                             "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
                             "You can't wear anything else around your neck.\r\n",
                             "You're already wearing something on your body.\r\n",
                             "You're already wearing something on your head.\r\n",
                             "You're already wearing something on your legs.\r\n",
                             "You're already wearing something on your feet.\r\n",
                             "You're already wearing something on your hands.\r\n",
                             "You're already wearing something on your arms.\r\n",
                             "You're already using a shield.\r\n",
                             "You're already wearing something about your body.\r\n",
                             "You already have something around your waist.\r\n",
                             "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
                             "You're already wearing something around both of your wrists.\r\n",
                             "You're already wielding a weapon.\r\n",
                             "You're already holding something.\r\n",
                             "You're already wearing something on your face.\r\n",
                             "YOU SHOULD NEVER SEE THIS MESSAGE. PLEASE REPORT.\r\n",
                             "You're already using earrings in both your ears.\r\n",
                             "You're already wearing something on your eyes.\r\n",
                             "YOU SHOULD NEVER SEE THIS MESSAGE. PLEASE REPORT.\r\n",
                             "You're already wearing something on both your ankles.\r\n",
                             "Your hands are full.\r\n",
                             "Your hands are full.\r\n",
                             "You need both hands free to wield that.\r\n"};

  /* Pets can only use lights */
  if (IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM) && (GET_OBJ_TYPE(obj) != ITEM_LIGHT)) {
    if (ch->master && !IS_NPC(ch->master)) {
      send_to_char("Pets are only allowed to use lights.\r\n", ch->master);
    }
    return;
  }

  /* first, make sure that the wear position is valid. */
  if (!CAN_WEAR(obj, wear_bitvectors[where])) {
    act("You can't wear $p there.", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }
  if (GET_OBJ_SLOTS(obj) && CHAR_WEARING(ch))
    if (IS_SET(GET_OBJ_SLOTS(obj), CHAR_WEARING(ch)) && GET_OBJ_TYPE(obj) != ITEM_WEAPON) {
      send_to_char(already_wearing[where], ch);
      return;
    }
  /* for neck, finger, and wrist, try pos 2 if pos 1 is already full */
  if ((where == WEAR_FINGER_R) || (where == WEAR_NECK_1) || (where == WEAR_WRIST_R) || (where == WEAR_EAR_R) ||
      (where == WEAR_ANKLE_R))
    if (GET_EQ(ch, where))
      where++;

  if (where == WEAR_WIELD && !GET_SKILL(ch, skillnum) && GET_EQ(ch, WEAR_WIELD)) {
    send_to_char(already_wearing[where], ch);
    return;
  }
  /* else if (where == WEAR_WIELD && GET_SKILL(ch, skillnum) && GET_EQ(ch, WEAR_WIELD))
   where = WEAR_WIELD_2;
   */

  if ((where == WEAR_WIELD || where == WEAR_WIELD_2) && (GET_OBJ_TYPE(obj) == ITEM_WEAPON) &&
      (GET_OBJ_VAL(obj, 0) > 3) &&
      (GET_EQ(ch, WEAR_HOLD) || GET_EQ(ch, WEAR_WIELD) || GET_EQ(ch, WEAR_SHIELD) || GET_EQ(ch, WEAR_HOLD_2) ||
       GET_EQ(ch, WEAR_WIELD_2) || GET_EQ(ch, WEAR_2HANDED))) {
    send_to_char(already_wearing[where + 10], ch);
    return;
  }
  if ((where == WEAR_WIELD || where == WEAR_WIELD_2) && (GET_OBJ_TYPE(obj) == ITEM_WEAPON) && (GET_OBJ_VAL(obj, 0) > 3))
    where = WEAR_2HANDED;

  if (where == WEAR_HOLD || ((where == WEAR_WIELD || where == WEAR_WIELD_2) && !GET_SKILL(ch, skillnum))) {
    if (GET_EQ(ch, WEAR_HOLD) && (GET_EQ(ch, WEAR_WIELD) || GET_EQ(ch, WEAR_SHIELD) || GET_EQ(ch, WEAR_HOLD_2) ||
                                  GET_EQ(ch, WEAR_2HANDED) || GET_EQ(ch, WEAR_WIELD_2))) {
      send_to_char(already_wearing[where + 8], ch);
      return;
    } else if (GET_EQ(ch, WEAR_SHIELD) && (GET_EQ(ch, WEAR_HOLD) || GET_EQ(ch, WEAR_HOLD_2) || GET_EQ(ch, WEAR_WIELD) ||
                                           GET_EQ(ch, WEAR_2HANDED) || GET_EQ(ch, WEAR_WIELD_2))) {
      send_to_char(already_wearing[where + 8], ch);
      return;
    } else if (GET_EQ(ch, WEAR_WIELD) &&
               (GET_EQ(ch, WEAR_HOLD) || GET_EQ(ch, WEAR_HOLD_2) || GET_EQ(ch, WEAR_WIELD_2) ||
                GET_EQ(ch, WEAR_2HANDED) || GET_EQ(ch, WEAR_SHIELD))) {
      send_to_char(already_wearing[where + 8], ch);
      return;
    } else if (GET_EQ(ch, WEAR_2HANDED)) {
      send_to_char(already_wearing[where + 8], ch);
      return;
    } else if (GET_EQ(ch, where))
      where += 8;
  } else if (where == WEAR_HOLD || where == WEAR_WIELD) {
    if (GET_EQ(ch, where))
      where += 8;
  }

  if (GET_SKILL(ch, skillnum) && (where == WEAR_WIELD || where == WEAR_WIELD_2)) {
    if (GET_EQ(ch, WEAR_HOLD) && (GET_EQ(ch, WEAR_WIELD) || GET_EQ(ch, WEAR_SHIELD) || GET_EQ(ch, WEAR_HOLD_2) ||
                                  GET_EQ(ch, WEAR_2HANDED) || GET_EQ(ch, WEAR_WIELD_2))) {
      send_to_char(already_wearing[where], ch);
      return;
    } else if (GET_EQ(ch, WEAR_SHIELD) && (GET_EQ(ch, WEAR_HOLD) || GET_EQ(ch, WEAR_HOLD_2) || GET_EQ(ch, WEAR_WIELD) ||
                                           GET_EQ(ch, WEAR_2HANDED) || GET_EQ(ch, WEAR_WIELD_2))) {
      send_to_char(already_wearing[where], ch);
      return;
    } else if (GET_EQ(ch, WEAR_WIELD) &&
               (GET_EQ(ch, WEAR_HOLD) || GET_EQ(ch, WEAR_HOLD_2) || GET_EQ(ch, WEAR_WIELD_2) ||
                GET_EQ(ch, WEAR_2HANDED) || GET_EQ(ch, WEAR_SHIELD))) {
      send_to_char(already_wearing[where], ch);
      return;
    } else if (GET_EQ(ch, WEAR_2HANDED)) {
      send_to_char(already_wearing[where], ch);
      return;
    }
  }

  if (where == WEAR_SHIELD)
    if (GET_EQ(ch, WEAR_2HANDED) || (GET_EQ(ch, WEAR_HOLD) && GET_EQ(ch, WEAR_WIELD)) ||
        (GET_EQ(ch, WEAR_WIELD) && GET_EQ(ch, WEAR_WIELD_2)) || (GET_EQ(ch, WEAR_HOLD) && GET_EQ(ch, WEAR_WIELD_2)) ||
        (GET_EQ(ch, WEAR_HOLD_2) && GET_EQ(ch, WEAR_WIELD)) || (GET_EQ(ch, WEAR_HOLD) && GET_EQ(ch, WEAR_HOLD_2))) {
      send_to_char(already_wearing[where + 13], ch);
      return;
    }

  if (GET_EQ(ch, where)) {
    send_to_char(already_wearing[where], ch);
    return;
  }

  if (IS_MONK(ch) && (GET_OBJ_WEIGHT(obj) > monk_stat[(sh_int)GET_LEVEL(ch)][3])) {
    send_to_char("That much weight would make your skills useless.\r\n", ch);
    return;
  }

  if ((GET_OBJ_TYPE(obj) == ITEM_LIGHT) && (where == WEAR_HOLD_2)) {
    wear_message(ch, obj, where - 10);
  } else {
    wear_message(ch, obj, where);
  }
  obj_from_char(obj);
  equip_char(ch, obj, where);
}

int find_eq_pos(struct char_data *ch, struct obj_data *obj, char *g_arg) {
  int where = -1;

  static char *keywords[] = {"badge",      "finger", "!RESERVED!", "neck",       "!RESERVED!", "body",
                             "head",       "legs",   "feet",       "hands",      "arms",       "shield",
                             "about",      "waist",  "wrist",      "!RESERVED!", "!RESERVED!", "!RESERVED!",
                             "!RESERVED!", "face",   "ear",        "eyes",       "ankles",     "\n"};

  if (!g_arg || !*g_arg) {
    if (CAN_WEAR(obj, ITEM_WEAR_FINGER)) {
      where = WEAR_FINGER_R;
    }
    if (CAN_WEAR(obj, ITEM_WEAR_NECK)) {
      where = WEAR_NECK_1;
    }
    if (CAN_WEAR(obj, ITEM_WEAR_BODY)) {
      where = WEAR_BODY;
    }
    if (CAN_WEAR(obj, ITEM_WEAR_HEAD)) {
      where = WEAR_HEAD;
    }
    if (CAN_WEAR(obj, ITEM_WEAR_LEGS)) {
      where = WEAR_LEGS;
    }
    if (CAN_WEAR(obj, ITEM_WEAR_FEET)) {
      where = WEAR_FEET;
    }
    if (CAN_WEAR(obj, ITEM_WEAR_HANDS)) {
      where = WEAR_HANDS;
    }
    if (CAN_WEAR(obj, ITEM_WEAR_ARMS)) {
      where = WEAR_ARMS;
    }
    if (CAN_WEAR(obj, ITEM_WEAR_SHIELD)) {
      where = WEAR_SHIELD;
    }
    if (CAN_WEAR(obj, ITEM_WEAR_ABOUT)) {
      where = WEAR_ABOUT;
    }
    if (CAN_WEAR(obj, ITEM_WEAR_WAIST)) {
      where = WEAR_WAIST;
    }
    if (CAN_WEAR(obj, ITEM_WEAR_WRIST)) {
      where = WEAR_WRIST_R;
    }
    if (CAN_WEAR(obj, ITEM_WEAR_FACE)) {
      where = WEAR_FACE;
    }
    if (CAN_WEAR(obj, ITEM_WEAR_EAR)) {
      where = WEAR_EAR_R;
    }
    if (CAN_WEAR(obj, ITEM_WEAR_EYES)) {
      where = WEAR_EYES;
    }
    if (CAN_WEAR(obj, ITEM_WEAR_ANKLES)) {
      where = WEAR_ANKLE_R;
    }
    if (CAN_WEAR(obj, ITEM_WEAR_BADGE)) {
      where = WEAR_BADGE;
    }
  } else {
    if ((where = search_block(g_arg, keywords, FALSE)) < 0) {
      safe_snprintf(g_buf, MAX_STRING_LENGTH, "'%s'?  What part of your body is THAT?\r\n", g_arg);
      send_to_char(g_buf, ch);
    }
  }

  return where;
}

ACMD(do_wear) {
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  struct obj_data *obj, *next_obj;
  int where, dotmode, items_worn = 0;

  two_arguments(argument, arg1, arg2);

  if (!*arg1) {
    send_to_char("Wear what?\r\n", ch);
    return;
  }
  dotmode = find_all_dots(arg1);

  if (*arg2 && (dotmode != FIND_INDIV)) {
    send_to_char("You can't specify the same body location for more than one item!\r\n", ch);
    return;
  }
  if (dotmode == FIND_ALL) {
    for (obj = ch->carrying; obj; obj = next_obj) {
      next_obj = obj->next_content;
      if (CAN_SEE_OBJ(ch, obj) && (where = find_eq_pos(ch, obj, 0)) >= 0) {
        items_worn++;
        perform_wear(ch, obj, where);
      }
    }
    if (!items_worn) {
      send_to_char("You don't seem to have anything wearable.\r\n", ch);
    }
  } else if (dotmode == FIND_ALLDOT) {
    if (!*arg1) {
      send_to_char("Wear all of what?\r\n", ch);
      return;
    }
    if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
      safe_snprintf(g_buf, MAX_STRING_LENGTH, "You don't seem to have any %ss.\r\n", arg1);
      send_to_char(g_buf, ch);
    } else
      while (obj) {
        next_obj = get_obj_in_list_vis(ch, arg1, obj->next_content);
        if ((where = find_eq_pos(ch, obj, 0)) >= 0) {
          perform_wear(ch, obj, where);
        } else {
          act("You can't wear $p.", FALSE, ch, obj, 0, TO_CHAR);
        }
        obj = next_obj;
      }
  } else {
    if (!(obj = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
      safe_snprintf(g_buf, MAX_STRING_LENGTH, "You don't seem to have %s %s.\r\n", AN(arg1), arg1);
      send_to_char(g_buf, ch);
    } else {
      if ((where = find_eq_pos(ch, obj, arg2)) >= 0) {
        perform_wear(ch, obj, where);
      } else if (!*arg2) {
        act("You can't wear $p.", FALSE, ch, obj, 0, TO_CHAR);
      }
    }
  }
}

ACMD(do_wield) {
  struct obj_data *obj;
  extern struct spell_info_type *spells;
  char buf2[64];

  one_argument(argument, g_arg);

  if (!*g_arg) {
    send_to_char("Wield what?\r\n", ch);
  } else if (!(obj = get_obj_in_list_vis(ch, g_arg, ch->carrying))) {
    safe_snprintf(g_buf, MAX_STRING_LENGTH, "You don't seem to have %s %s.\r\n", AN(g_arg), g_arg);
    send_to_char(g_buf, ch);
  } else {
    if (CAN_WEAR(obj, ITEM_WEAR_WIELD)) {
      memset(g_buf2, 0, 64);
      sprinttype(GET_OBJ_VAL(obj, 0), weapon_handed, g_buf2);
    }
    if (!CAN_WEAR(obj, ITEM_WEAR_WIELD)) {
      send_to_char("You can't wield that.\r\n", ch);
    } else if (GET_OBJ_WEIGHT(obj) > stats[STR_WWEIGHT][GET_STR(ch)]) {
      send_to_char("It's too heavy for you to use.\r\n", ch);
    } else if (!IS_NPC(ch) && (GET_LEVEL(ch) < spells[find_skill_num(g_buf2)].min_level[(int)GET_CLASS(ch)])) {
      send_to_char("You are not allowed to use that weapon.\r\n", ch);
    } else if ((GET_SKILL(ch, (spells[find_skill_num(g_buf2)].spellindex)) < 5) && !IS_NPC(ch)) {
      send_to_char("You cant figure out how to use this weapon. Maybe you should learn?\r\n", ch);
    } else {
      perform_wear(ch, obj, WEAR_WIELD);
    }
  }
}

ACMD(do_grab) {
  struct obj_data *obj;

  one_argument(argument, g_arg);

  if (!*g_arg) {
    send_to_char("Hold what?\r\n", ch);
  } else if (!(obj = get_obj_in_list_vis(ch, g_arg, ch->carrying))) {
    safe_snprintf(g_buf, MAX_STRING_LENGTH, "You don't seem to have %s %s.\r\n", AN(g_arg), g_arg);
    send_to_char(g_buf, ch);
  } else {
    if (GET_OBJ_TYPE(obj) == ITEM_BADGE) {
      perform_wear(ch, obj, WEAR_BADGE);
    } else {
      if (!CAN_WEAR(obj, ITEM_WEAR_HOLD) && GET_OBJ_TYPE(obj) != ITEM_WAND && GET_OBJ_TYPE(obj) != ITEM_STAFF &&
          GET_OBJ_TYPE(obj) != ITEM_SCROLL && GET_OBJ_TYPE(obj) != ITEM_POTION) {
        send_to_char("You can't hold that.\r\n", ch);
      } else {
        perform_wear(ch, obj, WEAR_HOLD);
      }
    }
  }
}

void perform_remove(struct char_data *ch, int pos) {
  struct obj_data *obj;

  if (!(obj = ch->equipment[pos])) {
    stderr_log("Error in perform_remove: bad pos passed.");
    return;
  }
  if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
    act("$p: you can't carry that many items!", FALSE, ch, obj, 0, TO_CHAR);
  } else {
    obj_to_char(unequip_char(ch, pos), ch);
    act("You stop using $p.", FALSE, ch, obj, 0, TO_CHAR);
    act("$n stops using $p.", TRUE, ch, obj, 0, TO_ROOM);
  }
}

ACMD(do_remove) {
  int i, dotmode, found;

  one_argument(argument, g_arg);

  if (!*g_arg) {
    send_to_char("Remove what?\r\n", ch);
    return;
  }
  dotmode = find_all_dots(g_arg);

  if (dotmode == FIND_ALL) {
    found = 0;
    for (i = 0; i < NUM_WEARS; i++) {
      if (ch->equipment[i]) {
        if (i != WEAR_2HANDED || (i == WEAR_2HANDED && !AFF3_FLAGGED(ch, AFF3_WIELDINGSPIRITUALHAMMER) &&
                                  !AFF3_FLAGGED(ch, AFF3_WIELDINGFLAMEBLADE))) {
          REMOVE_BIT(CHAR_WEARING(ch), GET_OBJ_SLOTS(ch->equipment[i]));
          perform_remove(ch, i);
          found = 1;
        } else {
          send_to_char("You cannot remove that weapon.\r\n", ch);
        }
      }
    }
    if (!found) {
      send_to_char("You're not using anything.\r\n", ch);
    }
  } else if (dotmode == FIND_ALLDOT) {
    if (!*g_arg) {
      send_to_char("Remove all of what?\r\n", ch);
    } else {
      found = 0;
      for (i = 0; i < NUM_WEARS; i++) {
        if (ch->equipment[i] && CAN_SEE_OBJ(ch, ch->equipment[i]) && isname(g_arg, ch->equipment[i]->name)) {
          if (i != WEAR_2HANDED || (i == WEAR_2HANDED && !AFF3_FLAGGED(ch, AFF3_WIELDINGSPIRITUALHAMMER) &&
                                    !AFF3_FLAGGED(ch, AFF3_WIELDINGFLAMEBLADE))) {
            REMOVE_BIT(CHAR_WEARING(ch), GET_OBJ_SLOTS(ch->equipment[i]));
            perform_remove(ch, i);
            found = 1;
          } else {
            send_to_char("You cannot remove that weapon.\r\n", ch);
          }
        }
      }
      if (!found) {
        safe_snprintf(g_buf, MAX_STRING_LENGTH, "You don't seem to be using any %ss.\r\n", g_arg);
        send_to_char(g_buf, ch);
      }
    }
  } else {
    if (!get_object_in_equip_vis(ch, g_arg, ch->equipment, &i)) {
      safe_snprintf(g_buf, MAX_STRING_LENGTH, "You don't seem to be using %s %s.\r\n", AN(g_arg), g_arg);
      send_to_char(g_buf, ch);
    } else {
      if (i != WEAR_2HANDED || (i == WEAR_2HANDED && !AFF3_FLAGGED(ch, AFF3_WIELDINGSPIRITUALHAMMER) &&
                                !AFF3_FLAGGED(ch, AFF3_WIELDINGFLAMEBLADE))) {
        REMOVE_BIT(CHAR_WEARING(ch), GET_OBJ_SLOTS(ch->equipment[i]));
        perform_remove(ch, i);
      } else {
        send_to_char("You cannot remove that weapon.\r\n", ch);
      }
    }
  }
}
