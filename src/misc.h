/*--------------------------------*-C-*---------------------------------*
 * File:	misc.h
 *
 * miscellaneous service routines
 *
 * Copyright 1996,97
 * mj olesen <olesen@me.QueensU.CA> Queen's Univ at Kingston
 *
 * You can do what you like with this source code provided you don't make
 * money from it and you include an unaltered copy of this message
 * (including the copyright).  As usual, the author accepts no
 * responsibility for anything, nor does he guarantee anything whatsoever.
 *----------------------------------------------------------------------*/

#ifndef _MISC_H_
#define _MISC_H_
#include <X11/Xfuncproto.h>

/* prototypes */
_XFUNCPROTOBEGIN

const char *my_basename(const char *str);
void print_error(const char *fmt,...);
void print_warning(const char *fmt,...);
void fatal_error(const char *fmt,...);
unsigned long str_leading_match(register const char *, register const char *);
char *str_trim(char *str);
int parse_escaped_string(char *str);
const char *search_path(const char *pathlist, const char *file, const char *ext);
const char *find_file(const char *file, const char *ext);
void Draw_tl(Window win, GC gc, int x, int y, int w, int h);
void Draw_br(Window win, GC gc, int x, int y, int w, int h);
void Draw_Shadow(Window win, GC topShadow, GC botShadow, int x, int y, int w, int h);
void Draw_Triangle(Window win, GC topShadow, GC botShadow, int x, int y, int w, int type);

_XFUNCPROTOEND

#endif	/* whole file */
