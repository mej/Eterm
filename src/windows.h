/*  windows.h -- Eterm window handling module header file
 *            -- 29 April 1999, mej
 *
 * This file is original work by Michael Jennings <mej@eterm.org> and
 * Tuomo Venalainen <vendu@cc.hut.fi>.  This file, and any other file
 * bearing this same message or a similar one, is distributed under
 * the GNU Public License (GPL) as outlined in the COPYING file.
 *
 * Copyright (C) 1997-1999, Michael Jennings and Tuomo Venalainen
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

#ifndef _WINDOWS_H_
#define _WINDOWS_H_
/* includes */
#include <X11/Xfuncproto.h>
#include <X11/Intrinsic.h>	/* Xlib, Xutil, Xresource, Xfuncproto */

/************ Macros and Definitions ************/

/************ Variables ************/
extern char *rs_color[NRS_COLORS];
extern Pixel PixColors[NRS_COLORS + NSHADOWCOLORS];
extern XSetWindowAttributes Attributes;
extern XWindowAttributes attr;
extern XSizeHints szHint;

/************ Function Prototypes ************/
_XFUNCPROTOBEGIN

extern void set_text_property(Window, char *, char *);
extern Pixel get_bottom_shadow_color(Pixel, const char *);
extern Pixel get_top_shadow_color(Pixel, const char *);
extern void Create_Windows(int, char * []);
extern void resize_subwindows(int, int);
extern void resize(void);
extern void resize_window1(unsigned int, unsigned int);
extern void set_width(unsigned short);
extern void resize_window(void);
#ifdef XTERM_COLOR_CHANGE
extern void set_window_color(int, const char *);
#else
# define set_window_color(idx,color) ((void)0)
#endif /* XTERM_COLOR_CHANGE */

_XFUNCPROTOEND

#endif	/* _WINDOWS_H_ */
