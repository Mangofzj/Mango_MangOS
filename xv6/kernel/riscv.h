#ifndef __ASSEMBLER__

#define SSTATUS_SPP (1L << 8)
#define SSTATUS_SIE (1L << 1)
#define SSTATUS_SPIE (1L << 5)
#define SIE_SEIE (1L << 9)
#define SIE_STIE (1L << 5)

static inline uint64
r_sstatus()
{
    uint64 x;
    asm volatile("csrr %0, sstatus" : "=r" (x));
    return x;
}

static inline void
w_sstatus(uint64 x)
{
    asm volatile("csrw sstatus, %0" : : "r" (x));
}

static inline void
w_stvec(uint64 x)
{
    asm volatile("csrw stvec, %0" : : "r" (x));
}

static inline uint64
r_sepc()
{
    uint64 x;
    asm volatile("csrr %0, sepc" : "=r" (x));
    return x;
}

static inline void
w_sepc(uint64 x)
{
    asm volatile("csrw sepc, %0" : : "r" (x));
}

static inline uint64
r_scause()
{
    uint64 x;
    asm volatile("csrr %0, scause" : "=r" (x));
    return x;
}

static inline uint64
r_stval()
{
    uint64 x;
    asm volatile("csrr %0, stval" : "=r" (x));
    return x;
}

static inline int
intr_get()
{
    uint64 x = r_sstatus();
    return (x & SSTATUS_SIE) != 0;
}

static inline uint64
r_sie()
{
    uint64 x;
    asm volatile("csrr %0, sie" : "=r" (x));
    return x;
}

static inline void
w_sie(uint64 x)
{
    asm volatile("csrw sie, %0" : : "r" (x));
}

static inline uint64
r_time()
{
    uint64 x;
    asm volatile("csrr %0, time" : "=r" (x));
    return x;
}

static inline void
intr_on()
{
    w_sstatus(r_sstatus() | SSTATUS_SIE);
}

static inline void
intr_off()
{
    w_sstatus(r_sstatus() & ~SSTATUS_SIE);
}

#endif

#define PGSIZE  4096
#define PGSHIFT 12

#define PGROUNDUP(sz)  (((sz)+PGSIZE-1) & ~(PGSIZE-1))
#define PGROUNDDOWN(a) (((a)) & ~(PGSIZE-1))
