/*  misc.h -- Eterm toolkit header file
 *
 * This file is original work by Michael Jennings <mej@eterm.org> and
 * Tuomo Venalainen <vendu@cc.hut.fi>.  This file, and any other file
 * bearing this same message or a similar one, is distributed under
 * the GNU Public License (GPL) as outlined in the COPYING file.
 *
 * Copyright (C) 1997, Michael Jennings and Tuomo Venalainen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
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
extern void Draw_tl(Window win, GC gc, int x, int y, int w, int h);
extern void Draw_br(Window win, GC gc, int x, int y, int w, int h);
extern void Draw_Shadow(Window win, GC topShadow, GC botShadow, int x, int y, int w, int h);
extern void Draw_Triangle(Window win, GC topShadow, GC botShadow, int x, int y, int w, int type);

_XFUNCPROTOEND

#endif	/* whole file */
