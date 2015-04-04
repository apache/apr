#
#

from SCons.Environment import Environment
from os.path import join as pjoin
import aprconf
import re
import os
import traceback

_platforms = [ 
    'aix', 
    'beos', 
    'netware', 
    'os2', 
    'os390', 
    'unix', 
    'win32' 
]

_platform_dirs = [
    'atomic',
    'dso',
    'file_io',
    'helpers',
    'locks',
    'memory',
    'misc',
    'mmap',
    'network_io',
    'passwd',
    'poll',
    'random',
    'shmem',
    'strings',
    'support',
    'tables',
    'threadproc',
    'time',
    'user'
]

_simple_dirs = [
    'buckets',
    'crypto',
    'dbd',
    'dbm',
    'dbm/sdbm',
    'encoding',
    'hooks',
    'memcache',
    'tables',
    'strings',
    'strmatch',
    'util-misc',
    'uri',
    'xlate',
    'xml'
]

class APREnv(Environment):
  def __init__(self, parent=None, args=None, **kw):
    Environment.__init__(self, ENV=os.environ,
                         tools = ['default', 'subst'],
                         toolpath = [pjoin(os.getcwd(), 'build')],
                          **kw)

    # See SCons/Platform/__init__.py for possible values
    if self['PLATFORM'] in _platforms:
      self['APR_PLATFORM'] = self['PLATFORM']
    else:
      self['APR_PLATFORM'] = 'unix'
      
    # if no *.c files are found in the original APR_PLATFORM, we switch to 
    # using this fallback platform.
    self['APR_FALLBACK_PLATFORM'] = 'unix'

    self.AppendUnique(CPPPATH = ['include', 'include/private', 'include/arch/'+self['APR_PLATFORM']])
    self.autoconf = aprconf.APRConfigure(self)
    self.AppendUnique(LIBS = ['expat'])

  def is_gcc(self):
    # TOOD: This detection should be smarter, need look at SCons Internals
    # for how it works/base it on the Tool selection.
    return self['CC'] == 'gcc'

  def core_lib_files(self):
    rv = []
    for d in _platform_dirs:
      p = pjoin(d, self['APR_PLATFORM'], '*.c')
      files = self.Glob(p)
      if not files and self['APR_PLATFORM'] != self['APR_FALLBACK_PLATFORM']:
        p = pjoin(d, self['APR_FALLBACK_PLATFORM'], '*.c')
        files = self.Glob(p)
      rv.extend(files)

    for d in _simple_dirs:
      p = pjoin(d, '*.c')
      files = self.Glob(p)
      rv.extend(files)
    return rv

  def APRVersion(self):
    if not self.has_key('APR_VERSION'):
      self['APR_VERSION'] = self.read_version('APR', '#include/apr_version.h')
    return self['APR_VERSION']

  def read_version(self, prefix, path):
    version_re = re.compile("(.*)%s_(?P<id>MAJOR|MINOR|PATCH)_VERSION(\s+)(?P<num>\d)(.*)" % prefix)
    versions = {}
    fp = self.File(path).get_contents()
    for line in fp.splitlines():
      m = version_re.match(line)
      if m:
        versions[m.group('id')] = int(m.group('num'))
    return (versions['MAJOR'], versions['MINOR'], versions['PATCH'])

  def Filter(self, **kw):
    for k in kw.keys():
      self[k] = [x for x in self[k] if x is not kw[k]]

  def APRHints(self):
    # TOOD: port more from apr_hints.m4
    if self['PLATFORM'] == 'darwin':
      self.AppendUnique(CPPFLAGS=['-DDARWIN', '-DSIGPROCMASK_SETS_THREAD_MASK'])


  def critical_value(self, f, value, *args):
    rv = f(*args)

    if rv != value:
      traceback.print_stack()
      print "Critial Test failed."
      self.Exit(1)
    return rv

  def critical(self, f, *args):
    rv = f(*args)

    if not rv:
      traceback.print_stack()
      print "Critial Test failed."
      self.Exit(1)

  def APRAutoconf(self):
    self.apr_config_h = 'include/arch/%s/apr_private.h' % (self['APR_PLATFORM'])
    self.apu_config_h = 'include/private/apu_config.h'
    subst = {}

    if self.GetOption('clean') or self.GetOption('help'):
      return self

    # TODO Port header detection here etc
    conf = self.Configure(custom_tests = {
        'CheckFile':
            self.autoconf.CheckFile,
        'CheckTypesCompatible': 
            self.autoconf.CheckTypesCompatible,
        'Check_apr_atomic_builtins': 
            self.autoconf.Check_apr_atomic_builtins,
        'Check_apr_largefile64': 
            self.autoconf.Check_apr_largefile64,
        'Check_apr_big_endian': 
            self.autoconf.Check_apr_big_endian,
        'Check_apr_mmap_mapping_dev_zero': 
            self.autoconf.Check_apr_mmap_mapping_dev_zero,
        'Check_apr_semaphores':
            self.autoconf.Check_apr_semaphores,
        'Check_apr_semun':
            self.autoconf.Check_apr_semun,
        'Check_apr_check_tcp_nodelay_inherited':
            self.autoconf.Check_apr_check_tcp_nodelay_inherited,
        'Check_apr_nonblock_inherited':
            self.autoconf.Check_apr_nonblock_inherited,
        'Check_apr_ebcdic':
            self.autoconf.Check_apr_ebcdic,
        'Check_apr_sctp':
            self.autoconf.Check_apr_sctp,
        },
        config_h = self.apr_config_h)

    # Do we have a working C Compiler?
    self.critical(conf.CheckCC)

    flag_headers = self.Split("""ByteOrder.h
    conio.h
    crypt.h
    ctype.h
    dir.h
    dirent.h
    dl.h
    dlfcn.h
    errno.h
    fcntl.h
    grp.h
    io.h
    limits.h
    mach-o/dyld.h
    malloc.h
    memory.h
    netdb.h
    osreldate.h
    poll.h
    process.h
    pwd.h
    semaphore.h
    signal.h
    stdarg.h
    stddef.h
    stdio.h
    stdlib.h
    string.h
    strings.h
    sysapi.h
    sysgtime.h
    termios.h
    time.h
    tpfeq.h
    tpfio.h
    unistd.h
    unix.h
    windows.h
    winsock2.h
    arpa/inet.h
    kernel/OS.h
    net/errno.h
    netinet/in.h
    netinet/sctp.h
    netinet/tcp.h
    netinet/sctp_uio.h
    sys/file.h
    sys/ioctl.h
    sys/mman.h
    sys/param.h        
    sys/poll.h
    sys/resource.h
    sys/select.h
    sys/sem.h
    sys/sendfile.h
    sys/signal.h
    sys/socket.h
    sys/sockio.h
    sys/stat.h
    sys/sysctl.h
    sys/syslimits.h
    sys/time.h
    sys/types.h
    sys/uio.h
    sys/un.h
    sys/wait.h
    pthread.h
    """)
    for x in flag_headers:
      s = x.replace('/', '_').replace('.', '').replace('-', '')
      if conf.CheckCHeader(x):
        subst['@%s@' % (s)] = 1
      else:
        subst['@%s@' % (s)] = 0
        
    sizeof_char = conf.CheckTypeSize('char')
    sizeof_int = self.critical_value(conf.CheckTypeSize, 4, 'int')
    subst['@int_value@'] = 'int'
    sizeof_long = conf.CheckTypeSize('long')
    sizeof_short = self.critical_value(conf.CheckTypeSize, 2, 'short')
    subst['@short_value@'] = 'short'
    sizeof_long_long = conf.CheckTypeSize('long long')
    sizeof_longlong = conf.CheckTypeSize('longlong')
    sizeof_pid_t = conf.CheckTypeSize('pid_t', includes='#include <sys/types.h>')
    sizeof_off_t = conf.CheckTypeSize('off_t', includes='#include <sys/types.h>')
    sizeof_size_t = conf.CheckTypeSize('size_t', includes='#include <sys/types.h>')
    sizeof_ssize_t = conf.CheckTypeSize('ssize_t', includes='#include <sys/types.h>')
    subst['@voidp_size@'] = conf.CheckTypeSize('void*')

    if sizeof_size_t:
      subst['@size_t_value@'] = 'size_t'
      subst['@size_t_fmt@'] = '#define APR_SIZE_T_FMT "lu"'
    else:
      subst['@size_t_value@'] = 'apr_int32_t'
      subst['@size_t_fmt@'] = '#define APR_SIZE_T_FMT "u"'

    if sizeof_ssize_t:
      subst['@ssize_t_value@'] = 'ssize_t'
      subst['@ssize_t_fmt@'] = '#define APR_SSIZE_T_FMT "ld"'
    else:
      subst['@ssize_t_value@'] = 'apr_int32_t'
      subst['@ssize_t_fmt@'] = '#define APR_SSIZE_T_FMT "d"'

    if conf.Check_apr_big_endian():
      subst['@bigendian@'] = 1
    else:
      subst['@bigendian@'] = 0
    # Now we need to find what apr_int64_t (sizeof == 8) will be.
    # The first match is our preference.
    if sizeof_int == 8:
      subst['@int64_literal@'] = '#define APR_INT64_C(val) (val)'
      subst['@uint64_literal@'] = '#define APR_UINT64_C(val) (val##U)'
      subst['@int64_t_fmt@'] = '#define APR_INT64_T_FMT "d"'
      subst['@uint64_t_fmt@'] = '#define APR_UINT64_T_FMT "u"'
      subst['@uint64_t_hex_fmt@'] = '#define APR_UINT64_T_HEX_FMT "x"'
      subst['@int64_value@'] = 'int'
      subst['@long_value@'] = 'int'
      subst['@int64_strfn=@'] = 'strtoi'
    elif sizeof_long == 8:
      subst['@int64_literal@'] = '#define APR_INT64_C(val) (val##L)'
      subst['@uint64_literal@'] = '#define APR_UINT64_C(val) (val##UL)'
      subst['@int64_t_fmt@'] = '#define APR_INT64_T_FMT "ld"'
      subst['@uint64_t_fmt@'] = '#define APR_UINT64_T_FMT "lu"'
      subst['@uint64_t_hex_fmt@'] = '#define APR_UINT64_T_HEX_FMT "lx"'
      subst['@int64_value@'] = 'long'
      subst['@long_value@'] = 'long'
      subst['@int64_strfn=@'] = 'strtol'
    elif sizeof_long_long == 8:
      subst['@int64_literal@'] = '#define APR_INT64_C(val) (val##LL)'
      subst['@uint64_literal@'] = '#define APR_UINT64_C(val) (val##ULL)'
      # Linux, Solaris, FreeBSD all support ll with printf.
      # BSD 4.4 originated 'q'.  Solaris is more popular and 
      # doesn't support 'q'.  Solaris wins.  Exceptions can
      # go to the OS-dependent section.
      subst['@int64_t_fmt@'] = '#define APR_INT64_T_FMT "lld"'
      subst['@uint64_t_fmt@'] = '#define APR_UINT64_T_FMT "llu"'
      subst['@uint64_t_hex_fmt@'] = '#define APR_UINT64_T_HEX_FMT "llx"'
      subst['@int64_value@'] = 'long long'
      subst['@long_value@'] = 'long long'
      subst['@int64_strfn=@'] = 'strtoll'
    elif sizeof_longlong == 8:
      subst['@int64_literal@'] = '#define APR_INT64_C(val) (val##LL)'
      subst['@uint64_literal@'] = '#define APR_UINT64_C(val) (val##ULL)'
      subst['@int64_t_fmt@'] = '#define APR_INT64_T_FMT "qd"'
      subst['@uint64_t_fmt@'] = '#define APR_UINT64_T_FMT "qu"'
      subst['@uint64_t_hex_fmt@'] = '#define APR_UINT64_T_HEX_FMT "qx"'
      subst['@int64_value@'] = '__int64'
      subst['@long_value@'] = '__int64'
      subst['@int64_strfn=@'] = 'strtoll'
    else:
      print("could not detect a 64-bit integer type")
      self.Exit(1)

    if conf.CheckDeclaration('INT64_C', includes='#include <stdint.h>'):
      subst['@int64_literal@'] = '#define APR_INT64_C(val) INT64_C(val)'
      subst['@uint64_literal@'] = '#define APR_UINT64_C(val) UINT64_C(val)'
      subst['@stdint@'] = 1

    if sizeof_pid_t == sizeof_short:
      subst['@pid_t_fmt@'] = '#define APR_PID_T_FMT "hd"'
    elif sizeof_pid_t == sizeof_int:
      subst['@pid_t_fmt@'] = '#define APR_PID_T_FMT "d"'
    elif sizeof_pid_t == sizeof_long:
      subst['@pid_t_fmt@'] = '#define APR_PID_T_FMT "ld"'
    elif sizeof_pid_t == sizeof_long_long:
      subst['@pid_t_fmt@'] = '#define APR_PID_T_FMT APR_INT64_T_FMT'
    else:
      subst['@pid_t_fmt@'] = '#error Can not determine the proper size for pid_t'
    
    # TODO: Per OS changing of these

    if conf.Check_apr_largefile64():
      self.AppendUnique(CPPFLAGS = ['-D_LARGEFILE64_SOURCE'])

    aprlfs=0
    if self['lfs'] and sizeof_off_t == 4:
      # Check whether the transitional LFS API is sufficient
      aprlfs=1
      for f in ['mmap64', 'sendfile64', 'sendfilev64', 'mkstemp64', 'readdir64_r']:
        conf.CheckFunc(f)
    elif sizeof_off_t == sizeof_size_t:
      aprlfs=1

    if aprlfs and sizeof_off_t == 4:
      # LFS is go!
      subst['@off_t_fmt@'] = '#define APR_OFF_T_FMT APR_INT64_T_FMT'
      subst['@off_t_value@'] = 'off64_t'
      subst['@off_t_strfn@'] = 'apr_strtoi64'
    elif sizeof_off_t == 4 and sizeof_long == 4:
      # Special case: off_t may change size with _FILE_OFFSET_BITS
      # on 32-bit systems with LFS support.  To avoid compatibility
      # issues when other packages do define _FILE_OFFSET_BITS,
      # hard-code apr_off_t to long.
      subst['@off_t_fmt@'] = '#define APR_OFF_T_FMT "ld"'
      subst['@off_t_value@'] = 'long'
      subst['@off_t_strfn@'] = 'strtol'
    elif sizeof_off_t != 0:
      subst['@off_t_value@'] = 'off_t'
      if sizeof_off_t == sizeof_long:
        subst['@off_t_fmt@'] = '#define APR_OFF_T_FMT "ld"'
        subst['@off_t_strfn@'] = 'strtol'
      elif sizeof_off_t == sizeof_int:
        subst['@off_t_fmt@'] = '#define APR_OFF_T_FMT "d"'
        subst['@off_t_strfn@'] = 'strtoi'
      elif sizeof_off_t == sizeof_long_long:
        subst['@off_t_fmt@'] = '#define APR_OFF_T_FMT APR_INT64_T_FMT'
        subst['@off_t_strfn@'] = 'apr_strtoi64'
      else:
        print("could not determine the size of off_t")
        self.Exit(1)
    else:
      # Fallback on int
      subst['@off_t_fmt@'] = '#define APR_OFF_T_FMT "d"'
      subst['@off_t_value@'] = 'apr_int32_t'
      subst['@off_t_strfn@'] = 'strtoi'

    if conf.Check_apr_atomic_builtins():
      conf.Define('HAVE_ATOMIC_BUILTINS', 1)

    if not conf.CheckType('size_t', includes='#include <sys/types.h>'):
      subst['@size_t_value@'] = 'apr_int32_t'

    if not conf.CheckType('ssize_t', includes='#include <sys/types.h>'):
      subst['@ssize_t_value@'] = 'apr_int32_t'

    if not conf.CheckType('socklen_t', includes='#include <sys/socket.h>'):
      subst['@socklen_t_value@'] = 'int'
    else:
      if self['PLATFORM'] == 'hpux' and sizeof_long == 8:
        # 64-bit HP-UX requires 32-bit socklens in
        # kernel, but user-space declarations say
        # 64-bit (socklen_t == size_t == long).
        # This will result in many compile warnings,
        # but we're functionally busted otherwise.
        subst['@socklen_t_value@'] = 'int'
      else:
        subst['@socklen_t_value@'] = 'socklen_t'
        
    # Regardless of whether _LARGEFILE64_SOURCE is used, on 32-bit
    # platforms _FILE_OFFSET_BITS will affect the size of ino_t and hence
    # the build-time ABI may be different from the apparent ABI when using
    # APR with another package which *does* define _FILE_OFFSET_BITS.
    # (Exactly as per the case above with off_t where LFS is *not* used)
    #
    # To be safe, hard-code apr_ino_t as 'unsigned long' iff that is
    # exactly the size of ino_t here; otherwise use ino_t as existing
    # releases did.  To be correct, apr_ino_t should have been made an
    # ino64_t as apr_off_t is off64_t, but this can't be done now without
    # breaking ABI.
    subst['@ino_t_value@'] = 'ino_t'
    if sizeof_long == 4 and conf.CheckTypesCompatible('ino_t', 'unsigned long', '#include <fts.h>'):
      subst['@ino_t_value@'] = 'unsigned long'

    # check for mmap functions
    # store the results into mmap_results dictionary for use later
    mmap_funcs = ['mmap', 'mmap', 'shm_open', 'shm_unlink', 'shm_get',
            'shmat', 'shmdt', 'shmctl']
    mmap_results = dict([[k, conf.CheckFunc(k)] for k in mmap_funcs])

    # check for mmap mapping dev zero
    if mmap_results['mmap'] and \
       conf.CheckFile("/dev/zero") and \
       conf.Check_apr_mmap_mapping_dev_zero():
           subst['@havemmapzero@'] = 1
    else:
        subst['@havemmapzero@'] = 0

    # check for locking mechanisms
    if conf.Check_apr_semaphores():
        subst['@hassysvser@'] = 1
    else:
        subst['@hassysvser@'] = 0

    if conf.CheckDeclaration('F_SETLK', '#include <fcntl.h>'):
        subst['@hasfcntlser@'] = 1
    else:
        subst['@hasfcntlser@'] = 0

    if conf.CheckFunc('flock'):
        subst['@hasflockser@'] = 1
    else:
        subst['@hasflockser@'] = 0

    apr_tcp_nopush_flag="0"
    if conf.CheckDeclaration('TCP_CORK', '#include <netinet/tcp.h>'):
        subst['@have_corkable_tcp@'] = 1
        apr_tcp_nopush_flag="TCP_CORK"
    else:
        subst['@have_corkable_tcp@'] = 0

    if conf.CheckDeclaration('TCP_NOPUSH', '#include <netinet/tcp.h>'):
        subst['@apr_tcp_nopush_flag@'] = 3
        subst['@have_corkable_tcp@'] = 1

    subst['@apr_tcp_nopush_flag@'] = apr_tcp_nopush_flag

    if conf.CheckFunc('flock'):
        subst['@hasflockser@'] = 1
    else:
        subst['@hasflockser@'] = 0

    if conf.CheckFunc('getrlimit'):
        subst['@have_getrlimit@'] = 1
    else:
        subst['@have_getrlimit@'] = 0

    if conf.CheckFunc('setrlimit'):
        subst['@have_setrlimit@'] = 1
    else:
        subst['@have_setrlimit@'] = 0

    if conf.CheckType('struct in_addr', includes='#include <netinet/in.h>'):
        subst['@have_in_addr@'] = 1
    else:
        subst['@have_in_addr@'] = 0

    if conf.CheckType('struct sockaddr_storage', includes='#include <netinet/in.h>'):
        subst['@have_sa_storage@'] = 1
    else:
        subst['@have_sa_storage@'] = 0

    if conf.CheckType('struct rlimit', includes='#include <sys/resource.h>'):
        subst['@struct_rlimit@'] = 1
    else:
        subst['@struct_rlimit@'] = 0

    if conf.Check_apr_semun():
        subst['@have_union_semun@'] = 1
    else:
        subst['@have_union_semun@'] = 0

    check_functions = [
        'inet_addr',
        'inet_network',
        'memmove',
        'sigaction',
        'sigsuspend',
        'sigwait',
        'strdup',
        'stricmp',
        'strcasecmp',
        'strncasecmp',
        'strnicmp',
        'strstr',
        'memchr',
        'iovec',
        'fork',
        'mmap',
        'uuid_create',
        'uuid_generate',
        'waitpid'
    ]

    for func in check_functions:
        if conf.CheckFunc(func):
            subst['@have_%s@' % func] = 1
        else:
            subst['@have_%s@' % func] = 0

    if self['PLATFORM'] in ['sunos']:
      conf.Define("SIGWAIT_TAKES_ONE_ARG")

    # Set Features
    # TODO: Not done yet
    subst['@sharedmem@'] = 1
    subst['@threads@'] = 1
    subst['@sendfile@'] = 0
    subst['@mmap@'] = subst['@have_mmap@']
    subst['@fork@'] = subst['@have_fork@']
    subst['@rand@'] = 0
    subst['@oc@'] = 0
    subst['@aprdso@'] = 0
    subst['@have_unicode_fs@'] = 0
    subst['@have_proc_invoked@'] = 0
    subst['@aprlfs@'] = 0
    subst['@osuuid@'] = subst['@have_uuid_generate@'] or subst['@have_uuid_create@']
    subst['@file_as_socket@'] = 1

    # check for IPv6 (the user is allowed to disable this via commandline
    # options
    subst['@have_ipv6@'] = 0
    if self['ipv6']:
        if conf.CheckType('struct sockaddr_in6', 
                          includes='#include <netinet/in.h>') and \
           conf.CheckFunc('getaddrinfo') and \
           conf.CheckFunc('getnameinfo'):
            subst['@have_ipv6@'] = 1

    if conf.CheckDeclaration('SO_ACCEPTFILTER', '#include <sys/socket.h>'):
        subst['@acceptfilter@'] = 1
    else:
        subst['@acceptfilter@'] = 0

    if conf.CheckDeclaration('IPPROTO_SCTP', '#include <netinet/in.h>') and \
        conf.Check_apr_sctp():
        subst['@have_sctp@'] = 1
    else:
        subst['@have_sctp@'] = 0

    if conf.CheckDeclaration('SO_ACCEPTFILTER', '#include <sys/socket.h>'):
        subst['@acceptfilter@'] = 1
    else:
        subst['@acceptfilter@'] = 0

    if conf.Check_apr_check_tcp_nodelay_inherited():
        subst['@tcp_nodelay_inherited@'] = 1
    else:
        subst['@tcp_nodelay_inherited@'] = 0

    if conf.Check_apr_nonblock_inherited():
        subst['@o_nonblock_inherited@'] = 1
    else:
        subst['@o_nonblock_inherited@'] = 0

    if conf.Check_apr_ebcdic():
        subst['@apr_charset_ebcdic@'] = 1
    else:
        subst['@apr_charset_ebcdic@'] = 0

    if conf.CheckType('struct iovec', includes='#include <sys/types.h>\n#include <sys/uio.h>'):
      subst['@have_iovec@'] = 1
    else:
      subst['@have_iovec@'] = 0
    

    if conf.CheckType('struct sockaddr_un', includes='#include <sys/un.h>'):
      subst['@have_sockaddr_un@'] = 1
    else:
      subst['@have_sockaddr_un@'] = 0

    subst['@proc_mutex_is_global@'] = 0
    if self['PLATFORM'] in ['os2', 'beos', 'win32', 'cygwin']:
      subst['@proc_mutex_is_global@'] = 1
  
    # note: the current APR use of shared mutex requires /dev/zero
    if conf.CheckFile('/dev/zero') and \
        conf.CheckDeclaration('PTHREAD_PROCESS_SHARED', includes='#include <pthread.h>') and \
        conf.CheckFunc('pthread_mutexattr_setpshared'):
      subst['@hasprocpthreadser@'] = 1
    else:
      subst['@hasprocpthreadser@'] = 0
    
    
    subst['@havemmaptmp@'] = 0
    subst['@havemmapshm@'] = 0
    subst['@haveshmgetanon@'] = 0
    subst['@haveshmget@'] = 0
    subst['@havemmapanon@'] = 0
    subst['@havebeosarea@'] = 0

    subst['@usemmaptmp@'] = 0
    subst['@usemmapshm@'] = 0
    subst['@usemmapzero@'] = 0
    subst['@useshmgetanon@'] = 0
    subst['@useshmget@'] = 0
    subst['@usemmapanon@'] = 0
    subst['@usebeosarea@'] = 0

    subst['@flockser@'] = 0
    subst['@sysvser@'] = 0
    subst['@posixser@'] = 0
    subst['@fcntlser@'] = 0
    subst['@procpthreadser@'] = 0 
    subst['@pthreadser@'] = 0
    subst['@hasposixser@'] = 0
    subst['@proclockglobal@'] = 0

    if self['APR_PLATFORM'] in ['win32']:
      subst['@eolstr@'] = "\\\\r\\\\n"
    else:
      subst['@eolstr@'] = "\\\\n"

    subst['@shlibpath_var@'] = pjoin(self['prefix'], 'lib')

    # APR Util things to fix:
    subst['@apu_have_sdbm@'] = 1
    subst['@apu_have_gdbm@'] = 0
    subst['@apu_have_ndbm@'] = 0
    subst['@apu_have_db@'] = 0
    subst['@apu_use_sdbm@'] = 1
    subst['@apu_use_ndbm@'] = 0
    subst['@apu_use_gdbm@'] = 0
    subst['@apu_use_db@'] = 0

    subst['@apu_db_version@'] = 0

    subst['@apu_have_pgsql@'] = 0
    subst['@apu_have_mysql@'] = 0
    subst['@apu_have_sqlite3@'] = 0
    subst['@apu_have_sqlite2@'] = 0
    subst['@apu_have_oracle@'] = 0
    subst['@apu_have_odbc@'] = 0


    subst['@apu_have_crypto@'] = 0
    subst['@apu_have_openssl@'] = 0
    subst['@apu_have_nss@'] = 0

    subst['@have_iconv@'] = 0
    
    self.SubstFile('include/apr.h', 'include/apr.h.in', SUBST_DICT = subst)
    self.SubstFile('include/apu.h', 'include/apu.h.in', SUBST_DICT = subst)
    self.SubstFile('include/apu_want.h', 'include/apu_want.h.in', SUBST_DICT = subst)
    self.SubstFile('include/private/apu_select_dbm.h', 'include/private/apu_select_dbm.h.in', SUBST_DICT = subst)
    if hasattr(conf, "config_h_text"):
      conf.Define("APR_OFF_T_STRFN", subst['@off_t_strfn@'])
      conf.config_h_text = conf.config_h_text + '#include "arch/apr_private_common.h"\n'
 
    return conf.Finish()
