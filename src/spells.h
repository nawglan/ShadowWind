/* ************************************************************************
 *   File: spells.h                                      Part of CircleMUD *
 *  Usage: header file: constants and fn prototypes for spell system       *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#ifndef _SPELLS_H_
#define _SPELLS_H_

#include "structs.h"

/* Forward declarations */
struct spell_info_type;

#define DEFAULT_STAFF_LVL 12
#define DEFAULT_WAND_LVL  12

#define CAST_UNDEFINED -1
#define CAST_SPELL     0
#define CAST_POTION    1
#define CAST_WAND      2
#define CAST_STAFF     3
#define CAST_SCROLL    4

#define TYPE_UNDEFINED     -1000
#define SPELL_RESERVED_DBC 0 /* SKILL NUMBER ZERO -- RESERVED */

/* Rates of improvement by use for skills */
#define SKUSE_FREQUENT 700
#define SKUSE_AVERAGE  200
#define SKUSE_RARE     50

/* Insert new spells here, up to MAX_SPELLS */
#define MAX_SPELLS 400

#define TOP_SPELL_DEFINE 599
/* NEW NPC/OBJECT SPELLS can be inserted here up to 599 */

/* WEAPON ATTACK TYPES */

#define TYPE_HIT        600
#define TYPE_STING      601
#define TYPE_WHIP       602
#define TYPE_SLASH      603
#define TYPE_BITE       604
#define TYPE_BLUDGEON   605
#define TYPE_CRUSH      606
#define TYPE_POUND      607
#define TYPE_CLAW       608
#define TYPE_MAUL       609
#define TYPE_THRASH     610
#define TYPE_PIERCE     611
#define TYPE_BLAST      612
#define TYPE_PUNCH      613
#define TYPE_STAB       614
#define TYPE_DUAL_WIELD 615

/* new attack types can be added here - up to TYPE_SUFFERING */
#define TYPE_SUFFERING 699

#define SAVING_PARA   0
#define SAVING_ROD    1
#define SAVING_PETRI  2
#define SAVING_BREATH 3
#define SAVING_SPELL  4

#define DIFF_EASY    0
#define DIFF_AVERAGE 1
#define DIFF_HARD    2
#define DIFF_VRHARD  3
#define DIFF_EXHARD  4

#define SPELL_TYPE_SPELL  0
#define SPELL_TYPE_POTION 1
#define SPELL_TYPE_WAND   2
#define SPELL_TYPE_STAFF  3
#define SPELL_TYPE_SCROLL 4

/* Attacktypes with grammar */

#define ASPELL(spellname) \
  void(spellname)(struct spell_info_type * sinfo, int waitstate, struct char_data *ch, char *arg, int isobj)

struct attack_hit_type {
  char *singular;
  char *plural;
  int damtype;
};

/* spell declarations */
#define DO_SPELL(sinfo, spellnum, waitstate, ch, arg, isobj) \
  (*spells[(spellnum)].spell_pointer)(sinfo, waitstate, ch, arg, isobj)

ASPELL(spell_dam);
ASPELL(spell_char);
ASPELL(spell_general);
ASPELL(spell_obj_char);
ASPELL(spell_obj);
ASPELL(spell_obj_room);
ASPELL(spell_confusion);
ASPELL(spell_turn_undead);
ASPELL(spell_dimdoor);
ASPELL(spell_create_water);
ASPELL(spell_teleport);
ASPELL(spell_summon);
ASPELL(spell_locate_object);
ASPELL(spell_charm_person);
ASPELL(spell_identify);
ASPELL(spell_enchant_weapon);
ASPELL(spell_enchant_armor);

/* basic magic calling functions */

int find_skill_num(char *name);

void mag_affects(int level, struct char_data *ch, struct char_data *victim, int spellnum, int savetype);

void mag_group_switch(int level, struct char_data *ch, struct char_data *tch, int spellnum, int savetype);

void mag_groups(int level, struct char_data *ch, int spellnum, int savetype);

void mag_masses(int level, struct char_data *ch, int spellnum, int savetype);

void mag_areas(byte level, struct char_data *ch, int spellnum, int savetype);

void mag_summons(int level, struct char_data *ch, struct obj_data *obj, int spellnum, int savetype);

void mag_unaffects(int level, struct char_data *ch, struct char_data *victim, int spellnum, int type);

void mag_alter_objs(int level, struct char_data *ch, struct obj_data *obj, int spellnum, int type);

void mag_creations(int level, struct char_data *ch, int spellnum);

void mag_objectmagic(struct char_data *ch, struct obj_data *obj, char *argument);
int mag_affect_char(struct spell_info_type *sinfo, int affect_flag, struct char_data *caster, struct char_data *vict,
                    int value);

int find_spell_num(char *name);

#endif /* _SPELLS_H_ */
