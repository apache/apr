#include "apr_config.h"
#include "apr_general.h"
#include "apr_pools.h"
#include "apr_errno.h"
#include "apr_dso.h"
#include <string.h>
#include <stdlib.h>

#define LIB_NAME "mod_test.so"

int main (int argc, char ** argv)
{
    ap_dso_handle_t *h = NULL;
    ap_dso_handle_sym_t func1 = NULL;
    ap_dso_handle_sym_t func2 = NULL;
    ap_pool_t *cont;
    void (*function)(void);
    void (*function1)(int);
    int *retval;
    char filename[256];   

    getcwd(filename, 256);
    strcat(filename, "/");
    strcat(filename, LIB_NAME);

    ap_initialize();
    atexit(ap_terminate);
        
    if (ap_create_pool(&cont, NULL) != APR_SUCCESS) {
        fprintf(stderr, "Couldn't allocate context.");
        exit(-1);
    }

    fprintf(stdout,"Initializing DSO's.........................");
    if (ap_dso_init() != APR_SUCCESS) {
        fprintf(stderr, "Couldn't initialize DSO's !");
        exit (-1);
    }
    fprintf(stdout,"OK\n");
    fprintf(stdout,"Trying to load DSO now.....................");
    fflush(stdout);
    if (ap_dso_load(&h, filename, cont) != APR_SUCCESS){
        fprintf(stderr, "Failed to load %s!\n", filename);
        exit (-1);
    }
    fprintf(stdout,"OK\n");

    fprintf(stdout,"Trying to get the DSO's attention..........");
    fflush(stdout);
    if (ap_dso_sym(&func1, h, "print_hello") != APR_SUCCESS) { 
        fprintf(stderr, "Failed!\n");
        exit (-1);
    }        
    fprintf(stdout,"OK\n");
    
    function = (void *)func1;
    (*function)();

    fprintf(stdout,"Saying farewell 5 times....................");
    fflush(stdout);
    if (ap_dso_sym(&func2, h, "print_goodbye") != APR_SUCCESS) {
        fprintf(stderr, "Failed!\n");
        exit (-1);
    }        
    fprintf(stdout,"OK\n");

    function1 = (void *)(int)func2;
    (*function1)(5);

    fprintf(stdout,"Checking how many times I said goodbye..");
    fflush(stdout);
    if (ap_dso_sym(&func1, h, "goodbyes") != APR_SUCCESS) {
        fprintf(stderr, "Failed!\n");
        exit (-1);
    }
    retval = (int *)func1;
    fprintf(stdout,"%d..", (*retval));
    fflush(stdout);
    if ((*retval) == 5){
        fprintf(stderr,"OK\n");
    } else {
        fprintf(stderr,"Failed!\n");
    }
       
    fprintf(stdout,"Trying to unload DSO now...................");
    if (ap_dso_unload(h) != APR_SUCCESS) {
        fprintf(stderr, "Failed!\n");
        exit (-1);
    }
    fprintf(stdout,"OK\n");

    fprintf(stdout,"Checking it's been unloaded................");
    fflush(stdout);
    if (ap_dso_sym(&func1, h, "print_hello") == APR_SUCCESS) {
        fprintf(stderr, "Failed!\n");
        exit (-1);
    }        
    fprintf(stdout,"OK\n");
    
    return 0;
}
