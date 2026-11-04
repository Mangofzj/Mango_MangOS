#include "types.h"

void *
memset(void *dst, int c, uint n)
{
    char *cdst = (char *)dst;
    int i;

    for (i = 0; i < n; i++) {
        cdst[i] = c;
    }

    return dst;
}

char *
safestrcpy(char *s, const char *t, int n)
{
    char *os;

    os = s;
    if (n <= 0)
        return os;
    while (--n > 0 && (*s++ = *t++) != 0)
        ;
    *s = 0;
    return os;
}

int
strlen(const char *s)
{
    int n;

    for (n = 0; s[n]; n++)
        ;
    return n;
}
