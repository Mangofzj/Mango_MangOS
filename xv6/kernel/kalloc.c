#include "types.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"

/* 空闲页链表节点 */
struct run {
    struct run *next;  /* 下一个空闲页 */
};

/* 物理页分配器 */
struct {
    struct spinlock lock;  /* 保护 freelist 的自旋锁 */
    struct run *freelist;  /* 空闲页链表 */
} kmem;

void freerange(void *pa_start, void *pa_end);

extern char end[];  /* 内核镜像结束地址，由链接脚本定义 */

/**
 * @brief 初始化物理页分配器。
 *
 * 将 end 到 PHYSTOP 之间的物理页加入空闲链表。
 */
void
kinit()
{
    initlock(&kmem.lock, "kmem");
    freerange(end, (void *)PHYSTOP);
}

/**
 * @brief 释放 [pa_start, pa_end) 范围内的物理页。
 *
 * 起始地址向上取整到页边界，末尾不足一页的部分不释放。
 *
 * @param pa_start 释放范围起始地址
 * @param pa_end 释放范围结束地址（不含）
 */
void
freerange(void *pa_start, void *pa_end)
{
    char *p;

    p = (char *)PGROUNDUP((uint64)pa_start);

    for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
        kfree(p);
}

/**
 * @brief 释放一个物理页，将其加入空闲链表。
 *
 * @param pa 待释放页的物理地址
 * @note 页地址未按页对齐或超出 [end, PHYSTOP) 时 panic。
 */
void
kfree(void *pa)
{
    struct run *r;

    if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
        panic("kfree");

    memset(pa, 1, PGSIZE);

    r = (struct run *)pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
}

/**
 * @brief 从空闲链表分配一个物理页。
 *
 * @return 分配到的页地址；空闲链表为空时返回 0
 */
void *
kalloc(void)
{
    struct run *r;

    acquire(&kmem.lock);
    r = kmem.freelist;
    if (r)
        kmem.freelist = r->next;
    release(&kmem.lock);

    if (r)
        memset((char *)r, 5, PGSIZE);
    return (void *)r;
}
