#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(void)
{
    int rc=1;
    char buf[256];

    while (rc == 1) {
        read(STDERR_FILENO, buf, 256);
    }
    return 0; /* just to keep the compiler happy */
}
