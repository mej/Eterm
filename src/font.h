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
#define FONT_TYPE_X             (0x01)
#define FONT_TYPE_TTF           (0x02)
#define FONT_TYPE_FNLIB         (0x03)

#define font_cache_add_ref(font) ((font)->ref_cnt++)

#define NFONTS 5
#define FONT_CMD       '#'
#define BIGGER_FONT    "#+"
#define SMALLER_FONT   "#-"

#define NEXT_FONT(i)   do { if (font_idx + ((i)?(i):1) >= font_cnt) {font_idx = font_cnt - 1;} else {font_idx += ((i)?(i):1);} \
                            while (!etfonts[font_idx]) {if (font_idx == font_cnt) {font_idx--; break;} font_idx++;} } while (0)
#define PREV_FONT(i)   do { if (font_idx - ((i)?(i):1) < 0) {font_idx = 0;} else {font_idx -= ((i)?(i):1);} \
                            while (!etfonts[font_idx]) {if (font_idx == 0) break; font_idx--;} } while (0)
#define DUMP_FONTS()   do {unsigned char i; D_FONT(("DUMP_FONTS():  Font count is %u\n", (unsigned int) font_cnt)); \
                           for (i = 0; i < font_cnt; i++) {D_FONT(("DUMP_FONTS():  Font %u == \"%s\"\n", (unsigned int) i, NONULL(etfonts[i])));}} while (0)

/************ Structures ************/
typedef struct cachefont_struct {
  char *name;
  unsigned char type;
  unsigned char ref_cnt;
  union {
    XFontStruct *xfontinfo;
  } fontinfo;
  struct cachefont_struct *next;
} cachefont_t;

/************ Variables ************/
extern unsigned char font_idx, def_font_idx, font_cnt, font_chg;
extern const char *def_fontName[];
extern char *rs_font[NFONTS];
extern char **etfonts, **etmfonts;
# ifdef MULTI_CHARSET
extern const char *def_mfontName[];
extern char *rs_mfont[NFONTS];
# endif

/************ Function Prototypes ************/
_XFUNCPROTOBEGIN

extern void eterm_font_add(char ***plist, const char *fontname, unsigned char idx);
extern void eterm_font_delete(char **flist, unsigned char idx);
extern void *load_font(const char *, const char *, unsigned char);
extern void free_font(const void *);
extern void change_font(int, const char *);

_XFUNCPROTOEND

#endif	/* _FONT_H_ */
