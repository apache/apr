#ifndef APR_CONFIG_H
#define APR_CONFIG_H

/* So that we can use inline on some critical functions, and use
 * GNUC attributes (such as to get -Wall warnings for printf-like
 * functions).  Only do this in gcc 2.7 or later ... it may work
 * on earlier stuff, but why chance it.
 *
 * We've since discovered that the gcc shipped with NeXT systems
 * as "cc" is completely broken.  It claims to be __GNUC__ and so
 * on, but it doesn't implement half of the things that __GNUC__
 * means.  In particular it's missing inline and the __attribute__
 * stuff.  So we hack around it.  PR#1613. -djg
 */
#if !defined(__GNUC__) || __GNUC__ < 2 || \
    (__GNUC__ == 2 && __GNUC_MINOR__ < 7) ||\
    defined(NEXT)
#define ap_inline
#define __attribute__(__x)
#define ENUM_BITFIELD(e,n,w)  signed int n : w
#else
#define ap_inline __inline__
#define USE_GNU_INLINE
#define ENUM_BITFIELD(e,n,w)  e n : w
#endif

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

#undef READDIR_IS_THREAD_SAFE

#undef NEED_RLIM_T
#undef USEBCOPY

#undef HAVE_GMTOFF
#undef USE_THREADS

#undef SIZEOF_SSIZE_T

@BOTTOM@

/* Make sure we have ssize_t defined to be somethine */
#undef ssize_t

/* We want this in config.h, because it is a macro that Windows requires.  This
 * way, every thread start function has this definition, and things are happy.
 */
#define API_THREAD_FUNC

#ifdef HAVE_SIGACTION
typedef void Sigfunc(int);
#define signal(s,f)    ap_signal(s, f)
Sigfunc *signal(int signo, Sigfunc * func);

#if defined(SIG_ING) && !defined(SIG_ERR)
#define SIG_ERR ((Sigfunc *)-1)
#endif
#endif

#if !defined(HAVE_PTHREAD_SIGMASK) && defined(_AIX)
#define pthread_sigmask sigprocmask
#endif

#if !defined(HAVE_STRCASECMP) && defined(HAVE_STRICMP)
#define strcasecmp(s1,s2) stricmp(s1,s2)
#endif

#endif /* APR_CONFIG_H */
