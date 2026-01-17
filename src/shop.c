/* ************************************************************************
 *   File: shop.c                                        Part of CircleMUD *
 *  Usage: shopkeepers: loading config files, spec procs.                  *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

/***
 * The entire shop rewrite for Circle 3.0 was done by Jeff Fink.  Thanks Jeff!
 ***/

#define __SHOP_C__

#include "shop.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "structs.h"
#include "utils.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* External variables */
extern struct index_data *mob_index;
extern struct index_data *obj_index;
extern struct char_data *mob_proto;
extern struct obj_data *obj_proto;
extern struct room_data *world;
extern struct time_info_data time_info;
extern int top_of_mobt;
extern char *item_types[];
extern char *extra_bits[];

/* Forward/External function declarations */
ACMD(do_tell);
ACMD(do_action);
ACMD(do_echo);
ACMD(do_say);
char *fname(char *namelist);
void sort_keeper_objs(struct char_data *keeper, int shop_nr);

/* Local variables */
struct shop_data *shop_index;
int top_shop = 0;
int cmd_say, cmd_tell, cmd_emote, cmd_slap, cmd_puke, cmd_push;

const char *operator_str[] = {"[({", "])}", "|+", "&*", "^'"};

int is_ok_char(struct char_data *keeper, struct char_data *ch, int shop_nr) {
  char buf[200];

  if (!(CAN_SEE(keeper, ch))) {
    do_say(keeper, MSG_NO_SEE_CHAR, cmd_say, 0);
    return (FALSE);
  }
  if (IS_GOD(ch))
    return (TRUE);

  if ((IS_GOOD(ch) && NOTRADE_GOOD(shop_nr)) || (IS_EVIL(ch) && NOTRADE_EVIL(shop_nr)) ||
      (IS_NEUTRAL(ch) && NOTRADE_NEUTRAL(shop_nr))) {
    safe_snprintf(buf, sizeof(buf), "%s %s", GET_NAME(ch), MSG_NO_SELL_ALIGN);
    do_tell(keeper, buf, cmd_tell, 0);
    return (FALSE);
  }
  if (IS_NPC(ch))
    return (TRUE);

  if ((IS_WIZARD(ch) && NOTRADE_WIZARD(shop_nr)) || (IS_CLERIC(ch) && NOTRADE_CLERIC(shop_nr)) ||
      (IS_THIEF(ch) && NOTRADE_THIEF(shop_nr)) || (IS_WARRIOR(ch) && NOTRADE_WARRIOR(shop_nr))) {
    safe_snprintf(buf, sizeof(buf), "%s %s", GET_NAME(ch), MSG_NO_SELL_CLASS);
    do_tell(keeper, buf, cmd_tell, 0);
    return (FALSE);
  }
  return (TRUE);
}

int is_open(struct char_data *keeper, int shop_nr, int msg) {
  char buf[200];

  *buf = 0;
  if (SHOP_OPEN1(shop_nr) > time_info.hours) {
    safe_snprintf(buf, sizeof(buf), "%s", MSG_NOT_OPEN_YET);
  } else if (SHOP_CLOSE1(shop_nr) < time_info.hours) {
    if (SHOP_OPEN2(shop_nr) > time_info.hours) {
      safe_snprintf(buf, sizeof(buf), "%s", MSG_NOT_REOPEN_YET);
    } else if (SHOP_CLOSE2(shop_nr) < time_info.hours) {
      safe_snprintf(buf, sizeof(buf), "%s", MSG_CLOSED_FOR_DAY);
    }
  }

  if (!(*buf)) {
    return (TRUE);
  }
  if (msg) {
    do_say(keeper, buf, cmd_tell, 0);
  }
  return (FALSE);
}

int is_ok(struct char_data *keeper, struct char_data *ch, int shop_nr) {
  if ((is_open(keeper, shop_nr, TRUE)) || IS_IMMO(ch))
    return (is_ok_char(keeper, ch, shop_nr));
  else
    return (FALSE);
}

void push(struct stack_data *stack, int pushval) {
  S_DATA(stack, S_LEN(stack)++) = pushval;
}

int top(struct stack_data *stack) {
  if (S_LEN(stack) > 0)
    return (S_DATA(stack, S_LEN(stack) - 1));
  else
    return (NOTHING);
}

int pop(struct stack_data *stack) {
  if (S_LEN(stack) > 0)
    return (S_DATA(stack, --S_LEN(stack)));
  else {
    stderr_log("Illegal expression in shop keyword list");
    return (0);
  }
}

void evaluate_operation(struct stack_data *ops, struct stack_data *vals) {
  int oper;

  if ((oper = pop(ops)) == OPER_NOT)
    push(vals, !pop(vals));
  else if (oper == OPER_AND)
    push(vals, pop(vals) && pop(vals));
  else if (oper == OPER_OR)
    push(vals, pop(vals) || pop(vals));
}

int find_oper_num(char token) {
  int index;

  for (index = 0; index <= MAX_OPER; index++)
    if (strchr(operator_str[index], token))
      return (index);
  return (NOTHING);
}

int evaluate_expression(struct obj_data *obj, char *expr) {
  struct stack_data ops, vals;
  char *ptr, *end, name[200];
  int temp, index;

  if (!expr)
    return (TRUE);

  ops.len = vals.len = 0;
  ptr = expr;
  while (*ptr) {
    if (isspace(*ptr))
      ptr++;
    else {
      if ((temp = find_oper_num(*ptr)) == NOTHING) {
        end = ptr;
        while (*ptr && !isspace(*ptr) && (find_oper_num(*ptr) == NOTHING))
          ptr++;
        strncpy(name, end, ptr - end);
        name[ptr - end] = 0;
        for (index = 0; *extra_bits[index] != '\n'; index++)
          if (!str_cmp(name, extra_bits[index])) {
            push(&vals, IS_SET(GET_OBJ_EXTRA(obj), 1 << index));
            break;
          }
        if (*extra_bits[index] == '\n')
          push(&vals, isname(name, obj->name));
      } else {
        if (temp != OPER_OPEN_PAREN)
          while (top(&ops) > temp)
            evaluate_operation(&ops, &vals);

        if (temp == OPER_CLOSE_PAREN) {
          if ((temp = pop(&ops)) != OPER_OPEN_PAREN) {
            stderr_log("Illegal parenthesis in shop keyword expression");
            return (FALSE);
          }
        } else
          push(&ops, temp);
        ptr++;
      }
    }
  }
  while (top(&ops) != NOTHING)
    evaluate_operation(&ops, &vals);
  temp = pop(&vals);
  if (top(&vals) != NOTHING) {
    stderr_log("Extra operands left on shop keyword expression stack");
    return (FALSE);
  }
  return (temp);
}

int trade_with(struct obj_data *item, int shop_nr) {
  int counter;

  if (GET_OBJ_COST(item) < 1)
    return (OBJECT_NOTOK);

  if (IS_OBJ_STAT(item, ITEM_NOSELL))
    return (OBJECT_NOTOK);
  if (IS_OBJ_STAT(item, ITEM_DONATED))
    return (OBJECT_DONATED);

  for (counter = 0; SHOP_BUYTYPE(shop_nr, counter) != NOTHING; counter++) {
    if (SHOP_BUYTYPE(shop_nr, counter) == GET_OBJ_TYPE(item)) {
      if ((GET_OBJ_VAL(item, 2) == 0) && ((GET_OBJ_TYPE(item) == ITEM_WAND) || (GET_OBJ_TYPE(item) == ITEM_STAFF))) {
        return (OBJECT_DEAD);
      } else if (evaluate_expression(item, SHOP_BUYWORD(shop_nr, counter))) {
        return (OBJECT_OK);
      }
    }
  }

  return (OBJECT_NOTOK);
}

int same_obj(struct obj_data *obj1, struct obj_data *obj2) {
  int index;

  if (!obj1 || !obj2)
    return (obj1 == obj2);

  if (GET_OBJ_RNUM(obj1) != GET_OBJ_RNUM(obj2))
    return (FALSE);

  if (GET_OBJ_COST(obj1) != GET_OBJ_COST(obj2))
    return (FALSE);

  if (GET_OBJ_EXTRA(obj1) != GET_OBJ_EXTRA(obj2))
    return (FALSE);

  for (index = 0; index < MAX_OBJ_AFFECT; index++)
    if ((obj1->affected[index].location != obj2->affected[index].location) ||
        (obj1->affected[index].modifier != obj2->affected[index].modifier))
      return (FALSE);

  return (TRUE);
}

int shop_producing(struct obj_data *item, int shop_nr) {
  int counter;

  if (GET_OBJ_RNUM(item) < 0)
    return (FALSE);

  for (counter = 0; SHOP_PRODUCT(shop_nr, counter) != NOTHING; counter++)
    if (same_obj(item, &obj_proto[SHOP_PRODUCT(shop_nr, counter)]))
      return (TRUE);
  return (FALSE);
}

int transaction_amt(char *arg) {
  int num;

  one_argument(arg, buf);
  if (*buf)
    if ((is_number(buf))) {
      num = atoi(buf);
      size_t offset = strlen(buf) + 1;
      memmove(arg, arg + offset, strlen(arg + offset) + 1);
      return (num);
    }
  return (1);
}

char *times_message(struct obj_data *obj, char *name, int num) {
  static char buf[256];
  char *ptr;
  size_t len;

  if (obj)
    len = safe_snprintf(buf, sizeof(buf), "%s", obj->short_description);
  else {
    if ((ptr = strchr(name, '.')) == NULL)
      ptr = name;
    else
      ptr++;
    len = safe_snprintf(buf, sizeof(buf), "%s %s", AN(ptr), ptr);
  }

  if (num > 1)
    safe_snprintf(buf + len, sizeof(buf) - len, " (x %d)", num);
  return (buf);
}

struct obj_data *get_slide_obj_vis(struct char_data *ch, char *name, struct obj_data *list) {
  struct obj_data *i, *last_match = 0;
  int j, number;
  char tmpname[MAX_INPUT_LENGTH];
  char *tmp;

  safe_snprintf(tmpname, sizeof(tmpname), "%s", name);
  tmp = tmpname;
  if (!(number = get_number(&tmp)))
    return (0);

  for (i = list, j = 1; i && (j <= number); i = i->next_content)
    if (isname(tmp, i->name))
      if (CAN_SEE_OBJ(ch, i) && !same_obj(last_match, i)) {
        if (j == number)
          return (i);
        last_match = i;
        j++;
      }
  return (0);
}

struct obj_data *get_hash_obj_vis(struct char_data *ch, char *name, struct obj_data *list) {
  struct obj_data *loop, *last_obj = 0;
  int index;

  if ((is_number(name + 1)))
    index = atoi(name + 1);
  else
    return (0);

  for (loop = list; loop; loop = loop->next_content)
    if (CAN_SEE_OBJ(ch, loop) && (loop->obj_flags.cost > 0))
      if (!same_obj(last_obj, loop)) {
        if (--index == 0)
          return (loop);
        last_obj = loop;
      }
  return (0);
}

struct obj_data *get_purchase_obj(struct char_data *ch, char *arg, struct char_data *keeper, int shop_nr, int msg) {
  char buf[MAX_STRING_LENGTH], name[MAX_INPUT_LENGTH];
  struct obj_data *obj;

  one_argument(arg, name);
  do {
    if (*name == '#')
      obj = get_hash_obj_vis(ch, name, keeper->carrying);
    else
      obj = get_slide_obj_vis(ch, name, keeper->carrying);
    if (!obj) {
      if (msg) {
        safe_snprintf(buf, MAX_STRING_LENGTH, shop_index[shop_nr].no_such_item1, GET_NAME(ch));
        do_tell(keeper, buf, cmd_tell, 0);
      }
      return (0);
    }
    if (GET_OBJ_COST(obj) <= 0) {
      extract_obj(obj);
      obj = 0;
    }
  } while (!obj);
  return (obj);
}

int buy_price(struct obj_data *obj, int shop_nr) {
  return ((int)(GET_OBJ_COST(obj) * SHOP_BUYPROFIT(shop_nr)));
}

void shopping_buy(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr) {
  char tempstr[200], buf[MAX_STRING_LENGTH];
  struct obj_data *obj = NULL, *last_obj = NULL;
  int goldamt = 0, buynum, bought = 0;
  int plat = 0;
  int gold = 0;
  int silver = 0;
  int copper = 0;
  int temp;

  if (!(is_ok(keeper, ch, shop_nr)))
    return;

  if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
    sort_keeper_objs(keeper, shop_nr);

  if ((buynum = transaction_amt(arg)) < 0) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "%s A negative amount?  Try selling me something.", GET_NAME(ch));
    do_tell(keeper, buf, cmd_tell, 0);
    return;
  }
  if (!(*arg) || !(buynum)) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "%s What do you want to buy??", GET_NAME(ch));
    do_tell(keeper, buf, cmd_tell, 0);
    return;
  }
  if (!(obj = get_purchase_obj(ch, arg, keeper, shop_nr, TRUE)))
    return;

  temp = buy_price(obj, shop_nr);

  if ((buy_price(obj, shop_nr) > GET_TEMP_GOLD(ch)) && !IS_GOD(ch)) {
    safe_snprintf(buf, MAX_STRING_LENGTH, shop_index[shop_nr].missing_cash2, GET_NAME(ch));
    do_tell(keeper, buf, cmd_tell, 0);
    return;
  }
  if ((IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch))) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "%s: You can't carry any more items.\n\r", fname(obj->name));
    send_to_char(buf, ch);
    return;
  }
  /*
   if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch)) {
   safe_snprintf(buf, MAX_STRING_LENGTH, "%s: You can't carry that much weight.\n\r",
   fname(obj->name));
   send_to_char(buf, ch);
   return;
   }
   */
  while ((obj) && ((GET_TEMP_GOLD(ch) >= buy_price(obj, shop_nr)) || IS_GOD(ch)) &&
         (IS_CARRYING_N(ch) < CAN_CARRY_N(ch)) && (bought < buynum)
         /* && (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) <= CAN_CARRY_W(ch))*/) {
    bought++;
    /* Test if producing shop ! */
    if (shop_producing(obj, shop_nr))
      obj = read_object(GET_OBJ_RNUM(obj), REAL);
    else {
      obj_from_char(obj);
      SHOP_SORT(shop_nr)--;
    }
    obj_to_char(obj, ch);

    goldamt += buy_price(obj, shop_nr);
    GET_TEMP_GOLD(ch) -= buy_price(obj, shop_nr);

    last_obj = obj;
    obj = get_purchase_obj(ch, arg, keeper, shop_nr, FALSE);
    if (!same_obj(obj, last_obj))
      break;
  }

  if (bought < buynum) {
    if (!obj || !same_obj(last_obj, obj))
      safe_snprintf(buf, MAX_STRING_LENGTH, "%s I only have %d to sell you.", GET_NAME(ch), bought);
    else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
      safe_snprintf(buf, MAX_STRING_LENGTH, "%s You can only hold %d.", GET_NAME(ch), bought);
    else if (temp < (buynum * buy_price(obj, shop_nr)))
      safe_snprintf(buf, MAX_STRING_LENGTH, "%s You can only afford %d.", GET_NAME(ch), bought);
    /*
     else if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) > CAN_CARRY_W(ch))
     safe_snprintf(buf, MAX_STRING_LENGTH, "%s You can only carry %d.", GET_NAME(ch), bought);
     */
    else
      safe_snprintf(buf, MAX_STRING_LENGTH, "%s Something screwy only gave you %d.", GET_NAME(ch), bought);
    do_tell(keeper, buf, cmd_tell, 0);
  }

  safe_snprintf(tempstr, sizeof(tempstr), "%s", times_message(ch->carrying, 0, bought));
  safe_snprintf(buf, MAX_STRING_LENGTH, "$n buys %s.", tempstr);
  act(buf, FALSE, ch, obj, 0, TO_ROOM);

  temp = goldamt;
  plat = temp / 1000;
  temp -= plat * 1000;
  gold = temp / 100;
  temp -= gold * 100;
  silver = temp / 10;
  temp -= silver * 10;
  copper = temp;

  GET_PLAT(ch) -= plat;
  GET_GOLD(ch) -= gold;
  GET_SILVER(ch) -= silver;
  GET_COPPER(ch) -= copper;

  if (GET_COPPER(ch) < 0) {
    while (GET_SILVER(ch) > 0 && GET_COPPER(ch) < 0) {
      GET_SILVER(ch)--;
      GET_COPPER(ch) += 10;
    }
    while (GET_GOLD(ch) > 0 && GET_COPPER(ch) < 0) {
      GET_GOLD(ch)--;
      GET_COPPER(ch) += 100;
    }
    while (GET_PLAT(ch) > 0 && GET_COPPER(ch) < 0) {
      GET_PLAT(ch)--;
      GET_COPPER(ch) += 1000;
    }
  }

  if (GET_SILVER(ch) < 0) {
    while (GET_COPPER(ch) > 10 && GET_SILVER(ch) < 0) {
      GET_COPPER(ch) -= 10;
      GET_SILVER(ch)++;
    }
    while (GET_GOLD(ch) > 0 && GET_SILVER(ch) < 0) {
      GET_GOLD(ch)--;
      GET_SILVER(ch) += 10;
    }
    while (GET_PLAT(ch) > 0 && GET_SILVER(ch) < 0) {
      GET_PLAT(ch)--;
      GET_SILVER(ch) += 100;
    }
  }

  if (GET_GOLD(ch) < 0) {
    while (GET_COPPER(ch) > 100 && GET_GOLD(ch) < 0) {
      GET_COPPER(ch) -= 100;
      GET_GOLD(ch)++;
    }
    while (GET_SILVER(ch) > 10 && GET_GOLD(ch) < 0) {
      GET_SILVER(ch) -= 10;
      GET_GOLD(ch)++;
    }
    while (GET_PLAT(ch) > 0 && GET_GOLD(ch) < 0) {
      GET_PLAT(ch)--;
      GET_GOLD(ch) += 10;
    }
  }

  if (GET_PLAT(ch) < 0) {
    while (GET_COPPER(ch) > 1000 && GET_PLAT(ch) < 0) {
      GET_COPPER(ch) -= 1000;
      GET_PLAT(ch)++;
    }
    while (GET_SILVER(ch) > 100 && GET_PLAT(ch) < 0) {
      GET_SILVER(ch) -= 100;
      GET_PLAT(ch)++;
    }
    while (GET_GOLD(ch) > 10 && GET_PLAT(ch) < 0) {
      GET_GOLD(ch) -= 10;
      GET_PLAT(ch)++;
    }
  }

  safe_snprintf(buf, MAX_STRING_LENGTH, shop_index[shop_nr].message_buy, GET_NAME(ch), make_money_text(goldamt));
  do_tell(keeper, buf, cmd_tell, 0);
  safe_snprintf(buf, MAX_STRING_LENGTH, "You now have %s.\n\r", tempstr);
  send_to_char(buf, ch);
}

struct obj_data *get_selling_obj(struct char_data *ch, char *name, struct char_data *keeper, int shop_nr, int msg) {
  char buf[MAX_STRING_LENGTH];
  struct obj_data *obj;
  int result;

  if (!(obj = get_obj_in_list_vis(ch, name, ch->carrying))) {
    if (msg) {
      safe_snprintf(buf, MAX_STRING_LENGTH, shop_index[shop_nr].no_such_item2, GET_NAME(ch));
      do_tell(keeper, buf, cmd_tell, 0);
    }
    return (0);
  }
  if ((result = trade_with(obj, shop_nr)) == OBJECT_OK)
    return (obj);

  switch (result) {
  case OBJECT_NOTOK:
    safe_snprintf(buf, MAX_STRING_LENGTH, shop_index[shop_nr].do_not_buy, GET_NAME(ch));
    break;
  case OBJECT_DEAD:
    safe_snprintf(buf, MAX_STRING_LENGTH, "%s %s", GET_NAME(ch), MSG_NO_USED_WANDSTAFF);
    break;
  case OBJECT_DONATED:
    safe_snprintf(buf, MAX_STRING_LENGTH, "%s You shouldn't sell what has been given to you!", GET_NAME(ch));
    break;
  default:
    safe_snprintf(buf, MAX_STRING_LENGTH, "Illegal return value of %d from trade_with() (shop.c)", result);
    stderr_log(buf);
    safe_snprintf(buf, MAX_STRING_LENGTH, "%s An error has occurred.", GET_NAME(ch));
    break;
  }
  if (msg)
    do_tell(keeper, buf, cmd_tell, 0);
  return (0);
}

int sell_price(struct char_data *ch, struct obj_data *obj, int shop_nr) {
  return ((int)(GET_OBJ_COST(obj) * SHOP_SELLPROFIT(shop_nr)));
}

struct obj_data *slide_obj(struct obj_data *obj, struct char_data *keeper, int shop_nr)
/*
 This function is a slight hack!  To make sure that duplicate items are
 only listed once on the "list", this function groups "identical"
 objects together on the shopkeeper's inventory list.  The hack involves
 knowing how the list is put together, and manipulating the order of
 the objects on the list.  (But since most of DIKU is not encapsulated,
 and information hiding is almost never used, it isn't that big a deal) -JF
 */
{
  struct obj_data *loop;
  int temp;

  if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
    sort_keeper_objs(keeper, shop_nr);

  /* Extract the object if it is identical to one produced */
  if (shop_producing(obj, shop_nr)) {
    temp = GET_OBJ_RNUM(obj);
    extract_obj(obj);
    return (&obj_proto[temp]);
  }
  SHOP_SORT(shop_nr)++;
  loop = keeper->carrying;
  obj_to_char(obj, keeper);
  keeper->carrying = loop;
  while (loop) {
    if (same_obj(obj, loop)) {
      obj->next_content = loop->next_content;
      loop->next_content = obj;
      return (obj);
    }
    loop = loop->next_content;
  }
  keeper->carrying = obj;
  return (obj);
}

void sort_keeper_objs(struct char_data *keeper, int shop_nr) {
  struct obj_data *list = 0, *temp;

  while (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper)) {
    temp = keeper->carrying;
    obj_from_char(temp);
    temp->next_content = list;
    list = temp;
  }

  while (list) {
    temp = list;
    list = list->next_content;
    if ((shop_producing(temp, shop_nr)) && !(get_obj_in_list_num(GET_OBJ_RNUM(temp), keeper->carrying))) {
      obj_to_char(temp, keeper);
      SHOP_SORT(shop_nr)++;
    } else
      (void)slide_obj(temp, keeper, shop_nr);
  }
}

void shopping_sell(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr) {
  char tempstr[200], buf[MAX_STRING_LENGTH], name[200];
  struct obj_data *obj;
  int sellnum, sold = 0, goldamt = 0;
  int temp = 0;
  int plat = 0;
  int gold = 0;
  int silver = 0;
  int copper = 0;

  if (!(is_ok(keeper, ch, shop_nr)))
    return;

  if ((sellnum = transaction_amt(arg)) < 0) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "%s A negative amount?  Try buying something.", GET_NAME(ch));
    do_tell(keeper, buf, cmd_tell, 0);
    return;
  }
  if (!(*arg) || !(sellnum)) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "%s What do you want to sell??", GET_NAME(ch));
    do_tell(keeper, buf, cmd_tell, 0);
    return;
  }
  one_argument(arg, name);
  if (!(obj = get_selling_obj(ch, name, keeper, shop_nr, TRUE)))
    return;

  while ((obj) && (sold < sellnum)) {
    sold++;

    obj_from_char(obj);
    slide_obj(obj, keeper, shop_nr);

    goldamt += sell_price(ch, obj, shop_nr);
    obj = get_selling_obj(ch, name, keeper, shop_nr, FALSE);
  }

  if (sold < sellnum) {
    if (!obj)
      safe_snprintf(buf, MAX_STRING_LENGTH, "%s You only have %d of those.", GET_NAME(ch), sold);
    else
      safe_snprintf(buf, MAX_STRING_LENGTH, "%s Something really screwy made me buy %d.", GET_NAME(ch), sold);

    do_tell(keeper, buf, cmd_tell, 0);
  }
  temp = goldamt;
  plat = temp / 1000;
  temp -= (plat * 1000);
  gold = temp / 100;
  temp -= (gold * 100);
  silver = temp / 10;
  temp -= (silver * 10);
  copper = temp;
  GET_PLAT(ch) += plat;
  GET_TEMP_GOLD(ch) += plat * 1000;
  GET_GOLD(ch) += gold;
  GET_TEMP_GOLD(ch) += gold * 100;
  GET_SILVER(ch) += silver;
  GET_TEMP_GOLD(ch) += silver * 10;
  GET_COPPER(ch) += copper;
  GET_TEMP_GOLD(ch) += copper;
  safe_snprintf(tempstr, sizeof(tempstr), "%s", times_message(0, name, sold));
  safe_snprintf(buf, MAX_STRING_LENGTH, "$n sells %s.", tempstr);
  act(buf, FALSE, ch, obj, 0, TO_ROOM);

  safe_snprintf(buf, MAX_STRING_LENGTH, shop_index[shop_nr].message_sell, GET_NAME(ch), make_money_text(goldamt));
  do_tell(keeper, buf, cmd_tell, 0);
  safe_snprintf(buf, MAX_STRING_LENGTH, "The shopkeeper now has %s.\n\r", tempstr);
  send_to_char(buf, ch);
}

void shopping_value(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr) {
  char buf[MAX_STRING_LENGTH];
  struct obj_data *obj;
  char name[MAX_INPUT_LENGTH];
  int temp = 0;

  if (!(is_ok(keeper, ch, shop_nr)))
    return;

  if (!(*arg)) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "%s What do you want me to valuate??", GET_NAME(ch));
    do_tell(keeper, buf, cmd_tell, 0);
    return;
  }
  one_argument(arg, name);
  if (!(obj = get_selling_obj(ch, name, keeper, shop_nr, TRUE)))
    return;

  temp = sell_price(ch, obj, shop_nr);

  safe_snprintf(buf, MAX_STRING_LENGTH, "%s I'll give you %s for that!", GET_NAME(ch), make_money_text(temp));
  do_tell(keeper, buf, cmd_tell, 0);

  return;
}

char *list_object(struct obj_data *obj, int cnt, int index, int shop_nr) {
  int j;
  static char buf[256];
  char buf2[300], buf3[300], buf4[80];
  char *p;
  char *p2;
  int temp;
  int plat;
  int gold;
  int silver;
  int copper;
  int numspaces = 72;

  size_t blen;
  if (shop_producing(obj, shop_nr))
    safe_snprintf(buf2, sizeof(buf2), "Unlimited   ");
  else
    safe_snprintf(buf2, sizeof(buf2), "%5d       ", cnt);
  blen = safe_snprintf(buf, sizeof(buf), " %2d)  %s", index, buf2);

  numspaces -= strlen(buf);
  /* Compile object name and information */
  size_t b3len = safe_snprintf(buf3, sizeof(buf3), "%s", obj->short_description);

  if ((GET_OBJ_TYPE(obj) == ITEM_WAND) || (GET_OBJ_TYPE(obj) == ITEM_STAFF))
    if (GET_OBJ_VAL(obj, 2) < GET_OBJ_VAL(obj, 1))
      b3len += safe_snprintf(buf3 + b3len, sizeof(buf3) - b3len, " (partially used)");

  temp = buy_price(obj, shop_nr);
  plat = temp / 1000;
  temp -= plat * 1000;
  gold = temp / 100;
  temp -= gold * 100;
  silver = temp / 10;
  temp -= silver * 10;
  copper = temp;
  safe_snprintf(buf4, sizeof(buf4), " {c%d{Wp {c%d{Yg {c%d{ws {c%d{yc{x\n\r", plat, gold, silver, copper);

  numspaces -= strlen(buf4);
  p = buf3;
  while ((p2 = strchr(p, '{'))) {
    p = p2 + 1;
    numspaces += 2;
  }
  p = buf4;
  while ((p2 = strchr(p, '{'))) {
    p = p2 + 1;
    numspaces += 2;
  }
  for (j = strlen(buf3); j < numspaces; j++)
    buf3[j] = ' ';
  buf3[j] = '\0';
  blen += safe_snprintf(buf + blen, sizeof(buf) - blen, "%s", CAP(buf3));
  safe_snprintf(buf + blen, sizeof(buf) - blen, "%s", buf4);
  return (buf);
}

void shopping_list(char *arg, struct char_data *ch, struct char_data *keeper, int shop_nr) {
  char buf[MAX_STRING_LENGTH], name[200];
  struct obj_data *obj, *last_obj = 0;
  int cnt = 0, index = 0;

  if (!(is_ok(keeper, ch, shop_nr)))
    return;

  if (SHOP_SORT(shop_nr) < IS_CARRYING_N(keeper))
    sort_keeper_objs(keeper, shop_nr);

  one_argument(arg, name);
  size_t buflen =
      safe_snprintf(buf, sizeof(buf), " ##   Available   Item                                               Cost\n\r");
  buflen += safe_snprintf(buf + buflen, sizeof(buf) - buflen,
                          "-------------------------------------------------------------------------\n\r");
  if (keeper->carrying)
    for (obj = keeper->carrying; obj; obj = obj->next_content)
      if (CAN_SEE_OBJ(ch, obj) && (obj->obj_flags.cost > 0)) {
        if (!last_obj) {
          last_obj = obj;
          cnt = 1;
        } else if (same_obj(last_obj, obj))
          cnt++;
        else {
          index++;
          if (!(*name) || isname(name, last_obj->name))
            buflen +=
                safe_snprintf(buf + buflen, sizeof(buf) - buflen, "%s", list_object(last_obj, cnt, index, shop_nr));
          cnt = 1;
          last_obj = obj;
        }
      }
  index++;
  if (!last_obj)
    if (*name)
      safe_snprintf(buf, sizeof(buf), "Presently, none of those are for sale.\n\r");
    else
      safe_snprintf(buf, sizeof(buf), "Currently, there is nothing for sale.\n\r");
  else if (!(*name) || isname(name, last_obj->name))
    safe_snprintf(buf + buflen, sizeof(buf) - buflen, "%s", list_object(last_obj, cnt, index, shop_nr));

  page_string(ch->desc, buf, 1);
}

int ok_shop_room(int shop_nr, int room) {
  /*
   int index;

   for (index = 0; SHOP_ROOM(shop_nr, index) != NOWHERE; index++)
   if (SHOP_ROOM(shop_nr, index) == room)
   return (TRUE);
   */
  return (TRUE);
}

SPECIAL(shop_keeper) {
  char argm[MAX_INPUT_LENGTH];
  struct char_data *keeper = (struct char_data *)me;
  int shop_nr;

  for (shop_nr = 0; shop_nr < top_shop; shop_nr++)
    if (SHOP_KEEPER(shop_nr) == keeper->nr)
      break;

  if (shop_nr >= top_shop)
    return (FALSE);

  if (SHOP_FUNC(shop_nr)) /* Check secondary function */
    if ((SHOP_FUNC(shop_nr))(ch, me, cmd, arg, SPEC_STANDARD))
      return (TRUE);

  if (keeper == ch) {
    if (cmd)
      SHOP_SORT(shop_nr) = 0; /* Safety in case "drop all" */
    return (FALSE);
  }
  /*
   if (!ok_shop_room(shop_nr, world[ch->in_room].number))
   return (0);
   */

  if (!AWAKE(keeper))
    return (FALSE);

  if (CMD_IS("steal")) {
    safe_snprintf(argm, sizeof(argm), "$N shouts '%s'", MSG_NO_STEAL_HERE);
    do_action(keeper, GET_NAME(ch), cmd_slap, 0);
    act(argm, FALSE, ch, 0, keeper, TO_CHAR);
    return (TRUE);
  }
  /*
   if (CMD_IS("give")) {
   do_action(keeper, GET_NAME(ch), cmd_push, 0);
   return (TRUE);
   }
   */

  if (CMD_IS("buy")) {
    if (!IS_NPC(ch) && GET_LEVEL(ch) == LVL_IMMORT && !PLR_FLAGGED(ch, PLR_UNRESTRICT)) {
      send_to_char("No, there is no reason for you to buy that.\r\n", ch);
      return TRUE;
    }
    shopping_buy(argument, ch, keeper, shop_nr);
    return (TRUE);
  } else if (CMD_IS("sell")) {
    if (!IS_NPC(ch) && GET_LEVEL(ch) == LVL_IMMORT && !PLR_FLAGGED(ch, PLR_UNRESTRICT)) {
      send_to_char("No, there is no reason for you to sell that.\r\n", ch);
      return TRUE;
    }
    shopping_sell(argument, ch, keeper, shop_nr);
    return (TRUE);
  } else if (CMD_IS("value")) {
    if (!IS_NPC(ch) && GET_LEVEL(ch) == LVL_IMMORT && !PLR_FLAGGED(ch, PLR_UNRESTRICT)) {
      send_to_char("No, there is no reason for you to value that.\r\n", ch);
      return TRUE;
    }
    shopping_value(argument, ch, keeper, shop_nr);
    return (TRUE);
  } else if (CMD_IS("list")) {
    shopping_list(argument, ch, keeper, shop_nr);
    return (TRUE);
  }
  return (FALSE);
}

int ok_damage_shopkeeper(struct char_data *ch, struct char_data *victim) {
  /*  char buf[200];
   int index;

   if (IS_NPC(victim) && (mob_index[GET_MOB_RNUM(victim)].func == shop_keeper))
   for (index = 0; index < top_shop; index++)
   if ((GET_MOB_RNUM(victim) == SHOP_KEEPER(index)) && !SHOP_KILL_CHARS(index)) {
   do_action(victim, GET_NAME(ch), cmd_slap, 0);
   safe_snprintf(buf, MAX_STRING_LENGTH, "%s %s", GET_NAME(ch), MSG_CANT_KILL_KEEPER);
   do_tell(victim, buf, cmd_tell, 0);
   return (FALSE);
   } */
  return (TRUE);
}

int add_to_list(struct shop_buy_data *list, int type, int *len, int *val) {
  if (*val >= 0) {
    if (*len < MAX_SHOP_OBJ) {
      if (type == LIST_PRODUCE) {
        *val = real_object(*val);
      }
      if (*val >= 0) {
        BUY_TYPE(list[*len]) = *val;
        BUY_WORD(list[(*len)++]) = 0;
      } else {
        *val = 0;
      }
      return (FALSE);
    } else {
      return (TRUE);
    }
  }
  return (FALSE);
}

int end_read_list(struct shop_buy_data *list, int len, int error) {
  if (error) {
    safe_snprintf(buf, MAX_STRING_LENGTH, "Raise MAX_SHOP_OBJ constant in shop.h to %d", len + error);
    stderr_log(buf);
  }
  BUY_WORD(list[len]) = 0;
  BUY_TYPE(list[len++]) = NOTHING;
  return (len);
}

void read_line(FILE *shop_f, char *string, void *data) {
  if (!get_line(shop_f, buf) || !sscanf(buf, string, data)) {
    fprintf(stderr, "Error in shop #%d\n", SHOP_NUM(top_shop));
    fflush(NULL);
    exit(1);
  }
}

int read_list(FILE *shop_f, struct shop_buy_data *list, int new_format, int max, int type) {
  int count, temp, len = 0, error = 0;

  if (new_format) {
    do {
      read_line(shop_f, "%d", &temp);
      error += add_to_list(list, type, &len, &temp);
    } while (temp >= 0);
  } else
    for (count = 0; count < max; count++) {
      read_line(shop_f, "%d", &temp);
      error += add_to_list(list, type, &len, &temp);
    }
  return (end_read_list(list, len, error));
}

int read_type_list(FILE *shop_f, struct shop_buy_data *list, int new_format, int max) {
  int index, num, len = 0, error = 0;
  char *ptr;

  if (!new_format)
    return (read_list(shop_f, list, 0, max, LIST_TRADE));
  do {
    fgets(buf, MAX_STRING_LENGTH - 1, shop_f);
    if ((ptr = strchr(buf, ';')) != NULL)
      *ptr = 0;
    else
      buf[strlen(buf) - 1] = '\0';
    for (index = 0, num = NOTHING; *item_types[index] != '\n'; index++)
      if (!strncasecmp(item_types[index], buf, strlen(item_types[index]))) {
        num = index;
        size_t offset = strlen(item_types[index]);
        memmove(buf, buf + offset, strlen(buf + offset) + 1);
        break;
      }
    ptr = buf;
    if (num == NOTHING) {
      sscanf(buf, "%d", &num);
      while (!isdigit(*ptr))
        ptr++;
      while (isdigit(*ptr))
        ptr++;
    }
    while (isspace(*ptr))
      ptr++;
    while ((strlen(ptr) >= 1) && isspace(ptr[strlen(ptr) - 1]))
      ptr[strlen(ptr) - 1] = '\0';
    error += add_to_list(list, LIST_TRADE, &len, &num);
    if (*ptr)
      BUY_WORD(list[len - 1]) = strdup(ptr);
  } while (num >= 0);
  return (end_read_list(list, len, error));
}

void boot_the_shops(FILE *shop_f, char *filename, int rec_count) {
  char *buf, buf2[150];
  int temp, count, new_format = 0;
  struct shop_buy_data list[MAX_SHOP_OBJ + 1];
  int done = 0;

  safe_snprintf(buf2, sizeof(buf2), "beginning of shop file %s", filename);

  while (!done) {
    buf = fread_string(shop_f, buf2);
    if (*buf == '#') { /* New shop */
      sscanf(buf, "#%d\n", &temp);
      safe_snprintf(buf2, sizeof(buf2), "shop #%d in shop file %s", temp, filename);
      FREE(buf);
      /* Plug memory leak! */
      if (!top_shop)
        CREATE(shop_index, struct shop_data, rec_count);

      SHOP_NUM(top_shop) = temp;
      temp = read_list(shop_f, list, new_format, MAX_PROD, LIST_PRODUCE);
      CREATE(shop_index[top_shop].producing, int, temp);
      for (count = 0; count < temp; count++)
        SHOP_PRODUCT(top_shop, count) = BUY_TYPE(list[count]);

      read_line(shop_f, "%f", &SHOP_BUYPROFIT(top_shop));
      read_line(shop_f, "%f", &SHOP_SELLPROFIT(top_shop));

      temp = read_type_list(shop_f, list, new_format, MAX_TRADE);
      CREATE(shop_index[top_shop].type, struct shop_buy_data, temp);
      for (count = 0; count < temp; count++) {
        SHOP_BUYTYPE(top_shop, count) = BUY_TYPE(list[count]);
        SHOP_BUYWORD(top_shop, count) = BUY_WORD(list[count]);
      }

      shop_index[top_shop].no_such_item1 = fread_string(shop_f, buf2);
      shop_index[top_shop].no_such_item2 = fread_string(shop_f, buf2);
      shop_index[top_shop].do_not_buy = fread_string(shop_f, buf2);
      shop_index[top_shop].missing_cash1 = fread_string(shop_f, buf2);
      shop_index[top_shop].missing_cash2 = fread_string(shop_f, buf2);
      shop_index[top_shop].message_buy = fread_string(shop_f, buf2);
      shop_index[top_shop].message_sell = fread_string(shop_f, buf2);
      read_line(shop_f, "%d", &SHOP_BROKE_TEMPER(top_shop));
      read_line(shop_f, "%d", &SHOP_BITVECTOR(top_shop));
      read_line(shop_f, "%d", &SHOP_KEEPER(top_shop));

      SHOP_KEEPER(top_shop) = real_mobile(SHOP_KEEPER(top_shop));
      read_line(shop_f, "%d", &SHOP_TRADE_WITH(top_shop));

      temp = read_list(shop_f, list, new_format, 1, LIST_ROOM);
      CREATE(shop_index[top_shop].in_room, int, temp);
      for (count = 0; count < temp; count++)
        SHOP_ROOM(top_shop, count) = BUY_TYPE(list[count]);

      read_line(shop_f, "%d", &SHOP_OPEN1(top_shop));
      read_line(shop_f, "%d", &SHOP_CLOSE1(top_shop));
      read_line(shop_f, "%d", &SHOP_OPEN2(top_shop));
      read_line(shop_f, "%d", &SHOP_CLOSE2(top_shop));

      SHOP_BANK(top_shop) = 0;
      SHOP_SORT(top_shop) = 0;
      SHOP_FUNC(top_shop) = 0;
      top_shop++;
    } else {
      if (*buf == '$') /* EOF */
        done = TRUE;
      else if (strstr(buf, VERSION3_TAG)) /* New format marker */
        new_format = 1;
      FREE(buf);
      /* Plug memory leak! */
    }
  }
}

void assign_the_shopkeepers(void) {
  int index;
  int rnum;

  cmd_say = find_command("say");
  cmd_tell = find_command("tell");
  cmd_emote = find_command("emote");
  cmd_slap = find_command("slap");
  cmd_puke = find_command("puke");
  cmd_push = find_command("push");
  for (index = 0; index < top_shop; index++) {
    rnum = SHOP_KEEPER(index);
    if (rnum > -1 && rnum <= top_of_mobt) {
      if (mob_index[rnum].func)
        SHOP_FUNC(index) = mob_index[rnum].func;
      mob_index[rnum].func = shop_keeper;
      mob_index[rnum].spec_type = SPEC_STANDARD;
    } else {
      printf("Shopkeeper #%d had invalid rnum.\n", SHOP_NUM(index));
    }
  }
}

char *customer_string(int shop_nr, int detailed) {
  int index, cnt = 1;
  static char buf[256];
  size_t blen = 0;

  *buf = 0;
  for (index = 0; *trade_letters[index] != '\n'; index++, cnt *= 2)
    if (!(SHOP_TRADE_WITH(shop_nr) & cnt))
      if (detailed) {
        if (*buf)
          blen += safe_snprintf(buf + blen, sizeof(buf) - blen, ", ");
        blen += safe_snprintf(buf + blen, sizeof(buf) - blen, "%s", trade_letters[index]);
      } else
        blen += safe_snprintf(buf + blen, sizeof(buf) - blen, "%c", *trade_letters[index]);
    else if (!detailed)
      blen += safe_snprintf(buf + blen, sizeof(buf) - blen, "_");

  return (buf);
}

void list_all_shops(struct char_data *ch) {
  int shop_nr;
  size_t buflen;

  buflen = safe_snprintf(buf, MAX_STRING_LENGTH, "\n\r");
  for (shop_nr = 0; shop_nr < top_shop; shop_nr++) {
    if (!(shop_nr % 19)) {
      buflen += safe_snprintf(buf + buflen, MAX_STRING_LENGTH - buflen,
                              " ##   Virtual   Where    Keeper    Buy   Sell   Customers\n\r");
      buflen += safe_snprintf(buf + buflen, MAX_STRING_LENGTH - buflen,
                              "---------------------------------------------------------\n\r");
    }
    size_t b2len = safe_snprintf(buf2, MAX_STRING_LENGTH, "%3d   %6d   %6d    ", shop_nr + 1, SHOP_NUM(shop_nr),
                                 SHOP_ROOM(shop_nr, 0));
    if (SHOP_KEEPER(shop_nr) < 0)
      safe_snprintf(buf1, MAX_STRING_LENGTH, "<NONE>");
    else
      safe_snprintf(buf1, MAX_STRING_LENGTH, "%6d", mob_index[SHOP_KEEPER(shop_nr)].virtual);
    b2len += safe_snprintf(buf2 + b2len, MAX_STRING_LENGTH - b2len, "%s   %3.2f   %3.2f    %s", buf1,
                           SHOP_SELLPROFIT(shop_nr), SHOP_BUYPROFIT(shop_nr), customer_string(shop_nr, FALSE));
    buflen += safe_snprintf(buf + buflen, MAX_STRING_LENGTH - buflen, "%s\n\r", buf2);
  }

  page_string(ch->desc, buf, 1);
}

void handle_detailed_list(char *buf, char *buf1, struct char_data *ch) {
  if ((strlen(buf1) + strlen(buf) < 78) || (strlen(buf) < 20))
    safe_snprintf(buf + strlen(buf), MAX_STRING_LENGTH - strlen(buf), "%s", buf1);
  else {
    safe_snprintf(buf + strlen(buf), MAX_STRING_LENGTH - strlen(buf), "\n\r");
    send_to_char(buf, ch);
    safe_snprintf(buf, MAX_STRING_LENGTH, "            %s", buf1);
  }
}

void list_detailed_shop(struct char_data *ch, int shop_nr) {
  struct obj_data *obj;
  struct char_data *k;
  int index, temp;

  safe_snprintf(buf, MAX_STRING_LENGTH, "Vnum:       [%5d], Rnum: [%5d]\n\r", SHOP_NUM(shop_nr), shop_nr + 1);
  send_to_char(buf, ch);

  size_t blen = safe_snprintf(buf, MAX_STRING_LENGTH, "Rooms:      ");
  for (index = 0; SHOP_ROOM(shop_nr, index) != NOWHERE; index++) {
    if (index)
      blen += safe_snprintf(buf + blen, MAX_STRING_LENGTH - blen, ", ");
    if ((temp = real_room(SHOP_ROOM(shop_nr, index))) != NOWHERE)
      safe_snprintf(buf1, MAX_STRING_LENGTH, "%s (#%d)", world[temp].name, world[temp].number);
    else
      safe_snprintf(buf1, MAX_STRING_LENGTH, "<UNKNOWN> (#%d)", SHOP_ROOM(shop_nr, index));
    handle_detailed_list(buf, buf1, ch);
    blen = strlen(buf);
  }
  if (!index)
    send_to_char("Rooms:      None!\n\r", ch);
  else {
    safe_snprintf(buf + blen, MAX_STRING_LENGTH - blen, "\n\r");
    send_to_char(buf, ch);
  }

  blen = safe_snprintf(buf, MAX_STRING_LENGTH, "Shopkeeper: ");
  if (SHOP_KEEPER(shop_nr) >= 0) {
    safe_snprintf(buf + blen, MAX_STRING_LENGTH - blen, "%s (#%d), Special Function: %s\n\r",
                  GET_NAME(&mob_proto[SHOP_KEEPER(shop_nr)]), mob_index[SHOP_KEEPER(shop_nr)].virtual,
                  YESNO(SHOP_FUNC(shop_nr)));
    if ((k = get_char_num(SHOP_KEEPER(shop_nr)))) {
      send_to_char(buf, ch);
      safe_snprintf(buf, MAX_STRING_LENGTH, "Coins:      [%9d], Bank: [%9d] (Total: %d)\n\r", GET_GOLD(k),
                    SHOP_BANK(shop_nr), GET_GOLD(k) + SHOP_BANK(shop_nr));
    }
  } else
    safe_snprintf(buf + blen, MAX_STRING_LENGTH - blen, "<NONE>\n\r");
  send_to_char(buf, ch);

  safe_snprintf(buf1, MAX_STRING_LENGTH, "%s", customer_string(shop_nr, TRUE));
  safe_snprintf(buf, MAX_STRING_LENGTH, "Customers:  %s\n\r", (*buf1) ? buf1 : "None");
  send_to_char(buf, ch);

  blen = safe_snprintf(buf, MAX_STRING_LENGTH, "Produces:   ");
  for (index = 0; SHOP_PRODUCT(shop_nr, index) != NOTHING; index++) {
    obj = &obj_proto[SHOP_PRODUCT(shop_nr, index)];
    if (index)
      blen += safe_snprintf(buf + blen, MAX_STRING_LENGTH - blen, ", ");
    safe_snprintf(buf1, MAX_STRING_LENGTH, "%s (#%d)", obj->short_description,
                  obj_index[SHOP_PRODUCT(shop_nr, index)].virtual);
    handle_detailed_list(buf, buf1, ch);
    blen = strlen(buf);
  }
  if (!index)
    send_to_char("Produces:   Nothing!\n\r", ch);
  else {
    safe_snprintf(buf + blen, MAX_STRING_LENGTH - blen, "\n\r");
    send_to_char(buf, ch);
  }

  blen = safe_snprintf(buf, MAX_STRING_LENGTH, "Buys:       ");
  for (index = 0; SHOP_BUYTYPE(shop_nr, index) != NOTHING; index++) {
    if (index)
      blen += safe_snprintf(buf + blen, MAX_STRING_LENGTH - blen, ", ");
    size_t b1len = safe_snprintf(buf1, MAX_STRING_LENGTH, "%s (#%d) ", item_types[SHOP_BUYTYPE(shop_nr, index)],
                                 SHOP_BUYTYPE(shop_nr, index));
    if (SHOP_BUYWORD(shop_nr, index))
      safe_snprintf(buf1 + b1len, MAX_STRING_LENGTH - b1len, "[%s]", SHOP_BUYWORD(shop_nr, index));
    else
      safe_snprintf(buf1 + b1len, MAX_STRING_LENGTH - b1len, "[all]");
    handle_detailed_list(buf, buf1, ch);
    blen = strlen(buf);
  }
  if (!index)
    send_to_char("Buys:       Nothing!\n\r", ch);
  else {
    safe_snprintf(buf + blen, MAX_STRING_LENGTH - blen, "\n\r");
    send_to_char(buf, ch);
  }

  safe_snprintf(buf, MAX_STRING_LENGTH, "Buy at:     [%4.2f], Sell at: [%4.2f], Open: [%d-%d, %d-%d]%s",
                SHOP_SELLPROFIT(shop_nr), SHOP_BUYPROFIT(shop_nr), SHOP_OPEN1(shop_nr), SHOP_CLOSE1(shop_nr),
                SHOP_OPEN2(shop_nr), SHOP_CLOSE2(shop_nr), "\n\r");

  send_to_char(buf, ch);

  sprintbit((long)SHOP_BITVECTOR(shop_nr), shop_bits, buf1);
  safe_snprintf(buf, MAX_STRING_LENGTH, "Bits:       %s\n\r", buf1);
  send_to_char(buf, ch);
}

void show_shops(struct char_data *ch, char *arg) {
  int shop_nr;

  if (!*arg)
    list_all_shops(ch);
  else {
    if (!str_cmp(arg, ".")) {
      for (shop_nr = 0; shop_nr < top_shop; shop_nr++)
        if (ok_shop_room(shop_nr, world[ch->in_room].number))
          break;

      if (shop_nr == top_shop) {
        send_to_char("This isn't a shop!\n\r", ch);
        return;
      }
    } else if (is_number(arg))
      shop_nr = atoi(arg) - 1;
    else
      shop_nr = -1;

    if ((shop_nr < 0) || (shop_nr >= top_shop)) {
      send_to_char("Illegal shop number.\n\r", ch);
      return;
    }
    list_detailed_shop(ch, shop_nr);
  }
}
