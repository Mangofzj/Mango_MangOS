#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

uint64
sys_exit(void)
{
    int n;

    argint(0, &n);
    kexit(n);

    return 0;
}

uint64
sys_getpid(void)
{
    return myproc()->pid;
}

uint64
sys_fork(void)
{
    return kfork();
}

uint64
sys_wait(void)
{
    uint64 p;

    argaddr(0, &p);

    return kwait(p);
}

uint64
sys_kill(void)
{
    int pid;

    argint(0, &pid);

    return kkill(pid);
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
        if (killed(myproc())) {
            release(&tickslock);
            return -1;
        }

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
