/*
 * QEMU RISC-V Quard Star 板卡机器模型的类型与常量定义
 *
 * Copyright (c) 2026 Mango <13084743591@163.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef HW_RISCV_QUARD_STAR__H
#define HW_RISCV_QUARD_STAR__H

#include "hw/riscv/riscv_hart.h"
#include "hw/sysbus.h"
#include "qom/object.h"
#include "hw/block/flash.h"

#define TYPE_RISCV_QUARD_STAR_MACHINE MACHINE_TYPE_NAME("quard-star")
typedef struct QuardStarState QuardStarState;
DECLARE_INSTANCE_CHECKER(QuardStarState, QUARD_STAR_MACHINE,
                         TYPE_RISCV_QUARD_STAR_MACHINE)

#define QUARD_STAR_CPUS_MAX    8
#define QUARD_STAR_SOCKETS_MAX 8

/* 板卡私有状态 */
struct QuardStarState {
    MachineState parent;

    RISCVHartArrayState soc[QUARD_STAR_SOCKETS_MAX];

    PFlashCFI01 *flash;
    DeviceState *plic[QUARD_STAR_SOCKETS_MAX];
};

/* 内存映射索引 */
enum {
    QUARD_STAR_MROM,
    QUARD_STAR_SRAM,
    QUARD_STAR_ACLINT,
    QUARD_STAR_PLIC,
    QUARD_STAR_UART0,
    QUARD_STAR_UART1,
    QUARD_STAR_UART2,
    QUARD_STAR_RTC,
    QUARD_STAR_VIRTIO0,
    QUARD_STAR_FLASH,
    QUARD_STAR_DRAM,
};

/* 外设中断号 */
enum {
    QUARD_STAR_VIRTIO0_IRQ = 1,
    QUARD_STAR_UART0_IRQ = 10,
    QUARD_STAR_UART1_IRQ = 11,
    QUARD_STAR_UART2_IRQ = 12,
    QUARD_STAR_RTC_IRQ   = 13,
};

#define QUARD_STAR_PLIC_NUM_SOURCES    127
#define QUARD_STAR_PLIC_NUM_PRIORITIES 7
#define QUARD_STAR_PLIC_PRIORITY_BASE  0x04
#define QUARD_STAR_PLIC_PENDING_BASE   0x1000
#define QUARD_STAR_PLIC_ENABLE_BASE    0x2000
#define QUARD_STAR_PLIC_ENABLE_STRIDE  0x80
#define QUARD_STAR_PLIC_CONTEXT_BASE   0x200000
#define QUARD_STAR_PLIC_CONTEXT_STRIDE 0x1000

#endif /* HW_RISCV_QUARD_STAR__H */
