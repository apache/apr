#include "apr.h"
#if APR_FORCE_ATOMIC_GENERIC 
#include "../unix/apr_atomic.c"
#else
#endif
