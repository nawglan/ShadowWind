/*
 magic.c
 Written by Fred Merkel
 Modified heavily for ShadowWind, by Desmond Daignault
 Copyright (C) 1993 Trustees of The Johns Hopkins Unversity
 All Rights Reserved.
 Based on DikuMUD, Copyright (C) 1990, 1991.
 */

#include <stdio.h>
#include <string.h>
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "event.h"
#include "spells.h"
#include "handler.h"
#include "db.h"

extern struct room_data *world;
extern struct obj_data *object_list;
extern struct char_data *character_list;
extern sh_int stats[11][101];
extern struct index_data *obj_index;
extern int SECS_PER_MUD_HOUR;

extern struct weather_data weather_info;
extern struct descriptor_data *descriptor_list;
extern struct zone_data *zone_table;
extern struct spell_info_type *spells;

extern int mini_mud;
extern int pk_allowed;
extern int SECS_PER_MUD_YEAR;
extern struct default_mobile_stats *mob_defaults;
extern char weapon_verbs[];
extern int *max_ac_applys;
extern struct apply_mod_defaults *apmd;

int find_skill_num_def(int define);
int find_spell_num(char *name);
void clearMemory(struct char_data * ch);
void act(char *str, int i, struct char_data * c, struct obj_data * o, void *vict_obj, int j);

void die(struct char_data * ch, struct char_data * killer);
void weight_change_object(struct obj_data * obj, int weight);
void add_follower(struct char_data * ch, struct char_data * leader);
int dice(int number, int size);
extern struct spell_info_type spell_info[];
extern byte class_saving_throws[NUM_CLASSES][5];
extern byte race_saving_throws[NUM_RACES][5];
extern byte mob_race_saving_throws[NUM_NPC_RACES][5];
void say_spell(struct char_data *ch, int spellnum, struct char_data *tch, struct obj_data *tobj);
int modBySpecialization(struct char_data *ch, struct spell_info_type *sinfo, int dam);

struct char_data *read_mobile(int, int);

int mag_savingthrow(struct char_data * ch, int type)
{
  int save;

  /* negative apply_saving_throw values make saving throws better! */
  if (!ch) {
    return 0;
  }

  if (IS_NPC(ch)) {
    save = class_saving_throws[(int) CLASS_WARRIOR][type] - (GET_LEVEL(ch) / 3);
    save += mob_race_saving_throws[(int) GET_RACE(ch)][type];
  } else {
    save = class_saving_throws[(int) GET_CLASS(ch)][type] - (GET_LEVEL(ch) / 4);
    save += race_saving_throws[(int) GET_RACE(ch)][type];
  }

  save += GET_SAVE(ch, type);

  /* throwing a 0 is always a failure */
  if (MAX(1, save) < number(0, 20))
    return TRUE;
  else
    return FALSE;
}

/* affect_update: called from comm.c (causes spells to wear off) */
void affect_update(void)
{
  static struct affected_type *af, *next;
  static struct char_data *i;

  for (i = character_list; i; i = i->next)
    for (af = i->affected; af; af = next) {
      next = af->next;
      if (af->duration >= 1)
        af->duration--;
      else if (af->duration == -1) /* No action */
        af->duration = -1; /* GODs only! unlimited */
      else {
        if ((af->type > 0) && (af->type <= MAX_SPELLS))
          if (!af->next || (af->next->type != af->type) || (af->next->duration > 0))
            if (spells[find_skill_num_def(af->type)].wear_off) {
              send_to_char(spells[find_skill_num_def(af->type)].wear_off, i);
              send_to_char("\r\n", i);
            }
        if (strcmp(spells[find_skill_num_def(af->type)].command, "wraith form") == 0) {
          die(i, NULL);
        } else
          affect_remove(i, af);
      }
    }
}

/*
 *  mag_materials:
 *  Checks for up to 3 vnums (spell reagents) in the player's inventory.
 *
 * No spells implemented in Circle 3.0 use mag_materials, but you can use
 * it to implement your own spells which require ingredients (i.e., some
 * heal spell which requires a rare herb or some such.)
 */
int mag_materials(struct char_data * ch, int item0, int item1, int item2, int extract, int verbose)
{
  struct obj_data *tobj = NULL;
  struct obj_data *obj0 = NULL, *obj1 = NULL, *obj2 = NULL;

  for (tobj = ch->carrying; tobj; tobj = tobj->next) {
    if ((item0 > 0) && (GET_OBJ_VNUM(tobj) == item0)) {
      obj0 = tobj;
      item0 = -1;
    } else if ((item1 > 0) && (GET_OBJ_VNUM(tobj) == item1)) {
      obj1 = tobj;
      item1 = -1;
    } else if ((item2 > 0) && (GET_OBJ_VNUM(tobj) == item2)) {
      obj2 = tobj;
      item2 = -1;
    }
  }
  if ((item0 > 0) || (item1 > 0) || (item2 > 0)) {
    if (verbose) {
      switch (number(0, 2)) {
        case 0:
          send_to_char("A wart sprouts on your nose.\r\n", ch);
          break;
        case 1:
          send_to_char("Your hair falls out in clumps.\r\n", ch);
          break;
        case 2:
          send_to_char("A huge corn develops on your big toe.\r\n", ch);
          break;
      }
    }
    return (FALSE);
  }
  if (extract) {
    if (item0 < 0) {
      obj_from_char(obj0);
      extract_obj(obj0);
    }
    if (item1 < 0) {
      obj_from_char(obj1);
      extract_obj(obj1);
    }
    if (item2 < 0) {
      obj_from_char(obj2);
      extract_obj(obj2);
    }
  }
  if (verbose) {
    send_to_char("A puff of smoke rises from your pack.\r\n", ch);
    act("A puff of smoke rises from $n's pack.", TRUE, ch, NULL, NULL, TO_ROOM);
  }
  return (TRUE);
}

int mag_affect_char(struct spell_info_type *sinfo, struct char_data *caster, struct char_data *vict, int level)
{
  struct affected_type af;
  int i;
  int spell = 0;
  int unaffect = 0;
  int circle = (level + 4) / 5;

  /* innate */
  if (level == -1) {
    SET_BIT(AFF_FLAGS(vict), (int) sinfo);
    return 1;
  }

  if (sinfo->unaffect) {
    spell = spells[find_spell_num(sinfo->unaffect)].spellindex;
    unaffect = 1;
  }

  if (!unaffect) {
    af.type = sinfo->spellindex;
    af.bitvector = sinfo->spell_plr_bit;
    af.bitvector2 = sinfo->spell_plr_bit2;
    af.bitvector3 = sinfo->spell_plr_bit3;

    if (sinfo->spell_duration)
      af.duration = sinfo->spell_duration * 12;
    else
      af.duration = (SECS_PER_MUD_HOUR * level) / 15; /* level/3 mud hours duration */

    af.duration = modBySpecialization(caster, sinfo, af.duration);
    gain_exp(caster, af.duration * 2);
    if (strcmp(sinfo->command, "stoneskin") == 0)
      af.duration = level * 5;
    for (i = 0; i < NUM_MODIFY; i++) {
      af.modifier[i] = 0;
      af.location[i] = 0;
    }

    for (i = 0; i < NUM_MODIFY; i++) {
      if (sinfo->plr_aff[i].location) {
        if (strcmp(sinfo->command, "vitality") == 0 || strcmp(sinfo->command, "vigorize") == 0)
          af.modifier[i] = (sinfo->plr_aff[i].modifier * level);
        else if (strcmp(sinfo->command, "barkskin") == 0) {
          af.modifier[i] = (sinfo->plr_aff[i].modifier - (3.5 * circle));
        } else
          af.modifier[i] = sinfo->plr_aff[i].modifier;
        af.location[i] = sinfo->plr_aff[i].location;
      }
    }
    if (IS_NPC(vict) && IS_AFFECTED(vict, sinfo->spell_plr_bit) && IS_AFFECTED2(vict, sinfo->spell_plr_bit2) && IS_AFFECTED3(vict, sinfo->spell_plr_bit3) && !affected_by_spell(vict, sinfo->spellindex))
      return 0;

    if (affected_by_spell(vict, sinfo->spellindex) && !(sinfo->accum_duration || sinfo->accum_affect))
      return 0;

    affect_join(vict, &af, sinfo->accum_duration, sinfo->avg_duration, sinfo->accum_affect, sinfo->avg_affect);
  } else {
    if (affected_by_spell(vict, sinfo->spellindex) && !(sinfo->accum_duration || sinfo->accum_affect))
      return 0;
    affect_from_char(vict, spell);
  }
  return 1;
}

void mag_affect_obj(struct char_data *caster, struct obj_data *object, struct spell_info_type *sinfo, int level)
{
  int i, j;

  if (object) {
    for (i = 0; i < NUM_MODIFY; i++) {
      switch (sinfo->obj_aff[i].location) {
        case OBJ_EXTRA:
          if (strcmp(sinfo->command, "darkness") == 0) {
            REMOVE_BIT(GET_OBJ_EXTRA(object), sinfo->spell_obj_bit);
            break;
          }
          if (!sinfo->spell_obj_bit)
            SET_BIT(GET_OBJ_EXTRA(object), sinfo->obj_aff[i].location);
          else
            SET_BIT(GET_OBJ_EXTRA(object), sinfo->spell_obj_bit);
          break;
        case OBJ_VALUE_0:
          if (strcmp(sinfo->command, "enchant shield") == 0) {
            if (GET_OBJ_WEAR(object) == ITEM_WEAR_SHIELD) {
              GET_OBJ_VAL(object, 0) += number((level / 20), sinfo->obj_aff[i].modifier);
            }
          } else {
            GET_OBJ_VAL(object, 0) += sinfo->obj_aff[i].modifier;
          }
          break;
        case OBJ_VALUE_1:
          GET_OBJ_VAL(object, 1) += sinfo->obj_aff[i].modifier;
          break;
        case OBJ_VALUE_2:
          GET_OBJ_VAL(object, 2) += sinfo->obj_aff[i].modifier;
          break;
        case OBJ_VALUE_3:
          GET_OBJ_VAL(object, 3) += sinfo->obj_aff[i].modifier;
          break;
        case OBJ_VALUE_4:
          GET_OBJ_VAL(object, 4) += sinfo->obj_aff[i].modifier;
          break;
        case OBJ_DAMROLL:
          if (strcmp(sinfo->command, "enchant weapon") == 0) {
            for (j = 0; j < MAX_OBJ_AFFECT; j++)
              if (object->affected[j].location != APPLY_NONE)
                break;
            if (j == MAX_OBJ_AFFECT) {
              object->affected[0].location = APPLY_DAMROLL;
              object->affected[0].modifier = number((level / 33), sinfo->obj_aff[i].modifier);
            }
          }
          break;
        case OBJ_HITROLL:
          if (strcmp(sinfo->command, "enchant weapon") == 0) {
            for (j = 0; j < MAX_OBJ_AFFECT; j++)
              if (object->affected[j].location != APPLY_NONE)
                break;
            if (j == MAX_OBJ_AFFECT) {
              object->affected[0].location = APPLY_HITROLL;
              object->affected[0].modifier = number((level / 33), sinfo->obj_aff[i].modifier);
            }
          }
          break;
        case OBJ_SAVING_PARA:
          for (j = 0; j < MAX_OBJ_AFFECT; j++)
            if (object->affected[j].location != APPLY_NONE)
              break;
          if (j == MAX_OBJ_AFFECT) {
            object->affected[0].location = APPLY_SAVING_PARA;
            object->affected[0].modifier = sinfo->obj_aff[i].modifier;
          }
          break;
        case OBJ_SAVING_ROD:
          for (j = 0; j < MAX_OBJ_AFFECT; j++)
            if (object->affected[j].location != APPLY_NONE)
              break;
          if (j == MAX_OBJ_AFFECT) {
            object->affected[0].location = APPLY_SAVING_ROD;
            object->affected[0].modifier = sinfo->obj_aff[i].modifier;
          }
          break;
        case OBJ_SAVING_PETRI:
          for (j = 0; j < MAX_OBJ_AFFECT; j++)
            if (object->affected[j].location != APPLY_NONE)
              break;
          if (j == MAX_OBJ_AFFECT) {
            object->affected[0].location = APPLY_SAVING_PETRI;
            object->affected[0].modifier = sinfo->obj_aff[i].modifier;
          }
          break;
        case OBJ_SAVING_BREATH:
          for (j = 0; j < MAX_OBJ_AFFECT; j++)
            if (object->affected[j].location != APPLY_NONE)
              break;
          if (j == MAX_OBJ_AFFECT) {
            object->affected[0].location = APPLY_SAVING_BREATH;
            object->affected[0].modifier = sinfo->obj_aff[i].modifier;
          }
          break;
        case OBJ_SAVING_SPELL:
          for (j = 0; j < MAX_OBJ_AFFECT; j++)
            if (object->affected[j].location != APPLY_NONE)
              break;
          if (j == MAX_OBJ_AFFECT) {
            object->affected[0].location = APPLY_SAVING_SPELL;
            object->affected[0].modifier = sinfo->obj_aff[i].modifier;
          }
          break;
        case OBJ_MAX_HIT:
          for (j = 0; j < MAX_OBJ_AFFECT; j++)
            if (object->affected[j].location != APPLY_NONE)
              break;
          if (j == MAX_OBJ_AFFECT) {
            object->affected[0].location = APPLY_MAX_HIT;
            object->affected[0].modifier = sinfo->obj_aff[i].modifier;
          }
          break;
        case OBJ_MAX_MANA:
          for (j = 0; j < MAX_OBJ_AFFECT; j++)
            if (object->affected[j].location != APPLY_NONE)
              break;
          if (j == MAX_OBJ_AFFECT) {
            object->affected[0].location = APPLY_MAX_MANA;
            object->affected[0].modifier = sinfo->obj_aff[i].modifier;
          }
          break;
        case OBJ_MAX_MOVE:
          for (j = 0; j < MAX_OBJ_AFFECT; j++)
            if (object->affected[j].location != APPLY_NONE)
              break;
          if (j == MAX_OBJ_AFFECT) {
            object->affected[0].location = APPLY_MAX_MOVE;
            object->affected[0].modifier = sinfo->obj_aff[i].modifier;
          }
          break;
        case OBJ_RES_LIGHT:
          GET_OBJ_RESIST(object, DAM_LIGHT) += sinfo->obj_aff[i].modifier;
          break;
        case OBJ_RES_DARK:
          GET_OBJ_RESIST(object, DAM_DARK) += sinfo->obj_aff[i].modifier;
          break;
        case OBJ_RES_FIRE:
          GET_OBJ_RESIST(object, DAM_FIRE) += sinfo->obj_aff[i].modifier;
          break;
        case OBJ_RES_COLD:
          GET_OBJ_RESIST(object, DAM_COLD) += sinfo->obj_aff[i].modifier;
          break;
        case OBJ_RES_ACID:
          GET_OBJ_RESIST(object, DAM_ACID) += sinfo->obj_aff[i].modifier;
          break;
        case OBJ_RES_POISON:
          GET_OBJ_RESIST(object, DAM_POISON) += sinfo->obj_aff[i].modifier;
          break;
        case OBJ_RES_DISEASE:
          GET_OBJ_RESIST(object, DAM_DISEASE) += sinfo->obj_aff[i].modifier;
          break;
        case OBJ_RES_CHARM:
          GET_OBJ_RESIST(object, DAM_CHARM) += sinfo->obj_aff[i].modifier;
          break;
        case OBJ_RES_SLEEP:
          GET_OBJ_RESIST(object, DAM_SLEEP) += sinfo->obj_aff[i].modifier;
          break;
        case OBJ_RES_SLASH:
          GET_OBJ_RESIST(object, DAM_SLASH) += sinfo->obj_aff[i].modifier;
          break;
        case OBJ_RES_PIERCE:
          GET_OBJ_RESIST(object, DAM_PIERCE) += sinfo->obj_aff[i].modifier;
          break;
        case OBJ_RES_BLUDGEON:
          GET_OBJ_RESIST(object, DAM_BLUDGEON) += sinfo->obj_aff[i].modifier;
          break;
        case OBJ_RES_NWEAP:
          GET_OBJ_RESIST(object, DAM_NWEAP) += sinfo->obj_aff[i].modifier;
          break;
        case OBJ_RES_MWEAP:
          GET_OBJ_RESIST(object, DAM_MWEAP) += sinfo->obj_aff[i].modifier;
          break;
        case OBJ_RES_MAGIC:
          GET_OBJ_RESIST(object, DAM_MAGIC) += sinfo->obj_aff[i].modifier;
          break;
        case OBJ_RES_ELECTRICITY:
          GET_OBJ_RESIST(object, DAM_ELECTRICITY) += sinfo->obj_aff[i].modifier;
          break;
        case OBJ_TIMER:
          GET_OBJ_TIMER(object) += (sinfo->obj_aff[i].modifier * level);
          break;
      }
    }
  }
}

int mag_points_char(struct spell_info_type *sinfo, struct char_data *caster, struct char_data *vict, int level)
{
  int modifier = dice(sinfo->num_dice, sinfo->size_dice);

  if (sinfo->unaffect)
    modifier = -modifier;

  modifier = modBySpecialization(caster, sinfo, modifier);
  gain_exp(caster, modifier * 2);
  switch (sinfo->point_loc) {
    case APPLY_AGE:
      vict->player.time.birth += (SECS_PER_MUD_YEAR * modifier);
      break;
    case APPLY_STR:
      GET_VSTR(vict) += modifier;
      GET_STR(vict) = MIN(100, GET_VSTR(vict));
      break;
    case APPLY_DEX:
      GET_VDEX(vict) += modifier;
      GET_DEX(vict) = MIN(100, GET_VDEX(vict));
      break;
    case APPLY_INT:
      GET_VINT(vict) += modifier;
      GET_INT(vict) = MIN(100, GET_VINT(vict));
      break;
    case APPLY_WIS:
      GET_VWIS(vict) += modifier;
      GET_WIS(vict) = MIN(100, GET_VWIS(vict));
      break;
    case APPLY_CON:
      GET_VCON(vict) += modifier;
      GET_CON(vict) = MIN(100, GET_VCON(vict));
      break;
    case APPLY_AGI:
      GET_VAGI(vict) += modifier;
      GET_AGI(vict) = MIN(100, GET_VAGI(vict));
      break;
    case APPLY_MANA:
      GET_MANA(vict) += modifier;
      if (GET_MANA(vict) > GET_MAX_MANA(vict))
        GET_MANA(vict) = GET_MAX_MANA(vict);
      break;
    case APPLY_HIT:
      GET_HIT(vict) += modifier;
      if (GET_HIT(vict) > GET_MAX_HIT(vict))
        GET_HIT(vict) = GET_MAX_HIT(vict);
      break;
    case APPLY_MOVE:
      GET_MOVE(vict) += modifier;
      if (GET_MOVE(vict) > GET_MAX_MOVE(vict))
        GET_MOVE(vict) = GET_MAX_MOVE(vict);
      break;
    case APPLY_HITROLL:
      GET_HITROLL(vict) += modifier;
      break;
    case APPLY_DAMROLL:
      GET_DAMROLL(vict) += modifier;
      break;
    case APPLY_SAVING_PARA:
      GET_SAVE(vict,SAVING_PARA) += modifier;
      break;
    case APPLY_SAVING_ROD:
      GET_SAVE(vict,SAVING_ROD) += modifier;
      break;
    case APPLY_SAVING_PETRI:
      GET_SAVE(vict,SAVING_PETRI) += modifier;
      break;
    case APPLY_SAVING_BREATH:
      GET_SAVE(vict,SAVING_BREATH) += modifier;
      break;
    case APPLY_SAVING_SPELL:
      GET_SAVE(vict,SAVING_SPELL) += modifier;
      break;
    case APPLY_MAX_HIT:
      GET_MAX_HIT(vict) += modifier;
      break;
    case APPLY_MAX_MANA:
      GET_MAX_MANA(vict) += modifier;
      break;
    case APPLY_MAX_MOVE:
      GET_MAX_MOVE(vict) += modifier;
      break;
    case APPLY_RES_DARK:
      GET_RESIST(vict, DAM_DARK) += modifier;
      break;
    case APPLY_RES_FIRE:
      GET_RESIST(vict, DAM_FIRE) += modifier;
      break;
    case APPLY_RES_COLD:
      GET_RESIST(vict, DAM_COLD) += modifier;
      break;
    case APPLY_RES_ACID:
      GET_RESIST(vict, DAM_ACID) += modifier;
      break;
    case APPLY_RES_POISON:
      GET_RESIST(vict, DAM_POISON) += modifier;
      break;
    case APPLY_RES_DISEASE:
      GET_RESIST(vict, DAM_DISEASE) += modifier;
      break;
    case APPLY_RES_CHARM:
      GET_RESIST(vict, DAM_CHARM) += modifier;
      break;
    case APPLY_RES_SLEEP:
      GET_RESIST(vict, DAM_SLEEP) += modifier;
      break;
    case APPLY_RES_SLASH:
      GET_RESIST(vict, DAM_SLASH) += modifier;
      break;
    case APPLY_RES_PIERCE:
      GET_RESIST(vict, DAM_PIERCE) += modifier;
      break;
    case APPLY_RES_BLUDGEON:
      GET_RESIST(vict, DAM_BLUDGEON) += modifier;
      break;
    case APPLY_RES_NWEAP:
      GET_RESIST(vict, DAM_NWEAP) += modifier;
      break;
    case APPLY_RES_MWEAP:
      GET_RESIST(vict, DAM_MWEAP) += modifier;
      break;
    case APPLY_RES_MAGIC:
      GET_RESIST(vict, DAM_MAGIC) += modifier;
      break;
    case APPLY_RES_ELECTRICITY:
      GET_RESIST(vict, DAM_ELECTRICITY) += modifier;
      break;
  }
  return 1;
}
