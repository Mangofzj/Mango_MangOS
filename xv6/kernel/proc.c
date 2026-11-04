#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct cpu cpus[NCPU];  /* CPU 状态数组 */
struct proc proc[NPROC];  /* 进程表 */
int nextpid = 1;  /* 下一个待分配的进程 ID */
struct spinlock pid_lock;  /* 保护 nextpid 的自旋锁 */

/**
 * @brief 初始化进程表：初始化锁、分配内核栈，并将进程状态置为 UNUSED。
 *
 * @note 内核栈分配失败时 panic。
 */
void
procinit(void)
{
    struct proc *p;

    initlock(&pid_lock, "nextpid");

    for (p = proc; p < &proc[NPROC]; p++) {
        initlock(&p->lock, "proc");
        p->state = UNUSED;
        p->kstack = (uint64)kalloc();
        if (p->kstack == 0)
            panic("procinit: kalloc");
    }
}

/**
 * @brief 读取当前 CPU 编号。
 *
 * @return 当前 CPU 编号，取自 tp 寄存器
 */
int
cpuid()
{
    int id = r_tp();
    return id;
}

/**
 * @brief 返回当前 CPU 的状态结构体。
 *
 * @return 指向 cpus 数组中当前 CPU 元素的指针
 */
struct cpu *
mycpu(void)
{
    int id = cpuid();
    struct cpu *c = &cpus[id];
    return c;
}

/**
 * @brief 返回当前 CPU 上运行的进程。
 *
 * 读取期间关闭中断，防止读取过程被调度打断。
 *
 * @return 当前进程指针；调度器上下文中可能为 0
 */
struct proc *
myproc(void)
{
    push_off();
    struct cpu *c = mycpu();
    struct proc *p = c->proc;
    pop_off();

    return p;
}

/**
 * @brief 分配一个新的进程 ID。
 *
 * @return 新分配的进程 ID
 */
int
allocpid()
{
    int pid;

    acquire(&pid_lock);
    pid = nextpid;
    nextpid = nextpid + 1;
    release(&pid_lock);

    return pid;
}

/**
 * @brief 分配一个空闲进程，并初始化其上下文和任务入口。
 *
 * 找到空闲进程后保持其锁处于持有状态，由调用方继续设置状态并释放。
 *
 * @param start_routin 任务入口函数
 * @return 分配到的进程；无空闲进程时返回 0
 */
static struct proc *
allocproc(void (*start_routin)(void))
{
    struct proc *p;

    for (p = proc; p < &proc[NPROC]; p++) {
        acquire(&p->lock);
        if (p->state == UNUSED) {
            goto found;
        } else {
            release(&p->lock);
        }
    }
    return 0;

found:
    /* 此时已持有 p->lock */
    p->pid = allocpid();
    p->state = USED;

    memset(&p->context, 0, sizeof(p->context));
    p->context.ra = (uint64)kerneltrapret;
    p->context.sp = p->kstack + PGSIZE;
    p->start = start_routin;

    return p;
}

/**
 * @brief 创建初始任务：task0、task1 与 shell，并将它们置为可运行。
 */
void
userinit(void)
{
    struct proc *p;

    p = allocproc(user_task0);
    if (p == 0)
        panic("userinit: allocproc task0 failed");
    safestrcpy(p->name, "task0", 6);
    p->state = RUNNABLE;
    release(&p->lock);

    p = allocproc(user_task1);
    if (p == 0)
        panic("userinit: allocproc task1 failed");
    safestrcpy(p->name, "task1", 6);
    p->state = RUNNABLE;
    release(&p->lock);

    p = allocproc(user_shell);
    if (p == 0)
        panic("userinit: allocproc shell failed");
    safestrcpy(p->name, "shell", 6);
    p->state = RUNNABLE;
    release(&p->lock);
}

/**
 * @brief 调度器主循环：选择可运行的进程并切换到其上下文。
 *
 * 持有进程锁完成状态切换；进程通过 sched() 切回后继续遍历。
 */
void
scheduler(void)
{
    struct proc *p;
    struct cpu *c = mycpu();

    c->proc = 0;
    for (;;) {
        for (p = proc; p < &proc[NPROC]; p++) {
            acquire(&p->lock);
            if (p->state == RUNNABLE) {
                p->state = RUNNING;
                c->proc = p;
                swtch(&c->context, &p->context);
                c->proc = 0;
            }
            release(&p->lock);
        }
    }
}

/**
 * @brief 从当前进程切换到调度器上下文。
 *
 * 调用前必须持有当前进程锁、关闭中断（noff 为 1），且进程状态不能是 RUNNING。
 */
void
sched(void)
{
    int intena;
    struct proc *p = myproc();

    if (!holding(&p->lock))
        panic("sched p->lock");
    if (mycpu()->noff != 1)
        panic("sched locks");
    if (p->state == RUNNING)
        panic("sched RUNNING");
    if (intr_get())
        panic("sched interruptible");

    /* 保存中断使能状态，swtch 返回后恢复 */
    intena = mycpu()->intena;
    swtch(&p->context, &mycpu()->context);
    mycpu()->intena = intena;
}

/**
 * @brief 当前进程主动让出 CPU，重新进入可运行状态。
 */
void
yield(void)
{
    struct proc *p = myproc();

    acquire(&p->lock);
    p->state = RUNNABLE;
    sched();
    release(&p->lock);
}

/**
 * @brief 新进程的首次入口：释放进程锁并跳转到任务入口。
 *
 * 设置 sstatus 的 SPP、SPIE 与 sepc 后执行 sret，进入 p->start 指向的任务函数。
 */
void
kerneltrapret(void)
{
    struct proc *p = myproc();

    release(&p->lock);

    uint64 x = r_sstatus();
    x |= SSTATUS_SPP;
    x |= SSTATUS_SPIE;
    w_sstatus(x);

    w_sepc((uint64)p->start);

    asm volatile("sret");
}

/**
 * @brief 打印进程表：列出每个已使用进程的 ID、状态与名称。
 */
void
procdump(void)
{
    /* 进程状态名称表 */
    static char *states[] = {
        [UNUSED]   = "unused",
        [USED]     = "used",
        [SLEEPING] = "sleep ",
        [RUNNABLE] = "runble",
        [RUNNING]  = "run   ",
        [ZOMBIE]   = "zombie"
    };
    struct proc *p;
    char *state;

    printf("\n");
    for (p = proc; p < &proc[NPROC]; p++) {
        if (p->state == UNUSED)
            continue;
        if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
            state = states[p->state];
        else
            state = "???";
        printf("%d %s %s", p->pid, state, p->name);
        printf("\n");
    }
}
