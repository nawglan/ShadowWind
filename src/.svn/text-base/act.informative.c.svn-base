/*************************************************************************
 *   File: act.informative.c                             Part of CircleMUD *
 *  Usage: Player-level commands of an informative nature                  *
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
#include <time.h>
#include <errno.h>
#include <sys/time.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "event.h"
#include "spells.h"
#include "screen.h"

/* extern variables */
extern struct zone_data *zone_table; /* zone table       */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern struct obj_data *object_list;
extern struct index_data *obj_index;
extern struct index_data *mob_index;
extern int MaxExperience[LVL_IMMORT + 2];
extern const struct command_info cmd_info[];
extern int file_to_string_alloc(char *name, char **buf);
char *get_spell_name(int spellnum);
extern const char *sector_types[];
void stop_follower(struct char_data *ch);
int is_class(char *arg);
int is_race(char *arg);
int find_skill_num_def(int spellindex);

extern int max_players;
extern int SECS_PER_MUD_HOUR;

extern char *credits;
extern char *news;
extern char *info;
extern char *motd;
extern char *nmotd;
extern char *imotd;
extern char *wizlist;
extern char *immlist;
extern char *policies;
extern char *namepol;
extern char *signoff;
extern char *quest;
extern char *handbook;
extern char *dirs[];
extern char *where[];
extern char *color_liquid[];
extern char *fullness[];
extern char *class_abbrevs[];
extern char *room_bits[];
extern struct spell_info_type *spells;
extern char *weapon_types[];
extern char *pc_class_types[];
extern char *pc_race_types[];
extern char *pc_race_types_color[];
extern int equip_order[NUM_WEARS];

void display_who_list(struct char_data *ch, int minlev, int maxlev, int class, int race, char *name);

ACMD(do_exp)
{
  float exp_percent;

  if (GET_LEVEL(ch) == LVL_IMMORT) {
    send_to_char("You have already achieved the ultimate goal.\r\n", ch);
    return;
  }

  exp_percent = (GET_EXP(ch) - MaxExperience[(int) GET_LEVEL(ch)]);
  exp_percent /= (MaxExperience[(int) GET_LEVEL(ch) + 1] - MaxExperience[(int) GET_LEVEL(ch)]);

  exp_percent *= 100;

  if (exp_percent <= 10) {
    send_to_char("You have just started the journey to your next level.\r\n", ch);
  } else if (exp_percent > 10 && exp_percent <= 15) {
    send_to_char("You have gained some headway.\r\n", ch);
  } else if (exp_percent > 15 && exp_percent <= 20) {
    send_to_char("You are still a long ways from your next level.\r\n", ch);
  } else if (exp_percent > 20 && exp_percent <= 25) {
    send_to_char("You are nearing the one quarter mark.\r\n", ch);
  } else if (exp_percent > 25 && exp_percent <= 30) {
    send_to_char("You are at the first quarter mark of your next level.\r\n", ch);
  } else if (exp_percent > 30 && exp_percent <= 35) {
    send_to_char("You are just past the first quarter mark of your next level.\r\n", ch);
  } else if (exp_percent > 35 && exp_percent <= 40) {
    send_to_char("You have gained some progress.\r\n", ch);
  } else if (exp_percent > 40 && exp_percent <= 45) {
    send_to_char("You are quite a way from your next level.\r\n", ch);
  } else if (exp_percent > 45 && exp_percent <= 50) {
    send_to_char("You are nearing the halfway point to your next level.\r\n", ch);
  } else if (exp_percent > 50 && exp_percent <= 55) {
    send_to_char("You are at the halfway point to your next level.\r\n", ch);
  } else if (exp_percent > 55 && exp_percent <= 60) {
    send_to_char("You are just past the halfway point to your next level.\r\n", ch);
  } else if (exp_percent > 60 && exp_percent <= 65) {
    send_to_char("You have gained much progress.\r\n", ch);
  } else if (exp_percent > 65 && exp_percent <= 70) {
    send_to_char("You are well on your way to the third quarter mark.\r\n", ch);
  } else if (exp_percent > 70 && exp_percent <= 75) {
    send_to_char("You are nearing the three quarter mark.\r\n", ch);
  } else if (exp_percent > 75 && exp_percent <= 80) {
    send_to_char("You are at the third quarter mark of your next level.\r\n", ch);
  } else if (exp_percent > 80 && exp_percent <= 85) {
    send_to_char("You are just past the third quarter mark of your next level.\r\n", ch);
  } else if (exp_percent > 85 && exp_percent <= 90) {
    send_to_char("You are well on your way.\r\n", ch);
  } else if (exp_percent > 90 && exp_percent <= 95) {
    send_to_char("You are well along the path to your next level.\r\n", ch);
  } else if (exp_percent > 95 && exp_percent <= 98) {
    send_to_char("Your level draws near.\r\n", ch);
  } else if (exp_percent > 98 && exp_percent <= 100) {
    send_to_char("You should level anytime now.\r\n", ch);
  } else if (exp_percent > 100) {
    send_to_char("You should have already leveled!!!\r\n", ch);
  }
}

ACMD(do_whois)
{

  struct char_data *vict;

  one_argument(argument, arg);

  if (*arg) {
    if (!(vict = get_char_vis(ch, arg)) || IS_NPC(vict)) {
      send_to_char("Who is that?\r\n", ch);
      return;
    }
    if (!PRF_FLAGGED(vict, PRF_WHOIS) && !COM_FLAGGED(ch, COM_ADMIN)) {
      send_to_char("That player does not allow you to retrieve WHOIS information.\r\n", ch);
      return;
    }
  } else {
    send_to_char("Who do you want to know more about?\r\n", ch);
    return;
  }

  sprintf(buf, "%s is a ", GET_NAME(vict));

  switch (vict->player.sex) {
    case SEX_NEUTRAL:
      strcat(buf, "neutral-sex");
      break;
    case SEX_MALE:
      strcat(buf, "male");
      break;
    case SEX_FEMALE:
      strcat(buf, "female");
      break;
    default:
      strcat(buf, "ILLEGAL-SEX!!");
      break;
  }
  if (PRF_FLAGGED(vict, PRF_ANONYMOUS) && !COM_FLAGGED(ch, COM_ADMIN)) {
    sprintf(buf, "%s level-ANON ", buf);
  } else {
    sprintf(buf, "%s level-%d ", buf, GET_LEVEL(vict));
  }
  sprinttype(vict->player_specials->saved.race, pc_race_types, buf2);
  strcat(buf, buf2);
  sprintf(buf, "%s ", buf);
  sprinttype(vict->player.class, pc_class_types, buf2);
  strcat(buf, buf2);
  strcat(buf, ".\r\n");
  send_to_char(buf, ch);

  if (IS_IMMO(ch)) {
    sprintf(buf, "Started: %-20.16s  Last: %-20.16s  Played: %3dh %2dm\r\n", ctime(&vict->player.time.birth), ctime(&vict->player.time.logon), (int) (vict->player.time.played / 3600), (int) (vict->player.time.played / 60 % 60));
    send_to_char(buf, ch);
  }
}

ACMD(do_evaluate)
{
  struct char_data *found_char = NULL;
  struct obj_data *found_obj = NULL;

  one_argument(argument, arg);
  if (!*arg) {
    send_to_char("Want me to guess which weapon you want to evaluate??\r\n", ch);
    return;
  }
  generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &found_char, &found_obj);
  if (found_obj == NULL) {
    send_to_char("You cant find any such weapon to evaluate.\r\n", ch);
    return;
  }
  if (GET_OBJ_TYPE(found_obj) == ITEM_WEAPON) {
    sprintf(buf, "$p seems to be a %s-like weapon.", weapon_types[GET_OBJ_VAL(found_obj,3)]);
    act(buf, FALSE, ch, found_obj, 0, TO_CHAR);
  } else {
    act("$p is not a weapon.", FALSE, ch, found_obj, 0, TO_CHAR);
  }
}

void show_obj_to_char(struct obj_data * object, struct char_data * ch, int mode)
{
  int i;

  *buf = '\0';
  if ((mode == 0) && (object->description || object->cdescription)) {
    if (object->cdescription) {
      strcpy(buf, object->cdescription);
    } else {
      strcpy(buf, object->description);
    }
  } else if ((object->cshort_description || object->short_description) && ((mode == 1) || (mode == 2) || (mode == 3) || (mode == 4))) {
    if (object->cshort_description) {
      strcpy(buf, object->cshort_description);
    } else {
      strcpy(buf, object->short_description);
    }
  } else if (mode == 5) {
    if (GET_OBJ_TYPE(object) == ITEM_SPELLBOOK) {
      if (!IS_IMMO(ch) && GET_OBJ_VAL(object, 2) != -1 && GET_OBJ_VAL(object, 2) != GET_CLASS(ch)) {
        send_to_char("It appears to be written in a language you don't understand.\r\n", ch);
        return;
      }
      sprintf(buf, "It appears to have the following spells written in it:\r\n");
      send_to_char(buf, ch);
      for (i = 0; i < GET_OBJ_VAL(object, 0); i++) {
        if (GET_OBJ_SPELLLISTNUM(object, i)) {
          sprintf(buf, "%s\r\n", spells[find_skill_num_def(GET_OBJ_SPELLLISTNUM(object, i))].command);
          send_to_char(buf, ch);
        }
      }
      sprintf(buf, "There are %d pages left in this spellbook.\r\n", GET_OBJ_VAL(object, 1));
      send_to_char(buf, ch);
      return;
    }
    if (GET_OBJ_TYPE(object) == ITEM_NOTE) {
      if (object->action_description) {
        strcpy(buf, "There is something written upon it:\r\n\r\n");
        strcat(buf, object->action_description);
        page_string(ch->desc, buf, 1);
      } else {
        act("It's blank.", FALSE, ch, 0, 0, TO_CHAR);
      }
      return;
    } else if (GET_OBJ_TYPE(object) != ITEM_DRINKCON) {
      strcpy(buf, "You see nothing special..");
    } else { /* ITEM_TYPE == ITEM_DRINKCON||FOUNTAIN */
      strcpy(buf, "It looks like a drink container.");
    }
  }
  if (mode != 3) {
    if (IS_OBJ_STAT(object, ITEM_INVISIBLE)) {
      strcat(buf, " (invis)");
    }
    if ((IS_OBJ_STAT(object, ITEM_BLESS) && (IS_AFFECTED(ch, AFF_DETECT_ALIGN)))) {
      strcat(buf, " ..It glows {Bblue!{x");
    }
    if (IS_OBJ_STAT(object, ITEM_MAGIC) && IS_AFFECTED(ch, AFF_DETECT_MAGIC)) {
      strcat(buf, " ..It glows {Yyellow!{x");
    }
    if (IS_OBJ_STAT(object, ITEM_GLOW)) {
      strcat(buf, "  {w({Cglowing{w){x");
    }
    if (IS_OBJ_STAT(object, ITEM_HUM)) {
      strcat(buf, " {w({Whumming{w){x");
    }
    if (IS_OBJ_STAT(object, ITEM_ISLIGHT)) {
      strcat(buf, " {b({WIlluminating{b){x");
    } else if (IS_OBJ_STAT(object, ITEM_ISLIGHTDIM)) {
      strcat(buf, " {b({wIlluminating{b){x");
    }
  }
  strcat(buf, "\r\n");
  page_string(ch->desc, buf, 1);
}

void list_obj_to_char(struct obj_data * list, struct char_data * ch, int mode, bool show)
{
  struct obj_data *i, *j;
  char buf[10];
  bool found;
  int num;

  found = FALSE;
  for (i = list; i; i = i->next_content) {
    num = 0;
    for (j = list; j != i; j = j->next_content) {
      if (j->item_number == NOTHING) {
        if (strcmp(j->short_description, i->short_description) == 0) {
          break;
        }
      } else {
        if (j->item_number == i->item_number) {
          if (j->cshort_description && i->cshort_description && strcmp(j->cshort_description, i->cshort_description) == 0) {
            break;
          }
          if (!j->cshort_description && !i->cshort_description) {
            break;
          }
        }
      }
    }
    if (j != i) {
      continue;
    }
    for (j = i; j; j = j->next_content) {
      if (j->item_number == NOTHING) {
        if (j->short_description && i->short_description && strcmp(j->short_description, i->short_description) == 0) {
          num++;
        }
      } else {
        if (j->item_number == i->item_number) {
          if (j->cshort_description && i->cshort_description && strcmp(j->cshort_description, i->cshort_description) == 0) {
            num++;
          } else {
            if (!j->cshort_description && !i->cshort_description) {
              num++;
            }
          }
        }
      }
    }

    if (CAN_SEE_OBJ(ch, i) && !((i->obj_flags.extra_flags & ITEM_NOT_OBV) && (i->carried_by == NULL))) {
      if (num != 1) {
        sprintf(buf, "[%2i] ", num);
        send_to_char(buf, ch);
      }
      show_obj_to_char(i, ch, mode);
      found = TRUE;
    }
  }
  if (!found && show) {
    send_to_char(" Nothing.\r\n", ch);
  }
}

void diag_char_to_char(struct char_data * i, struct char_data * ch)
{
  int percent;

  if (GET_MAX_HIT(i) > 0) {
    percent = (100 * GET_HIT(i)) / GET_MAX_HIT(i);
  } else {
    percent = -1; /* How could MAX_HIT be < 1?? */
  }

  strcpy(buf, PERS(i, ch));
  CAP(buf);

  if (percent >= 100) {
    strcat(buf, " is in {Gexcellent{x condition.\r\n");
  } else if (percent >= 90) {
    strcat(buf, " has a {yfew scratches.{x\r\n");
  } else if (percent >= 75) {
    strcat(buf, " has some {Ysmall wounds and bruises.{x\r\n");
  } else if (percent >= 50) {
    strcat(buf, " has {Mquite a few wounds.{x\r\n");
  } else if (percent >= 30) {
    strcat(buf, " has some {mbig nasty wounds and scratches.{x\r\n");
  } else if (percent >= 15) {
    strcat(buf, " looks {Rpretty hurt.{x\r\n");
  } else if (percent >= 0) {
    strcat(buf, " is in {rawful condition.{x\r\n");
  } else {
    strcat(buf, " is {R{Fbleeding awfully from big wounds.{x\r\n");
  }

  send_to_char(buf, ch);
  if (AFF2_FLAGGED(i, AFF2_STONESKIN)) {
    act("{D$s skin appears to be hard as stone!{x", TRUE, i, 0, ch, TO_VICT);
  }
  if (AFF2_FLAGGED(i, AFF2_FIRESHIELD)) {
    act("{R$s body is encased in flames!{x", TRUE, i, 0, ch, TO_VICT);
  }
  if (AFF2_FLAGGED(i, AFF2_ICESHIELD)) {
    act("{B$s body is encased in ice!{x", TRUE, i, 0, ch, TO_VICT);
  }
  if (AFF2_FLAGGED(i, AFF2_BARKSKIN)) {
    act("{y$s body is covered in a barklike skin.{x", TRUE, i, 0, ch, TO_VICT);
  }
}

void look_at_char(struct char_data * i, struct char_data * ch)
{
  int j, found;
  struct obj_data *tmp_obj;

  if (GET_DESC(i)) {
    send_to_char(GET_DESC(i), ch);
  } else {
    act("You see nothing special about $m.", FALSE, i, 0, ch, TO_VICT);
  }

  diag_char_to_char(i, ch);

  found = FALSE;
  for (j = 0; !found && j < NUM_WEARS; j++) {
    if (i->equipment[j] && CAN_SEE_OBJ(ch, i->equipment[j])) {
      found = TRUE;
    }
  }

  if (found) {
    act("\r\n$n is using:", FALSE, i, 0, ch, TO_VICT);
    for (j = 0; j < NUM_WEARS; j++) {
      if (i->equipment[equip_order[j]] && CAN_SEE_OBJ(ch, i->equipment[equip_order[j]])) {
        send_to_char(where[equip_order[j]], ch);
        show_obj_to_char(i->equipment[equip_order[j]], ch, 1);
      }
    }
  }
  if (ch != i && GET_LEVEL(i) <= GET_LEVEL(ch) && (GET_CLASS(ch) == CLASS_THIEF || GET_LEVEL(ch) >= LVL_IMMORT || GET_CLASS(ch) == CLASS_ROGUE)) {
    found = FALSE;
    act("\r\nYou attempt to peek at $s inventory:", FALSE, i, 0, ch, TO_VICT);
    for (tmp_obj = i->carrying; tmp_obj; tmp_obj = tmp_obj->next_content) {
      if (CAN_SEE_OBJ(ch, tmp_obj) && (number(0, 20) < GET_LEVEL(ch))) {
        show_obj_to_char(tmp_obj, ch, 1);
        found = TRUE;
      }
    }

    if (!found) {
      send_to_char("You can't see anything.\r\n", ch);
    }
  }
}

void list_one_char(struct char_data * i, struct char_data * ch)
{
  char *positions[] = {
      " is lying here, dead.{x",
      " is lying here, mortally wounded.{x",
      " is lying here, incapacitated.{x",
      " is lying here, stunned.{x",
      " is sleeping{x",
      " is resting{x",
      " is sitting{x",
      " is standing{x", /*fighting stance*/
      " is standing{x"
  };
  char pcolor[5];
  int j = 0;

  if (IS_NPC(i)) {
    strcpy(pcolor, "{w");
    send_to_char("{w", ch);
  } else {
    strcpy(pcolor, "{w");
    send_to_char("{w", ch);
  }

  if (IS_NPC(i) && GET_LDESC(i) && GET_POS(i) == GET_DEFAULT_POS(i)) {
    *buf = '\0';

    if (IS_AFFECTED(i, AFF_INVISIBLE)) {
      sprintf(buf, "*%s", pcolor);
      if (IS_ANIMATED(i)) {
        sprintf(buf, "%s%s", buf, GET_ADESC(i));
      } else {
        sprintf(buf, "%s%s", buf, GET_LDESC(i));
      }
    } else {
      if (IS_ANIMATED(i)) {
        sprintf(buf, "%s", GET_ADESC(i));
      } else {
        sprintf(buf, "%s", GET_LDESC(i));
      }
    }
    j = strlen(buf);
    while (buf[j - 1] == '\n' || buf[j - 1] == '\r' || buf[j - 1] == ' ') {
      buf[j - 1] = '\0';
      j--;
    }
    if (IS_AFFECTED(ch, AFF_DETECT_ALIGN)) {
      if (IS_EVIL(i)) {
        sprintf(buf, "%s%s", buf, " {r({rRed Aura{r){x");
      } else if (IS_GOOD(i)) {
        sprintf(buf, "%s%s", buf, " {Y({YGold Aura{Y){x");
      }
    }

    if (IS_AFFECTED(i, AFF_FLY)) {
      sprintf(buf, "%s%s", buf, " {b({yFlying{b)");
    }
    if (HUNTING(i)) {
      sprintf(buf, "%s%s", buf, " {b({gHunting{b)");
    }
    if (IS_AFFECTED(i, AFF_CHARM)) {
      sprintf(buf, "%s%s", buf, " {b({mCharmed{b)");
    }
    if (IS_AFFECTED(i, AFF_BLIND)) {
      sprintf(buf, "%s%s", buf, " {b({dBlind{b){x");
    }
    if (IS_AFFECTED(i, AFF_HIDE)) {
      sprintf(buf, "%s%s", buf, " {b({DHidden{b){x");
    }

    sprintf(buf, "%s%s", buf, "\r\n");
    send_to_char(buf, ch);

    return;
  }

  if (IS_NPC(i)) {
    if (IS_AFFECTED(i, AFF_INVISIBLE)) {
      strcpy(buf, "*");
      strcat(buf, CAP(i->player.short_descr));
    } else {
      strcpy(buf, i->player.short_descr);
      CAP(buf);
    }
  } else {
    if (IS_AFFECTED(i, AFF_INVISIBLE)) {
      /*if (GET_LEVEL(i) < LVL_IMMORT && !IS_NPC(i)) {*/
      sprintf(buf, "*%s", i->player.name);
      /*} else if (NAMECOLOR(i)) {
       sprintf(buf, "*%s%s", NAMECOLOR(i), i->player.name);
       } else {
       sprintf(buf, "*{w%s", i->player.name);
       }*/
    } else {
      /*if (!IS_NPC(i) && GET_LEVEL(i) < LVL_IMMORT) {*/
      sprintf(buf, "%s", i->player.name);
      /*} else {
       if (!i->desc) {
       sprintf(buf, "{w%s", i->player.name);
       } else {
       if (NAMECOLOR(i)) {
       sprintf(buf, "%s%s", NAMECOLOR(i), i->player.name);
       } else {
       sprintf(buf, "{w%s", i->player.name);
       }
       }
       }*/
    }
  }
  if (!IS_NPC(i)) {
    if (GET_RACE(i) == RACE_HUMAN) {
      sprintf(buf, "%s {W({cHuman{W)%s", buf, pcolor);
    }
    if (GET_RACE(i) == RACE_TROLL) {
      sprintf(buf, "%s {W({gTroll{W)%s", buf, pcolor);
    }
    if (GET_RACE(i) == RACE_OGRE) {
      sprintf(buf, "%s {W({yOgre{W)%s", buf, pcolor);
    }
    if (GET_RACE(i) == RACE_DWARF) {
      sprintf(buf, "%s {W({YDwarf{W)%s", buf, pcolor);
    }
    if (GET_RACE(i) == RACE_ELF) {
      sprintf(buf, "%s {W({GElf{W)%s", buf, pcolor);
    }
    if (GET_RACE(i) == RACE_HALFELF) {
      sprintf(buf, "%s {W({CHalf-Elf{W)%s", buf, pcolor);
    }
    if (GET_RACE(i) == RACE_GNOME) {
      sprintf(buf, "%s {W({RGnome{W)%s", buf, pcolor);
    }
    if (GET_RACE(i) == RACE_HALFLING) {
      sprintf(buf, "%s {W({mHalfing{W)%s", buf, pcolor);
    }
    if (GET_RACE(i) == RACE_UNDEFINED) {
      sprintf(buf, "%s {W({ME:Notify Admin{W)%s", buf, pcolor);
    }
  }
  if (IS_AFFECTED(i, AFF_HIDE)) {
    sprintf(buf, "%s {W({Dhidden{W)%s", buf, pcolor);
  }
  if (!IS_NPC(i) && !i->desc) {
    sprintf(buf, "%s {W({Rlinkless{W)%s", buf, pcolor);
  }
  if (PLR_FLAGGED(i, PLR_WRITING)) {
    sprintf(buf, "%s {b({Wwriting{b)%s", buf, pcolor);
  }
  if (PLR_FLAGGED(i, PLR_THIEF)) {
    sprintf(buf, "%s {b({RTHIEF{b)%s", buf, pcolor);
  }
  if (PLR_FLAGGED(i, PLR_KILLER)) {
    sprintf(buf, "%s {b({RKILLER{b)%s", buf, pcolor);
  }
  if (PLR_FLAGGED(i, PLR_EDITING)) {
    sprintf(buf, "%s {b({Wediting{b)%s", buf, pcolor);
  }
  if (!FIGHTING(i)) {
    if (!IS_NPC(i)) {
      sprintf(buf, "%s%s", buf, positions[(int) GET_POS(i)]);
      if (IS_AFFECTED(i, AFF_FLY)) {
        sprintf(buf, "%s%s", buf, " in mid-air here.");
      } else if (IS_AFFECTED2(i, AFF2_LEVITATE)) {
        sprintf(buf, "%s%s", buf, " a few inches off the ground here.");
      } else {
        sprintf(buf, "%s%s", buf, " here.");
      }
    }
    /*
     if (GET_LEVEL(i) >= LVL_IMMORT && !IS_NPC(i)) {
     if (!i->desc) {
     sprintf(buf, "%s%s", buf, " {wis here.");
     } else {
     if (NAMECOLOR(i)) {
     sprintf(buf, "%s %s %sis here.{x", buf, GET_ROOMTITLE(i), NAMECOLOR(i));
     } else {
     sprintf(buf, "%s %s {wis here.{x", buf, GET_ROOMTITLE(i));
     }
     if (PLR_FLAGGED(i, PLR_EDITING)) {
     sprintf(buf, "%s {b({Wediting{b)%s", buf, pcolor);
     }
     if (PLR_FLAGGED(i, PLR_WRITING)) {
     sprintf(buf, "%s {b({Wwriting{b)%s", buf, pcolor);
     }
     }
     }
     */
  } else {
    if (FIGHTING(i)) {
      sprintf(buf, "%s%s", buf, positions[(int) GET_POS(i)]);
      sprintf(buf, "%s%s", buf, " here, fighting ");
      if (FIGHTING(i) == ch) {
        sprintf(buf, "%s%s", buf, "{YYOU!{x");
      } else {
        if (i->in_room == FIGHTING(i)->in_room) {
          sprintf(buf, "%s%s", buf, PERS(FIGHTING(i), ch));
        } else {
          sprintf(buf, "%s%s", buf, "someone who has already left");
        }
        sprintf(buf, "%s%s", buf, "!");
      }
    } else { /* NIL fighting pointer */
      sprintf(buf, "%s%s", buf, " is here struggling with thin air.");
    }
  }
  if (IS_AFFECTED(ch, AFF_DETECT_ALIGN) && !FIGHTING(i)) {
    if (IS_EVIL(i)) {
      sprintf(buf, "%s%s", buf, " {r({rRed Aura{r){x");
    } else if (IS_GOOD(i)) {
      sprintf(buf, "%s%s", buf, " {Y({YGold Aura{Y){x");
    }
  }

  if (IS_AFFECTED(i, AFF_CHARM)) {
    sprintf(buf, "%s%s", buf, " {b({mcharmed{b){x");
  }
  if (IS_AFFECTED(i, AFF_BLIND)) {
    sprintf(buf, "%s%s", buf, " {b({dblind{b){x");
  }
  if (HUNTING(i)) {
    sprintf(buf, "%s%s", buf, " {b({ghunting{b){x");
  }
  if (!FIGHTING(i)) {
    if (PRF_FLAGGED(i, PRF_AFK)) {
      sprintf(buf, "%s%s", buf, " {W({RAFK{W){x");
    }
  }
  sprintf(buf, "%s%s", buf, "\r\n");
  send_to_char(buf, ch);

  send_to_char(CCNRM(ch, C_SPR), ch);
}

void list_char_to_char(struct char_data * list, struct char_data * ch)
{
  struct char_data *i;

  for (i = list; i; i = i->next_in_room) {
    if (ch != i) {
      if (CAN_SEE(ch, i)) {
        list_one_char(i, ch);
      } else if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch) && IS_AFFECTED(ch, AFF_INFRAVISION)) {
        send_to_char("{rYou see a red shape.{x\r\n", ch);
      }
    }
  }
}

void do_auto_exits(struct char_data * ch)
{
  int door;

  *buf = '\0';

  for (door = 0; door < NUM_OF_DIRS; door++) {
    if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE && !IS_SET(EXIT(ch, door)->exit_info, EX_HIDDEN)) {
      if (!IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)) {
        sprintf(buf, "%s %c", buf, LOWER(*dirs[door]));
      } else {
        sprintf(buf, "%s {x#{R%c", buf, LOWER(*dirs[door]));
      }
    }
  }

  sprintf(buf2, "{B[{WExits:{R%s{B]{x\r\n", *buf ? buf : " None!");

  send_to_char(buf2, ch);
}

const char *armor_class[] = {
    "{rYour armor is equivalent to that of an ancient dragon.{x",/* -100+ */
    "You are ready to kick some ass!.", /* -90 - -100 */
    "You are ready to tank.", /* -80 - -90 */
    "You are almost ready to tank.", /* -70 - -80 */
    "Your armor class is better than good.", /* -60 - -70 */
    "Your armor class is good.", /* -50 - -60 */
    "Your armor class is below good.", /* -40 - -50 */
    "Your armor class is well above average.", /* -30 - -40 */
    "Your armor class is above average.", /* -20 - -30 */
    "Your armor class is slightly above average.",/* -10 - -20 */
    "Your armor class is average.", /* 0 - -10 */
    "Your armor class is slightly below average.",/* 10-0 */
    "Your armor class is below average.", /* 20-10 */
    "Your armor class is well above awful.", /* 30-20 */
    "Your armor class is above awful.", /* 40-30 */
    "Your armor class is slightly above awful.", /* 50-40 */
    "Your armor class is awful.", /* 60-50 */
    "Your armor class is slightly below awful.", /* 70-60 */
    "Your armor class is below awful.", /* 80-70 */
    "Your armor class is horrible.", /* 90-80 */
    "What armor class, your NAKED!?!", /* 100-90 */
};

char *stats_msg(int stat)
{
  if (stat <= 30) {
    return "bad    ";
  }
  if (stat <= 50) {
    return "fair   ";
  }
  if (stat <= 65) {
    return "average";
  }
  if (stat <= 99) {
    return "good   ";
  }
  return "excellent";
}

char *align_msg(int stat)
{
  if (stat <= -900) {
    return "extremely evil";
  }
  if (stat <= -350) {
    return "evil";
  }
  if (stat <= -250) {
    return "leaning towards evil";
  }
  if (stat <= 250) {
    return "neutral";
  }
  if (stat <= 350) {
    return "leaning towards good";
  }
  if (stat <= 900) {
    return "good";
  }
  return "extremely good";
}

ACMD(do_stats)
{
  int calc_thac0 = 0;

  extern int thaco[NUM_CLASSES][2];
  extern sh_int stats[11][101];
  extern sh_int monk_stat[LVL_IMMORT + 1][5];

  sprintf(buf, "{c        Attribute information for: {y%s\r\n\r\n", GET_NAME(ch));
  sprintf(buf, "%s{cLevel: {w%2d      {cRace:{C %s    {cClass: {G%s\r\n", buf, GET_LEVEL(ch), pc_race_types[(int) GET_RACE(ch)], pc_class_types[(int) GET_CLASS(ch)]);
  send_to_char(buf, ch);

  if (GET_LEVEL(ch) < 10) {
    sprintf(buf, "{cSTR: {W%9s ", stats_msg(GET_STR(ch)));
    sprintf(buf, "%s{cINT: {W%9s ", buf, stats_msg(GET_INT(ch)));
    sprintf(buf, "%s{cWIS: {W%9s\r\n{x", buf, stats_msg(GET_WIS(ch)));
    sprintf(buf, "%s{cDEX: {W%9s ", buf, stats_msg(GET_DEX(ch)));
    sprintf(buf, "%s{cCON: {W%9s ", buf, stats_msg(GET_CON(ch)));
    sprintf(buf, "%s{cAGI: {W%9s\r\n{x", buf, stats_msg(GET_AGI(ch)));
    send_to_char(buf, ch);
  } else {
    sprintf(buf, "{cSTR: {W%3d ", GET_STR(ch));
    sprintf(buf, "%s{cINT: {W%3d ", buf, GET_INT(ch));
    sprintf(buf, "%s{cWIS: {W%3d\r\n{x", buf, GET_WIS(ch));
    sprintf(buf, "%s{cDEX: {W%3d ", buf, GET_DEX(ch));
    sprintf(buf, "%s{cCON: {W%3d ", buf, GET_CON(ch));
    sprintf(buf, "%s{cAGI: {W%3d\r\n{x", buf, GET_AGI(ch));
    send_to_char(buf, ch);
  }
  if (GET_LEVEL(ch) >= 25) {
    sprintf(buf, "{cArmor class: {y%d  {c(100 to -100)\r\n", GET_AC(ch));
  } else {
    if (GET_AC(ch) <= 100 && GET_AC(ch) >= -100) {
      sprintf(buf, "{cArmor class: {y%s\r\n", armor_class[(GET_AC(ch) / 10) + 10]);
    } else {
      sprintf(buf, "{cArmor class: {yERROR: Contact an admin please.\r\n");
    }
  }

  if (GET_LEVEL(ch) > 20) {
    calc_thac0 = thaco[(int) GET_CLASS(ch)][0];
    calc_thac0 -= ((int) GET_LEVEL(ch) / thaco[(int) GET_CLASS(ch)][1]);
    calc_thac0 -= stats[STR_TOHIT][GET_STR(ch)];
    calc_thac0 -= GET_HITROLL(ch);
    if (calc_thac0 < 1) {
      calc_thac0 = 1;
    }
    if (calc_thac0 > 20) {
      calc_thac0 = 20;
    }

    sprintf(buf, "%s{cDamroll: {W%d{c, Hitroll: {W%d{c, THAC0: {W%d\r\n", buf, GET_DAMROLL(ch), GET_HITROLL(ch), calc_thac0);
  }

  if (GET_LEVEL(ch) < 25) {
    sprintf(buf, "%s{cAlignment: {W%s{c     (extremely good to extremely evil)\r\n", buf, align_msg(GET_ALIGNMENT(ch)));
  } else {
    sprintf(buf, "%s{cYour alignment is {W%d{c  (1000 to -1000)\r\n", buf, GET_ALIGNMENT(ch));
  }

  send_to_char(buf, ch);

  sprintf(buf, "{cSaving Throws: "
      "{B[{CPAR{B] {W%d "
      "{B[{CROD{B] {W%d "
      "{B[{CPET{B] {W%d "
      "{B[{CBRE{B] {W%d "
      "{B[{CSPE{B] {W%d{x\r\n", GET_SAVE(ch, SAVING_PARA), GET_SAVE(ch, SAVING_ROD), GET_SAVE(ch, SAVING_PETRI), GET_SAVE(ch, SAVING_BREATH), GET_SAVE(ch, SAVING_SPELL));
  send_to_char(buf, ch);
  if (GET_CLASS(ch) == CLASS_MONK) {
    sprintf(buf, "{cBarehand Damage: {W%d{wd{W%d{x\r\n", monk_stat[(int) GET_LEVEL(ch)][0], monk_stat[(int) GET_LEVEL(ch)][1]);
    send_to_char(buf, ch);
  }
  if ((IS_MAGE(ch) || IS_PRI(ch)) && GET_LEVEL(ch) >= 21) {
    switch (GET_SPECIALIZED(ch)) {
      case (1 << 0):
        send_to_char("{cYou are specialized in the general realm.{x\r\n", ch);
        break;
      case (1 << 1):
        send_to_char("{cYou are specialized in the creation realm.{x\r\n", ch);
        break;
      case (1 << 2):
        send_to_char("{cYou are specialized in the divination realm.{x\r\n", ch);
        break;
      case (1 << 3):
        send_to_char("{cYou are specialized in the elemental realm.{x\r\n", ch);
        break;
      case (1 << 4):
        send_to_char("{cYou are specialized in the healing realm.{x\r\n", ch);
        break;
      case (1 << 5):
        send_to_char("{cYou are specialized in the protection realm.{x\r\n", ch);
        break;
      case (1 << 6):
        send_to_char("{cYou are specialized in the summoning realm.{x\r\n", ch);
        break;
      case (1 << 7):
        send_to_char("{cYou are specialized in the enchantment realm.{x\r\n", ch);
        break;
      default:
        send_to_char("{cYou havn't specialized in a realm of magic yet!{x\r\n", ch);
        break;
    }
  }
}

ACMD(do_exits)
{
  int door;

  *buf = '\0';

  if (IS_AFFECTED(ch, AFF_BLIND)) {
    send_to_char("You can't see a damned thing, you're blind!\r\n", ch);
    return;
  }
  for (door = 0; door < NUM_OF_DIRS; door++) {
    if (EXIT(ch, door) && EXIT(ch, door)->to_room != NOWHERE && !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED) && !IS_SET(EXIT(ch, door)->exit_info, EX_HIDDEN)) {
      if (GET_LEVEL(ch) >= LVL_IMMORT) {
        sprintf(buf2, "%-5s - [%5d] %s\r\n", dirs[door], world[EXIT(ch, door)->to_room].number, world[EXIT(ch, door)->to_room].name);
      } else {
        sprintf(buf2, "%-5s - ", dirs[door]);
        if (IS_DARK(EXIT(ch, door)->to_room) && !CAN_SEE_IN_DARK(ch) && !IS_SET(ROOM_FLAGS(EXIT(ch, door)->to_room), ROOM_DEATH)) {
          strcat(buf2, "Too dark to tell\r\n");
        } else {
          strcat(buf2, world[EXIT(ch, door)->to_room].name);
          strcat(buf2, "\r\n");
        }
      }
      strcat(buf, CAP(buf2));
    }
  }
  send_to_char("Obvious exits:\r\n", ch);

  if (*buf) {
    send_to_char(buf, ch);
  } else {
    send_to_char(" None.\r\n", ch);
  }
}

void look_at_room(struct char_data * ch, int ignore_brief)
{
  int skip = 0;
  if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch) && !AFF_FLAGGED(ch, AFF_INFRAVISION)) {
    sprintf(buf, "%sIt is pitch black %s...%s\r\n", CBGRE(ch,C_SPR), CBGRE(ch,C_SPR), CCNRM(ch,C_SPR));
    send_to_char(buf, ch);
    return;
  } else if (IS_AFFECTED(ch, AFF_BLIND)) {
    sprintf(buf, "%sYou see nothing but infinite darkness... %sYou are BLIND!%s\r\n", CBGRE(ch,C_SPR), CBGRE(ch,C_SPR), CCNRM(ch,C_SPR));
    send_to_char(buf, ch);
    return;
  }
  if (PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
    sprintbit((long) ROOM_FLAGS(ch->in_room), room_bits, buf);
    sprintf(buf2, "{Y%s {W[{B%5d{W]\r\n[{B%s{W]\r\n[{B%s{W]{x", world[ch->in_room].name, world[ch->in_room].number, buf, sector_types[GET_SECT(ch->in_room)]);
    send_to_char(buf2, ch);
  } else {
    send_to_char("{Y", ch);
    send_to_char(world[ch->in_room].name, ch);
    send_to_char("{x", ch);
  }

  if (!CAN_SEE_IN_DARK(ch) && AFF_FLAGGED(ch, AFF_INFRAVISION) && IS_DARK(ch->in_room)) {
    sprintf(buf, "\r\n%sIt is pitch black %s...%s\r\n", CBGRE(ch,C_SPR), CBGRE(ch,C_SPR), CCNRM(ch,C_SPR));
    send_to_char(buf, ch);
    skip = 1;
  }
  send_to_char("\r\n{c", ch);

  if (!skip) {
    if (!IS_NPC(ch) && (ignore_brief != 2) && (!PRF_FLAGGED(ch, PRF_BRIEF) || ignore_brief || ROOM_FLAGGED(ch->in_room, ROOM_DEATH))) {
      send_to_char(world[ch->in_room].description, ch);
    }

    send_to_char("{x", ch);

    /* autoexits */
    if (PRF_FLAGGED(ch, PRF_AUTOEXIT)) {
      do_auto_exits(ch);
    }
  }

  /* now list characters & objects */
  send_to_char("{x", ch);
  list_obj_to_char(world[ch->in_room].contents, ch, 0, FALSE);
  send_to_char(CCNRM(ch, C_SPR), ch);
  list_char_to_char(world[ch->in_room].people, ch);
  send_to_char(CCNRM(ch, C_SPR), ch);
}

void farsee_into_room(struct char_data * ch, room_num roomnum)
{
  int oldroomnum = ch->in_room;
  int skip = 0;

  if (roomnum == -1) {
    return;
  }

  if (IS_DARK(roomnum) && !CAN_SEE_IN_DARK(ch) && !AFF_FLAGGED(ch, AFF_INFRAVISION)) {
    sprintf(buf, "%sIt is pitch black %s...%s\r\n", CBGRE(ch,C_SPR), CBGRE(ch,C_SPR), CCNRM(ch,C_SPR));
    send_to_char(buf, ch);
    return;
  } else if (IS_AFFECTED(ch, AFF_BLIND)) {
    sprintf(buf, "%sYou see nothing but infinite darkness... %sYou are BLIND!%s\r\n", CBGRE(ch,C_SPR), CBGRE(ch,C_SPR), CCNRM(ch,C_SPR));
    send_to_char(buf, ch);
    return;
  }
  if (PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
    sprintbit((long) ROOM_FLAGS(roomnum), room_bits, buf);
    sprintf(buf2, "{Y%s {W[{B%5d{W]\r\n[{B%s{W]\r\n[{B%s{W]{x", world[roomnum].name, world[roomnum].number, buf, sector_types[GET_SECT(roomnum)]);
    send_to_char(buf2, ch);
  } else {
    send_to_char("{Y", ch);
    send_to_char(world[roomnum].name, ch);
    send_to_char("{x", ch);
  }
  if (!AFF_FLAGGED(ch, AFF_NIGHTVISION) && AFF_FLAGGED(ch, AFF_INFRAVISION) && IS_DARK(ch->in_room)) {
    sprintf(buf, "\r\n%sIt is pitch black %s...%s\r\n", CBGRE(ch,C_SPR), CBGRE(ch,C_SPR), CCNRM(ch,C_SPR));
    send_to_char(buf, ch);
    skip = 1;
  }

  send_to_char("\r\n{c", ch);
  if (!skip) {
    send_to_char(world[roomnum].description, ch);
  }
  send_to_char("{x", ch);

  /* autoexits */
  if (PRF_FLAGGED(ch, PRF_AUTOEXIT)) {
    ch->in_room = roomnum;
    do_auto_exits(ch);
    ch->in_room = oldroomnum;
  }

  /* now list characters & objects */
  send_to_char("{x", ch);
  if (!skip) {
    list_obj_to_char(world[roomnum].contents, ch, 0, FALSE);
    send_to_char(CCNRM(ch, C_SPR), ch);
    list_char_to_char(world[roomnum].people, ch);
    send_to_char(CCNRM(ch, C_SPR), ch);
  }
}

void look_in_direction(struct char_data * ch, int dir)
{
  int founddoor = 0;

  if (EXIT(ch, dir)) {
    if (EXIT(ch, dir)->general_description) {
      send_to_char(EXIT(ch, dir)->general_description, ch);
    } else {
      send_to_char("You see nothing special.\r\n", ch);
    }

    if (!IS_SET(EXIT(ch, dir)->exit_info, EX_HIDDEN) && IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED)) {
      founddoor = 1;
      if (EXIT(ch, dir)->keyword) {
        sprintf(buf, "The %s is closed.\r\n", fname(EXIT(ch, dir)->keyword));
        send_to_char(buf, ch);
      } else {
        sprintf(buf, "The door is closed.\r\n");
        send_to_char(buf, ch);
      }
      if (!AFF2_FLAGGED(ch, AFF2_FARSEE)) {
        return;
      }
    }
    if (!IS_SET(EXIT(ch, dir)->exit_info, EX_HIDDEN) && IS_SET(EXIT(ch, dir)->exit_info, EX_ISDOOR) && !IS_SET(EXIT(ch, dir)->exit_info, EX_CLOSED)) {
      if (EXIT(ch, dir)->keyword) {
        sprintf(buf, "The %s is open.\r\n", fname(EXIT(ch, dir)->keyword));
        send_to_char(buf, ch);
      } else {
        sprintf(buf, "The door is open.\r\n");
        send_to_char(buf, ch);
      }
    }
    if (!IS_SET(EXIT(ch, dir)->exit_info, EX_HIDDEN)) {
      farsee_into_room(ch, EXIT(ch, dir)->to_room);
    }
    if (AFF2_FLAGGED(ch, AFF2_FARSEE) && !founddoor && _2ND_EXIT(ch, dir)) {
      farsee_into_room(ch, _2ND_EXIT(ch, dir)->to_room);
    }
  } else {
    send_to_char("Nothing special there...\r\n", ch);
  }
}

void look_in_obj(struct char_data * ch, char *arg)
{
  struct obj_data *obj = NULL;
  struct char_data *dummy = NULL;
  int amt, bits;

  if (!*arg) {
    send_to_char("Look in what?\r\n", ch);
  } else if (!(bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &dummy, &obj))) {
    sprintf(buf, "There doesn't seem to be %s %s here.\r\n", AN(arg), arg);
    send_to_char(buf, ch);
  } else if ((GET_OBJ_TYPE(obj) != ITEM_DRINKCON) && (GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN) && (GET_OBJ_TYPE(obj) != ITEM_CONTAINER) && (GET_OBJ_TYPE(obj) != ITEM_PCORPSE)) {
    send_to_char("There's nothing inside that!\r\n", ch);
  } else {
    if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER || GET_OBJ_TYPE(obj) == ITEM_PCORPSE) {
      if (IS_SET(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) {
        send_to_char("It is closed.\r\n", ch);
      } else {
        act("$n looks inside $p.", TRUE, ch, obj, 0, TO_ROOM);
        send_to_char((obj->cname ? fname(obj->cname) : fname(obj->name)), ch);
        switch (bits) {
          case FIND_OBJ_INV:
            send_to_char(" (carried): \r\n", ch);
            break;
          case FIND_OBJ_ROOM:
            send_to_char(" (here): \r\n", ch);
            break;
          case FIND_OBJ_EQUIP:
            send_to_char(" (used): \r\n", ch);
            break;
        }

        list_obj_to_char(obj->contains, ch, 2, TRUE);
      }
    } else { /* item must be a fountain or drink container */
      if (GET_OBJ_VAL(obj, 1) <= 0) {
        send_to_char("It is empty.\r\n", ch);
      } else {
        amt = ((GET_OBJ_VAL(obj, 1) * 3) / GET_OBJ_VAL(obj, 0));
        sprintf(buf, "It's %sfull of a %s liquid.\r\n", fullness[amt], color_liquid[GET_OBJ_VAL(obj, 2)]);
        send_to_char(buf, ch);
      }
    }
  }
}

char *find_exdesc(char *word, struct extra_descr_data * list)
{
  struct extra_descr_data *i;

  for (i = list; i; i = i->next) {
    if (isname(word, i->keyword)) {
      return (i->description);
    }
  }

  return NULL;
}

/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room
 * with the name.  Then check local objs for exdescs.
 */
void look_at_target(struct char_data * ch, char *arg)
{
  int bits, found = 0, j;
  struct char_data *found_char = NULL;
  struct obj_data *obj = NULL, *found_obj = NULL;
  char *desc;

  if (!*arg) {
    send_to_char("Look at what?\r\n", ch);
    return;
  }
  bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_CHAR_ROOM, ch, &found_char, &found_obj);

  /* Is the target a character? */
  if (found_char != NULL) {
    look_at_char(found_char, ch);
    if (ch != found_char) {
      if (CAN_SEE(found_char, ch) && GET_POS(found_char) > POS_SLEEPING) {
        act("$n looks at you.", TRUE, ch, 0, found_char, TO_VICT);
      }
      act("$n looks at $N.", TRUE, ch, 0, found_char, TO_NOTVICT);
    }
    return;
  }
  /* Does the argument match an extra desc in the room? */
  if ((desc = find_exdesc(arg, world[ch->in_room].ex_description)) != NULL) {
    page_string(ch->desc, desc, 1);
    return;
  }
  /* Does the argument match an extra desc in the char's equipment? */
  for (j = 0; j < NUM_WEARS && !found; j++) {
    if (ch->equipment[j] && CAN_SEE_OBJ(ch, ch->equipment[j])) {
      if ((desc = find_exdesc(arg, ch->equipment[j]->ex_description)) != NULL) {
        send_to_char(desc, ch);
        found = 1;
      }
    }
  }
  /* Does the argument match an extra desc in the char's inventory? */
  for (obj = ch->carrying; obj && !found; obj = obj->next_content) {
    if (CAN_SEE_OBJ(ch, obj)) {
      if ((desc = find_exdesc(arg, obj->ex_description)) != NULL) {
        send_to_char(desc, ch);
        found = 1;
      }
    }
  }

  /* Does the argument match an extra desc of an object in the room? */
  for (obj = world[ch->in_room].contents; obj && !found; obj = obj->next_content) {
    if (CAN_SEE_OBJ(ch, obj)) {
      if ((desc = find_exdesc(arg, obj->ex_description)) != NULL) {
        send_to_char(desc, ch);
        act("$n looks at $p.", TRUE, ch, obj, 0, TO_ROOM);
        found = 1;
      }
    }
  }
  if (bits) { /* If an object was found back in generic_find */
    if (!found) {
      show_obj_to_char(found_obj, ch, 5); /* Show no-description */
    } else {
      show_obj_to_char(found_obj, ch, 6); /* Find hum, glow etc */
    }
  } else if (!found) {
    send_to_char("You do not see that here.\r\n", ch);
  }
}

ACMD(do_look)
{
  static char arg2[MAX_INPUT_LENGTH];
  int look_type;

  if (!ch->desc) {
    return;
  }

  if (GET_POS(ch) < POS_SLEEPING) {
    send_to_char("You can't see anything but stars!\r\n", ch);
  } else if (IS_AFFECTED(ch, AFF_BLIND)) {
    send_to_char("{DYou can't see a damned thing, you're blind!{x\r\n", ch);
  } else if (IS_DARK(ch->in_room) && !CAN_SEE_IN_DARK(ch)) {
    /*
     if (!AFF_FLAGGED(ch, AFF_INFRAVISION)) {
     sprintf(buf, "%sIt is pitch black %s...%s\r\n", CBGRE(ch,C_SPR),CBGRE(ch,C_SPR),CCNRM(ch,C_SPR));
     }
     */
    look_at_room(ch, 1);
  } else {
    half_chop(argument, arg, arg2);

    if (subcmd == SCMD_READ) {
      if (!*arg) {
        send_to_char("Read what?\r\n", ch);
      } else {
        look_at_target(ch, arg);
      }
      return;
    }
    if (!*arg && PRF_FLAGGED(ch, PRF_BRIEF2)) { /* "look" alone, without an argument at all */
      look_at_room(ch, 2);
    } else if (!*arg) {
      look_at_room(ch, 0);
    } else if (is_abbrev(arg, "room")) {
      look_at_room(ch, 1);
    } else if (is_abbrev(arg, "in")) {
      look_in_obj(ch, arg2);
      /* did the char type 'look <direction>?' */
    } else if ((look_type = search_block(arg, dirs, FALSE)) >= 0) {
      look_in_direction(ch, look_type);
    } else if (is_abbrev(arg, "at")) {
      look_at_target(ch, arg2);
    } else {
      look_at_target(ch, arg);
    }
  }
}

ACMD(do_examine)
{
  struct char_data *tmp_char;
  struct obj_data *tmp_object;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char("Examine what?\r\n", ch);
    return;
  }
  look_at_target(ch, arg);

  generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM | FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);

  if (tmp_object) {
    if ((GET_OBJ_TYPE(tmp_object) == ITEM_DRINKCON) || (GET_OBJ_TYPE(tmp_object) == ITEM_FOUNTAIN) || (GET_OBJ_TYPE(tmp_object) == ITEM_CONTAINER) || (GET_OBJ_TYPE(tmp_object) == ITEM_PCORPSE)) {
      send_to_char("When you look inside, you see:\r\n", ch);
      look_in_obj(ch, arg);
    }
  }
}

ACMD(do_score)
{
  struct affected_type *aff;
  struct time_info_data playing_time;
  struct time_info_data real_time_passed(time_t t2, time_t t1);
  struct char_data *pl;

  one_argument(argument, arg);
  if (GET_LEVEL(ch) < LVL_IMMORT || !*arg) {
    pl = ch;
  } else {
    if (!(pl = get_char_vis(ch, arg))) {
      send_to_char("No-one by that name in the game.\r\n", ch);
      return;
    }
  }

  sprintf(buf, "\r\n{y                      Score information for: {c%s{x\r\n\r\n", GET_NAME(ch));

  sprintf(buf, "%s{BRace: {c%s  {BClass: {c%s{B  Level: {c%d  {BAge: {c%d years old.\r\n", buf, pc_race_types[(int) GET_RACE(pl)], pc_class_types[(int) GET_CLASS(pl)], GET_LEVEL(pl), GET_AGE(pl));

  sprintf(buf, "%s{BHits: {y%d(%d)    {BMana: {y%d(%d){B    Movement: {y%d(%d){x\r\n", buf, GET_HIT(pl), GET_MAX_HIT(pl), GET_MANA(pl), GET_MAX_MANA(pl), GET_MOVE(pl), GET_MAX_MOVE(pl));

  if (!IS_NPC(pl))
    sprintf(buf, "%s{BCoins carried:  {W%dp {Y%dg {w%ds {y%dc {B   Coins in bank:  {W%dp {Y%dg {w%ds {y%dc\r\n", buf, GET_PLAT(pl), GET_GOLD(pl), GET_SILVER(pl), GET_COPPER(pl), GET_BANK_PLAT(pl), GET_BANK_GOLD(pl), GET_BANK_SILVER(pl), GET_BANK_COPPER(pl));

  sprintf(buf, "%s{BYou weigh {c%d{B pounds and stand {c%d{B inches high.{x\r\n", buf, GET_WEIGHT(pl), GET_HEIGHT(pl));

  if (!IS_NPC(pl)) {
    playing_time = real_time_passed((time(0) - pl->player.time.logon) + pl->player.time.played, 0);
    sprintf(buf, "%s{BYou have been playing for {c%d {Bdays and {c%d {Bhours.\r\n", buf, playing_time.day, playing_time.hours);

    sprintf(buf, "%s{BTitle:  {w%s{x\r\n{BStatus:  {W", buf, GET_TITLE(pl));
  }

  switch (GET_POS(pl)) {
    case POS_DEAD:
      strcat(buf, "{RDEAD as a doornail.\r\n");
      break;
    case POS_MORTALLYW:
      strcat(buf, "Mortally Wounded, and going to Die.\r\n");
      break;
    case POS_INCAP:
      strcat(buf, "Incapacitated, slowly fading away...\r\n");
      break;
    case POS_STUNNED:
      strcat(buf, "Stunned, unable to move!\r\n");
      break;
    case POS_SLEEPING:
      strcat(buf, "You are sleeping.\r\n");
      break;
    case POS_RESTING:
      strcat(buf, "You are resting.\r\n");
      break;
    case POS_SITTING:
      strcat(buf, "You are sitting.\r\n");
      break;
    case POS_FIGHTING:
      if (FIGHTING(pl)) {
        sprintf(buf, "%sYou are fighting {c%s.\r\n", buf, PERS(FIGHTING(pl), pl));
      } else {
        strcat(buf, "You are fighting thin air.\r\n");
      }
      break;
    case POS_STANDING:
      if (IS_AFFECTED(pl, AFF_FLY)) {
        strcat(buf, "You are flying.\r\n");
      } else {
        strcat(buf, "You are standing.\r\n");
      }
      break;
    default:
      strcat(buf, "You are floating.\r\n");
      break;
  }

  if (GET_COND(pl, FULL) == 0) {
    strcat(buf, "{WYou are hungry.\r\n");
  }

  if (GET_COND(pl, THIRST) == 0) {
    strcat(buf, "{WYou are thirsty.\r\n");
  }

  strcat(buf, "\r\n{x");
  send_to_char(buf, pl);
  send_to_char("Affecting spells:\r\n", pl);

  /* Routine to show what spells a char is affected by */
  if (pl->affected) {
    for (aff = pl->affected; aff; aff = aff->next) {
      sprintf(buf, "{cSpell: {W%-21s{x\r\n", get_spell_name(aff->type));
      send_to_char(buf, pl);
    }
  }
  sprintf(buf, "{WInnate Abilities: {c");
  switch (GET_RACE(pl)) {
    case RACE_HALFELF:
    case RACE_OGRE:
    case RACE_TROLL:
      strcat(buf, "Infravision\r\n");
      send_to_char(buf, pl);
      break;
    case RACE_ELF:
      strcat(buf, "Infravision, Sneak\r\n");
      send_to_char(buf, pl);
      break;
    case RACE_DWARF:
      strcat(buf, "Detect Alignment, Infravision\r\n");
      send_to_char(buf, pl);
      break;
    case RACE_HALFLING:
      strcat(buf, "Sneak\r\n");
      send_to_char(buf, pl);
      break;
  }
  if (GET_DRAGGING(pl)) {
    sprintf(buf, "{xDragging: %s\r\n", OBJS(GET_DRAGGING(pl), pl));
    send_to_char(buf, pl);
  }
}

ACMD(do_inventory)
{
  struct char_data *pl;

  one_argument(argument, arg);
  if (GET_LEVEL(ch) < LVL_IMMORT || !*arg) {
    pl = ch;
  } else {
    if (!(pl = get_char_vis(ch, arg))) {
      send_to_char("No-one by that name in the game.\r\n", ch);
      return;
    }
  }
  if (pl == ch) {
    send_to_char("You are carrying:\r\n", ch);
    list_obj_to_char(ch->carrying, ch, 1, TRUE);
  } else {
    sprintf(buf, "%s is carrying:\r\n", GET_NAME(pl));
    send_to_char(buf, ch);
    list_obj_to_char(pl->carrying, ch, 1, TRUE);
  }

}

ACMD(do_equipment)
{
  struct char_data *pl;
  int i, found = 0;

  one_argument(argument, arg);

  if (GET_LEVEL(ch) < LVL_IMMORT || !*arg) {
    pl = ch;
  } else { /* code to search for argument */

    if (!(pl = get_char_vis(ch, arg))) {
      send_to_char("No-one by that name in the game.\r\n", ch);
      return;
    }
  }

  if (pl == ch) {
    send_to_char("You are using:\r\n", ch);
  } else {
    sprintf(buf, "%s is using:\r\n", GET_NAME(pl));
    send_to_char(buf, ch);
  }

  for (i = 0; i < NUM_WEARS; i++) {
    if (pl->equipment[equip_order[i]]) {
      if (CAN_SEE_OBJ(ch, pl->equipment[equip_order[i]])) {
        send_to_char(where[equip_order[i]], ch);
        show_obj_to_char(pl->equipment[equip_order[i]], ch, 1);
        found = TRUE;
      } else {
        send_to_char(where[equip_order[i]], ch);
        send_to_char("Something.\r\n", ch);
        found = TRUE;
      }
    }
  }
  if (!found) {
    send_to_char(" Nothing.\r\n", ch);
  }
}

ACMD(do_time)
{
  char *suf;
  int weekday, day;
  extern struct time_info_data time_info;
  extern const char *weekdays[];
  extern const char *month_name[];
  char *tmstr;
  time_t mytime;
  int d, h, m;
  extern time_t boot_time;

  sprintf(buf, "{CIt is {c%d %s{C, on{y ", ((time_info.hours % 12 == 0) ? 12 : ((time_info.hours) % 12)), ((time_info.hours >= 12) ? "pm" : "am"));

  /* 35 days in a month */
  weekday = ((35 * time_info.month) + time_info.day + 1) % 7;

  strcat(buf, weekdays[weekday]);
  strcat(buf, "\r\n");
  send_to_char(buf, ch);

  day = time_info.day + 1; /* day in [1..35] */

  if (day == 1) {
    suf = "st";
  } else if (day == 2) {
    suf = "nd";
  } else if (day == 3) {
    suf = "rd";
  } else if (day < 20) {
    suf = "th";
  } else if ((day % 10) == 1) {
    suf = "st";
  } else if ((day % 10) == 2) {
    suf = "nd";
  } else if ((day % 10) == 3) {
    suf = "rd";
  } else {
    suf = "th";
  }

  sprintf(buf, "{CThe {c%d%s {CDay of the {y%s{C, Year {c%d{C.{x\r\n", day, suf, month_name[(int) time_info.month], time_info.year);

  send_to_char(buf, ch);

  mytime = time(0);

  tmstr = (char*) asctime(localtime(&mytime));
  *(tmstr + strlen(tmstr) - 1) = '\0';
  sprintf(buf, "\r\n{CCurrent machine time: {c%s{x\r\n", tmstr);
  send_to_char(buf, ch);
  mytime = boot_time;
  tmstr = (char*) asctime(localtime(&mytime));
  *(tmstr + strlen(tmstr) - 1) = '\0';
  mytime = time(0) - boot_time;
  d = mytime / 86400;
  h = (mytime / 3600) % 24;
  m = (mytime / 60) % 60;
  sprintf(buf, "{CUp since {c%s : %d {Cday%s, {c%d {Chour%s {c%d {Cminute%s{x\r\n", tmstr, d, ((d == 1) ? "" : "s"), h, ((h == 1) ? "" : "s"), m, ((m == 1) ? "" : "s"));

  send_to_char(buf, ch);
}

ACMD(do_weather)
{
  struct new_weather_data cond = GET_ZONE_COND(IN_ZONE(ch));
  char *weather_msg[] = {"A violent scorching wind blows hard in the face of any poor travellers in the area.\r\n", "A hot wind gusts wildly through the area.\r\n", "A fierce wind cuts the air like a razor-sharp knife.\r\n", "A freezing gale blasts through the area.\r\n", "An icy wind drains the warmth from all in sight.\r\n", "A hot, dry breeze blows languidly around.\r\n", "A warm pocket of air is rolling through here.\r\n", "It's breezy.\r\n", "A cool breeze wafts by.\r\n", "A slight wind blows a chill into living tissue.\r\n", "A freezing wind blows gently, but firmly against all obstacles in the area.\r\n", "The wind isn't very strong here, but the cold makes it quite noticeable.\r\n", "It's hotter than anyone could imagine.\r\n", "It's really, really hot here. A slight breeze would really improve things.\r\n", "It's hot out here.\r\n", "It's nice and warm out.\r\n", "It's mild out today.\r\n", "It's cool out here.\r\n", "It's a bit nippy here.\r\n", "It's cold!\r\n", "It's really c-c-c-cold!!\r\n", "Better get inside - this is too cold for man or -most- beasts.\r\n", "There's a hurricane out here!\r\n", "The wind and the rain are nearly too much to handle.\r\n", "It's raining really hard right now.\r\n", "What a rainstorm!\r\n", "The wind is lashing this wild rain seemingly straight into your face.\r\n", "It's raining pretty hard.\r\n", "A respectable rain is being thrashed about by a vicious wind.\r\n", "It's rainy and windy but, altogether not too uncomfortable.\r\n", "Hey, it's raining...\r\n", "The light rain here is nearly unnoticeable compared to the horrendous wind.\r\n", "A light rain is being driven fiercely by the wind.\r\n", "It's raining lightly.\r\n", "A few drops of rain are falling admidst a fierce windstorm.\r\n", "The wind and a bit of rain hint at the possibility of a storm.\r\n", "A light drizzle is falling here.\r\n", "This must be the worst blizzard ever.\r\n", "There's a blizzard out here, making it quite difficult to see.\r\n", "It's snowing very hard.\r\n", "The heavily falling snow is being whipped up to a frenzy by a ferocious wind.\r\n", "A heavy snow is being blown randomly about by a brisk wind.\r\n", "Drifts in the snow are being formed by the wind.\r\n", "The snow's coming down pretty fast now.\r\n", "The snow wouldn't be too bad, except for the awful wind blowing it in every possible directon.\r\n", "There's a minor blizzard here, more wind than snow.\r\n", "Snow is being blown about by a stiff breeze.\r\n", "It is snowing here.\r\n", "A light snow is being tossed about by a fierce wind.\r\n", "A lightly falling snow is being driven by a strong wind.\r\n", "A light snow is falling admidst an unsettled wind.\r\n", "It is lightly snowing.\r\n"};

  if (cond.precip_rate == 0) {
    if (cond.windspeed > 60) {
      if (cond.temp > 50) {
        send_to_char(weather_msg[0], ch);
      } else if (cond.temp > 21) {
        send_to_char(weather_msg[1], ch);
      } else if (cond.temp > 0) {
        send_to_char(weather_msg[2], ch);
      } else if (cond.temp > -10) {
        send_to_char(weather_msg[3], ch);
      } else {
        send_to_char(weather_msg[4], ch);
      }
    } else if (cond.windspeed > 25) {
      if (cond.temp > 50) {
        send_to_char(weather_msg[4], ch);
      } else if (cond.temp > 22) {
        send_to_char(weather_msg[5], ch);
      } else if (cond.temp > 10) {
        send_to_char(weather_msg[6], ch);
      } else if (cond.temp > 2) {
        send_to_char(weather_msg[7], ch);
      } else if (cond.temp > -5) {
        send_to_char(weather_msg[8], ch);
      } else if (cond.temp > -15) {
        send_to_char(weather_msg[9], ch);
      } else {
        send_to_char(weather_msg[10], ch);
      }
    } else if (cond.temp > 52) {
      send_to_char(weather_msg[11], ch);
    } else if (cond.temp > 37) {
      send_to_char(weather_msg[12], ch);
    } else if (cond.temp > 25) {
      send_to_char(weather_msg[13], ch);
    } else if (cond.temp > 19) {
      send_to_char(weather_msg[14], ch);
    } else if (cond.temp > 9) {
      send_to_char(weather_msg[15], ch);
    } else if (cond.temp > 1) {
      send_to_char(weather_msg[16], ch);
    } else if (cond.temp > -5) {
      send_to_char(weather_msg[17], ch);
    } else if (cond.temp > -20) {
      send_to_char(weather_msg[18], ch);
    } else if (cond.temp > -25) {
      send_to_char(weather_msg[19], ch);
    } else {
      send_to_char(weather_msg[20], ch);
    }
  } else if (cond.temp > 0) {
    if (cond.precip_rate > 80) {
      if (cond.windspeed > 80) {
        send_to_char(weather_msg[21], ch);
      } else if (cond.windspeed > 40) {
        send_to_char(weather_msg[22], ch);
      } else {
        send_to_char(weather_msg[23], ch);
      }
    } else if (cond.precip_rate > 50) {
      if (cond.windspeed > 60) {
        send_to_char(weather_msg[24], ch);
      } else if (cond.windspeed > 30) {
        send_to_char(weather_msg[25], ch);
      } else {
        send_to_char(weather_msg[26], ch);
      }
    } else if (cond.precip_rate > 30) {
      if (cond.windspeed > 50) {
        send_to_char(weather_msg[27], ch);
      } else if (cond.windspeed > 25) {
        send_to_char(weather_msg[28], ch);
      } else {
        send_to_char(weather_msg[29], ch);
      }
    } else if (cond.precip_rate > 10) {
      if (cond.windspeed > 50) {
        send_to_char(weather_msg[30], ch);
      } else if (cond.windspeed > 24) {
        send_to_char(weather_msg[31], ch);
      } else {
        send_to_char(weather_msg[32], ch);
      }
    } else if (cond.windspeed > 55) {
      send_to_char(weather_msg[33], ch);
    } else if (cond.windspeed > 30) {
      send_to_char(weather_msg[34], ch);
    } else {
      send_to_char(weather_msg[35], ch);
    }
  } else if (cond.precip_rate > 70) {
    if (cond.windspeed > 50) {
      send_to_char(weather_msg[36], ch);
    } else if (cond.windspeed > 25) {
      send_to_char(weather_msg[37], ch);
    } else {
      send_to_char(weather_msg[38], ch);
    }
  } else if (cond.precip_rate > 40) {
    if (cond.windspeed > 60) {
      send_to_char(weather_msg[39], ch);
    } else if (cond.windspeed > 35) {
      send_to_char(weather_msg[40], ch);
    } else if (cond.windspeed > 18) {
      send_to_char(weather_msg[41], ch);
    } else {
      send_to_char(weather_msg[42], ch);
    }
  } else if (cond.precip_rate > 19) {
    if (cond.windspeed > 70) {
      send_to_char(weather_msg[43], ch);
    } else if (cond.windspeed > 45) {
      send_to_char(weather_msg[44], ch);
    } else if (cond.windspeed > 12) {
      send_to_char(weather_msg[45], ch);
    } else {
      send_to_char(weather_msg[46], ch);
    }
  } else if (cond.windspeed > 60) {
    send_to_char(weather_msg[47], ch);
  } else if (cond.windspeed > 42) {
    send_to_char(weather_msg[48], ch);
  } else if (cond.windspeed > 18) {
    send_to_char(weather_msg[49], ch);
  } else {
    send_to_char(weather_msg[50], ch);
  }
}

#define WHO_FORMAT \
"format: who [[minlev[-maxlev]] [name] [class] [race] [gods]]\r\n"

ACMD(do_who)
{
  int minlev = 0;
  int tmp;
  int maxlev = LVL_IMMORT;
  int race = ((1 << RACE_HUMAN) + (1 << RACE_TROLL) + (1 << RACE_OGRE) + (1 << RACE_DWARF) + (1 << RACE_ELF) + (1 << RACE_HALFELF) + (1 << RACE_GNOME) + (1 << RACE_HALFLING));
  int class = ((1 << CLASS_WARRIOR) + (1 << CLASS_ROGUE) + (1 << CLASS_THIEF) + (1 << CLASS_SORCERER) + (1 << CLASS_WIZARD) + (1 << CLASS_ENCHANTER) + (1 << CLASS_CONJURER) + (1 << CLASS_NECROMANCER) + (1 << CLASS_CLERIC) + (1 << CLASS_PRIEST) + (1 << CLASS_SHAMAN) + (1 << CLASS_MONK) + (1 << CLASS_DRUID) + (1 << CLASS_ASSASSIN) + (1 << CLASS_BARD) + (1 << CLASS_RANGER) + (1 << CLASS_MERCENARY));
  char name[MAX_NAME_LENGTH + 1];

  skip_spaces(&argument);

  memset(name, 0, MAX_NAME_LENGTH + 1);

  if (*argument) {
    if (*argument == '?') {
      send_to_char(WHO_FORMAT, ch);
      return;
    }
    half_chop(argument, buf, buf1);
    if (isdigit(*buf)) {
      sscanf(buf, "%d-%d", &minlev, &maxlev);
      if (minlev > maxlev) {
        tmp = minlev;
        minlev = maxlev;
        maxlev = tmp;
      }
      strcpy(arg, buf1);
    } else {
      strcpy(arg, buf);
    }
    if (*arg) {
      half_chop(arg, buf, buf1);
      if (strncasecmp("gods", buf, strlen(buf)) == 0) {
        minlev = LVL_IMMORT;
      } else if ((tmp = is_class(buf)) > CLASS_UNDEFINED) {
        class &= (1 << tmp);
        race = RACE_UNDEFINED;
      } else if ((tmp = is_race(buf)) > RACE_UNDEFINED) {
        race &= (1 << tmp);
        class = CLASS_UNDEFINED;
      } else {
        strcpy(name, buf);
      }
    }
  }
  display_who_list(ch, minlev, maxlev, class, race, name);
}

void display_who_list(struct char_data *ch, int minlev, int maxlev, int class, int race, char *name)
{
  struct descriptor_data *d;
  struct char_data *wch = NULL;
  char godbuf[MAX_STRING_LENGTH];
  char mortbuf[MAX_STRING_LENGTH];
  char whospecout[80];
  int color = 0;
  int charctr = 0;
  int i;
  int gods = 0;
  int morts = 0;

  const char *godlevs[2] = {"{W  Immortal   {x", "{W  -- Error --{x"};

  if (ch->desc->color && (COLOR_LEV(ch) > 0)) {
    color = 1;
  }

  if (!*name) {
    strcpy(godbuf, " {BListing of the Immortals\r\n{b-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-                       -=-=-=-=-=-{x\r\n");
    strcpy(mortbuf, " {BListing of the Mortals\r\n{b-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-                       -=-=-=-=-=-{x\r\n");
  } else {
    strcpy(godbuf, "{b-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-                        -=-=-=-=-=-{x\r\n");
    strcpy(mortbuf, "{b-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-                        -=-=-=-=-=-{x\r\n");
  }

  for (d = descriptor_list; d; d = d->next) {
    if (d->connected && d->connected != CON_MEDIT && d->connected != CON_OEDIT && d->connected != CON_REDIT && d->connected != CON_SEDIT && d->connected != CON_ZEDIT) {
      continue;
    }

    if (d->original) {
      if (d->original == wch) {
        continue;
      }
    }

    if (!(wch = d->character)) {
      continue;
    }

    if (!CAN_SEE(ch, wch)) {
      continue;
    }

    if (GET_LEVEL(wch) < minlev || GET_LEVEL(wch) > maxlev) {
      continue;
    }

    if (*name && str_cmp(GET_NAME(wch), name)) {
      continue;
    }

    if (class && !(class & (1 << GET_CLASS(wch)))) {
      continue;
    }

    if (race && !(race & (1 << GET_RACE(wch)))) {
      continue;
    }

    if (!PRF_FLAGGED(wch, PRF_ANONYMOUS) || (GET_LEVEL(ch) >= LVL_IMMORT)) {
      if (GET_LEVEL(wch) >= LVL_IMMORT) {
        if (WHOSPEC(wch)) {
          charctr = 0;
          i = 0;
          *whospecout = '\0';
          strcpy(whospecout, WHOSPEC(wch));
          while (charctr < 13) {
            if (whospecout[i] == '\0') {
              while (charctr < 13) {
                whospecout[i] = ' ';
                charctr++;
                i++;
              }
            } else if (whospecout[i] != '{') {
              charctr++;
              i++;
            } else if (whospecout[i] == '{') {
              i++;
              if (whospecout[i] == '{') {
                charctr++;
              }
              i++;
            }
          }
          whospecout[i] = '\0';

          sprintf(godbuf, "%s%s[%s%s%s] %s%s %s", godbuf, CBBLU(ch, C_SPR), CBWHT(ch, C_SPR), whospecout, CBBLU(ch, C_SPR), (NAMECOLOR(wch) && color) ? NAMECOLOR(wch) : CBCYN(ch, C_SPR)
          , GET_NAME(wch), GET_TITLE(wch));
        } else {
          sprintf(godbuf, "%s%s[%s%s%s] %s%s %s", godbuf, CBBLU(ch, C_SPR), CBWHT(ch, C_SPR), godlevs[GET_LEVEL(wch) - LVL_IMMORT], CBBLU(ch, C_SPR), (NAMECOLOR(wch) && color) ? NAMECOLOR(wch) : CBCYN(ch, C_SPR), GET_NAME(wch), GET_TITLE(wch));
        }
        gods++;
      } else {
        sprintf(mortbuf, "%s%s[%s%2d %s%s] %s%s %s %s(%s%s)", mortbuf, CBBLU(ch, C_SPR), CBWHT(ch, C_SPR), GET_LEVEL(wch), CLASS_ABBR(wch), CBBLU(ch, C_SPR), (!(wch->desc->original) ? ((NAMECOLOR(wch) && color) ? NAMECOLOR(wch) : CCCYN(ch, C_SPR)) : ""),

            GET_NAME(wch), GET_TITLE(wch), CBBLU(ch, C_SPR), GET_RACE_NAME_COLOR(wch), CBBLU(ch, C_SPR));
        morts++;
      }
    } else {
      if (GET_LEVEL(wch) >= LVL_IMMORT) {
        if (WHOSPEC(wch)) {
          charctr = 0;
          i=0;
          *whospecout = '\0';
          strcpy (whospecout, WHOSPEC(wch));
          while (charctr < 13) {
            if (whospecout[i] == '\0') {
              while (charctr < 13) {
                whospecout[i] = ' ';
                charctr++;
                i++;
              }
            } else if (whospecout[i] != '{') {
              charctr++;
              i++;
            } else if (whospecout[i] == '{') {
              i++;
              if (whospecout[i] == '{') {
                charctr++;
              }
              i++;
            }
          }
          whospecout[i] = '\0';

          sprintf(godbuf, "%s%s[%s%s%s] %s%s %s", godbuf, CBBLU(ch, C_SPR), CBWHT(ch, C_SPR), whospecout, CBBLU(ch, C_SPR), (NAMECOLOR(wch) && color) ? NAMECOLOR(wch) : CBCYN(ch, C_SPR), GET_NAME(wch), GET_TITLE(wch));
        } else {
          sprintf(godbuf, "%s%s[{D -Anonymous- {x%s] %s%s %s", godbuf, CBBLU(ch, C_SPR), CBBLU(ch, C_SPR), (NAMECOLOR(wch) && color) ? NAMECOLOR(wch) : CBCYN(ch, C_SPR), GET_NAME(wch), GET_TITLE(wch));
        }
        gods++;
      } else {
        sprintf(mortbuf, "%s%s[{D-Anon-{x%s] %s%s %s %s(%s%s)", mortbuf, CBBLU(ch, C_SPR), CBBLU(ch, C_SPR), (NAMECOLOR(wch) && color) ? NAMECOLOR(wch) : CCCYN(ch, C_SPR), GET_NAME(wch), GET_TITLE(wch), CBBLU(ch, C_SPR), GET_RACE_NAME_COLOR(wch), CBBLU(ch

                , C_SPR));
        morts++;
      }
    }

    *buf = '\0';
    if (GET_INVIS_LEV(wch)) {
      sprintf(buf, "%s (i%d)", buf, GET_INVIS_LEV(wch));
    }

    if (PLR_FLAGGED(wch, PLR_MAILING)) {
      strcat(buf, " {B({Wmailing{B)");
    } else if (PLR_FLAGGED(wch, PLR_WRITING)) {
      strcat(buf, " {B({Wwriting{B)");
    }
    if (PLR_FLAGGED(wch, PLR_EDITING)) {
      strcat(buf, " {B({Wediting{B)");
    }

    if (PLR_FLAGGED(wch, PLR_THIEF)) {
      strcat(buf, " {R({WTHIEF{R)");
    }
    if (PLR_FLAGGED(wch, PLR_KILLER)) {
      strcat(buf, " {R({WKILLER{R)");
    }
    if (PRF_FLAGGED(wch, PRF_AFK)) {
      strcat(buf, " {W({RAFK{W){x");
    }
    strcat(buf, "\r\n");

    if (GET_LEVEL(wch) >= LVL_IMMORT) {
      strcat(godbuf, buf);
    } else {
      strcat(mortbuf, buf);
    }
  } /* end of for */

  if (gods != 0 && !*name) {
    sprintf(godbuf, "%s{b-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-                       -=-=-=-=-=-{x\r\n{rThere %s {W%d{r immortal%s shown.{x\r\n", godbuf, (gods == 1 ? "is" : "are"), gods, (gods == 1 ? "" : "s"));
    page_string(ch->desc, godbuf, 1);
    send_to_char("\r\n", ch);
  } else if (*name && gods) {
    sprintf(godbuf, "%s{b-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-                       -=-=-=-=-=-{x\r\n", godbuf);
    page_string(ch->desc, godbuf, 1);
    send_to_char("\r\n", ch);
  }

  if (morts != 0 && !*name) {
    sprintf(mortbuf, "%s{b-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-                       -=-=-=-=-=-{x\r\n{rThere %s {W%d{r mortal%s shown.{x\r\n", mortbuf, (morts == 1 ? "is" : "are"), morts, (morts == 1 ? "" : "s"));
    page_string(ch->desc, mortbuf, 1);
    strcat(buf, "\r\n");
  } else if (*name && morts) {
    sprintf(mortbuf, "%s{b-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-                       -=-=-=-=-=-{x\r\n", mortbuf);
    page_string(ch->desc, mortbuf, 1);
    strcat(buf, "\r\n");
  }

  if ((gods + morts) > max_players) {
    max_players = gods + morts;
  }
  sprintf(buf, "{wTotal visible players: {W%d{x.\r\n", gods + morts);
  sprintf(buf, "%s{wMaximum number of players on this boot: {W%d{x.\r\n", buf, max_players);
  send_to_char(buf, ch);
}

#define USERS_FORMAT \
"{gformat: users -l -n -h -c -o -p{x\r\n"\
"        {gusers -l minlevel-maxlevel{x\r\n"\
"        {gusers -n playername{x\r\n"\
"        {gusers -h host{x\r\n"\
"        {gusers -c playerclass{x\r\n"

ACMD(do_users)
{
  extern char *connected_types[];
  char line[200], line2[220], idletime[10], classname[20], hostname[24];
  char state[30], *timeptr;

  struct char_data *tch;
  struct descriptor_data *d;
  char name_search[MAX_INPUT_LENGTH], host_search[MAX_INPUT_LENGTH], mode, *format;
  int low = 0, high = LVL_IMMORT, num_can_see = 0;
  int outlaws = 0, playing = 0, deadweight = 0;

  strcpy(buf, argument);
  memset(line, 0, 200);
  memset(line2, 0, 200);
  memset(idletime, 0, 10);
  memset(classname, 0, 20);
  memset(hostname, 0, 24);
  memset(state, 0, 30);
  memset(name_search, 0, MAX_INPUT_LENGTH);
  memset(host_search, 0, MAX_INPUT_LENGTH);
  while (*buf) {
    half_chop(buf, arg, buf1);
    if (*arg == '-') {
      mode = *(arg + 1); /* just in case; we destroy arg in the switch */
      switch (mode) {
        case 'o':
        case 'k':
          outlaws = 1;
          playing = 1;
          strcpy(buf, buf1);
          break;
        case 'p':
          playing = 1;
          strcpy(buf, buf1);
          break;
        case 'd':
          deadweight = 1;
          strcpy(buf, buf1);
          break;
        case 'l':
          playing = 1;
          half_chop(buf1, arg, buf);
          sscanf(arg, "%d-%d", &low, &high);
          break;
        case 'n':
          playing = 1;
          half_chop(buf1, name_search, buf);
          break;
        case 'h':
          playing = 1;
          half_chop(buf1, host_search, buf);
          break;
        default:
          sprintf(buf1, USERS_FORMAT);
          send_to_char(buf1, ch);
          return;
      } /* end of switch */

    } else { /* endif */
      send_to_char(USERS_FORMAT, ch);
      return;
    }
  } /* end while (parser) */
  strcpy(line, "\r\n{gNum Class    Name         State          Idl Login@   Site{x\r\n{G--- -------- ------------ -------------- --- -------- -----------------------{x\r\n");
  send_to_char(line, ch);

  one_argument(argument, arg);

  for (d = descriptor_list; d; d = d->next) {
    if (d->connected && playing && d->connected != CON_MEDIT && d->connected != CON_OEDIT && d->connected != CON_REDIT) {
      continue;
    }
    if (!d->connected && deadweight) {
      continue;
    }
    if (!d->connected || d->connected == CON_REDIT || d->connected == CON_OEDIT || d->connected == CON_MEDIT) {
      if (d->original) {
        tch = d->original;
      } else if (!(tch = d->character)) {
        continue;
      }

      if (*host_search && !strstr(GET_HOST(d->character), host_search)) {
        continue;
      }
      if (*name_search && str_cmp(GET_NAME(tch), name_search)) {
        continue;
      }
      if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high) {
        continue;
      }
      if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) && !PLR_FLAGGED(tch, PLR_THIEF)) {
        continue;
      }
      if (GET_INVIS_LEV(ch) > GET_LEVEL(ch)) {
        continue;
      }

      if (d->original) {
        sprintf(classname, "{B[{W%2d %s{B]{x", GET_LEVEL(d->original), CLASS_ABBR(d->original));
      } else {
        sprintf(classname, "{B[{W%2d %s{B]{x", GET_LEVEL(d->character), CLASS_ABBR(d->character));
      }
    } else {
      strcpy(classname, "   -    ");
    }

    timeptr = asctime(localtime(&d->login_time));
    timeptr += 11;
    *(timeptr + 8) = '\0';

    if (!d->connected && d->original) {
      strcpy(state, "Switched");
    } else {
      strcpy(state, connected_types[d->connected]);
    }

    /* swap this line with the one below it. to remove idle counter bein shown
     if (d->character && !d->connected && !COM_FLAGGED(d->character, COM_QUEST))
     */
    if (d->character && !d->connected) {
      sprintf(idletime, "%3d", d->character->char_specials.timer * SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
    } else {
      strcpy(idletime, "");
    }

    format = "%3d %-7s %-12s %-14s %-3s %-8s ";

    if (d->character && d->character->player.name) {
      if (d->original) {
        sprintf(line, format, d->desc_num, classname, d->original->player.name, state, idletime, timeptr);
      } else {
        sprintf(line, format, d->desc_num, classname, d->character->player.name, state, idletime, timeptr);
      }
    } else {
      sprintf(line, format, d->desc_num, "   -   ", "UNDEFINED", state, idletime, timeptr);
    }

    if (d->character && GET_HOST(d->character) && *GET_HOST(d->character)) {
      strncpy(hostname, GET_HOST(d->character), 23);
      if (strlen(GET_HOST(d->character)) < 23) {
        *(hostname + strlen(GET_HOST(d->character))) = '\0';
      } else {
        hostname[23] = '\0';
      }
      sprintf(line + strlen(line), "%s[%s%-23s%s]%s\r\n", CCCYN(ch, C_CMP), CBWHT(ch, C_SPR), hostname, CCCYN(ch, C_SPR), CCNRM(ch, C_SPR));
    } else {
      strcat(line, "[Hostname unknown]\r\n");
    }

    if (d->connected) {
      sprintf(line2, "%s%s%s", CCGRN(ch, C_SPR), line, CCNRM(ch, C_SPR));
      strcpy(line, line2);
    }
    if (d->connected || (!d->connected && CAN_SEE(ch, d->character))) {
      send_to_char(line, ch);
      num_can_see++;
    }
  }

  sprintf(line, "\r\n%d visible sockets connected.\r\n", num_can_see);
  send_to_char(line, ch);
}

/* Generic page_string function for displaying text */ACMD(do_gen_ps)
{
  extern char circlemud_version[];

  switch (subcmd) {
    case SCMD_CREDITS:
      page_string(ch->desc, credits, 1);
      break;
    case SCMD_NEWS:
      page_string(ch->desc, news, 1);
      break;
    case SCMD_INFO:
      page_string(ch->desc, info, 1);
      break;
    case SCMD_WIZLIST:
      file_to_string_alloc(WIZLIST_FILE, &wizlist);
      page_string(ch->desc, wizlist, 1);
      break;
    case SCMD_IMMLIST:
      file_to_string_alloc(IMMLIST_FILE, &immlist);
      page_string(ch->desc, immlist, 1);
      break;
    case SCMD_HANDBOOK:
      page_string(ch->desc, handbook, 1);
      break;
    case SCMD_POLICIES:
      page_string(ch->desc, policies, 1);
      break;
    case SCMD_NAMEPOL:
      page_string(ch->desc, namepol, 1);
      break;
    case SCMD_MOTD:
      page_string(ch->desc, motd, 1);
      break;
    case SCMD_NMOTD:
      page_string(ch->desc, nmotd, 1);
      break;
    case SCMD_IMOTD:
      page_string(ch->desc, imotd, 1);
      break;
    case SCMD_CLEAR:
      send_to_char("\033[H\033[J", ch);
      break;
    case SCMD_VERSION:
      send_to_char(circlemud_version, ch);
      break;
    case SCMD_WHOAMI:
      send_to_char(strcat(strcpy(buf, GET_NAME(ch)), "\r\n"), ch);
      break;
    case SCMD_QM:
      page_string(ch->desc, quest, 1);
      break;
    case SCMD_SIGNOFF:
      page_string(ch->desc, signoff, 1);
      break;
    default:
      return;
  }
}

void print_object_location(int num, struct obj_data * obj, struct char_data * ch, char *abuff, int recur)
{
  if (num > 0) {
    sprintf(abuff, "%sO%3d. %-25s [%5d] - ", abuff, num, (obj->cshort_description ? obj->cshort_description : obj->short_description), GET_OBJ_VNUM(obj));
  } else {
    sprintf(abuff, "%s%42s", abuff, " - ");
  }

  if (obj->in_room > NOWHERE) {
    sprintf(abuff, "%s[%5d] %s\r\n", abuff, world[obj->in_room].number, world[obj->in_room].name);
  } else if (obj->carried_by) {
    sprintf(abuff, "%scarried by %s\r\n", abuff, PERS(obj->carried_by, ch));
  } else if (obj->worn_by) {
    sprintf(abuff, "%sworn by %s\r\n", abuff, PERS(obj->worn_by, ch));
  } else if (obj->in_obj) {
    sprintf(abuff, "%sinside %s%s\n\r", abuff, (obj->in_obj->cshort_description ? obj->in_obj->cshort_description : obj->in_obj->short_description), (recur ? ", which is" : " "));
    if (recur) {
      print_object_location(0, obj->in_obj, ch, abuff, recur);
    }
  } else {
    sprintf(abuff, "%sin an unknown location\r\n", abuff);
  }
}

void perform_immort_where(struct char_data * ch, char *arg)
{
  register struct char_data *i;
  register struct obj_data *k;
  struct descriptor_data *d;
  int num = 0, found = 0;
  char abuff[MAX_STRING_LENGTH * 8];

  abuff[0] = '\0';
  if (!*arg || (!COM_FLAGGED(ch, COM_QUEST))) {
    send_to_char("Players\r\n-------\r\n", ch);
    for (d = descriptor_list; d; d = d->next)
      if (!d->connected) {
        i = (d->original ? d->original : d->character);
        if (i && CAN_SEE(ch, i) && (i->in_room != NOWHERE)) {
          if (d->original)
            sprintf(abuff, "%s%-20s - [%5d] %s (in %s)\r\n", abuff, GET_NAME(i), world[d->character->in_room].number, world[d->character->in_room].name, GET_NAME(d->character));
          else
            sprintf(abuff, "%s%-20s - [%5d] %s\r\n", abuff, GET_NAME(i), world[i->in_room].number, world[i->in_room].name);
        }
      }
    page_string(ch->desc, abuff, 1);
  } else {
    for (i = character_list; i; i = i->next) {
      if (CAN_SEE(ch, i) && i->in_room != NOWHERE && isname(arg, i->player.name) && check_access(ch, i->in_room)) {
        found = 1;
        sprintf(abuff, "%s%c%3d. %-25s [%5ld] - [%5d] %s\r\n", abuff, IS_NPC(i) ? 'M' : 'P', ++num, GET_NAME(i), IS_NPC(i) ? (long) GET_MOB_VNUM(i) : (long) GET_IDNUM(i),
        world[i->in_room].number, world[i->in_room].name);
      }
    }
    for (num = 0, k = object_list; k; k = k->next) {
      if (CAN_SEE_OBJ(ch, k) && (k->cname ? isname(arg, k->cname) : isname(arg, k->name))) {
        if ((k->in_room > NOWHERE) && !check_access(ch, k->in_room)) {
          continue;
        } else if (k->carried_by && !check_access(ch, k->carried_by->in_room)) {
          continue;
        } else if (k->worn_by && !check_access(ch, k->worn_by->in_room)) {
          continue;
        } else if (k->in_obj) {
          if ((k->in_obj->in_room > NOWHERE) && !check_access(ch, k->in_obj->in_room)) {
            continue;
          } else if (k->in_obj->carried_by && !check_access(ch, k->in_obj->carried_by->in_room)) {
            continue;
          } else if (k->in_obj->worn_by && !check_access(ch, k->in_obj->worn_by->in_room)) {
            continue;
          } else if (k->in_obj->in_obj) {
            continue;
          }
        }
        found = 1;
        print_object_location(++num, k, ch, abuff, TRUE);
      }
    }
    if (!found) {
      send_to_char("Couldn't find any such thing.\r\n", ch);
    } else {
      page_string(ch->desc, abuff, 1);
      sprintf(logbuffer, "%s did a 'where %s'", GET_NAME(ch), arg);
      mudlog(logbuffer, 'X', COM_IMMORT, FALSE);
    }
  }
}

ACMD(do_where)
{
  one_argument(argument, arg);

  if (GET_LEVEL(ch) >= LVL_IMMORT) {
    perform_immort_where(ch, arg);
  } else {
    send_to_char("You are here.\r\n", ch);
  }
}

ACMD(do_consider)
{
  struct char_data *victim;
  int diff;

  one_argument(argument, buf);

  if (!(victim = get_char_room_vis(ch, buf))) {
    send_to_char("Consider killing who?\r\n", ch);
    return;
  }
  if (victim == ch) {
    send_to_char("Hah! Easy! Very easy indeed!\r\n", ch);
    return;
  }
  if (!IS_NPC(victim)) {
    send_to_char("Would you like to borrow a cross and a shovel?\r\n", ch);
    return;
  }
  diff = (GET_LEVEL(victim) - GET_LEVEL(ch));

  if (diff <= -10) {
    send_to_char("Now where did that chicken go?\r\n", ch);
  } else if (diff <= -8) {
    send_to_char("You could do it with a needle!\r\n", ch);
  } else if (diff <= -6) {
    send_to_char("Easy.\r\n", ch);
  } else if (diff <= -4) {
    send_to_char("Fairly easy.\r\n", ch);
  } else if (diff <= -2) {
    send_to_char("The perfect match!\r\n", ch);
  } else if (diff <= -1) {
    send_to_char("You would need some luck!\r\n", ch);
  } else if (diff == 0) {
    send_to_char("You would need a lot of luck!\r\n", ch);
  } else if (diff <= 1) {
    send_to_char("You would need a lot of luck and great equipment!\r\n", ch);
  } else if (diff <= 2) {
    send_to_char("Do you feel lucky, punk?\r\n", ch);
  } else if (diff <= 4) {
    send_to_char("Are you mad!?\r\n", ch);
  } else if (diff <= 6) {
    send_to_char("You and what army?!\r\n", ch);
  } else if (diff <= 8) {
    send_to_char("Why don't you just lie down and play dead?\r\n", ch);
  } else if (diff <= 10) {
    send_to_char("You will have your head smashed in!\r\n", ch);
  } else if (diff <= 20) {
    send_to_char("What? Life is not good enough?\r\n", ch);
  } else if (diff <= 40) {
    send_to_char("R.I.P. Sound familiar?\r\n", ch);
  } else if (diff <= 55) {
    send_to_char("You drop DEAD from fear!\r\n", ch);
  } else if (diff <= 100) {
    send_to_char("Yeah, right..\r\n", ch);
  }
}

ACMD(do_diagnose)
{
  struct char_data *vict;

  one_argument(argument, buf);

  if (*buf) {
    if (!(vict = get_char_room_vis(ch, buf))) {
      send_to_char(NOPERSON, ch);
      return;
    } else {
      diag_char_to_char(vict, ch);
    }
  } else {
    if (FIGHTING(ch)) {
      diag_char_to_char(FIGHTING(ch), ch);
    } else {
      send_to_char("Diagnose who?\r\n", ch);
    }
  }
}

ACMD(do_toggle)
{
  struct follow_type *j, *k;

  if (IS_NPC(ch)) {
    return;
  }
  if (!*argument) {
    if (GET_WIMP_LEV(ch) == 0) {
      strcpy(buf2, "OFF");
    } else {
      sprintf(buf2, "%-3d", GET_WIMP_LEV(ch));
    }

    sprintf(buf, "{B=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-={x\r\n"
        "{c    Hit Display:{x %-3s    " "{c   Move Display:{x %-3s    " "{c   Mana Display:{x %-3s\r\n"
        "{c  Tankname disp:{x %-3s    " "{c  Tankcond disp:{x %-3s    " "{c Enemyname disp:{x %-3s\r\n"
        "{c Enemycond disp:{x %-3s    " "{c            AFK:{x %-3s    " "{c           Anon:{x %-3s\r\n"
        "{B=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-={x\r\n"
        "{c     Brief Mode:{x %-3s    " "{c   Compact Mode:{x %-3s    " "{c           Tell:{x %-3s\r\n"
        "{c   Repeat Comm.:{x %-3s    " "{c       Autoexit:{x %-3s    " "{c     Wimp Level:{x %-3s\r\n"
        "{c       Autoloot:{x %-3s    " "{c      Autosplit:{x %-3s    " "{c      Autocoins:{x %-3s\r\n"
        "{c   Chat Channel:{x %-3s    " "{c  Shout Channel:{x %-3s    " "{c          Color:{x %s  \r\n"
        "{c     Whois info:{x %-3s    " "{c      Num lines:{x %-3d    " "{c    Num columns:{x %-3d\r\n"
        /*"{c     Fightcolor:{x %-3s\r\n"*/
        "{B=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-={x\r\n",

    ONOFF(IS_SET(GET_PROMPT(ch), PRM_HP)), ONOFF(IS_SET(GET_PROMPT(ch), PRM_MOVE)), ONOFF(IS_SET(GET_PROMPT(ch), PRM_MANA)), ONOFF(IS_SET(GET_PROMPT(ch), PRM_TANKNAME)), ONOFF(IS_SET(GET_PROMPT(ch), PRM_TANKCOND)), ONOFF(IS_SET(GET_PROMPT(ch), PRM_ENEMYNAME)), ONOFF(IS_SET(GET_PROMPT(ch), PRM_ENEMYCOND)), ONOFF(IS_SET(GET_PROMPT(ch), PRM_AFK)), ONOFF(PRF_FLAGGED(ch, PRF_ANONYMOUS)),

    ONOFF((PRF_FLAGGED(ch, PRF_BRIEF)||PRF_FLAGGED(ch, PRF_BRIEF2))), ONOFF(PRF_FLAGGED(ch, PRF_COMPACT)), ONOFF(!PRF_FLAGGED(ch, PRF_NOTELL)), ONOFF(!PRF_FLAGGED(ch, PRF_NOREPEAT)), ONOFF(PRF_FLAGGED(ch, PRF_AUTOEXIT)), buf2, ONOFF(PRF_FLAGGED(ch, PRF_AUTOLOOT)), ONOFF(PRF_FLAGGED(ch, PRF_AUTOSPLIT)), ONOFF(PRF_FLAGGED(ch, PRF_AUTOGOLD)), ONOFF(!PRF_FLAGGED(ch, PRF_NOCHAT)), ONOFF(!PRF_FLAGGED(ch, PRF_DEAF)), ONOFF(PRF_FLAGGED(ch, PRF_COLOR_2)), ONOFF(PRF_FLAGGED(ch, PRF_WHOIS)), GET_SCREEN_HEIGHT(ch), GET_SCREEN_WIDTH(ch)/*,
     ONOFF(PRF_FLAGGED(ch, PRF_FIGHTCOLOR))*/
    );

    send_to_char(buf, ch);
    if (COM_FLAGGED(ch, COM_IMMORT)) {
      sprintf(buf, "{c       Nohassle:{x %-3s    "
          "{c    Wiz channel:{x %-3s    "
          "{c Iquest channel:{x %-3s\r\n"

          "{c      Roomflags:{x %-3s    "
          "{c      Holylight:{x %-3s    "
          "{c        Mobdeaf:{x %-3s\r\n"
          "{B=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-={x\r\n", ONOFF(PRF_FLAGGED(ch, PRF_NOHASSLE)), ONOFF(!PRF_FLAGGED(ch, PRF_NOWIZ)), ONOFF(!PRF_FLAGGED(ch, PRF_NOIMMQUEST)),

      ONOFF(PRF_FLAGGED(ch, PRF_ROOMFLAGS)), ONOFF(PRF_FLAGGED(ch, PRF_HOLYLIGHT)), ONOFF(PRF_FLAGGED(ch, PRF_MOBDEAF)));
      send_to_char(buf, ch);
    }
  } else {
    char first_arg[80];
    char second_arg[80];

    two_arguments(argument, first_arg, second_arg);
    if (strncasecmp(first_arg, "follower", strlen(first_arg)) == 0) {
      for (k = ch->followers; k; k = j) {
        j = k->next;
        stop_follower(k->follower);
      }
    } else if (strncasecmp(first_arg, "brief", strlen(first_arg)) == 0) {
      if (*second_arg && second_arg[0] == '2') {
        TOGGLE_BIT(PRF_FLAGS(ch), PRF_BRIEF2);
        if (PRF_FLAGGED(ch, PRF_BRIEF2)) {
          send_to_char("Brief mode level 2 on.\r\n", ch);
        } else {
          send_to_char("Brief mode level 2 off.\r\n", ch);
        }
      } else {
        TOGGLE_BIT(PRF_FLAGS(ch), PRF_BRIEF);
        if (PRF_FLAGGED(ch, PRF_BRIEF)) {
          send_to_char("Brief mode level 1 on.\r\n", ch);
        } else {
          send_to_char("Brief mode level 1 off.\r\n", ch);
        }
      }
    } else if (strncasecmp(first_arg, "repeat", strlen(first_arg)) == 0) {
      TOGGLE_BIT(PRF_FLAGS(ch), PRF_NOREPEAT);
      if (PRF_FLAGGED(ch, PRF_NOREPEAT)) {
        send_to_char("You will no longer have your communication repeated.\r\n", ch);
      } else {
        send_to_char("You will now have your communication repeated.\r\n", ch);
      }
    } else if (strncasecmp(first_arg, "anonymous", strlen(first_arg)) == 0) {
      TOGGLE_BIT(PRF_FLAGS(ch), PRF_ANONYMOUS);
      if (PRF_FLAGGED(ch, PRF_ANONYMOUS)) {
        send_to_char("You are now anonymous.\r\n", ch);
      } else {
        send_to_char("You are no longer anonymous.\r\n", ch);
      }
    } else if (strncasecmp(first_arg, "autoloot", strlen(first_arg)) == 0) {
      TOGGLE_BIT(PRF_FLAGS(ch), PRF_AUTOLOOT);
      if (PRF_FLAGGED(ch, PRF_AUTOLOOT)) {
        send_to_char("You are now auto-looting.\r\n", ch);
      } else {
        send_to_char("You are no longer auto-looting.\r\n", ch);
      }
    } else if (strncasecmp(first_arg, "autosplit", strlen(first_arg)) == 0) {
      TOGGLE_BIT(PRF_FLAGS(ch), PRF_AUTOSPLIT);
      if (PRF_FLAGGED(ch, PRF_AUTOSPLIT)) {
        send_to_char("You are now auto-splitting coins.\r\n", ch);
      } else {
        send_to_char("You are no longer auto-splitting coins.\r\n", ch);
      }
    } else if (strncasecmp(first_arg, "autocoins", strlen(first_arg)) == 0) {
      TOGGLE_BIT(PRF_FLAGS(ch), PRF_AUTOGOLD);
      if (PRF_FLAGGED(ch, PRF_AUTOGOLD)) {
        send_to_char("You are now auto-looting gold.\r\n", ch);
      } else {
        send_to_char("You are no longer auto-looting gold.\r\n", ch);
      }
    } else if (strncasecmp(first_arg, "chat", strlen(first_arg)) == 0) {
      TOGGLE_BIT(PRF_FLAGS(ch), PRF_NOCHAT);
      if (PRF_FLAGGED(ch, PRF_NOCHAT)) {
        send_to_char("You are now deaf to chat.\r\n", ch);
      } else {
        send_to_char("You can now hear chat...\r\n{RThis channel is for Non-Mud Related Topics ONLY!{x\r\n", ch);
      }
    } else if (strncasecmp(first_arg, "color", strlen(first_arg)) == 0) {
      TOGGLE_BIT(PRF_FLAGS(ch), PRF_COLOR_2);
      REMOVE_BIT(PRF_FLAGS(ch), PRF_COLOR_1);
      if (PRF_FLAGGED(ch, PRF_COLOR_2)) {
        ch->desc->color = 1;
        send_to_char("Color is now on.\r\n", ch);
      } else {
        ch->desc->color = 0;
        send_to_char("Color is now off.\r\n", ch);
      }
    } else if (strncasecmp(first_arg, "compact", strlen(first_arg)) == 0) {
      TOGGLE_BIT(PRF_FLAGS(ch), PRF_COMPACT);
      if (PRF_FLAGGED(ch, PRF_COMPACT)) {
        send_to_char("Compact mode on.\r\n", ch);
      } else {
        send_to_char("Compact mode off.\r\n", ch);
      }
    } else if (strncasecmp(first_arg, "fightcolor", strlen(first_arg)) == 0) {
      /* removed until it's done
       TOGGLE_BIT(PRF_FLAGS(ch), PRF_FIGHTCOLOR);
       if (PRF_FLAGGED(ch, PRF_FIGHTCOLOR)) {
       send_to_char("You will now receive fight colors.\r\n", ch);
       ch->desc->fightcolor = 1;
       } else {
       send_to_char("You will no longer recieve fight colors.\r\n", ch);
       ch->desc->fightcolor = 0;
       }
       */
      send_to_char("This command is still under construction, please be patient.\r\n", ch);
      /*ch->desc->fightcolor = 0;*/
    } else if (strncasecmp(first_arg, "tell", strlen(first_arg)) == 0) {
      TOGGLE_BIT(PRF_FLAGS(ch), PRF_NOTELL);
      if (PRF_FLAGGED(ch, PRF_NOTELL)) {
        send_to_char("You are now deaf to tells.\r\n", ch);
      } else {
        send_to_char("You can now hear tells.\r\n", ch);
      }
    } else if (strncasecmp(first_arg, "autoexit", strlen(first_arg)) == 0) {
      TOGGLE_BIT(PRF_FLAGS(ch), PRF_AUTOEXIT);
      if (PRF_FLAGGED(ch, PRF_AUTOEXIT)) {
        send_to_char("Autoexits enabled.\r\n", ch);
      } else {
        send_to_char("Autoexits disabled.\r\n", ch);
      }
    } else if (strncasecmp(first_arg, "shout", strlen(first_arg)) == 0) {
      TOGGLE_BIT(PRF_FLAGS(ch), PRF_DEAF);
      if (PRF_FLAGGED(ch, PRF_DEAF)) {
        send_to_char("You are now deaf to shouts.\r\n", ch);
      } else {
        send_to_char("You can now hear shouts.\r\n", ch);
      }
    } else if (strncasecmp(first_arg, "hit", strlen(first_arg)) == 0) {
      TOGGLE_BIT(GET_PROMPT(ch), PRM_HP);
      if (IS_SET(GET_PROMPT(ch), PRM_HP)) {
        send_to_char("You will now see your hitpoints in the prompt.\r\n", ch);
      } else {
        send_to_char("You will not see your hitpoints in the prompt.\r\n", ch);
      }
    } else if (strncasecmp(first_arg, "move", strlen(first_arg)) == 0) {
      TOGGLE_BIT(GET_PROMPT(ch), PRM_MOVE);
      if (IS_SET(GET_PROMPT(ch), PRM_MOVE)) {
        send_to_char("You will now see your moves in the prompt.\r\n", ch);
      } else {
        send_to_char("You will not see your moves in the prompt.\r\n", ch);
      }
    } else if (strncasecmp(first_arg, "tankname", strlen(first_arg)) == 0) {
      TOGGLE_BIT(GET_PROMPT(ch), PRM_TANKNAME);
      if (IS_SET(GET_PROMPT(ch), PRM_TANKNAME)) {
        send_to_char("You will now see the name of the tank in the prompt.\r\n", ch);
      } else {
        send_to_char("You will not see the name of the tank in the prompt.\r\n", ch);
      }
    } else if (strncasecmp(first_arg, "tankcond", strlen(first_arg)) == 0) {
      TOGGLE_BIT(GET_PROMPT(ch), PRM_TANKCOND);
      if (IS_SET(GET_PROMPT(ch), PRM_TANKCOND)) {
        send_to_char("You will now see the condition of the tank in the prompt.\r\n", ch);
      } else {
        send_to_char("You will not see the condition of the tank in the prompt.\r\n", ch);
      }
    } else if (strncasecmp(first_arg, "enemyname", strlen(first_arg)) == 0) {
      TOGGLE_BIT(GET_PROMPT(ch), PRM_ENEMYNAME);
      if (IS_SET(GET_PROMPT(ch), PRM_ENEMYNAME)) {
        send_to_char("You will now see the name of the enemy in the prompt.\r\n", ch);
      } else {
        send_to_char("You will not see the name of the enemy in the prompt.\r\n", ch);
      }
    } else if (strncasecmp(first_arg, "enemycond", strlen(first_arg)) == 0) {
      TOGGLE_BIT(GET_PROMPT(ch), PRM_ENEMYCOND);
      if (IS_SET(GET_PROMPT(ch), PRM_ENEMYCOND)) {
        send_to_char("You will now see the condition of the enemy in the prompt.\r\n", ch);
      } else {
        send_to_char("You will not see the condition of the enemy in the prompt.\r\n", ch);
      }
    } else if (strncasecmp(first_arg, "afk", strlen(first_arg)) == 0) {
      TOGGLE_BIT(GET_PROMPT(ch), PRM_AFK);
      TOGGLE_BIT(PRF_FLAGS(ch), PRF_AFK);
      if (IS_SET(GET_PROMPT(ch), PRM_AFK)) {
        send_to_char("You are now afk.\r\n", ch);
      } else {
        send_to_char("You are no longer afk.\r\n", ch);
      }
    } else if (strncasecmp(first_arg, "mana", strlen(first_arg)) == 0) {
      if (IS_MAGE(ch) || IS_PRIEST(ch)) {
        TOGGLE_BIT(GET_PROMPT(ch), PRM_MANA);
      } else {
        send_to_char("You are not a magic user.\r\n", ch);
        REMOVE_BIT(GET_PROMPT(ch), PRM_MANA);
      }
    } else if (strncasecmp(first_arg, "wimp", strlen(first_arg)) == 0) {
      int wimp_lev;

      if (!*second_arg) {
        if (GET_WIMP_LEV(ch)) {
          sprintf(buf, "Your current wimp level is %d hit points.\r\n", GET_WIMP_LEV(ch));
          send_to_char(buf, ch);
          return;
        } else {
          send_to_char("At the moment, you're not a wimp.  (sure, sure...)\r\n", ch);
          return;
        }
      }
      if (isdigit(*second_arg)) {
        if ((wimp_lev = atoi(second_arg))) {
          if (wimp_lev < 0) {
            send_to_char("Heh, heh, heh.. we are jolly funny today, eh?\r\n", ch);
          } else if (wimp_lev > GET_MAX_HIT(ch)) {
            send_to_char("That doesn't make much sense, now does it?\r\n", ch);
          } else if (wimp_lev > (GET_MAX_HIT(ch) >> 1)) {
            send_to_char("You can't set your wimp level above half your hit points.\r\n", ch);
          } else {
            sprintf(buf, "Okay, you'll wimp out if you drop below %d hit points.\r\n", wimp_lev);
            send_to_char(buf, ch);
            GET_WIMP_LEV(ch) = wimp_lev;
          }
        } else {
          send_to_char("Okay, you'll now tough out fights to the bitter end.\r\n", ch);
          GET_WIMP_LEV(ch) = 0;
        }
      } else {
        send_to_char("Specify at how many hit points you want to wimp out at.  (0 to disable)\r\n", ch);
      }
    } else if (strncasecmp(first_arg, "whois", strlen(first_arg)) == 0) {
      TOGGLE_BIT(PRF_FLAGS(ch), PRF_WHOIS);
      if (PRF_FLAGGED(ch, PRF_WHOIS)) {
        send_to_char("WHOIS information is now visible.\r\n", ch);
      } else {
        send_to_char("WHOIS information is no longer visible.\r\n", ch);
      }
    } else if (strncasecmp(first_arg, "nohassle", strlen(first_arg)) == 0) {
      if (!COM_FLAGGED(ch, COM_IMMORT)) {
        send_to_char("You wish to toggle what?\r\n", ch);
        return;
      }
      TOGGLE_BIT(PRF_FLAGS(ch), PRF_NOHASSLE);
      if (PRF_FLAGGED(ch, PRF_NOHASSLE)) {
        send_to_char("Nohassle enabled.\r\n", ch);
      } else {
        send_to_char("Nohassle disabled.\r\n", ch);
      }
    } else if (strncasecmp(first_arg, "nowiz", strlen(first_arg)) == 0) {
      if (!COM_FLAGGED(ch, COM_IMMORT)) {
        send_to_char("You wish to toggle what?\r\n", ch);
        return;
      }
      TOGGLE_BIT(PRF_FLAGS(ch), PRF_NOWIZ);
      if (PRF_FLAGGED(ch, PRF_NOWIZ)) {
        send_to_char("You are now deaf to the Wiz-channel.\r\n", ch);
      } else {
        send_to_char("You can now hear the Wiz-channel.\r\n", ch);
      }
    } else if (strncasecmp(first_arg, "noiquest", strlen(first_arg)) == 0) {
      if (!COM_FLAGGED(ch, COM_IMMORT)) {
        send_to_char("You wish to toggle what?\r\n", ch);
        return;
      }
      TOGGLE_BIT(PRF_FLAGS(ch), PRF_NOIMMQUEST);
      if (PRF_FLAGGED(ch, PRF_NOIMMQUEST)) {
        send_to_char("You are now deaf to the immquest channel.\r\n", ch);
      } else {
        send_to_char("You can now hear the immquest channel.\r\n", ch);
      }
    } else if (strncasecmp(first_arg, "roomflags", strlen(first_arg)) == 0) {
      if (!COM_FLAGGED(ch, COM_IMMORT)) {
        send_to_char("You wish to toggle what?\r\n", ch);
        return;
      }
      TOGGLE_BIT(PRF_FLAGS(ch), PRF_ROOMFLAGS);
      if (PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
        send_to_char("You will now see the room flags.\r\n", ch);
      } else {
        send_to_char("You will no longer see the room flags.\r\n", ch);
      }
    } else if (strncasecmp(first_arg, "holylight", strlen(first_arg)) == 0) {
      if (!COM_FLAGGED(ch, COM_IMMORT)) {
        send_to_char("You wish to toggle what?\r\n", ch);
        return;
      }
      TOGGLE_BIT(PRF_FLAGS(ch), PRF_HOLYLIGHT);
      if (PRF_FLAGGED(ch, PRF_HOLYLIGHT)) {
        send_to_char("HolyLight mode on.\r\n", ch);
      } else {
        send_to_char("HolyLight mode off.\r\n", ch);
      }
    } else if (strncasecmp(first_arg, "mobdeaf", strlen(first_arg)) == 0) {
      if (!COM_FLAGGED(ch, COM_IMMORT)) {
        send_to_char("You wish to toggle what?\r\n", ch);
        return;
      }
      TOGGLE_BIT(PRF_FLAGS(ch), PRF_MOBDEAF);
      if (PRF_FLAGGED(ch, PRF_MOBDEAF)) {
        send_to_char("You are now deaf to mob shouts.\r\n", ch);
      } else {
        send_to_char("You can now hear mob shouts.\r\n", ch);
      }
    } else if (strncasecmp(first_arg, "lines", strlen(first_arg)) == 0) {
      int lines;

      if (!*second_arg) {
        if (GET_SCREEN_HEIGHT(ch)) {
          sprintf(buf, "Your current screen length is set at %d lines.\r\n", GET_SCREEN_HEIGHT(ch));
          send_to_char(buf, ch);
          return;
        }
      }
      if (isdigit(*second_arg)) {
        if ((lines = atoi(second_arg))) {
          if (lines < 1) {
            send_to_char("Heh, heh, heh.. we are jolly funny today, eh?\r\n", ch);
            return;
          } else {
            sprintf(buf, "Okay, your screen length is now set at %d lines.\r\n", lines);
            send_to_char(buf, ch);
            GET_SCREEN_HEIGHT(ch) = lines;
          }
        }
      } else {
        send_to_char("Specify how many lines you want your screen to have (1 is minimum).\r\n", ch);
      }
    } else if (strncasecmp(first_arg, "columns", strlen(first_arg)) == 0) {
      /*
       int columns;

       if (!*second_arg) {
       if (GET_SCREEN_WIDTH(ch)) {
       sprintf(buf, "Your current screen width is set at %d columns.\r\n", GET_SCREEN_WIDTH(ch));
       send_to_char(buf, ch);
       return;
       }
       }
       if (isdigit(*second_arg)) {
       if ((columns = atoi(second_arg))) {
       if (columns < 1) {
       send_to_char("Heh, heh, heh.. we are jolly funny today, eh?\r\n", ch);
       return;
       } else {
       sprintf(buf, "Okay, your screen width is now set at %d columns.\r\n", columns);
       send_to_char(buf, ch);
       GET_SCREEN_WIDTH(ch) = columns;
       }
       }
       } else {
       send_to_char("Specify how many columns you want your screen to have (1 is minimum).\r\n", ch);
       }
       */
      send_to_char("This command is still under construction, please be patient.\r\n", ch);
    } else {
      send_to_char("You wish to toggle what?\r\n", ch);
    }
  }
}

ACMD(do_commands)
{
  int no, i;
  int socials = 0;
  struct char_data *vict = ch;
  int level = 0;
  ACMD(do_action);

  one_argument(argument, arg);

  if (*arg) {
    if ((!(vict = get_char_vis(ch, arg)) || IS_NPC(vict)) && !(is_number(arg))) {
      send_to_char("Who is that?\r\n", ch);
      return;
    }
    if (is_number(arg)) {
      level = MAX(atoi(arg), LVL_IMMORT);
      vict = ch;
    }
    if (GET_LEVEL(ch) < GET_LEVEL(vict) || level > GET_LEVEL(ch)) {
      return;
    }
  } else {
    vict = ch;
  }

  if (subcmd == SCMD_SOCIALS) {
    socials = 1;
  }

  sprintf(buf, "The following %s are available to %s:\r\n", socials ? "socials" : "commands", vict == ch ? "you" : GET_NAME(vict));

  if (subcmd == SCMD_SOCIALS) { /* socials */
    for (no = 1, i = 1; cmd_info[i].command[0] != '\n'; i++) {
      if (cmd_info[i].minimum_level >= 0 && (GET_LEVEL(vict) >= cmd_info[i].minimum_level) && (cmd_info[i].minimum_level < LVL_IMMORT) && (!AFF_FLAGGED(ch, cmd_info[i].anti_aff)) && (!AFF2_FLAGGED(ch, cmd_info[i].anti_aff2)) && (!AFF3_FLAGGED(ch, cmd_info[i].anti_aff3)) && (cmd_info[i].command_pointer == do_action)) {

        if (!(no % 7)) {
          sprintf(buf + strlen(buf), "%-11s\r\n", cmd_info[i].command);
        } else {
          sprintf(buf + strlen(buf), "%-11s", cmd_info[i].command);
        }

        no++;
      }
    }
  } else { /* commands */
    for (no = 1, i = 1; cmd_info[i].command[0] != '\n'; i++) {
      if ((cmd_info[i].minimum_position <= GET_POS(vict)) && cmd_info[i].minimum_level >= 0 && (GET_LEVEL(vict) >= cmd_info[i].minimum_level) && (cmd_info[i].minimum_level < LVL_IMMORT) && (cmd_info[i].command_pointer != do_action) && (!AFF_FLAGGED(ch, cmd_info[i].anti_aff)) && (!AFF2_FLAGGED(ch, cmd_info[i].anti_aff2)) && (!AFF3_FLAGGED(ch, cmd_info[i].anti_aff3)) && (strncmp(cmd_info[i].command, "mp", 2) != 0) && (strcmp(cmd_info[i].command, "return") != 0) && (strcmp(cmd_info[i].command, "sing") != 0) && (strcmp(cmd_info[i].command, "wiznet") != 0) && (strcmp(cmd_info[i].command, "helpn") != 0) && (cmd_info[i].command[0] != ';')) {

        if (!(no % 7)) {
          sprintf(buf + strlen(buf), "%-11s\r\n", cmd_info[i].command);
        } else {
          sprintf(buf + strlen(buf), "%-11s", cmd_info[i].command);
        }

        no++;
      }
    }
  }
  strcat(buf, "\r\n");
  page_string(ch->desc, buf, 1);
}

