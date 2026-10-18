/*
 * QEMU RISC-V Quard Star 板卡机器模型实现
 *
 * Copyright (c) 2026 Mango <13084743591@163.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "qemu/units.h"
#include "qemu/error-report.h"
#include "qapi/error.h"
#include "hw/boards.h"
#include "hw/loader.h"
#include "hw/qdev-properties.h"
#include "hw/char/serial.h"
#include "target/riscv/cpu.h"
#include "hw/riscv/riscv_hart.h"
#include "hw/riscv/quard_star.h"
#include "hw/riscv/boot.h"
#include "hw/riscv/numa.h"
#include "hw/intc/riscv_aclint.h"
#include "hw/intc/sifive_plic.h"
#include "sysemu/sysemu.h"

#define QUARD_STAR_FLASH_SECTOR_SIZE (256 * KiB)

/* 内存地址映射 */
static const MemMapEntry quard_star_memmap[] = {
    [QUARD_STAR_MROM]    = { 0x00000000, 0x00008000 },  /* 32KB Mask ROM */
    [QUARD_STAR_SRAM]    = { 0x00008000, 0x00008000 },  /* 32KB SRAM */
    [QUARD_STAR_ACLINT]  = { 0x02000000, 0x00010000 },  /* ACLINT */
    [QUARD_STAR_PLIC]    = { 0x0c000000, 0x00210000 },  /* PLIC */
    [QUARD_STAR_UART0]   = { 0x10000000, 0x00001000 },  /* UART 0 */
    [QUARD_STAR_UART1]   = { 0x10001000, 0x00001000 },  /* UART 1 */
    [QUARD_STAR_UART2]   = { 0x10002000, 0x00001000 },  /* UART 2 */
    [QUARD_STAR_RTC]     = { 0x10003000, 0x00001000 },  /* RTC */
    [QUARD_STAR_VIRTIO0] = { 0x10100000, 0x00001000 },  /* virtio-mmio 0 */
    [QUARD_STAR_FLASH]   = { 0x20000000, 0x02000000 },  /* 32MB pflash */
    [QUARD_STAR_DRAM]    = { 0x80000000, 0x40000000 },  /* DDR，容量由 -m 决定 */
};

/* 创建 RISC-V CPU */
static void quard_star_cpu_create(MachineState *machine)
{
    QuardStarState *s = QUARD_STAR_MACHINE(machine);
    int i, base_hartid, hart_count;
    char *soc_name;

    /* 检查 Socket 数量是否超出支持范围 */
    if (riscv_socket_count(machine) > QUARD_STAR_SOCKETS_MAX) {
        error_report("number of sockets exceeds maximum %d",
                     QUARD_STAR_SOCKETS_MAX);
        exit(1);
    }

    /* 按 Socket 创建 RISC-V HartArray */
    for (i = 0; i < riscv_socket_count(machine); i++) {

        /* 检查 Hart ID 是否连续 */
        if (!riscv_socket_check_hartids(machine, i)) {
            error_report("discontinuous hartids in socket%d", i);
            exit(1);
        }

        base_hartid = riscv_socket_first_hartid(machine, i);
        if (base_hartid < 0) {
            error_report("can't find hartid base for socket%d", i);
            exit(1);
        }

        hart_count = riscv_socket_hart_count(machine, i);
        if (hart_count < 0) {
            error_report("can't find hart count for socket%d", i);
            exit(1);
        }

        soc_name = g_strdup_printf("soc%d", i);
        object_initialize_child(OBJECT(machine), soc_name,
                                &s->soc[i], TYPE_RISCV_HART_ARRAY);
        g_free(soc_name);

        object_property_set_str(OBJECT(&s->soc[i]), "cpu-type",
                                machine->cpu_type, &error_abort);

        object_property_set_int(OBJECT(&s->soc[i]), "hartid-base",
                                base_hartid, &error_abort);

        object_property_set_int(OBJECT(&s->soc[i]), "num-harts",
                                hart_count, &error_abort);

        /* 复位向量指向 MROM 起始地址，与 reset blob 的写入位置保持一致 */
        object_property_set_int(OBJECT(&s->soc[i]), "resetvec",
                                quard_star_memmap[QUARD_STAR_MROM].base,
                                &error_abort);

        sysbus_realize(SYS_BUS_DEVICE(&s->soc[i]), &error_abort);
    }
}

/* 创建并映射 pflash 设备 */
static void quard_star_flash_create(MachineState *machine)
{
    QuardStarState *s = QUARD_STAR_MACHINE(machine);
    MemoryRegion *system_memory = get_system_memory();
    DeviceState *dev = qdev_new(TYPE_PFLASH_CFI01);
    hwaddr flashbase = quard_star_memmap[QUARD_STAR_FLASH].base;
    hwaddr flashsize = quard_star_memmap[QUARD_STAR_FLASH].size;

    /* 设置 pflash 芯片属性 */
    qdev_prop_set_uint64(dev, "sector-length",
                         QUARD_STAR_FLASH_SECTOR_SIZE);
    qdev_prop_set_uint8(dev, "width", 4);
    qdev_prop_set_uint8(dev, "device-width", 2);
    qdev_prop_set_bit(dev, "big-endian", false);

    /* 设置 pflash 芯片 ID 与设备名 */
    qdev_prop_set_uint16(dev, "id0", 0x89);
    qdev_prop_set_uint16(dev, "id1", 0x18);
    qdev_prop_set_uint16(dev, "id2", 0x00);
    qdev_prop_set_uint16(dev, "id3", 0x00);
    qdev_prop_set_string(dev, "name", "quard-star.flash0");

    /* 将 pflash 注册为 Machine 的子设备 */
    object_property_add_child(OBJECT(s),
                              "quard-star.flash0", OBJECT(dev));

    /* 为 pflash 的 drive 属性创建别名 */
    object_property_add_alias(OBJECT(s), "pflash0",
                              OBJECT(dev), "drive");

    /* 保存 pflash 设备指针并绑定固件镜像后端 */
    s->flash = PFLASH_CFI01(dev);
    pflash_cfi01_legacy_drive(s->flash,
                              drive_get(IF_PFLASH, 0, 0));

    /* 校验 pflash 容量并设置扇区数量 */
    assert(QEMU_IS_ALIGNED(flashsize,
                           QUARD_STAR_FLASH_SECTOR_SIZE));
    assert(flashsize / QUARD_STAR_FLASH_SECTOR_SIZE <= UINT32_MAX);

    qdev_prop_set_uint32(dev, "num-blocks",
                         flashsize / QUARD_STAR_FLASH_SECTOR_SIZE);

    /* 完成 pflash 设备初始化 */
    sysbus_realize_and_unref(SYS_BUS_DEVICE(dev), &error_fatal);

    /* 将 pflash 映射到系统物理地址空间 */
    memory_region_add_subregion(
        system_memory,
        flashbase,
        sysbus_mmio_get_region(SYS_BUS_DEVICE(dev), 0));
}

/* 创建 MROM、SRAM、DRAM 并设置复位向量 */
static void quard_star_memory_create(MachineState *machine)
{
    QuardStarState *s = QUARD_STAR_MACHINE(machine);
    MemoryRegion *system_memory = get_system_memory();

    MemoryRegion *dram_mem = g_new(MemoryRegion, 1);
    MemoryRegion *sram_mem = g_new(MemoryRegion, 1);
    MemoryRegion *mask_rom = g_new(MemoryRegion, 1);

    /* 创建 DRAM */
    memory_region_init_ram(
        dram_mem,
        NULL,
        "riscv_quard_star_board.dram",
        machine->ram_size,
        &error_fatal);

    memory_region_add_subregion(
        system_memory,
        quard_star_memmap[QUARD_STAR_DRAM].base,
        dram_mem);

    /* 创建 SRAM */
    memory_region_init_ram(
        sram_mem,
        NULL,
        "riscv_quard_star_board.sram",
        quard_star_memmap[QUARD_STAR_SRAM].size,
        &error_fatal);

    memory_region_add_subregion(
        system_memory,
        quard_star_memmap[QUARD_STAR_SRAM].base,
        sram_mem);

    /* 创建 MROM */
    memory_region_init_rom(
        mask_rom,
        NULL,
        "riscv_quard_star_board.mrom",
        quard_star_memmap[QUARD_STAR_MROM].size,
        &error_fatal);

    memory_region_add_subregion(
        system_memory,
        quard_star_memmap[QUARD_STAR_MROM].base,
        mask_rom);

    /* 将复位向量写入 MROM，目标为 pflash */
    riscv_setup_rom_reset_vec(
        machine,
        &s->soc[0],
        quard_star_memmap[QUARD_STAR_FLASH].base,
        quard_star_memmap[QUARD_STAR_MROM].base,
        quard_star_memmap[QUARD_STAR_MROM].size,
        0x0,
        0x0);
}

/* 创建 PLIC 外部中断控制器 */
static void quard_star_plic_create(MachineState *machine)
{
    QuardStarState *s = QUARD_STAR_MACHINE(machine);
    int socket_count = riscv_socket_count(machine);
    int i, hart_count, base_hartid;
    char *plic_hart_config;

    /* 按 Socket 创建 PLIC */
    for (i = 0; i < socket_count; i++) {

        /* 获取当前 Socket 的 Hart 数量和起始 Hart ID */
        hart_count = riscv_socket_hart_count(machine, i);
        base_hartid = riscv_socket_first_hartid(machine, i);

        /* 生成 Hart 对应的 M/S 模式中断上下文配置 */
        plic_hart_config =
            riscv_plic_hart_config_string(hart_count);

        s->plic[i] = sifive_plic_create(
            quard_star_memmap[QUARD_STAR_PLIC].base +
                i * quard_star_memmap[QUARD_STAR_PLIC].size,
            plic_hart_config,
            hart_count,
            base_hartid,
            QUARD_STAR_PLIC_NUM_SOURCES,
            QUARD_STAR_PLIC_NUM_PRIORITIES,
            QUARD_STAR_PLIC_PRIORITY_BASE,
            QUARD_STAR_PLIC_PENDING_BASE,
            QUARD_STAR_PLIC_ENABLE_BASE,
            QUARD_STAR_PLIC_ENABLE_STRIDE,
            QUARD_STAR_PLIC_CONTEXT_BASE,
            QUARD_STAR_PLIC_CONTEXT_STRIDE,
            quard_star_memmap[QUARD_STAR_PLIC].size);

        g_free(plic_hart_config);
    }
}

/* 创建 ACLINT 软件中断控制器和机器模式定时器 */
static void quard_star_aclint_create(MachineState *machine)
{
    int socket_count = riscv_socket_count(machine);
    int i, base_hartid, hart_count;
    hwaddr aclint_base;

    /* 按 Socket 创建 ACLINT：机器模式软件中断控制器和定时器 */
    for (i = 0; i < socket_count; i++) {

        /* 获取当前 Socket 对应的 Hart ID 范围 */
        base_hartid = riscv_socket_first_hartid(machine, i);
        hart_count = riscv_socket_hart_count(machine, i);

        /* 计算当前 Socket 的 ACLINT 地址 */
        aclint_base = quard_star_memmap[QUARD_STAR_ACLINT].base +
                      i * quard_star_memmap[QUARD_STAR_ACLINT].size;

        riscv_aclint_swi_create(
            aclint_base,
            base_hartid,
            hart_count,
            false);

        riscv_aclint_mtimer_create(
            aclint_base + RISCV_ACLINT_SWI_SIZE,
            RISCV_ACLINT_DEFAULT_MTIMER_SIZE,
            base_hartid,
            hart_count,
            RISCV_ACLINT_DEFAULT_MTIMECMP,
            RISCV_ACLINT_DEFAULT_MTIME,
            RISCV_ACLINT_DEFAULT_TIMEBASE_FREQ,
            true);
    }
}

/* 创建三路 UART 串口设备 */
static void quard_star_serial_create(MachineState *machine)
{
    QuardStarState *s = QUARD_STAR_MACHINE(machine);
    MemoryRegion *system_memory = get_system_memory();

    /* 创建 UART0，中断接入 PLIC */
    serial_mm_init(
        system_memory,
        quard_star_memmap[QUARD_STAR_UART0].base,
        0,
        qdev_get_gpio_in(DEVICE(s->plic[0]), QUARD_STAR_UART0_IRQ),
        399193,
        serial_hd(0),
        DEVICE_LITTLE_ENDIAN);

    /* 创建 UART1，中断接入 PLIC */
    serial_mm_init(
        system_memory,
        quard_star_memmap[QUARD_STAR_UART1].base,
        0,
        qdev_get_gpio_in(DEVICE(s->plic[0]), QUARD_STAR_UART1_IRQ),
        399193,
        serial_hd(1),
        DEVICE_LITTLE_ENDIAN);

    /* 创建 UART2，中断接入 PLIC */
    serial_mm_init(
        system_memory,
        quard_star_memmap[QUARD_STAR_UART2].base,
        0,
        qdev_get_gpio_in(DEVICE(s->plic[0]), QUARD_STAR_UART2_IRQ),
        399193,
        serial_hd(2),
        DEVICE_LITTLE_ENDIAN);
}

/* 创建 RTC 实时时钟设备 */
static void quard_star_rtc_create(MachineState *machine)
{
    QuardStarState *s = QUARD_STAR_MACHINE(machine);

    /* 将 RTC 映射到物理地址，中断接入 PLIC */
    sysbus_create_simple(
        "goldfish_rtc",
        quard_star_memmap[QUARD_STAR_RTC].base,
        qdev_get_gpio_in(DEVICE(s->plic[0]),
                         QUARD_STAR_RTC_IRQ));
}

/* 创建 virtio-mmio 设备接口 */
static void quard_star_virtio_mmio_create(MachineState *machine)
{
    QuardStarState *s = QUARD_STAR_MACHINE(machine);

    /* 创建 virtio-mmio 0，中断接入 PLIC */
    sysbus_create_simple(
        "virtio-mmio",
        quard_star_memmap[QUARD_STAR_VIRTIO0].base,
        qdev_get_gpio_in(DEVICE(s->plic[0]),
                         QUARD_STAR_VIRTIO0_IRQ));
}

/* Quard Star 板卡硬件初始化入口 */
static void quard_star_machine_init(MachineState *machine)
{
    quard_star_cpu_create(machine);
    quard_star_memory_create(machine);
    quard_star_flash_create(machine);
    quard_star_plic_create(machine);
    quard_star_aclint_create(machine);
    quard_star_serial_create(machine);
    quard_star_rtc_create(machine);
    quard_star_virtio_mmio_create(machine);
}

/* 初始化 Quard Star Machine 类 */
static void quard_star_machine_class_init(ObjectClass *oc, void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);

    mc->desc = "RISC-V Quard Star Board";
    mc->init = quard_star_machine_init;
    mc->max_cpus = QUARD_STAR_CPUS_MAX;
    mc->default_cpu_type = TYPE_RISCV_CPU_BASE;

    /* 配置 RISC-V CPU 拓扑及 NUMA 支持 */
    mc->possible_cpu_arch_ids = riscv_numa_possible_cpu_arch_ids;
    mc->cpu_index_to_instance_props = riscv_numa_cpu_index_to_props;
    mc->get_default_cpu_node_id = riscv_numa_get_default_cpu_node_id;
    mc->numa_mem_supported = true;
}

static void quard_star_machine_instance_init(Object *obj)
{
}

/* 定义 Quard Star Machine 类型信息 */
static const TypeInfo quard_star_machine_typeinfo = {
    .name = TYPE_RISCV_QUARD_STAR_MACHINE,
    .parent = TYPE_MACHINE,
    .class_init = quard_star_machine_class_init,
    .instance_init = quard_star_machine_instance_init,
    .instance_size = sizeof(QuardStarState),
};

/* 注册 Quard Star Machine 类型 */
static void quard_star_machine_init_register_types(void)
{
    type_register_static(&quard_star_machine_typeinfo);
}

type_init(quard_star_machine_init_register_types)
