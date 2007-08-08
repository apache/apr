BEGIN {

  # fetch APR version numbers from input file and writes them to STDOUT

  while ((getline < ARGV[1]) > 0) {
    if (match ($0, /^#define APR_MAJOR_VERSION/)) {
      ver_major = $3;
    }
    else if (match ($0, /^#define APR_MINOR_VERSION/)) {
      ver_minor = $3;
    }
    else if (match ($0, /^#define APR_PATCH_VERSION/)) {
      ver_patch = $3;
    }
    else if (match ($0, /^#define APR_IS_DEV_VERSION/)) {
      ver_devbuild = 1;
    }
  }
  ver_str = ver_major "." ver_minor "." ver_patch (ver_devbuild ? "-dev" : "");
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
    print "VERSION = " ver_nlm "";
    print "VERSION_STR = " ver_str "";
  }

}


