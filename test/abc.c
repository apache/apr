#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include "apr_file_io.h"
#include "apr_general.h"

int main(int argc, char *argv[])
{
    ap_file_t *fd;
    char ch;
    int status = 0;
    ap_context_t *context;

    ap_create_context(&context, NULL); 

    ap_open(&fd, context, argv[1], APR_READ, -1);
    
    while (!status) {
        status = ap_getc(fd, &ch);
        if (status == APR_EOF )
            fprintf(stdout, "EOF, YEAH!!!!!!!!!\n");
        else if (status == APR_SUCCESS)
            fprintf(stdout, "%c", ch);
        else
            fprintf(stdout, " Big error, NOooooooooo!\n");
    }
    return 1; 
}    
