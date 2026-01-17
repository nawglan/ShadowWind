/* ************************************************************************
 *   File: handler.c                                     Part of CircleMUD *
 *  Usage: internal funcs: moving and finding chars/objs                   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#include <assert.h>
#include <ctype.h>
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

/* external vars */
extern struct spell_info_type *spells;
extern int top_of_world;
extern struct room_data *world;
extern struct obj_data *object_list;
extern struct char_data *character_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct descriptor_data *descriptor_list;
extern struct zone_data *zone_table;
extern char *MENU;
extern int SECS_PER_MUD_YEAR;

/* external functions */
void remove_consent(struct char_data *ch);
int find_skill_num_def(int define);
void add_innates(struct char_data *ch);
void free_char(struct char_data *ch);
void stop_fighting(struct char_data *ch);
void clearMemory(struct char_data *ch);
void mob_delay_purge(struct char_data *ch);
bool clean_events(void *pointer, EVENT(*func));
char *strip_color(char *from, char *to, int length);

char *fname(char *namelist) {
  static char holder[30];
  register char *point;

  for (point = holder; isalpha(*namelist); namelist++, point++)
    *point = *namelist;

  *point = '\0';

  return (holder);
}

/* check if a player is allowed to access a certain room */
int check_access(struct char_data *ch, int room) {
  int oktosee = 0;
  int zonenum = 0;
  int i;

  if (IS_NPC(ch) || GET_LEVEL(ch) < LVL_IMMORT)
    return 1;
  if (PLR_FLAGGED(ch, PLR_STAYGOD) && world[room].zone != world[real_room(1206)].zone)
    return 0;

  zonenum = (world[room].number - (world[room].number % 100)) / 100;
  if (zonenum != 12 && zonenum != 30 && zonenum != 31 && !COM_FLAGGED(ch, COM_ADMIN)) {
    for (i = 0; i < 4; i++) {
      if (ch->olc_zones[i] == zonenum) {
        return 1;
      }
    }
    if (!oktosee || zonenum == 0) {
      return 0;
    }
  }

  if (IS_SET(zone_table[world[room].zone].bits, ZONE_RESTRICTED) && !PLR_FLAGGED(ch, PLR_ZONEOK) &&
      !COM_FLAGGED(ch, COM_QUEST))
    return 0;

  return 1;
}

int isname(char *str, char *namelist) {
  char *curstr;
  char temp[80];
  char *p;

  if (!str || !namelist)
    return 0;
  if (!*str || !*namelist)
    return 0;
  memset(temp, 0, 80);
  strip_color(namelist, temp, strlen(namelist));
  p = temp;

  while ((curstr = strtok(p, " \n"))) {
    if (!strcasecmp(curstr, str))
      return 1;
    p = NULL;
  }
  return 0;
}

void obj_affect_modify(struct char_data *ch, int loc, int mod, long bitv, long bitv2, long bitv3, bool add) {
  char abuf[80];

  if (add) {
    SET_BIT(AFF_FLAGS(ch), bitv);
    SET_BIT(AFF2_FLAGS(ch), bitv2);
  } else {
    REMOVE_BIT(AFF_FLAGS(ch), bitv);
    REMOVE_BIT(AFF2_FLAGS(ch), bitv2);
    mod = -mod;
  }

  switch (loc) {
  case APPLY_NONE:
    break;

  case APPLY_STR:
    GET_VSTR(ch) += mod;
    GET_STR(ch) = MIN(100, GET_VSTR(ch));
    break;

  case APPLY_DEX:
    GET_VDEX(ch) += mod;
    GET_DEX(ch) = MIN(100, GET_VDEX(ch));
    break;

  case APPLY_INT:
    GET_VINT(ch) += mod;
    GET_INT(ch) = MIN(100, GET_VINT(ch));
    break;

  case APPLY_WIS:
    GET_VWIS(ch) += mod;
    GET_WIS(ch) = MIN(100, GET_VWIS(ch));
    break;

  case APPLY_CON:
    GET_VCON(ch) += mod;
    GET_CON(ch) = MIN(100, GET_VCON(ch));
    break;

  case APPLY_AGI:
    GET_VAGI(ch) += mod;
    GET_AGI(ch) = MIN(100, GET_VAGI(ch));
    break;

  case APPLY_AGE:
    ch->player.time.birth -= (mod * SECS_PER_MUD_YEAR);
    break;

  case APPLY_CHAR_WEIGHT:
    GET_WEIGHT(ch) += mod;
    break;

  case APPLY_CHAR_HEIGHT:
    GET_HEIGHT(ch) += mod;
    break;

  case APPLY_MANA:
    GET_MAX_MANA(ch) += mod;
    GET_MANA(ch) += mod;
    break;

  case APPLY_HIT:
    GET_MAX_HIT(ch) += mod;
    GET_HIT(ch) += mod;
    break;

  case APPLY_MOVE:
    GET_MAX_MOVE(ch) += mod;
    GET_MOVE(ch) += mod;
    break;

  case APPLY_AC:
    if (!IS_MONK(ch))
      GET_AC(ch) += mod;
    break;

  case APPLY_HITROLL:
    if ((GET_HITROLL(ch) >= -120) && (GET_HITROLL(ch) <= 120)) {
      if (IS_MOB(ch)) {
        GET_HITROLL(ch) -= mod;
      } else {
        GET_HITROLL(ch) += mod;
      }
    }
    break;

  case APPLY_DAMROLL:
    if ((GET_DAMROLL(ch) >= -120) && (GET_DAMROLL(ch) <= 120)) {
      GET_DAMROLL(ch) += mod;
    }
    break;

  case APPLY_SAVING_PARA:
    GET_SAVE(ch, SAVING_PARA) += mod;
    break;

  case APPLY_SAVING_ROD:
    GET_SAVE(ch, SAVING_ROD) += mod;
    break;

  case APPLY_SAVING_PETRI:
    GET_SAVE(ch, SAVING_PETRI) += mod;
    break;

  case APPLY_SAVING_BREATH:
    GET_SAVE(ch, SAVING_BREATH) += mod;
    break;

  case APPLY_SAVING_SPELL:
    GET_SAVE(ch, SAVING_SPELL) += mod;
    break;

  case APPLY_MAX_HIT:
    GET_MAX_HIT(ch) += mod;
    break;

  case APPLY_MAX_MANA:
    GET_MAX_MANA(ch) += mod;
    break;

  case APPLY_MAX_MOVE:
    GET_MAX_MOVE(ch) += mod;
    break;

  default:
    stderr_log("SYSERR: Unknown apply adjust attempt (affect_modify).");
    safe_snprintf(abuf, sizeof(abuf), "Name = %s, apply adjust = %d", GET_NAME(ch), loc);
    stderr_log(abuf);
    break;
  } /* switch */
}

void affect_modify(struct char_data *ch, int loc[], int mod[], long bitv, long bitv2, long bitv3, bool add) {
  int i;
  int location[NUM_MODIFY];
  int modifier[NUM_MODIFY];
  struct affected_type *temp;
  struct affected_type *af = ch->affected;

  for (i = 0; i < NUM_MODIFY; i++) {
    if (loc[i] > NUM_APPLIES || loc[i] < 0) {
      REMOVE_FROM_LIST(af, ch->affected, next);
      FREE(af);
      affect_total(ch);
      return;
    }
    location[i] = loc[i];
    modifier[i] = mod[i];
  }

  if (add) {
    SET_BIT(AFF_FLAGS(ch), bitv);
    SET_BIT(AFF2_FLAGS(ch), bitv2);
    SET_BIT(AFF3_FLAGS(ch), bitv3);
  } else {
    REMOVE_BIT(AFF_FLAGS(ch), bitv);
    REMOVE_BIT(AFF2_FLAGS(ch), bitv2);
    REMOVE_BIT(AFF3_FLAGS(ch), bitv3);
    for (i = 0; i < NUM_MODIFY; i++)
      if (location[i])
        modifier[i] = -modifier[i];
  }

  for (i = 0; i < NUM_MODIFY; i++) {
    if (location[i])
      switch (location[i]) {
      case APPLY_NONE:
        break;

      case APPLY_STR:
        GET_VSTR(ch) += modifier[i];
        GET_STR(ch) = MIN(100, GET_VSTR(ch));
        break;

      case APPLY_DEX:
        GET_VDEX(ch) += modifier[i];
        GET_DEX(ch) = MIN(100, GET_VDEX(ch));
        break;

      case APPLY_INT:
        GET_VINT(ch) += modifier[i];
        GET_INT(ch) = MIN(100, GET_VINT(ch));
        break;

      case APPLY_WIS:
        GET_VWIS(ch) += modifier[i];
        GET_WIS(ch) = MIN(100, GET_VWIS(ch));
        break;

      case APPLY_CON:
        GET_VCON(ch) += modifier[i];
        GET_CON(ch) = MIN(100, GET_VCON(ch));
        break;

      case APPLY_AGI:
        GET_VAGI(ch) += modifier[i];
        GET_AGI(ch) = MIN(100, GET_VAGI(ch));
        break;

      case APPLY_AGE:
        ch->player.time.birth -= (modifier[i] * SECS_PER_MUD_YEAR);
        break;

      case APPLY_CHAR_WEIGHT:
        GET_WEIGHT(ch) += modifier[i];
        break;

      case APPLY_CHAR_HEIGHT:
        GET_HEIGHT(ch) += modifier[i];
        break;

      case APPLY_MANA:
        GET_MANA(ch) += modifier[i];
        GET_MAX_MANA(ch) += modifier[i];
        break;

      case APPLY_MAX_MANA:
        GET_MAX_MANA(ch) += modifier[i];
        break;

      case APPLY_HIT:
        GET_HIT(ch) += modifier[i];
        GET_MAX_HIT(ch) += modifier[i];
        break;

      case APPLY_MAX_HIT:
        GET_MAX_HIT(ch) += modifier[i];
        break;

      case APPLY_MOVE:
        GET_MOVE(ch) += modifier[i];
        GET_MAX_MOVE(ch) += modifier[i];
        break;

      case APPLY_MAX_MOVE:
        GET_MAX_MOVE(ch) += modifier[i];
        break;

      case APPLY_AC:
        if (!IS_MONK(ch))
          GET_AC(ch) += modifier[i];
        break;

      case APPLY_HITROLL:
        if ((GET_HITROLL(ch) >= -120) && (GET_HITROLL(ch) <= 120))
          GET_HITROLL(ch) += modifier[i];
        break;

      case APPLY_DAMROLL:
        if ((GET_DAMROLL(ch) >= -120) && (GET_DAMROLL(ch) <= 120))
          GET_DAMROLL(ch) += modifier[i];
        break;

      case APPLY_SAVING_PARA:
        GET_SAVE(ch, SAVING_PARA) += modifier[i];
        break;

      case APPLY_SAVING_ROD:
        GET_SAVE(ch, SAVING_ROD) += modifier[i];
        break;

      case APPLY_SAVING_PETRI:
        GET_SAVE(ch, SAVING_PETRI) += modifier[i];
        break;

      case APPLY_SAVING_BREATH:
        GET_SAVE(ch, SAVING_BREATH) += modifier[i];
        break;

      case APPLY_SAVING_SPELL:
        GET_SAVE(ch, SAVING_SPELL) += modifier[i];
        break;

      case APPLY_RES_LIGHT:
        GET_RESIST(ch, DAM_LIGHT) += modifier[i];
        break;

      case APPLY_RES_DARK:
        GET_RESIST(ch, DAM_DARK) += modifier[i];
        break;

      case APPLY_RES_FIRE:
        GET_RESIST(ch, DAM_FIRE) += modifier[i];
        break;

      case APPLY_RES_COLD:
        GET_RESIST(ch, DAM_COLD) += modifier[i];
        break;

      case APPLY_RES_ACID:
        GET_RESIST(ch, DAM_ACID) += modifier[i];
        break;

      case APPLY_RES_POISON:
        GET_RESIST(ch, DAM_POISON) += modifier[i];
        break;

      case APPLY_RES_DISEASE:
        GET_RESIST(ch, DAM_DISEASE) += modifier[i];
        break;

      case APPLY_RES_CHARM:
        GET_RESIST(ch, DAM_CHARM) += modifier[i];
        break;

      case APPLY_RES_SLEEP:
        GET_RESIST(ch, DAM_SLEEP) += modifier[i];
        break;

      case APPLY_RES_SLASH:
        GET_RESIST(ch, DAM_SLASH) += modifier[i];
        break;

      case APPLY_RES_PIERCE:
        GET_RESIST(ch, DAM_PIERCE) += modifier[i];
        break;

      case APPLY_RES_BLUDGEON:
        GET_RESIST(ch, DAM_BLUDGEON) += modifier[i];
        break;

      case APPLY_RES_NWEAP:
        GET_RESIST(ch, DAM_NWEAP) += modifier[i];
        break;

      case APPLY_RES_MWEAP:
        GET_RESIST(ch, DAM_MWEAP) += modifier[i];
        break;

      case APPLY_RES_MAGIC:
        GET_RESIST(ch, DAM_MAGIC) += modifier[i];
        break;

      case APPLY_RES_ELECTRICITY:
        GET_RESIST(ch, DAM_ELECTRICITY) += modifier[i];
        break;

      default:
        stderr_log("SYSERR: Unknown apply adjust attempt (affect_modify).");
        break;
      } /* switch */
  }
}

/* This updates a character by subtracting everything he is affected by */
/* restoring original abilities, and then affecting all again           */
void affect_total(struct char_data *ch) {
  struct affected_type *af;
  int i, j, c;

  for (i = 0; i < NUM_WEARS; i++) {
    if (ch->equipment[i]) {
      for (j = 0; j < MAX_OBJ_AFFECT; j++)
        obj_affect_modify(ch, ch->equipment[i]->affected[j].location, ch->equipment[i]->affected[j].modifier,
                          ch->equipment[i]->obj_flags.bitvector, ch->equipment[i]->obj_flags.bitvector2,
                          ch->equipment[i]->obj_flags.bitvector3, FALSE);
      for (c = 0; c < MAX_DAM_TYPE; c++)
        GET_RESIST(ch, c) -= ch->equipment[i]->resists[c];
    }
  }

  for (af = ch->affected; af; af = af->next)
    affect_modify(ch, af->location, af->modifier, af->bitvector, af->bitvector2, af->bitvector3, FALSE);

  ch->aff_abils = ch->real_abils;
  GET_VSTR(ch) = GET_STR(ch);
  GET_VDEX(ch) = GET_DEX(ch);
  GET_VINT(ch) = GET_INT(ch);
  GET_VWIS(ch) = GET_WIS(ch);
  GET_VCON(ch) = GET_CON(ch);
  GET_VAGI(ch) = GET_AGI(ch);

  for (i = 0; i < NUM_WEARS; i++) {
    if (ch->equipment[i]) {
      for (j = 0; j < MAX_OBJ_AFFECT; j++)
        obj_affect_modify(ch, ch->equipment[i]->affected[j].location, ch->equipment[i]->affected[j].modifier,
                          ch->equipment[i]->obj_flags.bitvector, ch->equipment[i]->obj_flags.bitvector2,
                          ch->equipment[i]->obj_flags.bitvector3, TRUE);
      for (c = 0; c < MAX_DAM_TYPE; c++)
        GET_RESIST(ch, c) += ch->equipment[i]->resists[c];
    }
  }

  for (af = ch->affected; af; af = af->next)
    affect_modify(ch, af->location, af->modifier, af->bitvector, af->bitvector2, af->bitvector3, TRUE);

  /* Make sure certain values are between 0..100 */

  GET_DEX(ch) = BOUNDED(0, GET_DEX(ch), 100);
  GET_INT(ch) = BOUNDED(0, GET_INT(ch), 100);
  GET_WIS(ch) = BOUNDED(0, GET_WIS(ch), 100);
  GET_CON(ch) = BOUNDED(0, GET_CON(ch), 100);
  GET_STR(ch) = BOUNDED(0, GET_STR(ch), 100);
}

/* Insert an affect_type in a char_data structure
 Automatically sets apropriate bits and apply's */
void affect_to_char(struct char_data *ch, struct affected_type *af) {
  struct affected_type *affected_alloc;

  CREATE(affected_alloc, struct affected_type, 1);

  *affected_alloc = *af;
  affected_alloc->next = ch->affected;
  ch->affected = affected_alloc;

  affect_modify(ch, af->location, af->modifier, af->bitvector, af->bitvector2, af->bitvector3, TRUE);
  affect_total(ch);
}

/*
 * Remove an affected_type structure from a char (called when duration
 * reaches zero). Pointer *af must never be NIL!  Frees mem and calls
 * affect_location_apply
 */
void affect_remove(struct char_data *ch, struct affected_type *af) {
  struct affected_type *temp;

  assert(ch->affected);

  affect_modify(ch, af->location, af->modifier, af->bitvector, af->bitvector2, af->bitvector3, FALSE);
  REMOVE_FROM_LIST(af, ch->affected, next);
  FREE(af);
  affect_total(ch);
}

/* Call affect_remove with every spell of spelltype "skill" */
void affect_from_char(struct char_data *ch, sh_int type) {
  struct affected_type *hjp;
  struct affected_type *next_hjp;

  for (hjp = ch->affected; hjp; hjp = next_hjp) {
    next_hjp = hjp->next;
    if (hjp->type == type)
      affect_remove(ch, hjp);
  }
  add_innates(ch);
}

int reduce_affect(struct char_data *ch, int type, int how_much) {
  struct affected_type *hjp;
  struct affected_type *nexthjp;
  int retval = 0;

  for (hjp = ch->affected; hjp; hjp = nexthjp) {
    nexthjp = hjp->next;
    if (hjp->type == type) {
      retval = 1;
      hjp->duration -= how_much;
      if (hjp->duration <= 0) {
        if (spells[find_skill_num_def(hjp->type)].wear_off) {
          send_to_char(spells[find_skill_num_def(hjp->type)].wear_off, ch);
          send_to_char("\r\n", ch);
        }
        affect_from_char(ch, hjp->type);
      }
    }
  }
  return retval;
}

/*
 * Return if a char is affected by a spell (SPELL_XXX), NULL indicates
 * not affected
 */
bool affected_by_spell(struct char_data *ch, sh_int type) {
  struct affected_type *hjp;

  for (hjp = ch->affected; hjp; hjp = hjp->next)
    if (hjp->type == type)
      return TRUE;

  return FALSE;
}

void affect_join(struct char_data *ch, struct affected_type *af, bool add_dur, bool avg_dur, bool add_mod,
                 bool avg_mod) {
  struct affected_type *hjp;
  bool found = FALSE;
  int i;

  for (hjp = ch->affected; !found && hjp; hjp = hjp->next) {

    if ((hjp->type == af->type) && (hjp->location == af->location)) {
      if (add_dur)
        af->duration += hjp->duration;
      if (avg_dur)
        af->duration = (af->duration + hjp->duration) >> 1;

      if (add_mod)
        for (i = 0; i < NUM_MODIFY; i++)
          af->modifier[i] += hjp->modifier[i];
      if (avg_mod)
        for (i = 0; i < NUM_MODIFY; i++)
          af->modifier[i] = (hjp->modifier[i] + af->modifier[i]) >> 1;

      affect_remove(ch, hjp);
      affect_to_char(ch, af);
      found = TRUE;
    }
  }
  if (!found)
    affect_to_char(ch, af);
}

/* move a player out of a room */
void char_from_room(struct char_data *ch) {
  struct char_data *temp;
  int i;
  struct obj_data *tobj;
  struct obj_data *tobj_next;

  if (ch == NULL || ch->in_room == NOWHERE) {
    stderr_log("SYSERR: NULL or NOWHERE in handler.c, char_from_room");
    fflush(NULL);
    exit(1);
  }
  if (AFF2_FLAGGED(ch, AFF2_FIRESHIELD)) {
    world[ch->in_room].light--;
  }
  for (tobj = ch->carrying; tobj; tobj = tobj_next) {
    tobj_next = tobj->next_content;
    if (IS_OBJ_STAT(tobj, ITEM_ISLIGHT)) {
      world[ch->in_room].light--;
    }
  }
  for (i = 0; i < NUM_WEARS; i++) {
    if (ch->equipment[i] != NULL) {
      if (GET_OBJ_TYPE(ch->equipment[i]) == ITEM_LIGHT) {
        if (GET_OBJ_VAL(ch->equipment[i], 2)) { /* Light is ON */
          world[ch->in_room].light--;
        }
      } else {
        if (IS_OBJ_STAT(ch->equipment[i], ITEM_ISLIGHT)) {
          world[ch->in_room].light--;
        }
      }
    }
  }

  REMOVE_FROM_LIST(ch, world[ch->in_room].people, next_in_room);
  ch->in_room = NOWHERE;
  ch->next_in_room = NULL;
}

/* place a character in a room */
void char_to_room(struct char_data *ch, int room) {
  int i;
  struct obj_data *tobj;
  struct obj_data *tobj_next;

  if (!ch || room < 0 || room > top_of_world) {
    stderr_log("SYSERR: Illegal value(s) passed to char_to_room");
  } else {
    ch->next_in_room = world[room].people;
    world[room].people = ch;
    ch->in_room = room;

    if (AFF2_FLAGGED(ch, AFF2_FIRESHIELD)) {
      world[room].light++;
    }
    for (tobj = ch->carrying; tobj; tobj = tobj_next) {
      tobj_next = tobj->next_content;
      if (IS_OBJ_STAT(tobj, ITEM_ISLIGHT)) {
        world[room].light++;
      }
    }
    for (i = 0; i < NUM_WEARS; i++) {
      if (ch->equipment[i]) {
        if (GET_OBJ_TYPE(ch->equipment[i]) == ITEM_LIGHT) {
          if (GET_OBJ_VAL(ch->equipment[i], 2)) { /* Light ON */
            world[room].light++;
          }
        } else {
          if (IS_OBJ_STAT(ch->equipment[i], ITEM_ISLIGHT)) {
            world[room].light++;
          }
        }
      }
    }
  }
}

/* give an object to a char   */
void obj_to_char(struct obj_data *object, struct char_data *ch) {
  if (object && ch) {
    object->next_content = ch->carrying;
    ch->carrying = object;
    object->carried_by = ch;
    object->in_room = NOWHERE;
    object->in_obj = NULL;
    IS_CARRYING_W(ch) += GET_OBJ_WEIGHT(object);
    IS_CARRYING_N(ch)++;

    /* set flag for crash-save system */
    SET_BIT(PLR_FLAGS(ch), PLR_CRASH);
    if (GET_OBJ_RNUM(object) > -1) {
      if (obj_index[GET_OBJ_RNUM(object)].qic) {
        safe_snprintf(logbuffer, sizeof(logbuffer), "%s to character %s", object->short_description, GET_NAME(ch));
        mudlog(logbuffer, 'J', COM_ADMIN, FALSE);
      }
    }
  } else
    stderr_log("SYSERR: NULL obj or char passed to obj_to_char");
}

/* take an object from a char */
void obj_from_char(struct obj_data *object) {
  struct obj_data *temp;

  if (object == NULL) {
    stderr_log("SYSERR: NULL object passed to obj_from_char");
    return;
  }
  if (object->carried_by == NULL) {
    stderr_log("SYSERR: NULL object->carried_by in obj_from_char");
    return;
  }
  REMOVE_FROM_LIST(object, object->carried_by->carrying, next_content);

  /* set flag for crash-save system */
  SET_BIT(PLR_FLAGS(object->carried_by), PLR_CRASH);
  if (GET_OBJ_RNUM(object) > -1)
    if (obj_index[GET_OBJ_RNUM(object)].qic) {
      safe_snprintf(logbuffer, sizeof(logbuffer), "%s from character %s", object->short_description,
                    GET_NAME(object->carried_by));
      mudlog(logbuffer, 'J', COM_ADMIN, FALSE);
    }
  IS_CARRYING_W(object->carried_by) -= GET_OBJ_WEIGHT(object);
  IS_CARRYING_N(object->carried_by)--;
  object->carried_by = NULL;
  object->next_content = NULL;
}

/* Return the effect of a piece of armor in position eq_pos */
int apply_ac(struct char_data *ch, int eq_pos) {
  int factor;

  assert(ch->equipment[eq_pos]);

  if (!(GET_OBJ_TYPE(ch->equipment[eq_pos]) == ITEM_ARMOR))
    return 0;
  if (IS_MONK(ch))
    return 0;

  switch (eq_pos) {

  case WEAR_BODY:
    factor = 3;
    break; /* 30% */
  case WEAR_HEAD:
    factor = 2;
    break; /* 20% */
  case WEAR_LEGS:
    factor = 2;
    break; /* 20% */
  default:
    factor = 1;
    break; /* all others 10% */
  }

  return (factor * GET_OBJ_VAL(ch->equipment[eq_pos], 0));
}

void equip_char(struct char_data *ch, struct obj_data *obj, int pos) {
  int j;
  int invalid_class(struct char_data * ch, struct obj_data * obj);

  if (!(pos >= 0 && pos < NUM_WEARS)) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "SYSERR: Invalid position (ch: %s, obj: %s, pos: %d)", GET_NAME(ch),
                  obj->short_description, pos);
    stderr_log(buf);
    return;
  }

  if (ch->equipment[pos]) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "SYSERR: Char is already equipped: %s, %s", GET_NAME(ch),
                  obj->short_description);
    stderr_log(buf);
    return;
  }
  if (obj->carried_by != NULL) {
    stderr_log("SYSERR: EQUIP: Obj is carried_by when equip.");
    return;
  }
  if (obj->in_room != NOWHERE) {
    stderr_log("SYSERR: EQUIP: Obj is in_room when equip.");
    return;
  }
  if ((IS_OBJ_STAT(obj, ITEM_ANTI_EVIL) && IS_EVIL(ch)) || (IS_OBJ_STAT(obj, ITEM_ANTI_GOOD) && IS_GOOD(ch)) ||
      (IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch)) || invalid_class(ch, obj)) {
    if (ch->in_room != NOWHERE) {
      act("You are zapped by $p and instantly let go of it.", FALSE, ch, obj, 0, TO_CHAR);
      act("$n is zapped by $p and instantly lets go of it.", FALSE, ch, obj, 0, TO_ROOM);
    }
    REMOVE_BIT(CHAR_WEARING(ch), GET_OBJ_SLOTS(obj));
    obj_to_char(obj, ch); /* drop in inventory instead of ground */
    return;
  }

  ch->equipment[pos] = obj;
  CHAR_WEARING(ch) |= GET_OBJ_SLOTS(obj);
  obj->worn_by = ch;
  obj->worn_on = pos;

  IS_CARRYING_W(ch) += (GET_OBJ_WEIGHT(obj) >> 1);

  if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
    GET_AC(ch) -= apply_ac(ch, pos);

  if (ch->in_room != NOWHERE) {
    if (GET_OBJ_TYPE(obj) == ITEM_LIGHT) {
      if (GET_OBJ_VAL(obj, 2)) { /* if light is ON */
        world[ch->in_room].light++;
        if ((GET_OBJ_VAL(obj, 2) >= 2) || (GET_OBJ_VAL(obj, 2) == -1))
          SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ISLIGHT);
        else
          SET_BIT(GET_OBJ_EXTRA(obj), ITEM_ISLIGHTDIM);
      }
    } else if (IS_OBJ_STAT(obj, ITEM_ISLIGHT))
      world[ch->in_room].light++;
  } /* else
   stderr_log("SYSERR: ch->in_room = NOWHERE when equipping char."); */

  for (j = 0; j < MAX_OBJ_AFFECT; j++)
    obj_affect_modify(ch, obj->affected[j].location, obj->affected[j].modifier, GET_OBJ_BITV(obj), GET_OBJ_BITV2(obj),
                      GET_OBJ_BITV3(obj), TRUE);

  for (j = 0; j < MAX_DAM_TYPE; j++)
    GET_RESIST(ch, j) += obj->resists[j];

  affect_total(ch);
}

struct obj_data *unequip_char(struct char_data *ch, int pos) {
  int j;
  struct obj_data *obj;

  assert(pos >= 0 && pos < NUM_WEARS);
  assert(ch->equipment[pos]);

  obj = ch->equipment[pos];
  obj->worn_by = NULL;
  obj->worn_on = -1;

  if (GET_OBJ_TYPE(obj) == ITEM_ARMOR)
    GET_AC(ch) += apply_ac(ch, pos);

  IS_CARRYING_W(ch) -= (GET_OBJ_WEIGHT(obj) >> 1);

  if (ch->in_room != NOWHERE) {
    if (GET_OBJ_TYPE(obj) == ITEM_LIGHT) {
      if (GET_OBJ_VAL(obj, 2)) { /* if light is ON */
        if (ch->in_room > NOWHERE && ch->in_room <= top_of_world)
          world[ch->in_room].light--;
        if (GET_OBJ_VAL(obj, 2) >= 2)
          REMOVE_BIT(GET_OBJ_EXTRA(obj), ITEM_ISLIGHT);
        else
          REMOVE_BIT(GET_OBJ_EXTRA(obj), ITEM_ISLIGHTDIM);
      }
    } else if (IS_OBJ_STAT(obj, ITEM_ISLIGHT))
      if (ch->in_room > NOWHERE && ch->in_room <= top_of_world)
        world[ch->in_room].light--;
  } /* else
   stderr_log("SYSERR: ch->in_room = NOWHERE when unequipping char."); */

  ch->equipment[pos] = NULL;

  for (j = 0; j < MAX_OBJ_AFFECT; j++)
    obj_affect_modify(ch, obj->affected[j].location, obj->affected[j].modifier, GET_OBJ_BITV(obj), GET_OBJ_BITV2(obj),
                      GET_OBJ_BITV3(obj), FALSE);

  for (j = 0; j < MAX_DAM_TYPE; j++)
    GET_RESIST(ch, j) -= obj->resists[j];

  affect_total(ch);

  return (obj);
}

int get_number(char **name) {
  int i;
  char *ppos;
  char number[MAX_INPUT_LENGTH];

  *number = '\0';

  if ((ppos = strchr(*name, '.'))) {
    *ppos++ = '\0';
    safe_snprintf(number, sizeof(number), "%s", *name);
    memmove(*name, ppos, strlen(ppos) + 1);

    for (i = 0; *(number + i); i++)
      if (!isdigit(*(number + i)))
        return 0;

    return (atoi(number));
  }
  return 1;
}

/* Search a given list for an object number, and return a ptr to that obj */
struct obj_data *get_obj_in_list_num(int num, struct obj_data *list) {
  struct obj_data *i;

  for (i = list; i != NULL; i = i->next_content)
    if (GET_OBJ_RNUM(i) == num)
      return i;

  return NULL;
}

/* Search a given list for an object vnum, and return a ptr to that obj */
struct obj_data *get_obj_in_list_vnum(int vnum, struct obj_data *list) {
  struct obj_data *i;

  for (i = list; i != NULL; i = i->next_content)
    if (GET_OBJ_VNUM(i) == vnum)
      return i;

  return NULL;
}

/* search the entire world for an object number, and return a pointer  */
struct obj_data *get_obj_num(int nr) {
  struct obj_data *i;

  for (i = object_list; i != NULL; i = i->next)
    if (GET_OBJ_RNUM(i) == nr)
      return i;

  return NULL;
}

/* search a room for a char, and return a pointer if found..  */
struct char_data *get_char_room(char *name, int room) {
  struct char_data *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  safe_snprintf(tmp, MAX_INPUT_LENGTH, "%s", name);
  if (!(number = get_number(&tmp)))
    return NULL;

  for (i = world[room].people; i != NULL && (j <= number); i = i->next_in_room)
    if (isname(tmp, i->player.name))
      if (++j == number)
        return i;

  return NULL;
}

/* search all over the world for a char num, and return a pointer if found */
struct char_data *get_char_num(int nr) {
  struct char_data *i;

  for (i = character_list; i != NULL; i = i->next)
    if (GET_MOB_RNUM(i) == nr)
      return i;

  return NULL;
}

/* put an object in a room */
void obj_to_room(struct obj_data *object, int room) {
  struct obj_data *obj;
  int plat;
  int gold;
  int silver;
  int copper;

  if (!object || room < 0 || room > top_of_world)
    stderr_log("SYSERR: Illegal value(s) passed to obj_to_room");
  else {
    if (GET_OBJ_TYPE(object) == ITEM_MONEY) {
      obj = world[room].contents;
      while (obj) {
        if (GET_OBJ_TYPE(obj) == ITEM_MONEY) {
          plat = GET_OBJ_VAL(obj, 0) + GET_OBJ_VAL(object, 0);
          gold = GET_OBJ_VAL(obj, 1) + GET_OBJ_VAL(object, 1);
          silver = GET_OBJ_VAL(obj, 2) + GET_OBJ_VAL(object, 2);
          copper = GET_OBJ_VAL(obj, 3) + GET_OBJ_VAL(object, 3);

          extract_obj(object);
          obj_from_room(obj);
          extract_obj(obj);
          object = create_money(plat, gold, silver, copper);
          break;
        } else {
          obj = obj->next_content;
        }
      }
    }
    object->next_content = world[room].contents;
    world[room].contents = object;
    object->in_room = room;
    object->in_obj = NULL;
    object->carried_by = NULL;
    if (!IS_OBJ_STAT(object, ITEM_FLOAT) && !(GET_OBJ_TYPE(object) == ITEM_PCORPSE)) {
      switch (GET_SECT(room)) {
      case SECT_WATER_SWIM:
      case SECT_WATER_NOSWIM:
        act("$p slips silently into the water, vanishing without a trace.", TRUE, NULL, object, NULL, TO_ROOM);
        SET_BIT(GET_OBJ_EXTRA(object), ITEM_UNDERWATER);
        break;
      }
    }
    if (IS_OBJ_STAT(object, ITEM_ISLIGHT))
      world[object->in_room].light++;
    if (ROOM_FLAGGED(room, ROOM_HOUSE))
      SET_BIT(ROOM_FLAGS(room), ROOM_HOUSE_CRASH);
    if (GET_OBJ_RNUM(object) > -1)
      if (obj_index[GET_OBJ_RNUM(object)].qic) {
        safe_snprintf(logbuffer, sizeof(logbuffer), "%s to room %s", object->short_description, world[room].name);
        mudlog(logbuffer, 'J', COM_ADMIN, FALSE);
      }
  }
}

/* Take an object from a room */
void obj_from_room(struct obj_data *object) {
  struct obj_data *temp;

  if (!object || object->in_room == NOWHERE) {
    stderr_log("SYSERR: NULL object or obj not in a room passed to obj_from_room");
    return;
  }
  REMOVE_FROM_LIST(object, world[object->in_room].contents, next_content);

  if (ROOM_FLAGGED(object->in_room, ROOM_HOUSE))
    SET_BIT(ROOM_FLAGS(object->in_room), ROOM_HOUSE_CRASH);

  if (GET_OBJ_RNUM(object) > -1)
    if (obj_index[GET_OBJ_RNUM(object)].qic) {
      safe_snprintf(logbuffer, sizeof(logbuffer), "%s from room %s", object->short_description,
                    world[object->in_room].name);
      mudlog(logbuffer, 'J', COM_ADMIN, FALSE);
    }

  if (IS_OBJ_STAT(object, ITEM_ISLIGHT))
    world[object->in_room].light--;
  object->in_room = NOWHERE;
  object->next_content = NULL;
}

/* put an object in an object (quaint)  */
void obj_to_obj(struct obj_data *obj, struct obj_data *obj_to) {
  struct obj_data *tmp_obj;

  if (GET_OBJ_RNUM(obj) > -1)
    if (obj_index[GET_OBJ_RNUM(obj)].qic) {
      safe_snprintf(logbuffer, sizeof(logbuffer), "%s to obj %s", obj->short_description, obj_to->short_description);
      mudlog(logbuffer, 'J', COM_ADMIN, FALSE);
    }

  obj->next_content = obj_to->contains;
  obj_to->contains = obj;
  obj->in_obj = obj_to;

  for (tmp_obj = obj->in_obj; tmp_obj->in_obj != NULL; tmp_obj = tmp_obj->in_obj)
    GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj);

  /* top level object.  Subtract weight from inventory if necessary. */ GET_OBJ_WEIGHT(tmp_obj) += GET_OBJ_WEIGHT(obj);
  if (tmp_obj->carried_by != NULL)
    IS_CARRYING_W(tmp_obj->carried_by) += GET_OBJ_WEIGHT(obj);
}

/* put an object in an object WEIGHTLESS !  */
void obj_to_obj2(struct obj_data *obj, struct obj_data *obj_to) {
  if (GET_OBJ_RNUM(obj) > -1)
    if (obj_index[GET_OBJ_RNUM(obj)].qic) {
      safe_snprintf(logbuffer, sizeof(logbuffer), "%s to obj %s", obj->short_description, obj_to->short_description);
      mudlog(logbuffer, 'J', COM_ADMIN, FALSE);
    }

  obj->next_content = obj_to->contains;
  obj_to->contains = obj;
  obj->in_obj = obj_to;
}

/* remove an object from an object */
void obj_from_obj(struct obj_data *obj) {
  struct obj_data *temp, *obj_from;

  if (obj->in_obj == NULL) {
    stderr_log("error (handler.c): trying to illegally extract obj from obj");
    return;
  }
  obj_from = obj->in_obj;
  REMOVE_FROM_LIST(obj, obj_from->contains, next_content);

  if (GET_OBJ_RNUM(obj) > -1)
    if (obj_index[GET_OBJ_RNUM(obj)].qic) {
      safe_snprintf(logbuffer, sizeof(logbuffer), "%s from obj %s", obj->short_description,
                    obj_from->short_description);
      mudlog(logbuffer, 'J', COM_ADMIN, FALSE);
    }

  /* Subtract weight from containers container */
  for (temp = obj->in_obj; temp->in_obj != NULL; temp = temp->in_obj)
    GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(obj);

  /* Subtract weight from char that carries the object */ GET_OBJ_WEIGHT(temp) -= GET_OBJ_WEIGHT(obj);
  if (temp->carried_by != NULL)
    IS_CARRYING_W(temp->carried_by) -= GET_OBJ_WEIGHT(obj);

  obj->in_obj = NULL;
  obj->next_content = NULL;
  obj->carried_by = NULL;
  obj->in_room = NOWHERE;
  obj->worn_by = NULL;
}

/* Set all carried_by to point to new owner */
void object_list_new_owner(struct obj_data *list, struct char_data *ch) {
  if (list) {
    if (list->contains) {
      object_list_new_owner(list->contains, ch);
    }
    if (list->next_content) {
      object_list_new_owner(list->next_content, ch);
    }
    list->carried_by = ch;
  }
}

/* Extract an object from the world */
void extract_obj(struct obj_data *obj) {
  struct obj_data *temp = NULL;
  struct obj_data *tmpobj = NULL;
  struct obj_data *next_obj = NULL;
  struct corpse_obj_save *extract_container_stack = NULL;
  struct corpse_obj_save *tmp_stack = NULL;
  struct corpse_obj_save *tmp_stack_next = NULL;

  if (obj->worn_by != NULL)
    if (unequip_char(obj->worn_by, obj->worn_on) != obj)
      stderr_log("SYSERR: Inconsistent worn_by and worn_on pointers!!");
  if (obj->in_room != NOWHERE)
    obj_from_room(obj);
  else if (obj->carried_by != NULL)
    obj_from_char(obj);
  /* OLD CODE
   else if (obj->in_obj != NULL)
   obj_from_obj(obj);
   */

  /* Get rid of the contents of the object, as well. */
  /* OLD CODE
   while (obj->contains != NULL)
   extract_obj(obj->contains);
   */

  CREATE(extract_container_stack, struct corpse_obj_save, 1);
  extract_container_stack->level = 0;
  extract_container_stack->prev = NULL;

  /* Get rid of the contents if any. */
  for (tmpobj = obj->contains; tmpobj; tmpobj = next_obj) {
    next_obj = tmpobj->next_content;
    if (next_obj == tmpobj) {
      /* infinite loop */
      next_obj = NULL;
      continue;
    }
    if (tmpobj->contains) {
      extract_container_stack->next_obj = next_obj;
      CREATE(tmp_stack, struct corpse_obj_save, 1);
      tmp_stack->level = extract_container_stack->level + 1;
      tmp_stack->prev = extract_container_stack;
      extract_container_stack = tmp_stack;
      next_obj = tmpobj->contains;
      if (next_obj == tmpobj) {
        /* infinite loop */
        next_obj = NULL;
        continue;
      }
    } else if (next_obj == NULL && extract_container_stack->level > 0) {
      /* pop off of stack and set next_obj */
      tmp_stack = extract_container_stack;
      extract_container_stack = extract_container_stack->prev;
      FREE(tmp_stack);
      next_obj = extract_container_stack->next_obj;
    }
    REMOVE_FROM_LIST(tmpobj, object_list, next);
    if (GET_OBJ_RNUM(tmpobj) > -1) {
      (obj_index[GET_OBJ_RNUM(tmpobj)].number)--;
      if (obj_index[GET_OBJ_RNUM(tmpobj)].qic) {
        safe_snprintf(logbuffer, sizeof(logbuffer), "%s purged", tmpobj->short_description);
        mudlog(logbuffer, 'J', COM_ADMIN, FALSE);
      }
    }
    if (GET_OBJ_TYPE(tmpobj) == ITEM_PCORPSE)
      corpsesaveall();
    if (tmpobj) {
      free_obj(tmpobj);
      tmpobj = NULL;
    }
  }
  if (obj->cname) {
    FREE(obj->cname);
    obj->cname = NULL;
  }
  if (obj->cdescription) {
    FREE(obj->cdescription);
    obj->cdescription = NULL;
  }
  if (obj->cshort_description) {
    FREE(obj->cshort_description);
    obj->cshort_description = NULL;
  }
  /* Get rid of the object. */
  REMOVE_FROM_LIST(obj, object_list, next);
  if (GET_OBJ_RNUM(obj) > -1) {
    (obj_index[GET_OBJ_RNUM(obj)].number)--;
    if (obj_index[GET_OBJ_RNUM(obj)].qic) {
      safe_snprintf(logbuffer, sizeof(logbuffer), "%s purged", obj->short_description);
      mudlog(logbuffer, 'J', COM_ADMIN, FALSE);
    }
  }
  if (GET_OBJ_TYPE(obj) == ITEM_PCORPSE)
    corpsesaveall();
  if (obj) {
    free_obj(obj);
    obj = NULL;
  }

  for (tmp_stack = extract_container_stack; tmp_stack; tmp_stack = tmp_stack_next) {
    tmp_stack_next = tmp_stack->prev;
    FREE(tmp_stack);
  }
}

/* Extract an object from the world without QIC check/log */
void extract_obj_q(struct obj_data *obj) {
  struct obj_data *temp;
  struct obj_data *tmpobj;
  struct obj_data *next_obj;
  struct corpse_obj_save *extract_container_stack;
  struct corpse_obj_save *tmp_stack;
  struct corpse_obj_save *tmp_stack_next;

  if (obj->worn_by != NULL)
    if (unequip_char(obj->worn_by, obj->worn_on) != obj)
      stderr_log("SYSERR: Inconsistent worn_by and worn_on pointers!!");
  if (obj->in_room != NOWHERE)
    obj_from_room(obj);
  else if (obj->carried_by != NULL)
    obj_from_char(obj);

  /* OLDCODE
   else if (obj->in_obj != NULL)
   obj_from_obj(obj);
   */

  /* Get rid of the contents of the object, as well. */
  /* OLDCODE
   while (obj->contains != NULL)
   extract_obj_q(obj->contains);
   */
  CREATE(extract_container_stack, struct corpse_obj_save, 1);
  extract_container_stack->level = 0;
  extract_container_stack->prev = NULL;

  /* Get rid of the contents if any */
  for (tmpobj = obj->contains; tmpobj; tmpobj = next_obj) {
    next_obj = tmpobj->next_content;
    if (next_obj == tmpobj) {
      /* infinite loop */
      next_obj = NULL;
      continue;
    }
    if (tmpobj->contains) {
      extract_container_stack->next_obj = next_obj;
      CREATE(tmp_stack, struct corpse_obj_save, 1);
      tmp_stack->level = extract_container_stack->level + 1;
      tmp_stack->prev = extract_container_stack;
      extract_container_stack = tmp_stack;
      next_obj = tmpobj->contains;
      if (next_obj == tmpobj) {
        /* infinite loop */
        next_obj = NULL;
        continue;
      }
    } else if (next_obj == NULL && extract_container_stack->level > 0) {
      /* pop off of stack and set next_obj */
      tmp_stack = extract_container_stack;
      extract_container_stack = extract_container_stack->prev;
      FREE(tmp_stack);
      next_obj = extract_container_stack->next_obj;
    }
    REMOVE_FROM_LIST(tmpobj, object_list, next);
    if (GET_OBJ_RNUM(tmpobj) > -1) {
      (obj_index[GET_OBJ_RNUM(tmpobj)].number)--;
    }
    if (GET_OBJ_TYPE(tmpobj) == ITEM_PCORPSE)
      corpsesaveall();
    if (tmpobj) {
      free_obj(tmpobj);
      tmpobj = NULL;
    }
  }
  /* Get rid of the object */
  REMOVE_FROM_LIST(obj, object_list, next);
  if (GET_OBJ_RNUM(obj) > -1) {
    (obj_index[GET_OBJ_RNUM(obj)].number)--;
  }
  if (GET_OBJ_TYPE(obj) == ITEM_PCORPSE)
    corpsesaveall();
  if (obj) {
    free_obj(obj);
    obj = NULL;
  }
  for (tmp_stack = extract_container_stack; tmp_stack; tmp_stack = tmp_stack_next) {
    tmp_stack_next = tmp_stack->prev;
    FREE(tmp_stack);
  }
}

void update_object(struct obj_data *obj, int use) {
  if (GET_OBJ_TIMER(obj) > 0)
    GET_OBJ_TIMER(obj) -= use;
  if (obj->contains)
    update_object(obj->contains, use);
  if (obj->next_content)
    update_object(obj->next_content, use);
}

void update_char_objects(struct char_data *ch) {
  int i;

  if (ch->equipment[WEAR_HOLD] != NULL)
    if (GET_OBJ_TYPE(ch->equipment[WEAR_HOLD]) == ITEM_LIGHT)
      if (GET_OBJ_VAL(ch->equipment[WEAR_HOLD], 2) > 0) {
        i = --GET_OBJ_VAL(ch->equipment[WEAR_HOLD], 2);
        if (i == 1) {
          act("Your light begins to flicker and fade.", FALSE, ch, 0, 0, TO_CHAR);
          act("$n's light begins to flicker and fade.", FALSE, ch, 0, 0, TO_ROOM);
          if (IS_OBJ_STAT(ch->equipment[WEAR_HOLD], ITEM_ISLIGHT))
            REMOVE_BIT(GET_OBJ_EXTRA(ch->equipment[WEAR_HOLD]), ITEM_ISLIGHT);
          SET_BIT(GET_OBJ_EXTRA(ch->equipment[WEAR_HOLD]), ITEM_ISLIGHTDIM);
        } else if (i == 0) {
          act("Your light sputters out and dies.", FALSE, ch, 0, 0, TO_CHAR);
          act("$n's light sputters out and dies.", FALSE, ch, 0, 0, TO_ROOM);
          if (IS_OBJ_STAT(ch->equipment[WEAR_HOLD], ITEM_ISLIGHTDIM))
            REMOVE_BIT(GET_OBJ_EXTRA(ch->equipment[WEAR_HOLD]), ITEM_ISLIGHTDIM);
          world[ch->in_room].light--;
        }
      }

  for (i = 0; i < NUM_WEARS; i++)
    if (ch->equipment[i])
      update_object(ch->equipment[i], 2);

  if (ch->carrying != NULL)
    update_object(ch->carrying, 1);
}

/* Extract a ch completely from the world, and leave his stuff behind */
void extract_char(struct char_data *ch, int dropeq) {
  struct char_data *k, *temp;
  struct descriptor_data *t_desc;
  struct obj_data *obj;
  struct consent_data *consent;
  struct consent_data *consent_next;
  int i;

  extern struct char_data *combat_list;

  ACMD(do_return);

  void die_follower(struct char_data * ch);

  /*
   Call to extract_mobprog in mobprog.c.  Removes any delayed mob_progs from
   the delayed_mprog list.  Save vs Crash -15.
   */
  mob_delay_purge(ch);

  if (!IS_NPC(ch) && !ch->desc) {
    for (t_desc = descriptor_list; t_desc; t_desc = t_desc->next)
      if (t_desc->original == ch)
        do_return(t_desc->character, "", 0, 0);
  }
  if (ch->in_room == NOWHERE) {
    stderr_log("SYSERR: NOWHERE extracting char. (handler.c, extract_char)");
    fflush(NULL);
    exit(1);
  }
  if (ch->followers != NULL || ch->master != NULL)
    die_follower(ch);

  /* Forget snooping, if applicable */
  if (ch->desc) {
    if (ch->desc->snooping) {
      ch->desc->snooping->snoop_by = NULL;
      ch->desc->snooping = NULL;
    }
    if (ch->desc->snoop_by) {
      SEND_TO_Q("Your victim is no longer among us.\r\n", ch->desc->snoop_by);
      ch->desc->snoop_by->snooping = NULL;
      ch->desc->snoop_by = NULL;
    }
  }

  /* transfer objects to room, if any */
  while (dropeq && ch->carrying != NULL) {
    obj = ch->carrying;
    obj_from_char(obj);
    obj_to_room(obj, ch->in_room);
  }

  /* transfer equipment to room, if any */
  for (i = 0; dropeq && i < NUM_WEARS; i++)
    if (ch->equipment[i])
      obj_to_room(unequip_char(ch, i), ch->in_room);

  if (FIGHTING(ch))
    stop_fighting(ch);

  for (k = combat_list; k; k = temp) {
    temp = k->next_fighting;
    if (FIGHTING(k) == ch)
      stop_fighting(k);
  }

  char_from_room(ch);

  clean_events(ch, NULL);

  /* pull the char from the list */

  REMOVE_FROM_LIST(ch, character_list, next);

  if (ch->desc && ch->desc->original)
    do_return(ch, "", 0, 0);

  if (!IS_NPC(ch)) {
    remove_consent(ch);
    if (ch->consent) {
      for (consent = ch->consent; consent; consent = consent_next) {
        consent_next = consent->next;
        FREE(consent);
      }
      ch->consent = NULL;
    }
    if (GET_TELL(ch)) {
      FREE(GET_TELL(ch));
      GET_TELL(ch) = NULL;
    }
    if (!PRF_FLAGGED(ch, PRF_DELETED)) {
      save_char_text(ch, NOWHERE);
      Crash_delete_crashfile(ch);
      if (ch->desc) {
        STATE(ch->desc) = CON_MENU;
        SEND_TO_Q_COLOR(MENU, ch->desc);
        SEND_TO_Q_COLOR("{WMake your choice{G:{x ", ch->desc);
      }
    }
  } else {
    if (GET_MOB_RNUM(ch) > -1) /* if mobile */
      mob_index[GET_MOB_RNUM(ch)].number--;
    clearMemory(ch); /* Only NPC's can have memory */
    free_char(ch);
  }
}

/* ***********************************************************************
 Here follows high-level versions of some earlier routines, ie functions
 which incorporate the actual player-data.
 *********************************************************************** */

struct char_data *get_player_vis(struct char_data *ch, char *name, int inroom) {
  struct char_data *i;

  for (i = character_list; i; i = i->next)
    if (!IS_NPC(i) && (!inroom || i->in_room == ch->in_room) && !str_cmp(i->player.name, name) && CAN_SEE(ch, i))
      return i;

  return NULL;
}

struct char_data *get_char_room_vis(struct char_data *ch, char *name) {
  struct char_data *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  /* JE 7/18/94 :-) :-) */
  if (ch && ((!str_cmp(name, "self") || !str_cmp(name, "me") || !str_cmp(name, GET_NAME(ch))))) {
    return ch;
  } else if (!ch) {
    return NULL;
  }

  /* 0.<name> means PC with name */
  safe_snprintf(tmp, MAX_INPUT_LENGTH, "%s", name);
  if (!(number = get_number(&tmp)))
    return get_player_vis(ch, tmp, 1);

  if (ch->in_room <= NOWHERE)
    return NULL;

  for (i = world[ch->in_room].people; i && j <= number; i = i->next_in_room)
    if (i->player.name)
      if (isname(tmp, i->player.name))
        if (CAN_SEE(ch, i))
          if (++j == number)
            return i;

  return NULL;
}

struct char_data *get_char_vis(struct char_data *ch, char *name) {
  struct char_data *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  /* check the room first */
  if ((i = get_char_room_vis(ch, name)) != NULL)
    return i;

  safe_snprintf(tmp, MAX_INPUT_LENGTH, "%s", name);
  if (!(number = get_number(&tmp)))
    return get_player_vis(ch, tmp, 0);

  for (i = character_list; i && (j <= number); i = i->next)
    if (isname(tmp, i->player.name) && CAN_SEE(ch, i))
      if (++j == number)
        return i;

  return NULL;
}

struct obj_data *get_obj_in_list_vis(struct char_data *ch, char *name, struct obj_data *list) {
  struct obj_data *i;
  struct obj_data *next;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  if (list == NULL || name == NULL || ch == NULL)
    return NULL;

  safe_snprintf(tmp, MAX_INPUT_LENGTH, "%s", name);
  if (!(number = get_number(&tmp)))
    return NULL;

  for (i = list; i && (j <= number); i = next) {
    next = i->next_content;
    if (i && ((i->name && isname(tmp, i->name)) || (i->cname && isname(tmp, i->cname))))
      if (CAN_SEE_OBJ(ch, i))
        if (++j == number)
          return i;
  }

  return NULL;
}

/* search the entire world for an object, and return a pointer  */
struct obj_data *get_obj_vis(struct char_data *ch, char *name) {
  struct obj_data *i;
  int j = 0, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp = tmpname;

  if (!name) {
    stderr_log("NULL name passed to get_obj_vis");
    return NULL;
  }
  /* scan items carried */
  if ((i = get_obj_in_list_vis(ch, name, ch->carrying)))
    return i;

  /* scan room */
  if ((i = get_obj_in_list_vis(ch, name, world[ch->in_room].contents)))
    return i;

  safe_snprintf(tmp, MAX_INPUT_LENGTH, "%s", name);
  if (!(number = get_number(&tmp)))
    return NULL;

  /* ok.. no luck yet. scan the entire obj list   */
  for (i = object_list; i && (j <= number); i = i->next)
    if (isname(tmp, i->name) || (i->cname && isname(tmp, i->cname)))
      if (CAN_SEE_OBJ(ch, i))
        if (++j == number)
          return i;

  return NULL;
}

struct obj_data *get_object_in_equip_vis(struct char_data *ch, char *arg, struct obj_data *equipment[], int *j) {
  for ((*j) = 0; (*j) < NUM_WEARS; (*j)++)
    if (equipment[(*j)])
      if (CAN_SEE_OBJ(ch, equipment[(*j)]))
        if (isname(arg, equipment[(*j)]->name) || (equipment[(*j)]->cname && isname(arg, equipment[(*j)]->cname)))
          return (equipment[(*j)]);

  return NULL;
}

char *money_desc(int amount) {
  if (amount <= 0) {
    stderr_log("SYSERR: Try to create negative or 0 money.");
    return NULL;
  }
  if (amount == 1)
    return "a coin";
  else if (amount <= 10)
    return "a tiny pile of coins";
  else if (amount <= 20)
    return "a handful of coins";
  else if (amount <= 75)
    return "a little pile of coins";
  else if (amount <= 200)
    return "a small pile of coins";
  else if (amount <= 1000)
    return "a pile of coins";
  else if (amount <= 5000)
    return "a big pile of coins";
  else if (amount <= 10000)
    return "a large heap of coins";
  else if (amount <= 20000)
    return "a huge mound of coins";
  else if (amount <= 75000)
    return "an enormous mound of coins";
  else if (amount <= 150000)
    return "a small mountain of coins";
  else if (amount <= 250000)
    return "a mountain of coins";
  else if (amount <= 500000)
    return "a huge mountain of coins";
  else if (amount <= 1000000)
    return "an enormous mountain of coins";
  else
    return "an absolutely colossal mountain of coins";
}

struct obj_data *create_money(int plat, int gold, int silver, int copper) {
  struct obj_data *obj;
  struct extra_descr_data *new_descr;
  char buf[200];
  int amount = (plat * 1000) + (gold * 100) + (silver * 10) + copper;

  if (amount <= 0) {
    stderr_log("SYSERR: Try to create negative or 0 money.");
    return NULL;
  }
  obj = create_obj();
  CREATE(new_descr, struct extra_descr_data, 1);

  if (amount == 1) {
    obj->name = strdup("coin");
    obj->short_description = strdup("a coin");
    obj->description = strdup("One miserable coin is lying here.");
    new_descr->keyword = strdup("coin");
    new_descr->description = strdup("It's just one miserable little coin.");
  } else {
    obj->name = strdup("coins");
    obj->short_description = strdup(money_desc(amount));
    safe_snprintf(buf, sizeof(buf), "%s is lying here.", money_desc(amount));
    obj->description = strdup(CAP(buf));

    new_descr->keyword = strdup("coins");
    safe_snprintf(buf, sizeof(buf), "You guess there are, maybe, %dp %dg %ds %dc.",
                  1000 * ((plat / 1000) + number(0, (plat / 1000))), 1000 * ((gold / 1000) + number(0, (gold / 1000))),
                  1000 * ((silver / 1000) + number(0, (silver / 1000))),
                  1000 * ((copper / 1000) + number(0, (copper / 1000))));
    new_descr->description = strdup(buf);
  }

  new_descr->next = NULL;
  obj->ex_description = new_descr;

  GET_OBJ_TYPE(obj) = ITEM_MONEY;
  GET_OBJ_WEAR(obj) = ITEM_WEAR_TAKE;
  GET_OBJ_VAL(obj, 0) = plat;
  GET_OBJ_VAL(obj, 1) = gold;
  GET_OBJ_VAL(obj, 2) = silver;
  GET_OBJ_VAL(obj, 3) = copper;
  GET_OBJ_COST(obj) = amount;
  obj->item_number = NOTHING;

  return obj;
}

/* Generic Find, designed to find any object/character                    */
/* Calling :                                                              */
/*  *arg     is the sting containing the string to be searched for.       */
/*           This string doesn't have to be a single word, the routine    */
/*           extracts the next word itself.                               */
/*  bitv..   All those bits that you want to "search through".            */
/*           Bit found will be result of the function                     */
/*  *ch      This is the person that is trying to "find"                  */
/*  **tar_ch Will be NULL if no character was found, otherwise points     */
/* **tar_obj Will be NULL if no object was found, otherwise points        */
/*                                                                        */
/* The routine returns a pointer to the next word in *arg (just like the  */
/* one_argument routine).                                                 */

int generic_find(char *arg, int bitvector, struct char_data *ch, struct char_data **tar_ch, struct obj_data **tar_obj) {
  int i, found;
  char name[256];

  one_argument(arg, name);

  if (!*name)
    return (0);

  *tar_ch = NULL;
  *tar_obj = NULL;

  if (IS_SET(bitvector, FIND_CHAR_ROOM)) { /* Find person in room */
    if ((*tar_ch = get_char_room_vis(ch, name))) {
      return (FIND_CHAR_ROOM);
    }
  }
  if (IS_SET(bitvector, FIND_CHAR_WORLD)) {
    if ((*tar_ch = get_char_vis(ch, name))) {
      return (FIND_CHAR_WORLD);
    }
  }
  if (IS_SET(bitvector, FIND_OBJ_ROOM)) {
    if ((*tar_obj = get_obj_in_list_vis(ch, name, world[ch->in_room].contents))) {
      return (FIND_OBJ_ROOM);
    }
  }
  if (IS_SET(bitvector, FIND_OBJ_INV)) {
    if ((*tar_obj = get_obj_in_list_vis(ch, name, ch->carrying))) {
      return (FIND_OBJ_INV);
    }
  }
  if (IS_SET(bitvector, FIND_OBJ_EQUIP)) {
    for (found = FALSE, i = 0; i < NUM_WEARS && !found; i++)
      if (ch->equipment[i] && CAN_SEE_OBJ(ch, ch->equipment[i]) && isname(name, ch->equipment[i]->name)) {
        *tar_obj = ch->equipment[i];
        found = TRUE;
      }
    if (found) {
      return (FIND_OBJ_EQUIP);
    }
  }
  if (IS_SET(bitvector, FIND_OBJ_WORLD)) {
    if ((*tar_obj = get_obj_vis(ch, name))) {
      return (FIND_OBJ_WORLD);
    }
  }
  return (0);
}

/* a function to scan for "all" or "all.x" */
int find_all_dots(char *arg) {
  if (!strcmp(arg, "all"))
    return FIND_ALL;
  else if (!strncmp(arg, "all.", 4)) {
    memmove(arg, arg + 4, strlen(arg + 4) + 1);
    return FIND_ALLDOT;
  } else
    return FIND_INDIV;
}
