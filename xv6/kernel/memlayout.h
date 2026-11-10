/* 内核物理内存范围 */
#define KERNBASE 0x80200000L  /* 内核起始地址 */
#define PHYSTOP  0x82200000L  /* 可用物理内存上限 */

/* UART0 基地址与中断源 */
#define UART0     0x10000000L  /* UART0 寄存器基地址 */
#define UART0_IRQ 10  /* UART0 在 PLIC 中的中断源编号 */

/* PLIC 寄存器 */
#define PLIC                  0x0c000000L  /* PLIC 基地址 */
#define PLIC_PRIORITY        (PLIC + 0x0)  /* 中断源优先级基址 */
#define PLIC_PENDING         (PLIC + 0x1000)  /* 中断挂起位 */
#define PLIC_SENABLE(hart)   (PLIC + 0x2080 + (hart) * 0x100)  /* S 模式中断使能位图 */
#define PLIC_SPRIORITY(hart) (PLIC + 0x201000 + (hart) * 0x2000)  /* S 模式优先级阈值 */
#define PLIC_SCLAIM(hart)    (PLIC + 0x201004 + (hart) * 0x2000)  /* 中断认领与完成寄存器 */

#define KSTACK(p) (TRAMPOLINE - ((p) + 1) * 2 * PGSIZE)  /* 第 p 个进程的内核栈地址，每个进程间隔 2 页 */

#define TRAMPOLINE (MAXVA - PGSIZE)  /* 跳板页虚拟地址 */
#define TRAPFRAME  (TRAMPOLINE - PGSIZE)  /* 陷阱帧虚拟地址 */
