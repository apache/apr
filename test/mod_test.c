#include <stdio.h>

void print_hello(char str[256])
{
    apr_cpystrn(str, "Hello - I'm a DSO!\n", strlen("Hello - I'm a DSO!\n") + 1);
}

int count_reps(int reps)
{
    int i = 0;
    for (i = 0;i < reps; i++);
    return i;
}
