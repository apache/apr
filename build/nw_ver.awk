# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

BEGIN {

  # fetch APR version numbers from input file and write them to STDOUT

  while ((getline < ARGV[1]) > 0) {
    if (match ($0, /^#define AP._MAJOR_VERSION/)) {
      ver_major = $3;
    }
    else if (match ($0, /^#define AP._MINOR_VERSION/)) {
      ver_minor = $3;
    }
    else if (match ($0, /^#define AP._PATCH_VERSION/)) {
      ver_patch = $3;
    }
    else if (match ($0, /^#define AP._IS_DEV_VERSION/)) {
      ver_devbuild = 1;
    }
  }
  if (ver_devbuild) {
    ver_dev = "-dev"
    if (ARGV[2]) {
      while ((getline < ARGV[2]) > 0) {
        if (match ($0, /^\/repos\/asf\/!svn\/ver\/[0-9]+\/apr\/(apr|apr-util)\/(trunk|branches\/[0-9]\.[0-9]\.x)$/)) {
          gsub(/^\/repos\/asf\/!svn\/ver\/|\/apr\/(apr|apr-util)\/(trunk|branches\/[0-9]\.[0-9]\.x)$/, "", $0);
          ver_dev = svn_rev = "-r" $0;
        }
      }
    }
  }

  if (WANTED) {
    ver_num = ver_major * 1000000 + ver_minor * 1000 + ver_patch;
    if (ver_num < WANTED) {
      print "ERROR: APR version " ver_str " does NOT match!";
      exit 1;
    } else if (ver_num > (WANTED + 1000)) {
      print "WARNING: APR version " ver_str " higher than expected!";
      exit 0;
    } else {
      print "OK: APR version " ver_str "";
      exit 0;
    }
  } else {
    ver_nlm = ver_major "," ver_minor "," ver_patch;
    ver_str = ver_major "." ver_minor "." ver_patch ver_dev;
    print "VERSION = " ver_nlm "";
    print "VERSION_STR = " ver_str "";
    print "VERSION_MAJMIN = " ver_major ver_minor "";
    # print "COPYRIGHT_STR = " copyright_str "";
    print "SVN_REVISION = " svn_rev "";
  }

}


