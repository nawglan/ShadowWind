#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include "structs.h"
#include "db.h"
#include "utils.h"
#include "maze.h"

#define TOP_WALL 0
#define BOTTOM_WALL 1
#define OTHER_WALL 1

void create_rooms(maze * m, char log_filename[80]);
void create_maze(maze * m);
void print_maze(maze * m, FILE * f);
int choose_exit(maze * m);
int place_wall(maze * m);
int backup(maze * m);
int boot_mazes();
void reboot_maze(maze * m, int zone_num);
void free_dir(struct room_data * world, int i, int dir);
void free_wall(struct room_data * world, int i, int wall);

void free_dir(struct room_data * world, int i, int dir)
{
  if (world[i].dir_option[dir]->general_description) {
    FREE(world[i].dir_option[dir]->general_description);
  }
  if (world[i].dir_option[dir]->keyword) {
    FREE(world[i].dir_option[dir]->keyword);
  }
  FREE(world[i].dir_option[dir]);
  world[i].dir_option[dir] = NULL;
}

void free_wall(struct room_data * world, int i, int wall)
{
  int x;

  if (wall == TOP_WALL) {
    for (x = 0; x < 9; x++) {
      if (x == 0) {
        if (world[i].dir_option[1]) {
          free_dir(world, i, 1);
        }
        if (world[i].dir_option[2]) {
          free_dir(world, i, 2);
        }
      } else if (x == 9) {
        if (world[i].dir_option[2]) {
          free_dir(world, i, 2);
        }
        if (world[i].dir_option[3]) {
          free_dir(world, i, 3);
        }
      } else {
        if (world[i].dir_option[1]) {
          free_dir(world, i, 1);
        }
        if (world[i].dir_option[2]) {
          free_dir(world, i, 2);
        }
        if (world[i].dir_option[3]) {
          free_dir(world, i, 3);
        }
      }
    }
    /* bottom wall of maze */
  } else if (wall == BOTTOM_WALL) {
    for (x = 0; x < 9; x++) {
      if (x == 0) {
        if (world[i].dir_option[0]) {
          free_dir(world, i, 0);
        }
        if (world[i].dir_option[1]) {
          free_dir(world, i, 1);
        }
      } else if (x == 9) {
        if (world[i].dir_option[0]) {
          free_dir(world, i, 0);
        }
        if (world[i].dir_option[3]) {
          free_dir(world, i, 3);
        }
      } else {
        if (world[i].dir_option[0]) {
          free_dir(world, i, 0);
        }
        if (world[i].dir_option[1]) {
          free_dir(world, i, 1);
        }
        if (world[i].dir_option[3]) {
          free_dir(world, i, 3);
        }
      }
    }
    /* all others */
  } else {
    for (x = 0; x < 9; x++) {
      if (x == 0) {
        if (world[i].dir_option[0]) {
          free_dir(world, i, 0);
        }
        if (world[i].dir_option[1]) {
          free_dir(world, i, 1);
        }
        if (world[i].dir_option[2]) {
          free_dir(world, i, 2);
        }
      } else if (x == 9) {
        if (world[i].dir_option[0]) {
          free_dir(world, i, 0);
        }
        if (world[i].dir_option[2]) {
          free_dir(world, i, 2);
        }
        if (world[i].dir_option[3]) {
          free_dir(world, i, 3);
        }
      } else {
        if (world[i].dir_option[0]) {
          free_dir(world, i, 0);
        }
        if (world[i].dir_option[1]) {
          free_dir(world, i, 1);
        }
        if (world[i].dir_option[2]) {
          free_dir(world, i, 2);
        }
        if (world[i].dir_option[3]) {
          free_dir(world, i, 3);
        }
      }
    }
  }
}

void reboot_maze(maze * m, int zone_num)
{
  int room_nr = 0;
  int zone_number = 0;
  int i = 0;
  int x = 0;
  int y = 0;
  extern struct room_data *world;
  extern int top_of_world;
  extern struct zone_data *zone_table;
  extern int top_of_zone_table;
  int start_room = 0;

  for (i = 0; i <= top_of_zone_table; i++) {
    if (zone_table[i].number == zone_num) {
      zone_number = i;
    }
  }

  for (room_nr = 0; room_nr <= top_of_world; room_nr++) {
    if (world[room_nr].zone == zone_number) {
      break;
    }
  }

  start_room = room_nr;

  for (i = 0; i <= top_of_world; i++) {
    if (world[i].zone == zone_number) {
      if ((world[i].number % 100) == 0) {
        free_wall(world, i, TOP_WALL);
      } else if ((world[i].number % 100) == 90) {
        free_wall(world, i, BOTTOM_WALL);
      } else {
        free_wall(world, i, OTHER_WALL);
      }
    }
  }

  room_nr = start_room;

  for (y = 0; y < 10; y++) {
    for (x = 0; x < 10; x++) {
      if (m->path[x][y] & DONORTH) {
        if (!world[room_nr].dir_option[0]) {
          CREATE(world[room_nr].dir_option[0], struct room_direction_data, 1);
        }
        world[room_nr].dir_option[0]->to_room = real_room(world[room_nr].number - 10);
        world[room_nr].dir_option[0]->to_room_vnum = world[room_nr].number - 10;
        world[room_nr].dir_option[0]->key = -1;
        world[room_nr].dir_option[0]->exit_info = 0;
      }
      if (m->path[x][y] & DOEAST) {
        if (!world[room_nr].dir_option[1]) {
          CREATE(world[room_nr].dir_option[1], struct room_direction_data, 1);
        }
        world[room_nr].dir_option[1]->to_room = real_room(world[room_nr].number + 1);
        world[room_nr].dir_option[1]->to_room_vnum = world[room_nr].number + 1;
        world[room_nr].dir_option[1]->key = -1;
        world[room_nr].dir_option[1]->exit_info = 0;
      }
      if (m->path[x][y] & DOSOUTH) {
        if (!world[room_nr].dir_option[2]) {
          CREATE(world[room_nr].dir_option[2], struct room_direction_data, 1);
        }
        world[room_nr].dir_option[2]->to_room = real_room(world[room_nr].number + 10);
        world[room_nr].dir_option[2]->to_room_vnum = world[room_nr].number + 10;
        world[room_nr].dir_option[2]->key = -1;
        world[room_nr].dir_option[2]->exit_info = 0;
      }
      if (m->path[x][y] & DOWEST) {
        if (!world[room_nr].dir_option[3]) {
          CREATE(world[room_nr].dir_option[3], struct room_direction_data, 1);
        }
        world[room_nr].dir_option[3]->to_room = real_room(world[room_nr].number - 1);
        world[room_nr].dir_option[3]->to_room_vnum = world[room_nr].number - 1;
        world[room_nr].dir_option[3]->key = -1;
        world[room_nr].dir_option[3]->exit_info = 0;
      }
      room_nr++;
    }
  }
}

void create_rooms(maze * m, char log_filename[80])
{
  int room_nr = 0;
  int zone_number = 0;
  int i = 0;
  int x = 0;
  int y = 0;
  struct room_data *new_world;
  extern struct room_data *world;
  extern int top_of_world;
  extern struct zone_data *zone_table;
  extern int top_of_zone_table;
  char buf[80];
  int room_count = 0;
  int found = 0;
  int start_room = 0;

  zone_number = atoi(log_filename);

  snprintf(buf, MAX_STRING_LENGTH, "   Building maze #%d", zone_number);
  stderr_log(buf);

  for (i = 0; i <= top_of_zone_table; i++) {
    if (((zone_table[i].top - (zone_table[i].top % 100)) / 100) == zone_number) {
      zone_number = i;
      found = 1;
    }
  }

  if (!found) {
    snprintf(buf, MAX_STRING_LENGTH, "SYSERR: Could not find zone #%d", zone_number);
    stderr_log(buf);
    fflush(NULL);
    exit(1);
  }

  for (i = 0; i <= top_of_world; i++) {
    if (world[i].zone == zone_number) {
      room_count++;
    }
  }

  if (room_count < 3) {
    snprintf(buf, MAX_STRING_LENGTH, "SYSERR: Maze %s only has %d rooms.", log_filename, room_count);
    stderr_log(buf);
    fflush(NULL);
    exit(1);
  }

  for (room_nr = 0; room_nr <= top_of_world; room_nr++) {
    if (world[room_nr].zone > zone_number) {
      break;
    }
  }

  start_room = world[room_nr - room_count].number;

  if (room_count < 100) {
    CREATE(new_world, struct room_data, ((100 - room_count) + top_of_world) + 2);
    /* insert rooms */

    room_nr--;

    /* copy all rooms before last room */
    for (i = 0; i < room_nr; i++) {
      new_world[i] = world[i];
    }

    /* these are the new rooms */
    for (i = 0; i < (100 - room_count); i++) {
      new_world[i + room_nr].zone = new_world[i + room_nr - 1].zone;
      new_world[i + room_nr].number = start_room + room_count + i - 1;
      new_world[i + room_nr].name = strdup(new_world[i + room_nr - 1].name);
      new_world[i + room_nr].description = strdup(new_world[i + room_nr - 1].description);
      new_world[i + room_nr].room_flags = new_world[i + room_nr - 1].room_flags;
      new_world[i + room_nr].sector_type = new_world[i + room_nr - 1].sector_type;
      new_world[i + room_nr].func = NULL;
      new_world[i + room_nr].contents = NULL;
      new_world[i + room_nr].people = NULL;
      new_world[i + room_nr].light = 0;
      new_world[i + room_nr].ex_description = NULL;

      for (x = 0; x < NUM_OF_DIRS; x++) {
        new_world[i + room_nr].dir_option[x] = NULL;
      }
    }

    /* copy the rest of the rooms */
    for (i = room_nr; i <= top_of_world; i++) {
      new_world[i + (100 - room_count)] = world[i];
    }

    top_of_world += (100 - room_count);
    FREE(world);
    world = new_world;
  }
  for (room_nr = 0; room_nr <= top_of_world; room_nr++) {
    if (world[room_nr].zone == zone_number) {
      break;
    }
  }

  for (i = 0; i <= top_of_world; i++) {
    if (world[i].zone == zone_number) {
      if ((world[i].number % 100) == 0) {
        free_wall(world, i, TOP_WALL);
      } else if ((world[i].number % 100) == 90) {
        free_wall(world, i, BOTTOM_WALL);
      } else {
        free_wall(world, i, OTHER_WALL);
      }
    }
  }

  for (i = 0; i < 100; i++) {
    world[room_nr + i].number = start_room + i;
  }

  for (y = 0; y < 10; y++) {
    for (x = 0; x < 10; x++) {
      if (m->path[x][y] & DONORTH) {
        if (!world[room_nr].dir_option[0]) {
          CREATE(world[room_nr].dir_option[0], struct room_direction_data, 1);
        }
        world[room_nr].dir_option[0]->to_room = world[room_nr].number - 10;
        world[room_nr].dir_option[0]->key = -1;
        world[room_nr].dir_option[0]->exit_info = 0;
      }
      if (m->path[x][y] & DOEAST) {
        if (!world[room_nr].dir_option[1]) {
          CREATE(world[room_nr].dir_option[1], struct room_direction_data, 1);
        }
        world[room_nr].dir_option[1]->to_room = world[room_nr].number + 1;
        world[room_nr].dir_option[1]->key = -1;
        world[room_nr].dir_option[1]->exit_info = 0;
      }
      if (m->path[x][y] & DOSOUTH) {
        if (!world[room_nr].dir_option[2]) {
          CREATE(world[room_nr].dir_option[2], struct room_direction_data, 1);
        }
        world[room_nr].dir_option[2]->to_room = world[room_nr].number + 10;
        world[room_nr].dir_option[2]->key = -1;
        world[room_nr].dir_option[2]->exit_info = 0;
      }
      if (m->path[x][y] & DOWEST) {
        if (!world[room_nr].dir_option[3]) {
          CREATE(world[room_nr].dir_option[3], struct room_direction_data, 1);
        }
        world[room_nr].dir_option[3]->to_room = world[room_nr].number - 1;
        world[room_nr].dir_option[3]->key = -1;
        world[room_nr].dir_option[3]->exit_info = 0;
      }
      room_nr++;
    }
  }
}

void create_maze(maze * m)
{
  int i;
  int j;
  struct timeval t;
  int c_time;

  m->x = 0;
  m->y = 0;
  m->lastdir = 0;
  m->step = 0;

  gettimeofday(&t, 0);
  c_time = t.tv_sec;
  srand(c_time);
  for (i = 0; i < 1000; i++) {
    rand();
  }

  for (i = 0; i < 10; i++) {
    for (j = 0; j < 10; j++) {
      m->path[i][j] = 0;
    }
  }

  for (i = 0; i < 100; i++) {
    m->saved_path[i].x = 0;
    m->saved_path[i].y = 0;
    m->solve_path[i].x = 0;
    m->solve_path[i].y = 0;
  }

  /* top wall */
  for (j = 0; j < 10; j++) {
    m->path[j][0] |= WNORTH;
  }

  /* bottom wall */
  for (j = 0; j < 10; j++) {
    m->path[j][9] |= WSOUTH;
  }

  /* left wall */
  for (i = 0; i < 10; i++) {
    m->path[0][i] |= WWEST;
  }

  /* right wall */
  for (i = 0; i < 10; i++) {
    m->path[9][i] |= WEAST;
  }

  for (;;) {
    m->saved_path[m->step].x = m->x;
    m->saved_path[m->step].y = m->y;

    while ((m->lastdir = choose_exit(m)) == -1) {
      if (backup(m) == -1)
        return;
    }

    switch (m->lastdir) {
      case 0:
        m->path[m->x][m->y] |= DONORTH;
        m->y--;
        m->path[m->x][m->y] |= DOSOUTH;
        break;
      case 1:
        m->path[m->x][m->y] |= DOEAST;
        m->x++;
        m->path[m->x][m->y] |= DOWEST;
        break;
      case 2:
        m->path[m->x][m->y] |= DOSOUTH;
        m->y++;
        m->path[m->x][m->y] |= DONORTH;
        break;
      case 3:
        m->path[m->x][m->y] |= DOWEST;
        m->x--;
        m->path[m->x][m->y] |= DOEAST;
        break;
    }
    m->step++;
    if (m->x == 9 && m->y == 9) {
      int z;

      for (z = 0; z < m->step; z++) {
        m->solve_path[z].x = m->saved_path[z].x;
        m->solve_path[z].y = m->saved_path[z].y;
      }
      m->solve_path[m->step].x = 9;
      m->solve_path[m->step].y = 9;
    }
  }
}

int backup(maze * m)
{
  m->step--;
  if (m->step >= 0) {
    m->x = m->saved_path[m->step].x;
    m->y = m->saved_path[m->step].y;
  }
  return (m->step);
}

int choose_exit(maze * m)
{
  int choice[3];
  int num_choice = 0;

  if (!(m->path[m->x][m->y] & WNORTH) && !(m->path[m->x][m->y] & DONORTH)) {
    if (m->path[m->x][m->y - 1] & DOANY) {
      m->path[m->x][m->y] |= WNORTH;
      m->path[m->x][m->y - 1] |= WSOUTH;
    } else {
      choice[num_choice++] = 0;
    }
  }
  if (!(m->path[m->x][m->y] & WEAST) && !(m->path[m->x][m->y] & DOEAST)) {
    if (m->path[m->x + 1][m->y] & DOANY) {
      m->path[m->x][m->y] |= WEAST;
      m->path[m->x + 1][m->y] |= WWEST;
    } else {
      choice[num_choice++] = 1;
    }
  }
  if (!(m->path[m->x][m->y] & WSOUTH) && !(m->path[m->x][m->y] & DOSOUTH)) {
    if (m->path[m->x][m->y + 1] & DOANY) {
      m->path[m->x][m->y] |= WSOUTH;
      m->path[m->x][m->y + 1] |= WNORTH;
    } else {
      choice[num_choice++] = 2;
    }
  }
  if (!(m->path[m->x][m->y] & WWEST) && !(m->path[m->x][m->y] & DOWEST)) {
    if (m->path[m->x - 1][m->y] & DOANY) {
      m->path[m->x][m->y] |= WWEST;
      m->path[m->x - 1][m->y] |= WEAST;
    } else {
      choice[num_choice++] = 3;
    }
  }
  if (num_choice == 0) {
    return -1;
  } else if (num_choice == 1) {
    return choice[0];
  }
  return (choice[rand() % num_choice]);
}

void print_maze(maze * m, FILE * f)
{
  int x;
  int y;
  int i;

  for (y = 0; y < 10; y++) {
    for (x = 0; x < 10; x++) {
      if (m->path[x][y] & WNORTH) {
        m->path[x][y] ^= WNORTH;
      }
      if (m->path[x][y] & WEAST) {
        m->path[x][y] ^= WEAST;
      }
      if (m->path[x][y] & WSOUTH) {
        m->path[x][y] ^= WSOUTH;
      }
      if (m->path[x][y] & WWEST) {
        m->path[x][y] ^= WWEST;
      }
      fprintf(f, "%4d", m->path[x][y]);
    }
    fprintf(f, "\n");
  }

  fprintf(f, "\nCan go W = %d\n", (DOWEST));
  fprintf(f, "Can go S = %d\n", (DOSOUTH));
  fprintf(f, "Can go S, W = %d\n", (DOSOUTH | DOWEST));
  fprintf(f, "Can go E = %d\n", (DOEAST));
  fprintf(f, "Can go E, W = %d\n", (DOWEST | DOEAST));
  fprintf(f, "Can go E, S = %d\n", (DOSOUTH | DOEAST));
  fprintf(f, "Can go E, S, W = %d\n", (DOEAST | DOWEST | DOSOUTH));
  fprintf(f, "Can go N = %d\n", (DONORTH));
  fprintf(f, "Can go N, W = %d\n", (DONORTH | DOWEST));
  fprintf(f, "Can go N, S = %d\n", (DONORTH | DOSOUTH));
  fprintf(f, "Can go N, S, W = %d\n", (DONORTH | DOWEST | DOSOUTH));
  fprintf(f, "Can go N, E = %d\n", (DONORTH | DOEAST));
  fprintf(f, "Can go N, E, W = %d\n", (DONORTH | DOEAST | DOWEST));
  fprintf(f, "Can go N, E, S = %d\n", (DONORTH | DOEAST | DOSOUTH));
  fprintf(f, "Can go N, E, S, W = %d\n\n", (DOANY));

  for (i = 0; i < 100; i++) {
    if (m->solve_path[i].x != 9 || m->solve_path[i].y != 9) {
      if (i == 0) {
        fprintf(f, "%d) Start\n", i);
      }
      if ((m->solve_path[i].x == m->solve_path[i - 1].x) && (m->solve_path[i].y < m->solve_path[i - 1].y)) {
        fprintf(f, "%d) North\n", i);
      }
      if ((m->solve_path[i].x > m->solve_path[i - 1].x) && (m->solve_path[i].y == m->solve_path[i - 1].y)) {
        fprintf(f, "%d) East\n", i);
      }
      if ((m->solve_path[i].x == m->solve_path[i - 1].x) && (m->solve_path[i].y > m->solve_path[i - 1].y)) {
        fprintf(f, "%d) South\n", i);
      }
      if ((m->solve_path[i].x < m->solve_path[i - 1].x) && (m->solve_path[i].y == m->solve_path[i - 1].y)) {
        fprintf(f, "%d) West\n", i);
      }
    } else {
      break;
    }
  }

  if ((m->solve_path[i].x == m->solve_path[i - 1].x) && (m->solve_path[i].y < m->solve_path[i - 1].y)) {
    fprintf(f, "%d) North\n", i);
  }
  if ((m->solve_path[i].x > m->solve_path[i - 1].x) && (m->solve_path[i].y == m->solve_path[i - 1].y)) {
    fprintf(f, "%d) East\n", i);
  }
  if ((m->solve_path[i].x == m->solve_path[i - 1].x) && (m->solve_path[i].y > m->solve_path[i - 1].y)) {
    fprintf(f, "%d) South\n", i);
  }
  if ((m->solve_path[i].x < m->solve_path[i - 1].x) && (m->solve_path[i].y == m->solve_path[i - 1].y)) {
    fprintf(f, "%d) West\n", i);
  }
  fprintf(f, "%d) Done\n", i + 1);
  fclose(f);
}

int boot_mazes(void)
{
  FILE *index = NULL;
  FILE *f = NULL;
  char index_filename[80];
  char log_filename[160];
  char temp[160];
  maze m;

  sprintf(index_filename, "%s/%s", MAZE_PREFIX, INDEX_FILE);

  index = fopen(index_filename, "r");
  if (index == NULL) {
    stderr_log("SYSERR: unable to read maze index.");
    fflush(NULL);
    exit(1);
  }
  fgets(temp, 80, index);
  while (!feof(index)) {

    if (temp[0] == '$') {
      fclose(index);
      if (f) {
        fclose(f);
      }
      return 0;
    }
    temp[strlen(temp) - 1] = '\0';
    snprintf(log_filename, sizeof(log_filename), "%s/%s", MAZE_PREFIX, temp);

    f = fopen(log_filename, "w");
    if (f == NULL) {
      snprintf(temp, sizeof(temp), "SYSERR: unable to create maze log %s.", log_filename);
      stderr_log(temp);
      fflush(NULL);
      exit(1);
    }
    create_maze(&m);
    print_maze(&m, f);
    create_rooms(&m, temp);
    fgets(temp, 80, index);
    sleep(1);
  }

  fclose(index);
  return 0;
}
