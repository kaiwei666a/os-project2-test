#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf(2, "Usage: sleep <ticks>\n");
        exit();  
    }
    int ticks = atoi(argv[1]);

    
    if (ticks <= 0) {
        printf(2, "sleep: Invalid number of ticks\n");
        exit(); 
    }

    sleep(ticks);

    exit();  
}


