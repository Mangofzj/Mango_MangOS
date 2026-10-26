/*
 * xv6 内核入口：由 OpenSBI 以 S 模式跳转至此，设置栈后进入 main
 */

    .section .text
    # 导出入口符号，供链接脚本 ENTRY(_entry) 使用
    .globl  _entry

_entry:
    # 保存 OpenSBI 传入的 hart ID，用于计算栈顶
    mv  t0, a0

    # 栈顶 = stack0 + (hart ID + 1) * 4096
    la  sp, stack0
    li  t1, 1024*4
    addi    t0, t0, 1
    mul t0, t0, t1
    add sp, sp, t0

    # 调用 C 语言入口 main
    call    main

    # 停机循环：main 返回后停在此处
spin:
    j   spin

    # 栈空间位于 .bss.stack 段，按 16 字节对齐以满足 RISC-V ABI 的栈对齐要求
    .section .bss.stack
    .align  4
    .globl  stack0

stack0:
    .space  4096
