/*
 * Copyright (C) 1997-2000, Michael Jennings
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of the Software, its documentation and marketing & publicity
 * materials, and acknowledgment shall be given in the documentation, materials
 * and software packages that this Software was used.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

static const char cvs_ident[] = "$Id$";

#include "config.h"
#include "feature.h"

#include "../libmej/debug.h"
#include "../libmej/mem.h"
#include "../libmej/strings.h"
#include "debug.h"
#include "draw.h"
#include "misc.h"
#include "pixmap.h"
#include "startup.h"

void
draw_shadow(Drawable d, GC gc_top, GC gc_bottom, int x, int y, int w, int h, int shadow)
{

  ASSERT(w != 0);
  ASSERT(h != 0);
  LOWER_BOUND(shadow, 1);

  for (w += x - 1, h += y - 1; shadow > 0; shadow--, w--, h--) {
    XDrawLine(Xdisplay, d, gc_top, x, y, w, y);
    XDrawLine(Xdisplay, d, gc_top, x, y, x, h);
    x++; y++;
    XDrawLine(Xdisplay, d, gc_bottom, w, h, w, y);
    XDrawLine(Xdisplay, d, gc_bottom, w, h, x, h);
  }
}

void
draw_shadow_from_colors(Drawable d, Pixel top, Pixel bottom, int x, int y, int w, int h, int shadow)
{
  static GC gc_top = (GC) 0, gc_bottom = (GC) 0;

  if (gc_top == 0) {
    gc_top = XCreateGC(Xdisplay, TermWin.parent, 0, NULL);
    gc_bottom = XCreateGC(Xdisplay, TermWin.parent, 0, NULL);
  }

  XSetForeground(Xdisplay, gc_top, top);
  XSetForeground(Xdisplay, gc_bottom, bottom);
  draw_shadow(d, gc_top, gc_bottom, x, y, w, h, shadow);
}

void
draw_arrow(Drawable d, GC gc_top, GC gc_bottom, int x, int y, int w, int shadow, unsigned char type)
{

  BOUND(shadow, 1, 2);

  switch (type) {
    case DRAW_ARROW_UP:
      for (; shadow > 0; shadow--, x++, y++, w--) {
        XDrawLine(Xdisplay, d, gc_top, x, y + w, x + w / 2, y);
        XDrawLine(Xdisplay, d, gc_bottom, x + w, y + w, x + w / 2, y);
        XDrawLine(Xdisplay, d, gc_bottom, x + w, y + w, x, y + w);
      }
      break;
    case DRAW_ARROW_DOWN:
      for (; shadow > 0; shadow--, x++, y++, w--) {
        XDrawLine(Xdisplay, d, gc_top, x, y, x + w / 2, y + w);
        XDrawLine(Xdisplay, d, gc_top, x, y, x + w, y);
        XDrawLine(Xdisplay, d, gc_bottom, x + w, y, x + w / 2, y + w);
      }
      break;
    case DRAW_ARROW_LEFT:
      for (; shadow > 0; shadow--, x++, y++, w--) {
        XDrawLine(Xdisplay, d, gc_bottom, x + w, y + w, x + w, y);
        XDrawLine(Xdisplay, d, gc_bottom, x + w, y + w, x, y + w / 2);
        XDrawLine(Xdisplay, d, gc_top, x, y + w / 2, x + w, y);
      }
      break;
    case DRAW_ARROW_RIGHT:
      for (; shadow > 0; shadow--, x++, y++, w--) {
        XDrawLine(Xdisplay, d, gc_top, x, y, x, y + w);
        XDrawLine(Xdisplay, d, gc_top, x, y, x + w, y + w / 2);
        XDrawLine(Xdisplay, d, gc_bottom, x, y + w, x + w, y + w / 2);
      }
      break;
    default:
      break;
  }
}

void
draw_arrow_from_colors(Drawable d, Pixel top, Pixel bottom, int x, int y, int w, int shadow, unsigned char type)
{
  static GC gc_top = (GC) 0, gc_bottom = (GC) 0;

  if (gc_top == 0) {
    gc_top = XCreateGC(Xdisplay, TermWin.parent, 0, NULL);
    gc_bottom = XCreateGC(Xdisplay, TermWin.parent, 0, NULL);
  }

  XSetForeground(Xdisplay, gc_top, top);
  XSetForeground(Xdisplay, gc_bottom, bottom);
  draw_arrow(d, gc_top, gc_bottom, x, y, w, shadow, type);
}

void
draw_box(Drawable d, GC gc_top, GC gc_bottom, int x, int y, int w, int h)
{
  XDrawLine(Xdisplay, d, gc_top, x + w, y, x, y);
  XDrawLine(Xdisplay, d, gc_top, x, y, x, y + h);
  XDrawLine(Xdisplay, d, gc_bottom, x, y + h, x + w, y + h);
  XDrawLine(Xdisplay, d, gc_bottom, x + w, y + h, x + w, y);
}
