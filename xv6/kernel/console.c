#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"
#include "sbi.h"

#define C(x) ((x) - '@')  /* 字母到控制字符的转换 */
#define BACKSPACE 0x100  /* 退格伪字符 */

/* 控制台输入缓冲 */
struct {
    struct spinlock lock;  /* 保护输入缓冲的自旋锁 */

#define INPUT_BUF_SIZE 128  /* 输入缓冲大小 */
    char buf[INPUT_BUF_SIZE];  /* 输入缓冲 */
    uint r;  /* 读位置 */
    uint w;  /* 写位置 */
    uint e;  /* 编辑位置 */
} cons;

/**
 * @brief 初始化控制台：初始化输入缓冲锁并使能 UART 接收中断。
 */
void
consoleinit(void)
{
    initlock(&cons.lock, "cons");

    uartinit();
}

/**
 * @brief 经 SBI 控制台输出一个字符。
 *
 * BACKSPACE 输出为 "\b \b"，用空格覆盖原字符后回退光标。
 *
 * @param c 待输出的字符
 */
void
consputc(int c)
{
    if (c == BACKSPACE) {
        sbi_console_putchar('\b');
        sbi_console_putchar(' ');
        sbi_console_putchar('\b');
    } else {
        sbi_console_putchar(c);
    }
}

/**
 * @brief 处理控制台输入字符：更新输入缓冲、回显并处理行编辑。
 *
 * Ctrl-P 打印进程信息，Ctrl-U 删除当前行，Ctrl-H 或 DEL 删除一个字符；
 * 其他字符写入缓冲并回显，遇到换行、Ctrl-D 或缓冲写满时提交给 consoleread。
 *
 * @param c 接收到的字符
 */
void
consoleintr(int c)
{
    acquire(&cons.lock);

    switch (c) {
    case C('P'):
        /* Ctrl-P：打印进程信息 */
        procdump();
        break;

    case C('U'):
        /* Ctrl-U：删除当前行已输入的内容 */
        while (cons.e != cons.w && cons.buf[(cons.e - 1) % INPUT_BUF_SIZE] != '\n') {
            cons.e--;
            consputc(BACKSPACE);
        }
        break;

    case C('H'):
    case '\x7f':
        /* Ctrl-H 或 DEL：退格删除一个字符 */
        if (cons.e != cons.w) {
            cons.e--;
            consputc(BACKSPACE);
        }
        break;

    default:
        /* 缓冲未满时写入字符并回显 */
        if (c != 0 && cons.e - cons.r < INPUT_BUF_SIZE) {
            c = (c == '\r') ? '\n' : c;  /* 回车统一为换行 */
            consputc(c);

            cons.buf[cons.e++ % INPUT_BUF_SIZE] = c;

            /* 换行、Ctrl-D 或缓冲写满时提交数据 */
            if (c == '\n' || c == C('D') || cons.e - cons.r == INPUT_BUF_SIZE) {
                cons.w = cons.e;
            }
        }
        break;
    }

    release(&cons.lock);
}

/**
 * @brief 从控制台输入缓冲读取数据。
 *
 * 缓冲为空时忙等，直到有数据可读；读取到换行符或 Ctrl-D 时结束。
 * 已读取部分数据时，Ctrl-D 会留在缓冲中供下次读取。
 *
 * @param dst 目标缓冲区地址
 * @param n 期望读取的最大字节数
 * @return 实际读取的字节数，最多为 n
 */
int
consoleread(uint64 dst, int n)
{
    uint target;
    int c;
    char cbuf;

    target = n;
    acquire(&cons.lock);
    while (n > 0) {
        /* 等待期间释放锁，允许其他执行流写入缓冲 */
        release(&cons.lock);
        while (cons.r == cons.w) {
            /* 编译器屏障，确保每轮重新读取 cons.r 和 cons.w */
            asm volatile ("" : : : "memory");
        }
        acquire(&cons.lock);

        c = cons.buf[cons.r++ % INPUT_BUF_SIZE];

        if (c == C('D')) {
            /* 已读走部分数据时退回读位置，让 Ctrl-D 留在缓冲中 */
            if (n < target) {
                cons.r--;
            }
            break;
        }

        cbuf = c;
        *((char *)dst) = cbuf;

        dst++;
        n--;

        if (c == '\n') {
            break;
        }
    }
    release(&cons.lock);

    return target - n;
}

int
consolewrite(int user_src, uint64 src, int n)
{
    char buf[32];
    int i = 0;

    while (i < n) {
        int nn = sizeof(buf);

        if (nn > n - i)
            nn = n - i;

        if (either_copyin(buf, user_src, src + i, nn) == -1)
            break;

        for (int j = 0; j < nn; j++)
            consputc(buf[j]);

        i += nn;
    }

    return i;
}
