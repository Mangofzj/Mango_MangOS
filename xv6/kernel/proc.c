#include "types.h"
#include "param.h"
#include "riscv.h"
#include "proc.h"
#include "defs.h"

struct cpu cpus[NCPU];
struct proc proc[NPROC];
int nextpid = 1;

void
procinit(void)
{
    struct proc *p;

    for (p = proc; p < &proc[NPROC]; p++) {
        p->state = UNUSED;
        p->kstack = (uint64)kalloc();
        if (p->kstack == 0)
            panic("procinit: kalloc");
    }
}

int
cpuid()
{
    return 0;
}

struct cpu *
mycpu(void)
{
    int id = cpuid();
    struct cpu *c = &cpus[id];
    return c;
}

int
allocpid()
{
    int pid;

    pid = nextpid;
    nextpid = nextpid + 1;

    return pid;
}

static struct proc *
allocproc(void (*start_routin)(void))
{
    struct proc *p;

    for (p = proc; p < &proc[NPROC]; p++) {
        if (p->state == UNUSED) {
            goto found;
        }
    }
    return 0;

found:
    p->pid = allocpid();
    p->state = USED;

    memset(&p->context, 0, sizeof(p->context));
    p->context.ra = (uint64)start_routin;
    p->context.sp = p->kstack + PGSIZE;

    return p;
}

void
userinit(void)
{
    struct proc *p;

    p = allocproc(user_task0);
    p->state = RUNNABLE;
}

void
scheduler(void)
{
    struct proc *p;
    struct cpu *c = mycpu();

    c->proc = 0;
    for (;;) {
        for (p = proc; p < &proc[NPROC]; p++) {
            if (p->state == RUNNABLE) {
                p->state = RUNNING;
                c->proc = p;
                swtch(&c->context, &p->context);
                c->proc = 0;
            }
        }
    }
}
