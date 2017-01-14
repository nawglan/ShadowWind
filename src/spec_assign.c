/* ************************************************************************
 *   File: spec_assign.c                                 Part of CircleMUD *
 *  Usage: Functions to assign function pointers to objs/mobs/rooms        *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#include <stdio.h>
#include "structs.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"

extern struct room_data *world;
extern int top_of_world;
extern int mini_mud;
extern struct index_data *mob_index;
extern struct index_data *obj_index;

/* functions to perform assignments */

void ASSIGNMOB(int mob, SPECIAL(fname), int type)
{
  if (real_mobile(mob) >= 0) {
    mob_index[real_mobile(mob)].func = fname;
    mob_index[real_mobile(mob)].spec_type = type;
  } else if (!mini_mud) {
    sprintf(buf, "SYSERR: Attempt to assign spec to non-existant mob #%d", mob);
    stderr_log(buf);
  }
}

void ASSIGNOBJ(int obj, SPECIAL(fname), int type)
{
  if (real_object(obj) >= 0) {
    obj_index[real_object(obj)].func = fname;
    obj_index[real_object(obj)].spec_type = type;
  } else if (!mini_mud) {
    sprintf(buf, "SYSERR: Attempt to assign spec to non-existant obj #%d", obj);
    stderr_log(buf);
  }
}

void ASSIGNROOM(int room, SPECIAL(fname), int type)
{
  if (real_room(room) >= 0) {
    world[real_room(room)].func = fname;
    world[real_room(room)].spec_type = type;
  } else if (!mini_mud) {
    sprintf(buf, "SYSERR: Attempt to assign spec to non-existant rm. #%d", room);
    stderr_log(buf);
  }
}

/* ********************************************************************
 *  Assignments                                                        *
 ******************************************************************** */

/* assign special procedures to mobiles */
void assign_mobiles(void)
{
  SPECIAL(postmaster);
  SPECIAL(cityguard);
  SPECIAL(receptionist);
  SPECIAL(guild_guard);
  SPECIAL(guild);
  SPECIAL(fido);
  SPECIAL(janitor);
  SPECIAL(snake);
  SPECIAL(thief);
  SPECIAL(magic_user);
  SPECIAL(black_soul);
  SPECIAL(slave);
  SPECIAL(troglodyte);
  SPECIAL(dark_being);
  SPECIAL(black_dragon);
  SPECIAL(master_sin);
  /* Unused
   SPECIAL(vice_master);
   SPECIAL(vice_slave);
   SPECIAL(tarbaby);
   SPECIAL(high_priest_of_terror);
   SPECIAL(intelligent);
   SPECIAL(leviathan);
   SPECIAL(serpent);
   SPECIAL(auctioneer);
   SPECIAL(mayor);
   SPECIAL(weapon);
   SPECIAL(cryogenicist);
   */
  SPECIAL(elemental);
  SPECIAL(ticket_vendor);
  SPECIAL(bank);

  /* leave commented out until the castle gets rewritten
   void assign_kings_castle(void);
   stderr_log("Assigning the castle procs.");
   assign_kings_castle();
   */

  /* Zone 30 to 31, Weirvane - Farrot */
  ASSIGNMOB(3140, guild_guard, SPEC_STANDARD);
  ASSIGNMOB(3149, guild_guard, SPEC_STANDARD);
  ASSIGNMOB(3073, guild_guard, SPEC_STANDARD);
  ASSIGNMOB(3087, guild_guard, SPEC_STANDARD);
  ASSIGNMOB(3001, cityguard, SPEC_STANDARD);
  ASSIGNMOB(3008, cityguard, SPEC_STANDARD);
  ASSIGNMOB(3040, cityguard, SPEC_STANDARD);
  ASSIGNMOB(3041, cityguard, SPEC_STANDARD);
  ASSIGNMOB(3041, cityguard, SPEC_STANDARD);
  ASSIGNMOB(3042, cityguard, SPEC_STANDARD);
  ASSIGNMOB(3043, cityguard, SPEC_STANDARD);
  ASSIGNMOB(3057, cityguard, SPEC_STANDARD);
  ASSIGNMOB(3058, cityguard, SPEC_STANDARD);
  ASSIGNMOB(3059, cityguard, SPEC_STANDARD);
  ASSIGNMOB(3060, cityguard, SPEC_STANDARD);
  ASSIGNMOB(3067, cityguard, SPEC_STANDARD);
  ASSIGNMOB(3102, janitor, SPEC_STANDARD);
  ASSIGNMOB(3146, thief, SPEC_STANDARD);
  ASSIGNMOB(3149, guild_guard, SPEC_STANDARD);
  ASSIGNMOB(3140, guild_guard, SPEC_STANDARD);
  ASSIGNMOB(3087, guild_guard, SPEC_STANDARD);
  ASSIGNMOB(3073, guild_guard, SPEC_STANDARD);
  ASSIGNMOB(3005, receptionist, SPEC_STANDARD);
  ASSIGNMOB(3010, postmaster, SPEC_STANDARD);
  ASSIGNMOB(3020, guild, SPEC_STANDARD);
  ASSIGNMOB(3021, guild, SPEC_STANDARD);
  ASSIGNMOB(3022, guild, SPEC_STANDARD);
  ASSIGNMOB(3023, guild, SPEC_STANDARD);
  ASSIGNMOB(3019, guild_guard, SPEC_STANDARD);
  /* Hope */
  ASSIGNMOB(5313, bank, SPEC_STANDARD);
  ASSIGNMOB(5319, receptionist, SPEC_STANDARD);
  ASSIGNMOB(5999, guild_guard, SPEC_STANDARD);

  /* Port Halyon */

  /*
   ASSIGNMOB(12018, cityguard, SPEC_STANDARD);
   ASSIGNMOB(12021, cityguard, SPEC_STANDARD);
   */
  ASSIGNMOB(12009, magic_user, SPEC_STANDARD);
  ASSIGNMOB(12025, magic_user, SPEC_STANDARD);
  ASSIGNMOB(12020, magic_user, SPEC_STANDARD);
  ASSIGNMOB(12030, magic_user, SPEC_STANDARD);
  ASSIGNMOB(12039, guild_guard, SPEC_STANDARD);
  ASSIGNMOB(12042, receptionist, SPEC_STANDARD);

  /* Mountain Pass */
  ASSIGNMOB(5206, fido, SPEC_STANDARD);

  /* Elven village Inn*/
  ASSIGNMOB(14309, receptionist, SPEC_STANDARD);

  /* Caryllion's Inn */
  ASSIGNMOB(3994, receptionist, SPEC_STANDARD);

  /* Brethrenmere Inn - Zone 206 */
  ASSIGNMOB(20656, receptionist, SPEC_STANDARD);

  /* Ocam's Reach Inn */
  ASSIGNMOB(14201, receptionist, SPEC_STANDARD);

  /* Zone 34, The Ferry */
  ASSIGNMOB(3410, guild_guard, SPEC_STANDARD);

  /* Zone 59, Caryllion Docks */
  ASSIGNMOB(5999, guild_guard, SPEC_STANDARD);

  /* Zone 37, Caryllion */
  ASSIGNMOB(3700, guild_guard, SPEC_STANDARD);
  ASSIGNMOB(3412, guild_guard, SPEC_STANDARD);
  ASSIGNMOB(3413, guild_guard, SPEC_STANDARD);
  ASSIGNMOB(3994, receptionist, SPEC_STANDARD);
  ASSIGNMOB(3990, guild_guard, SPEC_STANDARD);
  ASSIGNMOB(3991, guild_guard, SPEC_STANDARD);
  ASSIGNMOB(3992, guild_guard, SPEC_STANDARD);
  ASSIGNMOB(3993, guild_guard, SPEC_STANDARD);
  ASSIGNMOB(3995, guild, SPEC_STANDARD);
  ASSIGNMOB(3996, guild, SPEC_STANDARD);
  ASSIGNMOB(3997, guild, SPEC_STANDARD);
  ASSIGNMOB(3998, guild, SPEC_STANDARD);
  ASSIGNMOB(3999, guild, SPEC_STANDARD);

  /* EDEN */
  ASSIGNMOB(1500, cityguard, SPEC_STANDARD);
  ASSIGNMOB(1505, snake, SPEC_STANDARD);
  ASSIGNMOB(1506, cityguard, SPEC_STANDARD);
  ASSIGNMOB(1507, cityguard, SPEC_STANDARD);
  ASSIGNMOB(1508, cityguard, SPEC_STANDARD);
  ASSIGNMOB(1509, cityguard, SPEC_STANDARD);
  ASSIGNMOB(1510, cityguard, SPEC_STANDARD);
  ASSIGNMOB(1511, cityguard, SPEC_STANDARD);
  ASSIGNMOB(1512, cityguard, SPEC_STANDARD);
  ASSIGNMOB(1513, cityguard, SPEC_STANDARD);
  ASSIGNMOB(1514, cityguard, SPEC_STANDARD);
  ASSIGNMOB(1515, cityguard, SPEC_STANDARD);
  ASSIGNMOB(1516, cityguard, SPEC_STANDARD);
  ASSIGNMOB(1517, cityguard, SPEC_STANDARD);
  ASSIGNMOB(1518, cityguard, SPEC_STANDARD);
  ASSIGNMOB(1519, cityguard, SPEC_STANDARD);
  ASSIGNMOB(1520, cityguard, SPEC_STANDARD);

  /* Elementals */
  ASSIGNMOB(1201, elemental, SPEC_HEARTBEAT);
  ASSIGNMOB(1202, elemental, SPEC_HEARTBEAT);
  ASSIGNMOB(1203, elemental, SPEC_HEARTBEAT);
  ASSIGNMOB(1204, elemental, SPEC_HEARTBEAT);

  /* Animated Corpses */
  ASSIGNMOB(1205, elemental, SPEC_HEARTBEAT);
  ASSIGNMOB(1206, elemental, SPEC_HEARTBEAT);
  ASSIGNMOB(1207, elemental, SPEC_HEARTBEAT);
  ASSIGNMOB(1208, elemental, SPEC_HEARTBEAT);
  ASSIGNMOB(1209, elemental, SPEC_HEARTBEAT);

  /* MORIA */
  ASSIGNMOB(4000, snake, SPEC_STANDARD);
  ASSIGNMOB(4001, snake, SPEC_STANDARD);
  ASSIGNMOB(4053, snake, SPEC_STANDARD);
  ASSIGNMOB(4103, thief, SPEC_STANDARD);
  ASSIGNMOB(4100, magic_user, SPEC_STANDARD);
  ASSIGNMOB(4102, snake, SPEC_STANDARD);

  /* DROW */
  ASSIGNMOB(5104, magic_user, SPEC_STANDARD);
  ASSIGNMOB(5107, magic_user, SPEC_STANDARD);
  ASSIGNMOB(5108, magic_user, SPEC_STANDARD);
  ASSIGNMOB(5103, magic_user, SPEC_STANDARD);

  /* RANDS TOWER */
  ASSIGNMOB(9803, magic_user, SPEC_STANDARD);
  ASSIGNMOB(9804, magic_user, SPEC_STANDARD);

  /* Zone 47 -- Freak Show */
  ASSIGNMOB(4701, ticket_vendor, SPEC_STANDARD);

  /* TUNNELS OF SIN */
  ASSIGNMOB(5801, black_dragon, SPEC_STANDARD);
  ASSIGNMOB(5807, dark_being, SPEC_STANDARD);
  ASSIGNMOB(5811, troglodyte, SPEC_STANDARD);
  ASSIGNMOB(5808, slave, SPEC_STANDARD);
  ASSIGNMOB(5823, black_soul, SPEC_STANDARD);
  ASSIGNMOB(5827, master_sin, SPEC_STANDARD);

  /*
   *** ASSIGNMOB's below here are all removed from the game because
   the zone is not in use anymore or the mob nolonger exists. *** 
   */

  /*
   ASSIGNMOB(3071, auctioneer, SPEC_STANDARD);  
   */

  /* SEWERS */
  /*
   ASSIGNMOB(7006, snake, SPEC_STANDARD);
   ASSIGNMOB(7200, magic_user, SPEC_STANDARD);
   ASSIGNMOB(7201, magic_user, SPEC_STANDARD);
   ASSIGNMOB(7240, magic_user, SPEC_STANDARD);
   */

  /* FOREST */
  /*
   ASSIGNMOB(6113, snake, SPEC_STANDARD);
   ASSIGNMOB(6115, magic_user, SPEC_STANDARD);
   ASSIGNMOB(6112, magic_user, SPEC_STANDARD);
   ASSIGNMOB(6114, magic_user, SPEC_STANDARD);
   ASSIGNMOB(6116, magic_user, SPEC_STANDARD); 
   */

  /* ARACHNOS */
  /*
   ASSIGNMOB(6312, magic_user, SPEC_STANDARD);
   ASSIGNMOB(6314, magic_user, SPEC_STANDARD);
   ASSIGNMOB(6315, magic_user, SPEC_STANDARD);
   ASSIGNMOB(6309, magic_user, SPEC_STANDARD);
   ASSIGNMOB(6302, magic_user, SPEC_STANDARD); 
   */

  /* Thalos */
  /*
   ASSIGNMOB(5014, magic_user, SPEC_STANDARD);
   ASSIGNMOB(5010, magic_user, SPEC_STANDARD);
   ASSIGNMOB(5200, magic_user, SPEC_STANDARD);
   ASSIGNMOB(5201, magic_user, SPEC_STANDARD);
   ASSIGNMOB(5004, magic_user, SPEC_STANDARD);
   ASSIGNMOB(5352, magic_user, SPEC_STANDARD); 
   */

  /* DWARVEN KINGDOM */
  /*
   ASSIGNMOB(6502, magic_user, SPEC_STANDARD);
   ASSIGNMOB(6516, magic_user, SPEC_STANDARD);
   ASSIGNMOB(6509, magic_user, SPEC_STANDARD);
   ASSIGNMOB(6500, cityguard, SPEC_STANDARD); 
   */

  /* OCEANIA */
  /*
   ASSIGNMOB(9501, serpent, SPEC_STANDARD);
   ASSIGNMOB(9503, leviathan, SPEC_STANDARD); 
   */

  /* GHENNA */
  /*
   ASSIGNMOB(9913, magic_user, SPEC_STANDARD);
   ASSIGNMOB(9914, magic_user, SPEC_STANDARD);  
   ASSIGNMOB(9923, magic_user, SPEC_STANDARD);
   ASSIGNMOB(9909, magic_user, SPEC_STANDARD);
   ASSIGNMOB(9910, magic_user, SPEC_STANDARD);
   ASSIGNMOB(9908, magic_user, SPEC_STANDARD);
   ASSIGNMOB(9907, magic_user, SPEC_STANDARD); 
   */

  /* TRADE ROAD */
  /* 
   ASSIGNMOB(9711, cityguard, SPEC_STANDARD); Taken out by Farrot 
   */

  /* New Dwarven zone */
  /*
   ASSIGNMOB(13024, receptionist, SPEC_STANDARD); 
   */

  /* ASTRAL AREA */
  /*
   ASSIGNMOB(18502, magic_user, SPEC_STANDARD);
   ASSIGNMOB(18505, thief, SPEC_STANDARD);
   ASSIGNMOB(18510, magic_user, SPEC_STANDARD);
   ASSIGNMOB(18511, magic_user, SPEC_STANDARD);
   ASSIGNMOB(18512, magic_user, SPEC_STANDARD);
   ASSIGNMOB(18517, magic_user, SPEC_STANDARD);
   ASSIGNMOB(18518, magic_user, SPEC_STANDARD);
   */

  /* PYRAMID AREA */
  /*
   ASSIGNMOB(18600, snake, SPEC_STANDARD);
   ASSIGNMOB(18601, snake, SPEC_STANDARD);
   ASSIGNMOB(18604, thief, SPEC_STANDARD);
   ASSIGNMOB(18605, thief, SPEC_STANDARD);
   ASSIGNMOB(18611, magic_user, SPEC_STANDARD);
   */

  /* Vice Island */
  /*
   ASSIGNMOB(12101, vice_slave, SPEC_STANDARD);
   ASSIGNMOB(12102, vice_slave, SPEC_STANDARD);
   ASSIGNMOB(12103, vice_slave, SPEC_STANDARD);
   ASSIGNMOB(12104, vice_slave, SPEC_STANDARD);
   ASSIGNMOB(12105, vice_slave, SPEC_STANDARD);
   ASSIGNMOB(12106, vice_master, SPEC_STANDARD);
   ASSIGNMOB(12110, magic_user, SPEC_STANDARD);
   ASSIGNMOB(12114, high_priest_of_terror, SPEC_STANDARD);
   ASSIGNMOB(12119, magic_user, SPEC_STANDARD); 
   */

  /* High Tower Of Sorcery */
  /*
   ASSIGNMOB(2501, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2504, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2507, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2508, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2510, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2511, thief, SPEC_STANDARD);
   ASSIGNMOB(2514, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2515, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2516, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2517, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2518, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2520, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2521, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2522, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2523, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2524, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2525, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2526, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2527, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2528, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2529, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2530, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2531, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2532, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2533, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2534, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2536, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2537, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2538, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2540, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2541, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2548, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2549, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2552, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2553, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2554, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2556, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2557, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2559, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2560, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2562, magic_user, SPEC_STANDARD);
   ASSIGNMOB(2564, magic_user, SPEC_STANDARD); */

  /* Intelligent mobiles */
  /*   ASSIGNMOB(1220, magic_user, SPEC_HEARTBEAT | SPEC_COMMAND); removed by TIG */

  /*  ASSIGNMOB(5001, guild_guard, SPEC_STANDARD); *//* brass dragon */
  /*  ASSIGNMOB(4061, cityguard, SPEC_STANDARD); */
  /* a dog?!  ASSIGNMOB(3062, guild_guard, SPEC_STANDARD); */

}

/* assign special procedures to objects */
void assign_objects(void)
{
  SPECIAL(bank);
  SPECIAL(gen_board);
  SPECIAL(tunnel_key);
  SPECIAL(portal);
  SPECIAL(statue_39);
  SPECIAL(wraithform_corpse);
  /* Unused
   SPECIAL(statue_of_autumn);
   SPECIAL(slot_machine);
   SPECIAL(weep_blade);
   SPECIAL(icecube);
   SPECIAL(toggle_qa);
   */

  /* Caryllion's Underground */
  ASSIGNOBJ(3900, statue_39, SPEC_STANDARD);

  /* Godzone Objects */
  ASSIGNOBJ(1291, gen_board, SPEC_STANDARD); /* Market Board */
  ASSIGNOBJ(1292, gen_board, SPEC_STANDARD); /* Board of creation */
  ASSIGNOBJ(1293, gen_board, SPEC_STANDARD); /* Code board */
  ASSIGNOBJ(1294, gen_board, SPEC_STANDARD); /* Worldwide board */
  ASSIGNOBJ(1295, gen_board, SPEC_STANDARD); /* Mortal board */
  ASSIGNOBJ(1296, gen_board, SPEC_STANDARD); /* Freeze board */
  ASSIGNOBJ(1297, gen_board, SPEC_STANDARD); /* Immortal board */
  ASSIGNOBJ(1298, gen_board, SPEC_STANDARD); /* God board */
  ASSIGNOBJ(1299, gen_board, SPEC_STANDARD); /* Quest board */
  ASSIGNOBJ(1203, wraithform_corpse, SPEC_STANDARD); /* corpses */
  ASSIGNOBJ(1287, portal, SPEC_STANDARD); /* portal */
  ASSIGNOBJ(1280, portal, SPEC_STANDARD); /* portal */
  ASSIGNOBJ(1281, portal, SPEC_STANDARD); /* portal */
  ASSIGNOBJ(1285, portal, SPEC_STANDARD); /* portal */
  ASSIGNOBJ(1286, portal, SPEC_STANDARD); /* portal */

  /* Bank teller */
  ASSIGNOBJ(3034, bank, SPEC_STANDARD); /* atm */

  /* Sin Objects */
  ASSIGNOBJ(5802, tunnel_key, SPEC_STANDARD);
  ASSIGNOBJ(5806, tunnel_key, SPEC_STANDARD);
  ASSIGNOBJ(5805, tunnel_key, SPEC_STANDARD);
  ASSIGNOBJ(5801, tunnel_key, SPEC_STANDARD);
  ASSIGNOBJ(5813, tunnel_key, SPEC_STANDARD);
  ASSIGNOBJ(5819, tunnel_key, SPEC_STANDARD);
  ASSIGNOBJ(5804, tunnel_key, SPEC_STANDARD);

  /*
   *** ASSIGNOBJ's below here are all removed from the game because
   the zone is not in use anymore or the mob nolonger exists. *** 
   */

  /*
   ASSIGNOBJ(1225, toggle_qa, SPEC_STANDARD);*//* toggle PLR_QA flag */

  /*
   ASSIGNOBJ(205, icecube, SPEC_HEARTBEAT);*//* the wonderful icecube :) */

  /*
   ASSIGNOBJ(16005, weep_blade, SPEC_HEARTBEAT);*//* a blade that bursts into tears */

  /*  Autumns Retreat
   ASSIGNOBJ(4228, statue_of_autumn, SPEC_STANDARD);
   */

}

/* assign special procedures to rooms */
void assign_rooms(void)
{
  extern int dts_are_dumps;
  int i;
  SPECIAL(dump);
  SPECIAL(arctic);
  SPECIAL(mist);
  SPECIAL(gargoyle);
  SPECIAL(mist_room);
  SPECIAL(pentagram);
  /* Unused
   SPECIAL(pray);
   SPECIAL(pray_for_items);
   SPECIAL(pet_shops);
   */

  /* Rooms in Mahn-Tor's zone */
  ASSIGNROOM(2251, arctic, SPEC_STANDARD);
  ASSIGNROOM(2252, arctic, SPEC_STANDARD);
  ASSIGNROOM(2254, arctic, SPEC_STANDARD);
  ASSIGNROOM(2255, arctic, SPEC_STANDARD);
  ASSIGNROOM(2256, arctic, SPEC_STANDARD);
  ASSIGNROOM(2257, arctic, SPEC_STANDARD);
  ASSIGNROOM(2258, arctic, SPEC_STANDARD);
  ASSIGNROOM(2259, arctic, SPEC_STANDARD);
  ASSIGNROOM(2232, mist, SPEC_STANDARD);
  ASSIGNROOM(2233, mist, SPEC_STANDARD);
  ASSIGNROOM(2234, mist, SPEC_STANDARD);
  ASSIGNROOM(2235, mist, SPEC_STANDARD);
  ASSIGNROOM(2236, mist, SPEC_STANDARD);
  ASSIGNROOM(2237, mist, SPEC_STANDARD);

  /* Sin Rooms */
  ASSIGNROOM(5804, mist_room, SPEC_STANDARD);
  ASSIGNROOM(5805, mist_room, SPEC_STANDARD);
  ASSIGNROOM(5851, mist_room, SPEC_STANDARD);
  ASSIGNROOM(5877, mist_room, SPEC_STANDARD);
  ASSIGNROOM(5848, mist_room, SPEC_STANDARD);
  ASSIGNROOM(5849, mist_room, SPEC_STANDARD);
  ASSIGNROOM(5838, gargoyle, SPEC_STANDARD);
  ASSIGNROOM(5885, pentagram, SPEC_STANDARD);

  /*  ASSIGNROOM(3054, pray, SPEC_COMMAND); */

  /* wtf is this stuff? does it belong to the roomproc above? Dorga */
  if (dts_are_dumps)
    for (i = 0; i < top_of_world; i++)
      if (IS_SET(ROOM_FLAGS(i), ROOM_DEATH))
        world[i].func = dump;

  /*
   ASSIGNROOM(3030, dump, SPEC_STANDARD);
   */

  /*
   ASSIGNROOM(3031, pet_shops, SPEC_STANDARD); 
   */

}
