#include "user/user.h"

int
main(void)
{
    char c;

    for (;;) {
        read(0, &c, 1);
        printf("->");
        printf("%c", c);
    }
}
