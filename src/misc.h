/*
 * Copyright (C) 1997-2000, Michael Jennings
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of the Software, its documentation and marketing & publicity
 * materials, and acknowledgment shall be given in the documentation, materials
 * and software packages that this Software was used.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _MISC_H_
#define _MISC_H_

#include <X11/Intrinsic.h>	/* Xlib, Xutil, Xresource, Xfuncproto */
#include <X11/Xfuncproto.h>

/************ Macros and Definitions ************/
#define MAKE_CTRL_CHAR(c) ((c) == '?' ? 127 : ((toupper(c)) - '@'))

/************ Function Prototypes ************/
_XFUNCPROTOBEGIN

extern const char *my_basename(const char *str);
extern void print_error(const char *fmt,...);
extern void print_warning(const char *fmt,...);
extern void fatal_error(const char *fmt,...);
extern unsigned long str_leading_match(register const char *, register const char *);
extern char *str_trim(char *str);
extern int parse_escaped_string(char *str);
extern const char *search_path(const char *pathlist, const char *file, const char *ext);
extern const char *find_file(const char *file, const char *ext);
extern unsigned long add_carriage_returns(unsigned char *buff, unsigned long cnt);

_XFUNCPROTOEND

#endif
