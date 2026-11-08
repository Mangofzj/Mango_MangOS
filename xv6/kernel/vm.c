#include "types.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

extern char etext[];  /* 内核代码段结束地址 */

pagetable_t kernel_pagetable;  /* 内核页表根指针 */

/**
 * @brief 创建内核页表，映射内核内存、设备寄存器与进程内核栈。
 *
 * @return 内核页表根指针
 */
pagetable_t
kvmmake(void)
{
    pagetable_t kpgtbl;

    kpgtbl = (pagetable_t)kalloc();
    memset(kpgtbl, 0, PGSIZE);

    kvmmap(kpgtbl, PLIC, PLIC, 0x210000, PTE_R | PTE_W);  /* PLIC 寄存器 */
    kvmmap(kpgtbl, UART0, UART0, PGSIZE, PTE_R | PTE_W);  /* UART0 寄存器 */
    kvmmap(kpgtbl, KERNBASE, KERNBASE, (uint64)etext - KERNBASE, PTE_R | PTE_X);  /* 内核代码段，可读可执行 */
    kvmmap(kpgtbl, (uint64)etext, (uint64)etext, PHYSTOP - (uint64)etext, PTE_R | PTE_W);  /* etext 之后的物理内存，可读可写 */

    proc_mapstacks(kpgtbl);  /* 进程内核栈 */

    return kpgtbl;
}

/**
 * @brief 在内核页表中建立虚拟地址到物理地址的映射。
 *
 * @param kpgtbl 内核页表根指针
 * @param va 虚拟地址
 * @param pa 物理地址
 * @param sz 映射大小（字节）
 * @param perm 页表项权限
 * @note mappages 返回非 0 时 panic。
 */
void
kvmmap(pagetable_t kpgtbl, uint64 va, uint64 pa, uint64 sz, int perm)
{
    if (mappages(kpgtbl, va, sz, pa, perm) != 0)
        panic("kvmmap");
}

/**
 * @brief 创建内核页表并保存到 kernel_pagetable。
 */
void
kvminit(void)
{
    kernel_pagetable = kvmmake();
}

/**
 * @brief 将当前 hart 切换到内核页表。
 *
 * 写入 satp 前后各刷新一次 TLB。
 */
void
kvminithart(void)
{
    sfence_vma();

    w_satp(MAKE_SATP(kernel_pagetable));

    sfence_vma();
}

/**
 * @brief 查找虚拟地址对应的页表项，必要时创建中间页表。
 *
 * 从第 2 级开始逐级查找；遇到无效项时，仅在 alloc 非 0 的情况下分配新页表。
 *
 * @param pagetable 页表指针
 * @param va 虚拟地址，必须小于 MAXVA
 * @param alloc 非 0 时分配缺失的中间页表
 * @note 返回的表项可能为无效项。
 * @return 指向第 0 级页表项的指针；无法得到中间页表时返回 0
 */
pte_t *
walk(pagetable_t pagetable, uint64 va, int alloc)
{
    if (va >= MAXVA)
        panic("walk");

    for (int level = 2; level > 0; level--) {
        pte_t *pte = &pagetable[PX(level, va)];

        if (*pte & PTE_V) {
            pagetable = (pagetable_t)PTE2PA(*pte);
        } else {
            if (!alloc || (pagetable = (pde_t *)kalloc()) == 0)
                return 0;

            memset(pagetable, 0, PGSIZE);
            *pte = PA2PTE(pagetable) | PTE_V;
        }
    }

    return &pagetable[PX(0, va)];
}

/**
 * @brief 建立一段虚拟地址到物理地址的逐页映射。
 *
 * va 与 size 必须按页对齐，size 不能为 0；目标页表项已有效时 panic。
 *
 * @param pagetable 页表指针
 * @param va 虚拟地址起始地址
 * @param size 映射大小（字节）
 * @param pa 物理地址起始地址
 * @param perm 页表项权限
 * @return 成功返回 0；中间页表分配失败时返回 -1
 */
int
mappages(pagetable_t pagetable, uint64 va, uint64 size, uint64 pa, int perm)
{
    uint64 a, last;
    pte_t *pte;

    if ((va % PGSIZE) != 0)
        panic("mappages: va not aligned");

    if ((size % PGSIZE) != 0)
        panic("mappages: size not aligned");

    if (size == 0)
        panic("mappages: size");

    a = va;
    last = va + size - PGSIZE;  /* 最后一页的虚拟地址 */

    for (;;) {
        if ((pte = walk(pagetable, a, 1)) == 0)
            return -1;

        if (*pte & PTE_V)
            panic("mappages: remap");

        *pte = PA2PTE(pa) | perm | PTE_V;

        if (a == last)
            break;

        a += PGSIZE;
        pa += PGSIZE;
    }

    return 0;
}
