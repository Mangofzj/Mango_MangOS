#!/bin/bash

# 入口设置：命令返回非零时立即退出，并以脚本所在目录为项目根目录
set -e
PROJECT_ROOT=$(cd "$(dirname "$0")" && pwd)
CROSS_PREFIX=riscv64-unknown-elf-

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

# 准备 Boot 测试固件构建目录
mkdir -p "$BUILD_DIR/boot"
# 预处理并编译启动汇编，生成目标文件
"${CROSS_PREFIX}gcc" \
    -x assembler-with-cpp \
    -fno-pie \
    -c "$PROJECT_ROOT/boot/start.S" \
    -o "$BUILD_DIR/boot/start.o"
# 按链接脚本链接目标文件，生成 Boot 测试固件 ELF 和映射文件
"${CROSS_PREFIX}gcc" \
    -nostdlib \
    -no-pie \
    -T "$PROJECT_ROOT/boot/boot.ld" \
    -Wl,-Map="$BUILD_DIR/boot/boot.map" \
    -Wl,--gc-sections \
    "$BUILD_DIR/boot/start.o" \
    -o "$BUILD_DIR/boot/boot.elf"
# 导出含重定位信息的反汇编清单
"${CROSS_PREFIX}objdump" \
    --source \
    --demangle \
    --disassemble \
    --reloc \
    --wide \
    "$BUILD_DIR/boot/boot.elf" \
    > "$BUILD_DIR/boot/boot.lst"
# 去掉符号并提取纯二进制 Boot 测试镜像
"${CROSS_PREFIX}objcopy" \
    -O binary \
    -S \
    "$BUILD_DIR/boot/boot.elf" \
    "$BUILD_DIR/boot/boot.bin"

# 准备 FSBL 固件构建目录
mkdir -p "$BUILD_DIR/fsbl"
# 预处理并编译启动汇编，生成目标文件
"${CROSS_PREFIX}gcc" \
    -x assembler-with-cpp \
    -fno-pie \
    -c "$PROJECT_ROOT/fsbl/start.S" \
    -o "$BUILD_DIR/fsbl/start.o"
# 按链接脚本链接目标文件，生成 FSBL 固件 ELF 和映射文件
"${CROSS_PREFIX}gcc" \
    -nostdlib \
    -no-pie \
    -T "$PROJECT_ROOT/fsbl/fsbl.ld" \
    -Wl,-Map="$BUILD_DIR/fsbl/fsbl.map" \
    -Wl,--gc-sections \
    "$BUILD_DIR/fsbl/start.o" \
    -o "$BUILD_DIR/fsbl/fsbl.elf"
# 导出含重定位信息的反汇编清单
"${CROSS_PREFIX}objdump" \
    --source \
    --demangle \
    --disassemble \
    --reloc \
    --wide \
    "$BUILD_DIR/fsbl/fsbl.elf" \
    > "$BUILD_DIR/fsbl/fsbl.lst"
# 去掉符号并提取纯二进制 FSBL 固件
"${CROSS_PREFIX}objcopy" \
    -O binary \
    -S \
    "$BUILD_DIR/fsbl/fsbl.elf" \
    "$BUILD_DIR/fsbl/fsbl.bin"

# 准备设备树构建目录与输出目录
mkdir -p "$BUILD_DIR/dts" "$OUTPUT_DIR/dts"
# 预处理设备树源文件，展开宏与包含的文件
cpp \
    -nostdinc \
    -I "$PROJECT_ROOT/dts" \
    -undef \
    -x assembler-with-cpp \
    "$PROJECT_ROOT/dts/quard_star_sbi.dts" \
    > "$BUILD_DIR/dts/quard_star_sbi.dts"
# 编译预处理后的设备树，生成二进制设备树文件（DTB）
dtc \
    -I dts \
    -O dtb \
    -o "$OUTPUT_DIR/dts/quard_star_sbi.dtb" \
    "$BUILD_DIR/dts/quard_star_sbi.dts"

# 准备固件镜像输出目录
mkdir -p "$OUTPUT_DIR/fw"
# 生成 32 MiB 全零固件镜像
dd if=/dev/zero \
    of="$OUTPUT_DIR/fw/fw.bin" \
    bs=1K count=32768 \
    status=none
# 将 FSBL 镜像写入固件镜像起始位置
dd if="$BUILD_DIR/fsbl/fsbl.bin" \
    of="$OUTPUT_DIR/fw/fw.bin" \
    bs=1K seek=0 \
    conv=notrunc \
    status=none
# 将 DTB 写入固件镜像的 512 KiB 偏移处，位于 FSBL 固件区之后
dd if="$OUTPUT_DIR/dts/quard_star_sbi.dtb" \
    of="$OUTPUT_DIR/fw/fw.bin" \
    bs=1K seek=512 \
    conv=notrunc \
    status=none
