/* ************************************************************************
 *  file:  autowiz.c                                     Part of CircleMUD *
 *  Usage: self-updating wizlists                                          *
 *  Written by Jeremy Elson                                                *
 *  All Rights Reserved                                                    *
 *  Copyright (C) 1993 The Trustees of The Johns Hopkins University        *
 ************************************************************************* */

/*
 WARNING:  THIS CODE IS A HACK.  WE CAN NOT AND WILL NOT BE RESPONSIBLE
 FOR ANY NASUEA, DIZZINESS, VOMITING, OR SHORTNESS OF BREATH RESULTING
 FROM READING THIS CODE.  PREGNANT WOMEN AND INDIVIDUALS WITH BACK
 INJURIES, HEART CONDITIONS, OR ARE UNDER THE CARE OF A PHYSICIAN SHOULD
 NOT READ THIS CODE.

 -- The Management
 */

#include "../db.h"
#include "../structs.h"
#include "../utils.h"
#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMM_LMARG "   "
#define IMM_NSIZE 16
#define LINE_LEN  64
#define MIN_LEVEL LVL_IMMORT

/* max level that should be in columns instead of centered */
#define COL_LEVEL LVL_IMMORT

struct name_rec {
  char name[25];
  struct name_rec *next;
};

struct control_rec {
  byte level;
  char *level_name;
};

struct level_rec {
  struct control_rec *params;
  struct level_rec *next;
  struct name_rec *names;
};

struct control_rec level_params[] = {
    {COM_IMMORT, "Immortals"}, {COM_QUEST, "Quest Managers"}, {COM_BUILDER, "Builders"}, {COM_ADMIN, "Admin"}, {0, ""}};

struct level_rec *levels = 0;

void initialize(void) {
  struct level_rec *tmp;
  byte i = 0;

  while (level_params[(int)i].level > 0) {
    tmp = (struct level_rec *)malloc(sizeof(struct level_rec));
    tmp->names = 0;
    tmp->params = &(level_params[(int)i++]);
    tmp->next = levels;
    levels = tmp;
  }
}

void read_file(void) {
  /*
  void add_name(int level, char *name);

  struct char_file_u player;
  FILE * fl;

  if (!(fl = fopen(PLAYER_FILE, "rb"))) {
    perror("Error opening playerfile");
    exit(1);
  }

  while (!feof(fl)) {
    fread(&player, sizeof(struct char_file_u), 1, fl);
    if (!feof(fl) && (player.player_specials_saved.commands & COM_IMMORT) && !(IS_SET(player.char_specials_saved.act,
  PLR_FROZEN)) && !(IS_SET(player.char_specials_saved.act, PLR_NOWIZLIST)) && !(IS_SET(player.char_specials_saved.act,
  PLR_DELETED))) add_name(player.player_specials_saved.commands, player.name);
  }

  fclose(fl);
  */
}

void add_name(int level, char *name) {
  struct name_rec *tmp;
  struct level_rec *curr_level;
  char *ptr;

  if (!*name)
    return;

  for (ptr = name; *ptr; ptr++)
    if (!isalpha(*ptr))
      return;

  tmp = (struct name_rec *)malloc(sizeof(struct name_rec));
  strcpy(tmp->name, name);
  tmp->next = 0;

  curr_level = levels;
  while (curr_level->params->level > level)
    curr_level = curr_level->next;

  tmp->next = curr_level->names;
  curr_level->names = tmp;
}

void sort_names(void) {
  struct level_rec *curr_level;
  struct name_rec *a, *b;
  char temp[100];

  for (curr_level = levels; curr_level; curr_level = curr_level->next) {
    for (a = curr_level->names; a && a->next; a = a->next) {
      for (b = a->next; b; b = b->next) {
        if (strcmp(a->name, b->name) > 0) {
          strcpy(temp, a->name);
          strcpy(a->name, b->name);
          strcpy(b->name, temp);
        }
      }
    }
  }
}

void write_wizlist(FILE *out, int minlev, int maxlev) {
  char buf[100];
  struct level_rec *curr_level;
  struct name_rec *curr_name;
  int i, j;

  fprintf(out, "{B=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-={x\n"
               "{b   These are the powerful beings of ShadowWind... Treat them with respect!{x\n"
               "{B=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-={x\n\n");

  for (curr_level = levels; curr_level; curr_level = curr_level->next) {
    if (curr_level->params->level < minlev || curr_level->params->level > maxlev)
      continue;
    i = 39 - (strlen(curr_level->params->level_name) >> 1);
    for (j = 1; j <= i; j++)
      fputc(' ', out);
    fprintf(out, "%s\n", curr_level->params->level_name);
    for (j = 1; j <= i; j++)
      fputc(' ', out);
    for (j = 1; j <= strlen(curr_level->params->level_name); j++)
      fputc('-', out);
    fprintf(out, "\n");

    strcpy(buf, "");
    curr_name = curr_level->names;
    while (curr_name) {
      strcat(buf, curr_name->name);
      if (strlen(buf) > LINE_LEN) {
        if (curr_level->params->level <= COL_LEVEL)
          fprintf(out, IMM_LMARG);
        else {
          i = 40 - (strlen(buf) >> 1);
          for (j = 1; j <= i; j++)
            fputc(' ', out);
        }
        fprintf(out, "%s\n", buf);
        strcpy(buf, "");
      } else {
        if (curr_level->params->level <= COL_LEVEL) {
          for (j = 1; j <= (IMM_NSIZE - strlen(curr_name->name)); j++)
            strcat(buf, " ");
        }
        if (curr_level->params->level > COL_LEVEL)
          strcat(buf, "   ");
      }
      curr_name = curr_name->next;
    }

    if (*buf) {
      if (curr_level->params->level <= COL_LEVEL)
        fprintf(out, "%s%s\n", IMM_LMARG, buf);
      else {
        i = 40 - (strlen(buf) >> 1);
        for (j = 1; j <= i; j++)
          fputc(' ', out);
        fprintf(out, "%s\n", buf);
      }
    }

    fprintf(out, "\n");
  }
}

void main(int argc, char **argv) {
  int wizlevel, immlevel, pid = 0;
  FILE *fl;

  if (argc != 5 && argc != 6) {
    printf("Format: %s wizlev wizlistfile immlev immlistfile [pid to signal]\n", argv[0]);
    exit(1);
  }

  wizlevel = atoi(argv[1]);
  immlevel = atoi(argv[3]);
  if (argc == 6)
    pid = atoi(argv[5]);
  initialize();
  read_file();
  sort_names();
  fl = fopen(argv[2], "w");
  write_wizlist(fl, wizlevel, LVL_IMMORT);
  fclose(fl);
  fl = fopen(argv[4], "w");
  write_wizlist(fl, immlevel, wizlevel - 1);
  if (pid)
    kill(pid, SIGUSR1);
}
