dnl
dnl APR_PRELOAD
dnl
dnl  Preload various ENV/makefile paramsm such as CC, CFLAGS, etc
dnl  based on outside knowledge
dnl
dnl  Generally, we force the setting of CC, and add flags
dnl  to CFLAGS, LIBS and LDFLAGS. 
dnl
AC_DEFUN(APR_PRELOAD, [
echo "Applying hints file rules for $host"

case "$host" in
    *mint)
	APR_ADDTO(CFLAGS, [-DMINT])
	APR_ADDTO(LIBS, [-lportlib -lsocket])
	;;
    *MPE/iX*)
	APR_ADDTO(CFLAGS, [-DMPE -D_POSIX_SOURCE -D_SOCKET_SOURCE])
	APR_ADDTO(LIBS, [-lsocket -lsvipc -lcurses])
	APR_ADDTO(LDFLAGS, [-Xlinker \"-WL,cap=ia,ba,ph;nmstack=1024000\"])
	APR_SETVAR(CAT, [/bin/cat])
	;;
    *-apple-aux3*)
        APR_SETVAR(CC, [gcc])
	APR_ADDTO(CFLAGS, [-DAUX3 -D_POSIX_SOURCE])
	APR_ADDTO(LIBS, [-lposix -lbsd])
	APR_ADDTO(LDFLAGS, [-s])
	APR_SETVAR(SHELL, [/bin/ksh])
	;;
    *-ibm-aix*)
        case $host in
        i386-ibm-aix*)
	    APR_ADDTO(CFLAGS, [-U__STR__ -DUSEBCOPY])
	    ;;
        *-ibm-aix[1-2].*)
	    APR_ADDTO(CFLAGS, [-DNEED_RLIM_T -U__STR__])
	    ;;
        *-ibm-aix3.*)
	    APR_ADDTO(CFLAGS, [-DNEED_RLIM_T -U__STR__])
	    ;;
        *-ibm-aix4.1)
	    APR_ADDTO(CFLAGS, [-DNEED_RLIM_T -U__STR__])
	    ;;
        *-ibm-aix4.1.*)
            APR_ADDTO(CFLAGS, [-DNEED_RLIM_T -U__STR__])
            ;;
        *-ibm-aix4.2)
	    APR_ADDTO(CFLAGS, [-U__STR__])
	    APR_ADDTO(LDFLAGS, [-lm])
	    ;;
        *-ibm-aix4.2.*)
            APR_ADDTO(CFLAGS, [-U__STR__])
            APR_ADDTO(LDFLAGS, [-lm])
            ;;
        *-ibm-aix4.3)
	    APR_ADDTO(CFLAGS, [-D_USE_IRS -U__STR__])
	    APR_ADDTO(LDFLAGS, [-lm])
	    ;;
        *-ibm-aix4.3.*)
            APR_ADDTO(CFLAGS, [-D_USE_IRS -U__STR__])
            APR_ADDTO(LDFLAGS, [-lm])
            ;;
        *-ibm-aix*)
	    APR_ADDTO(CFLAGS, [-U__STR__])
	    APR_ADDTO(LDFLAGS, [-lm])
	    ;;
        esac
        dnl Must do a check for gcc or egcs here, to get the right options  
        dnl to the compiler.
	AC_PROG_CC
        if test "$GCC" != "yes"; then
          APR_ADDTO(CFLAGS, [-qHALT=E])
          APR_ADDTO(CFLAGS, [-qLANGLVL=extended])
        fi
        ;;
    *-apollo-*)
	APR_ADDTO(CFLAGS, [-DAPOLLO])
	;;
    *-dg-dgux*)
	APR_ADDTO(CFLAGS, [-DDGUX])
	;;
    *os2_emx*)
	APR_SETVAR(SHELL, [sh])
        APR_SETIFNULL(file_as_socket, [0])
	;;
    *-hi-hiux)
	APR_ADDTO(CFLAGS, [-DHIUX])
	;;
    *-hp-hpux11.*)
	APR_ADDTO(CFLAGS, [-DHPUX11])
	APR_ADDTO(LIBS, [-lm -lpthread])
	;;
    *-hp-hpux10.*)
 	case $host in
 	  *-hp-hpux10.01)
dnl	       # We know this is a problem in 10.01.
dnl	       # Not a problem in 10.20.  Otherwise, who knows?
	       APR_ADDTO(CFLAGS, [-DSELECT_NEEDS_CAST])
	       ;;	     
 	esac
	;;
    *-hp-hpux*)
	APR_ADDTO(CFLAGS, [-DHPUX])
	APR_ADDTO(LIBS, [-lm])
	;;
    *-linux-*)
        case `uname -r` in
	    2.2* ) APR_ADDTO(CFLAGS, [-DLINUX=2])
	           APR_ADDTO(LIBS, [-lm])
	           ;;
	    2.0* ) APR_ADDTO(CFLAGS, [-DLINUX=2])
	           APR_ADDTO(LIBS, [-lm])
	           ;;
	    1.* )  APR_ADDTO(CFLAGS, [-DLINUX=1])
	           ;;
	    * )
	           ;;
        esac
	;;
    *-GNU*)
	APR_ADDTO(CFLAGS, [-DHURD])
	APR_ADDTO(LIBS, [-lm -lcrypt])
	;;
    *-lynx-lynxos)
	APR_ADDTO(CFLAGS, [-D__NO_INCLUDE_WARN__ -DLYNXOS])
	APR_ADDTO(LIBS, [-lbsd -lcrypt])
	;;
    *486-*-bsdi*)
	APR_ADDTO(CFLAGS, [-m486])
	;;
    *-netbsd*)
	APR_ADDTO(CFLAGS, [-DNETBSD])
	APR_ADDTO(LIBS, [-lcrypt])
	;;
    *-freebsd*)
	case $host in
	    *freebsd[2345]*)
		APR_ADDTO(CFLAGS, [-funsigned-char])
		;;
	esac
	APR_ADDTO(LIBS, [-lcrypt])
	;;
    *-next-nextstep*)
	APR_SETIFNULL(OPTIM, [-O])
	APR_ADDTO(CFLAGS, [-DNEXT])
	;;
    *-next-openstep*)
	APR_SETVAR(CC, [cc])
	APR_SETIFNULL(OPTIM, [-O])
	APR_ADDTO(CFLAGS, [-DNEXT])
	;;
dnl    *-apple-rhapsody*)
dnl	APR_ADDTO(CFLAGS, [-DDARWIN -DMAC_OS_X_SERVER])
dnl	;;
    *-apple-darwin*)
	APR_ADDTO(CFLAGS, [-DDARWIN])
	;;
    *-dec-osf*)
	APR_ADDTO(CFLAGS, [-DOSF1])
	APR_ADDTO(LIBS, [-lm])
	;;
    *-qnx)
	APR_ADDTO(CFLAGS, [-DQNX])
	APR_ADDTO(LIBS, [-N128k -lsocket -lunix])
	;;
    *-qnx32)
        APR_SETVAR(CC, [cc -F])
	APR_ADDTO(CFLAGS, [-DQNX -mf -3])
	APR_ADDTO(LIBS, [-N128k -lsocket -lunix])
	;;
    *-isc4*)
	APR_SETVAR(CC, [gcc])
	APR_ADDTO(CFLAGS, [-posix -DISC])
	APR_ADDTO(LDFLAGS, [-posix])
	APR_ADDTO(LIBS, [-linet])
	;;
    *-sco3*)
	APR_ADDTO(CFLAGS, [-DSCO -Oacgiltz])
	APR_ADDTO(LIBS, [-lPW -lsocket -lmalloc -lcrypt_i])
	;;
    *-sco5*)
	APR_ADDTO(CFLAGS, [-DSCO5])
	APR_ADDTO(LIBS, [-lsocket -lmalloc -lprot -ltinfo -lx -lm])
	;;
    *-sco_sv*|*-SCO_SV*)
	APR_ADDTO(CFLAGS, [-DSCO])
	APR_ADDTO(LIBS, [-lPW -lsocket -lmalloc -lcrypt_i])
	;;
    *-solaris2*)
    	PLATOSVERS=`echo $host | sed 's/^.*solaris2.//'`
	APR_ADDTO(CFLAGS, [-DSOLARIS2=$PLATOSVERS])
	APR_ADDTO(LIBS, [-lsocket -lnsl])
	;;
    *-sunos4*)
	APR_ADDTO(CFLAGS, [-DSUNOS4 -DUSEBCOPY])
	;;
    *-unixware1)
	APR_ADDTO(CFLAGS, [-DUW=100])
	APR_ADDTO(LIBS, [-lsocket -lnsl -lcrypt])
	;;
    *-unixware2)
	APR_ADDTO(CFLAGS, [-DUW=200])
	APR_ADDTO(LIBS, [-lsocket -lnsl -lcrypt -lgen])
	;;
    *-unixware211)
	APR_ADDTO(CFLAGS, [-DUW=211])
	APR_ADDTO(LIBS, [-lsocket -lnsl -lcrypt -lgen])
	;;
    *-unixware212)
	APR_ADDTO(CFLAGS, [-DUW=212])
	APR_ADDTO(LIBS, [-lsocket -lnsl -lcrypt -lgen])
	;;
    *-unixware7)
	APR_ADDTO(CFLAGS, [-DUW=700])
	APR_ADDTO(LIBS, [-lsocket -lnsl -lcrypt -lgen])
	;;
    maxion-*-sysv4*)
	APR_ADDTO(CFLAGS, [-DSVR4])
	APR_ADDTO(LIBS, [-lsocket -lnsl -lc -lgen])
	;;
    *-*-powermax*)
	APR_ADDTO(CFLAGS, [-DSVR4])
	APR_ADDTO(LIBS, [-lsocket -lnsl -lgen])
	;;
    TPF)
       APR_SETVAR(CC, [c89])
       APR_ADDTO(CFLAGS, [-DTPF -D_POSIX_SOURCE])
       ;;
    BS2000*-siemens-sysv4*)
	APR_SETVAR(CC, [c89 -XLLML -XLLMK -XL -Kno_integer_overflow])
	APR_ADDTO(CFLAGS, [-DSVR4 -D_XPG_IV])
	;;
    *-siemens-sysv4*)
	APR_ADDTO(CFLAGS, [-DSVR4 -D_XPG_IV -DHAS_DLFCN -DUSE_MMAP_FILES -DUSE_SYSVSEM_SERIALIZED_ACCEPT -DNEED_UNION_SEMUN])
	APR_ADDTO(LIBS, [-lsocket -lnsl -lc])
	;;
    pyramid-pyramid-svr4)
	APR_ADDTO(CFLAGS, [-DSVR4 -DNO_LONG_DOUBLE])
	APR_ADDTO(LIBS, [-lsocket -lnsl -lc])
	;;
    DS/90\ 7000-*-sysv4*)
	APR_ADDTO(CFLAGS, [-DUXPDS])
	APR_ADDTO(LIBS, [-lsocket -lnsl])
	;;
    *-tandem-sysv4*)
	APR_ADDTO(CFLAGS, [-DSVR4])
	APR_ADDTO(LIBS, [-lsocket -lnsl])
	;;
    *-ncr-sysv4)
	APR_ADDTO(CFLAGS, [-DSVR4 -DMPRAS])
	APR_ADDTO(LIBS, [-lsocket -lnsl -lc -L/usr/ucblib -lucb])
	;;
    *-sysv4*)
	APR_ADDTO(CFLAGS, [-DSVR4])
	APR_ADDTO(LIBS, [-lsocket -lnsl -lc])
	;;
    88k-encore-sysv4)
	APR_ADDTO(CFLAGS, [-DSVR4 -DENCORE])
	APR_ADDTO(LIBS, [-lPW])
	;;
    *-uts*)
	PLATOSVERS=`echo $host | sed 's/^.*,//'`
	case $PLATOSVERS in
	    2*) APR_ADDTO(CFLAGS, [-Xa -eft -DUTS21 -DUSEBCOPY])
	        APR_ADDTO(LIBS, [-lsocket -lbsd -la])
	        ;;
	    *)  APR_ADDTO(CFLAGS, [-Xa -DSVR4])
	        APR_ADDTO(LIBS, [-lsocket -lnsl])
	        ;;
	esac
	;;
    *-ultrix)
	APR_ADDTO(CFLAGS, [-DULTRIX])
	APR_SETVAR(SHELL, [/bin/sh5])
	;;
    *powerpc-tenon-machten*)
	APR_ADDTO(LDFLAGS, [-Xlstack=0x14000 -Xldelcsect])
	;;
    *-machten*)
	APR_ADDTO(LDFLAGS, [-stack 0x14000])
	;;
    *convex-v11*)
	APR_ADDTO(CFLAGS, [-ext -DCONVEXOS11])
	APR_SETIFNULL(OPTIM, [-O1])
	APR_SETVAR(CC, [cc])
	;;
    i860-intel-osf1)
	APR_ADDTO(CFLAGS, [-DPARAGON])
	;;
    *-sequent-ptx2.*.*)
	APR_ADDTO(CFLAGS, [-DSEQUENT=20 -Wc,-pw])
	APR_ADDTO(LIBS, [-lsocket -linet -lnsl -lc -lseq])
	;;
    *-sequent-ptx4.0.*)
	APR_ADDTO(CFLAGS, [-DSEQUENT=40 -Wc,-pw])
	APR_ADDTO(LIBS, [-lsocket -linet -lnsl -lc])
	;;
    *-sequent-ptx4.[123].*)
	APR_ADDTO(CFLAGS, [-DSEQUENT=41 -Wc,-pw])
	APR_ADDTO(LIBS, [-lsocket -lnsl -lc])
	;;
    *-sequent-ptx4.4.*)
	APR_ADDTO(CFLAGS, [-DSEQUENT=44 -Wc,-pw])
	APR_ADDTO(LIBS, [-lsocket -lnsl -lc])
	;;
    *-sequent-ptx4.5.*)
	APR_ADDTO(CFLAGS, [-DSEQUENT=45 -Wc,-pw])
	APR_ADDTO(LIBS, [-lsocket -lnsl -lc])
	;;
    *-sequent-ptx5.0.*)
	APR_ADDTO(CFLAGS, [-DSEQUENT=50 -Wc,-pw])
	APR_ADDTO(LIBS, [-lsocket -lnsl -lc])
	;;
    *NEWS-OS*)
	APR_ADDTO(CFLAGS, [-DNEWSOS])
	;;
    *-riscix)
	APR_ADDTO(CFLAGS, [-DRISCIX])
	APR_SETIFNULL(OPTIM, [-O])
	APR_SETIFNULL(MAKE, [make])
	;;
    *beos*)
        APR_ADDTO(CFLAGS, [-DBEOS])
        PLATOSVERS=`uname -r`
        case $PLATOSVERS in
            5.1)
                APR_ADDTO(CPPFLAGS, [-I/boot/develop/headers/bone])
                APR_ADDTO(LDFLAGS, [-nodefaultlibs -L/boot/develop/lib/x86 -L/boot/beos/system/lib])
                APR_ADDTO(EXTRA_LIBS, [-lbind -lsocket -lbe -lroot])
                APR_SETIFNULL(file_as_socket, [0])
                ;;
            default)
                APR_SETIFNULL(file_as_socket, [0])
                ;;
	esac
	;;
    4850-*.*)
	APR_ADDTO(CFLAGS, [-DSVR4 -DMPRAS])
	APR_ADDTO(LIBS, [-lsocket -lnsl -lc -L/usr/ucblib -lucb])
	;;
    drs6000*)
	APR_ADDTO(CFLAGS, [-DSVR4])
	APR_ADDTO(LIBS, [-lsocket -lnsl -lc -L/usr/ucblib -lucb])
	;;
    m88k-*-CX/SX|CYBER)
	APR_ADDTO(CFLAGS, [-D_CX_SX -Xa])
	APR_SETVAR(CC, [cc])
	;;
    *-tandem-oss)
	APR_ADDTO(CFLAGS, [-D_TANDEM_SOURCE -D_XOPEN_SOURCE_EXTENDED=1])
	APR_SETVAR(CC, [c89])
	;;
    *-ibm-os390)
       APR_SETIFNULL(apr_lock_method, [USE_SYSVSEM_SERIALIZE])
       APR_SETIFNULL(apr_process_lock_is_global, [yes])
       APR_SETIFNULL(CC, [cc])
       APR_ADDTO(CFLAGS, [-U_NO_PROTO])
       APR_ADDTO(CFLAGS, [-DPTHREAD_ATTR_SETDETACHSTATE_ARG2_ADDR])
       APR_ADDTO(CFLAGS, [-DPTHREAD_SETS_ERRNO])
       APR_ADDTO(CFLAGS, [-DPTHREAD_DETACH_ARG1_ADDR])
       APR_ADDTO(CFLAGS, [-DSIGPROCMASK_SETS_THREAD_MASK])
       APR_ADDTO(CFLAGS, [-DTCP_NODELAY=1])
       ;;
esac
APR_DOEXTRA
])
