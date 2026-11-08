#include <stdarg.h>

#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"

volatile int panicking = 0;  /* panic 打印期间跳过 printf 加锁 */
volatile int panicked = 0;  /* panic 消息已输出完成 */

static char digits[] = "0123456789abcdef";  /* 数字字符表，供十进制和十六进制转换使用 */

/* printf 输出锁 */
static struct {
    struct spinlock lock;
} pr;

/**
 * @brief 初始化 printf 输出锁。
 */
void
printfinit(void)
{
    initlock(&pr.lock, "pr");
}

/**
 * @brief 按指定进制输出有符号或无符号整数。
 *
 * 逆序生成各位数字后，逐字符经 SBI 控制台输出。
 *
 * @param xx 待输出的值
 * @param base 进制（10 或 16）
 * @param sign 非 0 时按有符号数处理，为负时在结果前加 '-'
 */
static void
printint(long long xx, int base, int sign)
{
    char buf[20];  /* 足以容纳 64 位十进制及符号 */
    int i;
    unsigned long long x;

    /* sign 非 0 时取绝对值，并记录是否为负数 */
    if (sign && (sign = (xx < 0)))
        x = -xx;
    else
        x = xx;

    i = 0;
    do {
        buf[i++] = digits[x % base];
    } while ((x /= base) != 0);

    if (sign)
        buf[i++] = '-';

    while (--i >= 0)
        consputc(buf[i]);
}

/**
 * @brief 以 0x 前缀和 16 个十六进制数字输出 64 位值。
 *
 * @param x 待输出的值
 */
static void
printptr(uint64 x)
{
    int i;

    consputc('0');
    consputc('x');
    for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)
        consputc(digits[x >> (sizeof(uint64) * 8 - 4)]);
}

/**
 * @brief 格式化输出到 SBI 控制台。
 *
 * 支持 %d、%u、%x、%c、%s、%p、%% 以及 l/ll 长度修饰；
 * 未知转换输出 '%' 与该字符，格式串以 '%' 结尾时直接结束。
 *
 * @param fmt 格式字符串
 * @param ... 与格式说明对应的可变参数
 * @note panicking 非 0 时跳过 pr.lock，panic 路径不再等待输出锁。
 * @return 固定返回 0
 */
int
printf(char *fmt, ...)
{
    va_list ap;
    int i, cx, c0, c1, c2;
    char *s;

    if (panicking == 0)
        acquire(&pr.lock);

    va_start(ap, fmt);
    for (i = 0; (cx = fmt[i] & 0xff) != 0; i++) {
        if (cx != '%') {
            consputc(cx);
            continue;
        }
        i++;
        /* 预读转换字符及 l/ll 长度修饰 */
        c0 = fmt[i+0] & 0xff;
        c1 = c2 = 0;
        if (c0) c1 = fmt[i+1] & 0xff;
        if (c1) c2 = fmt[i+2] & 0xff;
        if (c0 == 'd') {
            printint(va_arg(ap, int), 10, 1);
        } else if (c0 == 'l' && c1 == 'd') {
            printint(va_arg(ap, uint64), 10, 1);
            i += 1;
        } else if (c0 == 'l' && c1 == 'l' && c2 == 'd') {
            printint(va_arg(ap, uint64), 10, 1);
            i += 2;
        } else if (c0 == 'u') {
            printint(va_arg(ap, uint32), 10, 0);
        } else if (c0 == 'l' && c1 == 'u') {
            printint(va_arg(ap, uint64), 10, 0);
            i += 1;
        } else if (c0 == 'l' && c1 == 'l' && c2 == 'u') {
            printint(va_arg(ap, uint64), 10, 0);
            i += 2;
        } else if (c0 == 'x') {
            printint(va_arg(ap, uint32), 16, 0);
        } else if (c0 == 'l' && c1 == 'x') {
            printint(va_arg(ap, uint64), 16, 0);
            i += 1;
        } else if (c0 == 'l' && c1 == 'l' && c2 == 'x') {
            printint(va_arg(ap, uint64), 16, 0);
            i += 2;
        } else if (c0 == 'p') {
            printptr(va_arg(ap, uint64));
        } else if (c0 == 'c') {
            consputc(va_arg(ap, uint));
        } else if (c0 == 's') {
            if ((s = va_arg(ap, char *)) == 0)
                s = "(null)";
            for (; *s; s++)
                consputc(*s);
        } else if (c0 == '%') {
            consputc('%');
        } else if (c0 == 0) {
            break;
        } else {
            consputc('%');
            consputc(c0);
        }
    }
    va_end(ap);

    if (panicking == 0)
        release(&pr.lock);

    return 0;
}

/**
 * @brief 打印 panic 消息并停机。
 *
 * 输出消息期间置 panicking，输出完成后置 panicked，
 * 随后进入死循环，不返回。
 *
 * @param s panic 描述
 */
void
panic(char *s)
{
    panicking = 1;
    printf("panic: ");
    printf("%s\n", s);
    panicked = 1;
    for (;;)
        ;
}
