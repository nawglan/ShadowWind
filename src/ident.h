/* ************************************************************************
 *  File: ident.h                              				  *
 *                                                                         *
 *  Usage: Header file containing stuff required for rfc 931/1413 ident    *
 *         lookups                                                         *
 *                                                                         *
 *  Written by Eric Green (thrytis@imaxx.net)				  *
 ************************************************************************ */

void ident_start(struct descriptor_data *d, long addr);
void ident_check(struct descriptor_data *d, int pulse);
int waiting_for_ident(struct descriptor_data *d);

extern int ident;

#ifndef IDBUFSIZE
#define IDBUFSIZE 2048
#endif

#ifndef IDPORT
#define IDPORT 113
#endif

typedef struct {
  int fd;
  char buf[IDBUFSIZE];
} ident_t;

typedef struct {
  int lport;        /* Local port */
  int fport;        /* Far (remote) port */
  char *identifier; /* Normally user name */
  char *opsys;      /* OS */
  char *charset;    /* Charset (what did you expect?) */
} IDENT;            /* For higher-level routines */

/* Low-level calls and macros */
#define id_fileno(ID) ((ID)->fd)

/* High-level calls */
extern char id_version[];
