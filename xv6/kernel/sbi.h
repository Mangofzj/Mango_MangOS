
#ifndef __SBI_H__
#define __SBI_H__

enum sbi_ext_id {
    SBI_EXT_0_1_CONSOLE_PUTCHAR = 0x1,
};

struct sbiret {
    long error;
    long value;
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

#endif
