/* system calls */
int read(int, void *, int);
int getpid(void);
int pause(int);
int uptime(void);
int write(int, const void *, int);

/* ulib.c */
char *gets(char *, int max);
uint strlen(const char *);
void *memset(void *, int, uint);

/* printf.c */
void fprintf(int, const char *, ...) __attribute__ ((format (printf, 2, 3)));
void printf(const char *, ...) __attribute__ ((format (printf, 1, 2)));
