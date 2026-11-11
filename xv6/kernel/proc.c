#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct cpu cpus[NCPU];  /* CPU 状态数组 */
struct proc proc[NPROC];  /* 进程表 */
struct proc *initproc;  /* 初始进程指针 */

int nextpid = 1;  /* 下一个待分配的进程 ID */
struct spinlock pid_lock;  /* 保护 nextpid 的自旋锁 */

extern void forkret(void);

extern char trampoline[];  /* 跳板页代码起始地址 */

/**
 * @brief 为每个进程分配一页内核栈并映射到内核页表。
 *
 * @param kpgtbl 内核页表根指针
 * @note 内核栈分配失败时 panic。
 */
void
proc_mapstacks(pagetable_t kpgtbl)
{
    struct proc *p;

    for (p = proc; p < &proc[NPROC]; p++) {
        char *pa = kalloc();
        if (pa == 0)
            panic("kalloc");

        uint64 va = KSTACK((int)(p - proc));
        kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);

        if (p == proc)
            printf("stack: va [%p] --> pa [%p]\n", (void *)va, pa);
    }
}

/**
 * @brief 初始化进程表：初始化锁，设置内核栈地址，并将进程状态置为 UNUSED。
 */
void
procinit(void)
{
    struct proc *p;

    initlock(&pid_lock, "nextpid");

    for (p = proc; p < &proc[NPROC]; p++) {
        initlock(&p->lock, "proc");
        p->state = UNUSED;
        p->kstack = KSTACK((int)(p - proc));
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
 * @brief 解除跳板页与陷阱帧映射，并释放进程页表及用户内存。
 *
 * @param pagetable 进程页表根指针
 * @param sz 用户内存大小（字节）
 */
void
proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmunmap(pagetable, TRAPFRAME, 1, 0);
    uvmfree(pagetable, sz);
}

/**
 * @brief 释放陷阱帧和进程页表，并将进程控制块重置为未使用状态。
 *
 * @param p 待释放的进程
 */
static void
freeproc(struct proc *p)
{
    if (p->trapframe)
        kfree((void *)p->trapframe);
    p->trapframe = 0;

    if (p->pagetable)
        proc_freepagetable(p->pagetable, p->sz);

    p->pagetable = 0;
    p->sz = 0;
    p->pid = 0;
    p->name[0] = 0;
    p->state = UNUSED;
}

/**
 * @brief 创建进程的用户页表，并映射跳板页与陷阱帧。
 *
 * @return 页表根指针；创建或映射失败时返回 0
 */
pagetable_t
proc_pagetable(struct proc *p)
{
    pagetable_t pagetable;

    pagetable = uvmcreate();
    if (pagetable == 0)
        return 0;

    if (mappages(pagetable, TRAMPOLINE, PGSIZE, (uint64)trampoline, PTE_R | PTE_X) < 0) {
        uvmfree(pagetable, 0);
        return 0;
    }

    if (mappages(pagetable, TRAPFRAME, PGSIZE, (uint64)(p->trapframe), PTE_R | PTE_W) < 0) {
        uvmunmap(pagetable, TRAMPOLINE, 1, 0);
        uvmfree(pagetable, 0);
        return 0;
    }

    return pagetable;
}

/**
 * @brief 分配一个空闲进程，并初始化其陷阱帧、页表与上下文。
 *
 * 找到空闲进程后保持其锁处于持有状态，由调用方继续设置状态并释放；
 * 陷阱帧或页表分配失败时调用 freeproc 释放资源并返回 0。
 *
 * @return 分配到的进程；无空闲进程、陷阱帧分配失败或页表创建失败时返回 0
 */
static struct proc *
allocproc(void)
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

    if ((p->trapframe = (struct trapframe *)kalloc()) == 0) {
        freeproc(p);
        release(&p->lock);
        return 0;
    }

    p->pagetable = proc_pagetable(p);
    if (p->pagetable == 0) {
        freeproc(p);
        release(&p->lock);
        return 0;
    }

    memset(&p->context, 0, sizeof(p->context));
    p->context.ra = (uint64)forkret;
    p->context.sp = p->kstack + PGSIZE;

    return p;
}

extern uchar initcode[];
extern int sizeof_initcode;

/**
 * @brief 将初始进程的机器码加载到用户虚拟地址 0。
 *
 * 分配一页并映射到地址 0，复制机器码后设置进程名、epc 与 sp。
 *
 * @param src 机器码起始地址
 * @param sz 机器码长度（字节）
 * @note 机器码超过一页、内存分配或映射失败时 panic。
 * @return 固定返回 0
 */
static int
loadinit(uchar *src, uint sz)
{
    char *mem;
    struct proc *p = myproc();
    pagetable_t pagetable = p->pagetable;

    if (sz > PGSIZE)
        panic("loadinit: more than a page");

    mem = kalloc();
    if (mem == 0)
        panic("kalloc");
    memset(mem, 0, PGSIZE);

    if (mappages(pagetable, 0, PGSIZE, (uint64)mem, PTE_W | PTE_R | PTE_X | PTE_U) < 0)
        panic("loadinit: mappages failed");

    memmove(mem, src, sz);
    p->sz = PGSIZE;

    safestrcpy(p->name, "init", sizeof(p->name));

    p->trapframe->epc = 0;
    p->trapframe->sp = PGSIZE;

    return 0;
}

/**
 * @brief 创建初始进程并置为可运行。
 */
void
userinit(void)
{
    struct proc *p;

    p = allocproc();
    initproc = p;
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
 * @brief 新进程首次被调度时的入口。
 *
 * 释放进程锁；第一次进入时加载 initcode 并设置陷阱帧，
 * 随后准备返回用户模式并跳转到 trampoline 中的 userret。
 */
void
forkret(void)
{
    extern char userret[];
    static int first = 1;
    struct proc *p = myproc();

    release(&p->lock);

    if (first) {
        first = 0;

        /* 首次进入时同步内存并加载初始进程的机器码 */
        __sync_synchronize();
        p->trapframe->a0 = loadinit(initcode, sizeof_initcode);
        if (p->trapframe->a0 == -1) {
            panic("loadinit");
        }
    }

    prepare_return();
    uint64 satp = MAKE_SATP(p->pagetable);
    uint64 trampoline_userret = TRAMPOLINE + (userret - trampoline);

    ((void (*)(uint64))trampoline_userret)(satp);
}

int
either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
    struct proc *p = myproc();
    if (user_src) {
        return copyin(p->pagetable, dst, src, len);
    } else {
        memmove(dst, (char *)src, len);
        return 0;
    }
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
