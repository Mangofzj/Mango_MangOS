struct context;
struct proc;

/* printf.c */
void printfinit(void);
int printf(char *, ...) __attribute__((format (printf, 1, 2)));
void panic(char *) __attribute__((noreturn));

/* string.c */
void *memset(void *, int, uint);

/* kalloc.c */
void *kalloc(void);
void kfree(void *);
void kinit(void);

/* proc.c */
int cpuid(void);
struct cpu *mycpu(void);
struct proc *myproc(void);
void procinit(void);
void userinit(void);
void scheduler(void) __attribute__((noreturn));
void sched(void);
void yield(void);

/* swtch.S */
void swtch(struct context *, struct context *);

/* trap.c */
void trapinit(void);
void trapinithart(void);

/* uart.c */
void uartinit(void);
int uartgetc(void);
void uartintr(void);

/* plic.c */
void plicinit(void);
void plicinithart(void);
int plic_claim(void);
void plic_complete(int);

/* trap.c */
int devintr(void);

/* user.c */
void user_task0(void);
void user_task1(void);
