#include "apr.h"
/* Pick up the default implementations of any atomic operations
 * that haven't been redefined as Sparc-specific functions
 */
#include "../unix/apr_atomic.c"
