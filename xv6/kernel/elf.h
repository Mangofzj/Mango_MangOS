#define ELF_MAGIC 0x464C457FU  /* ELF 魔数，字节序列为 7F 45 4C 46 */

/* ELF 文件头 */
struct elfhdr {
    uint   magic;  /* ELF 魔数 */
    uchar  elf[12];  /* 标识剩余 12 字节 */
    ushort type;  /* 文件类型 */
    ushort machine;  /* 目标架构 */
    uint   version;  /* ELF 版本 */
    uint64 entry;  /* 入口地址 */
    uint64 phoff;  /* 程序头表偏移 */
    uint64 shoff;  /* 节头表偏移 */
    uint   flags;  /* 处理器相关标志 */
    ushort ehsize;  /* 文件头大小 */
    ushort phentsize;  /* 程序头表项大小 */
    ushort phnum;  /* 程序头表项数 */
    ushort shentsize;  /* 节头表项大小 */
    ushort shnum;  /* 节头表项数 */
    ushort shstrndx;  /* 节名字符串表索引 */
};

/* ELF 程序头 */
struct proghdr {
    uint32 type;  /* 段类型 */
    uint32 flags;  /* 段权限标志 */
    uint64 off;  /* 段在文件中的偏移 */
    uint64 vaddr;  /* 段虚拟地址 */
    uint64 paddr;  /* 段物理地址 */
    uint64 filesz;  /* 段在文件中的大小 */
    uint64 memsz;  /* 段在内存中的大小 */
    uint64 align;  /* 对齐要求 */
};

/* 程序头类型 */
#define ELF_PROG_LOAD 1  /* 可加载段 */

/* 程序头权限标志 */
#define ELF_PROG_FLAG_EXEC  1  /* 可执行 */
#define ELF_PROG_FLAG_WRITE 2  /* 可写 */
#define ELF_PROG_FLAG_READ  4  /* 可读 */
