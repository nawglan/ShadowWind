#include "boards.h"
#include "comm.h"
#include "db.h"
#include "olc.h"
#include "shop.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*------------------------------------------------------------------------*/
/* external variables */

extern struct obj_data *obj_proto;
extern struct index_data *obj_index;
extern struct obj_data *object_list;
extern int top_of_objt;
extern struct zone_data *zone_table;
extern int top_of_zone_table;
extern struct shop_data *shop_index;             /*. shop.c	.*/
extern int top_shop;                             /*. shop.c	.*/
extern struct attack_hit_type attack_hit_text[]; /*. fight.c 	.*/
extern char *item_types[];
extern char *wear_bits[];
extern char *extra_bits[];
extern char *drinks[];
extern char *apply_types[];
extern char *container_bits[];
extern char *spells[];
extern char *material_types[];
extern struct board_info_type board_info[];
extern struct descriptor_data *descriptor_list; /*. comm.c	.*/
extern int top_of_world;
extern struct room_data *world;
extern struct char_data *mob_proto;
extern char *room_bits[];
extern char *sector_types[];
extern char *exit_bits[];
extern sh_int r_mortal_start_room;
extern sh_int r_immort_start_room;
extern sh_int r_frozen_start_room;
extern sh_int mortal_start_room;
extern sh_int immort_start_room;
extern sh_int frozen_start_room;
extern struct index_data *mob_index;             /*. db.c    	.*/
extern struct char_data *character_list;         /*. db.c    	.*/
extern int top_of_mobt;                          /*. db.c    	.*/
extern struct player_special_data dummy_mob;     /*. db.c    	.*/
extern struct attack_hit_type attack_hit_text[]; /*. fight.c 	.*/
extern char *action_bits[];                      /*. constants.c .*/
extern char *affected_bits[];                    /*. constants.c .*/
extern char *position_types[];                   /*. constants.c .*/
extern char *genders[];                          /*. constants.c .*/
extern char *equip_order[];                      /*. constants.c .*/
extern struct descriptor_data *descriptor_list;  /*. comm.c	.*/

/*------------------------------------------------------------------------*/
/*. Macros .*/

#define S_PRODUCT(s, i)   ((s)->producing[(i)])
#define W_EXIT(room, num) (world[(room)].dir_option[(num)])
#define ZCMD              zone_table[zone].cmd[cmd_no]
#define GET_NDD(mob)      ((mob)->mob_specials.mob_attacks->nodice)
#define GET_SDD(mob)      ((mob)->mob_specials.mob_attacks->sizedice)
#define GET_ALIAS(mob)    ((mob)->player.name)
#define GET_SDESC(mob)    ((mob)->player.short_descr)
#define GET_LDESC(mob)    ((mob)->player.long_descr)
#define GET_DDESC(mob)    ((mob)->player.description)
#define GET_SIZE(mob)     ((mob)->mob_specials.size)
#define GET_ATTACKS(mob)  ((mob)->mob_specials.mob_attacks)
#define GET_EQUIP(mob)    ((mob)->mob_specials.mob_equip)
#define GET_ACTION(mob)   ((mob)->mob_specials.mob_action)
#define GET_ATTACK(mob)   ((mob)->mob_specials.mob_attacks->attack_type)
#define S_KEEPER(shop)    ((shop)->keeper)

/*------------------------------------------------------------------------*/
/* function protos */

void convert_objs_to_disk(int zone_num);
void convert_rooms_to_disk(int zone_num);
void convert_mobs_to_disk(int zone_num);

/*------------------------------------------------------------------------*/

void convert_objs_to_disk(int zone_num) {
  int counter, counter2, realcounter;
  FILE *fp;
  struct obj_data *obj;
  struct extra_descr_data *ex_desc;
  int tmpmod;

  sprintf(buf, "%s/%d.obj", OBJ_PREFIX, zone_table[zone_num].number);
  if (!(fp = fopen(buf, "w+"))) {
    return;
  }

  /* start running through all objects in this zone */
  for (counter = zone_table[zone_num].number * 100; counter <= zone_table[zone_num].top;
       counter++) { /* write object to disk */
    realcounter = real_object(counter);
    if (realcounter >= 0) {
      obj = (obj_proto + realcounter);

      if (obj->action_description) {
        strcpy(buf1, obj->action_description);
        strip_string(buf1);
      } else
        *buf1 = 0;

      fprintf(fp,
              "#%d\n"
              "%s~\n"
              "%s~\n"
              "%s~\n"
              "%s~\n"
              "%d %d %d\n"
              "%d %d %d %d %d\n"
              "%d %d %d\n",

              GET_OBJ_VNUM(obj), obj->name, obj->short_description, obj->description, buf1, GET_OBJ_TYPE(obj),
              GET_OBJ_EXTRA(obj), GET_OBJ_WEAR(obj), GET_OBJ_VAL(obj, 0), GET_OBJ_VAL(obj, 1), GET_OBJ_VAL(obj, 2),
              GET_OBJ_VAL(obj, 3), GET_OBJ_VAL(obj, 4), GET_OBJ_WEIGHT(obj), GET_OBJ_COST(obj), GET_OBJ_RENT(obj)
              /*.     GET_OBJ_LEVEL(obj) -- Level flags for objects .*/
      );

      /* Do we have extra descriptions? */
      if (obj->ex_description) { /*. Yep, save them too .*/
        for (ex_desc = obj->ex_description; ex_desc;
             ex_desc = ex_desc->next) { /*. Sanity check to prevent nasty protection faults .*/
          if (!*ex_desc->keyword || !*ex_desc->description) {
            mudlog("SYSERR: OLC: oedit_save_to_disk: Corrupt ex_desc!", 'G', COM_BUILDER, TRUE);
            continue;
          }
          strcpy(buf1, ex_desc->description);
          strip_string(buf1);
          fprintf(fp,
                  "E\n"
                  "%s~\n"
                  "%s~\n",
                  ex_desc->keyword, buf1);
        }
      }

      /* Do we have affects? */
      for (counter2 = 0; counter2 < MAX_OBJ_AFFECT; counter2++)
        if (obj->affected[counter2].modifier) {
          if ((obj->affected[counter2].location > 0) && (obj->affected[counter2].location < 7))
            tmpmod = obj->affected[counter2].modifier * 5;
          else
            tmpmod = obj->affected[counter2].modifier;
          fprintf(fp,
                  "A\n"
                  "%d %d\n",
                  obj->affected[counter2].location, tmpmod);
        }
    }
  }

  /* write final line, close */
  fprintf(fp, "$~\n");
  fclose(fp);
}

/*------------------------------------------------------------------------*/

void convert_rooms_to_disk(int zone_num) {
  int counter, counter2, realcounter;
  FILE *fp;
  struct room_data *room;
  struct extra_descr_data *ex_desc;

  sprintf(buf, "%s/%d.wld", WLD_PREFIX, zone_table[zone_num].number);
  if (!(fp = fopen(buf, "w+"))) {
    return;
  }

  for (counter = zone_table[zone_num].number * 100; counter <= zone_table[zone_num].top; counter++) {
    realcounter = real_room(counter);
    if (realcounter >= 0) {
      room = (world + realcounter);

      /*. Remove the '\r\n' sequences from description .*/
      strcpy(buf1, room->description);
      strip_string(buf1);

      /*. Build a buffer ready to save .*/
      sprintf(buf, "#%d\n%s~\n%s~\n%d %d %d\n", counter, room->name, buf1, zone_table[room->zone].number,
              room->room_flags, room->sector_type);
      /*. Save this section .*/
      fputs(buf, fp);

      /*. Handle exits .*/
      for (counter2 = 0; counter2 < NUM_OF_DIRS; counter2++) {
        if (room->dir_option[counter2]) {
          int temp_door_flag;

          /*. Again, strip out the crap .*/
          if (room->dir_option[counter2]->general_description) {
            strcpy(buf1, room->dir_option[counter2]->general_description);
            strip_string(buf1);
          } else
            strcpy(buf1, "");

          /*. Figure out door flag .*/
          if (IS_SET(room->dir_option[counter2]->exit_info, EX_ISDOOR)) {
            if (IS_SET(room->dir_option[counter2]->exit_info, EX_PICKPROOF))
              temp_door_flag = 2;
            else
              temp_door_flag = 1;
          } else
            temp_door_flag = 0;

          /*. Check for keywords .*/
          if (room->dir_option[counter2]->keyword)
            strcpy(buf2, room->dir_option[counter2]->keyword);
          else
            strcpy(buf2, "");

          /*. Ok, now build a buffer to output to file .*/
          sprintf(buf, "D%d\n%s~\n%s~\n%d %d %d %d\n", counter2, buf1, buf2, temp_door_flag,
                  room->dir_option[counter2]->key, room->dir_option[counter2]->to_room_vnum,
                  room->dir_option[counter2]->add_move);
          /*. Save this door .*/
          fputs(buf, fp);
        }
      }
      if (room->ex_description) {
        for (ex_desc = room->ex_description; ex_desc;
             ex_desc = ex_desc->next) { /*. Home straight, just deal with extras descriptions..*/
          strcpy(buf1, ex_desc->description);
          strip_string(buf1);
          sprintf(buf, "E\n%s~\n%s~\n", ex_desc->keyword, buf1);
          fputs(buf, fp);
        }
      }
      fprintf(fp, "S\n");
    }
  }
  /* write final line and close */
  fprintf(fp, "$~\n");
  fclose(fp);
}

/*-------------------------------------------------------------------*/
/*. Save ALL mobiles for a zone to their .mob file, mobs are all
 saved in AnotherWorld format, regardless of whether they have any
 extended fields.  Thanks to Samedi for ideas on this bit of code.*/

void convert_mobs_to_disk(int zone_num) {
  int i, rmob_num, zone, top;
  FILE *mob_file;
  char fname[64];
  struct char_data *mob;
  struct mob_attacks_data *attack;
  struct mob_equipment_data *equipment;
  struct mob_action_data *action;
  MPROG_DATA *mob_prog;

  zone = zone_table[zone_num].number;
  top = zone_table[zone_num].top;

  sprintf(fname, "%s/%i.mob", MOB_PREFIX, zone);

  if (!(mob_file = fopen(fname, "w"))) {
    mudlog("SYSERR: OLC: Cannot open mob file!", 'G', COM_BUILDER, TRUE);
    return;
  }

  /*. Seach database for mobs in this zone and save em .*/
  for (i = zone * 100; i <= top; i++) {
    rmob_num = real_mobile(i);

    if (rmob_num != -1) {
      if (fprintf(mob_file, "#%d\n", i) < 0) {
        mudlog("SYSERR: OLC: Cannot write mob file!\r\n", 'G', COM_BUILDER, TRUE);
        fclose(mob_file);
        return;
      }

      mob = (mob_proto + rmob_num);
      attack = GET_ATTACKS(mob);
      equipment = GET_EQUIP(mob);
      action = GET_ACTION(mob);
      mob_prog = mob_index[rmob_num].mobprogs;

      /*. Clean up strings .*/
      strcpy(buf1, GET_LDESC(mob));
      strip_string(buf1);
      if (GET_DDESC(mob)) {
        strcpy(buf2, GET_DDESC(mob));
        strip_string(buf2);
      } else
        strcpy(buf2, "");

      fprintf(mob_file,
              "%s~\n"
              "%s~\n"
              "%s~\n"
              "%s~\n"
              "%ld %ld %i X\n"
              "%d %d %i %dd%d+%d %dd%d+%d\n"
              "%d %d\n" /*. Gold & Exp are longs in my mud, ignore any warning .*/
              "%d %d %d %d %d %d\n",
              GET_ALIAS(mob), GET_SDESC(mob), buf1, buf2, MOB_FLAGS(mob), AFF_FLAGS(mob), GET_ALIGNMENT(mob),
              GET_LEVEL(mob), GET_HITROLL(mob), GET_AC(mob) / 10, GET_HIT(mob), GET_MANA(mob), GET_MOVE(mob),
              GET_NDD(mob), GET_SDD(mob), GET_DAMROLL(mob), GET_GOLD(mob), GET_EXP(mob), GET_POS(mob),
              GET_DEFAULT_POS(mob), GET_SEX(mob), GET_CLASS(mob), GET_RACE(mob), GET_SIZE(mob));

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

      /*. Deal with Extra stats in case they are there .*/
      if ((GET_STR(mob) != 11) && (GET_STR(mob) < 19))
        fprintf(mob_file, "Str: %d\n", (GET_STR(mob) * 5));
      else if ((GET_STR(mob) > 18) && (GET_STR(mob) != 50))
        fprintf(mob_file, "Str: 100\n");
      if ((GET_DEX(mob) != 11) && (GET_DEX(mob) < 19)) {
        fprintf(mob_file, "Dex: %d\n", (GET_DEX(mob) * 5));
        fprintf(mob_file, "Agi: %d\n", (GET_DEX(mob) * 5));
      } else if ((GET_DEX(mob) > 18) && (GET_DEX(mob) != 50)) {
        fprintf(mob_file, "Dex: 100\n");
        fprintf(mob_file, "Agi: 100\n");
      }
      if ((GET_INT(mob) != 11) && (GET_INT(mob) < 19))
        fprintf(mob_file, "Int: %d\n", (GET_INT(mob) * 5));
      else if ((GET_INT(mob) > 18) && (GET_INT(mob) != 50))
        fprintf(mob_file, "Int: 100\n");
      if ((GET_WIS(mob) != 11) && (GET_WIS(mob) < 19))
        fprintf(mob_file, "Wis: %d\n", (GET_WIS(mob) * 5));
      else if ((GET_WIS(mob) > 18) && (GET_WIS(mob) != 50))
        fprintf(mob_file, "Wis: 100\n");
      if ((GET_CON(mob) != 11) && (GET_CON(mob) < 19))
        fprintf(mob_file, "Con: %d\n", (GET_CON(mob) * 5));
      else if ((GET_CON(mob) > 18) && (GET_CON(mob) != 50))
        fprintf(mob_file, "Con: 100\n");

      /*. Deal with Mob Progs .*/
      while (mob_prog) {
        switch (mob_prog->type) {
        case IN_FILE_PROG:
          fprintf(mob_file, ">in_file_prog");
          break;
        case ACT_PROG:
          fprintf(mob_file, ">act_prog");
          break;
        case SPEECH_PROG:
          fprintf(mob_file, ">speech_prog");
          break;
        case RAND_PROG:
          fprintf(mob_file, ">rand_prog");
          break;
        case FIGHT_PROG:
          fprintf(mob_file, ">fight_prog");
          break;
        case HITPRCNT_PROG:
          fprintf(mob_file, ">hitprcnt_prog");
          break;
        case DEATH_PROG:
          fprintf(mob_file, ">death_prog");
          break;
        case ENTRY_PROG:
          fprintf(mob_file, ">entry_prog");
          break;
        case GREET_PROG:
          fprintf(mob_file, ">greet_prog");
          break;
        case ALL_GREET_PROG:
          fprintf(mob_file, ">all_greet_prog");
          break;
        case GIVE_PROG:
          fprintf(mob_file, ">give_prog");
          break;
        case BRIBE_PROG:
          fprintf(mob_file, ">bribe_prog");
          break;
        case SHOUT_PROG:
          fprintf(mob_file, ">shout_prog");
          break;
        case HOLLER_PROG:
          fprintf(mob_file, ">holler_prog");
          break;
        case TELL_PROG:
          fprintf(mob_file, ">tell_prog");
          break;
        case TIME_PROG:
          fprintf(mob_file, ">time_prog");
          break;
        }
        strcpy(buf1, mob_prog->arglist);
        strip_string(buf1);
        strcpy(buf2, mob_prog->comlist);
        strip_string(buf2);
        fprintf(mob_file, " %s~\n%s", buf1, buf2);
        mob_prog = mob_prog->next;
        if (!mob_prog)
          fprintf(mob_file, "~\n|\n");
        else
          fprintf(mob_file, "~\n");
      }
    }
  }
  fclose(mob_file);
}
