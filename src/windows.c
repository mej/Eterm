/* windows.c -- Eterm window handling module

 * This file is original work by Michael Jennings <mej@eterm.org> and
 * Tuomo Venalainen <vendu@cc.hut.fi>.  This file, and any other file
 * bearing this same message or a similar one, is distributed under
 * the GNU Public License (GPL) as outlined in the COPYING file.
 *
 * Copyright (C) 1997-2000, Michael Jennings and Tuomo Venalainen
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
#include "buttons.h"
#include "command.h"
#include "e.h"
#include "events.h"
#include "font.h"
#include "startup.h"
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
    prop.value = (unsigned char *) value;
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
  int r, g, b;

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
  int r, g, b;

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

  xcol.red = MAX((white.red / 5), xcol.red);
  xcol.green = MAX((white.green / 5), xcol.green);
  xcol.blue = MAX((white.blue / 5), xcol.blue);

  xcol.red = MIN(white.red, (xcol.red * 7) / 5);
  xcol.green = MIN(white.green, (xcol.green * 7) / 5);
  xcol.blue = MIN(white.blue, (xcol.blue * 7) / 5);
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

Pixel
get_color_by_name(const char *name, const char *fallback)
{
  XColor xcol;

  if (name == NULL) {
    if (fallback == NULL) {
      return ((Pixel) -1);
    } else {
      name = fallback;
    }
  }
  if (!XParseColor(Xdisplay, cmap, name, &xcol)) {
    print_warning("Unable to resolve \"%s\" as a color name.  Falling back on \"%s\".", name, NONULL(fallback));
    name = fallback;
    if (name) {
      if (!XParseColor(Xdisplay, cmap, name, &xcol)) {
        print_warning("Unable to resolve \"%s\" as a color name.  This should never fail.  Please repair/restore your RGB database.", name);
        return ((Pixel) -1);
      }
    } else {
      return ((Pixel) -1);
    }
  }
  if (Xdepth < 24) {
    int r, g, b;

    r = xcol.red;
    g = xcol.green;
    b = xcol.blue;
    xcol.pixel = Imlib_best_color_match(imlib_id, &r, &g, &b);
    xcol.red = r;
    xcol.green = g;
    xcol.blue = b;
  }
  if (!XAllocColor(Xdisplay, cmap, &xcol)) {
    print_warning("Unable to allocate \"%s\" (0x%08x:  0x%04x, 0x%04x, 0x%04x) in the color map.  Falling back on \"%s\".",
                  name, xcol.pixel, xcol.red, xcol.green, xcol.blue, NONULL(fallback));
    name = fallback;
    if (name) {
      if (!XAllocColor(Xdisplay, cmap, &xcol)) {
        print_warning("Unable to allocate \"%s\" (0x%08x:  0x%04x, 0x%04x, 0x%04x) in the color map.", name, xcol.pixel, xcol.red, xcol.green, xcol.blue);
        return ((Pixel) -1);
      }
    } else {
      return ((Pixel) -1);
    }
  }
  return (xcol.pixel);
}

Pixel
get_color_by_pixel(Pixel pixel, Pixel fallback)
{
  XColor xcol;

  xcol.pixel = pixel;
  if (!XQueryColor(Xdisplay, cmap, &xcol)) {
    print_warning("Unable to convert pixel value 0x%08x to an XColor structure.  Falling back on 0x%08x.", pixel, fallback);
    xcol.pixel = fallback;
    if (!XQueryColor(Xdisplay, cmap, &xcol)) {
      print_warning("Unable to convert pixel value 0x%08x to an XColor structure.", xcol.pixel);
      return ((Pixel) 0);
    }
  }
  if (Xdepth < 24) {
    int r, g, b;

    r = xcol.red;
    g = xcol.green;
    b = xcol.blue;
    xcol.pixel = Imlib_best_color_match(imlib_id, &r, &g, &b);
    xcol.red = r;
    xcol.green = g;
    xcol.blue = b;
  }
  if (!XAllocColor(Xdisplay, cmap, &xcol)) {
    print_warning("Unable to allocate 0x%08x (0x%04x, 0x%04x, 0x%04x) in the color map.  Falling back on 0x%08x.", xcol.pixel, xcol.red, xcol.green, xcol.blue, fallback);
    xcol.pixel = fallback;
    if (!XAllocColor(Xdisplay, cmap, &xcol)) {
      print_warning("Unable to allocate 0x%08x (0x%04x, 0x%04x, 0x%04x) in the color map.", xcol.pixel, xcol.red, xcol.green, xcol.blue);
      return ((Pixel) 0);
    }
  }
  return (xcol.pixel);
}

void
process_colors(void)
{
  int i;
  Pixel pixel;

  for (i = 0; i < NRS_COLORS; i++) {
    if ((Xdepth <= 2) || ((pixel = get_color_by_name(rs_color[i], def_colorName[i])) == (Pixel) -1)) {
      switch (i) {
	case fgColor:
          pixel = WhitePixel(Xdisplay, Xscreen);
          break;
	case bgColor:
          pixel = BlackPixel(Xdisplay, Xscreen);
          break;
#ifndef NO_CURSORCOLOR
	case cursorColor:           pixel = PixColors[bgColor]; break;
	case cursorColor2:          pixel = PixColors[fgColor]; break;
#endif /* NO_CURSORCOLOR */
        case pointerColor:          pixel = PixColors[fgColor]; break;
        case borderColor:           pixel = PixColors[bgColor]; break;
#ifndef NO_BOLDUNDERLINE
        case colorBD:               pixel = PixColors[fgColor]; break;
        case colorUL:               pixel = PixColors[fgColor]; break;
#endif
	default:
	  pixel = PixColors[fgColor];	/* None */
	  break;
      }
    }
    PixColors[i] = pixel;
  }

  if (Xdepth <= 2) {		/* Monochrome */
    PixColors[topShadowColor] = PixColors[fgColor];
    PixColors[bottomShadowColor] = PixColors[fgColor];
    PixColors[unfocusedTopShadowColor] = PixColors[fgColor];
    PixColors[unfocusedBottomShadowColor] = PixColors[fgColor];

    PixColors[menuTopShadowColor] = PixColors[fgColor];
    PixColors[menuBottomShadowColor] = PixColors[fgColor];
    PixColors[unfocusedMenuTopShadowColor] = PixColors[fgColor];
    PixColors[unfocusedMenuBottomShadowColor] = PixColors[fgColor];
  } else {
    PixColors[bottomShadowColor] = get_bottom_shadow_color(images[image_sb].norm->bg, "bottomShadowColor");
    PixColors[unfocusedBottomShadowColor] = get_bottom_shadow_color(images[image_sb].disabled->bg, "unfocusedBottomShadowColor");
    PixColors[topShadowColor] = get_top_shadow_color(images[image_sb].norm->bg, "topShadowColor");
    PixColors[unfocusedTopShadowColor] = get_top_shadow_color(images[image_sb].disabled->bg, "unfocusedTopShadowColor");

    PixColors[menuBottomShadowColor] = get_bottom_shadow_color(images[image_menu].norm->bg, "menuBottomShadowColor");
    PixColors[unfocusedMenuBottomShadowColor] = get_bottom_shadow_color(images[image_menu].disabled->bg, "unfocusedMenuBottomShadowColor");
    PixColors[menuTopShadowColor] = get_top_shadow_color(images[image_menu].norm->bg, "menuTopShadowColor");
    PixColors[unfocusedMenuTopShadowColor] = get_top_shadow_color(images[image_menu].disabled->bg, "unfocusedMenuTopShadowColor");
  }
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
  int x = 0, y = 0, flags;
  unsigned int width = 0, height = 0;
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

  szHint.base_width = (2 * TermWin.internalBorder + ((Options & Opt_scrollbar) ? (scrollbar_get_width() + (2 * scrollbar_get_shadow())) : 0));
  szHint.base_height = (2 * TermWin.internalBorder) + bbar_calc_docked_height(BBAR_DOCKED);

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
      x += (DisplayWidth(Xdisplay, Xscreen) - (szHint.width + TermWin.internalBorder));
      szHint.win_gravity = NorthEastGravity;
    }
    szHint.x = x;
    szHint.flags |= USPosition;
  }
  if (flags & YValue) {
    if (flags & YNegative) {
      y += (DisplayHeight(Xdisplay, Xscreen) - (szHint.height + TermWin.internalBorder));
      szHint.win_gravity = (szHint.win_gravity == NorthEastGravity ?
			    SouthEastGravity : SouthWestGravity);
    }
    szHint.y = y;
    szHint.flags |= USPosition;
  }
  if (flags) {
    D_X11(("Geometry values after parsing:  %dx%d%+d%+d\n", width, height, x, y));
  }

  Attributes.background_pixel = PixColors[bgColor];
  Attributes.border_pixel = PixColors[bgColor];
  D_X11(("szHint == { %d, %d, %d, %d }\n", szHint.x, szHint.y, szHint.width, szHint.height));
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
  TermWin.x = (((Options & Opt_scrollbar) && !(Options & Opt_scrollbar_right)) ? (scrollbar_get_width() + (2 * scrollbar_get_shadow())) : 0);
  TermWin.y = bbar_calc_docked_height(BBAR_DOCKED_TOP);
  if ((!(Options & Opt_borderless)) && (Options & Opt_backing_store)) {
    D_X11(("Creating term window with save_under = TRUE\n"));
    TermWin.vt = XCreateWindow(Xdisplay, TermWin.parent, TermWin.x, TermWin.y, szHint.width, szHint.height, 0, Xdepth, InputOutput, CopyFromParent,
			       CWBackPixel | CWBorderPixel | CWOverrideRedirect | CWBackingStore | CWColormap, &Attributes);
  } else {
    D_X11(("Creating term window with no backing store\n"));
    TermWin.vt = XCreateWindow(Xdisplay, TermWin.parent, TermWin.x, TermWin.y, szHint.width, szHint.height, 0, Xdepth, InputOutput, CopyFromParent,
			       CWBackPixel | CWBorderPixel | CWOverrideRedirect | CWColormap, &Attributes);
  }
  if (!(background_is_pixmap()) && !(Options & Opt_borderless)) {
    XSetWindowBackground(Xdisplay, TermWin.vt, PixColors[bgColor]);
    XClearWindow(Xdisplay, TermWin.vt);
  }
  XDefineCursor(Xdisplay, TermWin.vt, TermWin_cursor);
  XSelectInput(Xdisplay, TermWin.vt, (EnterWindowMask | LeaveWindowMask | ExposureMask | ButtonPressMask | ButtonReleaseMask | Button1MotionMask | Button3MotionMask));

  /* If the user wants a specific desktop, tell the WM that */
  if (rs_desktop != -1) {
    prop = XInternAtom(Xdisplay, "_WIN_WORKSPACE", False);
    val = rs_desktop;
    XChangeProperty(Xdisplay, TermWin.parent, prop, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &val, 1);
  }

  /* We're done creating our windows.  Now let's initialize the event subsystem to handle them. */
  event_init_subsystem((event_dispatcher_t) process_x_event, (event_dispatcher_init_t) event_init_primary_dispatcher);

  XMapWindow(Xdisplay, TermWin.vt);
  XMapWindow(Xdisplay, TermWin.parent);
  XSetWindowBackground(Xdisplay, TermWin.vt, PixColors[bgColor]);

  render_simage(images[image_bg].current, TermWin.vt, TermWin_TotalWidth(), TermWin_TotalHeight(), image_bg, 0);
  if (image_mode_is(image_bg, MODE_AUTO)) {
    enl_ipc_sync();
  }

  /* graphics context for the vt window */
  {
    XGCValues gcvalue;

    gcvalue.font = TermWin.font->fid;
    gcvalue.foreground = PixColors[fgColor];
    gcvalue.background = PixColors[bgColor];
    gcvalue.graphics_exposures = 0;
    TermWin.gc = XCreateGC(Xdisplay, TermWin.vt, GCForeground | GCBackground | GCFont | GCGraphicsExposures, &gcvalue);
  }

  if (Options & Opt_noCursor) {
    scr_cursor_visible(0);
  }
}

/* good for toggling 80/132 columns */
void
set_width(unsigned short width)
{
  unsigned short height = TermWin.nrow;

  if (width != TermWin.ncol) {
    width = szHint.base_width + width * TermWin.fwidth;
    height = szHint.base_height + height * TermWin.fheight;

    XResizeWindow(Xdisplay, TermWin.parent, width, height);
    handle_resize(width, height);
  }
}

void
update_size_hints(void)
{
  D_X11(("Called.\n"));
  szHint.base_width = (2 * TermWin.internalBorder) + ((scrollbar_is_visible()) ? (scrollbar_trough_width()) : (0));
  szHint.base_height = (2 * TermWin.internalBorder) + bbar_calc_docked_height(BBAR_DOCKED);

  szHint.width_inc = TermWin.fwidth;
  szHint.height_inc = TermWin.fheight;

  D_X11(("Size Hints:  base width/height == %lux%lu, width/height increment == %lux%lu\n", szHint.base_width, szHint.base_height, szHint.width_inc, szHint.height_inc));

  szHint.min_width = szHint.base_width + szHint.width_inc;
  szHint.min_height = szHint.base_height + szHint.height_inc;
  szHint.width = szHint.base_width + TermWin.width;
  szHint.height = szHint.base_height + TermWin.height;
  D_X11(("             Minimum width/height == %lux%lu, width/height == %lux%lu\n",
         szHint.min_width, szHint.min_height, szHint.width, szHint.height));

  szHint.flags = PMinSize | PResizeInc | PBaseSize | PWinGravity;
  XSetWMNormalHints(Xdisplay, TermWin.parent, &szHint);
}

/* Resize terminal window and scrollbar window */
void
term_resize(int width, int height)
{
  static int last_width = 0, last_height = 0;

  D_X11(("term_resize(%d, %d)\n", width, height));
  TermWin.width = TermWin.ncol * TermWin.fwidth;
  TermWin.height = TermWin.nrow * TermWin.fheight;
  D_X11((" -> New TermWin width/height == %lux%lu\n", TermWin.width, TermWin.height));
  width = TermWin_TotalWidth();
  height = TermWin_TotalHeight();
  XMoveResizeWindow(Xdisplay, TermWin.vt, ((Options & Opt_scrollbar_right) ? (0) : ((scrollbar_is_visible()) ? (scrollbar_trough_width()) : (0))),
                    bbar_calc_docked_height(BBAR_DOCKED_TOP), width, height);
  if (width != last_width || height != last_height) {
    render_simage(images[image_bg].current, TermWin.vt, TermWin_TotalWidth(), TermWin_TotalHeight(), image_bg, 0);
    scr_reset();
    scr_touch();
    if (image_mode_is(image_bg, MODE_AUTO)) {
      enl_ipc_sync();
    }
    last_width = width;
    last_height = height;
  }
}

/* Resize due to font change; update size hints and child windows */
void
parent_resize(void)
{
  D_X11(("Called.\n"));
  update_size_hints();
  XResizeWindow(Xdisplay, TermWin.parent, szHint.width, szHint.height);
  D_X11((" -> New parent width/height == %lux%lu\n", szHint.width, szHint.height));
  term_resize(szHint.width, szHint.height);
  scrollbar_resize(szHint.width, szHint.height - bbar_calc_docked_height(BBAR_DOCKED));
  bbar_resize_all(szHint.width);
}

void
handle_resize(unsigned int width, unsigned int height)
{
  static short first_time = 1;
  int new_ncol = (width - szHint.base_width) / TermWin.fwidth;
  int new_nrow = (height - szHint.base_height) / TermWin.fheight;

  D_EVENTS(("handle_resize(%u, %u)\n", width, height));
  if (first_time || (new_ncol != TermWin.ncol) || (new_nrow != TermWin.nrow)) {
    TermWin.ncol = new_ncol;
    TermWin.nrow = new_nrow;

    term_resize(width, height);
    szHint.width = szHint.base_width + TermWin.width;
    szHint.height = szHint.base_height + TermWin.height;
    D_X11((" -> New szHint.width/height == %lux%lu\n", szHint.width, szHint.height));
    scrollbar_resize(width, szHint.height - bbar_calc_docked_height(BBAR_DOCKED));
    bbar_resize_all(szHint.width);
    first_time = 0;
  }
}

void
handle_move(int x, int y)
{
  if ((TermWin.x != x) || (TermWin.y != y)) {
    if (image_mode_any(MODE_TRANS | MODE_VIEWPORT)) {
      redraw_images_by_mode(MODE_TRANS | MODE_VIEWPORT);
    }
    TermWin.x = x;
    TermWin.y = y;
  }
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

  if (!background_is_pixmap() && (idx == bgColor)) {
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

Window
find_window_by_coords(Window win, int win_x, int win_y, int rel_x, int rel_y)
{
  Window *children = NULL;
  XWindowAttributes attr;
  Window child = 0, parent_win = 0, root_win = 0;
  int i;
  unsigned int ww, wh, num;
  int wx, wy;

  D_X11(("win 0x%08x at %d, %d.  Coords are %d, %d.\n", win, win_x, win_y, rel_x, rel_y));

  /* Bad or invisible window. */
  if ((!XGetWindowAttributes(Xdisplay, win, &attr)) || (attr.map_state != IsViewable)) {
    return None;
  }
  wx = attr.x + win_x;
  wy = attr.y + win_y;
  ww = attr.width;
  wh = attr.height;
   
  if (!((rel_x >= wx) && (rel_y >= wy) && (rel_x < (int)(wx + ww)) && (rel_y < (int)(wy + wh)))) {
    return None;
  }

  if (!XQueryTree(Xdisplay, win, &root_win, &parent_win, &children, &num)) {
    return win;
  }
  if (children) {
    D_X11(("%d children.\n", num));
    for (i = num - 1; i >= 0; i--) {
      D_X11(("Trying children[%d] (0x%08x)\n", i, children[i]));
      if ((child = find_window_by_coords(children[i], wx, wy, rel_x, rel_y)) != None) {
        D_X11(("Match!\n"));
        XFree(children);
        return child;
      }
    }
    D_X11(("XFree(children)\n"));
    XFree(children);
  }
  D_X11(("Returning 0x%08x\n", win));
  return win;
}
