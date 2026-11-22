#include "kernel/types.h"
#include "user/user.h"

typedef long Align;
/* 空闲块头部 */
union header {
    struct {
        union header *ptr;  /* 指向下一个空闲块 */
        uint size;  /* 块大小 */
    } s;

    Align x;  /* 保证头部按 Align 对齐 */
};

typedef union header Header;

static Header base;  /* 空闲链表哨兵节点 */
static Header *freep;  /* 空闲链表起始指针 */

/**
 * @brief 释放 malloc 分配的内存块。
 *
 * 将块按地址顺序插入空闲链表，并合并相邻的空闲块。
 *
 * @param ap 待释放的内存块地址
 */
void
free(void *ap)
{
    Header *bp, *p;

    bp = (Header *)ap - 1;
    /* 查找 bp 在空闲链表中的插入位置 */
    for (p = freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr)
        if (p >= p->s.ptr && (bp > p || bp < p->s.ptr))
        break;
    /* 与后一个空闲块合并 */
    if (bp + bp->s.size == p->s.ptr) {
        bp->s.size += p->s.ptr->s.size;
        bp->s.ptr = p->s.ptr->s.ptr;
    } else
        bp->s.ptr = p->s.ptr;
    /* 与前一个空闲块合并 */
    if (p + p->s.size == bp) {
        p->s.size += bp->s.size;
        p->s.ptr = bp->s.ptr;
    } else
        p->s.ptr = bp;
    freep = p;
}

/**
 * @brief 通过 sbrk 扩展用户内存，并将新增内存加入空闲链表。
 *
 * 请求大小不足 4096 个 Header 单位时按 4096 处理；sbrk 失败时返回 0。
 *
 * @param nu 需要的 Header 单位数
 * @return 空闲链表起始指针；sbrk 失败时返回 0
 */
static Header *
morecore(uint nu)
{
    char *p;
    Header *hp;

    if (nu < 4096)
        nu = 4096;
    p = sbrk(nu * sizeof(Header));
    if (p == SBRK_ERROR)
        return 0;
    hp = (Header *)p;
    hp->s.size = nu;
    /* 将新内存块加入空闲链表 */
    free((void *)(hp + 1));

    return freep;
}

/**
 * @brief 从空闲链表分配内存块。
 *
 * 所需空间按 Header 大小向上取整，并额外包含一个头部单位；
 * 找到足够大的块后拆分，没有合适块时通过 morecore 扩展内存。
 *
 * @param nbytes 请求分配的字节数
 * @return 分配到的内存块地址，位于块头部之后；内存不足时返回 0
 */
void *
malloc(uint nbytes)
{
    Header *p, *prevp;
    uint nunits;

    /* 需求量按 Header 单位向上取整，另加一个头部单位 */
    nunits = (nbytes + sizeof(Header) - 1) / sizeof(Header) + 1;
    if ((prevp = freep) == 0) {
        base.s.ptr = freep = prevp = &base;
        base.s.size = 0;
    }
    for (p = prevp->s.ptr; ; prevp = p, p = p->s.ptr) {
        if (p->s.size >= nunits) {
            /* 刚好匹配时摘下整块，否则从块尾切出所需大小 */
            if (p->s.size == nunits)
                prevp->s.ptr = p->s.ptr;
            else {
                p->s.size -= nunits;
                p += p->s.size;
                p->s.size = nunits;
            }
            freep = prevp;
            return (void *)(p + 1);
        }
        if (p == freep) {
            /* 遍历一圈仍未找到，扩展内存后继续查找 */
            if ((p = morecore(nunits)) == 0)
                return 0;
        }
    }
}
