int read(int, void *, int);
int getpid(void);
int pause(int);
int uptime(void);
int write(int, const void *, int);

void fprintf(int, const char *, ...) __attribute__ ((format (printf, 2, 3)));
void printf(const char *, ...) __attribute__ ((format (printf, 1, 2)));
