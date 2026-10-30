#include "types.h"
#include "defs.h"

void
main(void)
{
    printfinit();

    printf("\n");
    printf("Hello, xv6!\n");
    printf("\n");

    kinit();
    procinit();
    trapinit();
    trapinithart();
    uartinit();
    plicinit();
    plicinithart();
    userinit();

    scheduler();

    while (1);
}
