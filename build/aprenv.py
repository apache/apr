#
#

from SCons.Environment import Environment
from os.path import join as pjoin
import re
import os
import traceback

_platforms = [ 'aix', 'beos', 'netware', 'os2', 'os390', 'unix', 'win32' ]

_platform_dirs = ['atomic',
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
              'user']

_simple_dirs = ['tables', 'strings']

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

    self.AppendUnique(CPPPATH = ['include', 'include/arch/'+self['APR_PLATFORM']])

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
      self['APR_VERSION'] = self.read_version('APR', 'include/apr_version.h')
    return self['APR_VERSION']

  def read_version(self, prefix, path):
    version_re = re.compile("(.*)%s_(?P<id>MAJOR|MINOR|PATCH)_VERSION(\s+)(?P<num>\d)(.*)" % prefix)
    versions = {}
    fp = open(path, 'rb')
    for line in fp.readlines():
      m = version_re.match(line)
      if m:
        versions[m.group('id')] = int(m.group('num'))
    fp.close()
    return (versions['MAJOR'], versions['MINOR'], versions['PATCH'])

  def Filter(self, **kw):
    for k in kw.keys():
      self[k] = [x for x in self[k] if x is not kw[k]]

  def APRHints(self):
    # TOOD: port more from apr_hints.m4
    if self['PLATFORM'] == 'darwin':
      self.AppendUnique(CPPFLAGS=['-DDARWIN', '-DSIGPROCMASK_SETS_THREAD_MASK', '-no-cpp-precomp'])


  def Check_apr_atomic_builtins(self, context):
    context.Message('Checking whether the compiler provides atomic builtins... ')
    source = """
int main()
{
    unsigned long val = 1010, tmp, *mem = &val;

    if (__sync_fetch_and_add(&val, 1010) != 1010 || val != 2020)
        return 1;

    tmp = val;

    if (__sync_fetch_and_sub(mem, 1010) != tmp || val != 1010)
        return 1;

    if (__sync_sub_and_fetch(&val, 1010) != 0 || val != 0)
        return 1;

    tmp = 3030;

    if (__sync_val_compare_and_swap(mem, 0, tmp) != 0 || val != tmp)
        return 1;

    if (__sync_lock_test_and_set(&val, 4040) != 3030)
        return 1;

    mem = &tmp;

    if (__sync_val_compare_and_swap(&mem, &tmp, &val) != &tmp)
        return 1;

    __sync_synchronize();

    if (mem != &val)
        return 1;

    return 0;
}
    """
    result = context.TryRun(source, '.c')
    context.Result(result[0])
    return result[0]

  def Check_apr_largefile64(self, context):
    context.Message('Checking whether to enable -D_LARGEFILE64_SOURCE... ')
    self.AppendUnique(CPPFLAGS = '-D_LARGEFILE64_SOURCE')
    source = """
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

void main(void)
{
    int fd, ret = 0;
    struct stat64 st;
    off64_t off = 4242;

    if (sizeof(off64_t) != 8 || sizeof(off_t) != 4)
       exit(1);
    if ((fd = open("conftest.lfs", O_LARGEFILE|O_CREAT|O_WRONLY, 0644)) < 0)
       exit(2);
    if (ftruncate64(fd, off) != 0)
       ret = 3;
    else if (fstat64(fd, &st) != 0 || st.st_size != off)
       ret = 4;
    else if (lseek64(fd, off, SEEK_SET) != off)
       ret = 5;
    else if (close(fd) != 0)
       ret = 6;
    else if (lstat64("conftest.lfs", &st) != 0 || st.st_size != off)
       ret = 7;
    else if (stat64("conftest.lfs", &st) != 0 || st.st_size != off)
       ret = 8;
    unlink("conftest.lfs");

    exit(ret);
}
    """
    result = context.TryRun(source, '.c')
    self.Filter(CPPFLAGS = '-D_LARGEFILE64_SOURCE')
    context.Result(result[0])
    return result[0]

  def Check_apr_big_endian(self, context):
    context.Message("Checking for big endianess... ")
    import struct
    array = struct.pack('cccc', '\x01', '\x02', '\x03', '\x04')
    i = struct.unpack('i', array)
    if i == struct.unpack('>i', array):
      context.Result('yes')
      return 1
    else:
      context.Result('no')
      return 0

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
    subst = {}

    if self.GetOption('clean') or self.GetOption('help'):
      return self
    # TODO Port header detection here etc
    conf = self.Configure(custom_tests = {
                            'Check_apr_atomic_builtins': self.Check_apr_atomic_builtins,
                            'Check_apr_largefile64': self.Check_apr_largefile64,
                            'Check_apr_big_endian': self.Check_apr_big_endian,
                            },
                          config_h = 'include/arch/%s/apr_private.h' % (self['APR_PLATFORM']))

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
    sizeof_int = conf.CheckTypeSize('int')
    sizeof_long = conf.CheckTypeSize('long')
    sizeof_short = self.critical_value(conf.CheckTypeSize, 2, 'short')
    sizeof_long_long = conf.CheckTypeSize('long long')
    sizeof_longlong = conf.CheckTypeSize('longlong')
    sizeof_pid_t = conf.CheckTypeSize('pid_t', includes='#include <sys/types.h>')
    sizeof_off_t = conf.CheckTypeSize('off_t', includes='#include <sys/types.h>')
    sizeof_size_t = conf.CheckTypeSize('size_t')

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
      self.AppendUnique(CPPFLAGS = '-D_LARGEFILE64_SOURCE')

    aprlfs=0
    if self['lfs'] and sizeof_off_t == 4:
      # Check whether the transitional LFS API is sufficient
      aprlfs=1
      for f in ['mmap64', 'sendfile64', 'sendfilev64', 'mkstemp64', 'readdir64_r']:
        conf.CheckFunc(f)
    elif sizeof_off_t == sizeof_size_t:
      aprlfs=1

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
        
    self.SubstFile('include/apr.h', 'include/apr.h.in', SUBST_DICT = subst)

    return conf.Finish()
