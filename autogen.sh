#!/bin/sh
# Run this to generate all the initial makefiles, etc.
# $Id$

DIE=0

echo "Generating configuration files for Eterm, please wait...."

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo
        echo "You must have autoconf installed to compile Eterm."
        echo "Download the appropriate package for your distribution,"
        echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
        DIE=1
}

(libtool --version) < /dev/null > /dev/null 2>&1 || {
        echo
        echo "You must have libtool installed to compile Eterm."
        echo "Download the appropriate package for your distribution,"
        echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
        DIE=1
}

(automake --version) < /dev/null > /dev/null 2>&1 || {
        echo
        echo "You must have automake installed to compile Eterm."
        echo "Download the appropriate package for your distribution,"
        echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
        DIE=1
}

if test "$DIE" -eq 1; then
        exit 1
fi

echo "  libtoolize -c -f"
libtoolize -c -f
echo "  aclocal -I . $ACLOCAL_FLAGS"
aclocal -I . $ACLOCAL_FLAGS
echo "  autoconf"
autoconf
echo "  autoheader"
autoheader
echo "  automake -a -c"
automake -a -c

./configure "$@"

if [ -f cvs.motd ]; then
  echo "ATTENTION CVS Users!"
  echo ""
  cat cvs.motd
  echo ""
fi
