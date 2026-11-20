#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h"
#include "defs.h"

int
fetchaddr(uint64 addr, uint64 *ip)
{
    struct proc *p = myproc();
    if (addr >= p->sz || addr+sizeof(uint64) > p->sz)
        return -1;
    if (copyin(p->pagetable, (char *)ip, addr, sizeof(*ip)) != 0)
        return -1;

    return 0;
}

int
fetchstr(uint64 addr, char *buf, int max)
{
    struct proc *p = myproc();
    if (copyinstr(p->pagetable, buf, addr, max) < 0)
        return -1;

    return strlen(buf);
}

int
argstr(int n, char *buf, int max)
{
    uint64 addr;

    argaddr(n, &addr);

    return fetchstr(addr, buf, max);
}

/**
 * @brief 读取第 n 个系统调用参数。
 *
 * 参数依次取自 a0~a5；序号越界时 panic。
 *
 * @param n 参数序号（0~5）
 * @return 对应寄存器的值
 */
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

/**
 * @brief 读取第 n 个系统调用参数并转为 int。
 *
 * @param n 参数序号
 * @param ip 输出参数值
 */
void
argint(int n, int *ip)
{
    *ip = argraw(n);
}

/**
 * @brief 读取第 n 个系统调用参数作为地址。
 *
 * @param n 参数序号
 * @param ip 输出地址值
 */
void
argaddr(int n, uint64 *ip)
{
    *ip = argraw(n);
}

extern uint64 sys_fork(void);
extern uint64 sys_exit(void);
extern uint64 sys_wait(void);
extern uint64 sys_read(void);
extern uint64 sys_kill(void);
extern uint64 sys_exec(void);
extern uint64 sys_getpid(void);
extern uint64 sys_pause(void);
extern uint64 sys_uptime(void);
extern uint64 sys_write(void);

/* 系统调用分发表，按 SYS_* 编号索引 */
static uint64 (*syscalls[])(void) = {
    [SYS_fork]   sys_fork,
    [SYS_exit]   sys_exit,
    [SYS_wait]   sys_wait,
    [SYS_read]   sys_read,
    [SYS_kill]   sys_kill,
    [SYS_exec]   sys_exec,
    [SYS_getpid] sys_getpid,
    [SYS_pause]  sys_pause,
    [SYS_uptime] sys_uptime,
    [SYS_write]  sys_write,
};

/**
 * @brief 处理系统调用：按 a7 编号分发，并将返回值写回 a0。
 *
 * 编号不在有效范围或表项为空时打印错误，并将 a0 置为 -1。
 */
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
