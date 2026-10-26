/*
 * FSBL 启动入口：从 pflash 开始执行，仅 hart 0 搬运 OpenSBI 固件和设备树到 DDR
 * 搬运完成后跳转 OpenSBI，其余 hart 进入停机循环
 */

	# 按字（4 字节）从 _src_start 复制到 [_dst_start, _dst_end)，使用 t0 作为中转
	.macro	load_data, _src_start, _dst_start, _dst_end
	bgeu	\_dst_start, \_dst_end, 2f
1:
	lw	t0, 0(\_src_start)
	sw	t0, 0(\_dst_start)
	addi	\_src_start, \_src_start, 4
	addi	\_dst_start, \_dst_start, 4
	bltu	\_dst_start, \_dst_end, 1b
2:
	.endm

	# 导出入口符号，供链接脚本 ENTRY(_start) 使用
	.global	_start

	.text
_start:
	# 读取硬件线程 ID，仅 hart 0 继续，其余 hart 直接停机
	csrr	s0, mhartid
	bnez	s0, park

	# 将 pflash 0x20200000 处的 2 MiB OpenSBI 固件搬到 DDR 0x80000000
	li	a0, 0x20200000
	li	a1, 0x80000000
	li	a2, 0x80200000
	load_data	a0, a1, a2

	# 将 pflash 0x20080000 处的 512 KiB 设备树搬到 DDR 0x82200000
	li	a0, 0x20080000
	li	a1, 0x82200000
	li	a2, 0x82280000
	load_data	a0, a1, a2

	# 将 pflash 0x20800000 处的 4 MiB xv6 内核镜像区搬到 DDR 0x80200000
	li	a0, 0x20800000
	li	a1, 0x80200000
	li	a2, 0x80600000
	load_data	a0, a1, a2

	# 设置 OpenSBI 入口参数：a0 为 hart ID，a1 为设备树地址
	mv	a0, s0
	li	a1, 0x82200000

	# 同步指令缓存，确保搬运后的固件可被取指执行
	fence.i

	# 跳转到 DDR 中的 OpenSBI 入口
	li	t0, 0x80000000
	jr	t0

# 停机循环：等待中断后继续等待，不返回
park:
	wfi
	j	park
