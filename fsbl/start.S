/*
 * FSBL 启动入口：从 pflash 开始执行，仅 Hart 0 搬运 OpenSBI 固件、设备树和内核镜像到 DDR
 * 搬运完成后 Hart 0 置同步标志并跳转 OpenSBI，其余 Hart 等待标志置位后跳转
 */

    # 同步标志：Hart 0 在 SYNC_ADDR 写入 SYNC_MAGIC，其余 Hart 轮询该地址
    .equ    SYNC_ADDR, 0x00008000
    .equ    SYNC_MAGIC, 0x5A5A5A5A

    # 按字（4 字节）从 _src_start 复制到 [_dst_start, _dst_end)，使用 t0 作为中转
    .macro  load_data, _src_start, _dst_start, _dst_end
    bgeu    \_dst_start, \_dst_end, 2f
1:
    lw  t0, 0(\_src_start)
    sw  t0, 0(\_dst_start)
    addi    \_src_start, \_src_start, 4
    addi    \_dst_start, \_dst_start, 4
    bltu    \_dst_start, \_dst_end, 1b
2:
    .endm

    # 导出入口符号，供链接脚本 ENTRY(_start) 使用
    .global _start

    .text
_start:
    # 读取硬件线程 ID，仅 Hart 0 执行搬运，其余 Hart 等待同步标志置位
    csrr    s0, mhartid
    bnez    s0, secondary_wait

    # 将 pflash 0x20200000 处的 2 MiB OpenSBI 固件搬运到 DDR 0x80000000
    li  a0, 0x20200000
    li  a1, 0x80000000
    li  a2, 0x80200000
    load_data   a0, a1, a2

    # 将 pflash 0x20080000 处的 512 KiB 设备树搬运到 DDR 0x82200000
    li  a0, 0x20080000
    li  a1, 0x82200000
    li  a2, 0x82280000
    load_data   a0, a1, a2

    # 将 pflash 0x20800000 处的 4 MiB xv6 内核镜像区搬运到 DDR 0x80200000
    li  a0, 0x20800000
    li  a1, 0x80200000
    li  a2, 0x80600000
    load_data   a0, a1, a2

    # 置同步标志，通知其余 Hart 继续启动
    li  t1, SYNC_ADDR
    li  t2, SYNC_MAGIC

    # 确保此前的搬运结果先于标志写入可见
    fence   rw, w
    sw  t2, 0(t1)

    # 设置 OpenSBI 入口参数：a0 为 Hart ID，a1 为设备树地址
    mv  a0, s0
    li  a1, 0x82200000

    # 同步指令缓存，确保搬运后的代码可被取指执行
    fence.i

    # 跳转到 DDR 中的 OpenSBI 入口
    li  t0, 0x80000000
    jr  t0

secondary_wait:
    # 轮询同步标志，等待 Hart 0 置位
    li  t1, SYNC_ADDR
    li  t3, SYNC_MAGIC

swait:
    lw  t2, 0(t1)
    bne t2, t3, swait

    # 确保标志读取先于后续访存
    fence   r, rw

    # 设置 OpenSBI 入口参数：a0 为 Hart ID，a1 为设备树地址
    mv  a0, s0
    li  a1, 0x82200000

    # 同步指令缓存，确保搬运后的代码可被取指执行
    fence.i

    # 跳转到 DDR 中的 OpenSBI 入口
    li  t0, 0x80000000
    jr  t0
