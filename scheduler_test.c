#include "types.h"
#include "stat.h"
#include "user.h"

// Function to perform CPU-intensive work
void cpu_work(int iterations) {
    volatile int i, j;
    for(i = 0; i < iterations; i++) {
        for(j = 0; j < 1000; j++) {
            // Do some work
        }
    }
}

// Function to perform I/O work (reading/writing)
void io_work(int iterations) {
    int i;
    char buf[512];
    int fd;

    // Create a test file
    fd = open("test_file", 0x200 | 0x002); // O_CREATE | O_RDWR
    if(fd < 0) {
        printf(1, "Error creating test file\n");
        return;
    }

    // Do read/write operations
    for(i = 0; i < iterations; i++) {
        write(fd, buf, sizeof(buf));
    }

    close(fd);
    unlink("test_file");
}

void test_scheduler(int test_type) {
    int start_time, end_time;
    int pid = getpid();
    int initial_ticks = ticks_run(pid);

    // Record start time
    start_time = uptime();

    switch(test_type) {
        case 0: // CPU-bound test
            cpu_work(1000);
            break;
        case 1: // I/O-bound test
            io_work(100);
            break;
        case 2: // Mixed workload
            cpu_work(500);
            io_work(50);
            cpu_work(500);
            break;
    }

    // Record end time and final ticks
    end_time = uptime();
    int final_ticks = ticks_run(pid);

    // Print results
    printf(1, "Process %d:\n", pid);
    printf(1, "  Runtime: %d ticks\n", end_time - start_time);
    printf(1, "  CPU ticks: %d\n", final_ticks - initial_ticks);
}

void concurrent_test(int num_processes, int test_type) {
    int i;
    int pids[10];
    int start_time = uptime();

    // Fork multiple processes
    for(i = 0; i < num_processes; i++) {
        pids[i] = fork();
        if(pids[i] < 0) {
            printf(1, "Fork failed!\n");
            exit();
        }
        if(pids[i] == 0) {
            // Child process
            test_scheduler(test_type);
            exit();
        }
    }

    // Parent waits for all children
    for(i = 0; i < num_processes; i++) {
        wait();
    }

    int total_time = uptime() - start_time;
    printf(1, "\nTotal time for %d processes: %d ticks\n", num_processes, total_time);
}

int main(int argc, char *argv[]) {
    printf(1, "\n=== Scheduler Performance Test ===\n\n");

    // Test 1: Single process tests
    printf(1, "1. Single Process Tests\n");
    printf(1, "CPU-bound task:\n");
    test_scheduler(0);
    printf(1, "\nI/O-bound task:\n");
    test_scheduler(1);
    printf(1, "\nMixed workload:\n");
    test_scheduler(2);

    // Test 2: Concurrent process tests
    printf(1, "\n2. Concurrent Process Tests\n");
    printf(1, "Running 5 CPU-bound processes:\n");
    concurrent_test(5, 0);
    printf(1, "\nRunning 5 I/O-bound processes:\n");
    concurrent_test(5, 1);
    printf(1, "\nRunning 5 mixed workload processes:\n");
    concurrent_test(5, 2);

    exit();
} 