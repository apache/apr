#ifndef APR_PRIVATE_H
#define APR_PRIVATE_H

@TOP@

/* Various #defines we need to know about */
#undef HAVE_LOCK_EX
#undef HAVE_F_SETLK
#undef HAVE_SEM_UNDO
#undef HAVE_PTHREAD_PROCESS_SHARED
#undef DEV_RANDOM
#undef HAVE_EGD
#undef EGD_DEFAULT_SOCKET
#undef HAVE_TRUERAND
#undef HAVE_POLLIN
#undef HAVE_isascii
#undef HAVE_SO_ACCEPTFILTER
#undef HAVE_MAP_ANON

/* Cross process serialization techniques */
#undef USE_FLOCK_SERIALIZE
#undef USE_SYSVSEM_SERIALIZE
#undef USE_FCNTL_SERIALIZE
#undef USE_PROC_PTHREAD_SERIALIZE
#undef USE_PTHREAD_SERIALIZE

#undef POSIXSEM_IS_GLOBAL
#undef SYSVSEM_IS_GLOBAL
#undef FCNTL_IS_GLOBAL
#undef FLOCK_IS_GLOBAL

#undef READDIR_IS_THREAD_SAFE
#undef GETHOSTBYNAME_IS_THREAD_SAFE
#undef GETHOSTBYADDR_IS_THREAD_SAFE
#undef STRERROR_R_RC_INT

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

#undef HAVE_VLA

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

#endif /* APR_PRIVATE_H */
