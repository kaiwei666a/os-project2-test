// #include "types.h"
// #include "stat.h"
// #include "user.h"

// int
// main(void)
// {
//   int pid = getpid();
//   int val = ticks_run(pid);
//   printf(1, "PID %d has run %d ticks.\n", pid, val);
//   volatile int i;
//   for(i = 0; i < 100000000; i++){
//   }

//   val = ticks_run(pid);
//   printf(1, "PID %d now has run %d ticks.\n", pid, val);

//   exit();
// }


#include "types.h"
#include "stat.h"
#include "user.h"

int
main(void)
{
  printf(1, "\n=== Test 1: ticks_run for the calling process ===\n");
  int pid = getpid();
  int val = ticks_run(pid);
  printf(1, "Initial: ticks_run(%d) = %d\n", pid, val);

  volatile int i;
  for(i = 0; i < 100000000; i++) {
  }

  val = ticks_run(pid);
  printf(1, "After busy loop: ticks_run(%d) = %d\n", pid, val);

  printf(1, "\n=== Test 2: ticks_run for any process ID ===\n");
  int child_pid = fork();
  if(child_pid < 0) {
    printf(1, "Error: fork failed\n");
    exit();
  } else if(child_pid == 0) {

    for(i = 0; i < 100000000; i++) {
    }
    exit();
  } else {

    sleep(100);
    int child_ticks = ticks_run(child_pid);
    printf(1, "Child's ticks_run(%d) while running: %d\n", child_pid, child_ticks);
    wait(); 
  }

  printf(1, "\n=== Test 3: ticks_run returns 0 for an unscheduled process ===\n");
  int child2_pid = fork();
  if(child2_pid < 0) {
    printf(1, "Error: fork failed\n");
    exit();
  } else if(child2_pid == 0) {
    exit();
  } else {

    int child2_ticks = ticks_run(child2_pid);
    printf(1, "Child2's ticks_run(%d) immediately after fork: %d\n", child2_pid, child2_ticks);
    wait();
  }


  printf(1, "\n=== Test 4: ticks_run returns -1 for non-existent process ===\n");
  int invalid_pid = 99999; 
  int val_invalid = ticks_run(invalid_pid);
  printf(1, "ticks_run(%d) = %d (expected -1)\n", invalid_pid, val_invalid);

  exit();
}
