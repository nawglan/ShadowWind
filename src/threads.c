/* multithreading support */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#define PTHREAD_KERNEL

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h"
#include "handler.h"

/* External Structures */
extern struct descriptor_data *descriptor_list;
extern struct index_data *obj_index;
extern struct obj_data *object_list;
extern struct obj_data *obj_proto;
extern struct player_index_element *player_table;
extern int top_of_objt;
extern int top_of_p_table;

void* new_thread(void* arg)
{
  int i;

  i = 1;
  while (i != 0) {
    log("hello!");
    sleep(10);
  }
  return (NULL);

}

void start_main_threads(void)
{
  pthread_t thread;

  if (pthread_create(&thread, NULL, new_thread, (void *) 0xdeadbeef)) {
    log("Error: creating new thread\n");
    exit(0);
  }

}

void stop_main_threads(void)
{
  pthread_exit(NULL);
}
