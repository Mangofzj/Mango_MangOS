#include "types.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

extern char etext[];  /* 内核代码段结束地址 */

extern char trampoline[];  /* 跳板页代码起始地址 */

pagetable_t kernel_pagetable;  /* 内核页表根指针 */

/**
 * @brief 创建内核页表，映射内核内存、设备寄存器、跳板页与进程内核栈。
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
    kvmmap(kpgtbl, TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R | PTE_X);  /* 跳板页，可读可执行 */

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

uint64
walkaddr(pagetable_t pagetable, uint64 va)
{
    pte_t *pte;
    uint64 pa;

    if (va >= MAXVA)
        return 0;

    pte = walk(pagetable, va, 0);
    if (pte == 0)
        return 0;
    if ((*pte & PTE_V) == 0)
        return 0;
    if ((*pte & PTE_U) == 0)
        return 0;

    pa = PTE2PA(*pte);
    return pa;
}

/**
 * @brief 递归释放页表结构占用的页。
 *
 * 只处理非叶表项；遇到仍有效的叶表项时 panic，映射的物理页需先用 uvmunmap 释放。
 *
 * @param pagetable 待释放的页表指针
 */
void
freewalk(pagetable_t pagetable)
{
    for (int i = 0; i < 512; i++) {
        pte_t pte = pagetable[i];

        /* 有效且无读写执行权限，说明指向下一级页表 */
        if ((pte & PTE_V) && (pte & (PTE_R | PTE_W | PTE_X)) == 0) {
            uint64 child = PTE2PA(pte);
            freewalk((pagetable_t)child);
            pagetable[i] = 0;
        } else if (pte & PTE_V) {
            panic("freewalk: leaf");
        }
    }

    kfree((void *)pagetable);
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

/**
 * @brief 解除 [va, va + npages * PGSIZE) 范围的页映射。
 *
 * va 必须按页对齐；底层页表缺失、表项无效或非叶表项时 panic。
 *
 * @param pagetable 页表指针
 * @param va 起始虚拟地址
 * @param npages 解除映射的页数
 * @param do_free 非 0 时同时释放映射的物理页
 */
void
uvmunmap(pagetable_t pagetable, uint64 va,
         uint64 npages, int do_free)
{
    uint64 a;
    pte_t *pte;

    if ((va % PGSIZE) != 0)
        panic("uvmunmap: not aligned");

    for (a = va; a < va + npages * PGSIZE; a += PGSIZE) {
        if ((pte = walk(pagetable, a, 0)) == 0)
            panic("uvmunmap: walk");

        if ((*pte & PTE_V) == 0)
            panic("uvmunmap: not mapped");

        /* 只有有效位而没有权限位，说明是下一级页表 */
        if (PTE_FLAGS(*pte) == PTE_V)
            panic("uvmunmap: not a leaf");

        if (do_free) {
            uint64 pa = PTE2PA(*pte);
            kfree((void *)pa);
        }

        *pte = 0;
    }
}

/**
 * @brief 创建并清零一个用户页表根页。
 *
 * @return 页表根指针；分配失败时返回 0
 */
pagetable_t
uvmcreate(void)
{
    pagetable_t pagetable;

    pagetable = (pagetable_t)kalloc();
    if (pagetable == 0)
        return 0;

    memset(pagetable, 0, PGSIZE);
    return pagetable;
}

/**
 * @brief 释放用户页表及 sz 字节用户内存的映射。
 *
 * 先解除 [0, sz) 的映射并释放物理页，再释放页表结构。
 *
 * @param pagetable 用户页表根指针
 * @param sz 用户内存大小（字节）
 */
void
uvmfree(pagetable_t pagetable, uint64 sz)
{
    if (sz > 0)
        uvmunmap(pagetable, 0, PGROUNDUP(sz) / PGSIZE, 1);

    freewalk(pagetable);
}

int
copyin(pagetable_t pagetable, char *dst, uint64 srcva, uint64 len)
{
    uint64 n, va0, pa0;

    while (len > 0) {
        va0 = PGROUNDDOWN(srcva);
        pa0 = walkaddr(pagetable, va0);
        if (pa0 == 0)
            return -1;
        n = PGSIZE - (srcva - va0);
        if (n > len)
            n = len;
        memmove(dst, (void *)(pa0 + (srcva - va0)), n);

        len -= n;
        dst += n;
        srcva = va0 + PGSIZE;
    }
    return 0;
}

int
copyout(pagetable_t pagetable, uint64 dstva, char *src, uint64 len)
{
    uint64 n, va0, pa0;
    pte_t *pte;

    while (len > 0) {
        va0 = PGROUNDDOWN(dstva);
        if (va0 >= MAXVA)
            return -1;
        pa0 = walkaddr(pagetable, va0);
        if (pa0 == 0)
            return -1;
        pte = walk(pagetable, va0, 0);
        if ((*pte & PTE_W) == 0)
            return -1;
        n = PGSIZE - (dstva - va0);
        if (n > len)
            n = len;
        memmove((void *)(pa0 + (dstva - va0)), src, n);

        len -= n;
        src += n;
        dstva = va0 + PGSIZE;
    }

    return 0;
}
