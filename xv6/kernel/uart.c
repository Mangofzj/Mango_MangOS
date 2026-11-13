#include "types.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"

/* 寄存器偏移转为访问地址 */
#define Reg(reg) ((volatile unsigned char *)(UART0 + (reg)))

#define ReadReg(reg)     (*(Reg(reg)))
#define WriteReg(reg, v) (*(Reg(reg)) = (v))

#define RHR           0  /* 接收保持寄存器 */
#define THR           0  /* 发送保持寄存器 */

#define IER           1  /* 中断使能寄存器 */
#define IER_RX_ENABLE (1 << 0)  /* 接收中断使能 */
#define IER_TX_ENABLE (1 << 1)  /* 发送中断使能 */

#define ISR           2  /* 中断状态寄存器 */

#define FCR             2  /* FIFO 控制寄存器 */
#define FCR_FIFO_ENABLE (1 << 0)  /* FIFO 使能 */
#define FCR_FIFO_CLEAR  (3 << 1)  /* 清空接收与发送 FIFO */

#define LCR             3  /* 线路控制寄存器 */
#define LCR_EIGHT_BITS  (3 << 0)  /* 8 位字长 */
#define LCR_BAUD_LATCH  (1 << 7)  /* 波特率除数锁存使能 */

#define LSR           5  /* 线路状态寄存器 */
#define LSR_RX_READY  (1 << 0)  /* 接收数据就绪 */
#define LSR_TX_IDLE   (1 << 5)  /* 发送保持寄存器空闲 */

static struct spinlock tx_lock;  /* 保护发送状态的自旋锁 */
static int tx_busy;  /* 发送忙标志，由发送中断清除 */

extern volatile int panicking;  /* panic 打印期间跳过中断关闭 */
extern volatile int panicked;  /* panic 消息已输出完成 */

/**
 * @brief 初始化发送锁，并使能 UART 接收与发送中断。
 */
void
uartinit(void)
{
    WriteReg(IER, IER_RX_ENABLE | IER_TX_ENABLE);

    initlock(&tx_lock, "uart");
}

/**
 * @brief 向 UART 发送 n 个字节。
 *
 * 逐字节等待 tx_busy 清零后写入 THR，并置 tx_busy 等待发送中断清除。
 *
 * @param buf 待发送数据缓冲区
 * @param n 发送字节数
 */
void
uartwrite(char buf[], int n)
{
    acquire(&tx_lock);

    int i = 0;
    while (i < n) {
        release(&tx_lock);

        while (tx_busy != 0) {
            asm volatile("" : : : "memory");
        }

        acquire(&tx_lock);

        if (buf[i] == '\n') {
            while ((ReadReg(LSR) & LSR_TX_IDLE) == 0)
                ;
            WriteReg(THR, '\r');

            while ((ReadReg(LSR) & LSR_TX_IDLE) == 0)
                ;
        }

        WriteReg(THR, buf[i]);
        i++;
        tx_busy = 1;
    }

    release(&tx_lock);
}

/**
 * @brief 同步输出一个字符。
 *
 * 等待发送保持寄存器空闲后写入；非 panic 路径在输出前后关闭和恢复中断，
 * panicked 置位后进入死循环。
 *
 * @param c 待输出的字符
 */
void
uartputc_sync(int c)
{
    if (panicking == 0)
        push_off();
    if (panicked) {
        for (;;)
            ;
    }

    if (c == '\n') {
        while ((ReadReg(LSR) & LSR_TX_IDLE) == 0)
            ;
        WriteReg(THR, '\r');
    }

    while ((ReadReg(LSR) & LSR_TX_IDLE) == 0)
        ;
    WriteReg(THR, c);

    if (panicking == 0)
        pop_off();
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
 * 发送空闲时清除 tx_busy；随后循环读取接收缓冲直到无数据可读。
 */
void
uartintr(void)
{
    ReadReg(ISR);

    acquire(&tx_lock);

    if (ReadReg(LSR) & LSR_TX_IDLE) {
        tx_busy = 0;
    }

    release(&tx_lock);

    while (1) {
        int c = uartgetc();
        if (c == -1)
            break;

        consoleintr(c);
    }
}
