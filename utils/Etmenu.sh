#!/bin/sh
#
# Eterm Menu Command Utility
#
# $Id$

if [ $# -eq 0 ]; then
  echo "Syntax:  Etmenu.sh { <command(s)> | <file(s)> }"
  echo
  echo "See the Eterm Technical Reference for valid menu commands."
  exit 0
fi

# Code to figure out if we need 'echo -n' or 'echo "\c"', stolen from configure
if (echo "testing\c"; echo 1,2,3) | grep c >/dev/null; then
  # Stardent Vistra SVR4 grep lacks -e, says ghazi@caip.rutgers.edu.
  if (echo -n testing; echo 1,2,3) | sed s/-n/xn/ | grep xn >/dev/null; then
    ac_n= ac_c='
'
  else
    ac_n=-n ac_c=
  fi
else
  ac_n= ac_c='\c'
fi

while [ "X$1" != "X" ]; do
  case $1 in
    # Send commands to Eterm
    +* | -* | '<'* | '['*) echo $ac_n "]10;$1$ac_c" ;;

    # Read in a menubar file
    *) echo $ac_n "]10;[read:$1]$ac_c" ;;
  esac
  shift
done
