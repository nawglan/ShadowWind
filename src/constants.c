/* ************************************************************************
 *   File: constants.c                                   Part of CircleMUD *
 *  Usage: Numeric and string contants used by the MUD                     *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#include "screen.h"
#include "structs.h"

const char circlemud_version[] = {"ESC v0.5b (based on CircleMUD 3.00pl4)\r\n"};

/* strings corresponding to ordinals/bitvectors in structs.h ***********/

/* (Note: strings for class definitions in class.c instead of here) */

const int equip_order[NUM_WEARS] = {WEAR_HEAD,    WEAR_FACE,    WEAR_EAR_R,    WEAR_EAR_L,    WEAR_EYES,  WEAR_NECK_1,
                                    WEAR_NECK_2,  WEAR_ABOUT,   WEAR_BODY,     WEAR_BADGE,    WEAR_ARMS,  WEAR_WRIST_R,
                                    WEAR_WRIST_L, WEAR_HANDS,   WEAR_FINGER_R, WEAR_FINGER_L, WEAR_WIELD, WEAR_WIELD_2,
                                    WEAR_2HANDED, WEAR_HOLD,    WEAR_HOLD_2,   WEAR_SHIELD,   WEAR_WAIST, WEAR_LEGS,
                                    WEAR_ANKLE_R, WEAR_ANKLE_L, WEAR_FEET};

const char *zone_extras[] = {"RESTRICTED", "!TELEPORT", "\n"};

const char *resist_short_name[] = {"ResLight",   "ResDark",  "ResFire",  "ResCold",        "ResAcid",   "ResPoison",
                                   "ResDisease", "ResCharm", "ResSleep", "ResSlash",       "ResPierce", "ResBludgeon",
                                   "ResNWeap",   "ResMWeap", "ResMagic", "ResElectricity", "\n"};
const char *resists_names[] = {
    "Undefined   ", "Light       ", "Dark        ", "Fire        ", "Cold        ", "Acid        ",
    "Poison      ", "Disease     ", "Charm       ", "Sleep       ", "Slash       ", "Pierce      ",
    "Bludgeon    ", "NWeapons    ", "MWeapons    ", "Magic       ", "Electricity ", "\n"};

const struct balance_type balance_table[] = {
    /* hitnum,hitsize,hitadd,exp,thac0,ac,damnum,damsize,damadd,gold */
    {1, 9, 1, 25, 20, 10, 1, 4, 0, 0}, /* 0 */
    {1, 11, 11, 100, 20, 9, 1, 5, 0, 10},
    {1, 12, 23, 200, 19, 8, 1, 6, 0, 50},
    {1, 11, 36, 350, 18, 7, 1, 7, 0, 100},
    {1, 12, 48, 600, 17, 6, 1, 8, 0, 200},
    {1, 11, 61, 900, 16, 5, 2, 4, 0, 300}, /* 5 */
    {1, 12, 73, 1500, 15, 4, 1, 8, 1, 400},
    {1, 11, 86, 2250, 14, 4, 2, 4, 1, 500},
    {1, 12, 98, 3750, 13, 3, 2, 5, 1, 600},
    {1, 11, 111, 6000, 12, 3, 2, 5, 1, 800},
    {1, 12, 123, 9000, 11, 2, 2, 6, 1, 1000}, /* 10 */
    {1, 11, 136, 11000, 10, 2, 2, 6, 1, 1200},
    {1, 12, 148, 13000, 9, 2, 2, 7, 1, 1400},
    {1, 11, 161, 16000, 8, 2, 2, 7, 1, 1600},
    {1, 12, 173, 18000, 7, 1, 2, 8, 1, 1800},
    {1, 11, 186, 21000, 6, 1, 2, 8, 2, 2000}, /* 15 */
    {1, 12, 198, 24000, 5, 1, 2, 8, 2, 2400},
    {1, 11, 211, 28000, 4, 1, 3, 6, 2, 2800},
    {1, 12, 223, 30000, 3, 0, 3, 6, 2, 3200},
    {1, 11, 236, 35000, 2, 0, 3, 6, 3, 3600},
    {1, 12, 248, 40000, 1, 0, 3, 6, 4, 4000}, /* 20 */
    {1, 11, 261, 50000, 0, -1, 3, 7, 5, 5000},
    {1, 12, 274, 60000, 0, -1, 3, 8, 4, 6000},
    {1, 11, 286, 80000, 0, -2, 3, 8, 4, 7000},
    {1, 12, 299, 100000, 0, -2, 3, 8, 4, 8000},
    {1, 11, 311, 110000, 0, -3, 4, 6, 4, 9000}, /* 25 */
    {1, 12, 324, 120000, 0, -3, 4, 6, 4, 10000},
    {1, 11, 336, 130000, 0, -4, 4, 6, 4, 11000},
    {1, 12, 349, 140000, 0, -4, 4, 6, 5, 12000},
    {1, 11, 361, 150000, 0, -5, 4, 7, 5, 13000},
    {1, 12, 374, 160000, 0, -5, 4, 8, 5, 14000}, /* 30 */
    {1, 15, 385, 180000, 0, -6, 4, 8, 5, 15000},
    {1, 50, 401, 200000, 0, -6, 4, 8, 5, 16000},
    {1, 50, 451, 220000, 0, -7, 4, 8, 6, 17000},
    {1, 50, 501, 240000, 0, -7, 4, 8, 6, 18000},
    {1, 50, 551, 260000, 0, -8, 4, 8, 6, 19000}, /* 35 */
    {1, 100, 601, 280000, 0, -8, 4, 8, 6, 20000},
    {1, 100, 701, 300000, 0, -9, 4, 9, 6, 21000},
    {1, 100, 801, 320000, 0, -9, 4, 9, 6, 22000},
    {1, 200, 900, 340000, 0, -10, 4, 9, 6, 23000},
    {1, 400, 1101, 360000, 0, -10, 5, 9, 6, 240000}, /* 40 */
    {1, 400, 1101, 360000, 0, -10, 5, 9, 6, 240000}, /* 41 */
    {1, 400, 1101, 360000, 0, -10, 5, 9, 6, 240000}, /* 42 */
    {1, 400, 1101, 360000, 0, -10, 5, 9, 6, 240000}, /* 43 */
    {1, 400, 1101, 360000, 0, -10, 5, 9, 6, 240000}, /* 44 */
    {1, 400, 1101, 360000, 0, -10, 5, 9, 6, 240000}, /* 45 */
    {1, 400, 1101, 360000, 0, -10, 5, 9, 6, 240000}, /* 46 */
    {1, 400, 1101, 360000, 0, -10, 5, 9, 6, 240000}, /* 47 */
    {1, 400, 1101, 360000, 0, -10, 5, 9, 6, 240000}, /* 48 */
    {1, 400, 1101, 360000, 0, -10, 5, 9, 6, 240000}, /* 49 */
    {1, 400, 1101, 360000, 0, -10, 5, 9, 6, 240000}, /* 50 */
    {1, 400, 1101, 360000, 0, -10, 5, 9, 6, 240000}, /* 51 */
    {1, 400, 1101, 360000, 0, -10, 5, 9, 6, 240000}, /* 52 */
    {1, 400, 1101, 360000, 0, -10, 5, 9, 6, 240000}, /* 53 */
    {1, 400, 1101, 360000, 0, -10, 5, 9, 6, 240000}, /* 54 */
    {1, 400, 1101, 360000, 0, -10, 5, 9, 6, 240000}, /* 55 */
    {1, 400, 1101, 360000, 0, -10, 5, 9, 6, 240000}, /* 56 */
    {1, 400, 1101, 360000, 0, -10, 5, 9, 6, 240000}, /* 57 */
    {1, 400, 1101, 360000, 0, -10, 5, 9, 6, 240000}, /* 58 */
    {1, 400, 1101, 360000, 0, -10, 5, 9, 6, 240000}, /* 59 */
    {1, 400, 1101, 360000, 0, -10, 5, 9, 6, 240000}, /* 60 */
    {1, 400, 1101, 360000, 0, -10, 5, 9, 6, 240000}  /* 61 */
};

const char *hit_prompt[] = {"excellent",        /* => 100% */
                            "scratched",        /* => 90% */
                            "small bruises",    /* => 80% */
                            "small wounds",     /* =>  70% */
                            "light wounds",     /* => 60% */
                            "wounded",          /* => 50% */
                            "heavy wounds",     /* => 40% */
                            "pretty hurt",      /* => 30% */
                            "awful",            /* => 20% */
                            "bleeding",         /* => 10% */
                            "mortally wounded", /* =< 0% */
                            "\n"};

/* Directions, has arrived from x */

const char *arrived_from[] = {"the south.", "the west.", "the north.", "the east.", "below.", "above.", "\n"};

/* Weapon types */
const char *weapon_types[] = {"Undefined",    "Shortsword", "Broadsword", "Longsword", "Handaxe",      "Battleaxe",
                              "Quarterstaff", "Polearm",    "Mace",       "Warhammer", "Morning star", "Club",
                              "Dagger",       "Whip",       "Two-handed", "Bow",       "Spear",        "\n"};

/* weapon handedness */
const char *weapon_handed[] = {"1h Bludgeon", "1h Misc", "1h Piercing", "1h Slashing",
                               "2h Bludgeon", "2h Misc", "2h Slashing", "\n"};

/* material types for objects */
const char *material_types[] = {"Undefined", "Other", "Food",    "Cloth",    "Plant",   "Wood",   "Stone",  "Granite",
                                "Iron",      "Steel", "Mithril", "Titanium", "Copper",  "Bronze", "Silver", "Gold",
                                "Diamond",   "Ice",   "Glass",   "Paper",    "Leather", "Ivory",  "Ebony",  "Flesh",
                                "Skin",      "Bone",  "Water",   "Crystal",  "Earth",   "Light",  "\n"};

/* cardinal directions */
const char *dirs[] = {"north", "east", "south", "west", "up", "down", "\n"};

/* Default whosets for levels 31-35 */

const char *who_sets[] = {"-+IMMORTAL+-", "\n"};

/* Size names for NPCs */

const char *size_names[] = {"UNDEFINED", "VERY SMALL", "SMALL", "MEDIUM", "LARGE", "VERY LARGE", "GIANT", "\n"};

/* ROOM_x */
const char *room_bits[] = {"DARK",   "DEATH",  "!MOB",      "INDOORS",  "PEACEFUL", "SOUNDPROOF",
                           "!TRACK", "!MAGIC", "TUNNEL",    "PRIVATE",  "GODROOM",  "HOUSE",
                           "HCRSH",  "ATRIUM", "OLC",       "*", /* BFS MARK */
                           "COLD",   "HOT",    "!TELEPORT", "FASTHEAL", "CRIMEOK",  "UNAFFECT",
                           "NOHEAL", "HARM",   "LIGHT",     "NOCAMP",   "\n"};

/* EX_x */
const char *exit_bits[] = {"DOOR", "CLOSED", "LOCKED", "PICKPROOF", "HIDDEN", "\n"};

/* SECT_ */
const char *sector_types[] = {"Inside",     "City",      "Field",        "Forest",
                              "Hills",      "Mountains", "Water (Swim)", "Water (No Swim)",
                              "Underwater", "In Flight", "Underground",  "\n"};

/* SEX_x */
const char *genders[] = {"Neutral", "Male", "Female"};

/* POS_x */
const char *position_types[] = {"Dead",    "Mortally wounded", "Incapacitated", "Stunned",  "Sleeping",
                                "Resting", "Sitting",          "Fighting",      "Standing", "\n"};

/* PLR_x */
const char *player_bits[] = {"KILLER",  "THIEF",      "FROZEN",  "DONTSET",  "WRITING", "MAILING", "CSH",
                             "SITEOK",  "NOSHOUT",    "NOTITLE", "DELETED",  "LOADRM",  "!WIZL",   "!DEL",
                             "INVST",   "CRYO",       "KIT",     "OUTLAW",   "SPEC",    "ZONEOK",  "EDITING",
                             "STAYGOD", "UNRESTRICT", "NOEMOTE", "NOSOCIAL", "\n"};

/* COM_x */
const char *com_bits[] = {"IMMORT", "QUEST", "BUILDER", "ADMIN", "MOB", "\n"};

/* MOB_x */
const char *action_bits[] = {"SPEC",       "SENTINEL",  "SCAVENGER",   "ISNPC",      "AWARE",        "AGGR",
                             "STAY-ZONE",  "WIMPY",     "AGGR_EVIL",   "AGGR_GOOD",  "AGGR_NEUTRAL", "MEMORY",
                             "HELPER",     "!CHARM",    "!SUMMN",      "!SLEEP",     "!BASH",        "!BLIND",
                             "WATERONLY",  "MOUNTABLE", "PROGALWAYS",  "WRAITHLIKE", "NOKILL",       "HAS_MAGE",
                             "HAS_CLERIC", "HAS_THIEF", "HAS_WARRIOR", "STAY_PUT",   "TRACKER",      "\n"};

/* mob_progs */
const char *mobprog_types[] = {"INFILE",    "ACT",  "SPEECH", "RAND",  "FIGHT",  "DEATH", "HITPRCNT", "ENTRY", "GREET",
                               "ALL_GREET", "GIVE", "BRIBE",  "SHOUT", "HOLLER", "TELL",  "TIME",     "ASK",   "\n"};

/* PRF_x */
const char *preference_bits[] = {"BRIEF",   "COMPACT", "!SHOUT",   "!TELL",  "AUTOLOOT", "AUTOGOLD", "AUTOSPLIT",
                                 "AUTOEX",  "!HASS",   "ACCEPTED", "!SUMN",  "!REPEAT",  "LIGHT",    "C1",
                                 "C2",      "!WIZ",    "L1",       "BRIEF2", "!AUC",     "!CHAT",    "!GRATZ",
                                 "RMFLG",   "AFK",     "WHOIS",    "TICKER", "!CITIZEN", "ANON",     "!HOLLER",
                                 "!IQUEST", "MOBDEAF", "DELETED",  "\n"};

/* AFF_x */
const char *affected_bits[] = {"BLIND",     "INVIS",       "DET-ALIGN",  "DET-INVIS", "DET-MAGIC",  "SENSE-LIFE",
                               "WATWALK",   "MAJOR_GLOBE", "GROUP",      "CURSE",     "INFRA",      "POISON",
                               "PROT-EVIL", "PROT-GOOD",   "SLEEP",      "!TRACK",    "FLY",        "NIGHTVISION",
                               "SNEAK",     "HIDE",        "SILENCE",    "CHARM",     "DISEASE",    "MAJOR-PARA",
                               "SEEING",    "SUPERINVIS",  "DET-POISON", "TEACHING",  "MEDITATING", "CAMPING",
                               "AID",       "BLESS",       "\n"};

/* AFF2_x */
const char *affected_bits2[] = {"CASTING",    "FARSEE",     "ANTIMAGIC", "STONESKIN",    "FIRESHIELD", "WRAITHFORM",
                                "VAMP-TOUCH", "SLOWNESS",   "HASTE",     "LEVITATE",     "PROT-LIGHT", "PROT-ACID",
                                "PROT-ICE",   "PROT-GAS",   "PROT-FIRE", "OGRESTRENGTH", "WITHERED",   "MINOR-PARA",
                                "BARKSKIN",   "KNOCKEDOUT", "ICESHIELD", "SCRIBING",     "MEMMING",    "ARMOR",
                                "FEEBLEMIND", "\n"};

/* AFF3_x */
const char *affected_bits3[] = {"FLAMEBLADE", "SPIRITUALHAMMER", "\n"};

/* CON_x */
const char *connected_types[] = {"Playing",       "Disconnecting",  "Get name",
                                 "Confirm name",  "Get password",   "Get new PW",
                                 "Confirm PW",    "Select sex",     "Select class",
                                 "Reading MOTD",  "Main Menu",      "Get descript.",
                                 "Changing PW 1", "Changing PW 2",  "Changing PW 3",
                                 "Self-Delete 1", "Self-Delete 2",  "Select Race",
                                 "OLC Object",    "OLC Room",       "OLC Mob",
                                 "OLC Shop",      "OLC Zone",       "Ident conning",
                                 "Ident conned",  "Ident reading",  "Ident read",
                                 "Asking name",   "Newbie",         "Reconnect as",
                                 "Rolling Stats", "Getting TType",  "Text Editor",
                                 "Getting Align", "Getting Home",   "Accept/Decline",
                                 "Get New Name",  "Reading Policy", "\n"};

/* WEAR_x - for eq list */
const char *where[] = {
    "<worn as badge>      ", /*  0 */
    "<worn on finger>     ", /*  1 */
    "<worn on finger>     ", /*  2 */
    "<worn around neck>   ", /*  3 */
    "<worn around neck>   ", /*  4 */
    "<worn on body>       ", /*  5 */
    "<worn on head>       ", /*  6 */
    "<worn on legs>       ", /*  7 */
    "<worn on feet>       ", /*  8 */
    "<worn on hands>      ", /*  9 */
    "<worn on arms>       ", /* 10 */
    "<worn as shield>     ", /* 11 */
    "<worn about body>    ", /* 12 */
    "<worn about waist>   ", /* 13 */
    "<worn around wrist>  ", /* 14 */
    "<worn around wrist>  ", /* 15 */
    "<wielded>            ", /* 16 */
    "<held>               ", /* 17 */
    "<worn on face>       ", /* 18 */
    "<used as earring>    ", /* 19 */
    "<used as earring>    ", /* 20 */
    "<worn on eyes>       ", /* 21 */
    "<worn on ankle>      ", /* 22 */
    "<worn on ankle>      ", /* 23 */
    "<wielded>            ", /* 24 */
    "<held>               ", /* 25 */
    "<wielded two-handed> "  /* 26 */
};

/* WEAR_x - for stat */
const char *equipment_types[] = {"Worn as badge",
                                 "Worn on right finger",
                                 "Worn on left finger",
                                 "First worn around Neck",
                                 "Second worn around Neck",
                                 "Worn on body",
                                 "Worn on head",
                                 "Worn on legs",
                                 "Worn on feet",
                                 "Worn on hands",
                                 "Worn on arms",
                                 "Worn as shield",
                                 "Worn about body",
                                 "Worn around waist",
                                 "Worn around right wrist",
                                 "Worn around left wrist",
                                 "Wielded",
                                 "Held",
                                 "Worn on face",
                                 "Used as earring in left ear",
                                 "Used as earring in right ear",
                                 "Worn on eyes",
                                 "Worn on right ankle",
                                 "Worn on left ankle",
                                 "Wielded",
                                 "Held",
                                 "Wielded two-handed",
                                 "\n"};

/* ITEM_x (ordinal object types) */
const char *item_types[] = {"UNDEFINED",   "LIGHT",   "SCROLL",   "WAND",      "STAFF",  "WEAPON",
                            "FIRE WEAPON", "MISSILE", "TREASURE", "ARMOR",     "POTION", "WORN",
                            "OTHER",       "TRASH",   "TRAP",     "CONTAINER", "NOTE",   "LIQ CONTAINER",
                            "KEY",         "FOOD",    "MONEY",    "PEN",       "BOAT",   "FOUNTAIN",
                            "INSTRUMENT",  "BADGE",   "PCORPSE",  "SPELLBOOK", "PORTAL", "\n"};

/* ITEM_WEAR_ (wear bitvector) */
const char *wear_bits[] = {"TAKE", "FINGER", "NECK",   "BODY",  "HEAD",    "LEGS",  "FEET", "HANDS",
                           "ARMS", "SHIELD", "ABOUT",  "WAIST", "WRIST",   "WIELD", "HOLD", "FACE",
                           "EAR",  "EYES",   "ANKLES", "BADGE", "2HANDED", "\n"};

/* WORN_ (wear bitvector) */
const char *worn_bits[] = {"BADGE",   "FINGER_R", "FINGER_L", "NECK_1",  "NECK_2", "BODY",    "HEAD",
                           "LEGS",    "FEET",     "HANDS",    "ARMS",    "SHIELD", "ABOUT",   "WAIST",
                           "WRIST_R", "WRIST_L",  "WIELD",    "HOLD",    "FACE",   "EAR_R",   "EAR_L",
                           "EYES",    "ANKLE_R",  "ANKLE_L",  "WIELD_2", "HOLD_2", "2HANDED", "\n"};
/* ITEM_x (extra bits) */
const char *extra_bits[] = {"GLOW",        "HUM",       "!RENT",      "!DONATE",    "!INVIS",    "INVISIBLE",
                            "MAGIC",       "!DROP",     "BLESS",      "!GOOD",      "!EVIL",     "!NEUTRAL",
                            "!MAGE",       "!CLERIC",   "!THIEF",     "!WARRIOR",   "!SELL",     "DONATED",
                            "NOAUCTION",   "CARRIED",   "ISLIGHT",    "ISLIGHTDIM", "POISONED",  "FLOAT",
                            "NOT_OBVIOUS", "!IDENTIFY", "UNDERWATER", "NOBURN",     "ENCHANTED", "\n"};

/* APPLY_x */
const char *apply_types[] = {"NONE",
                             "STR",
                             "DEX",
                             "INT",
                             "WIS",
                             "CON",
                             "AGI",
                             "CLASS",
                             "LEVEL",
                             "AGE",
                             "CHAR_WEIGHT",
                             "CHAR_HEIGHT",
                             "MANA",
                             "HIT",
                             "MOVE",
                             "GOLD",
                             "EXP",
                             "ARMOR",
                             "HITROLL",
                             "DAMROLL",
                             "SAVING_PARA",
                             "SAVING_ROD",
                             "SAVING_PETRI",
                             "SAVING_BREATH",
                             "SAVING_SPELL",
                             "MAX_HIT",
                             "MAX_MANA",
                             "MAX_MOVE",
                             "RES_LIGHT",
                             "RES_DARK",
                             "RES_FIRE",
                             "RES_COLD",
                             "RES_ACID",
                             "RES_POISON",
                             "RES_DISEASE",
                             "RES_CHARM",
                             "RES_SLEEP",
                             "RES_SLASH",
                             "RES_PIERCE",
                             "RES_BLUDGEON",
                             "RES_NWEAP",
                             "RES_MWEAP",
                             "RES_MAGIC",
                             "RES_ELECTRICITY",
                             "\n"};

/* CONT_x */
const char *container_bits[] = {
    "CLOSEABLE", "PICKPROOF", "CLOSED", "LOCKED", "\n",
};

/* LIQ_x */
const char *drinks[] = {"water",
                        "beer",
                        "wine",
                        "ale",
                        "dark ale",
                        "whisky",
                        "lemonade",
                        "firebreather",
                        "local speciality",
                        "slime mold juice",
                        "milk",
                        "tea",
                        "coffee",
                        "blood",
                        "salt water",
                        "clear water",
                        "coke",
                        "\n"};

/* other constants for liquids ******************************************/

/* one-word alias for each drink */
const char *drinknames[] = {"water", "beer", "wine", "ale",    "ale",   "whisky", "lemonade", "liquor", "local",
                            "juice", "milk", "tea",  "coffee", "blood", "salt",   "water",    "coke",   "\n"};

/* effect of drinks on hunger, thirst, and drunkenness -- see values.doc */
const int drink_aff[][3] = {
    /* DRUNK, FULL, THIRST */
    {0, 0, 3}, {0, 0, 3}, {0, 0, 3}, {0, 0, 3}, {0, 0, 3}, {0, 0, 1}, {0, 0, 1}, {0, 0, 1}, {0, 0, 4},
    {0, 0, 5}, {0, 0, 4}, {0, 0, 1}, {0, 0, 1}, {0, 0, 3}, {0, 0, 1}, {0, 0, 3}, {0, 0, 3}};

/* color of the various drinks */
const char *color_liquid[] = {
    "clear",       "brown", "clear", "brown", "dark", "golden", "red",           "green", "clear",
    "light green", "white", "brown", "black", "red",  "clear",  "crystal clear", "brown",
};

/* level of fullness for drink containers */
const char *fullness[] = {"less than half ", "about half ", "more than half ", ""};

/* str, int, wis, dex, con applies **************************************/

sh_int stats[12][101];

void init_stats() {
  int i = 0;

  /* STR_TOHIT */
  stats[STR_TOHIT][0] = 0; /* str 0 should not exist */
  for (i = 1; i < 101; i++)
    stats[STR_TOHIT][i] = ((i / 10) - 5);

  /* STR_TODAM */
  stats[STR_TODAM][0] = 0;
  for (i = 1; i < 101; i++)
    stats[STR_TODAM][i] = ((i / 10) - 5);

  /* STR_CWEIGHT */
  stats[STR_CWEIGHT][0] = 0;
  for (i = 1; i < 101; i++)
    stats[STR_CWEIGHT][i] = (i * 5);

  /* STR_WWEIGHT */
  stats[STR_WWEIGHT][0] = 0;
  for (i = 1; i < 101; i++)
    stats[STR_WWEIGHT][i] = (i / 3);

  /* WIS_PRAC -- set to 0 for now */
  stats[WIS_PRAC][0] = 0;
  for (i = 1; i < 101; i++)
    stats[WIS_PRAC][i] = 0;

  /* INT_LEARN -- % of a skill learned per prac */
  stats[INT_LEARN][0] = 0;
  for (i = 1; i < 101; i++) {
    if (i > 0)
      stats[INT_LEARN][i] = 1;
    if (i > 30)
      stats[INT_LEARN][i] = 2;
    if (i > 65)
      stats[INT_LEARN][i] = 3;
  }

  /* CON_HITP */
  stats[CON_HITP][0] = 0;
  for (i = 1; i < 101; i++)
    stats[CON_HITP][i] = ((i / 10) - 5);

  /* CON_SHOCK */
  stats[CON_SHOCK][0] = 0;
  for (i = 1; i < 101; i++) {
    if (i > 0)
      stats[CON_SHOCK][i] = (i + 20);
    if (i > 30)
      stats[CON_SHOCK][i] = (i + 30);
    if (i > 60)
      stats[CON_SHOCK][i] = 95;
    if (i > 70)
      stats[CON_SHOCK][i] = 98;
    if (i > 90)
      stats[CON_SHOCK][i] = 99;
  }

  /* DEX_REACT */
  stats[DEX_REACT][0] = 0;
  for (i = 1; i < 101; i++)
    stats[DEX_REACT][i] = ((i / 10) - 5);

  /* DEX_MISSATT */
  stats[DEX_MISSATT][0] = 0;
  for (i = 1; i < 101; i++)
    stats[DEX_MISSATT][i] = ((i / 10) - 5);

  /* DEX_SKILL */
  stats[DEX_SKILL][0] = 0;
  for (i = 1; i < 101; i++)
    stats[DEX_SKILL][i] = ((i / 5) - 10);

  /* AGI_DEFENSE */
  stats[AGI_DEFENSE][0] = 0;
  for (i = 1; i < 101; i++)
    stats[AGI_DEFENSE][i] = (((i - 50) / 3));

  return;
}

/* [ch] strength apply (all) */
const struct str_app_type str_app[100] = {
    {-5, -4, 0, 0},    /* 0 */
    {-5, -4, 3, 1},    /* 1 */
    {-3, -2, 3, 2},    /* 2 */
    {-3, -1, 10, 3},   /* 3 */
    {-2, -1, 25, 4},   /* 4 */
    {-2, -1, 55, 5},   /* 5 */
    {-1, 0, 80, 6},    /* 6 */
    {-1, 0, 90, 7},    /* 7 */
    {0, 0, 100, 8},    /* 8 */
    {0, 0, 100, 9},    /* 9 */
    {0, 0, 115, 10},   /* 10 */
    {0, 0, 115, 11},   /* 11 */
    {0, 0, 140, 12},   /* 12 */
    {0, 0, 140, 13},   /* 13 */
    {0, 0, 170, 14},   /* 14 */
    {0, 0, 170, 15},   /* 15 */
    {0, 1, 195, 16},   /* 16 */
    {1, 1, 220, 18},   /* 17 */
    {1, 2, 255, 20},   /* 18 */
    {3, 7, 640, 40},   /* 19 */
    {3, 8, 700, 40},   /* 20 */
    {4, 9, 810, 40},   /* 21 */
    {4, 10, 970, 40},  /* 22 */
    {5, 11, 1130, 40}, /* 23 */
    {6, 12, 1440, 40}, /* 24 */
    {7, 14, 1750, 40}, /* 25 */
    {1, 3, 280, 22},   /* 26 */
    {2, 3, 305, 24},   /* 27 */
    {2, 4, 330, 26},   /* 28 */
    {2, 5, 380, 28},   /* 29 */
    {3, 6, 480, 30},   /* 30 */
    {3, 6, 480, 30},   /* 31 */
    {3, 6, 480, 30},   /* 32 */
    {3, 6, 480, 30},   /* 33 */
    {3, 6, 480, 30},   /* 34 */
    {3, 6, 480, 30},   /* 35 */
    {3, 6, 480, 30},   /* 36 */
    {3, 6, 480, 30},   /* 37 */
    {3, 6, 480, 30},   /* 38 */
    {3, 6, 480, 30},   /* 39 */
    {3, 6, 480, 30},   /* 40 */
    {3, 6, 480, 30},   /* 41 */
    {3, 6, 480, 30},   /* 42 */
    {3, 6, 480, 30},   /* 43 */
    {3, 6, 480, 30},   /* 44 */
    {3, 6, 480, 30},   /* 45 */
    {3, 6, 480, 30},   /* 46 */
    {3, 6, 480, 30},   /* 47 */
    {3, 6, 480, 30},   /* 48 */
    {3, 6, 480, 30},   /* 49 */
    {3, 6, 480, 30},   /* 50 */
    {3, 6, 480, 30},   /* 51 */
    {3, 6, 480, 30},   /* 52 */
    {3, 6, 480, 30},   /* 53 */
    {3, 6, 480, 30},   /* 54 */
    {3, 6, 480, 30},   /* 55 */
    {3, 6, 480, 30},   /* 56 */
    {3, 6, 480, 30},   /* 57 */
    {3, 6, 480, 30},   /* 58 */
    {3, 6, 480, 30},   /* 59 */
    {3, 6, 480, 30},   /* 60 */
};

/* [dex] skill apply (thieves only) */
const struct dex_skill_type dex_app_skill[61] = {
    {-99, -99, -90, -99, -60}, /* 0 */
    {-90, -90, -60, -90, -50}, /* 1 */
    {-80, -80, -40, -80, -45}, /* 2 */
    {-70, -70, -30, -70, -40}, /* 3 */
    {-60, -60, -30, -60, -35}, /* 4 */
    {-50, -50, -20, -50, -30}, /* 5 */
    {-40, -40, -20, -40, -25}, /* 6 */
    {-30, -30, -15, -30, -20}, /* 7 */
    {-20, -20, -15, -20, -15}, /* 8 */
    {-15, -10, -10, -20, -10}, /* 9 */
    {-10, -5, -10, -15, -5},   /* 10 */
    {-5, 0, -5, -10, 0},       /* 11 */
    {0, 0, 0, -5, 0},          /* 12 */
    {0, 0, 0, 0, 0},           /* 13 */
    {0, 0, 0, 0, 0},           /* 14 */
    {0, 0, 0, 0, 0},           /* 15 */
    {0, 5, 0, 0, 0},           /* 16 */
    {5, 10, 0, 5, 5},          /* 17 */
    {10, 15, 5, 10, 10},       /* 18 */
    {15, 20, 10, 15, 15},      /* 19 */
    {15, 20, 10, 15, 15},      /* 20 */
    {20, 25, 10, 15, 20},      /* 21 */
    {20, 25, 15, 20, 20},      /* 22 */
    {25, 25, 15, 20, 20},      /* 23 */
    {25, 30, 15, 25, 25},      /* 24 */
    {25, 30, 15, 25, 25},      /* 25 */
    {30, 30, 20, 25, 25},      /* 26 */
    {30, 30, 20, 25, 25},      /* 27 */
    {30, 35, 20, 30, 30},      /* 28 */
    {30, 35, 20, 30, 30},      /* 29 */
    {35, 35, 25, 30, 30},      /* 30 */
    {35, 35, 25, 35, 35},      /* 31 */
    {35, 35, 25, 35, 35},      /* 32 */
    {35, 40, 25, 35, 35},      /* 33 */
    {35, 40, 30, 35, 35},      /* 34 */
    {40, 40, 30, 35, 35},      /* 35 */
    {40, 40, 30, 35, 35},      /* 36 */
    {40, 40, 30, 35, 35},      /* 37 */
    {40, 40, 30, 35, 35},      /* 38 */
    {40, 40, 30, 35, 35},      /* 39 */
    {40, 40, 30, 35, 35},      /* 40 */
    {40, 40, 30, 35, 35},      /* 41 */
    {40, 40, 30, 35, 35},      /* 42 */
    {40, 40, 30, 35, 35},      /* 43 */
    {40, 40, 30, 35, 35},      /* 44 */
    {40, 40, 30, 35, 35},      /* 45 */
    {40, 40, 30, 35, 35},      /* 46 */
    {40, 40, 30, 35, 35},      /* 47 */
    {40, 40, 30, 35, 35},      /* 48 */
    {40, 40, 30, 35, 35},      /* 49 */
    {40, 40, 30, 35, 35},      /* 50 */
    {40, 40, 30, 35, 35},      /* 51 */
    {40, 40, 30, 35, 35},      /* 52 */
    {40, 40, 30, 35, 35},      /* 53 */
    {40, 40, 30, 35, 35},      /* 54 */
    {40, 40, 30, 35, 35},      /* 55 */
    {40, 40, 30, 35, 35},      /* 56 */
    {40, 40, 30, 35, 35},      /* 57 */
    {40, 40, 30, 35, 35},      /* 58 */
    {40, 40, 30, 35, 35},      /* 59 */
    {40, 40, 30, 35, 35}       /* 60 */
};

/* [level] backstab multiplyer (thieves only) */
const byte backstab_mult[61] = {
    1, /* 0 */
    2, /* 1 */
    2, /* 2 */
    2, /* 3 */
    2, /* 4 */
    2, /* 5 */
    2, /* 6 */
    2, /* 7 */
    2, /* 8 */
    2, /* 9 */
    2, /* 10 */
    2, /* 11 */
    2, /* 12 */
    2, /* 13 */
    2, /* 14 */
    2, /* 15 */
    2, /* 16 */
    2, /* 17 */
    2, /* 18 */
    2, /* 19 */
    2, /* 20 */
    2, /* 21 */
    3, /* 22 */
    3, /* 23 */
    3, /* 24 */
    3, /* 25 */
    3, /* 26 */
    3, /* 27 */
    3, /* 28 */
    3, /* 29 */
    3, /* 30 */
    3, /* 31 */
    3, /* 32 */
    3, /* 33 */
    3, /* 34 */
    3, /* 35 */
    3, /* 36 */
    3, /* 37 */
    3, /* 38 */
    3, /* 39 */
    3, /* 40 */
    3, /* 41 */
    4, /* 42 */
    4, /* 43 */
    4, /* 44 */
    4, /* 45 */
    4, /* 46 */
    4, /* 47 */
    4, /* 48 */
    4, /* 49 */
    4, /* 50 */
    4, /* 51 */
    4, /* 52 */
    4, /* 53 */
    4, /* 54 */
    4, /* 55 */
    4, /* 56 */
    5, /* 57 */
    5, /* 58 */
    5, /* 59 */
    5  /* 60 */
};

/* [level] backstab multiplyer (thieves only) */
const byte backstab_asn[61] = {
    1, /* 0 */
    2, /* 1 */
    2, /* 2 */
    2, /* 3 */
    2, /* 4 */
    2, /* 5 */
    2, /* 6 */
    2, /* 7 */
    2, /* 8 */
    2, /* 9 */
    3, /* 10 */
    3, /* 11 */
    3, /* 12 */
    3, /* 13 */
    3, /* 14 */
    3, /* 15 */
    3, /* 16 */
    3, /* 17 */
    3, /* 18 */
    3, /* 19 */
    3, /* 20 */
    3, /* 21 */
    3, /* 22 */
    3, /* 23 */
    3, /* 24 */
    4, /* 25 */
    4, /* 26 */
    4, /* 27 */
    4, /* 28 */
    4, /* 29 */
    4, /* 30 */
    4, /* 31 */
    4, /* 32 */
    4, /* 33 */
    5, /* 34 */
    5, /* 35 */
    5, /* 36 */
    5, /* 37 */
    5, /* 38 */
    5, /* 39 */
    5, /* 40 */
    5, /* 41 */
    5, /* 42 */
    5, /* 43 */
    5, /* 44 */
    5, /* 45 */
    5, /* 46 */
    6, /* 47 */
    6, /* 48 */
    6, /* 49 */
    6, /* 50 */
    6, /* 51 */
    6, /* 52 */
    6, /* 53 */
    6, /* 54 */
    6, /* 55 */
    6, /* 56 */
    7, /* 57 */
    7, /* 58 */
    7, /* 59 */
    7  /* 60 */
};

/* [dex] apply (all) */
struct dex_app_type dex_app[61] = {
    {-7, -7, 6}, /* 0 */
    {-6, -6, 5}, /* 1 */
    {-4, -4, 5}, /* 2 */
    {-3, -3, 4}, /* 3 */
    {-2, -2, 3}, /* 4 */
    {-1, -1, 2}, /* 5 */
    {0, 0, 1},   /* 6 */
    {0, 0, 0},   /* 7 */
    {0, 0, 0},   /* 8 */
    {0, 0, 0},   /* 9 */
    {0, 0, 0},   /* 10 */
    {0, 0, 0},   /* 11 */
    {0, 0, 0},   /* 12 */
    {0, 0, 0},   /* 13 */
    {0, 0, 0},   /* 14 */
    {0, 0, -1},  /* 15 */
    {1, 1, -2},  /* 16 */
    {2, 2, -3},  /* 17 */
    {2, 2, -4},  /* 18 */
    {3, 3, -4},  /* 19 */
    {3, 3, -4},  /* 20 */
    {4, 4, -5},  /* 21 */
    {4, 4, -5},  /* 22 */
    {4, 4, -5},  /* 23 */
    {5, 5, -6},  /* 24 */
    {5, 5, -6},  /* 25 */
    {5, 5, -6},  /* 26 */
    {5, 5, -6},  /* 27 */
    {6, 6, -7},  /* 28 */
    {6, 6, -7},  /* 29 */
    {6, 6, -7},  /* 30 */
    {6, 6, -7},  /* 31 */
    {7, 7, -8},  /* 32 */
    {7, 7, -8},  /* 33 */
    {7, 7, -8},  /* 34 */
    {7, 7, -6},  /* 35 */
    {7, 7, -6},  /* 36 */
    {7, 7, -6},  /* 37 */
    {7, 7, -6},  /* 38 */
    {7, 7, -6},  /* 39 */
    {7, 7, -6},  /* 40 */
    {7, 7, -6},  /* 41 */
    {7, 7, -6},  /* 42 */
    {7, 7, -6},  /* 43 */
    {7, 7, -6},  /* 44 */
    {7, 7, -6},  /* 45 */
    {7, 7, -6},  /* 46 */
    {7, 7, -6},  /* 47 */
    {7, 7, -6},  /* 48 */
    {7, 7, -6},  /* 49 */
    {7, 7, -6},  /* 50 */
    {7, 7, -6},  /* 51 */
    {7, 7, -6},  /* 52 */
    {7, 7, -6},  /* 53 */
    {7, 7, -6},  /* 54 */
    {7, 7, -6},  /* 55 */
    {7, 7, -6},  /* 56 */
    {7, 7, -6},  /* 57 */
    {7, 7, -6},  /* 58 */
    {7, 7, -6},  /* 59 */
    {7, 7, -6}   /* 60 */
};

/* [con] apply (all) */
struct con_app_type con_app[61] = {
    {-4, 20}, /* 0 */
    {-3, 25}, /* 1 */
    {-2, 30}, /* 2 */
    {-2, 35}, /* 3 */
    {-1, 40}, /* 4 */
    {-1, 45}, /* 5 */
    {-1, 50}, /* 6 */
    {0, 55},  /* 7 */
    {0, 60},  /* 8 */
    {0, 65},  /* 9 */
    {0, 70},  /* 10 */
    {0, 75},  /* 11 */
    {0, 80},  /* 12 */
    {0, 85},  /* 13 */
    {0, 88},  /* 14 */
    {1, 90},  /* 15 */
    {2, 95},  /* 16 */
    {2, 97},  /* 17 */
    {3, 99},  /* 18 */
    {3, 99},  /* 19 */
    {4, 99},  /* 20 */
    {5, 99},  /* 21 */
    {5, 99},  /* 22 */
    {5, 99},  /* 23 */
    {6, 99},  /* 24 */
    {6, 99},  /* 25 */
    {6, 100}, /* 26 */
    {7, 100}, /* 27 */
    {7, 100}, /* 28 */
    {7, 100}, /* 29 */
    {7, 100}, /* 30 */
    {8, 100}, /* 31 */
    {8, 100}, /* 32 */
    {8, 100}, /* 33 */
    {8, 100}, /* 34 */
    {9, 100}, /* 35 */
    {9, 100}, /* 36 */
    {9, 100}, /* 37 */
    {9, 100}, /* 38 */
    {9, 100}, /* 39 */
    {9, 100}, /* 40 */
    {9, 100}, /* 41 */
    {9, 100}, /* 42 */
    {9, 100}, /* 43 */
    {9, 100}, /* 44 */
    {9, 100}, /* 45 */
    {9, 100}, /* 46 */
    {9, 100}, /* 47 */
    {9, 100}, /* 48 */
    {9, 100}, /* 49 */
    {9, 100}, /* 50 */
    {9, 100}, /* 51 */
    {9, 100}, /* 52 */
    {9, 100}, /* 53 */
    {9, 100}, /* 54 */
    {9, 100}, /* 55 */
    {9, 100}, /* 56 */
    {9, 100}, /* 57 */
    {9, 100}, /* 58 */
    {9, 100}, /* 59 */
    {9, 100}  /* 60 */
};

/* [int] apply (all) */
struct int_app_type int_app[61] = {
    {3},  /* 0 */
    {5},  /* 1 */
    {7},  /* 2 */
    {8},  /* 3 */
    {9},  /* 4 */
    {10}, /* 5 */
    {11}, /* 6 */
    {12}, /* 7 */
    {13}, /* 8 */
    {15}, /* 9 */
    {17}, /* 10 */
    {19}, /* 11 */
    {22}, /* 12 */
    {25}, /* 13 */
    {30}, /* 14 */
    {35}, /* 15 */
    {40}, /* 16 */
    {45}, /* 17 */
    {50}, /* 18 */
    {53}, /* 19 */
    {55}, /* 20 */
    {56}, /* 21 */
    {57}, /* 22 */
    {58}, /* 23 */
    {59}, /* 24 */
    {60}, /* 25 */
    {61}, /* 26 */
    {62}, /* 27 */
    {63}, /* 28 */
    {64}, /* 29 */
    {65}, /* 30 */
    {66}, /* 31 */
    {69}, /* 32 */
    {70}, /* 33 */
    {80}, /* 34 */
    {99}, /* 35 */
    {99}, /* 36 */
    {99}, /* 37 */
    {99}, /* 38 */
    {99}, /* 39 */
    {99}, /* 40 */
    {99}, /* 41 */
    {99}, /* 42 */
    {99}, /* 43 */
    {99}, /* 44 */
    {99}, /* 45 */
    {99}, /* 46 */
    {99}, /* 47 */
    {99}, /* 48 */
    {99}, /* 49 */
    {99}, /* 50 */
    {99}, /* 51 */
    {99}, /* 52 */
    {99}, /* 53 */
    {99}, /* 54 */
    {99}, /* 55 */
    {99}, /* 56 */
    {99}, /* 57 */
    {99}, /* 58 */
    {99}, /* 59 */
    {99}  /* 60 */
};

/* [wis] apply (all) */
struct wis_app_type wis_app[61] = {
    {0}, /* 0 */
    {0}, /* 1 */
    {0}, /* 2 */
    {0}, /* 3 */
    {0}, /* 4 */
    {0}, /* 5 */
    {0}, /* 6 */
    {0}, /* 7 */
    {0}, /* 8 */
    {0}, /* 9 */
    {0}, /* 10 */
    {0}, /* 11 */
    {2}, /* 12 */
    {2}, /* 13 */
    {3}, /* 14 */
    {3}, /* 15 */
    {3}, /* 16 */
    {4}, /* 17 */
    {5}, /* 18 */
    {6}, /* 19 */
    {6}, /* 20 */
    {6}, /* 21 */
    {6}, /* 22 */
    {7}, /* 23 */
    {7}, /* 24 */
    {7}, /* 25 */
    {7}, /* 26 */
    {8}, /* 27 */
    {8}, /* 28 */
    {8}, /* 29 */
    {8}, /* 30 */
    {8}, /* 31 */
    {8}, /* 32 */
    {8}, /* 33 */
    {9}, /* 34 */
    {9}, /* 35 */
    {9}, /* 36 */
    {9}, /* 37 */
    {9}, /* 38 */
    {9}, /* 39 */
    {9}, /* 40 */
    {9}, /* 41 */
    {9}, /* 42 */
    {9}, /* 43 */
    {9}, /* 44 */
    {9}, /* 45 */
    {9}, /* 46 */
    {9}, /* 47 */
    {9}, /* 48 */
    {9}, /* 49 */
    {9}, /* 50 */
    {9}, /* 51 */
    {9}, /* 52 */
    {9}, /* 53 */
    {9}, /* 54 */
    {9}, /* 55 */
    {9}, /* 56 */
    {9}, /* 57 */
    {9}, /* 58 */
    {9}, /* 59 */
    {9}  /* 60 */
};

const char *npc_race_names[] = {"Undefined", "Human", "Troll",    "Ogre",   "Dwarf",     "Elf",
                                "Half-Elf",  "Gnome", "Halfling", "Drow",   "Draconian", "Dragon",
                                "Minotaur",  "Orc",   "Animal",   "Insect", "Plant",     "\n"};

const char *npc_class_types[] = {"Undefined",    "Other",  "Undead", "Humanoid",  "Animal", "Dragon",
                                 "Giant",        "Spirit", "Insect", "Elemental", "Fish",   "Amphibian",
                                 "Extra-planar", "Bird",   "Slime",  "Plant",     "\n"};

const int rev_dir[] = {2, 3, 0, 1, 5, 4};

const int movement_loss[] = {
    1, /* Inside     */
    1, /* City       */
    2, /* Field      */
    3, /* Forest     */
    4, /* Hills      */
    6, /* Mountains  */
    4, /* Swimming   */
    1, /* Unswimable */
    1, /* Fly */
    1  /* Underground */
};

const char *weekdays[7] = {"the Day of the Moon", "the Day of the Bull", "the Day of the Deception",
                           "the Day of Thunder",  "the Day of Freedom",  "the day of the Great Gods",
                           "the Day of the Sun"};

const char *month_name[17] = {"Month of Winter", /* 0 */
                              "Month of the Winter Wolf",
                              "Month of the Frost Giant",
                              "Month of the Old Forces",
                              "Month of the Grand Struggle",
                              "Month of the Spring",
                              "Month of Nature",
                              "Month of Futility",
                              "Month of the Dragon",
                              "Month of the Sun",
                              "Month of the Heat",
                              "Month of the Battle",
                              "Month of the Dark Shades",
                              "Month of the Shadows",
                              "Month of the Long Shadows",
                              "Month of the Ancient Darkness",
                              "Month of the Great Evil"};

const int sharp[] = {
    0, 0, 0, 0, /* Slashing */
    0, 0, 0, 0, /* Bludgeon */
    0, 0, 0, 0  /* Pierce   */
};

/* used by medit */

const struct max_mob_stats mob_stats[] = {
    /* level, hp_dice, hp_sides, hp_bonus, exp, gold, thac0, ac, damage */
    {0, 1, 1, 5, 20, 10, 20, 10, 1},
    {1, 1, 1, 10, 150, 50, 19, 9, 1},
    {2, 1, 1, 20, 300, 150, 19, 9, 2},
    {3, 2, 2, 30, 600, 190, 18, 8, 3},
    {4, 2, 2, 40, 1000, 270, 18, 8, 4},
    {5, 3, 3, 50, 1600, 350, 17, 7, 5},
    {6, 3, 3, 60, 2400, 450, 17, 7, 6},
    {7, 4, 4, 70, 2900, 540, 16, 6, 7},
    {8, 4, 4, 80, 3500, 600, 16, 6, 8},
    {9, 5, 5, 90, 4200, 690, 15, 5, 9},
    {10, 5, 5, 100, 5000, 800, 15, 5, 10},
    {11, 6, 6, 110, 6900, 880, 14, 4, 11},
    {12, 6, 6, 120, 7500, 950, 14, 4, 12},
    {13, 7, 7, 130, 8700, 1000, 14, 3, 13},
    {14, 7, 7, 140, 9900, 1080, 14, 3, 14},
    {15, 8, 8, 150, 11000, 1150, 14, 2, 15},
    {16, 8, 8, 160, 12500, 1200, 13, 2, 16},
    {17, 9, 9, 170, 14300, 1280, 13, 1, 17},
    {18, 9, 9, 180, 16800, 1340, 13, 1, 18},
    {19, 10, 10, 190, 20000, 1430, 12, 0, 19},
    {20, 10, 10, 200, 25000, 1500, 12, 0, 20},
    {21, 11, 11, 210, 27500, 1600, 12, -1, 21},
    {22, 11, 11, 220, 31000, 1750, 11, -1, 22},
    {23, 12, 12, 230, 35000, 2000, 11, -2, 23},
    {24, 12, 12, 240, 40000, 2200, 11, -2, 24},
    {25, 13, 13, 250, 45500, 2650, 10, -3, 25},
    {26, 13, 13, 260, 51000, 3000, 10, -3, 26},
    {27, 14, 14, 270, 55500, 3400, 10, -4, 27},
    {28, 14, 14, 280, 61000, 3900, 9, -4, 28},
    {29, 15, 15, 290, 70000, 4400, 9, -5, 29},
    {30, 15, 15, 300, 80000, 5000, 9, -5, 30},
    {31, 16, 16, 310, 85000, 5100, 8, -6, 31},
    {32, 16, 16, 320, 92000, 5250, 8, -6, 32},
    {33, 17, 17, 330, 99000, 5500, 8, -7, 33},
    {34, 17, 17, 340, 106000, 5800, 7, -7, 34},
    {35, 18, 18, 350, 112000, 6100, 7, -8, 35},
    {36, 18, 18, 360, 118000, 6500, 7, -8, 36},
    {37, 19, 19, 370, 124000, 6900, 6, -9, 37},
    {38, 19, 19, 380, 132000, 7200, 6, -9, 38},
    {39, 20, 20, 390, 140000, 7600, 6, -10, 39},
    {40, 20, 20, 400, 150000, 8000, 5, -10, 40},
    {41, 21, 21, 410, 165000, 8100, 5, -10, 41},
    {42, 21, 21, 420, 190000, 8200, 5, -10, 42},
    {43, 22, 22, 430, 220000, 8300, 4, -10, 43},
    {44, 22, 22, 440, 250000, 8500, 4, -10, 44},
    {45, 23, 23, 450, 280000, 8700, 4, -10, 45},
    {46, 23, 23, 460, 310000, 8900, 3, -10, 46},
    {47, 24, 24, 470, 350000, 9100, 3, -10, 47},
    {48, 24, 24, 480, 390000, 9400, 3, -10, 48},
    {49, 25, 25, 490, 440000, 9700, 2, -10, 49},
    {50, 25, 25, 500, 500000, 10000, 2, -10, 50},
    {51, 26, 26, 1510, 1500000, 20000, 2, -10, 99999},
    {52, 26, 26, 2520, 2600000, 20000, 1, -10, 99999},
    {53, 27, 27, 3530, 3700000, 20000, 1, -10, 99999},
    {54, 27, 27, 4540, 4800000, 20000, 1, -10, 99999},
    {55, 28, 28, 5550, 6000000, 20000, 0, -10, 99999},
    {56, 28, 28, 7650, 7000000, 20000, 0, -10, 99999},
    {57, 29, 29, 9650, 8000000, 20000, 0, -10, 99999},
    {58, 29, 29, 13580, 9000000, 20000, 0, -10, 99999},
    {59, 30, 30, 19000, 9500000, 20000, 0, -10, 99999},
    {60, 30, 30, 30000, 10000000, 20000, 0, -10, 99999}};

char *COLOR_TABLE[] = {CLEAR,    C_RED,     C_GREEN, C_YELLOW,  C_BLUE,     C_MAGENTA, C_CYAN,
                       C_WHITE,  C_D_GREY,  C_B_RED, C_B_GREEN, C_B_YELLOW, C_B_BLUE,  C_B_MAGENTA,
                       C_B_CYAN, C_B_WHITE, FLASH,   UNDERLINE, "{"};

const struct transport_data transport_list[MAX_TRANSPORT] = {
    /* SCMD, Obj vnum, inside vnum, exit1 vnum, exit2 vnum */
    {1, 3499, 399, 398, 3701, "You board the ferry from the dock.\r\n", "$n boards the ferry.",
     "$n steps onto the ferry.", "You step off the ferry onto the dock.\r\n", "$n steps off the ferry.",
     "$n arrives off the ferry.", "The ferry is still sailing.\r\n"},
    {1, 301, 301, 5963, 12069, "You board the ship from the dock.\r\n", "$n boards the ship.",
     "$n steps onto the ship.", "You step off the ship onto the dock.\r\n", "$n steps off the ship.",
     "$n arrives off the ship.", "The ship is still sailing.\r\n"},
    {1, 302, 302, 12070, 5977, "You board the ship from the dock.\r\n", "$n boards the ship.",
     "$n steps onto the ship.", "You step off the ship onto the dock.\r\n", "$n steps off the ship.",
     "$n arrives off the ship.", "The ship is still sailing.\r\n"}};

const char *wind_types[] = {"calm", "breezy", "unsettled", "windy", "chinook", "violent", "hurricane", "\n"};
const char *precip_types[] = {"none",        "arid",   "dry",     "low_precip", "avg_precip",
                              "high_precip", "stormy", "torrent", "constant",   "\n"};
const char *temp_types[] = {"frostbite", "nippy", "freezing", "cold",       "cool",    "mild",
                            "warm",      "hot",   "swelter",  "heatstroke", "boiling", "\n"};
const char *season_patterns[] = {"one season",
                                 "two seasons equal",
                                 "two seasons 1st long",
                                 "two seasons 2nd long",
                                 "three seasons equal",
                                 "four seasons equal",
                                 "four seasons even long",
                                 "four seasons odd long",
                                 "\n"};
const char *season_flags[] = {"no moon ever", "no sun ever", "non-controllable", "affects indoors", "\n"};
const char *season_variance[] = {"variable", "static", "\n"};

/* 1 entry per circle. to get circle use
 ((GET_LEVEL(ch) + 4) / 5). */
const int mental_dice[12][2] = {
    /* number, side */
    {3, 20},
    /*  0 */  /* begin mortal */
    {4, 20},  /*  1 */
    {6, 20},  /*  2 */
    {9, 20},  /*  3 */
    {12, 20}, /*  4 */
    {19, 20}, /*  5 */
    {21, 20}, /*  6 */
    {28, 20}, /*  7 */
    {33, 20}, /*  8 */
    {38, 20}, /*  9 */
    {40, 20},
    /* 10 */    /* end mortal */
    {100, 100}, /* 11 */
};

const int hometowns[][NUM_CLASSES] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, /* unused */
    /* wa,   ro,   th,   so,   wi,   en,   co,   ne,   cl,   pr,   sh,   mo,   dr,   as,   ba,   ra,   me */
    {3142, 3075, 3075, 3090, 3090, 3090, 3090, 3090, 3151, 3151, 3151, 3142, 3151, 3075, 3151, 3142, 3075}};

const char *difficulty[] = {"Easy", "Average", "Hard", "Very Hard"};

const char *saving_throws[] = {"SavPara", "SavRod", "SavPetri", "SavBreath", "SavSpell"};

const int RaceFull[NUM_RACES] = {24, 36, 54, 72, 27, 27, 36, 18, 18};
