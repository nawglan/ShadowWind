/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  _TwyliteMud_ by Rv.                          Based on CircleMud3.0bpl9 *
 *    				                                          *
 *  OasisOLC - olc.h 		                                          *
 *    				                                          *
 *  Copyright 1996 Harvey Gilpin.                                          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*. Macros, defines, structs and globals for the OLC suite .*/

/*. Utils exported from olc.c .*/
void strip_string(char *);
void cleanup_olc(struct descriptor_data *d, byte cleanup_type);
void get_char_cols(struct char_data *ch);
void olc_add_to_save_list(int zone, char type);
void olc_remove_from_save_list(int zone, char type);

#define OASIS_MPROG

/*. OLC structs .*/

struct olc_data {
  int mode;
  int zone_num;
  int number;
  int value;
  struct char_data *mob;
  struct room_data *room;
  struct obj_data *obj;
  struct zone_data *zone;
  struct shop_data *shop;
  struct extra_descr_data *desc;
  struct mob_prog_data *mprog;
  struct mob_prog_data *mprogl;
};

struct olc_save_info {
  int zone;
  char type;
  struct olc_save_info *next;
};

/*. Exported globals .*/
#ifdef _RV_OLC_
char *nrm, *grn, *cyn, *yel;
struct olc_save_info *olc_save_list = NULL;
#else
extern char *nrm, *grn, *cyn, *yel;
extern struct olc_save_info *olc_save_list;
#endif

/*. Descriptor access macros .*/
#define OLC_MODE(d)   ((d)->olc->mode)     /*. Parse input mode	.*/
#define OLC_NUM(d)    ((d)->olc->number)   /*. Room/Obj VNUM 	.*/
#define OLC_VAL(d)    ((d)->olc->value)    /*. Scratch variable	.*/
#define OLC_ZNUM(d)   ((d)->olc->zone_num) /*. Real zone number	.*/
#define OLC_ROOM(d)   ((d)->olc->room)     /*. Room structure	.*/
#define OLC_OBJ(d)    ((d)->olc->obj)      /*. Object structure	.*/
#define OLC_ZONE(d)   ((d)->olc->zone)     /*. Zone structure	.*/
#define OLC_MOB(d)    ((d)->olc->mob)      /*. Mob structure	.*/
#define OLC_SHOP(d)   ((d)->olc->shop)     /*. Shop structure	.*/
#define OLC_DESC(d)   ((d)->olc->desc)     /*. Extra description	.*/
#define OLC_MPROG(d)  ((d)->olc->mprog)    /*. Temp Mob prog	.*/
#define OLC_MPROGL(d) ((d)->olc->mprogl)   /*. Mob prog list	.*/

/*. Other macros .*/

#define OLC_EXIT(d)     (OLC_ROOM(d)->dir_option[OLC_VAL(d)])
#define GET_OLC_ZONE(c) ((c)->player_specials->saved.olc_zone)

/*. Cleanup types .*/
#define CLEANUP_ALL     1 /*. Free the whole lot  .*/
#define CLEANUP_STRUCTS 2 /*. Don't free strings  .*/

/*. Add/Remove save list types	.*/
#define OLC_SAVE_ROOM 0
#define OLC_SAVE_OBJ  1
#define OLC_SAVE_ZONE 2
#define OLC_SAVE_MOB  3
#define OLC_SAVE_SHOP 4

/* Submodes of OEDIT connectedness */
#define OEDIT_MAIN_MENU             1
#define OEDIT_EDIT_NAMELIST         2
#define OEDIT_SHORTDESC             3
#define OEDIT_LONGDESC              4
#define OEDIT_ACTDESC               5
#define OEDIT_TYPE                  6
#define OEDIT_EXTRAS                7
#define OEDIT_WEAR                  8
#define OEDIT_WEIGHT                9
#define OEDIT_COST                  10
#define OEDIT_COSTPERDAY            11
#define OEDIT_TIMER                 12
#define OEDIT_VALUE_1               13
#define OEDIT_VALUE_2               14
#define OEDIT_VALUE_3               15
#define OEDIT_VALUE_4               16
#define OEDIT_APPLY                 17
#define OEDIT_APPLYMOD              18
#define OEDIT_EXTRADESC_KEY         19
#define OEDIT_CONFIRM_SAVEDB        20
#define OEDIT_CONFIRM_SAVESTRING    21
#define OEDIT_PROMPT_APPLY          22
#define OEDIT_EXTRADESC_DESCRIPTION 23
#define OEDIT_EXTRADESC_MENU        24
#define OEDIT_LEVEL                 25
#define OEDIT_MATERIAL              26
#define OEDIT_RESIST                27
#define OEDIT_RESIST_MOD            28
#define OEDIT_BITVECTOR             29
#define OEDIT_SLOTS                 30
#define OEDIT_SVAL1                 31
#define OEDIT_SVAL2                 32
#define OEDIT_SVAL3                 33
#define OEDIT_BITVECTOR2            34

/* Submodes of REDIT connectedness */
#define REDIT_MAIN_MENU             1
#define REDIT_NAME                  2
#define REDIT_DESC                  3
#define REDIT_FLAGS                 4
#define REDIT_SECTOR                5
#define REDIT_EXIT_MENU             6
#define REDIT_CONFIRM_SAVEDB        7
#define REDIT_CONFIRM_SAVESTRING    8
#define REDIT_EXIT_NUMBER           9
#define REDIT_EXIT_DESCRIPTION      10
#define REDIT_EXIT_KEYWORD          11
#define REDIT_EXIT_KEY              12
#define REDIT_EXIT_DOORFLAGS        13
#define REDIT_EXTRADESC_MENU        14
#define REDIT_EXTRADESC_KEY         15
#define REDIT_EXTRADESC_DESCRIPTION 16
#define REDIT_ADD_MOVE              17

/*. Submodes of ZEDIT connectedness 	.*/
#define ZEDIT_MAIN_MENU            0
#define ZEDIT_DELETE_ENTRY         1
#define ZEDIT_NEW_ENTRY            2
#define ZEDIT_CHANGE_ENTRY         3
#define ZEDIT_COMMAND_TYPE         4
#define ZEDIT_IF_FLAG              5
#define ZEDIT_ARG1                 6
#define ZEDIT_ARG2                 7
#define ZEDIT_ARG3                 8
#define ZEDIT_ZONE_NAME            9
#define ZEDIT_ZONE_LIFE            10
#define ZEDIT_ZONE_TOP             11
#define ZEDIT_ZONE_RESET           12
#define ZEDIT_CONFIRM_SAVESTRING   13
#define ZEDIT_ZONE_EXTRAS          14
#define ZEDIT_ZONE_CLIMATE         15
#define ZEDIT_ZONE_SEASON_PATTERN  16
#define ZEDIT_ZONE_SEASON_FLAGS    17
#define ZEDIT_ZONE_SEASON_ENERGY   18
#define ZEDIT_ZONE_SEASON_WIND     19
#define ZEDIT_ZONE_SEASON_VAR      20
#define ZEDIT_ZONE_SEASON_WINDDIR  21
#define ZEDIT_ZONE_SEASON_PRECIP   22
#define ZEDIT_ZONE_SEASON_TEMP     23
#define ZEDIT_ZONE_SEASON_WIND1    24
#define ZEDIT_ZONE_SEASON_WIND2    25
#define ZEDIT_ZONE_SEASON_WIND3    26
#define ZEDIT_ZONE_SEASON_WIND4    27
#define ZEDIT_ZONE_SEASON_VAR1     28
#define ZEDIT_ZONE_SEASON_VAR2     29
#define ZEDIT_ZONE_SEASON_VAR3     30
#define ZEDIT_ZONE_SEASON_VAR4     31
#define ZEDIT_ZONE_SEASON_WINDDIR1 32
#define ZEDIT_ZONE_SEASON_WINDDIR2 33
#define ZEDIT_ZONE_SEASON_WINDDIR3 34
#define ZEDIT_ZONE_SEASON_WINDDIR4 35
#define ZEDIT_ZONE_SEASON_PRECIP1  36
#define ZEDIT_ZONE_SEASON_PRECIP2  37
#define ZEDIT_ZONE_SEASON_PRECIP3  38
#define ZEDIT_ZONE_SEASON_PRECIP4  39
#define ZEDIT_ZONE_SEASON_TEMP1    40
#define ZEDIT_ZONE_SEASON_TEMP2    41
#define ZEDIT_ZONE_SEASON_TEMP3    42
#define ZEDIT_ZONE_SEASON_TEMP4    43
#define ZEDIT_ARG4                 44

/*. Submodes of MEDIT connectedness 	.*/
#define MEDIT_MAIN_MENU          0
#define MEDIT_ALIAS              1
#define MEDIT_S_DESC             2
#define MEDIT_L_DESC             3
#define MEDIT_D_DESC             4
#define MEDIT_NPC_FLAGS          5
#define MEDIT_AFF_FLAGS          6
#define MEDIT_CONFIRM_SAVESTRING 7
#define MEDIT_ATTACK_MENU        8
#define MEDIT_CHANGE_ATTACK      9
#define MEDIT_ATTACK_NEW         10
#define MEDIT_ATTACK_PURGE       11
#define MEDIT_ACTION_MENU        12
#define MEDIT_CHANGE_ACTION      13
#define MEDIT_ACTION_NEW         14
#define MEDIT_ACTION_PURGE       15
#define MEDIT_EQUIP_MENU         16
#define MEDIT_CHANGE_EQUIP       17
#define MEDIT_EQUIP_NEW          18
#define MEDIT_EQUIP_PURGE        19

/*. Numerical responses .*/
#define MEDIT_NUMERICAL_RESPONSE 20
#define MEDIT_SEX                21
#define MEDIT_DAMROLL            22
#define MEDIT_NDD                23
#define MEDIT_SDD                24
#define MEDIT_NUM_HP_DICE        25
#define MEDIT_SIZE_HP_DICE       26
#define MEDIT_ADD_HP             27
#define MEDIT_AC                 28
#define MEDIT_EXP                29
#define MEDIT_GOLD               30
#define MEDIT_POS                31
#define MEDIT_DEFAULT_POS        32
#define MEDIT_ATTACK             33
#define MEDIT_LEVEL              34
#define MEDIT_ALIGNMENT          35
#define MEDIT_ATTACK_SPEED       36
#define MEDIT_ACTION_PERCENT     37
#define MEDIT_ACTION_POS         38
#define MEDIT_ACTION_ACTION      39
#define MEDIT_EQUIP_CHANCE       40
#define MEDIT_EQUIP_NUMBER       41
#define MEDIT_EQUIP_MAXLOAD      42
#define MEDIT_EQUIP_POS          43
#define MEDIT_THAC0              44
#define MEDIT_E_SPEC             45
#define MEDIT_CHANGE_STR         46
#define MEDIT_CHANGE_ADD         47
#define MEDIT_CHANGE_DEX         48
#define MEDIT_CHANGE_AGI         49
#define MEDIT_CHANGE_INT         50
#define MEDIT_CHANGE_WIS         51
#define MEDIT_CHANGE_CON         52
#define MEDIT_MPROG              53
#define MEDIT_CHANGE_MPROG       54
#define MEDIT_MPROG_COMLIST      55
#define MEDIT_MPROG_ARGS         56
#define MEDIT_MPROG_TYPE         57
#define MEDIT_PURGE_MPROG        58
#define MEDIT_RESIST             59
#define MEDIT_RESIST_MOD         60
#define MEDIT_CLASS              61
#define MEDIT_RACE               62
#define MEDIT_SIZE               63
#define MEDIT_AFF2_FLAGS         64
#define MEDIT_QUESTNUM           65

/*. Submodes of SEDIT connectedness 	.*/
#define SEDIT_MAIN_MENU          0
#define SEDIT_CONFIRM_SAVESTRING 1
#define SEDIT_NOITEM1            2
#define SEDIT_NOITEM2            3
#define SEDIT_NOCASH1            4
#define SEDIT_NOCASH2            5
#define SEDIT_NOBUY              6
#define SEDIT_BUY                7
#define SEDIT_SELL               8
#define SEDIT_PRODUCTS_MENU      11
#define SEDIT_ROOMS_MENU         12
#define SEDIT_NAMELIST_MENU      13
#define SEDIT_NAMELIST           14
/*. Numerical responses .*/
#define SEDIT_NUMERICAL_RESPONSE 20
#define SEDIT_OPEN1              21
#define SEDIT_OPEN2              22
#define SEDIT_CLOSE1             23
#define SEDIT_CLOSE2             24
#define SEDIT_KEEPER             25
#define SEDIT_BUY_PROFIT         26
#define SEDIT_SELL_PROFIT        27
#define SEDIT_TYPE_MENU          29
#define SEDIT_DELETE_TYPE        30
#define SEDIT_DELETE_PRODUCT     31
#define SEDIT_NEW_PRODUCT        32
#define SEDIT_DELETE_ROOM        33
#define SEDIT_NEW_ROOM           34
#define SEDIT_SHOP_FLAGS         35
#define SEDIT_NOTRADE            36

/*. Limit info .*/
#define MAX_ROOM_NAME  75
#define MAX_MOB_NAME   50
#define MAX_OBJ_NAME   50
#define MAX_ROOM_DESC  1024
#define MAX_EXIT_DESC  256
#define MAX_EXTRA_DESC 512
#define MAX_MOB_DESC   512
#define MAX_OBJ_DESC   512
