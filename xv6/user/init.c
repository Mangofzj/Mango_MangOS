#include "user/user.h"

int
main(void)
{
    int ticks1, ticks2;
    int pid = getpid();

    printf("Hello, World! My pid is %d\n", pid);

    printf("--> Sleeping 2s ......\n");
    ticks1 = uptime();
    pause(20);
    ticks2 = uptime();
    printf("<-- Woke-up! Elapsed ticks = %d.\n", ticks2 - ticks1);

    return 0;
}
