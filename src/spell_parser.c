/*
 * newspellparser.c Portions from DikuMUD, Copyright (C) 1990, 1991. Written
 * by Fred Merkel and Jeremy Elson Part of JediMUD Copyright (C) 1993
 * Trustees of The Johns Hopkins Unversity All Rights Reserved.
 */

#include <stdio.h>
#include <string.h>

#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "event.h"
#include "spells.h"
#include "handler.h"
#include "comm.h"
#include "db.h"

#define SINFO spells[spellnum]

extern struct room_data *world;
extern struct actd_msg *get_actd(int vnum);
extern struct index_data *obj_index;
extern char *pc_class_types[];
char *get_spell_name(int spellnum);
extern struct spell_info_type *spells;

struct syllable {
  char *org;
  char *new;
};

struct syllable syls[] = { {" ", " "}, {"ar", "abra"}, {"ate", "i"}, {"cau", "kada"}, {"blind", "nose"}, {"bur", "mosa"}, {"cu", "judi"}, {"de", "oculo"}, {"dis", "mar"}, {"ect", "kamina"}, {"en", "uns"}, {"gro", "cra"}, {"light", "dies"}, {"lo", "hi"}, {"magi", "kari"}, {"mon", "bar"}, {"mor", "zak"}, {"move", "sido"}, {"ness", "lacri"}, {"ning", "illa"}, {"per", "duda"}, {"ra", "gru"}, {"re", "candus"}, {"son", "sabru"}, {"tect", "infra"}, {"tri", "cula"}, {"ven", "nofo"}, {"word of", "inset"}, {"a", "i"}, {"b", "v"}, {"c", "q"}, {"d", "m"}, {"e", "o"}, {"f", "y"}, {"g", "t"}, {"h", "p"}, {"i", "u"}, {"j", "y"}, {"k", "t"}, {"l", "r"}, {"m", "w"}, {"n", "b"}, {"o", "a"}, {"p", "s"}, {"q", "d"}, {"r", "f"}, {"s", "g"}, {"t", "h"}, {"u", "e"}, {"v", "z"}, {"w", "x"}, {"x", "n"}, {"y", "l"}, {"z", "k"}, {"", ""}};

int mag_manacost(struct char_data * ch, int spellnum)
{
  return MAX(SINFO.mana_max - (SINFO.mana_change * (GET_LEVEL(ch) - SINFO.min_level[(int) GET_CLASS(ch)])), SINFO.mana_min);
}

/* say_spell erodes buf, buf1, buf2 */
void say_spell(struct char_data * ch, int spellnum, struct char_data * tch, struct obj_data * tobj)
{
  char buf[256];
  char buf1[256];
  char buf2[256];
  char lbuf[256];
  char *pName;
  int x;
  int length;

  struct char_data *i;

  buf[0] = '\0';
  size_t buflen = 0;

  safe_snprintf(lbuf, sizeof(lbuf), "%s", get_spell_name(spellnum));
  for (pName = lbuf; *pName != '\0'; pName += length) {
    for (x = 0; (length = strlen(syls[x].org)) > 0; x++) {
      if (!strncmp(syls[x].org, pName, length)) {
        buflen += safe_snprintf(buf + buflen, sizeof(buf) - buflen, "%s", syls[x].new);
        break;
      }
    }
    if (length == 0)
      length = 1;
  }

  if (tch != NULL && tch->in_room == ch->in_room) {
    if (tch == ch)
      safe_snprintf(lbuf, sizeof(lbuf), "$n closes $s eyes and utters the words, '%%s'.");
    else
      safe_snprintf(lbuf, sizeof(lbuf), "$n stares at $N and utters the words, '%%s'.");
  } else if (tobj != NULL && tobj->in_room == ch->in_room)
    safe_snprintf(lbuf, sizeof(lbuf), "$n stares at $p and utters the words, '%%s'.");
  else
    safe_snprintf(lbuf, sizeof(lbuf), "$n utters the words, '%%s'.");

  safe_snprintf(buf2, MAX_STRING_LENGTH, lbuf, buf);
  safe_snprintf(buf1, MAX_STRING_LENGTH, lbuf, get_spell_name(spellnum));

  for (i = world[ch->in_room].people; i; i = i->next_in_room) {
    if (i == ch || i == tch || !i->desc || !AWAKE(i))
      continue;
    if (GET_CLASS(ch) == GET_CLASS(i))
      perform_act(buf1, ch, tobj, tch, i);
    else
      perform_act(buf2, ch, tobj, tch, i);
  }
  if (tch != NULL && tch != ch) {
    safe_snprintf(buf1, MAX_STRING_LENGTH, "$n stares at you and utters the words, '%s'.", GET_CLASS(ch) == GET_CLASS(tch) ? get_spell_name(spellnum) : buf);
    act(buf1, FALSE, ch, NULL, tch, TO_VICT);
  }
}

int find_spell_num(char *name)
{
  int index = 0;
  /* not needed, see comment below.
   int ok;
   char *temp, *temp2;
   char first[256], first2[256];
   */

  if (strcmp(name, "Unknown") == 0)
    return 0;
  while (spells[++index].command[0] != '\n') {
    if (is_abbrev(name, spells[index].command) && spells[index].spell_pointer)
      return index;

    /* this code was causing fire bolt to return for firestorm and flame blade for flamestrike
     ok = 1;
     temp = any_one_arg(spells[index].command, first);
     temp2 = any_one_arg(name, first2);
     while (*first && *first2 && ok) {
     if (!is_abbrev(first, first2))
     ok = 0;
     temp = any_one_arg(temp, first);
     temp2 = any_one_arg(temp2, first2);
     }

     if (ok && !*first2 && spells[index].spell_pointer)
     return index;
     */
  }

  return -1;
}

int find_skill_num(char *name)
{
  int index = 0;
  /* not needed, see comment below.
   int ok;
   char *temp, *temp2;
   char first[256], first2[256];
   */

  while (spells[++index].command[0] != '\n') {
    if (is_abbrev(name, spells[index].command))
      return index;

    /* this code returns some false hits.
     ok = 1;
     temp = any_one_arg(spells[index].command, first);
     temp2 = any_one_arg(name, first2);
     while (*first && *first2 && ok) {
     if (!is_abbrev(first, first2))
     ok = 0;
     temp = any_one_arg(temp, first);
     temp2 = any_one_arg(temp2, first2);
     }

     if (ok && !*first2)
     return index;
     */
  }

  return -1;
}

int find_skill_num_def(int define)
{
  int index = 0;

  while (spells[++index].command[0] != '\n') {
    if (spells[index].spellindex == define)
      return index;
  }

  return -1;
}

/*
 * mag_objectmagic: This is the entry-point for all magic items.
 *
 * staff  - [0]  level  [1] max charges  [2] num charges  [3] spell num
 * wand   - [0]  level  [1] max charges  [2] num charges  [3] spell num
 * scroll - [0]  level  [1] spell num  [2] spell num  [3] spell num
 * potion - [0] level  [1] spell num  [2] spell num  [3] spell num
 *
 * Staves and wands will default to level 14 if the level is not specified.
 */

void mag_objectmagic(struct char_data * ch, struct obj_data * obj, char *argument)
{
  int i, k;
  struct actd_msg *actd;
  struct char_data *tch = NULL, *next_tch;
  struct obj_data *tobj = NULL;
  int spellnum;

  one_argument(argument, arg);

  actd = get_actd(GET_OBJ_VNUM(obj)); /* get actd message */

  k = generic_find(arg, FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &tch, &tobj);
  if (actd) {
    if (!*arg) { /* No argument */
      act(actd->char_no_arg, FALSE, ch, obj, 0, TO_CHAR);
      act(actd->others_no_arg, TRUE, ch, obj, 0, TO_ROOM);
    } else if (tch == NULL) { /* Target not found */
      act(actd->not_found, FALSE, ch, obj, 0, TO_CHAR);
    } else if (tch == ch) { /* target is player */
      act(actd->char_auto, FALSE, ch, obj, tch, TO_CHAR);
      act(actd->others_auto, TRUE, ch, obj, tch, TO_ROOM);
    } else { /* target found, etc */
      act(actd->char_found, FALSE, ch, obj, tch, TO_CHAR);
      act(actd->others_found, TRUE, ch, obj, tch, TO_NOTVICT);
      act(actd->vict_found, FALSE, ch, obj, tch, TO_VICT);
    }
  }

  switch (GET_OBJ_TYPE(obj)) {
    case ITEM_STAFF:
      if (actd == NULL) {
        act("You tap $p three times on the ground.", FALSE, ch, obj, 0, TO_CHAR);
        if (obj->action_description) {
          act(obj->action_description, FALSE, ch, obj, 0, TO_ROOM);
        } else {
          act("$n taps $p three times on the ground.", FALSE, ch, obj, 0, TO_ROOM);
        }
      }

      if (GET_OBJ_VAL(obj, 2) <= 0) {
        act("It seems powerless.", FALSE, ch, obj, 0, TO_CHAR);
        act("Nothing seems to happen.", FALSE, ch, obj, 0, TO_ROOM);
      } else {
        GET_OBJ_VAL(obj, 2)--;
        WAIT_STATE(ch, PULSE_VIOLENCE);
        for (tch = world[ch->in_room].people; tch; tch = next_tch) {
          next_tch = tch->next_in_room;
          if (ch == tch) {
            continue;
          }
          spellnum = find_skill_num_def(GET_OBJ_VAL(obj, 3));
          DO_SPELL(&spells[spellnum], spellnum, 1, ch, GET_NAME(tch), YES);
        }
      }
      break;
    case ITEM_WAND:
      if (actd == NULL) {
        if (k == FIND_CHAR_ROOM) {
          if (tch == ch) {
            act("You point $p at yourself.", FALSE, ch, obj, 0, TO_CHAR);
            act("$n points $p at $mself.", FALSE, ch, obj, 0, TO_ROOM);
          } else {
            act("You point $p at $N.", FALSE, ch, obj, tch, TO_CHAR);
            if (obj->action_description != NULL) {
              act(obj->action_description, FALSE, ch, obj, tch, TO_ROOM);
            } else {
              act("$n points $p at $N.", TRUE, ch, obj, tch, TO_ROOM);
            }
          }
        } else if (tobj != NULL) {
          act("You point $p at $P.", FALSE, ch, obj, tobj, TO_CHAR);
          if (obj->action_description != NULL) {
            act(obj->action_description, FALSE, ch, obj, tobj, TO_ROOM);
          } else {
            act("$n points $p at $P.", TRUE, ch, obj, tobj, TO_ROOM);
          }
        } else {
          act("At what should $p be pointed?", FALSE, ch, obj, NULL, TO_CHAR);
          return;
        }
      }
      if (GET_OBJ_VAL(obj, 2) <= 0) {
        act("It seems powerless.", FALSE, ch, obj, 0, TO_CHAR);
        return;
      }
      GET_OBJ_VAL(obj, 2)--;
      WAIT_STATE(ch, PULSE_VIOLENCE);
      spellnum = find_skill_num_def(GET_OBJ_VAL(obj, 3));
      DO_SPELL(&spells[spellnum], spellnum, 1, ch, GET_NAME(tch), YES);
      break;
    case ITEM_SCROLL:
      if (*arg) {
        if (!k && actd == NULL) {
          act("There is nothing to here to affect with $p.", FALSE, ch, obj, NULL, TO_CHAR);
          return;
        }
      } else {
        tch = ch;
      }

      if (actd == NULL) {
        act("You recite $p which dissolves.", TRUE, ch, obj, 0, TO_CHAR);
        if (obj->action_description) {
          act(obj->action_description, FALSE, ch, obj, NULL, TO_ROOM);
        } else {
          act("$n recites $p.", FALSE, ch, obj, NULL, TO_ROOM);
        }
      }

      WAIT_STATE(ch, PULSE_VIOLENCE);
      for (i = 1; i < 4; i++) {
        if (GET_OBJ_VAL(obj, i)) {
          spellnum = find_skill_num_def(GET_OBJ_VAL(obj, i));
          if (*arg) {
            DO_SPELL(&spells[spellnum], spellnum, 1, ch, arg, YES);
          } else {
            DO_SPELL(&spells[spellnum], spellnum, 1, ch, GET_NAME(tch), YES);
          }
        }
      }

      if (obj != NULL) {
        extract_obj(obj);
      }
      break;
    case ITEM_POTION:
      tch = ch;
      if (actd == NULL) {
        act("As you quaff $p, it disappears in a flash of light!", FALSE, ch, obj, NULL, TO_CHAR);
        if (obj->action_description) {
          act(obj->action_description, FALSE, ch, obj, NULL, TO_ROOM);
        } else {
          act("$n quaffs $p.", TRUE, ch, obj, NULL, TO_ROOM);
        }
      }

      WAIT_STATE(ch, PULSE_VIOLENCE);
      for (i = 1; i < 4; i++)
        if (GET_OBJ_VAL(obj, i)) {
          spellnum = find_skill_num_def(GET_OBJ_VAL(obj, i));
          DO_SPELL(&spells[spellnum], spellnum, 1, ch, GET_NAME(tch), YES);
        }

      if (obj != NULL)
        extract_obj(obj);
      break;
    default:
      stderr_log("SYSERR: Unknown object_type in mag_objectmagic");
      break;
  }
}

/*
 * do_cast is the entry point for PC-casted spells.  It parses the arguments,
 * determines the spell number and finds a target, throws the die to see if
 * the spell can be cast, checks for sufficient mana and subtracts it, and
 * passes control to cast_spell().
 */

ACMD(do_cast)
{
  char *s, *t;
  int mana = 0, spellnum, waitstate = 1;
  struct obj_data *instr = ch->equipment[WEAR_HOLD];
  char abuf[255];
  struct char_data *vict;
  struct obj_data *vobj;
  event *tempevent;
  char *items[] = {"ration", "barrel", "torch", "raft", "\n"};
  if (subcmd == SCMD_SING) {
    if (!IS_BARD(ch)) {
      send_to_char("You are not a bard, and your singing seems to have a negative effect.\r\n", ch);
      return;
    } else {
      if (instr != NULL)
        if (GET_OBJ_TYPE(instr) != ITEM_INSTR) {
          send_to_char("You don't seem to have an instrument.", ch);
          return;
        }
      if (instr == NULL) {
        send_to_char("You need to hold an instrument, to be able to sing magic.", ch);
        return;
      }
    }
  }

  if (IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM))
    return;

  if (IS_BARD(ch) && subcmd != SCMD_SING) {
    send_to_char("You must sing to use magic.", ch);
    return;
  }
  if (!IS_NPC(ch) && GET_LEVEL(ch) == LVL_IMMORT && !COM_FLAGGED(ch, COM_IMMORT)) {
    send_to_char("You are too confused to cast any spells.\r\n", ch);
    return;
  }

  /* get: blank, spell name, target name */
  {
    size_t arglen = strlen(argument);
    safe_snprintf(argument + arglen, MAX_INPUT_LENGTH - arglen, " ");
  }
  s = strtok(argument, "'");
  if (s == NULL) {
    send_to_char("Cast what where?\r\n", ch);
    return;
  }

  s = strtok(NULL, "'");
  if (s == NULL) {
    send_to_char("Spell names must be enclosed in the Holy Magic Symbols: '\r\n", ch);
    return;
  }

  spellnum = find_spell_num(s);

  if (spellnum < 1) {
    send_to_char("Cast what?!?\r\n", ch);
    return;
  }
  t = strtok(NULL, "\0");
  if (!IS_NPC(ch)) {
    if (GET_SKILL(ch, spells[spellnum].spellindex) == 0) {
      send_to_char("You are unfamiliar with the words of the spell.\r\n", ch);
      return;
    }
  }

  /* Find the target */
  if (t != NULL) {
    safe_snprintf(arg, MAX_STRING_LENGTH, "%s", t);
    one_argument(arg, t);
    skip_spaces(&t);
  }
  if (!IS_NPC(ch)) {
    mana = mag_manacost(ch, spellnum);
    if ((mana > 0) && (GET_MANA(ch) < mana) && (GET_LEVEL(ch) < LVL_IMMORT)) {
      send_to_char("You haven't the energy to cast that spell!\r\n", ch);
      return;
    }

    vict = NULL;
    vobj = NULL;
    tempevent = spells[spellnum].event_pointer;
    if (t) {
      if (tempevent == spell_obj_char_event) {
        vict = get_char_room_vis(ch, t);
        vobj = get_obj_in_list_vis(ch, t, ch->carrying);
        if (!vobj) {
          vobj = get_obj_in_list_vis(ch, t, world[ch->in_room].contents);
        }
      } else if (tempevent == spell_dimdoor_event) {
        vict = get_char_vis(ch, t);
        if (vict && IS_NPC(vict)) {
          vict = NULL;
        }
      } else if (tempevent == spell_obj_room_event || tempevent == spell_obj_event) {
        vobj = get_obj_in_list_vis(ch, t, ch->carrying);
        if (!vobj) {
          vobj = get_obj_in_list_vis(ch, t, world[ch->in_room].contents);
        }
      } else if (tempevent == spell_locate_obj_event) {
        vobj = (struct obj_data*) 1;
      } else if (tempevent == spell_create_obj_event) {
        if (strcmp(spells[spellnum].command, "minor creation") == 0) {
          if (search_block(t, items, FALSE) == -1) {
            send_to_char("Options are: torch, ration, raft, barrel\r\n", ch);
            return;
          } else {
            vobj = (struct obj_data*) 1;
          }
        } else if (strcmp(spells[spellnum].command, "moonwell") == 0) {
          vict = get_char_vis(ch, t);
        } else if (strcmp(spells[spellnum].command, "create food") == 0) {
          vobj = (struct obj_data*) 1;
        } else if (strcmp(spells[spellnum].command, "create water") == 0) {
          vobj = get_obj_in_list_vis(ch, t, ch->carrying);
          if (!vobj) {
            vobj = get_obj_in_list_vis(ch, t, world[ch->in_room].contents);
          }
        } else if (strcmp(spells[spellnum].command, "create spring") == 0) {
          switch (GET_SECT(IN_ROOM(ch))) {
            case SECT_INSIDE:
            case SECT_CITY:
            case SECT_WATER_SWIM:
            case SECT_WATER_NOSWIM:
            case SECT_UNDERWATER:
            case SECT_FLYING:
              send_to_char("You cannot create springs here!\r\n", ch);
              return;
          }
        } else {
          vict = get_char_vis(ch, t);
        }
        if (!vict && !vobj) {
          send_to_char("Yes, but to whom or what?!\r\n", ch);
          return;
        } else if (vict && IS_NPC(vict) && MOB_FLAGGED(vict, MOB_NOKILL) && spells[spellnum].aggressive) {
          send_to_char("That's not a good idea.\r\n", ch);
          return;
        } else if (vict && IS_NPC(vict) && spells[spellnum].aggressive && GET_LEVEL(vict) >= GET_LEVEL(ch) && number(0, 100) < 10) {
          if (FIGHTING(vict)) {
            stop_fighting(vict);
            if (GET_POS(ch) >= POS_DEAD) {
              hit(vict, ch, TYPE_UNDEFINED);
              act("$n {Cswitches to a new target!{x", TRUE, vict, 0, ch, TO_ROOM);
            }
          } else {
            hit(vict, ch, TYPE_UNDEFINED);
          }
        }
      } else if (tempevent == spell_area_dam_event) {
        if (strcmp(spells[spellnum].command, "sea storm") == 0 || strcmp(spells[spellnum].command, "tidal wave") == 0) {
          if (GET_SECT(IN_ROOM(ch)) != SECT_WATER_SWIM && GET_SECT(IN_ROOM(ch)) != SECT_WATER_NOSWIM) {
            switch (GET_SECT(IN_ROOM(ch))) {
              case SECT_INSIDE:
                send_to_char("You cannot cause tidal waves inside.\r\n", ch);
                break;
              case SECT_CITY:
                send_to_char("You cannot cause tidal waves in the city.\r\n", ch);
                break;
              case SECT_UNDERWATER:
                send_to_char("You cannot cause tidal waves while underwater.\r\n", ch);
                break;
              case SECT_FLYING:
                send_to_char("You cannot cause tidal waves while flying.\r\n", ch);
                break;
              default:
                send_to_char("You cannot cause tidal waves on land.\r\n", ch);
                break;
            }
            return;
          }
        } else if (strcmp(spells[spellnum].command, "earthquake") == 0) {
          switch (GET_SECT(IN_ROOM(ch))) {
            case SECT_INSIDE:
              send_to_char("You cannot cause tidal waves inside.\r\n", ch);
              return;
            case SECT_CITY:
              send_to_char("You cannot cause tidal waves in the city.\r\n", ch);
              return;
            case SECT_UNDERWATER:
              send_to_char("You cannot cause earthquakes while underwater.\r\n", ch);
              return;
            case SECT_FLYING:
              send_to_char("You cannot cause earthquakes while flying.\r\n", ch);
              return;
            case SECT_WATER_SWIM:
            case SECT_WATER_NOSWIM:
              send_to_char("You cannot cause earthquakes on water.\r\n", ch);
              return;
          }
        }
      }
    }
    if (IS_MAGE(ch)) {
      if (spells[spellnum].delay) {
        waitstate = spells[spellnum].delay - (GET_INT(ch) / 10);
      } else {
        waitstate = 26 - ((GET_INT(ch) / 10) + (GET_SKILL(ch, spells[spellnum].realm) / 10));
      }
    } else {
      if (spells[spellnum].delay) {
        waitstate = spells[spellnum].delay - (GET_WIS(ch) / 10);
      } else {
        waitstate = 26 - ((GET_WIS(ch) / 10) + (GET_SKILL(ch, spells[spellnum].realm) / 10));
      }
    }
    if (IS_IMMO(ch)) {
      waitstate = 1;
    }
  }
  if (!t) {
    if (FIGHTING(ch)) {
      vict = FIGHTING(ch);
      one_argument(GET_PLR_NAME(vict), t);
    }
  } else {
    vobj = get_obj_in_list_vis(ch, t, ch->carrying);
    if (FIGHTING(ch) && !get_char_room_vis(ch, t)) {
      vict = FIGHTING(ch);
    } else {
      vict = get_char_room_vis(ch, t);
      if (find_spell_num("dimension door") == spellnum) {
        vict = get_char_vis(ch, t);
      }
    }
    if (!vict && !vobj) {
      if (find_spell_num("locate object") != spellnum && find_spell_num("moonwell") != spellnum && find_spell_num("minor creation") != spellnum && find_spell_num("conjure elemental") != spellnum && find_spell_num("word of recall") != spellnum && find_spell_num("create food") != spellnum && find_spell_num("create spring") != spellnum && tempevent != spell_area_event && tempevent != spell_area_dam_event && tempevent != spell_room_event && tempevent != spell_group_event && tempevent != spell_teleport_event) {
        send_to_char("You can't find your target to cast this spell.\r\n", ch);
        return;
      }
    }
    if (FIGHTING(ch) && !get_char_room_vis(ch, t)) {
      t = NULL;
    }
  }
  if (spells[spellnum].aggressive) {
    act("$n starts casting an offensive spell.", TRUE, ch, 0, 0, TO_ROOM);
  } else {
    act("$n starts casting a spell.", TRUE, ch, 0, 0, TO_ROOM);
  }
  if (IS_NPC(ch)) {
    DO_SPELL(&spells[spellnum], spellnum, 26 - ((GET_INT(ch)/10) + (GET_WIS(ch)/10)), ch, t, NO);
  } else {
    safe_snprintf(abuf, sizeof(abuf), "You start casting %s.\r\n", spells[spellnum].command);
    send_to_char(abuf, ch);
    DO_SPELL(&spells[spellnum], spellnum, waitstate, ch, t, NO);
  }
  if (mana > 0 && !COM_FLAGGED(ch, COM_IMMORT) && !IS_NPC(ch)) {
    GET_MANA(ch) = BOUNDED(0, GET_MANA(ch) - mana, GET_MAX_MANA(ch));
  }
}

ACMD(do_mpcast)
{
  char *s, *t;
  int spellnum;
  struct obj_data *instr = ch->equipment[WEAR_HOLD];
  char debugbuf[1024];

  if ((!IS_NPC(ch) && !COM_FLAGGED(ch, COM_MOB)) || IS_AFFECTED(ch, AFF_CHARM)) {
    send_to_char("Huh!?\r\n", ch);
    return;
  }

  if (subcmd == SCMD_SING) {
    if (!IS_BARD(ch)) {
      send_to_char("You are not a bard, and your singing seems to have a negative effect.\r\n", ch);
      return;
    } else {
      if (instr != NULL)
        if (GET_OBJ_TYPE(instr) != ITEM_INSTR) {
          send_to_char("You don't seem to have an instrument.", ch);
          return;
        }
      if (instr == NULL) {
        send_to_char("You need to hold an instrument, to be able to sing magic.", ch);
        return;
      }
    }
  }

  if (IS_BARD(ch) && subcmd != SCMD_SING) {
    send_to_char("You must sing to use magic.", ch);
    return;
  }
  /* get: blank, spell name, target name */

  s = strtok(argument, "'");

  if (s == NULL) {
    send_to_char("Cast what where?\r\n", ch);
    return;
  }

  s = strtok(NULL, "'");

  if (s == NULL) {
    send_to_char("Spell names must be enclosed in the Holy Magic Symbols: '\r\n", ch);
    return;
  }

  t = strtok(NULL, "\0");

  spellnum = find_spell_num(s);

  if (spellnum < 1) {
    send_to_char("Cast what?!?\r\n", ch);
    return;
  }

  safe_snprintf(debugbuf, sizeof(debugbuf), "MPCAST: %s casting '%s'", IS_NPC(ch) ? GET_MOB_NAME(ch) : GET_NAME(ch), spells[spellnum].command);

  /* Find the target */
  if (t != NULL) {
    safe_snprintf(arg, MAX_STRING_LENGTH, "%s", t);
    one_argument(arg, t);
    skip_spaces(&t);
  }

  DO_SPELL(&spells[spellnum], spellnum, 1, ch, t, MPCAST);
}

ACMD(do_spells)
{
  ACMD(do_practice);
  extern int parse_class_spec(char *arg);
  int i, class;

  if (IS_NPC(ch)) {
    send_to_char("Drop dead!\r\n", ch);
    return;
  }

  if (!IS_IMMO(ch)) {
    do_practice(ch, argument, cmd, subcmd);
    return;
  }

  one_argument(argument, arg);
  if (!*arg) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "You know of the following spells:\r\n");
    safe_snprintf(buf2, MAX_STRING_LENGTH, "%s", buf);

    for (i = 1; spells[i].command[0] != '\n'; i++) {
      if (spells[i].spell_pointer) {
        if (strlen(buf2) >= MAX_STRING_LENGTH - 32) {
          safe_snprintf(buf2 + strlen(buf2), MAX_STRING_LENGTH - strlen(buf2), "**OVERFLOW**\r\n");
          break;
        }
        if ((GET_LEVEL(ch) >= spells[i].min_level[(int) GET_CLASS(ch)] || GET_SKILL(ch, i))) {
          safe_snprintf(buf2 + strlen(buf2), MAX_STRING_LENGTH - strlen(buf2), "%2d: %-20s  Mana: %d\r\n", spells[i].min_level[(int) GET_CLASS(ch)], spells[i].command, mag_manacost(ch, i));
        }
      }
    }
  } else {
    class = parse_class_spec(arg);
    if (class == CLASS_UNDEFINED) {
      send_to_char("I have never heard of that class.\r\n", ch);
      return;
    }

    sprinttype(class, pc_class_types, buf);
    safe_snprintf(buf2, MAX_STRING_LENGTH, "%ss can use the following spells:\r\n", buf);

    for (i = 1; spells[i].command[0] != '\n'; i++) {
      if (spells[i].spell_pointer) {
        if (strlen(buf2) >= MAX_STRING_LENGTH - 32) {
          safe_snprintf(buf2 + strlen(buf2), MAX_STRING_LENGTH - strlen(buf2), "**OVERFLOW**\r\n");
          break;
        }
        if (spells[i].min_level[class] < LVL_IMMORT) {
          safe_snprintf(buf2 + strlen(buf2), MAX_STRING_LENGTH - strlen(buf2), "%2d: %-30s\r\n", spells[i].min_level[class], spells[i].command);
        }
      }
    }
  }
  page_string(ch->desc, buf2, 1);
}

char *get_spell_name(int spellnum)
{
  int i;
  for (i = 1; spells[i].command[0] != '\n'; i++)
    if (spellnum == spells[i].spellindex)
      return spells[i].command;
  return "Unknown";
}
