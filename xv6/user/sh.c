#include "kernel/types.h"
#include "user/user.h"

/**
 * @brief 演示父进程退出后子进程被 init 收养。
 */
void
cmd_zombie(void)
{
    printf("zombie: starting\n");

    int pid = fork();
    if (pid < 0) {
        printf("zombie: fork failed\n");
        exit(1);
    }
    if (pid > 0) {
        pause(50);
        printf("zombie: parent[%d] exiting, child[%d] was adopted by init\n", getpid(), pid);
    } else {
        pause(100);
        printf("zombie: child[%d] exiting\n", getpid());
    }

    exit(0);
}

#define BUF_SIZE (64 * 1024)  /* 每次分配的内存大小 */

/**
 * @brief 内存分配测试：连续分配 BUF_SIZE 大小的内存并记录耗时。
 *
 * 共分配 2000 次且不释放，分配失败时退出。
 */
void cmd_memtest(void)
{
    uint64 t1, t2;
    char *buf;

    printf("memtest: start ......\n");
    t1 = uptime();
    for (int i = 0; i < 256; i++) {
        buf = malloc(BUF_SIZE);
        if (buf == 0) {
            printf("Failed to allocate memory\n");
            exit(1);
        }
    }
    t2 = uptime();
    printf("memtest: end. Took %ld ticks\n", t2 - t1);

    exit(0);
}

/**
 * @brief 执行一条 shell 命令。
 *
 * 支持 exit、zombie、kill、memtest；kill 会终止 shell 自身，
 * 其余命令打印不支持提示。
 *
 * @param cmd 命令字符串
 */
void
runcmd(char *cmd)
{
    if (strcmp(cmd, "exit") == 0) {
        printf("Exiting shell...\n");
        exit(0);
    } else if (strcmp(cmd, "zombie") == 0) {
        cmd_zombie();
    } else if (strcmp(cmd, "kill") == 0) {
        printf("Oops, myself is killed...\n");
        kill(getpid());
    } else if (strcmp(cmd, "memtest") == 0) {
        cmd_memtest();
    } else {
        printf("Running command: '%s', unsupported!\n", cmd);
    }
}

/**
 * @brief 打印提示符并读取一条命令。
 *
 * @param buf 命令缓冲区
 * @param nbuf 缓冲区大小
 * @return 输入为空时返回 -1，否则返回 0
 */
int
getcmd(char *buf, int nbuf)
{
    write(2, "$ ", 2);
    memset(buf, 0, nbuf);
    gets(buf, nbuf);
    if (buf[0] == 0)
        return -1;

    return 0;
}

/**
 * @brief shell 主循环：读取并执行命令。
 */
void
main(void)
{
    static char buf[100];

    while (getcmd(buf, sizeof(buf)) >= 0) {
        char *cmd = buf;

        while (*cmd == ' ' || *cmd == '\t')
            cmd++;
        if (*cmd == '\n')
            continue;
        /* 去掉行尾换行符 */
        cmd[strlen(cmd) - 1] = '\0';
        runcmd(cmd);
    }
}
