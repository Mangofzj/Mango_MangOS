#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"
#include "sbi.h"

#define TIMER_INTERVAL 1000000

void
timerinit(void)
{
    sbi_set_timer(r_time() + TIMER_INTERVAL);
}

void
clockintr(void)
{
    if (cpuid() == 0) {
        acquire(&tickslock);

        ticks++;
        wakeup(&ticks);

        release(&tickslock);
    }

    sbi_set_timer(r_time() + TIMER_INTERVAL);
}
