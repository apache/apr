dnl -----------------------------------------------------------------
dnl apr_hints.m4: APR's autoconf macros for platform-specific hints
dnl
dnl  We preload various configure settings depending
dnl  on previously obtained platform knowledge.
dnl  We allow all settings to be overridden from
dnl  the command-line.
dnl
dnl  We maintain the "format" that we've used
dnl  under 1.3.x, so we don't exactly follow
dnl  what is "recommended" by autoconf.

dnl
dnl APR_PRELOAD
dnl
dnl  Preload various ENV/makefile params such as CC, CFLAGS, etc
dnl  based on outside knowledge
dnl
dnl  Generally, we force the setting of CC, and add flags
dnl  to CFLAGS, CPPFLAGS, LIBS and LDFLAGS. 
dnl
AC_DEFUN(APR_PRELOAD, [
if test "x$apr_preload_done" != "xyes" ; then

  apr_preload_done="yes"

  echo "Applying APR hints file rules for $host"

  case "$host" in
    *mint)
	APR_ADDTO(CPPFLAGS, [-DMINT])
	APR_ADDTO(LIBS, [-lportlib])
	;;
    *MPE/iX*)
	APR_ADDTO(CPPFLAGS, [-DMPE -D_POSIX_SOURCE -D_SOCKET_SOURCE])
	APR_ADDTO(LIBS, [-lsvipc -lcurses])
	APR_ADDTO(LDFLAGS, [-Xlinker \"-WL,cap=ia,ba,ph;nmstack=1024000\"])
	;;
    *-apple-aux3*)
	APR_ADDTO(CPPFLAGS, [-DAUX3 -D_POSIX_SOURCE])
	APR_ADDTO(LIBS, [-lposix -lbsd])
	APR_ADDTO(LDFLAGS, [-s])
	APR_SETVAR(SHELL, [/bin/ksh])
	;;
    *-ibm-aix*)
	APR_ADDTO(CPPFLAGS, [-U__STR__ -D_THREAD_SAFE])
        dnl _USR_IRS gets us the hstrerror() proto in netdb.h
        case $host in
            *-ibm-aix4.3)
	        APR_ADDTO(CPPFLAGS, [-D_USE_IRS])
	        ;;
            *-ibm-aix5*)
	        APR_ADDTO(CPPFLAGS, [-D_USE_IRS])
	        ;;
            *-ibm-aix4.3.*)
                APR_ADDTO(CPPFLAGS, [-D_USE_IRS])
                ;;
        esac
        dnl If using xlc, remember it, and give it the right options.
        if $CC 2>&1 | grep 'xlc' > /dev/null; then
          APR_SETIFNULL(AIX_XLC, [yes])
          APR_ADDTO(CFLAGS, [-qHALT=E])
        fi
	APR_SETIFNULL(apr_iconv_inbuf_const, [1])
	APR_SETIFNULL(apr_sysvsem_is_global, [yes])
	APR_ADDTO(LDFLAGS, [-Wl,-brtl])
        ;;
    *-apollo-*)
	APR_ADDTO(CPPFLAGS, [-DAPOLLO])
	;;
    *-dg-dgux*)
	APR_ADDTO(CPPFLAGS, [-DDGUX])
	;;
    *os2_emx*)
	APR_SETVAR(SHELL, [sh])
	APR_SETIFNULL(apr_gethostbyname_is_thread_safe, [yes])
	APR_SETIFNULL(apr_gethostbyaddr_is_thread_safe, [yes])
	;;
    *-hi-hiux)
	APR_ADDTO(CPPFLAGS, [-DHIUX])
	;;
    *-hp-hpux11.*)
	APR_ADDTO(CPPFLAGS, [-DHPUX11 -D_REENTRANT -D_XOPEN_SOURCE_EXTENDED])
	;;
    *-hp-hpux10.*)
 	case $host in
 	  *-hp-hpux10.01)
dnl	       # We know this is a problem in 10.01.
dnl	       # Not a problem in 10.20.  Otherwise, who knows?
	       APR_ADDTO(CPPFLAGS, [-DSELECT_NEEDS_CAST])
	       ;;	     
 	esac
	APR_ADDTO(CPPFLAGS, [-D_REENTRANT])
	;;
    *-hp-hpux*)
	APR_ADDTO(CPPFLAGS, [-DHPUX -D_REENTRANT])
	;;
    *-linux-*)
        case `uname -r` in
	    2.* )  APR_ADDTO(CPPFLAGS, [-DLINUX=2])
	           ;;
	    1.* )  APR_ADDTO(CPPFLAGS, [-DLINUX=1])
	           ;;
	    * )
	           ;;
        esac
	APR_ADDTO(CPPFLAGS, [-D_REENTRANT])
	;;
    *-GNU*)
	APR_ADDTO(CPPFLAGS, [-DHURD])
	;;
    *-lynx-lynxos)
	APR_ADDTO(CPPFLAGS, [-D__NO_INCLUDE_WARN__ -DLYNXOS])
	APR_ADDTO(LIBS, [-lbsd])
	;;
    *486-*-bsdi*)
	APR_ADDTO(CFLAGS, [-m486])
	;;
    *-*-bsdi*)
        case $host in
            *bsdi4.1)
                APR_ADDTO(CFLAGS, [-D_REENTRANT])
                ;;
        esac
        ;;
    *-openbsd*)
	APR_ADDTO(CPPFLAGS, [-D_POSIX_THREADS])
	;;
    *-netbsd*)
	APR_ADDTO(CPPFLAGS, [-DNETBSD])
	;;
    *-freebsd*)
	case $host in
	    *freebsd[2345]*)
		APR_ADDTO(CFLAGS, [-funsigned-char])
		;;
	esac
	APR_SETIFNULL(enable_threads, [no])
        APR_SETIFNULL(apr_lock_method, [USE_FLOCK_SERIALIZE])
	APR_ADDTO(CPPFLAGS, [-D_REENTRANT -D_THREAD_SAFE])
	;;
    *-next-nextstep*)
	APR_SETIFNULL(CFLAGS, [-O])
	APR_ADDTO(CPPFLAGS, [-DNEXT])
	;;
    *-next-openstep*)
	APR_SETIFNULL(CFLAGS, [-O])
	APR_ADDTO(CPPFLAGS, [-DNEXT])
	;;
    *-apple-rhapsody*)
	APR_ADDTO(CPPFLAGS, [-DRHAPSODY])
	;;
    *-apple-darwin*)
	APR_ADDTO(CPPFLAGS, [-DDARWIN -DSIGPROCMASK_SETS_THREAD_MASK -no-cpp-precomp])
	APR_SETIFNULL(apr_posixsem_is_global, [yes])
	;;
    *-dec-osf*)
	APR_ADDTO(CPPFLAGS, [-DOSF1])
	;;
    *-nto-qnx*)
	;;
    *-qnx)
	APR_ADDTO(CPPFLAGS, [-DQNX])
	APR_ADDTO(LIBS, [-N128k -lunix])
	;;
    *-qnx32)
	APR_ADDTO(CPPFLAGS, [-DQNX])
	APR_ADDTO(CFLAGS, [-mf -3])
	APR_ADDTO(LIBS, [-N128k -lunix])
	;;
    *-isc4*)
	APR_ADDTO(CPPFLAGS, [-posix -DISC])
	APR_ADDTO(LDFLAGS, [-posix])
	APR_ADDTO(LIBS, [-linet])
	;;
    *-sco3.2v[234]*)
	APR_ADDTO(CPPFLAGS, [-DSCO -D_REENTRANT])
	if test "$GCC" = "no"; then
	    APR_ADDTO(CFLAGS, [-Oacgiltz])
	fi
	APR_ADDTO(LIBS, [-lPW -lmalloc])
	;;
    *-sco3.2v5*)
	APR_ADDTO(CPPFLAGS, [-DSCO5 -D_REENTRANT])
	APR_ADDTO(LIBS, [-lprot])
	;;
    *-sco_sv*|*-SCO_SV*)
	APR_ADDTO(CPPFLAGS, [-DSCO -D_REENTRANT])
	APR_ADDTO(LIBS, [-lPW -lmalloc])
	;;
    *-solaris2*)
    	PLATOSVERS=`echo $host | sed 's/^.*solaris2.//'`
	APR_ADDTO(CPPFLAGS, [-DSOLARIS2=$PLATOSVERS -D_POSIX_PTHREAD_SEMANTICS -D_REENTRANT])
	APR_SETIFNULL(apr_iconv_inbuf_const, [1])
	;;
    *-sunos4*)
	APR_ADDTO(CPPFLAGS, [-DSUNOS4])
	;;
    *-unixware1)
	APR_ADDTO(CPPFLAGS, [-DUW=100])
	;;
    *-unixware2)
	APR_ADDTO(CPPFLAGS, [-DUW=200])
	APR_ADDTO(LIBS, [-lgen])
	;;
    *-unixware211)
	APR_ADDTO(CPPFLAGS, [-DUW=211])
	APR_ADDTO(LIBS, [-lgen])
	;;
    *-unixware212)
	APR_ADDTO(CPPFLAGS, [-DUW=212])
	APR_ADDTO(LIBS, [-lgen])
	;;
    *-unixware7)
	APR_ADDTO(CPPFLAGS, [-DUW=700])
	APR_ADDTO(LIBS, [-lgen])
	;;
    maxion-*-sysv4*)
	APR_ADDTO(CPPFLAGS, [-DSVR4])
	APR_ADDTO(LIBS, [-lc -lgen])
	;;
    *-*-powermax*)
	APR_ADDTO(CPPFLAGS, [-DSVR4])
	APR_ADDTO(LIBS, [-lgen])
	;;
    TPF)
       APR_ADDTO(CPPFLAGS, [-DTPF -D_POSIX_SOURCE])
       ;;
    bs2000*-siemens-sysv*)
	APR_SETIFNULL(CFLAGS, [-O])
	APR_ADDTO(CPPFLAGS, [-DSVR4 -D_XPG_IV -D_KMEMUSER])
	APR_ADDTO(LIBS, [-lsocket])
	APR_SETIFNULL(enable_threads, [no])
	;;
    *-siemens-sysv4*)
	APR_ADDTO(CPPFLAGS, [-DSVR4 -D_XPG_IV -DHAS_DLFCN -DUSE_MMAP_FILES -DUSE_SYSVSEM_SERIALIZED_ACCEPT])
	APR_ADDTO(LIBS, [-lc])
	;;
    pyramid-pyramid-svr4)
	APR_ADDTO(CPPFLAGS, [-DSVR4 -DNO_LONG_DOUBLE])
	APR_ADDTO(LIBS, [-lc])
	;;
    DS/90\ 7000-*-sysv4*)
	APR_ADDTO(CPPFLAGS, [-DUXPDS])
	;;
    *-tandem-sysv4*)
	APR_ADDTO(CPPFLAGS, [-DSVR4])
	;;
    *-ncr-sysv4)
	APR_ADDTO(CPPFLAGS, [-DSVR4 -DMPRAS])
	APR_ADDTO(LIBS, [-lc -L/usr/ucblib -lucb])
	;;
    *-sysv4*)
	APR_ADDTO(CPPFLAGS, [-DSVR4])
	APR_ADDTO(LIBS, [-lc])
	;;
    88k-encore-sysv4)
	APR_ADDTO(CPPFLAGS, [-DSVR4 -DENCORE])
	APR_ADDTO(LIBS, [-lPW])
	;;
    *-uts*)
	PLATOSVERS=`echo $host | sed 's/^.*,//'`
	case $PLATOSVERS in
	    2*) APR_ADDTO(CPPFLAGS, [-DUTS21])
	        APR_ADDTO(CFLAGS, [-Xa -eft])
	        APR_ADDTO(LIBS, [-lbsd -la])
	        ;;
	    *)  APR_ADDTO(CPPFLAGS, [-DSVR4])
	        APR_ADDTO(CFLAGS, [-Xa])
	        ;;
	esac
	;;
    *-ultrix)
	APR_ADDTO(CPPFLAGS, [-DULTRIX])
	APR_SETVAR(SHELL, [/bin/sh5])
	;;
    *powerpc-tenon-machten*)
	APR_ADDTO(LDFLAGS, [-Xlstack=0x14000 -Xldelcsect])
	;;
    *-machten*)
	APR_ADDTO(LDFLAGS, [-stack 0x14000])
	;;
    *convex-v11*)
	APR_ADDTO(CPPFLAGS, [-DCONVEXOS11])
	APR_SETIFNULL(CFLAGS, [-O1])
	APR_ADDTO(CFLAGS, [-ext])
	;;
    i860-intel-osf1)
	APR_ADDTO(CPPFLAGS, [-DPARAGON])
	;;
    *-sequent-ptx2.*.*)
	APR_ADDTO(CPPFLAGS, [-DSEQUENT=20])
	APR_ADDTO(CFLAGS, [-Wc,-pw])
	APR_ADDTO(LIBS, [-linet -lc -lseq])
	;;
    *-sequent-ptx4.0.*)
	APR_ADDTO(CPPFLAGS, [-DSEQUENT=40])
	APR_ADDTO(CFLAGS, [-Wc,-pw])
	APR_ADDTO(LIBS, [-linet -lc])
	;;
    *-sequent-ptx4.[123].*)
	APR_ADDTO(CPPFLAGS, [-DSEQUENT=41])
	APR_ADDTO(CFLAGS, [-Wc,-pw])
	APR_ADDTO(LIBS, [-lc])
	;;
    *-sequent-ptx4.4.*)
	APR_ADDTO(CPPFLAGS, [-DSEQUENT=44])
	APR_ADDTO(CFLAGS, [-Wc,-pw])
	APR_ADDTO(LIBS, [-lc])
	;;
    *-sequent-ptx4.5.*)
	APR_ADDTO(CPPFLAGS, [-DSEQUENT=45])
	APR_ADDTO(CFLAGS, [-Wc,-pw])
	APR_ADDTO(LIBS, [-lc])
	;;
    *-sequent-ptx5.0.*)
	APR_ADDTO(CPPFLAGS, [-DSEQUENT=50])
	APR_ADDTO(CFLAGS, [-Wc,-pw])
	APR_ADDTO(LIBS, [-lc])
	;;
    *NEWS-OS*)
	APR_ADDTO(CPPFLAGS, [-DNEWSOS])
	;;
    *-riscix)
	APR_ADDTO(CPPFLAGS, [-DRISCIX])
	APR_SETIFNULL(CFLAGS, [-O])
	;;
    *-irix*)
	APR_ADDTO(CPPFLAGS, [-D_POSIX_THREAD_SAFE_FUNCTIONS])
	;;
    *beos*)
        APR_ADDTO(CPPFLAGS, [-DBEOS])
        PLATOSVERS=`uname -r`
        APR_SETIFNULL(apr_process_lock_is_global, [yes])
        case $PLATOSVERS in
            5.0.4)
                APR_ADDTO(LDFLAGS, [-L/boot/develop/lib/x86 -L/boot/beos/system/lib -lbind -lsocket])
                APR_ADDTO(LIBS, [-lbind -lbe -lroot])
                APR_ADDTO(CPPFLAGS,[-DBONE7])
                ;;
            5.1)
                APR_ADDTO(LDFLAGS, [-L/boot/develop/lib/x86 -L/boot/beos/system/lib -lbind -lsocket])
                APR_ADDTO(LIBS, [-lbind -lbe -lroot])
                ;;
	esac
	APR_ADDTO(CPPFLAGS, [-DSIGPROCMASK_SETS_THREAD_MASK -DAP_AUTH_DBM_USE_APR])
        ;;
    4850-*.*)
	APR_ADDTO(CPPFLAGS, [-DSVR4 -DMPRAS])
	APR_ADDTO(LIBS, [-lc -L/usr/ucblib -lucb])
	;;
    drs6000*)
	APR_ADDTO(CPPFLAGS, [-DSVR4])
	APR_ADDTO(LIBS, [-lc -L/usr/ucblib -lucb])
	;;
    m88k-*-CX/SX|CYBER)
	APR_ADDTO(CPPFLAGS, [-D_CX_SX])
	APR_ADDTO(CFLAGS, [-Xa])
	;;
    *-tandem-oss)
	APR_ADDTO(CPPFLAGS, [-D_TANDEM_SOURCE -D_XOPEN_SOURCE_EXTENDED=1])
	;;
    *-ibm-os390)
       APR_SETIFNULL(apr_lock_method, [USE_SYSVSEM_SERIALIZE])
       APR_SETIFNULL(apr_sysvsem_is_global, [yes])
       APR_SETIFNULL(apr_gethostbyname_is_thread_safe, [yes])
       APR_SETIFNULL(apr_gethostbyaddr_is_thread_safe, [yes])
       APR_ADDTO(CPPFLAGS, [-U_NO_PROTO -DPTHREAD_ATTR_SETDETACHSTATE_ARG2_ADDR -DPTHREAD_SETS_ERRNO -DPTHREAD_DETACH_ARG1_ADDR -DSIGPROCMASK_SETS_THREAD_MASK -DTCP_NODELAY=1])
       ;;
    *-ibm-as400)
       APR_SETIFNULL(apr_lock_method, [USE_SYSVSEM_SERIALIZE])
       APR_SETIFNULL(apr_process_lock_is_global, [yes])
       APR_SETIFNULL(apr_gethostbyname_is_thread_safe, [yes])
       APR_SETIFNULL(apr_gethostbyaddr_is_thread_safe, [yes])
       ;;
    *cygwin*)
	APR_ADDTO(CPPFLAGS, [-DCYGWIN])
	APR_ADDTO(LIBS, [-lcrypt])
	;;
  esac

fi
])

dnl
dnl APR_CC_HINTS
dnl
dnl  Allows us to provide a default choice of compiler which
dnl  the user can override.
AC_DEFUN(APR_CC_HINTS, [
case "$host" in
  *-apple-aux3*)
      APR_SETIFNULL(CC, [gcc])
      ;;
  bs2000*-siemens-sysv*)
      APR_SETIFNULL(CC, [c89 -XLLML -XLLMK -XL -Kno_integer_overflow])
      ;;
  *convex-v11*)
      APR_SETIFNULL(CC, [cc])
      ;;
  *-ibm-os390)
      APR_SETIFNULL(CC, [cc])
      ;;
  *-ibm-as400)
      APR_SETIFNULL(CC, [icc])
      ;;
  *-isc4*)
      APR_SETIFNULL(CC, [gcc])
      ;;
  m88k-*-CX/SX|CYBER)
      APR_SETIFNULL(CC, [cc])
      ;;
  *-next-openstep*)
      APR_SETIFNULL(CC, [cc])
      ;;
  *-qnx32)
      APR_SETIFNULL(CC, [cc -F])
      ;;
  *-tandem-oss)
      APR_SETIFNULL(CC, [c89])
      ;;
  TPF)
      APR_SETIFNULL(CC, [c89])
      ;;
esac
])
