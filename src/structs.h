/* ************************************************************************
 *   File: structs.h                                     Part of CircleMUD *
 *  Usage: header file for central structures and contstants               *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#ifndef _STRUCTS_H_
#define _STRUCTS_H_

/* preamble *************************************************************/

#include <sodium.h>
#include <sys/types.h>

#define NOWHERE -1 /* nil reference for room-database      */
#define NOTHING -1 /* nil reference for objects            */
#define NOBODY  -1 /* nil reference for mobiles            */

#define SPECIAL(name) int(name)(struct char_data * ch, void *me, int cmd, char *argument, int type)

/* Spec proc defines -
 NOTE! This spec defines works on all kinds of spec_procs, you can
 even set timer ticks on rooms!
 */

#define SPEC_STANDARD  (1 << 0) /* Standard spec proc             */
#define SPEC_HEARTBEAT (1 << 1) /* Spec proc has mob heartbeat    */
#define SPEC_TICKBEAT  (1 << 2) /* Spec proc has tick heartbeat   */
#define SPEC_COMMAND   (1 << 3) /* Spec proc triggers on commands */
#define SPEC_GREET     (1 << 4) /* Spec proc triggers when enters */
#define SPEC_FIGHT     (1 << 5) /* Spec proc triggers in fights   */

/* misc editor defines **************************************************/

/* format modes for format_text */
#define FORMAT_INDENT (1 << 0)

/* room-related defines *************************************************/

/* The cardinal directions: used as index to room_data.dir_option[] */
#define NORTH 0
#define EAST  1
#define SOUTH 2
#define WEST  3
#define UP    4
#define DOWN  5

/* Room flags: used in room_data.room_flags */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define ROOM_DARK        (1 << 0)  /* a Dark                      */
#define ROOM_DEATH       (1 << 1)  /* b Death trap                */
#define ROOM_NOMOB       (1 << 2)  /* c MOBs not allowed          */
#define ROOM_INDOORS     (1 << 3)  /* d Indoors                   */
#define ROOM_PEACEFUL    (1 << 4)  /* e Violence not allowed      */
#define ROOM_SOUNDPROOF  (1 << 5)  /* f Shouts, gossip blocked    */
#define ROOM_NOTRACK     (1 << 6)  /* g Track won't go through    */
#define ROOM_NOMAGIC     (1 << 7)  /* h Magic not allowed         */
#define ROOM_TUNNEL      (1 << 8)  /* i room for only 1 pers      */
#define ROOM_PRIVATE     (1 << 9)  /* j Can't teleport in         */
#define ROOM_GODROOM     (1 << 10) /* k LVL_GOD+ only allowed     */
#define ROOM_HOUSE       (1 << 11) /* l (R) Room is a house       */
#define ROOM_HOUSE_CRASH (1 << 12) /* m (R) House needs saving    */
#define ROOM_ATRIUM      (1 << 13) /* n (R) The door to a house   */
#define ROOM_OLC         (1 << 14) /* o (R) Modifyable/!compress  */
#define ROOM_BFS_MARK    (1 << 15) /* p (R) breath-first srch mrk */
#define ROOM_COLD        (1 << 16) /* q Room is cold              */
#define ROOM_HOT         (1 << 17) /* r Room is hot               */
#define ROOM_NOTELEPORT  (1 << 18) /* s Room is noteleport        */
#define ROOM_FASTHEAL    (1 << 19) /* t Room is 2x heal           */
#define ROOM_CRIMEOK     (1 << 20) /* u Can attack players here   */
#define ROOM_UNAFFECT    (1 << 21) /* v Room unaffects chars      */
#define ROOM_NOHEAL      (1 << 22) /* w Room prevents healing     */
#define ROOM_HARM        (1 << 23) /* x Room harms chars          */
#define ROOM_LIGHT       (1 << 24) /* y Room is always lit        */
#define ROOM_NOCAMP      (1 << 25) /* z Room cannot be camped in  */

#define NUM_ROOM_FLAGS 26
/* Exit info: used in room_data.dir_option.exit_info */
#define EX_ISDOOR     (1 << 0) /* Exit is a door           */
#define EX_CLOSED     (1 << 1) /* The door is closed       */
#define EX_LOCKED     (1 << 2) /* The door is locked       */
#define EX_PICKPROOF  (1 << 3) /* Lock can't be picked     */
#define EX_HIDDEN     (1 << 4) /* exit is hidden           */
#define EX_MAGIC_LOCK (1 << 5) /* exit is magically locked */

/* Sector types: used in room_data.sector_type */
#define SECT_INSIDE       0  /* Indoors             */
#define SECT_CITY         1  /* In a city           */
#define SECT_FIELD        2  /* In a field          */
#define SECT_FOREST       3  /* In a forest         */
#define SECT_HILLS        4  /* In the hills        */
#define SECT_MOUNTAIN     5  /* On a mountain       */
#define SECT_WATER_SWIM   6  /* Swimmable water     */
#define SECT_WATER_NOSWIM 7  /* Water - need a boat */
#define SECT_UNDERWATER   8  /* Underwater          */
#define SECT_FLYING       9  /* Wheee!              */
#define SECT_UNDERGROUND  10 /* Underground         */

#define NUM_ROOM_SECTORS 11

/* Zone bits */
#define ZONE_RESTRICTED (1 << 0) /* Access is restricted */
#define ZONE_NOTELEPORT (1 << 1) /* Zone is no-teleport  */

#define NUM_ZONE_EXTRAS 2

/* char and mob-related defines *****************************************/

#define NUM_ATTACK_TYPES 29
#define NUMITEMINKIT     13 /* number of objs in newbie kit. */

/* Stat applies */
#define STR_TOHIT   0
#define STR_TODAM   1
#define STR_CWEIGHT 2
#define STR_WWEIGHT 3
#define WIS_PRAC    4
#define INT_LEARN   5
#define CON_HITP    6
#define CON_SHOCK   7
#define DEX_REACT   8
#define DEX_MISSATT 9
#define DEX_SKILL   10
#define AGI_DEFENSE 11

/* spell related defines */
#define STAT_STR 1
#define STAT_DEX 2
#define STAT_INT 3
#define STAT_WIS 4
#define STAT_CON 5
#define STAT_AGI 6

#define POINTS_HIT  0
#define POINTS_MANA 1
#define POINTS_MOVE 2

/* PC classes */
#define CLASS_UNDEFINED   -1
#define CLASS_WARRIOR     0
#define CLASS_ROGUE       1
#define CLASS_THIEF       2
#define CLASS_SORCERER    3
#define CLASS_WIZARD      4
#define CLASS_ENCHANTER   5
#define CLASS_CONJURER    6
#define CLASS_NECROMANCER 7
#define CLASS_CLERIC      8
#define CLASS_PRIEST      9
#define CLASS_SHAMAN      10
#define CLASS_MONK        11
#define CLASS_DRUID       12
#define CLASS_ASSASSIN    13
#define CLASS_BARD        14
#define CLASS_RANGER      15
#define CLASS_MERCENARY   16

#define NUM_CLASSES 17 /* This must be the number of classes!! */

/* PC races */
#define RACE_UNDEFINED 0
#define RACE_HUMAN     1
#define RACE_TROLL     2
#define RACE_OGRE      3
#define RACE_DWARF     4
#define RACE_ELF       5
#define RACE_HALFELF   6
#define RACE_GNOME     7
#define RACE_HALFLING  8

#define NUM_RACES 9 /* Number of races */

/* NPC sizes */
#define SIZE_UNDEFINED  0
#define SIZE_VERY_SMALL 1
#define SIZE_SMALL      2
#define SIZE_MEDIUM     3
#define SIZE_LARGE      4
#define SIZE_VERY_LARGE 5
#define SIZE_GIANT      6

#define NUM_SIZES 7 /* Number of sizes */

/* NPC classes (currently unused - feel free to implement!) */
#define CLASS_NORMAL       0
#define CLASS_OTHER        1
#define CLASS_UNDEAD       2
#define CLASS_HUMANOID     3
#define CLASS_MAMMAL       4
#define CLASS_DRAGON       5
#define CLASS_GIANT        6
#define CLASS_SPIRIT       7
#define CLASS_INSECT       8
#define CLASS_ELEMENTAL    9
#define CLASS_FISH         10
#define CLASS_AMPHIBIAN    11
#define CLASS_EXTRA_PLANAR 12
#define CLASS_BIRD         13
#define CLASS_SLIME        14
#define CLASS_PLANT        15

#define NUM_MOB_CLASSES 16

/* NPC races (currently unused) */
#define NPC_RACE_UNDEFINED 0
#define NPC_RACE_HUMAN     1
#define NPC_RACE_TROLL     2
#define NPC_RACE_OGRE      3
#define NPC_RACE_DWARF     4
#define NPC_RACE_ELF       5
#define NPC_RACE_HALF_ELF  6
#define NPC_RACE_GNOME     7
#define NPC_RACE_HALFLING  8
#define NPC_RACE_DROW      9
#define NPC_RACE_DRACONIAN 10
#define NPC_RACE_DRAGON    11
#define NPC_RACE_MINOTAUR  12
#define NPC_RACE_ORC       13
#define NPC_RACE_ANIMAL    14
#define NPC_RACE_INSECT    15
#define NPC_RACE_PLANT     16

#define NUM_NPC_RACES 17

/* Sex */
#define SEX_NEUTRAL 0
#define SEX_MALE    1
#define SEX_FEMALE  2

#define NUM_GENDERS 3

/* Positions */
#define POS_DEAD      0 /* dead             */
#define POS_MORTALLYW 1 /* mortally wounded */
#define POS_INCAP     2 /* incapacitated    */
#define POS_STUNNED   3 /* stunned          */
#define POS_SLEEPING  4 /* sleeping         */
#define POS_RESTING   5 /* resting          */
#define POS_SITTING   6 /* sitting          */
#define POS_FIGHTING  7 /* fighting         */
#define POS_STANDING  8 /* standing         */

#define NUM_POSITIONS 9

/* Player flags: used by char_data.char_specials.act */
#define PLR_KILLER      (1 << 0)  /* a Player is a player-killer         */
#define PLR_THIEF       (1 << 1)  /* b Player is a player-thief          */
#define PLR_FROZEN      (1 << 2)  /* c Player is frozen                  */
#define PLR_DONTSET     (1 << 3)  /* d Don't EVER set (ISNPC bit)        */
#define PLR_WRITING     (1 << 4)  /* e Player writing (board/mail/olc)   */
#define PLR_MAILING     (1 << 5)  /* f Player is writing mail            */
#define PLR_CRASH       (1 << 6)  /* g Player needs to be crash-saved    */
#define PLR_SITEOK      (1 << 7)  /* h Player has been site-cleared      */
#define PLR_NOSHOUT     (1 << 8)  /* i Player not allowed to shout/goss  */
#define PLR_NOTITLE     (1 << 9)  /* j Player not allowed to set title   */
#define PLR_DELETED     (1 << 10) /* k Player deleted - space reusable   */
#define PLR_LOADROOM    (1 << 11) /* l Player uses nonstandard loadroom  */
#define PLR_NOWIZLIST   (1 << 12) /* m Player shouldn't be on wizlist    */
#define PLR_NODELETE    (1 << 13) /* n Player shouldn't be deleted       */
#define PLR_INVSTART    (1 << 14) /* o Player should enter game wizinvis */
#define PLR_CRYO        (1 << 15) /* p Player is cryo-saved (purge prog) */
#define PLR_KIT         (1 << 16) /* q Player has been given a kit       */
#define PLR_OUTLAW      (1 << 17) /* r Player cannot be trusted          */
#define PLR_SPECIALIZED (1 << 18) /* s Players has specialized           */
#define PLR_ZONEOK      (1 << 19) /* t Player can access any zone        */
#define PLR_EDITING     (1 << 20) /* u Player is editing zones           */
#define PLR_STAYGOD     (1 << 21) /* v Player has to stay in god zone    */
#define PLR_UNRESTRICT  (1 << 22) /* w lvl-31 is unrestricted            */
#define PLR_NOEMOTE     (1 << 23) /* x Player can not emote              */
#define PLR_NOSOCIAL    (1 << 24) /* y Player can not socialize          */
#define PLR_RENT        (1 << 25) /* z Player chose to rent out          */
#define PLR_CAMP        (1 << 26) /* A Player chose to camp out          */
#define PLR_DENIED      (1 << 27) /* B Player was denied due to bad name */
#define PLR_MAX         28

/* Mobile flags: used by char_data.char_specials.act */
#define MOB_SPEC         (1 << 0)  /* a Mob has a callable spec-proc      */
#define MOB_SENTINEL     (1 << 1)  /* b Mob should not move               */
#define MOB_SCAVENGER    (1 << 2)  /* c Mob picks up stuff on the ground  */
#define MOB_ISNPC        (1 << 3)  /* d (R) Automatically set on all Mobs */
#define MOB_AWARE        (1 << 4)  /* e Mob can't be backstabbed          */
#define MOB_AGGRESSIVE   (1 << 5)  /* f Mob hits players in the room      */
#define MOB_STAY_ZONE    (1 << 6)  /* g Mob shouldn't wander out of zone  */
#define MOB_WIMPY        (1 << 7)  /* h Mob flees if severely injured     */
#define MOB_AGGR_EVIL    (1 << 8)  /* i auto attack evil PC's             */
#define MOB_AGGR_GOOD    (1 << 9)  /* j auto attack good PC's             */
#define MOB_AGGR_NEUTRAL (1 << 10) /* k auto attack neutral PC's          */
#define MOB_MEMORY       (1 << 11) /* l remember attackers if attacked    */
#define MOB_HELPER       (1 << 12) /* m attack PCs fighting other NPCs    */
#define MOB_NOCHARM      (1 << 13) /* n Mob can't be charmed              */
#define MOB_NOSUMMON     (1 << 14) /* o Mob can't be summoned             */
#define MOB_NOSLEEP      (1 << 15) /* p Mob can't be slept                */
#define MOB_NOBASH       (1 << 16) /* q Mob can't be bashed (e.g. trees)  */
#define MOB_NOBLIND      (1 << 17) /* r Mob can't be blinded              */
#define MOB_WATERONLY    (1 << 18) /* s Mob can't live on dry land        */
#define MOB_MOUNTABLE    (1 << 19) /* t Mob can be mounted                */
#define MOB_PROGALWAYS   (1 << 20) /* u Mob is always running the mobprog */
#define MOB_WRAITHLIKE   (1 << 21) /* v Mob is fall-thru                  */
#define MOB_NOKILL       (1 << 22) /* w Mob can't be killed               */
#define MOB_HAS_MAGE     (1 << 23) /* x Mob has mage spells               */
#define MOB_HAS_CLERIC   (1 << 24) /* y Mob has cleric spells             */
#define MOB_HAS_THIEF    (1 << 25) /* z Mob has thief skills              */
#define MOB_HAS_WARRIOR  (1 << 26) /* A Mob has warrior skills            */
#define MOB_STAY_PUT     (1 << 27) /* B Mob can't Move at all             */
#define MOB_TRACKER      (1 << 28) /* C Mob will hunt anyone tracking it  */

#define NUM_MOB_FLAGS 29

/* Preference flags: used by char_data.player_specials.pref */
#define PRF_BRIEF      (1 << 0)  /* a Room descs won't normally be shown    */
#define PRF_COMPACT    (1 << 1)  /* b No extra CRLF pair before prompts     */
#define PRF_DEAF       (1 << 2)  /* c Can't hear shouts                     */
#define PRF_NOTELL     (1 << 3)  /* d Can't receive tells                   */
#define PRF_AUTOLOOT   (1 << 4)  /* e Autoloot corpses                      */
#define PRF_AUTOGOLD   (1 << 5)  /* f Autogold                              */
#define PRF_AUTOSPLIT  (1 << 6)  /* g Autosplit gold                        */
#define PRF_AUTOEXIT   (1 << 7)  /* h Display exits in a room               */
#define PRF_NOHASSLE   (1 << 8)  /* i Aggr mobs won't attack                */
#define PRF_ACCEPTED   (1 << 9)  /* j Char has been acceted                 */
#define PRF_NOSUMMON   (1 << 10) /* k Can't be summoned                     */
#define PRF_NOREPEAT   (1 << 11) /* l No repetition of comm commands        */
#define PRF_HOLYLIGHT  (1 << 12) /* m Can see in dark                       */
#define PRF_COLOR_1    (1 << 13) /* n Color (low bit)                       */
#define PRF_COLOR_2    (1 << 14) /* o Color (high bit)                      */
#define PRF_NOWIZ      (1 << 15) /* p Can't hear wizline                    */
#define PRF_LOG        (1 << 16) /* r On-line System Log (on/off)           */
#define PRF_BRIEF2     (1 << 17) /* s Room descs will be shown on move only */
#define PRF_NOAUCT     (1 << 18) /* t Can't hear auction channel            */
#define PRF_NOCHAT     (1 << 19) /* v Can't hear gossip channel             */
#define PRF_NOGRATZ    (1 << 20) /* w Can't hear grats channel              */
#define PRF_ROOMFLAGS  (1 << 21) /* x Can see room flags (ROOM_x)           */
#define PRF_AFK        (1 << 22) /* y Player is away from keyboard          */
#define PRF_WHOIS      (1 << 23) /* z Player can be "whois"'d               */
#define PRF_TICKER     (1 << 24) /* A Player has the tickcounter on         */
#define PRF_NOCITIZEN  (1 << 25) /* B Player does not want to show cstat    */
#define PRF_ANONYMOUS  (1 << 26) /* C Player does not want level shown      */
#define PRF_NOHOLLER   (1 << 27) /* D Can't hear the holler channel         */
#define PRF_NOIMMQUEST (1 << 28) /* E Not on immquest channel               */
#define PRF_MOBDEAF    (1 << 29) /* F immo can't hear mob shouts            */
#define PRF_DELETED    (1 << 30) /* G Player has self-deleted or got NUKED  */
#define PRF_GCHAT      (1 << 31) /* H Player wants guild chat channel       */
#define PRF_MAX        32

/* Command flags, used for giving wiz commands to other levels */
#define COM_IMMORT  (1 << 0) /* Basic Immort commands          */
#define COM_QUEST   (1 << 1) /* Quest related commands         */
#define COM_BUILDER (1 << 2) /* OLC and other builder commands */
#define COM_ADMIN   (1 << 3) /* Administrative commands        */
#define COM_MOB     (1 << 4) /* Access to a few "mob" commands */
#define COM_MAX     5

/* Prompt mode              */
#define PRM_HP        (1 << 0) /* hitpoints       */
#define PRM_MOVE      (1 << 1) /* movement points */
#define PRM_MANA      (1 << 2) /* mana points     */
#define PRM_ENEMYCOND (1 << 3) /* enemy condition */
#define PRM_AFK       (1 << 4) /* afk             */
#define PRM_TANKNAME  (1 << 5) /* tank name       */
#define PRM_TANKCOND  (1 << 6) /* tank condition  */
#define PRM_ENEMYNAME (1 << 7) /* enemy name      */
#define PRM_MAX       8

/* Affect bits: used in char_data.char_specials.saved.affected_by */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define AFF_BLIND           (1 << 0)  /* a (R) Char is blind                */
#define AFF_INVISIBLE       (1 << 1)  /* b Char is invisible                */
#define AFF_DETECT_ALIGN    (1 << 2)  /* c Char is sensitive to align       */
#define AFF_DETECT_INVIS    (1 << 3)  /* d Char can see invis chars         */
#define AFF_DETECT_MAGIC    (1 << 4)  /* e Char is sensitive to magic       */
#define AFF_SENSE_LIFE      (1 << 5)  /* f Char can sense hidden life       */
#define AFF_WATERWALK       (1 << 6)  /* g Char can walk on water           */
#define AFF_MAJOR_GLOBE     (1 << 7)  /* h Char is protected by major globe */
#define AFF_GROUP           (1 << 8)  /* i (R) Char is grouped              */
#define AFF_CURSE           (1 << 9)  /* j Char is cursed                   */
#define AFF_INFRAVISION     (1 << 10) /* k Char can see red shapes          */
#define AFF_POISON          (1 << 11) /* l (R) Char is poisoned             */
#define AFF_PROTECT_EVIL    (1 << 12) /* m Char protected from evil         */
#define AFF_PROTECT_GOOD    (1 << 13) /* n Char protected from good         */
#define AFF_SLEEP           (1 << 14) /* o (R) Char magically asleep        */
#define AFF_NOTRACK         (1 << 15) /* p Char can't be tracked            */
#define AFF_FLY             (1 << 16) /* q Char is flying                   */
#define AFF_NIGHTVISION     (1 << 17) /* r Char can see in dark             */
#define AFF_SNEAK           (1 << 18) /* s Char can move quietly            */
#define AFF_HIDE            (1 << 19) /* t Char is hidden                   */
#define AFF_SILENCE         (1 << 20) /* u Char is "spell" silenced         */
#define AFF_CHARM           (1 << 21) /* v Char is charmed                  */
#define AFF_DISEASE         (1 << 22) /* w Char is infected?                */
#define AFF_MAJOR_PARALIZED (1 << 23) /* x Char is paralized                */
#define AFF_SEEING          (1 << 24) /* y Char has true seeing             */
#define AFF_SUPERINV        (1 << 25) /* z Char is invis 51                 */
#define AFF_DETECT_POISON   (1 << 26) /* A Char can detect poison           */
#define AFF_TEACHING        (1 << 27) /* B Char is teaching a spell         */
#define AFF_MEDITATING      (1 << 28) /* C Char is meditating               */
#define AFF_CAMPING         (1 << 29) /* D Char is camping                  */
#define AFF_AID             (1 << 30) /* E Char has aid spell               */
#define AFF_BLESS           (1 << 31) /* F Char has bless spell             */

#define NUM_AFF_FLAGS 32

#define AFF2_CASTING         (1 << 0)  /* a Char is casting a spell                           */
#define AFF2_FARSEE          (1 << 1)  /* b Char can see 2 rooms ahead                        */
#define AFF2_ANTIMAGIC       (1 << 2)  /* c Char is immune to all magic                       */
#define AFF2_STONESKIN       (1 << 3)  /* d Char's skin is as hard as stone                   */
#define AFF2_FIRESHIELD      (1 << 4)  /* e Char is protected by fire shield                  */
#define AFF2_WRAITHFORM      (1 << 5)  /* f Char is bodyless (can pass through doors)         */
#define AFF2_VAMPTOUCH       (1 << 6)  /* g Char gains damage hp                              */
#define AFF2_SLOWNESS        (1 << 7)  /* h Char is slow                                      */
#define AFF2_HASTE           (1 << 8)  /* i Char is fast                                      */
#define AFF2_LEVITATE        (1 << 9)  /* j Char is can levitate off the ground               */
#define AFF2_PROT_LIGHT      (1 << 10) /* k Char is protected from lightning                  */
#define AFF2_PROT_ACID       (1 << 11) /* l Char is protected from acid                       */
#define AFF2_PROT_ICE        (1 << 12) /* m Char is protected from ice                        */
#define AFF2_PROT_GAS        (1 << 13) /* n Char is protected from gas                        */
#define AFF2_PROT_FIRE       (1 << 14) /* o Char is protected from fire                       */
#define AFF2_OGRESTRENGTH    (1 << 15) /* p Char is supernaturally strong                     */
#define AFF2_WITHERED        (1 << 16) /* q Char is withered                                  */
#define AFF2_MINOR_PARALIZED (1 << 17) /* r Char is temporarily paralized                     */
#define AFF2_BARKSKIN        (1 << 18) /* s Char skin is turned to bark                       */
#define AFF2_KNOCKEDOUT      (1 << 19) /* t Char is knocked out and cannot move (like asleep) */
#define AFF2_ICESHIELD       (1 << 20) /* u Char is surrounded by an ice shield               */
#define AFF2_SCRIBING        (1 << 21) /* v Char is scribing a spell                          */
#define AFF2_MEMMING         (1 << 22) /* w Char is memming a spell                           */
#define AFF2_ARMOR           (1 << 23) /* x Char has armor spell                              */
#define AFF2_FEEBLEMIND      (1 << 24) /* y Char has feeblemind                               */
#define AFF2_MINOR_GLOBE     (1 << 25) /* z Char has minor globe                              */
#define AFF2_UNUSED4         (1 << 26) /* A */
#define AFF2_UNUSED5         (1 << 27) /* B */
#define AFF2_UNUSED6         (1 << 28) /* C */
#define AFF2_UNUSED7         (1 << 29) /* D */
#define AFF2_UNUSED8         (1 << 30) /* E */
#define AFF2_UNUSED9         (1 << 31) /* F */

#define NUM_AFF2_FLAGS 26

#define AFF3_WIELDINGFLAMEBLADE      (1 << 0) /* a Wielding flame blade      */
#define AFF3_WIELDINGSPIRITUALHAMMER (1 << 1) /* b Wielding spiritual hammer */

#define NUM_AFF3_FLAGS 2

/* Resistances and vulnerabilities... used in char_special_data_saved   */
#define DAM_UNDEFINED   0
#define DAM_LIGHT       1
#define DAM_DARK        2
#define DAM_FIRE        3
#define DAM_COLD        4
#define DAM_ACID        5
#define DAM_POISON      6
#define DAM_DISEASE     7
#define DAM_CHARM       8
#define DAM_SLEEP       9
#define DAM_SLASH       10
#define DAM_PIERCE      11
#define DAM_BLUDGEON    12
#define DAM_NWEAP       13
#define DAM_MWEAP       14
#define DAM_MAGIC       15
#define DAM_ELECTRICITY 16
#define MAX_DAM_TYPE    30

#define NUM_RESISTS 17

#define SPECIALIZE_NONE       0
#define SPECIALIZE_GENERAL    (1 << 0)
#define SPECIALIZE_CREATION   (1 << 1)
#define SPECIALIZE_DIVINATION (1 << 2)
#define SPECIALIZE_ELEMENTAL  (1 << 3)
#define SPECIALIZE_HEALING    (1 << 4)
#define SPECIALIZE_PROTECTION (1 << 5)
#define SPECIALIZE_SUMMONING  (1 << 6)

#define NUM_SPECIALIZE 7

/* Modes of connectedness: used by descriptor_data.state */
#define CON_PLAYING      0  /* Playing - Nominal state  */
#define CON_CLOSE        1  /* Disconnecting            */
#define CON_GET_NAME     2  /* By what name ..?         */
#define CON_NAME_CNFRM   3  /* Did I get that right, x? */
#define CON_PASSWORD     4  /* Password:                */
#define CON_NEWPASSWD    5  /* Give me a password for x */
#define CON_CNFPASSWD    6  /* Please retype password:  */
#define CON_QSEX         7  /* Sex?                     */
#define CON_QCLASS       8  /* Class?                   */
#define CON_RMOTD        9  /* PRESS RETURN after MOTD  */
#define CON_MENU         10 /* Your choice: (main menu) */
#define CON_EXDESC       11 /* Enter a new description: */
#define CON_CHPWD_GETOLD 12 /* Changing passwd: get old */
#define CON_CHPWD_GETNEW 13 /* Changing passwd: get new */
#define CON_CHPWD_VRFY   14 /* Verify new password      */
#define CON_DELCNF1      15 /* Delete confirmation 1    */
#define CON_DELCNF2      16 /* Delete confirmation 2    */
#define CON_QRACE        17 /* Get race                 */
#define CON_OEDIT        18 /* OLC oedit                */
#define CON_REDIT        19 /* OLC redit                */
#define CON_MEDIT        20 /* OLC medit                */
#define CON_SEDIT        21 /* OLC sedit                */
#define CON_ZEDIT        22 /* OLC zedit                */
#define CON_IDCONING     23
#define CON_IDCONED      24
#define CON_IDREADING    25
#define CON_IDREAD       26
#define CON_ASKNAME      27
#define CON_NEWBIE       28
#define CON_RECONNECT_AS 29
#define CON_ROLLSTATS    30
#define CON_GET_TERMTYPE 31
#define CON_TEXTED       32
#define CON_ALIGNMENT    33
#define CON_HOMETOWN     34
#define CON_ACCEPT       35 /* Accept or decline player */
#define CON_NEWNAME      36 /* Player was declined      */
#define CON_POLICY       37 /* Player reading policy    */

/* Character equipment positions: used as index for char_data.equipment[] */
/* NOTE: Don't confuse these constants with the ITEM_ bitvectors
 which control the valid places you can wear a piece of equipment */

#define WORN_BADGE    (1 << 0)  /* a */
#define WORN_FINGER_R (1 << 1)  /* b */
#define WORN_FINGER_L (1 << 2)  /* c */
#define WORN_NECK_1   (1 << 3)  /* d */
#define WORN_NECK_2   (1 << 4)  /* e */
#define WORN_BODY     (1 << 5)  /* f */
#define WORN_HEAD     (1 << 6)  /* g */
#define WORN_LEGS     (1 << 7)  /* h */
#define WORN_FEET     (1 << 8)  /* i */
#define WORN_HANDS    (1 << 9)  /* j */
#define WORN_ARMS     (1 << 10) /* k */
#define WORN_SHIELD   (1 << 11) /* l */
#define WORN_ABOUT    (1 << 12) /* m */
#define WORN_WAIST    (1 << 13) /* n */
#define WORN_WRIST_R  (1 << 14) /* o */
#define WORN_WRIST_L  (1 << 15) /* p */
#define WORN_WIELD    (1 << 16) /* q */
#define WORN_HOLD     (1 << 17) /* r */
#define WORN_FACE     (1 << 18) /* s */
#define WORN_EAR_R    (1 << 19) /* t */
#define WORN_EAR_L    (1 << 20) /* u */
#define WORN_EYES     (1 << 21) /* v */
#define WORN_ANKLE_R  (1 << 22) /* w */
#define WORN_ANKLE_L  (1 << 23) /* x */
#define WORN_WIELD_2  (1 << 24) /* y */
#define WORN_HOLD_2   (1 << 25) /* z */
#define WORN_2HANDED  (1 << 26) /* A */

#define WEAR_BADGE    0
#define WEAR_FINGER_R 1
#define WEAR_FINGER_L 2
#define WEAR_NECK_1   3
#define WEAR_NECK_2   4
#define WEAR_BODY     5
#define WEAR_HEAD     6
#define WEAR_LEGS     7
#define WEAR_FEET     8
#define WEAR_HANDS    9
#define WEAR_ARMS     10
#define WEAR_SHIELD   11
#define WEAR_ABOUT    12
#define WEAR_WAIST    13
#define WEAR_WRIST_R  14
#define WEAR_WRIST_L  15
#define WEAR_WIELD    16
#define WEAR_HOLD     17
#define WEAR_FACE     18
#define WEAR_EAR_R    19
#define WEAR_EAR_L    20
#define WEAR_EYES     21
#define WEAR_ANKLE_R  22
#define WEAR_ANKLE_L  23
#define WEAR_WIELD_2  24
#define WEAR_HOLD_2   25
#define WEAR_2HANDED  26

#define NUM_WEARS 27 /* This must be the # of eq positions!! */

/* object-related defines ********************************************/

/* defines for obj related spells */
#define OBJ_EXTRA           1
#define OBJ_VALUE_0         2
#define OBJ_VALUE_1         3
#define OBJ_VALUE_2         4
#define OBJ_VALUE_3         5
#define OBJ_VALUE_4         6
#define OBJ_DAMROLL         7
#define OBJ_HITROLL         8
#define OBJ_SAVING_PARA     9
#define OBJ_SAVING_ROD      10
#define OBJ_SAVING_PETRI    11
#define OBJ_SAVING_BREATH   12
#define OBJ_SAVING_SPELL    13
#define OBJ_MAX_HIT         14
#define OBJ_MAX_MANA        15
#define OBJ_MAX_MOVE        16
#define OBJ_RES_LIGHT       17
#define OBJ_RES_DARK        18
#define OBJ_RES_FIRE        19
#define OBJ_RES_COLD        20
#define OBJ_RES_ACID        21
#define OBJ_RES_POISON      22
#define OBJ_RES_DISEASE     23
#define OBJ_RES_CHARM       24
#define OBJ_RES_SLEEP       25
#define OBJ_RES_SLASH       26
#define OBJ_RES_PIERCE      27
#define OBJ_RES_BLUDGEON    28
#define OBJ_RES_NWEAP       29
#define OBJ_RES_MWEAP       30
#define OBJ_RES_MAGIC       31
#define OBJ_RES_ELECTRICITY 32
#define OBJ_TIMER           33

/* Item types: used by obj_data.obj_flags.type_flag */
#define ITEM_LIGHT      1  /* Item is a light source        */
#define ITEM_SCROLL     2  /* Item is a scroll              */
#define ITEM_WAND       3  /* Item is a wand                */
#define ITEM_STAFF      4  /* Item is a staff               */
#define ITEM_WEAPON     5  /* Item is a weapon              */
#define ITEM_FIREWEAPON 6  /* Unimplemented                 */
#define ITEM_MISSILE    7  /* Unimplemented                 */
#define ITEM_TREASURE   8  /* Item is a treasure, not gold  */
#define ITEM_ARMOR      9  /* Item is armor                 */
#define ITEM_POTION     10 /* Item is a potion              */
#define ITEM_WORN       11 /* Unimplemented                 */
#define ITEM_OTHER      12 /* Misc object                   */
#define ITEM_TRASH      13 /* Trash - shopkeeps won't buy   */
#define ITEM_TRAP       14 /* Unimplemented                 */
#define ITEM_CONTAINER  15 /* Item is a container           */
#define ITEM_NOTE       16 /* Item is note                  */
#define ITEM_DRINKCON   17 /* Item is a drink container     */
#define ITEM_KEY        18 /* Item is a key                 */
#define ITEM_FOOD       19 /* Item is food                  */
#define ITEM_MONEY      20 /* Item is money (gold)          */
#define ITEM_PEN        21 /* Item is a pen                 */
#define ITEM_BOAT       22 /* Item is a boat                */
#define ITEM_FOUNTAIN   23 /* Item is a fountain            */
#define ITEM_INSTR      24 /* Item is a bard instrument     */
#define ITEM_BADGE      25 /* Item is a badge               */
#define ITEM_PCORPSE    26 /* Item is a player's corpse     */
#define ITEM_SPELLBOOK  27 /* Item is a spellbook           */
#define ITEM_PORTAL     28 /* Item is a portal              */
#define ITEM_M_WALL     29 /* Item is a magic wall          */

#define NUM_ITEM_TYPES 30

/* Item material types */

#define TYPE_UNDEF    0
#define TYPE_OTHER    1
#define TYPE_FOOD     2
#define TYPE_CLOTH    3
#define TYPE_PLANT    4
#define TYPE_WOOD     5
#define TYPE_STONE    6
#define TYPE_GRANITE  7
#define TYPE_IRON     8
#define TYPE_STEEL    9
#define TYPE_MITHRIL  10
#define TYPE_TITANIUM 11
#define TYPE_COPPER   12
#define TYPE_BRONZE   13
#define TYPE_SILVER   14
#define TYPE_GOLD     15
#define TYPE_DIAMOND  16
#define TYPE_ICE      17
#define TYPE_GLASS    18
#define TYPE_PAPER    19
#define TYPE_LEATHER  20
#define TYPE_IVORY    21
#define TYPE_EBONY    22
#define TYPE_FLESH    23
#define TYPE_SKIN     24
#define TYPE_BONE     25
#define TYPE_WATER    26
#define TYPE_CRYSTAL  27
#define TYPE_EARTH    28
#define TYPE_LIGHT    29
#define NUM_MATERIALS 30 /* Number of materials */

/* Weapon types */
#define WEAPON_UNDEFINED    0
#define WEAPON_SHORTSWORD   1
#define WEAPON_BROADSWORD   2
#define WEAPON_LONGSWORD    3
#define WEAPON_HANDAXE      4
#define WEAPON_BATTLEAXE    5
#define WEAPON_QUARTERSTAFF 6
#define WEAPON_POLEARM      7
#define WEAPON_MACE         8
#define WEAPON_WARHAMMER    9
#define WEAPON_MORNING_STAR 10
#define WEAPON_CLUB         11
#define WEAPON_DAGGER       12
#define WEAPON_WHIP         13
#define WEAPON_2HANDED      14
#define WEAPON_BOW          15
#define WEAPON_SPEAR        16
#define WEAPON_MAX          17 /* Amount of different weapon types */

#define WEAPON_1H_BLUDGEON 1
#define WEAPON_1H_MISC     2
#define WEAPON_1H_PIERCING 3
#define WEAPON_1H_SLASHING 4
#define WEAPON_2H_BLUDGEON 5
#define WEAPON_2H_MISC     6
#define WEAPON_2H_SLASHING 7

#define WEAPON_HANDED 7

/* Take/Wear flags: used by obj_data.obj_flags.wear_flags */
#define ITEM_WEAR_TAKE    (1 << 0)  /* Item can be takes        */
#define ITEM_WEAR_FINGER  (1 << 1)  /* Can be worn on finger    */
#define ITEM_WEAR_NECK    (1 << 2)  /* Can be worn around neck  */
#define ITEM_WEAR_BODY    (1 << 3)  /* Can be worn on body      */
#define ITEM_WEAR_HEAD    (1 << 4)  /* Can be worn on head      */
#define ITEM_WEAR_LEGS    (1 << 5)  /* Can be worn on legs      */
#define ITEM_WEAR_FEET    (1 << 6)  /* Can be worn on feet      */
#define ITEM_WEAR_HANDS   (1 << 7)  /* Can be worn on hands     */
#define ITEM_WEAR_ARMS    (1 << 8)  /* Can be worn on arms      */
#define ITEM_WEAR_SHIELD  (1 << 9)  /* Can be used as a shield  */
#define ITEM_WEAR_ABOUT   (1 << 10) /* Can be worn about body   */
#define ITEM_WEAR_WAIST   (1 << 11) /* Can be worn around waist */
#define ITEM_WEAR_WRIST   (1 << 12) /* Can be worn on wrist     */
#define ITEM_WEAR_WIELD   (1 << 13) /* Can be wielded           */
#define ITEM_WEAR_HOLD    (1 << 14) /* Can be held              */
#define ITEM_WEAR_FACE    (1 << 15) /* Can be worn on face      */
#define ITEM_WEAR_EAR     (1 << 16) /* Can be worn on ear       */
#define ITEM_WEAR_EYES    (1 << 17) /* Can be worn on eyes      */
#define ITEM_WEAR_ANKLES  (1 << 18) /* Can be worn on ankles    */
#define ITEM_WEAR_BADGE   (1 << 19) /* Can be worn as a badge   */
#define ITEM_WEAR_2HANDED (1 << 20) /* Can be worn as a badge   */

#define NUM_ITEM_WEARS 21

/* Extra object flags: used by obj_data.obj_flags.extra_flags */
#define ITEM_GLOW            (1 << 0)  /* a Item is glowing              */
#define ITEM_HUM             (1 << 1)  /* b Item is humming              */
#define ITEM_NORENT          (1 << 2)  /* c Item cannot be rented        */
#define ITEM_NODONATE        (1 << 3)  /* d Item cannot be donated       */
#define ITEM_NOINVIS         (1 << 4)  /* e Item cannot be made invis    */
#define ITEM_INVISIBLE       (1 << 5)  /* f Item is invisible            */
#define ITEM_MAGIC           (1 << 6)  /* g Item is magical              */
#define ITEM_NODROP          (1 << 7)  /* h Item is cursed: can't drop   */
#define ITEM_BLESS           (1 << 8)  /* i Item is blessed              */
#define ITEM_ANTI_GOOD       (1 << 9)  /* j Not usable by good people    */
#define ITEM_ANTI_EVIL       (1 << 10) /* k Not usable by evil people    */
#define ITEM_ANTI_NEUTRAL    (1 << 11) /* l Not usable by neutral people */
#define ITEM_ANTI_MAGIC_USER (1 << 12) /* m Not usable by mages          */
#define ITEM_ANTI_CLERIC     (1 << 13) /* n Not usable by clerics        */
#define ITEM_ANTI_THIEF      (1 << 14) /* o Not usable by thieves        */
#define ITEM_ANTI_WARRIOR    (1 << 15) /* p Not usable by warriors       */
#define ITEM_NOSELL          (1 << 16) /* q Shopkeepers won't touch it   */
#define ITEM_DONATED         (1 << 17) /* r Items has been donated       */
#define ITEM_NOAUCTION       (1 << 18) /* s Auctioneers won't touch it   */
#define ITEM_CARRIED         (1 << 19) /* t Item has been carried        */
#define ITEM_ISLIGHT         (1 << 20) /* u Item is a light source       */
#define ITEM_ISLIGHTDIM      (1 << 21) /* v Item is a dim light source   */
#define ITEM_POISONED        (1 << 22) /* w Item is poisoned             */
#define ITEM_FLOAT           (1 << 23) /* x Item can float on water      */
#define ITEM_NOT_OBV         (1 << 24) /* y Item is hidden               */
#define ITEM_NOIDENTIFY      (1 << 25) /* z Item is unidentifiable       */
#define ITEM_UNDERWATER      (1 << 26) /* A Item is unidentifiable       */
#define ITEM_NOBURN          (1 << 27) /* B Item cannot be destroyed     */
#define ITEM_ENCHANTED       (1 << 28) /* C Item has been enchanted      */
#define ITEM_NODECAY         (1 << 29) /* D Item will not decay          */

#define NUM_ITEM_FLAGS 30

/* Modifier constants used with obj affects ('A' fields) */
#define APPLY_NONE            0  /* No effect                */
#define APPLY_STR             1  /* Apply to strength        */
#define APPLY_DEX             2  /* Apply to dexterity       */
#define APPLY_INT             3  /* Apply to intelligence    */
#define APPLY_WIS             4  /* Apply to wisdom          */
#define APPLY_CON             5  /* Apply to constitution    */
#define APPLY_AGI             6  /* Apply to agility         */
#define APPLY_CLASS           7  /* Reserved                 */
#define APPLY_LEVEL           8  /* Reserved                 */
#define APPLY_AGE             9  /* Apply to age             */
#define APPLY_CHAR_WEIGHT     10 /* Apply to weight          */
#define APPLY_CHAR_HEIGHT     11 /* Apply to height          */
#define APPLY_MANA            12 /* Apply to mana            */
#define APPLY_HIT             13 /* Apply to hit points      */
#define APPLY_MOVE            14 /* Apply to move points     */
#define APPLY_GOLD            15 /* Reserved                 */
#define APPLY_EXP             16 /* Reserved                 */
#define APPLY_AC              17 /* Apply to Armor Class     */
#define APPLY_HITROLL         18 /* Apply to hitroll         */
#define APPLY_DAMROLL         19 /* Apply to damage roll     */
#define APPLY_SAVING_PARA     20 /* Apply to save paralz     */
#define APPLY_SAVING_ROD      21 /* Apply to save rods       */
#define APPLY_SAVING_PETRI    22 /* Apply to save petrif     */
#define APPLY_SAVING_BREATH   23 /* Apply to save breath     */
#define APPLY_SAVING_SPELL    24 /* Apply to save spells     */
#define APPLY_MAX_HIT         25 /* Apply to max spells      */
#define APPLY_MAX_MANA        26 /* Apply to max spells      */
#define APPLY_MAX_MOVE        27 /* Apply to max spells      */
#define APPLY_RES_LIGHT       28 /* Apply to res light       */
#define APPLY_RES_DARK        29 /* Apply to res dark        */
#define APPLY_RES_FIRE        30 /* Apply to res fire        */
#define APPLY_RES_COLD        31 /* Apply to res cold        */
#define APPLY_RES_ACID        32 /* Apply to res acid        */
#define APPLY_RES_POISON      33 /* Apply to res poison      */
#define APPLY_RES_DISEASE     34 /* Apply to res disease     */
#define APPLY_RES_CHARM       35 /* Apply to res charm       */
#define APPLY_RES_SLEEP       36 /* Apply to res sleep       */
#define APPLY_RES_SLASH       37 /* Apply to res slash       */
#define APPLY_RES_PIERCE      38 /* Apply to res pierce      */
#define APPLY_RES_BLUDGEON    39 /* Apply to res bludgeon    */
#define APPLY_RES_NWEAP       40 /* Apply to res nweap       */
#define APPLY_RES_MWEAP       41 /* Apply to res mweap       */
#define APPLY_RES_MAGIC       42 /* Apply to res magic       */
#define APPLY_RES_ELECTRICITY 43 /* Apply to res electricity */

#define NUM_APPLIES 44

/* Container flags - value[1] */
#define CONT_CLOSEABLE (1 << 0) /* Container can be closed  */
#define CONT_PICKPROOF (1 << 1) /* Container is pickproof   */
#define CONT_CLOSED    (1 << 2) /* Container is closed      */
#define CONT_LOCKED    (1 << 3) /* Container is locked      */

/* Some different kind of liquids for use in values of drink containers */
#define LIQ_WATER      0
#define LIQ_BEER       1
#define LIQ_WINE       2
#define LIQ_ALE        3
#define LIQ_DARKALE    4
#define LIQ_WHISKY     5
#define LIQ_LEMONADE   6
#define LIQ_FIREBRT    7
#define LIQ_LOCALSPC   8
#define LIQ_SLIME      9
#define LIQ_MILK       10
#define LIQ_TEA        11
#define LIQ_COFFE      12
#define LIQ_BLOOD      13
#define LIQ_SALTWATER  14
#define LIQ_CLEARWATER 15
#define LIQ_COKE       16

#define NUM_LIQ_TYPES 17

/* other miscellaneous defines *******************************************/

/* Player conditions */
#define DRUNK  0
#define FULL   1
#define THIRST 2

/* Sun state for weather_data */
#define SUN_DARK  0
#define SUN_RISE  1
#define SUN_LIGHT 2
#define SUN_SET   3

/* Sky conditions for weather_data */
#define SKY_CLOUDLESS 0
#define SKY_CLOUDY    1
#define SKY_RAINING   2
#define SKY_LIGHTNING 3

/* Rent codes */
#define RENT_UNDEF    0
#define RENT_CRASH    1
#define RENT_RENTED   2
#define RENT_CRYO     3
#define RENT_FORCED   4
#define RENT_TIMEDOUT 5
#define RENT_CAMPED   6

/* other #defined constants **********************************************/

#define LVL_IMMORT 51
#define IMMORTAL   LVL_IMMORT + 1
#define QUEST      LVL_IMMORT + 2
#define BUILDER    LVL_IMMORT + 3
#define ADMIN      LVL_IMMORT + 4

#define LVL_FREEZE LVL_IMMORT

#define NUM_OF_DIRS 6 /* number of directions in a room (nsewud) */

#define OPT_USEC       100000 /* 10 passes per second */
#define PASSES_PER_SEC (1000000 / OPT_USEC)
#define RL_SEC         *PASSES_PER_SEC

#define PULSE_ZONE           (10 RL_SEC)
#define PULSE_MOBILE         (10 RL_SEC)
#define PULSE_MOBHUNT        (4 RL_SEC)
#define PULSE_MOBPROG        (1 RL_SEC)
#define PULSE_VIOLENCE       (25) /* 2.5 seconds */
#define PULSE_REGEN          (1 RL_SEC)
#define PULSE_UPDATE_AFFECTS (5 RL_SEC)
#define PULSE_FIGHT          (2 RL_SEC)
#define PULSE_EVENT          (1 RL_SEC)
#define PULSE_CHK_IDLE       (3 RL_SEC)

#define SMALL_BUFSIZE 1024
#define LARGE_BUFSIZE (12 * 1024)
#define GARBAGE_SPACE 32

#define MAX_STRING_LENGTH    32768
#define MAX_INPUT_LENGTH     512 /* Max length per *line* of input */
#define MAX_RAW_INPUT_LENGTH 512 /* Max size of *raw* input */
#define MAX_MESSAGES         100
#define MAX_NAME_LENGTH      20
#define MAX_PWD_LENGTH       10
#define MAX_TITLE_LENGTH     80
#define HOST_LENGTH          50
#define IDENT_LENGTH         10
#define EXDSCR_LENGTH        800
#define MAX_TONGUE           10
#define MAX_SKILLS           500
#define MAX_AFFECT           32
#define MAX_OBJ_AFFECT       6
#define NUM_SAVE_THROWS      5
#define NUM_CONDITIONS       3

#define MAX_TRANSPORT 3

/***********************************************************************
 * Structures                                                          *
 **********************************************************************/

typedef signed char sbyte;
typedef unsigned char ubyte;
typedef signed short int sh_int;
typedef unsigned short int ush_int;
typedef char bool;
typedef char byte;

typedef int room_num;
typedef int obj_num;

/* Extra description: used in objects, mobiles, and rooms */
struct extra_descr_data {
  char *keyword;                 /* Keyword in look/examine */
  char *description;             /* What to see             */
  struct extra_descr_data *next; /* Next in list            */
};

/* object-related structures ******************************************/

/* object flags; used in obj_data */
struct obj_flag_data {
  int value[5];    /* Values of the item (see list)   */
  int type_flag;   /* Type of item                    */
  int wear_flags;  /* Where you can wear it           */
  int wear_slots;  /* What it covers when you wear it */
  int extra_flags; /* If it hums, glows, etc.         */
  int weight;      /* Weigt what else                 */
  int cost;        /* Value when sold (gp.)           */
  int timer;       /* Timer for object                */
  long bitvector;  /* To set chars bits AFF_XXXX      */
  long bitvector2; /* To set chars bits AFF2_XXXX     */
  long bitvector3; /* To set chars bits AFF3_XXXX     */
};

/* Used in obj_file_elem *DO*NOT*CHANGE* */
struct obj_affected_type {
  int location; /* Which abilities to change (APPLY_XXX) */
  int modifier; /* How much it changes by                */
};

/* ================== Memory Structure for Objects ================== */
/* OBJ_DATA */
struct obj_data {
  obj_num item_number; /* Where in data-base              */
  room_num in_room;    /* In what room -1 when conta/carr */

  struct obj_flag_data obj_flags;                     /* Object information                    */
  struct obj_affected_type affected[MAX_OBJ_AFFECT];  /* affects                               */
  struct obj_affected_type affected2[MAX_OBJ_AFFECT]; /* affects                               */
  struct obj_affected_type affected3[MAX_OBJ_AFFECT]; /* affects                               */
  int resists[MAX_DAM_TYPE];                          /* resist apply fields                   */

  char *name;                              /* Title of object :get etc.             */
  char *cname;                             /* Title of object :get etc. for corpses */
  char *description;                       /* When in room                          */
  char *cdescription;                      /* When in room for corpses              */
  char *short_description;                 /* when worn/carry/in cont.              */
  char *cshort_description;                /* when worn/carry/in cont. for corpses  */
  char *action_description;                /* What to write when used               */
  char *owner;                             /* Owner for pcorpses                    */
  int objnum;                              /* for pcorpse/rent saving               */
  int inobj;                               /* for pcorpse/rent saving               */
  struct extra_descr_data *ex_description; /* extra descriptions                    */
  struct char_data *carried_by;            /* Carried by :NULL in room/conta        */
  struct char_data *dragged_by;            /* Dragged by :NULL when !being dragged  */
  struct char_data *worn_by;               /* Worn by?                              */
  sh_int worn_on;                          /* Worn where?                           */

  int spec_vars[3];          /* vars reserved for spec_procs          */
  struct obj_data *in_obj;   /* In what object NULL when none         */
  struct obj_data *contains; /* Contains objects                      */

  struct obj_data *next_content; /* For 'contains' lists                  */
  struct obj_data *next;         /* For the object list                   */
  int *spell_list;               /* List of spells resolved at boot       */
};
/* ======================================================================= */

/* ====================== File Element for Objects ======================= */
/*                 BEWARE: Changing it will ruin rent wfiles       */

struct obj_file_elem {
  obj_num item_number;
  int value[5];
  int extra_flags;
  int weight;
  int timer;
  long bitvector;
  struct obj_affected_type affected[MAX_OBJ_AFFECT];
  int position; /* for improved save */
};

/* header block for rent files.  BEWARE: Changing it will ruin rent files  */
/*
 struct rent_info {
 int  time;
 int  rentcode;
 int  net_cost_per_diem;
 int  gold;
 int  account;
 int  nitems;
 int  version;
 };
 */

#define QIC_OWNERS 20 /* number of owners to store on each object */

struct qic_data {
  obj_num vnum;                             /* vnum of QIC item                    */
  int limit;                                /* what the QIC limit is               */
  int items;                                /* current number of items in the game */
  char owners[QIC_OWNERS][MAX_NAME_LENGTH]; /* lists QIC_OWNERS of the item owners */
};

/* ======================================================================= */

/* room-related structures ************************************************/

struct room_direction_data {
  char *general_description; /* When look DIR.                     */

  char *keyword; /* for open/close                     */

  sh_int exit_info;      /* Exit info                          */
  obj_num key;           /* Key's number (-1 for no key)       */
  room_num to_room;      /* Where direction leads (NOWHERE)    */
  room_num to_room_vnum; /* the vnum of the room. Used for OLC */
  int add_move;          /* extra exit move cost               */
};

/* ================== Memory Structure for room ======================= */
/* ROOM_DATA */
struct room_data {
  room_num number;                                     /* Rooms number  (vnum)           */
  int zone;                                            /* Room zone (for resetting)      */
  int sector_type;                                     /* sector type (move/hide)        */
  char *name;                                          /* Rooms name 'You are ...'       */
  char *description;                                   /* Shown when entered             */
  struct extra_descr_data *ex_description;             /* for examine/look               */
  struct room_direction_data *dir_option[NUM_OF_DIRS]; /* Directions                     */
  int room_flags;                                      /* DEATH,DARK ... etc             */

  byte light; /* Number of lightsources in room */
  SPECIAL(*func);
  int spec_type;             /* kind of spec proc              */
  int spec_vars[3];          /* spec proc vars, for timers etc */
  struct obj_data *contents; /* List of items in room          */
  struct char_data *people;  /* List of NPC / PC in room       */
};
/* ====================================================================== */

/* char-related structures ************************************************/

/* memory structure for characters */
struct memory_rec_struct {
  long id;
  struct memory_rec_struct *next;
};

typedef struct memory_rec_struct memory_rec;

/* MOBProgram foo */
struct mob_prog_act_list {
  struct mob_prog_act_list *next;
  char *buf;
  struct char_data *ch;
  struct obj_data *obj;
  void *vo;
};
typedef struct mob_prog_act_list MPROG_ACT_LIST;

struct mob_prog_data {
  struct mob_prog_data *next;
  int type;
  char *arglist;
  char *comlist;
};

typedef struct mob_prog_data MPROG_DATA;

extern bool MOBTrigger;

#define ERROR_PROG     -1
#define IN_FILE_PROG   (1 << 0)
#define ACT_PROG       (1 << 1)
#define SPEECH_PROG    (1 << 2)
#define RAND_PROG      (1 << 3)
#define FIGHT_PROG     (1 << 4)
#define DEATH_PROG     (1 << 5)
#define HITPRCNT_PROG  (1 << 6)
#define ENTRY_PROG     (1 << 7)
#define GREET_PROG     (1 << 8)
#define ALL_GREET_PROG (1 << 9)
#define GIVE_PROG      (1 << 10)
#define BRIBE_PROG     (1 << 11)
#define SHOUT_PROG     (1 << 12)
#define HOLLER_PROG    (1 << 13)
#define TELL_PROG      (1 << 14)
#define TIME_PROG      (1 << 15)
#define ASK_PROG       (1 << 16)

#define NUM_PROGS 17
/* end of MOBProg foo */

/* This structure is purely intended to be an easy way to transfer */
/* and return information about time (real or mudwise).            */
struct time_info_data {
  byte hours, day, month;
  sh_int year;
};

/* These data contain information about a players time data */
struct time_data {
  time_t birth; /* This represents the characters age                */
  time_t logon; /* Time of the last logon (used to calculate played) */
  int played;   /* This is the total accumulated time played in secs */
};

/* general player-related info, usually PC's and NPC's */
struct char_player_data {
  char passwd[MAX_PWD_LENGTH + 1];             /* character's password              */
  char enc_passwd[crypto_pwhash_STRBYTES + 1]; /* character's encrypted password    */
  char *name;                                  /* PC / NPC s name (kill ...  )      */
  char *short_descr;                           /* for NPC 'actions'                 */
  char *long_descr;                            /* for 'look'                        */
  char *description;                           /* Extra descriptions                */
  char *title;                                 /* PC / NPC's title                  */
  byte sex;                                    /* PC / NPC's sex                    */
  byte class;                                  /* PC / NPC's class                  */
  byte level;                                  /* PC / NPC's level                  */
  int hometown;                                /* PC s Hometown (zone)              */
  struct time_data time;                       /* PC's AGE in days                  */
  int weight;                                  /* PC / NPC's weight                 */
  int height;                                  /* PC / NPC's height                 */
  char host[HOST_LENGTH + 1];                  /* PC ip of host they connected with */
};

/* Char's abilities. */
struct char_ability_data {
  sbyte str;
  sbyte intel;
  sbyte wis;
  sbyte dex;
  sbyte con;
  sbyte agi;

  /* Virtual stats to tally capped attributes. */
  sbyte vstr;
  sbyte vintel;
  sbyte vwis;
  sbyte vdex;
  sbyte vcon;
  sbyte vagi;
};

/* Char's points.  */
struct char_point_data {
  sh_int mana;     /* Current mana for PC/NPC                 */
  sh_int max_mana; /* Max mana for PC/NPC                     */
  sh_int hit;      /* Current hitpoints for PC/NPC            */
  sh_int max_hit;  /* Max hit for PC/NPC                      */
  sh_int move;     /* Current movement points for PC/NPC      */
  sh_int max_move; /* Max move for PC/NPC                     */

  sh_int armor;           /* Internal -100..100, external -10..10 AC */
  int bank_plat;          /* Platnum the char has in a bank account  */
  int bank_gold;          /* Gold the char has in a bank account     */
  int bank_silver;        /* Silver the char has in a bank account   */
  int bank_copper;        /* Copper the char has in a bank account   */
  int plat;               /* Platnum carried by the PC/NPC           */
  int gold;               /* Gold carried by the PC/NPC              */
  int silver;             /* Silver carried by the PC/NPC            */
  int copper;             /* Copper carried by the PC/NPC            */
  unsigned int temp_gold; /* Money carried                           */
  unsigned int exp;       /* The experience of the player            */

  sbyte hitroll; /* Any bonus or penalty to the hit roll    */
  sbyte damroll; /* Any bonus or penalty to the damage roll */
};

/*
 * char_special_data_saved: specials which both a PC and an NPC have in
 * common, but which must be saved to the playerfile for PC's.
 *
 * WARNING:  Do not change this structure.  Doing so will ruin the
 * playerfile.  If you want to add to the playerfile, use the spares
 * in player_special_data.
 */
struct char_special_data_saved {
  int alignment; /* +-1000 for alignments                    */
  long idnum;    /* player's idnum; idnumth mob              */
  long act;      /* act flag for NPC's; player flag for PC's */

  long affected_by;             /* Bitvector for spells/skills affected by  */
  long affected_by2;            /* Bitvector for spells/skills affected by  */
  long affected_by3;            /* Bitvector for spells/skills affected by  */
  sh_int apply_saving_throw[5]; /* Saving throw (Bonuses)                   */
  sh_int resists[MAX_DAM_TYPE]; /* Resistances and vulnerabilities          */
};

/* Special playing constants shared by PCs and NPCs which aren't in pfile */
struct char_special_data {
  struct char_data *fighting;   /* Opponent                             */
  long hunting;                 /* IDNUM hunted by this char/mob        */
  struct char_data *mounting;   /* Char that is mounted by this char    */
  struct char_data *mounted_by; /* Char that is mounting this char      */

  byte position;      /* Standing, fighting, sleeping, etc.   */
  room_num huntingrm; /* Room hunted by this char             */

  int carry_weight;                     /* Carried weight                       */
  byte carry_items;                     /* Number of items carried              */
  int timer;                            /* Timer for update                     */
  int fight_timer;                      /* Timer for fight update               */
  int skill_timer;                      /* Timer for skill update               */
  int fightwait;                        /* waitstate for fights                 */
  int attackednum;                      /* Number of attacks on char this round */
  struct char_special_data_saved saved; /* constants saved in plrfile           */
};

/*
 * If you want to add new values to the playerfile, do it here.  DO NOT
 * ADD, DELETE OR MOVE ANY OF THE VARIABLES - doing so will change the
 * size of the structure and ruin the playerfile.  However, you can change
 * the names of the spares to something more meaningful, and then use them
 * in your new code.  They will automatically be transferred from the
 * playerfile into memory when players log in.
 */
struct player_special_data_saved {
  byte skills[MAX_SKILLS + 1]; /* array of skills plus skill 0          */
  byte pracs[MAX_SKILLS + 1];  /* number of pracs available per skill   */
  byte spells_to_learn;        /* How many can you learn yet this level */
  int wimp_level;              /* Below this # of hit points, flee!     */
  byte freeze_level;           /* Level of god who froze char, if any   */
  int invis_level;             /* level of invisibility                 */
  room_num load_room;          /* Which room to place char in           */
  long pref;                   /* preference flags for PC's.            */
  int bad_pws;                 /* number of bad password attemps        */
  sbyte conditions[3];         /* Drunk, full, thirsty                  */

  int race; /* Race */
  ubyte prompt;
  int pk_count;
  int timer;
  int weapontimer;
  int log;
  int death_count;
  int kills;
  int train_sessions;
  long commands;
  int specialized;
  int questcompleted[10];
};

/*
 * Specials needed only by PCs, not NPCs.  Space for this structure is
 * not allocated in memory for NPCs, but it is for PCs and the portion
 * of it labelled 'saved' is saved in the playerfile.  This structure can
 * be changed freely; beware, though, that changing the contents of
 * player_special_data_saved will corrupt the playerfile.
 */
struct player_special_data {
  struct player_special_data_saved saved;

  char *poofin;                  /* Description on arrival of a god.       */
  char *poofout;                 /* Description upon a god's exit.         */
  char *whois;                   /* text line to be showed with whois info */
  struct alias *aliases;         /* Character's aliases                    */
  char *last_tell;               /* last tell from idnum                   */
  char *tell;                    /* a point to the last tell               */
  char *whospec;                 /* who listing [Lvl Class] replacement    */
  struct char_data *hunters[10]; /* hunted by chars                        */
  sbyte huntstate[10];           /* waitstate on hunters                   */
};

/* Specials used by NPCs, not PCs */
struct mob_special_data {
  byte last_direction;                  /* The last direction the monster went */
  byte default_pos;                     /* Default position for NPC            */
  memory_rec *memory;                   /* List of attackers to remember       */
  int race;                             /* Mobs race (unimplemented)           */
  byte size;                            /* The mobs size (unimplemented)       */
  int wait_state;                       /* Wait state for bashed mobs          */
  struct mob_attacks_data *mob_attacks; /* Mob attacks                         */
  struct mob_equipment_data *mob_equip; /* Mob equipment                       */
  struct mob_action_data *mob_action;   /* Mob actions                         */
  long attacked_by[20];                 /* mob attacked by idnums              */
  byte attacked_levels[20];             /* levels of attackers                 */
  int timer;                            /* timers for mobs, spec_procs         */
  bool mp_toggle;                       /* Hopefully for mobprogs toggling     */
  int invis_level;                      /* Invis level for mobs.               */
  int quest;
};

struct mob_attacks_data {
  byte nodice;                   /* number of damage dice's         */
  byte sizedice;                 /* size of damage dice             */
  byte damroll;                  /* damroll for this attack type    */
  byte attack_type;              /* Type of attack                  */
  int attacks;                   /* speed of attacks                */
  byte fight_timer;              /* Fight timer update              */
  struct mob_attacks_data *next; /* next record in mob_attacks list */
  unsigned int slow_attack;      /* For slow attacks                */
};

struct mob_equipment_data {
  byte pos;                        /* equipment position to load in */
  byte chance;                     /* chance that item will load    */
  int rnum;                        /* realnumber of item to load    */
  int max;                         /* max existing pieces in game   */
  int vnum;                        /* object vnum                   */
  struct mob_equipment_data *next; /* next equipment record         */
};

struct mob_action_data {
  byte chance;                  /* chance that action will occure */
  byte minpos;                  /* minimum position               */
  char *action;                 /* action string                  */
  struct mob_action_data *next; /* next action record             */
};

#define NUM_MODIFY 5 /* Number of modifications a spell can make */

/* An affect structure. */
struct affected_type {
  sh_int type;              /* The type of spell that caused this       */
  sh_int duration;          /* For how long its effects will last       */
  int modifier[NUM_MODIFY]; /* This is added to apropriate ability      */
  int location[NUM_MODIFY]; /* Tells which ability to change(APPLY_XXX) */
  long bitvector;           /* Tells which bits to set (AFF_XXX)        */
  long bitvector2;          /* Tells which bits to set (AFF2_XXX)       */
  long bitvector3;          /* Tells which bits to set (AFF3_XXX)       */

  struct affected_type *next;
};

/* Structure used for chars following other chars */
struct follow_type {
  struct char_data *follower;
  struct follow_type *next;
};

struct spell_slot_type {
  int spellindex; /* spellindex for spell in memory            */
  int has_mem;    /* number stored in memory                   */
  int is_mem;     /* number attempting to memorize             */
  int time_left;  /* time left to mem                          */
  int rate;       /* time it takes to memorize 1 of this spell */
  int circle;     /* Circle of the spell                       */
};

struct consent_data {
  char *name;
  struct consent_data *prev;
  struct consent_data *next;
};

/* ================== Structure for player/non-player ===================== */
/* CHAR_DATA */
struct char_data {
  int pfilepos;         /* playerfile pos                      */
  int nr;               /* Mob's rnum                          */
  room_num in_room;     /* Location (real room number)         */
  room_num was_in_room; /* location for linkdead people        */
  room_num loadin;      /* location mobs load in               */

  struct char_player_data player;              /* Normal data                         */
  struct char_ability_data real_abils;         /* Abilities without modifiers         */
  struct char_ability_data aff_abils;          /* Abils with spells/stones/etc        */
  struct char_point_data points;               /* Points                              */
  struct char_special_data char_specials;      /* PC/NPC specials                     */
  struct player_special_data *player_specials; /* PC specials                         */
  struct mob_special_data mob_specials;        /* NPC specials                        */

  struct affected_type *affected;        /* affected by what spells             */
  struct obj_data *equipment[NUM_WEARS]; /* Equipment array                     */
  struct obj_data *dragging;             /* obj char is dragging                */
  int wearing;                           /* bitvector of filled eq slots        */

  struct obj_data *carrying;    /* Head of list                        */
  struct descriptor_data *desc; /* NULL for mobiles                    */

  struct char_data *next_in_room;  /* For room->people - list             */
  struct char_data *next;          /* For either monster or ppl-list      */
  struct char_data *next_fighting; /* For fighting list                   */

  struct follow_type *followers; /* List of chars followers             */
  struct char_data *master;      /* Who is char following?              */
  MPROG_ACT_LIST *mpact;
  int mpactnum;
  int spec_vars[3]; /* vars to be used by spec_procs       */
  struct consent_data *consent;
  char *animated_name; /* holds the name of animated mob      */
  char *animated_desc; /* holds the desc of animated mob      */
  int animated;        /* set to 1 if animated mob            */
  int screen_height;   /* number of lines for screen          */
  int screen_width;    /* number of cols for screen           */
  int charnum;         /* used to count chars for word wrap   */
  char *pointer;
  int pagesize;
  char *temp;
  char wordwrap[80];
  char *wordp;
  struct spell_slot_type spell_memory[65];
  char can_mem[11]; /* number of spells per circle can mem */
  struct spell_info_type *casting;
  struct spell_info_type *scribing;
  int olc_zones[4]; /* up to 4 zones assigned to a player  */
};
/* ====================================================================== */

/* descriptor-related structures ******************************************/

struct txt_block {
  char *text;
  int aliased;
  struct txt_block *next;
};

struct txt_q {
  struct txt_block *head;
  struct txt_block *tail;
};

/* DESCRIPTOR_DATA */
struct descriptor_data {
  int descriptor; /* file descriptor for socket           */
  int ident_sock;
  int lookup_status;
  int idle_tics; /* checks idle during play              */
  int idle_cnt;  /* checks idle during connection        */
  u_short peer_port;
  char host[HOST_LENGTH + 1];
  char username[256];    /* username ident returns               */
  char hostIP[256];      /* holds the ipaddress as a dotted quad */
  char hostname[256];    /* holds the text name of the ipaddress */
  char *namecolor;       /* color for name in who                */
  byte bad_pws;          /* number of bad pw attemps this login  */
  int connected;         /* mode of 'connectedness'              */
  int wait;              /* wait for how many loops              */
  int desc_num;          /* unique num assigned to desc          */
  time_t login_time;     /* when the person connected            */
  char *showstr_head;    /* beginning of string to be shown      */
  char **showstr_vector; /* for paging through texts             */
  int showstr_count;     /* number of pages to page through      */
  int showstr_page;      /* which page are we currently showing? */
  char *showstr_point;
  char **str;    /* for the modify-str system            */
  char *backstr; /* added for handling abort buffers     */
  int max_str;
  char *mail_to;                     /* name for mail system                 */
  int prompt_mode;                   /* control of prompt-printing           */
  char inbuf[MAX_RAW_INPUT_LENGTH];  /* buffer for raw input                 */
  char last_input[MAX_INPUT_LENGTH]; /* the last input                       */
  char small_outbuf[SMALL_BUFSIZE];  /* standard output buffer               */
  char *output;                      /* ptr to the current output buffer     */
  int bufptr;                        /* ptr to end of current output         */
  int bufspace;                      /* space left in the output buffer      */
  struct txt_block *large_outbuf;    /* ptr to large buffer, if we need it   */
  struct txt_q input;                /* q of unprocessed input               */
  struct char_data *character;       /* linked to char                       */
  struct char_data *original;        /* original char if switched            */
  struct descriptor_data *snooping;  /* Who is this char snooping            */
  struct descriptor_data *snoop_by;  /* And who is snooping this char        */
  struct descriptor_data *next;      /* link to next descriptor              */
  char *storage;

  int color;        /* ANSI color from the start            */
  int edit_number2; /* misc number for editing              */
  void **misc_data; /* misc data, for extra data crap       */
  struct olc_data *olc;
};

/* other miscellaneous structures ***************************************/

struct msg_type {
  char *attacker_msg; /* message to attacker */
  char *victim_msg;   /* message to victim   */
  char *room_msg;     /* message to room     */
};

struct message_type {
  struct msg_type die_msg;   /* messages when death            */
  struct msg_type miss_msg;  /* messages when miss             */
  struct msg_type hit_msg;   /* messages when hit              */
  struct msg_type god_msg;   /* messages when hit on god       */
  struct message_type *next; /* to next messages of this kind. */
};

struct message_list {
  int a_type;               /* Attack type                             */
  int number_of_attacks;    /* How many attack messages to chose from. */
  struct message_type *msg; /* List of messages.                       */
};

struct actd_msg {
  int actd_nr;
  char *char_no_arg;
  char *others_no_arg;
  char *char_found;
  char *others_found;
  char *vict_found;
  char *not_found;
  char *char_auto;
  char *others_auto;
  char *char_object;
  char *others_object;
  struct actd_msg *next;
};

struct dex_skill_type {
  sh_int p_pocket;
  sh_int p_locks;
  sh_int traps;
  sh_int sneak;
  sh_int hide;
};

struct dex_app_type {
  sh_int reaction;
  sh_int miss_att;
  sh_int defensive;
};

struct str_app_type {
  sh_int tohit;   /* To Hit (THAC0) Bonus/Penalty        */
  sh_int todam;   /* Damage Bonus/Penalty                */
  sh_int carry_w; /* Maximum weight that can be carrried */
  sh_int wield_w; /* Maximum weight that can be wielded  */
};

struct wis_app_type {
  byte bonus; /* how many practices player gains per lev */
};

struct int_app_type {
  byte learn; /* how many % a player learns a spell/skill */
};

struct con_app_type {
  sh_int hitp;
  sh_int shock;
};

/* element in the balancing info table */
struct balance_type {
  int hit_dicenum;
  int hit_dicesize;
  int hit_add;
  int exp;
  sbyte thaco;
  int ac;
  sbyte dam_dicenum;
  sbyte dam_dicesize;
  sbyte dam_add;
  int gold;
};

/* element in monster and object index-tables   */
struct index_data {
  int virtual;          /* virtual number of this mob/obj           */
  int number;           /* number of existing units of this mob/obj */
  struct qic_data *qic; /* QIC info database                        */
  int progtypes;        /* Program types for MOBprogs               */
  MPROG_DATA *mobprogs; /* programs for MOBprog                     */
  SPECIAL(*func);
  int spec_type; /* type of spec_proc assigned to mobile     */
  int rent;
};

/* mob_prog that has been delayed */

struct delayed_mprog_type {
  int delay;

  char *remaining_cmnds;

  struct char_data *mob;
  struct char_data *actor;
  struct char_data *rndm;

  struct obj_data *obj;
  void *vo;

  struct delayed_mprog_type *next;
  struct delayed_mprog_type *prev;
};

/* used for medit stuff */

struct max_mob_stats {
  int level;
  int hp_dice;
  int hp_sides;
  int hp_bonus;
  int experience;
  int gold;
  int thac0;
  int ac;
  int damage;
};

struct lastinfo {
  int PlayerNum;
  int NumberConnects;
  char Name[MAX_NAME_LENGTH];
  int Time;
  struct lastinfo *prev;
  struct lastinfo *next;
};

/*
 struct sort_struct {
 int sort_pos;
 byte is_social;
 };
 */

/* struct for portals/ferries/etc.... */
struct transport_data {
  int type;             /* SCMD type to use on command line                    */
  int obj_vnum;         /* Vnum of Obj                                         */
  int room_vnum;        /* Vnum of room represented by obj_vnum (on the ferry) */
  int enter_vnum;       /* Room to enter/exit from                             */
  int exit_vnum;        /* Room to enter/exit from                             */
  char *enter_char_msg; /* Message sent to char entering                       */
  char *enter_room_msg; /* Message sent room when char enters                  */
  char *dest_room_msg;  /* Message sent to dest room                           */
  char *enter_char_msg2;
  char *enter_room_msg2;
  char *dest_room_msg2;
  char *transport_mv_msg; /* Message for a closed portal                         */
};

/**********************************************************************
 * Definitions for new zone-based weather routines. Made
 * 12/23/91 by Smandoggi/Dbra/abradfor. This is for DikuMUD, and whatever
 * legal things that apply to it, apply here as well, so there.
 *********************************************************************/

/* Season patterns */
#define ONE_SEASON              1
#define TWO_SEASONS_EQUAL       2
#define TWO_SEASONS_FIRST_LONG  3
#define TWO_SEASONS_SECOND_LONG 4
#define THREE_SEASONS_EQUAL     5
#define FOUR_SEASONS_EQUAL      6
#define FOUR_SEASONS_EVEN_LONG  7
#define FOUR_SEASONS_ODD_LONG   8

#define MAX_SEASONS 4

/* Seasonal Wind characteristics */
#define SEASON_CALM      1
#define SEASON_BREEZY    2
#define SEASON_UNSETTLED 3
#define SEASON_WINDY     4
#define SEASON_CHINOOK   5
#define SEASON_VIOLENT   6
#define SEASON_HURRICANE 7

/* Seasonal Precipitation characteristics */
#define SEASON_NO_PRECIP_EVER  1
#define SEASON_ARID            2
#define SEASON_DRY             3
#define SEASON_LOW_PRECIP      4
#define SEASON_AVG_PRECIP      5
#define SEASON_HIGH_PRECIP     6
#define SEASON_STORMY          7
#define SEASON_TORRENT         8
#define SEASON_CONSTANT_PRECIP 9 /* Interesting if it's cold enough to snow */

/* Seasonal Temperature characteristics */
#define SEASON_FROSTBITE  1 /* Need to keep warm...*/
#define SEASON_NIPPY      2
#define SEASON_FREEZING   3
#define SEASON_COLD       4
#define SEASON_COOL       5
#define SEASON_MILD       6
#define SEASON_WARM       7
#define SEASON_HOT        8
#define SEASON_BLUSTERY   9
#define SEASON_HEATSTROKE 10 /* Definite HP loss for these two */
#define SEASON_BOILING    11

/* Flags */
#define NO_MOON_EVER     (1 << 0)
#define NO_SUN_EVER      (1 << 1)
#define NON_CONTROLLABLE (1 << 2)
#define AFFECTS_INDOORS  (1 << 3)

#define NUM_ZONE_SEASON_FLAGS 4

struct climate_data {
  int season_pattern;
  int season_wind[MAX_SEASONS];
  int season_wind_dir[MAX_SEASONS];
  int season_wind_variance[MAX_SEASONS];
  int season_precip[MAX_SEASONS];
  int season_temp[MAX_SEASONS];
  int flags;
  int energy_add;
};

/* Weather flags */
#define MOON_VISIBLE       1
#define SUN_VISIBLE        2
#define WEATHER_CONTROLLED 4 /* Idea for expansion */

struct new_weather_data {
  signed char temp; /* In Celsius... So what if I'm a yankee, I */
                    /* still prefer the metric system */
  signed char humidity;
  signed char precip_rate;
  int windspeed;
  char wind_dir;
  int pressure;       /* Kept from previous system */
  char ambient_light; /* Interaction between sun, moon, */
                      /* clouds, etc. Local lights ignored */
  int free_energy;
  char flags;
  char precip_depth; /* Snowpack, flood level */
  char pressure_change;
  char precip_change;
};

/**********************************************************************/
/*         Zone Defines                                               */
/**********************************************************************/
/* structure for the reset commands */
struct reset_com {
  char command; /* current command                      */

  bool if_flag; /* if TRUE: exe only if preceding exe'd */
  int arg1;     /*                                      */
  int arg2;     /* Arguments to the command             */
  int arg3;     /*                                      */
  int arg4;     /*                                      */
  int line;     /* line number this command appears on  */

  /*
   *  Commands:                 *
   *  'M': Read a mobile        *
   *  'O': Read an object       *
   *  'G': Give obj to mob      *
   *  'P': Put obj in obj       *
   *  'G': Obj to char          *
   *  'E': Obj to char equip    *
   *  'D': Set state of door    *
   *  'R': Remove obj from room *
   */
};

/* zone definition structure. for the 'zone-table'   */
struct zone_data {
  char *name;   /* name of this zone                  */
  int lifespan; /* how long between resets (minutes)  */
  int age;      /* current age of this zone (minutes) */
  int top;      /* upper limit for rooms in this zone */
  int bottom;   /* lower limit for rooms in this zone */

  int reset_mode; /* conditions for reset (see below)   */
  int number;     /* virtual number of this zone    */
  int bits;       /* Bitvector - certain bits.. shrug   */
  /* 1 a Restricted                     */
  /* 2 b !Teleport                      */
  struct reset_com *cmd; /* command table for reset            */
  struct climate_data climate;
  struct new_weather_data conditions;

  /*
   *  Reset mode:                              *
   *  0: Don't reset, and don't update age.    *
   *  1: Reset if no PC's are located in zone. *
   *  2: Just reset.                           *
   */
};
/**********************************************************************/

struct corpse_obj_save {
  struct obj_data *next_obj; /* pointer to the obj after the container */
  int level;                 /* level of the container */
  int containernum;          /* vnum of the container */
  struct corpse_obj_save *prev;
};

/* Quest related stuff */

#define QUEST_SINGLEPLAYER (1 << 0) /* a quest is singleplayer only */
#define QUEST_MULTIPLAYER  (1 << 1) /* b quest is multiplayer */
#define QUEST_INORDER      (1 << 2) /* c quest must be done inorder */
#define QUEST_ONCEPLAYER   (1 << 3) /* d quest can be done once per player */
#define QUEST_ONCEBOOT     (1 << 4) /* e quest can be done once per boot */
#define QUEST_COMPLETED    (1 << 5) /* f quest has been completed */

#define QUEST_MONEY  0
#define QUEST_OBJECT 1

#define GOAL_KNOWLEDGE  0
#define GOAL_OBJECT     1
#define GOAL_EXPERIENCE 2
#define GOAL_SKILL      3

/* QUEST_DATA */
struct quest_message_data {
  char *keywords;
  char *message;
};

struct quest_participant_data {
  int id_num;
  int given;
  int complete;
  struct quest_participant_data *next;
};

struct quest_needs_data {
  int vnum;
  int amount;
  int destroy;
  int type;
  int complete;
  char *need_more_msg;
  char *needs_complete_msg;
  struct quest_participant_data *participants;
};

struct quest_data {
  int qnum;
  struct quest_message_data *messages;
  struct quest_needs_data *needs;
  char *knowledge;
  char *skillname;
  int classlist;
  int racelist;
  int maxlevel;
  int goal;
  int value;
  int flags;
  int maxmsgs;
  int maxneeds;
};

#endif /* _STRUCTS_H_ */
