#!/bin/sh
#
# Eterm Color Grid Utility
#
# $Id$

echo "[0m"
echo "Eterm Color Grid Utility"
echo "------------------------"

echo
echo "The grid below displays all possible combinations of the terminal colors"
echo "you have configured."
echo
echo "FG                                        BG"
echo "--                                        --"
echo "     0    1    2    3    4    5    6    7    8    9    10   11   12   13   14   15"
fgstyle=""
for fg in 0 1 2 3 4 5 6 7 ; do
  line=" $fg "
  case $fg in
    0) fg_esc=30 ;;
    1) fg_esc=31 ;;
    2) fg_esc=32 ;;
    3) fg_esc=33 ;;
    4) fg_esc=34 ;;
    5) fg_esc=35 ;;
    6) fg_esc=36 ;;
    7) fg_esc=37 ;;
  esac
  for bgstyle in "" ";5"; do
    for bg in 0 1 2 3 4 5 6 7 ; do
      case $bg in
        0) bg_esc=40 ;;
        1) bg_esc=41 ;;
        2) bg_esc=42 ;;
        3) bg_esc=43 ;;
        4) bg_esc=44 ;;
        5) bg_esc=45 ;;
        6) bg_esc=46 ;;
        7) bg_esc=47 ;;
      esac
      line="${line}[${fg_esc};${bg_esc}${fgstyle}${bgstyle}m txt [0m"
    done
  done
  echo "$line"
done
fgstyle=";1"
for fg in 8 9 10 11 12 13 14 15 ; do
  case $fg in
    8)  fg_esc=30; line=" $fg " ;;
    9)  fg_esc=31; line=" $fg " ;;
    10) fg_esc=32; line="$fg " ;;
    11) fg_esc=33; line="$fg " ;;
    12) fg_esc=34; line="$fg " ;;
    13) fg_esc=35; line="$fg " ;;
    14) fg_esc=36; line="$fg " ;;
    15) fg_esc=37; line="$fg " ;;
  esac
  for bgstyle in "" ";5"; do
    for bg in 0 1 2 3 4 5 6 7 ; do
      case $bg in
        0) bg_esc=40 ;;
        1) bg_esc=41 ;;
        2) bg_esc=42 ;;
        3) bg_esc=43 ;;
        4) bg_esc=44 ;;
        5) bg_esc=45 ;;
        6) bg_esc=46 ;;
        7) bg_esc=47 ;;
      esac
      line="${line}[${fg_esc};${bg_esc}${fgstyle}${bgstyle}m txt [0m"
    done
  done
  echo "$line"
done
