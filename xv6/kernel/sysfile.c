#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

extern int consolewrite(int user_src, uint64 src, int n);

uint64
sys_write(void)
{
    int n;
    uint64 p;

    argaddr(1, &p);
    argint(2, &n);

    return consolewrite(1, p, n);
}
