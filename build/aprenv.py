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
    Environment.__init__(self, ENV=os.environ, **kw)

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
    context.Message('Checking whether the compiler provides atomic builtins...')
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
    result = context.TryLink(source, '.c')
    context.Result(result)
    return result
  
  def critical(self, f, *args):
    rv = f(*args)

    if not rv:
      traceback.print_stack()
      print "Critial Test failed."
      self.Exit(1)

  def APRAutoconf(self):
    if self.GetOption('clean') or self.GetOption('help'):
      return self

    # TODO Port header detection here etc
    conf = self.Configure(custom_tests = {'Check_apr_atomic_builtins': self.Check_apr_atomic_builtins},
                          config_h = 'include/arch/%s/apr_private.h' % (self['APR_PLATFORM']))

    if conf.Check_apr_atomic_builtins():
      conf.Define('HAVE_ATOMIC_BUILTINS', 1)

    return conf.Finish()
