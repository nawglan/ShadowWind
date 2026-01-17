/* Header file for Auction system, written by Mattias Larsson (ml@algonet.se) */

#define AUCTION_PROFIT     0.02  /* How many percent the auctioneer wants */
#define AUCTION_MAX_PROFIT 10000 /* Maximum profit */
#define REPEAT_COUNTER                    \
  8 /* calls to wait before reannouncing, \
removing from list */
#define COUNT_COUNTER                    \
  2 /* Time to wait between, once, twice \
etc messages */

struct auction_item {
  long recnum;                          /* Identification number of this record */
  char *from;                           /* idnum of the objects owner 		*/
  char *to;                             /* idnum of the highest bidder		*/
  long price;                           /* The current bid                      */
  long created;                         /* Reserved - used to delete old stuff  */
  int announce;                         /* Number of times we have announced    */
  int sold;                             /* 0 - not sold, 1,2,3 - countd 4-sold, 5-paid  */
  long minbid;                          /* Minimum bid				*/
  int counter;                          /* Used for bid timing, and reannounce  */
  obj_num item_number;                  /* Vnum of item			        */
  int value[5];                         /* Object values			*/
  int extra_flags;                      /* Extra flags				*/
  int weight;                           /* Object weight			*/
  int timer;                            /* Item timer				*/
  long bitvector;                       /* Item bitvector			*/
  struct obj_affected_type affected[6]; /* Affections  */
  struct auction_item *next;            /* Pointer to next structure (if any)   */
};
