#ifndef __ASSEMBLER__

/* sstatus 与 sie 位定义 */
#define SSTATUS_SPP (1L << 8)  /* 陷入前的特权模式，1 为 S 模式 */
#define SSTATUS_SIE (1L << 1)  /* S 模式中断使能 */
#define SSTATUS_SPIE (1L << 5)  /* sret 恢复的中断使能 */
#define SIE_SEIE (1L << 9)  /* S 模式外部中断使能 */
#define SIE_STIE (1L << 5)  /* S 模式时钟中断使能 */

/**
 * @brief 读取 sstatus 寄存器。
 *
 * @return sstatus 寄存器的值
 */
static inline uint64
r_sstatus()
{
    uint64 x;
    asm volatile("csrr %0, sstatus" : "=r" (x));
    return x;
}

/**
 * @brief 写入 sstatus 寄存器。
 *
 * @param x 待写入的值
 */
static inline void
w_sstatus(uint64 x)
{
    asm volatile("csrw sstatus, %0" : : "r" (x));
}

/**
 * @brief 写入 stvec 寄存器。
 *
 * @param x 待写入的值
 */
static inline void
w_stvec(uint64 x)
{
    asm volatile("csrw stvec, %0" : : "r" (x));
}

/**
 * @brief 读取 sepc 寄存器。
 *
 * @return sepc 寄存器的值
 */
static inline uint64
r_sepc()
{
    uint64 x;
    asm volatile("csrr %0, sepc" : "=r" (x));
    return x;
}

/**
 * @brief 写入 sepc 寄存器。
 *
 * @param x 待写入的值
 */
static inline void
w_sepc(uint64 x)
{
    asm volatile("csrw sepc, %0" : : "r" (x));
}

/**
 * @brief 读取 scause 寄存器。
 *
 * @return scause 寄存器的值
 */
static inline uint64
r_scause()
{
    uint64 x;
    asm volatile("csrr %0, scause" : "=r" (x));
    return x;
}

/**
 * @brief 读取 stval 寄存器。
 *
 * @return stval 寄存器的值
 */
static inline uint64
r_stval()
{
    uint64 x;
    asm volatile("csrr %0, stval" : "=r" (x));
    return x;
}

/**
 * @brief 读取 sie 寄存器。
 *
 * @return sie 寄存器的值
 */
static inline uint64
r_sie()
{
    uint64 x;
    asm volatile("csrr %0, sie" : "=r" (x));
    return x;
}

/**
 * @brief 写入 sie 寄存器。
 *
 * @param x 待写入的值
 */
static inline void
w_sie(uint64 x)
{
    asm volatile("csrw sie, %0" : : "r" (x));
}

/**
 * @brief 读取 time 寄存器。
 *
 * @return time 寄存器的值
 */
static inline uint64
r_time()
{
    uint64 x;
    asm volatile("csrr %0, time" : "=r" (x));
    return x;
}

/**
 * @brief 读取 tp 寄存器。
 *
 * 其中保存当前 hart ID。
 *
 * @return tp 寄存器的值
 */
static inline uint64
r_tp()
{
    uint64 x;
    asm volatile("mv %0, tp" : "=r" (x));
    return x;
}

/**
 * @brief 查询中断是否使能。
 *
 * @return 中断使能时返回非 0，否则返回 0
 */
static inline int
intr_get()
{
    uint64 x = r_sstatus();
    return (x & SSTATUS_SIE) != 0;
}

/**
 * @brief 使能中断。
 */
static inline void
intr_on()
{
    w_sstatus(r_sstatus() | SSTATUS_SIE);
}

/**
 * @brief 关闭中断。
 */
static inline void
intr_off()
{
    w_sstatus(r_sstatus() & ~SSTATUS_SIE);
}

/* Sv39 分页模式值 */
#define SATP_SV39 (8L << 60)

/* 由页表根物理地址构造 satp 值 */
#define MAKE_SATP(pagetable) (SATP_SV39 | (((uint64)pagetable) >> 12))

typedef uint64 pte_t;  /* 页表项 */
typedef uint64 *pagetable_t;  /* 页表指针 */

/**
 * @brief 读取 satp 寄存器。
 *
 * @return satp 寄存器的值
 */
static inline uint64
r_satp()
{
    uint64 x;
    asm volatile("csrr %0, satp" : "=r" (x));
    return x;
}

/**
 * @brief 写入 satp 寄存器。
 *
 * @param x 待写入的值
 */
static inline void
w_satp(uint64 x)
{
    asm volatile("csrw satp, %0" : : "r" (x));
}

/**
 * @brief 刷新所有地址转换缓存（TLB）。
 */
static inline void
sfence_vma()
{
    asm volatile("sfence.vma zero, zero");
}

#endif

/* 页大小与对齐 */
#define PGSIZE  4096  /* 页大小，4 KiB */
#define PGSHIFT 12  /* 页大小的移位位数 */

#define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1))  /* 向上取整到页边界 */
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE-1))  /* 向下取整到页边界 */

/* 页表项标志 */
#define PTE_V (1L << 0)  /* 有效 */
#define PTE_R (1L << 1)  /* 可读 */
#define PTE_W (1L << 2)  /* 可写 */
#define PTE_X (1L << 3)  /* 可执行 */
#define PTE_U (1L << 4)  /* 用户态可访问 */

/* 页表项与物理地址转换 */
#define PA2PTE(pa)  ((((uint64)pa) >> 12) << 10)  /* 物理地址转页表项 */
#define PTE2PA(pte) (((pte) >> 10) << 12)  /* 页表项转物理地址 */

#define PTE_FLAGS(pte) ((pte) & 0x3FF)  /* 取低 10 位标志 */

/* Sv39 页表索引 */
#define PXMASK         0x1FF  /* 页表索引掩码（9 位） */
#define PXSHIFT(level) (PGSHIFT + (9 * (level)))  /* 第 level 级页表索引的移位位数 */
#define PX(level, va)  ((((uint64)(va)) >> PXSHIFT(level)) & PXMASK)  /* 取第 level 级页表索引 */

#define MAXVA (1L << (9 + 9 + 9 + 12 - 1))  /* Sv39 虚拟地址空间上限 */
