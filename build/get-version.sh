#!/bin/sh
#
# extract version numbers from a header file
#
# USAGE: get-version.sh CMD VERSION_HEADER
#   where CMD is one of: all, major, libtool
#
#   get-version.sh all returns a dotted version number
#   get-version.sh major returns just the major version number
#   get-version.sh libtool returns a version "libtool -version-info" format
#

if test $# != 2; then
  echo "USAGE: $0 CMD INCLUDEDIR"
  echo "  where CMD is one of: all, major"
  exit 1
fi

major="`sed -n '/#define.*APR_MAJOR_VERSION/s/^.*\([0-9][0-9]*\).*$/\1/p' $2`"
minor="`sed -n '/#define.*APR_MINOR_VERSION/s/^.*\([0-9][0-9]*\).*$/\1/p' $2`"
patch="`sed -n '/#define.*APR_PATCH_VERSION/s/^.*\([0-9][0-9]*\).*$/\1/p' $2`"

if test "$1" = "all"; then
  echo ${major}.${minor}.${patch}
elif test "$1" = "major"; then
  echo ${major}
elif test "$1" = "libtool"; then
  echo ${minor}:${patch}:${minor}
else
  echo "ERROR: unknown version CMD"
  exit 1
fi
