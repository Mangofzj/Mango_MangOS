#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "elf.h"

static int loadseg(pde_t *, uint64, uchar *, uint, uint);

extern uchar program__init[];  /* init 程序镜像起始地址 */
extern uchar program__sh[];

/**
 * @brief 将 ELF 段标志转换为页表项权限。
 *
 * 仅处理可执行与可写标志；可读与用户态权限由 uvmalloc 的基础权限提供。
 *
 * @param flags ELF 程序头的 flags 字段
 * @return 对应的 PTE_X、PTE_W 组合
 */
int
flags2perm(int flags)
{
    int perm = 0;

    if (flags & 0x1)
        perm = PTE_X;
    if (flags & 0x2)
        perm |= PTE_W;

    return perm;
}

/**
 * @brief 加载 init 程序并替换当前进程的用户地址空间。
 *
 * 从 program__init 读取 ELF 文件头与程序头，逐段分配并映射用户内存，
 * 在用户栈上布置参数与 argv 数组，最后设置入口地址和栈指针。
 *
 * @param path 程序路径，当前仅接受 "init"
 * @param argv 以 0 结尾的参数数组
 * @return 参数个数；路径不支持或加载过程中出错时返回 -1
 */
int
kexec(char *path, char **argv)
{
    char *s, *last;
    int i, off;
    uint64 argc, sz = 0, sp, ustack[MAXARG], stackbase;
    struct elfhdr elf;
    struct proghdr ph;
    pagetable_t pagetable = 0, oldpagetable;
    struct proc *p = myproc();
    uchar *program = 0;

    if (strncmp(path, "init", 5) == 0) {
        program = program__init;
    } else if (strncmp(path, "sh", 2) == 0) {
        program = program__sh;
    } else {
        goto bad;
    }

    memmove(&elf, program, sizeof(elf));
    if (elf.magic != ELF_MAGIC)
        goto bad;

    if ((pagetable = proc_pagetable(p)) == 0)
        goto bad;

    /* 映射所有可加载段 */
    for (i = 0, off = elf.phoff; i < elf.phnum; i++, off += sizeof(ph)) {
        memmove(&ph, program + off, sizeof(ph));
        if (ph.type != ELF_PROG_LOAD)
            continue;
        if (ph.memsz < ph.filesz)
            goto bad;
        if (ph.vaddr + ph.memsz < ph.vaddr)
            goto bad;
        if (ph.vaddr % PGSIZE != 0)
            goto bad;

        uint64 sz1;

        if ((sz1 = uvmalloc(pagetable, sz, ph.vaddr + ph.memsz, flags2perm(ph.flags))) == 0)
            goto bad;
        sz = sz1;
        if (loadseg(pagetable, ph.vaddr, program, ph.off, ph.filesz) < 0)
            goto bad;
    }

    p = myproc();
    uint64 oldsz = p->sz;
    sz = PGROUNDUP(sz);
    uint64 sz1;
    if ((sz1 = uvmalloc(pagetable, sz, sz + (USERSTACK+ 1) * PGSIZE, PTE_W)) == 0)
        goto bad;
    sz = sz1;
    /* 清除守护页的用户权限 */
    uvmclear(pagetable, sz - (USERSTACK + 1) * PGSIZE);
    sp = sz;
    stackbase = sp - USERSTACK * PGSIZE;

    /* 在用户栈上布置参数字符串与 argv 数组 */
    for (argc = 0; argv[argc]; argc++) {
        if (argc >= MAXARG)
            goto bad;
        sp -= strlen(argv[argc]) + 1;
        /* 按 RISC-V ABI 将栈指针向下对齐到 16 字节 */
        sp -= sp % 16;
        if (sp < stackbase)
            goto bad;
        if (copyout(pagetable, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
            goto bad;
        ustack[argc] = sp;
    }
    ustack[argc] = 0;

    sp -= (argc + 1) * sizeof(uint64);
    sp -= sp % 16;
    if (sp < stackbase)
        goto bad;
    if (copyout(pagetable, sp, (char *)ustack, (argc+1)*sizeof(uint64)) < 0)
        goto bad;
    p->trapframe->a1 = sp;

    /* 取路径最后一段作为进程名 */
    for (last = s = path; *s; s++) {
        if (*s == '/')
            last = s+1;
    }
    safestrcpy(p->name, last, sizeof(p->name));
    oldpagetable = p->pagetable;
    p->pagetable = pagetable;
    p->sz = sz;
    p->trapframe->epc = elf.entry;
    p->trapframe->sp = sp;
    proc_freepagetable(oldpagetable, oldsz);

    return argc;

bad:
    if (pagetable)
        proc_freepagetable(pagetable, sz);

    return -1;
}

/**
 * @brief 将 ELF 段内容复制到已映射的用户页。
 *
 * 按页查找物理地址并逐段复制；对应地址未映射时 panic。
 *
 * @param pagetable 用户页表根指针
 * @param va 段起始虚拟地址
 * @param ip 程序镜像起始地址
 * @param offset 段在镜像中的偏移
 * @param sz 段在文件中的大小
 * @return 固定返回 0
 */
static int
loadseg(pagetable_t pagetable, uint64 va, uchar *ip, uint offset, uint sz)
{
    uint i, n;
    uint64 pa;

    for(i = 0; i < sz; i += PGSIZE) {
        pa = walkaddr(pagetable, va + i);
        if(pa == 0)
            panic("loadseg: address should exist");
        if(sz - i < PGSIZE)
            n = sz - i;
        else
            n = PGSIZE;
        memmove((uchar *)pa, ip + offset + i, n);
    }

    return 0;
}
