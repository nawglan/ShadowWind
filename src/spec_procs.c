/************************************************************************
 *   File: spec_procs.c                                  Part of CircleMUD *
 *  Usage: implementation of special procedures for mobiles/objects/rooms  *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "event.h"
#include "spells.h"

/*   external vars  */
extern struct char_data *combat_list; /* head of l-list of fighting chars */
extern int PRACS_COST;
extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct time_info_data time_info;
extern const struct command_info cmd_info[];
extern int max_lvl_skill[2][4][52];
extern struct spell_info_type *spells;
extern struct char_data *mob_proto;

/* extern functions */
struct char_data *find_hunted_char(int idnum);
int find_skill_num(char *name);
int find_spell_num(char *name);
void add_follower(struct char_data * ch, struct char_data * leader);
void hunt_victim(struct char_data * ch);
char *skill_prac_price_text(struct char_data *ch, struct spell_info_type *sinfo);
int skill_prac_price(struct char_data *ch, struct spell_info_type *sinfo);

struct social_type {
  char *cmd;
  int next_line;
};

void global_echo(char *string)
{
  struct descriptor_data *i;

  if (string)
    for (i = descriptor_list; i; i = i->next)
      if (!i->connected && !PLR_FLAGGED(i->character, PLR_WRITING) && !PLR_FLAGGED(i->character, PLR_EDITING))
        SEND_TO_Q(string, i);

}
void zone_echo(struct char_data *ch, char *string)
{
  struct descriptor_data *i;

  if (string && ch)
    for (i = descriptor_list; i; i = i->next)
      if (!i->connected && !PLR_FLAGGED(i->character, PLR_WRITING) && !PLR_FLAGGED(i->character, PLR_EDITING) && (world[ch->in_room].zone == world[i->character->in_room].zone))
        SEND_TO_Q_COLOR(string, i);

}

/* ********************************************************************
 *  Special procedures for mobiles                                     *
 ******************************************************************** */

char *how_good(int percent)
{
  static char buf[256];

  if (percent == 0)
    strcpy(buf, " {c({Ynot learned{c){x");
  else if (percent <= 10)
    strcpy(buf, " {c({rawful{c){x");
  else if (percent <= 20)
    strcpy(buf, " {c({Rbad{c){x");
  else if (percent <= 40)
    strcpy(buf, " {c({wbelow average{c){x");
  else if (percent <= 55)
    strcpy(buf, " {c({Baverage{c){x");
  else if (percent <= 70)
    strcpy(buf, " {c({gabove average{c){x");
  else if (percent <= 80)
    strcpy(buf, " {c({Ggood{c){x");
  else if (percent <= 85)
    strcpy(buf, " {c({wreally good{c){x");
  else if (percent <= 99)
    strcpy(buf, " {c({Csuperb{c){x");
  else
    strcpy(buf, " {c({Wmaster{c){x");

  return (buf);
}

char *prac_types[] = {"spell", "skill"};

#define LEARNED_LEVEL  0  /* % known which is considered "learned" */
#define MAX_PER_PRAC  1  /* max percent gain in skill per practice */
#define MIN_PER_PRAC  2  /* min percent gain in skill per practice */
#define PRAC_TYPE  3  /* should it say 'spell' or 'skill'?   */

/* actual prac_params are in class.c */
extern int prac_params[4][NUM_CLASSES];

#define LEARNED(ch) (prac_params[LEARNED_LEVEL][(int)GET_CLASS(ch)])
#define MINGAIN(ch) (prac_params[MIN_PER_PRAC][(int)GET_CLASS(ch)])
#define MAXGAIN(ch) (prac_params[MAX_PER_PRAC][(int)GET_CLASS(ch)])
#define SPLSKL(ch) (prac_types[prac_params[PRAC_TYPE][(int)GET_CLASS(ch)]])

void show_skills(struct char_data * ch, struct char_data *vict)
{
  int i;

  if (ch == vict)
    snprintf(buf, MAX_STRING_LENGTH, "You know of the following:\r\n\r\n{cSkills{C: {m-------{x\r\n");
  else
    snprintf(buf, MAX_STRING_LENGTH, "%s knows the following:\r\n\r\n{cSkills{C: {m-------{x\r\n", GET_NAME(vict));

  strcpy(buf2, buf);

  for (i = 1; spells[i].command[0] != '\n'; i++) {
    if (strlen(buf2) >= MAX_STRING_LENGTH - 32) {
      strcat(buf2, "**OVERFLOW**\r\n");
      break;
    }
    if ((GET_LEVEL(vict) >= spells[i].min_level[(int) GET_CLASS(vict)] || GET_SKILL(vict, spells[i].spellindex)) && !spells[i].spell_pointer && spells[i].command[0] != '<') {
      if (!PRACS_COST || ch == vict) {
        sprintf(buf2 + strlen(buf2), "%-20s %s\r\n", spells[i].command, how_good(GET_SKILL(vict, spells[i].spellindex)));
      } else {
        sprintf(buf2 + strlen(buf2), "%-20s %s %s\r\n", spells[i].command, how_good(GET_SKILL(vict, spells[i].spellindex)), skill_prac_price_text(vict, (spells + i)));
      }
    }
  }

  if (!IS_MAGE(vict) && !IS_PRI(vict)) {
    page_string(ch->desc, buf2, 1);
    return;
  }

  sprintf(buf2 + strlen(buf2), "\r\n{cSpells{C: {m-------{x\r\n\r\n");

  for (i = 1; spells[i].command[0] != '\n'; i++) {
    if (strlen(buf2) >= MAX_STRING_LENGTH - 32) {
      strcat(buf2, "**OVERFLOW**\r\n");
      break;
    }
    if (!PRACS_COST || ch == vict) {
      if ((GET_LEVEL(vict) >= spells[i].min_level[(int) GET_CLASS(vict)] || GET_SKILL(vict, spells[i].spellindex)) && spells[i].spell_pointer) {
        sprintf(buf2 + strlen(buf2), "%-20s %s\r\n", spells[i].command, (GET_SKILL(vict, spells[i].spellindex) ? " " : "(unlearned)"));
      }
    } else {
      if ((GET_LEVEL(vict) >= spells[i].min_level[(int) GET_CLASS(vict)] || GET_SKILL(vict, spells[i].spellindex)) && spells[i].spell_pointer) {
        if (GET_SKILL(ch, spells[i].spellindex))
          sprintf(buf2 + strlen(buf2), "%-20s\r\n", spells[i].command);
        else
          sprintf(buf2 + strlen(buf2), "%-20s (unlearned) %s\r\n", spells[i].command, skill_prac_price_text(ch, (spells + i)));
      }
    }
  }
  page_string(ch->desc, buf2, 1);
}

void list_skills(struct char_data * ch)
{
  int i;
  int found = 0;
  int circle;
  char abuf[8196];

  snprintf(buf2, MAX_STRING_LENGTH, "You know of the following:\r\n\r\n{cSkills{C: {m-------{x\r\n");

  for (i = 1; spells[i].command[0] != '\n'; i++) {
    if (strlen(buf2) >= MAX_STRING_LENGTH - 32) {
      strcat(buf2, "**OVERFLOW**\r\n");
      break;
    }
    if ((GET_LEVEL(ch) >= spells[i].min_level[(int) GET_CLASS(ch)] || GET_SKILL(ch, spells[i].spellindex)) && !spells[i].spell_pointer && spells[i].command[0] != '<') {
      sprintf(buf2 + strlen(buf2), "%-20s %s\r\n", spells[i].command, how_good(GET_SKILL(ch, spells[i].spellindex)));
    }
  }

  if (!IS_MAGE(ch) && !IS_PRI(ch)) {
    page_string(ch->desc, buf2, 1);
    return;
  }

  sprintf(buf2 + strlen(buf2), "\r\n{cSpells{C: {m-------{x\r\n\r\n");

  for (circle = 1; circle <= GET_PLR_CIRCLE(ch); circle++) {
    found = 0;
    sprintf(abuf, "{gCircle {G%d{x\r\n", circle);
    for (i = 1; spells[i].command[0] != '\n'; i++) {
      if (strlen(buf2) >= MAX_STRING_LENGTH - 32) {
        strcat(buf2, "**OVERFLOW**\r\n");
        break;
      }
      if ((((spells[i].min_level[(int) GET_CLASS(ch)] + 4) / 5) == circle) && (GET_LEVEL(ch) >= spells[i].min_level[(int) GET_CLASS(ch)] || GET_SKILL(ch, spells[i].spellindex)) && spells[i].spell_pointer) {
        sprintf(abuf + strlen(abuf), "\t%-20s %s\r\n", spells[i].command, (GET_SKILL(ch, spells[i].spellindex) ? "" : "(unlearned)"));
        found = 1;
      }
    }
    if (found)
      sprintf(buf2 + strlen(buf2), "%s", abuf);
  }
  page_string(ch->desc, buf2, 1);
}

char *skill_prac_price_text(struct char_data *ch, struct spell_info_type *sinfo)
{
  return make_money_text(skill_prac_price(ch, sinfo));
}

int getStat(struct char_data *ch, int val)
{
  switch (val) {
    case STAT_STR:
      return GET_STR(ch);
    case STAT_DEX:
      return GET_DEX(ch);
    case STAT_INT:
      return GET_INT(ch);
    case STAT_WIS:
      return GET_WIS(ch);
    case STAT_CON:
      return GET_CON(ch);
    case STAT_AGI:
      return GET_AGI(ch);
  }

  return 49;
}

int skill_prac_price(struct char_data *ch, struct spell_info_type *sinfo)
{
  int cost;
  int c;
  int prime;
  int second;

  if (sinfo->spell_pointer) { /* it's a spell */
    c = GET_SPELL_CIRCLE(ch, sinfo);
    if (c == 1)
      return 0;
    cost = c * c;
    if (cost >= 5) {
      cost *= c;
    }
    cost *= 129;
  } else { /* it's a skill */
    cost = ((GET_LEVEL(ch) * 666 * sinfo->cost_multiplier * GET_SKILL(ch, sinfo->spellindex)) / 50);
    c = MAX(1, GET_SKILL(ch, sinfo->spellindex));
    cost *= ((c * c * c) / 8000);
    if (sinfo->prime_stat && sinfo->second_stat) {
      prime = getStat(ch, sinfo->prime_stat);
      second = getStat(ch, sinfo->second_stat);
      if (strstr(sinfo->command, " realm")) {
        if (IS_PRI(ch)) { /* wis used for priests */
          prime = getStat(ch, sinfo->second_stat);
          second = getStat(ch, sinfo->prime_stat);
        }
      }
      cost = cost / (4 * ((2 * prime + second) / 3));
    } else if (sinfo->prime_stat) {
      prime = getStat(ch, sinfo->prime_stat);
      cost = cost / (4 * prime);
    } else {
      cost = cost / 196;
    }
    if (!cost)
      cost = 10;
  }
  return cost;
}

void list_skills_cost(struct char_data * ch)
{
  int i;
  int found = 0;
  int circle;
  char abuf[8196];

  snprintf(buf2, MAX_STRING_LENGTH, "You know of the following:\r\n\r\n{cSkills{C: {m-------{x\r\n");

  for (i = 1; spells[i].command[0] != '\n'; i++) {
    if (strlen(buf2) >= MAX_STRING_LENGTH - 32) {
      strcat(buf2, "**OVERFLOW**\r\n");
      break;
    }
    if ((GET_LEVEL(ch) >= spells[i].min_level[(int) GET_CLASS(ch)] || GET_SKILL(ch, spells[i].spellindex)) && !spells[i].spell_pointer && spells[i].command[0] != '<') {
      sprintf(buf2 + strlen(buf2), "%-20s %s %s\r\n", spells[i].command, how_good(GET_SKILL(ch, spells[i].spellindex)), skill_prac_price_text(ch, (spells + i)));
    }
  }

  if (!IS_MAGE(ch) && !IS_PRI(ch)) {
    page_string(ch->desc, buf2, 1);
    return;
  }

  sprintf(buf2 + strlen(buf2), "\r\n{cSpells{C: {m-------{x\r\n\r\n");

  for (circle = 1; circle <= GET_PLR_CIRCLE(ch); circle++) {
    found = 0;
    sprintf(abuf, "{gCircle {G%d{x\r\n", circle);
    for (i = 1; spells[i].command[0] != '\n'; i++) {
      if (strlen(buf2) >= MAX_STRING_LENGTH - 32) {
        strcat(buf2, "**OVERFLOW**\r\n");
        break;
      }
      if ((((spells[i].min_level[(int) GET_CLASS(ch)] + 4) / 5) == circle) && (GET_LEVEL(ch) >= spells[i].min_level[(int) GET_CLASS(ch)] || GET_SKILL(ch, spells[i].spellindex)) && spells[i].spell_pointer) {
        if (GET_SKILL(ch, spells[i].spellindex))
          sprintf(abuf + strlen(abuf), "\t%-20s\r\n", spells[i].command);
        else
          sprintf(abuf + strlen(abuf), "\t%-20s (unlearned) %s\r\n", spells[i].command, skill_prac_price_text(ch, (spells + i)));
        found = 1;
      }
    }
    if (found)
      sprintf(buf2 + strlen(buf2), "%s", abuf);
  }
  page_string(ch->desc, buf2, 1);
}

SPECIAL(weapon)
{
  int skill_num, percent;

  extern sh_int stats[11][101];
  int skillnum = spells[find_skill_num("backstab")].spellindex;

  if (IS_NPC(ch) || !CMD_IS("train"))
    return 0;

  skip_spaces(&argument);

  if (!*argument) {
    return 1;
  }
  if (GET_TRAINING(ch) <= 0) {
    send_to_char("You do not seem to be able to train now.\r\n", ch);
    return 1;
  }

  skill_num = find_skill_num(argument);

  if (skill_num < 1 || GET_LEVEL(ch) < spells[skill_num].min_level[(int) GET_CLASS(ch)] || (skill_num < skillnum)) {
    snprintf(buf, MAX_STRING_LENGTH, "What kind of combat technique is that?\r\n");
    send_to_char(buf, ch);
    return 1;
  }
  if (GET_SKILL(ch, skill_num) >= LEARNED(ch)) {
    send_to_char("You already master that combat technique.\r\n", ch);
    return 1;
  }
  send_to_char("You train some combat techniques for awhile...\r\n", ch);
  GET_TRAINING(ch)--;

  percent = GET_SKILL(ch, skill_num);
  percent += BOUNDED(MINGAIN(ch), stats[INT_LEARN][GET_INT(ch)], MAXGAIN(ch));

  SET_SKILL(ch, spells[skill_num].spellindex, MIN(LEARNED(ch), percent));

  if (GET_SKILL(ch, skill_num) >= LEARNED(ch))
    send_to_char("You are now learned in that area.\r\n", ch);

  return 1;
}

SPECIAL(statue_39)
{

  if (IS_NPC(ch) || !CMD_IS("kneel") || !IS_MAGE(ch))
    return 0;
  act("$n dissappears in a cloud of smoke.", FALSE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, real_room(3976));
  look_at_room(ch, 0);
  act("$n appears in a cloud of smoke.", FALSE, ch, 0, 0, TO_ROOM);
  return 1;
}

SPECIAL(guild)
{
  int skill_num, percent;
  int newpercent;
  int temp;
  int temp_gold;
  int temp_bank_gold;
  int prac_cost;

  extern sh_int stats[11][101];

  if (IS_NPC(ch) || (!CMD_IS("train") && !CMD_IS("practice") && !CMD_IS("specialize")))
    return 0;

  skip_spaces(&argument);

  if (!*argument && !CMD_IS("specialize")) {
    if (GET_LEVEL(ch) > 10 && PRACS_COST) {
      list_skills_cost(ch);
    } else {
      list_skills(ch);
    }
    return 1;
  } else if (!*argument) {
    send_to_char("You may specialize in one of the following:\r\n"
        "general realm, creation realm, divination realm, elemental realm,\r\n"
        "enchantment realm, healing realm, protection realm, or summoning realm.\r\n", ch);
    return 1;
  }

  if (CMD_IS("specialize")) {
    if (!IS_MAGE(ch) && !IS_PRI(ch)) {
      send_to_char("You have no need to specialize in anything.\r\n", ch);
      return 1;
    } else if (GET_SPECIALIZED(ch)) {
      send_to_char("You have already specialized in a realm of magic.\r\n", ch);
      return 1;
    }
    switch (*argument) {
      case 'c':
      case 'C':
        send_to_char("You have chosen to specialize in the creation realm of magic.\r\n", ch);
        GET_SPECIALIZED(ch) = (1 << 1);
        break;
      case 'd':
      case 'D':
        send_to_char("You have chosen to specialize in the divination realm of magic.\r\n", ch);
        GET_SPECIALIZED(ch) = (1 << 2);
        break;
      case 'e':
      case 'E':
        if (argument[1] == 'l' || argument[1] == 'L') {
          send_to_char("You have chosen to specialize in the elemental realm of magic.\r\n", ch);
          GET_SPECIALIZED(ch) = (1 << 3);
        } else {
          send_to_char("You have chosen to specialize in the elemental realm of magic.\r\n", ch);
          GET_SPECIALIZED(ch) = (1 << 7);
        }
        break;
      case 'g':
      case 'G':
        send_to_char("You have chosen to specialize in the general realm of magic.\r\n", ch);
        GET_SPECIALIZED(ch) = (1 << 0);
        break;
      case 'h':
      case 'H':
        send_to_char("You have chosen to specialize in the healing realm of magic.\r\n", ch);
        GET_SPECIALIZED(ch) = (1 << 4);
        break;
      case 'p':
      case 'P':
        send_to_char("You have chosen to specialize in the protection realm of magic.\r\n", ch);
        GET_SPECIALIZED(ch) = (1 << 5);
        break;
      case 's':
      case 'S':
        send_to_char("You have chosen to specialize in the summoning realm of magic.\r\n", ch);
        GET_SPECIALIZED(ch) = (1 << 6);
        break;
      default:
        send_to_char("You may specialize in one of the following:\r\n"
            "general realm, creation realm, divination realm, elemental realm,\r\n"
            "healing realm, protection realm, or summoning realm.\r\n", ch);
        break;
    }
    return 1;
  }
  skill_num = find_skill_num(argument);

  if (skill_num < 1 || GET_LEVEL(ch) < spells[skill_num].min_level[(int) GET_CLASS(ch)] || (skill_num >= MAX_SKILLS)) {
    snprintf(buf, MAX_STRING_LENGTH, "You do not know of that %s.\r\n", SPLSKL(ch));
    send_to_char(buf, ch);
    return 1;
  }

  if ((GET_SKILL(ch, spells[skill_num].spellindex) != 0) && spells[skill_num].spell_pointer) {
    snprintf(buf, MAX_STRING_LENGTH, "You have no need to learn that %s again.\r\n", SPLSKL(ch));
    send_to_char(buf, ch);
    return 1;
  }

  if (spells[skill_num].quest_only) {
    snprintf(buf, MAX_STRING_LENGTH, "I cannot teach you that %s.\r\n"
        "Perhapse you should seek out %s.\r\n", SPLSKL(ch), GET_MOB_NAME(mob_proto + real_mobile(spells[skill_num].qvnum)));
    send_to_char(buf, ch);
    return 1;
  }
  temp_gold = GET_TEMP_GOLD(ch);
  temp_bank_gold = GET_BANK_PLAT(ch) * 1000 + GET_BANK_GOLD(ch) * 100 + GET_BANK_SILVER(ch) * 10 + GET_BANK_COPPER(ch);
  prac_cost = skill_prac_price(ch, spells + skill_num);

  if (PRACS_COST && prac_cost > (temp_gold + temp_bank_gold)) {
    snprintf(buf, MAX_STRING_LENGTH, "You cannot afford to practice that %s.\r\n", SPLSKL(ch));
    send_to_char(buf, ch);
    return 1;
  }
  /* Code to allow for new skills added after player creation */
  if ((GET_PRACS(ch, spells[skill_num].spellindex) == 0) && (GET_SKILL(ch, spells[skill_num].spellindex) == 0)) {
    GET_PRACS(ch, spells[skill_num].spellindex) = (GET_LEVEL(ch) - spells[skill_num].min_level[(int) GET_CLASS(ch)]);
    SET_SKILL(ch, spells[skill_num].spellindex, 15);
  }

  if (GET_PRACS(ch, spells[skill_num].spellindex) == 0) {
    send_to_char("You have practiced that skill all you can this level.\r\n", ch);
    return 1;
  }

  percent = GET_SKILL(ch, spells[skill_num].spellindex);
  if (max_lvl_skill[0][spells[skill_num].difficulty][(int) GET_LEVEL(ch)] > percent) {
    newpercent = percent + BOUNDED(MINGAIN(ch), stats[INT_LEARN][GET_INT(ch)], MAXGAIN(ch));
    if (max_lvl_skill[0][spells[skill_num].difficulty][(int) GET_LEVEL(ch)] < newpercent)
      newpercent = max_lvl_skill[0][spells[skill_num].difficulty][(int) GET_LEVEL(ch)];
    send_to_char("You practice for a while...\r\n", ch);
    snprintf(buf2, MAX_STRING_LENGTH, "SKILLIMPROVE: %s practiced skill %s, int = %d, wis = %d, improved by = %d, now = %d\n", GET_NAME(ch), spells[skill_num].command, GET_INT(ch), GET_WIS(ch), newpercent - percent, MIN(LEARNED(ch), newpercent));
    mudlog(buf2, 'D', COM_IMMORT, TRUE);
    SET_SKILL(ch, spells[skill_num].spellindex, MIN(LEARNED(ch), newpercent));
    GET_PRACS(ch, spells[skill_num].spellindex)--;
    if (PRACS_COST) {
      if (temp_gold < prac_cost) {
        GET_BANK_PLAT(ch) += GET_PLAT(ch);
        GET_PLAT(ch) = 0;
        GET_BANK_GOLD(ch) += GET_GOLD(ch);
        GET_GOLD(ch) = 0;
        GET_BANK_SILVER(ch) += GET_SILVER(ch);
        GET_SILVER(ch) = 0;
        GET_BANK_COPPER(ch) += GET_COPPER(ch);
        GET_COPPER(ch) = 0;
        GET_TEMP_GOLD(ch) = 0;
        temp = prac_cost - temp_gold;
        temp_gold = 0;
        while (temp >= 1000) {
          if (GET_BANK_PLAT(ch)) {
            GET_BANK_PLAT(ch) -= 1;
            temp -= 1000;
          } else if (GET_BANK_GOLD(ch) >= 10) {
            GET_BANK_PLAT(ch) += 1;
            GET_BANK_GOLD(ch) -= 10;
          } else if (GET_BANK_SILVER(ch) >= 100) {
            GET_BANK_PLAT(ch) += 1;
            GET_BANK_SILVER(ch) -= 100;
          } else if (GET_BANK_COPPER(ch) >= 1000) {
            GET_BANK_PLAT(ch) += 1;
            GET_BANK_COPPER(ch) -= 1000;
          } else {
            break;
          }
        }
        while (temp >= 100) {
          if (GET_BANK_GOLD(ch)) {
            GET_BANK_GOLD(ch) -= 1;
            temp -= 100;
          } else if (GET_BANK_COPPER(ch) >= 100) {
            GET_BANK_COPPER(ch) -= 100;
            GET_BANK_GOLD(ch) += 1;
          } else if (GET_BANK_SILVER(ch) >= 10) {
            GET_BANK_SILVER(ch) -= 10;
            GET_BANK_GOLD(ch) += 1;
          } else if (GET_BANK_PLAT(ch)) {
            GET_BANK_PLAT(ch) -= 1;
            GET_BANK_GOLD(ch) += 10;
          } else {
            break;
          }
        }
        while (temp >= 10) {
          if (GET_BANK_SILVER(ch)) {
            GET_BANK_SILVER(ch) -= 1;
            temp -= 10;
          } else if (GET_BANK_COPPER(ch) >= 10) {
            GET_BANK_COPPER(ch) -= 10;
            GET_BANK_SILVER(ch) += 1;
          } else if (GET_BANK_GOLD(ch)) {
            GET_BANK_GOLD(ch) -= 1;
            GET_BANK_SILVER(ch) += 10;
          } else if (GET_BANK_PLAT(ch)) {
            GET_BANK_PLAT(ch) -= 1;
            GET_BANK_SILVER(ch) += 100;
          } else {
            break;
          }
        }
        while (temp >= 1) {
          if (GET_BANK_COPPER(ch)) {
            GET_BANK_COPPER(ch) -= 1;
            temp--;
          } else if (GET_BANK_SILVER(ch)) {
            GET_BANK_SILVER(ch) -= 1;
            GET_BANK_COPPER(ch) += 10;
          } else if (GET_BANK_GOLD(ch)) {
            GET_BANK_GOLD(ch) -= 1;
            GET_BANK_COPPER(ch) += 100;
          } else if (GET_BANK_PLAT(ch)) {
            GET_BANK_PLAT(ch) -= 1;
            GET_BANK_COPPER(ch) += 1000;
          } else {
            break;
          }
        }
      } else {
        temp_gold -= prac_cost;
      }
      if (temp_gold) {
        GET_TEMP_GOLD(ch) = temp_gold;
        GET_PLAT(ch) = temp_gold / 1000;
        temp_gold -= GET_PLAT(ch) * 1000;
        GET_GOLD(ch) = temp_gold / 100;
        temp_gold -= GET_GOLD(ch) * 100;
        GET_SILVER(ch) = temp_gold / 10;
        temp_gold -= GET_SILVER(ch) * 10;
        GET_COPPER(ch) = temp_gold;
      } else {
        GET_PLAT(ch) = 0;
        GET_GOLD(ch) = 0;
        GET_SILVER(ch) = 0;
        GET_COPPER(ch) = 0;
        GET_TEMP_GOLD(ch) = 0;
      }
    }
  } else {
    send_to_char("You have already learned all that you can be taught at your level.\r\n", ch);
  }

  if (GET_SKILL(ch, skill_num) >= LEARNED(ch))
    send_to_char("You are now learned in that area.\r\n", ch);

  return 1;
}

SPECIAL(dump)
{
  struct obj_data *k;
  int value = 0;

  ACMD(do_drop);

  for (k = world[ch->in_room].contents; k; k = world[ch->in_room].contents) {
    act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
    extract_obj(k);
  }

  if (!CMD_IS("drop"))
    return 0;

  do_drop(ch, argument, cmd, 0);

  for (k = world[ch->in_room].contents; k; k = world[ch->in_room].contents) {
    act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
    value += BOUNDED(1, GET_OBJ_COST(k) / 10, 50);
    extract_obj(k);
  }

  if (value) {
    act("You are awarded for outstanding performance.", FALSE, ch, 0, 0, TO_CHAR);
    act("$n has been awarded for being a good citizen.", TRUE, ch, 0, 0, TO_ROOM);

    if (GET_LEVEL(ch) < 3)
      gain_exp(ch, value);
    else {
      if (number(0, 1)) {
        GET_GOLD(ch) += 1;
        GET_TEMP_GOLD(ch) += 10;
      }
    }
  }
  return 1;
}

SPECIAL(mayor)
{
  static char open_path[] = "W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";

  static char close_path[] = "W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

  static char *path;
  static int index;
  static bool move = FALSE;

  ACMD(do_open);
  ACMD(do_lock);
  ACMD(do_unlock);
  ACMD(do_close);

  if (!move) {
    if (time_info.hours == 6) {
      move = TRUE;
      path = open_path;
      index = 0;
    } else if (time_info.hours == 20) {
      move = TRUE;
      path = close_path;
      index = 0;
    }
  }
  if (cmd || !move || (GET_POS(ch) < POS_SLEEPING) || (GET_POS(ch) == POS_FIGHTING))
    return FALSE;

  switch (path[index]) {
    case '0':
    case '1':
    case '2':
    case '3':
      perform_move(ch, path[index] - '0', 1);
      break;

    case 'W':
      GET_POS(ch) = POS_STANDING;
      act("$n awakens and groans loudly.", FALSE, ch, 0, 0, TO_ROOM);
      break;

    case 'S':
      GET_POS(ch) = POS_SLEEPING;
      act("$n lies down and instantly falls asleep.", FALSE, ch, 0, 0, TO_ROOM);
      break;

    case 'a':
      act("$n says 'Hello Honey!'", FALSE, ch, 0, 0, TO_ROOM);
      act("$n smirks.", FALSE, ch, 0, 0, TO_ROOM);
      break;

    case 'b':
      act("$n says 'What a view!  I must get something done about that dump!'", FALSE, ch, 0, 0, TO_ROOM);
      break;

    case 'c':
      act("$n says 'Vandals!  Youngsters nowadays have no respect for anything!'", FALSE, ch, 0, 0, TO_ROOM);
      break;

    case 'd':
      act("$n says 'Good day, citizens!'", FALSE, ch, 0, 0, TO_ROOM);
      break;

    case 'e':
      act("$n says 'I hereby declare the bazaar open!'", FALSE, ch, 0, 0, TO_ROOM);
      break;

    case 'E':
      act("$n says 'I hereby declare Midgaard closed!'", FALSE, ch, 0, 0, TO_ROOM);
      break;

    case 'O':
      do_unlock(ch, "gate", 0, 0);
      do_open(ch, "gate", 0, 0);
      break;

    case 'C':
      do_close(ch, "gate", 0, 0);
      do_lock(ch, "gate", 0, 0);
      break;

    case '.':
      move = FALSE;
      break;

  }

  index++;
  return FALSE;
}

/* ********************************************************************
 *  General special procedures for mobiles                             *
 ******************************************************************** */

void npc_steal(struct char_data * ch, struct char_data * victim)
{
  int coins;

  if (IS_NPC(victim))
    return;
  if (GET_LEVEL(victim) >= LVL_IMMORT)
    return;

  if (AWAKE(victim) && (number(0, GET_LEVEL(ch)) == 0)) {
    act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, victim, TO_VICT);
    act("$n tries to steal some coins from $N.", TRUE, ch, 0, victim, TO_NOTVICT);
  } else {
    /* Steal some gold coins */
    switch (number(0, 3)) {
      case 0:
        coins = (int) ((GET_PLAT(victim) * number(1, 10)) / 100);
        if (coins > 0) {
          GET_PLAT(ch) += coins;
          GET_PLAT(victim) -= coins;
          GET_TEMP_GOLD(victim) -= coins * 1000;
          snprintf(logbuffer, sizeof(logbuffer), "%s stole %d plat from %s", GET_NAME(ch), coins, GET_NAME(victim));
          mudlog(logbuffer, 'Y', COM_IMMORT, TRUE);
        }
        break;
      case 1:
        coins = (int) ((GET_GOLD(victim) * number(1, 10)) / 100);
        if (coins > 0) {
          GET_GOLD(ch) += coins;
          GET_GOLD(victim) -= coins;
          GET_TEMP_GOLD(victim) -= coins * 100;
          snprintf(logbuffer, sizeof(logbuffer), "%s stole %d gold from %s", GET_NAME(ch), coins, GET_NAME(victim));
          mudlog(logbuffer, 'Y', COM_IMMORT, TRUE);
        }
        break;
      case 2:
        coins = (int) ((GET_SILVER(victim) * number(1, 10)) / 100);
        if (coins > 0) {
          GET_SILVER(ch) += coins;
          GET_SILVER(victim) -= coins;
          GET_TEMP_GOLD(victim) -= coins * 10;
          snprintf(logbuffer, sizeof(logbuffer), "%s stole %d silver from %s", GET_NAME(ch), coins, GET_NAME(victim));
          mudlog(logbuffer, 'Y', COM_IMMORT, TRUE);
        }
        break;
      case 3:
        coins = (int) ((GET_COPPER(victim) * number(1, 10)) / 100);
        if (coins > 0) {
          GET_COPPER(ch) += coins;
          GET_COPPER(victim) -= coins;
          GET_TEMP_GOLD(victim) -= coins;
          snprintf(logbuffer, sizeof(logbuffer), "%s stole %d copper from %s", GET_NAME(ch), coins, GET_NAME(victim));
          mudlog(logbuffer, 'Y', COM_IMMORT, TRUE);
        }
        break;
    }
  }
}

SPECIAL(snake)
{
  if (cmd)
    return FALSE;

  if (GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  if (FIGHTING(ch) && (FIGHTING(ch)->in_room == ch->in_room) && (number(0, 42 - GET_LEVEL(ch)) == 0)) {
    act("$n bites $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
    act("$n bites you!", 1, ch, 0, FIGHTING(ch), TO_VICT);
    /*
     call_magic(ch, FIGHTING(ch), 0, SPELL_POISON, GET_LEVEL(ch), CAST_SPELL);
     */
    return TRUE;
  }
  return FALSE;
}

SPECIAL(thief)
{
  struct char_data *cons;

  if (cmd)
    return FALSE;

  if (GET_POS(ch) != POS_STANDING)
    return FALSE;

  for (cons = world[ch->in_room].people; cons; cons = cons->next_in_room)
    if (!IS_NPC(cons) && (GET_LEVEL(cons) < LVL_IMMORT) && (!number(0, 4))) {
      npc_steal(ch, cons);
      return TRUE;
    }
  return FALSE;
}

SPECIAL(magic_user)
{
  struct char_data *vict;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !number(0, 4))
      break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL)
    vict = FIGHTING(ch);

  /*
   if ((GET_LEVEL(ch) > 13) && (number(0, 10) == 0))
   cast_spell(ch, vict, NULL, SPELL_SLEEP);

   if ((GET_LEVEL(ch) > 7) && (number(0, 8) == 0))
   cast_spell(ch, vict, NULL, SPELL_BLINDNESS);

   if ((GET_LEVEL(ch) > 12) && (number(0, 12) == 0)) {
   if (IS_EVIL(ch))
   cast_spell(ch, vict, NULL, SPELL_ENERGY_DRAIN);
   else if (IS_GOOD(ch))
   cast_spell(ch, vict, NULL, SPELL_DISPEL_EVIL);
   }
   if (number(0, 4))
   return TRUE;

   switch (GET_LEVEL(ch)) {
   case 4:
   case 5:
   cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE);
   break;
   case 6:
   case 7:
   cast_spell(ch, vict, NULL, SPELL_CHILL_TOUCH);
   break;
   case 8:
   case 9:
   cast_spell(ch, vict, NULL, SPELL_BURNING_HANDS);
   break;
   case 10:
   case 11:
   cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP);
   break;
   case 12:
   case 13:
   cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT);
   break;
   case 14:
   case 15:
   case 16:
   case 17:
   cast_spell(ch, vict, NULL, SPELL_COLOR_SPRAY);
   break;
   default:
   cast_spell(ch, vict, NULL, SPELL_FIREBALL);
   break;
   }
   */
  return TRUE;

}

/* ********************************************************************
 *  Special procedures for mobiles                                      *
 ******************************************************************** */

SPECIAL(healer)
{
  int numspells = 4;
  int healer_costs[] = {0, 15000, 35000, 25000, 15000};
  int buying;
  struct char_data *healer = (struct char_data *) me;
  /*
   int healer_spells[5];

   healer_spells[0] = 0;
   healer_spells[1] = spells[find_spell_num("heal")].spellindex;
   healer_spells[2] = spells[find_spell_num("remove curse")].spellindex;
   healer_spells[3] = spells[find_spell_num("cure poison")].spellindex;
   healer_spells[4] = spells[find_spell_num("armor")].spellindex;
   */

  if (GET_POS(healer) == POS_FIGHTING)
    return 0;

  if (CMD_IS("list")) {
    send_to_char("The healer offers you the following services:\r\n", ch);
    send_to_char("\r\n", ch);
    send_to_char("(1)     Heal 100 hitpoints    Cost: 15000\r\n", ch);
    send_to_char("(2)     Remove curse          Cost: 35000\r\n", ch);
    send_to_char("(3)     Remove poison         Cost: 25000\r\n", ch);
    send_to_char("(4)     Armor                 Cost: 15000\r\n", ch);
    return 1;
  } else if (CMD_IS("buy")) {
    buying = atoi(argument);
    if (buying >= 1 && buying <= numspells) {
      if (GET_GOLD(ch) >= healer_costs[buying]) {
        snprintf(buf, MAX_STRING_LENGTH, "That will only cost you %d gold coins!\r\n", healer_costs[buying]);
        send_to_char(buf, ch);
        GET_GOLD(ch) -= healer_costs[buying];
        GET_TEMP_GOLD(ch) -= healer_costs[buying] * 100;
        GET_POS(healer) = POS_STANDING;
        /*
         cast_spell(healer, ch, NULL, healer_spells[buying]);
         */GET_POS(healer) = POS_SITTING;
        return 1;
      } else {
        send_to_char("You dont seem to have enough money to buy that.\r\n", ch);
        return 1;
      }
    } else {
      send_to_char("Sorry, I dont sell that. Try list and then buy #.\r\n", ch);
      return 1;
    }
  }
  return 0;
}

SPECIAL(ticket_vendor)
{
  struct char_data *guard = (struct char_data *) me;
  char *buf = "You bump into the rope, drawing an angry glance from the vendor.";
  char *buf2 = "$n tries to walk through the rope, drawing the attention of the vendor.";

  if (!IS_MOVE(cmd) || GET_LEVEL(ch) >= LVL_IMMORT || GET_POS(guard) != POS_STANDING)
    return FALSE;

  if (ch->in_room == guard->in_room && cmd == SCMD_EAST) {
    act(buf, FALSE, ch, 0, guard, TO_CHAR);
    act(buf2, FALSE, ch, 0, guard, TO_ROOM);
    return TRUE;
  }

  return FALSE;
}

SPECIAL(guild_guard)
{
  int i, flag;
  int roomnum, topzone, botzone;
  extern struct zone_data *zone_table;
  void raw_kill(struct char_data *ch, struct char_data *killer);
  void stop_fighting(struct char_data *ch);
  extern int guild_info[][3];
  struct char_data *guard = (struct char_data *) me;
  struct obj_data *corpse;
  extern const struct command_info cmd_info[];
  char *buf = "$N humiliates you, and blocks your way.";
  char *buf2 = "$N humiliates $n, and blocks $s way.";
  struct char_data *victim;

  if (!strcmp(cmd_info[cmd].command, "mprawkill") && FIGHTING(guard)) {
    victim = FIGHTING(guard);
    GET_POS(victim) = POS_DEAD;
    stop_fighting(guard);
    act("$N taunts you before chopping off your head.", FALSE, victim, 0, guard, TO_CHAR);
    act("$N taunts $n, and chops off $s head.", FALSE, victim, 0, guard, TO_ROOM);
    raw_kill(victim, guard);
    corpse = get_obj_in_list_vis(guard, "pcorpse", world[guard->in_room].contents);
    if (!corpse) {
      stderr_log("Guildguard Spec: Corpse not found!");
      return FALSE;
    }
    /* GET_OBJ_RNUM(corpse) = -1; */
    obj_from_room(corpse);
    botzone = real_room(zone_table[IN_ZONE(guard)].bottom);
    topzone = real_room(zone_table[IN_ZONE(guard)].top);
    for (i = 0; i < 10; i++)
      roomnum = number(botzone, topzone);
    obj_to_room(corpse, roomnum);
    act("$N peers around the room and says, 'Next?'", FALSE, guard, 0, guard, TO_ROOM);
    return TRUE;
  }
  if (!IS_MOVE(cmd) || GET_LEVEL(ch) >= LVL_IMMORT || GET_POS(guard) != POS_STANDING)
    return FALSE;

  flag = 0;
  for (i = 0; guild_info[i][0] != -1; i++) {
    if (world[ch->in_room].number == guild_info[i][1] && cmd == guild_info[i][2]) {
      if (!IS_NPC(ch) && GET_CLASS(ch) == guild_info[i][0])
        return FALSE;
      else
        flag = 1;
    }
  }

  if (flag) {
    act(buf, FALSE, ch, 0, guard, TO_CHAR);
    act(buf2, FALSE, ch, 0, guard, TO_ROOM);
    return TRUE;
  }

  return FALSE;
}

SPECIAL(fido)
{

  struct obj_data *i, *temp, *next_obj;

  if (cmd || !AWAKE(ch))
    return (FALSE);

  for (i = world[ch->in_room].contents; i; i = i->next_content) {
    if (GET_OBJ_TYPE(i) == ITEM_CONTAINER && GET_OBJ_VAL(i, 3)) {
      act("$n savagely devours a corpse.", FALSE, ch, 0, 0, TO_ROOM);
      for (temp = i->contains; temp; temp = next_obj) {
        next_obj = temp->next_content;
        obj_from_obj(temp);
        obj_to_room(temp, ch->in_room);
      }
      extract_obj(i);
      return (TRUE);
    }
  }
  return (FALSE);
}

SPECIAL(janitor)
{
  struct obj_data *i;

  if (cmd || !AWAKE(ch))
    return (FALSE);

  for (i = world[ch->in_room].contents; i; i = i->next_content) {
    if (!CAN_WEAR(i, ITEM_WEAR_TAKE))
      continue;
    if (GET_OBJ_TYPE(i) != ITEM_DRINKCON && GET_OBJ_COST(i) >= 15)
      continue;
    act("$n picks up some trash.", FALSE, ch, 0, 0, TO_ROOM);
    obj_from_room(i);
    obj_to_char(i, ch);
    return TRUE;
  }

  return FALSE;
}

SPECIAL(cityguard)
{
  ACMD(do_move);
  struct char_data *tch, *evil, *next_tch;
  struct char_data *hunted_ch;
  struct char_data *k, *temp;
  char tempbuf[256];
  int i;

  if (GET_POS(ch) == POS_FIGHTING && GET_POS(FIGHTING(ch)) == POS_FIGHTING && !HUNTING(ch) && !IS_NPC(FIGHTING(ch))) {
    HUNTINGRM(ch) = -1;
    HUNTING(ch) = GET_IDNUM(FIGHTING(ch));
  }

  if (cmd || !AWAKE(ch) || (GET_POS(ch) == POS_FIGHTING)) {
    return (FALSE);
  }

  if (HUNTING(ch) && GET_POS(ch) == POS_STANDING) {
    hunted_ch = find_hunted_char(HUNTING(ch));
    if (hunted_ch) {
      if (!ch) {
        return 0;
      }
      if (ch->in_room == hunted_ch->in_room) {
        act("$n says 'Be prepared to defend yourself!'", TRUE, ch, 0, 0, TO_ROOM);
        hit(ch, hunted_ch, TYPE_UNDEFINED);
        return 0;
      }
      tch = hunted_ch;
      hunt_victim(ch);
    }
    switch (number(0, 40)) {
      case 0:
        if (ch && tch) {
          snprintf(buf, MAX_STRING_LENGTH, "%s yells 'Stand arms guards, %s is a threat to the peace!!!'\r\n", GET_NAME(ch), GET_NAME(tch));
          zone_echo(ch, buf);
        }
        break;
      case 1:
        if (ch && tch) {
          snprintf(buf, MAX_STRING_LENGTH, "%s yells 'Come here %s, justice will be done!'\r\n", GET_NAME(ch), GET_NAME(tch));
          zone_echo(ch, buf);
        }
        break;
      case 2:
        if (ch && tch) {
          snprintf(buf, MAX_STRING_LENGTH, "%s yells 'You will be caught %s!!'\r\n", GET_NAME(ch), GET_NAME(tch));
          zone_echo(ch, buf);
        }
        break;
      case 3:
        if (ch && tch) {
          snprintf(buf, MAX_STRING_LENGTH, "%s yells 'Where are you %s, you cannot run forever!'\r\n", GET_NAME(ch), GET_NAME(tch));
          zone_echo(ch, buf);
        }
        break;
      case 4:
        if (ch && tch) {
          snprintf(buf, MAX_STRING_LENGTH, "%s yells '%s, you can run, but you can't hide!'\r\n", GET_NAME(ch), GET_NAME(tch));
          zone_echo(ch, buf);
        }
        break;
      default:
        break;
    }
    return 1;
  }

  if (GET_POS(ch) != POS_SLEEPING) {
    switch (number(0, 90)) {
      case 1:
        act("$n stands proud.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 2:
        act("$n yawns as $s shift draws near.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 3:
        act("$n coughs loudly.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 4:
        act("$n spits over $s left shoulder.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 5:
        act("$n smiles happily.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 6:
        act("$n paces back and forth.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 7:
        act("$n peers around, as if looking for something he can't see.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      case 8:
        act("$n glares over $s shoulder.", TRUE, ch, 0, 0, TO_ROOM);
        break;
      default:
        break;
    }
  }

  evil = 0;

  for (tch = world[ch->in_room].people; tch; tch = next_tch) {
    next_tch = tch->next_in_room;
    if (tch && !IS_NPC(tch) && CAN_SEE(ch, tch) && IS_SET(PLR_FLAGS(tch), PLR_KILLER)) {
      act("$n screams 'Draw your weapons!!! You're one of those PLAYER KILLERS!!!'", FALSE, ch, 0, 0, TO_ROOM);
      hit(ch, tch, TYPE_UNDEFINED);
      return (TRUE);
    }
  }

  for (tch = world[ch->in_room].people; tch; tch = next_tch) {
    next_tch = tch->next_in_room;
    if (tch && !IS_NPC(tch) && CAN_SEE(ch, tch) && IS_SET(PLR_FLAGS(tch), PLR_THIEF)) {
      act("$n screams 'Draw your weapons!!!  You're one of those PLAYER THIEVES!!!'", FALSE, ch, 0, 0, TO_ROOM);
      hit(ch, tch, TYPE_UNDEFINED);
      return (TRUE);
    }
  }

  for (i = 0; i < NUM_OF_DIRS; i++) {
    if (EXIT(ch, i) && EXIT(ch, i)->to_room != NOWHERE && !IS_SET(EXIT(ch, i)->exit_info, EX_CLOSED)) {
      if (world[EXIT(ch, i)->to_room].people) {
        for (evil = world[EXIT(ch, i)->to_room].people; evil; evil = evil->next_in_room) {
          if (FIGHTING(evil)) {
            if (!MOB_FLAGGED(ch, MOB_SENTINEL))
              do_move(ch, NULL, i + 1, 0);
            return (TRUE);
          }
        }
      } else if (_2ND_EXIT(ch, i) && _2ND_EXIT(ch, i)->to_room != NOWHERE && !IS_SET(_2ND_EXIT(ch, i)->exit_info, EX_CLOSED)) {
        if (world[_2ND_EXIT(ch, i)->to_room].people) {
          for (evil = world[_2ND_EXIT(ch, i)->to_room].people; evil; evil = evil->next_in_room) {
            if (FIGHTING(evil)) {
              if (!MOB_FLAGGED(ch, MOB_SENTINEL)) {
                do_move(ch, NULL, i + 1, 0);
              }
              return (TRUE);
            }
          }
        }
      }
    }
  }

  evil = NULL;
  for (tch = world[ch->in_room].people; tch; tch = next_tch) {
    next_tch = tch->next_in_room;
    if (CAN_SEE(ch, tch) && FIGHTING(tch)) {
      if (!IS_NPC(tch) || (IS_NPC(tch) && tch->master && !IS_NPC(tch->master))) {
        if (tch->master && ch->in_room == tch->master->in_room) {
          evil = tch->master;
          break;
        } else {
          evil = tch;
        }
      }
    }
  }

  if (evil && number(0, 5) && FIGHTING(evil) && IS_MOB(FIGHTING(evil)) && GET_MOB_RACE(FIGHTING(evil)) == GET_MOB_RACE(ch)) {
    if (GET_LEVEL(evil) < LVL_IMMORT) {
      act("$n screams 'STOP this fighting NOW! Peace will be maintained!'", FALSE, ch, 0, 0, TO_ROOM);
      act("$n strikes you on the head, knocking you out!.", FALSE, ch, 0, evil, TO_VICT);

      /* Commented out by Novo because Nudd is a safety Nazi
       if (FIGHTING(FIGHTING(evil))) {
       stop_fighting(FIGHTING(evil));
       }
       */

      if (FIGHTING(evil)) {
        stop_fighting(evil);
      }

      for (k = combat_list; k; k = temp) {
        temp = k->next_fighting;
        if (FIGHTING(k) == evil) {
          stop_fighting(k);
          forget(k, evil);
        }
      }

      /*Added if the char is an immortal*/
      if (COM_FLAGGED(evil, COM_ADMIN)) {
        send_to_char("The guard attempted to knock you out but failed", evil);
      } else {
        GET_POS(evil) = POS_SLEEPING;
        if (!AFF2_FLAGGED(evil, AFF2_KNOCKEDOUT)) {
          SET_BIT(AFF2_FLAGS(evil), AFF2_KNOCKEDOUT);
        }
        add_event(60, knockedout, EVENT_KNOCKEDOUT, evil, NULL, NULL, NULL, NULL, 0);
      }
    }
    return (TRUE);

  } else {
    if (evil && FIGHTING(evil) && IS_MOB(FIGHTING(evil)) && GET_MOB_RACE(FIGHTING(evil)) == GET_MOB_RACE(ch)) {
      act("$n screams 'Fighting $N is the same as fighting me!'", FALSE, ch, 0, FIGHTING(evil), TO_ROOM);
      sprintf(tempbuf, "rescue %s", GET_MOB_NAME(FIGHTING(evil)));
      command_interpreter(ch, tempbuf);
      sprintf(tempbuf, "hit %s", GET_NAME(evil));
      command_interpreter(ch, tempbuf);
      return TRUE;
    }
  }
  return (FALSE);
}

SPECIAL(pet_shops)
{
  char buf[MAX_STRING_LENGTH], pet_name[256];
  int pet_room;
  struct char_data *pet;

  pet_room = ch->in_room + 1;

  if (CMD_IS("list")) {
    send_to_char("Available pets are:\r\n", ch);
    for (pet = world[pet_room].people; pet; pet = pet->next_in_room) {
      snprintf(buf, MAX_STRING_LENGTH, "%8d - %s\r\n", 3 * GET_EXP(pet), GET_NAME(pet));
      send_to_char(buf, ch);
    }
    return (TRUE);
  } else if (CMD_IS("buy")) {

    argument = one_argument(argument, buf);
    argument = one_argument(argument, pet_name);

    if (!(pet = get_char_room(buf, pet_room))) {
      send_to_char("There is no such pet!\r\n", ch);
      return (TRUE);
    }
    if (GET_GOLD(ch) < (GET_EXP(pet) * 3)) {
      send_to_char("You don't have enough gold!\r\n", ch);
      return (TRUE);
    }
    GET_GOLD(ch) -= GET_EXP(pet) * 3;
    GET_TEMP_GOLD(ch) -= GET_EXP(pet) * 300;

    pet = read_mobile(GET_MOB_RNUM(pet), REAL);
    GET_EXP(pet) = 0;
    SET_BIT(AFF_FLAGS(pet), AFF_CHARM);

    if (*pet_name) {
      snprintf(buf, MAX_STRING_LENGTH, "%s %s", pet->player.name, pet_name);
      /* FREE(pet->player.name); don't free the prototype! */
      pet->player.name = strdup(buf);

      snprintf(buf, MAX_STRING_LENGTH, "%sA small sign on a chain around the neck says 'My name is %s'\r\n", pet->player.description, pet_name);
      /* FREE(pet->player.description); don't free the prototype! */
      pet->player.description = strdup(buf);
    }
    char_to_room(pet, ch->in_room);
    add_follower(pet, ch);

    /* Be certain that pets can't get/carry/use/wield/wear items */IS_CARRYING_W(pet) = 1000;
    IS_CARRYING_N(pet) = 100;

    send_to_char("May you enjoy your pet.\r\n", ch);
    act("$n buys $N as a pet.", FALSE, ch, 0, pet, TO_ROOM);

    return 1;
  }
  /* All commands except list and buy */
  return 0;
}

/* ********************************************************************
 *  Special procedures for objects                                     *
 ******************************************************************** */

SPECIAL(slot_machine)
{
  long amount;
  int lucky;

  if (CMD_IS("insert")) {
    if ((amount = atoi(argument)) <= 0) {
      send_to_char("Are you sure you wanted to play?\r\n", ch);
      act("The lights on the slot-machine flash in brilliant patterns.", TRUE, ch, 0, FALSE, TO_ROOM);
      return 1;
    }
    if (amount > 0) {
      if (GET_GOLD(ch) < amount) {
        send_to_char("You are too broke to play with that amount.\r\n", ch);
        act("The lights on the slot-machine flash in brilliant patterns.", TRUE, ch, 0, FALSE, TO_ROOM);
        return 1;
      }
      GET_GOLD(ch) -= amount;
      GET_TEMP_GOLD(ch) -= amount * 100;
      snprintf(buf, MAX_STRING_LENGTH, "You put the coins in the slot and pull the lever.\r\n");
      send_to_char(buf, ch);
      act("$n pulls the lever on the slot-machine.", TRUE, ch, 0, FALSE, TO_ROOM);
      if ((lucky = number(0, 100)) <= 1) {
        snprintf(buf, MAX_STRING_LENGTH, "You are a winner! You won %ld coins!\r\n", amount * 100);
        send_to_char(buf, ch);
        GET_GOLD(ch) += amount + amount * 100;
        GET_TEMP_GOLD(ch) += (amount + amount * 100) * 100;
        act("The lights on the slot-machine flash in brilliant patterns.", TRUE, ch, 0, FALSE, TO_ROOM);
        return 1;
      } else if (lucky > 1 && lucky <= 5) {
        snprintf(buf, MAX_STRING_LENGTH, "You are a winner! You won %ld coins!\r\n", amount * 5);
        send_to_char(buf, ch);
        GET_GOLD(ch) += amount + amount * 5;
        GET_TEMP_GOLD(ch) += (amount + amount * 5) * 100;
        act("The lights on the slot-machine flash in brilliant patterns.", TRUE, ch, 0, FALSE, TO_ROOM);
        return 1;
      } else if (lucky > 5 && lucky <= 15) {
        snprintf(buf, MAX_STRING_LENGTH, "You are a winner! You won %ld coins!\r\n", amount);
        send_to_char(buf, ch);
        GET_GOLD(ch) += amount;
        GET_TEMP_GOLD(ch) += amount * 100;
        act("The lights on the slot-machine flash in brilliant patterns.", TRUE, ch, 0, FALSE, TO_ROOM);
        return 1;
      } else if (lucky > 15) {
        snprintf(buf, MAX_STRING_LENGTH, "You are a loser! You lost %ld coins!\r\n", amount);
        send_to_char(buf, ch);
        act("The lights on the slot-machine flash in brilliant patterns.", TRUE, ch, 0, FALSE, TO_ROOM);
        return 1;
      }
      return 1;
    }
    return 1;
  }
  return 0;
}

SPECIAL(bank)
{
  int amount;
  int is_plat = 0;
  int is_gold = 0;
  int is_silver = 0;
  int is_copper = 0;
  char value[50];
  char type1[50];
  char type2[50];
  int has_money = GET_BANK_PLAT(ch) || GET_BANK_GOLD(ch) || GET_BANK_SILVER(ch) || GET_BANK_COPPER(ch);
  struct obj_data *obj = (struct obj_data*) me;

  if (CMD_IS("balance")) {
    if (has_money)
      snprintf(buf, MAX_STRING_LENGTH, "Your current balance is {W%d platinum {Y%d gold {w%d silver {y%d copper{x.\r\n", GET_BANK_PLAT(ch), GET_BANK_GOLD(ch), GET_BANK_SILVER(ch), GET_BANK_COPPER(ch));
    else
      snprintf(buf, MAX_STRING_LENGTH, "You currently have no money deposited.\r\n");
    send_to_char(buf, ch);
    return 1;
  } else if (CMD_IS("deposit")) {
    two_arguments(argument, value, type1);
    if ((amount = atoi(value)) <= 0) {
      send_to_char("How much do you want to deposit?\r\n", ch);
      return 1;
    }
    if (type1[0] == '\0') {
      send_to_char("Yes but what do you want to deposit?\r\n", ch);
      return 1;
    }
    switch (type1[0]) {
      case 'P':
      case 'p':
        if (GET_PLAT(ch) < amount) {
          send_to_char("You don't have that much platinum!\r\n", ch);
          return 1;
        }
        GET_PLAT(ch) -= amount;
        GET_TEMP_GOLD(ch) -= amount * 1000;
        GET_BANK_PLAT(ch) += amount;
        snprintf(buf, MAX_STRING_LENGTH, "You deposit {W%d platinum{x.\r\n", amount);
        send_to_char(buf, ch);
        act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
        return 1;
      case 'G':
      case 'g':
        if (GET_GOLD(ch) < amount) {
          send_to_char("You don't have that much gold!\r\n", ch);
          return 1;
        }
        GET_GOLD(ch) -= amount;
        GET_TEMP_GOLD(ch) -= amount * 100;
        GET_BANK_GOLD(ch) += amount;
        snprintf(buf, MAX_STRING_LENGTH, "You deposit {Y%d gold{x.\r\n", amount);
        send_to_char(buf, ch);
        act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
        return 1;
      case 'S':
      case 's':
        if (GET_SILVER(ch) < amount) {
          send_to_char("You don't have that much silver!\r\n", ch);
          return 1;
        }
        GET_SILVER(ch) -= amount;
        GET_TEMP_GOLD(ch) -= amount * 10;
        GET_BANK_SILVER(ch) += amount;
        snprintf(buf, MAX_STRING_LENGTH, "You deposit {w%d silver{x.\r\n", amount);
        send_to_char(buf, ch);
        act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
        return 1;
      case 'C':
      case 'c':
        if (GET_COPPER(ch) < amount) {
          send_to_char("You don't have that much copper!\r\n", ch);
          return 1;
        }
        GET_COPPER(ch) -= amount;
        GET_TEMP_GOLD(ch) -= amount;
        GET_BANK_COPPER(ch) += amount;
        snprintf(buf, MAX_STRING_LENGTH, "You deposit {y%d copper{x.\r\n", amount);
        send_to_char(buf, ch);
        act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
        return 1;
    }
  } else if (CMD_IS("withdraw")) {
    two_arguments(argument, value, type1);
    if ((amount = atoi(value)) <= 0) {
      send_to_char("How much do you want to withdraw?\r\n", ch);
      return 1;
    }
    if (type1[0] == '\0') {
      send_to_char("Yes but what do you want to withdraw?\r\n", ch);
      return 1;
    }
    switch (type1[0]) {
      case 'P':
      case 'p':
        if (GET_BANK_PLAT(ch) < amount) {
          send_to_char("You don't have that much platinum deposited!\r\n", ch);
          return 1;
        }
        GET_PLAT(ch) += amount;
        GET_TEMP_GOLD(ch) += amount * 1000;
        GET_BANK_PLAT(ch) -= amount;
        snprintf(buf, MAX_STRING_LENGTH, "You withdraw {W%d platinum{x.\r\n", amount);
        send_to_char(buf, ch);
        act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
        return 1;
      case 'G':
      case 'g':
        if (GET_BANK_GOLD(ch) < amount) {
          send_to_char("You don't have that much gold deposited!\r\n", ch);
          return 1;
        }
        GET_GOLD(ch) += amount;
        GET_TEMP_GOLD(ch) += amount * 100;
        GET_BANK_GOLD(ch) -= amount;
        snprintf(buf, MAX_STRING_LENGTH, "You withdraw {Y%d gold{x.\r\n", amount);
        send_to_char(buf, ch);
        act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
        return 1;
      case 'S':
      case 's':
        if (GET_BANK_SILVER(ch) < amount) {
          send_to_char("You don't have that much silver deposited!\r\n", ch);
          return 1;
        }
        GET_SILVER(ch) += amount;
        GET_TEMP_GOLD(ch) += amount * 10;
        GET_BANK_SILVER(ch) -= amount;
        snprintf(buf, MAX_STRING_LENGTH, "You withdraw {w%d silver{x.\r\n", amount);
        send_to_char(buf, ch);
        act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
        return 1;
      case 'C':
      case 'c':
        if (GET_BANK_COPPER(ch) < amount) {
          send_to_char("You don't have that much copper deposited!\r\n", ch);
          return 1;
        }
        GET_COPPER(ch) += amount;
        GET_TEMP_GOLD(ch) += amount;
        GET_BANK_COPPER(ch) -= amount;
        snprintf(buf, MAX_STRING_LENGTH, "You withdraw {y%d copper{x.\r\n", amount);
        send_to_char(buf, ch);
        act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
        return 1;
    }
  } else if (CMD_IS("exchange")) {
    three_arguments(argument, value, type1, type2);
    if ((amount = atoi(value)) <= 0) {
      send_to_char("How much do you want to exchange?\r\n", ch);
      return 1;
    }
    if (type1[0] == '\0') {
      send_to_char("Yes but what do you want to exchange from?\r\n", ch);
      return 1;
    }
    if (type2[0] == '\0') {
      send_to_char("Yes but what do you want to exchange to?\r\n", ch);
      return 1;
    }
    if (type1[0] == type2[0]) {
      send_to_char("That would be pointless wouldn't it?\r\n", ch);
      return 1;
    }
    switch (type1[0]) {
      case 'P':
      case 'p':
        if (GET_PLAT(ch) < amount) {
          send_to_char("You don't have that much platinum to exchange.\r\n", ch);
          return 1;
        }
        GET_PLAT(ch) -= amount;
        GET_TEMP_GOLD(ch) -= amount * 1000;
        is_plat = 1;
        break;
      case 'G':
      case 'g':
        if (GET_GOLD(ch) < amount) {
          send_to_char("You don't have that much gold to exchange.\r\n", ch);
          return 1;
        } else if ((type2[0] == 'p' || type2[0] == 'P') && amount < (10 + GET_OBJ_SVAL(obj, 0))) {
          send_to_char("You need more gold to complete that exchange.\r\n", ch);
          return 1;
        }
        GET_GOLD(ch) -= amount;
        GET_TEMP_GOLD(ch) -= amount * 100;
        is_gold = 1;
        break;
      case 'S':
      case 's':
        if (GET_SILVER(ch) < amount) {
          send_to_char("You don't have that much silver to exchange.\r\n", ch);
          return 1;
        } else if ((type2[0] == 'p' || type2[0] == 'P') && amount < (100 + (10 * GET_OBJ_SVAL(obj, 0)))) {
          send_to_char("You need more silver to complete that exchange.\r\n", ch);
          return 1;
        } else if ((type2[0] == 'g' || type2[0] == 'G') && amount < (10 + GET_OBJ_SVAL(obj, 0))) {
          send_to_char("You need more silver to complete that exchange.\r\n", ch);
          return 1;
        }
        GET_SILVER(ch) -= amount;
        GET_TEMP_GOLD(ch) -= amount * 10;
        is_silver = 1;
        break;
      case 'C':
      case 'c':
        if (GET_COPPER(ch) < amount) {
          send_to_char("You don't have that much copper to exchange.\r\n", ch);
          return 1;
        } else if ((type2[0] == 'p' || type2[0] == 'P') && (amount < (1000 + (100 * GET_OBJ_SVAL(obj, 0))))) {
          send_to_char("You need more copper to complete that exchange.\r\n", ch);
          return 1;
        } else if ((type2[0] == 'g' || type2[0] == 'G') && (amount < (100 + (10 * GET_OBJ_SVAL(obj, 0))))) {
          send_to_char("You need more copper to complete that exchange.\r\n", ch);
          return 1;
        } else if ((type2[0] == 's' || type2[0] == 'S') && (amount < (10 + GET_OBJ_SVAL(obj, 0)))) {
          send_to_char("You need more copper to complete that exchange.\r\n", ch);
          return 1;
        }
        GET_COPPER(ch) -= amount;
        GET_TEMP_GOLD(ch) -= amount;
        is_copper = 1;
        break;
      default: {
        send_to_char("What kind of money is that?\r\n", ch);
        return 1;
      }
    }
    switch (type2[0]) {
      case 'P':
      case 'p':
        if (is_gold && (amount >= (10 + GET_OBJ_SVAL(obj, 0)))) {
          snprintf(buf, MAX_STRING_LENGTH, "You exchange {Y%d gold{x for {W%d platinum{x.\r\n", amount - (amount % (10 + GET_OBJ_SVAL(obj, 0))), amount / (10 + GET_OBJ_SVAL(obj, 0)));
          send_to_char(buf, ch);
          GET_PLAT(ch) += amount / (10 + GET_OBJ_SVAL(obj, 0));
          GET_GOLD(ch) += amount % (10 + GET_OBJ_SVAL(obj, 0));
          GET_TEMP_GOLD(ch) += (amount * 1000) / (10 + GET_OBJ_SVAL(obj, 0));
          GET_TEMP_GOLD(ch) += (amount * 100) % (10 + GET_OBJ_SVAL(obj, 0));
        } else if (is_silver && (amount >= (100 + (GET_OBJ_SVAL(obj, 0) * 10)))) {
          snprintf(buf, MAX_STRING_LENGTH, "You exchange {w%d silver{x for {W%d platinum{x.\r\n", amount - (amount % (100 + (10 * GET_OBJ_SVAL(obj, 0)))), amount / (100 + (10 * GET_OBJ_SVAL(obj, 0))));
          send_to_char(buf, ch);
          GET_PLAT(ch) += amount / (100 + (10 * GET_OBJ_SVAL(obj, 0)));
          GET_SILVER(ch) += amount % (100 + (10 * GET_OBJ_SVAL(obj, 0)));
          GET_TEMP_GOLD(ch) += (amount * 1000) / (10 + GET_OBJ_SVAL(obj, 0));
          GET_TEMP_GOLD(ch) += (amount * 10) % (10 + GET_OBJ_SVAL(obj, 0));
        } else if (is_copper && (amount >= (1000 + (GET_OBJ_SVAL(obj, 0) * 100)))) {
          snprintf(buf, MAX_STRING_LENGTH, "You exchange {y%d copper{x for {W%d platinum{x.\r\n", amount - (amount % (1000 + (100 * GET_OBJ_SVAL(obj, 0)))), amount / (1000 + (100 * GET_OBJ_SVAL(obj, 0))));
          send_to_char(buf, ch);
          GET_PLAT(ch) += amount / (1000 + (100 * GET_OBJ_SVAL(obj, 0)));
          GET_COPPER(ch) += amount % (1000 + (100 * GET_OBJ_SVAL(obj, 0)));
          GET_TEMP_GOLD(ch) += (amount * 1000) / (10 + GET_OBJ_SVAL(obj, 0));
          GET_TEMP_GOLD(ch) += amount % (10 + GET_OBJ_SVAL(obj, 0));
        }
        break;
      case 'G':
      case 'g':
        if (is_plat) {
          snprintf(buf, MAX_STRING_LENGTH, "You exchange {W%d platinum{x for {Y%d gold{x.\r\n", amount, amount * (10 - GET_OBJ_SVAL(obj, 0) / 2));
          send_to_char(buf, ch);
          GET_GOLD(ch) += amount * (10 - GET_OBJ_SVAL(obj, 0) / 2);
          GET_TEMP_GOLD(ch) += (amount * 1000) * (10 - GET_OBJ_SVAL(obj, 0) / 2);
        } else if (is_silver && (amount >= (10 + GET_OBJ_SVAL(obj, 0)))) {
          snprintf(buf, MAX_STRING_LENGTH, "You exchange {w%d silver{x for {Y%d gold{x.\r\n", amount - (amount % (10 + GET_OBJ_SVAL(obj, 0))), amount / (10 + GET_OBJ_SVAL(obj, 0)));
          send_to_char(buf, ch);
          GET_GOLD(ch) += amount / (10 + GET_OBJ_SVAL(obj, 0));
          GET_SILVER(ch) += amount % (10 + GET_OBJ_SVAL(obj, 0));
          GET_TEMP_GOLD(ch) += (amount * 100) / (10 + GET_OBJ_SVAL(obj, 0));
          GET_TEMP_GOLD(ch) += (amount * 10) % (10 + GET_OBJ_SVAL(obj, 0));
        } else if (is_copper && (amount >= (100 + (GET_OBJ_SVAL(obj, 0) * 10)))) {
          snprintf(buf, MAX_STRING_LENGTH, "You exchange {y%d copper{x for {Y%d gold{x.\r\n", amount - (amount % (100 + (10 * GET_OBJ_SVAL(obj, 0)))), amount / (100 + (10 * GET_OBJ_SVAL(obj, 0))));
          send_to_char(buf, ch);
          GET_GOLD(ch) += amount / (100 + (10 * GET_OBJ_SVAL(obj, 0)));
          GET_COPPER(ch) += amount % (100 + (10 * GET_OBJ_SVAL(obj, 0)));
          GET_TEMP_GOLD(ch) += (amount * 100) / (10 + GET_OBJ_SVAL(obj, 0));
          GET_TEMP_GOLD(ch) += amount % (10 + GET_OBJ_SVAL(obj, 0));
        }
        break;
      case 'S':
      case 's':
        if (is_plat) {
          snprintf(buf, MAX_STRING_LENGTH, "You exchange {W%d platinum{x for {w%d silver{x.\r\n", amount, amount * (100 - (10 * GET_OBJ_SVAL(obj, 0) / 2)));
          send_to_char(buf, ch);
          GET_SILVER(ch) += amount * (100 - (10 * GET_OBJ_SVAL(obj, 0) / 2));
          GET_TEMP_GOLD(ch) += (amount * 10) * (100 - (10 * GET_OBJ_SVAL(obj, 0) / 2));
        } else if (is_gold) {
          snprintf(buf, MAX_STRING_LENGTH, "You exchange {Y%d gold{x for {w%d silver{x.\r\n", amount, amount * (10 - GET_OBJ_SVAL(obj, 0) / 2));
          send_to_char(buf, ch);
          GET_SILVER(ch) += amount * (10 - GET_OBJ_SVAL(obj, 0) / 2);
          GET_TEMP_GOLD(ch) += (amount * 10) * (10 - GET_OBJ_SVAL(obj, 0) / 2);
        } else if (is_copper && (amount >= (10 + GET_OBJ_SVAL(obj, 0)))) {
          snprintf(buf, MAX_STRING_LENGTH, "You exchange {y%d copper{x for {w%d silver{x.\r\n", amount - (amount % (10 + GET_OBJ_SVAL(obj, 0))), amount / (10 + GET_OBJ_SVAL(obj, 0)));
          send_to_char(buf, ch);
          GET_SILVER(ch) += amount / (10 + GET_OBJ_SVAL(obj, 0));
          GET_COPPER(ch) += amount % (10 + GET_OBJ_SVAL(obj, 0));
          GET_TEMP_GOLD(ch) += (amount * 10) / (10 + GET_OBJ_SVAL(obj, 0));
          GET_TEMP_GOLD(ch) += amount % (10 + GET_OBJ_SVAL(obj, 0));
        }
        break;
      case 'C':
      case 'c':
        if (is_plat) {
          snprintf(buf, MAX_STRING_LENGTH, "You exchange {W%d platinum{x for {y%d copper{x.\r\n", amount, amount * (1000 - (100 * GET_OBJ_SVAL(obj, 0) / 2)));
          send_to_char(buf, ch);
          GET_COPPER(ch) += amount * (1000 - (100 * GET_OBJ_SVAL(obj, 0) / 2));
          GET_TEMP_GOLD(ch) += amount * (1000 - (100 * GET_OBJ_SVAL(obj, 0) / 2));
        } else if (is_gold) {
          snprintf(buf, MAX_STRING_LENGTH, "You exchange {Y%d gold{x for {y%d copper{x.\r\n", amount, amount * (100 - (10 * GET_OBJ_SVAL(obj, 0) / 2)));
          send_to_char(buf, ch);
          GET_COPPER(ch) += amount * (100 - (10 * GET_OBJ_SVAL(obj, 0) / 2));
          GET_TEMP_GOLD(ch) += amount * (100 - (10 * GET_OBJ_SVAL(obj, 0) / 2));
        } else if (is_silver) {
          snprintf(buf, MAX_STRING_LENGTH, "You exchange {w%d silver{x for {y%d copper{x.\r\n", amount, amount * (10 - GET_OBJ_SVAL(obj, 0) / 2));
          send_to_char(buf, ch);
          GET_COPPER(ch) += amount * (10 - GET_OBJ_SVAL(obj, 0) / 2);
          GET_TEMP_GOLD(ch) += amount * (10 - GET_OBJ_SVAL(obj, 0) / 2);
        }
        break;
      default: {
        send_to_char("What kind of money is that?\r\n", ch);
        return 1;
      }
    }
    return 1;
  } else
    return 0;
  return 1;
}

SPECIAL(toggle_qa)
{
  struct char_data *pl;

  if (CMD_IS("use") && COM_FLAGGED(ch, COM_QUEST)) {
    two_arguments(argument, arg, buf);
    if (!(str_cmp("funny", arg)) || !(str_cmp("thing", arg))) {
      if (!*buf) {
        send_to_char("Who do you want to use the thing on?\r\n", ch);
        return 1;
      }
      if (!(pl = get_char_room_vis(ch, buf))) {
        send_to_char("That person doesn't seem to be here.\r\n", ch);
        return 1;
      }
      if (IS_NPC(pl)) {
        send_to_char("Give a mob QA power? Nah! I dont think so!\r\n", ch);
        return 1;
      }
      TOGGLE_BIT(COM_FLAGS(pl), COM_QUEST);
      act("$N uses a funny-looking thing on you! You scream in disgust!", FALSE, pl, 0, ch, TO_CHAR);
      act("You use a funny-looking thing on $N!", FALSE, ch, 0, pl, TO_CHAR);
      return 1;

    } else
      return 0;

  }
  return 0;
}

/* ========= Mahn-Tor specials ============== */

SPECIAL(mist) /* all objects in room should disappear in the mist */
{
  struct obj_data *k;

  for (k = world[ch->in_room].contents; k; k = world[ch->in_room].contents) {
    act("$p is enveloped in mist and disappears!", FALSE, 0, k, 0, TO_ROOM);
    extract_obj(k);
  }

  return 0;
}

SPECIAL(arctic) /* all objects in room destroyed by harsh cold wind */
{
  struct obj_data *k;

  for (k = world[ch->in_room].contents; k; k = world[ch->in_room].contents) {
    act("$p is disintegrated by a harsh cold wind!", FALSE, 0, k, 0, TO_ROOM);
    extract_obj(k);
  }

  return 0;
}

/* ====== Oceania Specials ======== */

SPECIAL(serpent) /* Fireball, 5% on people below lvl-10, 40% on people above */
{
  struct char_data *vict;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !number(0, 4))
      break;

  if (vict == NULL)
    vict = FIGHTING(ch);

  /*
   if ((GET_LEVEL(vict) >= 10) && (number(0, 100) < 40))
   cast_spell(ch, vict, NULL, SPELL_FIREBALL);

   if ((GET_LEVEL(vict) < 10) && (number(0, 100) < 6))
   cast_spell(ch, vict, NULL, SPELL_FIREBALL);
   */

  return TRUE;

}

SPECIAL(leviathan) /* 50% cure critic if low, casts tidal wave 10% */
{

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  /*
   if (GET_HIT(ch) < 50 && number(0, 100) < 80)   80% cure critic chance below 50hp
   cast_spell(ch, ch, NULL, SPELL_CURE_CRITIC);

   if (number(0, 100) < 11)
   cast_spell(ch, ch, NULL, SPELL_TIDAL_WAVE);
   */

  return TRUE;
}

/* SPECIALS FOR TUNNELS OF SIN */

int called_this_fight;
SPECIAL(slave)
{
  int i;
  if (cmd)
    return FALSE;
  if (world[ch->in_room].people->next_in_room) {

    /*Note: All the return TRUEs were done in this format to save execution   */
    /*      Time. Ie. Condition i==0 is true, no point in checking the others.*/
    /*      Not a big deal, but hey every clock cycle counts...*/
    i = (rand() % 4);
    if (i == 0) {
      act("$n says 'Please rescue us!'", FALSE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
    if (i == 1) {
      act("$n says 'We've been down here forever!'", FALSE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
    if (i == 2) {
      act("$n says 'Do you serve the Minotaur too?'", FALSE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
    if (i == 3) {
      act("$n groans as he struggles with the corpse.", FALSE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
  } else
    return FALSE;
  return FALSE;
}

/* Troglodyte worker special, another lame talking mob with a bit of randomness */SPECIAL(troglodyte)
{
  int i;
  if (cmd)
    return FALSE;
  if (world[ch->in_room].people->next_in_room) {

    /*Note: All the return TRUEs were done in this format to save execution   */
    /*      Time. Ie. Condition i==0 is true, no point in checking the others.*/
    /*      Not a big deal, but hey every clock cycle counts...*/
    i = (rand() % 4);
    if ((i == 0) && (GET_POS(ch) > POS_SLEEPING)) {
      act("$n says 'What are you doing here?!'", FALSE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
    if ((i == 1) && (GET_POS(ch) > POS_SLEEPING)) {
      act("$n says 'Get out of here before I KILL you!'", FALSE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
    if ((i == 2) && (GET_POS(ch) > POS_SLEEPING)) {
      act("$n sees you, and a smile comes to his face as he licks his lips.", FALSE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
    if ((i == 3) && (GET_POS(ch) > POS_SLEEPING)) {
      act("$n snarls wildly at you!", FALSE, ch, 0, 0, TO_ROOM);
      return TRUE;
    }
  } else
    return FALSE;
  return FALSE;
}
/* Special process for black dragon's acid spray */SPECIAL(black_dragon)
{
  struct char_data *vict;
  int dam;

  /* The following if condition ensures that the first time the special hits */
  /* the dragon will use his spray. This is SOMEWHAT consistant with         */
  /* dragon-lore which states that a dragon's first attack is always its     */
  /* breath if it intends to kill.                                           */
  if (GET_POS(ch) != POS_FIGHTING)
    called_this_fight = 0;

  if (cmd)
    return FALSE;
  if (GET_POS(ch) != POS_FIGHTING)
    return FALSE;
  if (!FIGHTING(ch))
    return FALSE;

  /* Dragon will spray 33% of the time */
  if (((rand() % 100) <= 33) || (called_this_fight == 0)) {
    act("Green fumes rise from the nostrils of the Black Dragon, and he slowly draws \nhis head backwards. With a sudden thrust forward, he opens his mouth and \nshowers the room with a spray of green acid! ", FALSE, ch, 0, 0, TO_ROOM);

    /* Hit each mortal in the room with the spray*/
    for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room) {
      if ((GET_LEVEL(vict) < LVL_IMMORT) && (vict != ch)) {
        act("You scream in pain as the acid burns your skin!", FALSE, ch, 0, vict, TO_VICT);
        act("$N screams in pain from the burning acid!", FALSE, ch, 0, vict, TO_NOTVICT);
        dam = number(15, 40);
        damage(ch, vict, dam, -1, 0, DAM_ACID, 0, 1); /*-1 To prevent a dammage */
        /*message from being generated */
        /*I think this should work with*/
        /*no problems....              */
      }

    }
    /* This line prevents the special from spraying the next time round... */
    called_this_fight = called_this_fight + 1;
  }

  return FALSE;
}

/* Special proc for the dark being, random spell to random opponent. */SPECIAL(dark_being)
{
  struct char_data *vict;
  int i, p;

  if (cmd)
    return FALSE;
  if (GET_POS(ch) != POS_FIGHTING)
    return FALSE;
  if (!FIGHTING(ch))
    return FALSE;

  i = 0;

  /* How many PC's do we have in the room fighting the Being? */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room) {
    if (FIGHTING(vict) == ch)
      i = i + 1;
  }

  /* Pick some sucker and set the vict pointer to him */
  i = (rand() % i) + 1;
  p = 0;

  for (vict = world[ch->in_room].people; 1; vict = vict->next_in_room) {
    if (FIGHTING(vict) == ch)
      p = p + 1;
    if (p == i)
      break;
  }

  /* How we gonna play with em? */
  i = (rand() % 3);

  if (i == 0) {
    act("$n claps his hands above his head and conjures a lightning bolt!", FALSE, ch, 0, 0, TO_ROOM);
    /*
     cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT);
     */
    return TRUE;
  }

  if (i == 1) {
    act("$n slowly waves his hand before your eyes!", FALSE, ch, 0, vict, TO_VICT);
    act("$n slowly waves his hand before $N's eyes!", FALSE, ch, 0, vict, TO_NOTVICT);
    /*
     cast_spell(ch, vict, NULL, SPELL_SLEEP);
     */
    return TRUE;
  }
  if (i == 2) {
    act("$n quickly throws back the hood of his robe revealing a horid face  and a pair of flaming red eyes!", FALSE, ch, 0, 0, TO_ROOM);
    /*
     cast_spell(ch, vict, NULL, SPELL_BLINDNESS);
     */
    return TRUE;
  }
  return FALSE;
}

/* Black sould special, lame talking mob deal. */SPECIAL(black_soul)
{

  if (cmd)
    return FALSE;

  /*No point in going on if the room is empty*/
  if (world[ch->in_room].people->next_in_room) {
    /*Can't have a fighting or resting soul floating around can we?*/
    if (GET_POS(ch) == POS_STANDING) {
      act("$n moans as it floats about the room.", FALSE, ch, 0, 0, TO_ROOM);
      return TRUE;
    } else
      return FALSE;
  }
  return FALSE;
}

/* This special makes sure that noone can leave the area with one of the
 keys in his/her possesion. (Note! Spec_procs are normally not called
 when the object is in a container) */

SPECIAL(tunnel_key)
{
  struct obj_data *key;

  if (world[ch->in_room].number < 5800 || world[ch->in_room].number > 5887) {
    key = me;
    if (key->in_room == NOWHERE) { /* carried by player */
      act("Suddenly, $p begins to glow bright red.\r\nIt becomes extremely hot, and you are forced to drop it. When it\r\nhit the ground, it explodes into a pile of ashes!", FALSE, ch, key, 0, TO_CHAR);
      act("$n screams loudly as he drops $p.", FALSE, ch, key, 0, TO_ROOM);
    } else { /* this should not happen very often, only when loaded by an immo :) */
      act("Suddenly, $p begins to glow bright red and\r\nexplodes into a pile of ashes.", FALSE, 0, key, 0, TO_ROOM);
    }
    extract_obj(key);
  }

  return FALSE;
}

/* The gargoyles that comes to life when someone tries to open the golden door
 This routine triggers when someone types open with "golden" or "door" as
 argument. (This is a room spec_proc, and only runs when obj #5835 is loaded
 into the room) */

SPECIAL(gargoyle)
{
  struct obj_data *k;
  struct char_data *mob;

  if (!CMD_IS("open"))
    return FALSE;

  one_argument(argument, arg);

  if (!str_cmp("golden", arg) || !str_cmp("door", arg)) { /* trying to open golden door */

    for (k = world[ch->in_room].contents; k; k = k->next_content) {
      if (GET_OBJ_VNUM(k) == 5835) {
        send_to_char("As you reach to open the golden door, you suddenly realize that the\r\ntwo demon statues are not real at all! With two loud shrieks, their\r\nwings start to flap, and in no time you see 8 sets of razor-sharp\r\ntalons flying straight at your face!\r\n", ch);
        act("As $n reach to open the golden door, you suddenly realize that the\r\ntwo demon statues are not real at all! With two loud shrieks, their\r\nwings start to flap, and in no time you see 8 sets of razor-sharp\r\ntalons flying straight at $n's face!", FALSE, ch, 0, 0, TO_ROOM);
        extract_obj(k);
        mob = read_mobile(5803, VIRTUAL);
        char_to_room(mob, ch->in_room);
        hit(mob, ch, TYPE_UNDEFINED);
        mob = read_mobile(5804, VIRTUAL);
        char_to_room(mob, ch->in_room);
        hit(mob, ch, TYPE_UNDEFINED);
        return TRUE;
      }
    }
  }
  return FALSE;
}

/* I know this one sucks, I'll work on it later */

SPECIAL(mist_room)
{

  if (time_info.hours < 9) {
    world[real_room(5804)].dir_option[NORTH]->exit_info = 0;
    world[real_room(5805)].dir_option[SOUTH]->exit_info = 0;
    world[real_room(5851)].dir_option[DOWN]->exit_info = 14;
    world[real_room(5877)].dir_option[UP]->exit_info = 14;
    world[real_room(5848)].dir_option[EAST]->exit_info = 14;
    world[real_room(5849)].dir_option[WEST]->exit_info = 14;
    return FALSE;
  } else if (time_info.hours < 17) {
    world[real_room(5804)].dir_option[NORTH]->exit_info = 14;
    world[real_room(5805)].dir_option[SOUTH]->exit_info = 14;
    world[real_room(5851)].dir_option[DOWN]->exit_info = 0;
    world[real_room(5877)].dir_option[UP]->exit_info = 0;
    world[real_room(5848)].dir_option[EAST]->exit_info = 14;
    world[real_room(5849)].dir_option[WEST]->exit_info = 14;
    return FALSE;
  } else {
    world[real_room(5804)].dir_option[NORTH]->exit_info = 14;
    world[real_room(5805)].dir_option[SOUTH]->exit_info = 14;
    world[real_room(5851)].dir_option[DOWN]->exit_info = 14;
    world[real_room(5877)].dir_option[UP]->exit_info = 14;
    world[real_room(5848)].dir_option[EAST]->exit_info = 0;
    world[real_room(5849)].dir_option[WEST]->exit_info = 0;
  }
  return FALSE;
}

SPECIAL(pentagram)
{
  struct obj_data *obj;
  static byte dropped;
  int j, vnums[] = {5807, 5822, 5808}; /* items that should be dropped */

  if (CMD_IS("enter")) { /* someone trying to enter portal? */

    one_argument(argument, arg);
    /* make sure object 5840 is in the room first */
    for (obj = world[ch->in_room].contents; obj; obj = obj->next_content) {
      if (GET_OBJ_VNUM(obj) == 5840) {
        if (!str_cmp("ring", arg) || !str_cmp("fire", arg)) {
          act("$n enters $p.", TRUE, ch, obj, 0, TO_ROOM);
          if (GET_DRAGGING(ch)) {
            snprintf(buf, MAX_STRING_LENGTH, "%s drags $p along.", GET_NAME(ch));
            act(buf, TRUE, 0, GET_DRAGGING(ch), 0, TO_ROOM);
          }
          char_from_room(ch);
          char_to_room(ch, real_room(5887));
          if (GET_DRAGGING(ch)) {
            obj_from_room(GET_DRAGGING(ch));
            obj_to_room(GET_DRAGGING(ch), ch->in_room);
            snprintf(buf, MAX_STRING_LENGTH, "You drag $p along with you.");
            act(buf, FALSE, ch, GET_DRAGGING(ch), 0, TO_CHAR);
            snprintf(buf, MAX_STRING_LENGTH, "%s drags $p along with $m.", GET_NAME(ch));
            act(buf, TRUE, ch, GET_DRAGGING(ch), 0, TO_ROOM);
          }
          look_at_room(ch, 0);
          return TRUE;
        } else
          return FALSE;
      }
    }
    return FALSE;
  }

  if (!CMD_IS("drop") || dropped == 7) /* return if pentagram already completed */
    return FALSE;

  one_argument(argument, arg);
  /* if anything goes wrong, leave control to standard *
   * drop routines, ie - return FALSE                  */

  if (!*arg)
    return FALSE;

  if (!(obj = get_obj_in_list_vis(ch, arg, ch->carrying)))
    return FALSE;

  for (j = 0; j < 3; j++) {
    if (vnums[j] == GET_OBJ_VNUM(obj))
      break;
  }
  if (vnums[j] != GET_OBJ_VNUM(obj)) /* nopes, return control to standard drop routine */
    return FALSE;
  act("As you drop $p it slowly rolls to one of\r\nthe points in the incompleted pentagram.", FALSE, ch, obj, 0, TO_CHAR);
  act("As $n drop $p it slowly rolls\r\nto one of the points in the incompleted pentagram.", FALSE, ch, obj, 0, TO_ROOM);

  obj_from_char(obj);
  extract_obj(obj);
  SET_BIT(dropped, (1 << j));
  /* set object as dropped */
  if (dropped == 7) { /* has all needed items been dropped? */
    send_to_room("As the pentagram is completed, it starts to glow bright blue. Slowly\r\nit rises into the sky, and with a quick burst of light it disappears.\r\nSuddenly, a large ring of blue flames burst up from the center of the\r\nroom. Hoof-beats are heard in the distance.\r\n", ch->in_room);
    obj = read_object(5840, VIRTUAL); /* load a ring of fire */
    obj_to_room(obj, ch->in_room);
    for (obj = world[ch->in_room].contents; obj; obj = obj->next_content) {
      if (GET_OBJ_VNUM(obj) == 5825) { /* remove incomplete pentagram */
        extract_obj(obj);
        return TRUE;
      }
    }
  }
  return TRUE;
}

SPECIAL(master_sin)
{
  struct obj_data *obj;
  struct char_data *vict;
  struct char_data *master = (struct char_data *) me;
  static int run;

  if (GET_POS(master) == POS_FIGHTING || cmd || GET_POS(master) < POS_RESTING)
    return FALSE;
  /* mob heartbeat - lets check time_info */
  if (!(time_info.day % 3) && (run == 0)) { /* execute spec_proc every third day */
    run = 1;
    vict = NULL;
    if (!(obj = get_obj_num(real_object(5830))))
      return FALSE;
    /* we found the Icon */
    if (obj->carried_by)
      vict = obj->carried_by;
    if (obj->worn_by)
      vict = obj->worn_by;
    /* add more stuff here to trace upwards through bags etc */
    if (!vict) /* item not used */
      return FALSE;
    if (vict == master)
      return FALSE;
    snprintf(buf, MAX_STRING_LENGTH, "%s shouts, '%s! You have what rightfully belongs to me!'\r\n", GET_NAME(master), GET_NAME(vict));
    global_echo(buf);
    char_from_room(vict);
    act("$N disappears in a cloud of dust.", TRUE, vict, 0, ch, TO_ROOM);
    char_to_room(vict, master->in_room);
    act("$N has summoned you!", FALSE, vict, 0, ch, TO_CHAR);
    look_at_room(vict, 0);
    hit(master, vict, TYPE_UNDEFINED);
  } else if ((time_info.day % 3))
    run = 0;
  return FALSE;
}

SPECIAL(icecube)
{
  struct obj_data *cube = me;

  cube->spec_vars[0]--;
  if (cube->spec_vars[0] < 1) {
    act("The icecube melts into a small puddle.", FALSE, 0, cube, 0, TO_ROOM);
    extract_obj(cube);
  }

  return FALSE;
}

SPECIAL(pray)
{
  int l;
  struct obj_data *obj;

  struct pray_info {
    char *name;
    int vnum;
  } pray_list[] = { {"kiwi", 1514}, {"doll", 1247}, {"dagger", 3020}, {"bag", 3032}, {"\n", 0}};

  if (CMD_IS("pray") && GET_LEVEL(ch) >= LVL_IMMORT) {

    one_argument(argument, arg);

    for (l = 0; *(pray_list[l].name) != '\n'; l++)
      if (!strncmp(arg, pray_list[l].name, strlen(arg)))
        break;

    if (*pray_list[l].name == '\n' || !*arg) {
      send_to_char("The gods seems to be confused about your request.\r\n", ch);
      return 1;
    }

    obj = read_object(pray_list[l].vnum, VIRTUAL);
    act("The gods have answered your prayers!", FALSE, ch, 0, 0, TO_CHAR);
    act("$n sits down by the altar and prays to the gods.", TRUE, ch, 0, 0, TO_ROOM);
    obj_to_char(obj, ch);
    return 1;
  }

  return 0;
}

/* Vice Island */

SPECIAL(vice_master)
{
  char buf[MAX_STRING_LENGTH];
  ACMD(do_order);

  if (cmd || !AWAKE(ch))
    return (FALSE);

  switch (number(0, 8)) {
    case 0:
    case 1:
    case 2:
      if (GET_POS(ch) != POS_FIGHTING) {
        act("The master looks at his minions and growls, 'get to work, bitches!'", FALSE, ch, 0, 0, TO_ROOM);
        snprintf(buf, MAX_STRING_LENGTH, "followers bow master");
        do_order(ch, buf, 0, 0);
        return (TRUE);
      }
      break;
    case 3:
    case 4:
    case 5:
    case 6:
      if (GET_POS(ch) == POS_FIGHTING) {
        act("The master screams loudly, 'HELP ME, MY SLAVES!'", FALSE, ch, 0, 0, TO_ROOM);
        snprintf(buf, MAX_STRING_LENGTH, "followers kill %s", GET_NAME(ch->char_specials.fighting));
        do_order(ch, buf, 0, 0);
        return (TRUE);
      }

      break;
    default:
      return (FALSE);
  }
  return (TRUE);
}

SPECIAL(vice_slave)
{
  struct char_data *master;
  char buf[MAX_STRING_LENGTH];

  if (!AWAKE(ch))
    return FALSE;

  snprintf(buf, MAX_STRING_LENGTH, "m1526");

  if ((!cmd) && ((master = get_char_room_vis(ch, buf)))) {
    if (ch->master != master) {
      if (ch->master)
        stop_follower(ch);
      add_follower(ch, master);
      SET_BIT(ch->char_specials.saved.affected_by, AFF_CHARM);
      IS_CARRYING_W(ch) = 1000;
      IS_CARRYING_N(ch) = 100;
      return TRUE;
    }
  }

  return (FALSE);
}

SPECIAL(tarbaby)
{
  struct char_data *vict, *temp;

  if (cmd != 224)
    return (FALSE);

  for (vict = character_list; vict; vict = temp) {
    temp = vict->next;
    if (ch->in_room == vict->in_room) {
      if (!IS_NPC(vict) && (vict != ch)) {
        if (ch->master != vict) {
          if (ch->master)
            stop_follower(ch);
          add_follower(ch, vict);
          IS_CARRYING_W(ch) = 100;
          IS_CARRYING_N(ch) = 10;
          return (TRUE);
        }
      }
    }
  }

  if (!vict)
    return (FALSE);

  if (CMD_IS("kill")) {
    act("$n says, 'Ho ho, hee hee, you are sooo funneeee\n\r", FALSE, ch, 0, 0, TO_ROOM);
    return (TRUE);
  }

  else if (number(0, 100) < 20) {
    act("$n looks at you with the cutest expression.", FALSE, ch, 0, vict, TO_CHAR);
    act("$n looks at $N with the cutest expression.", FALSE, ch, 0, vict, TO_ROOM);
    return (TRUE);
  }
  return (FALSE);
}

/*
 SPECIAL(high_priest_of_terror)
 {
 struct char_data *vict;
 struct char_data *newvict,*temp;
 if (cmd)
 return(FALSE);
 if (GET_POS(ch) != POS_FIGHTING)
 return(FALSE);
 if (!ch->char_specials.fighting)
 return(FALSE);

 vict=ch->char_specials.fighting;

 if (!vict)
 return(FALSE);

 if (number(0,100)<10)
 {
 cast_spell(ch, vict, NULL, SPELL_CHARM);
 return(TRUE);
 for (newvict=character_list;newvict;newvict=temp)
 {
 temp = newvict->next;
 if (ch->in_room == newvict->in_room)
 {
 if (!IS_NPC(newvict))
 {
 if (!IS_SET(newvict->char_specials.saved.affected_by, AFF_CHARM))
 {
 ch->char_specials.fighting=newvict;
 newvict->char_specials.fighting=ch;
 }
 }
 }
 }
 }
 else if(number(0,100)<30)
 {
 cast_spell(ch, vict, NULL, SPELL_EARTHQUAKE);
 return(TRUE);
 }
 return(TRUE);
 }
 */

SPECIAL(elemental)
{
  int vnum;
  char *dis_msg[] = {"$n is caught up in a wind and carried away.", "$n turns into a fine mist and disappears.", "$n crumbles to dust and blows away.", "$n burns itself out and turns into a pile of ash.", "$n is brought back to the dead by the gods.", "$n is brought back to the dead by the gods.", "$n is brought back to the dead by the gods.", "$n is brought back to the dead by the gods.", "$n is brought back to the dead by the gods.", "\n"};
  if (ch->mob_specials.timer == 0 || ch->master == NULL || (ch->master && !AFF_FLAGGED(ch, AFF_CHARM))) {
    vnum = GET_MOB_VNUM(ch);
    vnum = vnum - 1201;
    act(dis_msg[vnum], TRUE, ch, 0, 0, TO_ROOM);
    extract_char(ch, 1);
  }
  (ch->mob_specials.timer)--;

  return TRUE;

}

/* rabdu rana assassins guild */

SPECIAL(weep_blade)
{
  struct obj_data *blade = me;
  struct char_data *owner = NULL;

  if (blade->worn_by)
    owner = blade->worn_by;
  if (blade->carried_by)
    owner = blade->carried_by;
  if (owner) {
    switch (number(0, 20)) {
      case 1:
        send_to_room("The weeping blade sniffs sadly.\r\n", owner->in_room);
        break;
      case 2:
        send_to_room("The weeping blade bursts into tears.\r\n", owner->in_room);
        break;
      case 3:
        send_to_room("The weeping blade sobs quietly to itself.\r\n", owner->in_room);
        break;
      case 4:
        command_interpreter(owner, "cry");
        break;
      case 5:
        command_interpreter(owner, "sniff");
        break;
      case 6:
        command_interpreter(owner, "sad");
        break;
      case 7:
        command_interpreter(owner, "sad self");
        break;
    }
  }

  return FALSE;

}

/* Intelligent mobiles */

SPECIAL(intelligent)
{
  return FALSE;
}

SPECIAL(portal)
{
  ACMD(do_look);
  extern const struct command_info cmd_info[];
  struct obj_data* portal = (struct obj_data*) me;

  if (!strcmp(cmd_info[cmd].command, "enter")) {
    if (strstr(argument, "portal") || strstr(argument, "moonwell")) {
      act("$n enters $a $o, and slowly fades from view.", FALSE, ch, me, 0, TO_ROOM);
      if (GET_DRAGGING(ch)) {
        snprintf(buf, MAX_STRING_LENGTH, "%s drags $p along.", GET_NAME(ch));
        act(buf, TRUE, 0, GET_DRAGGING(ch), 0, TO_ROOM);
      }
      char_from_room(ch);
      /* should we check for 1.portal 2.portal? */
      char_to_room(ch, real_room(GET_OBJ_VAL(portal, 0)));
      if (GET_DRAGGING(ch)) {
        obj_from_room(GET_DRAGGING(ch));
        obj_to_room(GET_DRAGGING(ch), ch->in_room);
        snprintf(buf, MAX_STRING_LENGTH, "You drag $p along with you.");
        act(buf, FALSE, ch, GET_DRAGGING(ch), 0, TO_CHAR);
        snprintf(buf, MAX_STRING_LENGTH, "%s drags $p along with $m.", GET_NAME(ch));
        act(buf, TRUE, ch, GET_DRAGGING(ch), 0, TO_ROOM);
      }
      act("$n emerges from $a $o, looking dazed.", FALSE, ch, me, 0, TO_ROOM);
      do_look(ch, "room", 0, 0);
      return TRUE;
    }
  }

  return FALSE;

}

void file_list_skills(struct char_data * ch, FILE* f)
{
  int i;

  for (i = 1; spells[i].command[0] != '\n'; i++) {
    if (GET_SKILL(ch, spells[i].spellindex) && !spells[i].spell_pointer && spells[i].command[0] != '<') {
      fprintf(f, "-%s- %d %d\n", spells[i].command, GET_SKILL(ch, spells[i].spellindex), GET_PRACS(ch, spells[i].spellindex));
    }
  }
  for (i = 1; spells[i].command[0] != '\n'; i++) {
    if (GET_SKILL(ch, spells[i].spellindex) && spells[i].spell_pointer) {
      fprintf(f, "-%s- %d %d\n", spells[i].command, GET_SKILL(ch, spells[i].spellindex), GET_PRACS(ch, spells[i].spellindex));
    }
  }
}
SPECIAL(wraithform_corpse)
{
  extern const struct command_info cmd_info[];
  struct obj_data* corpse = (struct obj_data*) me;
  struct obj_data* tobj;
  struct obj_data* tobj2;

  skip_spaces(&argument);
  if (CMD_IS("enter") && AFF2_FLAGGED(ch, AFF2_WRAITHFORM)) {
    if ((strcmp(corpse->owner, GET_NAME(ch)) == 0) && (strstr(argument, "corpse") || (strncasecmp(argument, GET_NAME(ch), strlen(argument)) == 0))) {
      act("$n's soul merges with $s corpse, which springs forth with new life.", FALSE, ch, me, 0, TO_ROOM);
      act("Your soul merges with your corpse, which springs forth with new life.", FALSE, ch, me, 0, TO_CHAR);
      for (tobj = corpse->contains; tobj != NULL; tobj = tobj2) {
        tobj2 = tobj->next_content;
        if (GET_OBJ_TYPE(tobj) == ITEM_MONEY) {
          GET_PLAT(ch) += GET_OBJ_VAL(tobj, 0);
          GET_GOLD(ch) += GET_OBJ_VAL(tobj, 1);
          GET_SILVER(ch) += GET_OBJ_VAL(tobj, 2);
          GET_COPPER(ch) += GET_OBJ_VAL(tobj, 3);
          GET_TEMP_GOLD(ch) = GET_PLAT(ch) * 1000 + GET_GOLD(ch) * 100 + GET_SILVER(ch) * 10 + GET_COPPER(ch);
        } else {
          obj_from_obj(tobj);
          obj_to_char(tobj, ch);
        }
      }
      extract_obj(corpse);
      affect_from_char(ch, spells[find_spell_num("wraith form")].spellindex);
      return TRUE;
    }
  }
  return FALSE;
}
