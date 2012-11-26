#include <stdio.h>

int main(int argc, char *argv[])
{
  FILE *f;
  FILE *o;
  char buf[256];
  char sjunk[256];
  int ijunk = 0;
  int exp = 0;
  int found = 0;
  char X;

  if (argc < 2) {
    fprintf(stderr, "Usage: %s <mobfile>\n", argv[0]);
    exit(1);
  }

  f = fopen(argv[1], "r");
  if (!f) {
    fprintf(stderr, "Error: %s doesn't exist.\n", argv[1]);
    exit(1);
  }

  sprintf(buf, "%s.tmp", argv[1]);
  o = fopen(buf, "w");

  fgets(buf, 80, f);
  while (!feof(f)) {
    buf[strlen(buf) - 1] = '\0';
    if (buf[0] == '#')
      found = 1;
    if (found && (sscanf(buf, "%s %s %d %c", sjunk, sjunk, &ijunk, &X) == 4) && X == 'X') {
      found = 0;
      fprintf(o, "%s\n", buf);
      fgets(buf, 80, f);
      buf[strlen(buf) - 1] = '\0';
      fprintf(o, "%s\n", buf);
      fgets(buf, 80, f);
      buf[strlen(buf) - 1] = '\0';
      sscanf(buf, "%d %d", &ijunk, &exp);
      fprintf(o, "0 %d\n", exp);
    } else
      fprintf(o, "%s\n", buf);
    fgets(buf, 80, f);
  }

  return 0;
}
