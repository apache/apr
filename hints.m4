dnl
dnl Apache and APR "hints" file
dnl  We preload various configure settings depending
dnl  on previously obtained platform knowledge.
dnl  We allow all settings to be overridden from
dnl  the command-line.
dnl
dnl  We maintain the "format" that we've used
dnl  under 1.3.x, so we don't exactly follow
dnl  what is "recommended" by autoconf.

dnl
dnl APR_DOEXTRA
dnl
dnl  Handle the use of EXTRA_* variables.
dnl  Basically, EXTRA_* vars are added to the
dnl  current settings of their "parents". We
dnl  can expand as needed. This is ugly
dnl
AC_DEFUN(APR_DOEXTRA, [
  for i in CFLAGS LDFLAGS LIBS
  do
    eval APR_TMP=\$EXTRA_$i
    if test -n "$APR_TMP"; then
      eval $i=\"\$$i $APR_TMP\"
      eval export $i
      eval unset EXTRA_${i}
      eval export EXTRA_${i}
    fi
  done
])

dnl
dnl APR_SETIFNULL(variable, value)
dnl
dnl  Set variable iff it's currently null
dnl
AC_DEFUN(APR_SETIFNULL,[
  if test -z "$$1"; then
    $1="$2"; export $1
  fi
])

dnl
dnl APR_SETVAR(variable, value)
dnl
dnl  Set variable no matter what
dnl
AC_DEFUN(APR_SETVAR,[
  $1="$2"; export $1
])

dnl
dnl APR_ADDTO(variable, value)
dnl
dnl  Add value to variable
dnl
AC_DEFUN(APR_ADDTO,[
   $1="$$1 $2"; export $1
])

dnl
dnl APR_PRELOAD
dnl
dnl  Preload various ENV/makefile paramsm such as CC, CFLAGS, etc
dnl  based on outside knowledge
dnl
AC_DEFUN(APR_PRELOAD, [
echo "Applying hints file rules for $host"

case "$host" in
    *mint)
	APR_SETIFNULL(CFLAGS, [-DMINT])
	APR_SETIFNULL(LIBS, [-lportlib -lsocket])
	;;
    *MPE/iX*)
	APR_SETIFNULL(CFLAGS, [-DMPE -D_POSIX_SOURCE -D_SOCKET_SOURCE])
	APR_SETIFNULL(LIBS, [-lsocket -lsvipc -lcurses])
	APR_SETIFNULL(LDFLAGS, [-Xlinker \"-WL,cap=ia,ba,ph;nmstack=1024000\"])
	APR_SETIFNULL(CAT, [/bin/cat])
	;;
    *-apple-aux3*)
	APR_SETIFNULL(CFLAGS, [-DAUX3 -D_POSIX_SOURCE])
	APR_SETIFNULL(LIBS, [-lposix -lbsd])
	APR_SETIFNULL(LDFLAGS, [-s])
	APR_SETVAR(APACHE_MPM, [prefork])
        APR_SETVAR(SINGLE_LISTEN_UNSERIALIZED_ACCEPT, [1])
	;;
    *-ibm-aix*)
        case $host in
        i386-ibm-aix*)
	    APR_SETIFNULL(CFLAGS, [-U__STR__ -DUSEBCOPY])
	    ;;
        *-ibm-aix[1-2].*)
	    APR_SETIFNULL(CFLAGS, [-DNEED_RLIM_T -U__STR__])
	    ;;
        *-ibm-aix3.*)
	    APR_SETIFNULL(CFLAGS, [-DNEED_RLIM_T -U__STR__])
	    ;;
        *-ibm-aix4.1)
	    APR_SETIFNULL(CFLAGS, [-DNEED_RLIM_T -U__STR__])
	    ;;
        *-ibm-aix4.1.*)
            APR_SETIFNULL(CFLAGS, [-DNEED_RLIM_T -U__STR__])
            ;;
        *-ibm-aix4.2)
	    APR_SETIFNULL(CFLAGS, [-U__STR__])
	    APR_SETIFNULL(LDFLAGS, [-lm])
	    ;;
        *-ibm-aix4.2.*)
            APR_SETIFNULL(CFLAGS, [-U__STR__])
            APR_SETIFNULL(LDFLAGS, [-lm])
            ;;
        *-ibm-aix4.3)
	    APR_SETIFNULL(CFLAGS, [-D_USE_IRS -U__STR__])
	    APR_SETIFNULL(LDFLAGS, [-lm])
	    ;;
        *-ibm-aix4.3.*)
            APR_SETIFNULL(CFLAGS, [-D_USE_IRS -U__STR__])
            APR_SETIFNULL(LDFLAGS, [-lm])
            ;;
        *-ibm-aix*)
	    APR_SETIFNULL(CFLAGS, [-U__STR__])
	    APR_SETIFNULL(LDFLAGS, [-lm])
	    ;;
        esac
        dnl Must do a check for gcc or egcs here, to get the right options  
        dnl to the compiler.
	AC_PROG_CC
        if test "$GCC" != "yes"; then
          APR_ADDTO(CFLAGS, [-qHALT=E])
        fi
        ;;
    *-apollo-*)
	APR_SETIFNULL(CFLAGS, [-DAPOLLO])
	;;
    *-dg-dgux*)
	APR_SETIFNULL(CFLAGS, [-DDGUX])
	;;
    *os2_emx*)
	APR_SETIFNULL(SHELL, [sh])
        APR_SETIFNULL(file_as_socket, [0])
        APR_SETVAR(APACHE_MPM, [spmt_os2])
        APR_SETVAR(SINGLE_LISTEN_UNSERIALIZED_ACCEPT, [1])
	;;
    *-hi-hiux)
	APR_SETIFNULL(CFLAGS, [-DHIUX])
	;;
    *-hp-hpux11.*)
	APR_SETIFNULL(CFLAGS, [-DHPUX11])
	APR_SETIFNULL(LIBS, [-lm -lpthread])
	;;
    *-hp-hpux10.*)
	APR_SETIFNULL(CFLAGS, [-DHPUX10])
 	case $host in
 	  *-hp-hpux10.01)
dnl	       # We know this is a problem in 10.01.
dnl	       # Not a problem in 10.20.  Otherwise, who knows?
	       APR_ADDTO(CFLAGS, [-DSELECT_NEEDS_CAST])
	       ;;	     
 	esac
	;;
    *-hp-hpux*)
	APR_SETIFNULL(CFLAGS, [-DHPUX])
	APR_SETIFNULL(LIBS, [-lm])
	;;
    *-linux2)
        ver=`uname -r`
	APR_SETIFNULL(CFLAGS, [-DLINUX=2])
	APR_SETIFNULL(LIBS, [-lm])
	;;
    *-GNU*)
	APR_SETIFNULL(CFLAGS, [-DHURD])
	APR_SETIFNULL(LIBS, [-lm -lcrypt])
	;;
    *-linux1)
	APR_SETIFNULL(CFLAGS, [-DLINUX=1])
	;;
    *-lynx-lynxos)
	APR_SETIFNULL(CFLAGS, [-D__NO_INCLUDE_WARN__ -DLYNXOS])
	APR_SETIFNULL(LIBS, [-lbsd -lcrypt])
	;;
    *486-*-bsdi*)
	APR_SETIFNULL(CFLAGS, [-m486])
        APR_SETVAR(SINGLE_LISTEN_UNSERIALIZED_ACCEPT, [1])
	;;
    *-netbsd*)
	APR_SETIFNULL(CFLAGS, [-DNETBSD])
	APR_SETIFNULL(LIBS, [-lcrypt])
        APR_SETVAR(SINGLE_LISTEN_UNSERIALIZED_ACCEPT, [1])
	;;
    *-freebsd*)
	case $host in
	    *freebsd[2345]*)
		APR_SETIFNULL(CFLAGS, [-funsigned-char])
		;;
	esac
	APR_SETIFNULL(LIBS, [-lcrypt])
        APR_SETVAR(SINGLE_LISTEN_UNSERIALIZED_ACCEPT, [1])
	;;
    *-next-nextstep*)
	APR_SETIFNULL(OPTIM, [-O])
	APR_SETIFNULL(CFLAGS, [-DNEXT])
	;;
    *-next-openstep*)
	APR_SETIFNULL(CC, [cc])
	APR_SETIFNULL(OPTIM, [-O])
	APR_SETIFNULL(CFLAGS, [-DNEXT])
	;;
dnl    *-apple-rhapsody*)
dnl	APR_SETIFNULL(CFLAGS, [-DDARWIN -DMAC_OS_X_SERVER])
dnl     APR_SETVAR(SINGLE_LISTEN_UNSERIALIZED_ACCEPT, [1])
dnl	;;
    *-apple-darwin*)
	APR_SETIFNULL(CFLAGS, [-DDARWIN])
        APR_SETVAR(SINGLE_LISTEN_UNSERIALIZED_ACCEPT, [1])
	;;
    *-dec-osf*)
	APR_SETIFNULL(CFLAGS, [-DOSF1])
	APR_SETIFNULL(LIBS, [-lm])
        APR_SETVAR(SINGLE_LISTEN_UNSERIALIZED_ACCEPT, [1])
	;;
    *-qnx)
	APR_SETIFNULL(CFLAGS, [-DQNX])
	APR_SETIFNULL(LIBS, [-N128k -lsocket -lunix])
        APR_SETVAR(SINGLE_LISTEN_UNSERIALIZED_ACCEPT, [1])
	;;
    *-qnx32)
        APR_SETIFNULL(CC, [cc -F])
	APR_SETIFNULL(CFLAGS, [-DQNX -mf -3])
	APR_SETIFNULL(LIBS, [-N128k -lsocket -lunix])
	;;
    *-isc4*)
	APR_SETIFNULL(CC, [gcc])
	APR_SETIFNULL(CFLAGS, [-posix -DISC])
	APR_SETIFNULL(LDFLAGS, [-posix])
	APR_SETIFNULL(LIBS, [-linet])
	;;
    *-sco3*)
	APR_SETIFNULL(CFLAGS, [-DSCO -Oacgiltz])
	APR_SETIFNULL(LIBS, [-lPW -lsocket -lmalloc -lcrypt_i])
	;;
    *-sco5*)
	APR_SETIFNULL(CFLAGS, [-DSCO5])
	APR_SETIFNULL(LIBS, [-lsocket -lmalloc -lprot -ltinfo -lx -lm])
	;;
    *-sco_sv*|*-SCO_SV*)
	APR_SETIFNULL(CFLAGS, [-DSCO])
	APR_SETIFNULL(LIBS, [-lPW -lsocket -lmalloc -lcrypt_i])
	;;
    *-solaris2*)
    	PLATOSVERS=`echo $host | sed 's/^.*solaris2.//'`
	APR_SETIFNULL(CFLAGS, [-DSOLARIS2=$PLATOSVERS])
	APR_SETIFNULL(LIBS, [-lsocket -lnsl])
	;;
    *-sunos4*)
	APR_SETIFNULL(CFLAGS, [-DSUNOS4 -DUSEBCOPY])
	;;
    *-unixware1)
	APR_SETIFNULL(CFLAGS, [-DUW=100])
	APR_SETIFNULL(LIBS, [-lsocket -lnsl -lcrypt])
	;;
    *-unixware2)
	APR_SETIFNULL(CFLAGS, [-DUW=200])
	APR_SETIFNULL(LIBS, [-lsocket -lnsl -lcrypt -lgen])
	;;
    *-unixware211)
	APR_SETIFNULL(CFLAGS, [-DUW=211])
	APR_SETIFNULL(LIBS, [-lsocket -lnsl -lcrypt -lgen])
	;;
    *-unixware212)
	APR_SETIFNULL(CFLAGS, [-DUW=212])
	APR_SETIFNULL(LIBS, [-lsocket -lnsl -lcrypt -lgen])
	;;
    *-unixware7)
	APR_SETIFNULL(CFLAGS, [-DUW=700])
	APR_SETIFNULL(LIBS, [-lsocket -lnsl -lcrypt -lgen])
	;;
    maxion-*-sysv4*)
	APR_SETIFNULL(CFLAGS, [-DSVR4])
	APR_SETIFNULL(LIBS, [-lsocket -lnsl -lc -lgen])
	;;
    *-*-powermax*)
	APR_SETIFNULL(CFLAGS, [-DSVR4])
	APR_SETIFNULL(LIBS, [-lsocket -lnsl -lgen])
	;;
    TPF)
       APR_SETIFNULL(CC, [c89])
       APR_SETIFNULL(CFLAGS, [-DTPF -DCHARSET_EBCDIC -D_POSIX_SOURCE])
       ;;
    BS2000*-siemens-sysv4*)
	APR_SETIFNULL(CC, [c89 -XLLML -XLLMK -XL -Kno_integer_overflow])
	APR_SETIFNULL(CFLAGS, [-DCHARSET_EBCDIC -DSVR4 -D_XPG_IV])
	;;
    *-siemens-sysv4*)
	APR_SETIFNULL(CFLAGS, [-DSVR4 -D_XPG_IV -DHAS_DLFCN -DUSE_MMAP_FILES -DUSE_SYSVSEM_SERIALIZED_ACCEPT -DNEED_UNION_SEMUN])
	APR_SETIFNULL(LIBS, [-lsocket -lnsl -lc])
	;;
    pyramid-pyramid-svr4)
	APR_SETIFNULL(CFLAGS, [-DSVR4 -DNO_LONG_DOUBLE])
	APR_SETIFNULL(LIBS, [-lsocket -lnsl -lc])
	;;
    DS/90\ 7000-*-sysv4*)
	APR_SETIFNULL(CFLAGS, [-DUXPDS])
	APR_SETIFNULL(LIBS, [-lsocket -lnsl])
	;;
    *-tandem-sysv4*)
	APR_SETIFNULL(CFLAGS, [-DSVR4])
	APR_SETIFNULL(LIBS, [-lsocket -lnsl])
	;;
    *-ncr-sysv4)
	APR_SETIFNULL(CFLAGS, [-DSVR4 -DMPRAS])
	APR_SETIFNULL(LIBS, [-lsocket -lnsl -lc -L/usr/ucblib -lucb])
	;;
    *-sysv4*)
	APR_SETIFNULL(CFLAGS, [-DSVR4])
	APR_SETIFNULL(LIBS, [-lsocket -lnsl -lc])
	;;
    88k-encore-sysv4)
	APR_SETIFNULL(CFLAGS, [-DSVR4 -DENCORE])
	APR_SETIFNULL(LIBS, [-lPW])
	;;
    *-uts*)
	PLATOSVERS=`echo $host | sed 's/^.*,//'`
	case $PLATOSVERS in
	    2*) APR_SETIFNULL(CFLAGS, [-Xa -eft -DUTS21 -DUSEBCOPY])
	        APR_SETIFNULL(LIBS, [-lsocket -lbsd -la])
	        ;;
	    *)  APR_SETIFNULL(CFLAGS, [-Xa -DSVR4])
	        APR_SETIFNULL(LIBS, [-lsocket -lnsl])
	        ;;
	esac
	;;
    *-ultrix)
	APR_SETIFNULL(CFLAGS, [-DULTRIX])
	APR_SETIFNULL(SHELL, [/bin/sh5])
	;;
    *powerpc-tenon-machten*)
	APR_SETIFNULL(LDFLAGS, [-Xlstack=0x14000 -Xldelcsect])
	;;
    *-machten*)
	APR_SETIFNULL(LDFLAGS, [-stack 0x14000])
	;;
    *convex-v11*)
	APR_SETIFNULL(CFLAGS, [-ext -DCONVEXOS11])
	APR_SETIFNULL(OPTIM, [-O1])
	APR_SETIFNULL(CC, [cc])
	;;
    i860-intel-osf1)
	APR_SETIFNULL(CFLAGS, [-DPARAGON])
	;;
    *-sequent-ptx2.*.*)
	APR_SETIFNULL(CFLAGS, [-DSEQUENT=20 -Wc,-pw])
	APR_SETIFNULL(LIBS, [-lsocket -linet -lnsl -lc -lseq])
	;;
    *-sequent-ptx4.0.*)
	APR_SETIFNULL(CFLAGS, [-DSEQUENT=40 -Wc,-pw])
	APR_SETIFNULL(LIBS, [-lsocket -linet -lnsl -lc])
	;;
    *-sequent-ptx4.[123].*)
	APR_SETIFNULL(CFLAGS, [-DSEQUENT=41 -Wc,-pw])
	APR_SETIFNULL(LIBS, [-lsocket -lnsl -lc])
	;;
    *-sequent-ptx4.4.*)
	APR_SETIFNULL(CFLAGS, [-DSEQUENT=44 -Wc,-pw])
	APR_SETIFNULL(LIBS, [-lsocket -lnsl -lc])
	;;
    *-sequent-ptx4.5.*)
	APR_SETIFNULL(CFLAGS, [-DSEQUENT=45 -Wc,-pw])
	APR_SETIFNULL(LIBS, [-lsocket -lnsl -lc])
	;;
    *-sequent-ptx5.0.*)
	APR_SETIFNULL(CFLAGS, [-DSEQUENT=50 -Wc,-pw])
	APR_SETIFNULL(LIBS, [-lsocket -lnsl -lc])
	;;
    *NEWS-OS*)
	APR_SETIFNULL(CFLAGS, [-DNEWSOS])
	;;
    *-riscix)
	APR_SETIFNULL(CFLAGS, [-DRISCIX])
	APR_SETIFNULL(OPTIM, [-O])
	APR_SETIFNULL(MAKE, [make])
	;;
    *beos*)
        APR_SETIFNULL(CFLAGS, [-DBEOS])
        APR_SETVAR(APACHE_MPM, [beos])
        PLATOSVERS=`uname -r`
        case $PLATOSVERS in
            5.1)
                APR_ADDTO(CPPFLAGS, [-I/boot/develop/headers/bone])
                APR_ADDTO(LDFLAGS, [-nodefaultlibs -L/boot/develop/lib/x86 -L/boot/beos/system/lib])
                APR_SETIFNULL(EXTRA_LIBS, [-lbind -lsocket -lbe -lroot])
                APR_SETIFNULL(file_as_socket, [1])
                ;;
            default)
                APR_SETIFNULL(file_as_socket, [0])
                ;;
	esac
	;;
    4850-*.*)
	APR_SETIFNULL(CFLAGS, [-DSVR4 -DMPRAS])
	APR_SETIFNULL(LIBS, [-lsocket -lnsl -lc -L/usr/ucblib -lucb])
	;;
    drs6000*)
	APR_SETIFNULL(CFLAGS, [-DSVR4])
	APR_SETIFNULL(LIBS, [-lsocket -lnsl -lc -L/usr/ucblib -lucb])
	;;
    m88k-*-CX/SX|CYBER)
	APR_SETIFNULL(CFLAGS, [-D_CX_SX -Xa])
	APR_SETIFNULL(CC, [cc])
	;;
    *-tandem-oss)
	APR_SETIFNULL(CFLAGS, [-D_TANDEM_SOURCE -D_XOPEN_SOURCE_EXTENDED=1])
	APR_SETIFNULL(CC, [c89])
	;;
    *-ibm-os390)
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
