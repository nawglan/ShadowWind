/*
 * newmagic.c Written by Fred Merkel Part of JediMUD Copyright (C) 1993
 * Trustees of The Johns Hopkins Unversity All Rights Reserved. Based on
 * DikuMUD, Copyright (C) 1990, 1991.
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
#include "interpreter.h"

extern struct room_data *world;
extern struct zone_data *zone_table;
extern struct obj_data *object_list;
extern struct char_data *character_list;
extern sh_int stats[11][101];
extern struct index_data *obj_index;

extern struct weather_data weather_info;
extern struct descriptor_data *descriptor_list;
extern struct zone_data *zone_table;

extern int mini_mud;
extern int pk_allowed;

extern struct default_mobile_stats *mob_defaults;
extern char weapon_verbs[];
extern int *max_ac_applys;
extern struct apply_mod_defaults *apmd;

void clearMemory(struct char_data * ch);
void act(char *str, int i, struct char_data * c, struct obj_data * o, void *vict_obj, int j);

void improve_skill(struct char_data *ch, int skill, int chance);

void weight_change_object(struct obj_data * obj, int weight);
void add_follower(struct char_data * ch, struct char_data * leader);
int mag_savingthrow(struct char_data * ch, int type);
char *get_spell_name(int spellnum);

int magic_ok(struct char_data *ch, int aggressive)
{
  if (!IS_NPC(ch) && COM_FLAGGED(ch, COM_IMMORT))
    return 1;

  if (ROOM_FLAGGED(ch->in_room, ROOM_NOMAGIC)) {
    send_to_char("Your magic fizzles out and dies.\r\n", ch);
    act("$n's magic fizzles out and dies.", FALSE, ch, 0, 0, TO_ROOM);
    return 0;
  }
  if (IS_AFFECTED(ch, AFF_SILENCE)) {
    send_to_char("You are compeled to magical silence.\r\n", ch);
    return 0;
  }
  if (IS_SET(ROOM_FLAGS(ch->in_room), ROOM_PEACEFUL) && aggressive) {
    send_to_char("A flash of white light fills the room, dispelling your "
        "violent magic!\r\n", ch);
    act("White light from no particular source suddenly fills the room, "
        "then vanishes.", FALSE, ch, 0, 0, TO_ROOM);
    return 0;
  }

  return 1;
}

/* check if a player is allowed to access a certain room */
int check_teleport(struct char_data *ch, int room)
{
  if (IS_NPC(ch))
    return 1;

  if (IS_SET(zone_table[world[room].zone].bits, ZONE_NOTELEPORT) || ROOM_FLAGGED(room, ROOM_NOTELEPORT) || ROOM_FLAGGED(room, ROOM_NOMAGIC) || ROOM_FLAGGED(room, ROOM_GODROOM) || ROOM_FLAGGED(room, ROOM_PRIVATE))
    return 0;

  return 1;
}

int check_ch_ability(struct char_data *ch, struct spell_info_type *sinfo, int isobj, char *target, int waitstate)
{
  int bonus = 0;

  bonus = (IS_MAGE(ch) ? GET_INT(ch) : GET_WIS(ch));

  SET_BIT(AFF2_FLAGS(ch), AFF2_CASTING);
  if (!isobj && !(!IS_NPC(ch) && COM_FLAGGED(ch, COM_IMMORT)) && GET_CIRCLE_DIFF(ch, sinfo) == 0) {
    if ((GET_SKILL(ch, sinfo->realm) + bonus) > number(1, 175)) {
      add_event(waitstate, sinfo->event_pointer, EVENT_SPELL, ch, NULL, str_dup(target), sinfo, sinfo->command, (void *) isobj);
      return 1;
    } else {
      add_event(1, fail_spell_event, EVENT_SPELL, ch, NULL, str_dup(target), NULL, NULL, NULL);
    }
  } else {
    add_event(waitstate, sinfo->event_pointer, EVENT_SPELL, ch, NULL, str_dup(target), sinfo, sinfo->command, (void *) isobj);
    return 1;
  }

  return 0;
}

int check_mob_ability(struct char_data *ch, struct spell_info_type *sinfo, int isobj, char *target, int waitstate)
{
  SET_BIT(AFF2_FLAGS(ch), AFF2_CASTING);
  if (!isobj) {
    if (GET_LEVEL(ch) > number(1, 61)) {
      add_event(waitstate, sinfo->event_pointer, EVENT_SPELL, ch, NULL, str_dup(target), sinfo, sinfo->command, (void *) isobj);
      GET_MOB_WAIT(ch) = waitstate * 10;
      return 1;
    } else {
      add_event(1, fail_spell_event, EVENT_SPELL, ch, NULL, str_dup(target), NULL, NULL, NULL);
    }
  } else {
    add_event(waitstate, sinfo->event_pointer, EVENT_SPELL, ch, NULL, str_dup(target), sinfo, sinfo->command, (void *) isobj);
    GET_MOB_WAIT(ch) = waitstate * 10;
    return 1;
  }

  return 0;
}

/*
 * Special spells appear below.
 */

ASPELL(spell_dam)
{
  char *target = NULL;
  if (!isobj) {
    if (IS_CASTING(ch)) {
      act("$n stops casting.", TRUE, ch, 0, 0, TO_ROOM);
      act("You're already casting something.", TRUE, ch, 0, 0, TO_CHAR);
      return;
    }
    if (arg && !get_char_room_vis(ch, arg)) {
      act("$n stops casting.", TRUE, ch, 0, 0, TO_ROOM);
      act("Cast on who?", TRUE, ch, 0, 0, TO_CHAR);
      return;
    }
  }
  if (arg)
    target = strdup(arg);
  if (magic_ok(ch, sinfo->aggressive)) {
    if (!IS_NPC(ch))
      check_ch_ability(ch, sinfo, isobj, target, waitstate);
    else
      check_mob_ability(ch, sinfo, isobj, target, waitstate);
    if (!IS_NPC(ch) && !isobj)
      improve_skill(ch, sinfo->realm, SKUSE_FREQUENT);
  }
}

ASPELL(spell_char)
{
  event *tempevent;
  if (!isobj) {
    if (IS_CASTING(ch)) {
      act("$n stops casting.", TRUE, ch, 0, 0, TO_ROOM);
      act("You're already casting something.", TRUE, ch, 0, 0, TO_CHAR);
      return;
    }
    tempevent = sinfo->event_pointer;
    if (arg && !get_char_vis(ch, arg) && tempevent != spell_group_event) {
      act("$n stops casting.", TRUE, ch, 0, 0, TO_ROOM);
      act("Cast on who?", TRUE, ch, 0, 0, TO_CHAR);
      return;
    }
  }
  if (magic_ok(ch, sinfo->aggressive)) {
    if (!IS_NPC(ch))
      check_ch_ability(ch, sinfo, isobj, arg, waitstate);
    else
      check_mob_ability(ch, sinfo, isobj, arg, waitstate);
    if (!IS_NPC(ch) && !isobj)
      improve_skill(ch, sinfo->realm, SKUSE_FREQUENT);
  }
}

ASPELL(spell_general)
{
  if (!isobj) {
    if (IS_CASTING(ch)) {
      act("$n stops casting.", TRUE, ch, 0, 0, TO_ROOM);
      act("You're already casting something.", TRUE, ch, 0, 0, TO_CHAR);
      return;
    }
  }
  if (magic_ok(ch, sinfo->aggressive)) {
    if (!IS_NPC(ch))
      check_ch_ability(ch, sinfo, isobj, arg, waitstate);
    else
      check_mob_ability(ch, sinfo, isobj, arg, waitstate);
    if (!IS_NPC(ch) && !isobj)
      improve_skill(ch, sinfo->realm, SKUSE_FREQUENT);
  }
}

ASPELL(spell_obj_char)
{
  if (!isobj) {
    if (IS_CASTING(ch)) {
      act("$n stops casting.", TRUE, ch, 0, 0, TO_ROOM);
      act("You're already casting something.", TRUE, ch, 0, 0, TO_CHAR);
      return;
    }
    if (!arg && (!get_char_room_vis(ch, arg) && !get_obj_in_list_vis(ch, arg, ch->carrying) && !get_obj_in_list_vis(ch, arg, world[ch->in_room].contents))) {
      act("$n stops casting.", TRUE, ch, 0, 0, TO_ROOM);
      act("Cast on who or what?", TRUE, ch, 0, 0, TO_CHAR);
      return;
    }
  }
  if (magic_ok(ch, sinfo->aggressive)) {
    if (!IS_NPC(ch))
      check_ch_ability(ch, sinfo, isobj, arg, waitstate);
    else
      check_mob_ability(ch, sinfo, isobj, arg, waitstate);
    if (!IS_NPC(ch) && !isobj)
      improve_skill(ch, sinfo->realm, SKUSE_FREQUENT);
  }
}

ASPELL(spell_obj)
{
  if (!isobj) {
    if (IS_CASTING(ch)) {
      act("$n stops casting.", TRUE, ch, 0, 0, TO_ROOM);
      act("You're already casting something.", TRUE, ch, 0, 0, TO_CHAR);
      return;
    }
    if (!arg || (arg && (!get_obj_in_list_vis(ch, arg, ch->carrying) && !get_obj_in_list_vis(ch, arg, world[ch->in_room].contents)))) {
      act("$n stops casting.", TRUE, ch, 0, 0, TO_ROOM);
      act("Cast on what?", TRUE, ch, 0, 0, TO_CHAR);
      return;
    }
  }
  if (magic_ok(ch, sinfo->aggressive)) {
    if (!IS_NPC(ch))
      check_ch_ability(ch, sinfo, isobj, arg, waitstate);
    else
      check_mob_ability(ch, sinfo, isobj, arg, waitstate);
    if (!IS_NPC(ch) && !isobj)
      improve_skill(ch, sinfo->realm, SKUSE_FREQUENT);
  }
}

ASPELL(spell_obj_room)
{
  if (!isobj) {
    if (IS_CASTING(ch)) {
      act("$n stops casting.", TRUE, ch, 0, 0, TO_ROOM);
      act("You're already casting something.", TRUE, ch, 0, 0, TO_CHAR);
      return;
    }
    if (arg && (!get_obj_in_list_vis(ch, arg, ch->carrying) && !get_obj_in_list_vis(ch, arg, world[ch->in_room].contents))) {
      act("$n stops casting.", TRUE, ch, 0, 0, TO_ROOM);
      act("Cast on what?", TRUE, ch, 0, 0, TO_CHAR);
      return;
    }
  }
  if (magic_ok(ch, sinfo->aggressive)) {
    if (!IS_NPC(ch))
      check_ch_ability(ch, sinfo, isobj, arg, waitstate);
    else
      check_mob_ability(ch, sinfo, isobj, arg, waitstate);
    if (!IS_NPC(ch) && !isobj)
      improve_skill(ch, sinfo->realm, SKUSE_FREQUENT);
  }
}
