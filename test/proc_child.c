#include "apr.h"
#include <stdio.h>
#if APR_HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>

int main(void)
{
    char buf[256];

    read(STDIN_FILENO, buf, 256);
    fprintf(stdout, "%s", buf);

    return 0; /* just to keep the compiler happy */
}
