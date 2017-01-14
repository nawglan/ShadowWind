#define WNORTH 0x80
#define WEAST 0x40
#define WSOUTH 0x20
#define WWEST 0x10
#define DONORTH 0x8
#define DOEAST 0x4
#define DOSOUTH 0x2
#define DOWEST 0x1
#define DOANY 0xF

typedef struct {
  int x;
  int y;
} path;

typedef struct {
  int path[10][10];
  path saved_path[100];
  path solve_path[100];
  int x;
  int y;
  int lastdir;
  int step;
} maze;
