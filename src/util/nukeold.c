/******************************************************************************/
/* NAME           :   nukeold.c                                               */
/*                                                                            */
/* PURPOSE        :   Deletes player files after they have gone inactive.     */
/*                                                                            */
/* CONTAINS       :   main()                                                  */
/*                                                                            */
/* EXTERNAL CALLS :   None.                                                   */
/*                                                                            */
/* LIMITATIONS :      None.                                                   */
/*                                                                            */
/*                                                                            */
/* AUTHOR  :           Desmond Daignault                                      */
/*                                                                            */
/*----------------------------------------------------------------------------*/
/*                          Maintainance Log                                  */
/*           ( Plaese maintain a detailed description of changes )            */
/*                                                                            */
/*  Date      Prgm     LineNo                 Description                     */
/*  19980515  ddaigna                         Created.                        */
/*                                                                            */
/******************************************************************************/

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[]) {

  DIR *dirp;              /* Directory pointer used to read the directory */
  struct dirent *idirent; /* Entry struct in the directory */
  struct stat filestat;   /* contains information about the file */
  char filename[1024];    /* used to store the filename in the directory */
  time_t currtime;        /* holds the current time */

  /* ensure that the minimum number of parameters have been supplied */
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <in dir>\n", argv[0]);
    exit(1);
  }

  fileext[0] = '\0';
  newext[0] = '\0';

  /* ensure that we can read from the directory */
  if ((dirp = opendir(argv[1])) == NULL) {
    fprintf(stderr, "Error opening directory: %s\n", argv[1]);
    exit(1);
  }

  /* make the output directory if it does not exist */
  if (stat(argv[2], &filestat)) {
    if (mkdir(argv[2], 00777) == -1) {
      fprintf(stderr, "Unable to create directory %s\n", argv[2]);
      exit(1);
    }
  }

  /* get today's date */
  currtime = time(NULL);
  cftime(cdate, "%Y%m%d", &currtime);

  /* read the directory */
  while ((idirent = readdir(dirp))) {
    /* skip . and .. and anything with a null name */
    if ((strcmp(idirent->d_name, ".") == 0) || (strcmp(idirent->d_name, "..") == 0) || idirent->d_name[0] == '\0')
      continue;

    sprintf(filename, "%s/%s", argv[1], idirent->d_name);

    /* retrieve the information on the file */
    stat(filename, &filestat);

    /* format the modified time for a YYYYMMDD string */
    cftime(fdate, "%Y%m%d", &filestat.st_mtime);

    /* check if this file has the correct modified date, if dates match */
    /* copy the file to the archive directory */

    if (strcmp(fdate, cdate) == 0) {
      sprintf(systemcmd, "cp %s/%s %s/", argv[1], idirent->d_name, argv[2]);

      /* replace the extention with the new extention */
      if (argc == 4) {
        strcpy(filename, idirent->d_name);
        p = strrchr(filename, fileext[0]);
        if (strcmp(p, fileext) == 0)
          strcpy(p, newext);
        sprintf(systemcmd, "%s%s", systemcmd, filename);
      } else {
        sprintf(systemcmd, "%s%s", systemcmd, idirent->d_name);
      }

      /* issue the cp command */
      system(systemcmd);
    }
  }

  closedir(dirp);
  return 0;
}
