#include "types.h"
#include "riscv.h"
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
    sbi_set_timer(r_time() + TIMER_INTERVAL);
}
