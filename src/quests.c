#include "comm.h"
#include "db.h"
#include "event.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define QFLAGS(i) (mob_quests[(i)].flags)

extern struct spell_info_type *spells;
extern struct quest_data *mob_quests;
extern struct char_data *mob_proto;
extern struct obj_data *obj_proto;
extern struct index_data *obj_index;
int find_spell_num(char *name);
int find_skill_num(char *name);
void extract_obj(struct obj_data *obj);
void obj_from_char(struct obj_data *object);
void parse_pline(char *line, char *field, char *value);
void obj_to_char(struct obj_data *object, struct char_data *ch);
long asciiflag_conv(char *flag);
int search_block(char *arg, char **list, bool exact);
int quest_ask_trigger(char *buf2, struct char_data *ch, struct char_data *vict);

char *goal_list[] = {"knowledge", "object", "experience", "skill", "\n"};
char *needs_list[] = {"money", "object", "\n"};

void parse_quest(FILE *quest_file, int vnum) {
  static int i = 0;

  struct char_data *mob = NULL;
  char line[MAX_INPUT_LENGTH];
  char value[MAX_INPUT_LENGTH];
  char field[20];
  int numval = 0;
  int nummsgs = 0;
  int numneeds = 0;
  int mob_rnum = 0;

  i++;
  mob_rnum = real_mobile(vnum);
  if (mob_rnum == -1) {
    stderr_log("Error trying to assign quest to non-existant mob.");
    fflush(NULL);
    exit(1);
  }
  mob = (mob_proto + mob_rnum);
  GET_MOB_QUEST_NUM(mob) = i;
  (mob_quests + i)->maxlevel = 51;
  (mob_quests + i)->qnum = i;
  while (get_line(quest_file, line)) {
    if (line[0] == 'S') {
      return;
    }
    parse_pline(line, field, value);
    numval = atoi(value);

    switch (UPPER(*field)) {
    case 'A':
      if (strcmp(field, "amount") == 0) {
        (mob_quests + i)->needs[numneeds].amount = numval;
        numneeds++;
        (mob_quests + i)->maxneeds = numneeds;
      } else {
        safe_snprintf(buf2, MAX_STRING_LENGTH, "Unknown Quest field [%s]", field);
        stderr_log(buf2);
      }
      break;
    case 'C':
      if (strcmp(field, "classlist") == 0) {
        (mob_quests + i)->classlist = asciiflag_conv(value);
      } else {
        safe_snprintf(buf2, MAX_STRING_LENGTH, "Unknown Quest field [%s]", field);
        stderr_log(buf2);
      }
      break;
    case 'D':
      if (strcmp(field, "destroy") == 0) {
        if (strcasecmp(value, "no") == 0) {
          (mob_quests + i)->needs[numneeds].destroy = 0;
        }
      } else {
        safe_snprintf(buf2, MAX_STRING_LENGTH, "Unknown Quest field [%s]", field);
        stderr_log(buf2);
      }
      break;
    case 'F':
      if (strcmp(field, "flags") == 0) {
        (mob_quests + i)->flags = asciiflag_conv(value);
      } else {
        safe_snprintf(buf2, MAX_STRING_LENGTH, "Unknown Quest field [%s]", field);
        stderr_log(buf2);
      }
      break;
    case 'G':
      if (strcmp(field, "goal") == 0) {
        (mob_quests + i)->goal = search_block(value, goal_list, FALSE);
      } else {
        safe_snprintf(buf2, MAX_STRING_LENGTH, "Unknown Quest field [%s]", field);
        stderr_log(buf2);
      }
      break;
    case 'K':
      if (strcmp(field, "keywords") == 0) {
        RECREATE((mob_quests + i)->messages, struct quest_message_data, nummsgs + 1);
        (mob_quests + i)->messages[nummsgs].keywords = strdup(value);
      } else if (strcmp(field, "knowledge") == 0) {
        line[0] = '\0';
        (mob_quests + i)->knowledge = fread_string(quest_file, line);
      } else {
        safe_snprintf(buf2, MAX_STRING_LENGTH, "Unknown Quest field [%s]", field);
        stderr_log(buf2);
      }
      break;
    case 'M':
      if (strcmp(field, "maxlevel") == 0) {
        (mob_quests + i)->maxlevel = numval;
      } else if (strcmp(field, "message") == 0) {
        line[0] = '\0';
        (mob_quests + i)->messages[nummsgs].message = fread_string(quest_file, line);
        nummsgs++;
        (mob_quests + i)->maxmsgs = nummsgs;
      } else {
        safe_snprintf(buf2, MAX_STRING_LENGTH, "Unknown Quest field [%s]", field);
        stderr_log(buf2);
      }
      break;
    case 'N':
      if (strcmp(field, "needs") == 0) {
        RECREATE((mob_quests + i)->needs, struct quest_needs_data, numneeds + 1);
        (mob_quests + i)->needs[numneeds].destroy = 1;
        (mob_quests + i)->needs[numneeds].participants = NULL;
        (mob_quests + i)->needs[numneeds].type = search_block(value, needs_list, FALSE);
        (mob_quests + i)->needs[numneeds].complete = 0;
        (mob_quests + i)->needs[numneeds].needs_complete_msg = NULL;
        (mob_quests + i)->needs[numneeds].need_more_msg = NULL;
      } else if (strcmp(field, "need_more_msg") == 0) {
        line[0] = '\0';
        (mob_quests + i)->needs[numneeds].need_more_msg = fread_string(quest_file, line);
      } else if (strcmp(field, "needs_complete_msg") == 0) {
        line[0] = '\0';
        (mob_quests + i)->needs[numneeds].needs_complete_msg = fread_string(quest_file, line);
      } else {
        safe_snprintf(buf2, MAX_STRING_LENGTH, "Unknown Quest field [%s]", field);
        stderr_log(buf2);
      }
      break;
    case 'R':
      if (strcmp(field, "racelist") == 0) {
        (mob_quests + i)->racelist = asciiflag_conv(value);
      } else {
        safe_snprintf(buf2, MAX_STRING_LENGTH, "Unknown Quest field [%s]", field);
        stderr_log(buf2);
      }
      break;
    case 'S':
      if (strcmp(field, "skillname") == 0) {
        (mob_quests + i)->skillname = strdup(value);
      } else {
        safe_snprintf(buf2, MAX_STRING_LENGTH, "Unknown Quest field [%s]", field);
        stderr_log(buf2);
      }
      break;
    case 'V':
      if (strcmp(field, "value") == 0) {
        (mob_quests + i)->value = numval;
      } else if (strcmp(field, "vnum") == 0) {
        (mob_quests + i)->needs[numneeds].vnum = numval;
      } else {
        safe_snprintf(buf2, MAX_STRING_LENGTH, "Unknown Quest field [%s]", field);
        stderr_log(buf2);
      }
      break;
    default:
      safe_snprintf(buf2, MAX_STRING_LENGTH, "Unknown Quest field [%s]", field);
      stderr_log(buf2);
      break;
    }
  }
  i++;
}

void add_participant(struct char_data *ch, struct quest_data *quest, int i) {
  struct quest_participant_data *p;
  if (!ch || !quest) {
    return;
  }
  CREATE(p, struct quest_participant_data, 1);
  p->id_num = GET_IDNUM(ch);
  p->given = 0;
  p->next = quest->needs[i].participants;
  quest->needs[i].participants = p;
}

struct quest_participant_data *find_participant(struct char_data *ch, struct quest_data *quest, int i) {
  struct quest_participant_data *p;
  struct quest_participant_data *next_p;

  for (p = quest->needs[i].participants; p; p = next_p) {
    next_p = p->next;
    if (p->id_num == GET_IDNUM(ch)) {
      return p;
    }
  }

  return NULL;
}

int check_quest_status(struct char_data *me, struct char_data *player, struct quest_data *quest) {
  char qbuf[32768];

  if (IS_SET(quest->flags, QUEST_ONCEPLAYER)) {
    if (QCOMPLETED(player, quest->qnum) & (1 << (quest->qnum % 32))) {
      act("$n says, 'You have already completed this quest before.'", TRUE, me, 0, player, TO_VICT);
      return 0;
    }
  }

  if (quest->maxlevel < GET_LEVEL(player)) {
    safe_snprintf(
        qbuf, sizeof(qbuf),
        "$n says, 'Aren't you a bit advanced for this quest?  You must be of level %d or less to participate.'",
        quest->maxlevel);
    act(qbuf, TRUE, me, 0, player, TO_VICT);
    return 0;
  }

  if (!(quest->classlist & (1 << GET_CLASS(player)))) {
    if (!(quest->classlist & (1 << NUM_CLASSES))) {
      act("$n says, 'I don't know what you are talking about.'", TRUE, me, 0, player, TO_VICT);
      return 0;
    }
  }

  if (!(quest->racelist & (1 << GET_RACE(player)))) {
    if (!(quest->racelist & (1 << NUM_RACES))) {
      act("$n says, 'I don't deal with your kind.'", TRUE, me, 0, player, TO_VICT);
      return 0;
    }
  }

  return 1;
}

int check_quest_completed(struct quest_data *quest, struct char_data *ch) {
  int i;
  int completed = 0;
  int skillnum;
  struct quest_participant_data *p;
  struct quest_participant_data *temp;
  struct obj_data *obj;

  if (!quest->maxneeds) {
    return 0;
  }
  for (i = 0; i < quest->maxneeds; i++) {
    if (IS_SET(quest->flags, QUEST_SINGLEPLAYER)) {
      p = find_participant(ch, quest, i);
    } else {
      p = quest->needs[i].participants;
    }
    if (p) {
      completed += p->complete;
    }
  }
  if (completed / quest->maxneeds) {
    if (IS_SET(quest->flags, QUEST_ONCEBOOT)) {
      SET_BIT(quest->flags, QUEST_COMPLETED);
    }
    if (IS_SET(quest->flags, QUEST_ONCEPLAYER)) {
      i = quest->qnum % 10;
      ch->player_specials->saved.questcompleted[i] |= (1 << (quest->qnum % 32));
    }
    for (i = 0; i < quest->maxneeds; i++) {
      if (IS_SET(quest->flags, QUEST_SINGLEPLAYER)) {
        p = find_participant(ch, quest, i);
      } else {
        p = quest->needs[i].participants;
      }
      if (p) {
        REMOVE_FROM_LIST(p, quest->needs[i].participants, next);
      }
      FREE(p);
      p = NULL;
    }
    if (quest->knowledge) {
      send_to_char(quest->knowledge, ch);
    }
    switch (quest->goal) {
    case GOAL_OBJECT:
      if (quest->value) {
        obj = read_object(quest->value, VIRTUAL);
        if (obj) {
          safe_snprintf(logbuffer, sizeof(logbuffer), "%s completed quest #%d, received %s (%d) as reward.",
                        GET_NAME(ch), quest->qnum,
                        (obj->cshort_description ? obj->cshort_description
                                                 : (obj->short_description ? obj->short_description : "something")),
                        quest->value);
          mudlog(logbuffer, 'Q', COM_IMMORT, TRUE);
          obj_to_char(obj, ch);
        }
      }
      break;
    case GOAL_EXPERIENCE:
      if (quest->value) {
        safe_snprintf(logbuffer, sizeof(logbuffer), "%s completed quest #%d, received %d exp as reward.", GET_NAME(ch),
                      quest->qnum, quest->value);
        mudlog(logbuffer, 'Q', COM_IMMORT, TRUE);
        gain_exp(ch, quest->value);
      }
      break;
    case GOAL_SKILL:
      skillnum = find_skill_num(quest->skillname);
      if (skillnum < 0) {
        skillnum = find_spell_num(quest->skillname);
      }
      if (skillnum >= 0) {
        safe_snprintf(logbuffer, sizeof(logbuffer), "%s completed quest #%d, received %s skill as reward.",
                      GET_NAME(ch), quest->qnum, quest->skillname);
        mudlog(logbuffer, 'Q', COM_IMMORT, TRUE);
        SET_SKILL(ch, spells[skillnum].spellindex, 15);
      }
      break;
    }
    return 1;
  }
  return 0;
}

int give_quest_item(struct quest_data *quest, int type, struct obj_data *obj, int amount, struct char_data *ch,
                    int worktogether) {
  struct quest_participant_data *p;
  int i;
  int ok = 0;
  char tbuf[1024];

  for (i = 0; i < quest->maxneeds; i++) {
    if (quest->needs[i].type == type && (amount || (obj && quest->needs[i].vnum == GET_OBJ_VNUM(obj)))) {
      p = quest->needs[i].participants;
      while (p) {
        if (p->id_num == GET_IDNUM(ch)) {
          break;
        } else {
          p = p->next;
        }
      }
      if (!p) {
        if (!worktogether) {
          add_participant(ch, quest, i);
        } else {
          if (!quest->needs[i].participants) {
            add_participant(ch, quest, i);
          }
        }
        p = quest->needs[i].participants;
      }
      if (!IS_SET(quest->flags, QUEST_INORDER) || ok || i == 0) {
        if (amount) {
          p->given += amount;
        } else {
          p->given++;
        }
        if (quest->needs[i].amount <= p->given) {
          p->given = quest->needs[i].amount;
          p->complete = 1;
          if (quest->needs[i].needs_complete_msg) {
            send_to_char(quest->needs[i].needs_complete_msg, ch);
          }
        } else {
          if (quest->needs[i].need_more_msg) {
            if (amount) {
              safe_snprintf(tbuf, sizeof(tbuf), quest->needs[i].need_more_msg,
                            make_money_text(quest->needs[i].amount - p->given));
            } else {
              safe_snprintf(tbuf, sizeof(tbuf), quest->needs[i].need_more_msg, quest->needs[i].amount - p->given);
            }
            send_to_char(quest->needs[i].need_more_msg, ch);
          }
        }
      } else {
      }
      return i;
    } else {
      ok = 0;
      if (IS_SET(quest->flags, QUEST_INORDER)) {
        p = quest->needs[i].participants;
        while (p) {
          if (p->id_num == GET_IDNUM(ch)) {
            break;
          } else {
            p = p->next;
          }
        }
        if (p && (quest->needs[i].complete || p->complete || i == 0)) {
          ok = 1;
        } else {
        }
      } else {
        ok = 1;
      }
    }
  }
  return -1;
}

int quest_bribe_trigger(struct char_data *me, struct char_data *player, int amount) {
  int temp;
  int questnum;
  int i;
  int flag = 0;

  if (IS_NPC(player) || !IS_NPC(me)) {
    return 0;
  } else {
    questnum = GET_MOB_QUEST_NUM(me);
  }

  if (questnum == 0 || questnum == -1) {
    return 0;
  }

  if (!check_quest_status(me, player, (mob_quests + questnum))) {
    return 0;
  }

  /* one person per boot please */
  if (IS_SET(QFLAGS(questnum), QUEST_SINGLEPLAYER) && !IS_SET(QFLAGS(questnum), QUEST_MULTIPLAYER)) {
    if (mob_quests[questnum].maxneeds) {
      if (IS_SET(QFLAGS(questnum), QUEST_INORDER)) {
        if (mob_quests[questnum].needs[0].participants) {
          if (mob_quests[questnum].needs[0].participants->id_num != GET_IDNUM(player)) {
            act("$n says, 'The quest is already being attempted by somebody else.'", TRUE, me, 0, player, TO_VICT);
            return 0;
          }
        } else {
          add_participant(player, (mob_quests + questnum), 0);
        }
        if ((i = give_quest_item((mob_quests + questnum), QUEST_MONEY, NULL, amount, player, FALSE)) != -1) {
          if (mob_quests[questnum].needs[i].destroy) {
            me->points.temp_gold -= amount;
            temp = me->points.temp_gold;
            me->points.plat = temp / 1000;
            temp -= me->points.plat * 1000;
            me->points.gold = temp / 100;
            temp -= me->points.gold * 100;
            me->points.silver = temp / 10;
            temp -= me->points.silver * 10;
            me->points.copper = temp;
          }
          flag = 1;
        }
      } else { /* any order */
        for (i = 0; i < mob_quests[questnum].maxneeds; i++) {
          if (mob_quests[questnum].needs[i].participants) {
            if (mob_quests[questnum].needs[i].participants->id_num != GET_IDNUM(player)) {
              act("$n says, 'The quest is already being attempted by somebody else.'", TRUE, me, 0, player, TO_VICT);
              return 0;
            }
          }
        }
        if ((i = give_quest_item((mob_quests + questnum), QUEST_MONEY, NULL, amount, player, FALSE)) != -1) {
          if (mob_quests[questnum].needs[i].destroy) {
            me->points.temp_gold -= amount;
            temp = me->points.temp_gold;
            me->points.plat = temp / 1000;
            temp -= me->points.plat * 1000;
            me->points.gold = temp / 100;
            temp -= me->points.gold * 100;
            me->points.silver = temp / 10;
            temp -= me->points.silver * 10;
            me->points.copper = temp;
          }
          flag = 1;
        }
      }
    } else {
      act("$n says, 'I am in need of nothing at this time.'", TRUE, me, 0, player, TO_VICT);
      return 0;
    }
  } else if (IS_SET(QFLAGS(questnum), QUEST_SINGLEPLAYER) && IS_SET(QFLAGS(questnum), QUEST_MULTIPLAYER)) {
    /* multi player, each works alone */
    if (mob_quests[questnum].maxneeds) {
      if ((i = give_quest_item((mob_quests + questnum), QUEST_MONEY, NULL, amount, player, FALSE)) != -1) {
        if (mob_quests[questnum].needs[i].destroy) {
          me->points.temp_gold -= amount;
          temp = me->points.temp_gold;
          me->points.plat = temp / 1000;
          temp -= me->points.plat * 1000;
          me->points.gold = temp / 100;
          temp -= me->points.gold * 100;
          me->points.silver = temp / 10;
          temp -= me->points.silver * 10;
          me->points.copper = temp;
        }
        flag = 1;
      }
    } else {
      act("$n says, 'I am in need of nothing at this time.'", TRUE, me, 0, player, TO_VICT);
      return 0;
    }
  } else {
    /* multi player, working together */
    if (mob_quests[questnum].maxneeds) {
      if ((i = give_quest_item((mob_quests + questnum), QUEST_MONEY, NULL, amount, player, TRUE)) != -1) {
        if (mob_quests[questnum].needs[i].destroy) {
          me->points.temp_gold -= amount;
          temp = me->points.temp_gold;
          me->points.plat = temp / 1000;
          temp -= me->points.plat * 1000;
          me->points.gold = temp / 100;
          temp -= me->points.gold * 100;
          me->points.silver = temp / 10;
          temp -= me->points.silver * 10;
          me->points.copper = temp;
        }
        flag = 1;
      }
    } else {
      act("$n says, 'I am in need of nothing at this time.'", TRUE, me, 0, player, TO_VICT);
      return 0;
    }
  }
  check_quest_completed((mob_quests + questnum), player);
  return flag;
}

int quest_give_trigger(struct char_data *me, struct char_data *player, struct obj_data *obj) {
  int questnum;
  int i;
  int flag = 0;

  if (IS_NPC(player) || !IS_NPC(me)) {
    return 0;
  } else {
    questnum = GET_MOB_QUEST_NUM(me);
  }

  if (questnum == 0 || questnum == -1) {
    return 0;
  }

  if (!check_quest_status(me, player, (mob_quests + questnum))) {
    return 0;
  }

  /* one person per boot please */
  if (IS_SET(QFLAGS(questnum), QUEST_SINGLEPLAYER) && !IS_SET(QFLAGS(questnum), QUEST_MULTIPLAYER)) {
    if (mob_quests[questnum].maxneeds) {
      if (IS_SET(QFLAGS(questnum), QUEST_INORDER)) {
        if (mob_quests[questnum].needs[0].participants) {
          if (mob_quests[questnum].needs[0].participants->id_num != GET_IDNUM(player)) {
            act("$n says, 'The quest is already being attempted by somebody else.'", TRUE, me, 0, player, TO_VICT);
            return 0;
          }
        } else {
          add_participant(player, (mob_quests + questnum), 0);
        }
        if ((i = give_quest_item((mob_quests + questnum), QUEST_OBJECT, obj, 0, player, FALSE)) != -1) {
          if (mob_quests[questnum].needs[i].destroy) {
            obj_from_char(obj);
            extract_obj(obj);
          }
          flag = 1;
        }
      } else { /* any order */
        for (i = 0; i < mob_quests[questnum].maxneeds; i++) {
          if (mob_quests[questnum].needs[i].participants) {
            if (mob_quests[questnum].needs[i].participants->id_num != GET_IDNUM(player)) {
              act("$n says, 'The quest is already being attempted by somebody else.'", TRUE, me, 0, player, TO_VICT);
              return 0;
            }
          }
        }
        if ((i = give_quest_item((mob_quests + questnum), QUEST_OBJECT, obj, 0, player, FALSE)) != -1) {
          if (mob_quests[questnum].needs[i].destroy) {
            obj_from_char(obj);
            extract_obj(obj);
          }
          flag = 1;
        }
      }
    } else {
      act("$n says, 'I am in need of nothing at this time.'", TRUE, me, 0, player, TO_VICT);
      return 0;
    }
  } else if (IS_SET(QFLAGS(questnum), QUEST_SINGLEPLAYER) && IS_SET(QFLAGS(questnum), QUEST_MULTIPLAYER)) {
    /* multi player, each works alone */
    if (mob_quests[questnum].maxneeds) {
      if ((i = give_quest_item((mob_quests + questnum), QUEST_OBJECT, obj, 0, player, FALSE)) != -1) {
        if (mob_quests[questnum].needs[i].destroy) {
          obj_from_char(obj);
          extract_obj(obj);
        }
        flag = 1;
      }
    } else {
      act("$n says, 'I am in need of nothing at this time.'", TRUE, me, 0, player, TO_VICT);
      return 0;
    }
  } else {
    /* multi player, working together */
    if (mob_quests[questnum].maxneeds) {
      if ((i = give_quest_item((mob_quests + questnum), QUEST_OBJECT, obj, 0, player, TRUE)) != -1) {
        if (mob_quests[questnum].needs[i].destroy) {
          obj_from_char(obj);
          extract_obj(obj);
        }
        flag = 1;
      }
    } else {
      act("$n says, 'I am in need of nothing at this time.'", TRUE, me, 0, player, TO_VICT);
      return 0;
    }
  }
  check_quest_completed((mob_quests + questnum), player);
  return flag;
}

/* buf2 is the question, ch is asker, vict is mob */
int quest_ask_trigger(char *buf2, struct char_data *ch, struct char_data *vict) {
  int nummsgs = 0;
  int numneeds = 0;
  int questnum = 0;
  int i;
  char qbuf[32768];
  struct quest_participant_data *p;

  if (IS_NPC(ch) || !IS_NPC(vict)) {
    return 0;
  } else {
    questnum = GET_MOB_QUEST_NUM(vict);
  }

  if (questnum == 0 || questnum == -1) {
    return 0;
  }

  /* check to see if player is allowed to participate */
  if (!check_quest_status(vict, ch, (mob_quests + questnum))) {
    return 0;
  }

  if (strcasecmp(buf2, "status") == 0 || strcasecmp(buf2, "quest") == 0) {
    /* only happens if quest is once per boot */
    if (IS_SET(QFLAGS(questnum), QUEST_COMPLETED)) {
      act("$n says, 'The quest is closed.'", TRUE, vict, 0, ch, TO_VICT);
      return 0;
    }
    if (IS_SET(QFLAGS(questnum), QUEST_SINGLEPLAYER)) {
      if (mob_quests[questnum].maxneeds) {
        /* only 1 person at a time */
        if (IS_SET(QFLAGS(questnum), QUEST_SINGLEPLAYER) && !IS_SET(QFLAGS(questnum), QUEST_MULTIPLAYER)) {
          if (mob_quests[questnum].needs[numneeds].participants) {
            if (mob_quests[questnum].needs[numneeds].participants->id_num != GET_IDNUM(ch)) {
              act("$n says, 'The quest is already being attempted by somebody else.'", TRUE, vict, 0, ch, TO_VICT);
              return 0;
            }
          }
        }
        safe_snprintf(qbuf, sizeof(qbuf), "%s says, 'I am in need of the following:'\r\n", CAP(GET_MOB_NAME(vict)));
        for (i = 0; i < mob_quests[questnum].maxneeds; i++) {
          if (mob_quests[questnum].needs[numneeds].participants) {
            switch (mob_quests[questnum].needs[i].type) {
            case QUEST_MONEY:
              if (!mob_quests[questnum].needs[i].complete) {
                p = find_participant(ch, mob_quests + questnum, i);
                if (!p) {
                  safe_snprintf(qbuf + strlen(qbuf), sizeof(qbuf) - strlen(qbuf), "%s\r\n",
                                make_money_text(mob_quests[questnum].needs[i].amount));
                } else {
                  if (!p->complete) {
                    safe_snprintf(qbuf + strlen(qbuf), sizeof(qbuf) - strlen(qbuf), "%s\r\n",
                                  make_money_text(mob_quests[questnum].needs[i].amount - p->given));
                  }
                }
              }
              break;
            case QUEST_OBJECT:
              if (!mob_quests[questnum].needs[i].complete) {
                p = find_participant(ch, mob_quests + questnum, i);
                if (!p) {
                  safe_snprintf(qbuf + strlen(qbuf), sizeof(qbuf) - strlen(qbuf), "(%d) %s\r\n",
                                mob_quests[questnum].needs[i].amount,
                                OBJS((obj_proto + real_object(mob_quests[questnum].needs[i].vnum)), ch));
                } else {
                  if (!p->complete) {
                    safe_snprintf(qbuf + strlen(qbuf), sizeof(qbuf) - strlen(qbuf), "(%d) %s\r\n",
                                  mob_quests[questnum].needs[i].amount - p->given,
                                  OBJS((obj_proto + real_object(mob_quests[questnum].needs[i].vnum)), ch));
                  }
                }
              }
              break;
            }
          } else {
            switch (mob_quests[questnum].needs[i].type) {
            case QUEST_MONEY:
              if (!mob_quests[questnum].needs[i].complete) {
                safe_snprintf(qbuf + strlen(qbuf), sizeof(qbuf) - strlen(qbuf), "%s\r\n",
                              make_money_text(mob_quests[questnum].needs[i].amount));
              }
              break;
            case QUEST_OBJECT:
              if (!mob_quests[questnum].needs[i].complete) {
                safe_snprintf(qbuf + strlen(qbuf), sizeof(qbuf) - strlen(qbuf), "(%d) %s\r\n",
                              mob_quests[questnum].needs[i].amount,
                              OBJS((obj_proto + real_object(mob_quests[questnum].needs[i].vnum)), ch));
              }
              break;
            }
          }
        }
        send_to_char(qbuf, ch);
        return 1;
      } else {
        act("$n says, 'I am in need of nothing at this time.'", TRUE, vict, 0, ch, TO_VICT);
        return 0;
      }
      /* multiple people working together */
    } else if (IS_SET(QFLAGS(questnum), QUEST_MULTIPLAYER)) {
      if (mob_quests[questnum].maxneeds) {
        /* just show what is needed at this time */
        safe_snprintf(qbuf, sizeof(qbuf), "%s says, 'I am in need of the following:'\r\n", CAP(GET_MOB_NAME(vict)));
        for (i = 0; i < mob_quests[questnum].maxneeds; i++) {
          if (mob_quests[questnum].needs[i].participants) {
            switch (mob_quests[questnum].needs[i].type) {
            case QUEST_MONEY:
              if (!mob_quests[questnum].needs[i].complete) {
                safe_snprintf(qbuf + strlen(qbuf), sizeof(qbuf) - strlen(qbuf), "%s\r\n",
                              make_money_text(mob_quests[questnum].needs[i].amount -
                                              mob_quests[questnum].needs[i].participants->given));
              }
              break;
            case QUEST_OBJECT:
              if (!mob_quests[questnum].needs[i].complete) {
                safe_snprintf(qbuf + strlen(qbuf), sizeof(qbuf) - strlen(qbuf), "(%d) %s\r\n",
                              mob_quests[questnum].needs[i].amount - mob_quests[questnum].needs[i].participants->given,
                              OBJS((obj_proto + real_object(mob_quests[questnum].needs[i].vnum)), ch));
              }
              break;
            }
          } else {
            switch (mob_quests[questnum].needs[i].type) {
            case QUEST_MONEY:
              if (!mob_quests[questnum].needs[i].complete) {
                safe_snprintf(qbuf + strlen(qbuf), sizeof(qbuf) - strlen(qbuf), "%s\r\n",
                              make_money_text(mob_quests[questnum].needs[i].amount));
              }
              break;
            case QUEST_OBJECT:
              if (!mob_quests[questnum].needs[i].complete) {
                safe_snprintf(qbuf + strlen(qbuf), sizeof(qbuf) - strlen(qbuf), "(%d) %s\r\n",
                              mob_quests[questnum].needs[i].amount,
                              OBJS((obj_proto + real_object(mob_quests[questnum].needs[i].vnum)), ch));
              }
              break;
            }
          }
        }
        send_to_char(qbuf, ch);
        return 1;
      } else {
        act("$n says, 'I am in need of nothing at this time.'", TRUE, vict, 0, ch, TO_VICT);
        return 0;
      }
    }
  }

  for (nummsgs = 0; nummsgs < mob_quests[questnum].maxmsgs; nummsgs++) {
    if (strstr(mob_quests[questnum].messages[nummsgs].keywords, buf2)) {
      break;
    }
  }

  if (nummsgs != mob_quests[questnum].maxmsgs) {
    page_string(ch->desc, mob_quests[questnum].messages[nummsgs].message, 1);
    return 1;
  }
  return 0;
}
