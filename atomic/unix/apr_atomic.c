#include "apr.h"
#include "apr_lock.h"
#include "apr_thread_mutex.h"
#include "apr_atomic.h"

#ifdef WIN32
/* win32 implementation is all macros */
#else

#if APR_HAS_THREADS

#define NUM_ATOMIC_HASH 7
/* shift by 2 to get rid of alignment issues */
#define ATOMIC_HASH(x) (int)(((long)x>>2)%NUM_ATOMIC_HASH)
static apr_thread_mutex_t **hash_mutex;

apr_status_t apr_atomic_init(apr_pool_t *p )
{
    int i;
    apr_status_t rv;
    hash_mutex =apr_palloc(p,sizeof(apr_thread_mutex_t*) * NUM_ATOMIC_HASH);
    for (i=0;i<NUM_ATOMIC_HASH;i++) {
        rv = apr_thread_mutex_create(&(hash_mutex[i]), APR_THREAD_MUTEX_DEFAULT, p);
        if (rv != APR_SUCCESS)
           return rv;
    }
    return APR_SUCCESS;
}
long apr_atomic_add(volatile long*mem, long val) 
{
    apr_thread_mutex_t *lock = hash_mutex[ATOMIC_HASH(mem)];
    long prev;
       
    if (apr_thread_mutex_lock(lock) == APR_SUCCESS) {
        prev = *mem;
        *mem += val;
        apr_thread_mutex_unlock(lock);
        return prev;
    }
    return *mem;
}
long apr_atomic_set(volatile long*mem, long val) 
{
    apr_thread_mutex_t *lock = hash_mutex[ATOMIC_HASH(mem)];
    long prev;

    if (apr_thread_mutex_lock(lock) == APR_SUCCESS) {
        prev = *mem;
        *mem = val;
        apr_thread_mutex_unlock(lock);
        return prev;
    }
    return *mem;
}

long apr_atomic_inc( volatile long *mem) 
{
    apr_thread_mutex_t *lock = hash_mutex[ATOMIC_HASH(mem)];
    long prev;

    if (apr_thread_mutex_lock(lock) == APR_SUCCESS) {
        prev = *mem;
        (*mem)++;
        apr_thread_mutex_unlock(lock);
        return prev;
    }
    return *mem;
}
long apr_atomic_dec(volatile long *mem) 
{
    apr_thread_mutex_t *lock = hash_mutex[ATOMIC_HASH(mem)];
    long prev;

    if (apr_thread_mutex_lock(lock) == APR_SUCCESS) {
        prev = *mem;
        (*mem)--;
        apr_thread_mutex_unlock(lock);
        return prev;
    }
    return *mem;
}
long apr_atomic_cas(volatile long *mem,long with,long cmp)
{
    apr_thread_mutex_t *lock = hash_mutex[ATOMIC_HASH(mem)];
    long prev;

    if (apr_thread_mutex_lock(lock) == APR_SUCCESS) {
        prev = *mem;
        if ( *mem == cmp) {
            *mem = with;
        }
        apr_thread_mutex_unlock(lock);
        return prev;
    }
    return *mem;
}

#endif /* APR_HAS_THREADS */

#endif /* default implementation */
