#include "sbi.h"

void
main(void)
{
    const char *msg = "Hello, xv6!\n";

    while (*msg) {
        sbi_console_putchar(*msg);
        msg++;
    }

    for (;;)
        ;
}
