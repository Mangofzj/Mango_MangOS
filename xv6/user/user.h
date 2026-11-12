int getpid(void);
int write(int, const void *, int);

void fprintf(int, const char *, ...) __attribute__ ((format (printf, 2, 3)));
void printf(const char *, ...) __attribute__ ((format (printf, 1, 2)));
