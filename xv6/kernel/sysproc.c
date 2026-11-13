#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

uint64
sys_getpid(void)
{
    return myproc()->pid;
}

uint64
sys_pause(void)
{
    int n;
    uint ticks0;

    argint(0, &n);
    if (n < 0)
        n = 0;

    acquire(&tickslock);

    ticks0 = ticks;
    while (ticks - ticks0 < n) {
        sleep(&ticks, &tickslock);
    }

    release(&tickslock);

    return 0;
}

uint64
sys_uptime(void)
{
    uint xticks;

    acquire(&tickslock);

    xticks = ticks;

    release(&tickslock);

    return xticks;
}
