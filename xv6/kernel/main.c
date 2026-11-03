#include "types.h"
#include "param.h"
#include "defs.h"
#include "sbi.h"

void _entry(void);
volatile static int started = 0;

void
main(void)
{
    if (started == 0) {
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
        timerinit();

        __sync_synchronize();
        started = 1;

        for (int hart = 0; hart < NCPU; hart++) {
            if (hart == cpuid())
                continue;

            struct sbiret ret = sbi_hart_start(hart, (uint64)_entry, 0);
            if (ret.error != 0) {
                printf("hart %d start failed: %ld\n", hart, ret.error);
                panic("sbi_hart_start");
            }
        }
    } else {
        while (started == 0)
            ;
        __sync_synchronize();

        printf("hart %d starting\n", cpuid());

        trapinithart();
        plicinithart();
        timerinit();
    }

    scheduler();
}
