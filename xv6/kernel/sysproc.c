#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"
#include "defs.h"

/**
 * @brief 结束当前进程。
 *
 * 取第 0 个参数作为退出状态并调用 kexit；kexit 正常路径不返回。
 *
 * @return 固定返回 0
 */
uint64
sys_exit(void)
{
    int n;

    argint(0, &n);
    kexit(n);

    return 0;
}

/**
 * @brief 获取当前进程 ID。
 *
 * @return 当前进程 ID
 */
uint64
sys_getpid(void)
{
    return myproc()->pid;
}

/**
 * @brief 创建子进程。
 *
 * @return 子进程 ID；创建失败时返回 -1
 */
uint64
sys_fork(void)
{
    return kfork();
}

/**
 * @brief 等待子进程退出。
 *
 * 第 0 个参数为退出状态的用户地址，允许为 0。
 *
 * @return 退出的子进程 ID；失败时返回 -1
 */
uint64
sys_wait(void)
{
    uint64 p;

    argaddr(0, &p);

    return kwait(p);
}

/**
 * @brief 终止指定进程。
 *
 * 第 0 个参数为目标进程 ID。
 *
 * @return 成功返回 0；未找到进程时返回 -1
 */
uint64
sys_kill(void)
{
    int pid;

    argint(0, &pid);

    return kkill(pid);
}

/**
 * @brief 调整当前进程的用户内存大小。
 *
 * 第 0 个参数为增量字节数，第 1 个参数为 SBRK_EAGER 或 SBRK_LAZY；
 * 立即分配或收缩时调用 growproc，延迟分配时只更新进程大小。
 *
 * @return 调整前的内存大小；参数越界或分配失败时返回 -1
 */
uint64
sys_sbrk(void)
{
    uint64 addr;
    int t;
    int n;

    argint(0, &n);
    argint(1, &t);

    addr = myproc()->sz;
    if (t == SBRK_EAGER || n < 0) {
        if (growproc(n) < 0)
            return -1;
    } else {
        /* 延迟分配模式只更新进程大小，并检查地址溢出与上限 */
        if (addr + n < addr)
            return -1;
        if (addr + n > TRAPFRAME)
            return -1;
        myproc()->sz += n;
    }

    return addr;
}

/**
 * @brief 暂停当前进程指定的时钟 tick 数。
 *
 * 第 0 个参数为 tick 数，负数按 0 处理；等待期间可被终止。
 *
 * @return 成功返回 0；被标记终止时返回 -1
 */
uint64
sys_pause(void)
{
    int n;
    uint ticks0;

    argint(0, &n);
    if (n < 0)
        n = 0;

    acquire(&tickslock);
    ticks0 = ticks;
    while (ticks - ticks0 < n) {
        if (killed(myproc())) {
            release(&tickslock);
            return -1;
        }

        sleep(&ticks, &tickslock);
    }
    release(&tickslock);

    return 0;
}

/**
 * @brief 获取系统启动以来的时钟 tick 数。
 *
 * @return 当前 tick 数
 */
uint64
sys_uptime(void)
{
    uint xticks;

    acquire(&tickslock);
    xticks = ticks;
    release(&tickslock);

    return xticks;
}
