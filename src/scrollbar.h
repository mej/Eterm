/*--------------------------------*-C-*---------------------------------*
 * File:	scrollbar.h
 *
 * Copyright 1996,97
 * mj olesen <olesen@me.QueensU.CA> Queen's Univ at Kingston
 *
 * You can do what you like with this source code provided you don't make
 * money from it and you include an unaltered copy of this message
 * (including the copyright).  As usual, the author accepts no
 * responsibility for anything, nor does he guarantee anything whatsoever.
 *----------------------------------------------------------------------*/
#ifndef _SCROLLBAR_H
#  define _SCROLLBAR_H

#  include <X11/Xfuncproto.h>
#  include <ctype.h>
#  ifdef PIXMAP_SCROLLBAR
#    include "eterm_imlib.h"
#  endif

/* Scrollbar types we support */
# define SCROLLBAR_MOTIF	1
# define SCROLLBAR_XTERM	2
# define SCROLLBAR_NEXT		3

typedef struct {
  short beg, end;	/* beg/end of slider sub-window */
  short top, bot;	/* top/bot of slider */
  short state;		/* scrollbar state */
  unsigned char type;   /* scrollbar type (see above) */
  short width;          /* scrollbar width */
  Window win;
# ifdef PIXMAP_SCROLLBAR
  Window up_win;
  Window dn_win;
  Window sa_win;    
# endif
} scrollBar_t;
extern scrollBar_t scrollBar;
extern int sb_shadow;

/* prototypes */
_XFUNCPROTOBEGIN

extern int scrollbar_mapping(int);
extern void scrollbar_reset(void);
extern int scrollbar_show(int);
_XFUNCPROTOEND

/* macros */
#define scrollbar_visible()	(scrollBar.state)
#define scrollbar_isMotion()	(scrollBar.state == 'm')
#define scrollbar_isUp()	(scrollBar.state == 'U')
#define scrollbar_isDn()	(scrollBar.state == 'D')
#define scrollbar_isUpDn()	isupper (scrollBar.state)
#define scrollbar_setNone()	do { scrollBar.state = 1; } while (0)
#define scrollbar_setMotion()	do { scrollBar.state = 'm'; } while (0)
#define scrollbar_setUp()	do { scrollBar.state = 'U'; } while (0)
#define scrollbar_setDn()	do { scrollBar.state = 'D'; } while (0)

#define scrollbar_above_slider(y)	((y) < scrollBar.top)
#define scrollbar_below_slider(y)	((y) > scrollBar.bot)
#define scrollbar_position(y)		((y) - scrollBar.beg)
#define scrollbar_size()		(scrollBar.end - scrollBar.beg)

#define scrollbar_total_width()		(scrollBar.width + 2 * sb_shadow)
#define scrollbar_arrow_height()	((scrollBar.width + 1) + sb_shadow)
#define scrollbar_anchor_max_height()	((menubar_visible()) ? (TermWin.height) : (TermWin.height - ((scrollBar.width + 1) + sb_shadow)))

#define scrollbar_up_loc()	((scrollBar.type == SCROLLBAR_NEXT) ? (scrollBar.end + 1) : (sb_shadow))
#define scrollbar_dn_loc()	((scrollBar.type == SCROLLBAR_NEXT) \
                                 ? (scrollBar.end + scrollBar.width + 2) \
                                 : (scrollBar.end + 1))
#define scrollbar_upButton(y)	((scrollBar.type == SCROLLBAR_NEXT && (y) > scrollBar.end && (y) <= scrollbar_dn_loc()) \
                                 || (scrollBar.type != SCROLLBAR_NEXT && (y) < scrollBar.beg))
#define scrollbar_dnButton(y)	((scrollBar.type == SCROLLBAR_NEXT && (y) > scrollbar_dn_loc()) \
                                 || (scrollBar.type != SCROLLBAR_NEXT && (y) > scrollBar.end))

#ifdef PIXMAP_SCROLLBAR
# define isScrollbarWindow(w)     ((scrollbar_is_pixmapped() && scrollbar_visible() && (w) == scrollBar.sa_win) \
                                    || (!scrollbar_is_pixmapped() && scrollbar_visible() && (w) == scrollBar.win))
# define scrollbar_upButtonWin(w) ((w) == scrollBar.up_win)
# define scrollbar_dnButtonWin(w) ((w) == scrollBar.dn_win)
# define scrollbar_is_pixmapped() (0)
#else
# define isScrollbarWindow(w)	  (scrollbar_visible() && (w) == scrollBar.win)
# define scrollbar_is_pixmapped() (0)
#endif

#endif	/* _SCROLLBAR_H */
/*----------------------- end-of-file (C header) -----------------------*/
