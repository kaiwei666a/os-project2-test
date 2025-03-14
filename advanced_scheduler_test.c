#include "types.h"
#include "stat.h"
#include "user.h"

void print_metrics(int pid) {
    printf(1, "Process %d metrics:\n", pid);
    printf(1, "Tickets: %d\n", get_tickets(pid));
    printf(1, "Run ticks: %d\n", ticks_run(pid));
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
    printf(1, "Starting LOTTERY scheduler test with 3 processes...\n");
    int pids[3];
    int tickets[3] = {10, 20, 30};  // Different ticket allocations
    int work_sizes[3] = {2, 2, 2};  // Same workload for all processes
    
    // Create three processes simultaneously
    for(int i = 0; i < 3; i++) {
        pids[i] = fork();
        if(pids[i] < 0) {
            printf(1, "Fork failed\n");
            exit();
        }
        
        if(pids[i] == 0) {
            // Child process
            set_tickets(tickets[i]);  // Set tickets for this process
            printf(1, "Child process %d (pid %d) starting with %d tickets...\n", 
                   i, getpid(), tickets[i]);
            busy_work(work_sizes[i]);
            printf(1, "Child process %d (pid %d) completed\n", i, getpid());
            exit();
        }
    }
    
    // Parent process waits for all children
    for(int i = 0; i < 3; i++) {
        wait();
        print_metrics(pids[i]);
    }
    
    exit();
} 