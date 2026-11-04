#include "types.h"
#include "defs.h"

/**
 * @brief 忙等延时。
 *
 * 将计数值放大后空转递减，延时期间不放弃 CPU。
 *
 * @param count 延时计数值
 */
static void
task_delay(volatile int count)
{
    count *= 50000;
    while (count--);
}

#define DELAY 1000  /* 任务延时计数值 */

/**
 * @brief 任务 0：循环打印运行信息并忙等延时。
 */
void
user_task0(void)
{
    printf("Task 0: Created!\n");
    while (1) {
        task_delay(DELAY);
    }
}

/**
 * @brief 任务 1：循环打印运行信息并忙等延时。
 */
void
user_task1(void)
{
    printf("Task 1: Created!\n");
    while (1) {
        task_delay(DELAY);
    }
}

/**
 * @brief 从控制台读取数据。
 *
 * @param buf 目标缓冲区
 * @param count 最大读取字节数
 * @note fd 参数未使用。
 * @return 实际读取的字节数
 */
int
read(int fd, void *buf, int count)
{
    return consoleread((uint64)buf, count);
}

/**
 * @brief 向控制台写入数据。
 *
 * @param buf 源缓冲区
 * @param count 写入字节数
 * @note fd 参数未使用。
 * @return 写入的字节数，等于 count
 */
int
write(int fd, const void *buf, int count)
{
    return consolewrite((uint64)buf, count);
}

/**
 * @brief 从控制台读取一行到缓冲区。
 *
 * 最多读取 max - 1 个字符，遇到换行或回车时结束，并以 '\0' 结尾。
 *
 * @param buf 目标缓冲区
 * @param max 缓冲区大小
 * @return 指向 buf 的指针
 */
char *
gets(char *buf, int max)
{
    int i, cc;
    char c;

    for (i = 0; i + 1 < max; ) {
        cc = read(0, &c, 1);
        if (cc < 1)
            break;

        buf[i++] = c;

        if (c == '\n' || c == '\r')
            break;
    }

    buf[i] = '\0';
    return buf;
}

/**
 * @brief 打印提示符并读取一条命令。
 *
 * @param buf 命令缓冲区
 * @param nbuf 缓冲区大小
 * @return 输入为空时返回 -1，否则返回 0
 */
int
getcmd(char *buf, int nbuf)
{
    write(2, "$ ", 2);
    memset(buf, 0, nbuf);
    gets(buf, nbuf);

    if (buf[0] == 0)
        return -1;

    return 0;
}

void
runcmd(char *cmd)
{
    printf("Running command: '%s', unsupported!\n", cmd);
}

void
user_shell(void)
{
    static char buf[100];

    while (getcmd(buf, sizeof(buf)) >= 0) {
        char *cmd = buf;
        while (*cmd == ' ' || *cmd == '\t')
            cmd++;
        if (*cmd == '\n')
            continue;
        cmd[strlen(cmd) - 1] = '\0';
        runcmd(cmd);
    }
}
