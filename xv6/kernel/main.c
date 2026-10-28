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
    userinit();

    scheduler();

    while (1);
}
