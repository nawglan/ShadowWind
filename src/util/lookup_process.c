#include "lookup_process.h"
#include "../ident.h"
#include "../structs.h"
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

extern pid_t getpid(void);

static int dns_send_fifo;
static int dns_receive_fifo;
static const char *my_name;
static pid_t parent_pid;

static int get_message(int type, void *buffer, int msgsz);
static void clean_up(int sig);
static void setup_pid_file(char *dir);
static void setup_signals(void);

void clean_up(int sig) {
  (void)sig; /* unused parameter */
  _exit(1);
}

void setup_pid_file(char *dir) {
  FILE *pid_file;
  char filename[MAX_STRING_LENGTH];

  snprintf(filename, sizeof(filename), "%s/misc/lookup_%s_process.pid", dir, my_name);

  fprintf(stderr, "lookup process PID file: %s\n", filename);

  if ((pid_file = fopen(filename, "r")) != NULL) {
    pid_t old_pid;

    if (fscanf(pid_file, "%d", (int *)&old_pid)) {
      /* okay.. got a pid number... now lets try to find if it exists and kill it if so */
      if (old_pid && kill(old_pid, SIGTERM)) {
        /* errno:  ESRCH:   No such PID.
         EPERM:   We don't have permission to kill it */
        if (errno == EPERM) {
          /* Looks like someone started the mud as the wrong user */
          raise(SIGABRT);
        }
      }
    }
    fclose(pid_file);
    unlink(filename);
  }

  if ((pid_file = fopen(filename, "w")) == NULL) {
    fprintf(stderr, "Unable to open %s for writing\n", filename);
    raise(SIGABRT);
  }
  fprintf(pid_file, "%d\n", (int)getpid());
  fclose(pid_file);
}

void setup_signals(void) {
  struct sigaction sa;

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = clean_up;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  sigaction(SIGINT, &sa, NULL);  /* keyboard interrupt */
  sigaction(SIGQUIT, &sa, NULL); /* quit from keyboard */
  sigaction(SIGABRT, &sa, NULL); /* abort */
  sigaction(SIGFPE, &sa, NULL);  /* floating point exception */
  /* Note: SIGKILL cannot be caught - removed */
  sigaction(SIGSEGV, &sa, NULL); /* segmentation fault */
  sigaction(SIGALRM, &sa, NULL); /* alarm */
  sigaction(SIGTERM, &sa, NULL); /* termination signal */
}

int get_message(int type, void *buffer, int msgsz) {
  if (read(dns_send_fifo, buffer, msgsz) == -1) {
    if (errno == EAGAIN) {
      return -1;
    } else {
      perror("read");
      return 0;
    }
  }

  return 1;
}

int main(int argc, char **argv) {
  struct host_answer ans_buf;
  struct host_request req_buf;
  struct hostent *from;
  int found = 0;
  int pos = 1;
  char *dir;
  char tmppath[1024];

  while ((pos < argc) && (*(argv[pos]) == '-')) {
    switch (*(argv[pos] + 1)) {
    case 'd':
      if (*(argv[pos] + 2))
        dir = argv[pos] + 2;
      else if (++pos < argc)
        dir = argv[pos];
      else {
        fprintf(stderr, "Directory arg expected after option -d.\n");
        fflush(NULL);
        exit(1);
      }
      break;
    }
  }

  my_name = "HOSTS";

  setup_pid_file(dir);
  sleep(1);

  setup_signals();

  snprintf(tmppath, sizeof(tmppath), "%s/misc/dns_send_fifo", dir);
  fprintf(stderr, "opening send fifo: %s\n", tmppath);
  if ((dns_send_fifo = open(tmppath, O_RDONLY)) == -1) {
    perror("open fifo");
    exit(1);
  }
  snprintf(tmppath, sizeof(tmppath), "%s/misc/dns_receive_fifo", dir);
  fprintf(stderr, "opening receive fifo: %s\n", tmppath);
  if ((dns_receive_fifo = open(tmppath, O_WRONLY)) == -1) {
    perror("open fifo");
    exit(1);
  }
  parent_pid = getppid(); /* if my parent pid changes, then it must mean that my parent died... so
   I should as well */
  fprintf(stderr, "%s lookup process started with parent ID %d\n", my_name, (int)parent_pid);
  fflush(NULL);

  while (1) {
    while ((found = get_message(MSG_HOST_REQ, (void *)&req_buf, sizeof(struct host_request))) == -1)
      ;
    if (found) {

      /* okay... we have a message.. check for validity */

      if (req_buf.desc == 0) {
        break;
      }

      memset(&ans_buf, 0, sizeof(ans_buf));
      ans_buf.mtype = MSG_HOST_ANS;
      ans_buf.desc = req_buf.desc;
      strcpy(ans_buf.addr, req_buf.addr);

      if (!(from = gethostbyaddr((char *)&req_buf.sock.sin_addr, sizeof(req_buf.sock.sin_addr), AF_INET))) {
        if (h_errno != 1)
          strcpy(ans_buf.hostname, req_buf.addr);
      } else {
        *(from->h_name + 100) = 0;
        strcpy(ans_buf.hostname, from->h_name);
      }

      /* send the answer back... */
      if (write(dns_receive_fifo, (void *)&ans_buf, sizeof(struct host_answer)) == -1) {
        perror("write");
        clean_up(0);
        break;
      }
    } else {
      break;
    }
  }
  return 0;
}
