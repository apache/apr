#!/bin/sh
#
# extract version numbers from the apr_version.h header file
#
# USAGE: get-version.sh CMD INCLUDEDIR
#   where CMD is one of: all, major
#
#   get-version.sh all returns a dotted version number
#   get-version.sh major returns just the major version number
#

if test $# != 2; then
  echo "USAGE: $0 CMD INCLUDEDIR"
  echo "  where CMD is one of: all, major"
  exit 1
fi

versfile=${2}/apr_version.h

major="`sed -n '/#define.*APR_MAJOR_VERSION/s/^.*\([0-9][0-9]*\).*$/\1/p' $versfile`"
minor="`sed -n '/#define.*APR_MINOR_VERSION/s/^.*\([0-9][0-9]*\).*$/\1/p' $versfile`"
patch="`sed -n '/#define.*APR_PATCH_VERSION/s/^.*\([0-9][0-9]*\).*$/\1/p' $versfile`"

if test "$1" = "all"; then
  echo ${major}.${minor}.${patch}
elif test "$1" = "major"; then
  echo ${major}
else
  echo "ERROR: unknown version CMD"
  exit 1
fi
