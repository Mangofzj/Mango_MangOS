#ifndef __SBI_H__
#define __SBI_H__

/* SBI 扩展 ID */
enum sbi_ext_id {
    SBI_EXT_0_1_CONSOLE_PUTCHAR = 0x1,  /* 控制台输出字符（SBI 0.1） */
    SBI_EXT_TIME = 0x54494D45,  /* 定时器扩展，0x54494D45 即 "TIME" */
    SBI_EXT_HSM = 0x48534D,  /* Hart 状态管理扩展，0x48534D 即 "HSM" */
};

/* 定时器扩展的 set_timer 功能号 */
#define SBI_EXT_TIME_SET_TIMER 0

/* Hart 状态管理扩展的 hart_start 功能号 */
#define SBI_EXT_HSM_HART_START 0

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
struct sbiret sbi_hart_start(unsigned long hartid, unsigned long start_addr, unsigned long opaque);

#endif
