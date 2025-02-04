#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        exit();
    }
    int ticks = atoi(argv[1]);

    if (ticks <= 0) {
        exit();
    }
    sleep(ticks);
    exit();
}

