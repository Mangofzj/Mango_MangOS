/*
 * 启动入口：从 flash 开始执行，仅 hart 0 通过 UART0 输出字符串
 * 输出完成后所有 hart 进入停机循环
 */

#define UART0_BASE 0x10000000

	# 导出入口符号，供链接脚本 ENTRY(_start) 使用
	.global	_start

	.text
_start:
	# 读取硬件线程 ID，仅 hart 0 继续，其余 hart 直接停机
	csrr	t0, mhartid
	bnez	t0, park

	# t0 指向 UART0，t1 指向待输出字符串
	li	t0, UART0_BASE
	la	t1, hello_str

	# 逐字节输出字符串，遇到 NUL 结束
	# sb 写入 UART0 偏移 0 处的发送寄存器
putc:
	lb	t2, 0(t1)
	beqz	t2, park
	sb	t2, 0(t0)
	addi	t1, t1, 1
	j	putc

# 停机循环：等待中断后继续等待，不返回
park:
	wfi
	j	park

	.section	.rodata
# 待输出字符串，由 boot.ld 放入 flash
hello_str:
	.asciz	"Hello, Quard Star!\n"
