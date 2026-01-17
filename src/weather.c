/**************************************************************************
 *  file: weather.c , Weather and time module              Part of DIKUMUD *
 *  Usage: Performing the clock and the weather                            *
 *  Copyright (C) 1990, 1991 - see 'license.doc' for complete information. *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "structs.h"
#include "utils.h"

/* uses */
extern struct time_info_data time_info;
extern struct zone_data *zone_table;
extern int top_of_zone_table;
extern struct char_data *character_list;
extern struct room_data *world;
void send_to_zone_outdoor(int zone, char *message);

/* In this part. */

void weather_and_time(int mode);
void another_hour(int mode);
void weather_change(int mode);
void calc_light_zone(struct zone_data *zone);
char get_season(struct zone_data *zone);
void blow_out_torches();

/* Here comes the code */

void weather_and_time(int mode) {
  another_hour(mode);
  weather_change(mode);
}

void another_hour(int mode) {
  extern int num_hours;
  extern int num_days;
  extern int num_months;

  time_info.hours++;

  if (time_info.hours == num_hours) /* Changed by HHS due to bug ???*/
  {
    time_info.hours -= num_hours;
    time_info.day++;

    if (time_info.day == num_days) {
      time_info.day = 0;
      time_info.month++;

      if (time_info.month == num_months) {
        time_info.month = 0;
        time_info.year++;
      }
    }
  }
}

/* some magic values */
#define MAGIC_PRECIP_START 1060
#define MAGIC_PRECIP_STOP  970

void weather_change(int mode) {
  int number(int from, int to);

  int zon, magic, old_wind;
  signed char old_temp;
  unsigned char old_precip, season_num;
  struct climate_data *clime;
  struct new_weather_data *cond;

  if (mode) {

    /* Scan through each zone of the zone_table */
    for (zon = 0; zon < top_of_zone_table; zon++) {
      clime = &zone_table[zon].climate;
      cond = &zone_table[zon].conditions;
      old_temp = cond->temp;
      old_precip = cond->precip_rate;
      old_wind = cond->windspeed;

      /* Which season is it? */
      season_num = get_season(&zone_table[zon]);

      /* Clear the control weather bit */
      REMOVE_BIT(cond->flags, WEATHER_CONTROLLED);

      /* Create changes for this hour */
      cond->free_energy = MAX(3000, clime->energy_add + cond->free_energy);
      cond->free_energy = MIN(50000, cond->free_energy);
      switch (clime->season_wind[season_num]) {
      case SEASON_CALM:
        if (cond->windspeed > 25)
          cond->windspeed -= 5;
        else
          cond->windspeed += number(-2, 1);
        break;
      case SEASON_BREEZY:
        if (cond->windspeed > 40)
          cond->windspeed -= 5;
        else
          cond->windspeed += number(-2, 2);
        break;
      case SEASON_UNSETTLED:
        if (cond->windspeed < 5)
          cond->windspeed += 5;
        else if (cond->windspeed > 60)
          cond->windspeed -= 5;
        else
          cond->windspeed += number(-6, 6);
        break;
      case SEASON_WINDY:
        if (cond->windspeed < 15)
          cond->windspeed += 5;
        else if (cond->windspeed > 80)
          cond->windspeed -= 5;
        else
          cond->windspeed += number(-6, 6);
        break;
      case SEASON_CHINOOK:
        if (cond->windspeed < 25)
          cond->windspeed += 5;
        else if (cond->windspeed > 110)
          cond->windspeed -= 5;
        else
          cond->windspeed += number(-15, 15);
        break;
      case SEASON_VIOLENT:
        if (cond->windspeed < 40)
          cond->windspeed += 5;
        else
          cond->windspeed += number(-8, 8);
        break;
      case SEASON_HURRICANE:
        cond->windspeed = 100;
        break;
      default:
        break;
      }
      cond->free_energy += cond->windspeed; /* + or - ? */
      if (cond->free_energy < 0)
        cond->free_energy = 0;
      else if (cond->free_energy > 20000)
        cond->windspeed += number(-10, -1);
      cond->windspeed = MAX(0, cond->windspeed);
      switch (clime->season_wind_variance[season_num]) {
      case 0:
        cond->wind_dir = clime->season_wind_dir[season_num];
        break;
      case 1:
        if (dice(2, 15) * 1000 < cond->free_energy)
          cond->wind_dir = number(0, 3);
        break;
      }
      switch (clime->season_temp[season_num]) {
      case SEASON_FROSTBITE:
        if (cond->temp > -20)
          cond->temp -= 4;
        else
          cond->temp += number(-3, 3);
        break;
      case SEASON_NIPPY:
        if (cond->temp < -40)
          cond->temp += 2;
        else if (cond->temp > 5)
          cond->temp -= 3;
        else
          cond->temp += number(-3, 3);
        break;
      case SEASON_FREEZING:
        if (cond->temp < -20)
          cond->temp += 2;
        else if (cond->temp > 0)
          cond->temp -= 2;
        else
          cond->temp += number(-2, 2);
        break;
      case SEASON_COLD:
        if (cond->temp < -10)
          cond->temp += 1;
        else if (cond->temp > 5)
          cond->temp -= 2;
        else
          cond->temp += number(-2, 2);
        break;
      case SEASON_COOL:
        if (cond->temp < -3)
          cond->temp += 2;
        else if (cond->temp > 14)
          cond->temp -= 2;
        else
          cond->temp += number(-3, 3);
        break;
      case SEASON_MILD:
        if (cond->temp < 7)
          cond->temp += 2;
        else if (cond->temp > 26)
          cond->temp -= 2;
        else
          cond->temp += number(-2, 2);
        break;
      case SEASON_WARM:
        if (cond->temp < 19)
          cond->temp += 2;
        else if (cond->temp > 33)
          cond->temp -= 2;
        else
          cond->temp += number(-3, 3);
        break;
      case SEASON_HOT:
        if (cond->temp < 24)
          cond->temp += 3;
        else if (cond->temp > 46)
          cond->temp -= 2;
        else
          cond->temp += number(-3, 3);
        break;
      case SEASON_BLUSTERY:
        if (cond->temp < 34)
          cond->temp += 3;
        else if (cond->temp > 53)
          cond->temp -= 2;
        else
          cond->temp += number(-5, 5);
        break;
      case SEASON_HEATSTROKE:
        if (cond->temp < 44)
          cond->temp += 5;
        else if (cond->temp > 60)
          cond->temp -= 5;
        else
          cond->temp += number(-3, 3);
        break;
      case SEASON_BOILING:
        if (cond->temp < 80)
          cond->temp += 5;
        else if (cond->temp > 120)
          cond->temp -= 5;
        else
          cond->temp += number(-6, 6);
        break;
      default:
        break;
      }
      if (cond->flags & SUN_VISIBLE)
        cond->temp += 2;
      else if (!(clime->flags & NO_SUN_EVER))
        cond->temp -= 2;
      switch (clime->season_precip[season_num]) {
      case SEASON_NO_PRECIP_EVER:
        if (cond->precip_rate > 0)
          cond->precip_rate /= 2;
        cond->humidity = 0;
        break;
      case SEASON_ARID:
        if (cond->humidity > 30)
          cond->humidity -= 3;
        else
          cond->humidity += number(-3, 2);
        if (old_precip > 20)
          cond->precip_rate -= 8;
        break;
      case SEASON_DRY:
        if (cond->humidity > 50)
          cond->humidity -= 3;
        else
          cond->humidity += number(-4, 3);
        if (old_precip > 35)
          cond->precip_rate -= 6;
        break;
      case SEASON_LOW_PRECIP:
        if (cond->humidity < 13)
          cond->humidity += 3;
        else if (cond->humidity > 91)
          cond->humidity -= 2;
        else
          cond->humidity += number(-5, 4);
        if (old_precip > 45)
          cond->precip_rate -= 10;
        break;
      case SEASON_AVG_PRECIP:
        if (cond->humidity < 30)
          cond->humidity += 3;
        else if (cond->humidity > 80)
          cond->humidity -= 2;
        else
          cond->humidity += number(-9, 9);
        if (old_precip > 55)
          cond->precip_rate -= 10;
        break;
      case SEASON_HIGH_PRECIP:
        if (cond->humidity < 40)
          cond->humidity += 3;
        else if (cond->humidity > 90)
          cond->humidity -= 2;
        else
          cond->humidity += number(-8, 8);
        if (old_precip > 65)
          cond->precip_rate -= 10;
        break;
      case SEASON_STORMY:
        if (cond->humidity < 50)
          cond->humidity += 4;
        else
          cond->humidity += number(-6, 6);
        if (old_precip > 80)
          cond->precip_rate -= 10;
        break;
      case SEASON_TORRENT:
        if (cond->humidity < 60)
          cond->humidity += 4;
        else
          cond->humidity += number(-6, 9);
        if (old_precip > 100)
          cond->precip_rate -= 15;
        break;
      case SEASON_CONSTANT_PRECIP:
        cond->humidity = 100;
        if (cond->precip_rate == 0)
          cond->precip_rate = number(5, 12);
        break;
      default:
        break;
      }
      cond->humidity = MIN(100, cond->humidity);
      cond->humidity = MAX(0, cond->humidity);

      cond->pressure_change += number(-3, 3);
      cond->pressure_change = MIN(8, cond->pressure_change);
      cond->pressure_change = MAX(-8, cond->pressure_change);
      cond->pressure += cond->pressure_change;
      cond->pressure = MIN(cond->pressure, 1040);
      cond->pressure = MAX(cond->pressure, 960);

      cond->free_energy += cond->pressure_change;

      /* The numbers that follow are truly magic since  */
      /* they have little bearing on reality and are an */
      /* attempt to create a mix of precipitation which */
      /* will seem reasonable for a specified climate   */
      /* without any complicated formulae that could    */
      /* cause a performance hit. To get more specific  */
      /* or exacting would certainly not be "Diku..."   */

      magic = ((1240 - cond->pressure) * cond->humidity >> 4) + cond->temp + old_precip * 2 +
              (cond->free_energy - 10000) / 100;

      if (old_precip == 0) {
        if (magic > MAGIC_PRECIP_START) {
          cond->precip_rate += 1;
          if (cond->temp > 0)
            send_to_zone_outdoor(zon, "It begins to rain.\n\r");
          else
            send_to_zone_outdoor(zon, "It starts to snow.\n\r");
        } else if (!old_wind && cond->windspeed)
          send_to_zone_outdoor(zon, "The wind begins to blow.\n\r");
        else if (cond->windspeed - old_wind > 10)
          send_to_zone_outdoor(zon, "The wind picks up some.\n\r");
        else if (cond->windspeed - old_wind < -10)
          send_to_zone_outdoor(zon, "The wind calms down a bit.\n\r");
        else if (cond->windspeed > 60) {
          if (cond->temp > 50)
            send_to_zone_outdoor(zon, "A violent scorching wind "
                                      "blows hard in the face "
                                      "of any poor travellers "
                                      "in the area.\n\r");
          else if (cond->temp > 21)
            send_to_zone_outdoor(zon, "A hot wind gusts wildly "
                                      "through the area.\n\r");
          else if (cond->temp > 0)
            send_to_zone_outdoor(zon, "A fierce wind cuts the air "
                                      "like a razor-sharp knife.\n\r");
          else if (cond->temp > -10)
            send_to_zone_outdoor(zon, "A freezing gale blasts "
                                      "through the area.\n\r");
          else
            send_to_zone_outdoor(zon, "An icy wind drains the "
                                      "warmth from all in sight.\n\r");
        } else if (cond->windspeed > 25) {
          if (cond->temp > 50)
            send_to_zone_outdoor(zon, "A hot, dry breeze blows "
                                      "languidly around.\n\r");
          else if (cond->temp > 22)
            send_to_zone_outdoor(zon, "A warm pocket of air "
                                      "is rolling through here.\n\r");
          else if (cond->temp > 10)
            send_to_zone_outdoor(zon, "It's breezy.\n\r");
          else if (cond->temp > 2)
            send_to_zone_outdoor(zon, "A cool breeze wafts by.\n\r");
          else if (cond->temp > -5)
            send_to_zone_outdoor(zon, "A slight wind blows a "
                                      "chill into living tissue.\n\r");
          else if (cond->temp > -15)
            send_to_zone_outdoor(zon, "A freezing wind blows "
                                      "gently, but firmly "
                                      "against all obstacles in "
                                      "the area.\n\r");
          else
            send_to_zone_outdoor(zon, "The wind isn't very strong "
                                      "here, but the cold makes "
                                      "it quite noticeable.\n\r");
        } else if (cond->temp > 52)
          send_to_zone_outdoor(zon, "It's hotter than anyone could imagine.\n\r");
        else if (cond->temp > 37)
          send_to_zone_outdoor(zon, "It's really, really hot here. A "
                                    "slight breeze would really "
                                    "improve things.\n\r");
        else if (cond->temp > 25)
          send_to_zone_outdoor(zon, "It's hot out here.\n\r");
        else if (cond->temp > 19)
          send_to_zone_outdoor(zon, "It's nice and warm out.\n\r");
        else if (cond->temp > 9)
          send_to_zone_outdoor(zon, "It's mild out today.\n\r");
        else if (cond->temp > 1)
          send_to_zone_outdoor(zon, "It's cool out here.\n\r");
        else if (cond->temp > -5)
          send_to_zone_outdoor(zon, "It's a bit nippy here.\n\r");
        else if (cond->temp > -20)
          send_to_zone_outdoor(zon, "It's cold!\n\r");
        else if (cond->temp > -25)
          send_to_zone_outdoor(zon, "It's really c-c-c-cold!!\n\r");
        else
          send_to_zone_outdoor(zon, "Better get inside - this is too "
                                    "cold for man or -most- beasts.\n\r");
      } else if (magic < MAGIC_PRECIP_STOP) {
        cond->precip_rate = 0;
        if (old_temp > 0)
          send_to_zone_outdoor(zon, "The rain stops.\n\r");
        else
          send_to_zone_outdoor(zon, "It stops snowing.\n\r");
      } else {
        /* Still precip'ing, update the rate */
        /* Check rain->snow or snow->rain */
        if (cond->free_energy > 10000)
          cond->precip_change += number(-3, 4);
        else
          cond->precip_change += number(-4, 2);
        cond->precip_change = MAX(-10, cond->precip_change);
        cond->precip_change = MIN(10, cond->precip_change);
        cond->precip_rate += cond->precip_change;
        cond->precip_rate = MAX(1, cond->precip_rate);
        cond->precip_rate = MIN(100, cond->precip_rate);

        cond->free_energy -= cond->precip_rate * 3 - abs(cond->precip_change);

        if (old_temp > 0 && cond->temp <= 0)
          send_to_zone_outdoor(zon, "The rain turns to snow.\n\r");
        else if (old_temp <= 0 && cond->temp > 0)
          send_to_zone_outdoor(zon, "The snow turns to a cold rain.\n\r");
        else if (cond->precip_change > 5) {
          if (cond->temp > 0)
            send_to_zone_outdoor(zon, "It rains a bit harder.\n\r");
          else
            send_to_zone_outdoor(zon, "The snow is coming down "
                                      "faster now.\n\r");
        } else if (cond->precip_change < -5) {
          if (cond->temp > 0)
            send_to_zone_outdoor(zon, "The rain is falling less "
                                      "heavily now.\n\r");
          else
            send_to_zone_outdoor(zon, "The snow has let up a "
                                      "little.\n\r");
        } else if (cond->temp > 0) {
          if (cond->precip_rate > 80) {
            if (cond->windspeed > 80)
              send_to_zone_outdoor(zon, "There's a hurricane "
                                        "out here!\n\r");
            else if (cond->windspeed > 40)
              send_to_zone_outdoor(zon, "The wind and the rain"
                                        " are nearly too much "
                                        "to handle.\n\r");
            else
              send_to_zone_outdoor(zon, "It's raining really "
                                        "hard right now.\n\r");
          } else if (cond->precip_rate > 50) {
            if (cond->windspeed > 60) {
              send_to_zone_outdoor(zon, "What a rainstorm!\n\r");
            } else if (cond->windspeed > 30)
              send_to_zone_outdoor(zon, "The wind is lashing "
                                        "this wild rain seemi"
                                        "ngly straight into y"
                                        "our face.\n\r");
            else
              send_to_zone_outdoor(zon, "It's raining pretty "
                                        "hard.\n\r");
          } else if (cond->precip_rate > 30) {
            if (cond->windspeed > 50)
              send_to_zone_outdoor(zon, "A respectable rain "
                                        "is being thrashed "
                                        "about by a vicious "
                                        "wind.\n\r");
            else if (cond->windspeed > 25) {
              send_to_zone_outdoor(zon, "It's rainy and windy "
                                        "but, altogether not "
                                        "too uncomfortable.\n\r");
            } else
              send_to_zone_outdoor(zon, "Hey, it's raining...\n\r");
          } else if (cond->precip_rate > 10) {
            if (cond->windspeed > 50)
              send_to_zone_outdoor(zon, "The light rain here "
                                        "is nearly unnoticeab"
                                        "le compared to the h"
                                        "orrendous wind.\n\r");
            else if (cond->windspeed > 24)
              send_to_zone_outdoor(zon, "A light rain is bei"
                                        "ng driven fiercely "
                                        "by the wind.\n\r");
            else
              send_to_zone_outdoor(zon, "It's raining lightly.\n\r");
          } else if (cond->windspeed > 55)
            send_to_zone_outdoor(zon, "A few drops of rain are fall"
                                      "ing admidst a fierce windsto"
                                      "rm.\n\r");
          else if (cond->windspeed > 30)
            send_to_zone_outdoor(zon, "The wind and a bit of rain "
                                      "hint at the possibility of "
                                      "a storm.\n\r");
          else
            send_to_zone_outdoor(zon, "A light drizzle is falling "
                                      "here.\n\r");
        } else if (cond->precip_rate > 70) {
          if (cond->windspeed > 50)
            send_to_zone_outdoor(zon, "This must be the worst "
                                      "blizzard ever.\n\r");
          else if (cond->windspeed > 25)
            send_to_zone_outdoor(zon, "There's a blizzard out "
                                      "here, making it quite "
                                      "difficult to see.\n\r");
          else
            send_to_zone_outdoor(zon, "It's snowing very hard.\n\r");
        } else if (cond->precip_rate > 40) {
          if (cond->windspeed > 60)
            send_to_zone_outdoor(zon, "The heavily falling snow is "
                                      "being whipped up to a frenzy"
                                      " by a ferocious wind.\n\r");
          else if (cond->windspeed > 35)
            send_to_zone_outdoor(zon, "A heavy snow is being blown"
                                      " randomly about by a brisk "
                                      "wind.\n\r");
          else if (cond->windspeed > 18)
            send_to_zone_outdoor(zon, "Drifts in the snow are "
                                      "being formed by the wind.\n\r");
          else
            send_to_zone_outdoor(zon, "The snow's coming down "
                                      "pretty fast now.\n\r");
        } else if (cond->precip_rate > 19) {
          if (cond->windspeed > 70)
            send_to_zone_outdoor(zon, "The snow wouldn't be too "
                                      "bad, except for the awful "
                                      "wind blowing it in every "
                                      "possible directon.\n\r");
          else if (cond->windspeed > 45)
            send_to_zone_outdoor(zon, "There's a minor blizzard "
                                      "here, more wind than snow.\n\r");
          else if (cond->windspeed > 12)
            send_to_zone_outdoor(zon, "Snow is being blown about "
                                      "by a stiff breeze.\n\r");
          else
            send_to_zone_outdoor(zon, "It is snowing here.\n\r");
        } else if (cond->windspeed > 60)
          send_to_zone_outdoor(zon, "A light snow is being tossed about "
                                    "by a fierce wind.\n\r");
        else if (cond->windspeed > 42)
          send_to_zone_outdoor(zon, "A lightly falling snow is being "
                                    "driven by a strong wind.\n\r");
        else if (cond->windspeed > 18)
          send_to_zone_outdoor(zon, "A light snow is falling admidst "
                                    "an unsettled wind.\n\r");
        else
          send_to_zone_outdoor(zon, "It is lightly snowing.\n\r");
      }

      /* Handle celestial objects */
      if (!(clime->flags & NO_SUN_EVER)) {
        if (time_info.hours < 6 || time_info.hours > 18 || cond->humidity > 90 || cond->precip_rate > 80)
          cond->flags &= ~SUN_VISIBLE;
        else
          cond->flags |= SUN_VISIBLE;
      }
      if (!(clime->flags & NO_MOON_EVER)) {
        if ((time_info.hours > 5 && time_info.hours < 19) || cond->humidity > 80 || cond->precip_rate > 70 ||
            time_info.day < 3 || time_info.day > 31)
          cond->flags &= ~MOON_VISIBLE;
        else if (!(cond->flags & MOON_VISIBLE)) {
          cond->flags |= MOON_VISIBLE;
          if (time_info.day == 17)
            send_to_zone_outdoor(zon, "The full moon floods the "
                                      "area with light.\n\r");
          else
            send_to_zone_outdoor(zon, "The moon casts a little "
                                      "bit of light on the ground.\n\r");
        }
      }
      calc_light_zone(&zone_table[zon]);
    } /* End zone iteration */
    blow_out_torches();
  } /* End if(mode) */
}

void blow_out_torches() {
  struct char_data *i;
  struct obj_data *obj;
  for (i = character_list; i; i = i->next)
    if (!IS_SET(world[i->in_room].room_flags, ROOM_INDOORS) && i->equipment[WEAR_HOLD]) {
      obj = i->equipment[WEAR_HOLD];
      if (obj->obj_flags.value[3]) {
        if (GET_OBJ_TYPE(obj) == ITEM_LIGHT && GET_OBJ_VAL(obj, 3) > -1 &&
            GET_OBJ_VAL(obj, 3) < GET_WINDSPEED(IN_ZONE(i)) && number(0, 1)) {
          act("Your $p goes out and you put it away.", TRUE, i, obj, NULL, TO_CHAR);
          stderr_log("blowing out a light source");
          unequip_char(i, WEAR_HOLD);
          obj_to_char(obj, i);
        }
      }
    }
}

void calc_light_zone(struct zone_data *zone) {
  char light_sum = 0, temp, temp2;

  if (!(zone->climate.flags & NO_SUN_EVER)) {
    temp = time_info.hours;
    if (temp > 11)
      temp = 23 - temp;
    temp -= 5;
    if (temp > 0) {
      zone->conditions.flags |= SUN_VISIBLE;
      light_sum += temp * 10;
    } else
      zone->conditions.flags &= ~SUN_VISIBLE;
  }
  if (!(zone->climate.flags & NO_MOON_EVER)) {
    temp = abs(time_info.hours - 12);
    temp -= 7;
    if (temp > 0) {
      temp2 = 17 - abs(time_info.day - 17);
      temp2 -= 3;
      if (temp2 > 0) {
        zone->conditions.flags |= MOON_VISIBLE;
        light_sum += temp * temp2 / 2;
      } else
        zone->conditions.flags &= ~MOON_VISIBLE;
    }
  }
  light_sum -= zone->conditions.precip_rate;
  light_sum = MAX(0, light_sum);
  light_sum = MIN(100, light_sum);
  zone->conditions.ambient_light = light_sum;
}

char get_season(struct zone_data *zone) {
  char season_num;
  char buf[MAX_STRING_LENGTH];

  switch (zone->climate.season_pattern) {
  case ONE_SEASON:
    season_num = 0;
    break;
  case TWO_SEASONS_EQUAL:
    season_num = (time_info.month < 9) ? 0 : 1;
    break;
  case TWO_SEASONS_FIRST_LONG:
    season_num = (time_info.month < 11) ? 0 : 1;
    break;
  case TWO_SEASONS_SECOND_LONG:
    season_num = (time_info.month < 7) ? 0 : 1;
    break;
  case THREE_SEASONS_EQUAL:
    season_num = (time_info.month < 6) ? 0 : (time_info.month < 11) ? 1 : 2;
    break;
  case FOUR_SEASONS_EQUAL:
    season_num = (time_info.month < 5) ? 0 : (time_info.month < 9) ? 1 : (time_info.month < 13) ? 2 : 3;
    break;
  case FOUR_SEASONS_EVEN_LONG:
    season_num = (time_info.month < 4) ? 0 : (time_info.month < 9) ? 1 : (time_info.month < 11) ? 2 : 3;
    break;
  case FOUR_SEASONS_ODD_LONG:
    season_num = (time_info.month < 6) ? 0 : (time_info.month < 9) ? 1 : (time_info.month < 12) ? 2 : 3;
    break;
  default: /* Hmmm?!? */
    stderr_log("Bad Season spec in get_season!");
    safe_snprintf(g_buf, MAX_STRING_LENGTH, "--> %d in zone %s", zone->climate.season_pattern, zone->name);
    stderr_log(g_buf);
    season_num = 0;
    break;
  }
  return (season_num);
}
