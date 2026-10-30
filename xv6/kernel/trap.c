#include "types.h"
#include "memlayout.h"
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

    w_sie(r_sie() | SIE_SEIE);
}

void
kerneltrap(void)
{
    int which_dev = 0;
    uint64 sstatus = r_sstatus();
    uint64 scause = r_scause();

    if ((sstatus & SSTATUS_SPP) == 0)
        panic("kerneltrap: not from supervisor mode");

    if (intr_get() != 0)
        panic("kerneltrap: interrupts enabled");

    if ((which_dev = devintr()) == 0) {
        printf("scause=0x%lx sepc=0x%lx stval=0x%lx\n", scause, r_sepc(), r_stval());
        panic("kerneltrap");
    }
}

int
devintr()
{
    uint64 scause = r_scause();

    if (scause == 0x8000000000000009L) {
        int irq = plic_claim();

        if (irq == UART0_IRQ) {
            uartintr();
        } else if (irq) {
            printf("unexpected interrupt irq=%d\n", irq);
        }
        if (irq)
            plic_complete(irq);

        return 1;
    } else
        return 0;
}
