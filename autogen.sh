#!/bin/sh
# Run this to generate all the initial makefiles, etc.
# $Id$

broken() {
    echo
    echo "You need libtool, autoconf, and automake.  Install them"
    echo "and try again.  Get source at ftp://ftp.gnu.org/pub/gnu/"
    echo "ERROR:  $1 not found."
    exit -1
}

DIE=0

echo "Generating configuration files for Eterm, please wait...."

LIBTOOLIZE_CHOICES="$LIBTOOLIZE libtoolize glibtoolize"
ACLOCAL_CHOICES="$ACLOCAL aclocal"
AUTOCONF_CHOICES="$AUTOCONF autoconf"
AUTOHEADER_CHOICES="$AUTOHEADER autoheader"
AUTOMAKE_CHOICES="$AUTOMAKE automake"

for i in $LIBTOOLIZE_CHOICES ; do
    $i --version </dev/null >/dev/null 2>&1 && LIBTOOLIZE=$i && break
done
[ "x$LIBTOOLIZE" = "x" ] && broken libtool

for i in $ACLOCAL_CHOICES ; do
    $i --version </dev/null >/dev/null 2>&1 && ACLOCAL=$i && break
done
[ "x$ACLOCAL" = "x" ] && broken automake

for i in $AUTOCONF_CHOICES ; do
    $i --version </dev/null >/dev/null 2>&1 && AUTOCONF=$i && break
done
[ "x$AUTOCONF" = "x" ] && broken autoconf

for i in $AUTOHEADER_CHOICES ; do
    $i --version </dev/null >/dev/null 2>&1 && AUTOHEADER=$i && break
done
[ "x$AUTOHEADER" = "x" ] && broken autoconf

for i in $AUTOMAKE_CHOICES ; do
    $i --version </dev/null >/dev/null 2>&1 && AUTOMAKE=$i && break
done
[ "x$AUTOMAKE" = "x" ] && broken automake

# Export them so configure can AC_SUBST() them.
export LIBTOOLIZE ACLOCAL AUTOCONF AUTOHEADER AUTOMAKE

# Check for existing libast.m4 we can use.  Use the local one if not.
#if test ! -f "`$ACLOCAL --print-ac-dir`/libast.m4"; then
#    ACLOCAL_FLAGS="-I . $ACLOCAL_FLAGS"
#fi

# Run the stuff.
(set -x && $LIBTOOLIZE -c -f)
(set -x && $ACLOCAL -I . $ACLOCAL_FLAGS)
(set -x && $AUTOCONF)
(set -x && $AUTOHEADER)
(set -x && $AUTOMAKE -a -c)

# Run configure.
./configure "$@"

if [ -f cvs.motd ]; then
  echo "ATTENTION CVS Users!"
  echo ""
  cat cvs.motd
  echo ""
fi
