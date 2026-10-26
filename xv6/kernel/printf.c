#include <stdarg.h>

#include "types.h"
#include "sbi.h"
#include "defs.h"

static char digits[] = "0123456789abcdef";

static void
printint(long long xx, int base, int sign)
{
    char buf[20];
    int i;
    unsigned long long x;

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
        sbi_console_putchar(buf[i]);
}

static void
printptr(uint64 x)
{
    int i;

    sbi_console_putchar('0');
    sbi_console_putchar('x');
    for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)
        sbi_console_putchar(digits[x >> (sizeof(uint64) * 8 - 4)]);
}

int
printf(char *fmt, ...)
{
    va_list ap;
    int i, cx, c0, c1, c2;
    char *s;

    va_start(ap, fmt);
    for (i = 0; (cx = fmt[i] & 0xff) != 0; i++) {
        if (cx != '%') {
            sbi_console_putchar(cx);
            continue;
        }
        i++;
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
            sbi_console_putchar(va_arg(ap, uint));
        } else if (c0 == 's') {
            if ((s = va_arg(ap, char*)) == 0)
                s = "(null)";
            for (; *s; s++)
                sbi_console_putchar(*s);
        } else if (c0 == '%') {
            sbi_console_putchar('%');
        } else if (c0 == 0) {
            break;
        } else {
            sbi_console_putchar('%');
            sbi_console_putchar(c0);
        }
    }
    va_end(ap);

    return 0;
}

void
panic(char *s)
{
    printf("panic: ");
    printf("%s\n", s);
    for (;;)
        ;
}

void
printfinit(void)
{
}
