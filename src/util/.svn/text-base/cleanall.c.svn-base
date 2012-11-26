#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#define MOBDIR "lib/world/mob/"
#define OBJDIR "lib/world/obj/"
#define ZONDIR "lib/world/zon/"
#define WLDDIR "lib/world/wld/"
#define SHPDIR "lib/world/shp/"

int main(void)
{
  FILE* index;
  FILE* input;
  FILE* output;
  char filename[100];
  char newfilename[100];
  char buf[900];
  char tmp;

  strcpy(filename, MOBDIR);
  strcat(filename, "index");
  if ((index = fopen(filename, "r")) == NULL) {
    printf("Could not open %s.", filename);
    exit(1);
  }
  while ((fscanf(index, "%s", buf) != EOF) && buf[0] != '$') {
    strcpy(filename, MOBDIR);
    strcat(filename, buf);
    input = fopen(filename, "r");
    strcpy(newfilename, MOBDIR);
    strcat(newfilename, buf);
    strcat(newfilename, ".new");
    output = fopen(newfilename, "w");
    while (fscanf(input, "%c", &tmp) != EOF) {
      if (tmp == '\r')
        tmp = tmp;
      else
        fprintf(output, "%c", tmp);
    }
    fclose(output);
    fclose(input);
    strcpy(buf, "mv ");
    strcat(buf, newfilename);
    strcat(buf, " ");
    strcat(buf, filename);
    system(buf);
  }
  fclose(index);

  strcpy(filename, OBJDIR);
  strcat(filename, "index");
  if ((index = fopen(filename, "r")) == NULL) {
    printf("Could not open %s.", filename);
    exit(1);
  }
  while ((fscanf(index, "%s", buf) != EOF) && buf[0] != '$') {
    strcpy(filename, OBJDIR);
    strcat(filename, buf);
    input = fopen(filename, "r");
    strcpy(newfilename, OBJDIR);
    strcat(newfilename, buf);
    strcat(newfilename, ".new");
    output = fopen(newfilename, "w");
    while (fscanf(input, "%c", &tmp) != EOF) {
      if (tmp == '\r')
        tmp = tmp;
      else
        fprintf(output, "%c", tmp);
    }
    fclose(input);
    fclose(output);
    strcpy(buf, "mv ");
    strcat(buf, newfilename);
    strcat(buf, " ");
    strcat(buf, filename);
    system(buf);
  }
  fclose(index);

  strcpy(filename, WLDDIR);
  strcat(filename, "index");
  if ((index = fopen(filename, "r")) == NULL) {
    printf("Could not open %s.", filename);
    exit(1);
  }
  while ((fscanf(index, "%s", buf) != EOF) && buf[0] != '$') {
    strcpy(filename, WLDDIR);
    strcat(filename, buf);
    input = fopen(filename, "r");
    strcpy(newfilename, WLDDIR);
    strcat(newfilename, buf);
    strcat(newfilename, ".new");
    output = fopen(newfilename, "w");
    while (fscanf(input, "%c", &tmp) != EOF) {
      if (tmp == '\r')
        tmp = tmp;
      else
        fprintf(output, "%c", tmp);
    }
    fclose(input);
    fclose(output);
    strcpy(buf, "mv ");
    strcat(buf, newfilename);
    strcat(buf, " ");
    strcat(buf, filename);
    system(buf);
  }
  fclose(index);

  strcpy(filename, ZONDIR);
  strcat(filename, "index");
  if ((index = fopen(filename, "r")) == NULL) {
    printf("Could not open %s.", filename);
    exit(1);
  }
  while ((fscanf(index, "%s", buf) != EOF) && buf[0] != '$') {
    strcpy(filename, ZONDIR);
    strcat(filename, buf);
    input = fopen(filename, "r");
    strcpy(newfilename, ZONDIR);
    strcat(newfilename, buf);
    strcat(newfilename, ".new");
    output = fopen(newfilename, "w");
    while (fscanf(input, "%c", &tmp) != EOF) {
      if (tmp == '\r')
        tmp = tmp;
      else
        fprintf(output, "%c", tmp);
    }
    fclose(input);
    fclose(output);
    strcpy(buf, "mv ");
    strcat(buf, newfilename);
    strcat(buf, " ");
    strcat(buf, filename);
    system(buf);
  }
  fclose(index);

  strcpy(filename, SHPDIR);
  strcat(filename, "index");
  if ((index = fopen(filename, "r")) == NULL) {
    printf("Could not open %s.", filename);
    exit(1);
  }
  while ((fscanf(index, "%s", buf) != EOF) && buf[0] != '$') {
    strcpy(filename, SHPDIR);
    strcat(filename, buf);
    input = fopen(filename, "r");
    strcpy(newfilename, SHPDIR);
    strcat(newfilename, buf);
    strcat(newfilename, ".new");
    output = fopen(newfilename, "w");
    while (fscanf(input, "%c", &tmp) != EOF) {
      if (tmp == '\r')
        tmp = tmp;
      else
        fprintf(output, "%c", tmp);
    }
    fclose(input);
    fclose(output);
    strcpy(buf, "mv ");
    strcat(buf, newfilename);
    strcat(buf, " ");
    strcat(buf, filename);
    system(buf);
  }
  fclose(index);

  return 0;
}
