/* ************************************************************************
 *   File: utils.h                                       Part of CircleMUD *
 *  Usage: header file: utility macros and prototypes of utility funcs     *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

/* external declarations and prototypes ********************************* */

#include <ctype.h>

extern struct weather_data weather_info;

/* public functions in utils.c */
char *make_money_text(int coins);
char *str_dup(char *arg1);
int str_cmp(char *arg1, char *arg2);
int strn_cmp(char *arg1, char *arg2, int n);
void stderr_log(char *str);
int touch(char *path);
void mudlog(char *str, char type, sbyte level, byte file);
void log_death_trap(struct char_data *ch);
int number(int from, int to);
int dice(int number, int size);
void sprintbit(unsigned long vektor, char *names[], char *result);
void sprinttype(int type, char *names[], char *result);
int get_line(FILE *fl, char *buf);
void unget_line(FILE *fl);
void strip_cashreturn(char *string);
int get_filename(char *orig_name, char *filename, int mode);
struct time_info_data age(struct char_data *ch);
void plog(char *logtext, struct char_data * ch, int level);
int replace_str(char **string, char *pattern, char *replacement, int rep_all, int max_size);
void format_text(char **ptr_string, int mode, struct descriptor_data *d, int maxlen);
char *stripcr(char *dest, const char *src);

/* undefine MAX and MIN so that our functions are used instead */
#ifdef MAX
#undef MAX
#endif

#ifdef MIN
#undef MIN
#endif

int MAX(int a, int b);
int MIN(int a, int b);
int BOUNDED(int a, int b, int c);

/* define an absolute value macro */
#define ABS(x) (((x) > 0) ? (x) : -(x))

/* in magic.c */
bool circle_follow(struct char_data *ch, struct char_data * victim);

/* in act.informative.c */
void look_at_room(struct char_data *ch, int mode);

/* in act.movmement.c */
int do_simple_move(struct char_data *ch, int dir, int following);
int perform_move(struct char_data *ch, int dir, int following);

/* in limits.c */
int mana_limit(struct char_data *ch);
int hit_limit(struct char_data *ch);
int move_limit(struct char_data *ch);
int mana_gain(struct char_data *ch, int tickcount);
int hit_gain(struct char_data *ch, int tickcount);
int move_gain(struct char_data *ch, int tickcount);
void advance_level(struct char_data *ch);
void lose_level(struct char_data *ch);
void set_title(struct char_data *ch, char *title);
void gain_exp(struct char_data *ch, int gain);
void gain_exp_regardless(struct char_data *ch, int gain);
void gain_condition(struct char_data *ch, int condition, int value);
void check_idling(struct char_data *ch);
void point_update(void);
void update_pos(struct char_data *victim);

/* various constants *****************************************************/

/* defines for mudlog() */
#define OFF	0
#define BRF	1
#define NRM	2
#define CMP	3

/* get_filename() */
#define CRASH_FILE	0
#define ETEXT_FILE	1
#define PLOG_FILE       2
#define PDAT_FILE       3
#define PTDAT_FILE      4

/* breadth-first searching */
#define BFS_ERROR		-1
#define BFS_ALREADY_THERE	-2
#define BFS_NO_PATH		-3

/* real-life time (remember Real Life?) */
#define SECS_PER_REAL_MIN	60
#define SECS_PER_REAL_HOUR	(60*SECS_PER_REAL_MIN)
#define SECS_PER_REAL_DAY	(24*SECS_PER_REAL_HOUR)
#define SECS_PER_REAL_YEAR	(365*SECS_PER_REAL_DAY)

/* spec proc utils *******************************************************/

#define SPEC_ROOM_TYPE(room) (world[(room)].spec_type)
#define SPEC_MOB_TYPE(ch) (mob_index[(ch->nr)].spec_type)
#define SPEC_OBJ_TYPE(obj) (obj_index[(obj)->item_number].spec_type)

#define GET_ROOM_SVAL(room, num) (world[(room)].spec_vars[(num)])

/* string utils **********************************************************/

#define YESNO(a) ((a) ? "YES" : "NO")
#define ONOFF(a) ((a) ? "ON" : "OFF")

/*
 #define LOWER(c)   (((c)>='A'  && (c) <= 'Z') ? ((c)+('a'-'A')) : (c))
 #define UPPER(c)   (((c)>='a'  && (c) <= 'z') ? ((c)+('A'-'a')) : (c) )
 */

#define LOWER(c)   (tolower((c)))
#define UPPER(c)   (toupper((c)))

#define ISNEWL(ch) ((ch) == '\n' || (ch) == '\r') 
#define IF_STR(st) ((st) ? (st) : "\0")
#define CAP(st)  (*(st) = UPPER(*(st)), st)

#define AN(string) (strchr("aeiouAEIOU", *string) ? "an" : "a")

#define STRNCPY ((t), (s), (tl), (sl)) \
{ \
  strncpy((t), (s), (sl)); \
  if (strlen((s)) < (tl)) { \
    *((t) + strlen((s))) = '\0'; \
  } else if (strlen((s)) > (sl)) { \
    *((t) + (sl)) = '\0'; \
  } else if (strlen((s)) > (tl)) { \
    *((t) + (tl)) = '\0'; \
  } \
}

/* memory utils **********************************************************/

/* set this to 1 to enable logging of memory usage. WARNING this is spammy. */
#define LOG_FREE 0

#if (LOG_FREE==1)
#define FREE(x) { char sTemp[256]; \
                  sprintf (sTemp, "MEM: Freeing %d bytes in %s on line %d in %s.", \
                           sizeof(x), __FUNCTION__, __LINE__, __FILE__); \
                  log(sTemp); \
                  fflush(NULL); \
                  free(x); }
#define CREATE(result, type, number)  { char sTemp[256]; \
        sprintf(sTemp, "MEM: Allocating %d bytes in %s on line %d in %s.", \
                sizeof(type) * (number), __FUNCTION__, __LINE__, __FILE__); \
        log(sTemp); \
        fflush(NULL); \
	if (!((result) = (type *) calloc ((number), sizeof(type))))\
		{ perror("malloc failure"); abort(); } }

#define RECREATE(result,type,number) { char sTemp[256]; \
        sprintf(sTemp, "MEM: Re-Allocating %d bytes in %s on line %d in %s.", \
                sizeof(type) * (number), __FUNCTION__, __LINE__, __FILE__); \
        log(sTemp); \
        fflush(NULL); \
  if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
		{ perror("realloc failure"); abort(); } }

#else
#define FREE(x) (free(x))
#define CREATE(result, type, number)  {\
	if (!((result) = (type *) calloc ((number), sizeof(type))))\
		{ perror("malloc failure"); abort(); } }

#define RECREATE(result,type,number) {\
  if (!((result) = (type *) realloc ((result), sizeof(type) * (number))))\
		{ perror("realloc failure"); abort(); } }

#endif

/* the source previously used the same code in many places to remove an item
 * from a list: if it's the list head, change the head, else traverse the
 * list looking for the item before the one to be removed.  Now, we have a
 * macro to do this.  To use, just make sure that there is a variable 'temp'
 * declared as the same type as the list to be manipulated.  BTW, this is
 * a great application for C++ templates but, alas, this is not C++.  Maybe
 * CircleMUD 4.0 will be...
 */
#define REMOVE_FROM_LIST(item, head, next)	\
   if ((item) == (head))		\
      head = (item)->next;		\
   else {				\
      temp = head;			\
      while (temp && (temp->next != (item))) \
	 temp = temp->next;		\
      if (temp)				\
         temp->next = (item)->next;	\
   }					\


/* basic bitvector utils *************************************************/

#define IS_SET(flag,bit)  ((flag) & (bit))
#define SET_BIT(var,bit)  ((var) |= (bit))
#define REMOVE_BIT(var,bit)  ((var) &= ~(bit))
#define TOGGLE_BIT(var,bit) ((var) = (var) ^ (bit))

#define MOB_FLAGS(ch) ((ch)->char_specials.saved.act)
#define PLR_FLAGS(ch) ((ch)->char_specials.saved.act)
#define PRF_FLAGS(ch) ((ch)->player_specials->saved.pref)
#define AFF_FLAGS(ch) ((ch)->char_specials.saved.affected_by)
#define AFF2_FLAGS(ch) ((ch)->char_specials.saved.affected_by2)
#define AFF3_FLAGS(ch) ((ch)->char_specials.saved.affected_by3)
#define ROOM_FLAGS(loc) (world[(loc)].room_flags)
#define COM_FLAGS(ch) ((ch)->player_specials->saved.commands)
#define LOG_FLAGS(ch) ((ch)->player_specials->saved.log)
#define SEASON_PATTERN(zone) (zone_table[(zone)].climate.season_pattern)
#define SEASON_WIND(zone, season) (zone_table[(zone)].climate.season_wind[(season)])
#define SEASON_WIND_DIR(zone, season) (zone_table[(zone)].climate.season_wind_dir[(season)])
#define SEASON_WIND_VAR(zone, season) (zone_table[(zone)].climate.season_wind_variance[(season)])
#define SEASON_PRECIP(zone, season) (zone_table[(zone)].climate.season_precip[(season)])
#define SEASON_TEMP(zone, season) (zone_table[(zone)].climate.season_temp[(season)])
#define SEASON_FLAGS(zone) (zone_table[(zone)].climate.flags)
#define SEASON_ENERGY_ADD(zone) (zone_table[(zone)].climate.energy_add)

#define GET_WINDSPEED(zone) (zone_table[(zone)].conditions.windspeed)
#define GET_WINDDIR(zone) (zone_table[(zone)].conditions.wind_dir)
#define GET_ZONE_COND(zone) (zone_table[(zone)].conditions)
#define GET_ZONE_LIGHT(zone) (zone_table[(zone)].conditions.ambient_light)
#define IS_NPC(ch)  (IS_SET(MOB_FLAGS(ch), MOB_ISNPC))
#define IS_MOB(ch)  (IS_NPC(ch) && ((ch)->nr >-1))

#define MOB_FLAGGED(ch, flag) (IS_NPC(ch) && IS_SET(MOB_FLAGS(ch), (flag)))
#define PLR_FLAGGED(ch, flag) (!IS_NPC(ch) && IS_SET(PLR_FLAGS(ch), (flag)))
#define AFF_FLAGGED(ch, flag) (IS_SET(AFF_FLAGS(ch), (flag)))
#define AFF2_FLAGGED(ch, flag) (IS_SET(AFF2_FLAGS(ch), (flag)))
#define AFF3_FLAGGED(ch, flag) (IS_SET(AFF3_FLAGS(ch), (flag)))
#define PRF_FLAGGED(ch, flag) (IS_SET(PRF_FLAGS(ch), (flag)))
#define COM_FLAGGED(ch, flag) (IS_SET(COM_FLAGS(ch), (flag)))
#define ACOM_FLAGGED(ch, flag) (IS_SET(ACOM_FLAGS(ch), (flag)))
#define QST_FLAGGED(ch, flag) (IS_SET(QST_FLAGS(ch), (flag)))
#define LOG_FLAGGED(ch, flag) (IS_SET(LOG_FLAGS(ch), (flag)))
#define ROOM_FLAGGED(loc, flag) (IS_SET(ROOM_FLAGS(loc), (flag)))

/* IS_AFFECTED for backwards compatibility */
#define IS_AFFECTED(ch, skill) (AFF_FLAGGED((ch), (skill)))
#define IS_AFFECTED2(ch, skill) (AFF2_FLAGGED((ch), (skill)))
#define IS_AFFECTED3(ch, skill) (AFF3_FLAGGED((ch), (skill)))
#define IS_CASTING(ch) (AFF2_FLAGGED((ch), AFF2_CASTING))

#define PLR_TOG_CHK(ch,flag) ((TOGGLE_BIT(PLR_FLAGS(ch), (flag))) & (flag))
#define PRF_TOG_CHK(ch,flag) ((TOGGLE_BIT(PRF_FLAGS(ch), (flag))) & (flag))

/* room utils ************************************************************/

#define GET_SECT(room) (world[(room)].sector_type)
int IS_DARK(int room);

#define IS_LIGHT(room)  (!IS_DARK(room))
#define IS_COLD(room)  (ROOM_FLAGGED(room, ROOM_COLD))
#define IS_HOT(room) (ROOM_FLAGGED(room, ROOM_HOT))

#define GET_ROOM_SPEC(room) ((room) >= 0 ? world[(room)].func : NULL)

/* char utils ************************************************************/

#define GET_SCRIBING(ch) ((ch)->scribing)
#define GET_CASTING(ch) ((ch)->casting)
#define MEDITATING(ch) (AFF_FLAGGED(ch, AFF_MEDITATING))
#define IN_ROOM(ch)	((ch)->in_room)
#define IN_ZONE(ch)	(world[(ch)->in_room].zone)
#define IN_SECTOR(room)	(world[(room)].sector_type)
#define GET_WAS_IN(ch)	((ch)->was_in_room)
#define GET_SCREEN_WIDTH(ch) ((ch)->screen_width)
#define GET_SCREEN_HEIGHT(ch) ((ch)->screen_height)
#define GET_CONSENT(ch)	((ch)->consent)
#define GET_AGE(ch)     (age(ch).year)
#define IS_ANIMATED(ch) ((ch)->animated)
#define GET_LDESC(ch) ((ch)->player.long_descr)
#define GET_ADESC(ch) ((ch)->animated_desc)
#define GET_DESC(ch) ((ch)->player.description)
#define GET_SPECIALIZED(ch) ((ch)->player_specials->saved.specialized)
#define QCOMPLETED(ch, i) ((ch)->player_specials->saved.questcompleted[i%10])

#define GET_NAME(ch)    (IS_ANIMATED(ch) ? (ch)->animated_name : (IS_NPC(ch) ? \
			 (ch)->player.short_descr : (ch)->player.name))
#define GET_PLR_NAME(ch) ((ch)->player.name)
#define GET_MOB_NAME(ch) ((ch)->player.short_descr)
#define GET_MOB_QUEST_NUM(ch) ((ch)->mob_specials.quest)
#define GET_ANIMATED_MOB_NAME(ch) ((ch)->animated_name)
#define GET_TITLE(ch)   ((ch)->player.title)
#define GET_LEVEL(ch)   ((ch)->player.level)
#define GET_PASSWD(ch)  ((ch)->player.passwd)
#define GET_BIRTH(ch)   ((ch)->player.time.birth)
#define GET_PLAYED(ch)   ((ch)->player.time.played)
#define GET_LOGON(ch)   ((ch)->player.time.logon)
#define GET_PFILEPOS(ch)((ch)->pfilepos)
#define GET_CITIZEN(ch) ((ch)->player_specials->saved.citizen)
#define GET_DEATHCOUNT(ch) ((ch)->player_specials->saved.death_count)
#define GET_PKCOUNT(ch) ((ch)->player_specials->saved.pk_count)
#define IS_IMMO(ch)     (!IS_NPC(ch) && ((ch)->player.level >= LVL_IMMORT))
#define GET_HOST(ch)    ((ch)->player.host)
#define CHAR_WEARING(ch) ((ch)->wearing)

/*
 * I wonder if this definition of GET_REAL_LEVEL should be the definition
 * of GET_LEVEL?  JE
 */
#define GET_REAL_LEVEL(ch) \
   (ch->desc && ch->desc->original ? GET_LEVEL(ch->desc->original) : \
    GET_LEVEL(ch))

#define GET_CLASS(ch)   ((ch)->player.class)
#define GET_HOME(ch)	((ch)->player.hometown)
#define GET_HEIGHT(ch)	((ch)->player.height)
#define GET_WEIGHT(ch)	((ch)->player.weight)
#define GET_SEX(ch)	((ch)->player.sex)

#define GET_STR(ch)     ((ch)->aff_abils.str)
#define GET_DEX(ch)     ((ch)->aff_abils.dex)
#define GET_INT(ch)     ((ch)->aff_abils.intel)
#define GET_WIS(ch)     ((ch)->aff_abils.wis)
#define GET_CON(ch)     ((ch)->aff_abils.con)
#define GET_AGI(ch)     ((ch)->aff_abils.agi)
#define GET_VSTR(ch)     ((ch)->aff_abils.vstr)
#define GET_VDEX(ch)     ((ch)->aff_abils.vdex)
#define GET_VINT(ch)     ((ch)->aff_abils.vintel)
#define GET_VWIS(ch)     ((ch)->aff_abils.vwis)
#define GET_VCON(ch)     ((ch)->aff_abils.vcon)
#define GET_VAGI(ch)     ((ch)->aff_abils.vagi)

#define GET_EXP(ch)	  ((ch)->points.exp)
#define GET_AC(ch)        ((ch)->points.armor)
#define GET_HIT(ch)	  ((ch)->points.hit)
#define GET_MAX_HIT(ch)	  ((ch)->points.max_hit)
#define GET_MOVE(ch)	  ((ch)->points.move)
#define GET_MAX_MOVE(ch)  ((ch)->points.max_move)
#define GET_MANA(ch)	  ((ch)->points.mana)
#define GET_MAX_MANA(ch)  ((ch)->points.max_mana)
#define GET_TEMP_GOLD(ch)	  ((ch)->points.temp_gold)
#define GET_PLAT(ch)	  ((ch)->points.plat)
#define GET_GOLD(ch)	  ((ch)->points.gold)
#define GET_SILVER(ch)	  ((ch)->points.silver)
#define GET_COPPER(ch)	  ((ch)->points.copper)
#define GET_BANK_PLAT(ch) ((ch)->points.bank_plat)
#define GET_BANK_GOLD(ch) ((ch)->points.bank_gold)
#define GET_BANK_SILVER(ch) ((ch)->points.bank_silver)
#define GET_BANK_COPPER(ch) ((ch)->points.bank_copper)
#define GET_GUILD_BANK_PLAT(ch) ((ch)->points.bank_plat)
#define GET_GUILD_BANK_GOLD(ch) ((ch)->points.bank_gold)
#define GET_GUILD_BANK_SILVER(ch) ((ch)->points.bank_silver)
#define GET_GUILD_BANK_COPPER(ch) ((ch)->points.bank_copper)
#define GET_HITROLL(ch)	  ((ch)->points.hitroll)
#define GET_DAMROLL(ch)   ((ch)->points.damroll)
#define GET_ATTACKED(ch)  ((ch)->char_specials.attackednum)
#define GET_POS(ch)	  ((ch)->char_specials.position)
#define GET_IDNUM(ch)	  ((ch)->char_specials.saved.idnum)
#define IS_CARRYING_W(ch) ((ch)->char_specials.carry_weight)
#define IS_CARRYING_N(ch) ((ch)->char_specials.carry_items)
#define FIGHTING(ch)	  ((ch)->char_specials.fighting)
#define HUNTING(ch)	  ((ch)->char_specials.hunting)
#define HUNTINGRM(ch)	  ((ch)->char_specials.huntingrm)
#define MOUNTING(ch)      ((ch)->char_specials.mounting)
#define MOUNTED_BY(ch)    ((ch)->char_specials.mounted_by)
#define GET_SAVE(ch, i)	  ((ch)->char_specials.saved.apply_saving_throw[i])
#define GET_RESIST(ch, i) ((ch)->char_specials.saved.resists[i])
#define GET_ALIGNMENT(ch) ((ch)->char_specials.saved.alignment)

#define GET_COND(ch, i)		((ch)->player_specials->saved.conditions[(i)])
#define GET_LOADROOM(ch)	((ch)->player_specials->saved.load_room)
#define GET_PRACTICES(ch)	((ch)->player_specials->saved.spells_to_learn)
#define GET_INVIS_LEV(ch)	(IS_NPC(ch) ? GET_MOB_INVIS_LEV(ch) : \
                                GET_PLR_INVIS_LEV(ch))
#define GET_MOB_INVIS_LEV(ch)	((ch)->mob_specials.invis_level)
#define GET_MOB_QUEST(ch)	((ch)->mob_specials.quest)
#define GET_PLR_INVIS_LEV(ch)	((ch)->player_specials->saved.invis_level)
#define GET_WIMP_LEV(ch)	((ch)->player_specials->saved.wimp_level)
#define GET_TIMER(ch)	        ((ch)->player_specials->saved.timer)
#define GET_FREEZE_LEV(ch)	((ch)->player_specials->saved.freeze_level)
#define GET_BAD_PWS(ch)		((ch)->player_specials->saved.bad_pws)
#define GET_TALK(ch, i)		((ch)->player_specials->saved.talks[i])
#define POOFIN(ch)		((ch)->player_specials->poofin)
#define NAMECOLOR(ch)		((ch)->desc->namecolor)
#define POOFOUT(ch)		((ch)->player_specials->poofout)
#define GET_TELL(ch)		((ch)->player_specials->tell)

#define WHOSPEC(ch)             ((ch)->player_specials->whospec)
#define WHOSTR(ch)		((ch)->player_specials->whois)
#define GET_LAST_OLC_TARG(ch)	((ch)->player_specials->last_olc_targ)
#define GET_LAST_OLC_MODE(ch)	((ch)->player_specials->last_olc_mode)
#define GET_ALIASES(ch)		((ch)->player_specials->aliases)
#define GET_LAST_TELL(ch)	((ch)->player_specials->last_tell)
#define GET_RACE(ch)		((ch)->player_specials->saved.race)
#define GET_RACE_NAME(ch)	(pc_race_types[(ch)->player_specials->saved.race])
#define GET_RACE_NAME_COLOR(ch)	(pc_race_types_color[(ch)->player_specials->saved.race])
#define GET_TRAINING(ch)        ((ch)->player_specials->saved.train_sessions)
#define GET_PROMPT(ch)          ((ch)->player_specials->saved.prompt)
#define GET_KILLS(ch)		((ch)->player_specials->saved.kills)
#define GET_RECALL(ch)          ((ch)->player_specials->saved.recall_lvl)

#define GET_SKILL(ch, i)	((ch)->player_specials->saved.skills[i])
#define SET_SKILL(ch, i, pct)	{ (ch)->player_specials->saved.skills[i] = pct; }
#define GET_PRACS(ch, i)	((ch)->player_specials->saved.pracs[i])     

#define GET_EQ(ch, i)           ((ch)->equipment[i])
#define GET_DRAGGING(ch)           ((ch)->dragging)

#define GET_MOB_RACE(ch)	((ch)->mob_specials.race)
#define GET_MOB_SIZE(ch)	((ch)->mob_specials.size)
#define GET_MOB_SPEC(ch) (IS_MOB(ch) ? (mob_index[(ch->nr)].func) : NULL)
#define GET_MOB_RNUM(mob)	((mob)->nr)
#define GET_MOB_VNUM(mob)	(IS_MOB(mob) ? \
				 mob_index[GET_MOB_RNUM(mob)].virtual : -1)
#define GET_MOB_WAIT(ch)        ((ch)->mob_specials.wait_state)

#define MEMORY(ch)		((ch)->mob_specials.memory)
#define GET_DEFAULT_POS(ch)	((ch)->mob_specials.default_pos)
/*
 #define STRENGTH_APPLY_INDEX(ch) \
        ( ((GET_ADD(ch)==0) || (GET_STR(ch) != 18)) ? GET_STR(ch) :\
          (GET_ADD(ch) <= 50) ? 26 :( \
          (GET_ADD(ch) <= 75) ? 27 :( \
          (GET_ADD(ch) <= 90) ? 28 :( \
          (GET_ADD(ch) <= 99) ? 29 :  30 ) ) )                   \
        )
 */
#define CAN_CARRY_W(ch) (stats[STR_CWEIGHT][GET_STR(ch)])
#define CAN_CARRY_N(ch) (10 + (GET_DEX(ch) / 10) + (GET_LEVEL(ch) >> 1))
#define AWAKE(ch) (GET_POS(ch) > POS_SLEEPING)

#define IS_GOOD(ch)    (GET_ALIGNMENT(ch) >= 350)
#define IS_EVIL(ch)    (GET_ALIGNMENT(ch) <= -350)
#define IS_NEUTRAL(ch) (!IS_GOOD(ch) && !IS_EVIL(ch))

/* descriptor-based utils ************************************************/

#define WAIT_STATE(ch, cycle) { if ((ch)->desc) (ch)->desc->wait = (cycle); }
#define GET_WAIT(ch)   (ch)->desc->wait
#define FIGHT_STATE(ch, cycle) ((ch)->char_specials.fightwait = (cycle))
#define SKILL_TIMER(ch) ((ch)->char_specials.skill_timer)

#define CHECK_WAIT(ch)	(((ch)->desc) ? ((ch)->desc->wait > 1) : 0)
#define CHECK_FIGHT(ch)	((ch)->char_specials.fightwait > 0)
#define STATE(d)	((d)->connected)

/* object utils **********************************************************/

#define GET_OBJ_TYPE(obj)	((obj)->obj_flags.type_flag)
#define GET_OBJ_COST(obj)	((obj)->obj_flags.cost)
#define GET_OBJ_RENT(obj)	((obj)->obj_flags.cost_per_day)
#define GET_OBJ_EXTRA(obj)	((obj)->obj_flags.extra_flags)
#define GET_OBJ_BITV(obj)	((obj)->obj_flags.bitvector)
#define GET_OBJ_BITV2(obj)	((obj)->obj_flags.bitvector2)
#define GET_OBJ_BITV3(obj)	((obj)->obj_flags.bitvector3)
#define GET_OBJ_SPELLLIST(obj) ((obj)->spell_list)
#define GET_OBJ_SPELLLISTNUM(obj, i) ((obj)->spell_list[(i)])
#define GET_OBJ_WEAR(obj)	((obj)->obj_flags.wear_flags)
#define GET_OBJ_SLOTS(obj)	((obj)->obj_flags.wear_slots)
#define GET_OBJ_VAL(obj, val)	((obj)->obj_flags.value[(val)])
#define GET_OBJ_WEIGHT(obj)	((obj)->obj_flags.weight)
#define GET_OBJ_TIMER(obj)	((obj)->obj_flags.timer)
#define GET_OBJ_RNUM(obj)	((obj)->item_number)
#define GET_OBJ_VNUM(obj)	(GET_OBJ_RNUM(obj) >= 0 ? \
				 obj_index[GET_OBJ_RNUM(obj)].virtual : -1)
#define IS_OBJ_STAT(obj,stat)	(IS_SET((obj)->obj_flags.extra_flags,stat))
#define GET_OBJ_SVAL(obj, val)  ((obj)->spec_vars[(val)])
#define GET_MOB_SVAL(mob, val)  ((mob)->spec_vars[(val)])

#define GET_OBJ_SPEC(obj) ((obj)->item_number >= 0 ? \
	(obj_index[(obj)->item_number].func) : NULL)

#define CAN_WEAR(obj, part) (IS_SET((obj)->obj_flags.wear_flags, (part)))

/* Can wear take is used in CAN_GET_OBJ, trying to make it possible for imps
 to take anything */

#define CAN_WEARSPEC(obj, part, ch) (IS_SET((obj)->obj_flags.wear_flags, \
                                     (part)) || COM_FLAGGED(ch, COM_ADMIN))

#define GET_OBJ_RESIST(obj, val)  ((obj)->resists[(val)])

/* compound utilities and other macros **********************************/

#define HSHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "his":"her") :"its")
#define HSSH(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "he" :"she") : "it")
#define HMHR(ch) (GET_SEX(ch) ? (GET_SEX(ch)==SEX_MALE ? "him":"her") : "it")

#define ANA(obj) (strchr("aeiouyAEIOUY", *(obj)->name) ? "An" : "A")
#define SANA(obj) (strchr("aeiouyAEIOUY", *(obj)->name) ? "an" : "a")

/* Various macros building up to CAN_SEE */

int CAN_SEE_IN_DARK(struct char_data *ch);
int INVIS_OK(struct char_data *ch, struct char_data *vict);

int LIGHT_OK(struct char_data *ch);

#define MORT_CAN_SEE(sub, obj) (LIGHT_OK(sub) && INVIS_OK(sub, obj))

#define IMM_CAN_SEE(sub, obj) \
   (MORT_CAN_SEE(sub, obj) || PRF_FLAGGED(sub, PRF_HOLYLIGHT))

#define SELF(sub, obj)  ((sub) == (obj))

/* Can subject see character "obj"? */
int CAN_SEE(struct char_data *ch, struct char_data *vict);

/* End of CAN_SEE */

#define INVIS_OK_OBJ(sub, obj) \
  (!IS_OBJ_STAT((obj), ITEM_INVISIBLE) || IS_AFFECTED((sub), AFF_DETECT_INVIS))

#define MORT_CAN_SEE_OBJ(sub, obj) (LIGHT_OK(sub) && INVIS_OK_OBJ(sub, obj))

#define CAN_SEE_OBJ(sub, obj) \
   (!IS_OBJ_STAT((obj), ITEM_UNDERWATER) && (MORT_CAN_SEE_OBJ(sub, obj) || PRF_FLAGGED((sub), PRF_HOLYLIGHT)))

#define CAN_CARRY_OBJ(ch,obj)  \
   (((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) <= CAN_CARRY_W(ch)) &&   \
    ((IS_CARRYING_N(ch) + 1) <= CAN_CARRY_N(ch)))

#define CAN_GET_OBJ(ch, obj)   \
   (CAN_WEARSPEC((obj), ITEM_WEAR_TAKE, ch) && CAN_CARRY_OBJ((ch),(obj)) && \
    CAN_SEE_OBJ((ch),(obj)))

#define PERS(ch, vict)   (CAN_SEE(vict, ch) ? (GET_NAME(ch)?GET_NAME(ch):"UNKNOWN") : "someone")

#define OBJS(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	((obj)->cshort_description ? (obj)->cshort_description : (obj)->short_description)  : "something")

#define OBJN(obj, vict) (CAN_SEE_OBJ((vict), (obj)) ? \
	((obj)->cname ? fname((obj)->cname) : fname((obj)->name)) : "something")

#define EXIT(ch, door)  (world[(ch)->in_room].dir_option[door])
#define _2ND_EXIT(ch, door)  (world[EXIT(ch, door)->to_room].dir_option[door])
#define _3RD_EXIT(ch, door)  (world[_2ND_EXIT(ch, door)->to_room].dir_option[door])

#define CAN_GO(ch, door) (EXIT(ch,door) && \
			 (EXIT(ch,door)->to_room != NOWHERE) && \
			 !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED))

#define CLASS_ABBR(ch) (IS_NPC(ch) ? "--" : class_abbrevs[(int)GET_CLASS(ch)])

#define IS_WAR(ch) (GET_CLASS(ch) == CLASS_WARRIOR || \
              GET_CLASS(ch) == CLASS_MONK || \
              GET_CLASS(ch) == CLASS_RANGER)
#define IS_THI(ch) (GET_CLASS(ch) == CLASS_THIEF || \
              GET_CLASS(ch) == CLASS_ASSASSIN || \
              GET_CLASS(ch) == CLASS_ROGUE || \
              GET_CLASS(ch) == CLASS_MERCENARY || \
              GET_CLASS(ch) == CLASS_BARD)
#define IS_PRI(ch) (GET_CLASS(ch) == CLASS_PRIEST || \
              GET_CLASS(ch) == CLASS_CLERIC || \
              GET_CLASS(ch) == CLASS_DRUID || \
              GET_CLASS(ch) == CLASS_SHAMAN)
#define IS_MAGE(ch) (GET_CLASS(ch) == CLASS_SORCERER || \
              GET_CLASS(ch) == CLASS_WIZARD || \
              GET_CLASS(ch) == CLASS_ENCHANTER || \
              GET_CLASS(ch) == CLASS_CONJURER || \
              GET_CLASS(ch) == CLASS_NECROMANCER)

#define MAGE_CLASS(class) ( ((1 << CLASS_SORCERER) \
                           | (1 << CLASS_WIZARD) \
                           | (1 << CLASS_ENCHANTER) \
                           | (1 << CLASS_CONJURER) \
                           | (1 << CLASS_NECROMANCER)) & (1 << (class)))

#define PRIEST_CLASS(class) ( ((1 << CLASS_PRIEST) \
                             | (1 << CLASS_CLERIC) \
                             | (1 << CLASS_DRUID) \
                             | (1 << CLASS_SHAMAN)) & (1 << (class)))
#define GET_STARTROOM(ch, hometown) (hometowns[(hometown)][(int)GET_CLASS(ch)]) 

#define IS_THIEF(ch)	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_THIEF))

#define IS_PRIEST(ch)	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_PRIEST))

#define IS_WARRIOR(ch)	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_WARRIOR))

#define IS_WIZARD(ch)	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_WIZARD))
#define IS_MERCENARY(ch)	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_MERCENARY))
#define IS_MONK(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_MONK))
#define IS_CONJURER(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_CONJURER))
#define IS_RANGER(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_RANGER))
#define IS_ROGUE(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_ROGUE))
#define IS_ASSASSIN(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_ASSASSIN))
#define IS_CLERIC(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_CLERIC))
#define IS_SHAMAN(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_SHAMAN))
#define IS_SORCERER(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_SORCERER))
#define IS_NECROMANCER(ch)	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_NECROMANCER))
#define IS_ENCHANTER(ch)	(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_ENCHANTER))
#define IS_BARD(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_BARD))
#define IS_DRUID(ch)		(!IS_NPC(ch) && \
				(GET_CLASS(ch) == CLASS_DRUID))

#define OUTSIDE(ch) (!ROOM_FLAGGED((ch)->in_room, ROOM_INDOORS))

#define GET_PLR_CIRCLE(ch) (GET_LEVEL(ch) <= 51 ? (GET_LEVEL(ch)+4)/5 : 11)
int GET_SPELL_CIRCLE(struct char_data *ch, struct spell_info_type *sinfo);
int GET_CIRCLE_DIFF(struct char_data *ch, struct spell_info_type *sinfo);

/* OS compatibility ******************************************************/

/* there could be some strange OS which doesn't have NULL... */
#ifndef NULL
#define NULL (void *)0
#endif

#if !defined(FALSE)
#define FALSE 0
#endif

#if !defined(TRUE)
#define TRUE  (!FALSE)
#endif

#if !defined(NO)
#define NO  0
#endif

#if !defined(YES)
#define YES  1
#endif

#if !defined(MPCAST)
#define MPCAST  2
#endif

/* defines for fseek */
#ifndef SEEK_SET
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2
#endif

/*
 * Some systems such as Sun's don't have prototyping in their header files.
 * Thus, we try to compensate for them.
 *
 * Much of this is from Merc 2.2, used with permission.
 */

#if defined(apollo)
int atoi (const char *string);
void *calloc( unsigned nelem, size_t size);
#endif

#if defined(hpux)
#define srandom srand
#define random rand
#endif

#if defined(sequent)
int fclose(FILE *stream);
int fprintf(FILE *stream, const char *format, ... );
int fread(void *ptr, int size, int n, FILE *stream);
int fseek(FILE *stream, long offset, int ptrname);
void perror(const char *s);
int ungetc(int c, FILE *stream);
#endif

#if defined(sun)
#include <memory.h>
void bzero(char *b, int length);
int fclose(FILE *stream);
int fflush(FILE *stream);
void rewind(FILE *stream);
int sscanf(const char *s, const char *format, ... );
int fprintf(FILE *stream, const char *format, ... );
int fscanf(FILE *stream, const char *format, ... );
int fseek(FILE *stream, long offset, int ptrname);
size_t fread(void *ptr, size_t size, size_t n, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t n, FILE *stream);
void perror(const char *s);
int ungetc(int c, FILE *stream);
time_t time(time_t *tloc);
int system(const char *string);
#endif

#if defined(DGUX_TARGET)
#define bzero(a, b) memset((a), 0, (b))
#endif
