/* printf.c */
void printfinit(void);
int printf(char*, ...) __attribute__((format (printf, 1, 2)));
void panic(char*) __attribute__((noreturn));

/* string.c */
void *memset(void *, int, uint);

/* kalloc.c */
void *kalloc(void);
void kfree(void *);
void kinit(void);
