#include "types.h"
#include "stat.h"
#include "user.h"

int
main(void)
{
  int pid = getpid();
  int val = ticks_run(pid);
  printf(1, "PID %d has run %d ticks.\n", pid, val);
  volatile int i;
  for(i = 0; i < 100000000; i++){
  }

  val = ticks_run(pid);
  printf(1, "PID %d now has run %d ticks.\n", pid, val);

  exit();
}
