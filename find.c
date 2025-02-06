#include "types.h"
#include "stat.h"
#include "user.h"
#include "traps.h"
#include "fs.h"
//#include <stdio.h>


//error message
//char error_message[30] = "An error has occurred\n";

char *fmtname(char *path){
    static char buf[DIRSIZ+1];
    char *p;

    // Find first character after last slash.
    for(p=path+strlen(path);p>=path && *p != '/'; p--);
    p++;

    //Return blank-padded name
    if(strlen(p) >= DIRSIZ)
        return p;
    memmove(buf, p, strlen(p));
    memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
    buf[DIRSIZ] = 0;

    int len = strlen(buf);
    while (len > 0 && buf[len - 1] == ' ') {
        buf[--len] = '\0';
    }
    return buf;
}
  

void find(char *path, char *look, int type_flag, char type_val, int inum_flag, int printi_flag, int inum_val) {
    struct stat targetinfo;
    struct dirent de;
    char buf[512], *p;
    int fd;

    if ((fd = open(path, 0)) < 0) {
        printf(1, "find: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &targetinfo) < 0) {
        printf(2, "find: error retrieving file info\n");
        close(fd);
        return;
    }

    switch (targetinfo.type) {
    case T_FILE:
        if ((!type_flag || type_val == 'f')) { 
            if ((!inum_flag) ||                     
                (inum_flag == 1 && targetinfo.ino == inum_val) || 
                (inum_flag == 2 && targetinfo.ino > inum_val) ||   
                (inum_flag == 3 && targetinfo.ino < inum_val)) {  

                char *name = fmtname(path);
                if (strcmp(look, name) == 0) {
                    if (printi_flag) {
                        printf(1, "%d ", targetinfo.ino);
                    }
                    printf(1, "%s\n", path);
                }
            }
        }
        break;

    case T_DIR:
        if ((!type_flag || type_val == 'd')) {  
            if ((!inum_flag) || 
                (inum_flag == 1 && targetinfo.ino == inum_val) || 
                (inum_flag == 2 && targetinfo.ino > inum_val) ||   
                (inum_flag == 3 && targetinfo.ino < inum_val)) {   

                char *name = fmtname(path);
                if (strcmp(look, name) == 0) {
                    if (printi_flag) {
                        printf(1, "%d ", targetinfo.ino);
                    }
                    printf(1, "%s\n", path);
                }
            }
        }

        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
            printf(2, "find: path too long\n");
            break;
        }

        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';

        while (read(fd, &de, sizeof(de)) == sizeof(de)) {
            if (de.inum == 0) continue;
            if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) continue;

            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;

            if (stat(buf, &targetinfo) < 0) {
                printf(1, "find: cannot stat %s\n", buf);
                continue;
            }

            find(buf, look, type_flag, type_val, inum_flag, printi_flag, inum_val);
        }
        break;
    }
    close(fd);
}




int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf(2, "error\n");
        exit();
    }

    int i = 2;
    char* path = argv[1];
    char* look = 0;
    int type_flag = 0;
    char type_val = 0;
    int inum_flag = 0;
    int inum_val = 0;
    int printi_flag = 0;

    while (i < argc) {
        if (strcmp(argv[i], "-name") == 0) {
            if (i + 1 >= argc) {
                printf(2, "error\n");
                exit();
            }
            look = argv[i + 1];
            i += 2;
        }
        else if (strcmp(argv[i], "-type") == 0) {
            if (i + 1 >= argc) {
                printf(2, "error\n");
                exit();
            }
            type_flag = 1;
            type_val = argv[i + 1][0];
            if (type_val != 'd' && type_val != 'f') {
                printf(2, "error\n");
                exit();
            }
            i += 2;
        }
        else if (strcmp(argv[i], "-inum") == 0) {
            if (i + 1 >= argc) {
                printf(2, "error\n");
                exit();
            }
            inum_flag = 1;
        
        
            if (argv[i + 1][0] == '+') {
                inum_val = atoi(argv[i + 1] + 1); 
                inum_flag = 2;  
            } else if (argv[i + 1][0] == '-') {
                inum_val = atoi(argv[i + 1] + 1); 
                inum_flag = 3; 
            } else {
                inum_val = atoi(argv[i + 1]);  
            }
            i += 2;
        }
        
        else if (strcmp(argv[i], "-printi") == 0) {
            printi_flag = 1;  
            i += 1;
        }
        else {
            printf(2, "error\n");
            exit();
        }
    }

    if (look == 0) {
        printf(2, "error\n");
        exit();
    }

    find(path, look, type_flag, type_val, inum_flag, printi_flag, inum_val);

    exit();
}
