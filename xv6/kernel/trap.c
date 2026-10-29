#include "types.h"
#include "riscv.h"
#include "defs.h"

void kernelvec(void);

void
trapinit(void)
{
}

void
trapinithart(void)
{
    w_stvec((uint64)kernelvec);
}

void
kerneltrap(void)
{
    uint64 sstatus = r_sstatus();
    uint64 scause = r_scause();

    if ((sstatus & SSTATUS_SPP) == 0)
        panic("kerneltrap: not from supervisor mode");
    if (intr_get() != 0)
        panic("kerneltrap: interrupts enabled");

    if (scause & 0x8000000000000000L) {
        printf("Asynchronous trap - interrupt!\n");
    } else {
        printf("scause=0x%lx sepc=0x%lx stval=0x%lx\n", scause, r_sepc(), r_stval());
        panic("kerneltrap");
    }
}
