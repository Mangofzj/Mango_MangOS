#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct cpu cpus[NCPU];
struct proc proc[NPROC];
int nextpid = 1;
struct spinlock pid_lock;

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

int
cpuid()
{
    int id = r_tp();
    return id;
}

struct cpu *
mycpu(void)
{
    int id = cpuid();
    struct cpu *c = &cpus[id];
    return c;
}

struct proc *
myproc(void)
{
    push_off();
    struct cpu *c = mycpu();
    struct proc *p = c->proc;
    pop_off();

    return p;
}

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
    p->pid = allocpid();
    p->state = USED;

    memset(&p->context, 0, sizeof(p->context));
    p->context.ra = (uint64)kerneltrapret;
    p->context.sp = p->kstack + PGSIZE;
    p->start = start_routin;

    return p;
}

void
userinit(void)
{
    struct proc *p;

    p = allocproc(user_task0);
    if (p == 0)
        panic("userinit: allocproc task0 failed");
    p->state = RUNNABLE;
    release(&p->lock);

    p = allocproc(user_task1);
    if (p == 0)
        panic("userinit: allocproc task1 failed");
    p->state = RUNNABLE;
    release(&p->lock);
}

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

    intena = mycpu()->intena;
    swtch(&p->context, &mycpu()->context);
    mycpu()->intena = intena;
}

void
yield(void)
{
    struct proc *p = myproc();

    acquire(&p->lock);
    p->state = RUNNABLE;
    sched();
    release(&p->lock);
}

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
