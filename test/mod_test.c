#include <stdio.h>

int goodbyes = 0;

void print_hello(void)
{
    fprintf(stdout,"Hello - I'm a DSO!\n");
}

int print_goodbye(int reps)
{
    int i = 0;
    for (i = 0;i < reps; i++) {
        fprintf (stdout, "Goodbye from the DSO! (%d of %d)\n", i+1, reps);
    }
    goodbyes = reps;
}
