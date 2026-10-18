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

#endif /* HW_RISCV_QUARD_STAR__H */
