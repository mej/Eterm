/* windows.c -- Eterm window handling module

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

static const char cvs_ident[] = "$Id$";

#include "config.h"
#include "feature.h"

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <X11/cursorfont.h>

#include "../libmej/debug.h"
#include "../libmej/mem.h"
#include "../libmej/strings.h"
#include "debug.h"
#include "command.h"
#include "e.h"
#include "events.h"
#include "font.h"
#include "main.h"
#include "menus.h"
#include "options.h"
#include "pixmap.h"
#include "screen.h"
#include "scrollbar.h"
#include "term.h"
#include "windows.h"

XWindowAttributes attr;
XSetWindowAttributes Attributes;
XSizeHints szHint =
{
  PMinSize | PResizeInc | PBaseSize | PWinGravity,
  0, 0, 80, 24,			/* x, y, width, height */
  1, 1,				/* Min width, height */
  0, 0,				/* Max width, height - unused */
  1, 1,				/* increments: width, height */
  {1, 1},			/* increments: x, y */
  {0, 0},			/* Aspect ratio - unused */
  0, 0,				/* base size: width, height */
  NorthWestGravity		/* gravity */
};
Cursor TermWin_cursor;		/* cursor for vt window */

void
set_text_property(Window win, char *propname, char *value)
{
  XTextProperty prop;
  Atom atom;

  ASSERT(propname != NULL);

  if (value == NULL) {
    atom = XInternAtom(Xdisplay, propname, True);
    if (atom == None) {
      return;
    }
    XDeleteProperty(Xdisplay, win, atom);
  } else {
    atom = XInternAtom(Xdisplay, propname, False);
    prop.value = value;
    prop.encoding = XA_STRING;
    prop.format = 8;
    prop.nitems = strlen(value);
    XSetTextProperty(Xdisplay, win, &prop, atom);
  }
}

Pixel
get_bottom_shadow_color(Pixel norm_color, const char *type)
{

  XColor xcol;
  unsigned int r, g, b;

  xcol.pixel = norm_color;
  XQueryColor(Xdisplay, cmap, &xcol);

  xcol.red /= 2;
  xcol.green /= 2;
  xcol.blue /= 2;
  r = xcol.red;
  g = xcol.green;
  b = xcol.blue;
  xcol.pixel = Imlib_best_color_match(imlib_id, &r, &g, &b);

  if (!XAllocColor(Xdisplay, cmap, &xcol)) {
    print_error("Unable to allocate \"%s\" (0x%08x:  0x%04x, 0x%04x, 0x%04x) in the color map.", type, xcol.pixel, r, g, b);
    xcol.pixel = PixColors[minColor];
  }
  return (xcol.pixel);
}

Pixel
get_top_shadow_color(Pixel norm_color, const char *type)
{

  XColor xcol, white;
  unsigned int r, g, b;

# ifdef PREFER_24BIT
  white.red = white.green = white.blue = r = g = b = ~0;
  white.pixel = Imlib_best_color_match(imlib_id, &r, &g, &b);
  XAllocColor(Xdisplay, cmap, &white);
# else
  white.pixel = WhitePixel(Xdisplay, Xscreen);
  XQueryColor(Xdisplay, cmap, &white);
# endif

  xcol.pixel = norm_color;
  XQueryColor(Xdisplay, cmap, &xcol);

  xcol.red = max((white.red / 5), xcol.red);
  xcol.green = max((white.green / 5), xcol.green);
  xcol.blue = max((white.blue / 5), xcol.blue);

  xcol.red = min(white.red, (xcol.red * 7) / 5);
  xcol.green = min(white.green, (xcol.green * 7) / 5);
  xcol.blue = min(white.blue, (xcol.blue * 7) / 5);
  r = xcol.red;
  g = xcol.green;
  b = xcol.blue;
  xcol.pixel = Imlib_best_color_match(imlib_id, &r, &g, &b);

  if (!XAllocColor(Xdisplay, cmap, &xcol)) {
    print_error("Unable to allocate \"%s\" (0x%08x:  0x%04x, 0x%04x, 0x%04x) in the color map.", type, xcol.pixel, r, g, b);
    xcol.pixel = PixColors[WhiteColor];
  }
  return (xcol.pixel);
}

/* Create_Windows() - Open and map the window */
void
Create_Windows(int argc, char *argv[])
{

  Cursor cursor;
  XClassHint classHint;
  XWMHints wmHint;
  Atom prop = None;
  CARD32 val;
  int i, x = 0, y = 0, flags;
  unsigned int width = 0, height = 0;
  unsigned int r, g, b;
  MWMHints mwmhints;

  if (Options & Opt_borderless) {
    prop = XInternAtom(Xdisplay, "_MOTIF_WM_HINTS", True);
    if (prop == None) {
      print_warning("Window Manager does not support MWM hints.  Bypassing window manager control for borderless window.");
      Attributes.override_redirect = TRUE;
      mwmhints.flags = 0;
    } else {
      mwmhints.flags = MWM_HINTS_DECORATIONS;
      mwmhints.decorations = 0;
    }
  } else {
    mwmhints.flags = 0;
  }
  Attributes.backing_store = WhenMapped;
  Attributes.colormap = cmap;

  /*
   * grab colors before netscape does
   */
  for (i = 0; i < (Xdepth <= 2 ? 2 : NRS_COLORS); i++) {

    XColor xcol;
    unsigned char found_color = 0;

    if (rs_color[i]) {
      if (!XParseColor(Xdisplay, cmap, rs_color[i], &xcol)) {
	print_warning("Unable to resolve \"%s\" as a color name.  Falling back on \"%s\".",
		      rs_color[i], def_colorName[i] ? def_colorName[i] : "(nil)");
	rs_color[i] = def_colorName[i];
	if (rs_color[i]) {
	  if (!XParseColor(Xdisplay, cmap, rs_color[i], &xcol)) {
	    print_warning("Unable to resolve \"%s\" as a color name.  This should never fail.  Please repair/restore your RGB database.", rs_color[i]);
	    found_color = 0;
	  } else {
	    found_color = 1;
	  }
	}
      } else {
	found_color = 1;
      }
      if (found_color) {
	r = xcol.red;
	g = xcol.green;
	b = xcol.blue;
	xcol.pixel = Imlib_best_color_match(imlib_id, &r, &g, &b);
	if (!XAllocColor(Xdisplay, cmap, &xcol)) {
	  print_warning("Unable to allocate \"%s\" (0x%08x:  0x%04x, 0x%04x, 0x%04x) in the color map.  "
			"Falling back on \"%s\".",
			rs_color[i], xcol.pixel, r, g, b, def_colorName[i] ? def_colorName[i] : "(nil)");
	  rs_color[i] = def_colorName[i];
	  if (rs_color[i]) {
	    if (!XAllocColor(Xdisplay, cmap, &xcol)) {
	      print_warning("Unable to allocate \"%s\" (0x%08x:  0x%04x, 0x%04x, 0x%04x) in the color map.",
			    rs_color[i], xcol.pixel, r, g, b);
	      found_color = 0;
	    } else {
	      found_color = 1;
	    }
	  }
	} else {
	  found_color = 1;
	}
      }
    } else {
      found_color = 0;
    }
    if (!found_color) {
      switch (i) {
	case fgColor:
	case bgColor:
	  /* fatal: need bg/fg color */
	  fatal_error("Unable to get foreground/background colors!");
	  break;
#ifndef NO_CURSORCOLOR
	case cursorColor:
	  xcol.pixel = PixColors[bgColor];
	  break;
	case cursorColor2:
	  xcol.pixel = PixColors[fgColor];
	  break;
#endif /* NO_CURSORCOLOR */
	case unfocusedMenuColor:
	  xcol.pixel = PixColors[menuColor];
	  break;
	case unfocusedScrollColor:
	case menuColor:
	  xcol.pixel = PixColors[scrollColor];
	  break;
	default:
	  xcol.pixel = PixColors[bgColor];	/* None */
	  break;
      }
    }
    PixColors[i] = xcol.pixel;
  }

#ifndef NO_CURSORCOLOR
  if (Xdepth <= 2 || !rs_color[cursorColor])
    PixColors[cursorColor] = PixColors[bgColor];
  if (Xdepth <= 2 || !rs_color[cursorColor2])
    PixColors[cursorColor2] = PixColors[fgColor];
#endif /* NO_CURSORCOLOR */
  if (Xdepth <= 2 || !rs_color[pointerColor])
    PixColors[pointerColor] = PixColors[fgColor];
  if (Xdepth <= 2 || !rs_color[borderColor])
    PixColors[borderColor] = PixColors[bgColor];

#ifndef NO_BOLDUNDERLINE
  if (Xdepth <= 2 || !rs_color[colorBD])
    PixColors[colorBD] = PixColors[fgColor];
  if (Xdepth <= 2 || !rs_color[colorUL])
    PixColors[colorUL] = PixColors[fgColor];
#endif /* NO_BOLDUNDERLINE */

  /*
   * get scrollBar/menu shadow colors
   *
   * The calculations of topShadow/bottomShadow values are adapted
   * from the fvwm window manager.
   */
  if (Xdepth <= 2) {		/* Monochrome */
    PixColors[scrollColor] = PixColors[bgColor];
    PixColors[topShadowColor] = PixColors[fgColor];
    PixColors[bottomShadowColor] = PixColors[fgColor];

    PixColors[unfocusedScrollColor] = PixColors[bgColor];
    PixColors[unfocusedTopShadowColor] = PixColors[fgColor];
    PixColors[unfocusedBottomShadowColor] = PixColors[fgColor];

    PixColors[menuColor] = PixColors[bgColor];
    PixColors[menuTopShadowColor] = PixColors[fgColor];
    PixColors[menuBottomShadowColor] = PixColors[fgColor];

    PixColors[unfocusedMenuColor] = PixColors[bgColor];
    PixColors[unfocusedMenuTopShadowColor] = PixColors[fgColor];
    PixColors[unfocusedMenuBottomShadowColor] = PixColors[fgColor];
  } else {
    PixColors[bottomShadowColor] = get_bottom_shadow_color(PixColors[scrollColor], "bottomShadowColor");
    PixColors[unfocusedBottomShadowColor] = get_bottom_shadow_color(PixColors[unfocusedScrollColor], "unfocusedBottomShadowColor");
    PixColors[topShadowColor] = get_top_shadow_color(PixColors[scrollColor], "topShadowColor");
    PixColors[unfocusedTopShadowColor] = get_top_shadow_color(PixColors[unfocusedScrollColor], "unfocusedTopShadowColor");

    PixColors[menuBottomShadowColor] = get_bottom_shadow_color(PixColors[menuColor], "menuBottomShadowColor");
    PixColors[unfocusedMenuBottomShadowColor] = get_bottom_shadow_color(PixColors[unfocusedMenuColor], "unfocusedMenuBottomShadowColor");
    PixColors[menuTopShadowColor] = get_top_shadow_color(PixColors[menuColor], "menuTopShadowColor");
    PixColors[unfocusedMenuTopShadowColor] = get_top_shadow_color(PixColors[unfocusedMenuColor], "unfocusedMenuTopShadowColor");
  }

  szHint.base_width = (2 * TermWin.internalBorder + (Options & Opt_scrollBar ? scrollbar_trough_width() : 0));
  szHint.base_height = (2 * TermWin.internalBorder);

  flags = (rs_geometry ? XParseGeometry(rs_geometry, &x, &y, &width, &height) : 0);
  D_X11(("XParseGeometry(geom, %d, %d, %d, %d)\n", x, y, width, height));

  if (flags & WidthValue) {
    szHint.width = width;
    szHint.flags |= USSize;
  }
  if (flags & HeightValue) {
    szHint.height = height;
    szHint.flags |= USSize;
  }
  TermWin.ncol = szHint.width;
  TermWin.nrow = szHint.height;

  change_font(1, NULL);

  if (flags & XValue) {
    if (flags & XNegative) {
      if (check_for_enlightenment()) {
	x += (DisplayWidth(Xdisplay, Xscreen));
      } else {
	x += (DisplayWidth(Xdisplay, Xscreen) - (szHint.width + TermWin.internalBorder));
      }
      szHint.win_gravity = NorthEastGravity;
    }
    szHint.x = x;
    szHint.flags |= USPosition;
  }
  if (flags & YValue) {
    if (flags & YNegative) {
      if (check_for_enlightenment()) {
	y += (DisplayHeight(Xdisplay, Xscreen) - (2 * TermWin.internalBorder));
      } else {
	y += (DisplayHeight(Xdisplay, Xscreen) - (szHint.height + TermWin.internalBorder));
      }
      szHint.win_gravity = (szHint.win_gravity == NorthEastGravity ?
			    SouthEastGravity : SouthWestGravity);
    }
    szHint.y = y;
    szHint.flags |= USPosition;
  }
  if (flags) {
    D_X11(("Geometry values after parsing:  %dx%d%+d%+d\n", width, height, x, y));
  }

  /* parent window - reverse video so we can see placement errors
   * sub-window placement & size in resize_subwindows()
   */

  Attributes.background_pixel = PixColors[bgColor];
  Attributes.border_pixel = PixColors[bgColor];
  TermWin.parent = XCreateWindow(Xdisplay, Xroot, szHint.x, szHint.y, szHint.width, szHint.height, 0, Xdepth, InputOutput,
#ifdef PREFER_24BIT
				 Xvisual,
#else
				 CopyFromParent,
#endif
				 CWBackPixel | CWBorderPixel | CWColormap | CWOverrideRedirect, &Attributes);

  xterm_seq(XTerm_title, rs_title);
  xterm_seq(XTerm_iconName, rs_iconName);
  classHint.res_name = (char *) rs_name;
  classHint.res_class = APL_NAME;
  wmHint.window_group = TermWin.parent;
  wmHint.input = True;
  wmHint.initial_state = (Options & Opt_iconic ? IconicState : NormalState);
  wmHint.window_group = TermWin.parent;
  wmHint.flags = (InputHint | StateHint | WindowGroupHint);
#ifdef PIXMAP_SUPPORT
  set_icon_pixmap(rs_icon, &wmHint);
#endif

  XSetWMProperties(Xdisplay, TermWin.parent, NULL, NULL, argv, argc, &szHint, &wmHint, &classHint);
  XSelectInput(Xdisplay, Xroot, PropertyChangeMask);
  XSelectInput(Xdisplay, TermWin.parent, (KeyPressMask | FocusChangeMask | StructureNotifyMask | VisibilityChangeMask | PropertyChangeMask));
  if (mwmhints.flags) {
    XChangeProperty(Xdisplay, TermWin.parent, prop, prop, 32, PropModeReplace, (unsigned char *) &mwmhints, PROP_MWM_HINTS_ELEMENTS);
  }
  /* vt cursor: Black-on-White is standard, but this is more popular */
  TermWin_cursor = XCreateFontCursor(Xdisplay, XC_xterm);
  {

    XColor fg, bg;

    fg.pixel = PixColors[pointerColor];
    XQueryColor(Xdisplay, cmap, &fg);
    bg.pixel = PixColors[bgColor];
    XQueryColor(Xdisplay, cmap, &bg);
    XRecolorCursor(Xdisplay, TermWin_cursor, &fg, &bg);
  }

  /* cursor (menu/scrollbar): Black-on-White */
  cursor = XCreateFontCursor(Xdisplay, XC_left_ptr);

  /* the vt window */
  if ((!(Options & Opt_borderless)) && (Options & Opt_backing_store)) {
    D_X11(("Creating term window with save_under = TRUE\n"));
    TermWin.vt = XCreateWindow(Xdisplay, TermWin.parent, 0, 0, szHint.width, szHint.height, 0, Xdepth, InputOutput, CopyFromParent,
			       CWBackPixel | CWBorderPixel | CWOverrideRedirect | CWBackingStore | CWColormap, &Attributes);
    if (!(background_is_pixmap()) && !(Options & Opt_borderless)) {
      XSetWindowBackground(Xdisplay, TermWin.vt, PixColors[bgColor]);
      XClearWindow(Xdisplay, TermWin.vt);
    }
  } else {
    D_X11(("Creating term window with no backing store\n"));
    TermWin.vt = XCreateWindow(Xdisplay, TermWin.parent, 0, 0, szHint.width, szHint.height, 0, Xdepth, InputOutput, CopyFromParent,
			       CWBackPixel | CWBorderPixel | CWOverrideRedirect | CWColormap, &Attributes);
    if (!(background_is_pixmap()) && !(Options & Opt_borderless)) {
      XSetWindowBackground(Xdisplay, TermWin.vt, PixColors[bgColor]);
      XClearWindow(Xdisplay, TermWin.vt);
    }
  }

  XDefineCursor(Xdisplay, TermWin.vt, TermWin_cursor);
  XSelectInput(Xdisplay, TermWin.vt, (ExposureMask | ButtonPressMask | ButtonReleaseMask | Button1MotionMask | Button3MotionMask));

  /* If the user wants a specific desktop, tell the WM that */
  if (rs_desktop != -1) {
    prop = XInternAtom(Xdisplay, "_WIN_WORKSPACE", False);
    val = rs_desktop;
    XChangeProperty(Xdisplay, TermWin.parent, prop, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &val, 1);
  }

  /* We're done creating our windows.  Now let's initialize the event subsystem to handle them. */
  event_init_subsystem((event_dispatcher_t) process_x_event, (event_dispatcher_init_t) event_init_primary_dispatcher);

  /* Time for the scrollbar to create its windows and add itself to the event subsystem. */
  scrollbar_init();

  /* Same for the menu subsystem. */
  menu_init();

  XMapWindow(Xdisplay, TermWin.vt);
  XMapWindow(Xdisplay, TermWin.parent);
  XSetWindowBackground(Xdisplay, TermWin.vt, PixColors[bgColor]);
  XClearWindow(Xdisplay, TermWin.vt);

#ifdef PIXMAP_SUPPORT
  if (background_is_image()) {
    render_simage(images[image_bg].norm, TermWin.vt, TermWin_TotalWidth(), TermWin_TotalHeight(), image_bg, 0);
  }
#endif /* PIXMAP_SUPPORT */

  /* graphics context for the vt window */
  {
    XGCValues gcvalue;

    gcvalue.font = TermWin.font->fid;
    gcvalue.foreground = PixColors[fgColor];
    gcvalue.background = PixColors[bgColor];
    gcvalue.graphics_exposures = 0;
    TermWin.gc = XCreateGC(Xdisplay, TermWin.vt, GCForeground | GCBackground | GCFont | GCGraphicsExposures, &gcvalue);
  }

  if (Options & Opt_noCursor)
    scr_cursor_visible(0);

}

/* window resizing - assuming the parent window is the correct size */
void
resize_subwindows(int width, int height)
{

  int x = 0, y = 0;

#ifdef RXVT_GRAPHICS
  int old_width = TermWin.width;
  int old_height = TermWin.height;

#endif

  D_SCREEN(("resize_subwindows(%d, %d)\n", width, height));
  TermWin.width = TermWin.ncol * TermWin.fwidth;
  TermWin.height = TermWin.nrow * TermWin.fheight;

  /* size and placement */
  if (scrollbar_visible()) {
    scrollBar.beg = 0;
    scrollBar.end = height;
#ifdef MOTIF_SCROLLBAR
    if (scrollBar.type == SCROLLBAR_MOTIF) {
      /* arrows are as high as wide - leave 1 pixel gap */
      scrollBar.beg += scrollbar_arrow_height() + scrollbar_get_shadow() + 1;
      scrollBar.end -= scrollbar_arrow_height() + scrollbar_get_shadow() + 1;
    }
#endif
#ifdef NEXT_SCROLLBAR
    if (scrollBar.type == SCROLLBAR_NEXT) {
      scrollBar.beg = scrollbar_get_shadow();
      scrollBar.end -= (scrollBar.width * 2 + (scrollbar_get_shadow()? scrollbar_get_shadow() : 1) + 2);
    }
#endif
    width -= scrollbar_trough_width();
    XMoveResizeWindow(Xdisplay, scrollBar.win, ((Options & Opt_scrollBar_right) ? (width) : (x)), 0, scrollbar_trough_width(), height);
    if (scrollbar_is_pixmapped()) {
      scrollbar_show(0);
    }
    if (!(Options & Opt_scrollBar_right)) {
      x = scrollbar_trough_width();
    }
  }
  XMoveResizeWindow(Xdisplay, TermWin.vt, x, y, width, height + 1);

#ifdef RXVT_GRAPHICS
  if (old_width)
    Gr_Resize(old_width, old_height);
#endif
  if (!(background_is_pixmap())) {
    XSetWindowBackground(Xdisplay, TermWin.vt, PixColors[bgColor]);
    XClearWindow(Xdisplay, TermWin.vt);
  } else {
#ifdef PIXMAP_SUPPORT
    render_simage(images[image_bg].current, TermWin.vt, TermWin_TotalWidth(), TermWin_TotalHeight(), image_bg, 1);
#endif
  }
}

void
resize(void)
{
  szHint.base_width = (2 * TermWin.internalBorder);
  szHint.base_height = (2 * TermWin.internalBorder);

  szHint.base_width += (scrollbar_visible()? scrollbar_trough_width() : 0);

  szHint.min_width = szHint.base_width + szHint.width_inc;
  szHint.min_height = szHint.base_height + szHint.height_inc;

  szHint.width = szHint.base_width + TermWin.width;
  szHint.height = szHint.base_height + TermWin.height;

  szHint.flags = PMinSize | PResizeInc | PBaseSize | PWinGravity;

  XSetWMNormalHints(Xdisplay, TermWin.parent, &szHint);
  XResizeWindow(Xdisplay, TermWin.parent, szHint.width, szHint.height);

  resize_subwindows(szHint.width, szHint.height);
}

/*
 * Redraw window after exposure or size change
 */
void
resize_window1(unsigned int width, unsigned int height)
{
  static short first_time = 1;
  int new_ncol = (width - szHint.base_width) / TermWin.fwidth;
  int new_nrow = (height - szHint.base_height) / TermWin.fheight;

  if (first_time ||
      (new_ncol != TermWin.ncol) ||
      (new_nrow != TermWin.nrow)) {
    int curr_screen = -1;

    /* scr_reset only works on the primary screen */
    if (!first_time) {		/* this is not the first time thru */
      selection_clear();
      curr_screen = scr_change_screen(PRIMARY);
    }
    TermWin.ncol = new_ncol;
    TermWin.nrow = new_nrow;

    resize_subwindows(width, height);
    scr_reset();

    if (curr_screen >= 0)	/* this is not the first time thru */
      scr_change_screen(curr_screen);
    first_time = 0;
  } else if (image_mode_is(image_bg, MODE_TRANS) || image_mode_is(image_bg, MODE_VIEWPORT)) {
    resize_subwindows(width, height);
    scr_touch();
  }
}

/*
 * good for toggling 80/132 columns
 */
void
set_width(unsigned short width)
{
  unsigned short height = TermWin.nrow;

  if (width != TermWin.ncol) {
    width = szHint.base_width + width * TermWin.fwidth;
    height = szHint.base_height + height * TermWin.fheight;

    XResizeWindow(Xdisplay, TermWin.parent, width, height);
    resize_window1(width, height);
  }
}

/*
 * Redraw window after exposure or size change
 */
void
resize_window(void)
{
  Window root;
  int x, y;
  unsigned int border, depth, width, height;

  /* do we come from an fontchange? */
  if (font_change_count > 0) {
    font_change_count--;
    return;
  }
  XGetGeometry(Xdisplay, TermWin.parent, &root, &x, &y, &width, &height, &border, &depth);
  /* parent already resized */
  resize_window1(width, height);
}

#ifdef XTERM_COLOR_CHANGE
void
set_window_color(int idx, const char *color)
{

  XColor xcol;
  int i;
  unsigned int pixel, r, g, b;

  if (color == NULL || *color == '\0')
    return;

  /* handle color aliases */
  if (isdigit(*color)) {
    i = atoi(color);
    if (i >= 8 && i <= 15) {	/* bright colors */
      i -= 8;
# ifndef NO_BRIGHTCOLOR
      PixColors[idx] = PixColors[minBright + i];
# endif
    }
# ifndef NO_BRIGHTCOLOR
    else
# endif
    if (i >= 0 && i <= 7) {	/* normal colors */
      PixColors[idx] = PixColors[minColor + i];
    } else {
      print_warning("Color index %d is invalid.", i);
      return;
    }
  } else if (XParseColor(Xdisplay, cmap, color, &xcol)) {
    r = xcol.red;
    g = xcol.green;
    b = xcol.blue;
    pixel = Imlib_best_color_match(imlib_id, &r, &g, &b);
    xcol.pixel = pixel;
    if (!XAllocColor(Xdisplay, cmap, &xcol)) {
      print_warning("Unable to allocate \"%s\" in the color map.", color);
      return;
    }
    PixColors[idx] = xcol.pixel;
  } else {
    print_warning("Unable to resolve \"%s\" as a color name.", color);
    return;
  }

  if (idx == bgColor) {
    XSetWindowBackground(Xdisplay, TermWin.vt, PixColors[bgColor]);
  }

  /* handle colorBD, scrollbar background, etc. */

  set_colorfgbg();
  {

    XColor fg, bg;

    fg.pixel = PixColors[fgColor];
    XQueryColor(Xdisplay, cmap, &fg);
    bg.pixel = PixColors[bgColor];
    XQueryColor(Xdisplay, cmap, &bg);

    XRecolorCursor(Xdisplay, TermWin_cursor, &fg, &bg);
  }
  /* the only reasonable way to enforce a clean update */
  scr_poweron();
}
#else
# define set_window_color(idx,color) ((void)0)
#endif /* XTERM_COLOR_CHANGE */

