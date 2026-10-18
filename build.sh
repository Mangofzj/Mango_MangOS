#!/bin/bash

# 入口设置：命令返回非零时立即退出，并以脚本所在目录为项目根目录
set -e
PROJECT_ROOT=$(cd "$(dirname "$0")" && pwd)

# 准备构建目录与输出目录
BUILD_DIR="$PROJECT_ROOT/build"
OUTPUT_DIR="$PROJECT_ROOT/output"
mkdir -p "$BUILD_DIR" "$OUTPUT_DIR"

# 在源码树外的独立目录中构建 QEMU
mkdir -p "$BUILD_DIR/qemu"
cd "$BUILD_DIR/qemu"
# Makefile 不存在时执行 configure，只构建 riscv64-softmmu 目标并安装到 output/qemu
if [ ! -f Makefile ]; then
    "$PROJECT_ROOT/qemu-8.0.0/configure" \
        --prefix="$OUTPUT_DIR/qemu" \
        --target-list=riscv64-softmmu \
        --enable-gtk \
        --enable-virtfs \
        --disable-gio
fi
# 并行编译 QEMU 并安装到 output/qemu
make -j"$(nproc)"
make install
