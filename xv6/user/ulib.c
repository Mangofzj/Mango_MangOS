#include "kernel/types.h"
#include "kernel/riscv.h"
#include "kernel/vm.h"
#include "user/user.h"

char *sbrk(int);
char *sbrklazy(int);

/**
 * @brief 用户程序入口：调用 main 并以返回值退出。
 *
 * @param argc 参数个数
 * @param argv 参数数组
 */
void
start(int argc, char **argv)
{
    int r;
    extern int main(int argc, char **argv);

    r = main(argc, argv);
    exit(r);
}

/**
 * @brief 比较两个字符串。
 *
 * @param p 第一个字符串
 * @param q 第二个字符串
 * @return 首个不同字符的字节差值；完全相同返回 0
 */
int
strcmp(const char *p, const char *q)
{
    while (*p && *p == *q)
        p++, q++;

    return (uchar)*p - (uchar)*q;
}

/**
 * @brief 计算字符串长度。
 *
 * @param s 字符串
 * @return 长度，不含结尾 '\0'
 */
uint
strlen(const char *s)
{
    int n;

    for (n = 0; s[n]; n++)
        ;

    return n;
}

/**
 * @brief 将一段内存填充为指定字节。
 *
 * @param dst 目标地址
 * @param c 填充值
 * @param n 填充字节数
 * @return 目标地址 dst
 */
void *
memset(void *dst, int c, uint n)
{
    char *cdst = (char *)dst;

    for (int i = 0; i < n; i++) {
        cdst[i] = c;
    }

    return dst;
}

/**
 * @brief 从标准输入读取一行到缓冲区。
 *
 * 最多读取 max - 1 个字符，遇到换行、回车或读取失败时结束，并以 '\0' 结尾。
 *
 * @param buf 目标缓冲区
 * @param max 缓冲区大小
 * @return 目标缓冲区 buf
 */
char *
gets(char *buf, int max)
{
    int i, cc;
    char c;

    for (i = 0; i + 1 < max;) {
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
 * @brief 以立即分配方式调整当前进程的用户内存。
 *
 * @param n 增加或减少的字节数
 * @return 调整前的内存大小；失败时返回 SBRK_ERROR
 */
char *
sbrk(int n)
{
    return sys_sbrk(n, SBRK_EAGER);
}

/**
 * @brief 以延迟分配方式调整当前进程的用户内存。
 *
 * @param n 增加或减少的字节数
 * @return 调整前的内存大小；失败时返回 SBRK_ERROR
 */
char *
sbrklazy(int n)
{
    return sys_sbrk(n, SBRK_LAZY);
}
