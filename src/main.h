/*--------------------------------*-C-*---------------------------------*
 * File:	main.h
 */
/* notes: */
/*----------------------------------------------------------------------*
 * Copyright 1992 John Bovey, University of Kent at Canterbury.
 *
 * You can do what you like with this source code as long as you don't try
 * to make money out of it and you include an unaltered copy of this
 * message (including the copyright).
 *
 * This module has been heavily modified by R. Nation
 * <nation@rocket.sanders.lockheed.com>
 * No additional restrictions are applied
 *
 * Additional modifications by mj olesen <olesen@me.QueensU.CA>
 * No additional restrictions are applied.
 *
 * As usual, the author accepts no responsibility for anything, nor does
 * he guarantee anything whatsoever.
 *----------------------------------------------------------------------*/

#ifndef _MAIN_H
# define _MAIN_H
# include "config.h"
# include "feature.h"
# include <X11/Xfuncproto.h>
# include <assert.h>
# include <ctype.h>
# include <stdio.h>

/* STDC_HEADERS
 * don't check for these using configure, since we need them regardless.
 * if you don't have them -- figure a workaround.
 *
 * Sun is often reported as not being STDC_HEADERS, but it's not true
 * for our purposes and only generates spurious bug reports.
 */
# include <stdarg.h>
# include <stdlib.h>
# include <string.h>

# ifndef EXIT_SUCCESS		/* missing from <stdlib.h> */
#  define EXIT_SUCCESS	0	/* exit function success */
#  define EXIT_FAILURE	1	/* exit function failure */
# endif

# include <X11/Intrinsic.h>	/* Xlib, Xutil, Xresource, Xfuncproto */
# include "misc.h"

typedef struct {
    int   internalBorder; /* Internal border size */
    short width,  height;	/* window size [pixels] */
    short fwidth, fheight;	/* font width and height [pixels] */
    short	    fprop;	/* font is proportional                     */
    short ncol, nrow;	/* window size [characters] */
    short focus;		/* window has focus */
    short saveLines;	/* number of lines that fit in scrollback */
    short nscrolled;	/* number of line actually scrolled */
    short view_start;	/* scrollback view starts here */
    Window parent, vt;	/* parent (main) and vt100 window */
    Window wm_parent,   /* The parent assigned by the WM */
      wm_grandparent;   /* The grandparent assigned by the WM */
    GC gc;		/* GC for drawing text */
    XFontStruct	* font;	/* main font structure */
# ifndef NO_BOLDFONT
    XFontStruct	* boldFont;	/* bold font */
# endif
# ifdef KANJI
    XFontStruct	* kanji;	/* Kanji font structure */
# endif
# ifdef PIXMAP_SUPPORT
    Pixmap pixmap;
#  ifdef PIXMAP_BUFFERING
    Pixmap buf_pixmap;
#  endif
# endif
} TermWin_t;

extern TermWin_t TermWin;
extern Window root;
extern Display		* Xdisplay;
extern char *def_colorName[];
extern const char *def_fontName[];
# ifdef KANJI
extern const char *def_kfontName[];
# endif

# define MAX_COLS	200
# define MAX_ROWS	128

# ifndef min
#  define min(a,b)	(((a) < (b)) ? (a) : (b))
#  define max(a,b)	(((a) > (b)) ? (a) : (b))
# endif
# ifndef MIN
#  define MIN(a,b)	(((a) < (b)) ? (a) : (b))
# endif
# ifndef MAX
#  define MAX(a,b)	(((a) > (b)) ? (a) : (b))
# endif

# define MAX_IT(current, other)	if ((other) > (current)) (current) = (other)
# define MIN_IT(current, other)	if ((other) < (current)) (current) = (other)
# define SWAP_IT(one, two, tmp)				\
do {						\
(tmp) = (one); (one) = (two); (two) = (tmp);	\
} while (0)

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
# ifdef DEBUG_DEPTH
#  undef Xdepth
#  define Xdepth		DEBUG_DEPTH
# endif

# define Opt_console			(1LU <<  0)
# define Opt_loginShell			(1LU <<  1)
# define Opt_iconic			(1LU <<  2)
# define Opt_visualBell			(1LU <<  3)
# define Opt_mapAlert			(1LU <<  4)
# define Opt_reverseVideo		(1LU <<  5)
# define Opt_utmpLogging		(1LU <<  6)
# define Opt_scrollBar			(1LU <<  7)
# define Opt_meta8			(1LU <<  8)
# define Opt_pixmapScale		(1LU <<  9)
# define Opt_exec			(1LU << 10)
# define Opt_homeOnEcho			(1LU << 11)
# define Opt_homeOnRefresh		(1LU << 12)
# define Opt_scrollBar_floating		(1LU << 13)
# define Opt_scrollBar_right		(1LU << 14)
# define Opt_borderless			(1LU << 15)
# define Opt_pixmapTrans		(1LU << 16)
# define Opt_saveUnder			(1LU << 17)
# define Opt_noCursor			(1LU << 18)
# define Opt_pause			(1LU << 19)
# define Opt_watchDesktop		(1LU << 20)
# define Opt_homeOnInput		(1LU << 21)
# define Opt_menubar_move		(1LU << 22)
# define Opt_xterm_select		(1LU << 23)
# define Opt_select_whole_line		(1LU << 24)
# define Opt_viewport_mode		(1LU << 25)
# define Opt_scrollbar_popup		(1LU << 26)
# define Opt_select_trailing_spaces	(1LU << 24)

/* place holder used for parsing command-line options */
# define Opt_Boolean			(1LU << 31)
extern unsigned long Options;

extern const char *display_name;
extern char *rs_name;	/* client instance (resource name) */

/*
 * XTerm escape sequences: ESC ] Ps;Pt BEL
 */
# define XTerm_name	0
# define XTerm_iconName	1
# define XTerm_title	2
# define XTerm_logfile	46	/* not implemented */
# define XTerm_font	50

/*
 * rxvt/Eterm extensions of XTerm escape sequences: ESC ] Ps;Pt BEL
 */
# define XTerm_Takeover     5     /* Steal keyboard focus and raise window */
# define XTerm_EtermSeq     6     /* Eterm proprietary escape sequences */
# define XTerm_Menu	   10     /* set menu item */
# define XTerm_Pixmap	   20     /* new bg pixmap */
# define XTerm_restoreFG   39     /* change default fg color */
# define XTerm_restoreBG   49     /* change default bg color */

/*----------------------------------------------------------------------*/

# define restoreFG	39	/* restore default fg color */
# define restoreBG	49	/* restore default bg color */

enum colour_list {
    fgColor,
    bgColor,
    minColor,				/* 2 */
    BlackColor = minColor,
    Red3Color,
    Green3Color,
    Yellow3Color,
    Blue3Color,
    Magenta3Color,
    Cyan3Color,
    maxColor,				/* minColor + 7 */
# ifndef NO_BRIGHTCOLOR
    AntiqueWhiteColor = maxColor,
    minBright,				/* maxColor + 1 */
    Grey25Color = minBright,
    RedColor,
    GreenColor,
    YellowColor,
    BlueColor,
    MagentaColor,
    CyanColor,
    maxBright,				/* minBright + 7 */
    WhiteColor = maxBright,
# else
    WhiteColor = maxColor,
# endif
# ifndef NO_CURSORCOLOR
    cursorColor,
    cursorColor2,
# endif
    pointerColor,
    borderColor,
# ifndef NO_BOLDUNDERLINE
    colorBD,
    colorUL,
# endif
    menuTextColor,
# if defined(KEEP_SCROLLCOLOR)
    scrollColor,
#  if defined(CHANGE_SCROLLCOLOR_ON_FOCUS)
    unfocusedScrollColor,
#  endif
# endif
    NRS_COLORS,				/* */
# ifdef KEEP_SCROLLCOLOR
    topShadowColor = NRS_COLORS,
    bottomShadowColor,
#  ifdef CHANGE_SCROLLCOLOR_ON_FOCUS
    unfocusedTopShadowColor,
    unfocusedBottomShadowColor,
#  endif
    TOTAL_COLORS			/* */
# else
      TOTAL_COLORS = NRS_COLORS		/* */
# endif
};

# define NSHADOWCOLORS		(TOTAL_COLORS - NRS_COLORS)

# define DEFAULT_RSTYLE		(RS_None | (fgColor<<8) | (bgColor<<16))

extern char * rs_color [NRS_COLORS];
extern Pixel PixColors [NRS_COLORS + NSHADOWCOLORS];

# define NFONTS		5
extern const char * rs_font [NFONTS];
# ifdef KANJI
extern const char * rs_kfont [NFONTS];
# endif
# ifndef NO_BOLDFONT
extern const char * rs_boldFont;
# endif

# ifdef PRINTPIPE
extern char *rs_print_pipe;
# endif

# ifdef CUTCHAR_OPTION
extern char * rs_cutchars;
# endif

/* prototypes */
_XFUNCPROTOBEGIN

#ifndef GCC
extern void map_menuBar(int);
extern void map_scrollBar(int);
#else
extern inline void map_menuBar(int);
extern inline void map_scrollBar(int);
#endif
extern void xterm_seq(int, const char *);
extern void change_font(int, const char *);
extern void set_width(unsigned short);
extern void resize_window(void);

/* special (internal) prefix for font commands */
# define FONT_CMD	'#'
# define FONT_DN		"#-"
# define FONT_UP		"#+"

# ifdef USE_IMLIB
Pixmap ReadFileToPixmapViaImlib(Display *, char *, int *, int *);
# endif

_XFUNCPROTOEND

#endif	/* whole file */
