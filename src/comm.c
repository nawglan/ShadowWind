/* ************************************************************************
 *   File: comm.c                                        Part of CircleMUD *
 *  Usage: Communication, socket handling, main(), central game loop       *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ************************************************************************ */

#define __COMM_C__

#include "comm.h"
#include "db.h"
#include "event.h"
#include "handler.h"
#include "ident.h"
#include "interpreter.h"
#include "olc.h"
#include "screen.h"
#include "structs.h"
#include "util/lookup_process.h"
#include "utils.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* externs */
extern int restrict_game_lvl;
extern int mini_mud;
extern int no_rent_check;
extern FILE *qic_fl;
extern int DFLT_PORT;
extern char *DFLT_DIR;
extern int MAX_PLAYERS;
extern char *COLOR_TABLE[];
extern char *connected_types[];

extern struct room_data *world;         /* In db.c */
extern struct zone_data *zone_table;    /* In db.c */
extern int top_of_zone_table;           /* In db.c */
extern int top_of_world;                /* In db.c */
extern struct time_info_data time_info; /* In db.c */
extern void init_stats(void);
extern void run_events(void);
char *strip_color(char *from, char *to, int length);

/* local globals */
#define DNS_RECEIVE_FIFO "misc/dns_receive_fifo"
#define DNS_SEND_FIFO    "misc/dns_send_fifo"
int dns_receive_fifo;
int dns_send_fifo;
pid_t lookup_host_process;
struct descriptor_data *descriptor_list = NULL; /* master desc list */
struct txt_block *bufpool = 0;                  /* pool of large output buffers */
int buf_largecount = 0;                         /* # of large buffers which exist */
int buf_overflows = 0;                          /* # of overflows of output */
int buf_switches = 0;                           /* # of switches from small to large buf */
int circle_shutdown = 0;                        /* clean shutdown */
int circle_reboot = 0;                          /* reboot the game after a shutdown */
int no_specials = 0;                            /* Suppress ass. of special routines */
int avail_descs = 0;                            /* max descriptors available */
int tics = 0;                                   /* for extern checkpointing */
int pulse = 0;
bool MOBTrigger = TRUE;   /* For MOBProgs */
extern int auto_save;     /* see config.c */
extern int autosave_time; /* see config.c */
struct timeval null_time;
int port;
int SECS_PER_MUD_HOUR;
int SECS_PER_MUD_DAY;
int SECS_PER_MUD_MONTH;
int SECS_PER_MUD_YEAR;
int num_months;
int num_days;
int num_hours;

/* functions in this file */
int get_from_q(struct txt_q *queue, char *dest, int *aliased);
void init_game(int port);
void signal_setup(void);
void game_loop(int mother_desc);
int init_socket(int port);
int new_descriptor(int s);
int get_avail_descs(void);
int process_output(struct descriptor_data *t);
int process_input(struct descriptor_data *t);
void close_socket(struct descriptor_data *d);
struct timeval timediff(struct timeval *a, struct timeval *b);
void flush_queues(struct descriptor_data *d);
void nonblock(int s);
int perform_subst(struct descriptor_data *t, char *orig, char *subst);
int perform_alias(struct descriptor_data *d, char *orig);
void record_usage(void);
void make_prompt(struct descriptor_data *point);
void perform_idle_check(void);
void perform_camp_check(void);

/* extern fcnts */
void boot_db(void);
void echo_on(struct descriptor_data *d);
void zone_update(void);
void affect_update(void); /* In magic.c */
void point_update(void);  /* In limits.c */
void char_regen(void);    /* In limits.c */
void mobile_activity(void);
void perform_mob_defense(void);
void string_add(struct descriptor_data *d, char *str);
void perform_violence(void);
void show_string(struct descriptor_data *d, char *input);
int isbanned(char *hostname);
void weather_and_time(int mode);
void mprog_act_trigger(char *buf, struct char_data *mob, struct char_data *ch, struct obj_data *obj, void *vo);
void mprog_time_trigger(struct time_info_data time);
void perform_mobprog_activity();
void perform_mob_hunt();
void mprog_pulse();

#define SWPIDFILE "shadowwind.pid"

/* *********************************************************************
 *  main game loop and related stuff                                    *
 ********************************************************************* */

int main(int argc, char **argv) {
  char buf[512];
  int pos = 1;
  char *dir;
  FILE *pid_file;

  port = DFLT_PORT;
  dir = DFLT_DIR;

  SECS_PER_MUD_HOUR = 75;
  num_months = 17;
  num_days = 35;
  num_hours = 24;
  SECS_PER_MUD_DAY = (num_hours * SECS_PER_MUD_HOUR);
  SECS_PER_MUD_MONTH = (num_days * SECS_PER_MUD_DAY);
  SECS_PER_MUD_YEAR = (num_months * SECS_PER_MUD_MONTH);

  while ((pos < argc) && (*(argv[pos]) == '-')) {
    switch (*(argv[pos] + 1)) {
    case 'd':
      if (*(argv[pos] + 2))
        dir = argv[pos] + 2;
      else if (++pos < argc)
        dir = argv[pos];
      else {
        stderr_log("Directory arg expected after option -d.");
        fflush(NULL);
        exit(1);
      }
      safe_snprintf(buf, sizeof(buf), "Directory set to: %s", dir);
      stderr_log(buf);
      break;
    case 'm':
      mini_mud = 1;
      no_rent_check = 1;
      stderr_log("Running in minimized mode & with no rent check.");
      break;
    case 'q':
      no_rent_check = 1;
      stderr_log("Quick boot mode -- rent check supressed.");
      break;
    case 'r':
      restrict_game_lvl = 1;
      stderr_log("Restricting game -- no new players allowed.");
      break;
    case 's':
      no_specials = 1;
      stderr_log("Suppressing assignment of special routines.");
      break;
    default:
      safe_snprintf(buf, sizeof(buf), "SYSERR: Unknown option -%c in argument string.", *(argv[pos] + 1));
      stderr_log(buf);
      break;
    }
    pos++;
  }

  if (pos < argc) {
    if (!isdigit(*argv[pos])) {
      fprintf(stderr, "Usage: %s [-m] [-q] [-r] [-s] [-d pathname] [port #]\n", argv[0]);
      fflush(NULL);
      exit(1);
    } else if ((port = atoi(argv[pos])) <= 1024) {
      fprintf(stderr, "Illegal port number.\n");
      fflush(NULL);
      exit(1);
    }
  }
  safe_snprintf(buf, sizeof(buf), "%s/misc/%s", dir, SWPIDFILE);
  if ((pid_file = fopen(buf, "r")) != NULL) {
    pid_t old_pid;

    if (fscanf(pid_file, "%d", (int *)&old_pid)) {
      if (old_pid && kill(old_pid, SIGTERM)) {
        if (errno == EPERM) {
          raise(SIGABRT);
        }
      }
    }
    fclose(pid_file);
  }
  if ((pid_file = fopen(buf, "w")) == NULL) {
    raise(SIGABRT);
  }
  fprintf(pid_file, "%d\n", (int)getpid());
  fclose(pid_file);

  /* dnslookup */
  safe_snprintf(buf, sizeof(buf), "%s/%s", dir, DNS_RECEIVE_FIFO);
  unlink(buf);
  if (mkfifo(buf, S_IRUSR | S_IWUSR) == -1) {
    perror("mkfifo receive");
    exit(1);
  }
  if ((dns_receive_fifo = open(buf, O_RDWR | O_NONBLOCK)) == -1) {
    perror("Open receive fifo");
    exit(1);
  }
  safe_snprintf(buf, sizeof(buf), "%s/%s", dir, DNS_SEND_FIFO);
  unlink(buf);
  if (mkfifo(buf, S_IRUSR | S_IWUSR) == -1) {
    perror("mkfifo send");
    exit(1);
  }
  if ((dns_send_fifo = open(buf, O_RDWR | O_NONBLOCK)) == -1) {
    perror("Open send fifo");
    exit(1);
  }
  if (!(lookup_host_process = fork())) {
    safe_snprintf(buf, sizeof(buf), "%s/../bin/lookup_process", dir);
    stderr_log(buf);
    execl(buf, "lookup_process", "-d", dir, NULL);
  }
  /* end dnslookup */

  safe_snprintf(buf, sizeof(buf), "Running game on port %d.", port);
  stderr_log(buf);

  if (chdir(dir) < 0) {
    perror("Fatal error changing to data directory");
    fflush(NULL);
    exit(1);
  }
  safe_snprintf(buf, sizeof(buf), "Using %s as data directory.", dir);
  stderr_log(buf);

  init_game(port);

  safe_snprintf(buf, sizeof(buf), "%s/misc/%s", dir, SWPIDFILE);
  if (unlink(buf)) {
    perror("unlink");
  }
  return 0;
}

/* Init sockets, run game, and cleanup sockets */
void init_game(int port) {
  int mother_desc = 0;

  /*  void my_srand(unsigned long initial_seed);

   my_srand(time(0)); */

  stderr_log("Initializing sodium.");
  if (sodium_init() == -1) {
    stderr_log("Error initializing sodium.");
    exit(1);
  }

  stderr_log("Opening mother connection.");
  mother_desc = init_socket(port);

  avail_descs = get_avail_descs();

  stderr_log("Initializing stat matrix");
  init_stats();

  boot_db();

  stderr_log("Loading corpse file.");
  corpseloadall();

  stderr_log("Signal trapping.");
  signal_setup();

  stderr_log("Entering game loop.");

  game_loop(mother_desc);

  stderr_log("Closing all sockets.");
  while (descriptor_list)
    close_socket(descriptor_list);

  close(mother_desc);
  fclose(qic_fl);
  close(dns_send_fifo);
  close(dns_receive_fifo);
  if (circle_reboot) {
    stderr_log("Rebooting.");
    fflush(NULL);
    exit(42); /* what's so great about HHGTTG, anyhow? */
  }
  stderr_log("Normal termination of game.");
}

int init_socket(int port) {
  static struct sockaddr_in sa_zero;
  int x = 1;
  int fd = -1;

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Init_socket: socket");
    fflush(NULL);
    exit(1);
  }

  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&x, sizeof(x)) < 0) {
    perror("Init_socket: SO_REUSEADDR");
    close(fd);
    fflush(NULL);
    exit(1);
  }

#if !defined(sun) && defined(SO_DONTLINGER) && !defined(SYSV)
  {
    struct linger ld;

    ld.l_onoff = 1;
    ld.l_linger = 1000;

    if (setsockopt(fd, SOL_SOCKET, SO_DONTLINGER, (char *)&ld, sizeof(ld)) < 0) {
      perror("Init_socket: SO_DONTLINGER");
      close(fd);
      fflush(NULL);
      exit(1);
    }
  }
#endif

  sa_zero.sin_family = AF_INET;
  sa_zero.sin_port = htons(port);

  if (bind(fd, (struct sockaddr *)&sa_zero, sizeof(sa_zero)) < 0) {
    perror("Init_socket: bind");
    close(fd);
    fflush(NULL);
    exit(1);
  }

  if (listen(fd, 128) < 0) {
    perror("Init_socket: listen");
    close(fd);
    fflush(NULL);
    exit(1);
  }

  return fd;
}

int get_avail_descs(void) {
  int max_descs;
  max_descs = 0;
  /*
   * First, we'll try using getrlimit/setrlimit.  This will probably work
   * on most systems.
   */
#if defined(RLIMIT_NOFILE) || defined(RLIMIT_OFILE)
#if !defined(RLIMIT_NOFILE)
#define RLIMIT_NOFILE RLIMIT_OFILE
#endif
  {
    struct rlimit limit;

    getrlimit(RLIMIT_NOFILE, &limit);
    max_descs = MIN(MAX_PLAYERS + NUM_RESERVED_DESCS, limit.rlim_max);
    limit.rlim_cur = max_descs;
    setrlimit(RLIMIT_NOFILE, &limit);
  }
#elif defined(OPEN_MAX) || defined(FOPEN_MAX)
#if !defined(OPEN_MAX)
#define OPEN_MAX FOPEN_MAX
#endif
  max_descs = OPEN_MAX; /* Uh oh.. rlimit didn't work, but we have
                         * OPEN_MAX */
#else
  /*
   * Okay, you don't have getrlimit() and you don't have OPEN_MAX.  Time to
   * use the POSIX sysconf() function.  (See Stevens' _Advanced Programming
   * in the UNIX Environment_).
   */
  errno = 0;
  if ((max_descs = sysconf(_SC_OPEN_MAX)) < 0) {
    if (errno == 0)
      max_descs = MAX_PLAYERS + NUM_RESERVED_DESCS;
    else {
      perror("Error calling sysconf");
      fflush(NULL);
      exit(1);
    }
  }
#endif

  max_descs = MAX_PLAYERS + NUM_RESERVED_DESCS;

  if (max_descs <= 0) {
    stderr_log("Non-positive max player limit!");
    fflush(NULL);
    exit(1);
  }
  safe_snprintf(buf, MAX_STRING_LENGTH, "Setting player limit to %d.", max_descs);
  stderr_log(buf);
  return max_descs;
}

void update_last_login(struct descriptor_data *d) {
  if (d->username[0] != '\0') {
    if ((strlen(d->hostname) + strlen(d->username) + 1) > HOST_LENGTH) {
      d->hostname[HOST_LENGTH - strlen(d->username)] = '\0';
    }
    safe_snprintf(d->host, sizeof(d->host), "%s@%s", d->username, d->hostname);
  } else {
    safe_snprintf(d->host, sizeof(d->host), "%s", d->hostname);
  }
}

/*
 * game_loop contains the main loop which drives the entire MUD.  It
 * cycles once every 0.10 seconds and is responsible for accepting new
 * new connections, polling existing connections for input, dequeueing
 * output and sending it out to players, and calling "heartbeat" functions
 * such as mobile_activity().
 */
void game_loop(int mother_desc) {
  fd_set input_set, output_set, exc_set;
  struct timeval last_time, now, timespent, timeout, null_time, opt_time;
  char comm[MAX_INPUT_LENGTH];
  struct descriptor_data *d, *next_d;
  int mins_since_crashsave = 0, maxdesc, aliased;
  int hostchk = 0;
  struct host_answer host_ans_buf;

  /* initialize various time values */
  null_time.tv_sec = 0;
  null_time.tv_usec = 0;
  opt_time.tv_usec = OPT_USEC;
  opt_time.tv_sec = 0;
  gettimeofday(&last_time, (struct timezone *)0);

  /* The Main Loop.  The Big Cheese.  The Top Dog.  The Head Honcho.  The.. */
  while (!circle_shutdown) {

    hostchk = read(dns_receive_fifo, (void *)&host_ans_buf, sizeof(struct host_answer));

    if ((hostchk != -1) && (hostchk != 0)) {
      for (d = descriptor_list; d && (hostchk != -1); d = next_d) {
        next_d = d->next;
        if ((hostchk != -1) && (d->descriptor == host_ans_buf.desc) &&
            !strncmp(host_ans_buf.addr, d->hostIP, strlen(d->hostIP))) {
          if (host_ans_buf.hostname[0]) {
            safe_snprintf(d->hostname, sizeof(d->hostname), "%s", host_ans_buf.hostname);
          } else {
            safe_snprintf(d->hostname, sizeof(d->hostname), "%s", d->hostIP);
          }
          update_last_login(d);
          hostchk = -1;
        }
      }
    }

    /* Sleep if we don't have any connections */
    if (descriptor_list == NULL) {
      stderr_log("No connections.  Going to sleep.");
      FD_ZERO(&input_set);
      FD_SET(mother_desc, &input_set);
      if (select(mother_desc + 1, &input_set, (fd_set *)0, (fd_set *)0, NULL) < 0) {
        if (errno == EINTR)
          stderr_log("Waking up to process signal.");
        else
          perror("Select coma");
      } else
        stderr_log("New connection.  Waking up.");
      gettimeofday(&last_time, (struct timezone *)0);
    }
    /* Set up the input, output, and exception sets for select(). */
    FD_ZERO(&input_set);
    FD_ZERO(&output_set);
    FD_ZERO(&exc_set);
    FD_SET(mother_desc, &input_set);
    maxdesc = mother_desc;
    for (d = descriptor_list; d; d = d->next) {
      if (d->descriptor > maxdesc)
        maxdesc = d->descriptor;
      FD_SET(d->descriptor, &input_set);
      FD_SET(d->descriptor, &output_set);
      FD_SET(d->descriptor, &exc_set);
    }

    /*
     * At this point, the original Diku code set up a signal mask to avoid
     * block all signals from being delivered.  I believe this was done in
     * order to prevent the MUD from dying with an "interrupted system call"
     * error in the event that a signal be received while the MUD is dormant.
     * However, I think it is easier to check for an EINTR error return from
     * this select() call rather than to block and unblock signals.
     */
    do {
      errno = 0; /* clear error condition */

      /* figure out for how long we have to sleep */
      gettimeofday(&now, (struct timezone *)0);
      timespent = timediff(&now, &last_time);
      timeout = timediff(&opt_time, &timespent);

      /* sleep until the next 0.1 second mark */
      if (select(0, (fd_set *)0, (fd_set *)0, (fd_set *)0, &timeout) < 0)
        if (errno != EINTR) {
          perror("Select sleep");
          fflush(NULL);
          exit(1);
        }
    } while (errno);

    /* record the time for the next pass */
    gettimeofday(&last_time, (struct timezone *)0);

    /* poll (without blocking) for new input, output, and exceptions */
    if (select(maxdesc + 1, &input_set, &output_set, &exc_set, &null_time) < 0) {
      perror("Select poll");
      return;
    }
    /* new connection */
    if (FD_ISSET(mother_desc, &input_set))
      new_descriptor(mother_desc);

    /* kick out the freaky folks in the exception set */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (FD_ISSET(d->descriptor, &exc_set)) {
        FD_CLR(d->descriptor, &input_set);
        FD_CLR(d->descriptor, &output_set);
        close_socket(d);
      }
    }

    /* process descriptors with input pending */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (FD_ISSET(d->descriptor, &input_set))
        if (process_input(d) < 0)
          close_socket(d);
    }

    /* process descriptors with ident pending */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;

      if (waiting_for_ident(d))
        ident_check(d, pulse);
    }

    /* process commands we just read from process_input */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (!waiting_for_ident(d) && (--(d->wait) <= 0) && get_from_q(&d->input, comm, &aliased)) {
        if (d->character) {
          d->character->char_specials.timer = 0;
          if (!d->connected && GET_WAS_IN(d->character) != NOWHERE) {
            if (d->character->in_room != NOWHERE)
              char_from_room(d->character);
            char_to_room(d->character, GET_WAS_IN(d->character));
            GET_WAS_IN(d->character) = NOWHERE;
            act("$n has returned.", TRUE, d->character, 0, 0, TO_ROOM);
          }
        }
        d->wait = 1;
        d->prompt_mode = 1;

        if (d->showstr_count)
          show_string(d, comm);
        else if (d->str)
          string_add(d, comm);
        else if (d->connected != CON_PLAYING)
          nanny(d, comm);
        else {
          if (aliased)
            d->prompt_mode = 0;
          else {
            if (perform_alias(d, comm))
              get_from_q(&d->input, comm, &aliased);
          }
          command_interpreter(d->character, comm);
        }
      }
    }

    /* send queued output out to the operating system */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (FD_ISSET(d->descriptor, &output_set) && *(d->output)) {
        if (process_output(d) < 0) {
          close_socket(d);
        } else {
          d->prompt_mode = 1;
        }
      }
    }

    /* kick out folks in the CON_CLOSE state */
    for (d = descriptor_list; d; d = next_d) {
      next_d = d->next;
      if (STATE(d) == CON_CLOSE)
        close_socket(d);
    }

    /* give each descriptor an appropriate prompt */
    for (d = descriptor_list; d; d = d->next) {
      if (d->prompt_mode) {
        make_prompt(d);
        d->prompt_mode = 0;
      }
    }

    /* handle heartbeat stuff */
    /* Note: pulse now changes every 0.10 seconds  */

    pulse++;

    mprog_pulse();

    if (!(pulse % PULSE_ZONE)) {
      zone_update();
      perform_camp_check();
    }
    if (!(pulse % (PULSE_MOBILE / 2))) {
      perform_mob_defense();
    }
    if (!(pulse % PULSE_MOBILE)) {
      mobile_activity();
    }
    if (!(pulse % PULSE_MOBPROG)) {
      perform_mobprog_activity();
    }
    if (!(pulse % PULSE_MOBHUNT)) {
      perform_mob_hunt();
    }
    if (!(pulse % PULSE_EVENT))
      run_events();
    if (!(pulse % PULSE_VIOLENCE)) {
      perform_violence();
    }

    if (!(pulse % PULSE_CHK_IDLE)) {
      perform_idle_check();
    }

    if (!(pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC))) {
      safe_snprintf(logbuffer, sizeof(logbuffer), "*TICK* Gametime %dy %dm %dd %dh", time_info.year, time_info.month,
                    time_info.day, time_info.hours);
      mudlog(logbuffer, 'T', COM_IMMORT, FALSE);
      weather_and_time(1);
      point_update();
      mprog_time_trigger(time_info);
    }
    if (!(pulse % PULSE_UPDATE_AFFECTS)) {
      affect_update();
    }
    if (!(pulse % PULSE_REGEN)) {
      char_regen();
    }
    if (auto_save)
      if (!(pulse % (60 * PASSES_PER_SEC))) /* 1 minute */
        if (++mins_since_crashsave >= autosave_time) {
          mins_since_crashsave = 0;
          Crash_save_all();
          corpsesaveall();
          mudlog("Saving all corpses and plr objs", 'H', COM_IMMORT, FALSE);
        }
    if (!(pulse % (300 * PASSES_PER_SEC))) /* 5 minutes */
      record_usage();

    if (pulse >= (30 * 60 * PASSES_PER_SEC)) { /* 30 minutes */
      pulse = 0;
    }
    tics++; /* tics since last checkpoint signal */
  }
  Crash_save_all();
  corpsesaveall();
  kill(lookup_host_process, SIGKILL);
  mudlog("Saving all corpses and plr objs", 'H', COM_IMMORT, FALSE);
}

/* ******************************************************************
 *  general utility stuff (for local use)                            *
 ****************************************************************** */
void perform_camp_check(void) {
  EVENT(camp);
  struct descriptor_data *d;
  struct descriptor_data *next_d;
  extern struct descriptor_data *descriptor_list;
  extern struct event_info *pending_events;
  struct event_info *next_event;
  struct event_info *event;

  for (d = descriptor_list; d; d = next_d) {
    next_d = d->next;
    if (check_events(d->character, camp)) {
      for (event = pending_events; event; event = next_event) {
        next_event = event->next;
        if (event->func == camp) {
          if (((struct char_data *)event->causer == d->character) && (d->character->in_room != (long)event->info)) {
            send_to_char("So much for that camping effort.\r\n", d->character);
            clean_events(d->character, camp);
          }
        }
      }
    }
  }
}

void perform_idle_check(void) {
  struct descriptor_data *d;
  struct descriptor_data *next_d;
  extern char *nmotd;
  extern char *pc_race_types[];
  extern char *pc_class_types[];
  extern char *genders[];

  for (d = descriptor_list; d; d = next_d) {
    next_d = d->next;
    switch (STATE(d)) {
    case CON_HOMETOWN:
    case CON_ALIGNMENT:
    case CON_GET_TERMTYPE:
    case CON_GET_NAME:
    case CON_NAME_CNFRM:
    case CON_PASSWORD:
    case CON_NEWPASSWD:
    case CON_CHPWD_GETNEW:
    case CON_CNFPASSWD:
    case CON_CHPWD_VRFY:
    case CON_QSEX:
    case CON_QRACE:
    case CON_QCLASS:
    case CON_RMOTD:
    case CON_RECONNECT_AS:
    case CON_MENU:
    case CON_CHPWD_GETOLD:
    case CON_DELCNF1:
    case CON_DELCNF2:
    case CON_POLICY:
      if (d->idle_cnt > 200) {
        echo_on(d);
        SEND_TO_Q("\r\nTimed out... goodbye.\r\n", d);
        STATE(d) = CON_CLOSE;
      } else {
        d->idle_cnt++;
      }
      break;
    case CON_ACCEPT:
      if (d->idle_cnt > 40) {
        d->idle_cnt = 0;
        echo_on(d);
        SEND_TO_Q("\r\nNobody is avaliable to approve your character's name, so your character has been approved.\r\n"
                  "Be aware however that if your character's name is deemed out of context for both your\r\n"
                  "race and the mud, your character will be deleted with no reimbursement of eq or experience.\r\n",
                  d);
        SEND_TO_Q_COLOR(nmotd, d);
        SEND_TO_Q("\r\n\n*** PRESS RETURN: ", d);
        STATE(d) = CON_RMOTD;
        safe_snprintf(buf, MAX_STRING_LENGTH, "%s [%s] new player. (auto approved)", GET_NAME(d->character),
                      GET_HOST(d->character));
        REMOVE_BIT(PRF_FLAGS(d->character), PRF_DELETED);
        mudlog(buf, 'C', COM_IMMORT, TRUE);
      } else {
        d->idle_cnt++;
        if (!(d->idle_cnt % 10) || (d->idle_cnt == 1)) {
          safe_snprintf(buf, MAX_STRING_LENGTH, "%s (%s %s %s) [%s]. (needs approval)", GET_NAME(d->character),
                        genders[(int)GET_SEX(d->character)], pc_race_types[GET_RACE(d->character)],
                        pc_class_types[(int)GET_CLASS(d->character)], GET_HOST(d->character));
          mudlog(buf, 'C', COM_IMMORT, TRUE);
        }
      }
      break;
    }
  }
}

struct timeval timediff(struct timeval *a, struct timeval *b) {
  struct timeval rslt, tmp;

  tmp = *a;

  if ((rslt.tv_usec = tmp.tv_usec - b->tv_usec) < 0) {
    rslt.tv_usec += 1000000;
    --(tmp.tv_sec);
  }
  if ((rslt.tv_sec = tmp.tv_sec - b->tv_sec) < 0) {
    rslt.tv_usec = 0;
    rslt.tv_sec = 0;
  }
  return rslt;
}

void record_usage(void) {
  int sockets_connected = 0, sockets_playing = 0;
  struct descriptor_data *d;
  char buf[256];

  for (d = descriptor_list; d; d = d->next) {
    sockets_connected++;
    if (!d->connected)
      sockets_playing++;
  }

  safe_snprintf(buf, sizeof(buf), "nusage: %-3d sockets connected, %-3d sockets playing", sockets_connected,
                sockets_playing);
  stderr_log(buf);

#ifdef RUSAGE
  {
    struct rusage ru;

    getrusage(0, &ru);
    safe_snprintf(buf, sizeof(buf), "rusage: %d %d %d %d %d %d %d", ru.ru_utime.tv_sec, ru.ru_stime.tv_sec,
                  ru.ru_maxrss, ru.ru_ixrss, ru.ru_ismrss, ru.ru_idrss, ru.ru_isrss);
    stderr_log(buf);
  }
#endif
}

char *char_health(struct char_data *ch, struct char_data *viewer) {
  int percent;

  char *health[][2] = {{C_B_GREEN "excellent", "excellent"}, /* 100% */
                       {C_YELLOW "scratches", "scratches"},  /*  100% */
                       {C_B_YELLOW "bruises", "bruises"},    /*  80% */
                       {C_B_MAGENTA "wounds", "wounds"},     /*  60% */
                       {C_MAGENTA "nasty", "nasty"},         /*  40% */
                       {C_B_RED "hurt", "hurt"},             /*  20% */
                       {C_RED "awful", "awful"},             /*  0% */
                       {C_RED "stunned", "stunned"},
                       {C_RED "mortally wounded", "mortally wounded"},
                       {C_D_GREY "dead", "dead"}};

  percent = ((100 * GET_HIT(ch)) / GET_MAX_HIT(ch));
  if (percent >= 100)
    percent = 0;
  else if (percent >= 90)
    percent = 1;
  else if (percent >= 75)
    percent = 2;
  else if (percent >= 50)
    percent = 3;
  else if (percent >= 30)
    percent = 4;
  else if (percent >= 15)
    percent = 5;
  else if (percent >= 0)
    percent = 6;

  if (GET_HIT(ch) <= 0) {
    if (PRF_FLAGGED(viewer, PRF_COLOR_2))
      return health[9 - GET_POS(ch)][0];
    else
      return health[9 - GET_POS(ch)][1];
  }
  if (GET_HIT(ch) >= GET_MAX_HIT(ch)) {
    if (PRF_FLAGGED(viewer, PRF_COLOR_2))
      return health[0][0];
    else
      return health[0][1];
  }
  if (PRF_FLAGGED(viewer, PRF_COLOR_2))
    return health[percent][0];
  else
    return health[percent][1];
}

void make_prompt(struct descriptor_data *d) {
  char prompt[MAX_INPUT_LENGTH];

  /* reversed these top 2 if checks so that page_string() would work in */
  /* the editor */
  if (d->showstr_count) {
    safe_snprintf(prompt, sizeof(prompt),

                  "\r[ Return to continue, (q)uit, (r)efresh,"
                  " (b)ack, or page number (%d/%d) ]",

                  /* Removing the one wif color =(
                   "\r{c[ {WReturn to continue{c, ({Cq{c){Wuit{c, ({Cr{c){Wefresh{c,"
                   " {c({Cb{c){Wack{c, {Wor page number {c({C%d{c/{C%d{c) ]{x",
                   */
                  d->showstr_page, d->showstr_count);
    write_to_descriptor(d->descriptor, prompt);
  } else if (d->str)
    write_to_descriptor(d->descriptor, "] ");
  else if (!d->connected) {
    char prompt[MAX_INPUT_LENGTH];

    *prompt = '\0';

    if (GET_PROMPT(d->character) != 0)
      safe_snprintf(prompt, sizeof(prompt), "%s[%s", CCBLU(d->character, C_SPR), CCNRM(d->character, C_SPR));

    if (GET_INVIS_LEV(d->character))
      safe_snprintf(prompt + strlen(prompt), sizeof(prompt) - strlen(prompt), "i%d ", GET_INVIS_LEV(d->character));

    if (IS_SET(GET_PROMPT(d->character), PRM_HP))
      safe_snprintf(prompt + strlen(prompt), sizeof(prompt) - strlen(prompt), "%s%d%shp%s ", CBWHT(d->character, C_SPR),
                    GET_HIT(d->character), CCWHT(d->character, C_SPR), CCNRM(d->character, C_SPR));

    if ((IS_MAGE(d->character) || IS_PRI(d->character)) && IS_SET(GET_PROMPT(d->character), PRM_MANA))
      safe_snprintf(prompt + strlen(prompt), sizeof(prompt) - strlen(prompt), "%s%d%sm%s ", CBGRN(d->character, C_SPR),
                    GET_MANA(d->character), CCGRN(d->character, C_SPR), CCNRM(d->character, C_SPR));

    if (IS_SET(GET_PROMPT(d->character), PRM_MOVE))
      safe_snprintf(prompt + strlen(prompt), sizeof(prompt) - strlen(prompt), "%s%d%sv%s ", CBBLU(d->character, C_SPR),
                    GET_MOVE(d->character), CCBLU(d->character, C_SPR), CCNRM(d->character, C_SPR));
    if (IS_SET(GET_PROMPT(d->character), PRM_TANKCOND | PRM_TANKNAME) && FIGHTING(d->character) &&
        FIGHTING(FIGHTING(d->character))) {
      safe_snprintf(prompt + strlen(prompt), sizeof(prompt) - strlen(prompt), " T: ");
      if (IS_SET(GET_PROMPT(d->character), PRM_TANKNAME) && FIGHTING(d->character) &&
          FIGHTING(FIGHTING(d->character))) {
        char temp[256];
        char tempname[256];
        safe_snprintf(temp, sizeof(temp), "%s", GET_PLR_NAME(FIGHTING(FIGHTING(d->character))));
        one_argument(temp, tempname);
        safe_snprintf(prompt + strlen(prompt), sizeof(prompt) - strlen(prompt), "%s%s%s ", CCCYN(d->character, C_SPR),
                      strip_color(CAP(tempname), temp, strlen(tempname)), CCNRM(d->character, C_SPR));
      }
      if (IS_SET(GET_PROMPT(d->character), PRM_TANKCOND) && FIGHTING(d->character) &&
          FIGHTING(FIGHTING(d->character))) {
        safe_snprintf(prompt + strlen(prompt), sizeof(prompt) - strlen(prompt), "%s(%s%s%s)%s ",
                      CCNRM(d->character, C_SPR), CCNRM(d->character, C_SPR),
                      char_health(FIGHTING(FIGHTING(d->character)), d->character), CCNRM(d->character, C_SPR),
                      CCNRM(d->character, C_SPR));
      }
    }
    if (IS_SET(GET_PROMPT(d->character), PRM_ENEMYCOND | PRM_ENEMYNAME) && FIGHTING(d->character)) {
      safe_snprintf(prompt + strlen(prompt), sizeof(prompt) - strlen(prompt), " E: ");
      if (IS_SET(GET_PROMPT(d->character), PRM_ENEMYNAME) && FIGHTING(d->character)) {
        char temp[256];
        char tempname[256];
        safe_snprintf(temp, sizeof(temp), "%s", GET_PLR_NAME(FIGHTING(d->character)));
        one_argument(temp, tempname);
        safe_snprintf(prompt + strlen(prompt), sizeof(prompt) - strlen(prompt), "%s%s%s ", CCCYN(d->character, C_SPR),
                      strip_color(CAP(tempname), temp, strlen(tempname)), CCNRM(d->character, C_SPR));
      }
      if (IS_SET(GET_PROMPT(d->character), PRM_ENEMYCOND) && FIGHTING(d->character)) {
        safe_snprintf(prompt + strlen(prompt), sizeof(prompt) - strlen(prompt), "%s(%s%s%s)%s ",
                      CCNRM(d->character, C_SPR), CCNRM(d->character, C_SPR),
                      char_health(FIGHTING(d->character), d->character), CCNRM(d->character, C_SPR),
                      CCNRM(d->character, C_SPR));
      }
    }
    if (IS_SET(GET_PROMPT(d->character), PRM_AFK)) {
      safe_snprintf(prompt + strlen(prompt), sizeof(prompt) - strlen(prompt), "%s(%sAFK%s)%s ",
                    CBWHT(d->character, C_SPR), CBRED(d->character, C_SPR), CBWHT(d->character, C_SPR),
                    CCNRM(d->character, C_SPR));
    }
    if (GET_PROMPT(d->character) == 0 || !IS_SET(GET_PROMPT(d->character), PRM_HP | PRM_MANA | PRM_MOVE | PRM_AFK))
      safe_snprintf(prompt + strlen(prompt), sizeof(prompt) - strlen(prompt), "> ");
    else {
      prompt[strlen(prompt) - 1] = '\0';
      safe_snprintf(prompt + strlen(prompt), sizeof(prompt) - strlen(prompt), "%s]%s>%s ", CCBLU(d->character, C_SPR),
                    CBWHT(d->character, C_SPR), CCNRM(d->character, C_SPR));
    }

    write_to_descriptor(d->descriptor, prompt);
  }
}

void write_to_q(char *txt, struct txt_q *queue, int aliased) {
  struct txt_block *new;

  CREATE(new, struct txt_block, 1);
  CREATE(new->text, char, strlen(txt) + 1);
  if (txt)
    memcpy(new->text, txt, strlen(txt) + 1);
  else
    memcpy(new->text, "\n", 2);

  new->aliased = aliased;

  /* queue empty? */
  if (!queue->head) {
    new->next = NULL;
    queue->head = queue->tail = new;
  } else {
    queue->tail->next = new;
    queue->tail = new;
    new->next = NULL;
  }
}

int get_from_q(struct txt_q *queue, char *dest, int *aliased) {
  struct txt_block *tmp;

  if (queue && queue->head && queue->head->text && dest) {
    tmp = queue->head->next;
    safe_snprintf(dest, MAX_INPUT_LENGTH, "%s", queue->head->text);
  } else
    return 0;

  *aliased = queue->head->aliased;

  FREE(queue->head->text);
  FREE(queue->head);

  queue->head = tmp;

  return 1;
}

/* Empty the queues before closing connection */
void flush_queues(struct descriptor_data *d) {
  int dummy;

  if (d->large_outbuf) {
    d->large_outbuf->next = bufpool;
    bufpool = d->large_outbuf;
  }
  while (get_from_q(&d->input, buf2, &dummy))
    ;
}

void write_to_output(const char *txt, struct descriptor_data *t) {
  int size;

  size = strlen(txt);

  /* if we're in the overflow state already, ignore this new output */
  if (t->bufptr < 0)
    return;

  /* if we have enough space, just write to buffer and that's it! */
  if (t->bufspace >= size) {
    memcpy(t->output + t->bufptr, txt, size + 1);
    t->bufspace -= size;
    t->bufptr += size;
  } else { /* otherwise, try switching to a lrg buffer */
    if (t->large_outbuf || ((size + strlen(t->output)) > LARGE_BUFSIZE)) {
      /*
       * we're already using large buffer, or even the large buffer isn't big
       * enough -- switch to overflow state
       */
      t->bufptr = -1;
      buf_overflows++;
      return;
    }
    buf_switches++;

    /* if the pool has a buffer in it, grab it */
    if (bufpool != NULL) {
      t->large_outbuf = bufpool;
      bufpool = bufpool->next;
    } else { /* else create a new one */
      CREATE(t->large_outbuf, struct txt_block, 1);
      CREATE(t->large_outbuf->text, char, LARGE_BUFSIZE);
      buf_largecount++;
    }

    safe_snprintf(t->large_outbuf->text, LARGE_BUFSIZE, "%s", t->output); /* copy to big buffer */
    t->output = t->large_outbuf->text;                                    /* make big buffer primary */
    safe_snprintf(t->output + strlen(t->output), LARGE_BUFSIZE - strlen(t->output), "%s", txt); /* now add new text */

    /* calculate how much space is left in the buffer */
    t->bufspace = LARGE_BUFSIZE - 1 - strlen(t->output);

    /* set the pointer for the next write */
    t->bufptr = strlen(t->output);
  }
}

/* ******************************************************************
 *  socket handling                                                  *
 ****************************************************************** */

int new_descriptor(int s) {
  int desc, sockets_connected = 0;
  static int last_desc = 0; /* last descriptor number */
  struct descriptor_data *newd;
  struct sockaddr_in peer;
  unsigned int size = sizeof(peer);
  int found = FALSE;
  struct host_request hr_buf;

  memset(&peer, 0, sizeof(struct sockaddr_in));
  memset(&hr_buf, 0, sizeof(struct host_request));
  /* accept the new connection */
  if ((desc = accept(s, (struct sockaddr *)&peer, &size)) < 0) {
    perror("Accept");
    return -1;
  }
  /* keep it from blocking */
  nonblock(desc);

  /* make sure we have room for it */
  for (newd = descriptor_list; newd; newd = newd->next) {
    sockets_connected++;
    if (newd->ident_sock != -1)
      sockets_connected++;
  }

  if (sockets_connected >= avail_descs) {
    write_to_descriptor(desc, "Sorry, ShadowWind is full right now... try again later!  :-)\r\n");
    close(desc);
    return 0;
  }
  /* create a new descriptor */
  CREATE(newd, struct descriptor_data, 1);
  memset((char *)newd, 0, sizeof(struct descriptor_data));

  /* new code for hostlookups */
  if (getpeername(desc, (struct sockaddr *)&peer, &size) < 0) {
    perror("getpeername");
    safe_snprintf(newd->hostIP, sizeof(newd->hostIP), "Unknown");
    found = TRUE;
  } else {
    safe_snprintf(newd->hostIP, sizeof(newd->hostIP), "%d.%d.%d.%d", ((unsigned char *)&(peer.sin_addr))[0],
                  ((unsigned char *)&(peer.sin_addr))[1], ((unsigned char *)&(peer.sin_addr))[2],
                  ((unsigned char *)&(peer.sin_addr))[3]);
  }
  /* determine if the site is banned */
  if (isbanned(newd->hostIP) == BAN_ALL) {
    close(desc);
    safe_snprintf(buf2, MAX_STRING_LENGTH, "Connection attempt denied from [%s]", newd->hostIP);
    mudlog(buf2, 'S', COM_IMMORT, TRUE);
    FREE(newd);
    return 0;
  }

  /* Log new connections - probably unnecessary, but you may want it */
  safe_snprintf(buf2, MAX_STRING_LENGTH, "New connection from [%s]", newd->hostIP);
  mudlog(buf2, 'S', COM_ADMIN, FALSE);

  /* initialize descriptor data */
  newd->descriptor = desc;
  newd->connected = CON_GET_NAME;
  newd->peer_port = peer.sin_port;
  newd->wait = 2;
  newd->output = newd->small_outbuf;
  newd->bufspace = SMALL_BUFSIZE - 1;
  newd->next = descriptor_list;
  newd->login_time = time(0);
  newd->host[0] = '\0';

  if (++last_desc == 1000)
    last_desc = 1;
  newd->desc_num = last_desc;

  /* prepend to list */
  descriptor_list = newd;

  SEND_TO_Q("Verifying your hostname...\r\n", newd);
  ident_start(newd, peer.sin_addr.s_addr);
  SEND_TO_Q("\r\nANSI Terminal settings? (y/n) [y]", newd);

  if (!found) {
    hr_buf.mtype = MSG_HOST_REQ;
    hr_buf.desc = desc;
    safe_snprintf(hr_buf.addr, sizeof(hr_buf.addr), "%s", newd->hostIP);
    memcpy(&hr_buf.sock, &peer, sizeof(struct sockaddr_in));

    if (write(dns_send_fifo, (void *)&hr_buf, sizeof(struct host_request)) == -1) {
      perror("dns_send_req");
      exit(1);
    }
  }
  return 0;
}

int process_output(struct descriptor_data *t) {
  static char i[LARGE_BUFSIZE + GARBAGE_SPACE];
  static int result;

  /* we may need this \r\n for later -- see below */
  memcpy(i, "\r\n", 3);

  /* now, append the 'real' output */
  safe_snprintf(i + 2, sizeof(i) - 2, "%s", t->output);

  /* if we're in the overflow state, notify the user */
  if (t->bufptr < 0)
    safe_snprintf(i + strlen(i), sizeof(i) - strlen(i), "%s", "**OVERFLOW**");

  /* add the extra CRLF if the person isn't in compact mode */
  if (!t->connected && t->character && !PRF_FLAGGED(t->character, PRF_COMPACT))
    safe_snprintf(i + strlen(i), sizeof(i) - strlen(i), "\r\n");

  /*
   * now, send the output.  If this is an 'interruption', use the prepended
   * CRLF, otherwise send the straight output sans CRLF.
   */
  if (!t->prompt_mode && !t->connected)
    result = write_to_descriptor(t->descriptor, i);
  else
    result = write_to_descriptor(t->descriptor, i + 2);

  /* handle snooping: prepend "% " and send to snooper */
  if (t->snoop_by) {
    SEND_TO_Q("% ", t->snoop_by);
    SEND_TO_Q(t->output, t->snoop_by);
    SEND_TO_Q("%%", t->snoop_by);
  }
  /*
   * if we were using a large buffer, put the large buffer on the buffer pool
   * and switch back to the small one
   */
  if (t->large_outbuf) {
    t->large_outbuf->next = bufpool;
    bufpool = t->large_outbuf;
    t->large_outbuf = NULL;
    t->output = t->small_outbuf;
  }
  /* reset total bufspace back to that of a small buffer */
  t->bufspace = SMALL_BUFSIZE - 1;
  t->bufptr = 0;
  *(t->output) = '\0';

  return result;
}

int write_to_descriptor(int desc, char *txt) {
#define UMIN(a, b) ((a) < (b) ? (a) : (b))
  int iStart;
  int nWrite;
  int nBlock;
  int length;

  length = strlen(txt);

  for (iStart = 0; iStart < length; iStart += nWrite) {
    nBlock = UMIN(length - iStart, 4096);
    if ((nWrite = write(desc, txt + iStart, nBlock)) < 0) {
      perror("Write_to_descriptor");
      return FALSE;
    }
  }

  fflush(NULL);
  return TRUE;
}

/*
 * ASSUMPTION: There will be no newlines in the raw input buffer when this
 * function is called.  We must maintain that before returning.
 */
int process_input(struct descriptor_data *t) {
  int buf_length, bytes_read, space_left, failed_subst;
  char *ptr, *read_point, *write_point, *nl_pos = NULL;
  char tmp[MAX_INPUT_LENGTH + 8];

  /* first, find the point where we left off reading data */
  buf_length = strlen(t->inbuf);
  read_point = t->inbuf + buf_length;
  space_left = MAX_RAW_INPUT_LENGTH - buf_length - 1;

  do {
    if (space_left <= 0) {
      stderr_log("process_input: about to close connection: input overflow");
      return -1;
    }

    if ((bytes_read = read(t->descriptor, read_point, space_left)) < 0) {

#ifdef EWOULDBLOCK
      if (errno == EWOULDBLOCK)
        errno = EAGAIN;
#endif
      if (errno != EAGAIN) {
        perror("process_input: about to lose connection");
        return -1; /* some error condition was encountered on
                    * read */
      } else
        return 0; /* the read would have blocked: just means no
                   * data there */
    } else if (bytes_read == 0) {
      stderr_log("EOF on socket read (connection broken by peer)");
      return -1;
    }
    /* at this point, we know we got some data from the read */

    *(read_point + bytes_read) = '\0'; /* terminate the string */

    /* search for a newline in the data we just read */
    for (ptr = read_point; *ptr && !nl_pos; ptr++)
      if (ISNEWL(*ptr))
        nl_pos = ptr;

    read_point += bytes_read;
    space_left -= bytes_read;

    /*
     * on some systems such as AIX, POSIX-standard nonblocking I/O is broken,
     * causing the MUD to hang when it encounters input not terminated by a
     * newline.  This was causing hangs at the Password: prompt, for example.
     * I attempt to compensate by always returning after the _first_ read, instead
     * of looping forever until a read returns -1.  This simulates non-blocking
     * I/O because the result is we never call read unless we know from select()
     * that data is ready (process_input is only called if select indicates that
     * this descriptor is in the read set).  JE 2/23/95.
     */
#if !defined(POSIX_NONBLOCK_BROKEN)
  } while (nl_pos == NULL);
#else
  } while (0);

  if (nl_pos == NULL)
    return 0;
#endif

  /*
   * okay, at this point we have at least one newline in the string; now we
   * can copy the formatted data to a new array for further processing.
   */

  read_point = t->inbuf;

  while (nl_pos != NULL) {
    write_point = tmp;
    space_left = MAX_INPUT_LENGTH - 1;

    for (ptr = read_point; (space_left > 0) && (ptr < nl_pos); ptr++) {
      if (*ptr == '\b') { /* handle backspacing */
        if (write_point > tmp) {
          if (*(--write_point) == '$') {
            write_point--;
            space_left += 2;
          } else
            space_left++;
        }
      } else if (isascii(*ptr) && isprint(*ptr)) {
        if ((*(write_point++) = *ptr) == '$') { /* copy one character */
          *(write_point++) = '$';               /* if it's a $, double it */
          space_left -= 2;
        } else
          space_left--;
      }
    }

    *write_point = '\0';

    if ((space_left <= 0) && (ptr < nl_pos)) {
      char buffer[MAX_INPUT_LENGTH + 64];

      safe_snprintf(buffer, sizeof(buffer), "Line too long.  Truncated to:\r\n%s\r\n", tmp);
      if (write_to_descriptor(t->descriptor, buffer) < 0)
        return -1;
    }
    if (t->snoop_by) {
      SEND_TO_Q("% ", t->snoop_by);
      SEND_TO_Q(tmp, t->snoop_by);
      SEND_TO_Q("\r\n", t->snoop_by);
    }
    failed_subst = 0;

    if (*tmp == '!')
      safe_snprintf(tmp, sizeof(tmp), "%s", t->last_input);
    else if (*tmp == '^') {
      if (!(failed_subst = perform_subst(t, t->last_input, tmp)))
        safe_snprintf(t->last_input, MAX_INPUT_LENGTH, "%s", tmp);
    } else
      safe_snprintf(t->last_input, MAX_INPUT_LENGTH, "%s", tmp);

    if (!failed_subst)
      write_to_q(tmp, &t->input, 0);

    /* find the end of this line */
    while (ISNEWL(*nl_pos))
      nl_pos++;

    /* see if there's another newline in the input buffer */
    read_point = ptr = nl_pos;
    for (nl_pos = NULL; *ptr && !nl_pos; ptr++)
      if (ISNEWL(*ptr))
        nl_pos = ptr;
  }

  /* now move the rest of the buffer up to the beginning for the next pass */
  write_point = t->inbuf;
  while (*read_point)
    *(write_point++) = *(read_point++);
  *write_point = '\0';

  return 1;
}

/*
 * perform substitution for the '^..^' csh-esque syntax
 * orig is the orig string (i.e. the one being modified.
 * subst contains the substition string, i.e. "^telm^tell"
 */
int perform_subst(struct descriptor_data *t, char *orig, char *subst) {
  char new[MAX_INPUT_LENGTH + 5];

  char *first, *second, *strpos;

  /*
   * first is the position of the beginning of the first string (the one
   * to be replaced
   */
  first = subst + 1;

  /* now find the second '^' */
  if (!(second = strchr(first, '^'))) {
    SEND_TO_Q("Invalid substitution.\r\n", t);
    return 1;
  }

  /* terminate "first" at the position of the '^' and make 'second' point
   * to the beginning of the second string */
  *(second++) = '\0';

  /* now, see if the contents of the first string appear in the original */
  if (!(strpos = strstr(orig, first))) {
    SEND_TO_Q("Invalid substitution.\r\n", t);
    return 1;
  }

  /* now, we construct the new string for output. */

  /* first, everything in the original, up to the string to be replaced */
  strncpy(new, orig, (strpos - orig));
  new[(strpos - orig)] = '\0';

  /* now, the replacement string */
  strncat(new, second, (MAX_INPUT_LENGTH - strlen(new) - 1));

  /* now, if there's anything left in the original after the string to
   * replaced, copy that too. */
  if (((strpos - orig) + strlen(first)) < strlen(orig))
    strncat(new, strpos + strlen(first), (MAX_INPUT_LENGTH - strlen(new) - 1));

  /* terminate the string in case of an overflow from strncat */
  new[MAX_INPUT_LENGTH - 1] = '\0';
  safe_snprintf(subst, MAX_INPUT_LENGTH, "%s", new);

  return 0;
}

void close_socket(struct descriptor_data *d) {
  struct descriptor_data *temp;
  char buf[100];

  close(d->descriptor);
  flush_queues(d);

  if (d->ident_sock != -1)
    close(d->ident_sock);

  /* Forget snooping */
  if (d->snooping)
    d->snooping->snoop_by = NULL;

  if (d->snoop_by) {
    SEND_TO_Q("Your victim is no longer among us.\r\n", d->snoop_by);
    d->snoop_by->snooping = NULL;
  }

  /*. Kill any OLC stuff .*/
  switch (d->connected) {
  case CON_OEDIT:
  case CON_REDIT:
  case CON_ZEDIT:
  case CON_MEDIT:
  case CON_SEDIT:
    cleanup_olc(d, CLEANUP_ALL);
    break;
  default:
    break;
  }

  if (d->character) {
    if (d->connected == CON_PLAYING || d->connected == CON_OEDIT || d->connected == CON_REDIT ||
        d->connected == CON_MEDIT || d->connected == CON_SEDIT || d->connected == CON_ZEDIT) {
      if (!PRF_FLAGGED(d->character, PRF_DELETED)) {
        save_char_text(d->character, NOWHERE);
        save_text(d->character);
      }
      act("$n has lost $s link.", TRUE, d->character, 0, 0, TO_ROOM);
      safe_snprintf(buf, sizeof(buf), "Closing link to: %s.", GET_NAME(d->character));
      mudlog(buf, 'C', COM_IMMORT, TRUE);
      plog(buf, d->character, 0);
      d->character->desc = NULL;
    } else {
      safe_snprintf(buf, sizeof(buf), "Losing player: %s (%s).",
                    GET_NAME(d->character) ? GET_NAME(d->character) : "<null>", connected_types[STATE(d)]);
      mudlog(buf, 'C', COM_IMMORT, TRUE);
      if (GET_NAME(d->character) != NULL && !PRF_FLAGGED(d->character, PRF_DELETED) &&
          (STATE(d) == CON_MENU || STATE(d) == CON_CLOSE)) {
        plog(buf, d->character, 0);
        if (!PLR_FLAGGED(d->character, PLR_RENT) && !PLR_FLAGGED(d->character, PLR_CRYO) &&
            !PLR_FLAGGED(d->character, PLR_CAMP))
          save_char_text(d->character, NOWHERE);
        save_text(d->character);
      }
      free_char(d->character);
    }
  } else
    mudlog("Losing descriptor without char.", 'S', COM_ADMIN, TRUE);

  if (d->original && d->original->desc)
    d->original->desc = NULL;

  REMOVE_FROM_LIST(d, descriptor_list, next);

  if (d->showstr_head)
    FREE(d->showstr_head);
  if (d->showstr_count)
    FREE(d->showstr_vector);
  if (d->storage)
    FREE(d->storage);
  if (d->namecolor)
    FREE(d->namecolor);

  FREE(d);
}
/*
 * I tried to universally convert Circle over to POSIX compliance, but,
 * alas, some systems are still straggling behind and don't have all the
 * appropriate defines.  In particular, NeXT 2.x defines O_NDELAY but not
 * O_NONBLOCK.  Krusty old NeXT machines!  (Thanks to Michael Jones for
 * this and various other NeXT fixes.)
 */
#ifndef O_NONBLOCK
#define O_NONBLOCK O_NDELAY
#endif

void nonblock(int s) {
  if (fcntl(s, F_SETFL, O_NONBLOCK) < 0) {
    perror("Fatal error executing nonblock (comm.c)");
    fflush(NULL);
    exit(1);
  }
}

/* ******************************************************************
 *  signal-handling functions (formerly signals.c)                   *
 ****************************************************************** */

void checkpointing(int x) {
  if (!tics) {
    stderr_log("SYSERR: CHECKPOINT shutdown: tics not updated");
    fflush(NULL);
    abort();
  } else
    tics = 0;
}

void reread_wizlists(int x) {
  void reboot_wizlists(void);

  mudlog("Rereading wizlists.", 'S', COM_IMMORT, FALSE);
  reboot_wizlists();
}

void unrestrict_game(int x) {
  extern struct ban_list_element *ban_list;
  extern int num_invalid;
  extern int num_declined;

  mudlog("Received SIGUSR2 - completely unrestricting game (emergent)", 'S', COM_ADMIN, TRUE);
  ban_list = NULL;
  restrict_game_lvl = 0;
  num_invalid = 0;
  num_declined = 0;
}

void hupsig(int x) {
  stderr_log("Received SIGHUP, SIGINT, or SIGTERM.  Shutting down...");
  fflush(NULL);
  kill(lookup_host_process, SIGKILL);
  exit(0); /* perhaps something more elegant should
            * substituted */
}

/*
 * This is an implementation of signal() using sigaction() for portability.
 * (sigaction() is POSIX; signal() is not.)  Taken from Stevens' _Advanced
 * Programming in the UNIX Environment_.  We are specifying that all system
 * calls _not_ be automatically restarted because BSD systems do not restart
 * select(), even if SA_RESTART is used.
 *
 * Note that NeXT 2.x is not POSIX and does not have sigaction; therefore,
 * I just define it to be the old signal.  If your system doesn't have
 * sigaction either, you can use the same fix.
 */

#if defined(NeXT)
#define my_dignal(signo, func) signal(signo, func)
#else
sigfunc *my_signal(int signo, sigfunc *func) {
  struct sigaction act, oact;

  act.sa_handler = func;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
#ifdef SA_INTERRUPT
  act.sa_flags |= SA_INTERRUPT; /* SunOS */
#endif

  if (sigaction(signo, &act, &oact) < 0)
    return SIG_ERR;

  return oact.sa_handler;
}
#endif /* NeXT */

void signal_setup(void) {
  struct itimerval itime;
  struct timeval interval;

  /* user signal 1: reread wizlists.  Used by autowiz system. */
  my_signal(SIGUSR1, reread_wizlists);

  /*
   * user signal 2: unrestrict game.  Used for emergencies if you lock
   * yourself out of the MUD somehow.  (Duh...)
   */
  my_signal(SIGUSR2, unrestrict_game);

  /*
   * set up the deadlock-protection so that the MUD aborts itself if it gets
   * caught in an infinite loop for more than 3 minutes
   */
  interval.tv_sec = 180;
  interval.tv_usec = 0;
  itime.it_interval = interval;
  itime.it_value = interval;
  setitimer(ITIMER_VIRTUAL, &itime, NULL);
  my_signal(SIGVTALRM, checkpointing);

  /* just to be on the safe side: */
  my_signal(SIGHUP, hupsig);
  my_signal(SIGINT, hupsig);
  my_signal(SIGTERM, hupsig);
  my_signal(SIGPIPE, SIG_IGN);
  my_signal(SIGALRM, SIG_IGN);
}

/* ****************************************************************
 *       Public routines for system-to-player-communication        *
 *******************************************************************/

void send_to_char_bw(char *messg, struct char_data *ch) {
  if (ch->desc && messg)
    SEND_TO_Q(messg, ch->desc);
}

void send_to_char(char *messg, struct char_data *ch) {
  const char *point;
  char *point2;
  char buf[MAX_STRING_LENGTH * 4];

  if (IS_NPC(ch)) {
    /*
     * send_to_char_bw(messg, ch);
     */
    return;
  }

  buf[0] = '\0';
  point2 = buf;
  if (messg && ch->desc) {
    if (PRF_FLAGGED(ch, PRF_COLOR_2)) {
      for (point = messg; point && *point; point++) {
        if (*point == '{') {
          point++;
          size_t remaining = sizeof(buf) - (point2 - buf);
          if (remaining > 1) {
            safe_snprintf(point2, remaining, "%s", colorf(*point, ch));
            for (; *point2; point2++)
              ;
          }
          continue;
        }
        if (point2 - buf < (int)sizeof(buf) - 1) {
          *point2 = *point;
          *++point2 = '\0';
        }
      }
      *point2 = '\0';
      SEND_TO_Q(buf, ch->desc);
    } else {
      for (point = messg; *point; point++) {
        if (*point == '{') {
          point++;
          if (*point == '{' && point2 - buf < (int)sizeof(buf) - 1)
            *point2 = '{';
          continue;
        }
        if (point2 - buf < (int)sizeof(buf) - 1) {
          *point2 = *point;
          *++point2 = '\0';
        }
      }
      *point2 = '\0';
      SEND_TO_Q(buf, ch->desc);
    }
  }
  return;
}

void SEND_TO_Q_COLOR(char *messg, struct descriptor_data *d) {
  const char *point;
  char *point2;
  char buf[MAX_STRING_LENGTH * 4];

  buf[0] = '\0';
  point2 = buf;
  if (d->color) {
    for (point = messg; *point; point++) {
      if (*point == '{') {
        point++;
        size_t remaining = sizeof(buf) - (point2 - buf);
        if (remaining > 1) {
          safe_snprintf(point2, remaining, "%s", colorf_d(*point));
          for (; *point2; point2++)
            ;
        }
        continue;
      }
      if (point2 - buf < (int)sizeof(buf) - 1) {
        *point2 = *point;
        *++point2 = '\0';
      }
    }
    *point2 = '\0';
    SEND_TO_Q(buf, d);
  } else {
    for (point = messg; *point; point++) {
      if (*point == '{') {
        point++;
        if (*point == '{')
          *point2 = '{';
        continue;
      }
      *point2 = *point;
      *++point2 = '\0';
    }
    *point2 = '\0';
    SEND_TO_Q(buf, d);
  }
  return;
}

void send_to_all(char *messg) {
  struct descriptor_data *i;

  if (messg)
    for (i = descriptor_list; i; i = i->next)
      if (!i->connected)
        SEND_TO_Q_COLOR(messg, i);
}

void send_to_outdoor(char *messg) {
  struct descriptor_data *i;

  if (!messg || !*messg)
    return;

  for (i = descriptor_list; i; i = i->next)
    if (!i->connected && i->character && AWAKE(i->character) && OUTSIDE(i->character) &&
        !PLR_FLAGGED(i->character, PLR_EDITING | PLR_WRITING))
      SEND_TO_Q_COLOR(messg, i);
}

void send_to_room(char *messg, int room) {
  struct char_data *i;

  if (messg)
    for (i = world[room].people; i; i = i->next_in_room)
      if (i->desc && i->desc->connected != CON_REDIT && i->desc->connected != CON_OEDIT &&
          i->desc->connected != CON_SEDIT && i->desc->connected != CON_ZEDIT && i->desc->connected != CON_MEDIT &&
          GET_POS(i) > POS_SLEEPING)
        SEND_TO_Q(messg, i->desc);
}

char *ACTNULL = "<NULL>";

#define CHECK_NULL(pointer, expression) \
  if ((pointer) == NULL)                \
    i = ACTNULL;                        \
  else                                  \
    i = (expression);

/* higher-level communication: the act() function */
void perform_act(char *orig, struct char_data *ch, struct obj_data *obj, void *vict_obj, struct char_data *to) {
  register char *i = NULL, *buf, *tmpbuf, *lbuf2;
  static char lbuf[MAX_STRING_LENGTH];
  static char lbuf3[MAX_STRING_LENGTH];

  buf = lbuf;
  lbuf2 = lbuf3;
  tmpbuf = lbuf3;

  for (;;) {
    if (*orig == '$') {
      switch (*(++orig)) {
      case 'n':
        i = PERS(ch, to);
        break;
      case 'N':
        CHECK_NULL(vict_obj, PERS((struct char_data *)vict_obj, to));
        break;
      case 'm':
        i = HMHR(ch);
        break;
      case 'M':
        CHECK_NULL(vict_obj, HMHR((struct char_data *)vict_obj));
        break;
      case 's':
        i = HSHR(ch);
        break;
      case 'S':
        CHECK_NULL(vict_obj, HSHR((struct char_data *)vict_obj));
        break;
      case 'e':
        i = HSSH(ch);
        break;
      case 'E':
        CHECK_NULL(vict_obj, HSSH((struct char_data *)vict_obj));
        break;
      case 'o':
        CHECK_NULL(obj, OBJN(obj, to));
        break;
      case 'O':
        CHECK_NULL(vict_obj, OBJN((struct obj_data *)vict_obj, to));
        break;
      case 'p':
        CHECK_NULL(obj, OBJS(obj, to));
        break;
      case 'P':
        CHECK_NULL(vict_obj, OBJS((struct obj_data *)vict_obj, to));
        break;
      case 'a':
        CHECK_NULL(obj, SANA(obj));
        break;
      case 'A':
        CHECK_NULL(vict_obj, SANA((struct obj_data *)vict_obj));
        break;
      case 'T':
        CHECK_NULL(vict_obj, (char *)vict_obj);
        break;
      case 'F':
        CHECK_NULL(vict_obj, fname((char *)vict_obj));
        break;
      case '_':
        i = CCBLU(ch, C_SPR);
        break;
      case '^':
        i = CBWHT(ch, C_SPR);
        break;
      case '|':
        i = CCNRM(ch, C_SPR);
        break;
      case '$':
        i = "$";
        break;
      default:
        stderr_log("SYSERR: Illegal $-code to act():");
        safe_snprintf(buf1, MAX_STRING_LENGTH, "SYSERR: %s", orig);
        stderr_log(buf1);
        break;
      }
      while ((i != NULL) && (*tmpbuf = *(i++)))
        tmpbuf++;
      orig++;
    } else if (!(*(tmpbuf++) = *(orig++)))
      break;
  }

  *(++tmpbuf) = '\0';

  if (*lbuf2 == '{') {
    lbuf2 += 2;
    CAP(lbuf2);
    lbuf2 -= 2;
  } else {
    CAP(lbuf2);
  }

  for (;;) {
    if (*lbuf2 == '{') {
      ++lbuf2;
      i = NULL;
      if (PRF_FLAGGED(to, PRF_COLOR_2)) {
        i = colorf(*lbuf2, to);
      }
      while ((i != NULL) && (*buf = *(i++)))
        buf++;
      lbuf2++;
    } else if (!(*(buf++) = *(lbuf2++)))
      break;
  }

  *(--buf) = '\r';
  *(++buf) = '\n';
  *(++buf) = '\0';
  if (to->desc)
    SEND_TO_Q(lbuf, to->desc);
  if (ch && MOBTrigger)
    mprog_act_trigger(lbuf, to, ch, obj, vict_obj);
}

char *colorf(char type, struct char_data *ch) {
  if (IS_NPC(ch))
    return ("");

  switch (type) {
  case 'x':
    return COLOR_TABLE[0];
  case 'r':
    return COLOR_TABLE[1];
  case 'g':
    return COLOR_TABLE[2];
  case 'y':
    return COLOR_TABLE[3];
  case 'b':
    return COLOR_TABLE[4];
  case 'm':
    return COLOR_TABLE[5];
  case 'c':
    return COLOR_TABLE[6];
  case 'w':
    return COLOR_TABLE[7];
  case 'D':
    return COLOR_TABLE[8];
  case 'R':
    return COLOR_TABLE[9];
  case 'G':
    return COLOR_TABLE[10];
  case 'Y':
    return COLOR_TABLE[11];
  case 'B':
    return COLOR_TABLE[12];
  case 'M':
    return COLOR_TABLE[13];
  case 'C':
    return COLOR_TABLE[14];
  case 'W':
    return COLOR_TABLE[15];
  case 'f':
  case 'F':
    return COLOR_TABLE[16];
  case 'u':
  case 'U':
    return COLOR_TABLE[17];
  case '{':
    return COLOR_TABLE[18];
  }
  return COLOR_TABLE[0];
}

char *colorf_d(char type) {
  static char code[MAX_INPUT_LENGTH];

  switch (type) {
  default:
    safe_snprintf(code, sizeof(code), "%s", CLEAR);
    break;
  case 'x':
    safe_snprintf(code, sizeof(code), "%s", CLEAR);
    break;
  case 'b':
    safe_snprintf(code, sizeof(code), "%s", C_BLUE);
    break;
  case 'c':
    safe_snprintf(code, sizeof(code), "%s", C_CYAN);
    break;
  case 'g':
    safe_snprintf(code, sizeof(code), "%s", C_GREEN);
    break;
  case 'm':
    safe_snprintf(code, sizeof(code), "%s", C_MAGENTA);
    break;
  case 'r':
    safe_snprintf(code, sizeof(code), "%s", C_RED);
    break;
  case 'w':
    safe_snprintf(code, sizeof(code), "%s", C_WHITE);
    break;
  case 'y':
    safe_snprintf(code, sizeof(code), "%s", C_YELLOW);
    break;
  case 'B':
    safe_snprintf(code, sizeof(code), "%s", C_B_BLUE);
    break;
  case 'C':
    safe_snprintf(code, sizeof(code), "%s", C_B_CYAN);
    break;
  case 'G':
    safe_snprintf(code, sizeof(code), "%s", C_B_GREEN);
    break;
  case 'M':
    safe_snprintf(code, sizeof(code), "%s", C_B_MAGENTA);
    break;
  case 'R':
    safe_snprintf(code, sizeof(code), "%s", C_B_RED);
    break;
  case 'W':
    safe_snprintf(code, sizeof(code), "%s", C_B_WHITE);
    break;
  case 'Y':
    safe_snprintf(code, sizeof(code), "%s", C_B_YELLOW);
    break;
  case 'D':
    safe_snprintf(code, sizeof(code), "%s", C_D_GREY);
    break;
  case '{':
    safe_snprintf(code, sizeof(code), "%c", '{');
    break;
  }
  return code;
}

/* This function returns the number of color related bytes in a string
 for text formatting in shops, etc. */
int strlen_c(char *string) {
  int count = 0;
  char *point;

  for (point = string; *point; point++) {
    if (*point == '{') {
      count = count + 2;
      point++;
    }
  }
  return count;
}

/* This function returns the number of NON color related bytes in a string
 for text formatting in shops, etc. */
int strlen_nc(char *string) {
  int count = 0;
  char *point;

  for (point = string; *point; point++) {
    if (*point == '{')
      point++;
    else
      count++;
  }
  return count;
}

#define SENDOK(ch) ((AWAKE(ch) || sleep) && !PLR_FLAGGED((ch), PLR_WRITING) && !PLR_FLAGGED((ch), PLR_EDITING))

/*
 arg1 = string to send
 arg2 = causer char
 arg3 = causer obj
 arg4 = victim
 arg5 = TO_xxxxxx
 */

void act(char *str, int hide_invisible, struct char_data *ch, struct obj_data *obj, void *vict_obj, int type) {
  struct char_data *to;
  static int sleep;

  if (!str || !*str) {
    MOBTrigger = TRUE;
    return;
  }

  /*
   * Warning: the following TO_SLEEP code is a hack.
   *
   * I wanted to be able to tell act to deliver a message regardless of sleep
   * without adding an additional argument.  TO_SLEEP is 128 (a single bit
   * high up).  It's ONLY legal to combine TO_SLEEP with one other TO_x
   * command.  It's not legal to combine TO_x's with each other otherwise.
   */

  /* check if TO_SLEEP is there, and remove it if it is. */
  if ((sleep = (type & TO_SLEEP)))
    type &= ~TO_SLEEP;

  if (type == TO_CHAR) {
    if (ch && SENDOK(ch))
      MOBTrigger = TRUE;
    perform_act(str, ch, obj, vict_obj, ch);
    return;
  }
  if (type == TO_VICT) {
    if ((to = (struct char_data *)vict_obj) && SENDOK(to))
      MOBTrigger = TRUE;
    perform_act(str, ch, obj, vict_obj, to);
    return;
  }
  /* ASSUMPTION: at this point we know type must be TO_NOTVICT or TO_ROOM */

  if (type == TO_EXIT)
    to = world[(long)vict_obj].people;
  else if (ch && ch->in_room != NOWHERE)
    to = world[ch->in_room].people;
  else if (obj && obj->in_room != NOWHERE)
    to = world[obj->in_room].people;
  else {
    stderr_log("SYSERR: no valid target to act()!");
    return;
  }

  for (; to; to = to->next_in_room)
    if (SENDOK(to) && !(hide_invisible && ch && !CAN_SEE(to, ch)) && (to != ch) &&
        (type == TO_ROOM || (to != vict_obj))) {
      MOBTrigger = TRUE;
      perform_act(str, ch, obj, vict_obj, to);
    }
}

void send_to_zone_outdoor(int zone, char *messg) {
  int i;
  int top = real_room(zone_table[zone].top);
  int bottom = real_room(zone_table[zone].bottom);
  if (zone < 0 || zone >= top_of_zone_table) {
    stderr_log("Bad zone in send_to_zone_outdoor!");
    return;
  }
  if (messg)
    for (i = bottom; i <= top; i++) {
      if (i >= 0 && i < top_of_world)
        if (!(ROOM_FLAGGED(i, ROOM_INDOORS)) && !(GET_SECT(i) == SECT_UNDERGROUND) && !(GET_SECT(i) == SECT_INSIDE))
          send_to_room(messg, i);
    }
}
void send_to_zone(int zone, char *messg) {
  int i;
  int top = real_room(zone_table[zone].top);
  int bottom = real_room(zone_table[zone].bottom);
  if (zone < 0 || zone >= top_of_zone_table) {
    stderr_log("Bad zone in send_to_zone!");
    return;
  }
  if (messg)
    for (i = bottom; i <= top; i++) {
      if (i >= 0 && i < top_of_world)
        send_to_room(messg, i);
    }
}
