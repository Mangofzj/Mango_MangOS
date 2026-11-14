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
