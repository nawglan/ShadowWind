/************************************************************************
 * OasisOLC - medit.c            v1.5  *
 * Copyright 1996 Harvey Gilpin.          *
 ************************************************************************/

#include "comm.h"
#include "db.h"
#include "event.h"
#include "handler.h"
#include "olc.h"
#include "shop.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*-------------------------------------------------------------------*/

/*
 * External variable declarations.
 */
extern struct index_data *mob_index;
extern struct char_data *mob_proto;
extern struct char_data *character_list;
extern int top_of_mobt;
extern struct zone_data *zone_table;
extern int top_of_zone_table;
extern struct index_data *obj_index;
extern struct player_special_data dummy_mob;
extern struct attack_hit_type attack_hit_text[];
extern char *action_bits[];
extern char *affected_bits[];
extern char *affected_bits2[];
extern char *position_types[];
extern char *genders[];
extern char *where[];
extern char *resists_names[];
extern char *npc_class_types[];
extern char *npc_race_names[];
extern char *size_names[];
extern char *resist_short_name[];
extern struct max_mob_stats mob_stats[];
extern int top_shop;
extern struct shop_data *shop_index;
extern struct descriptor_data *descriptor_list;
extern const char *mobprog_types[];

/*-------------------------------------------------------------------*/

/*
 * Handy internal macros.
 */

#define GET_NDD(mob)        ((mob)->mob_specials.mob_attacks->nodice)
#define GET_SDD(mob)        ((mob)->mob_specials.mob_attacks->sizedice)
#define GET_ALIAS(mob)      ((mob)->player.name)
#define GET_SDESC(mob)      ((mob)->player.short_descr)
#define GET_DDESC(mob)      ((mob)->player.description)
#define GET_SIZE(mob)       ((mob)->mob_specials.size)
#define GET_ATTACKS(mob)    ((mob)->mob_specials.mob_attacks)
#define GET_EQUIP(mob)      ((mob)->mob_specials.mob_equip)
#define GET_ACTION(mob)     ((mob)->mob_specials.mob_action)
#define GET_ATTACK(mob)     ((mob)->mob_specials.mob_attacks->attack_type)
#define S_KEEPER(shop)      ((shop)->keeper)
#define GET_ST_HPD(level)   (mob_stats[(int)(level)].hp_dice)
#define GET_ST_HPS(level)   (mob_stats[(int)(level)].hp_sides)
#define GET_ST_HPB(level)   (mob_stats[(int)(level)].hp_bonus)
#define GET_ST_EXP(level)   (mob_stats[(int)(level)].experience)
#define GET_ST_GOLD(level)  (mob_stats[(int)(level)].gold)
#define GET_ST_THAC0(level) (mob_stats[(int)(level)].thac0)
#define GET_ST_AC(level)    (mob_stats[(int)(level)].ac)
#define GET_MPROG(mob)      (mob_index[(mob)->nr].mobprogs)
#define GET_MPROG_TYPE(mob) (mob_index[(mob)->nr].progtypes)
#define OLC_MTOTAL(d)       (d)->edit_number2

/*-------------------------------------------------------------------*/

/*
 * Function prototypes.
 */
void medit_parse(struct descriptor_data *d, char *arg);
void medit_disp_menu(struct descriptor_data *d);
void medit_setup_new(struct descriptor_data *d);
void medit_setup_existing(struct descriptor_data *d, int rmob_num);
void medit_save_internally(struct descriptor_data *d);
void medit_save_to_disk(int zone_num);
void init_mobile(struct char_data *mob);
void copy_mobile(struct char_data *tmob, struct char_data *fmob);
void medit_disp_positions(struct descriptor_data *d);
void medit_disp_sizes(struct descriptor_data *d);
void medit_disp_races(struct descriptor_data *d);
void medit_disp_mob_flags(struct descriptor_data *d);
void medit_disp_aff_flags(struct descriptor_data *d);
void medit_disp_aff2_flags(struct descriptor_data *d);
void medit_disp_attack_types(struct descriptor_data *d);
void medit_disp_attack_menu(struct descriptor_data *d);
void medit_change_attack(struct descriptor_data *d);
void medit_disp_action_menu(struct descriptor_data *d);
void medit_change_action(struct descriptor_data *d);
void medit_disp_equip_menu(struct descriptor_data *d);
void medit_change_equip(struct descriptor_data *d);
void medit_disp_equip_pos(struct descriptor_data *d);
void medit_disp_e_spec(struct descriptor_data *d);
void olc_print_bitvectors(FILE *f, long bitvector, long max);
void medit_disp_resist_menu(struct descriptor_data *d);
void medit_disp_mprog(struct descriptor_data *d);
void medit_change_mprog(struct descriptor_data *d);
const char *medit_get_mprog_type(struct mob_prog_data *mprog);
char *delete_doubledollar(char *string);

/*-------------------------------------------------------------------*\
  utility functions
 \*-------------------------------------------------------------------*/

/*
 * Free a mobile structure that has been edited.
 * Take care of existing mobiles and their mob_proto!
 */

void medit_free_mobile(struct char_data *mob) {
  int i;
  /*
   * Non-prototyped mobile.  Also known as new mobiles.
   */
  if (!mob) {
    return;
  } else if (GET_MOB_RNUM(mob) == -1) {
    if (mob->player.name) {
      FREE(mob->player.name);
    }
    if (mob->player.title) {
      FREE(mob->player.title);
    }
    if (mob->player.short_descr) {
      FREE(mob->player.short_descr);
    }
    if (mob->player.long_descr) {
      FREE(mob->player.long_descr);
    }
    if (mob->player.description) {
      FREE(mob->player.description);
    }
  } else if ((i = GET_MOB_RNUM(mob)) > -1) { /* Prototyped mobile. */
    if (mob->player.name && mob->player.name != mob_proto[i].player.name) {
      FREE(mob->player.name);
    }
    if (mob->player.title && mob->player.title != mob_proto[i].player.title) {
      FREE(mob->player.title);
    }
    if (mob->player.short_descr && mob->player.short_descr != mob_proto[i].player.short_descr) {
      FREE(mob->player.short_descr);
    }
    if (mob->player.long_descr && mob->player.long_descr != mob_proto[i].player.long_descr) {
      FREE(mob->player.long_descr);
    }
    if (mob->player.description && mob->player.description != mob_proto[i].player.description) {
      FREE(mob->player.description);
    }
  }
  while (mob->affected) {
    affect_remove(mob, mob->affected);
  }

  FREE(mob);
}

void medit_setup_new(struct descriptor_data *d) {
  struct char_data *mob;

  /*
   * Allocate a scratch mobile structure.
   */
  CREATE(mob, struct char_data, 1);

  init_mobile(mob);

  mob->player_specials = &dummy_mob;

  GET_MOB_RNUM(mob) = -1;
  /*
   * Set up some default strings.
   */
  GET_ALIAS(mob) = strdup("mob unfinished");
  GET_SDESC(mob) = strdup("the unfinished mob");
  GET_LDESC(mob) = strdup("An unfinished mob stands here.\r\n");
  GET_DDESC(mob) = strdup("It looks unfinished.\r\n");
  OLC_MPROGL(d) = NULL;
  OLC_MPROG(d) = NULL;

  GET_STR(mob) = 50;
  GET_DEX(mob) = 50;
  GET_AGI(mob) = 50;
  GET_INT(mob) = 50;
  GET_WIS(mob) = 50;
  GET_CON(mob) = 50;

  OLC_MOB(d) = mob;
  OLC_VAL(d) = 0; /* Has changed flag. (It hasn't so far, we just made it.) */

  GET_MOB_QUEST_NUM(OLC_MOB(d)) = -1;

  medit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

void medit_setup_existing(struct descriptor_data *d, int rmob_num) {
  struct char_data *mob;
  MPROG_DATA *temp;
  MPROG_DATA *head;

  /*
   * Allocate a scratch mobile structure.
   */
  CREATE(mob, struct char_data, 1);

  copy_mobile(mob, mob_proto + rmob_num);

  /*
   * I think there needs to be a brace from the if statement to the #endif
   * according to the way the original patch was indented.  If this crashes,
   * try it with the braces and report to greerga@van.ml.org on if that works.
   */
  if (GET_MPROG(mob))
    CREATE(OLC_MPROGL(d), MPROG_DATA, 1);
  head = OLC_MPROGL(d);
  for (temp = GET_MPROG(mob); temp; temp = temp->next) {
    OLC_MPROGL(d)->type = temp->type;
    OLC_MPROGL(d)->arglist = strdup(temp->arglist);
    OLC_MPROGL(d)->comlist = strdup(temp->comlist);
    if (temp->next) {
      CREATE(OLC_MPROGL(d)->next, MPROG_DATA, 1);
      OLC_MPROGL(d) = OLC_MPROGL(d)->next;
    }
  }
  OLC_MPROGL(d) = head;
  OLC_MPROG(d) = OLC_MPROGL(d);

  OLC_MOB(d) = mob;
  medit_disp_menu(d);
}

/*-------------------------------------------------------------------*/

/*
 * Copy one mob struct to another.
 */
void copy_mobile(struct char_data *tmob, struct char_data *fmob) {
  /*
   * Free up any used strings.
   */
  if (GET_ALIAS(tmob))
    FREE(GET_ALIAS(tmob));
  if (GET_SDESC(tmob))
    FREE(GET_SDESC(tmob));
  if (GET_LDESC(tmob))
    FREE(GET_LDESC(tmob));
  if (GET_DDESC(tmob))
    FREE(GET_DDESC(tmob));

  /*
   * Copy mob over.
   */
  *tmob = *fmob;

  /*
   * Reallocate strings.
   */
  GET_ALIAS(tmob) = strdup((GET_ALIAS(fmob) && *GET_ALIAS(fmob)) ? GET_ALIAS(fmob) : "undefined");
  GET_SDESC(tmob) = strdup((GET_SDESC(fmob) && *GET_SDESC(fmob)) ? GET_SDESC(fmob) : "undefined");
  GET_LDESC(tmob) = strdup((GET_LDESC(fmob) && *GET_LDESC(fmob)) ? GET_LDESC(fmob) : "undefined");
  GET_DDESC(tmob) = strdup((GET_DDESC(fmob) && *GET_DDESC(fmob)) ? GET_DDESC(fmob) : "undefined");
}

/*-------------------------------------------------------------------*/

/*
 * Ideally, this function should be in db.c, but I'll put it here for
 * portability.
 */
void init_mobile(struct char_data *mob) {
  clear_char(mob);

  GET_HIT(mob) = GET_MANA(mob) = 1;
  GET_MAX_MANA(mob) = GET_MAX_MOVE(mob) = 100;
  GET_WEIGHT(mob) = 200;
  GET_HEIGHT(mob) = 198;

  GET_STR(mob) = 50;
  GET_INT(mob) = 50;
  GET_WIS(mob) = 50;
  GET_DEX(mob) = 50;
  GET_AGI(mob) = 50;
  GET_CON(mob) = 50;

  mob->aff_abils = mob->real_abils;

  SET_BIT(MOB_FLAGS(mob), MOB_ISNPC);
}

/*-------------------------------------------------------------------*/

#define ZCMD zone_table[zone].cmd[cmd_no]

/*
 * Save new/edited mob to memory.
 */
void medit_save_internally(struct descriptor_data *d) {
  int rmob_num, found = 0, new_mob_num = 0, zone, cmd_no, shop;
  struct char_data *new_proto;
  struct index_data *new_index;
  struct char_data *live_mob;
  struct descriptor_data *dsc;

  /*
   * Mob exists? Just update it.
   */
  if ((rmob_num = real_mobile(OLC_NUM(d))) != -1) {
    copy_mobile((mob_proto + rmob_num), OLC_MOB(d));
    /*
     * Update live mobiles.
     */
    for (live_mob = character_list; live_mob; live_mob = live_mob->next)
      if (IS_MOB(live_mob) && GET_MOB_RNUM(live_mob) == rmob_num) {
        /*
         * Only really need to update the strings, since these can
         * cause protection faults.  The rest can wait till a reset/reboot.
         */
        GET_ALIAS(live_mob) = GET_ALIAS(mob_proto + rmob_num);
        GET_SDESC(live_mob) = GET_SDESC(mob_proto + rmob_num);
        GET_LDESC(live_mob) = GET_LDESC(mob_proto + rmob_num);
        GET_DDESC(live_mob) = GET_DDESC(mob_proto + rmob_num);
      }
  }
  /*
   * Mob does not exist, we have to add it.
   */
  else {

    CREATE(new_proto, struct char_data, top_of_mobt + 2);
    CREATE(new_index, struct index_data, top_of_mobt + 2);

    for (rmob_num = 0; rmob_num <= top_of_mobt; rmob_num++) {
      if (!found) { /* Is this the place? */
        /*  if ((rmob_num > top_of_mobt) || (mob_index[rmob_num].virtual > OLC_NUM(d))) {*/
        if (mob_index[rmob_num].virtual > OLC_NUM(d)) {
          /*
           * Yep, stick it here.
           */
          found = TRUE;
          new_index[rmob_num].virtual = OLC_NUM(d);
          new_index[rmob_num].number = 0;
          new_index[rmob_num].func = NULL;
          new_mob_num = rmob_num;
          GET_MOB_RNUM(OLC_MOB(d)) = rmob_num;
          copy_mobile((new_proto + rmob_num), OLC_MOB(d));
          /*
           * Copy the mob that should be here on top.
           */
          new_index[rmob_num + 1] = mob_index[rmob_num];
          new_proto[rmob_num + 1] = mob_proto[rmob_num];
          GET_MOB_RNUM(new_proto + rmob_num + 1) = rmob_num + 1;
        } else { /* Nope, copy over as normal. */
          new_index[rmob_num] = mob_index[rmob_num];
          new_proto[rmob_num] = mob_proto[rmob_num];
        }
      } else { /* We've already found it, copy the rest over. */
        new_index[rmob_num + 1] = mob_index[rmob_num];
        new_proto[rmob_num + 1] = mob_proto[rmob_num];
        GET_MOB_RNUM(new_proto + rmob_num + 1) = rmob_num + 1;
      }
    }
    if (!found) { /* Still not found, must add it to the top of the table. */
      new_index[rmob_num].virtual = OLC_NUM(d);
      new_index[rmob_num].number = 0;
      new_index[rmob_num].func = NULL;
      new_mob_num = rmob_num;
      GET_MOB_RNUM(OLC_MOB(d)) = rmob_num;
      copy_mobile((new_proto + rmob_num), OLC_MOB(d));
    }
    /*
     * Replace tables.
     */
    FREE(mob_index);
    FREE(mob_proto);
    mob_index = new_index;
    mob_proto = new_proto;
    top_of_mobt++;

    /*
     * Update live mobile rnums.
     */
    for (live_mob = character_list; live_mob; live_mob = live_mob->next)
      if (GET_MOB_RNUM(live_mob) > new_mob_num)
        GET_MOB_RNUM(live_mob)++;

    /*
     * Update zone table.
     */
    for (zone = 0; zone <= top_of_zone_table; zone++)
      for (cmd_no = 0; ZCMD.command != 'S'; cmd_no++)
        if (ZCMD.command == 'M')
          if (ZCMD.arg1 >= new_mob_num)
            ZCMD.arg1++;

    /*
     * Update shop keepers.
     */
    if (shop_index)
      for (shop = 0; shop <= top_shop; shop++)
        if (SHOP_KEEPER(shop) >= new_mob_num)
          SHOP_KEEPER(shop)++;

    /*
     * Update keepers in shops being edited and other mobs being edited.
     */
    for (dsc = descriptor_list; dsc; dsc = dsc->next)
      if (dsc->connected == CON_SEDIT) {
        if (S_KEEPER(OLC_SHOP(dsc)) >= new_mob_num)
          S_KEEPER(OLC_SHOP(dsc))++;
      } else if (dsc->connected == CON_MEDIT) {
        if (GET_MOB_RNUM(OLC_MOB(dsc)) >= new_mob_num)
          GET_MOB_RNUM(OLC_MOB(dsc))++;
      }
  }

  GET_MPROG(OLC_MOB(d)) = OLC_MPROGL(d);
  GET_MPROG_TYPE(OLC_MOB(d)) = (OLC_MPROGL(d) ? OLC_MPROGL(d)->type : 0);
  while (OLC_MPROGL(d)) {
    GET_MPROG_TYPE(OLC_MOB(d)) |= OLC_MPROGL(d)->type;
    OLC_MPROGL(d) = OLC_MPROGL(d)->next;
  }

  medit_save_to_disk(OLC_ZNUM(d));
}

/*-------------------------------------------------------------------*/

/*
 * Save ALL mobiles for a zone to their .mob file, mobs are all
 * saved in Extended format, regardless of whether they have any
 * extended fields.  Thanks to Sammy for ideas on this bit of code.
 */
void medit_save_to_disk(int zone_num) {
  int i, rmob_num, zone, top, counter;
  FILE *mob_file;
  char fname[64];
  struct char_data *mob;
  struct mob_attacks_data *attack;
  struct mob_equipment_data *equipment;
  struct mob_action_data *action;
  MPROG_DATA *mob_prog = NULL;

  zone = zone_table[zone_num].number;
  top = zone_table[zone_num].top;

  safe_snprintf(fname, sizeof(fname), "%s/%d.new", MOB_PREFIX, zone);
  if (!(mob_file = fopen(fname, "w"))) {
    mudlog("SYSERR: OLC: Cannot open mob file!", 'G', COM_BUILDER, TRUE);
    return;
  }

  /*
   * Seach the database for mobs in this zone and save them.
   */
  for (i = zone * 100; i <= top; i++) {
    if ((rmob_num = real_mobile(i)) != -1) {
      if (fprintf(mob_file, "#%d\n", i) < 0) {
        mudlog("SYSERR: OLC: Cannot write mob file!\r\n", 'G', COM_BUILDER, TRUE);
        fclose(mob_file);
        return;
      }
      mob = (mob_proto + rmob_num);
      attack = GET_ATTACKS(mob);
      action = GET_ACTION(mob);
      equipment = GET_EQUIP(mob);

      /*
       * Clean up strings.
       */
      safe_snprintf(buf1, MAX_STRING_LENGTH, "%s", (GET_LDESC(mob) && *GET_LDESC(mob)) ? GET_LDESC(mob) : "undefined");
      strip_string(buf1);
      safe_snprintf(buf2, MAX_STRING_LENGTH, "%s", (GET_DDESC(mob) && *GET_DDESC(mob)) ? GET_DDESC(mob) : "undefined");
      strip_string(buf2);

      fprintf(mob_file,
              "%s~\n"
              "%s~\n"
              "%s~\n"
              "%s~\n",
              (GET_ALIAS(mob) && *GET_ALIAS(mob)) ? GET_ALIAS(mob) : "undefined",
              (GET_SDESC(mob) && *GET_SDESC(mob)) ? GET_SDESC(mob) : "undefined", buf1, buf2);
      olc_print_bitvectors(mob_file, MOB_FLAGS(mob), NUM_MOB_FLAGS);
      olc_print_bitvectors(mob_file, AFF_FLAGS(mob), NUM_AFF_FLAGS);

      fprintf(mob_file, "%d X ", GET_ALIGNMENT(mob));
      olc_print_bitvectors(mob_file, AFF2_FLAGS(mob), NUM_AFF2_FLAGS);
      fprintf(mob_file,
              "\n"
              "%d %d %d %dd%d+%d\n"
              "%d %d %d\n"
              "%d %d %d %d %d %d\n",
              GET_LEVEL(mob), GET_HITROLL(mob), /* Hitroll -> THAC0 */
              GET_AC(mob) / 10, GET_HIT(mob), GET_MANA(mob), GET_MOVE(mob),

              GET_TEMP_GOLD(mob), GET_EXP(mob), (GET_MOB_QUEST_NUM(mob) ? GET_MOB_QUEST_NUM(mob) : -1),

              GET_POS(mob), GET_DEFAULT_POS(mob), GET_SEX(mob), GET_CLASS(mob), GET_MOB_RACE(mob), GET_SIZE(mob));

      while (attack) {
        fprintf(mob_file, "T %d %dd%d+%d %d\n", attack->attacks, attack->nodice, attack->sizedice, attack->damroll,
                attack->attack_type);
        attack = attack->next;
      }

      while (action) {
        fprintf(mob_file, "A %d %d %s\n", action->chance, action->minpos, action->action);
        action = action->next;
      }

      while (equipment) {
        fprintf(mob_file, "E %d %d %d %d\n", equipment->pos, equipment->chance, equipment->vnum, equipment->max);
        equipment = equipment->next;
      }

      /*
       * Deal with Extra stats in case they are there.
       */
      if (GET_STR(mob) != 50)
        fprintf(mob_file, "Str: %d\n", GET_STR(mob));
      if (GET_DEX(mob) != 50)
        fprintf(mob_file, "Dex: %d\n", GET_DEX(mob));
      if (GET_AGI(mob) != 50)
        fprintf(mob_file, "Agi: %d\n", GET_AGI(mob));
      if (GET_INT(mob) != 50)
        fprintf(mob_file, "Int: %d\n", GET_INT(mob));
      if (GET_WIS(mob) != 50)
        fprintf(mob_file, "Wis: %d\n", GET_WIS(mob));
      if (GET_CON(mob) != 50)
        fprintf(mob_file, "Con: %d\n", GET_CON(mob));

      /*. Deal with Resists in case they are there .*/
      for (counter = 1; counter <= NUM_RESISTS; counter++)
        if (GET_RESIST(mob, counter))
          fprintf(mob_file, "%s: %d\n", resist_short_name[counter - 1], GET_RESIST(mob, counter));

      /*
       * Write out the MobProgs.
       */
      mob_prog = GET_MPROG(mob);
      while (mob_prog) {
        safe_snprintf(buf1, MAX_STRING_LENGTH, "%s", mob_prog->arglist);
        strip_string(buf1);
        safe_snprintf(buf2, MAX_STRING_LENGTH, "%s", mob_prog->comlist);
        strip_string(buf2);
        fprintf(mob_file, "%s %s~\n%s", medit_get_mprog_type(mob_prog), buf1, buf2);
        mob_prog = mob_prog->next;
        fprintf(mob_file, "~\n%s", (!mob_prog ? "|\n" : ""));
      }
    }
  }
  fclose(mob_file);
  safe_snprintf(buf2, MAX_STRING_LENGTH, "%s/%d.mob", MOB_PREFIX, zone);
  /*
   * We're fubar'd if we crash between the two lines below.
   */
  remove(buf2);
  rename(fname, buf2);
}

/**************************************************************************
 Menu functions
 **************************************************************************/
/*. Display classes .*/

void medit_disp_classes(struct descriptor_data *d) {
  int i;

  get_char_cols(d->character);

  send_to_char("[H[J", d->character);
  for (i = 0; *npc_class_types[i] != '\n'; i++) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "%s%2d%s) %s\r\n", grn, i, nrm, npc_class_types[i]);
    send_to_char(buf, d->character);
  }
  send_to_char("Enter class : ", d->character);
}

/*. Display races .*/

void medit_disp_races(struct descriptor_data *d) {
  int i;

  get_char_cols(d->character);

  send_to_char("[H[J", d->character);
  for (i = 0; *npc_race_names[i] != '\n'; i++) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "%s%2d%s) %s\r\n", grn, i, nrm, npc_race_names[i]);
    send_to_char(buf, d->character);
  }
  send_to_char("Enter mob race : ", d->character);
}

/*. Display sizes .*/

void medit_disp_sizes(struct descriptor_data *d) {
  int i;

  get_char_cols(d->character);

  send_to_char("[H[J", d->character);
  for (i = 0; *size_names[i] != '\n'; i++) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "%s%2d%s) %s\r\n", grn, i, nrm, size_names[i]);
    send_to_char(buf, d->character);
  }
  send_to_char("Enter mob size : ", d->character);
}

/*
 * Display positions. (sitting, standing, etc)
 */
void medit_disp_positions(struct descriptor_data *d) {
  int i;

  get_char_cols(d->character);

  send_to_char("[H[J", d->character);
  for (i = 0; *position_types[i] != '\n'; i++) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "%s%2d%s) %s\r\n", grn, i, nrm, position_types[i]);
    send_to_char(buf, d->character);
  }
  send_to_char("Enter position number : ", d->character);
}

/*-------------------------------------------------------------------*/

/*
 * Get the type of MobProg.
 */
const char *medit_get_mprog_type(struct mob_prog_data *mprog) {
  switch (mprog->type) {
  case IN_FILE_PROG:
    return ">in_file_prog";
  case ACT_PROG:
    return ">act_prog";
  case SPEECH_PROG:
    return ">speech_prog";
  case RAND_PROG:
    return ">rand_prog";
  case FIGHT_PROG:
    return ">fight_prog";
  case HITPRCNT_PROG:
    return ">hitprcnt_prog";
  case DEATH_PROG:
    return ">death_prog";
  case ENTRY_PROG:
    return ">entry_prog";
  case GREET_PROG:
    return ">greet_prog";
  case ALL_GREET_PROG:
    return ">all_greet_prog";
  case GIVE_PROG:
    return ">give_prog";
  case BRIBE_PROG:
    return ">bribe_prog";
  case SHOUT_PROG:
    return ">shout_prog";
  case HOLLER_PROG:
    return ">holler_prog";
  case TELL_PROG:
    return ">tell_prog";
  case TIME_PROG:
    return ">time_prog";
  case ASK_PROG:
    return ">ask_prog";
  }
  return ">ERROR_PROG";
}

/*-------------------------------------------------------------------*/

/*
 * Display the MobProgs.
 */
void medit_disp_mprog(struct descriptor_data *d) {
  struct mob_prog_data *mprog = OLC_MPROGL(d);

  OLC_MTOTAL(d) = 1;

  send_to_char("[H[J", d->character);
  while (mprog) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "%d) %s %s\r\n", OLC_MTOTAL(d), medit_get_mprog_type(mprog),
                  (mprog->arglist ? mprog->arglist : "NONE"));
    send_to_char(buf, d->character);
    OLC_MTOTAL(d)++;
    mprog = mprog->next;
  }
  safe_snprintf(buf, MAX_STRING_LENGTH,
                "%d) Create New Mob Prog\r\n"
                "%d) Purge Mob Prog\r\n"
                "Enter number to edit [0 to exit]:  ",
                OLC_MTOTAL(d), OLC_MTOTAL(d) + 1);
  send_to_char(buf, d->character);
  OLC_MODE(d) = MEDIT_MPROG;
}

/*-------------------------------------------------------------------*/

/*
 * Change the MobProgs.
 */
void medit_change_mprog(struct descriptor_data *d) {
  send_to_char("[H[J", d->character);
  safe_snprintf(buf, MAX_STRING_LENGTH,
                "1) Type: %s\r\n"
                "2) Args: %s\r\n"
                "3) Commands:\r\n%s\r\n\r\n"
                "Enter number to edit [0 to exit]: ",
                medit_get_mprog_type(OLC_MPROG(d)), (OLC_MPROG(d)->arglist ? OLC_MPROG(d)->arglist : "NONE"),
                (OLC_MPROG(d)->comlist ? OLC_MPROG(d)->comlist : "NONE"));

  send_to_char(buf, d->character);
  OLC_MODE(d) = MEDIT_CHANGE_MPROG;
}

/*-------------------------------------------------------------------*/

/*
 * Change the MobProg type.
 */
void medit_disp_mprog_types(struct descriptor_data *d) {
  int i;

  get_char_cols(d->character);
  send_to_char("[H[J", d->character);

  for (i = 0; i < NUM_PROGS - 1; i++) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "%s%2d%s) %s\r\n", grn, i, nrm, mobprog_types[i]);
    send_to_char(buf, d->character);
  }
  send_to_char("Enter mob prog type : ", d->character);
  OLC_MODE(d) = MEDIT_MPROG_TYPE;
}

/*-------------------------------------------------------------------*/
/*. Display E-Specs .*/
void medit_disp_e_spec(struct descriptor_data *d) {

  send_to_char("[H[J", d->character);
  if (GET_STR(OLC_MOB(d)) != 50) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "1) Str:    %d\r\n", GET_STR(OLC_MOB(d)));
    send_to_char(buf, d->character);
  } else
    send_to_char("1) Str:    Not Set.\r\n", d->character);
  if (GET_DEX(OLC_MOB(d)) != 50) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "2) Dex:    %d\r\n", GET_DEX(OLC_MOB(d)));
    send_to_char(buf, d->character);
  } else
    send_to_char("2) Dex:    Not Set.\r\n", d->character);
  if (GET_AGI(OLC_MOB(d)) != 50) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "3) Agi:    %d\r\n", GET_AGI(OLC_MOB(d)));
    send_to_char(buf, d->character);
  } else
    send_to_char("3) Agi:    Not Set.\r\n", d->character);
  if (GET_INT(OLC_MOB(d)) != 50) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "4) Int:    %d\r\n", GET_INT(OLC_MOB(d)));
    send_to_char(buf, d->character);
  } else
    send_to_char("4) Int:    Not Set.\r\n", d->character);
  if (GET_WIS(OLC_MOB(d)) != 50) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "5) Wis:    %d\r\n", GET_WIS(OLC_MOB(d)));
    send_to_char(buf, d->character);
  } else
    send_to_char("5) Wis:    Not Set.\r\n", d->character);
  if (GET_CON(OLC_MOB(d)) != 50) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "6) Con:    %d\r\n", GET_CON(OLC_MOB(d)));
    send_to_char(buf, d->character);
  } else
    send_to_char("6) Con:    Not Set.\r\n", d->character);

  send_to_char("\r\nEnter E-Spec number [0 to go back]: ", d->character);
  OLC_MODE(d) = MEDIT_E_SPEC;
}

/*-------------------------------------------------------------------*/

/*
 * Display the gender of the mobile.
 */
void medit_disp_sex(struct descriptor_data *d) {
  int i;

  get_char_cols(d->character);

  send_to_char("[H[J", d->character);
  for (i = 0; i < NUM_GENDERS; i++) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "%s%2d%s) %s\r\n", grn, i, nrm, genders[i]);
    send_to_char(buf, d->character);
  }
  send_to_char("Enter gender number : ", d->character);
}

/*-------------------------------------------------------------------*/

/*
 * Display attack types menu.
 */
void medit_disp_attack_types(struct descriptor_data *d) {
  int i;

  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  for (i = 0; i < NUM_ATTACK_TYPES; i++) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "%s%2d%s) %s\r\n", grn, i, nrm, attack_hit_text[i].singular);
    send_to_char(buf, d->character);
  }
  send_to_char("Enter attack type : ", d->character);
  OLC_MODE(d) = MEDIT_ATTACK;
}

/*-------------------------------------------------------------------*/
/*. Display change attacks menu .*/

void medit_change_attack(struct descriptor_data *d) {
  struct mob_attacks_data *attack = GET_ATTACKS(OLC_MOB(d));
  int i;

  for (i = 1; i < d->edit_number2; i++)
    attack = attack->next;

  send_to_char("[H[J", d->character);
  send_to_char("1) Speed\r\n", d->character);
  send_to_char("2) Damage\r\n", d->character);
  send_to_char("3) Type of Attack\r\n", d->character);
  if (attack) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "\r\nCurrent Attack: T %d %dd%d+%d %s\r\n\r\n", attack->attacks,
                  attack->nodice, attack->sizedice, attack->damroll,
                  attack_hit_text[(int)attack->attack_type].singular);
    send_to_char(buf, d->character);
  } else
    send_to_char("\r\nCurrent Attack: Not Set.\r\n\r\n", d->character);
  send_to_char("Enter Choice [0 to exit]:  ", d->character);
  OLC_MODE(d) = MEDIT_CHANGE_ATTACK;
}

/*-------------------------------------------------------------------*/
/*. Display attacks menu .*/

void medit_disp_attack_menu(struct descriptor_data *d) {
  struct mob_attacks_data *attack = GET_ATTACKS(OLC_MOB(d));

  d->edit_number2 = 1;
  send_to_char("[H[J", d->character);
  while (attack) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "%d) T %d %dd%d+%d %s\r\n", d->edit_number2, attack->attacks, attack->nodice,
                  attack->sizedice, attack->damroll, attack_hit_text[(int)attack->attack_type].singular);
    send_to_char(buf, d->character);
    d->edit_number2++;
    attack = attack->next;
  }
  safe_snprintf(buf, MAX_STRING_LENGTH, "%d) Create New Attack\r\n", d->edit_number2);
  send_to_char(buf, d->character);
  safe_snprintf(buf, MAX_STRING_LENGTH, "%d) Purge Attack\r\n", d->edit_number2 + 1);
  send_to_char(buf, d->character);

  send_to_char("Enter number to edit [0 to exit]:  ", d->character);
  OLC_MODE(d) = MEDIT_ATTACK_MENU;
}

/*-------------------------------------------------------------------*/
/*. Display change action menu .*/

void medit_change_action(struct descriptor_data *d) {
  struct mob_action_data *action = GET_ACTION(OLC_MOB(d));
  int i;

  for (i = 1; i < d->edit_number2; i++)
    action = action->next;

  send_to_char("[H[J", d->character);
  send_to_char("1) Percentage\r\n", d->character);
  send_to_char("2) Min Position\r\n", d->character);
  send_to_char("3) Action\r\n", d->character);
  if (action) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "\r\nCurrent Action: A %d %d %s\r\n\r\n", action->chance, action->minpos,
                  action->action);
    send_to_char(buf, d->character);
  } else
    send_to_char("\r\nCurrent Action: Not Set.\r\n\r\n", d->character);
  send_to_char("Enter Choice [0 to exit]:  ", d->character);
  OLC_MODE(d) = MEDIT_CHANGE_ACTION;
}

/*-------------------------------------------------------------------*/
/*. Display action menu .*/

void medit_disp_action_menu(struct descriptor_data *d) {
  struct mob_action_data *action = GET_ACTION(OLC_MOB(d));

  d->edit_number2 = 1;
  send_to_char("[H[J", d->character);
  while (action) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "%d) A %d %d %s\r\n", d->edit_number2, action->chance, action->minpos,
                  action->action);
    send_to_char(buf, d->character);
    d->edit_number2++;
    action = action->next;
  }
  safe_snprintf(buf, MAX_STRING_LENGTH, "%d) Create New Action\r\n", d->edit_number2);
  send_to_char(buf, d->character);
  safe_snprintf(buf, MAX_STRING_LENGTH, "%d) Purge Action\r\n", d->edit_number2 + 1);
  send_to_char(buf, d->character);

  send_to_char("Enter number to edit [0 to exit]:  ", d->character);
  OLC_MODE(d) = MEDIT_ACTION_MENU;
}

/*-------------------------------------------------------------------*/
/*. Display equipment positions menu .*/

void medit_disp_equip_pos(struct descriptor_data *d) {
  int i, columns = 0;

  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  for (i = 0; i < NUM_WEARS; i++) {
    size_t blen = safe_snprintf(buf, MAX_STRING_LENGTH, "%s%2d%s) %-20.20s  ", grn, i, nrm, where[i]);
    if (!(++columns % 2))
      safe_snprintf(buf + blen, MAX_STRING_LENGTH - blen, "\r\n");
    send_to_char(buf, d->character);
  }

  send_to_char("\r\nEnter equipment position [-1 for inventory]:  ", d->character);
  OLC_MODE(d) = MEDIT_EQUIP_POS;
}

/*-------------------------------------------------------------------*/
/*. Display change equipment menu .*/

void medit_change_equip(struct descriptor_data *d) {
  struct mob_equipment_data *equip = GET_EQUIP(OLC_MOB(d));
  int i;

  for (i = 1; i < d->edit_number2; i++)
    equip = equip->next;

  send_to_char("[H[J", d->character);
  send_to_char("1) Position\r\n", d->character);
  send_to_char("2) Chance Load\r\n", d->character);
  send_to_char("3) Object Virtual Num\r\n", d->character);
  send_to_char("4) Max Existing\r\n", d->character);
  if (equip) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "\r\nCurrent Equipment: E %d %d %d %d\r\n\r\n", equip->pos, equip->chance,
                  equip->vnum, equip->max);
    send_to_char(buf, d->character);
  } else
    send_to_char("\r\nCurrent Equipment: Not Set.\r\n\r\n", d->character);
  send_to_char("Enter Choice [0 to exit]:  ", d->character);
  OLC_MODE(d) = MEDIT_CHANGE_EQUIP;
}

/*-------------------------------------------------------------------*/
/*. Display equipment menu .*/

void medit_disp_equip_menu(struct descriptor_data *d) {
  struct mob_equipment_data *equip = GET_EQUIP(OLC_MOB(d));

  d->edit_number2 = 1;
  send_to_char("[H[J", d->character);
  while (equip) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "%d) E %d %d %d %d\r\n", d->edit_number2, equip->pos, equip->chance,
                  equip->vnum, equip->max);
    send_to_char(buf, d->character);
    d->edit_number2++;
    equip = equip->next;
  }
  safe_snprintf(buf, MAX_STRING_LENGTH, "%d) Create New Equipment\r\n", d->edit_number2);
  send_to_char(buf, d->character);
  safe_snprintf(buf, MAX_STRING_LENGTH, "%d) Purge Equipment\r\n", d->edit_number2 + 1);
  send_to_char(buf, d->character);

  send_to_char("Enter number to edit [0 to exit]:  ", d->character);
  OLC_MODE(d) = MEDIT_EQUIP_MENU;
}

/*-------------------------------------------------------------------*/
/*. Display resists menu .*/

void medit_disp_resist_menu(struct descriptor_data *d) {
  int i;

  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  for (i = 1; i < NUM_RESISTS; i++) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "%s%2d%s) %-20.20s  %6d\r\n", grn, i, nrm, resists_names[i],
                  GET_RESIST(OLC_MOB(d), i));
    send_to_char(buf, d->character);
  }
  send_to_char("\r\nEnter resist type [0 to exit]: ", d->character);
}

/*-------------------------------------------------------------------*/

/*
 * Display mob-flags menu.
 */
void medit_disp_mob_flags(struct descriptor_data *d) {
  int i, columns = 0;

  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  for (i = 0; i < NUM_MOB_FLAGS; i++) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "%s%2d%s) %-20.20s  %s", grn, i + 1, nrm, action_bits[i],
                  !(++columns % 2) ? "\r\n" : "");
    send_to_char(buf, d->character);
  }
  sprintbit(MOB_FLAGS(OLC_MOB(d)), action_bits, buf1);
  safe_snprintf(buf, MAX_STRING_LENGTH, "\r\nCurrent flags : %s%s%s\r\nEnter mob flags (0 to quit) : ", cyn, buf1, nrm);
  send_to_char(buf, d->character);
}

/*-------------------------------------------------------------------*/

/*
 * Display affection flags menu.
 */
void medit_disp_aff_flags(struct descriptor_data *d) {
  int i, columns = 0;

  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  for (i = 0; i < NUM_AFF_FLAGS; i++) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "%s%2d%s) %-20.20s  %s", grn, i + 1, nrm, affected_bits[i],
                  !(++columns % 2) ? "\r\n" : "");
    send_to_char(buf, d->character);
  }
  sprintbit(AFF_FLAGS(OLC_MOB(d)), affected_bits, buf1);
  safe_snprintf(buf, MAX_STRING_LENGTH, "\r\nCurrent flags   : %s%s%s\r\nEnter aff flags (0 to quit) : ", cyn, buf1,
                nrm);
  send_to_char(buf, d->character);
}

void medit_disp_aff2_flags(struct descriptor_data *d) {
  int i, columns = 0;

  get_char_cols(d->character);
  send_to_char("[H[J", d->character);
  for (i = 0; i < NUM_AFF2_FLAGS; i++) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "%s%2d%s) %-20.20s  %s", grn, i + 1, nrm, affected_bits2[i],
                  !(++columns % 2) ? "\r\n" : "");
    send_to_char(buf, d->character);
  }
  sprintbit(AFF2_FLAGS(OLC_MOB(d)), affected_bits2, buf1);
  safe_snprintf(buf, MAX_STRING_LENGTH, "\r\nCurrent flags   : %s%s%s\r\nEnter aff flags (0 to quit) : ", cyn, buf1,
                nrm);
  send_to_char(buf, d->character);
}

/*-------------------------------------------------------------------*/

/*
 * Display main menu.
 */
void medit_disp_menu(struct descriptor_data *d) {
  struct char_data *mob;

  mob = OLC_MOB(d);
  get_char_cols(d->character);

  safe_snprintf(buf, MAX_STRING_LENGTH,
                "[H[J"
                "-- Mob Number:  [%s%d%s]\r\n"
                "%s0%s) Class: %s%s\r\n"
                "%s1%s) Sex: %s%-7.7s           %s2%s) Alias: %s%s\r\n"
                "%s3%s) S-Desc: %s%s\r\n"
                "%s4%s) L-Desc:-\r\n%s%s"
                "%s5%s) D-Desc:-\r\n%s%s"
                "%s6%s) Level:       [%s%4d%s],  %s7%s) Alignment:    [%s%4d%s],  %s8%s) Thac0:        [%s%4d%s]\r\n"
                "%s9%s) Num HP Dice: [%s%4d%s],  %sA%s) Size HP Dice: [%s%4d%s],  %sB%s) HP Bonus:     [%s%4d%s]\r\n"
                "%sC%s) Armor Class: [%s%4d%s],  %sD%s) Exp:          [%s%9d%s],  %sE%s) Copper:       [%s%8d%s]\r\n",

                cyn, OLC_NUM(d), nrm, grn, nrm, yel, npc_class_types[(int)GET_CLASS(mob)], grn, nrm, yel,
                genders[(int)GET_SEX(mob)], grn, nrm, yel, GET_ALIAS(mob), grn, nrm, yel, GET_SDESC(mob), grn, nrm, yel,
                GET_LDESC(mob), grn, nrm, yel, (GET_DDESC(mob) ? GET_DDESC(mob) : "NULL\r\n"), grn, nrm, cyn,
                GET_LEVEL(mob), nrm, grn, nrm, cyn, GET_ALIGNMENT(mob), nrm, grn, nrm, cyn, GET_HITROLL(mob), nrm, grn,
                nrm, cyn, GET_HIT(mob), nrm, grn, nrm, cyn, GET_MANA(mob), nrm, grn, nrm, cyn, GET_MOVE(mob), nrm, grn,
                nrm, cyn, GET_AC(mob), nrm, grn, nrm, cyn, GET_EXP(mob), nrm, grn, nrm, cyn, GET_TEMP_GOLD(mob), nrm);
  send_to_char(buf, d->character);

  sprintbit(MOB_FLAGS(mob), action_bits, buf1);
  sprintbit(AFF_FLAGS(mob), affected_bits, buf2);
  sprintbit(AFF2_FLAGS(mob), affected_bits2, buf3);
  safe_snprintf(
      buf, MAX_STRING_LENGTH,
      "%sF%s) Position  : %s%s"
      "  %sG%s) Default   : %s%s\r\n"
      "%sH%s) Attacks   : %s%s"
      "  %sI%s) Actions   : %s%s\r\n"
      "%sJ%s) Equipment : %s%s\r\n"
      "%sK%s) NPC Flags : %s%s\r\n"
      "%sL%s) AFF Flags : %s%s\r\n"
      "%sN%s) AFF2 Flags : %s%s\r\n"
      "%sM%s) E-Specs   : %s%s"
      "  %sP%s) Mob Progs : %s%s\r\n"
      "%sR%s) Mob Resists       "
      "%sS%s) Mob Race  : %s%s\r\n"
      "%sT%s) Mob Size  : %s%s"
      "  %sU%s) Mob Quest Num : %s%d\r\n"
      "%sQ%s) Quit\r\n"
      "Enter choice : ",

      grn, nrm, yel, position_types[(int)GET_POS(mob)], grn, nrm, yel, position_types[(int)GET_DEFAULT_POS(mob)], grn,
      nrm, cyn, (GET_ATTACKS(OLC_MOB(d)) ? "Set." : "Not Set."), grn, nrm, cyn,
      (GET_ACTION(OLC_MOB(d)) ? "Set." : "Not Set."), grn, nrm, cyn, (GET_EQUIP(OLC_MOB(d)) ? "Set." : "Not Set."), grn,
      nrm, cyn, buf1, grn, nrm, cyn, buf2, grn, nrm, cyn, buf3, grn, nrm, cyn,
      ((GET_STR(OLC_MOB(d)) == 50 && GET_DEX(OLC_MOB(d)) == 50 && GET_AGI(OLC_MOB(d)) == 50 &&
        GET_INT(OLC_MOB(d)) == 50 && GET_WIS(OLC_MOB(d)) == 50 && GET_CON(OLC_MOB(d)) == 50)
           ? "Not Set."
           : "Set."),
      grn, nrm, cyn, (OLC_MPROGL(d) ? "Set." : "Not Set."), grn, nrm, grn, nrm, yel,
      npc_race_names[(int)GET_MOB_RACE(mob)], grn, nrm, yel, size_names[(int)GET_MOB_SIZE(mob)], grn, nrm, yel,
      GET_MOB_QUEST_NUM(mob), grn, nrm);
  send_to_char(buf, d->character);

  OLC_MODE(d) = MEDIT_MAIN_MENU;
}

/************************************************************************
 *      The GARGANTAUN event handler      *
 ************************************************************************/

void medit_parse(struct descriptor_data *d, char *arg) {
  int i;

  if (OLC_MODE(d) > MEDIT_NUMERICAL_RESPONSE) {
    if (!*arg || (!isdigit(arg[0]) && ((*arg == '-') && (!isdigit(arg[1]))))) {
      send_to_char("Field must be numerical, try again : ", d->character);
      return;
    }
  }
  switch (OLC_MODE(d)) {
  /*-------------------------------------------------------------------*/
  case MEDIT_CONFIRM_SAVESTRING:
    /*
     * Ensure mob has MOB_ISNPC set or things will go pair shaped.
     */
    SET_BIT(MOB_FLAGS(OLC_MOB(d)), MOB_ISNPC);
    switch (*arg) {
    case 'y':
    case 'Y':
      /*
       * Save the mob in memory and to disk.
       */
      send_to_char("Saving mobile to memory and to disk.\r\n", d->character);
      medit_save_internally(d);
      safe_snprintf(buf, MAX_STRING_LENGTH, "OLC: %s finished editing mob %d", GET_NAME(d->character), OLC_NUM(d));
      mudlog(buf, 'G', MAX(COM_BUILDER, GET_INVIS_LEV(d->character)), TRUE);
      /* FALL THROUGH */
      cleanup_olc(d, CLEANUP_ALL);
      return;
    case 'n':
    case 'N':
      safe_snprintf(buf, MAX_STRING_LENGTH, "OLC: %s finished editing mob %d", GET_NAME(d->character), OLC_NUM(d));
      mudlog(buf, 'G', COM_BUILDER, TRUE);
      cleanup_olc(d, CLEANUP_ALL);
      return;
    default:
      send_to_char("Invalid choice!\r\n", d->character);
      send_to_char("Do you wish to save the mobile? : ", d->character);
      return;
    }

    /*-------------------------------------------------------------------*/
  case MEDIT_MAIN_MENU:
    i = 0;
    switch (*arg) {
    case 'q':
    case 'Q':
      if (OLC_VAL(d)) { /* Anything been changed? */
        send_to_char("Do you wish to save the changes to the mobile? (y/n) : ", d->character);
        OLC_MODE(d) = MEDIT_CONFIRM_SAVESTRING;
      } else {
        safe_snprintf(buf, MAX_STRING_LENGTH, "OLC: %s finished editing mob %d", GET_NAME(d->character), OLC_NUM(d));
        mudlog(buf, 'G', COM_BUILDER, TRUE);
        cleanup_olc(d, CLEANUP_ALL);
      }
      return;
    case '0':
      OLC_MODE(d) = MEDIT_CLASS;
      medit_disp_classes(d);
      return;
    case '1':
      OLC_MODE(d) = MEDIT_SEX;
      medit_disp_sex(d);
      return;
    case '2':
      OLC_MODE(d) = MEDIT_ALIAS;
      i--;
      break;
    case '3':
      OLC_MODE(d) = MEDIT_S_DESC;
      i--;
      break;
    case '4':
      OLC_MODE(d) = MEDIT_L_DESC;
      i--;
      break;
    case '5':
      OLC_MODE(d) = MEDIT_D_DESC;
      OLC_VAL(d) = 1;
      SEND_TO_Q("Enter mob description: (/s saves /h for help)\r\n\r\n", d);
      d->backstr = NULL;
      if (OLC_MOB(d)->player.description) {
        SEND_TO_Q(OLC_MOB(d)->player.description, d);
        d->backstr = strdup(OLC_MOB(d)->player.description);
      }
      d->str = &OLC_MOB(d)->player.description;
      d->max_str = MAX_MOB_DESC;
      d->mail_to = 0;
      return;
    case '6':
      OLC_MODE(d) = MEDIT_LEVEL;
      i++;
      break;
    case '7':
      OLC_MODE(d) = MEDIT_ALIGNMENT;
      i++;
      break;
    case '8':
      OLC_MODE(d) = MEDIT_THAC0;
      i++;
      break;
    case '9':
      OLC_MODE(d) = MEDIT_NUM_HP_DICE;
      i++;
      break;
    case 'a':
    case 'A':
      OLC_MODE(d) = MEDIT_SIZE_HP_DICE;
      i++;
      break;
    case 'b':
    case 'B':
      OLC_MODE(d) = MEDIT_ADD_HP;
      i++;
      break;
    case 'c':
    case 'C':
      OLC_MODE(d) = MEDIT_AC;
      i++;
      break;
    case 'd':
    case 'D':
      OLC_MODE(d) = MEDIT_EXP;
      i++;
      break;
    case 'e':
    case 'E':
      OLC_MODE(d) = MEDIT_GOLD;
      i++;
      break;
    case 'f':
    case 'F':
      OLC_MODE(d) = MEDIT_POS;
      medit_disp_positions(d);
      return;
    case 'g':
    case 'G':
      OLC_MODE(d) = MEDIT_DEFAULT_POS;
      medit_disp_positions(d);
      return;
    case 'h':
    case 'H':
      medit_disp_attack_menu(d);
      return;
    case 'i':
    case 'I':
      medit_disp_action_menu(d);
      return;
    case 'j':
    case 'J':
      medit_disp_equip_menu(d);
      return;
    case 'k':
    case 'K':
      OLC_MODE(d) = MEDIT_NPC_FLAGS;
      medit_disp_mob_flags(d);
      return;
    case 'l':
    case 'L':
      OLC_MODE(d) = MEDIT_AFF_FLAGS;
      medit_disp_aff_flags(d);
      return;
    case 'm':
    case 'M':
      OLC_MODE(d) = MEDIT_E_SPEC;
      medit_disp_e_spec(d);
      return;
    case 'n':
    case 'N':
      OLC_MODE(d) = MEDIT_AFF2_FLAGS;
      medit_disp_aff2_flags(d);
      return;
    case 'p':
    case 'P':
      OLC_MODE(d) = MEDIT_MPROG;
      medit_disp_mprog(d);
      return;
    case 'r':
    case 'R':
      OLC_MODE(d) = MEDIT_RESIST;
      medit_disp_resist_menu(d);
      return;
    case 's':
    case 'S':
      OLC_MODE(d) = MEDIT_RACE;
      medit_disp_races(d);
      return;
    case 't':
    case 'T':
      OLC_MODE(d) = MEDIT_SIZE;
      medit_disp_sizes(d);
      return;
    case 'u':
    case 'U':
      OLC_MODE(d) = MEDIT_QUESTNUM;
      i++;
      break;
    default:
      medit_disp_menu(d);
      return;
    }
    if (i != 0) {
      send_to_char(i == 1    ? "\r\nEnter new value : "
                   : i == -1 ? "\r\nEnter new text :\r\n] "
                             : "\r\nOops...:\r\n",
                   d->character);
      return;
    }
    break;

    /*-------------------------------------------------------------------*/
  case MEDIT_ALIAS:
    if (GET_ALIAS(OLC_MOB(d)))
      FREE(GET_ALIAS(OLC_MOB(d)));
    GET_ALIAS(OLC_MOB(d)) = strdup((arg && *arg) ? arg : "undefined");
    break;
    /*-------------------------------------------------------------------*/
  case MEDIT_S_DESC:
    if (GET_SDESC(OLC_MOB(d)))
      FREE(GET_SDESC(OLC_MOB(d)));
    GET_SDESC(OLC_MOB(d)) = strdup((arg && *arg) ? arg : "undefined");
    break;
    /*-------------------------------------------------------------------*/
  case MEDIT_L_DESC:
    if (GET_LDESC(OLC_MOB(d)))
      FREE(GET_LDESC(OLC_MOB(d)));
    if (arg && *arg) {
      safe_snprintf(buf, MAX_STRING_LENGTH, "%s\r\n", arg);
      GET_LDESC(OLC_MOB(d)) = strdup(buf);
    } else
      GET_LDESC(OLC_MOB(d)) = strdup("undefined");

    break;
    /*-------------------------------------------------------------------*/
  case MEDIT_D_DESC:
    /*
     * We should never get here.
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog("SYSERR: OLC: medit_parse(): Reached D_DESC case!", 'G', COM_BUILDER, TRUE);
    send_to_char("Oops...\r\n", d->character);
    break;
    /*-------------------------------------------------------------------*/
  case MEDIT_MPROG_COMLIST:
    /*
     * We should never get here, but if we do, bail out.
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog("SYSERR: OLC: medit_parse(): Reached MPROG_COMLIST case!", 'G', COM_BUILDER, TRUE);
    break;
    /*-------------------------------------------------------------------*/
  case MEDIT_NPC_FLAGS:
    if ((i = atoi(arg)) == 0)
      break;
    else if (!((i < 0) || (i > NUM_MOB_FLAGS)))
      TOGGLE_BIT(MOB_FLAGS(OLC_MOB(d)), 1 << (i - 1));
    medit_disp_mob_flags(d);
    OLC_VAL(d) = 1;
    return;
    /*-------------------------------------------------------------------*/
  case MEDIT_AFF_FLAGS:
    if ((i = atoi(arg)) == 0)
      break;
    else if (!((i < 0) || (i > NUM_AFF_FLAGS)))
      TOGGLE_BIT(AFF_FLAGS(OLC_MOB(d)), 1 << (i - 1));
    if (IS_SET(AFF_FLAGS(OLC_MOB(d)), AFF_SUPERINV))
      GET_MOB_INVIS_LEV(OLC_MOB(d)) = 51;
    else
      GET_MOB_INVIS_LEV(OLC_MOB(d)) = 0;
    medit_disp_aff_flags(d);
    OLC_VAL(d) = 1;
    return;
    /*-------------------------------------------------------------------*/
  case MEDIT_AFF2_FLAGS:
    if ((i = atoi(arg)) == 0)
      break;
    else if (!((i < 0) || (i > NUM_AFF2_FLAGS)))
      TOGGLE_BIT(AFF2_FLAGS(OLC_MOB(d)), 1 << (i - 1));
    medit_disp_aff2_flags(d);
    OLC_VAL(d) = 1;
    return;
    /*-------------------------------------------------------------------*/
  case MEDIT_ATTACK_MENU: {
    i = atoi(arg);

    if (i == 0)
      medit_disp_menu(d);
    else if (i == d->edit_number2) {
      send_to_char("Insert after which attack [0 if first]:  ", d->character);

      OLC_MODE(d) = MEDIT_ATTACK_NEW;
    } else if (i == d->edit_number2 + 1) {
      send_to_char("Which attack do you want to purge?  ", d->character);
      OLC_MODE(d) = MEDIT_ATTACK_PURGE;
    } else if (i < d->edit_number2) {
      d->edit_number2 = i;
      medit_change_attack(d);
    } else
      medit_disp_menu(d);
  }
    return;
    /*-------------------------------------------------------------------*/
  case MEDIT_ATTACK_NEW: {
    struct mob_attacks_data *temp;
    struct mob_attacks_data *temp2;
    int x = atoi(arg);
    int i;

    if (x < 0 || x >= d->edit_number2)
      medit_disp_attack_menu(d);
    else {
      temp = GET_ATTACKS(OLC_MOB(d));
      CREATE(temp2, struct mob_attacks_data, 1);
      temp2->nodice = 0;
      temp2->sizedice = 0;
      temp2->damroll = 0;
      temp2->attack_type = 0;
      temp2->attacks = 0;
      for (i = 1; i < x; i++) {
        temp = temp->next;
      }
      if (temp) {
        if (x == 0) {
          temp2->next = GET_ATTACKS(OLC_MOB(d));
          GET_ATTACKS(OLC_MOB(d)) = temp2;
          d->edit_number2 = x;
        } else {
          temp2->next = temp->next;
          temp->next = temp2;
          d->edit_number2 = x + 1;
        }
      } else {
        GET_ATTACKS(OLC_MOB(d)) = temp2;
        temp2->next = NULL;
      }
      OLC_VAL(d) = 1;
      medit_change_attack(d);
    }
  }
    return;
    /*-------------------------------------------------------------------*/
  case MEDIT_ATTACK_PURGE: {
    struct mob_attacks_data *attack = GET_ATTACKS(OLC_MOB(d));
    struct mob_attacks_data *temp;
    int i;
    int choice = atoi(arg);

    if (choice < 1 || choice >= d->edit_number2)
      medit_disp_attack_menu(d);
    else {
      for (i = 1; i < choice - 1; i++)
        attack = attack->next;
      if (choice == 1 && attack->next == NULL) {
        FREE(GET_ATTACKS(OLC_MOB(d)));
        GET_ATTACKS(OLC_MOB(d)) = NULL;
      } else if (choice == 1) {
        temp = GET_ATTACKS(OLC_MOB(d));
        GET_ATTACKS(OLC_MOB(d)) = GET_ATTACKS(OLC_MOB(d))->next;
        FREE(temp);
      } else {
        temp = attack->next;
        if (attack->next->next)
          attack->next = attack->next->next;
        else
          attack->next = NULL;
        FREE(temp);
      }
    }
    OLC_VAL(d) = 1;
    medit_disp_attack_menu(d);
  }
    return;
    /*-------------------------------------------------------------------*/
  case MEDIT_CHANGE_ATTACK: {
    i = atoi(arg);

    if (i == 0)
      medit_disp_attack_menu(d);
    else if (i == 1) {
      send_to_char("\r\nEnter speed value:  ", d->character);
      OLC_MODE(d) = MEDIT_ATTACK_SPEED;
    } else if (i == 2) {
      send_to_char("Enter number of dice:  ", d->character);
      OLC_MODE(d) = MEDIT_NDD;
    } else if (i == 3)
      medit_disp_attack_types(d);
    else
      medit_change_attack(d);
  }
    return;
    /*-------------------------------------------------------------------*/
  case MEDIT_ACTION_MENU: {
    i = atoi(arg);

    if (i == 0)
      medit_disp_menu(d);
    else if (i == d->edit_number2) {
      send_to_char("Insert after which action [0 if first]:  ", d->character);

      OLC_MODE(d) = MEDIT_ACTION_NEW;
    } else if (i == d->edit_number2 + 1) {
      send_to_char("Which action do you want to purge?  ", d->character);
      OLC_MODE(d) = MEDIT_ACTION_PURGE;
    } else if (i < d->edit_number2) {
      d->edit_number2 = i;
      medit_change_action(d);
    } else
      medit_disp_menu(d);
  }
    return;
    /*-------------------------------------------------------------------*/
  case MEDIT_ACTION_NEW: {
    struct mob_action_data *temp;
    struct mob_action_data *temp2;
    int x = atoi(arg);
    int i;

    if (x < 0 || x >= d->edit_number2)
      medit_disp_action_menu(d);
    else {
      temp = GET_ACTION(OLC_MOB(d));
      CREATE(temp2, struct mob_action_data, 1);
      temp2->chance = 0;
      temp2->minpos = 0;
      temp2->action = NULL;
      for (i = 1; i < x; i++) {
        temp = temp->next;
      }
      if (temp) {
        if (x == 0) {
          temp2->next = GET_ACTION(OLC_MOB(d));
          GET_ACTION(OLC_MOB(d)) = temp2;
          d->edit_number2 = x;
        } else {
          temp2->next = temp->next;
          temp->next = temp2;
          d->edit_number2 = x + 1;
        }
      } else {
        GET_ACTION(OLC_MOB(d)) = temp2;
        temp2->next = NULL;
      }
      OLC_VAL(d) = 1;
      medit_change_action(d);
    }
  }
    return;
    /*-------------------------------------------------------------------*/
  case MEDIT_ACTION_PURGE: {
    struct mob_action_data *action = GET_ACTION(OLC_MOB(d));
    struct mob_action_data *temp;
    int i;
    int choice = atoi(arg);

    if (choice < 1 || choice >= d->edit_number2)
      medit_disp_action_menu(d);
    else {
      for (i = 1; i < choice - 1; i++)
        action = action->next;
      if (choice == 1 && action->next == NULL) {
        FREE(GET_ACTION(OLC_MOB(d)));
        GET_ACTION(OLC_MOB(d)) = NULL;
      } else if (choice == 1) {
        temp = GET_ACTION(OLC_MOB(d));
        GET_ACTION(OLC_MOB(d)) = GET_ACTION(OLC_MOB(d))->next;
        FREE(temp);
      } else {
        temp = action->next;
        if (action->next->next)
          action->next = action->next->next;
        else
          action->next = NULL;
        FREE(temp);
      }
    }
    OLC_VAL(d) = 1;
    medit_disp_action_menu(d);
  }
    return;
    /*-------------------------------------------------------------------*/
  case MEDIT_CHANGE_ACTION: {
    i = atoi(arg);

    if (i == 1) {
      send_to_char("\r\nEnter new percentage:  ", d->character);
      OLC_MODE(d) = MEDIT_ACTION_PERCENT;
    } else if (i == 2) {
      medit_disp_positions(d);
      OLC_MODE(d) = MEDIT_ACTION_POS;
    } else if (i == 3) {
      send_to_char("Enter new action:  ", d->character);
      OLC_MODE(d) = MEDIT_ACTION_ACTION;
    } else
      medit_disp_action_menu(d);
  }
    return;
    /*-------------------------------------------------------------------*/
  case MEDIT_EQUIP_MENU: {
    i = atoi(arg);

    if (i == 0)
      medit_disp_menu(d);
    else if (i == d->edit_number2) {
      send_to_char("Insert after which equipment [0 if first]:  ", d->character);

      OLC_MODE(d) = MEDIT_EQUIP_NEW;
    } else if (i == d->edit_number2 + 1) {
      send_to_char("Which equipment do you want to purge?  ", d->character);
      OLC_MODE(d) = MEDIT_EQUIP_PURGE;
    } else if (i < d->edit_number2) {
      d->edit_number2 = i;
      medit_change_equip(d);
    } else
      medit_disp_menu(d);
  }
    return;
    /*-------------------------------------------------------------------*/
  case MEDIT_EQUIP_NEW: {
    struct mob_equipment_data *temp;
    struct mob_equipment_data *temp2;
    int x = atoi(arg);
    int i;

    if (x < 0 || x >= d->edit_number2)
      medit_disp_equip_menu(d);
    else {
      temp = GET_EQUIP(OLC_MOB(d));
      CREATE(temp2, struct mob_equipment_data, 1);
      temp2->pos = -1;
      temp2->chance = 0;
      temp2->vnum = -1;
      temp2->max = 0;
      for (i = 1; i < x; i++) {
        temp = temp->next;
      }
      if (temp) {
        if (x == 0) {
          temp2->next = GET_EQUIP(OLC_MOB(d));
          GET_EQUIP(OLC_MOB(d)) = temp2;
          d->edit_number2 = x;
        } else {
          temp2->next = temp->next;
          temp->next = temp2;
          d->edit_number2 = x + 1;
        }
      } else {
        GET_EQUIP(OLC_MOB(d)) = temp2;
        temp2->next = NULL;
      }
      OLC_VAL(d) = 1;
      medit_change_equip(d);
    }
  }
    return;
    /*-------------------------------------------------------------------*/
  case MEDIT_EQUIP_PURGE: {
    struct mob_equipment_data *equip = GET_EQUIP(OLC_MOB(d));
    struct mob_equipment_data *temp;
    int i;
    int choice = atoi(arg);

    if (choice < 1 || choice >= d->edit_number2)
      medit_disp_equip_menu(d);
    else {
      for (i = 1; i < choice - 1; i++)
        equip = equip->next;
      if (choice == 1 && equip->next == NULL) {
        FREE(GET_EQUIP(OLC_MOB(d)));
        GET_EQUIP(OLC_MOB(d)) = NULL;
      } else if (choice == 1) {
        temp = GET_EQUIP(OLC_MOB(d));
        GET_EQUIP(OLC_MOB(d)) = GET_EQUIP(OLC_MOB(d))->next;
        FREE(temp);
      } else {
        temp = equip->next;
        if (equip->next->next)
          equip->next = equip->next->next;
        else
          equip->next = NULL;
        FREE(temp);
      }
    }
    OLC_VAL(d) = 1;
    medit_disp_equip_menu(d);
  }
    return;
    /*-------------------------------------------------------------------*/
  case MEDIT_CHANGE_EQUIP: {
    i = atoi(arg);

    if (i == 1) {
      OLC_MODE(d) = MEDIT_EQUIP_POS;
      medit_disp_equip_pos(d);
    } else if (i == 2) {
      send_to_char("\r\nEnter percent chance load:  ", d->character);
      OLC_MODE(d) = MEDIT_EQUIP_CHANCE;
    } else if (i == 3) {
      send_to_char("Enter object number to load:  ", d->character);
      OLC_MODE(d) = MEDIT_EQUIP_NUMBER;
    } else if (i == 4) {
      send_to_char("Enter max allowed in game [-1 for nolimit]: ", d->character);
      OLC_MODE(d) = MEDIT_EQUIP_MAXLOAD;
    } else
      medit_disp_equip_menu(d);
  }
    return;
    /*-------------------------------------------------------------------*/
  case MEDIT_MPROG:
    if ((i = atoi(arg)) == 0)
      medit_disp_menu(d);
    else if (i == OLC_MTOTAL(d)) {
      struct mob_prog_data *temp;
      CREATE(temp, struct mob_prog_data, 1);
      temp->next = OLC_MPROGL(d);
      temp->type = -1;
      temp->arglist = NULL;
      temp->comlist = NULL;
      OLC_MPROG(d) = temp;
      OLC_MPROGL(d) = temp;
      OLC_MODE(d) = MEDIT_CHANGE_MPROG;
      medit_change_mprog(d);
    } else if (i < OLC_MTOTAL(d)) {
      struct mob_prog_data *temp;
      int x = 1;
      for (temp = OLC_MPROGL(d); temp && x < i; temp = temp->next)
        x++;
      OLC_MPROG(d) = temp;
      OLC_MODE(d) = MEDIT_CHANGE_MPROG;
      medit_change_mprog(d);
    } else if (i == OLC_MTOTAL(d) + 1) {
      send_to_char("Which mob prog do you want to purge? ", d->character);
      OLC_MODE(d) = MEDIT_PURGE_MPROG;
    } else
      medit_disp_menu(d);
    return;

  case MEDIT_PURGE_MPROG:
    if ((i = atoi(arg)) > 0 && i < OLC_MTOTAL(d)) {
      struct mob_prog_data *temp;
      int x = 1;

      for (temp = OLC_MPROGL(d); temp && x < i; temp = temp->next)
        x++;
      OLC_MPROG(d) = temp;
      REMOVE_FROM_LIST(OLC_MPROG(d), OLC_MPROGL(d), next);
      FREE(OLC_MPROG(d)->arglist);
      FREE(OLC_MPROG(d)->comlist);
      FREE(OLC_MPROG(d));
      OLC_MPROG(d) = NULL;
      OLC_VAL(d) = 1;
    }
    medit_disp_mprog(d);
    return;

  case MEDIT_CHANGE_MPROG:
    if ((i = atoi(arg)) == 1)
      medit_disp_mprog_types(d);
    else if (i == 2) {
      send_to_char("Enter new arg list: ", d->character);
      OLC_MODE(d) = MEDIT_MPROG_ARGS;
    } else if (i == 3) {
      send_to_char("Enter new mob prog commands:\r\n", d->character);
      /*
       * Pass control to modify.c for typing.
       */
      OLC_MODE(d) = MEDIT_MPROG_COMLIST;
      d->backstr = NULL;
      if (OLC_MPROG(d)->comlist) {
        SEND_TO_Q(OLC_MPROG(d)->comlist, d);
        d->backstr = strdup(OLC_MPROG(d)->comlist);
      }
      d->str = &OLC_MPROG(d)->comlist;
      d->max_str = MAX_STRING_LENGTH;
      d->mail_to = 0;
      OLC_VAL(d) = 1;
    } else
      medit_disp_mprog(d);
    return;

    /*-------------------------------------------------------------------*/

    /*
     * Numerical responses.
     */

  case MEDIT_CLASS:
    GET_CLASS(OLC_MOB(d)) = BOUNDED(0, atoi(arg), NUM_MOB_CLASSES - 1);
    break;

  case MEDIT_MPROG_TYPE:
    OLC_MPROG(d)->type = (1 << BOUNDED(0, atoi(arg), NUM_PROGS - 1));
    OLC_VAL(d) = 1;
    medit_change_mprog(d);
    return;

  case MEDIT_MPROG_ARGS:
    OLC_MPROG(d)->arglist = strdup(arg);
    delete_doubledollar(OLC_MPROG(d)->arglist);
    OLC_VAL(d) = 1;
    medit_change_mprog(d);
    return;

  case MEDIT_SEX:
    GET_SEX(OLC_MOB(d)) = BOUNDED(0, atoi(arg), NUM_GENDERS - 1);
    break;

  case MEDIT_THAC0:
    GET_HITROLL(OLC_MOB(d)) = BOUNDED(GET_ST_THAC0(GET_LEVEL(OLC_MOB(d))), atoi(arg), 20);
    break;

  case MEDIT_DAMROLL: {
    struct mob_attacks_data *attack = GET_ATTACKS(OLC_MOB(d));
    int i;

    for (i = 1; i < d->edit_number2; i++)
      attack = attack->next;
    attack->damroll = MAX(0, atoi(arg));
  }
    medit_change_attack(d);
    return;

  case MEDIT_NDD: {
    struct mob_attacks_data *attack = GET_ATTACKS(OLC_MOB(d));
    int i;

    for (i = 1; i < d->edit_number2; i++)
      attack = attack->next;

    attack->nodice = MAX(0, atoi(arg));
    send_to_char("Enter number of sides:  ", d->character);
    OLC_MODE(d) = MEDIT_SDD;
  }
    return;

  case MEDIT_SDD: {
    struct mob_attacks_data *attack = GET_ATTACKS(OLC_MOB(d));
    int i;

    for (i = 1; i < d->edit_number2; i++)
      attack = attack->next;

    attack->sizedice = MAX(0, atoi(arg));
    send_to_char("Enter amount of bonus:  ", d->character);
    OLC_MODE(d) = MEDIT_DAMROLL;
  }
    return;

  case MEDIT_NUM_HP_DICE:
    GET_HIT(OLC_MOB(d)) = BOUNDED(0, atoi(arg), GET_ST_HPD(GET_LEVEL(OLC_MOB(d))));
    break;

  case MEDIT_SIZE_HP_DICE:
    GET_MANA(OLC_MOB(d)) = BOUNDED(0, atoi(arg), GET_ST_HPS(GET_LEVEL(OLC_MOB(d))));
    break;

  case MEDIT_ADD_HP:
    GET_MOVE(OLC_MOB(d)) = BOUNDED(0, atoi(arg), GET_ST_HPB(GET_LEVEL(OLC_MOB(d))));
    break;

  case MEDIT_AC:
    GET_AC(OLC_MOB(d)) = BOUNDED(10 * GET_ST_AC(GET_LEVEL(OLC_MOB(d))), atoi(arg), 100);
    break;

  case MEDIT_EXP:
    GET_EXP(OLC_MOB(d)) = BOUNDED(0, atoi(arg), GET_ST_EXP(GET_LEVEL(OLC_MOB(d))));
    break;

  case MEDIT_GOLD:
    GET_TEMP_GOLD(OLC_MOB(d)) = BOUNDED(0, atoi(arg), GET_ST_GOLD(GET_LEVEL(OLC_MOB(d))));
    break;

  case MEDIT_POS:
    GET_POS(OLC_MOB(d)) = BOUNDED(0, atoi(arg), NUM_POSITIONS - 1);
    break;

  case MEDIT_DEFAULT_POS:
    GET_DEFAULT_POS(OLC_MOB(d)) = BOUNDED(0, atoi(arg), NUM_POSITIONS - 1);
    break;

  case MEDIT_RACE:
    GET_MOB_RACE(OLC_MOB(d)) = BOUNDED(0, atoi(arg), NUM_NPC_RACES - 1);
    break;

  case MEDIT_SIZE:
    GET_MOB_SIZE(OLC_MOB(d)) = BOUNDED(0, atoi(arg), NUM_SIZES - 1);
    break;

  case MEDIT_QUESTNUM:
    GET_MOB_QUEST_NUM(OLC_MOB(d)) = MAX(-1, atoi(arg));
    if (GET_MOB_QUEST_NUM(OLC_MOB(d)) == 0) {
      GET_MOB_QUEST_NUM(OLC_MOB(d)) = -1;
    }
    break;

  case MEDIT_ATTACK: {
    struct mob_attacks_data *attack = GET_ATTACKS(OLC_MOB(d));
    int i;

    for (i = 1; i < d->edit_number2; i++)
      attack = attack->next;

    attack->attack_type = BOUNDED(0, atoi(arg), NUM_ATTACK_TYPES - 1);
    OLC_VAL(d) = 1;
    medit_change_attack(d);
  }
    return;

  case MEDIT_ATTACK_SPEED: {
    struct mob_attacks_data *attack = GET_ATTACKS(OLC_MOB(d));
    int i;

    for (i = 1; i < d->edit_number2; i++)
      attack = attack->next;

    attack->attacks = MAX(0, atoi(arg));
    OLC_VAL(d) = 1;
    medit_change_attack(d);
  }
    return;

  case MEDIT_LEVEL:
    GET_LEVEL(OLC_MOB(d)) = BOUNDED(1, atoi(arg), 60);
    GET_HITROLL(OLC_MOB(d)) = GET_ST_THAC0(GET_LEVEL(OLC_MOB(d)));
    GET_AC(OLC_MOB(d)) = 10 * GET_ST_AC(GET_LEVEL(OLC_MOB(d)));
    GET_EXP(OLC_MOB(d)) = GET_ST_EXP(GET_LEVEL(OLC_MOB(d)));
    GET_HIT(OLC_MOB(d)) = GET_ST_HPD(GET_LEVEL(OLC_MOB(d)));
    GET_MANA(OLC_MOB(d)) = GET_ST_HPS(GET_LEVEL(OLC_MOB(d)));
    GET_MOVE(OLC_MOB(d)) = GET_ST_HPB(GET_LEVEL(OLC_MOB(d)));
    break;

  case MEDIT_ALIGNMENT:
    GET_ALIGNMENT(OLC_MOB(d)) = BOUNDED(-1000, atoi(arg), 1000);
    break;

  case MEDIT_CHANGE_STR:
    GET_STR(OLC_MOB(d)) = BOUNDED(-100, atoi(arg), 100);
    medit_disp_e_spec(d);
    return;

  case MEDIT_CHANGE_ADD:
    /*
     GET_ADD(OLC_MOB(d)) = BOUNDED(-100, atoi(arg), 100);
     */
    medit_disp_e_spec(d);
    return;

  case MEDIT_CHANGE_DEX:
    GET_DEX(OLC_MOB(d)) = BOUNDED(-100, atoi(arg), 100);
    medit_disp_e_spec(d);
    return;

  case MEDIT_CHANGE_AGI:
    GET_AGI(OLC_MOB(d)) = BOUNDED(-100, atoi(arg), 100);
    medit_disp_e_spec(d);
    return;

  case MEDIT_CHANGE_INT:
    GET_INT(OLC_MOB(d)) = BOUNDED(-100, atoi(arg), 100);
    medit_disp_e_spec(d);
    return;

  case MEDIT_CHANGE_WIS:
    GET_WIS(OLC_MOB(d)) = BOUNDED(-100, atoi(arg), 100);
    medit_disp_e_spec(d);
    return;

  case MEDIT_CHANGE_CON:
    GET_CON(OLC_MOB(d)) = BOUNDED(-100, atoi(arg), 100);
    medit_disp_e_spec(d);
    return;

  case MEDIT_E_SPEC: {
    int i = atoi(arg);

    if (i == 0) {
      medit_disp_menu(d);
      return;
    } else if (i == 1) {
      send_to_char("Enter value for Str: ", d->character);
      OLC_VAL(d) = 1;
      OLC_MODE(d) = MEDIT_CHANGE_STR;
      return;
    } else if (i == 2) {
      send_to_char("Enter value for Dex: ", d->character);
      OLC_VAL(d) = 1;
      OLC_MODE(d) = MEDIT_CHANGE_DEX;
      return;
    } else if (i == 3) {
      send_to_char("Enter value for Agi: ", d->character);
      OLC_VAL(d) = 1;
      OLC_MODE(d) = MEDIT_CHANGE_DEX;
      return;
    } else if (i == 4) {
      send_to_char("Enter value for Int: ", d->character);
      OLC_VAL(d) = 1;
      OLC_MODE(d) = MEDIT_CHANGE_INT;
      return;
    } else if (i == 5) {
      send_to_char("Enter value for Wis: ", d->character);
      OLC_VAL(d) = 1;
      OLC_MODE(d) = MEDIT_CHANGE_WIS;
      return;
    } else if (i == 6) {
      send_to_char("Enter value for Con: ", d->character);
      OLC_VAL(d) = 1;
      OLC_MODE(d) = MEDIT_CHANGE_CON;
      return;
    } else
      medit_disp_e_spec(d);
    return;
  }

  case MEDIT_ACTION_PERCENT: {
    struct mob_action_data *action = GET_ACTION(OLC_MOB(d));
    int i;

    for (i = 1; i < d->edit_number2; i++)
      action = action->next;

    action->chance = BOUNDED(0, atoi(arg), 100);
    OLC_VAL(d) = 1;
    medit_change_action(d);
  }
    return;

  case MEDIT_ACTION_POS: {
    struct mob_action_data *action = GET_ACTION(OLC_MOB(d));
    int i;

    for (i = 1; i < d->edit_number2; i++)
      action = action->next;

    action->minpos = BOUNDED(0, atoi(arg), NUM_POSITIONS - 1);
    OLC_VAL(d) = 1;
    medit_change_action(d);
  }
    return;

  case MEDIT_ACTION_ACTION: {
    struct mob_action_data *action = GET_ACTION(OLC_MOB(d));
    int i;

    for (i = 1; i < d->edit_number2; i++)
      action = action->next;

    action->action = strdup(arg);
    delete_doubledollar(action->action);
    OLC_VAL(d) = 1;
    medit_change_action(d);
  }
    return;

  case MEDIT_EQUIP_CHANCE: {
    struct mob_equipment_data *equip = GET_EQUIP(OLC_MOB(d));
    int i;

    for (i = 1; i < d->edit_number2; i++)
      equip = equip->next;

    equip->chance = BOUNDED(0, atoi(arg), 100);
    OLC_VAL(d) = 1;
    medit_change_equip(d);
  }
    return;

  case MEDIT_EQUIP_NUMBER: {
    struct mob_equipment_data *equip = GET_EQUIP(OLC_MOB(d));
    int i;

    for (i = 1; i < d->edit_number2; i++)
      equip = equip->next;

    equip->vnum = atoi(arg);
    equip->rnum = real_object(equip->vnum);
    OLC_VAL(d) = 1;
    medit_change_equip(d);
  }
    return;

  case MEDIT_EQUIP_MAXLOAD: {
    struct mob_equipment_data *equip = GET_EQUIP(OLC_MOB(d));
    int i;

    for (i = 1; i < d->edit_number2; i++)
      equip = equip->next;

    equip->max = MAX(-1, atoi(arg));
    OLC_VAL(d) = 1;
    medit_change_equip(d);
  }
    return;

  case MEDIT_EQUIP_POS: {
    struct mob_equipment_data *equip = GET_EQUIP(OLC_MOB(d));
    int i;

    for (i = 1; i < d->edit_number2; i++)
      equip = equip->next;

    equip->pos = BOUNDED(-1, atoi(arg), NUM_WEARS - 1);
    OLC_VAL(d) = 1;
    medit_change_equip(d);
  }
    return;

  case MEDIT_RESIST:
    OLC_VAL(d) = atoi(arg);
    if (OLC_VAL(d) < 0 || OLC_VAL(d) > NUM_RESISTS) {
      medit_disp_resist_menu(d);
      return;
    }
    if (OLC_VAL(d) == 0)
      break;
    send_to_char("Modifier: ", d->character);
    OLC_MODE(d) = MEDIT_RESIST_MOD;
    return;
  case MEDIT_RESIST_MOD:
    GET_RESIST(OLC_MOB(d), OLC_VAL(d)) = BOUNDED(-10000, atoi(arg), 10000);
    medit_disp_resist_menu(d);
    OLC_MODE(d) = MEDIT_RESIST;
    return;

    /*-------------------------------------------------------------------*/
  default:
    /*
     * We should never get here.
     */
    cleanup_olc(d, CLEANUP_ALL);
    mudlog("SYSERR: OLC: medit_parse(): Reached default case!", 'G', COM_BUILDER, TRUE);
    send_to_char("Oops...\r\n", d->character);
    break;
  }
  /*-------------------------------------------------------------------*/

  /*
   * END OF CASE
   * If we get here, we have probably changed something, and now want to
   * return to main menu.  Use OLC_VAL as a 'has changed' flag
   */

  OLC_VAL(d) = 1;
  medit_disp_menu(d);
}
/*
 * End of medit_parse(), thank god.
 */
