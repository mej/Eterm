/*  pixmap.c -- Eterm pixmap handling routines
 *           -- vendu & mej
 *
 * This file is original work by Michael Jennings <mej@tcserv.com> and
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

static const char cvs_ident[] = "$Id$";

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

#include "main.h"
#include "feature.h"
#include "../libmej/debug.h"
#include "mem.h"
#include "options.h"
#ifdef PIXMAP_SUPPORT
# include "pixmap.h"
# include "screen.h"
# ifdef USE_POSIX_THREADS
#  include "threads.h"
# endif
# include "Eterm.xpm"		/* Icon pixmap */
#endif
#ifdef PIXMAP_SCROLLBAR
# include "scrollbar.h"
#endif
#ifdef PIXMAP_MENUBAR
# include "menubar.h"
#endif

#ifdef USE_IMLIB
# include "eterm_imlib.h"
#endif

#ifdef PIXMAP_SUPPORT
/* Specifying a single extension is irrelevant with Imlib. -vendu */
# ifdef USE_IMLIB
#  define PIXMAP_EXT NULL
# endif

extern XWindowAttributes attr;
extern char *rs_path;
extern char *rs_pixmapScale;

# ifdef PIXMAP_OFFSET
extern const char *rs_pixmapTrans;
extern unsigned int rs_shadePct;
extern unsigned long rs_tintMask;
Pixmap desktop_pixmap = None;
Pixmap viewport_pixmap = None;
Window desktop_window = None;

# endif
# ifdef USE_EFFECTS
int fade_in(Pixmap *, ImlibImage *, int);
int fade_out(Pixmap *, ImlibImage *, int);

# endif

extern short bg_needs_update;

void set_bgPixmap(const char * /* file */ );
void resize_subwindows(int, int);

# ifdef USE_POSIX_THREADS
extern short bg_set;
extern char *sig_to_str(int sig);
extern pthread_t resize_sub_thr;
extern pthread_attr_t resize_sub_thr_attr;

# endif

# ifdef PIXMAP_OFFSET
extern XSizeHints szHint;
Pixmap offset_pixmap(Pixmap, int, int, renderop_t);

# endif

pixmap_t bgPixmap =
{0, 0, 50, 50, None};

# ifdef PIXMAP_SCROLLBAR
pixmap_t upPixmap =
{0, 0, 50, 50, None};
pixmap_t dnPixmap =
{0, 0, 50, 50, None};
pixmap_t sbPixmap =
{0, 0, 50, 50, None};
pixmap_t saPixmap =
{0, 0, 50, 50, None};

# endif

# ifdef USE_IMLIB
ImlibData *imlib_id = NULL;
imlib_t imlib_bg =
{NULL, 0, 0};

#  ifdef PIXMAP_SCROLLBAR
imlib_t imlib_up =
{NULL, 0, 0};
imlib_t imlib_dn =
{NULL, 0, 0};
imlib_t imlib_sb =
{NULL, 0, 0};
imlib_t imlib_sa =
{NULL, 0, 0};

#  endif

#  ifdef PIXMAP_MENUBAR
extern menu_t *ActiveMenu;
pixmap_t mbPixmap =
{0, 0, 50, 50, None};
pixmap_t mb_selPixmap =
{0, 0, 50, 50, None};

#   ifdef USE_IMLIB
imlib_t imlib_mb =
{NULL, 0, 0};
imlib_t imlib_ms =
{NULL, 0, 0};

#   endif
#  endif

inline ImlibImage *
ReadImageViaImlib(Display * d, const char *filename)
{
  Image *tmp;

  D_IMLIB(("ReadImageViaImlib(%s)\n", filename));

  tmp = ImlibLoadImage(imlib_id, (char *) filename, NULL);
  return tmp;
}

# endif				/* USE_IMLIB */

/*
 * These GEOM strings indicate absolute size/position:
 * @ `WxH+X+Y'
 * @ `WxH+X'    -> Y = X
 * @ `WxH'      -> Y = X = 50
 * @ `W+X+Y'    -> H = W
 * @ `W+X'      -> H = W, Y = X
 * @ `W'        -> H = W, X = Y = 50
 * @ `0xH'      -> H *= H/100, X = Y = 50 (W unchanged)
 * @ `Wx0'      -> W *= W/100, X = Y = 50 (H unchanged)
 * @ `=+X+Y'    -> (H, W unchanged)
 * @ `=+X'      -> Y = X (H, W unchanged)
 *
 * These GEOM strings adjust position relative to current position:
 * @ `+X+Y'
 * @ `+X'       -> Y = X
 *
 * And this GEOM string is for querying current scale/position:
 * @ `?'
 */


/*   '[', 2*4 + 2*3 digits + 3 delimiters, ']'. -vendu */
# define GEOM_LEN 19		/* #undef'd immediately after scale_pixmap(). - vendu */
int
scale_pixmap(const char *geom, pixmap_t * pmap)
{

  static char str[GEOM_LEN + 1] =
  {'\0'};
  int w = 0, h = 0, x = 0, y = 0;
  int flags;
  int changed = 0;
  char *p;
  int n;
  Screen *scr;

  if (geom == NULL)
    return 0;
  scr = ScreenOfDisplay(Xdisplay, Xscreen);
  if (!scr)
    return;

  D_PIXMAP(("scale_pixmap(\"%s\")\n", geom));
  if (!strcmp(geom, "?")) {
# if 0
    sprintf(str,		/* Nobody in their right mind would want this to happen -- mej */
	    "[%dx%d+%d+%d]",
	    bgPixmap.w,
	    bgPixmap.h,
	    bgPixmap.x,
	    bgPixmap.y);
    xterm_seq(XTerm_title, str);
# endif
    return 0;
  }
  if ((p = strchr(geom, ';')) == NULL)
    p = strchr(geom, '\0');
  n = (p - geom);
  if (n > GEOM_LEN - 1)
    return 0;

  strncpy(str, geom, n);
  str[n] = '\0';

  flags = XParseGeometry(str, &x, &y, &w, &h);

  if (!flags) {
    flags |= WidthValue;	/* default is tile */
    w = 0;
  }
  if (flags & WidthValue) {
    if (!(flags & XValue)) {
      x = 50;
    }
    if (!(flags & HeightValue))
      h = w;

    if (w && !h) {
      w = pmap->w * ((float) w / 100);
      h = pmap->h;
    } else if (h && !w) {
      w = pmap->w;
      h = pmap->h * ((float) h / 100);
    }
    /* Can't get any bigger than fullscreen */
    if (w > scr->width)
      w = scr->width;
    if (h > scr->height)
      h = scr->height;

    if (pmap->w != w) {
      pmap->w = w;
      changed++;
    }
    if (pmap->h != h) {
      pmap->h = h;
      changed++;
    }
  }
  if (!(flags & YValue)) {
    if (flags & XNegative)
      flags |= YNegative;
    y = x;
  }
  if (!(flags & WidthValue) && geom[0] != '=') {
    x += pmap->x;
    y += pmap->y;
  } else {
    if (flags & XNegative)
      x += 100;
    if (flags & YNegative)
      y += 100;
  }

  x = (x <= 0 ? 0 : (x >= 100 ? 100 : x));
  y = (y <= 0 ? 0 : (y >= 100 ? 100 : y));;
  if (pmap->x != x) {
    pmap->x = x;
    changed++;
  }
  if (pmap->y != y) {
    pmap->y = y;
    changed++;
  }
  D_PIXMAP(("scale_pixmap() exiting with pmap.w == %d, pmap.h == %d, pmap.x == %d, pmap.y == %d\n", pmap->w, pmap->h, pmap->x, pmap->y));
  return changed;
}
#  undef GEOM_LEN

void
render_pixmap(Window win, imlib_t image, pixmap_t pmap, int which, renderop_t renderop)
{

  XGCValues gcvalue;
  GC gc;
  unsigned int width = 0;
  unsigned int height = 0;
  float p, incr;
  int xsize, ysize;
  Pixmap pixmap = None;
  unsigned short rendered = 0;

#  ifdef PIXMAP_OFFSET
  static unsigned int last_width = 0, last_height = 0, last_x = 0, last_y = 0;
  int x, y;
  int px, py;
  unsigned int pw, ph, pb, pd;
  Window w;
  Screen *scr;

#   ifdef PIXMAP_SCROLLBAR
  int scr_x, scr_y, scr_w, scr_h;
  Window dummy;

#   endif
#  endif			/* PIXMAP_OFFSET */

  scr = ScreenOfDisplay(Xdisplay, Xscreen);
  if (!scr)
    return;

  if (!image.im
#  ifdef PIXMAP_OFFSET
      && !(Options & Opt_pixmapTrans)
#  endif
      ) {
    D_PIXMAP(("render_pixmap(): no image loaded\n"));

    if (!(background_is_pixmap())) {
      XSetWindowBackground(Xdisplay, win, PixColors[bgColor]);
      XClearWindow(Xdisplay, win);
    }
    return;
  }
  switch (which) {
    case pixmap_bg:
      D_PIXMAP(("render_pixmap(0x%x): rendering pixmap_bg\n", image.im));
      width =
#ifdef PIXMAP_OFFSET
	  Options & Opt_viewport_mode ? scr->width :
#endif
	  TermWin_TotalWidth();
      height =
#ifdef PIXMAP_OFFSET
	  Options & Opt_viewport_mode ? scr->height :
#endif
	  TermWin_TotalHeight();
      break;
    case pixmap_sb:
      if ((scrollbar_is_pixmapped())
	  || (!(Options & Opt_pixmapTrans))) {
	D_PIXMAP(("render_pixmap(): rendering pixmap_sb\n"));
	width = scrollbar_total_width();
	height = TermWin.height;
      }
      break;
    case pixmap_sa:
      if (scrollbar_is_pixmapped()) {
	D_PIXMAP(("render_pixmap(): rendering pixmap_sa\n"));
	width = scrollbar_total_width();
	height = scrollbar_anchor_max_height();
      }
      break;
    case pixmap_saclk:
      if (scrollbar_is_pixmapped()) {
	D_PIXMAP(("render_pixmap(): rendering pixmap_saclk\n"));
	width = scrollbar_total_width();
	height = scrollbar_anchor_max_height();
      }
      break;
    case pixmap_up:
      if (scrollbar_is_pixmapped()) {
	D_PIXMAP(("render_pixmap(): rendering pixmap_up\n"));
	width = scrollbar_total_width();
	height = scrollbar_arrow_height();
      }
      break;
    case pixmap_upclk:
      if (scrollbar_is_pixmapped()) {
	D_PIXMAP(("render_pixmap(): rendering pixmap_upclk\n"));
	width = scrollbar_total_width();
	height = scrollbar_arrow_height();
      }
      break;
    case pixmap_dn:
      if (scrollbar_is_pixmapped()) {
	D_PIXMAP(("render_pixmap(): rendering pixmap_dn\n"));
	width = scrollbar_total_width();
	height = scrollbar_arrow_height();
      }
      break;
    case pixmap_dnclk:
      if (scrollbar_is_pixmapped()) {
	D_PIXMAP(("render_pixmap(): rendering pixmap_dnclk\n"));
	width = scrollbar_total_width();
	height = scrollbar_arrow_height();
      }
      break;
    case pixmap_mb:
      if (menubar_is_pixmapped()) {
	D_PIXMAP(("render_pixmap(): rendering pixmap_mb\n"));
	width = menuBar_TotalHeight();
	height = menuBar_TotalHeight();
      }
      break;
    default:
      D_PIXMAP(("render_pixmap(): nothing to render\n"));
      return;
  }

  if (!(width) || !(height))
    return;

  gcvalue.foreground = PixColors[bgColor];
  gc = XCreateGC(Xdisplay, win, GCForeground, &gcvalue);

#  if defined(PIXMAP_OFFSET)
  if (Options & Opt_pixmapTrans) {

    if (desktop_window == None) {
      get_desktop_window();
    }
    if (desktop_window == None) {
      print_error("Unable to locate desktop window.  If you are running Enlightenment, please\n"
		  "restart.  If not, please set your background image with Esetroot, then try again.");
      Options &= ~(Opt_pixmapTrans);
      render_pixmap(win, image, pmap, which, renderop);
      return;
    }
    if (desktop_pixmap == None) {
      desktop_pixmap = get_desktop_pixmap();
      last_x = last_y = -1;
      if (desktop_pixmap != None) {
#ifdef IMLIB_TRANS
	if (imlib_bg.im != NULL) {
	  D_IMLIB(("ImlibDestroyImage()\n"));
	  Imlib_kill_image(imlib_id, imlib_bg.im);	/* No sense in caching transparency stuff */
	  imlib_bg.im = NULL;
	}
	imlib_bg.im = Imlib_create_image_from_drawable(imlib_id, desktop_pixmap, None, 0, 0, scr->width, scr->height);
	colormod_trans(imlib_bg, gc, scr->width, scr->height);
	D_IMLIB(("ImlibRender(0x%x@%dx%d)\n", imlib_bg.im, scr->width, scr->height));
	ImlibRender(imlib_id, imlib_bg.im, scr->width, scr->height);
	desktop_pixmap = Imlib_move_image(imlib_id, imlib_bg.im);
	Imlib_kill_image(imlib_id, imlib_bg.im);	/* No sense in caching transparency stuff */
	imlib_bg.im = NULL;
#else
	pixmap = desktop_pixmap;
	XGetGeometry(Xdisplay, desktop_pixmap, &w, &px, &py, &pw, &ph, &pb, &pd);
	if (pw < scr->width || ph < scr->height) {
	  desktop_pixmap = XCreatePixmap(Xdisplay, win, pw, ph, Xdepth);
	  XCopyArea(Xdisplay, pixmap, desktop_pixmap, gc, 0, 0, pw, ph, 0, 0);
	  colormod_trans(desktop_pixmap, gc, pw, ph);
	} else {
	  desktop_pixmap = XCreatePixmap(Xdisplay, win, scr->width, scr->height, Xdepth);
	  XCopyArea(Xdisplay, pixmap, desktop_pixmap, gc, 0, 0, scr->width, scr->height, 0, 0);
	  colormod_trans(desktop_pixmap, gc, scr->width, scr->height);
	}
#endif
      }
    }
    if (desktop_pixmap != None) {
      XTranslateCoordinates(Xdisplay, win, desktop_window, 0, 0, &x, &y, &w);
      if (width != last_width || height != last_height || x != last_x || y != last_y) {
	if (TermWin.pixmap != None) {
	  XFreePixmap(Xdisplay, TermWin.pixmap);
	}
	TermWin.pixmap = XCreatePixmap(Xdisplay, win, width, height, Xdepth);
	D_PIXMAP(("desktop_pixmap == %08x, TermWin.pixmap == %08x\n", desktop_pixmap, TermWin.pixmap));
	if (TermWin.pixmap != None) {
	  XGetGeometry(Xdisplay, desktop_pixmap, &w, &px, &py, &pw, &ph, &pb, &pd);
	  if (pw < scr->width || ph < scr->height) {
	    XFreeGC(Xdisplay, gc);
	    gc = XCreateGC(Xdisplay, desktop_pixmap, 0, &gcvalue);
	    XSetTile(Xdisplay, gc, desktop_pixmap);
	    XSetTSOrigin(Xdisplay, gc, pw - (x % pw), ph - (y % ph));
	    XSetFillStyle(Xdisplay, gc, FillTiled);
	    XFillRectangle(Xdisplay, TermWin.pixmap, gc, 0, 0, scr->width, scr->height);
	  } else {
	    XCopyArea(Xdisplay, desktop_pixmap, TermWin.pixmap, gc, x, y, width, height, 0, 0);
	  }
	}
      }
      last_x = x;
      last_y = y;
    } else {
      last_x = last_y = -1;
    }
    if (TermWin.pixmap != None) {
      pmap.pixmap = TermWin.pixmap;
    }
    last_width = width;
    last_height = height;
#   ifdef PIXMAP_SCROLLBAR
    if (scrollbar_visible() && Options & Opt_scrollBar_floating) {
      scr_w = scrollbar_total_width();
      scr_h = height;
      if (desktop_pixmap != None) {
	sbPixmap.pixmap = XCreatePixmap(Xdisplay, TermWin.parent, scr_w, scr_h, Xdepth);
	D_PIXMAP(("0x%x = XCreatePixmap(%d, %d)\n", sbPixmap.pixmap, scr_w, scr_h));
	XTranslateCoordinates(Xdisplay, scrollBar.win, desktop_window, 0, 0, &scr_x, &scr_y, &dummy);

	XGetGeometry(Xdisplay, desktop_pixmap, &w, &px, &py, &pw, &ph, &pb, &pd);
	XFreeGC(Xdisplay, gc);
	gc = XCreateGC(Xdisplay, desktop_pixmap, 0, &gcvalue);
	XSetTile(Xdisplay, gc, desktop_pixmap);
	XSetTSOrigin(Xdisplay, gc, pw - (scr_x % pw), ph - (scr_y % ph));
	XSetFillStyle(Xdisplay, gc, FillTiled);
	XFillRectangle(Xdisplay, sbPixmap.pixmap, gc, 0, 0, scr_w, scr_h);

	D_PIXMAP(("XSetWindowBackgroundPixmap(sbPixmap.pixmap)\n"));
      }
      XSetWindowBackgroundPixmap(Xdisplay, scrollBar.win, sbPixmap.pixmap);
      D_PIXMAP(("XFreePixmap(sbPixmap.pixmap)\n"));
      /*XFreePixmap(Xdisplay, sbPixmap.pixmap); */
      XClearWindow(Xdisplay, scrollBar.win);
    }
#   endif			/* PIXMAP_SCROLLBAR */
  } else
#  endif
  {

#  ifdef PIXMAP_OFFSET
    last_width = last_height = last_x = last_y = 0;
#  endif

    if (image.im) {
      int w = pmap.w;
      int h = pmap.h;
      int x = pmap.x;
      int y = pmap.y;

      xsize = image.im->rgb_width;
      ysize = image.im->rgb_height;

      D_PIXMAP(("render_pixmap():  w == %d, h == %d, x == %d, y == %d\n", w, h, x, y));

      /* Don't tile too big or scale too small */
      if (w > scr->width || h > scr->height) {
	w = 1;			/* scale to 100% */
      } else if (width > (10 * xsize) || height > (10 * ysize)) {
	w = 0;			/* tile */
      }
#  ifdef PIXMAP_OFFSET
      if (Options & Opt_viewport_mode) {
	D_PIXMAP(("Viewport mode enabled.  viewport_pixmap == 0x%08x and TermWin.pixmap == 0x%08x\n",
		  viewport_pixmap, TermWin.pixmap));
	if (viewport_pixmap == None) {
	  if (w) {
	    D_PIXMAP(("Scaling image to %dx%d\n", scr->width, scr->height));
	    colormod_pixmap(image, gc, scr->width, scr->height);
	    Imlib_render(imlib_id, image.im, scr->width, scr->height);
	  } else {
	    D_PIXMAP(("Tiling image at %dx%d\n", xsize, ysize));
	    colormod_pixmap(image, gc, xsize, ysize);
	    Imlib_render(imlib_id, image.im, xsize, ysize);
	  }
	  viewport_pixmap = Imlib_copy_image(imlib_id, image.im);
	}
	if (TermWin.pixmap != None) {
	  XGetGeometry(Xdisplay, TermWin.pixmap, &dummy, &px, &py, &pw, &ph, &pb, &pd);
	  if (pw != TermWin_TotalWidth() || ph != TermWin_TotalHeight()) {
	    XFreePixmap(Xdisplay, TermWin.pixmap);
	    TermWin.pixmap = None;
	  }
	}
	if (TermWin.pixmap == None) {
	  TermWin.pixmap = XCreatePixmap(Xdisplay, TermWin.vt, TermWin_TotalWidth(), TermWin_TotalHeight(), Xdepth);
	  D_PIXMAP(("Created pixmap == 0x%08x and TermWin.pixmap == 0x%08x\n", viewport_pixmap, TermWin.pixmap));
	}
	XTranslateCoordinates(Xdisplay, win, Xroot, 0, 0, &x, &y, &dummy);
	D_PIXMAP(("Translated coords are %d, %d\n", x, y));
	if (w) {
	  XCopyArea(Xdisplay, viewport_pixmap, TermWin.pixmap, gc, x, y, TermWin_TotalWidth(), TermWin_TotalHeight(), 0, 0);
	} else {
	  XFreeGC(Xdisplay, gc);
	  gc = XCreateGC(Xdisplay, viewport_pixmap, 0, &gcvalue);
	  XSetTile(Xdisplay, gc, viewport_pixmap);
	  XSetTSOrigin(Xdisplay, gc, xsize - (x % xsize), ysize - (y % ysize));
	  XSetFillStyle(Xdisplay, gc, FillTiled);
	  XFillRectangle(Xdisplay, TermWin.pixmap, gc, 0, 0, TermWin_TotalWidth(), TermWin_TotalHeight());
	}
	pmap.pixmap = TermWin.pixmap;
      } else
#  endif

      if (w) {

	/*
	 * horizontal scaling
	 */

	D_PIXMAP(("render_pixmap(): horizontal scaling\n"));

	incr = (float) xsize;

	p = 0;

	if (w == 1) {
	  /* display image directly - no scaling at all */
	  incr = width;

	  D_PIXMAP(("render_pixmap(): no horizontal scaling\n"));

	  if (xsize <= width) {
	    w = xsize;
	    x = (width - w) / 2;
	    w += x;
	  } else {
	    x = 0;
	    w = width;
	  }
	} else if (w < 10) {
	  incr *= w;		/* fit W images across screen */
	  x = 0;
	  w = width;
	} else {
	  incr *= 100.0 / w;
	  /* contract */
	  if (w < 100) {
	    w = (w * width) / 100;
	    /* position */
	    if (x >= 0) {
	      float pos;

	      pos = (float) x / 100 * width - (w / 2);

	      x = (width - w);
	      if (pos <= 0)
		x = 0;
	      else if (pos < x)
		x = pos;
	    } else {
	      x = (width - w) / 2;
	    }
	    w += x;
	  } else if (w >= 100) {
	    /* expand
	     */
	    /* position */
	    if (x > 0) {
	      float pos;

	      pos = (float) x / 100 * xsize - (incr / 2);

	      p = xsize - (incr);
	      if (pos <= 0)
		p = 0;
	      else if (pos < p)
		p = pos;
	    }
	    x = 0;
	    w = width;
	  }
	}
	incr /= width;

	/*
	 * vertical scaling
	 */

	D_PIXMAP(("render_pixmap(): vertical scaling\n"));

	incr = (float) ysize;
	p = 0;

	if (h == 1) {
	  /* display image directly - no scaling at all */
	  incr = height;

	  D_PIXMAP(("render_pixmap(): no vertical scaling\n"));

	  if (ysize <= height) {
	    h = ysize;
	    y = (height - h) / 2;
	    h += y;
	  } else {
	    y = 0;
	    h = height;
	  }
	} else if (h < 10) {
	  incr *= h;		/* fit H images across screen */
	  y = 0;
	  h = height;
	} else {
	  incr *= 100.0 / h;
	  /* contract */
	  if (h < 100) {
	    h = (h * height) / 100;
	    /* position */
	    if (y >= 0) {
	      float pos;

	      pos = (float) y / 100 * height - (h / 2);

	      y = (height - h);
	      if (pos < 0.0f)
		y = 0;
	      else if (pos < y)
		y = pos;
	    } else {
	      y = (height - h) / 2;
	    }
	    h += y;
	  } else if (h >= 100) {
	    /* expand
	     */
	    /* position */
	    if (y > 0) {
	      float pos;

	      pos = (float) y / 100 * ysize - (incr / 2);

	      p = ysize - (incr);
	      if (pos < 0)
		p = 0;
	      else if (pos < p)
		p = pos;
	    }
	    y = 0;
	    h = height;
	  }
	}
	incr /= height;

	image.last_w = w;
	image.last_h = h;

	D_IMLIB(("ImlibRender(0x%x@%dx%d)\n", image.im, w, h));
	colormod_pixmap(image, gc, w, h);
	ImlibRender(imlib_id, image.im, w, h);

	D_IMLIB(("pmap.pixmap = ImlibCopyImageToPixmap(0x%x)\n", image.im));
	pmap.pixmap = ImlibCopyImageToPixmap(imlib_id, image.im);

      } else {
	/* if (w), light years above. -vendu */
	/* tiled */

	D_PIXMAP(("render_pixmap(): tiling pixmap\n"));

	if (!((Options & Opt_pixmapTrans) && rendered)) {
	  D_IMLIB(("ImlibRender(0x%x@%dx%d)\n", image.im, xsize, ysize));
	  colormod_pixmap(image, gc, xsize, ysize);
	  ImlibRender(imlib_id, image.im, xsize, ysize);
	  rendered = 1;
	}
	D_IMLIB(("pixmap = ImlibCopyImageToPixmap()\n"));
	pixmap = ImlibCopyImageToPixmap(imlib_id, image.im);

#  ifdef PIXMAP_OFFSET
	if (Options & Opt_pixmapTrans) {
	  D_IMLIB(("ImlibFreePixmap(0x%x)\n", pixmap));
	  ImlibFreePixmap(imlib_id, pixmap);
	  pixmap = None;
	} else
#  endif
	{
	  D_PIXMAP(("XCreatePixmap(pmap.pixmap(%d,%d))\n", width, height));
	  pmap.pixmap = XCreatePixmap(Xdisplay, win,
				      width, height, Xdepth);

	  for (y = 0; y < height; y += ysize) {
	    unsigned int h = (height - y);

	    if (h > ysize)
	      h = ysize;

	    for (x = 0; x < width; x += xsize) {
	      unsigned int w = (width - x);

	      if (w > xsize)
		w = xsize;
	      D_PIXMAP(("XCopyArea(pixmap(%dx%d)->pmap.pixmap)\n", w, h));
	      XCopyArea(Xdisplay, pixmap, pmap.pixmap, gc, 0, 0, w, h, x, y);
	    }
	  }

	  D_IMLIB(("ImlibFreePixmap(0x%x)\n", pixmap));
	  ImlibFreePixmap(imlib_id, pixmap);
	  pixmap = None;
	}
      }
    } else
      XFillRectangle(Xdisplay, pixmap, gc, 0, 0, width, height);
  }
  if (pmap.pixmap != None) {
    D_PIXMAP(("XSetWindowBackgroundPixmap(pmap.pixmap)\n"));
    XSetWindowBackgroundPixmap(Xdisplay, win, pmap.pixmap);
    if (imlib_id
#ifndef IMLIB_TRANS
	&& !(Options & Opt_pixmapTrans || Options & Opt_viewport_mode)
#endif
	) {
      D_IMLIB(("ImlibFreePixmap(pmap.pixmap)\n"));
      ImlibFreePixmap(imlib_id, pmap.pixmap);
      pmap.pixmap = None;
    }
  }
  XSync(Xdisplay, 0);
  XFreeGC(Xdisplay, gc);
  XClearWindow(Xdisplay, win);
  XFlush(Xdisplay);
  XSync(Xdisplay, 0);
}
# else				/* PIXMAP_SUPPORT */
#  define scale_pixmap(str,p) ((void)0)
#  define render_pixmap() ((void)0)
# endif				/* PIXMAP_SUPPORT */

# if defined(PIXMAP_SUPPORT) || (MENUBAR_MAX)
/*
 * search for FILE in the current working directory, and within the
 * colon-delimited PATHLIST, adding the file extension EXT if required.
 *
 * FILE is either @ or zero terminated
 */
const char *
search_path(const char *pathlist, const char *file, const char *ext)
{
  /* FIXME: the 256 below should be changed to some #define in <dirent.h> */
  static char name[256];
  char *p;
  const char *path;
  int maxpath, len;
  struct stat fst;

  if (!pathlist || !file) {	/* If either one is NULL, there really isn't much point in going on.... */
    return ((const char *) NULL);
  }
  if (!ext) {
    ext = "";
  }
  D_OPTIONS(("search_path(\"%s\", \"%s\", \"%s\") called.\n", pathlist, file, ext));
  D_OPTIONS(("search_path():  Checking for file \"%s\"\n", file));
  if (!access(file, R_OK)) {
    if (stat(file, &fst)) {
      D_OPTIONS(("Unable to stat %s -- %s\n", file, strerror(errno)));
    } else {
      D_OPTIONS(("Stat returned mode 0x%08o, S_ISDIR() == %d\n", fst.st_mode, S_ISDIR(fst.st_mode)));
    }
    if (!S_ISDIR(fst.st_mode))
      return file;
  }
  /* Changed to use '@' as the delimiter. -vendu */
  if ((p = strchr(file, '@')) == NULL)
    p = strchr(file, '\0');
  len = (p - file);

  /* check about adding a trailing extension */
  if (ext != NULL) {

    char *dot;

    dot = strrchr(p, '.');
    path = strrchr(p, '/');
    if (dot != NULL || (path != NULL && dot <= path))
      ext = NULL;
  }
  /* leave room for an extra '/' and trailing '\0' */
  maxpath = sizeof(name) - (len + (ext ? strlen(ext) : 0) + 2);
  if (maxpath <= 0)
    return NULL;

  /* check if we can find it now */
  strncpy(name, file, len);
  name[len] = '\0';

  D_OPTIONS(("search_path():  Checking for file \"%s\"\n", name));
  if (!access(name, R_OK)) {
    stat(name, &fst);
    if (!S_ISDIR(fst.st_mode))
      return name;
  }
  if (ext) {
    strcat(name, ext);
    D_OPTIONS(("search_path():  Checking for file \"%s\"\n", name));
    if (!access(name, R_OK)) {
      stat(name, &fst);
      if (!S_ISDIR(fst.st_mode))
	return name;
    }
  }
  for (path = pathlist; path != NULL && *path != '\0'; path = p) {

    int n;

    /* colon delimited */
    if ((p = strchr(path, ':')) == NULL)
      p = strchr(path, '\0');

    n = (p - path);
    if (*p != '\0')
      p++;

    if (n > 0 && n <= maxpath) {

      strncpy(name, path, n);
      if (name[n - 1] != '/')
	name[n++] = '/';
      name[n] = '\0';
      strncat(name, file, len);

      D_OPTIONS(("search_path():  Checking for file \"%s\"\n", name));
      if (!access(name, R_OK)) {
	stat(name, &fst);
	if (!S_ISDIR(fst.st_mode))
	  return name;
      }
      if (ext) {
	strcat(name, ext);
	D_OPTIONS(("search_path():  Checking for file \"%s\"\n", name));
	if (!access(name, R_OK)) {
	  stat(name, &fst);
	  if (!S_ISDIR(fst.st_mode))
	    return name;
	}
      }
    }
  }
  return NULL;
}
# endif				/* PIXMAP_SUPPORT || (MENUBAR_MAX) */

# ifdef PIXMAP_SUPPORT
/* I think it might be cool to make Eterm load the pixmaps in background.
 * You'd be able to start typing commands without waiting for the bg
 * pixmap processing to end. Thoughts right now: fork(), pthreads. -vendu
 */

void
set_bgPixmap(const char *file)
{
  const char *f = NULL;

  ASSERT(file != NULL);

  if (!file) {
    return;
  }
  D_IMLIB(("set_bgPixmap(%s)\n", file));

  /* Turn on scaling */
  if ((Options & Opt_pixmapScale) || (rs_pixmapScale)) {
    bgPixmap.h = 100;
    bgPixmap.w = 100;
  }
  /*    XSetWindowBackground(Xdisplay, TermWin.vt, PixColors[bgColor]); */

  if (*file != '\0') {
    /*      XGetWindowAttributes(Xdisplay, TermWin.vt, &attr); */

    /* search environment variables here too */
#  ifdef USE_IMLIB
    if ((f = search_path(rs_path, file, PIXMAP_EXT)) == NULL)
#  endif
#  ifdef PATH_ENV
#   ifdef USE_IMLIB
      if ((f = search_path(getenv(PATH_ENV), file, PIXMAP_EXT)) == NULL)
#   endif
#  endif			/* PATH_ENV */
#  ifdef USE_IMLIB
	f = search_path(getenv("PATH"), file, PIXMAP_EXT);
#  endif

#  ifdef USE_IMLIB
    if (f != NULL) {
      rs_pixmaps[pixmap_bg] = strdup(f);
      if (imlib_bg.im != NULL) {
	D_IMLIB(("ImlibDestroyImage()\n"));
	ImlibDestroyImage(imlib_id, imlib_bg.im);
	imlib_bg.im = NULL;
      }
      D_IMLIB(("ReadImageViaImlib(%s)\n", (char *) f));
      imlib_bg.im = ReadImageViaImlib(Xdisplay, (char *) f);
    }
    if (imlib_bg.im == NULL) {
#  endif
      char *p;

      if ((p = strchr(file, ';')) == NULL && (p = strchr(file, '@')) == NULL)
	p = strchr(file, '\0');
      print_error("couldn't load image file \"%.*s\"", (p - file), file);
      if (!(background_is_pixmap())) {
	XSetWindowBackground(Xdisplay, TermWin.vt, PixColors[bgColor]);
      }
#  ifdef USE_POSIX_THREADS
      XSetWindowBackground(Xdisplay, TermWin.vt, PixColors[bgColor]);
#  endif
    } else {
#  ifdef USE_POSIX_THREADS
      /* FIXME: the if tests should be removed and done once at the
       * else above.
       */
      if (bg_needs_update) {
	D_THREADS(("pthread_attr_init(resize_sub_thr_attr)\n"));
	pthread_attr_init(&resize_sub_thr_attr);
#   ifdef MUTEX_SYNCH
	if (pthread_mutex_trylock(&mutex) == EBUSY) {
	  D_THREADS(("set_bgPixmap(): pthread_cancel(resize_sub_thr);\n"));
	  pthread_cancel(resize_sub_thr);
	} else {
	  D_THREADS(("pthread_mutex_trylock(&mutex): "));
	  pthread_mutex_unlock(&mutex);
	  D_THREADS(("pthread_mutex_unlock(&mutex)\n"));
	}
#   else
	if (bg_set) {
	  D_THREADS(("Background set, cancelling thread\n"));
	  pthread_cancel(resize_sub_thr);
	}
#   endif

	if (!(pthread_create(&resize_sub_thr, &resize_sub_thr_attr,
			     (void *) &render_bg_thread, NULL))) {
	  bg_set = 0;
#   ifdef MUTEX_SYNCH
	  /*          D_THREADS("pthread_mutex_lock(&mutex);\n"); */
	  /*          pthread_mutex_lock(&mutex); */
#   endif
	  D_THREADS(("thread created\n"));
	} else {
	  D_THREADS(("pthread_create() failed!\n"));
	}
      }
#  else

#   ifdef PIXMAP_OFFSET
      if (Options & Opt_viewport_mode) {
	if (viewport_pixmap != None) {
	  XFreePixmap(Xdisplay, viewport_pixmap);
	  viewport_pixmap = None;
	  bg_needs_update = 1;
	}
      }
#   endif
      if (bg_needs_update) {
	D_PIXMAP(("set_bgPixmap(): render_pixmap(TermWin.vt), case 2\n"));
	render_pixmap(TermWin.vt, imlib_bg, bgPixmap, 0, 1);
	scr_touch();
	bg_needs_update = 0;
      }
#  endif
    }

    /*      init_imlib = 1; */
    /*      pthread_join(resize_sub_thr, NULL); */
    D_PIXMAP(("set_bgPixmap() exitting\n"));
  }
#  ifdef IMLIB_TRANS
  else if (Options & Opt_pixmapTrans) {
    render_pixmap(TermWin.vt, imlib_bg, bgPixmap, 0, 1);
    scr_touch();
    bg_needs_update = 0;
  }
#  endif

  if (!f || *f == '\0') {
    if (imlib_bg.im != NULL) {
      D_IMLIB(("ImlibDestroyImage()\n"));
      ImlibDestroyImage(imlib_id, imlib_bg.im);
      imlib_bg.im = NULL;
    }
    XSetWindowBackground(Xdisplay, TermWin.vt, PixColors[bgColor]);
    XClearWindow(Xdisplay, TermWin.vt);
    scr_touch();
    XFlush(Xdisplay);
  }
}

#  ifdef PIXMAP_SCROLLBAR
void
set_Pixmap(const char *file, Pixmap dest_pmap, int type)
{
  const char *f;
  imlib_t img;

  /* FIXME: assert() looks like a bad thing. Calls abort(). IMHO, stupid. */
  assert(file != NULL);

  D_IMLIB(("set_Pixmap(%s)\n", file));

  if (*file != '\0') {
    /* search environment variables here too */
#   ifdef USE_IMLIB
    if ((f = search_path(rs_path, file, PIXMAP_EXT)) == NULL)
#   endif
#   ifdef PATH_ENV
#    ifdef USE_IMLIB
      if ((f = search_path(getenv(PATH_ENV), file, PIXMAP_EXT)) == NULL)
#    endif
#   endif			/* PATH_ENV */
#   ifdef USE_IMLIB
	f = search_path(getenv("PATH"), file, PIXMAP_EXT);
#   endif

#   ifdef USE_IMLIB
    if (f != NULL) {
      D_IMLIB(("ReadImageViaImlib(%s)\n", (char *) f));
      img.im = ReadImageViaImlib(Xdisplay, (char *) f);
    }
    if (img.im == NULL) {
#   endif
      char *p;

      if ((p = strchr(file, ';')) == NULL)
	p = strchr(file, '\0');
      print_error("couldn't load image file \"%.*s\"", (p - file), file);
    } else {
      switch (type) {
	case pixmap_sb:
	  render_pixmap(scrollBar.win, img, sbPixmap, pixmap_sb, 0);
	  break;
	case pixmap_sa:
	  render_pixmap(scrollBar.sa_win, img, saPixmap, pixmap_sa, 0);
	  break;
	case pixmap_saclk:
	  render_pixmap(scrollBar.sa_win, img, sa_clkPixmap, pixmap_saclk, 0);
	  break;
	case pixmap_up:
	  render_pixmap(scrollBar.up_win, img, upPixmap, pixmap_up, 0);
	  break;
	case pixmap_upclk:
	  render_pixmap(scrollBar.up_win, img, up_clkPixmap, pixmap_upclk, 0);
	  break;
	case pixmap_dn:
	  render_pixmap(scrollBar.dn_win, img, dnPixmap, pixmap_dn, 0);
	  break;
	case pixmap_dnclk:
	  render_pixmap(scrollBar.dn_win, img, dn_clkPixmap, pixmap_dnclk, 0);
	  break;
	case pixmap_mb:
/*                  render_pixmap(ActiveMenu->win, img, mbPixmap, pixmap_mb, 0); */
	  break;
	case pixmap_ms:
/*              render_pixmap(ActiveMenu->win, img, mb_selPixmap, pixmap_ms, 0); */
	  break;
	default:
	  D_PIXMAP(("WARNING: set_Pixmap() returning\n"));
	  return;
      }
    }
  }
  D_PIXMAP(("set_scrPixmap() exitting\n"));
}
#  endif			/* PIXMAP_SCROLLBAR */

void
colormod_pixmap(imlib_t image, GC gc, int w, int h)
{

  unsigned int bright;
  ImlibColorModifier xform =
  {0xff, 0xff, 0xff}, rx =
  {0xff, 0xff, 0xff}, gx =
  {0xff, 0xff, 0xff}, bx =
  {0xff, 0xff, 0xff};

  if (rs_shadePct == 0 && rs_tintMask == 0xffffff) {
    Imlib_set_image_modifier(imlib_id, image.im, &xform);
    Imlib_set_image_red_modifier(imlib_id, image.im, &rx);
    Imlib_set_image_green_modifier(imlib_id, image.im, &gx);
    Imlib_set_image_blue_modifier(imlib_id, image.im, &bx);
    return;
  }
  if (rs_shadePct != 0) {
    bright = 0xff - ((rs_shadePct * 0xff) / 100);
    xform.brightness = bright;
    Imlib_set_image_modifier(imlib_id, image.im, &xform);
  } else {
    Imlib_set_image_modifier(imlib_id, image.im, &xform);
  }

  if (rs_tintMask != 0xffffff) {
    rx.brightness = (rs_tintMask & 0xff0000) >> 16;
    gx.brightness = (rs_tintMask & 0x00ff00) >> 8;
    bx.brightness = rs_tintMask & 0x0000ff;
    Imlib_set_image_red_modifier(imlib_id, image.im, &rx);
    Imlib_set_image_green_modifier(imlib_id, image.im, &gx);
    Imlib_set_image_blue_modifier(imlib_id, image.im, &bx);
  } else {
    Imlib_set_image_red_modifier(imlib_id, image.im, &rx);
    Imlib_set_image_green_modifier(imlib_id, image.im, &gx);
    Imlib_set_image_blue_modifier(imlib_id, image.im, &bx);
  }

  D_PIXMAP(("Image modifiers:  xform == %08x, rx == %08x, gx == %08x, bx == %08x\n",
	    xform.brightness, rx.brightness, gx.brightness, bx.brightness));
}

#  ifdef USE_IMLIB
#   undef PIXMAP_EXT
#  endif

#  ifdef PIXMAP_OFFSET

#   ifdef IMLIB_TRANS
void
colormod_trans(imlib_t image, GC gc, int w, int h)
{

  unsigned int bright;
  ImlibColorModifier xform =
  {0xff, 0xff, 0xff}, rx =
  {0xff, 0xff, 0xff}, gx =
  {0xff, 0xff, 0xff}, bx =
  {0xff, 0xff, 0xff};

  if (rs_shadePct == 0 && rs_tintMask == 0xffffff) {
    return;
  }
  if (rs_shadePct != 0) {
    bright = 0xff - ((rs_shadePct * 0xff) / 100);
    xform.brightness = bright;
    Imlib_set_image_modifier(imlib_id, image.im, &xform);
  }
  if (rs_tintMask != 0xffffff) {
    rx.brightness = (rs_tintMask & 0xff0000) >> 16;
    gx.brightness = (rs_tintMask & 0x00ff00) >> 8;
    bx.brightness = rs_tintMask & 0x0000ff;
    Imlib_set_image_red_modifier(imlib_id, image.im, &rx);
    Imlib_set_image_green_modifier(imlib_id, image.im, &gx);
    Imlib_set_image_blue_modifier(imlib_id, image.im, &bx);
  }
  D_PIXMAP(("Image modifiers:  xform == %08x, rx == %08x, gx == %08x, bx == %08x\n",
	    xform.brightness, rx.brightness, gx.brightness, bx.brightness));
}

#    else			/* IMLIB_TRANS */

void
colormod_trans(Pixmap p, GC gc, int w, int h)
{

  XImage *ximg;
  register unsigned long v, i;
  unsigned long x, y;
  unsigned int r, g, b;
  float rm, gm, bm, shade;
  ImlibColor ctab[256];
  int real_depth = 0;
  register int br, bg, bb;
  register unsigned int mr, mg, mb;

  if (rs_shadePct == 0 && rs_tintMask == 0xffffff) {
    return;
  }
  if (Xdepth <= 8) {

    XColor cols[256];

    for (i = 0; i < (1 << Xdepth); i++) {
      cols[i].pixel = i;
      cols[i].flags = DoRed | DoGreen | DoBlue;
    }
    XQueryColors(Xdisplay, Xcmap, cols, 1 << Xdepth);
    for (i = 0; i < (1 << Xdepth); i++) {
      ctab[i].r = cols[i].red >> 8;
      ctab[i].g = cols[i].green >> 8;
      ctab[i].b = cols[i].blue >> 8;
      ctab[i].pixel = cols[i].pixel;
    }
  } else if (Xdepth == 16) {

    XWindowAttributes xattr;

    XGetWindowAttributes(Xdisplay, desktop_window, &xattr);
    if ((xattr.visual->red_mask == 0x7c00) && (xattr.visual->green_mask == 0x3e0) && (xattr.visual->blue_mask == 0x1f)) {
      real_depth = 15;
    }
  }
  if (!real_depth) {
    real_depth = Xdepth;
  }
  shade = (float) (100 - rs_shadePct) / 100.0;
  rm = (float) ((rs_tintMask & 0xff0000) >> 16) / 255.0 * shade;
  gm = (float) ((rs_tintMask & 0x00ff00) >> 8) / 255.0 * shade;
  bm = (float) (rs_tintMask & 0x0000ff) / 255.0 * shade;

  ximg = XGetImage(Xdisplay, p, 0, 0, w, h, -1, ZPixmap);
  if (ximg == NULL) {
    print_warning("colormod_trans:  XGetImage(Xdisplay, 0x%08x, 0, 0, %d, %d, -1, ZPixmap) returned NULL.",
		  p, w, h);
    return;
  }
  if (Xdepth <= 8) {
    for (y = 0; y < h; y++) {
      for (x = 0; x < w; x++) {
	v = XGetPixel(ximg, x, y);
	r = (int) ctab[v & 0xff].r * rm;
	g = (int) ctab[v & 0xff].g * gm;
	b = (int) ctab[v & 0xff].b * bm;
	v = Imlib_best_color_match(imlib_id, &r, &g, &b);
	XPutPixel(ximg, x, y, v);
      }
    }
  } else {
    /* Determine bitshift and bitmask values */
    switch (real_depth) {
      case 15:
	br = 7;
	bg = 2;
	bb = 3;
	mr = mg = mb = 0xf8;
	break;
      case 16:
	br = 8;
	bg = bb = 3;
	mr = mb = 0xf8;
	mg = 0xfc;
	break;
      case 24:
      case 32:
	br = 16;
	bg = 8;
	bb = 0;
	mr = mg = mb = 0xff;
	break;
      default:
	print_warning("colormod_trans:  Bit depth of %d is unsupported for tinting/shading.", real_depth);
	return;
    }

    for (y = 0; y < h; y++) {
      for (x = 0; x < w; x++) {
	v = XGetPixel(ximg, x, y);
	r = (int) (((v >> br) & mr) * rm) & 0xff;
	g = (int) (((v >> bg) & mg) * gm) & 0xff;
	b = (int) (((v << bb) & mb) * bm) & 0xff;
	v = ((r & mr) << br) | ((g & mg) << bg) | ((b & mb) >> bb);
	XPutPixel(ximg, x, y, v);
      }
    }
  }
  XPutImage(Xdisplay, p, gc, ximg, 0, 0, 0, 0, w, h);
  XDestroyImage(ximg);
}

#    endif			/* IMLIB_TRANS */

Window
get_desktop_window(void)
{

  Atom prop, type, prop2;
  int format;
  unsigned long length, after;
  unsigned char *data;
  unsigned int nchildren;
  Window w, root, *children, parent;
  Window last_desktop_window = desktop_window;

  if ((prop = XInternAtom(Xdisplay, "_XROOTPMAP_ID", True)) == None) {
    D_PIXMAP(("No _XROOTPMAP_ID found.\n"));
  }
  if ((prop2 = XInternAtom(Xdisplay, "_XROOTCOLOR_PIXEL", True)) == None) {
    D_PIXMAP(("No _XROOTCOLOR_PIXEL found.\n"));
  }
  if (prop == None && prop2 == None) {
    return None;
  }
#ifdef WATCH_DESKTOP_OPTION
  if (Options & Opt_watchDesktop) {
    if (TermWin.wm_parent != None) {
      XSelectInput(Xdisplay, TermWin.wm_parent, None);
    }
    if (TermWin.wm_grandparent != None) {
      XSelectInput(Xdisplay, TermWin.wm_grandparent, None);
    }
  }
#endif

  for (w = TermWin.parent; w; w = parent) {

    D_PIXMAP(("Current window ID is:  0x%08x\n", w));

    if ((XQueryTree(Xdisplay, w, &root, &parent, &children, &nchildren)) == False) {
      D_PIXMAP(("  Egad!  XQueryTree() returned false!\n"));
      return None;
    }
    D_PIXMAP(("  Window is 0x%08x with %d children, root is 0x%08x, parent is 0x%08x\n",
	      w, nchildren, root, parent));
    if (nchildren) {
      XFree(children);
    }
#ifdef WATCH_DESKTOP_OPTION
    if (Options & Opt_watchDesktop && parent != None) {
      if (w == TermWin.parent) {
	TermWin.wm_parent = parent;
	XSelectInput(Xdisplay, TermWin.wm_parent, StructureNotifyMask);
      } else if (w == TermWin.wm_parent) {
	TermWin.wm_grandparent = parent;
	XSelectInput(Xdisplay, TermWin.wm_grandparent, StructureNotifyMask);
      }
    }
#endif

    if (prop != None) {
      XGetWindowProperty(Xdisplay, w, prop, 0L, 1L, False, AnyPropertyType,
			 &type, &format, &length, &after, &data);
    } else if (prop2 != None) {
      XGetWindowProperty(Xdisplay, w, prop2, 0L, 1L, False, AnyPropertyType,
			 &type, &format, &length, &after, &data);
    } else {
      continue;
    }
    if (type != None) {
      D_PIXMAP(("  Found desktop as window 0x%08x\n", w));
      return (desktop_window = w);
    }
  }

  D_PIXMAP(("No suitable parent found.\n"));
  return (desktop_window = None);

}

Pixmap
get_desktop_pixmap(void)
{

  Pixmap p;
  Atom prop, type, prop2;
  int format;
  unsigned long length, after;
  unsigned char *data;
  register unsigned long i = 0;
  time_t blah;

  if (desktop_window == None)
    return None;

  prop = XInternAtom(Xdisplay, "_XROOTPMAP_ID", True);
  prop2 = XInternAtom(Xdisplay, "_XROOTCOLOR_PIXEL", True);

  if (prop == None && prop2 == None) {
    return None;
  }
  if (prop != None) {
    XGetWindowProperty(Xdisplay, desktop_window, prop, 0L, 1L, False, AnyPropertyType,
		       &type, &format, &length, &after, &data);
    if (type == XA_PIXMAP) {
      p = *((Pixmap *) data);
      /*
         blah = time(NULL);
         D_PIXMAP(("Time index:  %s\n", ctime(&blah)));
         for (i=0; i < 4000000000 && !p; i++) {
         D_PIXMAP(("  Null pixmap returned.  i == %lu  Trying again in 20 ms.\n", i));
         usleep(2000000);
         XGetWindowProperty(Xdisplay, desktop_window, prop, 0L, 1L, False, AnyPropertyType,
         &type, &format, &length, &after, &data);
         if (type != XA_PIXMAP) {
         p = None;
         break;
         } else {
         p = *((Pixmap *)data);
         }
         }
         blah = time(NULL);
         D_PIXMAP(("Time index:  %s\n", ctime(&blah)));
         if (i != 0 && p != 0) {
         _exit(0);
         }
       */
      D_PIXMAP(("  Found pixmap 0x%08x\n", p));
      return p;
    }
  }
  if (prop2 != None) {
    XGetWindowProperty(Xdisplay, desktop_window, prop2, 0L, 1L, False, AnyPropertyType,
		       &type, &format, &length, &after, &data);
    if (type == XA_CARDINAL) {
      D_PIXMAP(("  Solid color not yet supported.\n"));
      return None;
    }
  }
  D_PIXMAP(("No suitable attribute found.\n"));
  return None;

}
#   endif			/* PIXMAP_OFFSET) */

void
shaped_window_apply_mask(Window win, Pixmap mask)
{

  static signed char have_shape = -1;
  int unused;

  D_PIXMAP(("shaped_window_apply_mask(0x%08x, 0x%08x) called.\n", win, mask));
  if (win == None || mask == None)
    return;

#ifdef HAVE_X_SHAPE_EXT
  if (1 == have_shape) {
    D_PIXMAP(("shaped_window_apply_mask():  Shape extension available, applying mask.\n"));
    XShapeCombineMask(Xdisplay, win, ShapeBounding, 0, 0, mask, ShapeSet);
  } else if (0 == have_shape) {
    D_PIXMAP(("shaped_window_apply_mask():  Shape extension not available.\n"));
    return;
  } else {			/* Don't know yet? */
    D_PIXMAP(("shaped_window_apply_mask():  Looking for shape extension.\n"));
    if (!XQueryExtension(Xdisplay, "SHAPE", &unused, &unused, &unused)) {
      have_shape = 0;
      D_PIXMAP(("shaped_window_apply_mask():  Shape extension not found.\n"));
      return;
    } else {
      have_shape = 1;
      D_PIXMAP(("shaped_window_apply_mask():  Shape extension available, applying mask.\n"));
      XShapeCombineMask(Xdisplay, win, ShapeBounding, 0, 0, mask, ShapeSet);
    }
  }
#else
  D_PIXMAP(("shaped_window_apply_mask():  Shape support disabled.\n"));
#endif
}

void
set_icon_pixmap(char *filename, XWMHints * pwm_hints)
{

  const char *icon_path;
  ImlibImage *temp_im;
  XWMHints *wm_hints;

  if (pwm_hints) {
    wm_hints = pwm_hints;
  } else {
    wm_hints = XGetWMHints(Xdisplay, TermWin.parent);
  }

  if (filename && *filename) {
    if ((icon_path = search_path(rs_path, filename, NULL)) == NULL)
# ifdef PATH_ENV
      if ((icon_path = search_path(getenv(PATH_ENV), filename, NULL)) == NULL)
# endif
	icon_path = search_path(getenv("PATH"), filename, NULL);

    if (icon_path != NULL) {
      XIconSize *icon_sizes;
      int count, i, w = 64, h = 64;

      temp_im = ReadImageViaImlib(Xdisplay, icon_path);
      /* If we're going to render the image anyway, might as well be nice and give it to the WM in a size it likes. */
      if (XGetIconSizes(Xdisplay, Xroot, &icon_sizes, &count)) {
	for (i = 0; i < count; i++) {
	  D_PIXMAP(("Got icon sizes:  Width %d to %d +/- %d, Height %d to %d +/- %d\n", icon_sizes[i].min_width, icon_sizes[i].max_width,
		    icon_sizes[i].width_inc, icon_sizes[i].min_height, icon_sizes[i].max_height, icon_sizes[i].height_inc));
	  w = MIN(icon_sizes[i].max_width, 64);		/* 64x64 is plenty big */
	  h = MIN(icon_sizes[i].max_height, 64);
	}
	fflush(stdout);
	XFree(icon_sizes);
      }
      Imlib_render(imlib_id, temp_im, w, h);
      wm_hints->icon_pixmap = Imlib_copy_image(imlib_id, temp_im);
      wm_hints->icon_mask = Imlib_copy_mask(imlib_id, temp_im);
      wm_hints->icon_window = XCreateSimpleWindow(Xdisplay, TermWin.parent, 0, 0, w, h, 0, 0L, 0L);
      shaped_window_apply_mask(wm_hints->icon_window, wm_hints->icon_mask);
      XSetWindowBackgroundPixmap(Xdisplay, wm_hints->icon_window, wm_hints->icon_pixmap);
      wm_hints->flags |= IconWindowHint;
      Imlib_destroy_image(imlib_id, temp_im);
    }
  } else {
    /* Use the default.  It's 48x48, so if the WM doesn't like it, tough cookies.  Pixmap -> ImlibImage -> Render -> Pixmap would be
       too expensive, IMHO. */
    Imlib_data_to_pixmap(imlib_id, Eterm_xpm, &wm_hints->icon_pixmap, &wm_hints->icon_mask);
    wm_hints->icon_window = XCreateSimpleWindow(Xdisplay, TermWin.parent, 0, 0, 48, 48, 0, 0L, 0L);
    shaped_window_apply_mask(wm_hints->icon_window, wm_hints->icon_mask);
    XSetWindowBackgroundPixmap(Xdisplay, wm_hints->icon_window, wm_hints->icon_pixmap);
    wm_hints->flags |= IconWindowHint;
  }

  /* Only set the hints ourselves if we were passed a NULL pointer for pwm_hints */
  if (!pwm_hints) {
    XSetWMHints(Xdisplay, TermWin.parent, wm_hints);
    XFree(wm_hints);
  }
}

#   ifdef USE_EFFECTS
int
fade_in(Pixmap * pmap, ImlibImage * img, int frames)
{

  static int i = 0;
  register int f = frames;
  ImlibColorModifier mod;
  double gamma, brightness, contrast;

  Imlib_get_image_modifier(imlib_id, img, &mod);

  if (i < f) {
    i++;
    gamma = (double) mod.gamma / i;
    brightness = (double) mod.brightness / i;
    contrast = (double) mod.contrast / i;
    Imlib_set_image_modifier(imlib_id, img, &mod);
  } else if (i == f) {
    i = 0;
  }
  /* how many frames to go */
  return (f - i);
}
#   endif			/* USE_EFFECTS */

#endif /* PIXMAP_SUPPORT */
