#! /bin/sh

echo "buildconf: checking installation..."

# autoconf 2.13 or newer
ac_version=`${AUTOCONF:-autoconf} --version 2>/dev/null|head -1|sed -e 's/^[^0-9]*//' -e 's/[a-z]* *$//'`
if test -z "$ac_version"; then
echo "buildconf: autoconf not found."
echo "           You need autoconf version 2.13 or newer installed"
echo "           to build Apache from CVS."
exit 1
fi
IFS=.; set $ac_version; IFS=' '
if test "$1" = "2" -a "$2" -lt "13" || test "$1" -lt "2"; then
echo "buildconf: autoconf version $ac_version found."
echo "           You need autoconf version 2.13 or newer installed"
echo "           to build Apache from CVS."
exit 1
else
echo "buildconf: autoconf version $ac_version (ok)"
fi

# libtool 1.3.3 or newer
libtool=`build/PrintPath glibtool libtool`
lt_pversion=`$libtool --version 2>/dev/null|head -1|sed -e 's/^[^0-9]*//' -e 's/[- ].*//'`
if test -z "$lt_pversion"; then
echo "buildconf: libtool not found."
echo "           You need libtool version 1.3.3 or newer installed"
echo "           to build Apache from CVS."
exit 1
fi
lt_version=`echo $lt_pversion|sed -e 's/\([a-z]*\)$/.\1/'`
IFS=.; set $lt_version; IFS=' '
lt_status="good"
if test "$1" = "1"; then
   if test "$2" -lt "3"; then
      lt_status="bad"
   else
      if test "$2" = "3"; then
         if test -z "$3" -o "$3" = "1" -o "$3" = "2"; then
            lt_status="bad"
         fi
      fi
   fi
fi
if test $lt_status = "good"; then
   echo "buildconf: libtool version $lt_pversion (ok)"
   exit 0
fi

echo "buildconf: libtool version $lt_pversion found."
echo "           You need libtool version 1.3.3 or newer installed"
echo "           to build Apache from CVS."

exit 1
