#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

extern char trampoline[], uservec[];  /* 跳板页代码与 uservec 入口地址 */

struct spinlock tickslock;
uint ticks;

void kernelvec(void);

void
trapinit(void)
{
    initlock(&tickslock, "time");
}

void
trapinithart(void)
{
    w_stvec((uint64)kernelvec);

    w_sie(r_sie() | SIE_SEIE | SIE_STIE);
}

/**
 * @brief 处理来自用户模式的陷阱。
 *
 * 保存用户 pc 后处理设备中断，计时器中断时调用 yield 让出 CPU。
 * 系统调用尚未实现，其余无法识别的陷阱直接 panic。
 *
 * @return 用户页表的 satp 值，供 userret 切换地址空间
 */
uint64
usertrap(void)
{
    int which_dev = 0;

    if ((r_sstatus() & SSTATUS_SPP) != 0)
        panic("usertrap: not from user mode");

    w_stvec((uint64)kernelvec);

    struct proc *p = myproc();
    p->trapframe->epc = r_sepc();

    if (r_scause() == 8) {
        p->trapframe->epc += 4;
        intr_on();
        syscall();
    } else if ((which_dev = devintr()) != 0) {
    } else {
        printf("usertrap(): unexpected scause 0x%lx pid=%d\n", r_scause(), p->pid);
        printf("            sepc=0x%lx stval=0x%lx\n", r_sepc(), r_stval());
        panic("usertrap: should be killed but we have not support killing!");
    }

    if (which_dev == 2)
        yield();

    prepare_return();

    uint64 satp = MAKE_SATP(p->pagetable);

    return satp;
}

/**
 * @brief 准备返回用户模式所需的陷阱帧与 CSR。
 *
 * 将内核 satp、栈指针、陷阱入口和 hart ID 写入陷阱帧；
 * 设置 stvec 指向 uservec，并配置 sstatus 与 sepc 以便 sret 返回用户模式。
 */
void
prepare_return(void)
{
    struct proc *p = myproc();

    intr_off();

    uint64 trampoline_uservec = TRAMPOLINE + (uservec - trampoline);
    w_stvec(trampoline_uservec);

    p->trapframe->kernel_satp = r_satp();
    p->trapframe->kernel_sp = p->kstack + PGSIZE;
    p->trapframe->kernel_trap = (uint64)usertrap;
    p->trapframe->kernel_hartid = r_tp();

    uint64 x = r_sstatus();
    x &= ~SSTATUS_SPP;
    x |= SSTATUS_SPIE;
    w_sstatus(x);

    w_sepc(p->trapframe->epc);
}

void
kerneltrap(void)
{
    int which_dev = 0;
    uint64 sstatus = r_sstatus();
    uint64 sepc = r_sepc();
    uint64 scause = r_scause();

    if ((sstatus & SSTATUS_SPP) == 0)
        panic("kerneltrap: not from supervisor mode");

    if (intr_get() != 0)
        panic("kerneltrap: interrupts enabled");

    if ((which_dev = devintr()) == 0) {
        printf("scause=0x%lx sepc=0x%lx stval=0x%lx\n", scause, r_sepc(), r_stval());
        panic("kerneltrap");
    }

    if (which_dev == 2 && myproc() != 0)
        yield();

    w_sepc(sepc);
    w_sstatus(sstatus);
}

int
devintr()
{
    uint64 scause = r_scause();

    if (scause == 0x8000000000000009L) {
        int irq = plic_claim();

        if (irq == UART0_IRQ) {
            uartintr();
        } else if (irq) {
            printf("unexpected interrupt irq=%d\n", irq);
        }
        if (irq)
            plic_complete(irq);

        return 1;
    } else if (scause == 0x8000000000000005L) {
        clockintr();

        return 2;
    } else {
        return 0;
    }
}
