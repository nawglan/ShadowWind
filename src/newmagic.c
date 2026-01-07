#include <stdio.h>
#include <string.h>
#include "structs.h"
#include "db.h"
#include "comm.h"
#include "interpreter.h"
#include "utils.h"
#include "event.h"
#include "spells.h"

#define NUM_NPC_SPELLS     4    /* Number of npc spells per circle */

extern struct spell_info_type *spells;
extern struct room_data *world;
extern struct index_data *mob_index;
extern struct char_data *character_list;
int find_skill_num(char *name);
int find_spell_num(char *name);
void improve_skill(struct char_data *ch, int skill, int chance);
int find_skill_num_def(int spellindex);

int NPC_mage_spells[65];
int NPC_cleric_spells[65];

void clear_magic_memory(struct char_data *ch)
{
  int i;

  for (i = 0; i < 65; i++) {
    ch->spell_memory[i].spellindex = 0;
    ch->spell_memory[i].has_mem = 0;
    ch->spell_memory[i].is_mem = 0;
    ch->spell_memory[i].time_left = 0;
    ch->spell_memory[i].rate = 0;
  }
}

void set_magic_memory(struct char_data *ch)
{
  int i;
  int j;

  for (j = 0; j < GET_PLR_CIRCLE(ch); j++) {
    for (i = 0; i < 11; i++) {
      if (i == GET_PLR_CIRCLE(ch)) {
        ch->can_mem[i] = 2;
      } else if (i < GET_PLR_CIRCLE(ch)) {
        if (ch->can_mem[i] == 0) {
          ch->can_mem[i] = 2;
        } else {
          ch->can_mem[i]++;
        }
        if (ch->can_mem[i] > 10) {
          ch->can_mem[i] = 10;
        }
      } else {
        ch->can_mem[i] = 0;
      }
    }
  }
}

ACMD(do_meditate)
{
  if ((!IS_MAGE(ch) && !IS_PRI(ch)) || IS_NPC(ch)) {
    send_to_char("You have no need to meditate.\r\n", ch);
    return;
  }

  if (GET_POS(ch) > POS_SITTING) {
    send_to_char("You must be sitting or resting to meditate.\r\n", ch);
    return;
  }

  send_to_char("You begin meditating.\r\n", ch);
  SET_BIT(AFF_FLAGS(ch), AFF_MEDITATING);
}

ACMD(do_scribe)
{
  SPECIAL(guild);
  int time;
  int page;
  int numpages;
  int scribing;
  int meditate = spells[find_skill_num("meditate")].spellindex;
  struct char_data *p;
  struct char_data *nextp;
  int found = 0;
  /* priests don't need a writing instrument they "pray" their spells into
   * their holy symbol.
   */

  if (!IS_MAGE(ch) && !IS_PRI(ch)) {
    send_to_char("The gods ignore your attempt to learn magic.\r\n", ch);
    return;
  }

  for (p = world[ch->in_room].people; p; p = nextp) {
    nextp = p->next_in_room;
    if (GET_MOB_SPEC(p) == guild) {
      found = 1;
    }
  }

  if (!found) {
    send_to_char("You can only scribe at your guildmaster.\r\n", ch);
    return;
  }

  if (GET_POS(ch) < POS_RESTING || GET_POS(ch) > POS_SITTING) {
    send_to_char("You must be sitting or resting first.\r\n", ch);
    return;
  }

  skip_spaces(&argument);

  if (!argument || !*argument) {
    send_to_char("Scribe what?\r\n", ch);
    return;
  }

  if ((scribing = find_spell_num(argument)) == -1) {
    send_to_char("What spell is that?\r\n", ch);
    return;
  }

  GET_SCRIBING(ch) = (spells + scribing);
  scribing = spells[scribing].spellindex;
  time = GET_SPELL_CIRCLE(ch, GET_SCRIBING(ch)) * 5;
  if (IS_IMMO(ch)) {
    time = 1;
  }

  if (GET_SPELL_CIRCLE(ch, GET_SCRIBING(ch)) > GET_PLR_CIRCLE(ch)) {
    send_to_char("You have heard of that spell, but you are unable to comprehend it.\r\n", ch);
    GET_SCRIBING(ch) = NULL;
    return;
  }

  numpages = GET_SPELL_CIRCLE(ch, GET_SCRIBING(ch));

  if ((IS_MAGE(ch) && GET_EQ(ch, WEAR_HOLD) && GET_EQ(ch, WEAR_HOLD_2)) || ((GET_EQ(ch, WEAR_HOLD) || GET_EQ(ch, WEAR_HOLD_2)) && IS_PRI(ch))) {
    if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD)) == ITEM_SPELLBOOK) {
      if (IS_PRI(ch) || GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD_2)) == ITEM_PEN) {
        if (LIGHT_OK(ch)) {
          if (INVIS_OK_OBJ(ch, GET_EQ(ch, WEAR_HOLD))) {
            if (IS_PRI(ch) || INVIS_OK_OBJ(ch, GET_EQ(ch, WEAR_HOLD_2))) {
              if (GET_OBJ_VAL(GET_EQ(ch, WEAR_HOLD), 2) == -1) {
                GET_OBJ_VAL(GET_EQ(ch, WEAR_HOLD), 2) = GET_CLASS(ch);
              } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_HOLD), 2) != GET_CLASS(ch)) {
                if (IS_PRI(ch)) {
                  send_to_char("You do not recognize the markings on your religeous symbol.\r\n", ch);
                  GET_SCRIBING(ch) = NULL;
                  return;
                } else {
                  send_to_char("You do not recognize the language your spellbook is written in.\r\n", ch);
                  GET_SCRIBING(ch) = NULL;
                  return;
                }
              }
              if (GET_OBJ_VAL(GET_EQ(ch, WEAR_HOLD), 1) < numpages) {
                if (IS_PRI(ch)) {
                  send_to_char("Your religeous symbol cannot contain that spell.\r\n", ch);
                  GET_SCRIBING(ch) = NULL;
                  return;
                } else {
                  send_to_char("There isn't enough pages in your spellbook for that spell.\r\n", ch);
                  GET_SCRIBING(ch) = NULL;
                  return;
                }
              }
              for (page = 0; page < GET_OBJ_VAL(GET_EQ(ch, WEAR_HOLD), 0); page++) {
                if (GET_OBJ_SPELLLISTNUM(GET_EQ(ch, WEAR_HOLD), page) == 0) {
                  break;
                } else if (GET_OBJ_SPELLLISTNUM(GET_EQ(ch, WEAR_HOLD), page) == scribing) {
                  if (GET_SKILL(ch, scribing)) {
                    if (IS_PRI(ch)) {
                      send_to_char("That spell is already in your religeous symbol.\r\n", ch);
                      GET_SCRIBING(ch) = NULL;
                      return;
                    } else {
                      send_to_char("That spell is already in your spellbook.\r\n", ch);
                      GET_SCRIBING(ch) = NULL;
                      return;
                    }
                  } else {
                    GET_OBJ_VAL(GET_EQ(ch, WEAR_HOLD), 1) += numpages;
                    break;
                  }
                }
              }
              if (MEDITATING(ch) && GET_SKILL(ch, meditate) > number(0, 150)) {
                time >>= 1;
                improve_skill(ch, meditate, SKUSE_FREQUENT);
              }
              if (IS_PRI(ch)) {
                send_to_char("You begin praying to your god while rubbing your talisman.\r\n", ch);
              } else {
                send_to_char("You begin writing in your spellbook.\r\n", ch);
              }
              if (!GET_COND(ch, THIRST) || !GET_COND(ch, FULL)) {
                time <<= 1;
              }
              SET_BIT(AFF2_FLAGS(ch), AFF2_SCRIBING);
              add_event(time, scribe_event, EVENT_SCRIBE, ch, NULL, (void *) (long) (numpages - 1), NULL, NULL, page);
            } else { /* lost your pen? */
              send_to_char("You seem to have missplaced your writing instrument.\r\n", ch);
            }
          } else { /* lost your spellbook? */
            if (IS_PRI(ch)) {
              send_to_char("You seem to have missplaced religious symbol.\r\n", ch);
            } else {
              send_to_char("You seem to have missplaced your spellbook.\r\n", ch);
            }
          }
        } else { /* too dark to see */
          if (IS_PRI(ch)) {
            send_to_char("It is too dark for you to see your religious symbol\r\n", ch);
          } else {
            send_to_char("It is too dark to scribe!\r\n", ch);
          }
        }
      } else { /* need pen in other hand */
        send_to_char("You seem to have missplaced your writing instrument.\r\n", ch);
      }
    } else if (IS_PRI(ch) || GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD)) == ITEM_PEN) {
      if (GET_OBJ_TYPE(GET_EQ(ch, WEAR_HOLD_2)) == ITEM_SPELLBOOK) {
        if (LIGHT_OK(ch)) {
          if (IS_PRI(ch) || INVIS_OK_OBJ(ch, GET_EQ(ch, WEAR_HOLD))) {
            if (INVIS_OK_OBJ(ch, GET_EQ(ch, WEAR_HOLD_2))) {
              if (GET_OBJ_VAL(GET_EQ(ch, WEAR_HOLD_2), 2) == -1) {
                GET_OBJ_VAL(GET_EQ(ch, WEAR_HOLD_2), 2) = GET_CLASS(ch);
              } else if (GET_OBJ_VAL(GET_EQ(ch, WEAR_HOLD_2), 2) != GET_CLASS(ch)) {
                if (IS_PRI(ch)) {
                  send_to_char("You do not recognize the markings on your religeous symbol.\r\n", ch);
                  GET_SCRIBING(ch) = NULL;
                  return;
                } else {
                  send_to_char("You do not recognize the language your spellbook is written in.\r\n", ch);
                  GET_SCRIBING(ch) = NULL;
                  return;
                }
              }
              if (GET_OBJ_VAL(GET_EQ(ch, WEAR_HOLD_2), 1) < numpages) {
                if (IS_PRI(ch)) {
                  send_to_char("Your religeous symbol cannot contain that spell.\r\n", ch);
                  GET_SCRIBING(ch) = NULL;
                  return;
                } else {
                  send_to_char("There isn't enough pages in your spellbook for that spell.\r\n", ch);
                  GET_SCRIBING(ch) = NULL;
                  return;
                }
              }
              for (page = 0; page < GET_OBJ_VAL(GET_EQ(ch, WEAR_HOLD_2), 0); page++) {
                if (GET_OBJ_SPELLLISTNUM(GET_EQ(ch, WEAR_HOLD_2), page) == 0) {
                  break;
                } else if (GET_OBJ_SPELLLISTNUM(GET_EQ(ch, WEAR_HOLD_2), page) == scribing) {
                  if (GET_SKILL(ch, scribing)) {
                    if (IS_PRI(ch)) {
                      send_to_char("That spell is already in your religeous symbol.\r\n", ch);
                      GET_SCRIBING(ch) = NULL;
                      return;
                    } else {
                      send_to_char("That spell is already in your spellbook.\r\n", ch);
                      GET_SCRIBING(ch) = NULL;
                      return;
                    }
                  } else {
                    GET_OBJ_VAL(GET_EQ(ch, WEAR_HOLD_2), 1) += numpages;
                    break;
                  }
                }
              }
              if (MEDITATING(ch) && GET_SKILL(ch, meditate) > number(0, 150)) {
                time >>= 1;
                improve_skill(ch, meditate, SKUSE_FREQUENT);
              }
              if (IS_PRI(ch)) {
                send_to_char("You begin praying to your god while rubbing your talisman.\r\n", ch);
              } else {
                send_to_char("You begin writing in your spellbook.\r\n", ch);
              }
              if (!GET_COND(ch, THIRST) || !GET_COND(ch, FULL)) {
                time *= 2;
              }
              SET_BIT(AFF2_FLAGS(ch), AFF2_SCRIBING);
              add_event(time, scribe_event, EVENT_SCRIBE, ch, NULL, (void *) (long) (numpages - 1), NULL, NULL, page);
            } else { /* lost your spellbook? */
              if (IS_PRI(ch)) {
                send_to_char("You seem to have missplaced your religious symbol.\r\n", ch);
              } else {
                send_to_char("You seem to have missplaced your spellbook.\r\n", ch);
              }
            }
          } else { /* lost your pen? */
            send_to_char("You seem to have missplaced your writing instrument.\r\n", ch);
          }
        } else { /* too dark to see */
          if (IS_PRI(ch)) {
            send_to_char("It is too dark for you to see your religious symbol.\r\n", ch);
          } else {
            send_to_char("It is too dark to scribe!\r\n", ch);
          }
        }
      } else { /* spellbook must be in other hand */
        send_to_char("You need to be holding your spellbook too.\r\n", ch);
      }
    } else { /* where spellbook and pen? */
      if (IS_PRI(ch)) {
        send_to_char("You need to be holding your religious symbol.\r\n", ch);
      } else {
        send_to_char("You need to be holding your spellbook and a writing instrument.\r\n", ch);
      }
    }
  } else { /* where spellbook and pen? */
    if (IS_PRI(ch)) {
      send_to_char("You need to be holding your religious symbol.\r\n", ch);
    } else {
      send_to_char("You need to be holding your spellbook and a writing instrument.\r\n", ch);
    }
  }
}

ACMD(do_memorize)
{
  int i;
  int j;
  int time = 0;
  int found_circle;
  int circle;
  int count;
  struct event_info *temp_event;
  struct event_info *next_event;
  extern struct event_info *pending_events;

  if (IS_NPC(ch)) {
    return;
  }

  if (!IS_MAGE(ch) && !IS_PRI(ch)) {
    send_to_char("You have no need to memorize anything.\r\n", ch);
    return;
  }

  if (argument) {
    skip_spaces(&argument);
  }

  if (!argument || *argument == '\0') {
    buf[0] = '\0';
    strcpy(buf, "You have memorized the following spells:\r\n");
    for (i = GET_PLR_CIRCLE(ch); i > 0; i--) {
      found_circle = 0;
      for (i = 0; i < 65; i++) {
        if (ch->spell_memory[i].circle == i) {
          if (!found_circle) {
            found_circle = 1;
            sprintf(buf + strlen(buf), "(%2d%s circle) %2d - %s\r\n", i, i == 1 ? "st" : i == 2 ? "nd" : i == 3 ? "rd" : "th", ch->spell_memory[i].has_mem, spells[find_skill_num_def(ch->spell_memory[i].spellindex)].command);
          } else {
            sprintf(buf + strlen(buf), "               %2d - %s\r\n", ch->spell_memory[i].has_mem, spells[find_skill_num_def(ch->spell_memory[i].spellindex)].command);
          }
        }
      }
    }
    if (AFF2_FLAGGED(ch, AFF2_MEMMING)) {
      if (*buf) {
        strcat(buf, "\r\nAnd you ");
      } else {
        strcat(buf, "You ");
      }
      if (IS_MAGE(ch)) {
        strcat(buf, "are currently memorizing the following spells:\r\n");
      } else {
        strcat(buf, "are currently praying for the following spells:\r\n");
      }
      for (temp_event = pending_events; temp_event; temp_event = next_event) {
        next_event = temp_event->next;
        if (temp_event->type == EVENT_MEM && temp_event->causer == (void*) ch) {
          circle = GET_SPELL_CIRCLE(ch, temp_event->sinfo);
          sprintf(buf + strlen(buf), "%5d seconds: (%2d%s) %s\r\n", temp_event->ticks_to_go, circle, circle == 1 ? "st" : circle == 2 ? "nd" : circle == 3 ? "rd" : "th", temp_event->sinfo->command);
        }
      }
    }
    if (IS_MAGE(ch)) {
      strcat(buf, "\r\nYou can memorize ");
    } else {
      strcat(buf, "\r\nYou can pray ");
    }
    i = 0;
    count = 0;
    for (j = 0; j < 65; j++) {
      count += ch->spell_memory[j].has_mem;
      count += ch->spell_memory[j].is_mem;
      if (count) {
        if (i) {
          sprintf(buf + strlen(buf), ", %s", buf2);
        }
        i++;
        sprintf(buf2, "%d %d%s", ch->can_mem[j] - count, j, j == 1 ? "st" : j == 2 ? "nd" : j == 3 ? "rd" : "th");
      }
      count = 0;
    }
    if (!i) {
      strcat(buf, "no more spells.\r\n");
    } else {
      if (i > 1) {
        strcat(buf, "and ");
      }
      sprintf(buf + strlen(buf), " %s circle spells(s).\r\n", buf2);
    }
    page_string(ch->desc, buf, 1);
    if (find_event(ch, EVENT_MEM) && !AFF2_FLAGGED(ch, AFF2_MEMMING) && GET_COND(ch, FULL) && GET_COND(ch, THIRST) && GET_POS(ch) >= POS_RESTING && GET_POS(ch) <= POS_SITTING) {
      if (IS_MAGE(ch)) {
        send_to_char("You continue your studies.\r\n", ch);
        act("$n opens $e spellbook and resumes $e studies.", TRUE, ch, 0, 0, TO_ROOM);
      } else {
        send_to_char("You continue your praying.\r\n", ch);
        act("$n grabs $e religeous symbol and resumes $e meditations.", TRUE, ch, 0, 0, TO_ROOM);
      }
      SET_BIT(AFF2_FLAGS(ch), AFF2_MEMMING);
      add_event(time, memorize_event, EVENT_MEM, ch, NULL, NULL, NULL, NULL, 0);
    }
  }
}

ACMD(do_abort)
{
  if (!IS_CASTING(ch)) {
    send_to_char("You aren't casting anything.\r\n", ch);
    return;
  }

  send_to_char("You stop casting.\r\n", ch);
  act("$n stops casting.", TRUE, ch, 0, 0, TO_ROOM);
  clean_causer_events(ch, EVENT_SPELL);
}

void set_npc_spells(void)
{
  /* MAGE */
  /* first circle */
  NPC_mage_spells[0] = spells[find_spell_num("acid arrow")].spellindex;
  NPC_mage_spells[1] = spells[find_spell_num("burning hands")].spellindex;
  NPC_mage_spells[2] = spells[find_spell_num("detect invisibility")].spellindex;
  NPC_mage_spells[3] = spells[find_spell_num("faerie fire")].spellindex;
  NPC_mage_spells[4] = spells[find_spell_num("fire bolt")].spellindex;
  NPC_mage_spells[5] = spells[find_spell_num("ice bolt")].spellindex;
  NPC_mage_spells[6] = spells[find_spell_num("magic missile")].spellindex;

  /* second circle */
  NPC_mage_spells[7] = spells[find_spell_num("chill touch")].spellindex;
  NPC_mage_spells[8] = spells[find_spell_num("color spray")].spellindex;
  NPC_mage_spells[9] = spells[find_spell_num("dexterity")].spellindex;
  NPC_mage_spells[10] = spells[find_spell_num("poison")].spellindex;
  NPC_mage_spells[11] = spells[find_spell_num("strength")].spellindex;

  /* third circle */
  NPC_mage_spells[12] = spells[find_spell_num("blindness")].spellindex;
  NPC_mage_spells[13] = spells[find_spell_num("dispel magic")].spellindex;
  NPC_mage_spells[14] = spells[find_spell_num("fireball")].spellindex;
  NPC_mage_spells[15] = spells[find_spell_num("fire shield")].spellindex;
  NPC_mage_spells[16] = spells[find_spell_num("ice ball")].spellindex;
  NPC_mage_spells[17] = spells[find_spell_num("ice shield")].spellindex;
  NPC_mage_spells[18] = spells[find_spell_num("hide self")].spellindex;
  NPC_mage_spells[19] = spells[find_spell_num("ray of enfeeblement")].spellindex;
  NPC_mage_spells[20] = spells[find_spell_num("sleep")].spellindex;
  NPC_mage_spells[21] = spells[find_spell_num("slowness")].spellindex;

  /* fourth circle */
  NPC_mage_spells[22] = spells[find_spell_num("cone of cold")].spellindex;
  NPC_mage_spells[23] = spells[find_spell_num("firestorm")].spellindex;
  NPC_mage_spells[24] = spells[find_spell_num("ice storm")].spellindex;
  NPC_mage_spells[25] = spells[find_spell_num("lightning bolt")].spellindex;
  NPC_mage_spells[26] = spells[find_spell_num("minor paralysis")].spellindex;

  /* fifth circle */
  NPC_mage_spells[27] = spells[find_spell_num("nightvision")].spellindex;
  NPC_mage_spells[28] = spells[find_spell_num("haste")].spellindex;

  /* sixth circle */
  NPC_mage_spells[29] = spells[find_spell_num("chain lightning")].spellindex;
  NPC_mage_spells[30] = spells[find_spell_num("feeble mind")].spellindex;
  NPC_mage_spells[31] = spells[find_spell_num("minor globe of invulnerability")].spellindex;
  NPC_mage_spells[32] = spells[find_spell_num("prismatic spray")].spellindex;
  NPC_mage_spells[33] = spells[find_spell_num("stoneskin")].spellindex;

  /* seventh circle */
  NPC_mage_spells[34] = spells[find_spell_num("agony")].spellindex;
  NPC_mage_spells[35] = spells[find_spell_num("bigbys clenched fist")].spellindex;
  NPC_mage_spells[36] = spells[find_spell_num("blades of fury")].spellindex;
  NPC_mage_spells[37] = spells[find_spell_num("incendiary cloud")].spellindex;

  /* eighth circle */
  NPC_mage_spells[38] = spells[find_spell_num("maelstrom")].spellindex;
  NPC_mage_spells[39] = spells[find_spell_num("major globe of invulnerability")].spellindex;

  /* ninth circle */
  NPC_mage_spells[40] = spells[find_spell_num("major paralysis")].spellindex;
  NPC_mage_spells[41] = spells[find_spell_num("meteor swarm")].spellindex;
  NPC_mage_spells[42] = spells[find_spell_num("vampiric touch")].spellindex;

  /* tenth circle */
  NPC_mage_spells[43] = spells[find_spell_num("disintegrate")].spellindex;
  NPC_mage_spells[44] = spells[find_spell_num("power word kill")].spellindex;
  NPC_mage_spells[45] = spells[find_spell_num("wizard war")].spellindex;

  /* end of spells */
  NPC_mage_spells[46] = -1;

  /* CLERIC */
  /* first circle */
  NPC_cleric_spells[0] = spells[find_spell_num("armor")].spellindex;
  NPC_cleric_spells[1] = spells[find_spell_num("cause light")].spellindex;
  NPC_cleric_spells[2] = spells[find_spell_num("cure light")].spellindex;
  NPC_cleric_spells[3] = spells[find_spell_num("disease")].spellindex;
  NPC_cleric_spells[4] = spells[find_spell_num("lesser mending")].spellindex;
  NPC_cleric_spells[5] = spells[find_spell_num("shocking grasp")].spellindex;

  /* second circle */
  NPC_cleric_spells[6] = spells[find_spell_num("cause serious")].spellindex;
  NPC_cleric_spells[7] = spells[find_spell_num("cure blindness")].spellindex;
  NPC_cleric_spells[8] = spells[find_spell_num("cure serious")].spellindex;
  NPC_cleric_spells[9] = spells[find_spell_num("detect invisibility")].spellindex;
  NPC_cleric_spells[10] = spells[find_spell_num("faerie fire")].spellindex;
  NPC_cleric_spells[11] = spells[find_spell_num("remove curse")].spellindex;

  /* third circle */
  NPC_cleric_spells[12] = spells[find_spell_num("barkskin")].spellindex;
  NPC_cleric_spells[13] = spells[find_spell_num("blindness")].spellindex;
  NPC_cleric_spells[14] = spells[find_spell_num("cause critical")].spellindex;
  NPC_cleric_spells[15] = spells[find_spell_num("cure critical")].spellindex;
  NPC_cleric_spells[16] = spells[find_spell_num("cure disease")].spellindex;
  NPC_cleric_spells[17] = spells[find_spell_num("cure poison")].spellindex;
  NPC_cleric_spells[18] = spells[find_spell_num("earthquake")].spellindex;
  NPC_cleric_spells[19] = spells[find_spell_num("protection from acid")].spellindex;
  NPC_cleric_spells[20] = spells[find_spell_num("protection from evil")].spellindex;
  NPC_cleric_spells[21] = spells[find_spell_num("protection from fire")].spellindex;
  NPC_cleric_spells[22] = spells[find_spell_num("protection from gas")].spellindex;
  NPC_cleric_spells[23] = spells[find_spell_num("protection from good")].spellindex;
  NPC_cleric_spells[24] = spells[find_spell_num("protection from ice")].spellindex;
  NPC_cleric_spells[25] = spells[find_spell_num("protection from lightning")].spellindex;
  NPC_cleric_spells[26] = spells[find_spell_num("tidal wave")].spellindex;

  /* fourth circle */
  NPC_cleric_spells[27] = spells[find_spell_num("aid")].spellindex;
  NPC_cleric_spells[28] = spells[find_spell_num("bless")].spellindex;
  NPC_cleric_spells[29] = spells[find_spell_num("curse")].spellindex;
  NPC_cleric_spells[30] = spells[find_spell_num("dispel magic")].spellindex;
  NPC_cleric_spells[31] = spells[find_spell_num("flamestrike")].spellindex;
  NPC_cleric_spells[32] = spells[find_spell_num("mending")].spellindex;
  NPC_cleric_spells[33] = spells[find_spell_num("molten spray")].spellindex;
  NPC_cleric_spells[34] = spells[find_spell_num("sea storm")].spellindex;
  NPC_cleric_spells[35] = spells[find_spell_num("sunray")].spellindex;

  /* fifth circle */
  NPC_cleric_spells[36] = spells[find_spell_num("call lightning")].spellindex;
  NPC_cleric_spells[37] = spells[find_spell_num("harm")].spellindex;
  NPC_cleric_spells[38] = spells[find_spell_num("heal")].spellindex;
  NPC_cleric_spells[39] = spells[find_spell_num("ray of hope")].spellindex;
  NPC_cleric_spells[40] = spells[find_spell_num("silence")].spellindex;
  NPC_cleric_spells[41] = spells[find_spell_num("slowness")].spellindex;
  NPC_cleric_spells[42] = spells[find_spell_num("true seeing")].spellindex;

  /* sixth circle */
  NPC_cleric_spells[43] = spells[find_spell_num("stoneskin")].spellindex;
  NPC_cleric_spells[44] = spells[find_spell_num("vitality")].spellindex;

  /* seventh circle */
  NPC_cleric_spells[45] = spells[find_spell_num("full harm")].spellindex;
  NPC_cleric_spells[46] = spells[find_spell_num("full heal")].spellindex;
  NPC_cleric_spells[47] = spells[find_spell_num("greater mending")].spellindex;
  NPC_cleric_spells[48] = spells[find_spell_num("ogrestrength")].spellindex;

  /* eighth circle */
  NPC_cleric_spells[49] = spells[find_spell_num("death fog")].spellindex;

  /* ninth circle */
  NPC_cleric_spells[50] = spells[find_spell_num("arieks shattering iceball")].spellindex;
  NPC_cleric_spells[51] = spells[find_spell_num("creeping death")].spellindex;

  /* tenth circle */
  NPC_cleric_spells[52] = spells[find_spell_num("creeping doom")].spellindex;

  /* end of spells */
  NPC_cleric_spells[53] = -1;
}

void give_npc_spells(void)
{
  struct char_data *temp_ch;
  struct char_data *next_ch;
  int circ;
  int i;

  for (temp_ch = character_list; temp_ch; temp_ch = next_ch) {
    next_ch = temp_ch->next;
    if (IS_MOB(temp_ch) && (MOB_FLAGGED(temp_ch, MOB_HAS_CLERIC) || MOB_FLAGGED(temp_ch, MOB_HAS_MAGE))) {
      /* first make sure they have all the spells they need */
      set_magic_memory(temp_ch);
      for (i = 0; i < 65; i++) {
        if (!temp_ch->spell_memory[i].spellindex) {
          if (MOB_FLAGGED(temp_ch, MOB_HAS_MAGE) && !MOB_FLAGGED(temp_ch, MOB_HAS_CLERIC)) {
            temp_ch->spell_memory[i].spellindex = NPC_mage_spells[i];
          } else if (MOB_FLAGGED(temp_ch, MOB_HAS_CLERIC) && !MOB_FLAGGED(temp_ch, MOB_HAS_MAGE)) {
            temp_ch->spell_memory[i].spellindex = NPC_cleric_spells[i];
          } else if (number(0, 1)) {
            temp_ch->spell_memory[i].spellindex = NPC_mage_spells[i];
          } else {
            temp_ch->spell_memory[i].spellindex = NPC_cleric_spells[i];
          }
          temp_ch->spell_memory[i].has_mem += 1;
          circ = GET_SPELL_CIRCLE(temp_ch, (spells + find_skill_num_def(temp_ch->spell_memory[i].spellindex)));
          if (temp_ch->spell_memory[i].has_mem > temp_ch->can_mem[circ]) {
            temp_ch->spell_memory[i].has_mem = temp_ch->can_mem[circ];
          }
        }
      }
    }
  }
}

/* NPC spells during combat */
void do_mage_spell(struct char_data *ch)
{
}

void do_cleric_spell(struct char_data *ch)
{
}
