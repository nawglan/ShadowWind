/* ************************************************************************
 *   File: mobact.c                                      Part of CircleMUD *
 *  Usage: Functions for generating intelligent (?) behavior in mobiles    *
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

/* external structs */
extern struct zone_data *zone_table;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct room_data *world;
extern sh_int stats[11][101];
extern struct spell_info_type *spells;

int find_skill_num(char *name);
void mprog_random_trigger(struct char_data *mob);
void mprog_wordlist_check(char *arg, struct char_data *mob, struct char_data *actor, struct obj_data *obj, void *vo,
                          int type);
extern int is_empty(int zone_nr);
extern void raw_kill(struct char_data *ch, struct char_data *killer);
extern void hunt_aggro(struct char_data *ch);
extern void hunt_room(struct char_data *ch);
void improve_skill(struct char_data *ch, int skill, int chance);

#define MOB_AGGR_TO_ALIGN MOB_AGGR_EVIL | MOB_AGGR_NEUTRAL | MOB_AGGR_GOOD

void perform_mob_defense(void) {
  struct char_data *ch;
  struct char_data *next_ch;
  int mobdef = 0;
  char mobdefbuf[256];

  for (ch = character_list; ch; ch = next_ch) {
    next_ch = ch->next;

    memset(mobdefbuf, 0, 256);
    /* use this one when mob spell offense is done.
     if (!IS_MOB(ch) || AFF2_FLAGGED(ch, AFF2_CASTING) || FIGHTING(ch))
     */
    if (!IS_MOB(ch) || AFF2_FLAGGED(ch, AFF2_CASTING))
      continue;

    if (IS_MOB(ch) && MOB_FLAGGED(ch, MOB_HAS_MAGE)) {
      switch (number(0, 5)) {
      case 1:
        mobdef = 1;
        if (!AFF2_FLAGGED(ch, AFF2_FIRESHIELD) && !AFF2_FLAGGED(ch, AFF2_ICESHIELD)) {
          safe_snprintf(mobdefbuf, sizeof(mobdefbuf), "cast 'fire shield' me");
        }
        break;
      case 2:
        mobdef = 1;
        if (!AFF2_FLAGGED(ch, AFF2_ICESHIELD) && !AFF2_FLAGGED(ch, AFF2_FIRESHIELD)) {
          safe_snprintf(mobdefbuf, sizeof(mobdefbuf), "cast 'ice shield' me");
        }
        break;
      case 3:
        mobdef = 1;
        if (!AFF2_FLAGGED(ch, AFF2_STONESKIN)) {
          safe_snprintf(mobdefbuf, sizeof(mobdefbuf), "cast 'stoneskin' me");
        }
        break;
      case 4:
        mobdef = 1;
        if (!AFF2_FLAGGED(ch, AFF2_VAMPTOUCH)) {
          safe_snprintf(mobdefbuf, sizeof(mobdefbuf), "cast 'vampiric touch' me");
        }
        break;
      }
      if (mobdef) {
        command_interpreter(ch, mobdefbuf);
      }
    }

    if (IS_MOB(ch) && MOB_FLAGGED(ch, MOB_HAS_CLERIC)) {
      if (GET_HIT(ch) < GET_MAX_HIT(ch)) {
        if (FIGHTING(ch) && GET_HIT(ch) < (GET_MAX_HIT(ch) / 2)) {
          mobdef = 1;
          safe_snprintf(mobdefbuf, sizeof(mobdefbuf), "cast 'full heal' me");
        }
      }
      if (!mobdef) {
        switch (AFF_FLAGGED(ch, AFF_BLIND) ? 7 : number(0, 6)) {
        case 1:
          mobdef = 1;
          if (!AFF_FLAGGED(ch, AFF_AID)) {
            safe_snprintf(mobdefbuf, sizeof(mobdefbuf), "cast 'aid' me");
          }
          break;
        case 2:
          mobdef = 1;
          if (!AFF_FLAGGED(ch, AFF_BLESS)) {
            safe_snprintf(mobdefbuf, sizeof(mobdefbuf), "cast 'bless' me");
          }
          break;
        case 3:
          mobdef = 1;
          if (!AFF2_FLAGGED(ch, AFF2_BARKSKIN)) {
            safe_snprintf(mobdefbuf, sizeof(mobdefbuf), "cast 'barkskin' me");
          }
          break;
        case 4:
          mobdef = 1;
          if (!AFF2_FLAGGED(ch, AFF2_ARMOR)) {
            safe_snprintf(mobdefbuf, sizeof(mobdefbuf), "cast 'armor' me");
          }
          break;
        case 5:
          mobdef = 1;
          if (AFF_FLAGGED(ch, AFF_POISON)) {
            safe_snprintf(mobdefbuf, sizeof(mobdefbuf), "cast 'cure poison' me");
          }
          break;
        case 6:
          mobdef = 1;
          if (AFF_FLAGGED(ch, AFF_DISEASE)) {
            safe_snprintf(mobdefbuf, sizeof(mobdefbuf), "cast 'cure disease' me");
          }
          break;
        case 7:
          mobdef = 1;
          if (AFF_FLAGGED(ch, AFF_BLIND)) {
            safe_snprintf(mobdefbuf, sizeof(mobdefbuf), "cast 'cure blind' me");
          }
          break;
        }
      }
      if (mobdef) {
        command_interpreter(ch, mobdefbuf);
      }
    }
  }
}

void mobile_activity(void) {
  register struct char_data *ch, *next_ch, *vict;
  struct obj_data *obj, *best_obj;
  struct mob_action_data *action;
  int door, found, max;
  memory_rec *names;
  int skillnum = spells[find_skill_num("aggressive")].spellindex;
  char *resetpos[] = {
      "stand", /* dead  */
      "stand", /* mort  */
      "stand", /* incap */
      "stand", /* stunn */
      "sleep", /* sleep */
      "rest",  /* rest  */
      "sit",   /* sit   */
      "stand", /* fight */
      "stand"  /* stand */
  };

  extern int no_specials;

  for (ch = character_list; ch; ch = next_ch) {
    next_ch = ch->next;

    if (!IS_MOB(ch) || FIGHTING(ch) || AFF2_FLAGGED(ch, AFF2_CASTING))
      continue;
    /* Examine call for special procedure */
    if (!no_specials && GET_MOB_SPEC(ch) != NULL &&
        (IS_SET(SPEC_MOB_TYPE(ch), SPEC_STANDARD) || IS_SET(SPEC_MOB_TYPE(ch), SPEC_HEARTBEAT))) {
      if ((mob_index[GET_MOB_RNUM(ch)].func)(ch, ch, 0, "", SPEC_HEARTBEAT))
        continue; /* go to next char */
    }

    if (!is_empty(world[ch->in_room].zone) || MOB_FLAGGED(ch, MOB_PROGALWAYS))
      mprog_random_trigger(ch);

    if (ch->mob_specials.mob_action && !FIGHTING(ch)) { /* Mobile has actions defined */
      for (action = ch->mob_specials.mob_action; action; action = action->next)
        if ((number(0, 99) < action->chance) && (GET_POS(ch) >= action->minpos)) {
          act(action->action, TRUE, ch, 0, 0, TO_ROOM);
          break;
        }
    }

    /* Scavenger (picking up objects) */
    if (MOB_FLAGGED(ch, MOB_SCAVENGER) && !FIGHTING(ch) && AWAKE(ch) && !AFF2_FLAGGED(ch, AFF2_MINOR_PARALIZED) &&
        !AFF_FLAGGED(ch, AFF_MAJOR_PARALIZED))
      if (world[ch->in_room].contents && !number(0, 10)) {
        max = 1;
        best_obj = NULL;
        for (obj = world[ch->in_room].contents; obj; obj = obj->next_content)
          if (CAN_GET_OBJ(ch, obj) && GET_OBJ_COST(obj) > max) {
            best_obj = obj;
            max = GET_OBJ_COST(obj);
          }
        if (best_obj != NULL && GET_OBJ_TYPE(best_obj) != ITEM_PCORPSE) {
          obj_from_room(best_obj);
          obj_to_char(best_obj, ch);
          act("$n gets $p.", FALSE, ch, best_obj, 0, TO_ROOM);
        }
      }

    /* Mob Movement */
    if (!HUNTING(ch) && !AFF2_FLAGGED(ch, AFF2_MINOR_PARALIZED) && !AFF_FLAGGED(ch, AFF_MAJOR_PARALIZED) &&
        !MOUNTED_BY(ch) && !MOB_FLAGGED(ch, MOB_SENTINEL) && (GET_POS(ch) == POS_STANDING) &&
        ((door = number(0, 18)) < NUM_OF_DIRS) && CAN_GO(ch, door) &&
        !ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_NOMOB | ROOM_DEATH) &&
        (!MOB_FLAGGED(ch, MOB_STAY_ZONE) || (world[EXIT(ch, door)->to_room].zone == world[ch->in_room].zone)) &&
        !(MOB_FLAGGED(ch, MOB_WATERONLY) && (world[EXIT(ch, door)->to_room].sector_type != SECT_WATER_NOSWIM &&
                                             world[EXIT(ch, door)->to_room].sector_type != SECT_WATER_SWIM &&
                                             world[EXIT(ch, door)->to_room].sector_type != SECT_UNDERWATER))) {
      perform_move(ch, door, 1);
    }
    if (GET_POS(ch) != GET_DEFAULT_POS(ch)) {
      if (GET_POS(ch) == POS_SLEEPING && GET_DEFAULT_POS(ch) != POS_SLEEPING) {
        command_interpreter(ch, "wake");
      }
      command_interpreter(ch, resetpos[(int)GET_DEFAULT_POS(ch)]);
    }
    if (!HUNTING(ch) && MOB_FLAGGED(ch, MOB_SENTINEL) && ch->in_room != ch->loadin) {
      HUNTINGRM(ch) = ch->loadin;
    }

    /* Aggressive Mobs */
    if (MOB_FLAGGED(ch, MOB_AGGRESSIVE | MOB_AGGR_TO_ALIGN)) {
      found = FALSE;
      for (vict = world[ch->in_room].people; vict && !found; vict = vict->next_in_room) {
        if (!CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE) || AFF2_FLAGGED(vict, AFF2_WRAITHFORM))
          continue;
        if ((IS_NPC(vict) && !AFF_FLAGGED(vict, AFF_CHARM)) || (AFF_FLAGGED(vict, AFF_CHARM) && IS_NPC(vict->master)))
          continue;
        if (MOB_FLAGGED(ch, MOB_WIMPY) && AWAKE(vict))
          continue;
        if (AFF2_FLAGGED(ch, AFF2_MINOR_PARALIZED) || AFF_FLAGGED(ch, AFF_MAJOR_PARALIZED))
          continue;
        if (!MOB_FLAGGED(ch, MOB_AGGR_TO_ALIGN) || (MOB_FLAGGED(ch, MOB_AGGR_EVIL) && IS_EVIL(vict)) ||
            (MOB_FLAGGED(ch, MOB_AGGR_NEUTRAL) && IS_NEUTRAL(vict)) ||
            (MOB_FLAGGED(ch, MOB_AGGR_GOOD) && IS_GOOD(vict))) {
          if (GET_SKILL(vict, skillnum) > number(0, 101)) {
            improve_skill(vict, skillnum, SKUSE_AVERAGE);
            hit(vict, ch, TYPE_UNDEFINED);
          } else
            hit(ch, vict, TYPE_UNDEFINED);
          found = TRUE;
        }
      }
    }
    /* Mob Memory */
    if (MOB_FLAGGED(ch, MOB_MEMORY) && MEMORY(ch)) {
      found = FALSE;
      for (vict = world[ch->in_room].people; vict && !found; vict = vict->next_in_room) {
        if (IS_NPC(vict) || !CAN_SEE(ch, vict) || PRF_FLAGGED(vict, PRF_NOHASSLE))
          continue;
        for (names = MEMORY(ch); names && !found; names = names->next)
          if (names->id == GET_IDNUM(vict)) {
            found = TRUE;
            if (GET_CLASS(ch) == CLASS_HUMANOID)
              act("'Hey!  You're the fiend that attacked me!!!', exclaims $n.", FALSE, ch, 0, 0, TO_ROOM);
            hit(ch, vict, TYPE_UNDEFINED);
          }
      }
    }

    /* Helper Mobs */
    if (MOB_FLAGGED(ch, MOB_HELPER)) {
      found = FALSE;
      for (vict = world[ch->in_room].people; vict && !found; vict = vict->next_in_room) {
        if (ch && vict && ch != vict && IS_NPC(vict) && GET_MOB_RACE(vict) == GET_MOB_RACE(ch) && FIGHTING(vict) &&
            CAN_SEE(ch, vict) && CAN_SEE(ch, FIGHTING(vict)) && !AFF_FLAGGED(ch, AFF_CHARM)) {
          if (!IS_NPC(FIGHTING(vict)) || (IS_NPC(FIGHTING(vict)) && AFF_FLAGGED(FIGHTING(vict), AFF_CHARM))) {
            act("$n jumps to the aid of $N!", FALSE, ch, 0, vict, TO_ROOM);
            hit(ch, FIGHTING(vict), TYPE_UNDEFINED);
            found = TRUE;
          }
        }
      }
    }

    if (MOB_FLAGGED(ch, MOB_WATERONLY)) {
      found = FALSE;
      if (!(world[ch->in_room].sector_type == SECT_WATER_NOSWIM || world[ch->in_room].sector_type == SECT_WATER_SWIM ||
            world[ch->in_room].sector_type == SECT_UNDERWATER)) {
        act("$n lies on the ground, suffering from being on dry land!", FALSE, ch, 0, 0, TO_ROOM);
        act("$n chokes to death!", FALSE, ch, 0, 0, TO_ROOM);
        send_to_char("Argh! Where is the water? This is not the right element for you!\r\nYou fall down on the ground, "
                     "in terrible pain! You are dead!\r\n",
                     ch);
        raw_kill(ch, ch);
      }
    }
    if (IS_MOB(ch) && ch->master && isname("elemental", GET_NAME(ch)) && !AFF_FLAGGED(ch, AFF_CHARM)) {
      if (isname("water", GET_NAME(ch))) {
        act("With a splash, $n crashes into the ground leaving only a puddle behind.", FALSE, ch, 0, 0, TO_ROOM);
      } else if (isname("fire", GET_NAME(ch))) {
        act("With a loud puffing sound, $n disolves in a puff of smoke!", FALSE, ch, 0, 0, TO_ROOM);
      } else if (isname("earth", GET_NAME(ch))) {
        act("With a loud crash, $n dives into the ground. Only small stones remain where it once stood.", FALSE, ch, 0,
            0, TO_ROOM);
      } else if (isname("air", GET_NAME(ch))) {
        act("$n dissipates in a gust of wind!", FALSE, ch, 0, 0, TO_ROOM);
      }
      extract_char(ch, 1);
    }

    /* Add new mobile actions here  - non combat*/

  } /* end for() */

  for (obj = object_list; obj; obj = best_obj) {
    best_obj = obj->next; /* Next in object list */
    if (GET_OBJ_SPEC(obj) != NULL && IS_SET(SPEC_OBJ_TYPE(obj), SPEC_HEARTBEAT))
      GET_OBJ_SPEC(obj)(0, obj, 0, "", SPEC_HEARTBEAT);
  } /* end obj for() */
}

/* Mob Memory Routines */

/* make ch remember victim */
void remember(struct char_data *ch, struct char_data *victim) {
  memory_rec *tmp;
  bool present = FALSE;

  if (!IS_NPC(ch) || IS_NPC(victim))
    return;

  for (tmp = MEMORY(ch); tmp && !present; tmp = tmp->next)
    if (tmp->id == GET_IDNUM(victim))
      present = TRUE;

  if (!present) {
    CREATE(tmp, memory_rec, 1);
    tmp->next = MEMORY(ch);
    tmp->id = GET_IDNUM(victim);
    MEMORY(ch) = tmp;
  }
}

/* make ch forget victim */
void forget(struct char_data *ch, struct char_data *victim) {
  memory_rec *curr = NULL, *prev = NULL;

  if (!ch || !victim) {
    return;
  }

  if (!(curr = MEMORY(ch))) {
    return;
  }

  while (curr && (curr->id != GET_IDNUM(victim))) {
    prev = curr;
    curr = curr->next;
  }

  if (!curr)
    return; /* person wasn't there at all. */

  if (curr == MEMORY(ch))
    MEMORY(ch) = curr->next;
  else
    prev->next = curr->next;

  free(curr);
}

/* erase ch's memory */
void clearMemory(struct char_data *ch) {
  memory_rec *curr, *next;

  curr = MEMORY(ch);

  while (curr) {
    next = curr->next;
    free(curr);
    curr = next;
  }

  MEMORY(ch) = NULL;
}

void perform_mob_hunt() {
  register struct char_data *ch, *next_ch;

  for (ch = character_list; ch; ch = next_ch) {
    next_ch = ch->next;

    /* Mob hunting */
    if (HUNTING(ch) && !MOUNTED_BY(ch) && (GET_POS(ch) == POS_STANDING)) {
      hunt_aggro(ch);
    }

    /* Mob hunting room */
    if ((HUNTINGRM(ch) >= 0) && !MOUNTED_BY(ch) && (GET_POS(ch) == POS_STANDING)) {
      hunt_room(ch);
    }
  }
}

void perform_mobprog_activity() {
  register struct char_data *ch, *next_ch;

  for (ch = character_list; ch; ch = next_ch) {
    next_ch = ch->next;

    if ((!IS_MOB(ch) || FIGHTING(ch)) || (IS_NPC(ch) && ch->mob_specials.mp_toggle))
      continue;

    if (IS_NPC(ch) && ch->mpactnum > 0) {
      MPROG_ACT_LIST *tmp_act, *tmp2_act;
      if (!is_empty(world[ch->in_room].zone)) {
        for (tmp_act = ch->mpact; tmp_act != NULL; tmp_act = tmp_act->next) {
          mprog_wordlist_check(tmp_act->buf, ch, tmp_act->ch, tmp_act->obj, tmp_act->vo, ACT_PROG);
          free(tmp_act->buf);
        }
        for (tmp_act = ch->mpact; tmp_act != NULL; tmp_act = tmp2_act) {
          tmp2_act = tmp_act->next;
          free(tmp_act);
        }
        ch->mpactnum = 0;
        ch->mpact = NULL;
      } else {
        for (tmp_act = ch->mpact; tmp_act != NULL; tmp_act = tmp2_act) {
          tmp2_act = tmp_act->next;
          free(tmp_act->buf);
          free(tmp_act);
        }
        ch->mpactnum = 0;
        ch->mpact = NULL;
      }
    }
  }
}
