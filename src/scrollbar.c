/*--------------------------------*-C-*---------------------------------*
 * File:	scrollbar.c
 *
 * scrollbar routines
 *
 * Copyright 1996,97
 * mj olesen <olesen@me.QueensU.CA> Queen's Univ at Kingston
 *
 * You can do what you like with this source code provided you don't make
 * money from it and you include an unaltered copy of this message
 * (including the copyright).  As usual, the author accepts no
 * responsibility for anything, nor does he guarantee anything whatsoever.
 *----------------------------------------------------------------------*/

static const char cvs_ident[] = "$Id$";

#include "feature.h"
#include "main.h"
#include "scrollbar.h"
#include "screen.h"
#include "debug.h"
#include "options.h"
#ifdef PIXMAP_SCROLLBAR
# include "pixmap.h"
#endif

/* extern functions referenced */
/* extern variables referenced */
/* extern variables declared here */

#ifdef PIXMAP_SCROLLBAR
scrollBar_t scrollBar =
{0, 0, 0, 0, 0, SCROLLBAR_DEFAULT_TYPE, SB_WIDTH, None, None, None, None};

#else
scrollBar_t scrollBar =
{0, 0, 0, 0, 0, SCROLLBAR_DEFAULT_TYPE, SB_WIDTH, None};

#endif
int sb_shadow;

/*----------------------------------------------------------------------*
 */
static GC scrollbarGC;
static short last_top = 0, last_bot = 0;	/* old (drawn) values */

#ifdef XTERM_SCROLLBAR		/* bitmap scrollbar */
static GC shadowGC;
static char xterm_sb_bits[] =
{0xaa, 0x0a, 0x55, 0x05};	/* 12x2 bitmap */

#endif

#if defined(MOTIF_SCROLLBAR) || defined(NEXT_SCROLLBAR)
static GC topShadowGC, botShadowGC;

/* draw triangular up button with a shadow of SHADOW (1 or 2) pixels */
/* PROTO */
void
Draw_up_button(int x, int y, int state)
{

  const unsigned int sz = (scrollBar.width), sz2 = (scrollBar.width / 2);
  XPoint pt[3];
  GC top = None, bot = None;

# ifdef PIXMAP_SCROLLBAR
  Pixmap pixmap = upPixmap.pixmap;

# endif

  D_SCROLLBAR(("Draw_up_button(%d, %d, %d)\n", x, y, state));

  switch (state) {
    case +1:
      top = topShadowGC;
      bot = botShadowGC;
      break;
    case -1:
      top = botShadowGC;
      bot = topShadowGC;
      break;
    case 0:
    default:
      top = bot = scrollbarGC;
      break;
  }

# ifdef PIXMAP_SCROLLBAR
  if ((scrollbar_is_pixmapped()) && (pixmap != None)) {
    XSetWindowBackgroundPixmap(Xdisplay, scrollBar.up_win, pixmap);
    XClearWindow(Xdisplay, scrollBar.up_win);
  } else
# endif
  {
    /* fill triangle */
    pt[0].x = x;
    pt[0].y = y + sz - 1;
    pt[1].x = x + sz - 1;
    pt[1].y = y + sz - 1;
    pt[2].x = x + sz2;
    pt[2].y = y;
    XFillPolygon(Xdisplay, scrollBar.win, scrollbarGC,
		 pt, 3, Convex, CoordModeOrigin);

    /* draw base */
    XDrawLine(Xdisplay, scrollBar.win, bot,
	      pt[0].x, pt[0].y, pt[1].x, pt[1].y);

    /* draw shadow */
    pt[1].x = x + sz2 - 1;
    pt[1].y = y;
    XDrawLine(Xdisplay, scrollBar.win, top,
	      pt[0].x, pt[0].y, pt[1].x, pt[1].y);
# if (SHADOW > 1)
    /* doubled */
    pt[0].x++;
    pt[0].y--;
    pt[1].y++;
    XDrawLine(Xdisplay, scrollBar.win, top,
	      pt[0].x, pt[0].y, pt[1].x, pt[1].y);
# endif
    /* draw shadow */
    pt[0].x = x + sz2;
    pt[0].y = y;
    pt[1].x = x + sz - 1;
    pt[1].y = y + sz - 1;
    XDrawLine(Xdisplay, scrollBar.win, bot,
	      pt[0].x, pt[0].y, pt[1].x, pt[1].y);
# if (SHADOW > 1)
    /* doubled */
    pt[0].y++;
    pt[1].x--;
    pt[1].y--;
    XDrawLine(Xdisplay, scrollBar.win, bot,
	      pt[0].x, pt[0].y, pt[1].x, pt[1].y);
# endif
  }
}

/* draw triangular down button with a shadow of SHADOW (1 or 2) pixels */
/* PROTO */
void
Draw_dn_button(int x, int y, int state)
{

  const unsigned int sz = (scrollBar.width), sz2 = (scrollBar.width / 2);
  XPoint pt[3];
  GC top = None, bot = None;

# ifdef PIXMAP_SCROLLBAR
  Pixmap pixmap = dnPixmap.pixmap;

# endif

  D_SCROLLBAR(("Draw_dn_button(%d, %d, %d)\n", x, y, state));

  switch (state) {
    case +1:
      top = topShadowGC;
      bot = botShadowGC;
      break;
    case -1:
      top = botShadowGC;
      bot = topShadowGC;
      break;
    case 0:
    default:
      top = bot = scrollbarGC;
      break;
  }

# ifdef PIXMAP_SCROLLBAR
  if ((scrollbar_is_pixmapped()) && (pixmap != None)) {
    XSetWindowBackgroundPixmap(Xdisplay, scrollBar.up_win, pixmap);
    XClearWindow(Xdisplay, scrollBar.dn_win);
  } else
# endif
  {
    /* fill triangle */
    pt[0].x = x;
    pt[0].y = y;
    pt[1].x = x + sz - 1;
    pt[1].y = y;
    pt[2].x = x + sz2;
    pt[2].y = y + sz;
    XFillPolygon(Xdisplay, scrollBar.win, scrollbarGC,
		 pt, 3, Convex, CoordModeOrigin);

    /* draw base */
    XDrawLine(Xdisplay, scrollBar.win, top,
	      pt[0].x, pt[0].y, pt[1].x, pt[1].y);

    /* draw shadow */
    pt[1].x = x + sz2 - 1;
    pt[1].y = y + sz - 1;
    XDrawLine(Xdisplay, scrollBar.win, top,
	      pt[0].x, pt[0].y, pt[1].x, pt[1].y);
# if (SHADOW > 1)
    /* doubled */
    pt[0].x++;
    pt[0].y++;
    pt[1].y--;
    XDrawLine(Xdisplay, scrollBar.win, top,
	      pt[0].x, pt[0].y, pt[1].x, pt[1].y);
# endif
    /* draw shadow */
    pt[0].x = x + sz2;
    pt[0].y = y + sz - 1;
    pt[1].x = x + sz - 1;
    pt[1].y = y;
    XDrawLine(Xdisplay, scrollBar.win, bot,
	      pt[0].x, pt[0].y, pt[1].x, pt[1].y);
# if (SHADOW > 1)
    /* doubled */
    pt[0].y--;
    pt[1].x--;
    pt[1].y++;
    XDrawLine(Xdisplay, scrollBar.win, bot,
	      pt[0].x, pt[0].y, pt[1].x, pt[1].y);
# endif
  }
}
#endif /* MOTIF_SCROLLBAR || NEXT_SCROLLBAR */

/* PROTO */
int
scrollbar_mapping(int map)
{

  int change = 0;

  D_SCROLLBAR(("scrollbar_mapping(%d)\n", map));

  if (map && !scrollbar_visible()) {
    scrollBar.state = 1;
    XMapWindow(Xdisplay, scrollBar.win);
#ifdef PIXMAP_SCROLLBAR
    if (scrollbar_is_pixmapped()) {
      XMapWindow(Xdisplay, scrollBar.up_win);
      XMapWindow(Xdisplay, scrollBar.dn_win);
      XMapWindow(Xdisplay, scrollBar.sa_win);
    }
#endif
    change = 1;
  } else if (!map && scrollbar_visible()) {
    scrollBar.state = 0;
#ifdef PIXMAP_SCROLLBAR
    if (scrollbar_is_pixmapped()) {
      XUnmapWindow(Xdisplay, scrollBar.up_win);
      XUnmapWindow(Xdisplay, scrollBar.dn_win);
      XUnmapWindow(Xdisplay, scrollBar.sa_win);
    }
#endif
    XUnmapWindow(Xdisplay, scrollBar.win);
    change = 1;
  }
  return change;
}

void
scrollbar_reset(void)
{

  if (scrollbarGC != None) {
    XFreeGC(Xdisplay, scrollbarGC);
    scrollbarGC = None;
  }
  if (topShadowGC != None) {
    XFreeGC(Xdisplay, topShadowGC);
    topShadowGC = None;
  }
  if (botShadowGC != None) {
    XFreeGC(Xdisplay, botShadowGC);
    botShadowGC = None;
  }
  if (shadowGC != None) {
    XFreeGC(Xdisplay, shadowGC);
    shadowGC = None;
  }
  last_top = last_bot = 0;
  if (scrollBar.type == SCROLLBAR_XTERM) {
    sb_shadow = 0;
  } else {
    sb_shadow = (Options & Opt_scrollBar_floating) ? 0 : SHADOW;
  }
}

int
scrollbar_show(int mouseoffset)
{

  static short sb_width;	/* old (drawn) values */
  unsigned char xsb = 0, force_update = 0;

#ifdef CHANGE_SCROLLCOLOR_ON_FOCUS
  static int focus = -1;

#endif

  if (!scrollbar_visible())
    return 0;

  D_SCROLLBAR(("scrollbar_show(%d)\n", mouseoffset));

  if (scrollbarGC == None) {

    XGCValues gcvalue;

#ifdef XTERM_SCROLLBAR
    if (scrollBar.type == SCROLLBAR_XTERM) {
      sb_width = scrollBar.width - 1;
      gcvalue.stipple = XCreateBitmapFromData(Xdisplay, scrollBar.win,
					      xterm_sb_bits, 12, 2);
      if (!gcvalue.stipple) {
	print_error("can't create bitmap");
	exit(EXIT_FAILURE);
      }
      gcvalue.fill_style = FillOpaqueStippled;
      gcvalue.foreground = PixColors[fgColor];
      gcvalue.background = PixColors[bgColor];

      scrollbarGC = XCreateGC(Xdisplay, scrollBar.win,
			      GCForeground | GCBackground |
			      GCFillStyle | GCStipple,
			      &gcvalue);
      gcvalue.foreground = PixColors[borderColor];
      shadowGC = XCreateGC(Xdisplay, scrollBar.win, GCForeground, &gcvalue);
    }
#endif /* XTERM_SCROLLBAR */

#if defined(MOTIF_SCROLLBAR) || defined(NEXT_SCROLLBAR)
    if (scrollBar.type == SCROLLBAR_MOTIF || scrollBar.type == SCROLLBAR_NEXT) {
      sb_width = scrollBar.width;
# ifdef KEEP_SCROLLCOLOR
      gcvalue.foreground = (Xdepth <= 2 ? PixColors[fgColor]
			    : PixColors[scrollColor]);
# else
      gcvalue.foreground = PixColors[fgColor];
# endif
      scrollbarGC = XCreateGC(Xdisplay, scrollBar.win, GCForeground,
			      &gcvalue);

      if ((sb_shadow) && (!(scrollbar_is_pixmapped()))) {
	XSetWindowBackground(Xdisplay, scrollBar.win, gcvalue.foreground);
	XClearWindow(Xdisplay, scrollBar.win);
      } else if (Options & Opt_scrollBar_floating) {
	if (!(Options & Opt_pixmapTrans)) {
	  if (background_is_pixmap()) {
	    XSetWindowBackground(Xdisplay, scrollBar.win, PixColors[scrollColor]);
	  } else {
	    XSetWindowBackground(Xdisplay, scrollBar.win, PixColors[bgColor]);
	  }
	}
	XClearWindow(Xdisplay, scrollBar.win);
      }
# ifdef KEEP_SCROLLCOLOR
      gcvalue.foreground = PixColors[topShadowColor];
# else				/* KEEP_SCROLLCOLOR */
      gcvalue.foreground = PixColors[bgColor];
# endif				/* KEEP_SCROLLCOLOR */
      topShadowGC = XCreateGC(Xdisplay, scrollBar.win,
			      GCForeground,
			      &gcvalue);

# ifdef KEEP_SCROLLCOLOR
      gcvalue.foreground = PixColors[bottomShadowColor];
# else				/* KEEP_SCROLLCOLOR */
      gcvalue.foreground = PixColors[bgColor];
# endif				/* KEEP_SCROLLCOLOR */
      botShadowGC = XCreateGC(Xdisplay, scrollBar.win,
			      GCForeground,
			      &gcvalue);
    }
#endif /* MOTIF_SCROLLBAR || NEXT_SCROLLBAR */
  }
#if defined(CHANGE_SCROLLCOLOR_ON_FOCUS) && (defined(MOTIF_SCROLLBAR) || defined(NEXT_SCROLLBAR))
  if (scrollBar.type == SCROLLBAR_MOTIF || scrollBar.type == SCROLLBAR_NEXT) {
    if (focus != TermWin.focus) {
      XGCValues gcvalue;

      focus = TermWin.focus;
      gcvalue.foreground = PixColors[focus ? scrollColor : unfocusedScrollColor];
# ifdef PIXMAP_OFFSET
      if (!((Options & Opt_pixmapTrans) && (Options & Opt_scrollBar_floating))) {
# endif
	XSetWindowBackground(Xdisplay, scrollBar.win, gcvalue.foreground);
	XClearWindow(Xdisplay, scrollBar.win);
# ifdef PIXMAP_OFFSET
      }
# endif				/* PIXMAP_OFFSET */
      XChangeGC(Xdisplay, scrollbarGC, GCForeground,
		&gcvalue);

      gcvalue.foreground = PixColors[focus ? topShadowColor : unfocusedTopShadowColor];
      XChangeGC(Xdisplay, topShadowGC,
		GCForeground,
		&gcvalue);
      gcvalue.foreground = PixColors[focus ? bottomShadowColor : unfocusedBottomShadowColor];
      XChangeGC(Xdisplay, botShadowGC,
		GCForeground,
		&gcvalue);
      force_update = 1;
    }
  }
#endif /* CHANGE_SCROLLCOLOR_ON_FOCUS && (MOTIF_SCROLLBAR || NEXT_SCROLLBAR) */

  if (mouseoffset) {
    int top = (TermWin.nscrolled - TermWin.view_start);
    int bot = top + (TermWin.nrow - 1);
    int len = max((TermWin.nscrolled + (TermWin.nrow - 1)), 1);

    scrollBar.top = (scrollBar.beg + (top * scrollbar_size()) / len);
    scrollBar.bot = (scrollBar.beg + (bot * scrollbar_size()) / len);

    if (rs_min_anchor_size && scrollBar.type != SCROLLBAR_XTERM) {
      if ((scrollbar_size() > rs_min_anchor_size) && (scrollBar.bot - scrollBar.top < rs_min_anchor_size)) {

	int grab_point = scrollBar.top + mouseoffset;

	if (grab_point - mouseoffset < scrollBar.beg) {
	  scrollBar.top = scrollBar.beg;
	  scrollBar.bot = rs_min_anchor_size + scrollBar.beg;
	} else if (scrollBar.top + rs_min_anchor_size > scrollBar.end) {
	  scrollBar.top = scrollBar.end - rs_min_anchor_size;
	  scrollBar.bot = scrollBar.end;
	} else {
	  scrollBar.top = grab_point - mouseoffset;
	  scrollBar.bot = scrollBar.top + rs_min_anchor_size;
	}
	if (scrollBar.bot == scrollBar.end) {
	  scr_move_to(scrollBar.end, scrollbar_size());
	  scr_refresh(SMOOTH_REFRESH);
	}
      }
    }
    /* no change */
    if (!force_update && (scrollBar.top == last_top) && (scrollBar.bot == last_bot))
      return 0;
  }
#ifdef XTERM_SCROLLBAR
  xsb = ((scrollBar.type == SCROLLBAR_XTERM) && (Options & Opt_scrollBar_right)) ? 1 : 0;
#endif
  if (last_top < scrollBar.top) {
#ifdef PIXMAP_SCROLLBAR
    if (scrollbar_is_pixmapped()) {
      D_SCROLLBAR(("ATTN: XClearArea() #1, going down\n"));
      XClearArea(Xdisplay, scrollBar.win,
		 sb_shadow + xsb, last_top,
		 sb_width, (scrollBar.top - last_top),
		 False);
      XMoveResizeWindow(Xdisplay, scrollBar.sa_win,
			0, scrollBar.top,
			scrollbar_total_width(),
			(scrollBar.bot - scrollBar.top));
    } else
#endif
    {
      D_SCROLLBAR(("ATTN: XClearArea() #2\n"));
      XClearArea(Xdisplay, scrollBar.win,
		 sb_shadow + xsb, last_top,
		 sb_width, (scrollBar.top - last_top),
		 False);
    }
  }
  if (scrollBar.bot < last_bot) {
#ifdef PIXMAP_SCROLLBAR
    if (scrollbar_is_pixmapped()) {
      D_SCROLLBAR(("ATTN: XClearArea() #3, going up\n"));
      XClearArea(Xdisplay, scrollBar.win,
		 sb_shadow + xsb, scrollBar.bot,
		 sb_width, (last_bot - scrollBar.bot),
		 False);
      XMoveResizeWindow(Xdisplay, scrollBar.sa_win,
			0, scrollBar.top,
			scrollbar_total_width(),
			(scrollBar.bot - scrollBar.top));
    } else
#endif
    {
      D_SCROLLBAR(("ATTN: XClearArea() #4\n"));
      XClearArea(Xdisplay, scrollBar.win,
		 sb_shadow + xsb, scrollBar.bot,
		 sb_width, (last_bot - scrollBar.bot),
		 False);
    }
  }
  last_top = scrollBar.top;
  last_bot = scrollBar.bot;

  /* scrollbar slider */
#ifdef XTERM_SCROLLBAR
  if (scrollBar.type == SCROLLBAR_XTERM) {
    XFillRectangle(Xdisplay, scrollBar.win, scrollbarGC,
		   xsb + 1, scrollBar.top,
		   sb_width - 2, (scrollBar.bot - scrollBar.top));
    XDrawLine(Xdisplay, scrollBar.win, shadowGC,
	      xsb ? 0 : sb_width, scrollBar.beg, xsb ? 0 : sb_width, scrollBar.end);
  }
#endif /* XTERM_SCROLLBAR */

#if defined(MOTIF_SCROLLBAR) || defined(NEXT_SCROLLBAR)
  if (scrollBar.type == SCROLLBAR_MOTIF || scrollBar.type == SCROLLBAR_NEXT) {
# ifdef PIXMAP_SCROLLBAR
    if ((scrollbar_is_pixmapped()) && (saPixmap.pixmap != None)) {
      D_SCROLLBAR(("ATTN: XCopyArea(%dx%d)", sb_width, (scrollBar.bot - scrollBar.top)));
      XCopyArea(Xdisplay, saPixmap.pixmap, scrollBar.sa_win, scrollbarGC,
		0, 0, sb_width, (scrollBar.bot - scrollBar.top),
		0, scrollBar.top);
      XClearArea(Xdisplay, scrollBar.sa_win, 0, scrollBar.top,
		 sb_width, (scrollBar.bot - scrollBar.top), False);
    } else
# endif
    {
      XFillRectangle(Xdisplay, scrollBar.win, scrollbarGC,
		     sb_shadow, scrollBar.top,
		     sb_width, (scrollBar.bot - scrollBar.top));
    }
    if ((sb_shadow) && (!(scrollbar_is_pixmapped()))) {
      /* trough shadow */
      Draw_Shadow(scrollBar.win, botShadowGC, topShadowGC, 0, 0,
		  (sb_width + 2 * sb_shadow),
		  scrollBar.end + ((scrollBar.type == SCROLLBAR_NEXT) ? (2 * (sb_width + 1) + sb_shadow) : ((sb_width + 1) + sb_shadow)));
    }
    /* shadow for scrollbar slider */
    if (!(scrollbar_is_pixmapped())) {
      Draw_Shadow(scrollBar.win, topShadowGC, botShadowGC, sb_shadow, scrollBar.top, sb_width,
		  (scrollBar.bot - scrollBar.top));
    }
    /*
     * Redraw scrollbar arrows
     */
    Draw_up_button(sb_shadow, scrollbar_up_loc(), (scrollbar_isUp()? -1 : +1));
    Draw_dn_button(sb_shadow, scrollbar_dn_loc(), (scrollbar_isDn()? -1 : +1));
  }
#endif /* MOTIF_SCROLLBAR || NEXT_SCROLLBAR */

  return 1;
}
