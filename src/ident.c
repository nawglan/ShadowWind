/* ************************************************************************
 *  File: ident.c                                                          *
 *                                                                         *
 *  Usage: Functions for handling rfc 931/1413 ident lookups               *
 *                                                                         *
 *  Written by Eric Green (thrytis@imaxx.net)				  *
 *									  *
 *  Changes:								  *
 *      10/9/96 ejg:   Added compatibility with win95/winNT		  *
 *      10/25/97 ejg:  Updated email address, fixed close socket bug,      *
 *                     buffer overrun bug, and used extra hostlength space *
 *                     if available                                        *
 ************************************************************************ */

#define __IDENT_C__

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#if defined(linux) || defined(sun)
#include <sys/resource.h>
#endif
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

#ifndef CIRCLE_WINDOWS
#include <sys/socket.h>
/*
 #include <sys/resource.h>
 */
#include <netinet/in.h>
#include <netdb.h>
#endif

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "ident.h"

/* max time in seconds to make someone wait before entering game */
#define IDENT_TIMEOUT 10

#define IDENT_PORT    113

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

/* extern functions */
int isbanned(char *hostname);

/* log the errno/WSALastError() message */
void logerror(const char *msg)
{
#ifdef CIRCLE_WINDOWS
  fprintf(stderr, "%s: Winsock err #%d", msg, WSAGetLastError());
#else
  perror(msg);
#endif
}

/* start the process of looking up remote username */
void ident_start(struct descriptor_data *d, long addr)
{
  /* socket_t sock; */
  int sock;
  struct sockaddr_in sa;

  /* void nonblock(socket_t s); */
  void nonblock(int s);

  if (!ident) {
    STATE(d) = CON_ASKNAME;
    d->ident_sock = INVALID_SOCKET;
    return;
  }

  d->idle_tics = 0;

  /*
   * create a nonblocking socket, and start
   * the connection to the remote machine
   */

  if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
    logerror("socket");
    d->ident_sock = INVALID_SOCKET;
    STATE(d) = CON_ASKNAME;
    return;
  }

  sa.sin_family = AF_INET;
  sa.sin_port = ntohs(IDENT_PORT);
  sa.sin_addr.s_addr = addr;

  nonblock(sock);
  d->ident_sock = sock;

#ifdef CIRCLE_WINDOWS
  WSASetLastError(0);
#else
  errno = 0;
#endif
  if (connect(sock, (struct sockaddr*) &sa, sizeof(sa)) < 0) {
#ifdef CIRCLE_WINDOWS
    if (WSAGetLastError() == WSAEINPROGRESS ||
        WSAGetLastError() == WSAEWOULDBLOCK) {
#else
    if (errno == EINPROGRESS || errno == EWOULDBLOCK) {
#endif
      /* connection in progress */
      STATE(d) = CON_IDCONING;
      return;
    }

    /* connection failed */
#ifdef CIRCLE_WINDOWS
    else if (WSAGetLastError() != WSAECONNREFUSED)
#else
    else if (errno != ECONNREFUSED)
#endif
      logerror("ident connect");

    STATE(d) = CON_ASKNAME;
  }

  else
    /* connection completed */
    STATE(d) = CON_IDCONED;
}

void ident_check(struct descriptor_data *d, int pulse)
{
  fd_set fd, efd;
  int rc, rmt_port, our_port, len;
  char user[256], *p;

  extern struct timeval null_time;
  extern int port;

  /*
   * Each pulse, this checks if the ident is ready to proceed to the
   * next state, by calling select to see if the socket is writeable
   * (connected) or readable (response waiting).  
   */

  switch (STATE(d)) {
    case CON_IDCONING:
      /* waiting for connect() to finish */

      if (d->ident_sock != INVALID_SOCKET) {
        FD_ZERO(&fd);
        FD_ZERO(&efd);
        FD_SET(d->ident_sock, &fd);
        FD_SET(d->ident_sock, &efd);
      }

      if ((rc = select(d->ident_sock + 1, (fd_set *) 0, &fd, &efd, &null_time)) == 0)
        break;

      else if (rc < 0) {
        logerror("ident check select (conning)");
        STATE(d) = CON_ASKNAME;
        break;
      }

      if (FD_ISSET(d->ident_sock, &efd)) {
        /* exception, such as failure to connect */
        STATE(d) = CON_ASKNAME;
        break;
      }

      STATE(d) = CON_IDCONED;
      break;

    case CON_IDCONED:
      /* connected, write request */

      safe_snprintf(buf, MAX_STRING_LENGTH, "%d, %d\n\r", ntohs(d->peer_port), port);

      len = strlen(buf);
#ifdef CIRCLE_WINDOWS
      if (send(d->ident_sock, buf, len, 0) < 0) {
#else
      if (write(d->ident_sock, buf, len) != len) {
        if (errno != EPIPE) /* read end closed (no remote identd) */
#endif
          logerror("ident check write (conned)");

        STATE(d) = CON_ASKNAME;
        break;
      }

      STATE(d) = CON_IDREADING;
      break;

    case CON_IDREADING:
      /* waiting to read */

      if (d->ident_sock != INVALID_SOCKET) {
        FD_ZERO(&fd);
        FD_ZERO(&efd);
        FD_SET(d->ident_sock, &fd);
        FD_SET(d->ident_sock, &efd);
      }

      if ((rc = select(d->ident_sock + 1, &fd, (fd_set *) 0, &efd, &null_time)) == 0)
        break;

      else if (rc < 0) {
        logerror("ident check select (reading)");
        STATE(d) = CON_ASKNAME;
        break;
      }

      if (FD_ISSET(d->ident_sock, &efd)) {
        STATE(d) = CON_ASKNAME;
        break;
      }

      STATE(d) = CON_IDREAD;
      break;

    case CON_IDREAD:
      /* read ready, get the info */

#ifdef CIRCLE_WINDOWS
      if ((len = recv(d->ident_sock, buf, sizeof(buf) - 1, 0)) < 0)
#else
      if ((len = read(d->ident_sock, buf, sizeof(buf) - 1)) < 0)
#endif
        logerror("ident check read (read)");

      else {
        buf[len] = '\0';
        if (sscanf(buf, "%u , %u : USERID :%*[^:]:%255s", &rmt_port, &our_port, user) != 3) {

          /* check if error or malformed */
          if (sscanf(buf, "%u , %u : ERROR : %255s", &rmt_port, &our_port, user) == 3) {
            safe_snprintf(buf2, MAX_STRING_LENGTH, "Ident error from %s: \"%s\"", d->hostIP, user);
            stderr_log(buf2);
          } else {
            /* strip off trailing newline */
            for (p = buf + len - 1; p > buf && ISNEWL(*p); p--)
              ;
            p[1] = '\0';

            safe_snprintf(buf2, MAX_STRING_LENGTH, "Malformed ident response from %s: \"%s\"", d->hostIP, buf);
            stderr_log(buf2);
          }
        } else {

          len = HOST_LENGTH - strlen(d->hostIP);

          if (len > 0) {
            strncpy(buf2, user, len - 1);
            buf2[len - 1] = '\0';
            safe_snprintf(d->username, sizeof(d->username), "%s", buf2);
          }

          /* if len <= 0, no space for username */
        }
      }

      STATE(d) = CON_ASKNAME;
      break;

    case CON_ASKNAME:
      /* ident complete, ask for name */

      /* close up the ident socket, if one is opened. */
      if (d->ident_sock != INVALID_SOCKET) {
        close(d->ident_sock);
        d->ident_sock = INVALID_SOCKET;
      }
      d->idle_tics = 0;

      /* extra ban check */
      if ((d->host[0] != '\0' && isbanned(d->host) == BAN_ALL) || isbanned(d->hostIP)) {
        if (d->host[0] != '\0') {
          safe_snprintf(buf, MAX_STRING_LENGTH, "Connection attempt denied from [%s]", d->host);
        } else {
          safe_snprintf(buf, MAX_STRING_LENGTH, "Connection attempt denied from [%s]", d->hostIP);
        }
        mudlog(buf, 'S', COM_IMMORT, TRUE);
        close_socket(d);
        return;
      }

      /* SEND_TO_Q("\x1B[2K\n\rBy what name do you wish to be known? ", d); */
      STATE(d) = CON_GET_TERMTYPE;
      return;

    default:
      return;
  }

  /*
   * Print a dot every second so the user knows he hasn't been forgotten.
   * Allow the user to go on anyways after waiting IDENT_TIMEOUT seconds.
   */
  if ((pulse % PASSES_PER_SEC) == 0) {
    SEND_TO_Q(".", d);

    if (d->idle_tics++ >= IDENT_TIMEOUT)
      STATE(d) = CON_ASKNAME;
  }
}

/* returns 1 if waiting for ident to complete, else 0 */
int waiting_for_ident(struct descriptor_data *d)
{
  switch (STATE(d)) {
    case CON_IDCONING:
    case CON_IDCONED:
    case CON_IDREADING:
    case CON_IDREAD:
    case CON_ASKNAME:
      return 1;
  }
  return 0;
}
