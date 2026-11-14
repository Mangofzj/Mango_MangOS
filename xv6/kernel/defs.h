struct context;
struct proc;
struct spinlock;

#define NELEM(x) (sizeof(x) / sizeof((x)[0]))

/* printf.c */
void printfinit(void);
int printf(char *, ...) __attribute__((format (printf, 1, 2)));
void panic(char *) __attribute__((noreturn));

/* string.c */
void *memset(void *, int, uint);
void *memmove(void *dst, const void *src, uint n);
char *safestrcpy(char *, const char *, int);
int strlen(const char *);

/* kalloc.c */
void *kalloc(void);
void kfree(void *);
void kinit(void);

/* vm.c */
void kvminit(void);
void kvminithart(void);
void kvmmap(pagetable_t, uint64, uint64, uint64, int);
int mappages(pagetable_t, uint64, uint64, uint64, int);
pte_t *walk(pagetable_t, uint64, int);
uint64 walkaddr(pagetable_t, uint64);
pagetable_t uvmcreate(void);
void uvmfree(pagetable_t, uint64);
void uvmunmap(pagetable_t, uint64, uint64, int);
int copyin(pagetable_t, char *, uint64, uint64);
int copyout(pagetable_t, uint64, char *, uint64);

/* proc.c */
int cpuid(void);
struct cpu *mycpu(void);
struct proc *myproc(void);
void proc_mapstacks(pagetable_t);
void procinit(void);
void userinit(void);
void scheduler(void) __attribute__((noreturn));
void sched(void);
void yield(void);
void sleep(void *, struct spinlock *);
void wakeup(void *);
int either_copyin(void *dst, int user_src, uint64 src, uint64 len);
int either_copyout(int user_dst, uint64 dst, void *src, uint64 len);
void procdump(void);
pagetable_t proc_pagetable(struct proc *);
void proc_freepagetable(pagetable_t, uint64);

/* swtch.S */
void swtch(struct context *, struct context *);

/* trap.c */
extern uint ticks;
extern struct spinlock tickslock;
void trapinit(void);
void trapinithart(void);
int devintr(void);
void prepare_return(void);

/* syscall.c */
void argint(int, int *);
void argaddr(int, uint64 *);
void syscall(void);

/* uart.c */
void uartinit(void);
void uartputc_sync(int);
void uartwrite(char *, int);
int uartgetc(void);
void uartintr(void);

/* plic.c */
void plicinit(void);
void plicinithart(void);
int plic_claim(void);
void plic_complete(int);

/* timer.c */
void timerinit(void);
void clockintr(void);

/* spinlock.c */
void push_off(void);
void pop_off(void);
void initlock(struct spinlock *, char *);
int holding(struct spinlock *);
void acquire(struct spinlock *);
void release(struct spinlock *);

/* console.c */
void consoleinit(void);
void consoleintr(int);
void consputc(int);
int consoleread(int user_dst, uint64 dst, int n);
int consolewrite(int user_src, uint64 src, int n);

/* user.c */
void user_task0(void);
void user_task1(void);
void user_shell(void);
