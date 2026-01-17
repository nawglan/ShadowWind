#include <sys/time.h>
#include <time.h>

/*
 * I've gotten too many bad reports on OS-supplied random number generators.
 * This is the Mitchell-Moore algorithm from Knuth Volume II.
 * Best to leave the constants alone unless you've read Knuth.
 * -- Furey
 */
static int rgiState[2 + 55];

void init_mm() {
  int *piState;
  int iState;
  struct timeval last_time;
  int current_time;

  gettimeofday(&last_time, 0);
  current_time = last_time.tv_sec;
  piState = &rgiState[2];

  piState[-2] = 55 - 55;
  piState[-1] = 55 - 24;

  piState[0] = ((int)current_time) & ((1 << 30) - 1);
  piState[1] = 1;
  for (iState = 2; iState < 55; iState++) {
    piState[iState] = (piState[iState - 1] + piState[iState - 2]) & ((1 << 30) - 1);
  }
  return;
}

int number_mm(void) {
  int *piState;
  int iState1;
  int iState2;
  int iRand;

  piState = &rgiState[2];
  iState1 = piState[-2];
  iState2 = piState[-1];
  iRand = (piState[iState1] + piState[iState2]) & ((1 << 30) - 1);
  piState[iState1] = iRand;
  if (++iState1 == 55)
    iState1 = 0;
  if (++iState2 == 55)
    iState2 = 0;
  piState[-2] = iState1;
  piState[-1] = iState2;
  return iRand >> 6;
}

/*
 * Generate a random number.
 */
int number_range(int from, int to) {
  int power;
  int number;

  if ((to = to - from + 1) <= 1)
    return from;

  for (power = 2; power < to; power <<= 1)
    ;

  while ((number = number_mm() & (power - 1)) >= to)
    ;

  return from + number;
}

/*
 * Generate a percentile roll.
 */
int number_percent(void) {
  int percent;

  while ((percent = number_mm() & (128 - 1)) > 99)
    ;

  return 1 + percent;
}
