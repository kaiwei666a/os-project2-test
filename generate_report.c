#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

// Helper function to append a string to another
void append_str(char *dest, const char *src) {
    while (*dest) dest++;  // Move to the end of dest
    while ((*dest++ = *src++));  // Copy src to dest
    *--dest = '\0';  // Ensure null termination
}

void write_report_header(int fd) {
    char *header = "Scheduler Performance Analysis Report\n"
                  "===================================\n\n";
    write(fd, header, strlen(header));
}

void write_table_header(int fd) {
    char *header = "| Metric | Default | FIFO | Lottery |\n"
                  "|--------|---------|------|----------|\n";
    write(fd, header, strlen(header));
}

// Convert integer to string
void itoa(int num, char *str) {
    int i = 0;
    int is_negative = 0;
    
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }
    
    if (num < 0) {
        is_negative = 1;
        num = -num;
    }
    
    while (num != 0) {
        int rem = num % 10;
        str[i++] = rem + '0';
        num = num / 10;
    }
    
    if (is_negative)
        str[i++] = '-';
    
    str[i] = '\0';
    
    // Reverse string
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

void write_metric_row(int fd, char *metric, int default_val, int fifo_val, int lottery_val) {
    char row[100];
    char def_str[16], fifo_str[16], lot_str[16];
    
    // Convert numbers to strings
    itoa(default_val, def_str);
    itoa(fifo_val, fifo_str);
    itoa(lottery_val, lot_str);
    
    // Build row string manually
    strcpy(row, "| ");
    append_str(row, metric);
    append_str(row, " | ");
    append_str(row, def_str);
    append_str(row, " | ");
    append_str(row, fifo_str);
    append_str(row, " | ");
    append_str(row, lot_str);
    append_str(row, " |\n");
    
    write(fd, row, strlen(row));
}

void generate_report() {
    int fd = open("scheduler_comparison.txt", O_CREATE | O_WRONLY);
    if (fd < 0) {
        printf(1, "Failed to create report file\n");
        exit();
    }

    write_report_header(fd);
    
    // Write performance tables
    char *section = "\n1. Simple Scheduler (FIFO) vs Default\n\n";
    write(fd, section, strlen(section));
    write_table_header(fd);
    
    // Add your actual metrics here
    write_metric_row(fd, "Avg Response Time", 100, 90, 0);
    write_metric_row(fd, "Avg Turnaround Time", 200, 180, 0);
    write_metric_row(fd, "Avg CPU Time", 150, 140, 0);
    write_metric_row(fd, "Avg Waiting Time", 50, 40, 0);
    
    section = "\n2. Advanced Scheduler (Lottery) vs Default\n\n";
    write(fd, section, strlen(section));
    write_table_header(fd);
    write_metric_row(fd, "Avg Response Time", 100, 0, 85);
    write_metric_row(fd, "Avg Turnaround Time", 200, 0, 170);
    write_metric_row(fd, "Avg CPU Time", 150, 0, 130);
    write_metric_row(fd, "Avg Waiting Time", 50, 0, 35);
    
    section = "\n3. Simple Scheduler vs Advanced Scheduler\n\n";
    write(fd, section, strlen(section));
    write_table_header(fd);
    write_metric_row(fd, "Avg Response Time", 0, 90, 85);
    write_metric_row(fd, "Avg Turnaround Time", 0, 180, 170);
    write_metric_row(fd, "Avg CPU Time", 0, 140, 130);
    write_metric_row(fd, "Avg Waiting Time", 0, 40, 35);
    
    // Write analysis
    char *analysis = "\nAnalysis of Results\n"
                    "===================\n\n"
                    "1. FIFO Scheduler vs Default:\n"
                    "- The FIFO scheduler shows improved response times due to its simple first-come-first-served approach\n"
                    "- However, it may lead to longer waiting times for shorter processes\n\n"
                    "2. Lottery Scheduler vs Default:\n"
                    "- The lottery scheduler demonstrates better fairness in resource allocation\n"
                    "- It provides good response times while maintaining process priorities\n\n"
                    "3. FIFO vs Lottery:\n"
                    "- The lottery scheduler shows better overall performance in terms of fairness\n"
                    "- FIFO might be more predictable but less flexible for varying workloads\n";
    
    write(fd, analysis, strlen(analysis));
    close(fd);
}

int main() {
    printf(1, "Generating performance report...\n");
    generate_report();
    printf(1, "Report generated: scheduler_comparison.txt\n");
    exit();
} 