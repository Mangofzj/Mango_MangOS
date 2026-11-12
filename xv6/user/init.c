#include "user/user.h"

int
main(void)
{
    int pid = getpid();

    printf("Hello, World! My pid is %d\n", pid);

    return 0;
}
