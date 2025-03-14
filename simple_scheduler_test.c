#include "types.h"
#include "stat.h"
#include "user.h"

void print_metrics(int pid) {
    printf(1, "Process %d metrics:\n", pid);
    printf(1, "Tickets: -1\n");
    printf(1, "Run ticks: -1\n");
    printf(1, "Creation time: %d\n", get_creation_time(pid));
    printf(1, "Start time: %d\n", get_start_time(pid));
    printf(1, "Completion time: %d\n", get_completion_time(pid));
    printf(1, "Total run time: %d\n", get_total_run_time(pid));
    printf(1, "Total ready time: %d\n", get_total_ready_time(pid));
    printf(1, "-------------------\n");
}

// Function to prevent compiler optimization
volatile int dummy = 0;

void busy_work(int iterations) {
    for(int i = 0; i < iterations; i++) {
        for(int j = 0; j < 1000000; j++) {
            dummy += j;  // This prevents compiler optimization
        }
    }
}

int main(void) {
    printf(1, "Starting FIFO scheduler test with 3 processes...\n");
    int pids[3];
    int work_sizes[3] = {1, 2, 3};  // Different workloads for each process
    
    // Create and run processes one at a time
    for(int i = 0; i < 3; i++) {
        pids[i] = fork();
        if(pids[i] < 0) {
            printf(1, "Fork failed\n");
            exit();
        }
        
        if(pids[i] == 0) {
            // Child process
            printf(1, "Child process %d starting work...\n", getpid());
            busy_work(work_sizes[i]);
            printf(1, "Child process %d completed work\n", getpid());
            exit();
        } else {
            // Parent process waits for this child to complete before creating next
            wait();
            print_metrics(pids[i]);
        }
    }
    
    exit();
} 