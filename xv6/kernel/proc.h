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

/* 进程状态 */
enum procstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

/* 进程控制块 */
struct proc {
    struct spinlock lock;  /* 进程锁 */

    enum procstate state;  /* 进程状态 */
    int pid;  /* 进程 ID */

    uint64 kstack;  /* 内核栈虚拟地址 */
    struct context context;  /* 进程上下文 */
    char name[16];  /* 进程名称 */
    void (*start)(void);  /* 任务初始入口 */
};
