/*  The macros provide the type casting useful when writing event drivers. */
#define VICTIM_CH  ((struct char_data *)victim)
#define CAUSER_CH  ((struct char_data *)causer)
#define VICTIM_OBJ ((struct obj_data *)victim)
#define CAUSER_OBJ ((struct obj_data *)causer)

/* EVENT TYPES */
#define EVENT_CAMP       1
#define EVENT_SPELL      2
#define EVENT_MEM        3
#define EVENT_SCRIBE     4
#define EVENT_KNOCKEDOUT 5

struct spell_info_type;
typedef void event(void *causer, void *victim, long info, struct spell_info_type *sinfo, void *info2);
typedef void spell(struct spell_info_type *sinfo, int waitstate, struct char_data *ch, char *arg, int isobj);

#define EVENT(name) void (name) (void *causer, void *victim, long info, struct spell_info_type *sinfo, void *info2)
/* for spells
 causer = caster,
 victim = vict,
 info = target,
 sinfo = spell structure for spell being cast,
 info2 = isobj
 */

struct affect_struct {
  int location;
  int modifier;
};

struct spell_info_type {
  char *command;
  spell *spell_pointer;
  event *event_pointer;

  int min_position; /* Position for caster   */
  int mana_min; /* Min amount of mana used by a spell (highest lev) */
  int mana_max; /* Max amount of mana used by a spell (lowest lev) */
  int mana_change; /* Change in mana used by spell from lev to lev */
  int spellindex; /* index of spell/skill into spells array */
  int aggressive; /* is spell aggressive */
  int difficulty; /* Skill diffculty */
  int realm;
  int invisible;
  int quest_only;
  int qvnum;
  int dice_add;
  int dice_add2;
  int dice_limit;
  int dice_limit2;
  int size_limit;
  int size_limit2;
  int num_dice;
  int num_dice2;
  int size_dice;
  int size_dice2;
  int resist_type;
  int saving_throw;
  int point_loc;
  int num_obj_aff;
  int num_plr_aff;
  int accum_affect;
  int avg_affect;
  int accum_duration;
  int avg_duration;
  int spell_duration;
  int spell_obj_bit;
  int spell_plr_bit;
  int spell_plr_bit2;
  int spell_plr_bit3;
  int spell_room_bit;
  int delay;
  int cost_multiplier;
  int prime_stat;
  int second_stat;
  char *unaffect;
  char *send_to_vict;
  char *send_to_room;
  char *send_to_char;
  char *wear_off;
  struct affect_struct obj_aff[NUM_MODIFY];
  struct affect_struct plr_aff[NUM_MODIFY];
  int min_level[NUM_CLASSES];
  int vnum_list[10];
  int npc_offense_flags;
  int npc_defense_flags;
};

struct event_info {
  int ticks_to_go;
  EVENT(*func);
  void *causer;
  void *victim;
  void *info;
  void *info2;
  struct spell_info_type *sinfo;
  char *command;
  struct event_info *next;
  int ticker; /* used for concentration check */
  int type;
};

void add_event(int delay, EVENT(*func), int type, void *causer, void *victim, void *info, struct spell_info_type *sinfo, char *command, void* info2);
void run_events();
bool clean_events(void *pointer, EVENT(*func));
bool clean_causer_events(void *pointer, int type);
bool check_events(void *pointer, EVENT(*func));
struct event_info *find_event(struct char_data *ch, int type);

/* events */
EVENT(spell_teleport_event);
EVENT(knockedout);
EVENT(camp);
EVENT(fail_spell_event);
EVENT(spell_magic_missile_event);
EVENT(spell_word_recall_event);
EVENT(spell_dam_event);
EVENT(spell_char_event);
EVENT(spell_points_event);
EVENT(spell_obj_event);
EVENT(spell_obj_room_event);
EVENT(spell_room_event);
EVENT(spell_area_event);
EVENT(spell_dimdoor_event);
EVENT(spell_locate_obj_event);
EVENT(spell_create_obj_event);
EVENT(spell_area_points_event);
EVENT(spell_summon_event);
EVENT(spell_create_mob_event);
EVENT(spell_group_points_event);
EVENT(spell_group_event);
EVENT(spell_obj_char_event);
EVENT(spell_area_dam_event);
EVENT(spell_confusion_event);
EVENT(spell_charm_event);
EVENT(spell_dispel_magic_event);
EVENT(spell_telekinesis_event);
EVENT(spell_magical_lock_event);
EVENT(spell_magical_unlock_event);
EVENT(spell_disintegrate_event);
EVENT(spell_resurrection_event);
EVENT(spell_turn_undead_event);
EVENT(spell_identify_event);
EVENT(spell_create_water_event);
EVENT(scribe_event);
EVENT(memorize_event);
EVENT(spell_destroy_inventory_event);
EVENT(spell_destroy_equipment_event);
EVENT(spell_add_dam_event);
EVENT(spell_prismatic_spray_event);
