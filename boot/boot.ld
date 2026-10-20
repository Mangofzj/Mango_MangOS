/*
 * Boot 测试固件链接脚本：指定 RISC-V 架构、入口符号 _start 和 flash 段布局
 */

OUTPUT_ARCH("riscv")

/* 入口 _start 定义在 start.S */
ENTRY(_start)

/*
 * QEMU 在 0x20000000 处映射 pflash，这里划分 512K 供 Boot 测试固件使用
 * rxai!w 表示只读、可执行、可分配、已初始化，禁止写入
 */
MEMORY
{
    flash (rxai!w) : ORIGIN = 0x20000000, LENGTH = 512K
}

/*
 * .text 和 .rodata 均放入 flash
 * KEEP 防止链接时被 --gc-sections 回收
 */
SECTIONS
{
    .text : {
        KEEP(*(.text))
    } >flash

    .rodata : {
        KEEP(*(.rodata))
    } >flash
}
