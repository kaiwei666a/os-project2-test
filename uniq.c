#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define MAX_LINE 1024


int read_line(int fd, char *buf, int size) {
    int i = 0;
    char c;
    while (i < size - 1) {
        if (read(fd, &c, 1) == 0) break; 
        if (c == '\n') break;
        buf[i++] = c;
    }
    buf[i] = '\0';
    return i;
}

int strcasecmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        char c1 = (*s1 >= 'A' && *s1 <= 'Z') ? (*s1 + 32) : *s1;
        char c2 = (*s2 >= 'A' && *s2 <= 'Z') ? (*s2 + 32) : *s2;

        if (c1 != c2)
            return c1 - c2;

        s1++;
        s2++;
    }
    return *s1 - *s2;
}

void process_uniq(int fd, int count_flag, int ignore_case, int duplicates) {
    char prev_line[MAX_LINE] = "";
    char current_line[MAX_LINE];
    int count = 0;
    int first_line = 1;

    while (read_line(fd, current_line, MAX_LINE) > 0) {
        if (first_line || (ignore_case ? strcasecmp(current_line, prev_line) : strcmp(current_line, prev_line)) != 0) {
            if (!first_line) {
                if (!duplicates || count > 1) {
                    if (count_flag) printf(1, "%d ", count);
                    printf(1, "%s\n", prev_line);
                }
            }
            strcpy(prev_line, current_line);
            count = 1;
            first_line = 0;
        } else {
            count++;
        }
    }

    if (!first_line && (!duplicates || count > 1)) {
        if (count_flag) printf(1, "%d ", count);
        printf(1, "%s\n", prev_line);
    }
}

int main(int argc, char *argv[]) {
    int count_flag = 0, ignore_case = 0, duplicates = 0;
    char *filename = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0) count_flag = 1;
        else if (strcmp(argv[i], "-i") == 0) ignore_case = 1;
        else if (strcmp(argv[i], "-d") == 0) duplicates = 1;
        else filename = argv[i];
    }

    int fd;
    if (filename) {
        fd = open(filename, O_RDONLY);
        if (fd < 0) {
            printf(1, "uniq: cannot open %s\n", filename);
            exit();  
        }
    } else {
        fd = 0; 
    }

    process_uniq(fd, count_flag, ignore_case, duplicates);
    close(fd);
    exit();  
}

