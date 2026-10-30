#include "types.h"
#include "memlayout.h"
#include "defs.h"
#include "sbi.h"

/* 寄存器偏移转为访问地址 */
#define Reg(reg) ((volatile unsigned char *)(UART0 + (reg)))

#define ReadReg(reg)     (*(Reg(reg)))
#define WriteReg(reg, v) (*(Reg(reg)) = (v))

#define RHR           0  /* 接收保持寄存器 */
#define IER           1  /* 中断使能寄存器 */
#define IER_RX_ENABLE (1 << 0)  /* 接收中断使能 */
#define ISR           2  /* 中断状态寄存器 */
#define LSR           5  /* 线路状态寄存器 */
#define LSR_RX_READY  (1 << 0)  /* 接收数据就绪 */

/* UART 初始化：使能接收中断 */
void
uartinit(void)
{
    WriteReg(IER, IER_RX_ENABLE);
}

int
uartgetc(void)
{
    if (ReadReg(LSR) & LSR_RX_READY) {
        return ReadReg(RHR);
    } else {
        return -1;
    }
}

void
uartintr(void)
{
    ReadReg(ISR);

    while (1) {
        int c = uartgetc();
        if (c == -1)
            break;

        sbi_console_putchar(c);
        sbi_console_putchar('\n');
    }
}
