#ifndef APR_CONFIG_H
#define APR_CONFIG_H
@TOP@

/* Various #defines we need to know about */
#undef HAVE_STRUCT_UNION_SEMUN
#undef HAVE_LOCK_EX
#undef HAVE_F_SETLK
#undef HAVE_PTHREAD_PROCESS_SHARED

/* Cross process serialization techniques */
#undef USE_FLOCK_SERIALIZE
#undef USE_SYSVSEM_SERIALIZE
#undef USE_FCNTL_SERIALIZE
#undef USE_PROC_PTHREAD_SERIALIZE
#undef USE_PTHREAD_SERIALIZE

#undef NEED_RLIM_T
#undef USEBCOPY

@BOTTOM@
#define API_EXPORT(type) type
#define API_EXPORT_NONSTD(type) type
#define API_VAR_IMPORT extern

/* Make sure we have ssize_t defined to be somethine */
#undef ssize_t

/* We want this in config.h, because it is a macro that Windows requires.  This
 * way, every thread start function has this definition, and things are happy.
 */
#define API_THREAD_FUNC

#ifdef HAVE_SIGACTION
typedef void Sigfunc(int);

#if defined(SIG_ING) && !defined(SIG_ERR)
#define SIG_ERR ((Sigfunc *)-1)
#endif

#define signal(s,f)    ap_signal(s, f)
Sigfunc *signal(int signo, Sigfunc * func);
#endif


#endif /* APR_CONFIG_H */
