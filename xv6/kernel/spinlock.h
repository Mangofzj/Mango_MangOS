/* 自旋锁 */
struct spinlock {
    uint locked;  /* 锁状态 */
    char *name;  /* 锁名称 */
    struct cpu *cpu;  /* 持有锁的 CPU */
};
