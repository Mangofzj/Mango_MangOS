/*
 * xv6 内核链接脚本：指定 RISC-V 架构、入口符号 _entry 和内核段布局
 */

OUTPUT_ARCH("riscv")

/* 入口 _entry 定义在 entry.S */
ENTRY(_entry)

/*
 * 内核起始地址 0x80200000 与 FSBL 的装载地址和 OpenSBI 的固件跳转地址一致
 * entry.o 的 .text 放在最前，保证内核入口位于镜像起始位置
 */
SECTIONS
{
    . = 0x80200000;

    .text : {
        kernel/entry.o(.text)
        *(.text .text.*)
        . = ALIGN(0x1000);
        PROVIDE(etext = .);
    }

    .rodata : {
        . = ALIGN(16);
        *(.srodata .srodata.*)
        . = ALIGN(16);
        *(.rodata .rodata.*)
    }

    .data : {
        . = ALIGN(16);
        *(.sdata .sdata.*)
        . = ALIGN(16);
        *(.data .data.*)
    }

    .bss : {
        . = ALIGN(16);
        *(.sbss .sbss.*)
        . = ALIGN(16);
        *(.bss .bss.*)
        *(COMMON)
    }

    PROVIDE(end = .);
}
