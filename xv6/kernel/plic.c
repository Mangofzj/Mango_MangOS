#include "types.h"
#include "memlayout.h"
#include "defs.h"

void
plicinit(void)
{
    *(uint32 *)(PLIC_PRIORITY + UART0_IRQ * 4) = 1;
}

void
plicinithart(void)
{
    int hart = cpuid();

    *(uint32 *)PLIC_SENABLE(hart) = (1 << UART0_IRQ);

    *(uint32 *)PLIC_SPRIORITY(hart) = 0;
}

int
plic_claim(void)
{
    int hart = cpuid();
    int irq = *(uint32 *)PLIC_SCLAIM(hart);
    return irq;
}

void
plic_complete(int irq)
{
    int hart = cpuid();
    *(uint32 *)PLIC_SCLAIM(hart) = irq;
}
