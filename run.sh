#!/bin/bash

# 入口设置：以脚本所在目录为项目根目录
PROJECT_ROOT=$(cd "$(dirname "$0")" && pwd)
# 串口与 monitor 图形窗口的默认分辨率
DEFAULT_VC="1080x1000"

# 准备日志目录
mkdir -p "$PROJECT_ROOT/output/log"

# 以 Quard Star 板卡启动 QEMU：1G 内存、8 个 Hart、不使用内置 BIOS，固件取 output/fw/fw.bin
# 三路串口与 monitor 各占一个图形窗口，指令翻译日志写入 output/log/qemu.log
"$PROJECT_ROOT/output/qemu/bin/qemu-system-riscv64" \
    -M quard-star \
    -m 1G \
    -smp 8 \
    -bios none \
    -drive if=pflash,bus=0,unit=0,format=raw,file="$PROJECT_ROOT/output/fw/fw.bin" \
    -d in_asm -D "$PROJECT_ROOT/output/log/qemu.log" \
    --serial "vc:$DEFAULT_VC" \
    --serial "vc:$DEFAULT_VC" \
    --serial "vc:$DEFAULT_VC" \
    --monitor "vc:$DEFAULT_VC" \
    --parallel none
