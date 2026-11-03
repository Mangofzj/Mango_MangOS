#include <stdint.h>

#include "sbi.h"

/**
 * @brief 发起一次 SBI 调用。
 *
 * ext 与 fid 选择扩展功能，arg0~arg5 依次传入 a0~a5；
 * 调用返回后 a0 为错误码，a1 为返回值。
 *
 * @param ext 扩展 ID
 * @param fid 功能号
 * @param arg0 扩展功能参数 0
 * @param arg1 扩展功能参数 1
 * @param arg2 扩展功能参数 2
 * @param arg3 扩展功能参数 3
 * @param arg4 扩展功能参数 4
 * @param arg5 扩展功能参数 5
 * @return SBI 调用的错误码与返回值
 */
struct sbiret
sbi_ecall(int ext, int fid, unsigned long arg0,
          unsigned long arg1, unsigned long arg2,
          unsigned long arg3, unsigned long arg4,
          unsigned long arg5)
{
    struct sbiret ret;

    register uintptr_t a0 asm ("a0") = (uintptr_t)(arg0);
    register uintptr_t a1 asm ("a1") = (uintptr_t)(arg1);
    register uintptr_t a2 asm ("a2") = (uintptr_t)(arg2);
    register uintptr_t a3 asm ("a3") = (uintptr_t)(arg3);
    register uintptr_t a4 asm ("a4") = (uintptr_t)(arg4);
    register uintptr_t a5 asm ("a5") = (uintptr_t)(arg5);
    register uintptr_t a6 asm ("a6") = (uintptr_t)(fid);
    register uintptr_t a7 asm ("a7") = (uintptr_t)(ext);

    asm volatile ("ecall"
                  : "+r" (a0), "+r" (a1)
                  : "r" (a2), "r" (a3), "r" (a4),
                    "r" (a5), "r" (a6), "r" (a7)
                  : "memory");

    ret.error = a0;
    ret.value = a1;

    return ret;
}

/**
 * @brief 向 SBI 控制台输出一个字符。
 *
 * @param ch 待输出的字符
 */
void
sbi_console_putchar(int ch)
{
    sbi_ecall(SBI_EXT_0_1_CONSOLE_PUTCHAR, 0, ch, 0, 0, 0, 0, 0);
}

/**
 * @brief 设置下一次定时器中断的时间。
 *
 * @param stime_value 定时器触发时刻的 stime 值
 */
void
sbi_set_timer(unsigned long stime_value)
{
    sbi_ecall(SBI_EXT_TIME, SBI_EXT_TIME_SET_TIMER, stime_value, 0, 0, 0, 0, 0);
}

struct sbiret
sbi_hart_start(unsigned long hartid, unsigned long start_addr, unsigned long opaque)
{
    return sbi_ecall(
        SBI_EXT_HSM,
        SBI_EXT_HSM_HART_START,
        hartid,
        start_addr,
        opaque,
        0, 0, 0
    );
}
