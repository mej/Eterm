/* scrollbar.h -- Eterm scrollbar module header file
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
#ifndef _SCROLLBAR_H
# define _SCROLLBAR_H

# include <X11/Xfuncproto.h>
# include <ctype.h>
# ifdef PIXMAP_SCROLLBAR
#  include <Imlib.h>
# endif
# include "events.h"

/************ Macros and Definitions ************/
/* Scrollbar types we support */
# define SCROLLBAR_MOTIF	1
# define SCROLLBAR_XTERM	2
# define SCROLLBAR_NEXT		3

/* Scrollbar state macros */
#define scrollbar_visible()	(scrollBar.state)
#define scrollbar_isMotion()	(scrollBar.state == 'm')
#define scrollbar_isUp()	(scrollBar.state == 'U')
#define scrollbar_isDn()	(scrollBar.state == 'D')
#define scrollbar_isUpDn()	isupper (scrollBar.state)
#define scrollbar_setNone()	do { D_SCROLLBAR(("scrollbar_setNone():  Cancelling motion.\n")); scrollBar.state = 1; } while (0)
#define scrollbar_setMotion()	do { D_SCROLLBAR(("scrollbar_setMotion()\n")); scrollBar.state = 'm'; } while (0)
#define scrollbar_setUp()	do { D_SCROLLBAR(("scrollbar_setUp()\n")); scrollBar.state = 'U'; } while (0)
#define scrollbar_setDn()	do { D_SCROLLBAR(("scrollbar_setNone()\n")); scrollBar.state = 'D'; } while (0)

/* The various scrollbar elements */
#ifdef PIXMAP_SCROLLBAR
# define scrollbar_win_is_scrollbar(w)      (scrollbar_visible() && (w) == scrollBar.win)
# define scrollbar_win_is_uparrow(w)        ((w) == scrollBar.up_win)
# define scrollbar_win_is_downarrow(w)      ((w) == scrollBar.dn_win)
# define scrollbar_win_is_anchor(w)         ((w) == scrollBar.sa_win)
# define scrollbar_is_pixmapped()           ((images[image_sb].current->iml->im) && (images[image_sb].mode & MODE_MASK))
# define scrollbar_uparrow_is_pixmapped()   ((images[image_up].current->iml->im) && (images[image_up].mode & MODE_MASK))
# define scrollbar_downarrow_is_pixmapped() ((images[image_down].current->iml->im) && (images[image_down].mode & MODE_MASK))
# define scrollbar_anchor_is_pixmapped()    ((images[image_sa].current->iml->im) && (images[image_sa].mode & MODE_MASK))
# define scrollbar_upButton(w, y)	    ( scrollbar_uparrow_is_pixmapped() ? scrollbar_win_is_uparrow(w) \
                                              : ((scrollBar.type == SCROLLBAR_NEXT && (y) > scrollBar.end && (y) <= scrollbar_dn_loc()) \
                                                 || (scrollBar.type != SCROLLBAR_NEXT && (y) < scrollBar.beg)))
# define scrollbar_dnButton(w, y)	    ( scrollbar_downarrow_is_pixmapped() ? scrollbar_win_is_downarrow(w) \
                                              : ((scrollBar.type == SCROLLBAR_NEXT && (y) > scrollbar_dn_loc()) \
                                                 || (scrollBar.type != SCROLLBAR_NEXT && (y) > scrollBar.end)))
#else
# define scrollbar_win_is_scrollbar(w)	    (scrollbar_visible() && (w) == scrollBar.win)
# define scrollbar_win_is_uparrow(w)        (0)
# define scrollbar_win_is_downarrow(w)      (0)
# define scrollbar_win_is_anchor(w)         (0)
# define scrollbar_is_pixmapped()           (0)
# define scrollbar_uparrow_is_pixmapped()   (0)
# define scrollbar_downarrow_is_pixmapped() (0)
# define scrollbar_anchor_is_pixmapped()    (0)
# define scrollbar_upButton(w, y)	    ((scrollBar.type == SCROLLBAR_NEXT && (y) > scrollBar.end && (y) <= scrollbar_dn_loc()) \
                                              || (scrollBar.type != SCROLLBAR_NEXT && (y) < scrollBar.beg))
# define scrollbar_dnButton(w, y)	    ((scrollBar.type == SCROLLBAR_NEXT && (y) > scrollbar_dn_loc()) \
                                              || (scrollBar.type != SCROLLBAR_NEXT && (y) > scrollBar.end))
#endif

/* Scrollbar dimensions */
#define scrollbar_scrollarea_height()	(scrollBar.end - scrollBar.beg)
#define scrollbar_anchor_width()	(scrollBar.width)
#define scrollbar_anchor_height()	(scrollBar.bot - scrollBar.top)
#define scrollbar_trough_width()	(scrollBar.width + 2 * scrollBar.shadow)
#define scrollbar_trough_height()	(scrollBar.end + ((scrollBar.type == SCROLLBAR_NEXT) ? (2 * (scrollBar.width + 1) + scrollBar.shadow) \
                                                                                             : ((scrollBar.width + 1) + scrollBar.shadow)))
#define scrollbar_arrow_width()		(scrollBar.width)
#define scrollbar_arrow_height()	(scrollBar.width)
#define scrollbar_anchor_max_height()	((menubar_visible()) ? (TermWin.height) : (TermWin.height - ((scrollBar.width + 1) + scrollBar.shadow)))

/* Scrollbar positions */
#define scrollbar_is_above_anchor(w, y)	((y) < scrollBar.top && !scrollbar_win_is_anchor(w))
#define scrollbar_is_below_anchor(w, y)	((y) > scrollBar.bot && !scrollbar_win_is_anchor(w))
#define scrollbar_position(y)		((y) - scrollBar.beg)
#define scrollbar_up_loc()	(scrollbar_uparrow_is_pixmapped() ? ((scrollBar.type == SCROLLBAR_NEXT) ? (scrollBar.end + 1) : (scrollBar.shadow)) \
                                 : ((scrollBar.type == SCROLLBAR_NEXT) ? (scrollBar.end + 1) : (scrollBar.shadow)))
#define scrollbar_dn_loc()	(scrollbar_downarrow_is_pixmapped() \
                                 ? ((scrollBar.type == SCROLLBAR_NEXT) ? (scrollBar.end + scrollBar.width + 2) : (scrollBar.end + 1)) \
                                 : ((scrollBar.type == SCROLLBAR_NEXT) ? (scrollBar.end + scrollBar.width + 2) : (scrollBar.end + 1)))

/* Scrollbar operations */
#define map_scrollbar(show) do {if (scrollbar_mapping(show)) {scr_touch(); parent_resize();} PrivMode(show, PrivMode_scrollBar); } while (0)
#define scrollbar_get_shadow()		(scrollBar.shadow)
#define scrollbar_set_shadow(s)		do { scrollBar.shadow = (s); } while (0)
#define scrollbar_get_win() (scrollBar.win)
#define scrollbar_get_uparrow_win() (scrollBar.up_win)
#define scrollbar_get_downarrow_win() (scrollBar.dn_win)
#define scrollbar_get_anchor_win() (scrollBar.sa_win)

/************ Structures ************/
typedef struct {
  short beg, end;				/* beg/end of slider sub-window */
  short top, bot;				/* top/bot of slider */
  unsigned char state;				/* scrollbar state */
  unsigned char type:2;				/* scrollbar type (see above) */
  unsigned char init:1;				/* has scrollbar been drawn? */
  unsigned char shadow:5;			/* shadow width */
  unsigned short width, height;			/* scrollbar width and height, without the shadow */
  unsigned short win_width, win_height;		/* scrollbar window dimensions */
  short up_arrow_loc, down_arrow_loc;		/* y coordinates for arrows */
  unsigned short arrow_width, arrow_height;	/* scrollbar arrow dimensions */
  Window win;
# ifdef PIXMAP_SCROLLBAR
  Window up_win;
  Window dn_win;
  Window sa_win;    
# endif
} scrollbar_t;

/************ Variables ************/
extern scrollbar_t scrollBar;
#ifdef SCROLLBAR_BUTTON_CONTINUAL_SCROLLING
extern short scroll_arrow_delay;
#endif

/************ Function Prototypes ************/
_XFUNCPROTOBEGIN

extern void Draw_up_button(int, int, int);
extern void Draw_dn_button(int, int, int);
extern void scrollbar_init(void);
extern void scrollbar_event_init_dispatcher(void);
extern unsigned char sb_handle_configure_notify(event_t *);
extern unsigned char sb_handle_enter_notify(event_t *);
extern unsigned char sb_handle_leave_notify(event_t *);
extern unsigned char sb_handle_focus_in(event_t *);
extern unsigned char sb_handle_focus_out(event_t *);
extern unsigned char sb_handle_expose(event_t *);
extern unsigned char sb_handle_button_press(event_t *);
extern unsigned char sb_handle_button_release(event_t *);
extern unsigned char sb_handle_motion_notify(event_t *);
extern unsigned char scrollbar_dispatch_event(event_t *);
extern unsigned char scrollbar_mapping(unsigned char);
extern void scrollbar_reset(void);
extern void scrollbar_resize(int, int);
extern unsigned char scrollbar_show(short);

_XFUNCPROTOEND

#endif	/* _SCROLLBAR_H */
