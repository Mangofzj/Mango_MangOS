#ifndef __SBI_H__
#define __SBI_H__

/* SBI 扩展 ID */
enum sbi_ext_id {
    SBI_EXT_0_1_CONSOLE_PUTCHAR = 0x1,  /* 控制台输出字符（SBI 0.1） */
    SBI_EXT_TIME = 0x54494D45,  /* 定时器扩展，0x54494D45 即 "TIME" */
};

/* 定时器扩展的 set_timer 功能号 */
#define SBI_EXT_TIME_SET_TIMER 0

/* SBI 调用返回结果 */
struct sbiret {
    long error;  /* 错误码 */
    long value;  /* 返回值 */
};

struct sbiret sbi_ecall(
    int ext,
    int fid,
    unsigned long arg0,
    unsigned long arg1,
    unsigned long arg2,
    unsigned long arg3,
    unsigned long arg4,
    unsigned long arg5
);

void sbi_console_putchar(int ch);
void sbi_set_timer(unsigned long stime_value);

#endif
