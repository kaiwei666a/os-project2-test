#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

// CPU密集型工作负载
void cpu_work(int iterations) {
    int i, j, result = 0;
    for(i = 0; i < iterations; i++) {
        for(j = 0; j < 1000; j++) {
            result += (i * j) % 17;
        }
    }
    printf(1, "CPU work result: %d\n", result);
}

// IO密集型工作负载
void io_work(int iterations) {
    int i;
    int fd;
    char data[100];
    for(i = 0; i < iterations; i++) {
        fd = open("test.txt", O_CREATE | O_RDWR);
        if(fd >= 0) {
            write(fd, "test data", 9);
            close(fd);
            unlink("test.txt");
        }
    }
}

// 打印进程性能指标
void print_metrics(int pid) {
    int creation = get_creation_time(pid);
    int start = get_start_time(pid);
    int completion = get_completion_time(pid);
    int run_time = get_total_run_time(pid);
    int ready_time = get_total_ready_time(pid);

    printf(1, "Process %d metrics:\n", pid);
    printf(1, "Creation time: %d\n", creation);
    printf(1, "Start time: %d\n", start);
    printf(1, "Completion time: %d\n", completion);
    printf(1, "Total run time: %d\n", run_time);
    printf(1, "Total ready time: %d\n", ready_time);
    printf(1, "Response time: %d\n", start - creation);
    printf(1, "Turnaround time: %d\n", completion - creation);
    printf(1, "------------------------\n");
}

// 测试不同类型的工作负载
void test_scheduler(int test_type) {
    int pid = getpid();
    int start_time = uptime();

    switch(test_type) {
        case 0: // CPU密集型
            printf(1, "Running CPU-bound test...\n");
            cpu_work(5000);
            break;
        case 1: // IO密集型
            printf(1, "Running I/O-bound test...\n");
            io_work(100);
            break;
        case 2: // 混合工作负载
            printf(1, "Running mixed workload test...\n");
            cpu_work(2500);
            io_work(50);
            break;
    }

    int end_time = uptime();
    printf(1, "Test completed in %d ticks\n", end_time - start_time);
    print_metrics(pid);
}

// 并发测试
void concurrent_test(int num_processes, int test_type) {
    int i;
    int pids[10];
    int start_time = uptime();

    printf(1, "Starting concurrent test with %d processes...\n", num_processes);

    // 创建多个进程
    for(i = 0; i < num_processes; i++) {
        pids[i] = fork();
        if(pids[i] == 0) {
            test_scheduler(test_type);
            exit();
        }
    }

    // 等待所有进程完成
    for(i = 0; i < num_processes; i++) {
        wait();
    }

    int end_time = uptime();
    printf(1, "All processes completed in %d ticks\n", end_time - start_time);
}

int main(int argc, char *argv[]) {
    printf(1, "Starting scheduler performance tests...\n\n");

    // 单进程测试
    printf(1, "=== Single Process Tests ===\n");
    printf(1, "1. CPU-bound test\n");
    test_scheduler(0);
    
    printf(1, "2. I/O-bound test\n");
    test_scheduler(1);
    
    printf(1, "3. Mixed workload test\n");
    test_scheduler(2);

    // 并发测试
    printf(1, "\n=== Concurrent Process Tests ===\n");
    printf(1, "1. Multiple CPU-bound processes\n");
    concurrent_test(5, 0);
    
    printf(1, "2. Multiple I/O-bound processes\n");
    concurrent_test(5, 1);
    
    printf(1, "3. Multiple mixed workload processes\n");
    concurrent_test(5, 2);

    exit();
} 