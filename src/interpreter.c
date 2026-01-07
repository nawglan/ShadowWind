/************************************************************************
 *   File: interpreter.c                                 Part of CircleMUD *
 *  Usage: parse user commands, search for specials, call ACMD functions   *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#define __INTERPRETER_C__

#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>

#include "structs.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "utils.h"
#include "event.h"
#include "spells.h"
#include "handler.h"
#include "mail.h"
#include "screen.h"

extern char *motd;
extern char *nmotd;
extern char *imotd;
extern char *background;
extern char *START_MESSG;
extern struct char_data *character_list;
extern int restrict_game_lvl;
extern int max_players;
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct room_data *world;
extern void load_text(struct char_data * ch);
extern void save_text(struct char_data * ch);
struct lastinfo *LastInfo = NULL;

/* external functions */
void add_innates(struct char_data *ch);
void echo_on(struct descriptor_data * d);
void echo_off(struct descriptor_data * d);
void do_start(struct char_data * ch);
void display_stats(struct char_data *ch);
void roll_real_abils(struct char_data * ch);
void init_char(struct char_data * ch);
int get_idnum();
int result;
int special(struct char_data * ch, int cmd, char *arg);
int isbanned(char *hostname);
int Valid_Name(char *newname);
extern void oedit_parse(struct descriptor_data * d, char *arg);
extern void redit_parse(struct descriptor_data * d, char *arg);
extern void zedit_parse(struct descriptor_data * d, char *arg);
extern void medit_parse(struct descriptor_data * d, char *arg);
extern void sedit_parse(struct descriptor_data * d, char *arg);
void flush_queues(struct descriptor_data * d);
int process_output(struct descriptor_data *t);

/* prototypes for all do_x functions. */
ACMD(do_abort);
ACMD(do_applypoison);
ACMD(do_approve);
ACMD(do_assign);
ACMD(do_decline);
ACMD(do_bootmaze);
ACMD(do_config);
ACMD(do_olc);
ACMD(do_action);
ACMD(do_auclist);
ACMD(do_aucecho);
ACMD(do_actd);
ACMD(do_afk);
/* don't need cuz we don't have an auction anymore
 ACMD(do_auction);
 */
ACMD(do_advance);
ACMD(do_animate);
ACMD(do_alias);
ACMD(do_assist);
ACMD(do_at);
ACMD(do_backstab);
ACMD(do_ban);
ACMD(do_bandage);
ACMD(do_bash);
ACMD(do_camp);
ACMD(do_cast);
ACMD(do_close);
ACMD(do_commands);
ACMD(do_consent);
ACMD(do_consider);
ACMD(do_copyto);
ACMD(do_copyover);
ACMD(do_credits);
ACMD(do_date);
ACMD(do_dc);
ACMD(do_diagnose);
ACMD(do_dig);
ACMD(do_display);
ACMD(do_disengage);
ACMD(do_dismount);
ACMD(do_disarm);
ACMD(do_drag);
ACMD(do_drink);
ACMD(do_drop);
ACMD(do_eat);
ACMD(do_echo);
ACMD(do_enter);
ACMD(do_evaluate);
ACMD(do_equipment);
ACMD(do_examine);
ACMD(do_exit);
ACMD(do_exits);
ACMD(do_exp);
ACMD(do_flee);
ACMD(do_frent);
ACMD(do_follow);
ACMD(do_force);
ACMD(do_fpurge);
ACMD(do_gecho);
ACMD(do_gen_comm);
ACMD(do_gen_ps);
ACMD(do_gen_tog);
ACMD(do_gen_write);
ACMD(do_get);
ACMD(do_give);
ACMD(do_gold);
ACMD(do_goto);
ACMD(do_grab);
ACMD(do_group);
ACMD(do_gsay);
ACMD(do_hcontrol);
ACMD(do_help);
ACMD(do_wiz_help);
ACMD(do_hide);
ACMD(do_hit);
ACMD(do_info);
ACMD(do_insult);
ACMD(do_inventory);
ACMD(do_invis);
ACMD(do_iquest);
ACMD(do_kick);
ACMD(do_kill);
ACMD(do_kit);
ACMD(do_last);
ACMD(do_leave);
ACMD(do_levels);
ACMD(do_load);
ACMD(do_lock);
ACMD(do_logcheck);
ACMD(do_look);
ACMD(do_meditate);
ACMD(do_mobedit);
ACMD(do_mpasound);
ACMD(do_mplog);
ACMD(do_mpcast);
ACMD(do_mpjunk);
ACMD(do_mpdelay);
ACMD(do_mpecho);
ACMD(do_mpechoat);
ACMD(do_mpechoaround);
ACMD(do_mphunt);
ACMD(do_mphuntrm);
ACMD(do_mpkill);
ACMD(do_mprawkill);
ACMD(do_mpmload);
ACMD(do_mpoload);
ACMD(do_mppurge);
ACMD(do_mpgoto);
ACMD(do_mpat);
ACMD(do_mptransfer);
ACMD(do_mptrain);
ACMD(do_mpforce);
ACMD(do_move);
ACMD(do_mount);
ACMD(do_not_here);
ACMD(do_nuke);
ACMD(do_objtochar);
ACMD(do_objedit);
ACMD(do_owners);
ACMD(do_offer);
ACMD(do_open);
ACMD(do_order);
ACMD(do_page);
ACMD(do_pick);
ACMD(do_poofset);
ACMD(do_pour);
ACMD(do_practice);
ACMD(do_purge);
ACMD(do_put);
ACMD(do_qcomm);
ACMD(do_quit);
ACMD(do_qicinfo);
ACMD(do_qicsave);
ACMD(do_qload);
ACMD(do_reboot);
ACMD(do_remove);
ACMD(do_rename);
ACMD(do_rent);
ACMD(do_reply);
ACMD(do_roomedit);
ACMD(do_roundhouse);
ACMD(do_report);
ACMD(do_rescue);
ACMD(do_rest);
ACMD(do_restore);
ACMD(do_return);
ACMD(do_rload);
ACMD(do_save);
ACMD(do_say);
ACMD(do_sac);
ACMD(do_score);
ACMD(do_scribe);
ACMD(do_search);
ACMD(do_send);
ACMD(do_set);
ACMD(do_setqic);
ACMD(do_show);
ACMD(do_shutdown);
ACMD(do_sit);
ACMD(do_skillset);
ACMD(do_sleep);
ACMD(do_sneak);
ACMD(do_snoop);
ACMD(do_spec_comm);
ACMD(do_spells);
ACMD(do_split);
ACMD(do_stand);
ACMD(do_stat);
ACMD(do_stats);
ACMD(do_strings);
ACMD(do_steal);
ACMD(do_switch);
ACMD(do_syslog);
ACMD(do_target);
ACMD(do_tedit);
ACMD(do_teleport);
ACMD(do_tell);
ACMD(do_time);
ACMD(do_title);
ACMD(do_toggle);
ACMD(do_track);
ACMD(do_trans);
ACMD(do_unban);
ACMD(do_ungroup);
ACMD(do_unlock);
ACMD(do_use);
ACMD(do_users);
ACMD(do_scout);
ACMD(do_visible);
ACMD(do_vlist);
ACMD(do_vnum);
ACMD(do_vstat);
ACMD(do_wake);
ACMD(do_wear);
ACMD(do_weather);
ACMD(do_where);
ACMD(do_who);
ACMD(do_whoset);
ACMD(do_whois);
ACMD(do_wield);
ACMD(do_wimpy);
ACMD(do_wizlock);
ACMD(do_wiznet);
ACMD(do_wizutil);
ACMD(do_wizupdate);
ACMD(do_write);
ACMD(do_xname);
ACMD(do_zreset);
ACMD(do_zecho);

/* This is the Master Command List(tm).
 *
 * You can put new commands in, take commands out, change the order
 * they appear in, etc.  You can adjust the "priority" of commands
 * simply by changing the order they appear in the command list.
 * (For example, if you want "as" to mean "assist" instead of "ask",
 * just put "assist" above "ask" in the Master Command List(tm).
 *
 * In general, utility commands such as "at" should have high priority;
 * infrequently used and dangerously destructive commands should have low
 * priority.
 */

const struct command_info cmd_info[] = {

/* this must be first -- for specprocs */
{"RESERVED", 0, 0, 0, 0, 0, 0, 0, 0},

/* directions must come before other commands but after RESERVED */
{"north", POS_STANDING, do_move, 0, SCMD_NORTH, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_MINOR_PARALIZED, 0},
{"east", POS_STANDING, do_move, 0, SCMD_EAST, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_MINOR_PARALIZED, 0},
{"south", POS_STANDING, do_move, 0, SCMD_SOUTH, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_MINOR_PARALIZED, 0},
{"west", POS_STANDING, do_move, 0, SCMD_WEST, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_MINOR_PARALIZED, 0},
{"up", POS_STANDING, do_move, 0, SCMD_UP, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_MINOR_PARALIZED, 0},
{"down", POS_STANDING, do_move, 0, SCMD_DOWN, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_MINOR_PARALIZED, 0},

/* The main list, including commands and socials */
/* The commented out sections below were moved to enable ease of sorting */
/*
 These commands now handled by toggle
{"afk", POS_DEAD, do_afk, 0, 0, 0, 0, 0, 0},
{"autoexit", POS_RESTING, do_gen_tog, 0, SCMD_AUTOEXIT, 0, 0, 0, 0},
{"autogold", POS_RESTING, do_gen_tog, 0, SCMD_AUTOGOLD, 0, 0, 0, 0},
{"autoloot", POS_RESTING, do_gen_tog, 0, SCMD_AUTOLOOT, 0, 0, AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"autosplit", POS_RESTING, do_gen_tog, 0, SCMD_AUTOSPLIT, 0, 0, AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"brief", POS_DEAD, do_gen_tog, 0, SCMD_BRIEF, 0, 0, 0, 0},
{"color", POS_DEAD, do_gen_tog, 0, SCMD_COLOR, 0, 0, 0, 0},
{"compact", POS_DEAD, do_gen_tog, 0, SCMD_COMPACT, 0, 0, 0, 0},
{"display", POS_DEAD, do_display, 0, 0, 0, 0, 0, 0},
{"holylight", POS_DEAD, do_gen_tog, IMMORTAL, SCMD_HOLYLIGHT, COM_IMMORT, 0, 0, 0},
{"mobdeaf", POS_DEAD, do_gen_tog, IMMORTAL, SCMD_MOBDEAF, COM_IMMORT, 0, 0, 0},
{"nohassle", POS_DEAD, do_gen_tog, IMMORTAL, SCMD_NOHASSLE, COM_IMMORT, 0, 0, 0},
{"noiquest", POS_DEAD, do_gen_tog, IMMORTAL, SCMD_IMMQUEST, COM_IMMORT, 0, 0, 0},
{"norepeat", POS_DEAD, do_gen_tog, 0, SCMD_NOREPEAT, 0, 0, 0, 0},
{"noshout", POS_SLEEPING, do_gen_tog, 0, SCMD_DEAF, 0, 0, 0, 0},
{"notell", POS_DEAD, do_gen_tog, 0, SCMD_NOTELL, 0, 0, 0, 0},
{"nowhois", POS_DEAD, do_gen_tog, 0, SCMD_NOWHOIS, 0, 0, 0, 0},
{"nowiz", POS_DEAD, do_gen_tog, IMMORTAL, SCMD_NOWIZ, COM_IMMORT, 0, 0, 0},
{"prompt", POS_DEAD, do_display, 0, 0, 0, 0, 0, 0},
{"roomflags", POS_DEAD, do_gen_tog, IMMORTAL, SCMD_ROOMFLAGS, COM_IMMORT, 0, 0, 0},
{"wimpy", POS_DEAD, do_wimpy, 0, 0, 0, 0, 0, 0},
 */
/*
 Don't need cuz we don't have an auction anymore
{"auction", POS_SLEEPING, do_auction, 0, 0, 0, 0, AFF2_WRAITHFORM, 0},
{"aucecho", POS_DEAD, do_aucecho, ADMIN, 0, COM_ADMIN, 0, 0, 0},
{"auclist", POS_DEAD, do_auclist, BUILDER, 0, COM_BUILDER, 0, 0, 0},
{"noauction", POS_DEAD, do_gen_tog, 0, SCMD_NOAUCTION, 0, 0, AFF2_WRAITHFORM, 0},
 */
/*
 Functions we dont need for one reason or another!
{"donate", POS_RESTING, do_drop, 0, SCMD_DONATE, 0, 0, AFF2_WRAITHFORM, 0},
{"gold", POS_RESTING, do_gold, 0, 0, 0, 0, 0, 0},
{"nosummon", POS_DEAD, do_gen_tog, 0, SCMD_NOSUMMON, 0, 0, 0, 0},
{"objtochar", POS_DEAD, do_objtochar, ADMIN, 0, COM_ADMIN, 0, 0, 0},
{"offer", POS_STANDING, do_not_here, 0, 0, 0, 0, AFF2_WRAITHFORM, 0},
{"wizupdate", POS_DEAD, do_wizupdate, ADMIN, 0, COM_ADMIN, 0, 0, 0},
{"qecho", POS_DEAD, do_qcomm, QUEST, SCMD_QECHO, COM_QUEST, 0, 0, 0},
{"qm", POS_DEAD, do_gen_ps, IMMORTAL, SCMD_QM, COM_IMMORT, 0, 0, 0},
 */
/*
 These commands removed because real editors save - saving these commands
 because we may decide to reinstall them some day to make questing a bit
 easier for those making the quests.
{"mobedit", POS_DEAD, do_mobedit, IMMORTAL, 0, COM_IMMORT, 0, 0, 0},
{"objedit", POS_DEAD, do_objedit, IMMORTAL, 0, COM_IMMORT, 0, 0, 0},
{"roomedit", POS_DEAD, do_roomedit, IMMORTAL, 0, COM_IMMORT, 0, 0, 0},
 */
/*
 Removed socials that were also commands.
{"afk", POS_RESTING, do_action, 0, 0, 0, 0, 0, 0},
{"sing", POS_RESTING, do_action, 0, 0, 0, 0, 0, 0},
 */

{"'", POS_RESTING, do_say, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{":", POS_RESTING, do_echo, 1, SCMD_EMOTE, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{";", POS_DEAD, do_wiznet, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"?", POS_DEAD, do_help, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},

{"abort", POS_RESTING, do_abort, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING, 0},
{"ack", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"actd", POS_DEAD, do_actd, IMMORTAL, 0, COM_IMMORT, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"advance", POS_DEAD, do_advance, ADMIN, 0, COM_ADMIN, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"agree", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"alias", POS_SLEEPING, do_alias, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"amaze", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"animate", POS_DEAD, do_animate, ADMIN, 0, COM_ADMIN, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"anonymous", POS_RESTING, do_gen_tog, 0, SCMD_ANONYMOUS, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"apologize", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"applaud", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"apply poison", POS_RESTING, do_applypoison, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"approve", POS_DEAD, do_approve, IMMORTAL, 0, COM_IMMORT, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"arch", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"ask", POS_RESTING, do_spec_comm, 0, SCMD_ASK, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"assist", POS_FIGHTING, do_assist, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"assign", POS_DEAD, do_assign, ADMIN, 0, COM_ADMIN, 0, 0, 0},
{"at", POS_DEAD, do_at, IMMORTAL, 0, COM_IMMORT, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"attribute", POS_SLEEPING, do_stats, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"ayt", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},

{"backstab", POS_STANDING, do_backstab, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"balance", POS_STANDING, do_not_here, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"ban", POS_DEAD, do_ban, ADMIN, 0, COM_ADMIN, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"bandage", POS_STANDING, do_bandage, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"bang", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"bark", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"bash", POS_FIGHTING, do_bash, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"bathe", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"bbl", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"beer", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"beg", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"bird", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"bite", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"bleed", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"blink", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"blow", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"blush", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"board", POS_STANDING, do_enter, 0, SCMD_BOARD, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_MINOR_PARALIZED, 0},
{"boggle", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"bonk", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"bootmaze", POS_DEAD, do_bootmaze, QUEST, 0, COM_QUEST, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"bored", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"bounce", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"bow", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"brb", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"bug", POS_SLEEPING, do_gen_write, 0, SCMD_BUG, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"burp", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"buy", POS_STANDING, do_not_here, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_MINOR_PARALIZED, 0},
{"bye", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},

{"cast", POS_FIGHTING, do_cast, 0, 0, 0, AFF_SILENCE | AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"cackle", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"calm", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"camp", POS_RESTING, do_camp, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED | AFF2_KNOCKEDOUT, 0},
{"caress", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"censor", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"chat", POS_SLEEPING, do_gen_comm, 0, SCMD_CHAT, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_MINOR_PARALIZED, 0},
{"check", POS_STANDING, do_not_here, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"cheek", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"cheer", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"choke", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"chuckle", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"circle", POS_FIGHTING, do_backstab, 0, SCMD_CIRCLE, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"clap", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"clear", POS_RESTING, do_gen_ps, 0, SCMD_CLEAR, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"close", POS_SITTING, do_close, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"cls", POS_RESTING, do_gen_ps, 0, SCMD_CLEAR, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"comb", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"comfort", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"commands", POS_DEAD, do_commands, 0, SCMD_COMMANDS, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"consider", POS_RESTING, do_consider, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"config", POS_DEAD, do_config, ADMIN, 0, COM_ADMIN, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"congratulate", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"consent", POS_RESTING, do_consent, 0, 0, 0, 0, AFF2_WRAITHFORM, AFF2_SCRIBING | AFF2_CASTING},
{"copyto", POS_DEAD, do_copyto, BUILDER, 0, COM_BUILDER, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"copyove", POS_DEAD, do_shutdown, BUILDER, 0, COM_BUILDER, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"copyover", POS_DEAD, do_copyover, BUILDER, 0, COM_BUILDER, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"cough", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"credits", POS_SLEEPING, do_gen_ps, 0, SCMD_CREDITS, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"cringe", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"cry", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"cuddle", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"curious", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"curse", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"curtsey", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},

{"dance", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"dc", POS_DEAD, do_dc, BUILDER, 0, COM_BUILDER, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"decline", POS_DEAD, do_decline, IMMORTAL, 0, COM_IMMORT, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"deny", POS_DEAD, do_decline, IMMORTAL, 0, COM_IMMORT, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"deposit", POS_STANDING, do_not_here, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"diagnose", POS_RESTING, do_diagnose, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"dig", POS_DEAD, do_dig, BUILDER, 0, COM_BUILDER, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"disarm", POS_STANDING, do_disarm, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"disembark", POS_STANDING, do_leave, 0, SCMD_DISEMBARK, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_MINOR_PARALIZED, 0},
{"disengage", POS_FIGHTING, do_disengage, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"dismount", POS_SITTING, do_dismount, IMMORTAL, 0, COM_IMMORT, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"doh", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"drag", POS_STANDING, do_drag, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"dream", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"drink", POS_RESTING, do_drink, 0, SCMD_DRINK, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"drool", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"drop", POS_RESTING, do_drop, 0, SCMD_DROP, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"dropkick", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"duck", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},

{"eat", POS_RESTING, do_eat, 0, SCMD_EAT, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"echo", POS_SLEEPING, do_echo, IMMORTAL, SCMD_ECHO, COM_IMMORT, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"embrace", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"emote", POS_RESTING, do_echo, 1, SCMD_EMOTE, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"enter", POS_STANDING, do_enter, 0, SCMD_ENTER, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_MINOR_PARALIZED, 0},
{"envy", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"equipment", POS_RESTING, do_equipment, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"evaluate", POS_RESTING, do_evaluate, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"exits", POS_RESTING, do_exits, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"examine", POS_SITTING, do_examine, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"exchange", POS_STANDING, do_not_here, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"experience", POS_RESTING, do_exp, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"eyebrow", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},

{"fart", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"fidget", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"fill", POS_STANDING, do_pour, 0, SCMD_FILL, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"flame", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"flash", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"flee", POS_FIGHTING, do_flee, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"flex", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"flip", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"flirt", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"flutter", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"follow", POS_RESTING, do_follow, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"fondle", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"fool", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"force", POS_SLEEPING, do_force, QUEST, 0, COM_QUEST, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"freeze", POS_DEAD, do_wizutil, BUILDER, SCMD_FREEZE, COM_BUILDER, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"french", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"frent", POS_DEAD, do_frent, ADMIN, 0, COM_ADMIN, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"frown", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"full", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"fume", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"fuzzy", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},

{"gag", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"gape", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"gasp", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"gecho", POS_DEAD, do_gecho, QUEST, 0, COM_QUEST, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"get", POS_RESTING, do_get, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"giggle", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"give", POS_RESTING, do_give, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"glance", POS_RESTING, do_diagnose, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"glare", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"goto", POS_SLEEPING, do_goto, IMMORTAL, 0, COM_IMMORT, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"grab", POS_RESTING, do_grab, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"grin", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"groan", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"grope", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"group", POS_RESTING, do_group, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"grovel", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"growl", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"grumble", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"grunt", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"gsay", POS_SLEEPING, do_gsay, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"gtell", POS_SLEEPING, do_gsay, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},

{"hand", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"handbook", POS_DEAD, do_gen_ps, LVL_IMMORT, SCMD_HANDBOOK, COM_IMMORT, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"happy", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"help", POS_DEAD, do_help, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"helpn", POS_DEAD, do_gen_write, 0, SCMD_HELPN, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"hero", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"hi5", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"hiccup", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"hide", POS_RESTING, do_hide, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"hiss", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"hit", POS_FIGHTING, do_hit, 0, SCMD_HIT, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"hold", POS_RESTING, do_grab, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"holdon", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"hop", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"hug", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"hum", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"hunger", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},

{"inventory", POS_RESTING, do_inventory, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_MINOR_PARALIZED, 0},
{"idea", POS_SLEEPING, do_gen_write, 0, SCMD_IDEA, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"ident", POS_DEAD, do_gen_tog, ADMIN, SCMD_IDENT, COM_ADMIN, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"imitate", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"immlist", POS_DEAD, do_gen_ps, 0, SCMD_IMMLIST, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"imotd", POS_DEAD, do_gen_ps, IMMORTAL, SCMD_IMOTD, COM_IMMORT, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"impale", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"info", POS_DEAD, do_gen_ps, 0, SCMD_INFO, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"insert", POS_STANDING, do_not_here, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"insult", POS_RESTING, do_insult, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"introduce", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"invis", POS_DEAD, do_invis, IMMORTAL, 0, COM_IMMORT, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"iquest", POS_DEAD, do_iquest, QUEST, 0, COM_QUEST, 0, AFF2_SCRIBING | AFF2_CASTING, 0},

{"jam", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"jk", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"jump", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"junk", POS_RESTING, do_drop, IMMORTAL, SCMD_JUNK, COM_IMMORT, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},

{"kill", POS_FIGHTING, do_kill, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"kick", POS_FIGHTING, do_kick, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"kiss", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"kneel", POS_STANDING, do_not_here, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},

{"look", POS_RESTING, do_look, 0, SCMD_LOOK, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"laugh", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"lag", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"last", POS_DEAD, do_last, IMMORTAL, 0, COM_IMMORT, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"lean", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"leave", POS_STANDING, do_leave, 0, SCMD_LEAVE, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_MINOR_PARALIZED, 0},
{"lick", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"list", POS_STANDING, do_not_here, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"load", POS_DEAD, do_load, QUEST, 0, COM_QUEST, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"lock", POS_SITTING, do_lock, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"logcheck", POS_DEAD, do_logcheck, QUEST, 0, COM_QUEST, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"love", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"luck", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},

{"mail", POS_STANDING, do_not_here, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"massage", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"medit", POS_DEAD, do_olc, BUILDER, SCMD_OLC_MEDIT, COM_BUILDER, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"meditate", POS_RESTING, do_meditate, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"melt", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"moan", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"moon", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"mosh", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"motd", POS_DEAD, do_gen_ps, 0, SCMD_MOTD, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"mount", POS_STANDING, do_mount, IMMORTAL, 0, COM_IMMORT, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"mourn", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"mpasound", POS_DEAD, do_mpasound, 0, 0, COM_MOB, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_MINOR_PARALIZED, 0},
{"mpat", POS_DEAD, do_mpat, 0, 0, COM_MOB, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_MINOR_PARALIZED, 0},
{"mpcast", POS_DEAD, do_mpcast, 0, 0, COM_MOB, AFF_SILENCE | AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_MINOR_PARALIZED, 0},
{"mpdelay", POS_DEAD, do_mpdelay, 0, 0, COM_MOB, 0, AFF2_SCRIBING, 0},
{"mpecho", POS_DEAD, do_mpecho, 0, 0, COM_MOB, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_MINOR_PARALIZED, 0},
{"mpechoaround", POS_DEAD, do_mpechoaround, 0, 0, COM_MOB, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_MINOR_PARALIZED, 0},
{"mpechoat", POS_DEAD, do_mpechoat, 0, 0, COM_MOB, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_MINOR_PARALIZED, 0},
{"mpforce", POS_DEAD, do_mpforce, 0, 0, COM_MOB, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_MINOR_PARALIZED, 0},
{"mpgoto", POS_DEAD, do_mpgoto, 0, 0, COM_MOB, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_MINOR_PARALIZED, 0},
{"mphunt", POS_DEAD, do_mphunt, 0, 0, COM_MOB, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_MINOR_PARALIZED, 0},
{"mphuntrm", POS_DEAD, do_mphuntrm, 0, 0, COM_MOB, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_MINOR_PARALIZED, 0},
{"mpjunk", POS_DEAD, do_mpjunk, 0, 0, COM_MOB, 0, AFF2_SCRIBING, 0},
{"mpkill", POS_DEAD, do_mpkill, 0, 0, COM_MOB, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_MINOR_PARALIZED, 0},
{"mplog", POS_DEAD, do_mplog, 0, 0, COM_MOB, 0, AFF2_SCRIBING, 0},
{"mpmload", POS_DEAD, do_mpmload, 0, 0, COM_MOB, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_MINOR_PARALIZED, 0},
{"mpoload", POS_DEAD, do_mpoload, 0, 0, COM_MOB, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_MINOR_PARALIZED, 0},
{"mppurge", POS_DEAD, do_mppurge, 0, 0, COM_MOB, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_MINOR_PARALIZED, 0},
{"mprawkill", POS_DEAD, do_mprawkill, 0, 0, COM_MOB, 0, AFF2_SCRIBING, 0},
{"mptrain", POS_DEAD, do_mptrain, 0, 0, COM_MOB, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_MINOR_PARALIZED, 0},
{"mptransfer", POS_DEAD, do_mptransfer, 0, 0, COM_MOB, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_MINOR_PARALIZED, 0},
{"murder", POS_FIGHTING, do_hit, 0, SCMD_MURDER, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"mute", POS_DEAD, do_wizutil, QUEST, SCMD_SQUELCH, COM_QUEST, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"mutter", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},

{"namecolor", POS_DEAD, do_poofset, IMMORTAL, SCMD_NAMECOLOR, COM_IMMORT, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"nap", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"news", POS_SLEEPING, do_gen_ps, 0, SCMD_NEWS, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"nibble", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"nmotd", POS_DEAD, do_gen_ps, 0, SCMD_NMOTD, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"nod", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"noemote", POS_DEAD, do_wizutil, QUEST, SCMD_NOEMOTE, COM_QUEST, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"nog", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"noogie", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"nose", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"nosocial", POS_DEAD, do_wizutil, QUEST, SCMD_NOSOCIAL, COM_QUEST, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"nudge", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"nuke", POS_DEAD, do_nuke, ADMIN, 0, COM_ADMIN, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"nuzzle", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},

{"oedit", POS_DEAD, do_olc, BUILDER, SCMD_OLC_OEDIT, COM_BUILDER, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"ogle", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"open", POS_SITTING, do_open, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"order", POS_RESTING, do_order, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"outlaw", POS_DEAD, do_wizutil, ADMIN, SCMD_OUTLAW, COM_ADMIN, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"owners", POS_DEAD, do_owners, ADMIN, 0, COM_ADMIN, 0, AFF2_SCRIBING | AFF2_CASTING, 0},

{"page", POS_DEAD, do_page, IMMORTAL, 0, COM_IMMORT, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"panic", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"pant", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"pardon", POS_DEAD, do_wizutil, IMMORTAL, SCMD_PARDON, COM_IMMORT, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"pat", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"peer", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"pet", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"petition", POS_DEAD, do_gen_comm, 0, SCMD_PETI, 0, 0, 0, 0},
{"pick", POS_STANDING, do_pick, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"pillow", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"pinch", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"plonk", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"point", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"poke", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"policy", POS_DEAD, do_gen_ps, 0, SCMD_POLICIES, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"ponder", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"poofin", POS_DEAD, do_poofset, IMMORTAL, SCMD_POOFIN, COM_IMMORT, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"poofout", POS_DEAD, do_poofset, IMMORTAL, SCMD_POOFOUT, COM_IMMORT, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"pounce", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"pour", POS_STANDING, do_pour, 0, SCMD_POUR, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"pout", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"practice", POS_RESTING, do_practice, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"pray", POS_DEAD, do_gen_comm, 0, SCMD_PRAY, 0, 0, 0, 0},
{"prent", POS_DEAD, do_fpurge, ADMIN, SCMD_PRENT, COM_ADMIN, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"protect", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"ptext", POS_DEAD, do_fpurge, ADMIN, SCMD_PTEXT, COM_ADMIN, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"pucker", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"puke", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"pull", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"punch", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"purge", POS_DEAD, do_purge, QUEST, 0, COM_QUEST, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"purr", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"push", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"put", POS_RESTING, do_put, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"puzzle", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},

{"qicinfo", POS_DEAD, do_qicinfo, ADMIN, 0, COM_ADMIN, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"qicsave", POS_DEAD, do_qicsave, ADMIN, 0, COM_ADMIN, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"qload", POS_DEAD, do_qload, ADMIN, 0, COM_ADMIN, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"quaff", POS_RESTING, do_use, 0, SCMD_QUAFF, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"quit", POS_DEAD, do_quit, 0, SCMD_QUIT, 0, 0, AFF2_KNOCKEDOUT | AFF2_SCRIBING | AFF2_CASTING, 0},

{"raise", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"read", POS_RESTING, do_look, 0, SCMD_READ, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"ready", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"recite", POS_RESTING, do_use, 0, SCMD_RECITE, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"receive", POS_STANDING, do_not_here, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"redit", POS_DEAD, do_olc, BUILDER, SCMD_OLC_REDIT, COM_BUILDER, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"reload", POS_DEAD, do_reboot, BUILDER, 0, COM_BUILDER, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"remove", POS_RESTING, do_remove, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"rent", POS_STANDING, do_not_here, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"rename", POS_DEAD, do_rename, ADMIN, 0, COM_ADMIN, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"report", POS_RESTING, do_report, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"reply", POS_RESTING, do_reply, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"reroll", POS_DEAD, do_wizutil, ADMIN, SCMD_REROLL, COM_ADMIN, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"rescue", POS_FIGHTING, do_rescue, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"rest", POS_RESTING, do_rest, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_MINOR_PARALIZED, 0},
{"restore", POS_DEAD, do_restore, ADMIN, 0, COM_ADMIN, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"return", POS_DEAD, do_return, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"rload", POS_DEAD, do_rload, ADMIN, 0, COM_ADMIN, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"roar", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"rofl", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"roll", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"rose", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"roundhouse", POS_FIGHTING, do_roundhouse, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"ruffle", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},

{"sacrifice", POS_STANDING, do_sac, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"salute", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"save", POS_DEAD, do_save, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM, 0},
{"say", POS_RESTING, do_say, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"score", POS_SLEEPING, do_score, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"scan", POS_RESTING, do_exits, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"scare", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"scold", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"scout", POS_STANDING, do_scout, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_MINOR_PARALIZED, 0},
{"scratch", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"scream", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
/*
{"scribe", POS_RESTING, do_scribe, 0, SCMD_MAGE, 0, AFF_CAMPING | AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_MINOR_PARALIZED | AFF2_WRAITHFORM, 0},
 */
{"search", POS_RESTING, do_search, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_MINOR_PARALIZED, 0},
{"sedit", POS_DEAD, do_olc, BUILDER, SCMD_OLC_SEDIT, COM_BUILDER, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"seduce", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"sell", POS_STANDING, do_not_here, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"send", POS_SLEEPING, do_send, IMMORTAL, 0, COM_IMMORT, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"set", POS_DEAD, do_set, QUEST, 0, COM_QUEST, 0, 0, 0},
{"setqic", POS_DEAD, do_setqic, ADMIN, 0, COM_ADMIN, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"shake", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"shiver", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"shout", POS_RESTING, do_gen_comm, 0, SCMD_SHOUT, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_MINOR_PARALIZED, 0},
{"shove", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"show", POS_DEAD, do_show, 0, 0, COM_IMMORT, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"shrug", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"shudder", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"shush", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"shutdow", POS_DEAD, do_shutdown, BUILDER, 0, COM_BUILDER, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"shutdown", POS_DEAD, do_shutdown, BUILDER, SCMD_SHUTDOWN, COM_BUILDER, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"sigh", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"sing", POS_RESTING, do_cast, 0, SCMD_SING, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"sip", POS_RESTING, do_drink, 0, SCMD_SIP, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"sit", POS_RESTING, do_sit, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_MINOR_PARALIZED, 0},
{"skills", POS_RESTING, do_practice, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"skillset", POS_SLEEPING, do_skillset, ADMIN, 0, COM_ADMIN, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"skip", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"sleep", POS_RESTING, do_sleep, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_MINOR_PARALIZED, 0},
{"slap", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"slobber", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"slowns", POS_DEAD, do_gen_tog, ADMIN, SCMD_SLOWNS, COM_ADMIN, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"smell", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"smile", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"smirk", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"smoke", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"snap", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"snarl", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"sneak", POS_STANDING, do_sneak, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"sneer", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"sneeze", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"snicker", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"sniff", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"snoogie", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"snoop", POS_DEAD, do_snoop, ADMIN, 0, COM_ADMIN, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"snore", POS_SLEEPING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"snort", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"snuggle", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"socials", POS_DEAD, do_commands, 0, SCMD_SOCIALS, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"spam", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"spank", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"spells", POS_RESTING, do_spells, 0, 0, COM_IMMORT, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"specialize", POS_RESTING, do_practice, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"spin", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"spit", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"split", POS_SITTING, do_split, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"squeeze", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"squirm", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"stand", POS_RESTING, do_stand, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_MINOR_PARALIZED, 0},
{"stare", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"stat", POS_DEAD, do_stat, IMMORTAL, 0, COM_IMMORT, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"stats", POS_SLEEPING, do_stats, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"steal", POS_STANDING, do_steal, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"steam", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"stomp", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"strangle", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"stretch", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"strings", POS_DEAD, do_strings, IMMORTAL, 0, COM_IMMORT, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"strut", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"sulk", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"swat", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"sweat", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"sweep", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"switch", POS_DEAD, do_switch, QUEST, 0, COM_QUEST, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"syslog", POS_DEAD, do_syslog, IMMORTAL, 0, COM_IMMORT, 0, AFF2_SCRIBING | AFF2_CASTING, 0},

{"tell", POS_RESTING, do_tell, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"tackle", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"take", POS_RESTING, do_get, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"tango", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"tap", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"target", POS_FIGHTING, do_target, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"tarzan", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"taste", POS_RESTING, do_eat, 0, SCMD_TASTE, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"taunt", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"tease", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"tedit", POS_DEAD, do_tedit, BUILDER, 0, COM_BUILDER, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"teleport", POS_DEAD, do_teleport, IMMORTAL, 0, COM_IMMORT, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"thank", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"thaw", POS_DEAD, do_wizutil, BUILDER, SCMD_THAW, COM_BUILDER, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"think", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"thirst", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"throw", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"tickle", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"time", POS_SLEEPING, do_time, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"tip", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"tiptoe", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"todo", POS_DEAD, do_gen_write, ADMIN, SCMD_TODO, COM_ADMIN, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"toggle", POS_DEAD, do_toggle, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"tongue", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"toss", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"touch", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"track", POS_STANDING, do_track, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_MINOR_PARALIZED, 0},
{"train", POS_RESTING, do_practice, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"transfer", POS_SLEEPING, do_trans, QUEST, 0, COM_QUEST, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"trip", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"tug", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"tweak", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"twibble", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"twiddle", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"twirl", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"typo", POS_SLEEPING, do_gen_write, 0, SCMD_TYPO, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},

{"unaffect", POS_DEAD, do_wizutil, QUEST, SCMD_UNAFFECT, COM_QUEST, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"unban", POS_DEAD, do_unban, ADMIN, 0, COM_ADMIN, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"undress", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"ungroup", POS_RESTING, do_ungroup, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"unlock", POS_SITTING, do_unlock, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"use", POS_SITTING, do_use, 0, SCMD_USE, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"users", POS_DEAD, do_users, IMMORTAL, 0, COM_IMMORT, 0, AFF2_SCRIBING | AFF2_CASTING, 0},

{"value", POS_STANDING, do_not_here, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"version", POS_DEAD, do_gen_ps, 0, SCMD_VERSION, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"veto", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"visible", POS_RESTING, do_visible, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"vlist", POS_DEAD, do_vlist, QUEST, 0, COM_QUEST, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"vnum", POS_DEAD, do_vnum, QUEST, 0, COM_QUEST, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"vstat", POS_DEAD, do_vstat, QUEST, 0, COM_QUEST, 0, AFF2_SCRIBING | AFF2_CASTING, 0},

{"wake", POS_SLEEPING, do_wake, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_KNOCKEDOUT | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"wait", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"wave", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"wear", POS_STANDING, do_wear, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"weather", POS_RESTING, do_weather, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"welcome", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"wet", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"who", POS_RESTING, do_who, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"whap", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"whatever", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"wheeze", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"where", POS_RESTING, do_where, IMMORTAL, 0, COM_IMMORT, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"whine", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"whisper", POS_RESTING, do_spec_comm, 0, SCMD_WHISPER, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"whistle", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"whoami", POS_RESTING, do_gen_ps, 0, SCMD_WHOAMI, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"whois", POS_RESTING, do_whois, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"whoops", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"whoset", POS_DEAD, do_whoset, IMMORTAL, 0, COM_IMMORT, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"wield", POS_RESTING, do_wield, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"wiggle", POS_RESTING, do_action, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING, 0},
{"wince", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"wink", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"withdraw", POS_STANDING, do_not_here, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_WRAITHFORM | AFF2_MINOR_PARALIZED, 0},
{"wiznet", POS_DEAD, do_wiznet, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"wizhelp", POS_SLEEPING, do_wiz_help, LVL_IMMORT, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"wizlist", POS_DEAD, do_gen_ps, 0, SCMD_WIZLIST, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"wizlock", POS_DEAD, do_wizlock, ADMIN, 0, COM_ADMIN, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"worship", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"write", POS_STANDING, do_write, 0, 0, 0, AFF_MAJOR_PARALIZED, AFF2_SCRIBING | AFF2_CASTING | AFF2_MINOR_PARALIZED, 0},

{"xname", POS_DEAD, do_xname, ADMIN, 0, COM_ADMIN, 0, AFF2_SCRIBING | AFF2_CASTING, 0},

{"yawn", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"yodel", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},

{"zecho", POS_DEAD, do_zecho, QUEST, 0, COM_QUEST, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"zedit", POS_DEAD, do_olc, BUILDER, SCMD_OLC_ZEDIT, COM_BUILDER, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"zone", POS_RESTING, do_action, 0, 0, 0, 0, AFF2_SCRIBING | AFF2_CASTING, 0},
{"zreset", POS_DEAD, do_zreset, QUEST, 0, COM_QUEST, 0, AFF2_SCRIBING | AFF2_CASTING, 0},

/* this must be last */
{"\n", 0, 0, 0, 0, 0, 0, 0, 0}};

char *fill[] = {"in", "from", "with", "the", "on", "at", "to", "\n"};

char *reserved[] = {"self", "me", "all", "room", "someone", "something", "\n"};

/*
 * This is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you are the proper level and position to execute the command,
 * then calls the appropriate function.
 */
void command_interpreter(struct char_data * ch, char * argument)
{
  int cmd, say_cmd = -1, length;
  extern int level_can_color;
  char *tmpbuf, *tmpbuf2, *tmpbuf3;
  extern int no_specials;
  char *line;
  char astring[256];

  REMOVE_BIT(AFF_FLAGS(ch), AFF_HIDE);

  if (((int) ch->player.level < level_can_color) && !IS_NPC(ch)) {

    tmpbuf2 = argument;
    tmpbuf3 = strdup(argument);
    tmpbuf = tmpbuf3;

    while (*tmpbuf != '\0') {
      *tmpbuf2 = *tmpbuf;
      if (*tmpbuf2 == '{')
        *(++tmpbuf2) = '{';
      tmpbuf++;
      tmpbuf2++;
    }
    *(tmpbuf2) = '\0';
    FREE(tmpbuf3);
  }

  line = any_one_arg(argument, arg);

  /* just drop to next line for hitting CR */
  if (!*arg) {
    return;
  }

  /* otherwise, find the command */
  for (length = strlen(arg), cmd = 0; *cmd_info[cmd].command != '\n'; cmd++) {
    if (strcmp(cmd_info[cmd].command, "say") == 0) {
      if ((GET_LEVEL(ch) >= cmd_info[cmd].minimum_level || (COM_FLAGS(ch) & cmd_info[cmd].flag))) {
        say_cmd = cmd;
      }
    }
    if (strncmp(cmd_info[cmd].command, arg, length) == 0) {
      if ((GET_LEVEL(ch) >= cmd_info[cmd].minimum_level || (COM_FLAGS(ch) & cmd_info[cmd].flag))) {
        break;
      }
    }
  }
  if (*cmd_info[cmd].command == '\n') {
      cmd = say_cmd;
      line = argument;
  }

/*
  if (*cmd_info[cmd].command == '\n')
    switch (number(0, 5)) {
      case 0:
        send_to_char("Try hitting the right keys, will you??\r\n", ch);
        break;
      case 1:
        send_to_char("Excuse me?!?!?\r\n", ch);
        break;
      case 2:
        send_to_char("Ohh... That makes sense!\r\n", ch);
        break;
      case 3:
        send_to_char("Why would you want to do that?!\r\n", ch);
        break;
      case 4:
        send_to_char("I didn't quite understand you?!\r\n", ch);
        break;
      default:
        send_to_char("Want me to invent a new command for you??\r\n", ch);
        break;
  }
  else
*/
  if (cmd_info[cmd].anti_aff && AFF_FLAGGED(ch, cmd_info[cmd].anti_aff)) {
    if (AFF_FLAGGED(ch, AFF_MAJOR_PARALIZED)) {
      sprintf(astring, "You've been paralized! You cannot do that!\r\n");
    } else if (AFF_FLAGGED(ch, AFF_CAMPING)) {
      sprintf(astring, "You cannot %s while camping.\r\n", cmd_info[cmd].command);
    } else {
      sprintf(astring, "You are unable to %s at this time.\r\n", cmd_info[cmd].command);
    }
    send_to_char(astring, ch);
  } else if (cmd_info[cmd].anti_aff2 && AFF2_FLAGGED(ch, cmd_info[cmd].anti_aff2)) {
    if (AFF2_FLAGGED(ch, AFF2_KNOCKEDOUT)) {
      sprintf(astring, "You have been knocked out and cannot do that.\r\n");
    } else if (AFF2_FLAGGED(ch, AFF2_MINOR_PARALIZED)) {
      sprintf(astring, "You've been paralized! You cannot do that!\r\n");
    } else if (AFF2_FLAGGED(ch, AFF2_CASTING)) {
      sprintf(astring, "You're too busy casting!\r\n");
    } else if (AFF2_FLAGGED(ch, AFF2_SCRIBING)) {
      sprintf(astring, "You're too busy scribing!\r\n");
    } else {
      sprintf(astring, "You are unable to %s at this time.\r\n", cmd_info[cmd].command);
    }
    send_to_char(astring, ch);
  } else if (cmd_info[cmd].anti_aff3 && AFF3_FLAGGED(ch, cmd_info[cmd].anti_aff3)) {
    sprintf(astring, "You are unable to %s at this time.\r\n", cmd_info[cmd].command);
    send_to_char(astring, ch);
  } else if (PLR_FLAGGED(ch, PLR_FROZEN) && !COM_FLAGGED(ch, COM_ADMIN))
    send_to_char("You try, but the mind-numbing cold prevents you...\r\n", ch);
  else if (cmd_info[cmd].command_pointer == NULL)
    send_to_char("Sorry, that command hasn't been implemented yet.\r\n", ch);
  else if (IS_NPC(ch) && cmd_info[cmd].minimum_level >= LVL_IMMORT)
    send_to_char("You can't use immortal commands while switched.\r\n", ch);
  else if (GET_POS(ch) < cmd_info[cmd].minimum_position)
    switch (GET_POS(ch)) {
      case POS_DEAD:
        send_to_char("Lie still; you are DEAD!!! :-(\r\n", ch);
        break;
      case POS_INCAP:
      case POS_MORTALLYW:
        send_to_char("You are in a pretty bad shape, unable to do anything!\r\n", ch);
        break;
      case POS_STUNNED:
        send_to_char("All you can do right now is think about the stars!\r\n", ch);
        break;
      case POS_SLEEPING:
        send_to_char("In your dreams, or what?\r\n", ch);
        break;
      case POS_RESTING:
        send_to_char(CBMAG(ch,C_SPR), ch);
        send_to_char("Nah... You feel too relaxed to do that..\r\n", ch);
        send_to_char(CCNRM(ch,C_SPR), ch);
        break;
      case POS_SITTING:
        send_to_char("Maybe you should get on your feet first?\r\n", ch);
        break;
      case POS_FIGHTING:
        send_to_char("No way!  You're fighting for your life!\r\n", ch);
        break;
    }
  else if (no_specials || !special(ch, cmd, line)) {
    if (!IS_NPC(ch) && MEDITATING(ch) && strcmp(cmd_info[cmd].command, "memorize") && strcmp(cmd_info[cmd].command, "scribe")) {
      send_to_char("You stop meditating.\r\n", ch);
      REMOVE_BIT(AFF_FLAGS(ch), AFF_MEDITATING);
    }
    ((*cmd_info[cmd].command_pointer)(ch, line, cmd, cmd_info[cmd].subcmd));
  }
}

/**************************************************************************
 * Routines to handle aliasing                                             *
 **************************************************************************/

struct alias *find_alias(struct alias * alias_list, char *str)
{
  while (alias_list != NULL) {
    if (*str == *alias_list->alias) /* hey, every little bit counts :-) */
      if (!strcmp(str, alias_list->alias))
        return alias_list;

    alias_list = alias_list->next;
  }

  return NULL;
}

void free_alias(struct alias * a)
{
  if (a->alias)
    FREE(a->alias);
  if (a->replacement)
    FREE(a->replacement);
  a->replacement = NULL;
  a->alias = NULL;
  FREE(a);
  a = NULL;
}

/* The interface: do_alias */ACMD(do_alias)
{
  char *repl;
  struct alias *a, *temp;

  if (IS_NPC(ch))
    return;

  repl = any_one_arg(argument, arg);

  if (!*arg) {
    send_to_char("Currently defined aliases:\r\n", ch);
    if ((a = GET_ALIASES(ch)) == NULL)
      send_to_char(" None.\r\n", ch);
    else {
      while (a != NULL) {
        snprintf(buf, MAX_STRING_LENGTH, "%-15s %s\r\n", a->alias, a->replacement);
        send_to_char(buf, ch);
        a = a->next;
      }
    }
  } else {
    if ((a = find_alias(GET_ALIASES(ch), arg)) != NULL) {
      REMOVE_FROM_LIST(a, GET_ALIASES(ch), next);
      free_alias(a);
    }
    if (!*repl) {
      if (a == NULL)
        send_to_char("No such alias.\r\n", ch);
      else
        send_to_char("Alias deleted.\r\n", ch);
    } else {
      if (!str_cmp(arg, "alias")) {
        send_to_char("You can't alias 'alias'.\r\n", ch);
        return;
      }
      CREATE(a, struct alias, 1);
      a->alias = strdup(arg);
      delete_doubledollar(repl);
      a->replacement = strdup(repl);
      if (strchr(repl, ALIAS_SEP_CHAR) || strchr(repl, ALIAS_VAR_CHAR))
        a->type = ALIAS_COMPLEX;
      else
        a->type = ALIAS_SIMPLE;
      a->next = GET_ALIASES(ch);
      GET_ALIASES(ch) = a;
      send_to_char("Alias added.\r\n", ch);
    }
  }
}

/*
 * Valid numeric replacements are only &1 .. &9 (makes parsing a little
 * easier, and it's not that much of a limitation anyway.)  Also valid
 * is "&*", which stands for the entire original line after the alias.
 * ";" is used to delimit commands.
 */
#define NUM_TOKENS       9

void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias *a)
{
  struct txt_q temp_queue;
  char *tokens[NUM_TOKENS], *temp, *write_point;
  int num_of_tokens = 0, num;

  /* First, parse the original string */
  temp = strtok(strcpy(buf2, orig), " ");
  while (temp != NULL && num_of_tokens < NUM_TOKENS) {
    tokens[num_of_tokens++] = temp;
    temp = strtok(NULL, " ");
  }

  /* initialize */
  write_point = buf;
  temp_queue.head = temp_queue.tail = NULL;

  /* now parse the alias */
  for (temp = a->replacement; *temp; temp++) {
    if (*temp == ALIAS_SEP_CHAR) {
      *write_point = '\0';
      write_to_q(buf, &temp_queue, 1);
      write_point = buf;
    } else if (*temp == ALIAS_VAR_CHAR) {
      temp++;
      if ((num = *temp - '1') < num_of_tokens && num >= 0) {
        strcpy(write_point, tokens[num]);
        write_point += strlen(tokens[num]);
      } else if (*temp == ALIAS_GLOB_CHAR) {
        strcpy(write_point, orig);
        write_point += strlen(orig);
      } else if ((*(write_point++) = *temp) == '$') /* redouble $ for act safety */
        *(write_point++) = '$';
    } else
      *(write_point++) = *temp;
  }

  *write_point = '\0';
  write_to_q(buf, &temp_queue, 1);

  /* push our temp_queue on to the _front_ of the input queue */
  if (input_q->head == NULL)
    *input_q = temp_queue;
  else {
    temp_queue.tail->next = input_q->head;
    input_q->head = temp_queue.head;
  }
}

/*
 * Given a character and a string, perform alias replacement on it.
 *
 * Return values:
 *   0: String was modified in place; call command_interpreter immediately.
 *   1: String was _not_ modified in place; rather, the expanded aliases
 *      have been placed at the front of the character's input queue.
 */
int perform_alias(struct descriptor_data * d, char *orig)
{
  char first_arg[MAX_INPUT_LENGTH], *ptr;
  struct alias *a, *tmp;

  /* bail out immediately if the guy doesn't have any aliases */
  if ((tmp = GET_ALIASES(d->character)) == NULL)
    return 0;

  /* find the alias we're supposed to match */
  ptr = any_one_arg(orig, first_arg);

  /* bail out if it's null */
  if (!*first_arg)
    return 0;

  /* if the first arg is not an alias, return without doing anything */
  if ((a = find_alias(tmp, first_arg)) == NULL)
    return 0;

  if (a->type == ALIAS_SIMPLE) {
    strcpy(orig, a->replacement);
    return 0;
  } else {
    perform_complex_alias(&d->input, ptr, a);
    return 1;
  }
}

/***************************************************************************
 * Various other parsing utilities                                         *
 **************************************************************************/

/*
 * searches an array of strings for a target string.  "exact" can be
 * 0 or non-0, depending on whether or not the match must be exact for
 * it to be returned.  Returns -1 if not found; 0..n otherwise.  Array
 * must be terminated with a '\n' so it knows to stop searching.
 */
int search_block(char *arg, char **list, bool exact)
{
  register int i, l;

  /* Make into lower case, and get length of string */
  for (l = 0; *(arg + l); l++)
    *(arg + l) = LOWER(*(arg + l));

  if (exact) {
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strcmp(arg, *(list + i)))
        return (i);
  } else {
    if (!l)
      l = 1; /* Avoid "" to match the first available
       * string */
    for (i = 0; **(list + i) != '\n'; i++)
      if (!strncmp(arg, *(list + i), l))
        return (i);
  }

  return -1;
}

int is_number(char *str)
{
  while (*str)
    if (!isdigit(*(str++)))
      return 0;

  return 1;
}

void skip_spaces(char **string)
{
  for (; *string && **string && isspace(**string); (*string)++)
    ;
}

char *delete_doubledollar(char *string)
{
  char *read, *write;

  if ((write = strchr(string, '$')) == NULL)
    return string;

  read = write;

  while (*read)
    if ((*(write++) = *(read++)) == '$')
      if (*read == '$')
        read++;

  *write = '\0';

  return string;
}

int fill_word(char *argument)
{
  return (search_block(argument, fill, TRUE) >= 0);
}

int reserved_word(char *argument)
{
  return (search_block(argument, reserved, TRUE) >= 0);
}

/*
 * copy the first non-fill-word, space-delimited argument of 'argument'
 * to 'first_arg'; return a pointer to the remainder of the string.
 */
char *one_argument(char *argument, char *first_arg)
{
  char *begin = first_arg;

  do {
    skip_spaces(&argument);

    first_arg = begin;
    while (argument && *argument && !isspace(*argument)) {
      *(first_arg++) = LOWER(*argument);
      argument++;
    }

    *first_arg = '\0';
  } while (fill_word(begin));

  return argument;
}

/* same as one_argument except that it doesn't ignore fill words */
char *any_one_arg(char *argument, char *first_arg)
{
  skip_spaces(&argument);

  while (argument && *argument && !isspace(*argument)) {
    *(first_arg++) = LOWER(*argument);
    argument++;
  }

  *first_arg = '\0';

  return argument;
}

/*
 * Same as one_argument except that it takes two args and returns the rest;
 * ignores fill words
 */
char *two_arguments(char *argument, char *first_arg, char *second_arg)
{
  return one_argument(one_argument(argument, first_arg), second_arg); /* :-) */
}

/*
 * Same as one_argument except that it takes three args and returns the rest;
 * ignores fill words
 */
char *three_arguments(char *argument, char *first_arg, char *second_arg, char *third_arg)
{
  return one_argument(one_argument(one_argument(argument, first_arg), second_arg), third_arg);
}

/*
 * determine if a given string is an abbreviation of another
 * (now works symmetrically -- JE 7/25/94)
 */

int is_abbrev(char *arg1, char *arg2)
{
  if (!*arg1)
    return 0;

  for (; *arg1 && *arg2; arg1++, arg2++)
    if (LOWER(*arg1) != LOWER(*arg2))
      return 0;

  if (!*arg1)
    return 1;
  else
    return 0;
}

/* return first space-delimited token in arg1; remainder of string in arg2 */
void half_chop(char *string, char *arg1, char *arg2)
{
  char *temp;

  temp = any_one_arg(string, arg1);
  skip_spaces(&temp);
  strcpy(arg2, temp);
}

/* Used in specprocs, mostly.  (Exactly) matches "command" to cmd number */
int find_command(char *command)
{
  int cmd;

  for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
    if (!strcmp(cmd_info[cmd].command, command))
      return cmd;

  return -1;
}

int special(struct char_data * ch, int cmd, char *arg)
{
  register struct obj_data *i;
  register struct char_data *k;
  int j;

  /* special in room? */
  if (GET_ROOM_SPEC(ch->in_room) != NULL && (IS_SET(SPEC_ROOM_TYPE(ch->in_room), SPEC_COMMAND) || IS_SET(SPEC_ROOM_TYPE(ch->in_room), SPEC_STANDARD)))
    if (GET_ROOM_SPEC(ch->in_room)(ch, world + ch->in_room, cmd, arg, SPEC_COMMAND))
      return 1;

  /* special in equipment list? */
  for (j = 0; j < NUM_WEARS; j++)
    if (ch->equipment[j] && GET_OBJ_SPEC(ch->equipment[j]) != NULL && (IS_SET(SPEC_OBJ_TYPE(ch->equipment[j]), SPEC_COMMAND) || IS_SET(SPEC_OBJ_TYPE(ch->equipment[j]), SPEC_STANDARD)))
      if (GET_OBJ_SPEC(ch->equipment[j])(ch, ch->equipment[j], cmd, arg, SPEC_COMMAND))
        return 1;

  /* special in inventory? */
  for (i = ch->carrying; i; i = i->next_content)
    if (GET_OBJ_SPEC(i) != NULL && (IS_SET(SPEC_OBJ_TYPE(i), SPEC_COMMAND) || IS_SET(SPEC_OBJ_TYPE(i), SPEC_STANDARD)))
      if (GET_OBJ_SPEC(i)(ch, i, cmd, arg, SPEC_COMMAND))
        return 1;

  /* special in mobile present? */
  for (k = world[ch->in_room].people; k; k = k->next_in_room)
    if (IS_NPC(k) && GET_MOB_SPEC(k) != NULL && (IS_SET(SPEC_MOB_TYPE(k), SPEC_COMMAND) || IS_SET(SPEC_MOB_TYPE(k), SPEC_STANDARD)))
      if (GET_MOB_SPEC(k)(ch, k, cmd, arg, SPEC_COMMAND))
        return 1;

  /* special in object present? */
  for (i = world[ch->in_room].contents; i; i = i->next_content)
    if (GET_OBJ_SPEC(i) != NULL && (IS_SET(SPEC_OBJ_TYPE(i), SPEC_COMMAND) || IS_SET(SPEC_OBJ_TYPE(i), SPEC_STANDARD)))
      if (GET_OBJ_SPEC(i)(ch, i, cmd, arg, SPEC_COMMAND))
        return 1;

  return 0;
}

/* *************************************************************************
 *  Stuff for controlling the non-playing sockets (get name, pwd etc)       *
 ************************************************************************* */

int _parse_name(char *arg, char *name)
{
  int i;

  /* skip whitespaces */
  for (; isspace(*arg); arg++)
    ;

  if (strlen(arg) < 3)
    return 1;

  for (i = 1; arg[i] != '\0'; i++)
    arg[i] = tolower(arg[i]);

  for (; (*name = *arg); arg++, name++)
    if (!isalpha(*arg))
      return 1;

  if (!i)
    return 1;

  return 0;
}

/* deal with newcomers and other non-playing sockets */
void nanny(struct descriptor_data * d, char *arg)
{
  /* extern functions */
  int load_char_text(char *name, struct char_data * char_element);
  int parse_class_spec(char *arg);
  void disp_class_menu(struct descriptor_data *d);
  struct descriptor_data *temp2 = NULL;
  bool check_class(struct char_data *ch, int class);
  void set_magic_memory(struct char_data *ch);

  /* extern variables */
  extern char *MENU;
  extern char *GREET1;
  extern char *GREET2;
  extern char *policies;
  extern char *namepol;
  extern char *signoff;
  extern int hometowns[][NUM_CLASSES];
  extern struct descriptor_data *descriptor_list;
  extern sh_int r_immort_start_room;
  extern sh_int r_frozen_start_room;
  extern char *race_menu;

  /* local variables */
  char buf[MAX_STRING_LENGTH];
  int load_result = 0, playing = 0, tmp_num = 0;
  char tmp_name[MAX_INPUT_LENGTH];
  char tmp_policy[MAX_STRING_LENGTH];
  char tmp_namepolicy[MAX_STRING_LENGTH];
  struct lastinfo *tmpLast = NULL;
  struct lastinfo *newLast = NULL;
  struct char_data *tmp_ch = NULL;
  struct descriptor_data *k = NULL, *next = NULL, *next_d = NULL, *temp = NULL;
  int autokits[NUM_CLASSES][NUMITEMINKIT] = { {3022, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032, 3030, 3030}, /* Warrior */
  {3020, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032, 3030, 3030}, /* Rogue */
  {3020, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032, 3030, 3030}, /* Thief */
  {3018, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032, 3030, 3030}, /* Sorcerer*/
  {3018, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032, 3030, 3030}, /* Wizard */
  {3018, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032, 3030, 3030}, /* Enchanter */
  {3018, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032, 3030, 3030}, /* Conjurer */
  {3018, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032, 3030, 3030}, /* Necromancer */
  {3058, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032, 3030, 3030}, /* Cleric */
  {3058, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032, 3030, 3030}, /* Priest */
  {3058, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032, 3030, 3030}, /* Shaman */
  {3018, 3042, 3081, 3076, 3014, 3057, 3102, 3010, 3010, 3010, 3032, 3030, 3030}, /* Monk */
  {3058, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032, 3030, 3030}, /* Druid */
  {3020, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032, 3030, 3030}, /* Assassian */
  {3058, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032, 3030, 3030}, /* Bard */
  {3022, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032, 3030, 3030}, /* Ranger */
  {3020, 3042, 3081, 3076, 3041, 3057, 3102, 3010, 3010, 3010, 3032, 3030, 3030}, /* Mercenary */
  };
  sh_int load_room = 0;
  int i = 0;
  int found = 0;
  struct obj_data *obj = NULL;

  skip_spaces(&arg);
  memset(tmp_name, 0, sizeof(tmp_name));
  memset(tmp_policy, 0, sizeof(tmp_policy));
  memset(tmp_namepolicy, 0, sizeof(tmp_namepolicy));
  memset(buf, 0, sizeof(buf));

  switch (STATE(d)) {
    case CON_NEWNAME: {
      struct char_data *tmp_store = NULL;

      CREATE(tmp_store, struct char_data, 1);

      if (!*arg) {
        STATE(d) = CON_CLOSE;
        return;
      } else if ((_parse_name(arg, tmp_name)) || strlen(tmp_name) < 2 || strlen(tmp_name) > MAX_NAME_LENGTH || fill_word(strcpy(buf, tmp_name)) || reserved_word(buf) || !Valid_Name(tmp_name) || load_char_text(tmp_name, tmp_store)) {
        SEND_TO_Q("Invalid name, please try another.\r\nName: ", d);
        *tmp_name = '\0';
        *arg = '\0';
        *buf = '\0';
        free_char(tmp_store);
        tmp_store = NULL;
        return;
      }
      free_char(tmp_store);
      FREE(d->character->player.name);
      CREATE(d->character->player.name, char, strlen(arg) + 1);
      strcpy(d->character->player.name, CAP(arg));
      REMOVE_BIT(PLR_FLAGS(d->character), PLR_DENIED);
      SEND_TO_Q("\r\n"
      "Character approval - This process can take up to 2 minutes, auto approval\r\n"
      "                     will take place if there are no immortals online to \r\n"
      "                     approve or deny your name.  If your name is declined\r\n"
      "                     please try another.\r\n"
      "\r\n", d);
      STATE(d) = CON_ACCEPT;
      break;
    }
    case CON_ACCEPT:
      d->idle_cnt = 0; /* this case is handled in comm.c */
      break;
    case CON_HOMETOWN:
      d->idle_cnt = 0;
      switch (*arg) {
        case 'W':
        case 'w':
          GET_HOME(d->character) = 1; /* Weirvane */
          break;
      }
      sprintf(tmp_policy, "%s{x\r\n\r\nDo you accept these terms [y/n]?", policies);
      page_string(d, tmp_policy, 1);
      STATE(d) = CON_POLICY;
      break;
    case CON_POLICY:
      d->idle_cnt = 0;
      switch (*arg) {
        case 'Y':
        case 'y':
          SEND_TO_Q("\r\n"
          "Character approval - This process can take up to 2 minutes, auto approval\r\n"
          "                     will take place if there are no immortals online to \r\n"
          "                     approve or deny your name.  If your name is declined\r\n"
          "                     please try another.\r\n"
          "\r\n", d);
          STATE(d) = CON_ACCEPT;
          break;
        case 'N':
        case 'n':
          STATE(d) = CON_CLOSE;
          break;
        case '\0':
        default:
          SEND_TO_Q("You must choose either yes or no.\r\n", d);
          break;
      }
      break;
    case CON_ALIGNMENT:
      d->idle_cnt = 0;
      switch (*arg) {
        case 'G':
        case 'g':
          if (GET_CLASS(d->character) == CLASS_ASSASSIN || GET_CLASS(d->character) == CLASS_THIEF || GET_CLASS(d->character) == CLASS_ROGUE) {
            SEND_TO_Q("\r\nInvalid Choice.\r\n", d);
            SEND_TO_Q("Please choose an alignment for your new character.\r\n", d);
            SEND_TO_Q("[N]eutral\r\n", d);
            SEND_TO_Q("[E]vil\r\n", d);
            SEND_TO_Q("\r\nYour choice: ", d);
            STATE(d) = CON_ALIGNMENT;
          } else
            GET_ALIGNMENT(d->character) = 1000;
          break;
        case 'E':
        case 'e':
          GET_ALIGNMENT(d->character) = -1000;
          break;
        default:
          GET_ALIGNMENT(d->character) = 0;
          break;
      }

      SEND_TO_Q("\r\nPlease choose a hometown for your new character.\r\n", d);
      SEND_TO_Q("[W]eirvane\r\n", d);
      SEND_TO_Q("\r\nYour choice: ", d);
      STATE(d) = CON_HOMETOWN;
      break;
    case CON_MEDIT:
      medit_parse(d, arg);
      break;
    case CON_REDIT:
      redit_parse(d, arg);
      break;
    case CON_OEDIT:
      /* return control to oedit_parse */
      oedit_parse(d, arg);
      break;
    case CON_ZEDIT:
      zedit_parse(d, arg);
      break;
    case CON_SEDIT:
      sedit_parse(d, arg);
      break;
    case CON_GET_TERMTYPE:
      d->idle_cnt = 0;
      if (!*arg || *arg == 'y' || *arg == 'Y') {
        d->color = 1;
        SEND_TO_Q_COLOR(GREET1, d);
        SEND_TO_Q("\x1B[2K\r\nBy what name do you wish to be known? ", d);
      } else {
        d->color = 0;
        SEND_TO_Q_COLOR(GREET2, d);
        SEND_TO_Q("\r\nBy what name do you wish to be known? ", d);
      }
      STATE(d) = CON_GET_NAME;
      break;
    case CON_GET_NAME: /* wait for input of name */
      d->idle_cnt = 0;
      if (d->character == NULL) {
        CREATE(d->character, struct char_data, 1);
        clear_char(d->character);
        d->character->desc = d;
      }
      if (!*arg)
        close_socket(d);
      else {
        if ((_parse_name(arg, tmp_name)) || strlen(tmp_name) < 2 || strlen(tmp_name) > MAX_NAME_LENGTH || fill_word(strcpy(buf, tmp_name)) || reserved_word(buf)) {
          SEND_TO_Q("Invalid name, please try another.\r\n"
          "Name: ", d);
          *tmp_name = '\0';
          *arg = '\0';
          *buf = '\0';
          return;
        }

        tmp_num = 0;
        while (tmp_name[tmp_num] != '\0') {
          if (tmp_num == 0)
            tmp_name[tmp_num] = UPPER(tmp_name[tmp_num]);
          else
            tmp_name[tmp_num] = LOWER(tmp_name[tmp_num]);
          tmp_num++;
        }

        if (load_char_text(tmp_name, d->character)) {
          d->character->desc = d;
          load_text(d->character);
          /* undo it just in case they are set */
          REMOVE_BIT(PLR_FLAGS(d->character), PLR_WRITING | PLR_MAILING | PLR_CRYO | PLR_EDITING);
          REMOVE_BIT(AFF2_FLAGS(d->character), AFF2_SCRIBING | AFF2_CASTING | AFF2_MEMMING);
          SEND_TO_Q("Password: ", d);
          echo_off(d);
          STATE(d) = CON_PASSWORD;
        } else {
          /* player unknown -- make new character */
          CREATE(d->character->player_specials, struct player_special_data, 1);
          if (!Valid_Name(tmp_name)) {
            SEND_TO_Q("Invalid name, please try another.\r\n"
            "Name: ", d);
            return;
          }

          /* FIX FOR EQ DUPE BUG */
          for (temp2 = descriptor_list; temp2; temp2 = temp2->next) {
            if (temp2 && (temp2 != d) && temp2->character && !strcmp(CAP(tmp_name), GET_NAME(temp2->character))) {
              SEND_TO_Q("That name is being used, try another.\r\n"
              "Name: ", d);
              return;
            }
          }

          CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
          strcpy(d->character->player.name, CAP(tmp_name));
          GET_TITLE(d->character) = strdup(" ");
          GET_IDNUM(d->character) = get_idnum();
          sprintf(tmp_namepolicy, "%s{x\r\n\r\nDid I get that right, %s [y/n]?", namepol, tmp_name);
          if (d->color)
            page_string(d, tmp_namepolicy, 1);
          /*SEND_TO_Q_COLOR(tmp_namepolicy, d); */
          else
            page_string(d, tmp_namepolicy, 1);
          /* SEND_TO_Q(tmp_namepolicy, d); */
          SET_BIT(PRF_FLAGS(d->character), PRF_DELETED);
          STATE(d) = CON_NAME_CNFRM;
        }
      }
      break;
    case CON_NAME_CNFRM: /* wait for conf. of new name	 */
      d->idle_cnt = 0;
      strcpy(GET_HOST(d->character), d->host);
      if (*arg == 'y' || *arg == 'Y') {
        if (isbanned(GET_HOST(d->character)) >= BAN_NEW && isbanned(GET_HOST(d->character)) < BAN_OUTLAW) {
          snprintf(buf, MAX_STRING_LENGTH, "Request for new char %s denied from [%s] (siteban)", GET_NAME(d->character), GET_HOST(d->character));
          mudlog(buf, 'C', COM_IMMORT, TRUE);
          SEND_TO_Q("Sorry, new characters are not allowed from your site!\r\n", d);
          STATE(d) = CON_CLOSE;
          return;
        }
        if (restrict_game_lvl) {
          SEND_TO_Q("Sorry, new players can't be created at the moment.\r\n", d);
          snprintf(buf, MAX_STRING_LENGTH, "Request for new char %s denied from %s (wizlock)", GET_NAME(d->character), GET_HOST(d->character));
          mudlog(buf, 'C', COM_IMMORT, TRUE);
          STATE(d) = CON_CLOSE;
          return;
        }
        SEND_TO_Q("New character.\r\n", d);
        snprintf(buf, MAX_STRING_LENGTH, "Give me a password for %s: ", GET_NAME(d->character));
        SEND_TO_Q(buf, d);
        echo_off(d);
        STATE(d) = CON_NEWPASSWD;
      } else if (*arg == 'n' || *arg == 'N') {
        SEND_TO_Q("Okay, what IS it, then? ", d);
        FREE(d->character->player.name);
        d->character->player.name = NULL;
        STATE(d) = CON_GET_NAME;
      } else {
        SEND_TO_Q("Please type Yes or No: ", d);
      }
      break;
    case CON_PASSWORD: /* get pwd for known player	 */
      d->idle_cnt = 0;
      /* turn echo back on */
      echo_on(d);

      if (*GET_ENCPASSWD(d->character) == '\0' && *GET_PASSWD(d->character) == '\0') {
        SEND_TO_Q("This char's password is INVALID! Mail swadmin@shadowwind.org\r\nwith your chars names, wanted password, and your chars levels.\r\n", d);
        STATE(d) = CON_CLOSE;
        break;
      }
      if (!*arg)
        close_socket(d);
      else {
        /* we have plaintext password, check it and then convert to encoded version */
        if (*GET_ENCPASSWD(d->character) == '\0' && *GET_PASSWD(d->character) != '\0') {
          if (strcmp(GET_PASSWD(d->character), arg) == 0) {
            if (crypto_pwhash_str(GET_ENCPASSWD(d->character), GET_PASSWD(d->character), strlen(GET_PASSWD(d->character)), crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE) == 0) {
              *GET_PASSWD(d->character) = '\0';
              save_char_text(d->character, NOWHERE);
              save_text(d->character);
              if (d->character->player.host[0] != '\0') {
                snprintf(buf, MAX_STRING_LENGTH, "Auto updated PW: %s [%s]", GET_NAME(d->character), GET_HOST(d->character));
              } else {
                snprintf(buf, MAX_STRING_LENGTH, "Auto updated PW: %s [%s]", GET_NAME(d->character), d->hostIP);
              }
              mudlog(buf, 'P', COM_IMMORT, TRUE);
            }
          } else {
            if (d->character->player.host[0] != '\0') {
              snprintf(buf, MAX_STRING_LENGTH, "Bad PW: %s [%s]", GET_NAME(d->character), GET_HOST(d->character));
            } else {
              snprintf(buf, MAX_STRING_LENGTH, "Bad PW: %s [%s]", GET_NAME(d->character), d->hostIP);
            }
            mudlog(buf, 'P', COM_IMMORT, TRUE);
            GET_BAD_PWS(d->character)++;
            save_char_text(d->character, NOWHERE);
            save_text(d->character);
            if (++(d->bad_pws) >= 3) { /* 3 strikes and you're out. */
              SEND_TO_Q("Wrong password... disconnecting.\r\n", d);
              STATE(d) = CON_CLOSE;
            } else {
              SEND_TO_Q("Wrong password.\r\nPassword: ", d);
              echo_off(d);
            }
            return;
          }
        }
        mudlog(buf, 'P', COM_IMMORT, TRUE);
        if (crypto_pwhash_str_verify(GET_ENCPASSWD(d->character), arg, strlen(arg)) != 0) {
          if (d->character->player.host[0] != '\0') {
            snprintf(buf, MAX_STRING_LENGTH, "Bad PW: %s [%s]", GET_NAME(d->character), GET_HOST(d->character));
          } else {
            snprintf(buf, MAX_STRING_LENGTH, "Bad PW: %s [%s]", GET_NAME(d->character), d->hostIP);
          }
          mudlog(buf, 'P', COM_IMMORT, TRUE);
          GET_BAD_PWS(d->character)++;
          save_char_text(d->character, NOWHERE);
          save_text(d->character);
          if (++(d->bad_pws) >= 3) { /* 3 strikes and you're out. */
            SEND_TO_Q("Wrong password... disconnecting.\r\n", d);
            STATE(d) = CON_CLOSE;
          } else {
            SEND_TO_Q("Wrong password.\r\nPassword: ", d);
            echo_off(d);
          }
          return;
        }
        load_result = GET_BAD_PWS(d->character);
        GET_BAD_PWS(d->character) = 0;
        strcpy(GET_HOST(d->character), d->host);
        save_char_text(d->character, NOWHERE);
        save_text(d->character);

        if (isbanned(GET_HOST(d->character)) == BAN_SELECT && !PLR_FLAGGED(d->character, PLR_SITEOK)) {
          SEND_TO_Q("Sorry, this char has not been cleared for login from your site!\r\nMail swadmin@shadowwind.org for clearance.\r\n\r\n", d);
          STATE(d) = CON_CLOSE;
          snprintf(buf, MAX_STRING_LENGTH, "Connection attempt for %s denied from %s", GET_NAME(d->character), GET_HOST(d->character));
          mudlog(buf, 'C', COM_IMMORT, TRUE);
          return;
        }
        if (GET_LEVEL(d->character) < restrict_game_lvl) {
          SEND_TO_Q("The game is temporarily restricted.. try again later.\r\n", d);
          STATE(d) = CON_CLOSE;
          snprintf(buf, MAX_STRING_LENGTH, "Request for login denied for %s [%s] (wizlock)", GET_NAME(d->character), GET_HOST(d->character));
          mudlog(buf, 'C', COM_IMMORT, TRUE);
          return;
        }

        for (temp = descriptor_list; temp; temp = next_d) {
          next_d = temp->next;
          if (temp->character && GET_IDNUM(temp->character) == GET_IDNUM(d->character) && (temp->character != d->character)) {
            flush_queues(temp);
            close_socket(temp);
          }
        }

        /*
         * first, check to see if this person is already logged in, but
         * switched.  If so, disconnect the switched persona.
         */
        for (k = descriptor_list; k; k = k->next)
          if (k->original && (GET_IDNUM(k->original) == GET_IDNUM(d->character))) {
            SEND_TO_Q("Disconnecting for return to unswitched char.\r\n", k);
            flush_queues(k);
            STATE(k) = CON_CLOSE;
            free_char(d->character);
            d->character = k->original;
            d->character->desc = d;
            d->original = NULL;
            d->character->char_specials.timer = 0;
            if (k->character)
              k->character->desc = NULL;
            k->character = NULL;
            k->original = NULL;
            SEND_TO_Q("Reconnecting to unswitched char.", d);
            REMOVE_BIT(PLR_FLAGS(d->character), PLR_MAILING | PLR_WRITING | PLR_EDITING);
            REMOVE_BIT(AFF2_FLAGS(d->character), AFF2_SCRIBING | AFF2_CASTING | AFF2_MEMMING);
            STATE(d) = CON_PLAYING;
            snprintf(buf, MAX_STRING_LENGTH, "%s [%s] has reconnected.", GET_NAME(d->character), GET_HOST(d->character));
            mudlog(buf, 'C', COM_IMMORT, TRUE);
            return;
          }
        /* now check for linkless and usurpable */

        for (tmp_ch = character_list; tmp_ch; tmp_ch = tmp_ch->next)
          if (!IS_NPC(tmp_ch) && GET_IDNUM(d->character) == GET_IDNUM(tmp_ch)) {
            if (!tmp_ch->desc) {
              SEND_TO_Q("Reconnecting.\r\n", d);
              act("$n has reconnected.", TRUE, tmp_ch, 0, 0, TO_ROOM);

              snprintf(buf, MAX_STRING_LENGTH, "%s [%s] has reconnected.", GET_NAME(d->character), GET_HOST(d->character));
              mudlog(buf, 'C', COM_IMMORT, TRUE);
            } else {
              snprintf(buf, MAX_STRING_LENGTH, "%s has re-logged in ... disconnecting old socket.", GET_NAME(tmp_ch));
              mudlog(buf, 'C', COM_IMMORT, TRUE);
              SEND_TO_Q("This body has been usurped!\r\n", tmp_ch->desc);
              flush_queues(tmp_ch->desc);
              STATE(tmp_ch->desc) = CON_CLOSE;
              tmp_ch->desc->character = NULL;
              tmp_ch->desc = NULL;
              SEND_TO_Q("You take over your own body, already in use!\r\n", d);
              act("$n suddenly keels over in pain, surrounded by a white aura...\r\n"
                  "$n's body has been taken over by a new spirit!", TRUE, tmp_ch, 0, 0, TO_ROOM);
            }

            free_char(d->character);
            tmp_ch->desc = d;
            d->character = tmp_ch;
            load_text(d->character);
            tmp_ch->char_specials.timer = 0;
            REMOVE_BIT(PLR_FLAGS(d->character), PLR_MAILING | PLR_WRITING | PLR_EDITING);
            REMOVE_BIT(AFF_FLAGS(d->character), AFF_CAMPING | AFF_MEDITATING | AFF_TEACHING);
            REMOVE_BIT(AFF2_FLAGS(d->character), AFF2_SCRIBING | AFF2_CASTING | AFF2_MEMMING);
            STATE(d) = CON_PLAYING;
            return;
          }
        if (GET_LEVEL(d->character) >= LVL_IMMORT) {
          SEND_TO_Q_COLOR(imotd, d);
        } else if (GET_LEVEL(d->character) == 0)
          SEND_TO_Q_COLOR(nmotd, d);
        else
          SEND_TO_Q_COLOR(motd, d);

        if (d->character->player.host[0] != '\0') {
          snprintf(buf, MAX_STRING_LENGTH, "%s [%s] has connected.", GET_NAME(d->character), GET_HOST(d->character));
        } else {
          snprintf(buf, MAX_STRING_LENGTH, "%s [%s] has connected.", GET_NAME(d->character), d->hostIP);
        }
        mudlog(buf, 'C', COM_IMMORT, TRUE);

        if (load_result) {
          snprintf(buf, MAX_STRING_LENGTH, "\r\n\r\n\007\007\007"
              "%s%d LOGIN FAILURE%s SINCE LAST SUCCESSFUL LOGIN.%s\r\n", CCRED(d->character, C_SPR), load_result, (load_result > 1) ? "S" : "", CCNRM(d->character, C_SPR));
          SEND_TO_Q(buf, d);
        }
        SEND_TO_Q("\r\n\n*** PRESS RETURN: ", d);
        STATE(d) = CON_RMOTD;
      }
      break;

    case CON_NEWPASSWD:
    case CON_CHPWD_GETNEW:
      d->idle_cnt = 0;
      if (!*arg || strlen(arg) < 3 || !str_cmp(arg, GET_NAME(d->character))) {
        SEND_TO_Q("\r\nIllegal password.\r\n", d);
        SEND_TO_Q("Password: ", d);
        return;
      }
      if (crypto_pwhash_str(GET_ENCPASSWD(d->character), arg, strlen(arg), crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE) == 0) {
        SEND_TO_Q("\r\nPlease retype password: ", d);
        if (STATE(d) == CON_NEWPASSWD) {
          STATE(d) = CON_CNFPASSWD;
        } else {
          STATE(d) = CON_CHPWD_VRFY;
        }
      }

      break;

    case CON_CNFPASSWD:
    case CON_CHPWD_VRFY:
      d->idle_cnt = 0;
      if (crypto_pwhash_str_verify(GET_ENCPASSWD(d->character), arg, strlen(arg))) {
        SEND_TO_Q("\r\nPasswords don't match... start over.\r\n", d);
        SEND_TO_Q("Password: ", d);
        if (STATE(d) == CON_CNFPASSWD)
          STATE(d) = CON_NEWPASSWD;
        else
          STATE(d) = CON_CHPWD_GETNEW;
        return;
      }
      echo_on(d);

      if (STATE(d) == CON_CNFPASSWD) {
        SEND_TO_Q("What is your sex ([M]ale/[F]emale)? ", d);
        STATE(d) = CON_QSEX;
      } else {
        save_char_text(d->character, NOWHERE);
        save_text(d->character);
        echo_on(d);
        SEND_TO_Q("\r\nDone.\n\r", d);
        SEND_TO_Q_COLOR(MENU, d);
        SEND_TO_Q_COLOR("{WMake your choice{G:{x ", d);
        STATE(d) = CON_MENU;
      }

      break;

    case CON_QSEX: /* query sex of new user	 */
      d->idle_cnt = 0;
      switch (*arg) {
        case 'm':
        case 'M':
          d->character->player.sex = SEX_MALE;
          break;
        case 'f':
        case 'F':
          d->character->player.sex = SEX_FEMALE;
          break;
        default:
          SEND_TO_Q("That is not a sex..\r\n"
          "What IS your sex? ", d);
          return;
      }

      SEND_TO_Q_COLOR(race_menu, d);
      SEND_TO_Q("\r\nRace: ", d);
      STATE(d) = CON_QRACE;
      break;

    case CON_QRACE: /* query race of character */
      d->idle_cnt = 0;
      switch (*arg) {
        case '1':
          d->character->player_specials->saved.race = RACE_HUMAN;
          break;
        case '2':
          d->character->player_specials->saved.race = RACE_TROLL;
          break;
        case '3':
          d->character->player_specials->saved.race = RACE_OGRE;
          break;
        case '4':
          d->character->player_specials->saved.race = RACE_DWARF;
          break;
        case '5':
          d->character->player_specials->saved.race = RACE_ELF;
          break;
        case '6':
          d->character->player_specials->saved.race = RACE_HALFELF;
          break;
        case '7':
          d->character->player_specials->saved.race = RACE_GNOME;
          break;
        case '8':
          d->character->player_specials->saved.race = RACE_HALFLING;
          break;
        default:
          SEND_TO_Q("That is not a race!\r\n"
          "Which RACE do you want? ", d);
          return;
      }
      d->character->desc = d;
      roll_real_abils(d->character);
      display_stats(d->character);
      disp_class_menu(d);
      STATE(d) = CON_QCLASS;
      SEND_TO_Q("\r\nChoose a class, or hit enter to reroll: ", d);
      break;
    case CON_QCLASS:
      d->idle_cnt = 0;
      if (*arg == '\0') {
        roll_real_abils(d->character);
        display_stats(d->character);
        disp_class_menu(d);
        SEND_TO_Q("\r\nChoose a class, or hit enter to reroll: ", d);
        STATE(d) = CON_QCLASS;
      } else {
        if ((GET_CLASS(d->character) = parse_class_spec(arg)) == CLASS_UNDEFINED) {
          SEND_TO_Q("\r\nThat's not a class.\r\nChoose a class, or hit enter to reroll: ", d);
          return;
        }
        if (!check_class(d->character, parse_class_spec(arg))) {
          SEND_TO_Q("\r\nThat class is not available to you.\r\nChoose a class, or hit enter to reroll: ", d);
          return;
        }
        init_char(d->character);
        /* default settings for new users */
        SET_BIT(PRF_FLAGS(d->character), PRF_WHOIS);
        SET_BIT(PRF_FLAGS(d->character), PRF_AUTOEXIT);
        GET_PROMPT(d->character) = 15; /* set prompt display all */
        if (isbanned(GET_HOST(d->character)) == BAN_OUTLAW) {
          result = PLR_TOG_CHK(d->character, PLR_OUTLAW);
          snprintf(buf, MAX_STRING_LENGTH, "Outlaw ON for new char %s from [%s] (siteban)", GET_NAME(d->character), GET_HOST(d->character));
          mudlog(buf, 'M', COM_IMMORT, TRUE);
        }
        if (isbanned(GET_HOST(d->character)) == BAN_FROZEN) {
          result = PLR_TOG_CHK(d->character, PLR_FROZEN);
          snprintf(buf, MAX_STRING_LENGTH, "Frozen ON for new char %s from [%s] (siteban)", GET_NAME(d->character), GET_HOST(d->character));
          mudlog(buf, 'M', COM_IMMORT, TRUE);
        }
        if (GET_RACE(d->character) != RACE_TROLL && GET_CLASS(d->character) != CLASS_NECROMANCER && GET_CLASS(d->character) != CLASS_DRUID && GET_CLASS(d->character) != CLASS_RANGER && GET_RACE(d->character) != RACE_OGRE) {
          SEND_TO_Q("\r\nPlease choose an alignment for your new character.\r\n", d);
          if (GET_CLASS(d->character) != CLASS_ASSASSIN && GET_CLASS(d->character) != CLASS_ROGUE && GET_CLASS(d->character) != CLASS_THIEF)
            SEND_TO_Q("[G]ood\r\n", d);
          SEND_TO_Q("[N]eutral\r\n", d);
          SEND_TO_Q("[E]vil\r\n", d);
          SEND_TO_Q("\r\nYour choice: ", d);
          STATE(d) = CON_ALIGNMENT;
        } else {
          if (GET_RACE(d->character) == RACE_TROLL || GET_RACE(d->character) == RACE_OGRE || GET_CLASS(d->character) == CLASS_NECROMANCER) {
            GET_ALIGNMENT(d->character) = -1000;
          } else if (GET_CLASS(d->character) == CLASS_DRUID || GET_CLASS(d->character) == CLASS_RANGER) {
            GET_ALIGNMENT(d->character) = 0;
          }

          SEND_TO_Q("\r\nPlease choose a hometown for your new character.\r\n", d);
          SEND_TO_Q("[W]eirvane\r\n", d);
          SEND_TO_Q("\r\nYour choice: ", d);

          STATE(d) = CON_HOMETOWN;
        }
      }
      break;

    case CON_RMOTD: /* read CR after printing motd	 */
      d->idle_cnt = 0;

      if (d->color)
        SET_BIT(PRF_FLAGS(d->character), PRF_COLOR_1 | PRF_COLOR_2);
      SEND_TO_Q_COLOR(MENU, d);
      SEND_TO_Q_COLOR("{WMake your choice{G:{x ", d);
      save_char_text(d->character, NOWHERE);
      d->character->desc = d;
      save_text(d->character);
      STATE(d) = CON_MENU;
      break;

    case CON_RECONNECT_AS: {
      struct descriptor_data* dl = descriptor_list;
      int found = 0;
      int i = 0;

      d->idle_cnt = 0;
      if (!*arg) {
        SEND_TO_Q("\r\n\n*** PRESS RETURN: ", d);
        STATE(d) = CON_RMOTD;
        break;
      }

      while (arg[i]) {
        if (i == 0)
          arg[i] = UPPER(arg[i]);
        else
          arg[i] = LOWER(arg[i]);
        i++;
      }

      for (dl = descriptor_list; dl; dl = dl->next)
        if (dl->character && strcmp(arg, GET_NAME(dl->character)) == 0)
          if (STATE(dl) > CON_PLAYING && STATE(dl) <= CON_MEDIT)
            found = 1;

      if (!found) {
        if (d->character)
          free_char(d->character);
        d->character = NULL;
        SEND_TO_Q("Please confirm name to login as:", d);
        STATE(d) = CON_GET_NAME;
      } else {
        SEND_TO_Q("Sorry, you cannot login with that name.\r\n", d);
        SEND_TO_Q("\r\n\n*** PRESS RETURN: ", d);
        STATE(d) = CON_RMOTD;
      }
    }
      break;

    case CON_MENU: /* get selection from main menu	 */
      d->idle_cnt = 0;
      switch (*arg) {
        case '0':
          SEND_TO_Q_COLOR(signoff, d);
          process_output(d);
          close_socket(d);
          break;

        case '1':
          CHAR_WEARING(d->character) = 0;
          SKILL_TIMER(d->character) = 0;
          set_magic_memory(d->character);
          /* Update LastInfo list */
          if (!LastInfo) {
            CREATE(LastInfo, struct lastinfo, 1);
            LastInfo->prev = NULL;
            LastInfo->next = NULL;
            LastInfo->PlayerNum = GET_IDNUM(d->character);
            LastInfo->NumberConnects = 1;
            LastInfo->Time = GET_LOGON(d->character);
            strcpy(LastInfo->Name, GET_NAME(d->character));
          } else {
            tmpLast = LastInfo;
            found = 0;
            while (tmpLast) {
              if (tmpLast->PlayerNum == GET_IDNUM(d->character)) {
                tmpLast->NumberConnects++;
                tmpLast->Time = GET_LOGON(d->character);
                found = 1;
                break;
              } else {
                tmpLast = tmpLast->next;
              }
            }
            if (!found) {
              tmpLast = LastInfo;
              while (tmpLast->next)
                tmpLast = tmpLast->next;
              CREATE(newLast, struct lastinfo, 1);
              newLast->prev = tmpLast;
              tmpLast->next = newLast;
              newLast->next = NULL;
              newLast->PlayerNum = GET_IDNUM(d->character);
              newLast->NumberConnects = 1;
              newLast->Time = GET_LOGON(d->character);
              strcpy(newLast->Name, GET_NAME(d->character));
            }
          }
          /* this code is to prevent people from multiply logging in */
          for (k = descriptor_list; k; k = next) {
            next = k->next;
            if (!k->connected && k->character && !str_cmp(GET_NAME(k->character), GET_NAME(d->character))) {
              SEND_TO_Q("Your character has been deleted.\r\n", d);
              STATE(d) = CON_CLOSE;
              return;
            }
          }
          reset_char(d->character);
          if (PLR_FLAGGED(d->character, PLR_INVSTART))
            GET_PLR_INVIS_LEV(d->character) = GET_LEVEL(d->character);
          if ((load_result = Crash_load(d->character)))
            d->character->in_room = NOWHERE;
          save_char_text(d->character, NOWHERE);
          save_text(d->character);
          d->character->next = character_list;
          character_list = d->character;

          if (GET_LEVEL(d->character) >= LVL_IMMORT) {
            if (PLR_FLAGGED(d->character, PLR_LOADROOM)) {
              if ((load_room = real_room(GET_LOADROOM(d->character))) < 0)
                load_room = r_immort_start_room;
            } else
              load_room = r_immort_start_room;
          } else {
            load_room = real_room(GET_STARTROOM(d->character, GET_HOME(d->character)));

            if (PLR_FLAGGED(d->character, PLR_FROZEN))
              load_room = r_frozen_start_room;
            else if (PLR_FLAGGED(d->character, PLR_LOADROOM)) {
              if (real_room(GET_LOADROOM(d->character)) > 0)
                load_room = real_room(GET_LOADROOM(d->character));
            } else {
              if (real_room(d->character->in_room) > 0)
                load_room = real_room(d->character->in_room);
            }
          }
          if (!(PLR_FLAGGED(d->character, PLR_KIT))) {
            SET_BIT(PLR_FLAGS(d->character), PLR_KIT);

            /* Okay... time to give the kit to the player.. */
            add_innates(d->character);

            for (i = 0; i < NUMITEMINKIT; i++) {
              obj = read_object(autokits[(int) GET_CLASS(d->character)][i], VIRTUAL);
              if (obj != NULL) {
                SET_BIT(GET_OBJ_EXTRA(obj), ITEM_CARRIED);
                /* newbie items flagged as carried */
                SET_BIT(GET_OBJ_EXTRA(obj), ITEM_NOSELL);
                /* newbie items flagged as nosell */GET_OBJ_TIMER(obj) = 1; /* newbie items melt quick */
                obj_to_char(obj, d->character);
              } else
                mudlog("Error! QIC/buggy object in newbie kit!", 'E', COM_IMMORT, FALSE);
            }
          }
          load_text(d->character); /* load aliases etc */
          d->character->char_specials.timer = 0;
          GET_WAS_IN(d->character) = NOWHERE;
          char_to_room(d->character, load_room);
          act("$n has entered the game.", TRUE, d->character, 0, 0, TO_ROOM);

          /* remove this piece of code */
          if (GET_IDNUM(d->character) == 1) {
            GET_LEVEL(d->character) = LVL_IMMORT;
            SET_BIT(COM_FLAGS(d->character), COM_IMMORT);
            SET_BIT(COM_FLAGS(d->character), COM_QUEST);
            SET_BIT(COM_FLAGS(d->character), COM_BUILDER);
            SET_BIT(COM_FLAGS(d->character), COM_ADMIN);
            SET_BIT(PLR_FLAGS(d->character), PLR_UNRESTRICT);
            SET_BIT(PLR_FLAGS(d->character), PLR_ZONEOK);
            SET_BIT(PLR_FLAGS(d->character), PLR_NODELETE);
            SET_BIT(PRF_FLAGS(d->character), PRF_ROOMFLAGS);
            SET_BIT(PRF_FLAGS(d->character), PRF_NOHASSLE);
          }

          /*----*/

          STATE(d) = CON_PLAYING;
          if (!GET_LEVEL(d->character)) {
            do_start(d->character);
            send_to_char(START_MESSG, d->character);
          }
          look_at_room(d->character, 0);
          if (has_mail(GET_NAME(d->character)))
            send_to_char("{WYou have {Rmail {Wwaiting.{x\r\n", d->character);
          if (load_result == 2) { /* rented items lost */
            send_to_char("\r\n\007You could not afford rent!\r\n"
                "Your belongings have been sold to pay your back rent!\r\n", d->character);
          }

          playing = 0;

          for (temp = descriptor_list; temp; temp = temp->next)
            if (!d->connected || d->connected == CON_MEDIT || d->connected == CON_REDIT || d->connected == CON_SEDIT || d->connected == CON_ZEDIT || d->connected == CON_OEDIT)
              playing++;

          if (playing > max_players)
            max_players = playing;

          d->prompt_mode = 1;
          REMOVE_BIT(AFF2_FLAGS(d->character), AFF2_KNOCKEDOUT);
          REMOVE_BIT(PLR_FLAGS(d->character), PLR_CAMP);
          REMOVE_BIT(PLR_FLAGS(d->character), PLR_CRYO);
          REMOVE_BIT(PLR_FLAGS(d->character), PLR_RENT);
          GET_LOGON(d->character) = time(0);
          if (!IS_IMMO(d->character)) {
            char *p;

            while (GET_DESC(d->character) && (p = strchr(GET_DESC(d->character), '{')) != NULL) {
              *p = ' ';
              if (*(p + 1)) {
                *(p + 1) = ' ';
              }
            }
          }
          format_text(&(d->character->player.description), FORMAT_INDENT, d, EXDSCR_LENGTH);
          break;

        case '2':
          SEND_TO_Q("Enter the text you'd like others to see when they look at you.\r\n", d);
          SEND_TO_Q("(/s saves /h for help)\r\n", d);
          if (GET_DESC(d->character)) {
            SEND_TO_Q("Current description:\r\n", d);
            SEND_TO_Q(d->character->player.description, d);
            /* don't free this now... so that the old description gets loaded */
            /* as the current buffer in the editor */
            /* FREE(d->character->player.description); */
            /* d->character->player.description = NULL; */
            /* BUT, do setup the ABORT buffer here */
            d->backstr = strdup(d->character->player.description);
          } else {
            SEND_TO_Q("Current description:\r\n<NONE>\r\n", d);
          }
          d->str = &(d->character->player.description);
          d->max_str = EXDSCR_LENGTH;
          STATE(d) = CON_EXDESC;
          break;

        case '3':
          page_string(d, background, 1);
          STATE(d) = CON_RMOTD;
          break;

        case '4':
          SEND_TO_Q("\r\nEnter your old password: ", d);
          echo_off(d);
          STATE(d) = CON_CHPWD_GETOLD;
          break;
        case '5':
          SEND_TO_Q("\r\nWho do you want to be? ", d);
          STATE(d) = CON_RECONNECT_AS;
          break;

        default:
          SEND_TO_Q("\r\nThat's not a menu choice!\r\n", d);
          SEND_TO_Q_COLOR(MENU, d);
          SEND_TO_Q_COLOR("{WMake your choice{G:{x ", d);
          break;
      }

      break;

    case CON_CHPWD_GETOLD:
      d->idle_cnt = 0;
      if (crypto_pwhash_str_verify(GET_ENCPASSWD(d->character), arg, strlen(arg))) {
        SEND_TO_Q("\r\nIncorrect password.\r\n", d);
        echo_on(d);
        SEND_TO_Q_COLOR(MENU, d);
        SEND_TO_Q_COLOR("{WMake your choice{G:{x ", d);
        STATE(d) = CON_MENU;
        return;
      } else {
        SEND_TO_Q("\r\nEnter a new password: ", d);
        STATE(d) = CON_CHPWD_GETNEW;
        return;
      }

    case CON_CLOSE:
      close_socket(d);
      break;

    default:
      stderr_log("SYSERR: Nanny: illegal state of con'ness; closing connection");
      close_socket(d);
      break;
  }
}
