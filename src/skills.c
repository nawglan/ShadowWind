#include <stdio.h>
#include <string.h>
#include "structs.h"
#include "comm.h"
#include "interpreter.h"
#include "utils.h"
#include "event.h"
#include "handler.h"
#include "spells.h"
#include "db.h"

extern sh_int stats[11][101];
extern char *dirs[];
extern int rev_dir[];
extern int pk_allowed;
extern struct room_data *world;
extern struct spell_info_type *spells;
extern struct index_data *mob_index; /* index table for mobile file         */
extern struct zone_data *zone_table; /* zone table                         */
int find_skill_num_def(int define);
void improve_skill(struct char_data *ch, int skill, int chance);
int find_door(struct char_data * ch, char *type, char *dir);
SPECIAL(shop_keeper);
extern int max_lvl_skill[2][4][52];

/* code for skill improvement through use */
void improve_skill(struct char_data *ch, int skill, int chance)
{
  int percent = GET_SKILL(ch, skill);
  int newpercent, max;
  char skillbuf[MAX_STRING_LENGTH];
  char mybuf[256];

  max = max_lvl_skill[1][spells[find_skill_num_def(skill)].difficulty][(int) GET_LEVEL(ch)];
  if (percent >= max || IS_NPC(ch))
    return;
  if (number(1, (chance * 50)) > (GET_WIS(ch) + GET_INT(ch)))
    return;
  newpercent = 1;
  if (number(1, 120) <= GET_WIS(ch))
    newpercent++;
  if (number(1, 120) <= GET_INT(ch))
    newpercent++;
  percent += newpercent;
  percent = MIN(percent, max);
  SET_SKILL(ch, skill, percent);
  if (newpercent) {
    sprintf(mybuf, "SKILLIMPROVE: %s improved skill %s, int = %d, wis = %d, improved by = %d, now = %d", GET_NAME(ch), spells[find_skill_num_def(skill)].command, GET_INT(ch), GET_WIS(ch), newpercent, percent);
    mudlog(mybuf, 'D', COM_IMMORT, TRUE);
    sprintf(skillbuf, "{RYou feel your skill in {W%s {Rimproving.{x\r\n", spells[find_skill_num_def(skill)].command);
    send_to_char(skillbuf, ch);
  }
}

ACMD(do_disarm)
{
  struct obj_data *obj;
  struct char_data *vict;
  int percent, prob, secprob;
  int skillnum = spells[find_skill_num("disarm")].spellindex;

  one_argument(argument, arg);

  if (IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("You had better leave this skill to the players.\r\n", ch);
    return;
  }
  if (!IS_NPC(ch) && !GET_SKILL(ch, skillnum)) {
    send_to_char("You dont know of that skill.\r\n", ch);
    return;
  }
  if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
    send_to_char("You feel ashamed disturbing the tranquility of this place\r\n", ch);
    return;
  }
  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Disarm who?\r\n", ch);
      return;
    }
  }
  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  }

  if (!vict->equipment[WEAR_WIELD]) {
    send_to_char("Disarm what weapon?!\r\n", ch);
    return;
  }

  if ((CHECK_FIGHT(ch) != 0) && FIGHTING(ch)) {
    send_to_char("You are too engaged in combat to disarm now!\r\n", ch);
    return;
  }

  percent = number(1, 101); /* 101% is a complete failure */
  if (IS_NPC(ch))
    prob = GET_LEVEL(ch);
  else
    prob = GET_SKILL(ch, skillnum); /* very low success probability */

  secprob = 30 - (GET_LEVEL(vict) - GET_LEVEL(ch));

  if ((percent > prob || number(1, 101) > secprob) && GET_POS(vict) > POS_SLEEPING) {
    act("$n tries to disarm $N but fails.", TRUE, ch, 0, vict, TO_NOTVICT);
    act("You try to disarm $N but fail.", TRUE, ch, 0, vict, TO_CHAR);
    act("$n tries to disarm you but fails.", TRUE, ch, 0, vict, TO_VICT);
    if (!FIGHTING(vict))
      hit(vict, ch, TYPE_UNDEFINED);
  } else {
    act("$n makes $N drop $S weapon to the ground with some fast moves.", TRUE, ch, 0, vict, TO_NOTVICT);
    act("With some fast moves you manage to make $N drop the weapon.", TRUE, ch, 0, vict, TO_CHAR);
    act("$n performs some fast moves, and makes you drop your weapon.", TRUE, ch, 0, vict, TO_VICT);
    obj = unequip_char(vict, WEAR_WIELD);
    obj_to_room(obj, vict->in_room);
    if (!FIGHTING(vict))
      hit(vict, ch, TYPE_UNDEFINED);
  }
  improve_skill(ch, skillnum, SKUSE_AVERAGE);
  FIGHT_STATE(ch, 3);
}

ACMD(do_target)
{
  int percent, prob;
  struct char_data *vict;
  int skillnum = spells[find_skill_num("switch target")].spellindex;

  one_argument(argument, arg);

  if (IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("You had better leave this skill to the players.\r\n", ch);
    return;
  }
  if (!IS_NPC(ch) && !GET_SKILL(ch, skillnum)) {
    send_to_char("You dont know of that skill.\r\n", ch);
    return;
  }

  if (!FIGHTING(ch)) {
    send_to_char("You can't switch targets if your not fighting.\r\n", ch);
    return;
  }

  if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
    send_to_char("You feel ashamed disturbing the tranquility of this place\r\n", ch);
    return;
  }

  if ((CHECK_FIGHT(ch) != 0) && FIGHTING(ch)) {
    send_to_char("You can't get into a position to switch targets right now!\r\n", ch);
    return;
  }

  percent = number(1, 101); /* 101% is a complete failure */
  if (IS_NPC(ch))
    prob = GET_LEVEL(ch);
  else
    prob = GET_SKILL(ch, skillnum); /* very low success probability */

  if (percent > prob) {
    act("You try to switch targets, but your current opponent keeps you busy.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n attempts to move in a position to target another opponent.", FALSE, ch, 0, 0, TO_ROOM);
    FIGHT_STATE(ch, 3);
    improve_skill(ch, skillnum, SKUSE_AVERAGE);
    return;
  }

  if (!*arg)
    send_to_char("Target who?\r\n", ch);
  else if (!(vict = get_char_room_vis(ch, arg)))
    send_to_char("They don't seem to be here.\r\n", ch);
  else if (vict == ch) {
    send_to_char("You hit yourself...OUCH!.\r\n", ch);
    act("$n hits $mself, and says OUCH!", FALSE, ch, 0, vict, TO_ROOM);
  } else if (IS_AFFECTED(ch, AFF_CHARM) && (ch->master == vict))
    act("$N is just such a good friend, you simply can't hit $M.", FALSE, ch, 0, vict, TO_CHAR);
  else {
    if (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_NOKILL)) {
      send_to_char("That's not a good idea.\r\n", ch);
      return;
    }
    if (!pk_allowed) {
      if (!IS_NPC(vict) && !IS_NPC(ch) && (subcmd != SCMD_MURDER)) {
        send_to_char("Use 'murder' to hit another player.\r\n", ch);
        return;
      }
      if (IS_AFFECTED(ch, AFF_CHARM) && !IS_NPC(ch->master) && !IS_NPC(vict))
        return; /* you can't order a charmed pet to attack a
         * player */
    }
    if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
      send_to_char("You feel ashamed disturbing the tranquility of this place\r\n", ch);
      return;
    }
    act("You {Wswitches targets{w and now is fighting $N!{x", FALSE, ch, 0, vict, TO_CHAR);
    act("$n {Wswitches targets{w and now is fighting $N!{x", FALSE, ch, 0, vict, TO_NOTVICT);
    act("$n {Wswitches targets{w and now is fighting you!{x", FALSE, ch, 0, vict, TO_VICT);
    hit(ch, vict, TYPE_UNDEFINED);
    FIGHT_STATE(ch, 2);
    WAIT_STATE(ch, 1*PULSE_VIOLENCE);
    improve_skill(ch, skillnum, SKUSE_AVERAGE);
  }

}

ACMD(do_backstab)
{
  struct char_data *vict;
  byte percent, prob;
  int skillnum = spells[find_skill_num("backstab")].spellindex;

  one_argument(argument, buf);

  if (IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("You had better leave this skill to the players.\r\n", ch);
    return;
  }
  if (!(vict = get_char_room_vis(ch, buf))) {
    if (subcmd == SCMD_CIRCLE && FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Backstab who?\r\n", ch);
      return;
    }
  }
  if (!IS_NPC(ch) && (subcmd == SCMD_CIRCLE) && !GET_SKILL(ch, skillnum)) {
    send_to_char("You don't know that skill.\r\n", ch);
    return;
  }
  if (subcmd == SCMD_CIRCLE && vict != FIGHTING(ch)) {
    send_to_char("You can only circle those you are fighting.\r\n", ch);
    return;
  }
  if (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_NOKILL)) {
    send_to_char("That's not a good idea.\r\n", ch);
    return;
  }
  if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
    send_to_char("You feel ashamed disturbing the tranquility of this place\r\n", ch);
    return;
  }
  if (vict == ch) {
    send_to_char("How can you sneak up on yourself?\r\n", ch);
    return;
  } else if (IS_AFFECTED(ch, AFF_CHARM) && !IS_NPC(ch->master) && !IS_NPC(vict)) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  }
  if (!ch->equipment[WEAR_WIELD] && !ch->equipment[WEAR_WIELD_2]) {
    send_to_char("You need to wield a weapon to make it a success.\r\n", ch);
    return;
  }
  if ((GET_EQ(ch, WEAR_WIELD) && GET_OBJ_VAL(ch->equipment[WEAR_WIELD], 3) != WEAPON_SHORTSWORD && GET_OBJ_VAL(ch->equipment[WEAR_WIELD], 3) != WEAPON_DAGGER) && (GET_EQ(ch, WEAR_WIELD_2) && GET_OBJ_VAL(ch->equipment[WEAR_WIELD_2], 3) != WEAPON_SHORTSWORD && GET_OBJ_VAL(ch->equipment[WEAR_WIELD_2], 3) != WEAPON_DAGGER)) {
    send_to_char("Only piercing weapons can be used for backstabbing.\r\n", ch);
    return;
  }
  if (IS_SET(MOB_FLAGS(vict), MOB_AWARE)) {
    act("$N is watching your every move, your are unable to find an opportunity to backstab.", TRUE, ch, 0, vict, TO_CHAR);
    act("$N starts eyeing $n's every move, as $e attempts to be sneaky.", TRUE, ch, 0, vict, TO_ROOM);
    return;
  }
  if (!IS_NPC(ch) && SKILL_TIMER(ch) && vict == FIGHTING(ch)) {
    send_to_char("You're not able to move into position yet.\r\n", ch);
    return;
  }
  percent = number(1, 101); /* 101% is a complete failure */
  if (IS_NPC(ch))
    prob = GET_LEVEL(ch);
  else
    prob = GET_SKILL(ch, skillnum);

  if (FIGHTING(vict) && subcmd == SCMD_CIRCLE) {
    if (AWAKE(vict) && (percent > (prob >> 1)))
      damage(ch, vict, 0, skillnum, 0, DAM_PIERCE, 0, 0);
    else
      hit(ch, vict, skillnum);
    WAIT_STATE(ch, 1*PULSE_VIOLENCE);
  } else if ((FIGHTING(vict) && !FIGHTING(ch)) || !FIGHTING(vict)) {
    if (AWAKE(vict) && (percent > prob))
      damage(ch, vict, 0, skillnum, 0, DAM_PIERCE, 0, 0);
    else
      hit(ch, vict, skillnum);
    WAIT_STATE(ch, 2*PULSE_VIOLENCE);
  }
  improve_skill(ch, skillnum, SKUSE_AVERAGE);
  SKILL_TIMER(ch) = 3;
}

ACMD(do_bash)
{
  struct char_data *vict;
  int percent, prob;
  int skillnum = spells[find_skill_num("bash")].spellindex;

  one_argument(argument, arg);

  if (IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("You had better leave this skill to the players.\r\n", ch);
    return;
  }
  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_WRAITHLIKE)) {
    send_to_char("You're not solid leave this skill to solids.\r\n", ch);
    return;
  }

  if (!IS_NPC(ch) && !GET_SKILL(ch, skillnum)) {
    send_to_char("You dont know of that skill.\r\n", ch);
    return;
  }
  if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
    send_to_char("You feel ashamed disturbing the tranquility of this place\r\n", ch);
    return;
  }

  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Bash who?\r\n", ch);
      return;
    }
  }
  if (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_NOKILL)) {
    send_to_char("That's not a good idea.\r\n", ch);
    return;
  }
  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  } else if (IS_AFFECTED(ch, AFF_CHARM) && !IS_NPC(ch->master) && !IS_NPC(vict)) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  }
  if (!IS_NPC(ch) && !ch->equipment[WEAR_SHIELD]) {
    send_to_char("You need to be using a shield to make it a success.\r\n", ch);
    return;
  }

  if ((CHECK_FIGHT(ch) != 0) && FIGHTING(ch)) {
    send_to_char("You aren't in a position to bash yet!\r\n", ch);
    return;
  }

  if (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_WRAITHLIKE)) {
    act("You send yourself sprawling through $N falling to the ground.", TRUE, ch, 0, vict, TO_CHAR);
    act("$n tries to bash you, but falls right through.", TRUE, ch, 0, vict, TO_VICT);
    act("$n sends $mself sprawling through $N, falling to the ground.", TRUE, ch, 0, vict, TO_ROOM);
    GET_POS(ch) = POS_RESTING;
    return;
  }
  if (IS_NPC(vict) && GET_MOB_SIZE(vict) == SIZE_GIANT && GET_RACE(ch) != RACE_OGRE) {
    act("You try to bash $N and bounce off of $M knocking yourself dizzy.", TRUE, ch, 0, vict, TO_CHAR);
    act("$n tries to bash $N and bounces off of $M knocking $mself dizzy.", TRUE, ch, 0, vict, TO_NOTVICT);
    GET_POS(ch) = POS_RESTING;
    return;
  }
  if (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_NOBASH)) {
    act("You try to bash $N but $E moves out of the way to quickly.", TRUE, ch, 0, vict, TO_CHAR);
    act("$n tries to bash you, but you move out of the way.", TRUE, ch, 0, vict, TO_VICT);
    act("$n tries to bash $N, but $E moves out of $s way.", TRUE, ch, 0, vict, TO_NOTVICT);
    GET_POS(ch) = POS_RESTING;
    return;
  }
  percent = number(1, 160); /* 101% is a complete failure */
  if (IS_NPC(ch))
    prob = GET_LEVEL(ch);
  else
    prob = GET_SKILL(ch, skillnum);

  if (percent > prob) {
    damage(ch, vict, 0, skillnum, 0, DAM_BLUDGEON, 0, 0);
    GET_POS(ch) = POS_RESTING;
    if (IS_NPC(ch))
      GET_MOB_WAIT(ch) = 2;
    else
      WAIT_STATE(ch, 2*PULSE_VIOLENCE);
  } else {
    FIGHT_STATE(vict, 3);
    if (IS_NPC(vict))
      GET_MOB_WAIT(vict) = 2;
    else
      WAIT_STATE(vict, 2*PULSE_VIOLENCE);
    GET_POS(vict) = POS_RESTING;
    damage(ch, vict, 1, skillnum, 0, DAM_BLUDGEON, 0, 0);
  }
  improve_skill(ch, skillnum, SKUSE_AVERAGE);
  FIGHT_STATE(ch, 2);
}

ACMD(do_rescue)
{
  struct char_data *vict, *tmp_ch;
  byte percent, prob;
  int skillnum = spells[find_skill_num("rescue")].spellindex;

  one_argument(argument, arg);

  if (!IS_NPC(ch) && !GET_SKILL(ch, skillnum)) {
    send_to_char("You know not of that skill.\r\n", ch);
    return;
  }
  if (!(vict = get_char_room_vis(ch, arg))) {
    send_to_char("Who do you want to rescue?\r\n", ch);
    return;
  }
  if (vict == ch) {
    send_to_char("What about fleeing instead?\r\n", ch);
    return;
  }
  if (FIGHTING(ch) == vict) {
    send_to_char("How can you rescue someone you are trying to kill?\r\n", ch);
    return;
  }
  for (tmp_ch = world[ch->in_room].people; tmp_ch && (FIGHTING(tmp_ch) != vict); tmp_ch = tmp_ch->next_in_room)
    ;

  if (!tmp_ch) {
    act("But nobody is fighting $M!", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }

  if (IS_NPC(ch) && !IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("You'd better leave this skill to players.\r\n", ch);
    return;
  }

  percent = number(1, 101); /* 101% is a complete failure */
  if (IS_NPC(ch))
    prob = GET_LEVEL(ch);
  else
    prob = GET_SKILL(ch, skillnum);

  if ((CHECK_FIGHT(ch) != 0) && FIGHTING(ch)) {
    send_to_char("You are too engaged in combat to rescue now!\r\n", ch);
    return;
  }

  if (percent > prob) {
    send_to_char("You fail the rescue!\r\n", ch);
    improve_skill(ch, skillnum, SKUSE_AVERAGE);
    return;
  }
  send_to_char("{WBanzai!  {CTo the rescue...{x\r\n", ch);
  act("{WYou are rescued by {G$N{W!{x", FALSE, vict, 0, ch, TO_CHAR);
  act("{W$n {wheroically rescues {W$N{w!{x", FALSE, ch, 0, vict, TO_NOTVICT);

  if (FIGHTING(vict) == tmp_ch)
    stop_fighting(vict);
  if (FIGHTING(tmp_ch))
    stop_fighting(tmp_ch);
  if (FIGHTING(ch))
    stop_fighting(ch);

  set_fighting(ch, tmp_ch);
  set_fighting(tmp_ch, ch);
  improve_skill(ch, skillnum, SKUSE_AVERAGE);
  FIGHT_STATE(vict, 2);

}

ACMD(do_kick)
{
  struct char_data *vict;
  byte percent, prob;
  int skillnum = spells[find_skill_num("kick")].spellindex;

  if (IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("You had better leave this skill to the players.\r\n", ch);
    return;
  }
  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_WRAITHLIKE)) {
    send_to_char("You're not solid leave this skill to solids.\r\n", ch);
    return;
  }
  if (!IS_NPC(ch) && !GET_SKILL(ch, skillnum)) {
    send_to_char("You dont know of that skill.\r\n", ch);
    return;
  }
  if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
    send_to_char("You feel ashamed disturbing the tranquility of this place\r\n", ch);
    return;
  }

  one_argument(argument, arg);

  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Kick who?\r\n", ch);
      return;
    }
  }
  if (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_NOKILL)) {
    send_to_char("That's not a good idea.\r\n", ch);
    return;
  }
  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  } else if (IS_AFFECTED(ch, AFF_CHARM) && !IS_NPC(ch->master) && !IS_NPC(vict)) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  }

  if ((CHECK_FIGHT(ch) != 0) && FIGHTING(ch)) {
    send_to_char("You are too engaged in combat to kick now!\r\n", ch);
    return;
  }

  if (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_WRAITHLIKE)) {
    act("Your kick sails through $N.", TRUE, ch, 0, vict, TO_CHAR);
    act("$n tries to kick you, but passes right through.", TRUE, ch, 0, vict, TO_VICT);
    act("$n tries to kick $N, but passes through $e.", TRUE, ch, 0, vict, TO_ROOM);
    return;
  }
  percent = ((10 - (GET_AC(vict) / 10)) << 1) + number(1, 101); /* 101% is a complete */
  if (IS_NPC(ch))
    prob = GET_LEVEL(ch);
  else
    prob = GET_SKILL(ch, skillnum);

  if (percent > prob) {
    damage(ch, vict, 0, skillnum, 0, DAM_BLUDGEON, 0, 0);
  } else {
    damage(ch, vict, GET_LEVEL(ch) >> 1, skillnum, 0, DAM_BLUDGEON, 0, 0);
  }
  improve_skill(ch, skillnum, SKUSE_AVERAGE);
  FIGHT_STATE(ch, 2);
}

ACMD(do_pick)
{
  byte percent;
  int door, other_room;
  char type[MAX_INPUT_LENGTH], dir[MAX_INPUT_LENGTH];
  struct room_direction_data *back;
  struct obj_data *obj;
  struct char_data *v;
  int skillnum = spells[find_skill_num("pick lock")].spellindex;

  two_arguments(argument, type, dir);

  if (IS_NPC(ch))
    return;

  if (!GET_SKILL(ch, skillnum)) {
    if (!IS_THI(ch))
      send_to_char("You have a hard enough time WITH a key.\r\n", ch);
    else
      send_to_char("You know not of that skill.\r\n", ch);
    return;
  }

  percent = number(1, 101); /* 101% is a complete failure */

  if (!*type) {
    send_to_char("Pick what?\r\n", ch);
  } else if (generic_find(argument, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &v, &obj)) {
    /* this is an object */
    if (GET_OBJ_TYPE(obj) != ITEM_CONTAINER) {
      send_to_char("That's not a container.\r\n", ch);
    } else if (!IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) {
      send_to_char("Silly - it isn't even closed!\r\n", ch);
    } else if (GET_OBJ_VAL(obj, 2) < 0) {
      send_to_char("Odd - you can't seem to find a keyhole.\r\n", ch);
    } else if (!IS_SET(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) {
      send_to_char("Oho! This thing is NOT locked!\r\n", ch);
    } else if (IS_SET(GET_OBJ_VAL(obj, 1), CONT_PICKPROOF)) {
      send_to_char("It resists your attempts at picking it.\r\n", ch);
    } else if (percent > GET_SKILL(ch, skillnum)) {
      send_to_char("You failed to pick the lock.\r\n", ch);
    } else {
      REMOVE_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED);
      send_to_char("*click*\r\n", ch);
      improve_skill(ch, skillnum, SKUSE_AVERAGE);
      act("$n fiddles with $p.", FALSE, ch, obj, 0, TO_ROOM);
    }
  } else if ((door = find_door(ch, type, dir)) >= 0) {
    if (!IS_SET(EXIT(ch, door)->exit_info, EX_ISDOOR)) {
      send_to_char("That's absurd.\r\n", ch);
    } else if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)) {
      send_to_char("You realize that the door is already open.\r\n", ch);
    } else if (EXIT(ch, door)->key < 0) {
      send_to_char("You can't seem to spot any lock to pick.\r\n", ch);
    } else if (!IS_SET(EXIT(ch, door)->exit_info, EX_LOCKED)) {
      send_to_char("Oh.. it wasn't locked at all.\r\n", ch);
    } else if (IS_SET(EXIT(ch, door)->exit_info, EX_PICKPROOF)) {
      send_to_char("You seem to be unable to pick this lock.\r\n", ch);
    } else if (percent > GET_SKILL(ch, skillnum)) {
      send_to_char("You failed to pick the lock.\r\n", ch);
      improve_skill(ch, skillnum, SKUSE_AVERAGE);
    } else {
      REMOVE_BIT(EXIT(ch, door)->exit_info, EX_LOCKED);
      if (EXIT(ch, door)->keyword) {
        act("$n skillfully picks the lock of the $F.", 0, ch, 0, EXIT(ch, door)->keyword, TO_ROOM);
      } else {
        act("$n picks the lock of the door.", TRUE, ch, 0, 0, TO_ROOM);
      }
      send_to_char("The lock quickly yields to your skills.\r\n", ch);
      improve_skill(ch, skillnum, SKUSE_AVERAGE);
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

ACMD(do_sneak)
{
  struct affected_type af;
  byte percent;
  int skillnum = spells[find_skill_num("sneak")].spellindex;

  if (IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("You had better leave this skill to the players.\r\n", ch);
    return;
  }
  if (!IS_NPC(ch) && !GET_SKILL(ch, skillnum)) {
    if (!IS_THI(ch))
      send_to_char("You're about as sneaky as a peg legged Anclar.\r\n", ch);
    else
      send_to_char("You know not of that skill.\r\n", ch);
    return;
  }

  send_to_char("Okay, you'll try to move silently for a while.\r\n", ch);
  improve_skill(ch, skillnum, SKUSE_AVERAGE);
  if (IS_AFFECTED(ch, AFF_SNEAK))
    affect_from_char(ch, skillnum);

  percent = number(1, 101); /* 101% is a complete failure */

  if (IS_NPC(ch) && percent > GET_LEVEL(ch))
    return;
  if (percent > GET_SKILL(ch, skillnum) + stats[DEX_SKILL][GET_DEX(ch)])
    return;

  af.type = skillnum;
  af.duration = GET_LEVEL(ch);
  af.modifier[0] = 0;
  af.location[0] = APPLY_NONE;
  af.bitvector = AFF_SNEAK;
  affect_to_char(ch, &af);
}

ACMD(do_hide)
{
  byte percent;
  int skillnum = spells[find_skill_num("hide")].spellindex;

  if (IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("You had better leave this skill to the players.\r\n", ch);
    return;
  }
  if (!IS_NPC(ch) && !GET_SKILL(ch, skillnum)) {
    if (!IS_THI(ch))
      send_to_char("You might as well be holding a neon sign.\r\n", ch);
    else
      send_to_char("You know not of that skill.\r\n", ch);
    return;
  }

  send_to_char("You attempt to hide yourself.\r\n", ch);
  improve_skill(ch, skillnum, SKUSE_AVERAGE);

  if (IS_AFFECTED(ch, AFF_HIDE))
    REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);

  percent = number(1, 101); /* 101% is a complete failure */

  if (IS_NPC(ch) && percent > GET_LEVEL(ch))
    return;
  if (percent > GET_SKILL(ch, skillnum) + stats[DEX_SKILL][GET_DEX(ch)])
    return;

  SET_BIT(AFF_FLAGS(ch), AFF_HIDE);
}

ACMD(do_steal)
{
  struct char_data *vict;
  struct obj_data *obj;
  char vict_name[240];
  char obj_name[240];
  int percent, coins, eq_pos, pcsteal = 0;
  bool ohoh = FALSE;
  bool logg = FALSE;
  int skillnum = spells[find_skill_num("steal")].spellindex;

  argument = one_argument(argument, obj_name);
  one_argument(argument, vict_name);

  if (IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("You had better leave this skill to the players.\r\n", ch);
    return;
  }
  if (!IS_NPC(ch) && !GET_SKILL(ch, skillnum)) {
    if (!IS_THI(ch))
      send_to_char("You find your hands in your OWN pockets!\r\n", ch);
    else
      send_to_char("You know not of that skill.\r\n", ch);
    return;
  }

  if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
    send_to_char("You feel ashamed disturbing the tranquility of this room.\r\n", ch);
    return;
  }
  if (!(vict = get_char_room_vis(ch, vict_name))) {
    send_to_char("Steal what from who?\r\n", ch);
    return;
  } else if (vict == ch) {
    send_to_char("Come on now, that's rather stupid!\r\n", ch);
    return;
  }
  if (!IS_NPC(vict))
    logg = TRUE;

  if (!IS_NPC(vict) && !PLR_FLAGGED(vict, PLR_THIEF) && !PLR_FLAGGED(vict, PLR_KILLER) && !PLR_FLAGGED(ch, PLR_THIEF) && (!IS_NPC(ch) && !COM_FLAGGED(ch, COM_ADMIN))) {

    if (PLR_FLAGGED(ch, PLR_OUTLAW)) {
      send_to_char("You are not allowed to steal from players!\r\n", ch);
      return;
    }

    SET_BIT(PLR_FLAGS(ch), PLR_THIEF);

    logg = TRUE;

  }

  if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_THIEF))
    pcsteal = 1;

  /* 101% is a complete failure */
  percent = number(1, 101) - stats[DEX_SKILL][GET_DEX(ch)];

  if (GET_POS(vict) < POS_SLEEPING)
    percent = -1; /* ALWAYS SUCCESS */

  /* NO NO With Imp's and Shopkeepers! */
  if (pcsteal || GET_MOB_SPEC(vict) == shop_keeper)
    percent = 101; /* Failure */

  if (GET_LEVEL(vict) >= LVL_IMMORT && !IS_NPC(ch)) {
    send_to_char("No, no, no!!!!\r\n", ch);
    return;
  }

  if (str_cmp(obj_name, "coins")) {

    if (!(obj = get_obj_in_list_vis(vict, obj_name, vict->carrying))) {

      for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++)
        if (vict->equipment[eq_pos] && (isname(obj_name, vict->equipment[eq_pos]->name)) && CAN_SEE_OBJ(ch, vict->equipment[eq_pos])) {
          obj = vict->equipment[eq_pos];
          break;
        }
      if (!obj) {
        act("$E hasn't got that item.", FALSE, ch, 0, vict, TO_CHAR);
        return;
      } else { /* It is equipment */
        if ((GET_POS(vict) > POS_STUNNED)) {
          send_to_char("Steal the equipment now?  Impossible!\r\n", ch);
          return;
        } else {
          act("You unequip $p and steal it.", FALSE, ch, obj, 0, TO_CHAR);
          act("$n steals $p from $N.", FALSE, ch, obj, vict, TO_NOTVICT);
          obj_to_char(unequip_char(vict, eq_pos), ch);
          improve_skill(ch, skillnum, SKUSE_AVERAGE);
          if (logg) {
            snprintf(logbuffer, sizeof(logbuffer), "%s stole %s from %s", GET_NAME(ch), obj->short_description, GET_NAME(vict));
            mudlog(logbuffer, 'Y', COM_IMMORT, TRUE);
            plog(logbuffer, ch, LVL_IMMORT);
          }
        }
      }
    } else { /* obj found in inventory */

      percent += GET_OBJ_WEIGHT(obj); /* Make heavy harder */

      if (AWAKE(vict) && (percent > GET_SKILL(ch, skillnum))) {
        ohoh = TRUE;
        act("Oops..", FALSE, ch, 0, 0, TO_CHAR);
        act("$n tried to steal something from you!", FALSE, ch, 0, vict, TO_VICT);
        act("$n tries to steal something from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
        improve_skill(ch, skillnum, SKUSE_AVERAGE);
        if (logg) {
          snprintf(logbuffer, sizeof(logbuffer), "%s tried to steal %s from %s", GET_NAME(ch), obj->short_description, GET_NAME(vict));
          mudlog(logbuffer, 'Y', COM_IMMORT, TRUE);
          plog(logbuffer, ch, LVL_IMMORT);
        }
      } else { /* Steal the item */
        if ((IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch))) {
          if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) < CAN_CARRY_W(ch)) {
            obj_from_char(obj);
            obj_to_char(obj, ch);
            send_to_char("Got it!\r\n", ch);
            improve_skill(ch, skillnum, SKUSE_AVERAGE);
            if (logg) {
              snprintf(logbuffer, sizeof(logbuffer), "%s stole %s from %s", GET_NAME(ch), obj->short_description, GET_NAME(vict));
              mudlog(logbuffer, 'Y', COM_IMMORT, TRUE);
              plog(logbuffer, ch, LVL_IMMORT);
            }
          }
        } else
          send_to_char("You cannot carry that much.\r\n", ch);
      }
    }

  } else { /* Steal some coins */
    if (AWAKE(vict) && (percent > GET_SKILL(ch, skillnum))) {
      ohoh = TRUE;
      act("Oops..", FALSE, ch, 0, 0, TO_CHAR);
      act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, vict, TO_VICT);
      act("$n tries to steal coins from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
      improve_skill(ch, skillnum, SKUSE_AVERAGE);
      if (logg) {
        snprintf(logbuffer, sizeof(logbuffer), "%s tried to steal coins from %s", GET_NAME(ch), GET_NAME(vict));
        mudlog(logbuffer, 'Y', COM_IMMORT, TRUE);
        plog(logbuffer, ch, LVL_IMMORT);
      }
    } else {
      /* Steal some coins */
      switch (number(0, 3)) {
        case 0: /* plat */
          coins = (int) ((GET_PLAT(vict) * number(1, 10)) / 100);
          coins = MIN(1782, coins);
          if (coins > 0) {
            GET_PLAT(ch) += coins;
            GET_PLAT(vict) -= coins;
            if (!IS_NPC(vict)) {
              GET_TEMP_GOLD(ch) += coins * 1000;
            }
            if (!IS_NPC(vict)) {
              GET_TEMP_GOLD(vict) -= coins * 1000;
            }
            snprintf(buf, MAX_STRING_LENGTH, "Bingo!  You got %d platinum coins.\r\n", coins);
            send_to_char(buf, ch);
            improve_skill(ch, skillnum, SKUSE_AVERAGE);
            if (logg) {
              snprintf(logbuffer, sizeof(logbuffer), "%s stole %d plat from %s", GET_NAME(ch), coins, GET_NAME(vict));
              mudlog(logbuffer, 'Y', COM_IMMORT, TRUE);
              plog(logbuffer, ch, LVL_IMMORT);
            }

          } else {
            send_to_char("You couldn't get any coins...\r\n", ch);
          }
          break;
        case 1: /* gold */
          coins = (int) ((GET_GOLD(vict) * number(1, 10)) / 100);
          coins = MIN(1782, coins);
          if (coins > 0) {
            GET_GOLD(ch) += coins;
            GET_GOLD(vict) -= coins;
            if (!IS_NPC(vict)) {
              GET_TEMP_GOLD(ch) += coins * 100;
            }
            if (!IS_NPC(vict)) {
              GET_TEMP_GOLD(vict) -= coins * 100;
            }
            snprintf(buf, MAX_STRING_LENGTH, "Bingo!  You got %d gold coins.\r\n", coins);
            send_to_char(buf, ch);
            improve_skill(ch, skillnum, SKUSE_AVERAGE);
            if (logg) {
              snprintf(logbuffer, sizeof(logbuffer), "%s stole %d gold from %s", GET_NAME(ch), coins, GET_NAME(vict));
              mudlog(logbuffer, 'Y', COM_IMMORT, TRUE);
              plog(logbuffer, ch, LVL_IMMORT);
            }
          } else {
            send_to_char("You couldn't get any coins...\r\n", ch);
          }
          break;
        case 2: /* silver */
          coins = (int) ((GET_SILVER(vict) * number(1, 10)) / 100);
          coins = MIN(1782, coins);
          if (coins > 0) {
            GET_SILVER(ch) += coins;
            GET_SILVER(vict) -= coins;
            if (!IS_NPC(vict)) {
              GET_TEMP_GOLD(ch) += coins * 10;
            }
            if (!IS_NPC(vict)) {
              GET_TEMP_GOLD(vict) -= coins * 10;
            }
            snprintf(buf, MAX_STRING_LENGTH, "Bingo!  You got %d silver coins.\r\n", coins);
            send_to_char(buf, ch);
            improve_skill(ch, skillnum, SKUSE_AVERAGE);
            if (logg) {
              snprintf(logbuffer, sizeof(logbuffer), "%s stole %d silver from %s", GET_NAME(ch), coins, GET_NAME(vict));
              mudlog(logbuffer, 'Y', COM_IMMORT, TRUE);
              plog(logbuffer, ch, LVL_IMMORT);
            }
          } else {
            send_to_char("You couldn't get any coins...\r\n", ch);
          }
          break;
        case 3: /* copper */
          coins = (int) ((GET_COPPER(vict) * number(1, 10)) / 100);
          coins = MIN(1782, coins);
          if (coins > 0) {
            GET_COPPER(ch) += coins;
            GET_COPPER(vict) -= coins;
            if (!IS_NPC(vict)) {
              GET_TEMP_GOLD(ch) += coins;
            }
            if (!IS_NPC(vict)) {
              GET_TEMP_GOLD(vict) -= coins;
            }
            snprintf(buf, MAX_STRING_LENGTH, "Bingo!  You got %d copper coins.\r\n", coins);
            send_to_char(buf, ch);
            improve_skill(ch, skillnum, SKUSE_AVERAGE);
            if (logg) {
              snprintf(logbuffer, sizeof(logbuffer), "%s stole %d copper from %s", GET_NAME(ch), coins, GET_NAME(vict));
              mudlog(logbuffer, 'Y', COM_IMMORT, TRUE);
              plog(logbuffer, ch, LVL_IMMORT);
            }
          } else {
            send_to_char("You couldn't get any coins...\r\n", ch);
          }
          break;
      }
    }
  }

  if (ohoh && IS_NPC(vict) && AWAKE(vict))
    hit(vict, ch, TYPE_UNDEFINED);
}

ACMD(do_applypoison)
{
  struct obj_data *obj;
  extern struct spell_info_type *spells;
  int skillnum = spells[find_skill_num("apply poison")].spellindex;
  one_argument(argument, arg);

  if (IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("You had better leave this skill to the players.\r\n", ch);
    return;
  }
  if (!IS_NPC(ch) && !GET_SKILL(ch, skillnum)) {
    if (!IS_THI(ch))
      send_to_char("You REALLY ought not try that.\r\n", ch);
    else
      send_to_char("You know not of that skill.\r\n", ch);
    return;
  }

  if (!*arg)
    send_to_char("Apply poison to what?\r\n", ch);
  else if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying))) {
    snprintf(buf, MAX_STRING_LENGTH, "You don't seem to have %s %s.\r\n", AN(arg), arg);
    send_to_char(buf, ch);
  } else {
    if (!CAN_WEAR(obj, ITEM_WEAR_WIELD))
      send_to_char("You cannot apply poison to that.\r\n", ch);
    else if (!GET_SKILL(ch, skillnum))
      send_to_char("You don't know how to apply poison to anything.\r\n", ch);
    else if (number(1, 101) < GET_SKILL(ch, skillnum)) {
      SET_BIT(GET_OBJ_EXTRA(obj), ITEM_POISONED);
      snprintf(buf, MAX_STRING_LENGTH, "You smear poison all over %s %s.\r\n", AN(arg), arg);
      send_to_char(buf, ch);
      improve_skill(ch, skillnum, SKUSE_AVERAGE);
    } else {
      snprintf(buf, MAX_STRING_LENGTH, "Your attempt to poison %s %s failed.\r\n", AN(arg), arg);
      send_to_char(buf, ch);
      improve_skill(ch, skillnum, SKUSE_AVERAGE);
    }
  }
}

ACMD(do_mount)
{
  struct char_data *mount;
  int chance;
  int skillnum = spells[find_skill_num("mount")].spellindex;

  one_argument(argument, arg);

  if (IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("You had better leave this skill to the players.\r\n", ch);
    return;
  }
  if (MOUNTING(ch)) {
    send_to_char("But you are already mounting something???\r\n", ch);
    return;
  }

  if (!(mount = get_char_room_vis(ch, arg))) {
    send_to_char("Mount what?\r\n", ch);
    return;
  } else if (mount == ch) {
    send_to_char("Come on now, that's rather stupid!\r\n", ch);
    return;
  }

  if (GET_POS(mount) < POS_STANDING) {
    send_to_char("Mount someone that isnt standing up??\r\n", ch);
    return;
  }

  if (MOUNTED_BY(mount) || MOUNTING(mount)) {
    act("$N is already mounted, and there is not enough place for you.", FALSE, ch, 0, mount, TO_CHAR);
    return;
  }

  if (IS_NPC(mount) && MOB_FLAGGED(mount, MOB_MOUNTABLE)) { /* okay, the "mount object" is an NPC, go on... */
    if (IS_NPC(ch)) { /* no skill is required for mobiles */
      MOUNTING(ch) = mount; /* ch is now mounting the mobile */
      MOUNTED_BY(mount) = ch; /* mobile is now mounted by ch */
    } else {
      chance = GET_SKILL(ch, skillnum);
      if (chance < 5) {
        SET_SKILL(ch, skillnum, 5);
        chance = 5;
      }
      if (chance > 100) {
        SET_SKILL(ch, skillnum, 100);
        chance = 100;
      }

      if (number(0, 100) <= chance) { /* success */
        /* each time you succeed, you learn more */
        GET_SKILL(ch, skillnum) = GET_SKILL(ch, skillnum) + number(1, 6);
        MOUNTING(ch) = mount;
        MOUNTED_BY(mount) = ch;
      } else { /* okay, failure */
        if (number(0, 100) < 20 && !ROOM_FLAGGED(mount->in_room, ROOM_PEACEFUL)) { /* mount mobile gets angry and attacks */
          act("$N panics and attacks you in fear!", FALSE, ch, 0, mount, TO_CHAR);
          act("As $n tries to mount you, you panic and attack $m.", FALSE, ch, 0, mount, TO_VICT);
          act("$N panics and attacks $n!", FALSE, ch, 0, mount, TO_NOTVICT);
          improve_skill(ch, skillnum, SKUSE_AVERAGE);
          hit(mount, ch, TYPE_UNDEFINED);
        } else { /* ch just falls off */
          act("As you try to mount $N, you fall off and hit the ground!", FALSE, ch, 0, mount, TO_CHAR);
          act("$n tries to mount you, but you shake $m off!", FALSE, ch, 0, mount, TO_VICT);
          act("$n tries to mount $N, but fall off and hit the ground!", FALSE, ch, 0, mount, TO_NOTVICT);
          improve_skill(ch, skillnum, SKUSE_AVERAGE);
          GET_HIT(ch) = GET_HIT(ch) - number(1, 12); /* 1d12 damage when falling off */
          update_pos(ch);
        }
        return;
      }

      act("You jump up on $N's back, YIHAAA!", FALSE, ch, 0, mount, TO_CHAR);
      act("$n jumps up on your back!", FALSE, ch, 0, mount, TO_VICT);
      act("$n jumps up on $N's back!", FALSE, ch, 0, mount, TO_NOTVICT);
      improve_skill(ch, skillnum, SKUSE_AVERAGE);
    }
  } else { /* you cannot mount other players.... :) */
    act("You try to mount $N, but $E scream and push you away!", FALSE, ch, 0, mount, TO_CHAR);
    act("$n tries to mount you, but you scream and push $m away!", FALSE, ch, 0, mount, TO_VICT);
    act("$n tries to mount $N, who scream and push $m away!", FALSE, ch, 0, mount, TO_NOTVICT);
    return;
  }
}

ACMD(do_dismount)
{
  struct char_data *mount;

  if (MOUNTING(ch)) {
    mount = MOUNTING(ch);
    act("You jump off $N's back, and land heavily on the ground.", FALSE, ch, 0, mount, TO_CHAR);
    act("$n jumps off your back.", FALSE, ch, 0, mount, TO_VICT);
    act("$n jumps off $N's back, and lands heavily on the ground.", FALSE, ch, 0, mount, TO_NOTVICT);
    MOUNTED_BY(mount) = NULL;
    MOUNTING(ch) = NULL;
  } else {
    send_to_char("But you are not mounting anything?\r\n", ch);
  }

}

/* functions and macros for 'scan' command */
void list_scanned_chars(struct char_data * list, struct char_data * ch, int distance, int door)
{
  const char *how_far[] = {"close by", "a ways off", "far off to the"};

  struct char_data *i;
  *buf = '\0';

  for (i = list; i; i = i->next_in_room) {
    if (!CAN_SEE(ch, i))
      continue;
    /* you may want to add other checks in here, perhaps something special
     for Rangers, maybe check terrain, whatever.. */
    if (!*buf)
      snprintf(buf, MAX_STRING_LENGTH, "You see %s", GET_NAME(i));
    if (i->next_in_room) {
      if (i->next_in_room->next_in_room)
        snprintf(buf2, MAX_STRING_LENGTH, ", %s", GET_NAME(i->next_in_room));
      else
        snprintf(buf2, MAX_STRING_LENGTH, " and %s", GET_NAME(i->next_in_room));
    } else
      snprintf(buf2, MAX_STRING_LENGTH, " %s %s.\r\n", how_far[distance], dirs[door]);
    strcat(buf, buf2);

  }
  send_to_char(buf, ch);
}

/* utils.h: #define EXIT(ch, door)  (world[(ch)->in_room].dir_option[door]) */
/* utils.h: #define _2ND_EXIT(ch, door) (world[EXIT(ch, door)->to_room].dir_option[door]) */
/* utils.h: #define _3RD_EXIT(ch, door) (world[_2ND_EXIT(ch, door)->to_room].dir_option[door]) */

ACMD(do_scout)
{
  /* >scan
   You quickly scan the area.
   You see John, a large horse and Frank close by north.
   You see a small rabbit a ways off south.
   You see a huge dragon and a griffon far off to the west.

   *make scan a skill (ranger?) with a prof. check in each dir. ?
   */
  int door;
  int skillnumber = spells[find_skill_num("scout")].spellindex;

  *buf = '\0';

  if (IS_AFFECTED(ch, AFF_BLIND)) {
    send_to_char("You can't see a damned thing, you're blind!\r\n", ch);
    return;
  }
  /* may want to add more restrictions here, too */
  send_to_char("You quickly scan the area.\r\n", ch);
  improve_skill(ch, skillnumber, SKUSE_AVERAGE);
  for (door = 0; door < NUM_OF_DIRS; door++)
    if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE && !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED) && (GET_SKILL(ch, skillnumber) > number(0, 100))) {
      if (world[EXIT(ch, door)->to_room].people) {
        list_scanned_chars(world[EXIT(ch, door)->to_room].people, ch, 0, door);
      } else if (_2ND_EXIT(ch, door) && _2ND_EXIT(ch, door)->to_room != NOWHERE && !IS_SET(_2ND_EXIT(ch, door)->exit_info, EX_CLOSED)) {
        /* check the second room away */
        if (world[_2ND_EXIT(ch, door)->to_room].people) {
          list_scanned_chars(world[_2ND_EXIT(ch, door)->to_room].people, ch, 1, door);
        } else if (_3RD_EXIT(ch, door) && _3RD_EXIT(ch, door)->to_room != NOWHERE && !IS_SET(_3RD_EXIT(ch, door)->exit_info, EX_CLOSED)) {
          /* check the third room */
          if (world[_3RD_EXIT(ch, door)->to_room].people) {
            list_scanned_chars(world[_3RD_EXIT(ch, door)->to_room].people, ch, 2, door);
          }

        }
      }
    }
}

ACMD(do_bandage)
{
  struct char_data *vict;
  struct obj_data *obj;
  char vict_name[240];
  int percent;
  int skillnum = spells[find_skill_num("bandage")].spellindex;

  one_argument(argument, vict_name);

  if (IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("You had better leave this skill to the players.\r\n", ch);
    return;
  }
  if (!(vict = get_char_room_vis(ch, vict_name))) {
    send_to_char("Bandage who?\r\n", ch);
    return;
  } else if (vict == ch) {
    if (GET_HIT(ch) < 1) {
      send_to_char("The pain is too intense for you to bandage yourself properly.\r\n", ch);
    } else {
      send_to_char("You have no need of medical assistance.\r\n", ch);
    }
    return;
  } else if (GET_HIT(vict) >= 1) {
    send_to_char("They are beyond the need of your medical assistance.\r\n", ch);
    return;
  } else if (!(obj = get_obj_in_list_vis(ch, "bandage", ch->carrying))) {
    send_to_char("You need a bandage to make this a success.\r\n", ch);
    return;
  }

  percent = 50 + (GET_SKILL(ch, skillnum) / 2);

  if (number(1, 101) < percent) {
    extract_obj(obj);
    GET_HIT(vict) = 1;
    act("You quickly administer to $N's wounds.", FALSE, ch, 0, vict, TO_CHAR);
    act("$n applies bandages to your wounds allowing you to live a moment longer.", FALSE, ch, 0, vict, TO_VICT);
    act("$n quickly administers bandages to $N's wounds.", FALSE, ch, 0, vict, TO_NOTVICT);
  } else {
    act("You attempt administer to $N's wounds, but find your skill lacking.", FALSE, ch, 0, vict, TO_CHAR);
    act("$n attempts to bandage your wounds, but fails.", FALSE, ch, 0, vict, TO_VICT);
    act("$n attempts to bandage $N's wounds, but fails.", FALSE, ch, 0, vict, TO_NOTVICT);
  }

  improve_skill(ch, skillnum, SKUSE_AVERAGE);
}

ACMD(do_roundhouse)
{
  struct char_data *vict;
  byte percent, prob;
  int skillnum = spells[find_skill_num("roundhouse")].spellindex;

  if (IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("You had better leave this skill to the players.\r\n", ch);
    return;
  }
  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_WRAITHLIKE)) {
    send_to_char("You're not solid leave this skill to solids.\r\n", ch);
    return;
  }
  if (!IS_NPC(ch) && !GET_SKILL(ch, skillnum)) {
    send_to_char("You dont know of that skill.\r\n", ch);
    return;
  }
  if (ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
    send_to_char("You feel ashamed disturbing the tranquility of this place\r\n", ch);
    return;
  }

  one_argument(argument, arg);

  if (!(vict = get_char_room_vis(ch, arg))) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
    } else {
      send_to_char("Kick who?\r\n", ch);
      return;
    }
  }
  if (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_NOKILL)) {
    send_to_char("That's not a good idea.\r\n", ch);
    return;
  }
  if (vict == ch) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  } else if (IS_AFFECTED(ch, AFF_CHARM) && !IS_NPC(ch->master) && !IS_NPC(vict)) {
    send_to_char("Aren't we funny today...\r\n", ch);
    return;
  }

  if ((CHECK_FIGHT(ch) != 0) && FIGHTING(ch)) {
    send_to_char("You are too engaged in combat to kick now!\r\n", ch);
    return;
  }

  if (IS_NPC(vict) && MOB_FLAGGED(vict, MOB_WRAITHLIKE)) {
    act("Your round-house kick sails through $N.", TRUE, ch, 0, vict, TO_CHAR);
    act("$n tries to kick you, but passes right through.", TRUE, ch, 0, vict, TO_VICT);
    act("$n tries to kick $N, but passes through $e.", TRUE, ch, 0, vict, TO_ROOM);
    return;
  }
  percent = ((10 - (GET_AC(vict) / 10)) << 1) + number(1, 101); /* 101% is a complete */
  if (IS_NPC(ch))
    prob = GET_LEVEL(ch);
  else
    prob = GET_SKILL(ch, skillnum);

  if (percent > prob) {
    damage(ch, vict, 0, skillnum, 0, DAM_BLUDGEON, 0, 0);
  } else {
    damage(ch, vict, (number(0, 1) ? (GET_LEVEL(ch) >> 1) : GET_LEVEL(ch)), skillnum, 0, DAM_BLUDGEON, 0, 0);
  }
  improve_skill(ch, skillnum, SKUSE_AVERAGE);
  FIGHT_STATE(ch, 2);
}
