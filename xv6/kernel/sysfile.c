#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

extern int consoleread(int user_dst, uint64 dst, int n);
extern int consolewrite(int user_src, uint64 src, int n);

/**
 * @brief 从控制台读取数据。
 *
 * @note 第 0 个参数 fd 未使用。
 * @return 实际读取的字节数
 */
uint64
sys_read(void)
{
    int n;
    uint64 p;

    argaddr(1, &p);
    argint(2, &n);

    return consoleread(1, p, n);
}

/**
 * @brief 向控制台写入数据。
 *
 * @note 第 0 个参数 fd 未使用。
 * @return 实际写入的字节数
 */
uint64
sys_write(void)
{
    int n;
    uint64 p;

    argaddr(1, &p);
    argint(2, &n);

    return consolewrite(1, p, n);
}

uint64
sys_exec(void)
{
    char path[MAXPATH], *argv[MAXARG];
    int i;
    uint64 uargv, uarg;

    argaddr(1, &uargv);
    if (argstr(0, path, MAXPATH) < 0) {
        return -1;
    }
    memset(argv, 0, sizeof(argv));

    for (i = 0;; i++) {
        if (i >= NELEM(argv)) {
            goto bad;
        }
        if (fetchaddr(uargv + sizeof(uint64) * i, (uint64 *)&uarg) < 0) {
            goto bad;
        }
        if (uarg == 0) {
            argv[i] = 0;
            break;
        }
        argv[i] = kalloc();
        if (argv[i] == 0)
            goto bad;
        if (fetchstr(uarg, argv[i], PGSIZE) < 0)
            goto bad;
    }

    int ret = kexec(path, argv);
    for (i = 0; i < NELEM(argv) && argv[i] != 0; i++)
        kfree(argv[i]);

    return ret;

bad:
    for (i = 0; i < NELEM(argv) && argv[i] != 0; i++)
        kfree(argv[i]);

    return -1;
}
