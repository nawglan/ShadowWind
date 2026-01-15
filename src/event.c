#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "structs.h"
#include "event.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"

int in_event_handler = 0;
struct event_info *pending_events = NULL;
struct event_info *prev = NULL;
extern struct room_data *world;
extern sh_int stats[11][101];
extern struct spell_info_type *spells;
extern int hometowns[][NUM_CLASSES];
void stop_fighting(struct char_data *ch);
void make_corpse(struct char_data *ch);
int find_spell_num(char *name);
void say_spell(struct char_data *ch, int spellnum, struct char_data *tch, struct obj_data *tobj);
void Crash_save(struct char_data *ch, int type);
int mag_savingthrow(struct char_data *ch, int type);
struct obj_data *unequip_char(struct char_data * ch, int pos);
void mag_affect_obj(struct char_data *caster, struct obj_data *object, struct spell_info_type *sinfo, int value);
void mag_points_char(struct spell_info_type *sinfo, struct char_data *caster, struct char_data *vict, int value);
int check_teleport(struct char_data *ch, int room);
extern struct obj_data *object_list;
void add_follower(struct char_data *ch, struct char_data *leader);
char *get_spell_name(int spellnum);
struct char_data *get_char_room_vis(struct char_data *ch, char *name);
void weight_change_object(struct obj_data *obj, int amount);
void set_fighting(struct char_data *ch, struct char_data *vict);
void improve_skill(struct char_data *ch, int skill, int chance);
void clear_magic_memory(struct char_data *ch);
char debugbuf[1024];
int check_consent(struct char_data *ch, char *vict_name);
int Crash_report_unrentables(struct char_data *ch, struct char_data *recep, struct obj_data *obj);

extern int pk_allowed;
extern int top_of_world;

int getSpellDam(struct spell_info_type *sinfo, struct char_data *ch)
{
  int dam;
  if (!sinfo->num_dice2) { /* only 1 set of dice */
    if (!sinfo->size_limit && !sinfo->dice_limit) {
      dam = dice(sinfo->num_dice, sinfo->size_dice) + sinfo->dice_add;
    } else if (sinfo->size_limit && !sinfo->dice_limit) {
      dam = dice(sinfo->num_dice, MIN(sinfo->size_dice, GET_LEVEL(ch))) + sinfo->dice_add;
    } else if (!sinfo->size_limit && sinfo->dice_limit) {
      dam = dice(MIN(sinfo->num_dice, GET_LEVEL(ch)), sinfo->size_dice) + sinfo->dice_add;
    } else {
      dam = dice(MIN(sinfo->num_dice, GET_LEVEL(ch)), MIN(sinfo->size_dice, GET_LEVEL(ch))) + sinfo->dice_add;
    }
  } else { /* using 2 dice */
    if (!sinfo->size_limit && !sinfo->dice_limit && !sinfo->size_limit2 && !sinfo->dice_limit2) {
      dam = dice(sinfo->num_dice, sinfo->size_dice) + dice(sinfo->num_dice2, sinfo->size_dice2) + sinfo->dice_add + sinfo->dice_add2;
    } else if (!sinfo->size_limit && !sinfo->dice_limit && !sinfo->size_limit2 && sinfo->dice_limit2) {
      dam = dice(sinfo->num_dice, sinfo->size_dice) + dice(MIN(sinfo->num_dice2, GET_LEVEL(ch)), sinfo->size_dice2) + sinfo->dice_add + sinfo->dice_add2;
    } else if (!sinfo->size_limit && !sinfo->dice_limit && sinfo->size_limit2 && !sinfo->dice_limit2) {
      dam = dice(sinfo->num_dice, sinfo->size_dice) + dice(sinfo->num_dice2, MIN(sinfo->size_dice2, GET_LEVEL(ch))) + sinfo->dice_add + sinfo->dice_add2;
    } else if (!sinfo->size_limit && !sinfo->dice_limit && sinfo->size_limit2 && sinfo->dice_limit2) {
      dam = dice(sinfo->num_dice, sinfo->size_dice) + dice(MIN(sinfo->num_dice2, GET_LEVEL(ch)), MIN(sinfo->size_dice2, GET_LEVEL(ch))) + sinfo->dice_add + sinfo->dice_add2;
    } else if (!sinfo->size_limit && sinfo->dice_limit && !sinfo->size_limit2 && !sinfo->dice_limit2) {
      dam = dice(MIN(sinfo->num_dice, GET_LEVEL(ch)), sinfo->size_dice) + dice(sinfo->num_dice2, sinfo->size_dice2) + sinfo->dice_add + sinfo->dice_add2;
    } else if (!sinfo->size_limit && sinfo->dice_limit && !sinfo->size_limit2 && sinfo->dice_limit2) {
      dam = dice(MIN(sinfo->num_dice, GET_LEVEL(ch)), sinfo->size_dice) + dice(MIN(sinfo->num_dice2, GET_LEVEL(ch)), sinfo->size_dice2) + sinfo->dice_add + sinfo->dice_add2;
    } else if (!sinfo->size_limit && sinfo->dice_limit && sinfo->size_limit2 && !sinfo->dice_limit2) {
      dam = dice(MIN(sinfo->num_dice, GET_LEVEL(ch)), sinfo->size_dice) + dice(sinfo->num_dice2, MIN(sinfo->size_dice2, GET_LEVEL(ch))) + sinfo->dice_add + sinfo->dice_add2;
    } else if (!sinfo->size_limit && sinfo->dice_limit && sinfo->size_limit2 && sinfo->dice_limit2) {
      dam = dice(MIN(sinfo->num_dice, GET_LEVEL(ch)), sinfo->size_dice) + dice(MIN(sinfo->num_dice2, GET_LEVEL(ch)), MIN(sinfo->size_dice2, GET_LEVEL(ch))) + sinfo->dice_add + sinfo->dice_add2;
    } else if (sinfo->size_limit && !sinfo->dice_limit && !sinfo->size_limit2 && !sinfo->dice_limit2) {
      dam = dice(sinfo->num_dice, MIN(sinfo->size_dice, GET_LEVEL(ch))) + dice(sinfo->num_dice2, sinfo->size_dice2) + sinfo->dice_add + sinfo->dice_add2;
    } else if (sinfo->size_limit && !sinfo->dice_limit && !sinfo->size_limit2 && sinfo->dice_limit2) {
      dam = dice(sinfo->num_dice, MIN(sinfo->size_dice, GET_LEVEL(ch))) + dice(MIN(sinfo->num_dice2, GET_LEVEL(ch)), sinfo->size_dice2) + sinfo->dice_add + sinfo->dice_add2;
    } else if (sinfo->size_limit && !sinfo->dice_limit && sinfo->size_limit2 && !sinfo->dice_limit2) {
      dam = dice(sinfo->num_dice, MIN(sinfo->size_dice, GET_LEVEL(ch))) + dice(sinfo->num_dice2, MIN(sinfo->size_dice2, GET_LEVEL(ch))) + sinfo->dice_add + sinfo->dice_add2;
    } else if (sinfo->size_limit && !sinfo->dice_limit && sinfo->size_limit2 && sinfo->dice_limit2) {
      dam = dice(sinfo->num_dice, MIN(sinfo->size_dice, GET_LEVEL(ch))) + dice(MIN(sinfo->num_dice2, GET_LEVEL(ch)), MIN(sinfo->size_dice2, GET_LEVEL(ch))) + sinfo->dice_add + sinfo->dice_add2;
    } else if (sinfo->size_limit && sinfo->dice_limit && !sinfo->size_limit2 && !sinfo->dice_limit2) {
      dam = dice(MIN(sinfo->num_dice, GET_LEVEL(ch)), MIN(sinfo->size_dice, GET_LEVEL(ch))) + dice(sinfo->num_dice2, sinfo->size_dice2) + sinfo->dice_add + sinfo->dice_add2;
    } else if (sinfo->size_limit && sinfo->dice_limit && !sinfo->size_limit2 && sinfo->dice_limit2) {
      dam = dice(MIN(sinfo->num_dice, GET_LEVEL(ch)), MIN(sinfo->size_dice, GET_LEVEL(ch))) + dice(MIN(sinfo->num_dice2, GET_LEVEL(ch)), sinfo->size_dice2) + sinfo->dice_add + sinfo->dice_add2;
    } else if (sinfo->size_limit && sinfo->dice_limit && sinfo->size_limit2 && !sinfo->dice_limit2) {
      dam = dice(MIN(sinfo->num_dice, GET_LEVEL(ch)), MIN(sinfo->size_dice, GET_LEVEL(ch))) + dice(sinfo->num_dice2, MIN(sinfo->size_dice2, GET_LEVEL(ch))) + sinfo->dice_add + sinfo->dice_add2;
    } else {
      dam = dice(MIN(sinfo->num_dice, GET_LEVEL(ch)), MIN(sinfo->size_dice, GET_LEVEL(ch))) + dice(MIN(sinfo->num_dice2, GET_LEVEL(ch)), MIN(sinfo->size_dice2, GET_LEVEL(ch))) + sinfo->dice_add + sinfo->dice_add2;
    }
  }
  return dam;
}

int modBySpecialization(struct char_data *ch, struct spell_info_type *sinfo, int dam)
{
  int newdam = dam;

  if (GET_SPECIALIZED(ch)) {
    if (GET_SKILL(ch, sinfo->realm) >= number(1, 101)) {
      newdam = dam * 2;
    } else {
      if (!number(0, 4)) {
        newdam = dam * 3 / 2;
      }
    }
  }

  return newdam;
}

int getMaxCircle(struct spell_info_type *sinfo)
{
  int i;
  int best = 1;

  for (i = 0; i < NUM_CLASSES; i++) {
    if (sinfo->min_level[i] > best) {
      if (sinfo->min_level[i] != 51) {
        best = sinfo->min_level[i];
      }
    }
  }

  return (best + 4) / 5;
}

int is_area_spell(struct spell_info_type *sinfo)
{
  return (sinfo->spell_pointer == spell_general || sinfo->event_pointer == spell_area_event || sinfo->event_pointer == spell_area_points_event || sinfo->event_pointer == spell_area_dam_event || sinfo->event_pointer == spell_room_event || sinfo->event_pointer == spell_group_event || sinfo->event_pointer == spell_create_obj_event || sinfo->event_pointer == spell_create_mob_event || sinfo->event_pointer == spell_word_recall_event || sinfo->event_pointer == spell_group_points_event);
}

int spell_has_vict(struct spell_info_type *sinfo)
{
  return (sinfo->spell_pointer != spell_general && sinfo->event_pointer != spell_create_obj_event && sinfo->event_pointer != spell_create_mob_event && sinfo->event_pointer != spell_area_event && sinfo->event_pointer != spell_area_points_event && sinfo->event_pointer != spell_area_dam_event && sinfo->event_pointer != spell_room_event && sinfo->event_pointer != spell_word_recall_event && sinfo->event_pointer != spell_group_event && sinfo->event_pointer != spell_group_points_event && sinfo->event_pointer != spell_locate_obj_event);
}
#define EVENT_CH ((struct char_data*)temp->causer)
void run_events()
{
  struct event_info *temp, *prox;
  int i;
  char abuf[256];
  int bitvector = FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM;
  struct char_data *tempch;
  struct obj_data *tempobj;
  int circle;
  int bonus;
  int skillnum = spells[find_skill_num("quick chant")].spellindex;

  in_event_handler = 1;

  prev = NULL;
  for (temp = pending_events; temp; temp = prox) {
    prox = temp->next;
    temp->ticks_to_go--;
    if (temp->type == EVENT_SPELL && temp->sinfo && !temp->info2 && !is_area_spell(temp->sinfo)) {
      if (!temp->info) {
        if (temp->causer && EVENT_CH && FIGHTING(EVENT_CH)) {
          if (FIGHTING(EVENT_CH)->in_room != (EVENT_CH)->in_room) {
            send_to_char("Your loose focus, your target doesn't appear to be here.\r\n", EVENT_CH);
            temp->ticks_to_go = -1;
            REMOVE_BIT(AFF2_FLAGS(EVENT_CH), AFF2_CASTING);
            WAIT_STATE(EVENT_CH, 0);
          }
        }
      } else if (!generic_find(temp->info, bitvector, temp->causer, &tempch, &tempobj)) {
        send_to_char("You loose focus, your target doesn't appear to be here.\r\n", EVENT_CH);
        temp->ticks_to_go = -1;
        REMOVE_BIT(AFF2_FLAGS(EVENT_CH), AFF2_CASTING);
        WAIT_STATE(EVENT_CH, 0);
      }
    } else if (!(!IS_MOB(EVENT_CH) && COM_FLAGGED(EVENT_CH, COM_IMMORT)) && temp->sinfo && temp->type == EVENT_SPELL && !temp->info2 && !(temp->ticker--)) {
      circle = GET_CIRCLE_DIFF(EVENT_CH, temp->sinfo);
      if (IS_MAGE(EVENT_CH))
        bonus = GET_INT(EVENT_CH);
      else
        bonus = GET_WIS(EVENT_CH);
      if (circle < 2 && (GET_SKILL(EVENT_CH, temp->sinfo->realm) + bonus) < number(1, 195)) {
        act("$n lost concentration!", TRUE, EVENT_CH, 0, 0, TO_ROOM);
        act("You lost your concentration!", TRUE, EVENT_CH, 0, 0, TO_CHAR);
        temp->ticks_to_go = -1;
        REMOVE_BIT(AFF2_FLAGS(EVENT_CH), AFF2_CASTING);
        WAIT_STATE(EVENT_CH, 0);
      }
    } else if (IS_MOB(EVENT_CH) && temp->type == EVENT_SPELL && temp->sinfo && !temp->info2 && !(temp->ticker--)) {
      circle = GET_CIRCLE_DIFF(EVENT_CH, temp->sinfo);
      if (circle < 0) {
        circle = 0;
      }
      if (MOB_FLAGGED(EVENT_CH, MOB_HAS_MAGE)) {
        bonus = GET_INT(EVENT_CH);
      } else {
        bonus = GET_WIS(EVENT_CH);
      }
      if (circle < 2 && (number(1, 61) + bonus) < number(1, 156)) {
        act("$n fails miserably!", TRUE, EVENT_CH, 0, 0, TO_ROOM);
        act("You fail miserably!", TRUE, EVENT_CH, 0, 0, TO_CHAR);
        temp->ticks_to_go = -1;
        REMOVE_BIT(AFF2_FLAGS(EVENT_CH), AFF2_CASTING);
        WAIT_STATE(EVENT_CH, 0);
      }
    }
    if (temp->command && (temp->ticks_to_go >> 1) > 0 && number(1, 5) > 2) {
      size_t alen = safe_snprintf(abuf, sizeof(abuf), "casting %s: ", temp->command);
      if (GET_SKILL(EVENT_CH, skillnum) >= number(0, 100)) {
        temp->ticks_to_go >>= 1;
        WAIT_STATE(EVENT_CH, temp->ticks_to_go * 10);
        improve_skill(EVENT_CH, skillnum, SKUSE_FREQUENT);
      }
      for (i = 0; i < temp->ticks_to_go && alen < sizeof(abuf) - 3; i++) {
        abuf[alen++] = '*';
      }
      abuf[alen++] = '\r';
      abuf[alen++] = '\n';
      abuf[alen] = '\0';
      send_to_char(abuf, EVENT_CH);
      if (temp->causer && GET_POS(EVENT_CH) < POS_FIGHTING) {
        act("$n stops casting!", TRUE, EVENT_CH, 0, 0, TO_ROOM);
        act("You cannot cast unless you are standing!", TRUE, EVENT_CH, 0, 0, TO_CHAR);
        temp->ticks_to_go = -1;
        REMOVE_BIT(AFF2_FLAGS(EVENT_CH), AFF2_CASTING);
        WAIT_STATE(EVENT_CH, 0);
      }
    }
    if (temp->ticks_to_go == 0) {

      /* run the event */
      if (!temp->func) {
        stderr_log("SYSERR: Attempting to run a NULL event. Ignoring");
      } else {
        (temp->func)(temp->causer, temp->victim, (long) temp->info, temp->sinfo, temp->info2);
      }

      /* remove the event from the list. */
      if (!prev) {
        pending_events = prox;
      } else {
        prev->next = prox;
      }
      if (temp->command) {
        FREE(temp->command);
      }
      if (temp->type == EVENT_SPELL && temp->info) {
        FREE(temp->info);
      }
      FREE(temp);
    } else if (temp->ticks_to_go < 0) {
      if (temp->type == EVENT_SPELL && EVENT_CH) {
        REMOVE_BIT(AFF2_FLAGS(EVENT_CH), AFF2_CASTING);
      }
      if (!prev) {
        pending_events = prox;
      } else {
        prev->next = prox;
      }
      if (temp->command) {
        FREE(temp->command);
      }
      if (temp->type == EVENT_SPELL && temp->info) {
        FREE(temp->info);
      }
      FREE(temp);
    } else {
      prev = temp;
    }
  }

  in_event_handler = 0;
}

void add_event(int delay, EVENT(*func), int type, void *causer, void *victim, void *info, struct spell_info_type *sinfo, char *command, int info2)
{
  struct event_info *new;

  CREATE(new, struct event_info, 1);

  new->ticks_to_go = delay;
  new->func = func;
  new->type = type;
  new->causer = causer;
  new->victim = victim;
  new->info = info;
  new->info2 = info2;
  new->sinfo = sinfo;
  new->ticker = number(1, delay);
  if (command)
    new->command = strdup(command);
  else
    new->command = NULL;

  new->next = pending_events;
  pending_events = new;
  if (in_event_handler && !prev)
    prev = pending_events;
}

bool clean_events(void *pointer, EVENT(*func))
{
  struct event_info *temp, *prox;
  bool completed = 0;

  if (!func) {
    for (temp = pending_events; temp; temp = prox) {
      prox = temp->next;
      if (temp->causer == pointer || temp->victim == pointer || (void *) (temp->func) == pointer) {
        completed = 1;
        temp->ticks_to_go = -1;
      }
    }
  } else {
    for (temp = pending_events; temp; temp = prox) {
      prox = temp->next;
      if ((temp->causer == pointer || temp->victim == pointer) && ((event *) (temp->func) == func)) {
        completed = 1;
        temp->ticks_to_go = -1;
      }
    }
  }
  return completed;
}

bool clean_causer_events(void *pointer, int type)
{
  struct event_info *temp, *prox;
  bool completed = 0;

  for (temp = pending_events; temp; temp = prox) {
    prox = temp->next;
    if ((temp->causer == pointer) && temp->type == type) {
      completed = 1;
      temp->ticks_to_go = -1;
    }
  }

  return completed;
}

bool check_events(void *pointer, EVENT(*func))
{
  struct event_info *temp, *prox;

  if (!func) {
    for (temp = pending_events; temp; temp = prox) {
      prox = temp->next;
      if ((temp->causer == pointer || temp->victim == pointer) && temp->ticks_to_go)
        return 1;
    }
  } else {
    for (temp = pending_events; temp; temp = prox) {
      prox = temp->next;
      if ((temp->causer == pointer || temp->victim == pointer) && ((event *) (temp->func) == func) && temp->ticks_to_go)
        return 1;
    }
  }
  return 0;
}

struct event_info *find_event(struct char_data *ch, int type)
{
  struct event_info *temp, *prox;

  for (temp = pending_events; temp; temp = prox) {
    prox = temp->next;
    if (temp->causer == (void*) ch && temp->type == type) {
      return temp;
    }
  }

  return NULL;
}

/* ************************************ *
 *  Pre-defined events.(Just examples)  *
 * ************************************ */

EVENT(spell_teleport_event)
{
  int destin = 0;
  int tries = 0;

  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);

  if (!IS_MOB(CAUSER_CH) && COM_FLAGGED(CAUSER_CH, COM_IMMORT)) {
    send_to_char("Use goto.\r\n", CAUSER_CH);
    return;
  }

  if (!info2)
    send_to_char("You complete your spell.\r\n", CAUSER_CH);
  say_spell(CAUSER_CH, sinfo->spellindex, NULL, NULL);
  if (sinfo->send_to_room)
    act(sinfo->send_to_room, TRUE, CAUSER_CH, NULL, CAUSER_CH, TO_ROOM);
  if (sinfo->send_to_char)
    act(sinfo->send_to_char, TRUE, CAUSER_CH, NULL, CAUSER_CH, TO_CHAR);

  do {
    destin = number(1, top_of_world);
    tries++;
    if (tries == 100) {
      break;
    }
  } while (!IS_NPC(CAUSER_CH) && (IN_ZONE(CAUSER_CH) != world[destin].zone || ROOM_FLAGGED(destin, ROOM_PRIVATE | ROOM_GODROOM) || !check_teleport(CAUSER_CH, destin)));

  if (tries < 100) {
    act("$n slowly fades from existence.", TRUE, CAUSER_CH, 0, 0, TO_ROOM);
    char_from_room(CAUSER_CH);
    char_to_room(CAUSER_CH, destin);
    act("$n slowly fades into existence.", TRUE, CAUSER_CH, 0, 0, TO_ROOM);
    look_at_room(CAUSER_CH, 0);
  } else {
    send_to_char("You failed to teleport away.\r\n", CAUSER_CH);
  }
}

EVENT(knockedout)
{
  send_to_char("Your head stops hurting, you are now sleeping normally.\r\n", CAUSER_CH);
  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_KNOCKEDOUT);
}

EVENT(camp)
{
  int i;
  int unrentables = 0;

  if ((int) (CAUSER_CH->in_room) == (int) info) {
    unrentables = Crash_report_unrentables(CAUSER_CH, NULL, CAUSER_CH->carrying);
    for (i = 0; i < NUM_WEARS; i++) {
      unrentables = Crash_report_unrentables(CAUSER_CH, NULL, GET_EQ(CAUSER_CH, i));
    }
    if (unrentables) {
      return;
    }
    safe_snprintf(buf, MAX_STRING_LENGTH, "%s has camped in room #%d.", GET_NAME(CAUSER_CH), world[IN_ROOM(CAUSER_CH)].number);
    mudlog(buf, 'C', COM_IMMORT, TRUE);
    plog(buf, CAUSER_CH, 0);
    safe_snprintf(buf, MAX_STRING_LENGTH, "Goodbye, %s... Come back soon!\r\n", CAUSER_CH->player.name);
    send_to_char(buf, CAUSER_CH);
    SET_BIT(PLR_FLAGS(CAUSER_CH), PLR_CAMP);
    SET_BIT(PLR_FLAGS(CAUSER_CH), PLR_LOADROOM);
    GET_LOADROOM(CAUSER_CH) = world[CAUSER_CH->in_room].number;
    Crash_save(CAUSER_CH, RENT_CAMPED);
    act("$n has camped for the night.", TRUE, CAUSER_CH, 0, 0, TO_ROOM);
    extract_char(CAUSER_CH, 0);
    clear_magic_memory(CAUSER_CH);
    for (i = 0; i < NUM_WEARS; i++)
      if (GET_EQ(CAUSER_CH, i))
        obj_to_char(unequip_char(CAUSER_CH, i), CAUSER_CH);
  } else {
    send_to_char("So much for that camping effort.\r\n", CAUSER_CH);
    REMOVE_BIT(AFF_FLAGS(CAUSER_CH), AFF_CAMPING);
  }
}

ACMD(do_camp)
{
  if (GET_LEVEL(ch) < LVL_IMMORT) {
    if (GET_SECT(ch->in_room) == SECT_CITY)
      send_to_char("This is not a good place to camp for the night. Try an inn.\r\n", ch);
    else if (ROOM_FLAGGED(ch->in_room, ROOM_INDOORS))
      send_to_char("You cannot camp indoors. Try an inn.\r\n", ch);
    else if (GET_SECT(ch->in_room) == SECT_WATER_SWIM || GET_SECT(ch->in_room) == SECT_WATER_NOSWIM || GET_SECT(ch->in_room) == SECT_UNDERWATER || GET_SECT(ch->in_room) == SECT_FLYING)
      send_to_char("You must camp on solid ground.\r\n", ch);
    else if (ROOM_FLAGGED(ch->in_room, ROOM_NOCAMP))
      send_to_char("Camping is not allowed here.\r\n", ch);
    else {
      if (!check_events(ch, camp)) {
        SET_BIT(AFF_FLAGS(ch), AFF_CAMPING);
        add_event(120, camp, EVENT_CAMP, ch, NULL, (void *) (long) ch->in_room, NULL, NULL, 0);
        send_to_char("You start setting up your campsite.\r\n", ch);
      } else {
        send_to_char("You're camp preperations are not yet complete.\r\n", ch);
      }
    }
  } else
    send_to_char("You dumb ass just type quit....\r\n", ch);
  return;
}

EVENT(fail_spell_event)
{
  act("$n fails miserably!", TRUE, CAUSER_CH, 0, 0, TO_ROOM);
  act("You fail miserably!", TRUE, CAUSER_CH, 0, 0, TO_CHAR);
  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);
}

EVENT(spell_magic_missile_event)
{
  int amount = BOUNDED(1, GET_SKILL(CAUSER_CH, sinfo->realm) / 10, 5);
  int i;
  int origdam;
  int dam = (dice(sinfo->num_dice, sinfo->size_dice) + GET_LEVEL(CAUSER_CH)) >> 1;
  struct char_data *vict;
  char abuf[256];

  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);

  if (!IS_MOB(CAUSER_CH)) {
    dam = dam * GET_SKILL(CAUSER_CH, sinfo->realm) / 110;
  }
  if (dam < 1) {
    dam = 1;
  }
  origdam = dam;
  if ((char*) info) {
    vict = get_char_room_vis(CAUSER_CH, (char*) info);
    if (!vict) {
      safe_snprintf(abuf, sizeof(abuf), "You don't see %s here.\r\n", (char*) info);
      send_to_char(abuf, CAUSER_CH);
      return;
    }
    if (!pk_allowed) {
      if (!IS_MOB(CAUSER_CH) && !IS_MOB(vict) && vict != CAUSER_CH) {
        send_to_char("It's not a good idea to cast that on players.\r\n", CAUSER_CH);
        return;
      }
      if (IS_AFFECTED(CAUSER_CH, AFF_CHARM) && !IS_MOB(CAUSER_CH->master)) {
        send_to_char("You can't order a follower to cast that on a player.\r\n", CAUSER_CH->master);
        return;
      }
    }
  } else if (FIGHTING(CAUSER_CH))
    vict = FIGHTING(CAUSER_CH);
  else {
    if (info2)
      return;
    send_to_char("Casting this on youself would hurt!\r\n", CAUSER_CH);
    act("$n stops casting!", TRUE, CAUSER_CH, 0, 0, TO_ROOM);
    return;
  }
  if (CAUSER_CH != vict && sinfo->aggressive) {
    if (!IS_IMMO(CAUSER_CH) && mag_savingthrow(vict, sinfo->saving_throw)) {
      dam >>= 1;
    }
  }
  if (vict) {
    if (!info2) {
      send_to_char("You complete your spell.\r\n", CAUSER_CH);
      say_spell(CAUSER_CH, sinfo->spellindex, vict, NULL);
    }
    for (i = 0; i < amount; i++) {
      if (vict && GET_POS(vict) > POS_DEAD) {
        if (IS_MOB(CAUSER_CH)) {
          safe_snprintf(debugbuf, sizeof(debugbuf), "MOB CAST: <%s '%s' %s> dam = %d", GET_MOB_NAME(CAUSER_CH), sinfo->command, GET_NAME(vict), dam);
          mudlog(debugbuf, 'D', COM_IMMORT, TRUE);
        }
        if (CAUSER_CH != vict && sinfo->aggressive) {
          if (!IS_IMMO(CAUSER_CH) && mag_savingthrow(vict, sinfo->saving_throw)) {
            dam >>= 1;
          }
          if (getMaxCircle(sinfo) < 5 && AFF2_FLAGGED(vict, AFF2_MINOR_GLOBE)) {
            act("Your minor globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_VICT);
            act("Your spell was deflected by $N's minor globe of invunerability.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
            act("$N's minor globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
            return;
          } else if (getMaxCircle(sinfo) >= 5 && getMaxCircle(sinfo) < 9 && AFF_FLAGGED(vict, AFF_MAJOR_GLOBE)) {
            act("Your major globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_VICT);
            act("Your spell was deflected by $N's major globe of invunerability.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
            act("$N's major globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
            return;
          }
        }
        damage(CAUSER_CH, vict, dam, sinfo->spellindex, 0, sinfo->resist_type, TRUE, info2 ? 1 : 0);
        dam = origdam;
      }
    }
  } else {
    if (!info2) {
      act("$n stops casting!", TRUE, CAUSER_CH, 0, 0, TO_ROOM);
    }
  }
  FIGHT_STATE(CAUSER_CH, 2);
}

EVENT(spell_word_recall_event)
{

  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);

  if (!CAUSER_CH || IS_MOB(CAUSER_CH)) {
    send_to_char("Nothing seems to happen.\r\n", CAUSER_CH);
    return;
  }

  if (!info2) {
    send_to_char("You complete your spell.\r\n", CAUSER_CH);
    say_spell(CAUSER_CH, sinfo->spellindex, NULL, NULL);
  }
  act("$n disappears.", TRUE, CAUSER_CH, 0, 0, TO_ROOM);
  if (FIGHTING(CAUSER_CH)) {
    stop_fighting(CAUSER_CH);
  }
  if (GET_DRAGGING(CAUSER_CH)) {
    GET_DRAGGING(CAUSER_CH)->dragged_by = NULL;
    GET_DRAGGING(CAUSER_CH) = NULL;
  }
  char_from_room(CAUSER_CH);
  char_to_room(CAUSER_CH, real_room(GET_STARTROOM(CAUSER_CH, GET_HOME(CAUSER_CH))));
  act("$n appears in a cloud of smoke.", TRUE, CAUSER_CH, 0, 0, TO_ROOM);
  GET_MOVE(CAUSER_CH) = BOUNDED(0, CAN_CARRY_W(CAUSER_CH) - IS_CARRYING_W(CAUSER_CH), GET_MAX_MOVE(CAUSER_CH));
  look_at_room(CAUSER_CH, 0);
}

EVENT(spell_dam_event)
{
  int dam;
  struct char_data *vict;
  char abuf[256];

  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);

  dam = getSpellDam(sinfo, CAUSER_CH);

  if (!IS_MOB(CAUSER_CH)) {
    dam = modBySpecialization(CAUSER_CH, sinfo, dam);
    dam = dam * GET_SKILL(CAUSER_CH, sinfo->realm) / 100;
  }
  if ((char*) info) {
    vict = get_char_room_vis(CAUSER_CH, (char*) info);
    if (!vict) {
      safe_snprintf(abuf, sizeof(abuf), "You don't see %s here.\r\n", (char*) info);
      send_to_char(abuf, CAUSER_CH);
      return;
    }
    if (!pk_allowed) {
      if (!IS_MOB(CAUSER_CH) && !IS_MOB(vict) && vict != CAUSER_CH) {
        send_to_char("It's not a good idea to cast that on players.\r\n", CAUSER_CH);
        return;
      }
      if (IS_AFFECTED(CAUSER_CH, AFF_CHARM) && !IS_MOB(CAUSER_CH->master)) {
        send_to_char("You can't order a follower to cast that on a player.\r\n", CAUSER_CH->master);
        return;
      }
    }
  } else if (FIGHTING(CAUSER_CH)) {
    vict = FIGHTING(CAUSER_CH);
  } else {
    if (info2) {
      return;
    }
    send_to_char("Casting this on youself would hurt!\r\n", CAUSER_CH);
    act("$n stops casting!", TRUE, CAUSER_CH, 0, 0, TO_ROOM);
    return;
  }
  if (CAUSER_CH != vict && sinfo->aggressive) {
    if (!IS_IMMO(CAUSER_CH) && mag_savingthrow(vict, sinfo->saving_throw)) {
      dam >>= 1;
    }
    if (getMaxCircle(sinfo) < 5 && AFF2_FLAGGED(vict, AFF2_MINOR_GLOBE)) {
      act("Your minor globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_VICT);
      act("Your spell was deflected by $N's minor globe of invunerability.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
      act("$N's minor globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
      return;
    } else if (getMaxCircle(sinfo) >= 5 && getMaxCircle(sinfo) < 9 && AFF_FLAGGED(vict, AFF_MAJOR_GLOBE)) {
      act("Your major globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_VICT);
      act("Your spell was deflected by $N's major globe of invunerability.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
      act("$N's major globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
      return;
    }
  }
  if (vict) {
    if (!info2) {
      send_to_char("You complete your spell.\r\n", CAUSER_CH);
      say_spell(CAUSER_CH, sinfo->spellindex, vict, NULL);
    }
    if (IS_MOB(CAUSER_CH)) {
      safe_snprintf(debugbuf, sizeof(debugbuf), "MOB CAST: <%s '%s' %s> dam = %d", GET_MOB_NAME(CAUSER_CH), sinfo->command, GET_NAME(vict), dam);
      mudlog(debugbuf, 'D', COM_IMMORT, TRUE);
    }
    damage(CAUSER_CH, vict, dam, sinfo->spellindex, 0, sinfo->resist_type, TRUE, info2 ? 1 : 0);
    FIGHT_STATE(CAUSER_CH, 2);
  } else {
    if (!info2) {
      act("$n stops casting!", TRUE, CAUSER_CH, 0, 0, TO_ROOM);
    }
  }
}

EVENT(spell_char_event)
{
  struct char_data *vict;
  char abuf[256];

  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);

  if ((char*) info) {
    vict = get_char_room_vis(CAUSER_CH, (char*) info);
    if (!vict) {
      safe_snprintf(abuf, sizeof(abuf), "You don't see %s here.\r\n", (char*) info);
      send_to_char(abuf, CAUSER_CH);
      return;
    }
    if (!pk_allowed && sinfo->aggressive) {
      if (getMaxCircle(sinfo) < 5 && AFF2_FLAGGED(vict, AFF2_MINOR_GLOBE)) {
        act("Your minor globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_VICT);
        act("Your spell was deflected by $N's minor globe of invunerability.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
        act("$N's minor globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
        return;
      } else if (getMaxCircle(sinfo) >= 5 && getMaxCircle(sinfo) < 9 && AFF_FLAGGED(vict, AFF_MAJOR_GLOBE)) {
        act("Your major globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_VICT);
        act("Your spell was deflected by $N's major globe of invunerability.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
        act("$N's major globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
        return;
      }
      if (!IS_MOB(CAUSER_CH) && !IS_MOB(vict) && vict != CAUSER_CH) {
        send_to_char("It's not a good idea to cast that on players.\r\n", CAUSER_CH);
        return;
      }
      if (IS_AFFECTED(CAUSER_CH, AFF_CHARM) && !IS_MOB(CAUSER_CH->master)) {
        send_to_char("You can't order a follower to cast that on a player.\r\n", CAUSER_CH->master);
        return;
      }
    }
  } else {
    vict = CAUSER_CH;
  }

  if (CAUSER_CH != vict && sinfo->aggressive) {
    if (!IS_IMMO(CAUSER_CH) && mag_savingthrow(vict, sinfo->saving_throw)) {
      act("$N resists your spell.", TRUE, CAUSER_CH, NULL, vict, TO_CHAR);
      act("$N resists $n's spell.", TRUE, CAUSER_CH, NULL, vict, TO_ROOM);
      act("You resist $n's spell.", TRUE, CAUSER_CH, NULL, vict, TO_VICT);
      return;
    }
    if (getMaxCircle(sinfo) < 5 && AFF2_FLAGGED(vict, AFF2_MINOR_GLOBE)) {
      act("Your minor globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_VICT);
      act("Your spell was deflected by $N's minor globe of invunerability.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
      act("$N's minor globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
      return;
    } else if (getMaxCircle(sinfo) >= 5 && getMaxCircle(sinfo) < 9 && AFF_FLAGGED(vict, AFF_MAJOR_GLOBE)) {
      act("Your major globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_VICT);
      act("Your spell was deflected by $N's major globe of invunerability.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
      act("$N's major globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
      return;
    }
  }

  if (vict) {
    if (!info2) {
      send_to_char("You complete your spell.\r\n", CAUSER_CH);
      say_spell(CAUSER_CH, sinfo->spellindex, vict, NULL);
    }
    if (sinfo->unaffect) {
      affect_from_char(vict, spells[find_spell_num(sinfo->unaffect)].spellindex);
    } else {
      if (strcmp(sinfo->command, "fire shield") == 0) {
        if (AFF2_FLAGGED(vict, AFF2_ICESHIELD)) {
          send_to_char("You cannot have both fire shield and ice shield active.\r\n", vict);
        } else {
          mag_affect_char(sinfo, 0, CAUSER_CH, vict, GET_LEVEL(CAUSER_CH));
        }
      } else if (strcmp(sinfo->command, "ice shield") == 0) {
        if (AFF2_FLAGGED(vict, AFF2_FIRESHIELD)) {
          send_to_char("You cannot have both fire shield and ice shield active.\r\n", vict);
        } else {
          mag_affect_char(sinfo, 0, CAUSER_CH, vict, GET_LEVEL(CAUSER_CH));
        }
      } else {
        mag_affect_char(sinfo, 0, CAUSER_CH, vict, GET_LEVEL(CAUSER_CH));
      }
    }

    if (strcmp(sinfo->command, "wraith form") == 0) {
      make_corpse(vict);
    }

    if (sinfo->send_to_vict) {
      act(sinfo->send_to_vict, TRUE, CAUSER_CH, 0, vict, TO_VICT);
    }
    if (sinfo->send_to_room) {
      act(sinfo->send_to_room, TRUE, vict, 0, CAUSER_CH, TO_ROOM);
    }
    if (sinfo->send_to_char) {
      act(sinfo->send_to_char, TRUE, CAUSER_CH, 0, vict, TO_CHAR);
    }
  } else {
    if (!info2) {
      act("$n stops casting!", TRUE, CAUSER_CH, 0, 0, TO_ROOM);
    }
  }
  if (IS_NPC(vict) && vict->master != CAUSER_CH && sinfo->aggressive && number(0, 1)) {
    hit(vict, CAUSER_CH, TYPE_UNDEFINED);
    FIGHT_STATE(CAUSER_CH, 2);
  }
}

EVENT(spell_points_event)
{
  struct char_data *vict;
  char abuf[256];

  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);

  if ((char*) info) {
    vict = get_char_room_vis(CAUSER_CH, (char*) info);
    if (!vict) {
      safe_snprintf(abuf, sizeof(abuf), "You don't see %s here.\r\n", (char*) info);
      send_to_char(abuf, CAUSER_CH);
      return;
    }
    if (!pk_allowed && sinfo->aggressive) {
      if (!IS_MOB(CAUSER_CH) && !IS_MOB(vict) && vict != CAUSER_CH) {
        send_to_char("It's not a good idea to cast that on players.\r\n", CAUSER_CH);
        return;
      }
      if (IS_AFFECTED(CAUSER_CH, AFF_CHARM) && !IS_MOB(CAUSER_CH->master)) {
        send_to_char("You can't order a follower to cast that on a player.\r\n", CAUSER_CH->master);
        return;
      }
    }
  } else {
    vict = CAUSER_CH;
  }

  if (CAUSER_CH != vict && sinfo->aggressive) {
    if (getMaxCircle(sinfo) < 5 && AFF2_FLAGGED(vict, AFF2_MINOR_GLOBE)) {
      act("Your minor globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_VICT);
      act("Your spell was deflected by $N's minor globe of invunerability.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
      act("$N's minor globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
      return;
    } else if (getMaxCircle(sinfo) >= 5 && getMaxCircle(sinfo) < 9 && AFF_FLAGGED(vict, AFF_MAJOR_GLOBE)) {
      act("Your major globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_VICT);
      act("Your spell was deflected by $N's major globe of invunerability.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
      act("$N's major globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
      return;
    }
    if (!IS_IMMO(CAUSER_CH) && mag_savingthrow(vict, sinfo->saving_throw)) {
      act("$N resists your spell.", TRUE, CAUSER_CH, NULL, vict, TO_CHAR);
      act("$N resists $n's spell.", TRUE, CAUSER_CH, NULL, vict, TO_ROOM);
      act("You resist $n's spell.", TRUE, CAUSER_CH, NULL, vict, TO_VICT);
      return;
    }
  }

  if (vict) {
    if (!info2) {
      send_to_char("You complete your spell.\r\n", CAUSER_CH);
      say_spell(CAUSER_CH, sinfo->spellindex, vict, NULL);
    }
    mag_points_char(sinfo, CAUSER_CH, vict, GET_LEVEL(CAUSER_CH));
    if (sinfo->send_to_vict) {
      act(sinfo->send_to_vict, TRUE, CAUSER_CH, 0, vict, TO_VICT);
    }
    if (sinfo->send_to_room) {
      act(sinfo->send_to_room, TRUE, vict, 0, CAUSER_CH, TO_ROOM);
    }
    if (sinfo->send_to_char) {
      act(sinfo->send_to_char, TRUE, CAUSER_CH, 0, vict, TO_CHAR);
    }
  } else {
    if (!info2) {
      act("$n stops casting!", TRUE, CAUSER_CH, 0, 0, TO_ROOM);
    }
  }
  if (vict->master != CAUSER_CH && sinfo->aggressive && number(1, 10) < 5) {
    set_fighting(CAUSER_CH, vict);
  }
}

EVENT(spell_obj_event)
{
  char abuf[256];
  struct obj_data *object = NULL;

  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);

  if ((char*) info) {
    object = get_obj_in_list_vis(CAUSER_CH, (char*) info, CAUSER_CH->carrying);
    if (!object) {
      object = get_obj_in_list_vis(CAUSER_CH, (char*) info, world[CAUSER_CH->in_room].contents);
    }
    if (!object) {
      safe_snprintf(abuf, sizeof(abuf), "You don't see %s here.\r\n", (char*) info);
      send_to_char(abuf, CAUSER_CH);
      return;
    }
  } else {
    send_to_char("Cast on what?\r\n", CAUSER_CH);
    return;
  }

  if (object) {
    if (!info2) {
      send_to_char("You complete your spell.\r\n", CAUSER_CH);
      say_spell(CAUSER_CH, sinfo->spellindex, NULL, object);
    }
    if (!IS_OBJ_STAT(object, ITEM_ENCHANTED)) {
      mag_affect_obj(CAUSER_CH, object, sinfo, GET_SKILL(CAUSER_CH, sinfo->realm));
      SET_BIT(GET_OBJ_EXTRA(object), ITEM_ENCHANTED);
      if (sinfo->send_to_char) {
        act(sinfo->send_to_char, TRUE, CAUSER_CH, NULL, object, TO_CHAR);
      }
      if (sinfo->send_to_room) {
        act(sinfo->send_to_room, TRUE, CAUSER_CH, NULL, object, TO_ROOM);
      }
    } else {
      send_to_char("That item already has been enchanted.\r\n", CAUSER_CH);
    }
  } else {
    if (!info2) {
      act("$n stops casting!", TRUE, CAUSER_CH, 0, 0, TO_ROOM);
    }
  }
}

EVENT(spell_obj_room_event)
{
  char abuf[256];
  struct obj_data *object = NULL;
  int is_room = 0;

  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);

  if ((char*) info) {
    object = get_obj_in_list_vis(CAUSER_CH, (char*) info, CAUSER_CH->carrying);
    if (!object) {
      object = get_obj_in_list_vis(CAUSER_CH, (char*) info, world[CAUSER_CH->in_room].contents);
    }
    if (!object) {
      safe_snprintf(abuf, sizeof(abuf), "You don't see %s here.\r\n", (char*) info);
      send_to_char(abuf, CAUSER_CH);
      return;
    }
  } else {
    is_room = 1;
  }

  if (!info2) {
    send_to_char("You complete your spell.\r\n", CAUSER_CH);
    say_spell(CAUSER_CH, sinfo->spellindex, NULL, object);
  }
  if (!is_room) {
    if (!IS_OBJ_STAT(object, ITEM_ENCHANTED)) {
      mag_affect_obj(CAUSER_CH, object, sinfo, GET_SKILL(CAUSER_CH, sinfo->realm));
      SET_BIT(GET_OBJ_EXTRA(object), ITEM_ENCHANTED);
      if (sinfo->send_to_char) {
        act(sinfo->send_to_char, TRUE, CAUSER_CH, NULL, object, TO_CHAR);
      }
      if (sinfo->send_to_room) {
        act(sinfo->send_to_room, TRUE, CAUSER_CH, NULL, object, TO_ROOM);
      }
    } else {
      send_to_char("That item has already been enchanted.\r\n", CAUSER_CH);
    }
  }

  if (is_room && sinfo->spell_room_bit) {
    SET_BIT(ROOM_FLAGS(CAUSER_CH->in_room), sinfo->spell_room_bit);
    if (sinfo->send_to_char) {
      act(sinfo->send_to_char, TRUE, CAUSER_CH, NULL, NULL, TO_CHAR);
    }
    if (sinfo->send_to_room) {
      act(sinfo->send_to_room, TRUE, NULL, NULL, CAUSER_CH, TO_ROOM);
    }
  }
}

EVENT(spell_room_event)
{
  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);

  if (!info2) {
    send_to_char("You complete your spell.\r\n", CAUSER_CH);
    say_spell(CAUSER_CH, sinfo->spellindex, NULL, NULL);
  }

  if (sinfo->spell_room_bit) {
    SET_BIT(ROOM_FLAGS(CAUSER_CH->in_room), sinfo->spell_room_bit);
  }

  if (sinfo->send_to_char) {
    act(sinfo->send_to_char, TRUE, CAUSER_CH, NULL, NULL, TO_CHAR);
  }
  if (sinfo->send_to_room) {
    act(sinfo->send_to_room, TRUE, NULL, NULL, CAUSER_CH, TO_ROOM);
  }
}

EVENT(spell_area_event)
{
  struct char_data *vict;
  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);

  if (!info2) {
    send_to_char("You complete your spell.\r\n", CAUSER_CH);
    say_spell(CAUSER_CH, sinfo->spellindex, NULL, NULL);
  }
  if (sinfo->send_to_room) {
    act(sinfo->send_to_room, TRUE, NULL, NULL, CAUSER_CH, TO_ROOM);
  }
  for (vict = world[CAUSER_CH->in_room].people; vict; vict = vict->next_in_room) {
    if (CAUSER_CH != vict && sinfo->aggressive) {
      if (!IS_IMMO(CAUSER_CH) && mag_savingthrow(vict, sinfo->saving_throw)) {
        continue;
      }
      if (getMaxCircle(sinfo) < 5 && AFF2_FLAGGED(vict, AFF2_MINOR_GLOBE)) {
        act("Your minor globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_VICT);
        act("Your spell was deflected by $N's minor globe of invunerability.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
        act("$N's minor globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
        continue;
      } else if (getMaxCircle(sinfo) >= 5 && getMaxCircle(sinfo) < 9 && AFF_FLAGGED(vict, AFF_MAJOR_GLOBE)) {
        act("Your major globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_VICT);
        act("Your spell was deflected by $N's major globe of invunerability.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
        act("$N's major globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
        continue;
      }
    }
    if (sinfo->unaffect) {
      affect_from_char(vict, spells[find_spell_num(sinfo->unaffect)].spellindex);
    } else {
      mag_affect_char(sinfo, 0, CAUSER_CH, vict, GET_LEVEL(CAUSER_CH));
    }
  }
}

EVENT(spell_dimdoor_event)
{
  struct char_data *vict;

  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);
  if (!info) {
    send_to_char("You gather mana and {Wsparks{w fizzle at your finger tips.{x\r\n", CAUSER_CH);
    return;
  }

  vict = get_char_vis(CAUSER_CH, (char*) info);

  if (CAUSER_CH == NULL || vict == NULL) {
    send_to_char("That person is not present in the realms.\r\n", CAUSER_CH);
    return;
  }

  if (!info2) {
    send_to_char("You complete your spell.\r\n", CAUSER_CH);
    say_spell(CAUSER_CH, sinfo->spellindex, NULL, NULL);
  }

  if (CAUSER_CH == vict) {
    send_to_char("You gather mana and {Wsparks{w fizzle at your finger tips.{x\r\n", CAUSER_CH);
    return;
  }

  if (IS_MOB(vict)) {
    send_to_char("You gather mana and {Wsparks{w fizzle at your finger tips.{x\r\n", CAUSER_CH);
    return;
  }

  if (!IS_MOB(CAUSER_CH) && GET_LEVEL(CAUSER_CH) >= LVL_IMMORT) {
    send_to_char("Use goto, we don't lower ourselves to puny mortal teleport spells.\r\n", CAUSER_CH);
    return;
  }
  if (GET_LEVEL(vict) > MIN(LVL_IMMORT - 1, GET_LEVEL(CAUSER_CH) + 7)) {
    send_to_char("You gather mana and {Wsparks{w fizzle at your finger tips.{x\r\n", CAUSER_CH);
    return;
  }
  if (!check_teleport(CAUSER_CH, vict->in_room)) {
    send_to_char("You gather mana and {Wsparks{w fizzle at your finger tips.{x\r\n", CAUSER_CH);
    return;
  }

  if (sinfo->send_to_room) {
    act(sinfo->send_to_room, TRUE, NULL, NULL, CAUSER_CH, TO_ROOM);
  }
  if (sinfo->send_to_char) {
    act(sinfo->send_to_char, FALSE, CAUSER_CH, NULL, NULL, TO_CHAR);
  }
  if (GET_DRAGGING(CAUSER_CH)) {
    GET_DRAGGING(CAUSER_CH)->dragged_by = NULL;
    GET_DRAGGING(CAUSER_CH) = NULL;
  }
  act("{DA dark shimmering portal opens, as $n steps in it dissipates into thin air.{x", TRUE, CAUSER_CH, NULL, NULL, TO_ROOM);
  char_from_room(CAUSER_CH);
  char_to_room(CAUSER_CH, vict->in_room);
  act("{DA dark shimmering portal opens, as $n steps out it dissipates into thin air.{x", TRUE, CAUSER_CH, NULL, NULL, TO_ROOM);
  look_at_room(CAUSER_CH, 0);
}

EVENT(spell_locate_obj_event)
{
  struct obj_data *i;
  struct obj_data *obj = get_obj_vis(CAUSER_CH, (char*) info);
  char name[MAX_INPUT_LENGTH];
  int j;

  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);

  if (!info2) {
    send_to_char("You complete your spell.\r\n", CAUSER_CH);
    say_spell(CAUSER_CH, sinfo->spellindex, NULL, NULL);
  }
  if (!obj) {
    act("$n stops casting!", TRUE, CAUSER_CH, 0, 0, TO_ROOM);
    send_to_char("You were unable to locate such an object.", CAUSER_CH);
    return;
  }

  safe_snprintf(name, sizeof(name), "%s", fname(obj->name));
  j = GET_LEVEL(CAUSER_CH) >> 1;

  for (i = object_list; i && (j > 0); i = i->next) {
    if (!isname(name, i->name)) {
      continue;
    }

    if (i->carried_by && IS_MOB(i->carried_by)) {
      safe_snprintf(buf, MAX_STRING_LENGTH, "%s is being carried by %s.\n\r", i->short_description, PERS(i->carried_by, CAUSER_CH));
    } else if (i->in_room != NOWHERE) {
      safe_snprintf(buf, MAX_STRING_LENGTH, "%s is in %s.\n\r", i->short_description, world[i->in_room].name);
    } else if (i->in_obj) {
      safe_snprintf(buf, MAX_STRING_LENGTH, "%s is in %s.\n\r", i->short_description, i->in_obj->short_description);
    } else if (i->worn_by && IS_MOB(i->worn_by)) {
      safe_snprintf(buf, MAX_STRING_LENGTH, "%s is being worn by %s.\n\r", i->short_description, PERS(i->worn_by, CAUSER_CH));
    } else {
      safe_snprintf(buf, MAX_STRING_LENGTH, "%s's location is uncertain.\n\r", i->short_description);
    }

    CAP(buf);
    send_to_char(buf, CAUSER_CH);
    j--;
  }

  if (j == GET_LEVEL(CAUSER_CH) >> 1)
    send_to_char("You sense nothing.\n\r", CAUSER_CH);
}

EVENT(spell_create_obj_event)
{
  struct obj_data *object = NULL;
  struct char_data *vict;
  int success = 0;
  char abuf[256];

  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);
  if (!info2) {
    send_to_char("You complete your spell.\r\n", CAUSER_CH);
    say_spell(CAUSER_CH, sinfo->spellindex, NULL, NULL);
  }

  if (strcmp(sinfo->command, "flame blade") == 0) {
    if (GET_EQ(CAUSER_CH, WEAR_WIELD) || GET_EQ(CAUSER_CH, WEAR_HOLD) || GET_EQ(CAUSER_CH, WEAR_SHIELD) || GET_EQ(CAUSER_CH, WEAR_2HANDED) || GET_EQ(CAUSER_CH, WEAR_WIELD_2) || GET_EQ(CAUSER_CH, WEAR_HOLD_2)) {
      send_to_char("Your hands are too full.", CAUSER_CH);
      return;
    }
    object = read_object(sinfo->vnum_list[0], VIRTUAL);
    equip_char(CAUSER_CH, object, WEAR_2HANDED);
    SET_BIT(AFF3_FLAGS(CAUSER_CH), AFF3_WIELDINGFLAMEBLADE);
    CAUSER_CH->player_specials->saved.weapontimer = GET_LEVEL(CAUSER_CH);
    send_to_char(OK, CAUSER_CH);
  } else if (strcmp(sinfo->command, "spiritual hammer") == 0) {
    if (GET_EQ(CAUSER_CH, WEAR_WIELD) || GET_EQ(CAUSER_CH, WEAR_HOLD) || GET_EQ(CAUSER_CH, WEAR_SHIELD) || GET_EQ(CAUSER_CH, WEAR_2HANDED) || GET_EQ(CAUSER_CH, WEAR_WIELD_2) || GET_EQ(CAUSER_CH, WEAR_HOLD_2)) {
      send_to_char("Your hands are too full.", CAUSER_CH);
      return;
    }
    object = read_object(sinfo->vnum_list[0], VIRTUAL);
    equip_char(CAUSER_CH, object, WEAR_2HANDED);
    SET_BIT(AFF3_FLAGS(CAUSER_CH), AFF3_WIELDINGSPIRITUALHAMMER);
    CAUSER_CH->player_specials->saved.weapontimer = GET_LEVEL(CAUSER_CH);
    send_to_char(OK, CAUSER_CH);
  } else if (strcmp(sinfo->command, "create spring") == 0) {
    object = read_object(sinfo->vnum_list[0], VIRTUAL);
    obj_to_room(object, CAUSER_CH->in_room);
    success = 1;
  } else if (strcmp(sinfo->command, "create food") == 0) {
    object = read_object(sinfo->vnum_list[0], VIRTUAL);
    obj_to_room(object, CAUSER_CH->in_room);
    success = 1;
  } else if (strcmp(sinfo->command, "minor creation") == 0) {
    if ((char*) info) {
      if (strncmp((char*) info, "ration", strlen((char*) info)) == 0) {
        object = read_object(sinfo->vnum_list[0], VIRTUAL);
        obj_to_room(object, CAUSER_CH->in_room);
        success = 1;
      } else if (strncmp((char*) info, "barrel", strlen((char*) info)) == 0) {
        object = read_object(sinfo->vnum_list[1], VIRTUAL);
        obj_to_room(object, CAUSER_CH->in_room);
        success = 1;
      } else if (strncmp((char*) info, "torch", strlen((char*) info)) == 0) {
        object = read_object(sinfo->vnum_list[2], VIRTUAL);
        obj_to_room(object, CAUSER_CH->in_room);
        success = 1;
      } else if (strncmp((char*) info, "raft", strlen((char*) info)) == 0) {
        object = read_object(sinfo->vnum_list[3], VIRTUAL);
        obj_to_room(object, CAUSER_CH->in_room);
        success = 1;
      }
    } else {
      send_to_char("You can create: ration, barrel, torch, or raft.\r\n", CAUSER_CH);
    }
  } else if (strcmp(sinfo->command, "moonwell") == 0) {
    if ((char*) info) {
      vict = get_char_vis(CAUSER_CH, (char*) info);
      if (!vict || IS_MOB(vict) || IS_IMMO(vict)) {
        safe_snprintf(abuf, sizeof(abuf), "You can not see %s.\r\n", (char*) info);
        send_to_char(abuf, CAUSER_CH);
        return;
      }
    } else {
      send_to_char("A moonwell? Yes... but to whom?\r\n", CAUSER_CH);
      return;
    }
    if (CAUSER_CH != vict && sinfo->aggressive) {
      if (getMaxCircle(sinfo) < 5 && AFF2_FLAGGED(vict, AFF2_MINOR_GLOBE)) {
        act("Your minor globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_VICT);
        act("Your spell was deflected by $N's minor globe of invunerability.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
        act("$N's minor globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
        return;
      } else if (getMaxCircle(sinfo) >= 5 && getMaxCircle(sinfo) < 9 && AFF_FLAGGED(vict, AFF_MAJOR_GLOBE)) {
        act("Your major globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_VICT);
        act("Your spell was deflected by $N's major globe of invunerability.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
        act("$N's major globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
        return;
      }
    }
    if (!IS_IMMO(CAUSER_CH) && mag_savingthrow(vict, sinfo->saving_throw)) {
      act("$N resists your spell.", TRUE, CAUSER_CH, NULL, vict, TO_CHAR);
      act("$N resists $n's spell.", TRUE, CAUSER_CH, NULL, vict, TO_ROOM);
      act("You resist $n's spell.", TRUE, CAUSER_CH, NULL, vict, TO_VICT);
      return;
    }

    if (vict) {
      object = read_object(sinfo->vnum_list[0], VIRTUAL);
      GET_OBJ_VAL(object, 0) = world[CAUSER_CH->in_room].number;
      GET_OBJ_TIMER(object) = 2;
      SET_BIT(GET_OBJ_EXTRA(object), ITEM_CARRIED);
      obj_to_room(object, vict->in_room);
      object = read_object(sinfo->vnum_list[0], VIRTUAL);
      GET_OBJ_VAL(object, 0) = world[vict->in_room].number;
      GET_OBJ_TIMER(object) = 2;
      SET_BIT(GET_OBJ_EXTRA(object), ITEM_CARRIED);
      obj_to_room(object, CAUSER_CH->in_room);
      if (sinfo->send_to_room)
        act(sinfo->send_to_room, TRUE, vict, 0, CAUSER_CH, TO_ROOM);
      if (sinfo->send_to_vict)
        act(sinfo->send_to_vict, FALSE, CAUSER_CH, 0, vict, TO_VICT);
      if (sinfo->send_to_char)
        act(sinfo->send_to_char, FALSE, CAUSER_CH, 0, vict, TO_CHAR);
    } else {
      if (!info2) {
        act("$n stops casting!", TRUE, CAUSER_CH, 0, 0, TO_ROOM);
      }
    }
  }
  if (success) {
    act("$a $o magically appears.", TRUE, CAUSER_CH, object, 0, TO_CHAR);
  }
}

EVENT(spell_area_points_event)
{
  struct char_data *vict;
  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);

  if (!info2) {
    send_to_char("You complete your spell.\r\n", CAUSER_CH);
    say_spell(CAUSER_CH, sinfo->spellindex, NULL, NULL);
  }
  if (sinfo->send_to_room) {
    act(sinfo->send_to_room, TRUE, NULL, NULL, CAUSER_CH, TO_ROOM);
  }
  for (vict = world[CAUSER_CH->in_room].people; vict; vict = vict->next_in_room) {
    if (CAUSER_CH != vict && sinfo->aggressive) {
      if (!IS_IMMO(CAUSER_CH) && mag_savingthrow(vict, sinfo->saving_throw)) {
        continue;
      }
      if (getMaxCircle(sinfo) < 5 && AFF2_FLAGGED(vict, AFF2_MINOR_GLOBE)) {
        act("Your minor globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_VICT);
        act("Your spell was deflected by $N's minor globe of invunerability.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
        act("$N's minor globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
        continue;
      } else if (getMaxCircle(sinfo) >= 5 && getMaxCircle(sinfo) < 9 && AFF_FLAGGED(vict, AFF_MAJOR_GLOBE)) {
        act("Your major globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_VICT);
        act("Your spell was deflected by $N's major globe of invunerability.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
        act("$N's major globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
        continue;
      }
    }
    mag_points_char(sinfo, CAUSER_CH, vict, GET_LEVEL(CAUSER_CH));
  }
}

EVENT(spell_summon_event)
{
  struct char_data *vict = get_char_vis(CAUSER_CH, (char*) info);

  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);

  if (!info2) {
    send_to_char("You complete your spell.\r\n", CAUSER_CH);
    say_spell(CAUSER_CH, sinfo->spellindex, NULL, NULL);
  }
  if (CAUSER_CH == NULL || vict == NULL) {
    return;
  }
  if (IS_NPC(vict) && !IS_NPC(CAUSER_CH)) {
    send_to_char("You are unable to summon your victim.\r\n", CAUSER_CH);
    return;
  }
  if (IN_ZONE(vict) != IN_ZONE(CAUSER_CH)) {
    send_to_char("You must be in the same zone as your victim.\r\n", CAUSER_CH);
    return;
  }

  if ((GET_LEVEL(vict) > MIN(LVL_IMMORT - 1, GET_LEVEL(CAUSER_CH) + 5)) || MOB_FLAGGED(vict, MOB_NOSUMMON)) {
    send_to_char("You are unable to summon your victim.\r\n", CAUSER_CH);
    return;
  }
  /*failed goes here*/
  if (PRF_FLAGGED(vict, PRF_NOSUMMON) || (!IS_MOB(vict) && !check_consent(CAUSER_CH, GET_NAME(vict)))) {
    send_to_char("Your attempt at summoning failed.\r\n", CAUSER_CH);
    send_to_char("You feel a sense of tugging.\r\n", vict);
    return;
  }

  if (!IS_IMMO(CAUSER_CH) && mag_savingthrow(vict, sinfo->saving_throw)) {
    send_to_char("Your attempt at summoning failed.\r\n", CAUSER_CH);
    send_to_char("You feel a sense of tugging.\r\n", vict);
    return;
  }

  act("$n disappears suddenly.", TRUE, vict, 0, 0, TO_ROOM);

  char_from_room(vict);
  char_to_room(vict, CAUSER_CH->in_room);

  if (sinfo->send_to_room) {
    act(sinfo->send_to_room, TRUE, vict, NULL, NULL, TO_ROOM);
  }
  if (sinfo->send_to_vict) {
    act(sinfo->send_to_vict, FALSE, CAUSER_CH, NULL, vict, TO_VICT);
  }
  if (sinfo->send_to_char) {
    act(sinfo->send_to_char, FALSE, CAUSER_CH, NULL, vict, TO_CHAR);
  }
  look_at_room(vict, 0);
}

EVENT(spell_create_mob_event)
{
  extern int mental_dice[12][2];
  int success = 0;
  struct affected_type *af;
  int mag_circle = GET_CIRCLE_DIFF(CAUSER_CH, sinfo);
  struct char_data *mob = NULL;
  struct follow_type *f, *fnext;
  struct obj_data *corpse;
  struct obj_data *tobj;
  struct obj_data *next_obj;
  char buf[80];
  char *corpsename;
  int i;

  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);
  if (!info2) {
    send_to_char("You complete your spell.\r\n", CAUSER_CH);
    say_spell(CAUSER_CH, sinfo->spellindex, NULL, NULL);
  }
  if (strcmp(sinfo->command, "conjure elemental") == 0) {
    if (COM_FLAGGED(CAUSER_CH, COM_IMMORT) || GET_LEVEL(CAUSER_CH) >= 51) {
      mag_circle = 11;
    }
    for (f = CAUSER_CH->followers; f; f = fnext) {
      fnext = f->next;
      if (strstr(GET_NAME(f->follower), "elemental")) {
        send_to_char("You already have an elemental following you!\r\n", CAUSER_CH);
        return;
      }
    }
    success = 1;
    switch (number(1, 4)) {
      case 1: /* air mental */
        mob = read_mobile(real_mobile(sinfo->vnum_list[0]), REAL | NOEQUIP);
        GET_MANA(mob) = mental_dice[mag_circle][1];
        GET_AC(mob) = -100;
        SET_BIT(MOB_FLAGS(mob), MOB_WRAITHLIKE);
        break;
      case 2: /* water mental */
        mob = read_mobile(real_mobile(sinfo->vnum_list[1]), REAL | NOEQUIP);
        GET_MANA(mob) = 2 * mental_dice[mag_circle][1];
        GET_AC(mob) = number(-50, 50);
        break;
      case 3: /* earth mental */
        mob = read_mobile(real_mobile(sinfo->vnum_list[2]), REAL | NOEQUIP);
        GET_MANA(mob) = (int) (2.5 * mental_dice[mag_circle][1]);
        GET_AC(mob) = number(-50, 50);
        break;
      case 4: /* fire mental */
        mob = read_mobile(real_mobile(sinfo->vnum_list[3]), REAL | NOEQUIP);
        GET_MANA(mob) = (int) (1.5 * mental_dice[mag_circle][1]);
        GET_AC(mob) = -100;
        SET_BIT(MOB_FLAGS(mob), MOB_WRAITHLIKE);
        break;
    }
    GET_HIT(mob) = mental_dice[mag_circle][0];
    GET_MOVE(mob) = 2 * number(1, GET_LEVEL(CAUSER_CH));
    GET_MAX_HIT(mob) = dice(GET_HIT(mob), GET_MANA(mob)) + GET_MOVE(mob);
    GET_HIT(mob) = GET_MAX_HIT(mob);
    GET_MAX_MANA(mob) = GET_MANA(mob);
    GET_MAX_MOVE(mob) = GET_MOVE(mob);
    GET_CLASS(mob) = CLASS_ELEMENTAL;
  } else if (strcmp(sinfo->command, "animate dead") == 0) {
    success = 1;
    corpse = get_obj_in_list_vis(CAUSER_CH, (char*) info, world[CAUSER_CH->in_room].contents);
    if (!corpse || !(GET_OBJ_TYPE(corpse) == ITEM_CONTAINER && GET_OBJ_VAL(corpse, 3))) {
      send_to_char("Which corpse do you wish to animate?\r\n", CAUSER_CH);
      return;
    }
    if (GET_OBJ_VAL(corpse, 3) != 1) {
      send_to_char("Corpses of the undead cannot be animated.\r\n", CAUSER_CH);
      return;
    }
    corpsename = strchr(corpse->cshort_description, ' ');
    corpsename++;
    for (i = 0; i < 2; i++) {
      corpsename = strchr(corpsename, ' ');
      corpsename++;
    }
    switch (number(1, 20)) {
      case 1: /* skeleton */
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
        mob = read_mobile(real_mobile(sinfo->vnum_list[0]), REAL | NOEQUIP);
        IS_ANIMATED(mob) = 1;
        safe_snprintf(buf, MAX_STRING_LENGTH, "The skeleton of %s stands here.", corpsename);
        if (GET_ADESC(mob)) {
          FREE(GET_ADESC(mob));
        }
        GET_ADESC(mob) = strdup(buf);
        safe_snprintf(buf, MAX_STRING_LENGTH, "the skeleton of %s", corpsename);
        break;
      case 7: /* zombie */
      case 8:
      case 9:
      case 10:
      case 11:
        mob = read_mobile(real_mobile(sinfo->vnum_list[1]), REAL | NOEQUIP);
        IS_ANIMATED(mob) = 1;
        safe_snprintf(buf, MAX_STRING_LENGTH, "The zombie of %s stands here.", corpsename);
        if (GET_ADESC(mob)) {
          FREE(GET_ADESC(mob));
        }
        GET_ADESC(mob) = strdup(buf);
        safe_snprintf(buf, MAX_STRING_LENGTH, "the zombie of %s", corpsename);
        break;
      case 12: /* spectre */
      case 13:
      case 14:
      case 15:
        mob = read_mobile(real_mobile(sinfo->vnum_list[2]), REAL | NOEQUIP);
        IS_ANIMATED(mob) = 1;
        safe_snprintf(buf, MAX_STRING_LENGTH, "The spectre of %s stands here.", corpsename);
        if (GET_ADESC(mob)) {
          FREE(GET_ADESC(mob));
        }
        GET_ADESC(mob) = strdup(buf);
        safe_snprintf(buf, MAX_STRING_LENGTH, "the spectre of %s", corpsename);
        break;
      case 16: /* vampire */
      case 17:
      case 18:
        mob = read_mobile(real_mobile(sinfo->vnum_list[3]), REAL | NOEQUIP);
        IS_ANIMATED(mob) = 1;
        safe_snprintf(buf, MAX_STRING_LENGTH, "The vampire of %s stands here.", corpsename);
        if (GET_ADESC(mob)) {
          FREE(GET_ADESC(mob));
        }
        GET_ADESC(mob) = strdup(buf);
        safe_snprintf(buf, MAX_STRING_LENGTH, "the vampire of %s", corpsename);
        break;
      case 19: /* wraith */
      case 20:
        mob = read_mobile(real_mobile(sinfo->vnum_list[4]), REAL | NOEQUIP);
        IS_ANIMATED(mob) = 1;
        safe_snprintf(buf, MAX_STRING_LENGTH, "The wraith of %s stands here.", corpsename);
        if (GET_ADESC(mob)) {
          FREE(GET_ADESC(mob));
        }
        GET_ADESC(mob) = strdup(buf);
        safe_snprintf(buf, MAX_STRING_LENGTH, "the wraith of %s", corpsename);
        break;
    }
    GET_ANIMATED_MOB_NAME(mob) = strdup(buf);
    GET_CLASS(mob) = CLASS_UNDEAD;
    for (tobj = corpse->contains; tobj; tobj = next_obj) {
      next_obj = tobj->next_content;
      if (GET_OBJ_TYPE(tobj) == ITEM_MONEY) {
        GET_GOLD(mob) += GET_OBJ_VAL(tobj, 0);
      } else {
        obj_from_obj(tobj);
        obj_to_char(tobj, mob);
      }
    }
    extract_obj(corpse);
  }
  if (success) {
    mob->mob_specials.timer = (12 * GET_LEVEL(CAUSER_CH)) + number(-120, 120);
    CREATE(af, struct affected_type, 1);
    af->type = sinfo->spellindex;
    af->duration = mob->mob_specials.timer;
    af->modifier[0] = 0;
    af->location[0] = 0;
    af->bitvector = AFF_CHARM;
    affect_to_char(mob, af);
    GET_LEVEL(mob) = GET_LEVEL(CAUSER_CH);
    char_to_room(mob, CAUSER_CH->in_room);
    add_follower(mob, CAUSER_CH);
  }
}

EVENT(spell_group_points_event)
{
  struct char_data *tch, *k;
  struct follow_type *f, *f_next;

  if (CAUSER_CH == NULL) {
    return;
  }

  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);

  if (!IS_AFFECTED(CAUSER_CH, AFF_GROUP)) {
    return;
  }

  if (!info2) {
    send_to_char("You complete your spell.\r\n", CAUSER_CH);
    say_spell(CAUSER_CH, sinfo->spellindex, NULL, NULL);
  }

  if (CAUSER_CH->master != NULL) {
    k = CAUSER_CH->master;
  } else {
    k = CAUSER_CH;
  }

  for (f = k->followers; f; f = f_next) {
    f_next = f->next;
    tch = f->follower;
    if (tch->in_room != CAUSER_CH->in_room) {
      continue;
    }
    if (!IS_AFFECTED(tch, AFF_GROUP)) {
      continue;
    }

    if (CAUSER_CH != tch && sinfo->aggressive) {
      if (!IS_IMMO(CAUSER_CH) && mag_savingthrow(tch, sinfo->saving_throw)) {
        continue;
      }
      if (getMaxCircle(sinfo) < 5 && AFF2_FLAGGED(tch, AFF2_MINOR_GLOBE)) {
        act("Your minor globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, tch, TO_VICT);
        act("Your spell was deflected by $N's minor globe of invunerability.", TRUE, CAUSER_CH, 0, tch, TO_CHAR);
        act("$N's minor globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, tch, TO_CHAR);
        continue;
      } else if (getMaxCircle(sinfo) >= 5 && getMaxCircle(sinfo) < 9 && AFF_FLAGGED(tch, AFF_MAJOR_GLOBE)) {
        act("Your major globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, tch, TO_VICT);
        act("Your spell was deflected by $N's major globe of invunerability.", TRUE, CAUSER_CH, 0, tch, TO_CHAR);
        act("$N's major globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, tch, TO_CHAR);
        continue;
      }
    }

    mag_points_char(sinfo, CAUSER_CH, tch, GET_LEVEL(CAUSER_CH));
    if (sinfo->send_to_vict) {
      act(sinfo->send_to_vict, TRUE, CAUSER_CH, 0, tch, TO_VICT);
    }
  }
  if ((k != CAUSER_CH) && IS_AFFECTED(k, AFF_GROUP)) {
    mag_points_char(sinfo, CAUSER_CH, k, GET_LEVEL(CAUSER_CH));
  }

  if (sinfo->send_to_vict) {
    act(sinfo->send_to_vict, TRUE, CAUSER_CH, 0, k, TO_VICT);
  }

  mag_points_char(sinfo, CAUSER_CH, CAUSER_CH, GET_LEVEL(CAUSER_CH));

  if (sinfo->send_to_vict) {
    act(sinfo->send_to_vict, TRUE, CAUSER_CH, 0, CAUSER_CH, TO_VICT);
  }
  if (sinfo->send_to_room) {
    act(sinfo->send_to_room, TRUE, CAUSER_CH, 0, 0, TO_ROOM);
  }
  if (sinfo->send_to_char) {
    act(sinfo->send_to_char, TRUE, CAUSER_CH, 0, 0, TO_CHAR);
  }
}

EVENT(spell_group_event)
{
  struct char_data *tch, *k;
  struct follow_type *f, *f_next;

  if (CAUSER_CH == NULL)
    return;

  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);

  if (!IS_AFFECTED(CAUSER_CH, AFF_GROUP)) {
    return;
  }

  if (!info2) {
    send_to_char("You complete your spell.\r\n", CAUSER_CH);
    say_spell(CAUSER_CH, sinfo->spellindex, NULL, NULL);
  }

  if (CAUSER_CH->master != NULL) {
    k = CAUSER_CH->master;
  } else {
    k = CAUSER_CH;
  }

  for (f = k->followers; f; f = f_next) {
    f_next = f->next;
    tch = f->follower;
    if (tch->in_room != CAUSER_CH->in_room) {
      continue;
    }
    if (!IS_AFFECTED(tch, AFF_GROUP)) {
      continue;
    }

    if (CAUSER_CH != tch && sinfo->aggressive) {
      if (!IS_IMMO(CAUSER_CH) && mag_savingthrow(tch, sinfo->saving_throw)) {
        continue;
      }
      if (getMaxCircle(sinfo) < 5 && AFF2_FLAGGED(tch, AFF2_MINOR_GLOBE)) {
        act("Your minor globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, tch, TO_VICT);
        act("Your spell was deflected by $N's minor globe of invunerability.", TRUE, CAUSER_CH, 0, tch, TO_CHAR);
        act("$N's minor globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, tch, TO_CHAR);
        continue;
      } else if (getMaxCircle(sinfo) >= 5 && getMaxCircle(sinfo) < 9 && AFF_FLAGGED(tch, AFF_MAJOR_GLOBE)) {
        act("Your major globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, tch, TO_VICT);
        act("Your spell was deflected by $N's major globe of invunerability.", TRUE, CAUSER_CH, 0, tch, TO_CHAR);
        act("$N's major globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, tch, TO_CHAR);
        continue;
      }
    }

    if (sinfo->unaffect) {
      affect_from_char(tch, spells[find_spell_num(sinfo->unaffect)].spellindex);
    } else {
      mag_affect_char(sinfo, 0, CAUSER_CH, tch, GET_LEVEL(CAUSER_CH));
    }
  }
  if ((k != CAUSER_CH) && IS_AFFECTED(k, AFF_GROUP)) {
    if (sinfo->unaffect) {
      affect_from_char(k, spells[find_spell_num(sinfo->unaffect)].spellindex);
    } else {
      mag_affect_char(sinfo, 0, CAUSER_CH, k, GET_LEVEL(CAUSER_CH));
    }
  }

  if (sinfo->unaffect) {
    affect_from_char(CAUSER_CH, spells[find_spell_num(sinfo->unaffect)].spellindex);
  } else {
    mag_affect_char(sinfo, 0, CAUSER_CH, CAUSER_CH, GET_LEVEL(CAUSER_CH));
  }

  if (sinfo->send_to_vict) {
    act(sinfo->send_to_vict, TRUE, CAUSER_CH, 0, 0, TO_VICT);
  }
  if (sinfo->send_to_room) {
    act(sinfo->send_to_room, TRUE, CAUSER_CH, 0, 0, TO_ROOM);
  }
  if (sinfo->send_to_char) {
    act(sinfo->send_to_char, TRUE, CAUSER_CH, 0, 0, TO_CHAR);
  }
}

EVENT(spell_obj_char_event)
{
  int i = 0;
  struct char_data *vict;
  struct obj_data *object = NULL;
  char abuf[256];
  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);
  if ((char*) info) {
    vict = get_char_room_vis(CAUSER_CH, (char*) info);
    object = get_obj_in_list_vis(CAUSER_CH, (char*) info, CAUSER_CH->carrying);
    if (!object) {
      object = get_obj_in_list_vis(CAUSER_CH, (char*) info, world[CAUSER_CH->in_room].contents);
    }
    if (!vict && !object) {
      safe_snprintf(abuf, sizeof(abuf), "You don't see %s here.\r\n", (char*) info);
      send_to_char(abuf, CAUSER_CH);
      return;
    }
    if (vict && !pk_allowed && sinfo->aggressive) {
      if (!IS_MOB(CAUSER_CH) && !IS_MOB(vict) && vict != CAUSER_CH) {
        send_to_char("Your attempt causes a {Wsparks{w to fizzle at your finger tips.\r\n", CAUSER_CH);
        return;
      }
      if (IS_AFFECTED(CAUSER_CH, AFF_CHARM) && !IS_MOB(CAUSER_CH->master)) {
        send_to_char("You can't order a follower to cast that on a player.\r\n", CAUSER_CH->master);
        return;
      }
    }
  } else {
    vict = CAUSER_CH;
  }
  if (CAUSER_CH != vict && sinfo->aggressive && !object) {
    if (getMaxCircle(sinfo) < 5 && AFF2_FLAGGED(vict, AFF2_MINOR_GLOBE)) {
      act("Your minor globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_VICT);
      act("Your spell was deflected by $N's minor globe of invunerability.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
      act("$N's minor globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
      return;
    } else if (getMaxCircle(sinfo) >= 5 && getMaxCircle(sinfo) < 9 && AFF_FLAGGED(vict, AFF_MAJOR_GLOBE)) {
      act("Your major globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_VICT);
      act("Your spell was deflected by $N's major globe of invunerability.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
      act("$N's major globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
      return;
    }
    if (!IS_IMMO(CAUSER_CH) && mag_savingthrow(vict, sinfo->saving_throw)) {
      act("$N resists your spell.", TRUE, CAUSER_CH, NULL, vict, TO_CHAR);
      act("$N resists $n's spell.", TRUE, CAUSER_CH, NULL, vict, TO_ROOM);
      act("You resist $n's spell.", TRUE, CAUSER_CH, NULL, vict, TO_VICT);
      return;
    }
  }
  if (vict) {
    if (!info2) {
      send_to_char("You complete your spell.\r\n", CAUSER_CH);
      say_spell(CAUSER_CH, sinfo->spellindex, vict, NULL);
    }
    if (sinfo->unaffect) {
      affect_from_char(vict, spells[find_spell_num(sinfo->unaffect)].spellindex);
    } else {
      mag_affect_char(sinfo, 0, CAUSER_CH, vict, GET_LEVEL(CAUSER_CH));
    }
    if (sinfo->send_to_vict) {
      act(sinfo->send_to_vict, TRUE, CAUSER_CH, 0, vict, TO_VICT);
    }
    if (sinfo->send_to_room) {
      act(sinfo->send_to_room, TRUE, CAUSER_CH, 0, vict, TO_ROOM);
    }
    if (sinfo->send_to_char) {
      act(sinfo->send_to_char, TRUE, CAUSER_CH, 0, vict, TO_CHAR);
    }
    if (vict->master != CAUSER_CH && sinfo->aggressive && number(1, 10) < 5) {
      set_fighting(CAUSER_CH, vict);
    }
    for (i = 0; i < NUM_WEARS; i++) {
      if (spells[find_spell_num("remove curse")].spellindex == sinfo->spellindex && vict->equipment[i]) {
        object = vict->equipment[i];
        if (IS_OBJ_STAT(object, ITEM_NODROP)) {
          act("$P's {Ddark{w hue {Wflashes{w and it falls to the ground.{x", TRUE, CAUSER_CH, 0, object, TO_CHAR);
          act("$P's {Ddark{w hue {Wflashes{w and it falls to the ground.{x", TRUE, CAUSER_CH, 0, object, TO_ROOM);
          obj_to_room(object, vict->in_room);
          vict->equipment[i] = NULL;
        }
      }
    }
    object = NULL;
  }
  if (object) {
    if (!info2) {
      send_to_char("You complete your spell.\r\n", CAUSER_CH);
      say_spell(CAUSER_CH, sinfo->spellindex, NULL, object);
    }
    if (!IS_OBJ_STAT(object, ITEM_NODROP) && (spells[find_spell_num("curse")].spellindex == sinfo->spellindex)) {
      mag_affect_obj(CAUSER_CH, object, sinfo, GET_SKILL(CAUSER_CH, sinfo->realm));
      SET_BIT(GET_OBJ_EXTRA(object), ITEM_NODROP);
      SET_BIT(GET_OBJ_EXTRA(object), ITEM_ENCHANTED);
      act("$P glows a {Ddark{w hue that absorbs into itself.{x", TRUE, CAUSER_CH, 0, object, TO_ROOM);
      act("As you wave a hand over $P, it glows a {Ddark{w hue.{x", TRUE, CAUSER_CH, 0, object, TO_CHAR);
    } else {
      if (spells[find_spell_num("curse")].spellindex == sinfo->spellindex) {
        if (IS_OBJ_STAT(object, ITEM_NODROP)) {
          act("$P is already glowing a {Ddark{w hue.{x", TRUE, CAUSER_CH, 0, object, TO_CHAR);
          return;
        }
        if (IS_OBJ_STAT(object, ITEM_ENCHANTED)) {
          act("A {Ddark{w hue begins to glow around $P but fizzles out.{x", TRUE, CAUSER_CH, 0, object, TO_CHAR);
          return;
        }
      }
    }
    if (IS_OBJ_STAT(object, ITEM_NODROP) && (spells[find_spell_num("remove curse")].spellindex == sinfo->spellindex)) {
      mag_affect_obj(CAUSER_CH, object, sinfo, GET_SKILL(CAUSER_CH, sinfo->realm));
      REMOVE_BIT(GET_OBJ_EXTRA(object), ITEM_NODROP);
      if (IS_OBJ_STAT(object, ITEM_ENCHANTED)) {
        REMOVE_BIT(GET_OBJ_EXTRA(object), ITEM_ENCHANTED);
      }
      act("A {Ddark{w hue fades away from $P.{x", TRUE, CAUSER_CH, 0, object, TO_ROOM);
      act("A {Ddark{w hue fades away from $P.{x", TRUE, CAUSER_CH, 0, object, TO_CHAR);
    } else {
      if (spells[find_spell_num("remove curse")].spellindex == sinfo->spellindex) {
        if (!IS_OBJ_STAT(object, ITEM_NODROP)) {
          act("$P never had a {Ddark{w glowing hue.{x", TRUE, CAUSER_CH, 0, object, TO_CHAR);
          return;
        }
      }
    }
    if (!IS_OBJ_STAT(object, ITEM_ENCHANTED) && (spells[find_spell_num("enchant")].spellindex == sinfo->spellindex)) {
      mag_affect_obj(CAUSER_CH, object, sinfo, GET_SKILL(CAUSER_CH, sinfo->realm));
      SET_BIT(GET_OBJ_EXTRA(object), ITEM_ENCHANTED);
      if (sinfo->send_to_room) {
        act(sinfo->send_to_room, TRUE, CAUSER_CH, 0, object, TO_ROOM);
      }
      if (sinfo->send_to_char) {
        act(sinfo->send_to_char, TRUE, CAUSER_CH, 0, object, TO_CHAR);
      }
    } else {
      if (spells[find_spell_num("enchant")].spellindex == sinfo->spellindex) {
        if (!sinfo->aggressive) {
          send_to_char("That item already has been enchanted.\r\n", CAUSER_CH);
        }
      }
    }
  } else {
    if (!info2) {
      act("$n stops casting!", TRUE, CAUSER_CH, 0, 0, TO_ROOM);
    }
  }
}

/*
 * The skips: 1: the caster
 *            2: immortals
 *            3: if no pk on this mud, skips over all players
 *            4: pets (charmed NPCs)
 * players can only hit players in CRIMEOK rooms 4) players can only hit
 * charmed mobs in CRIMEOK rooms
 */

EVENT(spell_area_dam_event)
{
  struct char_data *tch, *next_tch;
  int dam;

  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);

  dam = getSpellDam(sinfo, CAUSER_CH);
  if (!IS_MOB(CAUSER_CH)) {
    dam = modBySpecialization(CAUSER_CH, sinfo, dam);
    dam = dam * GET_SKILL(CAUSER_CH, sinfo->realm) / 110;
  }
  if (!info2) {
    send_to_char("You complete your spell.\r\n", CAUSER_CH);
    say_spell(CAUSER_CH, sinfo->spellindex, NULL, NULL);
  }

  for (tch = world[CAUSER_CH->in_room].people; tch; tch = next_tch) {
    next_tch = tch->next_in_room;

    if (tch == CAUSER_CH) {
      continue;
    }
    if (IS_MOB(tch) && MOB_FLAGGED(tch, MOB_NOKILL)) {
      continue;
    }
    if (IS_MOB(tch) && AFF_FLAGGED(tch, AFF_SUPERINV)) {
      continue;
    }
    if (!ROOM_FLAGGED(IN_ROOM(CAUSER_CH), ROOM_CRIMEOK) && (!pk_allowed && !IS_MOB(CAUSER_CH) && !IS_MOB(tch))) {
      continue;
    }
    if (!ROOM_FLAGGED(IN_ROOM(CAUSER_CH), ROOM_CRIMEOK) && (!IS_MOB(CAUSER_CH) && IS_MOB(tch) && IS_AFFECTED(tch, AFF_CHARM) && GET_RESIST(tch, sinfo->resist_type) < 101)) {
      continue;
    }

    if (tch) {
      if (mag_savingthrow(tch, sinfo->saving_throw)) {
        dam >>= 1;
      }
      if (IS_MOB(CAUSER_CH)) {
        safe_snprintf(debugbuf, sizeof(debugbuf), "MOB CAST: <%s '%s' %s> dam = %d", GET_MOB_NAME(CAUSER_CH), sinfo->command, GET_NAME(tch), dam);
        mudlog(debugbuf, 'D', COM_IMMORT, TRUE);
      }
      damage(CAUSER_CH, tch, dam, sinfo->spellindex, 0, sinfo->resist_type, TRUE, info2 ? 1 : 0);
    }
  }
  FIGHT_STATE(CAUSER_CH, 2);
}

EVENT(spell_confusion_event)
{
  struct char_data *targ;
  struct char_data *vict = get_char_room_vis(CAUSER_CH, (char*) info);
  int prob;
  ACMD(do_flee);

  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);
  if (!info2) {
    send_to_char("You complete your spell.\r\n", CAUSER_CH);
    say_spell(CAUSER_CH, sinfo->spellindex, NULL, NULL);
  }

  if (!vict) {
    return;
  }

  if (!IS_MOB(vict)) {
    send_to_char("Maybe that is not such a good idea.\r\n", CAUSER_CH);
    return;
  }

  if (CAUSER_CH != vict && sinfo->aggressive) {
    if (getMaxCircle(sinfo) < 5 && AFF2_FLAGGED(vict, AFF2_MINOR_GLOBE)) {
      act("Your minor globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_VICT);
      act("Your spell was deflected by $N's minor globe of invunerability.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
      act("$N's minor globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
      return;
    } else if (getMaxCircle(sinfo) >= 5 && getMaxCircle(sinfo) < 9 && AFF_FLAGGED(vict, AFF_MAJOR_GLOBE)) {
      act("Your major globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_VICT);
      act("Your spell was deflected by $N's major globe of invunerability.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
      act("$N's major globe of invunerability deflected $n's spell.", TRUE, CAUSER_CH, 0, vict, TO_CHAR);
      return;
    }
    if (!IS_IMMO(CAUSER_CH) && mag_savingthrow(vict, sinfo->saving_throw)) {
      act("$N resists your spell.", TRUE, CAUSER_CH, NULL, vict, TO_CHAR);
      act("$N resists $n's spell.", TRUE, CAUSER_CH, NULL, vict, TO_ROOM);
      act("You resist $n's spell.", TRUE, CAUSER_CH, NULL, vict, TO_VICT);
      return;
    }
  }

  prob = number(0, 100);

  if (number(0, 100) < (20 + (GET_LEVEL(CAUSER_CH) - (2 * GET_LEVEL(vict))))) {
    if (prob < 10) {
      act("You feel very sleepy...  Zzzz......", FALSE, vict, 0, 0, TO_CHAR);
      act("$n goes to sleep.", TRUE, vict, 0, 0, TO_ROOM);
      GET_POS(vict) = POS_SLEEPING;
      update_pos(vict);
    } else if (prob < 20) {
      do_flee(vict, "", 0, 0);
    } else if (prob < 50) {
      for (targ = world[vict->in_room].people; targ; targ = targ->next_in_room) {
        if (targ == vict || targ == CAUSER_CH) {
          continue;
        }
        if (FIGHTING(vict) == targ) {
          continue;
        }
        if (IS_MOB(targ)) {
          stop_fighting(vict);
          hit(vict, targ, TYPE_UNDEFINED);
          break;
        }
      }
    } else {
      send_to_char("Nothing seems to happen.\r\n", CAUSER_CH);
    }
  }
}

EVENT(spell_charm_event)
{
  struct affected_type af;
  struct char_data *vict = get_char_room_vis(CAUSER_CH, (char*) info);

  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);

  if (vict == NULL || CAUSER_CH == NULL) {
    return;
  }

  if (!info2) {
    send_to_char("You complete your spell.\r\n", CAUSER_CH);
    say_spell(CAUSER_CH, sinfo->spellindex, vict, NULL);
  }
  if (vict == CAUSER_CH) {
    send_to_char("You like yourself even better!\r\n", CAUSER_CH);
  } else if (!IS_MOB(vict) && PRF_FLAGGED(vict, PRF_NOSUMMON)) {
    send_to_char("You fail.\r\n", CAUSER_CH);
  } else if (IS_AFFECTED(CAUSER_CH, AFF_CHARM)) {
    send_to_char("You can't have any followers of your own!\r\n", CAUSER_CH);
  } else if (IS_AFFECTED(vict, AFF_CHARM) || GET_LEVEL(CAUSER_CH) < GET_LEVEL(vict)) {
    send_to_char("You fail.\r\n", CAUSER_CH);
  } else if (!IS_MOB(vict)) {
    send_to_char("You fail - shouldn't be doing it anyway.\r\n", CAUSER_CH);
  } else if (circle_follow(vict, CAUSER_CH)) {
    send_to_char("Sorry, following in circles can not be allowed.\r\n", CAUSER_CH);
  } else if (!IS_IMMO(CAUSER_CH) && mag_savingthrow(vict, SAVING_PARA)) {
    act("$N resists your spell.", TRUE, VICTIM_CH, NULL, VICTIM_CH, TO_CHAR);
    act("$N resists $n's spell.", TRUE, VICTIM_CH, NULL, VICTIM_CH, TO_ROOM);
    act("You resist $n's spell.", TRUE, VICTIM_CH, NULL, VICTIM_CH, TO_VICT);
  } else {
    if (vict->master) {
      stop_follower(vict);
    }

    add_follower(vict, CAUSER_CH);

    af.type = sinfo->spellindex;

    if (GET_INT(vict)) {
      af.duration = 24 * 100 / GET_INT(vict);
    } else {
      af.duration = 24 * 18;
    }

    af.modifier[0] = 0;
    af.location[0] = 0;
    af.bitvector = AFF_CHARM;
    affect_to_char(vict, &af);

    act("Isn't $n just such a nice fellow?", FALSE, CAUSER_CH, 0, vict, TO_VICT);
    if (IS_MOB(vict)) {
      REMOVE_BIT(MOB_FLAGS(vict), MOB_AGGRESSIVE);
      REMOVE_BIT(MOB_FLAGS(vict), MOB_SPEC);
    }
  }
}

EVENT(spell_dispel_magic_event)
{
  struct char_data *vict;
  char abuf[256];

  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);

  if ((struct char_data*) victim) {
    vict = (struct char_data*) victim;
  } else if ((char*) info) {
    vict = get_char_room_vis(CAUSER_CH, (char*) info);
    if (!vict) {
      safe_snprintf(abuf, sizeof(abuf), "You don't see %s here.\r\n", (char*) info);
      send_to_char(abuf, CAUSER_CH);
      return;
    }
    if (!pk_allowed && sinfo->aggressive) {
      if (!IS_MOB(CAUSER_CH) && !IS_MOB(vict) && vict != CAUSER_CH) {
        send_to_char("It's not a good idea to cast that on players.\r\n", CAUSER_CH);
        return;
      }
      if (IS_AFFECTED(CAUSER_CH, AFF_CHARM) && !IS_MOB(CAUSER_CH->master)) {
        send_to_char("You can't order a follower to cast that on a player.\r\n", CAUSER_CH->master);
        return;
      }
    }
  } else {
    vict = CAUSER_CH;
  }

  if (CAUSER_CH != vict && sinfo->aggressive) {
    if (!IS_IMMO(CAUSER_CH) && mag_savingthrow(vict, sinfo->saving_throw)) {
      act("$N resists your spell.", TRUE, CAUSER_CH, NULL, vict, TO_CHAR);
      act("$N resists $n's spell.", TRUE, CAUSER_CH, NULL, vict, TO_ROOM);
      act("You resist $n's spell.", TRUE, CAUSER_CH, NULL, vict, TO_VICT);
      return;
    }
  }

  if (vict) {
    if (!info2) {
      send_to_char("You complete your spell.\r\n", CAUSER_CH);
      say_spell(CAUSER_CH, sinfo->spellindex, vict, NULL);
    }

    if (vict->affected && number(1, 100) <= (GET_SKILL(CAUSER_CH, sinfo->realm) / 2)) {
      while (vict->affected) {
        affect_remove(vict, vict->affected);
      }
      if ((sinfo->send_to_vict) && (vict != CAUSER_CH)) {
        act(sinfo->send_to_vict, TRUE, CAUSER_CH, 0, vict, TO_VICT);
      }
      if (sinfo->send_to_room) {
        act(sinfo->send_to_room, TRUE, vict, 0, CAUSER_CH, TO_ROOM);
      }
      if (sinfo->send_to_char) {
        act(sinfo->send_to_char, TRUE, CAUSER_CH, 0, vict, TO_CHAR);
      }
    }
  } else {
    if (!info2) {
      act("$n stops casting!", TRUE, CAUSER_CH, 0, 0, TO_ROOM);
    }
  }
  if (IS_NPC(vict) && vict->master != CAUSER_CH && sinfo->aggressive && number(0, 1)) {
    hit(vict, CAUSER_CH, TYPE_UNDEFINED);
    FIGHT_STATE(CAUSER_CH, 2);
  }
}

EVENT(spell_telekinesis_event)
{
  struct obj_data *obj;
  struct obj_data *next_obj;
  char telebuf[256];

  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);

  for (obj = world[CAUSER_CH->in_room].contents; obj; obj = next_obj) {
    next_obj = obj->next_content;
    if (IS_OBJ_STAT(obj, ITEM_UNDERWATER)) {
      safe_snprintf(telebuf, sizeof(telebuf), "You are able to raise %s from the depths of water.\r\n", OBJS(obj, CAUSER_CH));
      send_to_char(telebuf, CAUSER_CH);
      act("$n raises $p from the murky depths.", TRUE, CAUSER_CH, obj, NULL, TO_ROOM);
      REMOVE_BIT(GET_OBJ_EXTRA(obj), ITEM_UNDERWATER);
      return;
    }
  }
  send_to_char("You are unable to find anything here.\r\n", CAUSER_CH);
}

EVENT(spell_magical_lock_event)
{
  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);
}

EVENT(spell_magical_unlock_event)
{
  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);
}

EVENT(spell_disintegrate_event)
{
  int dam = 600;
  struct char_data *vict;
  char abuf[256];
  char pbuf[256];
  int saved = 0;
  int eq;

  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);
  if ((struct char_data*) victim) {
    vict = (struct char_data*) victim;
  } else if ((char*) info) {
    vict = get_char_room_vis(CAUSER_CH, (char*) info);
    if (!vict) {
      safe_snprintf(abuf, sizeof(abuf), "You don't see %s here.\r\n", (char*) info);
      send_to_char(abuf, CAUSER_CH);
      return;
    }
    if (!pk_allowed) {
      if (!IS_MOB(CAUSER_CH) && !IS_MOB(vict) && vict != CAUSER_CH) {
        send_to_char("It's not a good idea to cast that on players.\r\n", CAUSER_CH);
        return;
      }
      if (IS_AFFECTED(CAUSER_CH, AFF_CHARM) && !IS_MOB(CAUSER_CH->master)) {
        send_to_char("You can't order a follower to cast that on a player.\r\n", CAUSER_CH->master);
        return;
      }
    }
  } else if (FIGHTING(CAUSER_CH)) {
    vict = FIGHTING(CAUSER_CH);
  } else {
    if (info2) {
      return;
    }
    send_to_char("Casting this on youself would hurt!\r\n", CAUSER_CH);
    act("$n stops casting!", TRUE, CAUSER_CH, 0, 0, TO_ROOM);
    return;
  }
  if (vict) {
    if (!info2) {
      send_to_char("You complete your spell.\r\n", CAUSER_CH);
      say_spell(CAUSER_CH, sinfo->spellindex, vict, NULL);
    }

    dam = modBySpecialization(CAUSER_CH, sinfo, dam);
    dam = dam * GET_SKILL(CAUSER_CH, sinfo->realm) / 110;

    if (mag_savingthrow(vict, sinfo->saving_throw)) {
      dam >>= 1;
      saved = 1;
    }
    damage(CAUSER_CH, vict, dam, sinfo->spellindex, 0, sinfo->resist_type, TRUE, info2 ? 1 : 0);
    if (!saved) {
      for (eq = 0; eq < NUM_WEARS; eq++) {
        if (!GET_EQ(vict, eq))
          continue;
        if (IS_OBJ_STAT(GET_EQ(vict, eq), ITEM_NOBURN))
          continue;
        if (GET_OBJ_TYPE(GET_EQ(vict, eq)) != ITEM_ARMOR)
          continue;
        if (number(0, 1)) {
          act("$p{Y disintegrates into a cloud of dust!{x", FALSE, vict, GET_EQ(vict, eq), NULL, TO_CHAR);
          if (!IS_NPC(vict)) {
            safe_snprintf(pbuf, sizeof(pbuf), "%s lost item %s (disintegrate)", GET_NAME(vict), GET_EQ(vict, eq)->short_description);
            mudlog(pbuf, 'Q', COM_IMMORT, TRUE);
            plog(pbuf, vict, 0);
          }
          extract_obj(unequip_char(vict, eq));
        }
      }
    }
  } else {
    if (!info2) {
      act("$n stops casting!", TRUE, CAUSER_CH, 0, 0, TO_ROOM);
    }
  }
}

EVENT(spell_resurrection_event)
{
  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);
}

EVENT(spell_turn_undead_event)
{
  int prob = 0, dam = 0;
  ACMD(do_flee);
  struct char_data *vict = get_char_room_vis(CAUSER_CH, (char*) info);

  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);

  if (!vict) {
    return;
  }

  if (!IS_MOB(vict)) {
    send_to_char("Maybe that is not such a good idea.\r\n", CAUSER_CH);
    return;
  }

  if (GET_CLASS(vict) == CLASS_UNDEAD) {
    return;
  }

  prob = number(0, 100);

  if ((GET_LEVEL(CAUSER_CH) - GET_LEVEL(vict)) > 10) {
    if (prob < 50) {
      dam = GET_HIT(vict) + 30;
      if (IS_MOB(CAUSER_CH)) {
        safe_snprintf(debugbuf, sizeof(debugbuf), "MOB CAST: <%s '%s' %s> dam = %d", GET_MOB_NAME(CAUSER_CH), sinfo->command, GET_NAME(vict), dam);
        mudlog(debugbuf, 'D', COM_IMMORT, TRUE);
      }
      damage(CAUSER_CH, vict, dam, sinfo->spellindex, 0, DAM_MAGIC, TRUE, info2 ? 1 : 0);
    } else if (prob < 90) {
      do_flee(vict, "", 0, 0);
    }
  } else if ((GET_LEVEL(CAUSER_CH) - GET_LEVEL(vict)) > 4) {
    if (IS_MOB(CAUSER_CH)) {
      safe_snprintf(debugbuf, sizeof(debugbuf), "MOB CAST: <%s '%s' %s> dam = %d", GET_MOB_NAME(CAUSER_CH), sinfo->command, GET_NAME(vict), dam);
      mudlog(debugbuf, 'D', COM_IMMORT, TRUE);
    }
    damage(CAUSER_CH, vict, GET_LEVEL(CAUSER_CH), sinfo->spellindex, 0, DAM_MAGIC, TRUE, info2 ? 1 : 0);
    if (prob < 35) {
      do_flee(vict, "", 0, 0);
    }
  } else if ((GET_LEVEL(CAUSER_CH) - GET_LEVEL(vict)) < 4 && (GET_LEVEL(CAUSER_CH) - GET_LEVEL(vict)) > -4) {
    if (prob < ((GET_LEVEL(CAUSER_CH) - GET_LEVEL(vict)) * 2 + 20)) {
      do_flee(vict, "", 0, 0);
    }
  }
}

EVENT(spell_identify_event)
{
  int i;
  int found;

  extern char *item_types[];
  extern struct index_data *obj_index;
  extern char *extra_bits[];
  extern char *apply_types[];
  extern char *affected_bits[];
  extern char *affected_bits2[];
  struct obj_data *obj = get_obj_vis(CAUSER_CH, (char*) info);

  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);

  if (obj) {
    int oktosee = 0;
    int zonenum = 0;

    zonenum = (GET_OBJ_VNUM(obj) - (GET_OBJ_VNUM(obj) % 100)) / 100;
    if (zonenum != 12 && zonenum != 30 && zonenum != 31 && !COM_FLAGGED(CAUSER_CH, COM_ADMIN)) {
      for (i = 0; i < 4; i++) {
        if (CAUSER_CH->olc_zones[i] == zonenum) {
          oktosee = 1;
        }
      }
      if (!oktosee || zonenum == 0) {
        send_to_char("You are not authorized to see info in this zone.\r\n", CAUSER_CH);
        return;
      }
    }
    send_to_char("You feel informed:\r\n", CAUSER_CH);
    {
      size_t len = safe_snprintf(buf, MAX_STRING_LENGTH, "Object '%s', Item type: ", obj->short_description);
      sprinttype(GET_OBJ_TYPE(obj), item_types, buf2);
      len += safe_snprintf(buf + len, MAX_STRING_LENGTH - len, "%s\r\n", buf2);
      send_to_char(buf, CAUSER_CH);
    }

    if (GET_OBJ_BITV(obj) || GET_OBJ_BITV2(obj)) {
      send_to_char("Item will give you following abilities:  ", CAUSER_CH);
      sprintbit(GET_OBJ_BITV(obj), affected_bits, buf);
      sprintbit(GET_OBJ_BITV2(obj), affected_bits2, buf2);
      safe_snprintf(buf + strlen(buf), MAX_STRING_LENGTH - strlen(buf), " %s\r\n", buf2);
      send_to_char(buf, CAUSER_CH);
    }
    send_to_char("Item is: ", CAUSER_CH);
    sprintbit(GET_OBJ_EXTRA(obj), extra_bits, buf);
    safe_snprintf(buf + strlen(buf), MAX_STRING_LENGTH - strlen(buf), "\r\n");
    send_to_char(buf, CAUSER_CH);

    safe_snprintf(buf, MAX_STRING_LENGTH, "Weight: %d, Value: %d\r\n", GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj));
    send_to_char(buf, CAUSER_CH);

    switch (GET_OBJ_TYPE(obj)) {
      case ITEM_SCROLL:
      case ITEM_POTION:
        {
          size_t len = safe_snprintf(buf, MAX_STRING_LENGTH, "This %s casts: ", item_types[(int) GET_OBJ_TYPE(obj)]);

          if (GET_OBJ_VAL(obj, 1) >= 1) {
            len += safe_snprintf(buf + len, MAX_STRING_LENGTH - len, " %s", get_spell_name(GET_OBJ_VAL(obj, 1)));
          }
          if (GET_OBJ_VAL(obj, 2) >= 1) {
            len += safe_snprintf(buf + len, MAX_STRING_LENGTH - len, " %s", get_spell_name(GET_OBJ_VAL(obj, 2)));
          }
          if (GET_OBJ_VAL(obj, 3) >= 1) {
            len += safe_snprintf(buf + len, MAX_STRING_LENGTH - len, " %s", get_spell_name(GET_OBJ_VAL(obj, 3)));
          }
          safe_snprintf(buf + len, MAX_STRING_LENGTH - len, "\r\n");
          send_to_char(buf, CAUSER_CH);
        }
        break;
      case ITEM_WAND:
      case ITEM_STAFF:
        {
          size_t len = safe_snprintf(buf, MAX_STRING_LENGTH, "This %s casts: ", item_types[(int) GET_OBJ_TYPE(obj)]);
          len += safe_snprintf(buf + len, MAX_STRING_LENGTH - len, " %s\r\n", get_spell_name(GET_OBJ_VAL(obj, 3)));
          safe_snprintf(buf + len, MAX_STRING_LENGTH - len, "It has %d maximum charge%s and %d remaining.\r\n", GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 1) == 1 ? "" : "s", GET_OBJ_VAL(obj, 2));
          send_to_char(buf, CAUSER_CH);
        }
        break;
      case ITEM_WEAPON:
        {
          size_t len = safe_snprintf(buf, MAX_STRING_LENGTH, "Damage Dice is '%dD%d'", GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2));
          safe_snprintf(buf + len, MAX_STRING_LENGTH - len, " for an average per-round damage of %.1f.\r\n", ((GET_OBJ_VAL(obj, 1) + (GET_OBJ_VAL(obj,1) * GET_OBJ_VAL(obj, 2))) / 2.0));
          send_to_char(buf, CAUSER_CH);
        }
        break;
      case ITEM_ARMOR:
        safe_snprintf(buf, MAX_STRING_LENGTH, "AC-apply is %d\r\n", GET_OBJ_VAL(obj, 0));
        send_to_char(buf, CAUSER_CH);
        break;
    }
    found = FALSE;
    for (i = 0; i < MAX_OBJ_AFFECT; i++) {
      if ((obj->affected[i].location != APPLY_NONE) && (obj->affected[i].modifier != 0)) {
        if (!found) {
          send_to_char("Can affect you as :\r\n", CAUSER_CH);
          found = TRUE;
        }
        sprinttype(obj->affected[i].location, apply_types, buf2);
        safe_snprintf(buf, MAX_STRING_LENGTH, "   Affects: %s By %d\r\n", buf2, obj->affected[i].modifier);
        send_to_char(buf, CAUSER_CH);
      }
    }
  } else {
    safe_snprintf(buf, MAX_STRING_LENGTH, "You don't see %s here.\r\n", (char*) info);
    send_to_char(buf, CAUSER_CH);
  }
}

EVENT(spell_create_water_event)
{
  int water;
  void name_to_drinkcon(struct obj_data * obj, int type);
  void name_from_drinkcon(struct obj_data * obj);
  struct obj_data *obj = get_obj_vis(CAUSER_CH, (char*) info);

  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);

  if (CAUSER_CH == NULL) {
    return;
  }

  if (obj == NULL) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "You don't see %s here.\r\n", (char*) info);
    send_to_char(buf, CAUSER_CH);
    return;
  }
  if (GET_OBJ_TYPE(obj) == ITEM_DRINKCON) {
    if ((GET_OBJ_VAL(obj, 2) != LIQ_WATER) && (GET_OBJ_VAL(obj, 1) != 0)) {
      name_from_drinkcon(obj);
      GET_OBJ_VAL(obj, 2) = LIQ_SLIME;
      name_to_drinkcon(obj, LIQ_SLIME);
    } else {
      water = MAX(GET_OBJ_VAL(obj, 0) - GET_OBJ_VAL(obj, 1), 0);
      if (water > 0) {
        GET_OBJ_VAL(obj, 2) = LIQ_WATER;
        GET_OBJ_VAL(obj, 1) += water;
        weight_change_object(obj, water);
        name_from_drinkcon(obj);
        name_to_drinkcon(obj, LIQ_WATER);
        act("$p is filled.", FALSE, CAUSER_CH, obj, 0, TO_CHAR);
      }
    }
  }
}

EVENT(scribe_event)
{
  int time = GET_SPELL_CIRCLE(CAUSER_CH, GET_SCRIBING(CAUSER_CH)) * 5; /* five seconds per page */
  int meditate = spells[find_skill_num("meditate")].spellindex;
  int page = (int) info2; /* slot to store spell in */
  /* priests don't need a writing instrument they "pray" their spells into
   * their holy symbol.
   */

  if (IS_IMMO(CAUSER_CH)) {
    time = 1;
  }
  if ((int) info != 0) {
    if ((IS_MAGE(CAUSER_CH) && GET_EQ(CAUSER_CH, WEAR_HOLD) && GET_EQ(CAUSER_CH, WEAR_HOLD_2)) || ((GET_EQ(CAUSER_CH, WEAR_HOLD) || GET_EQ(CAUSER_CH, WEAR_HOLD_2)) && IS_PRI(CAUSER_CH))) {
      if (GET_OBJ_TYPE(GET_EQ(CAUSER_CH, WEAR_HOLD)) == ITEM_SPELLBOOK) {
        if (IS_PRI(CAUSER_CH) || GET_OBJ_TYPE(GET_EQ(CAUSER_CH, WEAR_HOLD_2)) == ITEM_PEN) {
          if (LIGHT_OK(CAUSER_CH)) {
            if (INVIS_OK_OBJ(CAUSER_CH, GET_EQ(CAUSER_CH, WEAR_HOLD))) {
              if (IS_PRI(CAUSER_CH) || INVIS_OK_OBJ(CAUSER_CH, GET_EQ(CAUSER_CH, WEAR_HOLD_2))) {
                /* add event with info - 1 */
                if (MEDITATING(CAUSER_CH) && GET_SKILL(CAUSER_CH, meditate) > number(0, 150)) {
                  time >>= 1;
                  improve_skill(CAUSER_CH, meditate, SKUSE_FREQUENT);
                }
                if (IS_PRI(CAUSER_CH)) {
                  send_to_char("You continue praying to your god while rubbing your talisman.\r\n", CAUSER_CH);
                } else {
                  send_to_char("You complete another page in your spellbook.\r\n", CAUSER_CH);
                }
                if (!GET_COND(CAUSER_CH, THIRST) || !GET_COND(CAUSER_CH, FULL)) {
                  time <<= 1;
                }
                add_event(time, scribe_event, EVENT_SCRIBE, CAUSER_CH, NULL, (void *) (long) (info - 1), NULL, NULL, page);
              } else { /* lost your pen? */
                REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_SCRIBING);
                send_to_char("You seem to have missplaced your writing instrument.\r\n", CAUSER_CH);
              }
            } else { /* lost your spellbook? */
              REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_SCRIBING);
              if (IS_PRI(CAUSER_CH)) {
                send_to_char("You seem to have missplaced religious symbol.\r\n", CAUSER_CH);
              } else {
                send_to_char("You seem to have missplaced your spellbook.\r\n", CAUSER_CH);
              }
            }
          } else { /* too dark to see */
            REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_SCRIBING);
            if (IS_PRI(CAUSER_CH)) {
              send_to_char("It is too dark for you to see your religious symbol\r\n", CAUSER_CH);
            } else {
              send_to_char("It is too dark to scribe!\r\n", CAUSER_CH);
            }
          }
        } else { /* need pen in other hand */
          REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_SCRIBING);
          send_to_char("You seem to have missplaced your writing instrument.\r\n", CAUSER_CH);
        }
      } else if (IS_PRI(CAUSER_CH) || GET_OBJ_TYPE(GET_EQ(CAUSER_CH, WEAR_HOLD)) == ITEM_PEN) {
        if (GET_OBJ_TYPE(GET_EQ(CAUSER_CH, WEAR_HOLD_2)) == ITEM_SPELLBOOK) {
          if (LIGHT_OK(CAUSER_CH)) {
            if (IS_PRI(CAUSER_CH) || INVIS_OK_OBJ(CAUSER_CH, GET_EQ(CAUSER_CH, WEAR_HOLD))) {
              if (INVIS_OK_OBJ(CAUSER_CH, GET_EQ(CAUSER_CH, WEAR_HOLD_2))) {
                /* add event with info - 1 */
                if (MEDITATING(CAUSER_CH) && GET_SKILL(CAUSER_CH, meditate) > number(0, 150)) {
                  time >>= 1;
                  improve_skill(CAUSER_CH, meditate, SKUSE_FREQUENT);
                }
                if (IS_PRI(CAUSER_CH)) {
                  send_to_char("You continue praying to your god while rubbing your talisman.\r\n", CAUSER_CH);
                } else {
                  send_to_char("You complete another page in your spellbook.\r\n", CAUSER_CH);
                }
                if (!GET_COND(CAUSER_CH, THIRST) || !GET_COND(CAUSER_CH, FULL)) {
                  time *= 2;
                }
                add_event(time, scribe_event, EVENT_SCRIBE, CAUSER_CH, NULL, (void *) (long) (info - 1), NULL, NULL, page);
              } else { /* lost your spellbook? */
                REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_SCRIBING);
                if (IS_PRI(CAUSER_CH)) {
                  send_to_char("You seem to have missplaced your religious symbol.\r\n", CAUSER_CH);
                } else {
                  send_to_char("You seem to have missplaced your spellbook.\r\n", CAUSER_CH);
                }
              }
            } else { /* lost your pen? */
              REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_SCRIBING);
              send_to_char("You seem to have missplaced your writing instrument.\r\n", CAUSER_CH);
            }
          } else { /* too dark to see */
            REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_SCRIBING);
            if (IS_PRI(CAUSER_CH)) {
              send_to_char("It is too dark for you to see your religious symbol.\r\n", CAUSER_CH);
            } else {
              send_to_char("It is too dark to scribe!\r\n", CAUSER_CH);
            }
          }
        } else { /* spellbook must be in other hand */
          REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_SCRIBING);
          send_to_char("You need to be holding your spellbook too.\r\n", CAUSER_CH);
        }
      } else { /* where spellbook and pen? */
        REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_SCRIBING);
        if (IS_PRI(CAUSER_CH)) {
          send_to_char("You need to be holding your religious symbol.\r\n", CAUSER_CH);
        } else {
          send_to_char("You need to be holding your spellbook and a writing instrument.\r\n", CAUSER_CH);
        }
      }
    } else { /* where spellbook and pen? */
      REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_SCRIBING);
      if (IS_PRI(CAUSER_CH)) {
        send_to_char("You need to be holding your religious symbol.\r\n", CAUSER_CH);
      } else {
        send_to_char("You need to be holding your spellbook and a writing instrument.\r\n", CAUSER_CH);
      }
    }
  } else { /* add spell to spellbook and remove scribing bit */
    REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_SCRIBING);
    if ((IS_MAGE(CAUSER_CH) && GET_EQ(CAUSER_CH, WEAR_HOLD) && GET_EQ(CAUSER_CH, WEAR_HOLD_2)) || ((GET_EQ(CAUSER_CH, WEAR_HOLD) || GET_EQ(CAUSER_CH, WEAR_HOLD_2)) && IS_PRI(CAUSER_CH))) {
      if (GET_OBJ_TYPE(GET_EQ(CAUSER_CH, WEAR_HOLD)) == ITEM_SPELLBOOK) {
        if (IS_PRI(CAUSER_CH) || GET_OBJ_TYPE(GET_EQ(CAUSER_CH, WEAR_HOLD_2)) == ITEM_PEN) {
          if (LIGHT_OK(CAUSER_CH)) {
            if (INVIS_OK_OBJ(CAUSER_CH, GET_EQ(CAUSER_CH, WEAR_HOLD))) {
              if (IS_PRI(CAUSER_CH) || INVIS_OK_OBJ(CAUSER_CH, GET_EQ(CAUSER_CH, WEAR_HOLD_2))) {
                /* add spell to spellbook */
                if (IS_PRI(CAUSER_CH)) {
                  send_to_char("You complete your prayers.\r\n", CAUSER_CH);
                } else {
                  send_to_char("You complete the spell in your spellbook.\r\n", CAUSER_CH);
                }
                GET_OBJ_SPELLLISTNUM(GET_EQ(CAUSER_CH, WEAR_HOLD), page) = GET_SCRIBING(CAUSER_CH)->spellindex;
                GET_OBJ_VAL(GET_EQ(CAUSER_CH, WEAR_HOLD), 1) -= GET_SPELL_CIRCLE(CAUSER_CH, GET_SCRIBING(CAUSER_CH));
              } else { /* lost your pen? */
                send_to_char("You seem to have missplaced your writing instrument.\r\n", CAUSER_CH);
              }
            } else { /* lost your spellbook? */
              if (IS_PRI(CAUSER_CH)) {
                send_to_char("You seem to have missplaced religious symbol.\r\n", CAUSER_CH);
              } else {
                send_to_char("You seem to have missplaced your spellbook.\r\n", CAUSER_CH);
              }
            }
          } else { /* too dark to see */
            if (IS_PRI(CAUSER_CH)) {
              send_to_char("It is too dark for you to see your religious symbol\r\n", CAUSER_CH);
            } else {
              send_to_char("It is too dark to scribe!\r\n", CAUSER_CH);
            }
          }
        } else { /* need pen in other hand */
          send_to_char("You seem to have missplaced your writing instrument.\r\n", CAUSER_CH);
        }
      } else if (IS_PRI(CAUSER_CH) || GET_OBJ_TYPE(GET_EQ(CAUSER_CH, WEAR_HOLD)) == ITEM_PEN) {
        if (GET_OBJ_TYPE(GET_EQ(CAUSER_CH, WEAR_HOLD_2)) == ITEM_SPELLBOOK) {
          if (LIGHT_OK(CAUSER_CH)) {
            if (IS_PRI(CAUSER_CH) || INVIS_OK_OBJ(CAUSER_CH, GET_EQ(CAUSER_CH, WEAR_HOLD))) {
              if (INVIS_OK_OBJ(CAUSER_CH, GET_EQ(CAUSER_CH, WEAR_HOLD_2))) {
                /* add spell to spellbook */
                if (IS_PRI(CAUSER_CH)) {
                  send_to_char("You complete your prayers.\r\n", CAUSER_CH);
                } else {
                  send_to_char("You complete the spell in your spellbook.\r\n", CAUSER_CH);
                }
                GET_OBJ_SPELLLISTNUM(GET_EQ(CAUSER_CH, WEAR_HOLD_2), page) = GET_SCRIBING(CAUSER_CH)->spellindex;
                GET_OBJ_VAL(GET_EQ(CAUSER_CH, WEAR_HOLD_2), 1) -= GET_SPELL_CIRCLE(CAUSER_CH, GET_SCRIBING(CAUSER_CH));
              } else { /* lost your spellbook? */
                if (IS_PRI(CAUSER_CH)) {
                  send_to_char("You seem to have missplaced your religious symbol.\r\n", CAUSER_CH);
                } else {
                  send_to_char("You seem to have missplaced your spellbook.\r\n", CAUSER_CH);
                }
              }
            } else { /* lost your pen? */
              send_to_char("You seem to have missplaced your writing instrument.\r\n", CAUSER_CH);
            }
          } else { /* too dark to see */
            if (IS_PRI(CAUSER_CH)) {
              send_to_char("It is too dark for you to see your religious symbol.\r\n", CAUSER_CH);
            } else {
              send_to_char("It is too dark to scribe!\r\n", CAUSER_CH);
            }
          }
        } else { /* spellbook must be in other hand */
          send_to_char("You need to be holding your spellbook too.\r\n", CAUSER_CH);
        }
      } else { /* where spellbook and pen? */
        if (IS_PRI(CAUSER_CH)) {
          send_to_char("You need to be holding your religious symbol.\r\n", CAUSER_CH);
        } else {
          send_to_char("You need to be holding your spellbook and a writing instrument.\r\n", CAUSER_CH);
        }
      }
    } else { /* where spellbook and pen? */
      if (IS_PRI(CAUSER_CH)) {
        send_to_char("You need to be holding your religious symbol.\r\n", CAUSER_CH);
      } else {
        send_to_char("You need to be holding your spellbook and a writing instrument.\r\n", CAUSER_CH);
      }
    }
  }
}

EVENT(memorize_event)
{
  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_MEMMING);
}

EVENT(spell_destroy_inventory_event)
{
  struct char_data *tch, *next_tch;
  struct obj_data *obj, *next_obj;
  char pbuf[256];
  int dam;

  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);

  dam = getSpellDam(sinfo, CAUSER_CH);
  if (!IS_MOB(CAUSER_CH)) {
    dam = modBySpecialization(CAUSER_CH, sinfo, dam);
    dam = dam * GET_SKILL(CAUSER_CH, sinfo->realm) / 110;
  }
  if (!info2) {
    send_to_char("You complete your spell.\r\n", CAUSER_CH);
    say_spell(CAUSER_CH, sinfo->spellindex, NULL, NULL);
  }

  for (tch = world[CAUSER_CH->in_room].people; tch; tch = next_tch) {
    next_tch = tch->next_in_room;

    if (tch == CAUSER_CH) {
      continue;
    }
    if (IS_MOB(tch) && MOB_FLAGGED(tch, MOB_NOKILL)) {
      continue;
    }
    if (!IS_MOB(tch) && GET_LEVEL(tch) >= LVL_IMMORT) {
      continue;
    }
    if (IS_MOB(tch) && IS_AFFECTED(tch, AFF_SUPERINV)) {
      continue;
    }
    if (!ROOM_FLAGGED(IN_ROOM(CAUSER_CH), ROOM_CRIMEOK) && (!pk_allowed && !IS_MOB(CAUSER_CH) && !IS_MOB(tch))) {
      continue;
    }
    if (!ROOM_FLAGGED(IN_ROOM(CAUSER_CH), ROOM_CRIMEOK) && (!IS_MOB(CAUSER_CH) && IS_MOB(tch) && IS_AFFECTED(tch, AFF_CHARM))) {
      continue;
    }
    if (AFF2_FLAGGED(tch, AFF2_PROT_FIRE) && strcmp(sinfo->command, "fire breath") == 0) {
      continue;
    }
    if (AFF2_FLAGGED(tch, AFF2_PROT_ICE) && strcmp(sinfo->command, "frost breath") == 0) {
      continue;
    }

    if (tch) {
      if (mag_savingthrow(tch, sinfo->saving_throw)) {
        dam >>= 1;
      }
      if (number(0, 61) < GET_LEVEL(tch)) {
        if (!mag_savingthrow(tch, sinfo->saving_throw)) {
          for (obj = tch->carrying; obj; obj = next_obj) {
            next_obj = obj->next_content;
            if (IS_OBJ_STAT(obj, ITEM_NOBURN))
              continue;
            if (number(0, 2)) {
              if (strcmp(sinfo->command, "fire breath") == 0) {
                act("$p{r is turned into ash!{x", FALSE, tch, obj, NULL, TO_CHAR);
              } else {
                act("$p{B shatters into tiny frozen shards!{x", FALSE, tch, obj, NULL, TO_CHAR);
              }
              safe_snprintf(pbuf, sizeof(pbuf), "%s lost item %s (dragonbreath)", GET_NAME(tch), obj->short_description);
              mudlog(pbuf, 'Q', COM_IMMORT, TRUE);
              plog(pbuf, tch, 0);
              extract_obj(obj);
            }
          }
        }
      }
      if (IS_MOB(CAUSER_CH)) {
        safe_snprintf(debugbuf, sizeof(debugbuf), "MOB CAST: <%s '%s' %s> dam = %d", GET_MOB_NAME(CAUSER_CH), sinfo->command, GET_NAME(tch), dam);
        mudlog(debugbuf, 'D', COM_IMMORT, TRUE);
      }
      damage(CAUSER_CH, tch, dam, sinfo->spellindex, 0, sinfo->resist_type, TRUE, info2 ? 1 : 0);
    }
  }
}

EVENT(spell_destroy_equipment_event)
{
  struct char_data *tch, *next_tch;
  char pbuf[256];
  int eq;
  int dam;

  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);

  dam = getSpellDam(sinfo, CAUSER_CH);
  if (!IS_MOB(CAUSER_CH)) {
    dam = modBySpecialization(CAUSER_CH, sinfo, dam);
    dam = dam * GET_SKILL(CAUSER_CH, sinfo->realm) / 110;
  }
  if (!info2) {
    send_to_char("You complete your spell.\r\n", CAUSER_CH);
    say_spell(CAUSER_CH, sinfo->spellindex, NULL, NULL);
  }

  for (tch = world[CAUSER_CH->in_room].people; tch; tch = next_tch) {
    next_tch = tch->next_in_room;

    if (tch == CAUSER_CH) {
      continue;
    }
    if (IS_MOB(tch) && MOB_FLAGGED(tch, MOB_NOKILL)) {
      continue;
    }
    if (!IS_MOB(tch) && GET_LEVEL(tch) >= LVL_IMMORT) {
      continue;
    }
    if (IS_MOB(tch) && IS_AFFECTED(tch, AFF_SUPERINV)) {
      continue;
    }
    if (!ROOM_FLAGGED(IN_ROOM(CAUSER_CH), ROOM_CRIMEOK) && (!pk_allowed && !IS_MOB(CAUSER_CH) && !IS_MOB(tch))) {
      continue;
    }
    if (!ROOM_FLAGGED(IN_ROOM(CAUSER_CH), ROOM_CRIMEOK) && (!IS_MOB(CAUSER_CH) && IS_MOB(tch) && IS_AFFECTED(tch, AFF_CHARM))) {
      continue;
    }
    if (AFF2_FLAGGED(tch, AFF2_PROT_ACID) && strcmp(sinfo->command, "acid breath") == 0) {
      continue;
    }

    if (tch) {
      if (mag_savingthrow(tch, sinfo->saving_throw)) {
        dam >>= 1;
      }
      if (number(0, 61) < GET_LEVEL(tch)) {
        if (!mag_savingthrow(tch, sinfo->saving_throw)) {
          for (eq = 0; eq < NUM_WEARS; eq++) {
            if (!GET_EQ(tch, eq))
              continue;
            if (IS_OBJ_STAT(GET_EQ(tch, eq), ITEM_NOBURN))
              continue;
            if (GET_OBJ_TYPE(GET_EQ(tch, eq)) != ITEM_ARMOR)
              continue;
            if (number(0, 1)) {
              if (strcmp(sinfo->command, "acid breath") == 0) {
                act("$p{Y dissolves!{x", FALSE, tch, GET_EQ(tch, eq), NULL, TO_CHAR);
              }
              safe_snprintf(pbuf, sizeof(pbuf), "%s lost item %s (dragonbreath)", GET_NAME(tch), GET_EQ(tch, eq)->short_description);
              mudlog(pbuf, 'Q', COM_IMMORT, TRUE);
              plog(pbuf, tch, 0);
              extract_obj(unequip_char(tch, eq));
            }
          }
        }
      }
      if (IS_MOB(CAUSER_CH)) {
        safe_snprintf(debugbuf, sizeof(debugbuf), "MOB CAST: <%s '%s'> dam = %d", GET_MOB_NAME(CAUSER_CH), sinfo->command, dam);
        mudlog(debugbuf, 'D', COM_IMMORT, TRUE);
      }
      damage(CAUSER_CH, tch, dam, sinfo->spellindex, 0, sinfo->resist_type, TRUE, info2 ? 1 : 0);
    }
  }
}

EVENT(spell_add_dam_event)
{
  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);
}

EVENT(spell_prismatic_spray_event)
{
  struct char_data *tch;
  struct char_data *tch2;
  int dam, t, t2, t3, t4;
  struct spell_info_type *sinfo2;

  REMOVE_BIT(AFF2_FLAGS(CAUSER_CH), AFF2_CASTING);

  if (!info2) {
    send_to_char("You complete your spell.\r\n", CAUSER_CH);
    say_spell(CAUSER_CH, sinfo->spellindex, NULL, NULL);
  }

  for (tch = world[CAUSER_CH->in_room].people; tch; tch = tch2) {
    tch2 = tch->next_in_room;
    if (tch == CAUSER_CH) {
      continue;
    }
    if (IS_MOB(tch) && MOB_FLAGGED(tch, MOB_NOKILL)) {
      continue;
    }
    if (IS_MOB(tch) && AFF_FLAGGED(tch, AFF_SUPERINV)) {
      continue;
    }
    if (!ROOM_FLAGGED(IN_ROOM(CAUSER_CH), ROOM_CRIMEOK) && (!pk_allowed && !IS_MOB(CAUSER_CH) && !IS_MOB(tch))) {
      continue;
    }
    if (!ROOM_FLAGGED(IN_ROOM(CAUSER_CH), ROOM_CRIMEOK) && (!IS_MOB(CAUSER_CH) && IS_MOB(tch) && IS_AFFECTED(tch, AFF_CHARM) && GET_RESIST(tch, sinfo->resist_type) < 101)) {
      continue;
    }
    t4 = number(2, 5) / 2;
    t3 = 10;
    t = 10;
    for (t2 = 0; t2 < t4; t2++) {
      if (tch && (tch->in_room == CAUSER_CH->in_room)) {
        while (t == t3) {
          t = number(0, 7);
        }
        t3 = t;
        switch (t3) {
          case 0:
            /* red */
            dam = 420;
            dam = modBySpecialization(CAUSER_CH, sinfo, dam);
            dam = dam * GET_SKILL(CAUSER_CH, sinfo->realm) / 110;
            if (mag_savingthrow(tch, sinfo->saving_throw)) {
              dam >>= 1;
            }
            damage(CAUSER_CH, tch, dam, sinfo->spellindex, 0, sinfo->resist_type, TRUE, info2 ? 1 : 0);
            act("You send a {RRED{x beam of light at $N.", TRUE, CAUSER_CH, NULL, tch, TO_CHAR);
            act("$N is bathed in a {RRED{x beam of light.", TRUE, CAUSER_CH, NULL, tch, TO_ROOM);
            act("You are bathed in a {RRed{x beam of light.", TRUE, CAUSER_CH, NULL, tch, TO_VICT);
            break;
          case 1:
            /* orange */
            dam = 280;
            dam = modBySpecialization(CAUSER_CH, sinfo, dam);
            dam = dam * GET_SKILL(CAUSER_CH, sinfo->realm) / 110;
            if (mag_savingthrow(tch, sinfo->saving_throw)) {
              dam >>= 1;
            }
            damage(CAUSER_CH, tch, dam, sinfo->spellindex, 0, sinfo->resist_type, TRUE, info2 ? 1 : 0);
            act("You send a {yORANGE{x beam of light at $N.", TRUE, CAUSER_CH, NULL, tch, TO_CHAR);
            act("$N is bathed in a {yORANGE{x beam of light.", TRUE, CAUSER_CH, NULL, tch, TO_ROOM);
            act("You are bathed in a {yORANGE{x beam of light.", TRUE, CAUSER_CH, NULL, tch, TO_VICT);
            break;
          case 2:
            /* yellow */
            dam = 140;
            dam = modBySpecialization(CAUSER_CH, sinfo, dam);
            dam = dam * GET_SKILL(CAUSER_CH, sinfo->realm) / 110;
            if (mag_savingthrow(tch, sinfo->saving_throw)) {
              dam >>= 1;
            }
            damage(CAUSER_CH, tch, dam, sinfo->spellindex, 0, sinfo->resist_type, TRUE, info2 ? 1 : 0);
            act("You send a {YYELLOW{x beam of light at $N.", TRUE, CAUSER_CH, NULL, tch, TO_CHAR);
            act("$N is bathed in a {YYELLOW{x beam of light.", TRUE, CAUSER_CH, NULL, tch, TO_ROOM);
            act("You are bathed in a {YYELLOW{x beam of light.", TRUE, CAUSER_CH, NULL, tch, TO_VICT);
            break;
          case 3:
            /* blue */
            /* major para */
            sinfo2 = (spells + find_spell_num("major paralysis"));
            act("You send a {BBLUE{x beam of light at $N.", TRUE, CAUSER_CH, NULL, tch, TO_CHAR);
            act("$N is bathed in a {BBLUE{x beam of light.", TRUE, CAUSER_CH, NULL, tch, TO_ROOM);
            act("You are bathed in a {BBLUE{x beam of light.", TRUE, CAUSER_CH, NULL, tch, TO_VICT);
            mag_affect_char(sinfo2, 0, CAUSER_CH, tch, GET_LEVEL(CAUSER_CH));
            break;
          case 4:
            /* indigo */
            /* feeblemind */
            sinfo2 = (spells + find_spell_num("feeble mind"));
            act("You send a {bINDIGO{x beam of light at $N.", TRUE, CAUSER_CH, NULL, tch, TO_CHAR);
            act("$N is bathed in a {bINDIGO{x beam of light.", TRUE, CAUSER_CH, NULL, tch, TO_ROOM);
            act("You are bathed in a {bINDIGO{x beam of light.", TRUE, CAUSER_CH, NULL, tch, TO_VICT);
            mag_affect_char(sinfo2, 0, CAUSER_CH, tch, GET_LEVEL(CAUSER_CH));
            break;
          case 5:
            /* green */
            /* poison */
            sinfo2 = (spells + find_spell_num("poison"));
            act("You send a {GGREEN{x beam of light at $N.", TRUE, CAUSER_CH, NULL, tch, TO_CHAR);
            act("$N is bathed in a {GGREEN{x beam of light.", TRUE, CAUSER_CH, NULL, tch, TO_ROOM);
            act("You are bathed in a {GGREEN{x beam of light.", TRUE, CAUSER_CH, NULL, tch, TO_VICT);
            mag_affect_char(sinfo2, 0, CAUSER_CH, tch, GET_LEVEL(CAUSER_CH));
            break;
          case 6:
            /* violet */
            /* dispel magic */
            sinfo2 = (spells + find_spell_num("dispel magic"));
            act("You send a {MVIOLET{x beam of light at $N.", TRUE, CAUSER_CH, NULL, tch, TO_CHAR);
            act("$N is bathed in a {MVIOLET{x beam of light.", TRUE, CAUSER_CH, NULL, tch, TO_ROOM);
            act("You are bathed in a {MVIOLET{x beam of light.", TRUE, CAUSER_CH, NULL, tch, TO_VICT);
            spell_dispel_magic_event(CAUSER_CH, tch, info, sinfo2, 1);
            break;
          case 7:
            /* azure */
            /* blind */
            sinfo2 = (spells + find_spell_num("blindness"));
            act("You send a {CAZURE{x beam of light at $N.", TRUE, CAUSER_CH, NULL, tch, TO_CHAR);
            act("$N is bathed in a {CAZURE{x beam of light.", TRUE, CAUSER_CH, NULL, tch, TO_ROOM);
            act("You are bathed in a {CAZURE{x beam of light.", TRUE, CAUSER_CH, NULL, tch, TO_VICT);
            mag_affect_char(sinfo2, 0, CAUSER_CH, tch, GET_LEVEL(CAUSER_CH));
            break;
        }
        if (IS_NPC(tch) && tch->master != CAUSER_CH && sinfo->aggressive && number(0, 1)) {
          hit(tch, CAUSER_CH, TYPE_UNDEFINED);
          FIGHT_STATE(CAUSER_CH, 2);
        }
      }
    }
  }
}
