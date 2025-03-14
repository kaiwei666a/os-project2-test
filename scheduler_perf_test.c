#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

// Performance metrics structure
struct PerfMetrics {
    int turnaround_time;
    int response_time;
    int cpu_time;
    int waiting_time;
};

// Function to calculate metrics for a process
void calculate_metrics(int pid, struct PerfMetrics *metrics) {
    int creation = get_creation_time(pid);
    int start = get_start_time(pid);
    int completion = get_completion_time(pid);
    int run_time = get_total_run_time(pid);
    int ready_time = get_total_ready_time(pid);

    // If completion time is 0, use current time
    if (completion == 0) {
        completion = uptime();
        printf(1, "WARNING: Completion time was 0, using current time: %d\n", completion);
    }

    printf(1, "\n=============================\n");
    printf(1, "Process %d raw metrics:\n", pid);
    printf(1, "Creation time: %d\n", creation);
    printf(1, "Start time: %d\n", start);
    printf(1, "Completion time: %d\n", completion);
    printf(1, "Run time: %d\n", run_time);
    printf(1, "Ready time: %d\n", ready_time);
    
    if (completion < creation) {
        printf(1, "WARNING: Completion time is less than creation time!\n");
    }
    printf(1, "=============================\n");

    // Only calculate metrics if we have valid times
    if (creation >= 0 && completion >= 0) {
        metrics->turnaround_time = completion - creation;
    } else {
        metrics->turnaround_time = -1;
    }

    if (creation >= 0 && start >= 0) {
        metrics->response_time = start - creation;
    } else {
        metrics->response_time = -1;
    }

    metrics->cpu_time = run_time;
    metrics->waiting_time = ready_time;
}

// Function to run stressfs workload
void run_stressfs() {
    int pid = fork();
    if (pid < 0) {
        printf(1, "Fork failed\n");
        exit();
    }
    if (pid == 0) {
        // Child process
        char *args[] = {"stressfs", 0};
        exec("stressfs", args);
        printf(1, "exec stressfs failed\n");
        exit();
    }
    
    // Parent process
    struct PerfMetrics metrics;
    
    // Wait for child to finish
    wait();
    
    // Calculate metrics for the completed child
    calculate_metrics(pid, &metrics);
    printf(1, "stressfs metrics:\n");
    printf(1, "Turnaround time: %d\n", metrics.turnaround_time);
    printf(1, "Response time: %d\n", metrics.response_time);
    printf(1, "CPU time: %d\n", metrics.cpu_time);
    printf(1, "Waiting time: %d\n", metrics.waiting_time);
}

// Function to run find workload
void run_find() {
    int pid = fork();
    if (pid < 0) {
        printf(1, "Fork failed\n");
        exit();
    }
    if (pid == 0) {
        // Child process
        char *args[] = {"find", ".", "-name", "find", 0};
        exec("find", args);
        printf(1, "exec find failed\n");
        exit();
    }
    
    // Parent process
    struct PerfMetrics metrics;
    
    // Wait for child to finish
    wait();
    
    // Calculate metrics for the completed child
    calculate_metrics(pid, &metrics);
    printf(1, "find metrics:\n");
    printf(1, "Turnaround time: %d\n", metrics.turnaround_time);
    printf(1, "Response time: %d\n", metrics.response_time);
    printf(1, "CPU time: %d\n", metrics.cpu_time);
    printf(1, "Waiting time: %d\n", metrics.waiting_time);
}

// Function to run cat and uniq on README
void run_cat_uniq() {
    int pid = fork();
    if (pid < 0) {
        printf(1, "Fork failed\n");
        exit();
    }
    if (pid == 0) {
        // Child process
        int p[2];
        pipe(p);
        
        if (fork() == 0) {
            close(1);
            dup(p[1]);
            close(p[0]);
            close(p[1]);
            char *args[] = {"cat", "README", 0};
            exec("cat", args);
            printf(1, "exec cat failed\n");
            exit();
        }
        
        close(0);
        dup(p[0]);
        close(p[0]);
        close(p[1]);
        char *args[] = {"uniq", 0};
        exec("uniq", args);
        printf(1, "exec uniq failed\n");
        exit();
    }
    
    // Parent process
    struct PerfMetrics metrics;
    
    // Wait for child to finish
    wait();
    
    // Calculate metrics for the completed child
    calculate_metrics(pid, &metrics);
    printf(1, "cat|uniq metrics:\n");
    printf(1, "Turnaround time: %d\n", metrics.turnaround_time);
    printf(1, "Response time: %d\n", metrics.response_time);
    printf(1, "CPU time: %d\n", metrics.cpu_time);
    printf(1, "Waiting time: %d\n", metrics.waiting_time);
}

// Custom workload: echo hello > test.txt
void run_custom_workload() {
    int pid = fork();
    if (pid < 0) {
        printf(1, "Fork failed\n");
        exit();
    }
    if (pid == 0) {
        // Child process
        close(1);  // Close standard output
        int fd = open("test.txt", O_CREATE|O_WRONLY);
        if(fd < 0) {
            printf(2, "Failed to create test.txt\n");
            exit();
        }
        char *args[] = {"echo", "hello", 0};
        exec("echo", args);
        printf(2, "exec echo failed\n");
        exit();
    }
    
    // Parent process
    struct PerfMetrics metrics;
    
    // Wait for child to finish
    wait();
    
    // Calculate metrics for the completed child
    calculate_metrics(pid, &metrics);
    printf(1, "Custom workload metrics:\n");
    printf(1, "Turnaround time: %d\n", metrics.turnaround_time);
    printf(1, "Response time: %d\n", metrics.response_time);
    printf(1, "CPU time: %d\n", metrics.cpu_time);
    printf(1, "Waiting time: %d\n", metrics.waiting_time);
}

// Function to run all workloads and collect metrics
void run_performance_test() {
    int start_time = uptime();
    int pids[4];  // Array to store process IDs
    struct PerfMetrics metrics[4];  // Array to store metrics
    
    printf(1, "Starting performance test...\n");
    printf(1, "Creating all processes...\n");
    
    // Create stressfs process
    pids[0] = fork();
    if (pids[0] < 0) {
        printf(1, "Fork failed\n");
        exit();
    }
    if (pids[0] == 0) {
        // Child process
        char *args[] = {"stressfs", 0};
        exec("stressfs", args);
        printf(1, "exec stressfs failed\n");
        exit();
    }
    
    // Create find process
    pids[1] = fork();
    if (pids[1] < 0) {
        printf(1, "Fork failed\n");
        exit();
    }
    if (pids[1] == 0) {
        // Child process
        char *args[] = {"find", ".", "-name", "find", 0};
        exec("find", args);
        printf(1, "exec find failed\n");
        exit();
    }
    
    // Create cat|uniq process
    pids[2] = fork();
    if (pids[2] < 0) {
        printf(1, "Fork failed\n");
        exit();
    }
    if (pids[2] == 0) {
        // Child process
        int p[2];
        pipe(p);
        
        if (fork() == 0) {
            close(1);
            dup(p[1]);
            close(p[0]);
            close(p[1]);
            char *args[] = {"cat", "README", 0};
            exec("cat", args);
            printf(1, "exec cat failed\n");
            exit();
        }
        
        close(0);
        dup(p[0]);
        close(p[0]);
        close(p[1]);
        char *args[] = {"uniq", 0};
        exec("uniq", args);
        printf(1, "exec uniq failed\n");
        exit();
    }
    
    // Create echo process
    pids[3] = fork();
    if (pids[3] < 0) {
        printf(1, "Fork failed\n");
        exit();
    }
    if (pids[3] == 0) {
        // Child process
        close(1);  // Close standard output
        int fd = open("test.txt", O_CREATE|O_WRONLY);
        if(fd < 0) {
            printf(2, "Failed to create test.txt\n");
            exit();
        }
        char *args[] = {"echo", "hello", 0};
        exec("echo", args);
        printf(2, "exec echo failed\n");
        exit();
    }
    
    printf(1, "\nAll processes created, waiting for completion...\n");
    
    // Wait for all processes to complete and collect metrics
    for(int i = 0; i < 4; i++) {
        wait();
        calculate_metrics(pids[i], &metrics[i]);
    }
    
    // Print metrics for all processes
    printf(1, "\nstressfs metrics:\n");
    printf(1, "Turnaround time: %d\n", metrics[0].turnaround_time);
    printf(1, "Response time: %d\n", metrics[0].response_time);
    printf(1, "CPU time: %d\n", metrics[0].cpu_time);
    printf(1, "Waiting time: %d\n", metrics[0].waiting_time);
    
    printf(1, "\nfind metrics:\n");
    printf(1, "Turnaround time: %d\n", metrics[1].turnaround_time);
    printf(1, "Response time: %d\n", metrics[1].response_time);
    printf(1, "CPU time: %d\n", metrics[1].cpu_time);
    printf(1, "Waiting time: %d\n", metrics[1].waiting_time);
    
    printf(1, "\ncat|uniq metrics:\n");
    printf(1, "Turnaround time: %d\n", metrics[2].turnaround_time);
    printf(1, "Response time: %d\n", metrics[2].response_time);
    printf(1, "CPU time: %d\n", metrics[2].cpu_time);
    printf(1, "Waiting time: %d\n", metrics[2].waiting_time);
    
    printf(1, "\nCustom workload metrics:\n");
    printf(1, "Turnaround time: %d\n", metrics[3].turnaround_time);
    printf(1, "Response time: %d\n", metrics[3].response_time);
    printf(1, "CPU time: %d\n", metrics[3].cpu_time);
    printf(1, "Waiting time: %d\n", metrics[3].waiting_time);
    
    int total_ticks = uptime() - start_time;
    printf(1, "\nTotal test time: %d ticks\n", total_ticks);
}

int main(int argc, char *argv[]) {
    printf(1, "Starting scheduler performance analysis...\n");
    run_performance_test();
    exit();
} 