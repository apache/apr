#ifndef APR_PRIVATE_H
#define APR_PRIVATE_H

@TOP@

/* Various #defines we need to know about */
#undef HAVE_LOCK_EX
#undef HAVE_F_SETLK
#undef HAVE_CODESET
#undef HAVE_PTHREAD_PROCESS_SHARED
#undef DEV_RANDOM
#undef HAVE_TRUERAND
#undef HAVE_POLLIN
#undef HAVE_isascii
#undef HAVE_SO_ACCEPT_FILTER

/* Cross process serialization techniques */
#undef USE_FLOCK_SERIALIZE
#undef USE_SYSVSEM_SERIALIZE
#undef USE_FCNTL_SERIALIZE
#undef USE_PROC_PTHREAD_SERIALIZE
#undef USE_PTHREAD_SERIALIZE

#undef READDIR_IS_THREAD_SAFE

#undef NEED_RLIM_T
#undef USEBCOPY

#undef HAVE_GMTOFF
#undef USE_THREADS

#undef DSO_USE_DLFCN
#undef DSO_USE_SHL
#undef DSO_USE_DYLD

#undef SIZEOF_SSIZE_T
#undef SIZEOF_SIZE_T
#undef SIZEOF_OFF_T
#undef SIZEOF_PID_T

#undef HAVE_INT64_C

#undef HAVE_MM_SHMT_MMFILE

/* BeOS specific flag */
#undef HAVE_BONE_VERSION

/* Does this system have a corkable TCP? */
#undef HAVE_TCP_CORK
#undef HAVE_TCP_NOPUSH

@BOTTOM@

/* Make sure we have ssize_t defined to be something */
#undef ssize_t

/* switch this on if we have a BeOS version below BONE */
#if BEOS && !HAVE_BONE_VERSION
#define BEOS_R5 1
#else
#define BEOS_BONE 1
#endif

#ifdef SIGWAIT_TAKES_ONE_ARG
#define apr_sigwait(a,b) ((*(b)=sigwait((a)))<0?-1:0)
#else
#define apr_sigwait(a,b) sigwait((a),(b))
#endif

/* Macros to deal with using either a pool or an sms
 * to do memory stuff...
 */
#define APR_CLEANUP_REGISTER(struct, data, func, scope) \
    if (struct->pool) { \
        apr_pool_cleanup_register(struct->pool, data, func, scope); \
    } else { \
        apr_sms_cleanup_register(struct->mem_sys, APR_CHILD_CLEANUP, \
                                 data, func); \
    }

#define APR_CLEANUP_REMOVE(struct, data, func) \
    if (struct->pool) { \
        apr_pool_cleanup_kill(struct->pool, data, func); \
    } else { \
        apr_sms_cleanup_unregister(struct->mem_sys, APR_CHILD_CLEANUP, \
                                   data, func); \
    }

#define APR_MEM_PSTRDUP(struct, ptr, str) \
    if (struct->pool) { \
        ptr = apr_pstrdup(struct->pool, str); \
    } else { \
        size_t len = strlen(str) + 1; \
        ptr = (char*) apr_sms_calloc(struct->mem_sys, len); \
        memcpy(ptr, str, len); \
    }

#define APR_MEM_MALLOC(ptr, struct, type) \
    if (struct->pool) { \
        ptr = (type *)apr_palloc(struct->pool, sizeof(type)); \
    } else { \
        ptr = (type *)apr_sms_malloc(struct->mem_sys, sizeof(type)); \
    }

#define APR_MEM_CALLOC(ptr, struct, type) \
    if (struct->pool) { \
        ptr = (type *)apr_pcalloc(struct->pool, sizeof(type)); \
    } else { \
        ptr = (type *)apr_sms_calloc(struct->mem_sys, sizeof(type)); \
    }

#endif /* APR_PRIVATE_H */
