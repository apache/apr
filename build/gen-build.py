#!/usr/bin/env python
#
# USAGE: gen-build.py TYPE
#
# where TYPE is one of: make, dsp, vcproj
#
# It reads build.conf from the current directory, and produces its output
# into the current directory.
#


import os
import ConfigParser
import getopt
import string
import glob
import re

#import ezt


def main():
  parser = ConfigParser.ConfigParser()
  parser.read('build.conf')

  dirs = { }
  files = get_files(parser.get('options', 'paths'))
  headers = get_files(parser.get('options', 'headers'))

  # compute the relevant headers, along with the implied includes
  legal_deps = map(os.path.basename, headers)
  h_deps = { }
  for fname in headers:
    h_deps[os.path.basename(fname)] = extract_deps(fname, legal_deps)
  resolve_deps(h_deps)

  f = open('build-outputs.mk', 'w')
  f.write('# DO NOT EDIT. AUTOMATICALLY GENERATED.\n\n')

  objects = [ ]
  for file in files:
    assert file[-2:] == '.c'
    obj = file[:-2] + '.lo'
    objects.append(obj)

    dirs[os.path.dirname(file)] = None

    # what headers does this file include, along with the implied headers
    deps = extract_deps(file, legal_deps)
    for hdr in deps.keys():
      deps.update(h_deps.get(hdr, {}))

    f.write('%s: %s include/%s\n' % (obj, file, string.join(deps.keys(), ' include/')))

  f.write('\nOBJECTS = %s\n\n' % string.join(objects))
  f.write('HEADERS = $(top_srcdir)/%s\n\n' % string.join(headers, ' $(top_srcdir)/'))
  f.write('SOURCE_DIRS = %s $(EXTRA_SOURCE_DIRS)\n\n' % string.join(dirs.keys()))


def extract_deps(fname, legal_deps):
  "Extract the headers this file includes."
  deps = { }
  for line in open(fname).readlines():
    if line[:8] != '#include':
      continue
    inc = _re_include.match(line).group(1)
    if inc in legal_deps:
      deps[inc] = None
  return deps
_re_include = re.compile('#include *["<](.*)[">]')


def resolve_deps(header_deps):
  "Alter the provided dictionary to flatten includes-of-includes."
  altered = 1
  while altered:
    altered = 0
    for hdr, deps in header_deps.items():
      # print hdr, deps
      start = len(deps)
      for dep in deps.keys():
        deps.update(header_deps.get(dep, {}))
      if len(deps) != start:
        altered = 1


def get_files(patterns):
  patterns = string.replace(patterns, '{platform}', get_platform())
  patterns = string.split(string.strip(patterns))
  files = [ ]
  for pat in patterns:
    files.extend(glob.glob(pat))
  return files

def get_platform():
  return 'unix'


if __name__ == '__main__':
  main()
