/* ************************************************************************
 *   File: graph.c                                       Part of CircleMUD *
 *  Usage: various graph algorithms                                        *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#define TRACK_THROUGH_DOORS 

/* You can define or not define TRACK_THOUGH_DOORS, above, depending on
 whether or not you want track to find paths which lead through closed
 or hidden doors.
 */

#include <stdio.h>
#include <stdlib.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "event.h"
#include "spells.h"

/* Externals */
extern int top_of_world;
extern struct char_data *character_list;
extern const char *dirs[];
extern struct room_data *world;
extern struct spell_info_type *spells;

int find_skill_num(char *name);
void improve_skill(struct char_data *ch, int skill, int chance);

struct bfs_queue_struct {
  sh_int room;
  char dir;
  struct bfs_queue_struct *next;
};

static struct bfs_queue_struct *queue_head = 0, *queue_tail = 0;

/* Utility macros */
#define MARK(room) (SET_BIT(ROOM_FLAGS(room), ROOM_BFS_MARK))
#define UNMARK(room) (REMOVE_BIT(ROOM_FLAGS(room), ROOM_BFS_MARK))
#define IS_MARKED(room) (IS_SET(ROOM_FLAGS(room), ROOM_BFS_MARK))
#define TOROOM(x, y) (world[(x)].dir_option[(y)]->to_room)
#define IS_CLOSED(x, y) (IS_SET(world[(x)].dir_option[(y)]->exit_info, EX_CLOSED))

#ifdef TRACK_THROUGH_DOORS
#define VALID_EDGE(x, y) (world[(x)].dir_option[(y)] && \
        (TOROOM(x, y) != NOWHERE) &&  \
        (!ROOM_FLAGGED(TOROOM(x, y), ROOM_NOTRACK)) && \
        (!ROOM_FLAGGED(TOROOM(x, y), ROOM_NOMOB)) && \
        (!IS_MARKED(TOROOM(x, y))))
#else
#define VALID_EDGE(x, y) (world[(x)].dir_option[(y)] && \
        (TOROOM(x, y) != NOWHERE) &&  \
        (!IS_CLOSED(x, y)) &&    \
        (!ROOM_FLAGGED(TOROOM(x, y), ROOM_NOTRACK)) && \
        (!ROOM_FLAGGED(TOROOM(x, y), ROOM_NOMOB)) && \
        (!IS_MARKED(TOROOM(x, y))))
#endif

struct char_data *find_hunted_char(int idnum)
{
  struct char_data *tmp;
  struct char_data *tmp_next;
  for (tmp = character_list; tmp; tmp = tmp_next) {
    tmp_next = tmp->next;
    if (GET_IDNUM(tmp) == idnum && !IS_AFFECTED(tmp, AFF_NOTRACK))
      return tmp;
  }
  return NULL;
}
void bfs_enqueue(sh_int room, char dir)
{
  struct bfs_queue_struct *curr;

  CREATE(curr, struct bfs_queue_struct, 1);
  curr->room = room;
  curr->dir = dir;
  curr->next = 0;

  if (queue_tail) {
    queue_tail->next = curr;
    queue_tail = curr;
  } else
    queue_head = queue_tail = curr;
}

void bfs_dequeue(void)
{
  struct bfs_queue_struct *curr;

  curr = queue_head;

  if (!(queue_head = queue_head->next)) {
    queue_tail = 0;
  }
  free(curr);
}

void bfs_clear_queue(void)
{
  while (queue_head) {
    bfs_dequeue();
  }
}

/* find_first_step: given a source room and a target room, find the first
 step on the shortest path from the source to the target.

 Intended usage: in mobile_activity, give a mob a dir to go if they're
 tracking another mob or a PC.  Or, a 'track' skill for PCs.
 */

int find_first_step(sh_int src, sh_int target, int stay_zone)
{
  int curr_dir;
  sh_int curr_room;
  int src_zone = ((src - (src % 100)) / 100);
  int target_zone = ((target - (target % 100)) / 100);

  if (src < 0 || src > top_of_world || target < 0 || target > top_of_world) {
    stderr_log("Illegal value passed to find_first_step (graph.c)");
    return BFS_ERROR;
  }

  /* dez 19980805
   if ((src_zone != target_zone && stay_zone == 1) || stay_zone == 2) {
   return BFS_NO_PATH;
   }
   */
  if (src_zone != target_zone && stay_zone == 1) {
    return BFS_NO_PATH;
  }

  if (src == target) {
    return BFS_ALREADY_THERE;
  }

  /* clear marks first */
  for (curr_room = 0; curr_room <= top_of_world; curr_room++) {
    UNMARK(curr_room);
  }

  MARK(src);

  /* first, enqueue the first steps, saving which direction we're going. */
  for (curr_dir = 0; curr_dir < NUM_OF_DIRS; curr_dir++) {
    if (VALID_EDGE(src, curr_dir)) {
      MARK(TOROOM(src, curr_dir));
      bfs_enqueue(TOROOM(src, curr_dir), curr_dir);
    }
  }
  /* now, do the classic BFS. */
  while (queue_head) {
    if (queue_head->room == target) {
      curr_dir = queue_head->dir;
      bfs_clear_queue();
      return curr_dir;
    } else {
      for (curr_dir = 0; curr_dir < NUM_OF_DIRS; curr_dir++) {
        if (VALID_EDGE(queue_head->room, curr_dir)) {
          MARK(TOROOM(queue_head->room, curr_dir));
          bfs_enqueue(TOROOM(queue_head->room, curr_dir), queue_head->dir);
        }
      }
      bfs_dequeue();
    }
  }

  return BFS_NO_PATH;
}

/************************************************************************
 *  Functions and Commands which use the above fns            *
 ************************************************************************/

ACMD(do_track)
{
  struct char_data *vict;
  int dir, num;
  int skillnum = spells[find_skill_num("track")].spellindex;

  if (!GET_SKILL(ch, skillnum)) {
    send_to_char("You have no idea how.\r\n", ch);
    return;
  }
  one_argument(argument, arg);
  if (!*arg) {
    send_to_char("Whom are you trying to track?\r\n", ch);
    return;
  }
  if (!(vict = get_char_vis(ch, arg))) {
    send_to_char("No-one around by that name.\r\n", ch);
    return;
  }

  if (GET_MOVE(ch) < 10) {
    send_to_char("You cant summon enough energy to do that!\r\n", ch);
    return;
  } else {
    GET_MOVE(ch) = GET_MOVE(ch) - 10;
  }

  if ((IS_NPC(vict) && (world[ch->in_room].zone != world[vict->in_room].zone)) || IS_AFFECTED(vict, AFF_NOTRACK)) {
    send_to_char("No-one around by that name.\r\n", ch);
    return;
  }

  /* dez 19980805
   if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_SENTINEL)) {
   dir = find_first_step(ch->in_room, vict->in_room, 2);
   } else if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_STAY_ZONE)) {
   */

  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_STAY_ZONE)) {
    dir = find_first_step(ch->in_room, vict->in_room, 1);
  } else {
    dir = find_first_step(ch->in_room, vict->in_room, 0);
  }

  switch (dir) {
    case BFS_ERROR:
      send_to_char("Hmm.. something seems to be wrong.\r\n", ch);
      break;
    case BFS_ALREADY_THERE:
      send_to_char("You're already in the same room!!\r\n", ch);
      break;
    case BFS_NO_PATH:
      safe_snprintf(buf, MAX_STRING_LENGTH, "You can't sense a trail to %s from here.\r\n", HMHR(vict));
      send_to_char(buf, ch);
      break;
    default:
      num = number(0, 101); /* 101% is a complete failure */
      if (GET_SKILL(ch, skillnum) < num) {
        do {
          dir = number(0, NUM_OF_DIRS - 1);
        } while (!CAN_GO(ch, dir));
      }
      safe_snprintf(buf, MAX_STRING_LENGTH, "You sense a trail %s from here!\r\n", dirs[dir]);
      send_to_char(buf, ch);
      improve_skill(ch, skillnum, SKUSE_AVERAGE);
      break;
  }
}

void hunt_victim(struct char_data * ch)
{
  extern struct char_data *character_list;
  ACMD(do_open);

  int dir;
  byte found;
  struct char_data *tmp;
  struct char_data *hunted_ch;
  char abuf[128];
  char doorname[80];

  if (!ch || !HUNTING(ch) || AFF2_FLAGGED(ch, AFF2_MINOR_PARALIZED) || AFF_FLAGGED(ch, AFF_MAJOR_PARALIZED)) {
    return;
  }

  hunted_ch = find_hunted_char(HUNTING(ch));
  /* make sure the char still exists */
  for (found = 0, tmp = character_list; tmp && !found && hunted_ch; tmp = tmp->next) {
    if (HUNTING(ch) == GET_IDNUM(tmp)) {
      found = 1;
    }
  }

  if (!found) {
    act("$n says, 'Damn!  My prey is gone!!'", TRUE, ch, 0, 0, TO_ROOM);
    /* don't forget vict until they die or I am dead
     HUNTING(ch) = 0;
     */
    return;
  }

  /* dez 19980805
   if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_SENTINEL)) {
   dir = find_first_step(ch->in_room, hunted_ch->in_room, 2);
   } else if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_STAY_ZONE)) {
   */

  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_STAY_ZONE)) {
    dir = find_first_step(ch->in_room, hunted_ch->in_room, 1);
  } else {
    dir = find_first_step(ch->in_room, hunted_ch->in_room, 0);
  }

  if (dir < 0) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "$n says 'Damn!  Lost %s!'", HMHR(hunted_ch));
    act(buf, TRUE, ch, 0, 0, TO_ROOM);
    /* don't forget vict until they die or I am dead
     HUNTING(ch) = 0;
     */
    return;
  } else {
    if (IS_CLOSED(ch->in_room, dir)) {
      one_argument(EXIT(ch, dir)->keyword, doorname);
      safe_snprintf(abuf, sizeof(abuf), "%s %s", doorname, dirs[dir]);
      do_open(ch, abuf, 0, 0);
    }
    perform_move(ch, dir, 1);
    if (ch->in_room == hunted_ch->in_room && !ROOM_FLAGGED(hunted_ch->in_room, ROOM_PEACEFUL)) {
      if (CAN_SEE(ch, hunted_ch)) {
        hit(ch, hunted_ch, TYPE_UNDEFINED);
      }
    }
    return;
  }
}

void hunt_room(struct char_data * ch)
{
  int dir = -1;

  if (!ch) {
    return;
  }

  /* make sure the room is valid */
  if (HUNTINGRM(ch) < 0 || AFF_FLAGGED(ch, AFF_MAJOR_PARALIZED) || AFF2_FLAGGED(ch, AFF2_MINOR_PARALIZED)) {
    return;
  }

  /* dez 19980805
   if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_SENTINEL)) {
   dir = find_first_step(ch->in_room, HUNTINGRM(ch), 2);
   } else if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_STAY_ZONE)) {
   */
  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_STAY_ZONE)) {
    dir = find_first_step(ch->in_room, HUNTINGRM(ch), 1);
  } else {
    dir = find_first_step(ch->in_room, HUNTINGRM(ch), 0);
  }

  if (dir < 0) {
    HUNTINGRM(ch) = -1;
    return;
  } else {
    perform_move(ch, dir, 1);
    if (ch->in_room == HUNTINGRM(ch)) {
      HUNTINGRM(ch) = -1;
    }
    return;
  }
}

void hunt_aggro(struct char_data * ch)
{
  int dir;
  int skillnum = spells[find_skill_num("aggressive")].spellindex;
  struct char_data *hunted_ch;

  if (!ch || !HUNTING(ch) || FIGHTING(ch)) {
    return;
  }
  /*
   if (world[ch->in_room].zone != world[HUNTING(ch)->in_room].zone) {
   HUNTING(ch) = 0;
   return;
   }
   */
  if ((hunted_ch = find_hunted_char(HUNTING(ch)))) {
    if (ch->in_room == hunted_ch->in_room && !ROOM_FLAGGED(hunted_ch->in_room, ROOM_PEACEFUL)) {
      if (CAN_SEE(hunted_ch, ch) && GET_SKILL(hunted_ch, skillnum) > number(0, 101)) {
        improve_skill(ch, skillnum, SKUSE_AVERAGE);
        hit(hunted_ch, ch, TYPE_UNDEFINED);
      } else {
        if (CAN_SEE(ch, hunted_ch)) {
          hit(ch, hunted_ch, TYPE_UNDEFINED);
        }
      }
      return;
    }
  } else {
    return;
  }

  /* dez 19980805
   if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_SENTINEL)) {
   dir = find_first_step(ch->in_room, hunted_ch->in_room, 2);
   } else if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_STAY_ZONE)) {
   */
  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_STAY_ZONE)) {
    dir = find_first_step(ch->in_room, hunted_ch->in_room, 1);
  } else {
    dir = find_first_step(ch->in_room, hunted_ch->in_room, 0);
  }

  if (dir < 0) {
    /* do not forget until player is dead or I die
     HUNTING(ch) = 0;
     */
    return;
  } else {
    perform_move(ch, dir, 1);
    if (ch->in_room == hunted_ch->in_room) {
      if (!ROOM_FLAGGED(ch->in_room, ROOM_PEACEFUL)) {
        if (CAN_SEE(hunted_ch, ch) && GET_SKILL(hunted_ch, skillnum) > number(0, 101)) {
          improve_skill(ch, skillnum, SKUSE_AVERAGE);
          hit(hunted_ch, ch, TYPE_UNDEFINED);
        } else {
          if (CAN_SEE(ch, hunted_ch)) {
            hit(ch, hunted_ch, TYPE_UNDEFINED);
          }
        }
      }
    }
    return;
  }
}
