#include "kernel/types.h"
#include "user/user.h"

char *argv[] = { "sh", 0 };  /* 启动 shell 的参数数组 */

/**
 * @brief 启动 shell，并在其退出后重新启动。
 *
 * 子进程 exec sh，父进程等待 shell 退出；wait 出错时退出。
 */
int
main(void)
{
    int pid, wpid;

    for (;;) {
        printf("init: starting sh\n");
        pid = fork();
        if (pid < 0) {
            printf("init: fork failed\n");
            exit(1);
        }
        if (pid == 0) {
            exec("sh", argv);
            printf("init: exec sh failed\n");
            exit(1);
        }

        for (;;) {
            wpid = wait((int *)0);
            printf("init: wait returned successfully, process[%d] exited\n", wpid);

            if (wpid == pid) {
                break;
            } else if (wpid < 0) {
                printf("init: wait returned an error\n");
                exit(1);
            } else {
            }
        }
    }
}
