#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include "apr_file_io.h"
#include "apr_general.h"

int main(int argc, char *argv[])
{
    apr_file_t *fd = NULL;
    char ch;
    int status = 0;
    apr_pool_t *context;

    apr_create_pool(&context, NULL); 

    apr_open(&fd, argv[1], APR_READ, -1, context);
    
    while (!status) {
        status = apr_getc(&ch, fd);
        if (status == APR_EOF )
            fprintf(stdout, "EOF, YEAH!!!!!!!!!\n");
        else if (status == APR_SUCCESS)
            fprintf(stdout, "%c", ch);
        else
            fprintf(stdout, " Big error, NOooooooooo!\n");
    }
    return 1; 
}    
