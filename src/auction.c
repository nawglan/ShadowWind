/* Auction system, written by Mattias Larsson (ml@algonet.se) */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "auction.h"
#include "comm.h"
#include "db.h"
#include "event.h"
#include "handler.h"
#include "interpreter.h"
#include "spells.h"
#include "structs.h"
#include "utils.h"

/* External Structures */
extern struct descriptor_data *descriptor_list;
extern struct index_data *obj_index;
extern struct obj_data *object_list;
extern int load_qic_check(int rnum);
extern struct spell_info_type *spells;
int find_skill_num_def(int define);
int find_spell_num(char *name);
void Crash_save(struct char_data *ch, int type);

/* Global variables for auctioning system */

FILE *auction_fl = NULL;                  /* File identification for auction file */
struct auction_item *auction_list = NULL; /* linked list of auc items */
int auction_recs;                         /* Number of items in auction file */
long auc_slots[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

/* Creates new auction object */
struct auction_item *create_auction(void) {
  struct auction_item *auc;

  CREATE(auc, struct auction_item, 1);
  auc->next = auction_list;
  auction_list = auc;
  return auc;
}

/* Finds .from id num in auction database */
struct auction_item *auc_from_id(char *find_name) {
  struct auction_item *i;
  long j;

  i = auction_list;

  if (auction_recs == 0)
    return FALSE;

  for (j = 0; j < auction_recs; j++) {
    if (!strcmp(i->from, find_name))
      return i;
    i = i->next;
  }
  return FALSE;
}

/* Finds unused record :( */
struct auction_item *auc_find_unused(void) {
  struct auction_item *i;
  long j;

  if (auction_recs == 0)
    return FALSE;

  i = auction_list;

  for (j = 0; j < auction_recs; j++) {
    if (i->from == NULL && i->to == NULL)
      return i;
    i = i->next;
  }
  return FALSE;
}

/* Finds next available record number */
long auc_unique(void) {
  struct auction_item *i;
  long j, unique;

  if (auction_recs == 0)
    return 1; /* no records, just return 1.. */

  i = auction_list;
  unique = 0;

  for (j = 0; j < auction_recs; j++) {
    if (i->recnum > unique)
      unique = i->recnum;
    i = i->next;
  }
  unique++;
  return unique; /* returns the unique number */
}

/* Gets item number from Auction database */
struct auction_item *auc_get_record(long record) {
  struct auction_item *i;
  int j;

  i = auction_list;

  for (j = 0; j < auction_recs; j++) {
    if (i->recnum == record)
      return i;
    i = i->next;
  }
  return FALSE;
}

ACMD(do_auclist) {
  int i = 0;
  struct auction_item *auc;
  extern struct obj_data *obj_proto;
  auc = auction_list;

  safe_snprintf(g_string_buf, MAX_STRING_LENGTH * 2, "Items/gold currently held by Auctioneer:\r\n");
  size_t buflen = strlen(g_string_buf);
  size_t bufmax = MAX_STRING_LENGTH * 2;

  while (auc) {
    safe_snprintf(g_buf, MAX_STRING_LENGTH, "[%3d] Owner: %-25s Item: %s\r\n", i, auc->from,
                  obj_proto[real_object(auc->item_number)].short_description);
    size_t addlen = strlen(g_buf);
    if (buflen + addlen >= bufmax - 1)
      break;
    buflen += safe_snprintf(g_string_buf + buflen, bufmax - buflen, "%s", g_buf);

    safe_snprintf(g_buf, MAX_STRING_LENGTH, "      Bid: %ld  Count: %d  Buyer: %s \r\n", auc->price, auc->sold,
                  auc->to);
    addlen = strlen(g_buf);
    if (buflen + addlen >= bufmax - 1)
      break;
    buflen += safe_snprintf(g_string_buf + buflen, bufmax - buflen, "%s", g_buf);

    auc = auc->next;
    i++;
  }
  page_string(ch->desc, g_string_buf, 0);
}

void save_auction(void) {
  struct auction_item *i, saveauc;
  long j;
  int k;

  if (!(auction_fl = fopen(AUCTION_FILE, "w+b"))) {
    if (errno != ENOENT) {
      perror("1. fatal error opening auctionfile");
      fflush(NULL);
      exit(1);
    }
  }

  /* Okay, auction file is open, lets scan through all records and write
   them to disk */

  i = auction_list;

  if (auction_recs != 0) {

    for (j = 0; j < auction_recs; j++) {
      saveauc.recnum = i->recnum;
      saveauc.from = i->from;
      saveauc.to = i->to;
      saveauc.price = i->price;
      saveauc.created = i->created;
      saveauc.announce = i->announce;
      for (k = 0; k < 10; k++) {
        if (auc_slots[k] == i->recnum)
          saveauc.announce = -1;
      }
      saveauc.sold = i->sold;
      saveauc.minbid = i->minbid;
      saveauc.counter = i->counter;
      if (i->item_number < 0)
        i->item_number = 0;
      saveauc.item_number = i->item_number;
      for (k = 0; k < 5; k++)
        saveauc.value[k] = i->value[k];
      saveauc.extra_flags = i->extra_flags;
      saveauc.weight = i->weight;
      saveauc.timer = i->timer;
      saveauc.bitvector = i->bitvector;
      for (k = 0; k < 6; k++)
        saveauc.affected[k] = i->affected[k];
      saveauc.next = 0;

      if (fwrite(&saveauc, sizeof(struct auction_item), 1, auction_fl) < 1) {
        perror("Error writing auction record in SPEC_PROC");
        return;
      }

      i = i->next;
    }
  }
  fclose(auction_fl);
}

/* Loads the auctioning database into memory */

void load_auction(void) {

  time_t ltime;
  struct auction_item *auction, in;
  long size, lp, temp;
  int j;

  /* Open auction file (create new if necessary) */

  if (!(auction_fl = fopen(AUCTION_FILE, "r+b"))) {
    if (errno != ENOENT) {
      perror("2. fatal error opening auctionfile");
      fflush(NULL);
      exit(1);
    } else {
      stderr_log("No auctionfile.  Creating a new one.");
      touch(AUCTION_FILE);
      if (!(auction_fl = fopen(AUCTION_FILE, "r+b"))) {
        perror("3. fatal error opening auctionfile");
        fflush(NULL);
        exit(1);
      }
    }
  }
  for (j = 0; j < 10; j++)
    auc_slots[j] = -1;
  fseek(auction_fl, 0L, SEEK_END); /* seek end of file */
  size = ftell(auction_fl);        /* get current position (end of file=size) */
  rewind(auction_fl);              /* return to offest 0 again in file */
  if (size % sizeof(struct auction_item))
    fprintf(stderr, "WARNING:  AUCTIONFILE IS PROBABLY CORRUPT!\n");
  auction_recs = size / sizeof(struct auction_item);
  if (auction_recs) {
    safe_snprintf(g_buf, MAX_STRING_LENGTH, "   %d items in auction database.", auction_recs);
    stderr_log(g_buf);
  } else {
    fclose(auction_fl);
    return;
  }
  /* Okay.. there are a couple of records we should retrieve.. */
  temp = 0;

  for (lp = 0; lp < auction_recs; lp++) {
    fread(&in, sizeof(struct auction_item), 1, auction_fl);
    ltime = time(0);
    if ((in.created + 604800) > ltime) {
      temp++;
      auction = create_auction();
      auction->recnum = in.recnum;
      auction->from = in.from;
      auction->to = in.to;
      auction->price = in.price;
      auction->created = in.created;
      auction->counter = in.counter;
      auction->item_number = in.item_number;
      if (in.item_number < 0)
        in.item_number = 0;
      load_qic_check(real_object(in.item_number)); /* check if QIC */
      auction->minbid = in.minbid;
      auction->sold = in.sold;
      auction->announce = in.announce;
      auction->value[0] = in.value[0];
      auction->value[1] = in.value[1];
      auction->value[2] = in.value[2];
      auction->value[3] = in.value[3];
      auction->value[4] = in.value[4];
      auction->extra_flags = in.extra_flags;
      auction->weight = in.weight;
      auction->timer = in.timer;
      auction->bitvector = in.bitvector;
      for (j = 0; j < 6; j++)
        auction->affected[j] = in.affected[j];
    } else {
      safe_snprintf(g_buf, MAX_STRING_LENGTH, "  Timing-out auc-record %ld", lp);
      stderr_log(g_buf);
    }
  }

  auction_recs = temp;
  save_auction(); /* save the cleaned up database */

  return;
}

void auc_echo(char *g_arg, struct char_data *ch, struct obj_data *obj) {

  struct descriptor_data *i;

  for (i = descriptor_list; i; i = i->next) {
    if (!i->connected && i->character && !PRF_FLAGGED(i->character, PRF_NOAUCT) &&
        !PLR_FLAGGED(i->character, PLR_WRITING) && !PLR_FLAGGED(i->character, PLR_EDITING)) {
      act(g_arg, FALSE, ch, obj, i->character, TO_VICT | TO_SLEEP);
    }
  }
}

/* stupid wiz command */

ACMD(do_aucecho) {
  skip_spaces(&argument);

  if (!*argument)
    send_to_char("Yes.. but what?\r\n", ch);
  else {
    safe_snprintf(g_buf, MAX_STRING_LENGTH, "[AUC] %s", argument);
    auc_echo(g_buf, ch, NULL);
  }
}

/* The auction command */

ACMD(do_auction) {
  struct obj_data *obj;
  struct auction_item *auction;
  int j;
  long thebid, theitem;
  EVENT(spell_identify_event);
  int owe_auction = 0;
  int spellnum = spells[find_spell_num("identify")].spellindex;

  argument = one_argument(argument, g_arg);
  argument = one_argument(argument, g_buf);
  theitem = atol(g_buf) - 1;
  one_argument(argument, g_buf);
  thebid = atol(g_buf);

  if ((!IS_NPC(ch) && !COM_FLAGGED(ch, COM_ADMIN) && !PLR_FLAGGED(ch, PLR_UNRESTRICT)) || IS_NPC(ch)) {
    send_to_char("Maybe that's not such a good idea.\r\n", ch);
    return;
  }

  if (is_abbrev(g_arg, "bid")) {
    /* player wants to bid */

    if (theitem > 9 || theitem < 0) {
      send_to_char("Item numbers ranges between 1 and 10!\r\n", ch);
      return;
    }

    if (auc_slots[theitem] == -1) {
      send_to_char("There are currently no objects in auction with that number.\r\n", ch);
      return;
    }

    for (auction = auction_list; auction; auction = auction->next) {
      if (auction->to && !strcmp(auction->to, GET_NAME(ch)))
        owe_auction += auction->price;
    }
    auction = auc_get_record(auc_slots[theitem]);
    if ((GET_GOLD(ch) + GET_BANK_GOLD(ch) - owe_auction) < thebid) {
      send_to_char("You dont have that much money!\r\n", ch);
      return;
    }

    if (thebid < auction->minbid) {
      send_to_char("You got to bid higher or equal to the minimum bid!\r\n", ch);
      return;
    }

    if (thebid <= (auction->price * 1.05)) {
      send_to_char("No, no, no! You must bid 5% higher than the current bid!\r\n", ch);
      return;
    }

    if (!strcmp(GET_NAME(ch), auction->from)) {
      send_to_char("Bidding on your own item? I dont think so.\r\n", ch);
      return;
    }

    if (auction->to && !strcmp(GET_NAME(ch), auction->to)) {
      send_to_char("Why would you do that!? You are the one with the highest bid!\r\n", ch);
      return;
    }

    /* Set new highest bid */
    obj = read_object_q(auction->item_number, VIRTUAL);
    safe_snprintf(g_buf, MAX_STRING_LENGTH, "[AUC] New bid on item #%ld: $p", theitem + 1);
    auc_echo(g_buf, ch, obj);
    safe_snprintf(g_buf, MAX_STRING_LENGTH, "[AUC] Old bid: %ld, New bid: %ld", auction->price, thebid);
    auc_echo(g_buf, ch, 0);
    auction->price = thebid;
    auction->counter = COUNT_COUNTER;   /* once, twice, thrice counter */
    auction->sold = 0;                  /* reset countdown sequence */
    auction->to = strdup(GET_NAME(ch)); /* Who is buying? */
    extract_obj_q(obj);
    return;
  } else if (is_abbrev(g_arg, "list")) {
    /* List all objects in auction */
    send_to_char("Item(s) currently being auctioned:\r\n", ch);
    for (j = 0; j < 10; j++) {
      if (auc_slots[j] != -1) {
        auction = auc_get_record(auc_slots[j]);
        obj = read_object_q(auction->item_number, VIRTUAL);
        safe_snprintf(g_buf, MAX_STRING_LENGTH, "#%d: $p, Curr bid: %ld, Min bid: %ld", j + 1, auction->price,
                      auction->minbid);
        act(g_buf, FALSE, ch, obj, 0, TO_CHAR);
        extract_obj_q(obj);
      }
    }
  } else if (is_abbrev(g_arg, "identify")) {

    if (theitem > 9 || theitem < 0) {
      send_to_char("Item numbers ranges between 1 and 10!\r\n", ch);
      return;
    }

    if (auc_slots[theitem] == -1) {
      send_to_char("There are currently no objects in auction with that number.\r\n", ch);
      return;
    }

    if (GET_GOLD(ch) - 3000 < 0) {
      send_to_char("The Auctioneer tells you, 'You can't afford it!'\r\n", ch);
      return;
    }

    GET_GOLD(ch) = GET_GOLD(ch) - 3000;
    send_to_char("The Auctioneer tells you, 'That will be 3000 coins!'\r\n", ch);
    send_to_char("The Auctioneer tells you, 'This is what I know about that item -'\r\n", ch);

    auction = auc_get_record(auc_slots[theitem]);
    obj = read_object_q(auction->item_number, VIRTUAL);
    GET_OBJ_VAL(obj, 0) = auction->value[0];
    GET_OBJ_VAL(obj, 1) = auction->value[1];
    GET_OBJ_VAL(obj, 2) = auction->value[2];
    GET_OBJ_VAL(obj, 3) = auction->value[3];
    GET_OBJ_VAL(obj, 4) = auction->value[4];
    GET_OBJ_EXTRA(obj) = auction->extra_flags;
    GET_OBJ_WEIGHT(obj) = auction->weight;
    GET_OBJ_TIMER(obj) = auction->timer;
    obj->obj_flags.bitvector = auction->bitvector;

    for (j = 0; j < 6; j++)
      obj->affected[j] = auction->affected[j];

    spell_identify_event(ch, NULL, (long)obj->name, &spells[find_skill_num_def(spellnum)], 1);
    extract_obj_q(obj);

  } else {
    send_to_char("Usage: auction bid <item> <gold>\r\n"
                 "       auction list\r\n"
                 "       auction identify <item>\r\n",
                 ch);
  }
}

void auction_give_char(struct char_data *ch, struct auction_item *auction) {
  struct obj_data *obj;
  int j;

  obj = read_object_q(auction->item_number, VIRTUAL);
  GET_OBJ_VAL(obj, 0) = auction->value[0];
  GET_OBJ_VAL(obj, 1) = auction->value[1];
  GET_OBJ_VAL(obj, 2) = auction->value[2];
  GET_OBJ_VAL(obj, 3) = auction->value[3];
  GET_OBJ_VAL(obj, 4) = auction->value[4];
  GET_OBJ_EXTRA(obj) = auction->extra_flags;
  GET_OBJ_WEIGHT(obj) = auction->weight;
  GET_OBJ_TIMER(obj) = auction->timer;
  obj->obj_flags.bitvector = auction->bitvector;

  for (j = 0; j < 6; j++)
    obj->affected[j] = auction->affected[j];

  obj_to_char(obj, ch);
  act("The Auctioneer gives you $p.", FALSE, ch, obj, 0, TO_CHAR);
  act("The Auctioneer gives $p to $n.", FALSE, ch, obj, 0, TO_ROOM);
}

/* Special procedure for the Auctioneer */

SPECIAL(auctioneer) {
  time_t ltime;
  struct obj_data *obj;
  struct auction_item *auction, *temp;
  char minbid[240];
  int dotmode, slot, j;

  if (GET_POS(ch) == POS_FIGHTING)
    return 0;

  argument = one_argument(argument, g_arg);
  one_argument(argument, minbid);

  if (CMD_IS("sell")) {

    if ((!IS_NPC(ch) && !COM_FLAGGED(ch, COM_ADMIN) && !PLR_FLAGGED(ch, PLR_UNRESTRICT)) || IS_NPC(ch)) {
      send_to_char("The Auctioneer tells you 'Go away you stupid immortal!'\r\n", ch);
      return 1;
    }

    if (!*minbid) {
      send_to_char("You have to set a minimum bid (sell <item> <minbid>).\r\n", ch);
      return 1;
    }

    if (atoi(minbid) == 0) {
      send_to_char("The minimum bid has to be at least 1 gold coin.\r\n", ch);
      return 1;
    }

    dotmode = find_all_dots(g_arg);

    if (dotmode == FIND_ALL || dotmode == FIND_ALLDOT) {
      send_to_char("The Auctioneer tells you 'Sorry, I only accept one item at a time.'\r\n", ch);
      return 1;
    }
    if (!(obj = get_obj_in_list_vis(ch, g_arg, ch->carrying))) {
      send_to_char("The Auctioneer tells you 'You dont seem to be carrying anything like that.'\r\n", ch);
      return 1;
    }
    if (IS_OBJ_STAT(obj, ITEM_DONATED)) {
      send_to_char("The Auctioneer tells you 'You shouldn't sell what has been given to you!'\r\n", ch);
      return 1;
    }
    if (IS_OBJ_STAT(obj, ITEM_NOAUCTION)) {
      send_to_char("The Auctioneer tells you 'I'm sorry, I can't accept that item.'\r\n", ch);
      return 1;
    }
    if (IS_OBJ_STAT(obj, ITEM_NOSELL)) {
      send_to_char("The Auctioneer tells you 'I'm sorry, I can't accept that item.'\r\n", ch);
      return 1;
    }

    slot = -1;
    for (j = 0; j < 10; j++) {
      if (auc_slots[j] == -1) {
        slot = j;
        break;
      }
    }
    if (slot == -1) {
      send_to_char("The Auctioneer tells you 'Could not find any free slots, please come back later!'\r\n", ch);
      return 1;
    }

    obj_from_char(obj); /* takes object from char */
    act("The Auctioneer receives $p from $n.", FALSE, ch, obj, 0, TO_ROOM);
    act("You give the Auctioneer $p.", FALSE, ch, obj, 0, TO_CHAR);
    safe_snprintf(g_buf, MAX_STRING_LENGTH, "Minimum bid is set to %d coins.\r\n", atoi(minbid));
    send_to_char(g_buf, ch);
    safe_snprintf(g_buf, MAX_STRING_LENGTH, "%s auctioned %s for min %d coins", GET_NAME(ch), obj->short_description,
                  atoi(minbid));
    mudlog(g_buf, 'U', COM_IMMORT, FALSE);
    plog(g_buf, ch, 0);

    /* Add new record to end of list, and increment auction_recs variable */
    auction = create_auction();
    auction_recs++;
    auction->recnum = auc_unique(); /* find unique number */
    auction->from = strdup(GET_NAME(ch));
    auction->to = NULL;
    auction->price = 0;
    ltime = time(0);
    auction->created = ltime; /* timestamp - record created */
    auction->counter = -1;    /* timer value, -1 indicates crash saved */
    auction->item_number = GET_OBJ_VNUM(obj);
    auction->minbid = atoi(minbid);
    auction->sold = 0;
    auction->announce = -1;
    auction->value[0] = GET_OBJ_VAL(obj, 0);
    auction->value[1] = GET_OBJ_VAL(obj, 1);
    auction->value[2] = GET_OBJ_VAL(obj, 2);
    auction->value[3] = GET_OBJ_VAL(obj, 3);
    auction->value[4] = GET_OBJ_VAL(obj, 4);
    auction->extra_flags = GET_OBJ_EXTRA(obj);
    auction->weight = GET_OBJ_WEIGHT(obj);
    auction->timer = GET_OBJ_TIMER(obj);
    auction->bitvector = obj->obj_flags.bitvector;
    for (j = 0; j < 6; j++)
      auction->affected[j] = obj->affected[j];

    save_auction();                    /* saves the auction database for crash safety */
    auc_slots[slot] = auction->recnum; /* set unique number in slot list */
    auction->announce = 0;             /* crash saved stuff */
    auction->counter = REPEAT_COUNTER;

    safe_snprintf(g_buf, MAX_STRING_LENGTH, "[AUC] New item in auction! #%d: $p", slot + 1);
    auc_echo(g_buf, ch, obj);
    safe_snprintf(g_buf, MAX_STRING_LENGTH, "[AUC] Minimum bid is set to %ld coins.", auction->minbid);
    auc_echo(g_buf, ch, 0);
    extract_obj_q(obj); /* remove object from database */
    return 1;
  }

  if (CMD_IS("receive")) {

    /* First check for items that haven't been sold, that should
     be given back to the character :) (This routine also gives
     back items that wasn't sold due to a crash) */

    auction = auction_list;

    if (auction_recs == 0) {
      send_to_char("The Auctioneer tells you 'I dont owe you anything! Go away!'\r\n", ch);
      return 1;
    }

    slot = 0;

    for (j = 0; j < auction_recs; j++) {
      if (!strcmp(auction->from, GET_NAME(ch)) && auction->announce == -1) {
        /* Hey.. this item was never sold!! */
        send_to_char("The Auctioneer tells you 'This one was never sold, I'll give it back.'\r\n", ch);
        auction_give_char(ch, auction);
        /* Free record */
        FREE(auction->from);
        auction->from = NULL;
        auction->to = NULL;
        slot = 1;
      } else if (!strcmp(auction->from, GET_NAME(ch)) && auction->sold == 5) {
        /* This item has been retrieved and paid for by buyer, give money */
        GET_GOLD(ch) = GET_GOLD(ch) + auction->price;
        send_to_char("The Auctioneer tells you 'Here is the gold for your item that I sold earlier.'\r\n", ch);
        safe_snprintf(g_buf, MAX_STRING_LENGTH, "The Auctioneer gives you %ld coins.\r\n", auction->price);
        send_to_char(g_buf, ch);
        act("The Auctioneer gives $n some gold.", FALSE, ch, 0, 0, TO_ROOM);
        /* this record is free now, mark it as unused */
        FREE(auction->to);
        auction->to = NULL;
        FREE(auction->from);
        auction->from = NULL;
        slot = 1;
      } else if (auction->to && !strcmp(auction->to, GET_NAME(ch)) && auction->sold == 4) {
        /* This item was bought by the player, give it to him and take money */
        if (GET_GOLD(ch) < auction->price) {
          obj = read_object_q(auction->item_number, VIRTUAL);
          safe_snprintf(
              g_buf, MAX_STRING_LENGTH,
              "The Auctioneer tells you 'I have $p for you that you cant afford, you bought it for %ld coins.'\r\n",
              auction->price);
          act(g_buf, FALSE, ch, obj, 0, TO_CHAR);
          extract_obj_q(obj);
        } else {
          GET_GOLD(ch) = GET_GOLD(ch) - auction->price;
          safe_snprintf(g_buf, MAX_STRING_LENGTH, "The Auctioneer tells you 'This item only costs you %ld coins!'\r\n",
                        auction->price);
          send_to_char(g_buf, ch);
          auction_give_char(ch, auction);
          /* Indicate that the item was retrieved */
          auction->sold++;
          /* Code for auctioneers profit added here later */
          if ((auction->price * AUCTION_PROFIT) < AUCTION_MAX_PROFIT) {
            /* Okay.. profit is less than MAX_PROFIT, take our share */
            auction->price = auction->price - (auction->price * AUCTION_PROFIT);
          } else {
            /* The profit is more than MAX_PROFIT, just take MAX_PROFIT */
            auction->price = auction->price - AUCTION_MAX_PROFIT;
          }
          if (auction->price < 1)
            auction->price = 1; /* You should receive at least 1 coin :) */
          slot = 1;
        }
      }
      auction = auction->next;
    }

    if (slot == 0) {
      send_to_char("The Auctioneer tells you 'I dont owe you anything! Go away!'\r\n", ch);
      return 1;
    } else {
      /* remove all unused records from file */
      while ((auction = auc_find_unused())) {
        REMOVE_FROM_LIST(auction, auction_list, next);
        FREE(auction);
        auction_recs--;
      }
      save_auction();
      Crash_save(ch, RENT_CRASH);
      send_to_char("The Auctioneer tells you 'That was all I had for you. Welcome back!'\r\n", ch);
      safe_snprintf(g_buf, MAX_STRING_LENGTH, "%s received items/cash from auction", GET_NAME(ch));
      mudlog(g_buf, 'U', COM_IMMORT, FALSE);
      plog(g_buf, ch, 0);
      return 1;
    }
  }

  /* if it is a command, do not count */

  if (cmd)
    return 0;

  for (j = 0; j < 10; j++) {
    if (auc_slots[j] != -1) {
      auction = auc_get_record(auc_slots[j]);
      obj = read_object_q(auction->item_number, VIRTUAL);

      /* If the counter is zero again, and we already have showed the message once
       we should remove it from the list and make it available for retrival by
       owner */

      if (auction->price == 0 && auction->counter == 0 && auction->announce == 1) {
        safe_snprintf(g_buf, MAX_STRING_LENGTH, "[AUC] No bids on item #%d: $p", j + 1);
        auc_echo(g_buf, ch, obj);
        safe_snprintf(g_buf, MAX_STRING_LENGTH, "[AUC] Removing it from list.");
        auc_echo(g_buf, ch, 0);
        auc_slots[j] = -1;
        auction->announce = -1;
        return 0;
      } else if (auction->price == 0 && auction->counter == 0 && auction->announce == 0) {
        safe_snprintf(g_buf, MAX_STRING_LENGTH, "[AUC] New item in auction! #%d: $p", j + 1);
        auc_echo(g_buf, ch, obj);
        safe_snprintf(g_buf, MAX_STRING_LENGTH, "[AUC] Minimum bid is set to %ld coins.", auction->minbid);
        auc_echo(g_buf, ch, 0);
        auction->announce++;
        auction->counter = REPEAT_COUNTER;
        return 0;
      } else if (auction->price != 0 && auction->counter == 0) {
        /* Countdown, once, twice, thrice */
        auction->counter = COUNT_COUNTER; /* reset counter */
        switch (auction->sold) {
        case 0:
          safe_snprintf(g_buf, MAX_STRING_LENGTH, "[AUC] Item number %d: $p", j + 1);
          auc_echo(g_buf, ch, obj);
          safe_snprintf(g_buf, MAX_STRING_LENGTH, "[AUC] Going once to %s for %ld coins.", auction->to, auction->price);
          auc_echo(g_buf, ch, 0);
          auction->sold++;
          break;
        case 1:
          safe_snprintf(g_buf, MAX_STRING_LENGTH, "[AUC] Item number %d: $p", j + 1);
          auc_echo(g_buf, ch, obj);
          safe_snprintf(g_buf, MAX_STRING_LENGTH, "[AUC] Going twice to %s for %ld coins.", auction->to,
                        auction->price);
          auc_echo(g_buf, ch, 0);
          auction->sold++;
          break;
        case 2:
          safe_snprintf(g_buf, MAX_STRING_LENGTH, "[AUC] Item number %d: $p", j + 1);
          auc_echo(g_buf, ch, obj);
          safe_snprintf(g_buf, MAX_STRING_LENGTH, "[AUC] Going thrice to %s for %ld coins.", auction->to,
                        auction->price);
          auc_echo(g_buf, ch, 0);
          auction->sold++;
          break;
        case 3:
          safe_snprintf(g_buf, MAX_STRING_LENGTH, "[AUC] Item number %d: $p", j + 1);
          auc_echo(g_buf, ch, obj);
          safe_snprintf(g_buf, MAX_STRING_LENGTH, "[AUC] Sold to %s for %ld coins.", auction->to, auction->price);
          auc_echo(g_buf, ch, 0);
          auction->sold++;
          auc_slots[j] = -1; /* remove if from auction */
          save_auction();    /* save auction database */
          break;
        default:
          safe_snprintf(g_buf, MAX_STRING_LENGTH, "[AUC] I should never auction this, please bug-report!");
          auc_echo(g_buf, ch, 0);
        }
      }
      auction->counter--;
      /* free the object thingie (I got to do this in some other way later) */
      extract_obj_q(obj);
    }
  }

  return 0;
}
