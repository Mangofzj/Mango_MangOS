/*
 * Quard Star 平台支持：从设备树初始化平台信息与驱动，并定义平台操作表与平台实例
 */

#include <libfdt.h>
#include <sbi/riscv_asm.h>
#include <sbi/sbi_hartmask.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_string.h>
#include <sbi_utils/fdt/fdt_domain.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/irqchip/fdt_irqchip.h>
#include <sbi_utils/serial/fdt_serial.h>
#include <sbi_utils/timer/fdt_timer.h>
#include <sbi_utils/ipi/fdt_ipi.h>

extern struct sbi_platform platform;
/* Hart 索引到 hartid 的映射表，索引按设备树 CPU 节点顺序分配 */
static u32 quard_star_hart_index2id[SBI_HARTMASK_MAX_BITS] = { 0 };

/**
 * @brief 从设备树初始化平台名称与 Hart 信息
 *
 * 由 OpenSBI 在引导 Hart 上早期调用，在 platform 实例被使用前完成平台相关配置。
 *
 * @param arg1 上一级引导阶段传入的 FDT 地址
 * @note arg0、arg2 ~ arg4 未使用
 * @return FDT 地址，原样返回 arg1
 */
unsigned long fw_platform_init(unsigned long arg0, unsigned long arg1,
				unsigned long arg2, unsigned long arg3,
				unsigned long arg4)
{
	const char *model;
	void *fdt = (void *)arg1;
	u32 hartid, hart_count = 0;
	int rc, root_offset, cpus_offset, cpu_offset, len;

	/* 获取根节点，用于读取 model 属性作为平台名称 */
	root_offset = fdt_path_offset(fdt, "/");
	if (root_offset < 0)
		goto fail;

	/* 按平台名称缓冲区大小截断复制 */
	model = fdt_getprop(fdt, root_offset, "model", &len);
	if (model)
		sbi_strncpy(platform.name, model, sizeof(platform.name) - 1);

	/* 获取 /cpus 节点，用于枚举 Hart */
	cpus_offset = fdt_path_offset(fdt, "/cpus");
	if (cpus_offset < 0)
		goto fail;

	/* 遍历 /cpus 子节点，按节点顺序记录 hartid */
	fdt_for_each_subnode(cpu_offset, fdt, cpus_offset) {
		rc = fdt_parse_hart_id(fdt, cpu_offset, &hartid);
		if (rc)
			continue;

		/* 跳过超出 hartmask 位宽的 hartid */
		if (SBI_HARTMASK_MAX_BITS <= hartid)
			continue;

		/* 跳过设备树中已禁用的 CPU 节点 */
		if (!fdt_node_is_enabled(fdt, cpu_offset))
			continue;

		/* 按枚举顺序写入 hartid，并统计有效 Hart 数量 */
		quard_star_hart_index2id[hart_count++] = hartid;
	}

	/* 以可解析、hartid 未超出范围且已启用的 CPU 节点数作为平台 Hart 数量 */
	platform.hart_count = hart_count;

	return arg1;

fail:
	/* FDT 无效或缺少 /cpus 节点，循环执行 wfi 等待中断 */
	while (1)
		wfi();
}

/* 平台早期初始化钩子：当前无需额外处理 */
static int quard_star_early_init(bool cold_boot)
{
	return 0;
}

/**
 * @brief 平台最终初始化：修正下一级启动使用的设备树
 * @param cold_boot 是否为冷启动；非冷启动时直接返回
 * @return 固定返回 0
 */
static int quard_star_final_init(bool cold_boot)
{
	void *fdt;

	if (!cold_boot)
		return 0;

	/* 取回当前 Hart 保存的下一级启动 FDT 地址 */
	fdt = sbi_scratch_thishart_arg1_ptr();

	/* 修正 CPU 节点、PLIC 中断映射与保留内存 */
	fdt_cpu_fixup(fdt);
	fdt_fixups(fdt);

	return 0;
}

/* 平台早期退出钩子：当前无需额外处理 */
static void quard_star_early_exit(void)
{
}

/* 平台最终退出钩子：当前无需额外处理 */
static void quard_star_final_exit(void)
{
}

/**
 * @brief 获取 TLB 范围刷新的上限
 * @return 默认上限（页大小 4 KiB）
 */
static u64 quard_star_tlbr_flush_limit(void)
{
	return SBI_PLATFORM_TLB_RANGE_FLUSH_LIMIT_DEFAULT;
}

/* 平台操作表：平台专用钩子与 FDT 通用驱动初始化 */
const struct sbi_platform_operations platform_ops = {
	.early_init             = quard_star_early_init,
	.final_init             = quard_star_final_init,
	.early_exit             = quard_star_early_exit,
	.final_exit             = quard_star_final_exit,
	.console_init           = fdt_serial_init,
	.irqchip_init           = fdt_irqchip_init,
	.irqchip_exit           = fdt_irqchip_exit,
	.ipi_init               = fdt_ipi_init,
	.ipi_exit               = fdt_ipi_exit,
	.get_tlbr_flush_limit   = quard_star_tlbr_flush_limit,
	.timer_init             = fdt_timer_init,
	.timer_exit             = fdt_timer_exit,
};

/* 平台描述实例：Hart 数量与映射表内容由 fw_platform_init() 按设备树填写 */
struct sbi_platform platform = {
	.opensbi_version   = OPENSBI_VERSION,
	.platform_version  = SBI_PLATFORM_VERSION(0x0, 0x01),
	.name              = "Quard Star",
	.features          = SBI_PLATFORM_DEFAULT_FEATURES,
	.hart_count        = SBI_HARTMASK_MAX_BITS,
	.hart_index2id     = quard_star_hart_index2id,
	.hart_stack_size   = SBI_PLATFORM_DEFAULT_HART_STACK_SIZE,
	.platform_ops_addr = (unsigned long)&platform_ops,
};
