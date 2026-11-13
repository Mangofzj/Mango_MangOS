/* 上下文 */
struct context {
    uint64 ra;  /* 返回地址 */
    uint64 sp;  /* 栈指针 */

    /* 被调用者保存寄存器 */
    uint64 s0;
    uint64 s1;
    uint64 s2;
    uint64 s3;
    uint64 s4;
    uint64 s5;
    uint64 s6;
    uint64 s7;
    uint64 s8;
    uint64 s9;
    uint64 s10;
    uint64 s11;
};

/* CPU 状态 */
struct cpu {
    struct proc *proc;  /* 当前运行的进程 */
    struct context context;  /* 调度器上下文 */
    int noff;  /* 中断关闭嵌套层数 */
    int intena;  /* 中断原始使能状态 */
};

/* CPU 状态数组 */
extern struct cpu cpus[NCPU];

/* 陷阱帧：保存用户现场及返回内核所需信息 */
struct trapframe {
    /*   0 */  uint64 kernel_satp;
    /*   8 */  uint64 kernel_sp;
    /*  16 */  uint64 kernel_trap;
    /*  24 */  uint64 epc;
    /*  32 */  uint64 kernel_hartid;
    /*  40 */  uint64 ra;
    /*  48 */  uint64 sp;
    /*  56 */  uint64 gp;
    /*  64 */  uint64 tp;
    /*  72 */  uint64 t0;
    /*  80 */  uint64 t1;
    /*  88 */  uint64 t2;
    /*  96 */  uint64 s0;
    /* 104 */  uint64 s1;
    /* 112 */  uint64 a0;
    /* 120 */  uint64 a1;
    /* 128 */  uint64 a2;
    /* 136 */  uint64 a3;
    /* 144 */  uint64 a4;
    /* 152 */  uint64 a5;
    /* 160 */  uint64 a6;
    /* 168 */  uint64 a7;
    /* 176 */  uint64 s2;
    /* 184 */  uint64 s3;
    /* 192 */  uint64 s4;
    /* 200 */  uint64 s5;
    /* 208 */  uint64 s6;
    /* 216 */  uint64 s7;
    /* 224 */  uint64 s8;
    /* 232 */  uint64 s9;
    /* 240 */  uint64 s10;
    /* 248 */  uint64 s11;
    /* 256 */  uint64 t3;
    /* 264 */  uint64 t4;
    /* 272 */  uint64 t5;
    /* 280 */  uint64 t6;
};

/* 进程状态 */
enum procstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

/* 进程控制块 */
struct proc {
    struct spinlock lock;  /* 进程锁 */

    enum procstate state;  /* 进程状态 */
    void *chan;  /* 睡眠等待通道 */
    int pid;  /* 进程 ID */

    uint64 kstack;  /* 内核栈虚拟地址 */
    uint64 sz;  /* 进程内存大小 */
    pagetable_t pagetable;  /* 用户页表根指针 */
    struct trapframe *trapframe;  /* 陷阱帧指针 */
    struct context context;  /* 进程上下文 */
    char name[16];  /* 进程名称 */
};
