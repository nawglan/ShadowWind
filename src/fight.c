/* ************************************************************************
 *   File: fight.c                                       Part of CircleMUD *
 *  Usage: Combat system                                                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "comm.h"
#include "db.h"
#include "event.h"
#include "handler.h"
#include "interpreter.h"
#include "screen.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"

extern struct spell_info_type *spells;
struct char_data *combat_list = NULL; /* head of l-list of fighting chars */
struct char_data *next_combat_list = NULL;

/* External structures */
extern int pulse;
extern struct room_data *world;
extern struct message_list fight_messages[MAX_MESSAGES];
extern struct obj_data *object_list;
extern struct index_data *obj_index;
extern int MaxExperience[LVL_IMMORT + 2];
extern sh_int r_mortal_start_room;
extern int pk_allowed;   /* see config.c */
extern int auto_save;    /* see config.c */
extern int max_exp_gain; /* see config.c */
extern int death_count;
extern struct char_data *character_list;
extern const sh_int monk_stat[LVL_IMMORT + 1][5];

/* External procedures */
void improve_skill(struct char_data *ch, int skill, int chance);
int reduce_affect(struct char_data *victim, int aff_type, int dam);
int find_skill_num(char *name);
int find_spell_num(char *name);
char *fread_action(FILE *fl, int nr);
char *fread_string(FILE *fl, char *error);
void stop_follower(struct char_data *ch);
ACMD(do_flee);
void hit(struct char_data *ch, struct char_data *victim, int type);
void multiple_hit(struct char_data *ch, struct char_data *vict, int type);
void forget(struct char_data *ch, struct char_data *victim);
void remember(struct char_data *ch, struct char_data *victim);
int ok_damage_shopkeeper(struct char_data *ch, struct char_data *victim);
void mprog_hitprcnt_trigger(struct char_data *mob, struct char_data *ch);
void mprog_death_trigger(struct char_data *mob, struct char_data *killer);
void mprog_fight_trigger(struct char_data *mob, struct char_data *ch);
extern int is_empty(int zone_nr);
extern int max_obj_time;
bool check_events(void *pointer, EVENT(*func));
bool clean_events(void *pointer, EVENT(*func));
EVENT(camp);
int get_weapon_speed(struct char_data *ch, int type);

/* Weapon attack texts */
struct attack_hit_type attack_hit_text[] = {
    {"hit", "hits", DAM_BLUDGEON},           /* 0 Barehanded    */
    {"pierce", "pierces", DAM_PIERCE},       /* 1 Shortsword    */
    {"slash", "slashes", DAM_SLASH},         /* 2 Broadsword    */
    {"slash", "slashes", DAM_SLASH},         /* 3 Longsword     */
    {"chop", "chops", DAM_SLASH},            /* 4 Handaxe       */
    {"cleave", "cleaves", DAM_SLASH},        /* 5 Battleaxe     */
    {"pound", "pounds", DAM_BLUDGEON},       /* 6 Quarterstaff  */
    {"cleave", "cleaves", DAM_SLASH},        /* 7 Polearm       */
    {"crush", "crushes", DAM_BLUDGEON},      /* 8 Mace          */
    {"smash", "smashes", DAM_BLUDGEON},      /* 9 Warhammer     */
    {"crush", "crushes", DAM_BLUDGEON},      /* 10 Morning star */
    {"pound", "pounds", DAM_BLUDGEON},       /* 11 Club         */
    {"stab", "stabs", DAM_PIERCE},           /* 12 Dagger       */
    {"whip", "whips", DAM_BLUDGEON},         /* 13 Whip         */
    {"slash", "slashes", DAM_SLASH},         /* 14 Two-handed   */
    {"pierce", "pierces", DAM_PIERCE},       /* 15 Bow          */
    {"pierce", "pierces", DAM_PIERCE},       /* 16 Spear        */
    {"hit", "hits", DAM_UNDEFINED},          /* 17 Spare 1      */
    {"hit", "hits", DAM_UNDEFINED},          /* 18 Spare 2      */
    {"hit", "hits", DAM_UNDEFINED},          /* 19 Spare 3      */
    {"hit", "hits", DAM_UNDEFINED},          /* 20 Spare 4      */
    {"sting", "stings", DAM_PIERCE},         /* 21 for mobs     */
    {"bite", "bites", DAM_PIERCE},           /* 22 for mobs     */
    {"bludgeon", "bludgeons", DAM_BLUDGEON}, /* 23 for mobs     */
    {"claw", "claws", DAM_SLASH},            /* 24 for mobs     */
    {"maul", "mauls", DAM_SLASH},            /* 25 for mobs     */
    {"blast", "blasts", DAM_BLUDGEON},       /* 26 for mobs     */
    {"punch", "punches", DAM_BLUDGEON},      /* 27 for mobs     */
    {"stab", "stabs", DAM_PIERCE}            /* 28 for mobs     */
};

int weapon_deadly[] = {
    /* deadliness for each weapon type */
    100, /* barehand     */
    100, /* shortsword   */
    100, /* broadsword   */
    100, /* longsword    */
    100, /* handaxe      */
    100, /* battleaxe    */
    100, /* quarterstaff */
    100, /* polearm      */
    100, /* mace         */
    100, /* warhammer    */
    100, /* Morning star */
    100, /* Club         */
    100, /* Dagger       */
    100, /* Whip         */
    100, /* Two-Handed   */
    100, /* Bow          */
    100  /* Spear        */
};

int weapon_speed[] = {
    /* speed for each weapon type */
    100, /* barehand     */
    90,  /* shortsword   */
    110, /* broadsword   */
    100, /* longsword    */
    100, /* handaxe      */
    110, /* battleaxe    */
    100, /* quarterstaff */
    100, /* polearm      */
    105, /* mace         */
    90,  /* warhammer    */
    105, /* Morning star */
    105, /* Club         */
    80,  /* Dagger       */
    110, /* Whip         */
    120, /* Two-Handed   */
    110, /* Bow          */
    110  /* Spear        */
};

#define IS_WEAPON(type) (((type) >= TYPE_HIT) && ((type) < TYPE_SUFFERING))

/* The Fight related routines */

/* calculates the PC attack timer */

void multiple_hit(struct char_data *ch, struct char_data *vict, int type) {
  int i;
  int j;
  int multiattack = spells[find_skill_num("multi attack")].spellindex;
  int maxnumhits = 3;

  if (GET_SKILL(ch, multiattack) && !IS_NPC(ch) && !IS_MONK(ch)) {
    if (FIGHTING(ch) && (GET_POS(vict) > POS_DEAD)) {
      hit(ch, vict, type);
      if (!ch || !vict) {
        return;
      }
      if (AFF2_FLAGGED(ch, AFF2_HASTE)) {
        hit(ch, vict, type);
        if (!ch || !vict) {
          return;
        }
        maxnumhits += number(0, 1);
      }
      for (i = 0; i < maxnumhits; i++) {
        if (number(1, 41) < (GET_SKILL(ch, multiattack) - (i * 45))) {
          if (FIGHTING(ch) && (GET_POS(vict) > POS_DEAD)) {
            hit(ch, vict, type);
            if (!ch || !vict) {
              return;
            }
          }
        }
      }
    }
    improve_skill(ch, multiattack, SKUSE_FREQUENT);
  } else if (GET_SKILL(ch, multiattack) && !IS_NPC(ch) && IS_MONK(ch)) {
    j = (GET_LEVEL(ch) / 10);
    if (AFF2_FLAGGED(ch, AFF2_HASTE)) {
      j += number(0, 1);
    }
    if (FIGHTING(ch) && (GET_POS(vict) > POS_DEAD)) {
      hit(ch, vict, type);
      if (!ch || !vict) {
        return;
      }
      if (GET_EQ(ch, WEAR_WIELD) || GET_EQ(ch, WEAR_WIELD_2) || GET_EQ(ch, WEAR_HOLD) || GET_EQ(ch, WEAR_HOLD_2) ||
          GET_EQ(ch, WEAR_2HANDED) || GET_EQ(ch, WEAR_SHIELD)) {
        /* monks only get 1 attack if something is in their hand */
        return;
      }
      if (AFF2_FLAGGED(ch, AFF2_HASTE)) {
        hit(ch, vict, type);
        if (!ch || !vict) {
          return;
        }
        j += number(0, 1);
      }
      for (i = 0; i < j; i++) {
        if (number(1, 200) < (GET_SKILL(ch, multiattack) + 80)) {
          if (FIGHTING(ch) && (GET_POS(vict) > POS_DEAD)) {
            hit(ch, vict, type);
            if (!ch || !vict) {
              return;
            }
          }
        }
      }
    }
    improve_skill(ch, multiattack, SKUSE_FREQUENT);
  } else if (IS_NPC(ch)) {
    if (AFF2_FLAGGED(ch, AFF2_HASTE)) {
      hit(ch, vict, type);
      if (!ch || !vict) {
        return;
      }
      if (number(0, 1) && vict && ch) {
        hit(ch, vict, type);
      }
      if (!ch || !vict) {
        return;
      }
    }
    j = get_weapon_speed(ch, type);
    if (j > 100) {
      struct mob_attacks_data *attack = ch->mob_specials.mob_attacks;

      while (attack && attack->fight_timer) {
        attack = attack->next;
      }
      if (!attack) {
        attack = ch->mob_specials.mob_attacks;
      }
      j = attack->slow_attack;
      if (j >= get_weapon_speed(ch, type)) {
        if (FIGHTING(ch) && (GET_POS(vict) > POS_DEAD)) {
          hit(ch, vict, type);
          if (!ch || !vict) {
            return;
          }
        }
        attack->slow_attack -= get_weapon_speed(ch, type);
      } else {
        if (FIGHTING(ch) && (GET_POS(vict) > POS_DEAD)) {
          attack->slow_attack += 100;
        } else {
          attack->slow_attack = 0;
        }
      }
    } else {
      if (!ch || !vict) {
        return;
      }
      for (i = 0; i < BOUNDED(1, 100 / get_weapon_speed(ch, type), 5); i++) {
        if (FIGHTING(ch) && (GET_POS(vict) > POS_DEAD)) {
          hit(ch, vict, type);
        }
        if (!ch || !vict) {
          return;
        }
      }
    }
  } else {
    hit(ch, vict, type);
    if (!ch || !vict) {
      return;
    }
    if (AFF2_FLAGGED(ch, AFF2_HASTE)) {
      hit(ch, vict, type);
      if (!ch || !vict) {
        return;
      }
      if (number(0, 1)) {
        hit(ch, vict, type);
      }
      if (!ch || !vict) {
        return;
      }
    }
  }
}

int get_weapon_speed(struct char_data *ch, int type) {
  int w_type;
  int weight;
  int speed;
  struct obj_data *wielded;
  if (ch == NULL) {
    return 100;
  }

  wielded = ch->equipment[WEAR_WIELD];
  if (type == TYPE_DUAL_WIELD)
    wielded = ch->equipment[WEAR_WIELD_2];

  if (IS_NPC(ch)) {
    struct mob_attacks_data *attack = ch->mob_specials.mob_attacks;
    while (attack && attack->fight_timer) {
      attack = attack->next;
    }
    if (!attack) {
      attack = ch->mob_specials.mob_attacks;
    }
    speed = attack->attacks;

    return speed;
  } else {
    if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
      w_type = GET_OBJ_VAL(wielded, 3);
      weight = GET_OBJ_WEIGHT(wielded);
      speed = weight * GET_STR(ch) / weapon_speed[w_type];
    } else {
      /* barehanded */
      speed = 1 + (GET_STR(ch) / weapon_speed[0]);
    }
  }

  return (BOUNDED(1, speed, 100));
}

void appear(struct char_data *ch) {
  int invisibility = spells[find_spell_num("invisibility")].spellindex;
  int massinvisibility = spells[find_spell_num("mass invisibility")].spellindex;

  act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);

  if (affected_by_spell(ch, invisibility)) {
    affect_from_char(ch, invisibility);
  }

  if (affected_by_spell(ch, massinvisibility)) {
    affect_from_char(ch, massinvisibility);
  }

  REMOVE_BIT(AFF_FLAGS(ch), AFF_INVISIBLE | AFF_HIDE);
}

void load_messages(void) {
  FILE *fl;
  int i, type;
  struct message_type *messages;
  extern int numspells;
  char chk[128];
  char *cp;

  if (!(fl = fopen(MESS_FILE, "r"))) {
    safe_snprintf(g_buf2, MAX_STRING_LENGTH, "Error reading combat message file %s", MESS_FILE);
    perror(g_buf2);
    fflush(NULL);
    exit(1);
  }
  for (i = 0; i < MAX_MESSAGES; i++) {
    fight_messages[i].a_type = 0;
    fight_messages[i].number_of_attacks = 0;
    fight_messages[i].msg = 0;
  }

  fgets(chk, 128, fl);
  while (!feof(fl) && (*chk == '\n' || *chk == '*')) {
    fgets(chk, 128, fl);
  }

  while (*chk == 'M') {
    fgets(chk, 128, fl);
    cp = chk;
    cp[strlen(cp) - 1] = '\0';
    while (*cp == ' ') {
      cp++;
    }
    if (isdigit(*cp)) {
      type = atoi(cp);
    } else {
      type = find_spell_num(cp);
      if (type < 0) {
        type = find_skill_num(cp);
      }
    }
    if (type < 0) {
      safe_snprintf(g_buf2, MAX_STRING_LENGTH, "Bad combat message: %s", cp);
      perror(g_buf2);
      fgets(chk, 128, fl);
      while (!feof(fl) && (*chk != 'M')) {
        fgets(chk, 128, fl);
      }
      continue;
    }
    if (type >= 0 && type < numspells) {
      type = spells[type].spellindex;
    }
    for (i = 0; (i < MAX_MESSAGES) && (fight_messages[i].a_type != type) && (fight_messages[i].a_type); i++)
      ;
    if (i >= MAX_MESSAGES) {
      fprintf(stderr, "Too many combat messages. Increase MAX_MESSAGES and recompile.");
      fflush(NULL);
      exit(1);
    }
    CREATE(messages, struct message_type, 1);
    fight_messages[i].number_of_attacks++;
    fight_messages[i].a_type = type;
    messages->next = fight_messages[i].msg;
    fight_messages[i].msg = messages;

    messages->die_msg.attacker_msg = fread_action(fl, i);
    messages->die_msg.victim_msg = fread_action(fl, i);
    messages->die_msg.room_msg = fread_action(fl, i);
    messages->miss_msg.attacker_msg = fread_action(fl, i);
    messages->miss_msg.victim_msg = fread_action(fl, i);
    messages->miss_msg.room_msg = fread_action(fl, i);
    messages->hit_msg.attacker_msg = fread_action(fl, i);
    messages->hit_msg.victim_msg = fread_action(fl, i);
    messages->hit_msg.room_msg = fread_action(fl, i);
    fgets(chk, 128, fl);
    while (!feof(fl) && (*chk == '\n' || *chk == '*')) {
      fgets(chk, 128, fl);
    }
  }

  fclose(fl);
}

void update_pos(struct char_data *victim) {
  if ((GET_HIT(victim) > 0) && (GET_POS(victim) > POS_STUNNED)) {
    return;
  } else if (GET_HIT(victim) > 0) {
    GET_POS(victim) = POS_STANDING;
  } else if (GET_HIT(victim) <= -8) {
    GET_POS(victim) = POS_DEAD;
  } else if (GET_HIT(victim) <= -6) {
    GET_POS(victim) = POS_MORTALLYW;
  } else if (GET_HIT(victim) <= -3) {
    GET_POS(victim) = POS_INCAP;
  } else {
    GET_POS(victim) = POS_STUNNED;
  }
}

void check_killer(struct char_data *ch, struct char_data *vict) {
  if (!IS_NPC(ch) && GET_LEVEL(ch) >= LVL_IMMORT && FIGHTING(ch) != vict) {
    safe_snprintf(g_logbuffer, sizeof(g_logbuffer), "IMMORT %s attacked %s", GET_NAME(ch), GET_NAME(vict));
    mudlog(g_logbuffer, 'N', COM_IMMORT, TRUE);
    plog(g_logbuffer, ch, MAX(LVL_IMMORT, GET_LEVEL(ch)));
  }

  if (ch->desc) {
    if (ch->desc->original) {
      if (GET_LEVEL(ch->desc->original) >= LVL_IMMORT && FIGHTING(ch) != vict) {
        safe_snprintf(g_logbuffer, sizeof(g_logbuffer), "IMMORT %s attacked %s", GET_NAME(ch), GET_NAME(vict));
        mudlog(g_logbuffer, 'N', COM_IMMORT, TRUE);
        plog(g_logbuffer, ch, MAX(LVL_IMMORT, GET_LEVEL(ch->desc->original)));
      }
    }
  }

  if (!PLR_FLAGGED(vict, PLR_KILLER) && !PLR_FLAGGED(vict, PLR_THIEF) && !PLR_FLAGGED(ch, PLR_KILLER) && !IS_NPC(ch) &&
      !IS_NPC(vict) && (ch != vict) && (!IS_NPC(ch) && !IS_NPC(vict) && GET_LEVEL(ch) < LVL_IMMORT)) {
    char buf[256];
    if (!pk_allowed) {
      SET_BIT(PLR_FLAGS(ch), PLR_KILLER);
      safe_snprintf(g_buf, sizeof(g_buf), "PC Killer bit set on %s for initiating attack on %s at %s.", GET_NAME(ch),
                    GET_NAME(vict), world[vict->in_room].name);
      mudlog(g_buf, 'Y', COM_IMMORT, TRUE);
      plog(g_buf, ch, 0);
      plog(g_buf, vict, 0);
      GET_PKCOUNT(ch)++;
      send_to_char("If you want to be a PLAYER KILLER, so be it...\r\n", ch);
    } else if (!FIGHTING(ch) || !FIGHTING(vict)) {
      safe_snprintf(g_logbuffer, sizeof(g_logbuffer), "PC %s attacked PC %s at %s", GET_NAME(ch), GET_NAME(vict),
                    world[vict->in_room].name);
      mudlog(g_logbuffer, 'Y', COM_IMMORT, TRUE);
      plog(g_buf, ch, 0);
      plog(g_buf, vict, 0);
    }
  }
}

/* start one char fighting another (yes, it is horrible, I know... )  */
void set_fighting(struct char_data *ch, struct char_data *vict) {
  struct mob_attacks_data *this;
  int i, j;
  int spellnum = spells[find_spell_num("sleep")].spellindex;

  if (ch == vict) {
    return;
  }
  if (FIGHTING(ch)) {
    stderr_log("Error: Trying to start fight when already fighting");
    return;
  }

  ch->next_fighting = combat_list;
  combat_list = ch;

  if (IS_NPC(ch)) {
    this = ch->mob_specials.mob_attacks;
    while (this != NULL) {
      if (!this->fight_timer) {
        this->fight_timer = 1;
      }
      this = this->next;
    }
  } else {
    ch->char_specials.fight_timer = 1;
  }

  if (IS_AFFECTED(ch, AFF_SLEEP)) {
    affect_from_char(ch, spellnum);
  }

  FIGHTING(ch) = vict;
  if (GET_POS(ch) == POS_STANDING) {
    GET_POS(ch) = POS_FIGHTING;
  }
  if (GET_POS(vict) >= POS_SLEEPING && GET_POS(vict) < POS_FIGHTING && IS_NPC(vict) && MOB_FLAGGED(vict, MOB_WIMPY) &&
      GET_MOB_WAIT(vict) == 0) {
    act("$n scrambles madly to $s feet!", TRUE, vict, 0, 0, TO_ROOM);
    GET_POS(vict) = POS_FIGHTING;
  } else if (GET_POS(vict) == POS_STANDING) {
    GET_POS(vict) = POS_FIGHTING;
  }

  check_killer(ch, vict);

  if (IS_NPC(vict) && !IS_NPC(ch)) { /* ch initiates attack on mob */
    j = 0;
    for (i = 0; i < 20; i++) {
      /* player already in attacked_by list */
      if (vict->mob_specials.attacked_by[i] == GET_IDNUM(ch)) {
        /* update level just in case */
        vict->mob_specials.attacked_levels[i] = GET_LEVEL(ch);
        return;
      }
      if (vict->mob_specials.attacked_by[i] == 0 && j == 0) {
        j = i; /* store the value of the free slot */
      }
    } /* end for() */

    /* okay add the player to the list..   */
    vict->mob_specials.attacked_by[j] = GET_IDNUM(ch);
    vict->mob_specials.attacked_levels[j] = GET_LEVEL(ch);

    /* Set the mob to hunt the PC if it is a tracker */
    if (MOB_FLAGGED(vict, MOB_TRACKER))
      HUNTING(vict) = GET_IDNUM(ch);
  } /* end if() */

  if (!IS_NPC(vict) && IS_NPC(ch)) { /* mob initates attack on ch */
    j = 0;
    for (i = 0; i < 20; i++) {
      /* player already in attacked_by list */
      if (ch->mob_specials.attacked_by[i] == GET_IDNUM(vict)) {
        /* update level just in case */
        ch->mob_specials.attacked_levels[i] = GET_LEVEL(vict);
        return;
      }
      if (ch->mob_specials.attacked_by[i] == 0 && j == 0) {
        j = i; /* store the value of the free slot */
      }
    } /* end for() */
    /* okay add the player to the list..   */
    ch->mob_specials.attacked_by[j] = GET_IDNUM(vict);
    ch->mob_specials.attacked_levels[j] = GET_LEVEL(vict);
  } /* end if() */
}

/* remove a char from the list of fighting chars */
void stop_fighting(struct char_data *ch) {
  struct char_data *tmp;
  struct char_data *tmp2;

  if (!IS_NPC(ch)) {
    GET_ATTACKED(ch) = 0;
    SKILL_TIMER(ch) = 0;
    WAIT_STATE(ch, (ch->char_specials.fightwait * 15));
  }

  if (!FIGHTING(ch)) {
    return;
  }

  if (ch == next_combat_list) {
    next_combat_list = ch->next_fighting;
  }

  if (combat_list == ch) {
    combat_list = ch->next_fighting;
  } else {
    for (tmp = combat_list; tmp && (tmp->next_fighting != ch); tmp = tmp2) {
      tmp2 = tmp->next_fighting;
    }
    if (!tmp) {
      stderr_log("SYSERR: Char fighting not found Error (fight.c, stop_fighting)");
    }
    tmp->next_fighting = ch->next_fighting;
  }

  if (!IS_NPC(ch)) {
    GET_ATTACKED(ch) = 0;
    SKILL_TIMER(ch) = 0;
    WAIT_STATE(ch, (ch->char_specials.fightwait * 15));
  }

  ch->char_specials.fightwait = 0;

  ch->next_fighting = NULL;
  FIGHTING(ch) = NULL;
  if (GET_POS(ch) == POS_FIGHTING) {
    GET_POS(ch) = POS_STANDING;
  }
  update_pos(ch);
}

void make_corpse(struct char_data *ch) {
  struct obj_data *corpse, *o;
  struct obj_data *money;
  char temp[50];
  int i;
  int has_money = GET_PLAT(ch) || GET_GOLD(ch) || GET_SILVER(ch) || GET_COPPER(ch);
  extern int max_npc_corpse_time, max_pc_corpse_time;

  struct obj_data *create_money(int plat, int gold, int silver, int copper);

  if (IS_NPC(ch) && GET_CLASS(ch) == CLASS_ELEMENTAL) {
    return;
  }
  corpse = read_object(1203, VIRTUAL);
  if (corpse->cname) {
    FREE(corpse->cname);
    corpse->cname = NULL;
  }
  if (corpse->cdescription) {
    FREE(corpse->cdescription);
    corpse->cdescription = NULL;
  }
  if (corpse->cshort_description) {
    FREE(corpse->cshort_description);
    corpse->cshort_description = NULL;
  }
  if (corpse->owner) {
    FREE(corpse->owner);
    corpse->owner = NULL;
  }

  corpse->in_room = NOWHERE;
  if (!IS_NPC(ch)) {
    safe_snprintf(temp, sizeof(temp), "%s pcorpse", GET_NAME(ch));
    corpse->cname = strdup(temp);
    corpse->owner = strdup(GET_NAME(ch));
  } else {
    corpse->cname = strdup("corpse");
  }

  safe_snprintf(g_buf2, MAX_STRING_LENGTH, "The corpse of %s is lying here.", GET_NAME(ch));
  corpse->cdescription = strdup(g_buf2);

  safe_snprintf(g_buf2, MAX_STRING_LENGTH, "the corpse of %s", GET_NAME(ch));
  corpse->cshort_description = strdup(g_buf2);

  GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
  GET_OBJ_EXTRA(corpse) = ITEM_NODONATE;
  GET_OBJ_VAL(corpse, 0) = 0; /* You can't store stuff in a corpse */
  if (IS_MOB(ch) && GET_CLASS(ch) == CLASS_UNDEAD) {
    GET_OBJ_VAL(corpse, 3) = GET_LEVEL(ch); /* corpse identifier */
  } else {
    GET_OBJ_VAL(corpse, 3) = 1; /* corpse identifier */
  }
  GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch) + IS_CARRYING_W(ch);
  if (IS_NPC(ch)) {
    GET_OBJ_TIMER(corpse) = max_npc_corpse_time / number(1, 2);
  } else {
    SET_BIT(GET_OBJ_EXTRA(corpse), ITEM_CARRIED);
    GET_OBJ_TIMER(corpse) = max_pc_corpse_time;
    GET_OBJ_TYPE(corpse) = ITEM_PCORPSE;
  }
  /* transfer character's inventory to the corpse */
  corpse->contains = ch->carrying;
  for (o = corpse->contains; o != NULL; o = o->next_content) {
    o->in_obj = corpse;
    SET_BIT(GET_OBJ_EXTRA(o), ITEM_CARRIED);
    GET_OBJ_TIMER(o) = max_obj_time;
  } /* item in circulation */
  object_list_new_owner(corpse, NULL);

  /* transfer character's equipment to the corpse */
  for (i = 0; i < NUM_WEARS; i++) {
    if (ch->equipment[i]) {
      /* item is in circulation, enable timer */
      SET_BIT(GET_OBJ_EXTRA(ch->equipment[i]), ITEM_CARRIED);
      GET_OBJ_TIMER(ch->equipment[i]) = max_obj_time;
      obj_to_obj(unequip_char(ch, i), corpse);
    }
  }
  /* transfer gold */
  if (has_money) {
    /* following 'if' clause added to fix gold duplication loophole */
    if (IS_NPC(ch) || (!IS_NPC(ch) && ch->desc)) {
      money = create_money(GET_PLAT(ch), GET_GOLD(ch), GET_SILVER(ch), GET_COPPER(ch));
      if (money) {
        obj_to_obj(money, corpse);
      }
    }
    GET_PLAT(ch) = 0;
    GET_GOLD(ch) = 0;
    GET_SILVER(ch) = 0;
    GET_COPPER(ch) = 0;
    GET_TEMP_GOLD(ch) = 0;
  }
  ch->carrying = NULL;
  IS_CARRYING_N(ch) = 0;
  IS_CARRYING_W(ch) = 0;

  if (ch->in_room != 1) {
    obj_to_room(corpse, ch->in_room);
  } else {
    obj_to_room(corpse, ch->was_in_room);
  }
  if (!IS_NPC(ch)) {
    corpsesaveall();
  }
}

/* When ch kills victim */
void change_alignment(struct char_data *ch, struct char_data *victim) {
  int amount = 0;

  if (GET_ALIGNMENT(victim) < GET_ALIGNMENT(ch)) {
    if (number(1, 10) >= 9) {
      amount = 1;
    }
  } else if (GET_ALIGNMENT(victim) > GET_ALIGNMENT(ch)) {
    if (number(1, 10) >= 9) {
      amount = -1;
    }
  } else /* == */ {
    if (number(1, 10) == 1) {
      amount = 1;
    } else if (number(1, 10) == 3) {
      amount = -1;
    }
  }

  GET_ALIGNMENT(ch) = BOUNDED(-1000, GET_ALIGNMENT(ch) + amount, 1000);
}

void death_cry(struct char_data *ch) {
  int door, was_in;

  act("Your blood freezes as you hear $n's death cry.", FALSE, ch, 0, 0, TO_ROOM);
  was_in = ch->in_room;

  for (door = 0; door < NUM_OF_DIRS; door++) {
    if (CAN_GO(ch, door)) {
      ch->in_room = world[was_in].dir_option[door]->to_room;
      act("Your blood freezes as you hear someone's death cry.", FALSE, ch, 0, 0, TO_ROOM);
      ch->in_room = was_in;
    }
  }
}

void raw_kill(struct char_data *ch, struct char_data *killer) {
  if (killer == NULL) {
    killer = ch;
  }

  if (FIGHTING(ch)) {
    stop_fighting(ch);
  }

  if (FIGHTING(killer) == ch) {
    stop_fighting(killer);
  }

  SKILL_TIMER(killer) = 0;
  /* remove memories of this char and stop hunting */
  while (ch->affected) {
    affect_remove(ch, ch->affected);
  }

  /* Hopefully puts an end to a crash bug where dead ppls spells
   go off before they're extracted */
  clean_events(ch, NULL);

  if (killer) {
    mprog_death_trigger(ch, killer);
  }

  if (!AFF2_FLAGGED(ch, AFF2_WRAITHFORM)) {
    make_corpse(ch);
  }
  extract_char(ch, 0);
}

void die(struct char_data *ch, struct char_data *killer) {
  ACMD(do_get);

  if (killer == NULL) {
    killer = ch;
  }
  if (!IS_NPC(ch)) {
    REMOVE_BIT(AFF2_FLAGS(ch), AFF2_CASTING);
    REMOVE_BIT(PLR_FLAGS(ch), PLR_KILLER | PLR_THIEF);
    REMOVE_BIT(PLR_FLAGS(ch), PLR_LOADROOM);
  }

  if (!(!IS_NPC(killer) && !IS_NPC(ch))) {
    safe_snprintf(g_logbuffer, sizeof(g_logbuffer), "%s killed by %s", GET_NAME(ch), GET_NAME(killer));
    mudlog(g_logbuffer, 'O', COM_IMMORT, FALSE);
  } else if ((!IS_NPC(killer) && GET_LEVEL(killer) == LVL_IMMORT)) {
    safe_snprintf(g_logbuffer, sizeof(g_logbuffer), "IMMORT %s killed MOBILE %s", GET_NAME(killer), GET_NAME(ch));
    mudlog(g_logbuffer, 'N', COM_ADMIN, TRUE);
    plog(g_logbuffer, killer, MAX(LVL_IMMORT, GET_LEVEL(killer)));
  } else if (killer->desc) {
    if (killer->desc->original) {
      if (!IS_NPC(killer->desc->original) && GET_LEVEL(killer->desc->original) == LVL_IMMORT) {
        safe_snprintf(g_logbuffer, sizeof(g_logbuffer), "IMMORT %s killed MOBILE %s", GET_NAME(killer), GET_NAME(ch));
        mudlog(g_logbuffer, 'N', COM_IMMORT, TRUE);
        plog(g_logbuffer, killer, MAX(LVL_IMMORT, GET_LEVEL(killer)));
      }
    }
  } else {
    safe_snprintf(g_logbuffer, sizeof(g_logbuffer), "PC %s killed by PC %s", GET_NAME(ch), GET_NAME(killer));
    mudlog(g_logbuffer, 'Y', COM_IMMORT, FALSE);
  }

  if (!IS_NPC(ch)) {
    death_count++;
    GET_DEATHCOUNT(ch)++;
    if (GET_LEVEL(ch) == 1)
      REMOVE_BIT(PLR_FLAGS(ch), PLR_KIT);
  }
  gain_exp(ch, -(GET_EXP(ch) / 25));
  raw_kill(ch, killer);
  if (killer && !IS_NPC(killer) && killer != ch) {
    if (PRF_FLAGGED(killer, PRF_AUTOGOLD)) {
      do_get(killer, "coins corpse", 0, 0);
    }
    if (PRF_FLAGGED(killer, PRF_AUTOLOOT)) {
      do_get(killer, "all corpse", 0, 0);
    }
  }
}

#define EXP_CAP       150000
#define EXP_CAP_1     250000
#define EXP_CAP_2     350000
#define EXP_CAP_3     450000
#define EXP_CAP_4     500000
#define EXP_CAP_5     550000
#define EXP_CAP_6     600000
#define EXP_CAP_7     700000
#define EXP_CAP_8     800000
#define EXP_CAP_OTHER 1000000

long ExpCaps(int group_count, long total) {
  if (group_count >= 1) {
    switch (group_count) {
    case 1:
      if (total > EXP_CAP_1) {
        total = EXP_CAP_1;
      }
      break;
    case 2:
      if (total > EXP_CAP_2) {
        total = EXP_CAP_2;
      }
      break;
    case 3:
      if (total > EXP_CAP_3) {
        total = EXP_CAP_3;
      }
      break;
    case 4:
      if (total > EXP_CAP_4) {
        total = EXP_CAP_4;
      }
      break;
    case 5:
      if (total > EXP_CAP_5) {
        total = EXP_CAP_5;
      }
      break;
    case 6:
      if (total > EXP_CAP_6) {
        total = EXP_CAP_6;
      }
      break;
    case 7:
      if (total > EXP_CAP_7) {
        total = EXP_CAP_7;
      }
      break;
    case 8:
      if (total > EXP_CAP_8) {
        total = EXP_CAP_8;
      }
      break;
    default:
      if (total > EXP_CAP_OTHER) {
        total = EXP_CAP_OTHER;
      }
      break;
    } /* end switch */
  } else {
    if (total > EXP_CAP) { /* not grouped, so limit max exp gained so */
      total = EXP_CAP;     /* grouping will be used more and benifical */
    }
  }
  return (total);
}

/* More than 10 levels difference, then we knock down */
/* the ratio of EXP he gets, keeping high level people */
/* from getting newbies up to fast... */
long GroupLevelRatioExp(struct char_data *ch, int group_max_level, long experincepoints) {
  unsigned int diff = 0;

  diff = abs(group_max_level - GET_LEVEL(ch));
  if (diff) {
    if (diff >= 10) {
      experincepoints /= 2;
    } else if (diff >= 20) {
      experincepoints /= 3;
    } else if (diff >= 30) {
      experincepoints /= 4;
    } else if (diff >= 40) {
      experincepoints /= 5;
    } else if (diff >= 49) {
      experincepoints /= 6;
    }
  }
  return (experincepoints);
}

int LevelMod(struct char_data *ch, struct char_data *v, int exp) {
  float ratio = 0.0;
  float fexp;

  ratio = (float)GET_LEVEL(v) / GET_LEVEL(ch);
  if (ratio < 1.0) {
    fexp = ratio * exp;
    /* Shouldn't get full exp if mob lower level.
     fexp = exp;
     */
  } else {
    /* changed to help players who go solo out.
     fexp = exp;
     */
    fexp = ratio * exp;
  }
  return ((int)fexp);
}

int RatioExp(struct char_data *ch, struct char_data *victim, int total) {
  if (!MOB_FLAGGED(victim, MOB_AGGRESSIVE) && !IS_AFFECTED(victim, AFF_CHARM)) {
    total = LevelMod(ch, victim, total);
  }
  if (MOB_FLAGGED(victim, MOB_AGGRESSIVE) && !IS_AFFECTED(victim, AFF_CHARM)) {
    /* make sure that poly mages don't abuse, by reducing their bonus */
    if (IS_NPC(ch)) {
      total *= 3;
      total /= 4;
    }
  }
  return (total);
}

void group_gain(struct char_data *ch, struct char_data *victim) {
  int no_members, share;
  struct char_data *k;
  struct follow_type *f;
  int total, pc, group_count = 0;
  int group_max_level = 1; /* the highest level number the group has */

  if (!(k = ch->master)) {
    k = ch;
  }

  if (IS_AFFECTED(k, AFF_GROUP) && (k->in_room == ch->in_room)) {
    no_members = GET_LEVEL(k);
  } else {
    no_members = 0;
  }

  pc = FALSE;
  group_max_level = GET_LEVEL(k);
  for (f = k->followers; f; f = f->next) {
    if (IS_AFFECTED(f->follower, AFF_GROUP) && (f->follower->in_room == ch->in_room)) {
      no_members += GET_LEVEL(f->follower);
      if (!IS_NPC(f->follower)) {
        pc++;
      }
      if (!IS_NPC(f->follower) && f->follower->in_room == k->in_room) {
        if (group_max_level < GET_LEVEL(f->follower)) {
          group_max_level = GET_LEVEL(f->follower);
        }
        group_count++;
      }
    }
  }
  if (pc > 10) {
    pc = 10;
  }
  if (no_members >= 1) {
    share = (GET_EXP(victim) / no_members);
  } else if ((GET_LEVEL(k) >> 2) >= 1) {
    share = (GET_EXP(victim) / (GET_LEVEL(k) >> 2));
  } else {
    share = GET_EXP(victim);
  }

  if (!IS_AFFECTED(k, AFF_GROUP) && ch != victim && ch->in_room == victim->in_room) {
    act("You receive your share of experience.", FALSE, k, 0, 0, TO_CHAR);
    gain_exp(k, share);
    change_alignment(k, victim);
  }
  if (IS_AFFECTED(k, AFF_GROUP) && (k->in_room == ch->in_room)) {
    total = share * GET_LEVEL(k);
    if (pc) {
      total *= (100 + (3 * pc));
      total /= 100;
    }
    total = GroupLevelRatioExp(k, group_max_level, total);
    total = ExpCaps(group_count, total); /* figure EXP MAXES */
    total = RatioExp(k, victim, total);
    gain_exp(k, total);
    change_alignment(k, victim);
    if (GET_POS(ch) != POS_SLEEPING && ch != victim && ch->in_room == victim->in_room) {
      act("You receive your share of experience.", FALSE, k, 0, 0, TO_CHAR);
    }
  }
  for (f = k->followers; f; f = f->next) {
    if (IS_AFFECTED(f->follower, AFF_GROUP) && (f->follower->in_room == ch->in_room)) {
      total = share * GET_LEVEL(f->follower);
      if (!IS_NPC(f->follower)) {
        total *= (100 + pc);
        total /= 100;
      } else {
        total /= 2;
      }
      if (!IS_NPC(f->follower)) {
        total = GroupLevelRatioExp(f->follower, group_max_level, total);
        total = ExpCaps(group_count, total); /* figure EXP MAXES */
        total = RatioExp(f->follower, victim, total);
        act("You receive your share of experience.", FALSE, f->follower, 0, 0, TO_CHAR);
        gain_exp(f->follower, total);
        change_alignment(f->follower, victim);
      } else {
        if (f->follower->master && IS_AFFECTED(f->follower, AFF_CHARM)) {
          total = GroupLevelRatioExp(f->follower, group_max_level, total);
          total = ExpCaps(group_count, total); /* figure EXP MAXES */
          total = RatioExp(f->follower->master, victim, total);
          if (f->follower->master->in_room == f->follower->in_room) {
            act("You receive $N's share of experience.", FALSE, f->follower->master, 0, f->follower, TO_CHAR);
            gain_exp(f->follower->master, total);
            change_alignment(f->follower, victim);
          }
        } else {
          total = GroupLevelRatioExp(f->follower, group_max_level, total);
          total = ExpCaps(group_count, total); /* figure EXP MAXES */
          total = RatioExp(f->follower, victim, total);
          act("You receive your share of experience.", FALSE, f->follower, 0, 0, TO_CHAR);
          gain_exp(f->follower, total);
          change_alignment(f->follower, victim);
        }
      }
    }
  }
}

char *replace_string(char *str, char *weapon_singular, char *weapon_plural) {
  static char g_buf[256];
  char *cp;

  cp = g_buf;

  for (; *str; str++) {
    if (*str == '#') {
      switch (*(++str)) {
      case 'W':
        for (; *weapon_plural; *(cp++) = *(weapon_plural++))
          ;
        break;
      case 'w':
        for (; *weapon_singular; *(cp++) = *(weapon_singular++))
          ;
        break;
      default:
        *(cp++) = '#';
        break;
      }
    } else {
      *(cp++) = *str;
    }

    *cp = 0;
  } /* For */

  return (g_buf);
}

/* message for doing damage with a weapon */
void dam_message(int dam, struct char_data *ch, struct char_data *victim, int w_type, int damorig) {
  char *g_buf;
  int msgnum, weapmin, weapmax, dampercent;
  struct obj_data *weap;

  static struct dam_weapon_type_mob {
    char *to_room;
    char *to_char;
    char *to_victim;
  } npc_dam_weapons[] = {

      /* use #w for singular (i.e. "slash") and #W for plural (i.e. "slashes") */

      {"$n tries to #w $N, but misses.", /* 0: 0       */
       "You try to #w $N, but miss.", "$n tries to #w you, but misses."},
      {"$n grazes $N with $s #w.", /* 1: 1..3    */
       "{GYou graze $N {Gwith your #w.", "{R$n {Rgrazes you with $s #w{R."},
      {"$n #W $N.", /* 2: 3..6    */
       "{GYou #w $N{G.", "{R$n {R#W you."},
      {"$n injures $N with $s #w.", /* 3: 6..9    */
       "{GYou injure $N {Gwith your #w.", "{R$n {Rinjures you with $s #w."},
      {"$n wounds $N with $s #w.", /* 4: 9..13   */
       "{GYou wound $N {Gwith your #w.", "{R$n {Rwounds you with $s #w."},
      {"$n mauls $N with $s #w.", /* 5: 7..10   */
       "{GYou maul $N {Gwith your #w.", "{R$n {Rmauls you with $s #w."},
      {"$n devastates $N with $s #w.", /* 6: 7..10   */
       "{GYou devastate $N {Gwith your #w.", "{R$n {Rdevastates you with $s #w."},
      {"$n decimates $N with $s ferocious #w.", /* 7: 19..23  */
       "{GYou decimate $N {Gwith your ferocious #w.", "{R$n {Rdecimates you with $s ferocious #w."},
      {"$n maims $N with $s fierce #w!!", /* 8: 23..35  */
       "{GYou maim $N {Gwith your fierce #w!!", "{R$n {Rmaims you with $s fierce #w!!"},
      {"$n mangles $N with $s nasty #w!!", /* 9: 23..35  */
       "{GYou mangles $N {Gwith your nasty #w!!", "{R$n {Rmangles you with $s nasty #w!!"},
      {"$n obliterates $N with $s savage #w!!", /* 10: 35..50 */
       "{GYou obliterate $N {Gwith your savage #w!!", "{R$n {Robliterates you with $s savage #w!!"},
      {"$n eradicates $N with $s gruesome #w!!", /* 11: 35..50 */
       "{GYou eradicate $N {Gwith your gruesome #w!!", "{R$n {Reradicates you with $s gruesome #w!!"},
      {"$n annihilates $N with $s deadly #w!!", /* 12: > 50   */
       "{GYou annihilate $N {Gwith your deadly #w!!", "{R$n {Rannihilates you with $s deadly #w!!"}};
  /* Damage weapon randomized for PC's */
  static struct dam_weapon_type_mob dam_weapons[][4] = {
      {{/* 0 dam */
        "$n's #w glances completely off $N.", "{GYour #w glances completely off $N{G.",
        "{R$n{R's #w glances completely off you."},
       {"$n's #w lands ineffectually on $N.", "{GYour #w lands ineffectually on $N{G.",
        "{R$n's {R#w lands ineffectually on you."},
       {"$n's #w strikes $N, but slides off.", "{GYour #w strikes $N{G, but slides off.",
        "{R$n's {R#w strikes you, but slides off."},
       {"$n's weak #w doesn't damage $N.", "{GYour weak #w doesn't damage $N{G.",
        "{R$n's {Rweak #w doesn't damage you."}},
      {{"$n's #w grazes $N as it glances off.", "{GYour #w grazes $N{G as it glances off.",
        "{R$n's {R#w grazes you as it glances off."},
       {"$n's #w grazes $N as it glances off.", "{GYour #w grazes $N {Gas it glances off.",
        "{R$n's {R#w grazes you as it glances off."},
       {"$n's #w barely damages $N.", "{GYour #w barely damages $N{G.", "{R$n's {R#w barely damages you."},
       {"$n's #w barely damages $N.", "{GYour #w barely damages $N{G.", "{R$n's {R#w barely damages you."}},
      {{/* category 2 */
        "$n's weak #w makes contact with $N.", "{GYour weak #w makes contact with $N{G.",
        "{R$n's {Rweak #w makes contact with you."},
       {"$n's weak #w makes contact with $N.", "{GYour weak #w makes contact with $N{G.",
        "{R$n's {Rweak #w makes contact with you."},
       {"$n's weak #w makes contact with $N.", "{GYour weak #w makes contact with $N{G.",
        "{R$n's {Rweak #w makes contact with you."},
       {"$n's weak #w does noticeable damage to $N.", "{GYour weak #w does noticeable damage to $N{G.",
        "{R$n's {Rweak #w does noticeable damage to you."}},
      {{/* category 3 */
        "$n's #w strikes $N solidly.", "{GYour #w strikes $N {Gsolidly.", "{R$n's {R#w strikes you solidly."},
       {"$n's #w strikes $N solidly.", "{GYour #w strikes $N {Gsolidly.", "{R$n's {R#w strikes you solidly."},
       {"$n's #w strikes $N solidly.", "{GYour #w strikes $N {Gsolidly.", "{R$n's {R#w strikes you solidly."},
       {"$n's #w strikes $N solidly.", "{GYour #w strikes $N {Gsolidly.", "{R$n's {R#w strikes you solidly."}},
      {{/* category 4 */
        "$n's powerful #w strikes $N squarely!", "{GYour powerful #w strikes $N {Gsquarely!",
        "{R$n's {Rpowerful #w strikes you squarely!"},
       {"$n's powerful #w strikes $N squarely!", "{GYour powerful #w strikes $N {Gsquarely!",
        "{R$n's {Rpowerful #w strikes you squarely!"},
       {"$n's powerful #w strikes $N squarely!", "{GYour powerful #w strikes $N {Gsquarely!",
        "{R$n's {Rpowerful #w strikes you squarely!"},
       {"$n's powerful #w strikes $N squarely!", "{GYour powerful #w strikes $N {Gsquarely!",
        "{R$n's {Rpowerful #w strikes you squarely!"}},
      {{/* category 5 */
        "$n's deadly #w lands perfectly on $N!!", "{GYour deadly #w lands perfectly on $N{G!!",
        "{R$n's {Rdeadly #w lands perfectly on you!!"},
       {"$n's deadly #w lands perfectly on $N!!", "{GYour deadly #w lands perfectly on $N{G!!",
        "{R$n's {Rdeadly #w lands perfectly on you!!"},
       {"$n's deadly #w lands perfectly on $N!!", "{GYour deadly #w lands perfectly on $N{G!!",
        "{R$n's {Rdeadly #w lands perfectly on you!!"},
       {"$n's deadly #w lands perfectly on $N!!", "{GYour deadly #w lands perfectly on $N{G!!",
        "{R$n's {Rdeadly #w lands perfectly on you!!"}}};

  w_type -= TYPE_HIT; /* Change to base of table with text */
  msgnum = 0;

  if (IS_NPC(ch)) {
    if (dam == 0)
      msgnum = 0;
    else if (dam <= 2)
      msgnum = 1;
    else if (dam <= 4)
      msgnum = 2;
    else if (dam <= 6)
      msgnum = 3;
    else if (dam <= 10)
      msgnum = 4;
    else if (dam <= 14)
      msgnum = 5;
    else if (dam <= 19)
      msgnum = 6;
    else if (dam <= 24)
      msgnum = 7;
    else if (dam <= 29)
      msgnum = 8;
    else if (dam <= 34)
      msgnum = 9;
    else if (dam <= 40)
      msgnum = 10;
    else if (dam <= 50)
      msgnum = 11;
    else
      msgnum = 12;
  } else if (!IS_NPC(ch)) {
    if (ch->equipment[WEAR_WIELD]) {
      weap = ch->equipment[WEAR_WIELD];
      weapmin = GET_OBJ_VAL(weap, 1);
      weapmax = (GET_OBJ_VAL(weap, 1) * GET_OBJ_VAL(weap, 2));
    } else {
      if (IS_MONK(ch)) {
        weapmin = monk_stat[(sh_int)GET_LEVEL(ch)][0];
        weapmax = (monk_stat[(sh_int)GET_LEVEL(ch)][0] * monk_stat[(sh_int)GET_LEVEL(ch)][1]);
      } else {
        weapmin = 0;
        weapmax = 2;
      }
    }
    dampercent = ((damorig - weapmin) * 100) / (1 + weapmax - weapmin);
    if (dam == 0)
      msgnum = 0;
    else if (dampercent <= 10)
      msgnum = 1;
    else if (dampercent <= 36)
      msgnum = 2;
    else if (dampercent <= 67)
      msgnum = 3;
    else if (dampercent <= 95)
      msgnum = 4;
    else if (dampercent <= 100 || damorig >= weapmax)
      msgnum = 5;
  }

  /* damage message to onlookers */
  if (IS_NPC(ch)) {
    g_buf = replace_string(npc_dam_weapons[msgnum].to_room, attack_hit_text[w_type].singular,
                           attack_hit_text[w_type].plural);
  } else {
    g_buf = replace_string(dam_weapons[msgnum][number(0, 3)].to_room, attack_hit_text[w_type].singular,
                           attack_hit_text[w_type].plural);
  }
  act(g_buf, FALSE, ch, NULL, victim, TO_NOTVICT);

  /* damage message to damager */
  if (IS_NPC(ch)) {
    g_buf = replace_string(npc_dam_weapons[msgnum].to_char, attack_hit_text[w_type].singular,
                           attack_hit_text[w_type].plural);
  } else {
    g_buf = replace_string(dam_weapons[msgnum][number(0, 3)].to_char, attack_hit_text[w_type].singular,
                           attack_hit_text[w_type].plural);
  }
  act(CAP(g_buf), FALSE, ch, NULL, victim, TO_CHAR);
  send_to_char(CCNRM(ch, C_CMP), ch);

  /* damage message to damagee */
  if (IS_NPC(ch)) {
    g_buf = replace_string(npc_dam_weapons[msgnum].to_victim, attack_hit_text[w_type].singular,
                           attack_hit_text[w_type].plural);
  } else {
    g_buf = replace_string(dam_weapons[msgnum][number(0, 3)].to_victim, attack_hit_text[w_type].singular,
                           attack_hit_text[w_type].plural);
  }
  act(CAP(g_buf), FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);
  send_to_char(CCNRM(victim, C_CMP), victim);
}

/*
 * message for doing damage with a spell or skill
 *  C3.0: Also used for weapon damage on miss and death blows
 */
int skill_message(int dam, struct char_data *ch, struct char_data *vict, int attacktype, int hide) {
  int i, j, nr;
  struct message_type *msg;

  struct obj_data *weap = ch->equipment[WEAR_WIELD];

  int attacks_msg[] = {TYPE_HIT,    TYPE_PIERCE, TYPE_SLASH, TYPE_SLASH, TYPE_THRASH, TYPE_CRUSH,
                       TYPE_HIT,    TYPE_POUND,  TYPE_CRUSH, TYPE_POUND, TYPE_CRUSH,  TYPE_POUND,
                       TYPE_PIERCE, TYPE_WHIP,   TYPE_SLASH, TYPE_POUND, TYPE_PIERCE, TYPE_HIT,
                       TYPE_HIT,    TYPE_HIT,    TYPE_HIT,   TYPE_STING, TYPE_BITE,   TYPE_BLUDGEON,
                       TYPE_CLAW,   TYPE_MAUL,   TYPE_BLAST, TYPE_PUNCH, TYPE_STAB};

  if (hide) {
    return 1;
  }
  if (IS_WEAPON(attacktype)) {
    attacktype = attacks_msg[attacktype - TYPE_HIT];
  }

  for (i = 0; i < MAX_MESSAGES; i++) {
    if (fight_messages[i].a_type == attacktype) {
      nr = dice(1, fight_messages[i].number_of_attacks);
      for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++) {
        msg = msg->next;
      }
      if (dam != 0) {
        if (GET_POS(vict) == POS_DEAD) {
          send_to_char(CBGRN(ch, C_CMP), ch);
          act(msg->die_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
          send_to_char(CCNRM(ch, C_CMP), ch);

          send_to_char(CBRED(vict, C_CMP), vict);
          act(msg->die_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
          send_to_char(CCNRM(vict, C_CMP), vict);

          act(msg->die_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
        } else {
          send_to_char(CBGRN(ch, C_CMP), ch);
          act(msg->hit_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
          send_to_char(CCNRM(ch, C_CMP), ch);

          send_to_char(CBRED(vict, C_CMP), vict);
          act(msg->hit_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
          send_to_char(CCNRM(vict, C_CMP), vict);

          act(msg->hit_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
        }
      } else if (ch != vict) { /* Dam == 0 */
        act(msg->miss_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
        act(msg->miss_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);
        act(msg->miss_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
      }
      return 1;
    }
  }
  return 0;
}

int resist_modify(struct char_data *ch, struct char_data *victim, int dam, int attacktype, int damtype) {
  if (attacktype >= 600 && attacktype <= 614) {
    if (ch->equipment[WEAR_WIELD]) {
      if (IS_OBJ_STAT(ch->equipment[WEAR_WIELD], ITEM_MAGIC)) {
        dam -= ((dam * GET_RESIST(victim, DAM_MWEAP)) / 100);
      } else if (!IS_OBJ_STAT(ch->equipment[WEAR_WIELD], ITEM_MAGIC)) {
        dam -= ((dam * GET_RESIST(victim, DAM_NWEAP)) / 100);
      }
    } else {
      dam -= ((dam * GET_RESIST(victim, DAM_NWEAP)) / 100);
    }
  }
  dam -= ((dam * GET_RESIST(victim, damtype)) / 100);
  return dam;
}

void damage(struct char_data *ch, struct char_data *victim, int dam, int attacktype, int damorig, int damtype,
            int is_spell, int hide) {
  struct char_data *tmp_ch;
  struct char_data *temp;
  struct follow_type *ftemp;
  int mpcast = FALSE;
  int fireshield = spells[find_spell_num("fire shield")].spellindex;
  int iceshield = spells[find_spell_num("ice shield")].spellindex;
  char debuglog[256];
  int shielddam;

  if (!(attacktype == TYPE_UNDEFINED)) {
    if ((attacktype < 0) && (-attacktype < MAX_SPELLS)) {
      attacktype = -attacktype;
      mpcast = TRUE;
    }
  }

  if (GET_POS(victim) <= POS_DEAD) {
    stderr_log("SYSERR: Attempt to damage a corpse.");
    stop_fighting(ch);
    return; /* -je, 7/7/92 */
  }
  /* You can damage an immortal! but not much.*/
  if (!IS_NPC(victim) && (GET_LEVEL(victim) >= LVL_IMMORT)) {
    dam >>= 2;
  }

  /* shopkeeper protection */
  if (!ok_damage_shopkeeper(ch, victim)) {
    stop_fighting(ch);
    return;
  }

  if (!IS_NPC(ch) && !IS_NPC(victim)) {
    if (PLR_FLAGGED(ch, PLR_OUTLAW)) {
      stop_fighting(ch);
      return;
    }
  }

  /* Minor paralisys */
  if (IS_AFFECTED2(victim, AFF2_MINOR_PARALIZED)) {
    reduce_affect(victim, spells[find_skill_num("minor paralysis")].spellindex, dam * 2);
  }

  /* stoneskin */
  if (IS_AFFECTED2(victim, AFF2_STONESKIN)) {
    reduce_affect(victim, spells[find_skill_num("stoneskin")].spellindex, (int)(dam * 1.5));
    dam = 0;
  }

  /* protection from XXXXX */
  switch (damtype) {
  case DAM_ELECTRICITY:
    if (AFF2_FLAGGED(victim, AFF2_PROT_LIGHT)) {
      dam >>= 1;
    }
    break;
  case DAM_ACID:
    if (AFF2_FLAGGED(victim, AFF2_PROT_ACID)) {
      dam >>= 1;
    }
    break;
  case DAM_COLD:
    if (AFF2_FLAGGED(victim, AFF2_PROT_ICE)) {
      dam >>= 1;
    }
    if (AFF2_FLAGGED(victim, AFF2_FIRESHIELD)) {
      dam <<= 1;
    }
    break;
  case DAM_POISON:
    if (AFF2_FLAGGED(victim, AFF2_PROT_GAS)) {
      dam >>= 1;
    }
    break;
  case DAM_FIRE:
    if (AFF2_FLAGGED(victim, AFF2_PROT_FIRE)) {
      dam >>= 1;
    }
    if (AFF2_FLAGGED(victim, AFF2_ICESHIELD)) {
      dam <<= 1;
    }
    break;
  }
  /* RESISTANCES HERE */
  dam = resist_modify(ch, victim, dam, attacktype, damtype);
  /* do the damage... heh */ GET_HIT(victim) -= dam;
  if (GET_HIT(victim) > GET_MAX_HIT(victim)) {
    GET_HIT(victim) = GET_MAX_HIT(victim);
  }
  if (dam < 0 && is_spell) {
    act("$N smiles as $e absorbs your spell.", TRUE, ch, 0, victim, TO_CHAR);
    act("You smile as you absorb $n's spell.", TRUE, ch, 0, victim, TO_VICT);
    act("$N smiles as $e absorbs $n's spell.", TRUE, ch, 0, victim, TO_ROOM);
    return;
  }
  if (damtype == DAM_FIRE && is_spell && IS_NPC(victim) &&
      !strncmp("elemental ", GET_MOB_NAME(victim), MIN(10, strlen(GET_MOB_NAME(victim))))) {
    safe_snprintf(debuglog, sizeof(debuglog), "%s's elemental didn't absorb spell Resist = %d, Damtype = %d",
                  GET_NAME(ch), GET_RESIST(victim, damtype), damtype);
    mudlog(debuglog, 'D', COM_IMMORT, TRUE);
  }
  if (AFF2_FLAGGED(ch, AFF2_VAMPTOUCH) && !is_spell) {
    if (dam > 0) {
      GET_HIT(ch) += dam / 5;
      if (GET_HIT(ch) > GET_MAX_HIT(ch)) {
        GET_HIT(ch) = GET_MAX_HIT(ch);
      }
    }
  }
  if (ch == NULL || victim == NULL) {
    return;
  }
  if (ch->in_room != victim->in_room) {
    return;
  }
  /* fireshield/iceshield 2d8 damage*/
  if (IS_AFFECTED2(victim, AFF2_FIRESHIELD) && !is_spell) {
    shielddam = dice(2, 8);
    if (reduce_affect(victim, fireshield, 3)) {
      dam = resist_modify(victim, ch, shielddam, fireshield, DAM_FIRE);
      if (AFF2_FLAGGED(ch, AFF2_PROT_FIRE)) {
        dam >>= 1;
      }
      if (AFF2_FLAGGED(ch, AFF2_ICESHIELD)) {
        dam <<= 1;
      }
      /* do the damage... heh */
      if (!AFF_FLAGGED(ch, AFF_MAJOR_GLOBE)) {
        GET_HIT(ch) -= dam;
        if (GET_HIT(ch) > GET_MAX_HIT(ch)) {
          GET_HIT(ch) = GET_MAX_HIT(ch);
        }
        if (dam > 0) {
          skill_message(dam, victim, ch, fireshield, 0);
        }
      } else {
        act("Your major globe of invunerability absorbs the damage from $N's fire shield.", TRUE, ch, 0, victim,
            TO_CHAR);
      }
      if (GET_POS(ch) == POS_DEAD) {
        if (IS_NPC(ch) || ch->desc) {
          group_gain(victim, ch);
        }
        if (!IS_NPC(ch)) {
          safe_snprintf(g_buf2, MAX_STRING_LENGTH, "%s killed by %s at %s", GET_NAME(ch), GET_NAME(victim),
                        world[ch->in_room].name);
          mudlog(g_buf2, 'K', COM_IMMORT, TRUE);
          plog(g_buf2, victim, 0);
          plog(g_buf2, ch, 0);

          if (IS_NPC(victim)) {
            if (MOB_FLAGGED(victim, MOB_MEMORY)) {
              forget(victim, ch);
            }
            if (HUNTING(victim) == GET_IDNUM(ch)) { /* makes hunting cityguards forget */
              HUNTING(victim) = 0;
            }

            for (tmp_ch = character_list; tmp_ch; tmp_ch = tmp_ch->next) {
              if (MOB_FLAGGED(tmp_ch, MOB_MEMORY)) {
                forget(tmp_ch, ch);
              }
              if (HUNTING(tmp_ch) == GET_IDNUM(ch)) {
                HUNTING(tmp_ch) = 0;
              }
            }
          }
        }

        die(ch, victim);
        return;
      }
    } else if (number(0, 3)) {
      dam = resist_modify(victim, ch, shielddam, fireshield, DAM_FIRE);
      if (AFF2_FLAGGED(ch, AFF2_PROT_FIRE)) {
        dam >>= 1;
      }
      if (AFF2_FLAGGED(ch, AFF2_ICESHIELD)) {
        dam <<= 1;
      }
      /* do the damage... heh */
      if (!AFF_FLAGGED(ch, AFF_MAJOR_GLOBE)) {
        GET_HIT(ch) -= dam;
        if (GET_HIT(ch) > GET_MAX_HIT(ch)) {
          GET_HIT(ch) = GET_MAX_HIT(ch);
        }
        if (dam > 0) {
          skill_message(dam, victim, ch, fireshield, 0);
        }
      } else {
        act("Your major globe of invunerability absorbs the damage from $N's fire shield.", TRUE, ch, 0, victim,
            TO_CHAR);
      }
      if (GET_POS(ch) == POS_DEAD) {
        if (IS_NPC(ch) || ch->desc) {
          group_gain(victim, ch);
        }
        if (!IS_NPC(ch)) {
          safe_snprintf(g_buf2, MAX_STRING_LENGTH, "%s killed by %s at %s", GET_NAME(ch), GET_NAME(victim),
                        world[ch->in_room].name);
          mudlog(g_buf2, 'K', COM_IMMORT, TRUE);
          plog(g_buf2, victim, 0);
          plog(g_buf2, ch, 0);

          if (IS_NPC(victim)) {
            if (MOB_FLAGGED(victim, MOB_MEMORY)) {
              forget(victim, ch);
            }
            if (HUNTING(victim) == GET_IDNUM(ch)) { /* makes hunting cityguards forget */
              HUNTING(victim) = 0;
            }

            for (tmp_ch = character_list; tmp_ch; tmp_ch = tmp_ch->next) {
              if (MOB_FLAGGED(tmp_ch, MOB_MEMORY)) {
                forget(tmp_ch, ch);
              }
              if (HUNTING(tmp_ch) == GET_IDNUM(ch)) {
                HUNTING(tmp_ch) = 0;
              }
            }
          }
        }

        die(ch, victim);
        return;
      }
    }
    if (!ch || !victim) {
      return;
    }
    if (ch->in_room != victim->in_room) {
      return;
    }
  } else if (IS_AFFECTED2(victim, AFF2_ICESHIELD) && !is_spell) {
    shielddam = dice(2, 8);
    if (reduce_affect(victim, iceshield, 3)) {
      dam = resist_modify(victim, ch, shielddam, iceshield, DAM_COLD);
      if (AFF2_FLAGGED(victim, AFF2_PROT_ICE)) {
        dam >>= 1;
      }
      if (AFF2_FLAGGED(victim, AFF2_FIRESHIELD)) {
        dam <<= 1;
      }
      if (!AFF_FLAGGED(ch, AFF_MAJOR_GLOBE)) {
        GET_HIT(ch) -= dam;
        if (GET_HIT(ch) > GET_MAX_HIT(ch)) {
          GET_HIT(ch) = GET_MAX_HIT(ch);
        }
        if (dam > 0) {
          skill_message(dam, victim, ch, iceshield, 0);
        }
      } else {
        act("Your major globe of invunerability absorbs the damage from $N's ice shield.", TRUE, ch, 0, victim,
            TO_CHAR);
      }
      if (GET_POS(ch) == POS_DEAD) {
        if (IS_NPC(ch) || ch->desc) {
          group_gain(victim, ch);
        }
        if (!IS_NPC(ch)) {
          safe_snprintf(g_buf2, MAX_STRING_LENGTH, "%s killed by %s at %s", GET_NAME(ch), GET_NAME(victim),
                        world[ch->in_room].name);
          mudlog(g_buf2, 'K', COM_IMMORT, TRUE);
          plog(g_buf2, victim, 0);
          plog(g_buf2, ch, 0);

          if (IS_NPC(victim)) {
            if (MOB_FLAGGED(victim, MOB_MEMORY)) {
              forget(victim, ch);
            }
            if (HUNTING(victim) == GET_IDNUM(ch)) { /* makes hunting cityguards forget */
              HUNTING(victim) = 0;
            }

            for (tmp_ch = character_list; tmp_ch; tmp_ch = tmp_ch->next) {
              if (MOB_FLAGGED(tmp_ch, MOB_MEMORY)) {
                forget(tmp_ch, ch);
              }
              if (HUNTING(tmp_ch) == GET_IDNUM(ch)) {
                HUNTING(tmp_ch) = 0;
              }
            }
          }
        }

        die(ch, victim);
        return;
      }
    } else if (number(0, 3)) {
      dam = resist_modify(victim, ch, shielddam, iceshield, DAM_COLD);
      if (AFF2_FLAGGED(ch, AFF2_PROT_ICE)) {
        dam >>= 1;
      }
      if (AFF2_FLAGGED(ch, AFF2_FIRESHIELD)) {
        dam <<= 1;
      }
      if (!AFF_FLAGGED(ch, AFF_MAJOR_GLOBE)) {
        GET_HIT(ch) -= dam;
        if (GET_HIT(ch) > GET_MAX_HIT(ch)) {
          GET_HIT(ch) = GET_MAX_HIT(ch);
        }
        if (dam > 0) {
          skill_message(dam, victim, ch, iceshield, 0);
        }
      } else {
        act("Your major globe of invunerability absorbs the damage from $N's ice shield.", TRUE, ch, 0, victim,
            TO_CHAR);
      }
      if (GET_POS(ch) == POS_DEAD) {
        if (IS_NPC(ch) || ch->desc) {
          group_gain(victim, ch);
        }
        if (!IS_NPC(ch)) {
          safe_snprintf(g_buf2, MAX_STRING_LENGTH, "%s killed by %s at %s", GET_NAME(ch), GET_NAME(victim),
                        world[ch->in_room].name);
          mudlog(g_buf2, 'K', COM_IMMORT, TRUE);
          plog(g_buf2, victim, 0);
          plog(g_buf2, ch, 0);

          if (IS_NPC(victim)) {
            if (MOB_FLAGGED(victim, MOB_MEMORY)) {
              forget(victim, ch);
            }
            if (HUNTING(victim) == GET_IDNUM(ch)) { /* makes hunting cityguards forget */
              HUNTING(victim) = 0;
            }

            for (tmp_ch = character_list; tmp_ch; tmp_ch = tmp_ch->next) {
              if (MOB_FLAGGED(tmp_ch, MOB_MEMORY)) {
                forget(tmp_ch, ch);
              }
              if (HUNTING(tmp_ch) == GET_IDNUM(ch)) {
                HUNTING(tmp_ch) = 0;
              }
            }
          }
        }

        die(ch, victim);
        return;
      }
    }
    if (!ch || !victim) {
      return;
    }
    if (ch->in_room != victim->in_room) {
      return;
    }
  }

  if (victim != ch) {
    if (GET_POS(ch) > POS_STUNNED) {
      if (!AFF_FLAGGED(victim, AFF_MAJOR_PARALIZED) && !AFF2_FLAGGED(victim, AFF2_MINOR_PARALIZED) &&
          GET_POS(victim) >= POS_SLEEPING && GET_POS(victim) < POS_FIGHTING && IS_NPC(victim) &&
          GET_MOB_WAIT(victim) == 0) {
        if (MOB_FLAGGED(victim, MOB_WIMPY)) {
          act("$n scrambles madly to $s feet!", TRUE, victim, 0, 0, TO_ROOM);
        } else {
          act("$n clambers to $s feet!", TRUE, victim, 0, 0, TO_ROOM);
        }
        GET_POS(victim) = POS_FIGHTING;
      }
      if (!(FIGHTING(ch))) {
        set_fighting(ch, victim);
      }
      if (IS_NPC(ch) && IS_NPC(victim) && victim->master &&
          (!number(0, 100) || ((number(0, 100) < 6) && (IS_MAGE(victim) || IS_PRI(victim)))) &&
          IS_AFFECTED(victim, AFF_CHARM) && !IS_AFFECTED(ch, AFF_BLIND) && (victim->master->in_room == ch->in_room)) {
        if (FIGHTING(ch)) {
          stop_fighting(ch);
        }
        if (GET_POS(victim->master) > POS_DEAD) {
          hit(ch, victim->master, TYPE_UNDEFINED);
        }
        act("$n {Cswitches to a new target!{x", TRUE, ch, 0, victim, TO_ROOM);
        return;
      }
    }
    if (GET_POS(victim) >= POS_DEAD && !FIGHTING(victim)) {
      set_fighting(victim, ch);
      if (MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch) && (GET_LEVEL(ch) < LVL_IMMORT)) {
        remember(victim, ch);
      }
    }
  }
  if (victim->master == ch) {
    stop_follower(victim);
  }

  if (IS_AFFECTED(ch, AFF_INVISIBLE | AFF_HIDE)) {
    if (IS_NPC(ch) && !MOB_FLAGGED(ch, MOB_WRAITHLIKE)) {
      appear(ch);
    } else if (!IS_NPC(ch)) {
      appear(ch);
    }
  }

  check_killer(ch, victim);
  if (ch != victim) {
    if ((ch->master || ch->followers) && (IS_AFFECTED(ch, AFF_GROUP))) {
      if (GET_LEVEL(victim) <= (GET_LEVEL(ch) + 5)) {
        gain_exp(ch, dam);
      } else if (GET_LEVEL(victim) > (GET_LEVEL(ch) + 6)) {
        gain_exp(ch, MAX(((1 - ((GET_LEVEL(victim) - (GET_LEVEL(ch) + 6)) / 10)) * dam), 0));
      }
    } else {
      gain_exp(ch, dam);
    }
  }

  /* if difference is 5 or less, 1:1 exp gain.
   if diference is 6 or more, -10% to a max of -100%
   All this if grouped.
   */
  update_pos(victim);

  if (GET_POS(victim) < POS_SLEEPING) {
    temp = ch->master;
    if (!temp) {
      ftemp = ch->followers;
      while (ftemp) {
        if (IS_AFFECTED(ftemp->follower, AFF_CHARM)) {
          stop_fighting(ftemp->follower);
        }
        ftemp = ftemp->next;
      }
    } else {
      ftemp = temp->followers;
      while (ftemp) {
        if (IS_AFFECTED(ftemp->follower, AFF_CHARM)) {
          stop_fighting(ftemp->follower);
        }
        ftemp = ftemp->next;
      }
    }
  }
  if (GET_POS(victim) < POS_SLEEPING) {
    if (IS_NPC(victim) && FIGHTING(victim)) {
      stop_fighting(victim);
    }
  }

  /*
   * skill_message sends a message from the messages file in lib/misc.
   * dam_message just sends a generic "You hit $n extremely hard.".
   * skill_message is preferable to dam_message because it is more
   * descriptive.
   *
   * If we are _not_ attacking with a weapon (i.e. a spell), always use
   * skill_message. If we are attacking with a weapon: If this is a miss or a
   * death blow, send a skill_message if one exists; if not, default to a
   * dam_message. Otherwise, always send a dam_message.
   */

  if (!IS_WEAPON(attacktype)) {
    if (mpcast == FALSE) {
      if (ch->in_room == victim->in_room) {
        skill_message(dam, ch, victim, attacktype, hide);
        if (dam == 0 && attacktype == spells[find_skill_num("bash")].spellindex) {
          GET_POS(ch) = POS_RESTING;
          FIGHT_STATE(ch, 2);
          if (IS_MOB(ch)) {
            GET_MOB_WAIT(ch) = 2;
          } else {
            WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
          }
        }
      }
    }
  } else {
    if (GET_POS(victim) == POS_DEAD || dam == 0) {
      if ((ch->in_room == victim->in_room) && !skill_message(dam, ch, victim, attacktype, hide)) {
        dam_message(dam, ch, victim, attacktype, damorig);
      }
    } else {
      if (ch->in_room == victim->in_room) {
        dam_message(dam, ch, victim, attacktype, damorig);
      }
    }
  }

  /* Use send_to_char -- act() doesn't send message if you are DEAD. */
  switch (GET_POS(victim)) {
  case POS_MORTALLYW:
    act("$n is mortally wounded, and will die soon, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("You are mortally wounded, and will die soon.\r\n", victim);
    stop_fighting(victim);
    break;
  case POS_INCAP:
    act("$n is incapacitated and will slowly die, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("You are incapacitated an will slowly die.\r\n", victim);
    stop_fighting(victim);
    break;
  case POS_STUNNED:
    act("$n is stunned, but will probably regain consciousness again.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char("You're stunned, but will probably regain"
                 " consciousness again.\r\n",
                 victim);
    stop_fighting(victim);
    break;
  case POS_DEAD:
    act("$n is dead!  R.I.P.", FALSE, victim, 0, 0, TO_ROOM);
    send_to_char("You are dead!  Sorry...\r\n", victim);
    break;
  default: /* >= POSITION SLEEPING */
    if (dam > (GET_MAX_HIT(victim) >> 2)) {
      act("{RThat really did {FHURT!{x", FALSE, victim, 0, 0, TO_CHAR);
    }
    if (GET_HIT(victim) < (GET_MAX_HIT(victim) >> 2)) {
      safe_snprintf(g_buf2, MAX_STRING_LENGTH,
                    "{DYou wish that your wounds"
                    " would stop {R{FBLEEDING{x {Dso much!{x\r\n");
      send_to_char(g_buf2, victim);
      if (MOB_FLAGGED(victim, MOB_WIMPY)) {
        do_flee(victim, "", 0, 0);
      }
    }
    if (!IS_NPC(victim) && GET_WIMP_LEV(victim) && victim != ch && !AFF_FLAGGED(ch, AFF_MAJOR_PARALIZED) &&
        !AFF2_FLAGGED(ch, AFF2_MINOR_PARALIZED) && GET_HIT(victim) < GET_WIMP_LEV(victim)) {
      send_to_char("You wimp out, and attempt to flee!\r\n", victim);
      do_flee(victim, "", 0, 0);
    }
    break;
  }
  temp = victim->master;
  ftemp = victim->followers;
  if (!IS_NPC(victim) && !(victim->desc)) {
    do_flee(victim, "", 0, 0);
    if (!FIGHTING(victim)) {
      act("$n is rescued by divine forces.", FALSE, victim, 0, 0, TO_ROOM);
      GET_WAS_IN(victim) = victim->in_room;
      char_from_room(victim);
      char_to_room(victim, 1);
    }
  }
  if (!AWAKE(victim)) {
    if (FIGHTING(victim)) {
      stop_fighting(victim);
    }
  }

  if (GET_POS(victim) == POS_DEAD) {
    if (IS_NPC(victim) || victim->desc) {
      group_gain(ch, victim);
    }
    if (!IS_NPC(victim)) {
      safe_snprintf(g_buf2, MAX_STRING_LENGTH, "%s killed by %s at %s", GET_NAME(victim), GET_NAME(ch),
                    world[victim->in_room].name);
      mudlog(g_buf2, 'K', COM_IMMORT, TRUE);
      plog(g_buf2, ch, 0);
      plog(g_buf2, victim, 0);

      if (MOB_FLAGGED(ch, MOB_MEMORY)) {
        forget(ch, victim);
      }
      if (HUNTING(ch) == GET_IDNUM(victim)) { /* makes hunting cityguards forget */
        HUNTING(ch) = 0;
      }

      for (tmp_ch = character_list; tmp_ch; tmp_ch = tmp_ch->next) {
        if (MOB_FLAGGED(tmp_ch, MOB_MEMORY)) {
          forget(tmp_ch, victim);
        }
        if (HUNTING(tmp_ch) == GET_IDNUM(victim)) {
          HUNTING(tmp_ch) = 0;
        }
      }
    }

    die(victim, ch);
  }
}

void hit(struct char_data *ch, struct char_data *victim, int type) {
  struct obj_data *wielded = ch->equipment[WEAR_WIELD];
  int damorig = 0;
  int w_type = 0;
  int victim_ac = 0, calc_thac0 = 0;
  int dam = 0, damdice = 0, damsize = 0, damroll = 0;
  int can_poison = 0; /* used for poisoned weapons */
  int good_swing = 0; /* used for skill check in weapon type */
  int wielded_type;
  int wieldpos = WEAR_WIELD;
  int dodgeroll = 0, dodgebonus = 0, dodgeskill = 0;
  struct mob_attacks_data *this;
  byte diceroll;
  int skillnum = spells[find_skill_num("backstab")].spellindex;
  int anatomy = spells[find_skill_num("anatomy")].spellindex;
  char fumblemsg[256];
  extern int thaco[NUM_CLASSES][2];
  extern byte backstab_mult[];
  extern byte backstab_asn[];
  extern sh_int stats[12][101];

  if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
    return;
  }
  if (ch == FIGHTING(ch)) {
    FIGHTING(ch) = NULL;
    return;
  }

  if (!wielded) {
    wielded = ch->equipment[WEAR_2HANDED];
    wieldpos = WEAR_2HANDED;
  }

  if (type == TYPE_DUAL_WIELD) {
    wielded = ch->equipment[WEAR_WIELD_2];
    wieldpos = WEAR_WIELD_2;
    type = TYPE_UNDEFINED;
  }

  if (type == skillnum) {
    wielded = ch->equipment[WEAR_WIELD];
    wieldpos = WEAR_WIELD;
    if (wielded) {
      if (GET_OBJ_VAL(wielded, 3) != WEAPON_SHORTSWORD && GET_OBJ_VAL(wielded, 3) != WEAPON_DAGGER) {
        wielded = ch->equipment[WEAR_WIELD_2];
      }
    }
  }

  if (wielded) {
    wielded_type = GET_OBJ_VAL(wielded, 0);
    if (IS_SET(GET_OBJ_EXTRA(wielded), ITEM_POISONED)) {
      can_poison = 1;
    }
    if (!IS_NPC(ch)) {
      if (GET_SKILL(ch, (spells[wielded_type + 1].spellindex)) > number(1, 101)) {
        good_swing = 1;
      }
      improve_skill(ch, spells[wielded_type + 1].spellindex, SKUSE_FREQUENT);
    } else {
      if (GET_LEVEL(ch) > number(1, 61)) {
        good_swing = 1;
      }
    }
  } else {
    if (!IS_NPC(ch) && (GET_SKILL(ch, (spells[find_skill_num("unarmed combat")].spellindex)))) {
      improve_skill(ch, spells[find_skill_num("unarmed combat")].spellindex, SKUSE_AVERAGE);
    }
  }
  if (ch->in_room != victim->in_room) {
    stderr_log("SYSERR: NOT SAME ROOM WHEN FIGHTING!");
    stop_fighting(ch);
    return;
  }

  mprog_hitprcnt_trigger(ch, FIGHTING(ch));
  mprog_fight_trigger(ch, FIGHTING(ch));

  if (!IS_NPC(ch)) {
    if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
      w_type = GET_OBJ_VAL(wielded, 3);
      damroll = (GET_DAMROLL(ch) * weapon_deadly[w_type]) / 100;
      w_type = w_type + TYPE_HIT;
      damdice = GET_OBJ_VAL(wielded, 1);
      damsize = GET_OBJ_VAL(wielded, 2);
      if (!good_swing && (wieldpos == WEAR_WIELD || wieldpos == WEAR_WIELD_2 || wieldpos == WEAR_2HANDED)) {
        if (GET_EQ(ch, wieldpos) && GET_DEX(ch) < number(1, 101) &&
            GET_SKILL(ch, (spells[wielded_type + 1].spellindex)) < number(1, 101) && !number(0, 1)) {
          safe_snprintf(fumblemsg, sizeof(fumblemsg), "{YYou swing very badly, sending your %s{Y flying!{x\r\n",
                        OBJS(wielded, ch));
          send_to_char(fumblemsg, ch);
          obj_to_room(unequip_char(ch, wieldpos), ch->in_room);
          GET_EQ(ch, wieldpos) = NULL;
          act("$n swings badly sending $e $p flying!", FALSE, ch, wielded, 0, TO_ROOM);
          return;
        }
        if (damdice > 1) {
          damdice--;
        } else if (damsize > 1) {
          damsize--;
        } else if (damroll > 1) {
          damroll--;
        }
      }
    } else { /* barehanded PC attack */
      damroll = (GET_DAMROLL(ch) * weapon_deadly[0]) / 100;
      w_type = TYPE_HIT;
      if (IS_MONK(ch)) {
        damdice = monk_stat[(sh_int)GET_LEVEL(ch)][0];
        damsize = monk_stat[(sh_int)GET_LEVEL(ch)][1];
        damroll = GET_DAMROLL(ch);
      }
    }
  } else {
    if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON && (ch->mob_specials.mob_attacks->fight_timer == 0)) {
      w_type = GET_OBJ_VAL(wielded, 3);
      w_type = w_type + TYPE_HIT;
      damdice = ch->mob_specials.mob_attacks->nodice;
      damsize = ch->mob_specials.mob_attacks->sizedice;
      damroll = ch->mob_specials.mob_attacks->damroll;
    } else {
      /* get MOB attack type.. This is done by scanning through
       all the attack records, and finding the one with the
       timer set to zero */
      this = ch->mob_specials.mob_attacks;
      while (this != NULL) {
        if (this->fight_timer == 0) {
          damdice = this->nodice;
          damsize = this->sizedice;
          damroll = this->damroll;
          w_type = this->attack_type + TYPE_HIT;
          break;
        }
        this = this->next;
      }
      if (this == NULL) { /* it is null... use the first attack... */
        this = ch->mob_specials.mob_attacks;
        damdice = this->nodice;
        damsize = this->sizedice;
        damroll = this->damroll;
        w_type = this->attack_type + TYPE_HIT;
      }
    }
  }

  if (can_poison && number(1, 10) < 3) {
    REMOVE_BIT(GET_OBJ_EXTRA(wielded), ITEM_POISONED);
    mag_affect_char((spells + find_spell_num("poison")), 0, ch, victim, GET_LEVEL(ch));
    act("You've been poisoned by $n", FALSE, ch, NULL, victim, TO_VICT);
    act("You're weapon poisons $t", FALSE, ch, NULL, victim, TO_CHAR);
  } else if (can_poison && number(1, 10) >= 8) {
    REMOVE_BIT(GET_OBJ_EXTRA(wielded), ITEM_POISONED);
  }
  /* Calculate the raw armor including magic armor.  Lower AC is better. */

  if (!IS_NPC(ch)) {
    calc_thac0 = thaco[(int)GET_CLASS(ch)][0] - ((int)GET_LEVEL(ch) / thaco[(int)GET_CLASS(ch)][1]);
    if (calc_thac0 < 1) {
      calc_thac0 = 1;
    }
    if (calc_thac0 > 20) {
      calc_thac0 = 20;
    }
    calc_thac0 -= stats[STR_TOHIT][GET_STR(ch)];
    calc_thac0 -= GET_HITROLL(ch);
    if (!wielded && (GET_SKILL(ch, (spells[find_skill_num("unarmed combat")].spellindex)))) {
      calc_thac0 -= ((GET_SKILL(ch, (spells[find_skill_num("unarmed combat")].spellindex))) / 10);
    }
  } else {
    /* THAC0 for monsters is set in the HitRoll */
    calc_thac0 = GET_HITROLL(ch);
  }

  if (!IS_NPC(victim) && (GET_SKILL(victim, (spells[find_skill_num("dodge")].spellindex)))) {
    dodgeroll = number(1, 100);
    dodgeskill = (GET_SKILL(victim, (spells[find_skill_num("dodge")].spellindex)));
    dodgebonus = (((dodgeskill - dodgeroll) / 7) - (GET_ATTACKED(victim)));
    if (dodgebonus > 0) {
      calc_thac0 += dodgebonus;
    }
    improve_skill(ch, (spells[find_skill_num("dodge")].spellindex), SKUSE_FREQUENT);
    GET_ATTACKED(victim)++;
  }

  diceroll = number(1, 20);

  victim_ac = GET_AC(victim) / 10;

  if (AWAKE(victim)) {
    victim_ac += stats[AGI_DEFENSE][GET_AGI(victim)];
  }

  victim_ac = MAX(-10, victim_ac); /* -10 is lowest */
  victim_ac = MIN(10, victim_ac);  /* 10 is highest */

  if (((diceroll < 20) && AWAKE(victim)) && ((calc_thac0 - diceroll) > victim_ac)) {
    if (dodgebonus && (((calc_thac0 - dodgebonus) - diceroll) <= victim_ac)) {
      act("$N nimbly dodges $n's attack!", FALSE, ch, 0, victim, TO_NOTVICT);
      act("$N nimbly dodges your attack!", FALSE, ch, 0, victim, TO_CHAR);
      act("You nimbly dodge $n's attack!", FALSE, ch, 0, victim, TO_VICT);
      improve_skill(victim, spells[find_skill_num("dodge")].spellindex, SKUSE_FREQUENT);
    } else if (type == skillnum) /* backstab */ {
      damage(ch, victim, 0, skillnum, 0, DAM_PIERCE, 0, 0);
    } else {
      damage(ch, victim, 0, w_type, damorig, attack_hit_text[w_type - 600].damtype, 0, 0);
    }
  } else {
    dam = stats[STR_TODAM][GET_STR(ch)];
    dam += damroll;
    if (wielded || IS_MONK(ch)) {
      damorig = dice(damdice, damsize);
      dam += damorig;
    } else {
      if (IS_NPC(ch)) {
        dam += dice(damdice, damsize);
      } else {
        damorig = number(0, 2);
        dam += damorig; /* Max. 2 dam with bare hands */
      }
    }

    if (GET_POS(victim) < POS_FIGHTING)
      dam *= 1 + (POS_FIGHTING - GET_POS(victim)) / 3;
    /* Position  sitting  x 1.33 */
    /* Position  resting  x 1.66 */
    /* Position  sleeping x 2.00 */
    /* Position  stunned  x 2.33 */
    /* Position  incap    x 2.66 */
    /* Position  mortally x 3.00 */

    dam = MAX(1, dam); /* Not less than 0 damage */

    if (type == skillnum) {
      if (!IS_THI(ch) && !IS_NPC(ch)) {
        send_to_char("You're not a thief.\r\n", ch);
        return;
      }
      if (GET_CLASS(ch) == CLASS_ASSASSIN) {
        dam *= backstab_asn[(int)GET_LEVEL(ch)];
      } else if (GET_CLASS(ch) == CLASS_MERCENARY) {
        dam *= backstab_mult[(int)GET_LEVEL(ch)] * 2 / 3;
      } else {
        dam *= backstab_mult[(int)GET_LEVEL(ch)];
      }
      if (GET_CLASS(ch) == CLASS_MONK || GET_CLASS(ch) == CLASS_MERCENARY) {
        if (GET_SKILL(ch, anatomy) > number(1, 250)) {
          damage(ch, victim, dam * 2, skillnum, 0, DAM_PIERCE, 0, 0);
          send_to_char("{WYou score a critical hit!{x\r\n", ch);
          if (victim && FIGHTING(ch)) {
            send_to_char("{WThat really HURT!{x\r\n", victim);
          }
          improve_skill(ch, anatomy, SKUSE_FREQUENT);
        } else {
          damage(ch, victim, dam, skillnum, 0, DAM_PIERCE, 0, 0);
        }
      } else {
        damage(ch, victim, dam, skillnum, 0, DAM_PIERCE, 0, 0);
      }
    } else {
      if (GET_CLASS(ch) == CLASS_MONK || GET_CLASS(ch) == CLASS_MERCENARY) {
        if (GET_SKILL(ch, anatomy) > number(1, 250)) {
          damage(ch, victim, dam * 2, w_type, damorig, attack_hit_text[w_type - 600].damtype, 0, 0);
          send_to_char("{WYou score a critical hit!{x\r\n", ch);
          if (victim) {
            send_to_char("{WThat really HURT!{x\r\n", victim);
          }
          improve_skill(ch, anatomy, SKUSE_FREQUENT);
        } else {
          damage(ch, victim, dam, w_type, damorig, attack_hit_text[w_type - 600].damtype, 0, 0);
        }
      } else {
        damage(ch, victim, dam, w_type, damorig, attack_hit_text[w_type - 600].damtype, 0, 0);
      }
    }
  }
}

struct char_data *get_target(struct char_data *ch) {
  struct char_data *tmp;
  struct char_data *next;

  for (tmp = world[ch->in_room].people; tmp; tmp = next) {
    next = tmp->next_in_room;
    if ((IS_MAGE(tmp) || IS_PRI(tmp)) && FIGHTING(tmp) == ch) {
      if (IS_CASTING(tmp)) {
        return tmp;
      }
    }
  }

  return FIGHTING(ch);
}

void perform_mob_defense_fight(struct char_data *ch) {
  char argument[256];
  int casttype = 0;
  int attacktype = 0;
  int skip = 0;
  struct char_data *target = get_target(ch);
  void do_mage_spell(struct char_data * ch);
  void do_cleric_spell(struct char_data * ch);

  if (MOB_FLAGGED(ch, MOB_HAS_WARRIOR)) {
    attacktype = 2;
  }
  if (MOB_FLAGGED(ch, MOB_HAS_THIEF)) {
    attacktype++;
  }
  if (attacktype && (MOB_FLAGGED(ch, MOB_HAS_CLERIC) || MOB_FLAGGED(ch, MOB_HAS_MAGE))) {
    if (attacktype && !IS_MAGE(target) && !IS_PRI(target)) {
      skip = 1;
    } else {
      if (attacktype == 3 && (IS_MAGE(target) || IS_PRI(target))) {
        attacktype = 2;
      } else {
        attacktype = number(1, 2);
      }
    }
  }

  if (attacktype && !skip) { /* more damage can be done with spells */
    switch (attacktype) {
    case 1:
      if (number(!0, 2) && (GET_EQ(ch, WEAR_WIELD) || GET_EQ(ch, WEAR_WIELD_2))) {
        safe_snprintf(argument, sizeof(argument), "circle %s", GET_NAME(target));
        command_interpreter(ch, argument);
      } else if (!number(0, 3)) {
        safe_snprintf(argument, sizeof(argument), "kick %s", GET_NAME(target));
        command_interpreter(ch, argument);
      }
      break;
    case 2:
      if (!number(0, 2)) {
        safe_snprintf(argument, sizeof(argument), "bash %s", GET_NAME(target));
        command_interpreter(ch, argument);
      } else if (!number(0, 3)) {
        safe_snprintf(argument, sizeof(argument), "kick %s", GET_NAME(target));
        command_interpreter(ch, argument);
      }
      break;
    }
  }

  if (MOB_FLAGGED(ch, MOB_HAS_CLERIC)) {
    casttype = 2;
  }
  if (MOB_FLAGGED(ch, MOB_HAS_MAGE)) {
    casttype++;
  }
  if (casttype == 3) {
    if (GET_HIT(ch) < (GET_MAX_HIT(ch) >> 1)) {
      casttype = 2;
    } else {
      casttype = 1;
    }
  }
  switch (casttype) {
  case 1:
    do_mage_spell(ch);
    break;
  case 2:
    do_cleric_spell(ch);
    break;
  }
}

/* control the fights going on. Called every tick from comm.c. */
void perform_violence(void) {
  struct char_data *ch;
  struct mob_attacks_data *this;
  extern struct index_data *mob_index;

  for (ch = combat_list; ch; ch = next_combat_list) {
    next_combat_list = ch->next_fighting;

    if (IS_NPC(ch) && !AFF_FLAGGED(ch, AFF_MAJOR_PARALIZED) &&
        !AFF2_FLAGGED(ch, AFF2_MINOR_PARALIZED)) { /* fight control for NPCs */
      if (GET_MOB_WAIT(ch) > 0) {
        GET_MOB_WAIT(ch)--;
      }
      perform_mob_defense_fight(ch);
      this = ch->mob_specials.mob_attacks; /* get first attack */
      while (this != NULL) {               /* okay.. check each and every attack record */
        this->fight_timer--;               /* decrement fight timer */
        if (this->fight_timer == 0) {      /* okay.. time to attack.. */
          if (FIGHTING(ch) != NULL) {
            if (FIGHTING(ch) == NULL || GET_POS(FIGHTING(ch)) <= POS_STUNNED || ch->in_room != FIGHTING(ch)->in_room) {
              stop_fighting(ch);
            } else {
              if (GET_POS(FIGHTING(ch)) > POS_DEAD) {
                multiple_hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
              }
              if (MOB_FLAGGED(ch, MOB_SPEC) && mob_index[GET_MOB_RNUM(ch)].func != NULL &&
                  ch->mob_specials.mp_toggle != FALSE)
                (mob_index[GET_MOB_RNUM(ch)].func)(ch, ch, 0, "", SPEC_FIGHT);
            }
          }
          this->fight_timer = 1;
        } /* mob attacked if */
        this = this->next;
      } /* while */
    } else if (!AFF_FLAGGED(ch, AFF_MAJOR_PARALIZED) &&
               !AFF2_FLAGGED(ch, AFF2_MINOR_PARALIZED)) { /* fight control for PCs */
      ch->char_specials.fight_timer--;                    /* decrement fight timer */
      GET_ATTACKED(ch) = 0;
      if (FIGHTING(ch) == NULL || ch->in_room != FIGHTING(ch)->in_room) {
        stop_fighting(ch);
      } else if (GET_POS(ch) == POS_DEAD) {
        die(ch, FIGHTING(ch));
      } else {
        if (check_events(ch, camp)) {
          send_to_char("Combat has interrupted your camping.\r\n", ch);
          clean_events(ch, camp);
        }
        if (ch->char_specials.fight_timer == 0) { /* okay.. HIT */
          if (FIGHTING(ch) == NULL || ch->in_room != FIGHTING(ch)->in_room) {
            stop_fighting(ch);
          } else if (!AFF2_FLAGGED(ch, AFF2_CASTING)) {
            multiple_hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
            if (ch->equipment[WEAR_WIELD_2]) {
              if (FIGHTING(ch) && (GET_POS(FIGHTING(ch)) > POS_DEAD)) {
                hit(ch, FIGHTING(ch), TYPE_DUAL_WIELD);
              }
            }
          }
          ch->char_specials.fight_timer = 1;
        } /* hit if thingie */
      } /* main if check thing */
    } /* another if.. */
    if (CHECK_FIGHT(ch)) {
      ch->char_specials.fightwait--; /* fight waitstate */
    }
    if (SKILL_TIMER(ch)) {
      SKILL_TIMER(ch)--;
    }
  } /* for loop */
}
