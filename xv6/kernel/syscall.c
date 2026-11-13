#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h"
#include "defs.h"

static uint64
argraw(int n)
{
    struct proc *p = myproc();
    switch (n) {
    case 0:
        return p->trapframe->a0;
    case 1:
        return p->trapframe->a1;
    case 2:
        return p->trapframe->a2;
    case 3:
        return p->trapframe->a3;
    case 4:
        return p->trapframe->a4;
    case 5:
        return p->trapframe->a5;
    }
    panic("argraw");

    return -1;
}

void
argint(int n, int *ip)
{
    *ip = argraw(n);
}

void
argaddr(int n, uint64 *ip)
{
    *ip = argraw(n);
}

extern uint64 sys_getpid(void);
extern uint64 sys_pause(void);
extern uint64 sys_uptime(void);
extern uint64 sys_write(void);

static uint64 (*syscalls[])(void) = {
    [SYS_getpid] sys_getpid,
    [SYS_pause]  sys_pause,
    [SYS_uptime] sys_uptime,
    [SYS_write]  sys_write,
};

void
syscall(void)
{
    int num;
    struct proc *p = myproc();

    num = p->trapframe->a7;
    if (num > 0 && num < NELEM(syscalls) && syscalls[num]) {
        p->trapframe->a0 = syscalls[num]();
    } else {
        printf("%d %s: unknown sys call %d\n", p->pid, p->name, num);
        p->trapframe->a0 = -1;
    }
}
