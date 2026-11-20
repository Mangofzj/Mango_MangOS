#include "kernel/types.h"
#include "user/user.h"

char *argv[] = { "sh", 0 };

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
