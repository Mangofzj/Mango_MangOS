#!/bin/bash

# 入口设置：以脚本所在目录为项目根目录
PROJECT_ROOT=$(cd "$(dirname "$0")" && pwd)

# 以 Quard Star 板卡启动 QEMU：1G 内存、8 个 Hart
"$PROJECT_ROOT/output/qemu/bin/qemu-system-riscv64" \
    -M quard-star \
    -m 1G \
    -smp 8 \
    -nographic \
    --parallel none
