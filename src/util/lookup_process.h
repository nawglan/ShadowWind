#ifndef __LOOKUP_PROCESS_H
#define __LOOKUP_PROCESS_H

#include <sys/types.h>

#define MSG_HOST_REQ  1
#define MSG_HOST_ANS  2

int run_lookup_host_process(void);

struct host_request {
  int mtype;
  sh_int desc;
  char addr[16];
  struct sockaddr_in sock;
};

struct host_answer {
  int mtype;
  sh_int desc;
  char addr[16]; /* stringified IP */
  char hostname[101]; /* actual hostname */
};

#endif /* __LOOKUP_PROCESS_H */
