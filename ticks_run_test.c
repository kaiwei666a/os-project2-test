#include "types.h"
#include "stat.h"
#include "user.h"

// Function to prevent compiler optimization
volatile int dummy = 0;

void busy_work(int iterations) {
    for(int i = 0; i < iterations; i++) {
        for(int j = 0; j < 1000000; j++) {
            dummy += j;
        }
    }
}

int main(void) {
    int pid = fork();
    
    if(pid < 0) {
        printf(1, "Fork failed\n");
        exit();
    }
    
    if(pid == 0) {
        // Child process
        printf(1, "Child process %d starting...\n", getpid());
        
        // Do some work
        busy_work(1);
        
        // Get and print ticks before exit
        int ticks = ticks_run(getpid());
        printf(1, "Process %d ran for %d ticks\n", getpid(), ticks);
        
        exit();
    } else {
        // Parent process
        wait();
        
        // Get parent's ticks
        int parent_ticks = ticks_run(getpid());
        printf(1, "Parent process %d ran for %d ticks\n", getpid(), parent_ticks);
        
        exit();
    }
} 