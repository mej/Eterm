/* font.h -- Eterm font module header file
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

#ifndef _FONT_H_
#define _FONT_H_

#include <stdio.h>
#include <X11/Xfuncproto.h>
#include <X11/Intrinsic.h>	/* Xlib, Xutil, Xresource, Xfuncproto */

/************ Macros and Definitions ************/
#define FONT_TYPE_X		(0x01)
#define FONT_TYPE_TTF		(0x02)
#define FONT_TYPE_FNLIB		(0x03)

#define font_cache_add_ref(font) ((font)->ref_cnt++)

# define NFONTS	5
/* special (internal) prefix for font commands */
# define FONT_CMD	'#'
# define FONT_DN	"#-"
# define FONT_UP	"#+"
#if (FONT0_IDX == 0)
# define IDX2FNUM(i) (i)
# define FNUM2IDX(f) (f)
#else
# define IDX2FNUM(i) (i == 0? FONT0_IDX : (i <= FONT0_IDX? (i-1) : i))
# define FNUM2IDX(f) (f == FONT0_IDX ? 0 : (f < FONT0_IDX ? (f+1) : f))
#endif
#define FNUM_RANGE(i)	(i <= 0 ? 0 : (i >= NFONTS ? (NFONTS-1) : i))

/************ Structures ************/
typedef struct font_struct {
  char *name;
  unsigned char type;
  unsigned char ref_cnt;
  union {
    XFontStruct *xfontinfo;
  } fontinfo;
  struct font_struct *next;
} etfont_t;
  
/************ Variables ************/
extern unsigned char font_change_count;
extern const char *def_fontName[];
extern char *rs_font[NFONTS];
# ifdef MULTI_CHARSET
extern const char *def_mfontName[];
extern char *rs_mfont[NFONTS];
# endif

/************ Function Prototypes ************/
_XFUNCPROTOBEGIN

extern void *load_font(const char *, const char *, unsigned char);
extern void free_font(const void *);
extern void change_font(int, const char *);

_XFUNCPROTOEND

#endif	/* _FONT_H_ */
