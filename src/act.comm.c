/* ************************************************************************
 *   File: act.comm.c                                    Part of CircleMUD *
 *  Usage: Player-level communication commands                             *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"

/* extern variables */
extern struct room_data *world;
extern struct descriptor_data *descriptor_list;
extern struct char_data *character_list;
extern struct zone_data *zone_table;
extern void mprog_speech_trigger(char *txt, struct char_data *mob);
extern void mprog_shout_trigger(char *txt, struct char_data *mob);
/* extern void mprog_holler_trigger(char *txt, struct char_data *mob); */
extern void mprog_tell_trigger(char *txt, struct char_data *mob, struct char_data *vmob);
extern void mprog_ask_trigger(char *txt, struct char_data *mob, struct char_data *vmob);
int quest_ask_trigger(char *buf2, struct char_data *ch, struct char_data *vict);

extern int top_of_zone_table;

ACMD(do_say)
{
  skip_spaces(&argument);

  buf[0] = '\0';
  if (!*argument) {
    send_to_char("Yes, but WHAT do you want to say?\r\n", ch);
  } else {
    sprintf(buf, "$n says, '%s'", argument);
    MOBTrigger = FALSE;
    act(buf, FALSE, ch, 0, 0, TO_ROOM);
    if (PRF_FLAGGED(ch, PRF_NOREPEAT)) {
      send_to_char(OK, ch);
    } else {
      sprintf(buf, "You say, '%s'", argument);
      act(buf, FALSE, ch, 0, argument, TO_CHAR);
    }
    mprog_speech_trigger(argument, ch);
  }
}

ACMD(do_gsay)
{
  struct char_data *k;
  struct follow_type *f;

  skip_spaces(&argument);

  buf[0] = '\0';
  if (!IS_AFFECTED(ch, AFF_GROUP)) {
    send_to_char("But you are not the member of a group!\r\n", ch);
    return;
  }
  if (!*argument) {
    send_to_char("Yes, but WHAT do you want to group-say?\r\n", ch);
  } else {
    if (ch->master) {
      k = ch->master;
    } else {
      k = ch;
    }

    sprintf(buf, "{g$n tells the group, '{G%s{g'{x", argument);

    if (IS_AFFECTED(k, AFF_GROUP) && (k != ch)) {
      act(buf, FALSE, ch, 0, k, TO_VICT | TO_SLEEP);
    }
    for (f = k->followers; f; f = f->next) {
      if (IS_AFFECTED(f->follower, AFF_GROUP) && (f->follower != ch)) {
        act(buf, FALSE, ch, 0, f->follower, TO_VICT | TO_SLEEP);
      }
    }

    if (PRF_FLAGGED(ch, PRF_NOREPEAT)) {
      send_to_char(OK, ch);
    } else {
      sprintf(buf, "{gYou tell the group, '{G%s{g'{x", argument);
      act(buf, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);
    }
  }
}

void perform_tell(struct char_data *ch, struct char_data *vict, char *arg)
{
  char **msg;

  buf[0] = '\0';
  if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
    sprintf(buf, "{r%s's $n tells you, '{R%s{r'{x", GET_NAME(ch->master), arg);
  } else {
    sprintf(buf, "{r$n tells you, '{R%s{r'{x", arg);
  }
  act(buf, FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);

  if (!IS_NPC(vict)) {
    msg = &(GET_TELL(vict));
    if (*msg) {
      FREE(*msg);
    }
    *msg = strdup(arg);
  }

  if (PRF_FLAGGED(ch, PRF_NOREPEAT)) {
    send_to_char(OK, ch);
  } else {
    sprintf(buf, "{rYou tell $N, '{R%s{r'{x", arg);
    act(buf, FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  }
}

/*
 * Yes, do_tell probably could be combined with whisper and ask, but
 * called frequently, and should IMHO be kept as tight as possible.
 */
ACMD(do_tell)
{
  struct char_data *vict;
  char tbuf[256];

  buf[0] = '\0';
  buf2[0] = '\0';
  tbuf[0] = '\0';
  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2) {
    if (IS_NPC(ch) || GET_LAST_TELL(ch) == NULL) {
      send_to_char("Who do you wish to tell what??\r\n", ch);
    } else {
      sprintf(buf, "{r%s told you, '{R%s{r'{x", GET_LAST_TELL(ch), GET_TELL(ch));
      act(buf, FALSE, ch, 0, 0, TO_CHAR);
    }
  } else if (!(vict = get_char_vis(ch, buf)) || (IS_NPC(vict) && !IS_IMMO(ch) && !IS_NPC(ch))) {
    send_to_char(NOPERSON, ch);
  } else if (ch == vict) {
    send_to_char("You try to tell yourself something.\r\n", ch);
  } else if (PRF_FLAGGED(ch, PRF_NOTELL)) {
    send_to_char("You can't tell other people while you have notell on.\r\n", ch);
  } else if (ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF) && (GET_LEVEL(ch) < LVL_IMMORT)) {
    send_to_char("The walls seem to absorb your words.\r\n", ch);
  } else if (!IS_NPC(vict) && !vict->desc) /* linkless */{
    act("$E's linkless at the moment.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  } else if (PLR_FLAGGED(vict, PLR_WRITING)) {
    act("$E's writing a message right now; try again later.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  } else if (PLR_FLAGGED(vict, PLR_EDITING)) {
    act("$E's editing some world files right now; try again later.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  } else if ((PRF_FLAGGED(vict, PRF_NOTELL) && GET_LEVEL(ch) < LVL_IMMORT) || (ROOM_FLAGGED(vict->in_room, ROOM_SOUNDPROOF) && GET_LEVEL(ch) < LVL_IMMORT) || ((GET_POS(vict) == POS_SLEEPING) && GET_LEVEL(ch) < LVL_IMMORT)) {
    act("$E can't hear you.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  } else {
    FREE(GET_LAST_TELL(vict));
    if (CAN_SEE(vict, ch)) {
      if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
        sprintf(tbuf, "%s's %s", GET_NAME(ch->master), CAP(PERS(ch, vict)));
        GET_LAST_TELL(vict) = strdup(tbuf);
      } else {
        GET_LAST_TELL(vict) = strdup(CAP(PERS(ch, vict)));
      }
    } else {
      GET_LAST_TELL(vict) = NULL;
    }
    perform_tell(ch, vict, buf2);
    if (PRF_FLAGGED(vict, PRF_AFK)) {
      act("$E is afk. Don't expect a response.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
    }
    if (IS_NPC(vict)) {
      mprog_tell_trigger(buf2, ch, vict);
    }
  }
}

ACMD(do_reply)
{
  char *newtell;
  skip_spaces(&argument);

  if (GET_LAST_TELL(ch) == NULL) {
    send_to_char("You have no-one to reply to!\r\n", ch);
  } else if (!*argument) {
    send_to_char("What is your reply?\r\n", ch);
  } else {
    newtell = (char*) malloc(strlen(GET_LAST_TELL(ch)) + strlen(argument) + 32);
    sprintf(newtell, "%s %s", GET_LAST_TELL(ch), argument);
    do_tell(ch, newtell, 0, 0);
    FREE(newtell);
  }
}

ACMD(do_spec_comm)
{
  struct char_data *vict;
  char *action_sing, *action_plur, *action_others;
  char buf2[8196];

  buf[0] = '\0';
  if (subcmd == SCMD_WHISPER) {
    action_sing = "whisper to";
    action_plur = "whispers to";
    action_others = "$n whispers something to $N.";
  } else {
    action_sing = "ask";
    action_plur = "asks";
    action_others = "$n asks $N a question.";
  }

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2) {
    sprintf(buf, "Whom do you want to %s.. and what??\r\n", action_sing);
    send_to_char(buf, ch);
  } else if (!(vict = get_char_room_vis(ch, buf))) {
    send_to_char(NOPERSON, ch);
  } else if (vict == ch) {
    send_to_char("You can't get your mouth close enough to your ear...\r\n", ch);
  } else {
    sprintf(buf, "$n %s you, '%s'", action_plur, buf2);
    act(buf, FALSE, ch, 0, vict, TO_VICT);
    if (PRF_FLAGGED(ch, PRF_NOREPEAT)) {
      send_to_char(OK, ch);
    } else {
      sprintf(buf, "You %s %s, '%s'", action_sing, GET_NAME(vict), buf2);
      act(buf, FALSE, ch, 0, 0, TO_CHAR);
    }
    act(action_others, FALSE, ch, 0, vict, TO_NOTVICT);
    if (IS_NPC(vict)) {
      if (!quest_ask_trigger(buf2, ch, vict)) {
        mprog_ask_trigger(buf2, ch, vict);
      }
    }
  }
}

#define MAX_NOTE_LENGTH 1000	/* arbitrary */

ACMD(do_write)
{
  struct obj_data *paper = 0, *pen = 0;
  char *papername, *penname;

  buf[0] = '\0';
  buf1[0] = '\0';
  buf2[0] = '\0';
  papername = buf1;
  penname = buf2;

  two_arguments(argument, papername, penname);

  if (!ch->desc) {
    return;
  }

  if (!*papername) { /* nothing was delivered */
    send_to_char("Write?  With what?  ON what?  What are you trying to do?!?\r\n", ch);
    return;
  }
  if (*penname) { /* there were two arguments */
    if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying))) {
      sprintf(buf, "You have no %s.\r\n", papername);
      send_to_char(buf, ch);
      return;
    }
    if (!(pen = get_obj_in_list_vis(ch, penname, ch->carrying))) {
      sprintf(buf, "You have no %s.\r\n", papername);
      send_to_char(buf, ch);
      return;
    }
  } else { /* there was one arg.. let's see what we can find */
    if (!(paper = get_obj_in_list_vis(ch, papername, ch->carrying))) {
      sprintf(buf, "There is no %s in your inventory.\r\n", papername);
      send_to_char(buf, ch);
      return;
    }
    if (GET_OBJ_TYPE(paper) == ITEM_PEN) { /* oops, a pen.. */
      pen = paper;
      paper = 0;
    } else if (GET_OBJ_TYPE(paper) != ITEM_NOTE) {
      send_to_char("That thing has nothing to do with writing.\r\n", ch);
      return;
    }
    /* One object was found.. now for the other one. */
    if (!ch->equipment[WEAR_HOLD]) {
      sprintf(buf, "You can't write with %s %s alone.\r\n", AN(papername), papername);
      send_to_char(buf, ch);
      return;
    }
    if (!CAN_SEE_OBJ(ch, ch->equipment[WEAR_HOLD])) {
      send_to_char("The stuff in your hand is invisible!  Yeech!!\r\n", ch);
      return;
    }
    if (pen) {
      paper = ch->equipment[WEAR_HOLD];
    } else {
      pen = ch->equipment[WEAR_HOLD];
    }
  }

  /* ok.. now let's see what kind of stuff we've found */
  if (GET_OBJ_TYPE(pen) != ITEM_PEN) {
    act("$p is no good for writing with.", FALSE, ch, pen, 0, TO_CHAR);
  } else if (GET_OBJ_TYPE(paper) != ITEM_NOTE) {
    act("You can't write on $p.", FALSE, ch, paper, 0, TO_CHAR);
  } else if (paper->action_description) {
    send_to_char("There's something written on it already.\r\n", ch);
  } else {
    /* we can write - hooray! */
    /* this is the PERFECT code example of how to set up:
     * a) the text editor with a message already loaed
     * b) the abort buffer if the player aborts the message
     */
    ch->desc->backstr = NULL;
    send_to_char("Write your note.  (/s saves /h for help)\r\n", ch);
    /* ok, here we check for a message ALREADY on the paper */
    if (paper->action_description) {
      /* we strdup the original text to the descriptors->backstr */
      ch->desc->backstr = strdup(paper->action_description);
      /* send to the player what was on the paper (cause this is already */
      /* loaded into the editor) */
      send_to_char(paper->action_description, ch);
    }
    act("$n begins to jot down a note.", TRUE, ch, 0, 0, TO_ROOM);
    /* assign the descriptor's->str the value of the pointer to the text */
    /* pointer so that we can reallocate as needed (hopefully that made */
    /* sense :>) */
    ch->desc->str = &paper->action_description;
    ch->desc->max_str = MAX_NOTE_LENGTH;
  }
}

ACMD(do_page)
{
  struct descriptor_data *d;
  struct char_data *vict;

  half_chop(argument, arg, buf2);

  if (IS_NPC(ch)) {
    send_to_char("Monsters can't page.. go away.\r\n", ch);
  } else if (!*arg) {
    send_to_char("Whom do you wish to page?\r\n", ch);
  } else {
    sprintf(buf, "\007\007*%s* %s\r\n", GET_NAME(ch), buf2);
    if (!str_cmp(arg, "all")) {
      if (COM_FLAGGED(ch, COM_BUILDER)) {
        for (d = descriptor_list; d; d = d->next) {
          if (!d->connected && d->character) {
            act(buf, FALSE, ch, 0, d->character, TO_VICT);
          }
        }
      } else {
        send_to_char("You will never be godly enough to do that!\r\n", ch);
        return;
      }
    }
    if ((vict = get_char_vis(ch, arg)) != NULL) {
      act(buf, FALSE, ch, 0, vict, TO_VICT);
      if (PRF_FLAGGED(ch, PRF_NOREPEAT)) {
        send_to_char(OK, ch);
      } else {
        act(buf, FALSE, ch, 0, vict, TO_CHAR);
      }
      return;
    } else {
      send_to_char("There is no such person in the game!\r\n", ch);
    }
  }
}

/**********************************************************************
 * generalized communication func, originally by Fred C. Merkel (Torg) *
 *********************************************************************/

ACMD(do_gen_comm)
{
  extern int level_can_shout;
  /*  extern int holler_move_cost; */
  /*  struct descriptor_data *i; */
  struct char_data *c;
  char color_on[24];
  int i, j, tozone = 0;
  struct char_data *tochar = NULL;
  bool totown = FALSE;

  /* Array of flags which must _not_ be set in order for comm to be heard */
  static int channels[] = {0, PRF_DEAF, PRF_NOCHAT, PRF_NOAUCT, PRF_NOGRATZ, 0};

  /*
   * com_msgs: [0] Message if you can't perform the action because of noshout
   * [1] name of the action [2] message if you're not on the channel [3] a
   * color string.
   */
  static char *com_msgs[][4] = {
      {"You cannot pray!!\r\n", "pray", "", "{C"},
      {"You cannot shout!!\r\n", "shout", "Turn off your noshout flag first!\r\n", "{W"},
      {"You cannot chat!!\r\n", "chat", "You aren't even on the channel!\r\n", "{Y"},
      {"You cannot auction!!\r\n", "auction", "You aren't even on the channel!\r\n", "{M"},
      {"You cannot congratulate!\r\n", "congrat", "You aren't even on the channel!\r\n", "{G"},
      {"You cannot petition!!\r\n", "petition", "", "{C"}
  };

  buf[0] = '\0';
  buf1[0] = '\0';
  buf2[0] = '\0';
  /* to keep pets, etc from being ordered to shout */
  /*
   if (!ch->desc) {
   return;
   }
   */

  if (IS_NPC(ch) && IS_AFFECTED(ch, AFF_CHARM)) {
    if (ch->master) {
      if (!IS_NPC(ch->master) && PLR_FLAGGED(ch->master, PLR_NOSHOUT)) {
        return;
      }
    }
  }

  if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
    send_to_char(com_msgs[subcmd][0], ch);
    return;
  }
  if ((ROOM_FLAGGED(ch->in_room, ROOM_SOUNDPROOF)) && (subcmd != SCMD_PETI) && (subcmd != SCMD_PRAY) && (!IS_NPC(ch) && !COM_FLAGGED(ch, COM_IMMORT))) {
    send_to_char("The walls seem to absorb your words.\r\n", ch);
    return;
  }
  /* level_can_shout defined in config.c */
  if (GET_LEVEL(ch) < level_can_shout) {
    sprintf(buf1, "You must be at least level %d before you can %s.\r\n", level_can_shout, com_msgs[subcmd][1]);
    send_to_char(buf1, ch);
    return;
  }
  /* make sure the char is on the channel */
  if (PRF_FLAGGED(ch, channels[subcmd])) {
    send_to_char(com_msgs[subcmd][2], ch);
    return;
  }
  /* skip leading spaces */
  skip_spaces(&argument);

  /* make sure that there is something there to say! */
  if (!*argument) {
    sprintf(buf1, "Yes, %s, fine, %s we must, but WHAT???\r\n", com_msgs[subcmd][1], com_msgs[subcmd][1]);
    send_to_char(buf1, ch);
    return;
  }
  if ((subcmd == SCMD_SHOUT) && (IS_IMMO(ch))) {
    if (*argument == '#') {
      one_argument(argument + 1, buf2);
      if (is_number(buf2)) {
        half_chop(argument + 1, buf2, argument);
        j = atoi(buf2);
        for (i = 0; i <= top_of_zone_table; i++) {
          if (zone_table[i].number == j) {
            break;
          }
        }
        if (i >= 0 && i <= top_of_zone_table) {
          tozone = i;
        } else {
          send_to_char("Invalid zone number.\r\n", ch);
          return;
        }
      } else {
        send_to_char("You must supply a valid zone to shout to.\r\n", ch);
        return;
      }
    } else {
      tozone = world[ch->in_room].zone;
    }
  } else if ((subcmd == SCMD_SHOUT) && !IS_IMMO(ch)) {
    tozone = world[ch->in_room].zone;
  }

  if ((subcmd == SCMD_SHOUT) && ((zone_table[tozone].number == 30) || (zone_table[tozone].number == 31))) {
    totown = TRUE;
  }

  if (subcmd == SCMD_PRAY || subcmd == SCMD_PETI) {
    if (IS_IMMO(ch)) {
      half_chop(argument, arg, buf2);
      if ((tochar = get_char_vis(ch, arg)) != NULL) {
        sprintf(buf1, "{r$n speaks to you, '{W%s{r'{x", buf2);
        act(buf1, FALSE, ch, 0, tochar, TO_VICT | TO_SLEEP);
      } else {
        send_to_char("That person is not here.\r\n", ch);
        return;
      }
    }
  }
  /* set up the color on code */
  strcpy(color_on, com_msgs[subcmd][3]);

  /* first, set up strings to be given to the communicator */
  if (PRF_FLAGGED(ch, PRF_NOREPEAT)) {
    send_to_char(OK, ch);
  } else {
    if ((subcmd == SCMD_SHOUT) && IS_IMMO(ch)) {
      sprintf(buf1, "{WYou %s, '%s{W' to zone %d.{x", com_msgs[subcmd][1], argument, zone_table[tozone].number);
    } else if ((subcmd == SCMD_CHAT) && IS_NPC(ch)) {
      send_to_char("You don't need to chat, your a NPC!\r\n", ch);
    } else if ((subcmd == SCMD_PRAY) && !IS_IMMO(ch)) {
      sprintf(buf1, "{rYou pray, '{W%s{r'{x", argument);
    } else if ((subcmd == SCMD_PETI) && !IS_IMMO(ch)) {
      sprintf(buf1, "{rYou petition, '{W%s{r'{x", argument);
      subcmd = SCMD_PRAY;
    } else if ((subcmd == SCMD_PRAY || subcmd == SCMD_PETI) && IS_IMMO(ch)) {
      sprintf(buf1, "{rYou speak, '{W%s{r' to {c%s{r.{x", buf2, GET_NAME(tochar));
    } else {
      sprintf(buf1, "%sYou %s, '%s%s'{x", color_on, com_msgs[subcmd][1], argument, color_on);
    }
    act(buf1, FALSE, ch, 0, 0, TO_CHAR | TO_SLEEP);
    /*  removed warning for using chat channel.
     if (subcmd == SCMD_CHAT && GET_LEVEL(ch) < LVL_IMMORT) {
     send_to_char("{RWARNING!{Y This channel is for non-mud related topics only.{x\r\n", ch);
     send_to_char("\r\n", ch);
     }
     */
  }

  /* now send all the strings out */
  for (c = character_list; c; c = c->next) {
    if (c != ch && c && !PRF_FLAGGED(c, channels[subcmd]) && !PLR_FLAGGED(c, PLR_WRITING) && !PLR_FLAGGED(c, PLR_EDITING) && (!ROOM_FLAGGED(c->in_room, ROOM_SOUNDPROOF) || subcmd == SCMD_PRAY || subcmd == SCMD_PETI)) {
      if ((subcmd == SCMD_SHOUT) && (IS_NPC(ch)) && (PRF_FLAGGED(c, PRF_MOBDEAF))) {
        continue;
      }
      if ((subcmd == SCMD_SHOUT) && (totown) && ((zone_table[world[c->in_room].zone].number != 30) && (zone_table[world[c->in_room].zone].number != 31)) && (!IS_IMMO(c))) {
        continue;
      }
      if ((subcmd == SCMD_SHOUT) && (!totown) && (((tozone != world[c->in_room].zone) && (!IS_IMMO(c))) || (GET_POS(c) < POS_RESTING))) {
        continue;
      }
      if ((subcmd == SCMD_PRAY || subcmd == SCMD_PETI) && !IS_IMMO(c)) {
        continue;
      }
      if ((subcmd == SCMD_PRAY || subcmd == SCMD_PETI) && IS_IMMO(c) && IS_IMMO(ch) && (c == tochar)) {
        continue;
      }
      if ((subcmd == SCMD_PRAY || subcmd == SCMD_PETI) && IS_IMMO(c) && IS_IMMO(ch) && (c != tochar)) {
        sprintf(buf, "{r$n speaks, '{W%s{r' to {c%s{r.{x", buf2, GET_NAME(tochar));
      } else if ((subcmd == SCMD_PRAY || subcmd == SCMD_PETI) && IS_IMMO(c) && !IS_IMMO(ch)) {
        sprintf(buf, "{r$n prays, '{W%s{r'{x", argument);
      } else if ((subcmd == SCMD_SHOUT) && (IS_IMMO(c))) {
        sprintf(buf, "{W$n %ss, '%s{W' to zone %d{x", com_msgs[subcmd][1], argument, zone_table[tozone].number);
      } else {
        if (IS_NPC(ch) && AFF_FLAGGED(ch, AFF_CHARM) && ch->master) {
          sprintf(buf, "%s%s's $n %s%ss, '%s%s'{x", color_on, GET_NAME(ch->master), color_on, com_msgs[subcmd][1], argument, color_on);
        } else {
          sprintf(buf, "%s$n %s%ss, '%s%s'{x", color_on, color_on, com_msgs[subcmd][1], argument, color_on);
        }
      }
      act(buf, FALSE, ch, 0, c, TO_VICT | TO_SLEEP);
    }
  }
  if (subcmd == SCMD_SHOUT) {
    mprog_shout_trigger(argument, ch);
  }
}
