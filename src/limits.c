/* ************************************************************************
 *   File: limits.c                                      Part of CircleMUD *
 *  Usage: limits & gain funcs for HMV, exp, hunger/thirst, idle time      *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include "structs.h"
#include "utils.h"
#include "event.h"
#include "spells.h"
#include "comm.h"
#include "db.h"
#include "handler.h"

extern struct char_data *character_list;
extern struct obj_data *object_list;
extern int MaxExperience[LVL_IMMORT + 2];
extern struct room_data *world;
extern int max_obj_time;
extern int max_pc_corpse_time;
extern int RaceFull[NUM_RACES];
extern struct spell_info_type *spells;
extern int NODECAY;
int find_spell_num(char *name);
void Crash_save(struct char_data *ch, int type);

/*
 * The hit_limit, mana_limit, and move_limit functions are gone.  They
 * added an unnecessary level of complexity to the internal structure,
 * weren't particularly useful, and led to some annoying bugs.  From the
 * players' point of view, the only difference the removal of these
 * functions will make is that a character's age will now only affect
 * the HMV gain per tick, and _not_ the HMV maximums.
 */

/* manapoint gain pr. game hour */
int mana_gain(struct char_data * ch, int tickcount)
{

  int gain = 0;

  /* Neat and fast mob mana gain */
  if (IS_NPC(ch)) {
    return (ROOM_FLAGGED(IN_ROOM(ch), ROOM_FASTHEAL) ? (GET_LEVEL(ch) + (GET_LEVEL(ch) >> 1)) : GET_LEVEL(ch));
  }

  if ((IS_AFFECTED(ch, AFF_POISON) || (IS_AFFECTED(ch, AFF_DISEASE)) || (GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0)) && number(1, 5) < 4)
    return 0;

  /* Position bases */
  switch (GET_POS(ch)) {
    case POS_FIGHTING:
      if (!((tickcount + 2) % 8)) { /* 1pt per 8 */
        gain++;
      }
      break;
    case POS_STUNNED:
    case POS_STANDING:
    case POS_SITTING:
      if (!((tickcount + 2) % 4)) { /* 1pt per 4 */
        gain++;
      }
      break;
    case POS_RESTING:
      if (!((tickcount + 2) % 3)) { /* 1pt per 3 */
        gain++;
      }
      break;
    case POS_SLEEPING:
      if (!((tickcount + 2) % 2)) { /* 1pt per 2 */
        gain++;
      }
      break;
    default:
      return 0;
  }
  /* Racial mods */
  switch (GET_RACE(ch)) {
    case RACE_ELF: /* 25% chance of 1pt/4sec */
      if (!((tickcount + 2) % 2) && !number(0, 3)) {
        gain++;
      }
      break;
    case RACE_GNOME: /* 12% chance of 1pt/4sec */
      if (!((tickcount + 2) % 2) && !number(0, 7)) {
        gain++;
      }
      break;
  }

  /* Int and Wis modifiers */
  if ((GET_INT(ch) < 35) && (GET_INT(ch) < GET_WIS(ch))) { /*chance of no */
    if (gain && ((50 - GET_INT(ch)) >= (number(1, 250)))) /*mana gain at */
      gain--; /* int 7: 7%    int 3: 15%     int or wis   */
  } else if (GET_WIS(ch) < 35) {
    if (gain && ((50 - GET_WIS(ch)) >= (number(1, 250))))
      gain--; /* wis 7: 7%    wis 3: 15% */
  }
  if (IS_MAGE(ch) && (GET_INT(ch) > 60)) {
    if (!((tickcount + 3) % 5) && ((GET_INT(ch) - 60) >= (number(1, 150))))
      gain++; /* int 15: 4%   int 18: 14% of 1pt/5sec */
  } else if (GET_WIS(ch) > 60) {
    if (!((tickcount + 3) % 5) && ((GET_WIS(ch) - 60) >= (number(1, 150))))
      gain++; /* wis 15: 4%   wis 18: 14% of 1pt/5sec */
  }

  return (gain);
}

int hit_gain(struct char_data * ch, int tickcount)
/* Hitpoint gain RL sec for PCs */
{
  /* struct char_data *temp; */
  int gain = 0;

  /* regen for mobs == LEVEL hps per tick */
  if (IS_NPC(ch)) {
    return (ROOM_FLAGGED(IN_ROOM(ch), ROOM_FASTHEAL) ? (GET_LEVEL(ch) + (GET_LEVEL(ch) >> 1)) : GET_LEVEL(ch));
  }

  if ((IS_AFFECTED(ch, AFF_POISON) || (IS_AFFECTED(ch, AFF_DISEASE)) || (GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0)) && number(1, 5) < 4)
    return 0;

  /* Position Bases */
  switch (GET_POS(ch)) {
    case POS_FIGHTING:
      if (!(tickcount % 10)) { /* 1pt per 10 */
        gain++;
      }
      break;
    case POS_STUNNED:
    case POS_SITTING:
    case POS_STANDING:
      if (!(tickcount % 5)) { /* 1pt per 5 */
        gain++;
      }
      break;
    case POS_RESTING:
      if (!(tickcount % 4)) { /* 1pt per 4 */
        gain++;
      }
      break;
    case POS_SLEEPING:
      if (!(tickcount % 2)) { /* 1pt per 3 */
        gain++;
      }
      break;
    default:
      return -1;
  }

  /* Racial mods */
  switch (GET_RACE(ch)) {
    case RACE_TROLL:
      if (!((tickcount + 2) % 2) && !number(0, 3)) {
        gain++;
      }
      break;
    case RACE_OGRE:
      if (!((tickcount + 2) % 4) && !number(0, 5)) {
        gain--;
      }
      break;
    default:
      break;
  }

  /* Constitution mods */
  if (GET_CON(ch) < 35) { /* chance of not regening due to low con */
    if (gain && ((50 - GET_CON(ch)) >= (number(1, 250))))
      gain--; /* con 7: 7%    con 3: 15% */
  }

  if (GET_CON(ch) > 60) { /* chance of regening more due to high con */
    if (!((tickcount + 3) % 5) && ((GET_CON(ch) - 60) >= (number(1, 150))))
      gain++; /* con 15: 4%   con 18: 14% of 1pt/5sec */
  }

  return (gain);
}

int move_gain(struct char_data * ch, int tickcount)
/* move gain pr. game hour */
{
  int gain = 0;

  /* Neat and fast move gain for mobs */
  if (IS_NPC(ch)) {
    if (!IS_AFFECTED(ch, AFF_POISON) && !IS_AFFECTED(ch, AFF_DISEASE)) {
      return (ROOM_FLAGGED(IN_ROOM(ch), ROOM_FASTHEAL) ? (GET_LEVEL(ch) + (GET_LEVEL(ch) >> 1)) : GET_LEVEL(ch));
    } else {
      return ((number(1, 5) == 1) ? (GET_LEVEL(ch) >> 1) : 0);
    }
  }

  if ((IS_AFFECTED(ch, AFF_POISON) || (GET_COND(ch, FULL) == 0) || (IS_AFFECTED(ch, AFF_DISEASE)) || (GET_COND(ch, THIRST) == 0)) && number(1, 5) < 4)
    return 0;

  /* Position bases    */
  switch (GET_POS(ch)) {
    case POS_FIGHTING:
      if (!(tickcount % 8)) {
        gain++;
      }
      break;
    case POS_STUNNED:
    case POS_STANDING:
    case POS_SITTING:
      if (!(tickcount % 4)) {
        gain++;
      }
      break;
    case POS_RESTING:
      if (!(tickcount % 3)) {
        gain++;
      }
      break;
    case POS_SLEEPING:
      if (!(tickcount % 2) && (number(1, 100) <= 75)) {
        gain++;
      }
      break;
    default:
      return 0;
  }

  /* Racial mods */
  switch (GET_RACE(ch)) {
    case RACE_OGRE: /* 20% chance of 1pt/4sec */
      if (!((tickcount + 2) % 4) && !number(0, 3)) {
        gain++;
      }
      break;
    case RACE_DWARF: /* 10% chance of 1pt/4sec */
      if (!((tickcount + 2) % 4) && !number(0, 9)) {
        gain++;
      }
      break;
    default:
      break;
  }

  /* Constitution mods */
  if (GET_CON(ch) < 35) { /* chance of not regening due to low con */
    if (gain && ((50 - GET_CON(ch)) >= (number(1, 250))))
      gain--; /* con 7: 7%    con 3: 15% */
  }
  if (GET_CON(ch) > 60) { /* chance of regening more due to high con */
    if (!((tickcount + 3) % 5) && ((GET_CON(ch) - 60) >= (number(1, 150))))
      gain++; /* con 15: 4%   con 18: 14% of 1pt/5sec */
  }

  return (gain);
}

void set_title(struct char_data * ch, char *title)
{

  if (title != NULL) {
    sprintf(buf, "%s", title);
    if (strlen(title) > MAX_TITLE_LENGTH)
      title[MAX_TITLE_LENGTH] = '\0';
    if (GET_TITLE(ch) != NULL)
      FREE(GET_TITLE(ch));
    GET_TITLE(ch) = strdup(buf);
  }
}

void check_autowiz(struct char_data * ch)
{
  char buf[100];
  extern int use_autowiz;
  extern int min_wizlist_lev;

  if (use_autowiz && GET_LEVEL(ch) >= LVL_IMMORT) {
    sprintf(buf, "nice ../bin/autowiz %d %s %d %s %d &", min_wizlist_lev, WIZLIST_FILE, LVL_IMMORT, IMMLIST_FILE, (int) getpid());
    mudlog("Initiating autowiz.", 'S', COM_ADMIN, FALSE);
    system(buf);
  }
}

void gain_exp(struct char_data * ch, int gain)
{
  int is_altered = FALSE;
  int lower_max_exp_gain;
  int lower_max_exp_loss;

  if (!IS_NPC(ch) && ((GET_LEVEL(ch) < 1 || GET_LEVEL(ch) >= LVL_IMMORT)))
    return;

  if (IS_NPC(ch)) {
    GET_EXP(ch) += gain;
    return;
  }

  /*
   * this will replace the max_exp_gain limit set in limits.h
   * now it is  (next_level_min - current_level_min / 10)
   * this will eliminate players from gaining levels rapid fire
   * like they used to. With our damage limits in place, it should
   * now take a minimum of 9 kills to level once
   */
  lower_max_exp_gain = (MaxExperience[GET_LEVEL(ch) + 1] - MaxExperience[(int) GET_LEVEL(ch)]) / 20;
  lower_max_exp_loss = (MaxExperience[GET_LEVEL(ch) + 1] - MaxExperience[(int) GET_LEVEL(ch)]) / 3;

  if (gain > 0) {
    /* put a cap on the max gain per kill */
    gain = MIN(lower_max_exp_gain, gain);
    GET_EXP(ch) += gain;
    while (GET_LEVEL(ch) < LVL_IMMORT - 1 && GET_EXP(ch) >= MaxExperience[GET_LEVEL(ch) + 1]) {
      send_to_char("You raise a level!\r\n", ch);
      GET_LEVEL(ch)++;
      advance_level(ch);
      if (GET_LEVEL(ch) == LVL_IMMORT) {
        send_to_char("Suddenly all of your equpiment begins to glow red and disappears.\r\n", ch);
        is_altered = TRUE;
        SET_BIT(PRF_FLAGS(ch), PRF_ROOMFLAGS);
        SET_BIT(PRF_FLAGS(ch), PRF_NOHASSLE);
        SET_BIT(PRF_FLAGS(ch), PRF_HOLYLIGHT);
      }
    }
    if (is_altered) {
      set_title(ch, NULL);
      check_autowiz(ch);
    }
  } else if (gain < 0) {
    gain = MAX(-lower_max_exp_loss, gain); /* Cap max exp lost per death */
    GET_EXP(ch) += gain;
    if (GET_LEVEL(ch) > 1 && GET_EXP(ch) < MaxExperience[(int) GET_LEVEL(ch)]) {
      lose_level(ch);
      send_to_char("You lose a level!\r\n", ch);
      GET_LEVEL(ch)--;
    }
    if (GET_EXP(ch) < 1)
      GET_EXP(ch) = 1;
  }
}

void gain_exp_regardless(struct char_data * ch, int gain)
{
  int is_altered = FALSE;

  GET_EXP(ch) += gain;
  if (GET_EXP(ch) < 1)
    GET_EXP(ch) = 1;

  if (!IS_NPC(ch)) {
    while (GET_LEVEL(ch) < LVL_IMMORT && GET_EXP(ch) >= MaxExperience[GET_LEVEL(ch) + 1]) {
      send_to_char("You raise a level!\r\n", ch);
      GET_LEVEL(ch)++;
      advance_level(ch);
      if (GET_LEVEL(ch) == LVL_IMMORT)
        send_to_char("Suddenly all of your equpiment begins to glow red and disappears.\r\n", ch);
      is_altered = TRUE;
    }
    if (is_altered) {
      set_title(ch, NULL);
      check_autowiz(ch);
    }
  }
}

void gain_condition(struct char_data * ch, int condition, int value)
{
  bool intoxicated;

  if (!ch || GET_COND(ch, condition) == -1) /* No change */
    return;

  intoxicated = (GET_COND(ch, DRUNK) > 0);

  GET_COND(ch, condition) += value;

  GET_COND(ch, condition) = MAX(0, GET_COND(ch, condition));
  if (!IS_NPC(ch) && condition == FULL) {
    GET_COND(ch, condition) = MIN(RaceFull[GET_RACE(ch)], GET_COND(ch, condition));
  } else {
    GET_COND(ch, condition) = MIN(24, GET_COND(ch, condition));
  }

  if (GET_COND(ch, condition) || PLR_FLAGGED(ch, PLR_WRITING))
    return;

  switch (condition) {
    case FULL:
      send_to_char("You are hungry.\r\n", ch);
      return;
    case THIRST:
      send_to_char("You are thirsty.\r\n", ch);
      return;
    case DRUNK:
      if (intoxicated)
        send_to_char("You are now sober.\r\n", ch);
      return;
    default:
      break;
  }

}

void check_idling(struct char_data * ch)
{
  extern int free_rent;

  if (++(ch->char_specials.timer) > 8 && !COM_FLAGGED(ch, COM_QUEST)) {
    if (GET_WAS_IN(ch) == NOWHERE && ch->in_room != NOWHERE) {
      GET_WAS_IN(ch) = ch->in_room;
      if (FIGHTING(ch)) {
        stop_fighting(FIGHTING(ch));
        stop_fighting(ch);
      }
      act("$n disappears into the void.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char("You have been idle, and are pulled into a void.\r\n", ch);
      save_char_text(ch, NOWHERE);
      Crash_save(ch, RENT_CRASH);
      char_from_room(ch);
      char_to_room(ch, 1);

      /* 48 = 60min, change 48 to 12 for 15min idle extrations */

    } else if (ch->char_specials.timer > 12) {
      if (ch->in_room != NOWHERE) {
        SET_BIT(PLR_FLAGS(ch), PLR_LOADROOM);
        GET_LOADROOM(ch) = world[ch->was_in_room].number;
        char_from_room(ch);
      }
      char_to_room(ch, 1);
      if (ch->desc) {
        close_socket(ch->desc);
      }
      ch->desc = NULL;
      if (free_rent) {
        Crash_save(ch, RENT_RENTED);
      } else {
        Crash_save(ch, RENT_TIMEDOUT);
      }
      sprintf(buf, "%s force-rented and extracted (idle).", GET_NAME(ch));
      mudlog(buf, 'R', COM_IMMORT, TRUE);
      extract_char(ch, 0);
    }
  }
}

void char_regen(void)
{
  struct char_data *i, *next_char;
  static sh_int tickcount = 1;
  int oldhit = 0;

  /* characters */
  for (i = character_list; i; i = next_char) {
    next_char = i->next;
    if (!ROOM_FLAGGED(IN_ROOM(i), ROOM_NOHEAL)) {
      if (!IS_NPC(i)) {
        if (GET_POS(i) >= POS_STUNNED) {
          if (GET_RACE(i) == RACE_TROLL || ROOM_FLAGGED(IN_ROOM(i), ROOM_FASTHEAL)) {
            if (ROOM_FLAGGED(IN_ROOM(i), ROOM_FASTHEAL) && GET_RACE(i) == RACE_TROLL) {
              GET_HIT(i) = MIN(GET_HIT(i) + 4 * hit_gain(i, tickcount), GET_MAX_HIT(i));
              GET_MANA(i) = MIN(GET_MANA(i) + 4 * mana_gain(i, tickcount), GET_MAX_MANA(i));
              GET_MOVE(i) = MIN(GET_MOVE(i) + 4 * move_gain(i, tickcount), GET_MAX_MOVE(i));
            } else {
              GET_HIT(i) = MIN(GET_HIT(i) + 2 * hit_gain(i, tickcount), GET_MAX_HIT(i));
              GET_MANA(i) = MIN(GET_MANA(i) + 2 * mana_gain(i, tickcount), GET_MAX_MANA(i));
              GET_MOVE(i) = MIN(GET_MOVE(i) + 2 * move_gain(i, tickcount), GET_MAX_MOVE(i));
            }
          } else if (ROOM_FLAGGED(IN_ROOM(i), ROOM_HARM)) {
            GET_HIT(i) = MIN(GET_HIT(i) - hit_gain(i, tickcount), GET_MAX_HIT(i));
            if (GET_HIT(i) < 1) {
              GET_HIT(i) = 1;
            }
            GET_MANA(i) = MIN(GET_MANA(i) - mana_gain(i, tickcount), GET_MAX_MANA(i));
            if (GET_MANA(i) < 1) {
              GET_MANA(i) = 1;
            }
            GET_MOVE(i) = MIN(GET_MOVE(i) - move_gain(i, tickcount), GET_MAX_MOVE(i));
            if (GET_MOVE(i) < 1) {
              GET_MOVE(i) = 1;
            }
          } else {
            GET_HIT(i) = MIN(GET_HIT(i) + hit_gain(i, tickcount), GET_MAX_HIT(i));
            GET_MANA(i) = MIN(GET_MANA(i) + mana_gain(i, tickcount), GET_MAX_MANA(i));
            GET_MOVE(i) = MIN(GET_MOVE(i) + move_gain(i, tickcount), GET_MAX_MOVE(i));
          }
        } else if (GET_POS(i) <= POS_INCAP) {
          if (!number(0, 15)) {
            oldhit = GET_HIT(i);
            damage(i, i, 2, TYPE_SUFFERING, 0, DAM_UNDEFINED, 0, 0);
            if (oldhit == GET_HIT(i))
              GET_HIT(i) = MIN(GET_HIT(i) + hit_gain(i, tickcount), GET_MAX_HIT(i));
          }
        }
      }
    }

    update_pos(i);
  }
  tickcount++;
  if (tickcount == 61)
    tickcount = 1;
}

/* Update PCs, NPCs, and objects */
void point_update(void)
{
  void update_char_objects(struct char_data * ch); /* handler.c */
  void extract_obj(struct obj_data * obj); /* handler.c */
  void do_wake(struct char_data *ch, char *argument, int cmd, int subcmd);
  struct char_data *i, *next_char;
  struct obj_data *j, *next_thing, *jj, *next_thing2;
  int spellnum = spells[find_spell_num("poison")].spellindex;
  int spellnum2 = spells[find_spell_num("disease")].spellindex;

  for (i = character_list; i; i = next_char) {
    next_char = i->next;
    if (AFF3_FLAGGED(i, AFF3_WIELDINGFLAMEBLADE)) {
      i->player_specials->saved.weapontimer--;
      if (i->player_specials->saved.weapontimer < 0) {
        REMOVE_BIT(AFF3_FLAGS(i), AFF3_WIELDINGFLAMEBLADE);
        if (GET_EQ(i, WEAR_2HANDED)) {
          j = unequip_char(i, WEAR_2HANDED);
          extract_obj(j);
          send_to_char(spells[find_spell_num("flame blade")].wear_off, i);
          send_to_char("\r\n", i);
        }
      }
    }
    if (AFF3_FLAGGED(i, AFF3_WIELDINGSPIRITUALHAMMER)) {
      i->player_specials->saved.weapontimer--;
      if (i->player_specials->saved.weapontimer < 0) {
        REMOVE_BIT(AFF3_FLAGS(i), AFF3_WIELDINGSPIRITUALHAMMER);
        if (GET_EQ(i, WEAR_2HANDED)) {
          j = unequip_char(i, WEAR_2HANDED);
          extract_obj(j);
          send_to_char(spells[find_spell_num("spiritual hammer")].wear_off, i);
          send_to_char("\r\n", i);
        }
      }
    }
    if (GET_POS(i) >= POS_STUNNED) {
      if (IS_NPC(i)) {
        if (ROOM_FLAGGED(IN_ROOM(i), ROOM_FASTHEAL)) {
          GET_HIT(i) = MIN(GET_HIT(i) + 2 * hit_gain(i, 0), GET_MAX_HIT(i));
          GET_MANA(i) = MIN(GET_MANA(i) + 2 * mana_gain(i, 0), GET_MAX_MANA(i));
          GET_MOVE(i) = MIN(GET_MOVE(i) + 2 * move_gain(i, 0), GET_MAX_MOVE(i));
        } else if (ROOM_FLAGGED(IN_ROOM(i), ROOM_HARM)) {
          GET_HIT(i) = MIN(GET_HIT(i) - hit_gain(i, 0), GET_MAX_HIT(i));
          GET_MANA(i) = MIN(GET_MANA(i) - mana_gain(i, 0), GET_MAX_MANA(i));
          GET_MOVE(i) = MIN(GET_MOVE(i) - move_gain(i, 0), GET_MAX_MOVE(i));
        } else {
          GET_HIT(i) = MIN(GET_HIT(i) + hit_gain(i, 0), GET_MAX_HIT(i));
          GET_MANA(i) = MIN(GET_MANA(i) + mana_gain(i, 0), GET_MAX_MANA(i));
          GET_MOVE(i) = MIN(GET_MOVE(i) + move_gain(i, 0), GET_MAX_MOVE(i));
        }
      }
      if (IS_AFFECTED(i, AFF_POISON) && !AFF2_FLAGGED(i, AFF2_PROT_GAS)) {
        damage(i, i, 2, spellnum, 0, DAM_POISON, TRUE, 0);
        if (GET_POS(i) == POS_SLEEPING) {
          do_wake(i, NULL, 0, 0);
        }
      }
      if (IS_AFFECTED(i, AFF_DISEASE)) {
        damage(i, i, 4, spellnum2, 0, DAM_DISEASE, TRUE, 0);
        if (GET_POS(i) == POS_SLEEPING) {
          do_wake(i, NULL, 0, 0);
        }
      }
      if (GET_POS(i) <= POS_STUNNED) {
        update_pos(i);
      }
    } else if (GET_POS(i) <= POS_INCAP) {
      damage(i, i, 2, TYPE_SUFFERING, 0, DAM_UNDEFINED, 0, 0);
    }
    if (!i) {
      return;
    }
    if (!IS_NPC(i)) {
      switch (GET_RACE(i)) {
        case RACE_HALFLING:
        case RACE_GNOME:
          gain_condition(i, FULL, -1);
          break;
        case RACE_DWARF:
        case RACE_ELF:
          if (number(0, 1)) {
            gain_condition(i, FULL, -2);
          } else {
            gain_condition(i, FULL, -1);
          }
          break;
        case RACE_HALFELF:
        case RACE_HUMAN:
          gain_condition(i, FULL, -2);
          break;
        case RACE_TROLL:
          if (number(0, 1)) {
            gain_condition(i, FULL, -3);
          } else {
            gain_condition(i, FULL, -2);
          }
          break;
        case RACE_OGRE:
          gain_condition(i, FULL, -3);
          break;
      }
    } else {
      gain_condition(i, FULL, -1);
    }
    gain_condition(i, THIRST, -1);
    if (!IS_NPC(i)) {
      update_char_objects(i);
      check_idling(i);
    }
  }

  /* objects */
  for (j = object_list; j && NODECAY == 0; j = next_thing) {
    next_thing = j->next; /* Next in object list */
    if (IS_OBJ_STAT(j, ITEM_NODECAY)) {
      continue;
    }

    /* If this is a corpse */
    if ((GET_OBJ_TYPE(j) == ITEM_CONTAINER || GET_OBJ_TYPE(j) == ITEM_PCORPSE) && GET_OBJ_VAL(j, 3)) {
      /* timer count down */
      if (GET_OBJ_TIMER(j) > 0) {
        GET_OBJ_TIMER(j)--;
      } else {
        if (j->carried_by) {
          act("$p decays in your hands.", FALSE, j->carried_by, j, 0, TO_CHAR);
        } else if ((j->in_room != NOWHERE) && (world[j->in_room].people)) {
          act("A quivering horde of maggots consumes $p.", TRUE, world[j->in_room].people, j, 0, TO_ROOM);
          act("A quivering horde of maggots consumes $p.", TRUE, world[j->in_room].people, j, 0, TO_CHAR);
        }
        for (jj = j->contains; jj; jj = next_thing2) {
          next_thing2 = jj->next_content; /* Next in inventory */
          if (IS_OBJ_STAT(jj, ITEM_NODECAY)) {
            continue;
          }
          obj_from_obj(jj);

          if (j->in_obj) {
            obj_to_obj(jj, j->in_obj);
          } else if (j->carried_by) {
            obj_to_room(jj, j->carried_by->in_room);
          } else if (j->in_room != NOWHERE) {
            obj_to_room(jj, j->in_room);
          } else {
            assert(FALSE);
          }
        }
        extract_obj(j);
      }
    } else { /* standard object, has been carried */
      if (j->in_room != NOWHERE && IS_OBJ_STAT(j, ITEM_CARRIED) && !ROOM_FLAGGED(j->in_room, ROOM_HOUSE)) {
        if (GET_OBJ_TIMER(j) > 0) {
          GET_OBJ_TIMER(j)--;
        }
        if (!GET_OBJ_TIMER(j)) {
          extract_obj(j);
        }
      } else {
        if (GET_OBJ_TYPE(j) == ITEM_PCORPSE) {
          GET_OBJ_TIMER(j) = max_pc_corpse_time;
        } else {
          GET_OBJ_TIMER(j) = max_obj_time;
        }
      }
    }
  }
}
