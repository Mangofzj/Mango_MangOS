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

/**
 * @brief 使能 UART 接收中断。
 */
void
uartinit(void)
{
    WriteReg(IER, IER_RX_ENABLE);
}

/**
 * @brief 读取一个接收字节。
 *
 * @return 接收到的字节；接收缓冲为空时返回 -1。
 */
int
uartgetc(void)
{
    if (ReadReg(LSR) & LSR_RX_READY) {
        return ReadReg(RHR);
    } else {
        return -1;
    }
}

/**
 * @brief 处理串口中断，逐字节读取接收内容并交给 consoleintr 处理。
 *
 * 先读取中断状态寄存器，再循环读取接收缓冲直到无数据可读。
 */
void
uartintr(void)
{
    ReadReg(ISR);

    while (1) {
        int c = uartgetc();
        if (c == -1)
            break;

        consoleintr(c);
    }
}
