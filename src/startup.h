/* startup.h -- Eterm main program header file
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

#ifndef _STARTUP_H
# define _STARTUP_H
# include <X11/Xfuncproto.h>
# include <X11/Intrinsic.h>	/* Xlib, Xutil, Xresource, Xfuncproto */
# include <ctype.h>
# include <stdio.h>
# include <stdarg.h>
# include <stdlib.h>
# include <string.h>
# include "misc.h"

/************ Macros and Definitions ************/
# ifndef EXIT_SUCCESS		/* missing from <stdlib.h> */
#  define EXIT_SUCCESS	0	/* exit function success */
#  define EXIT_FAILURE	1	/* exit function failure */
# endif

# define THEME_CFG	"theme.cfg"
# define USER_CFG	"user.cfg"

# define MAX_COLS	200
# define MAX_ROWS	128

# ifndef min
#  define min(a,b)	(((a) < (b)) ? (a) : (b))
# endif
# ifndef max
#  define max(a,b)	(((a) > (b)) ? (a) : (b))
# endif
# ifndef MIN
#  define MIN(a,b)	(((a) < (b)) ? (a) : (b))
# endif
# ifndef MAX
#  define MAX(a,b)	(((a) > (b)) ? (a) : (b))
# endif
# define LOWER_BOUND(current, other)    (((current) < (other)) ? ((current) = (other)) : (current))
# define AT_LEAST(current, other)       LOWER_BOUND(current, other)
# define MAX_IT(current, other)         LOWER_BOUND(current, other)
# define UPPER_BOUND(current, other)    (((current) > (other)) ? ((current) = (other)) : (current))
# define AT_MOST(current, other)        UPPER_BOUND(current, other)
# define MIN_IT(current, other)         UPPER_BOUND(current, other)
# define SWAP_IT(one, two, tmp)         do {(tmp) = (one); (one) = (two); (two) = (tmp);} while (0)

/* width of scrollBar, menuBar shadow ... don't change! */
# define SHADOW	2

/* convert pixel dimensions to row/column values */
# define Pixel2Width(x)		((x) / TermWin.fwidth)
# define Pixel2Height(y)	((y) / TermWin.fheight)
# define Pixel2Col(x)		Pixel2Width((x) - TermWin.internalBorder)
# define Pixel2Row(y)		Pixel2Height((y) - TermWin.internalBorder)
# define Width2Pixel(n)		((n) * TermWin.fwidth)
# define Height2Pixel(n)	((n) * TermWin.fheight)
# define Col2Pixel(col)		(Width2Pixel(col) + TermWin.internalBorder)
# define Row2Pixel(row)		(Height2Pixel(row) + TermWin.internalBorder)

# define TermWin_TotalWidth()	(TermWin.width  + 2 * TermWin.internalBorder)
# define TermWin_TotalHeight()	(TermWin.height + 2 * TermWin.internalBorder)

# define Xscreen		DefaultScreen(Xdisplay)
# define Xcmap			DefaultColormap(Xdisplay,Xscreen)
# define Xdepth			DefaultDepth(Xdisplay,Xscreen)
# define Xroot			DefaultRootWindow(Xdisplay)
# define Xvisual		DefaultVisual(Xdisplay, Xscreen)
# ifdef DEBUG_DEPTH
#  undef Xdepth
#  define Xdepth		DEBUG_DEPTH
# endif

/************ Structures ************/
typedef struct {
  int   internalBorder; 	/* Internal border size */
  short x, y;                   /* TermWin.parent coordinates */
  short width,  height;  	/* window size [pixels] */
  short fwidth, fheight;	/* font width and height [pixels] */
  short fprop;		        /* font is proportional */
  short ncol, nrow;		/* window size [characters] */
  short focus;		        /* window has focus */
  short saveLines;		/* number of lines that fit in scrollback */
  short nscrolled;		/* number of line actually scrolled */
  short view_start;		/* scrollback view starts here */
  Window parent, vt;		/* parent (main) and vt100 window */
  GC gc;			/* GC for drawing text */
  XFontStruct	* font;		/* main font structure */
  XFontSet fontset;
# ifndef NO_BOLDFONT
  XFontStruct	* boldFont;	/* bold font */
# endif
# ifdef MULTI_CHARSET
  XFontStruct	* mfont;	/* multibyte font structure */
# endif
} TermWin_t;

/************ Variables ************/
extern TermWin_t TermWin;
extern Window root;
extern Display *Xdisplay;
extern Colormap cmap;
extern char *orig_argv0;
#ifdef PIXMAP_SUPPORT
extern short bg_needs_update;
#endif
extern const char *display_name;

/************ Function Prototypes ************/
_XFUNCPROTOBEGIN
extern int eterm_bootstrap(int argc, char *argv[]);
_XFUNCPROTOEND

#endif
