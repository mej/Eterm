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

LIBTOOLIZE_CHOICES="$LIBTOOLIZE libtoolize"
AUTOHEADER_CHOICES="$AUTOHEADER autoheader213 autoheader-2.13 autoheader"
ACLOCAL_CHOICES="$ACLOCAL aclocal14 aclocal-1.4 aclocal"
AUTOMAKE_CHOICES="$AUTOMAKE automake14 automake-1.4 automake"
AUTOCONF_CHOICES="$AUTOCONF autoconf213 autoconf-2.13 autoconf"

for i in $LIBTOOLIZE_CHOICES ; do
    $i --version </dev/null >/dev/null 2>&1 && LIBTOOLIZE=$i && break
done
[ "x$LIBTOOLIZE" = "x" ] && broken libtool

for i in $AUTOHEADER_CHOICES ; do
    $i --version </dev/null >/dev/null 2>&1 && AUTOHEADER=$i && break
done
[ "x$AUTOHEADER" = "x" ] && broken autoconf

for i in $ACLOCAL_CHOICES ; do
    $i --version </dev/null >/dev/null 2>&1 && ACLOCAL=$i && break
done
[ "x$ACLOCAL" = "x" ] && broken automake

for i in $AUTOMAKE_CHOICES ; do
    $i --version </dev/null >/dev/null 2>&1 && AUTOMAKE=$i && break
done
[ "x$AUTOMAKE" = "x" ] && broken automake

for i in $AUTOCONF_CHOICES ; do
    $i --version </dev/null >/dev/null 2>&1 && AUTOCONF=$i && break
done
[ "x$AUTOCONF" = "x" ] && broken autoconf

# Export them so configure can AC_SUBST() them.
export LIBTOOLIZE AUTOHEADER ACLOCAL AUTOMAKE AUTOCONF

# Run the stuff.
(set -x && $LIBTOOLIZE -c -f)
(set -x && $AUTOHEADER)
(set -x && $ACLOCAL $ACLOCAL_FLAGS)
(set -x && $AUTOMAKE -a -c)
(set -x && $AUTOCONF)

# Run configure.
./configure "$@"

if [ -f cvs.motd ]; then
  echo "ATTENTION CVS Users!"
  echo ""
  cat cvs.motd
  echo ""
fi
