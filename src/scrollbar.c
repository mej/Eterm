/* scrollbar.c -- Eterm scrollbar module

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

static const char cvs_ident[] = "$Id$";

#include "config.h"
#include "feature.h"

#include <X11/cursorfont.h>

#include "../libmej/debug.h"
#include "../libmej/mem.h"
#include "command.h"
#include "e.h"
#include "events.h"
#include "startup.h"
#include "options.h"
#ifdef PIXMAP_SCROLLBAR
# include "pixmap.h"
#endif
#include "screen.h"
#include "scrollbar.h"
#include "term.h"
#include "windows.h"

event_dispatcher_data_t scrollbar_event_data;
#ifdef PIXMAP_SCROLLBAR
scrollbar_t scrollBar =
{0, 1, 0, 1, 0, SCROLLBAR_DEFAULT_TYPE, 0, 0, SB_WIDTH, 0, 0, 0, 0, 0, 0, 0, None, None, None, None};
#else                                             
scrollbar_t scrollBar =
{0, 1, 0, 1, 0, SCROLLBAR_DEFAULT_TYPE, 0, 0, SB_WIDTH, 0, 0, 0, 0, 0, 0, 0, None};
#endif
#ifdef SCROLLBAR_BUTTON_CONTINUAL_SCROLLING
short scroll_arrow_delay;
#endif
static GC scrollbarGC;
static short last_top = 0, last_bot = 0;	/* old (drawn) values */
#ifdef XTERM_SCROLLBAR		/* bitmap scrollbar */
static GC shadowGC;
static unsigned char xterm_sb_bits[] =
{0xaa, 0x0a, 0x55, 0x05};	/* 12x2 bitmap */
#endif
#if defined(MOTIF_SCROLLBAR) || defined(NEXT_SCROLLBAR)
static GC topShadowGC, botShadowGC;

/* draw triangular up button with a shadow of SHADOW (1 or 2) pixels */
void
Draw_up_button(int x, int y, int state)
{

  const unsigned int sz = (scrollBar.width), sz2 = (scrollBar.width / 2);
  XPoint pt[3];
  GC top = None, bot = None;

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

  /* fill triangle */
  pt[0].x = x;
  pt[0].y = y + sz - 1;
  pt[1].x = x + sz - 1;
  pt[1].y = y + sz - 1;
  pt[2].x = x + sz2;
  pt[2].y = y;
  XFillPolygon(Xdisplay, scrollBar.win, scrollbarGC, pt, 3, Convex, CoordModeOrigin);

  /* draw base */
  XDrawLine(Xdisplay, scrollBar.win, bot, pt[0].x, pt[0].y, pt[1].x, pt[1].y);

  /* draw shadow */
  pt[1].x = x + sz2 - 1;
  pt[1].y = y;
  XDrawLine(Xdisplay, scrollBar.win, top, pt[0].x, pt[0].y, pt[1].x, pt[1].y);
  if (scrollbar_get_shadow() > 1) {
    pt[0].x++;
    pt[0].y--;
    pt[1].y++;
    XDrawLine(Xdisplay, scrollBar.win, top, pt[0].x, pt[0].y, pt[1].x, pt[1].y);
  }
  /* draw shadow */
  pt[0].x = x + sz2;
  pt[0].y = y;
  pt[1].x = x + sz - 1;
  pt[1].y = y + sz - 1;
  XDrawLine(Xdisplay, scrollBar.win, bot, pt[0].x, pt[0].y, pt[1].x, pt[1].y);
  if (scrollbar_get_shadow() > 1) {
    /* doubled */
    pt[0].y++;
    pt[1].x--;
    pt[1].y--;
    XDrawLine(Xdisplay, scrollBar.win, bot, pt[0].x, pt[0].y, pt[1].x, pt[1].y);
  }
}

/* draw triangular down button with a shadow of SHADOW (1 or 2) pixels */
void
Draw_dn_button(int x, int y, int state)
{

  const unsigned int sz = (scrollBar.width), sz2 = (scrollBar.width / 2);
  XPoint pt[3];
  GC top = None, bot = None;

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

  /* fill triangle */
  pt[0].x = x;
  pt[0].y = y;
  pt[1].x = x + sz - 1;
  pt[1].y = y;
  pt[2].x = x + sz2;
  pt[2].y = y + sz;
  XFillPolygon(Xdisplay, scrollBar.win, scrollbarGC, pt, 3, Convex, CoordModeOrigin);

  /* draw base */
  XDrawLine(Xdisplay, scrollBar.win, top, pt[0].x, pt[0].y, pt[1].x, pt[1].y);

  /* draw shadow */
  pt[1].x = x + sz2 - 1;
  pt[1].y = y + sz - 1;
  XDrawLine(Xdisplay, scrollBar.win, top, pt[0].x, pt[0].y, pt[1].x, pt[1].y);
  if (scrollbar_get_shadow() > 1) {
    /* doubled */
    pt[0].x++;
    pt[0].y++;
    pt[1].y--;
    XDrawLine(Xdisplay, scrollBar.win, top, pt[0].x, pt[0].y, pt[1].x, pt[1].y);
  }
  /* draw shadow */
  pt[0].x = x + sz2;
  pt[0].y = y + sz - 1;
  pt[1].x = x + sz - 1;
  pt[1].y = y;
  XDrawLine(Xdisplay, scrollBar.win, bot, pt[0].x, pt[0].y, pt[1].x, pt[1].y);
  if (scrollbar_get_shadow() > 1) {
    /* doubled */
    pt[0].y--;
    pt[1].x--;
    pt[1].y++;
    XDrawLine(Xdisplay, scrollBar.win, bot, pt[0].x, pt[0].y, pt[1].x, pt[1].y);
  }
}
#endif /* MOTIF_SCROLLBAR || NEXT_SCROLLBAR */

void
scrollbar_init(void)
{

  Cursor cursor;
  long mask;

  D_SCROLLBAR(("scrollbar_init():  Initializing all scrollbar elements.\n"));

  Attributes.background_pixel = PixColors[scrollColor];
  Attributes.border_pixel = PixColors[bgColor];
  Attributes.override_redirect = TRUE;
  Attributes.save_under = TRUE;
  cursor = XCreateFontCursor(Xdisplay, XC_left_ptr);
  mask = ExposureMask | EnterWindowMask | LeaveWindowMask | ButtonPressMask | ButtonReleaseMask
      | Button1MotionMask | Button2MotionMask | Button3MotionMask;

  scrollBar.win = XCreateWindow(Xdisplay, TermWin.parent, 0, 0, 1, 1, 0, Xdepth, InputOutput, CopyFromParent,
				CWOverrideRedirect | CWBackingStore | CWBackPixel | CWBorderPixel | CWColormap, &Attributes);
  XDefineCursor(Xdisplay, scrollBar.win, cursor);
  XSelectInput(Xdisplay, scrollBar.win, mask);
  D_SCROLLBAR(("scrollbar_init():  Created scrollbar window 0x%08x\n", scrollBar.win));

#ifdef PIXMAP_SCROLLBAR
  if (scrollbar_uparrow_is_pixmapped()) {
    scrollBar.up_win = XCreateWindow(Xdisplay, scrollBar.win, 0, 0, 1, 1, 0, Xdepth, InputOutput, CopyFromParent,
				     CWOverrideRedirect | CWSaveUnder | CWBackingStore | CWColormap, &Attributes);
    XSelectInput(Xdisplay, scrollBar.up_win, mask);
    D_SCROLLBAR(("scrollbar_init():  Created scrollbar up arrow window 0x%08x\n", scrollBar.up_win));
  }
  if (scrollbar_downarrow_is_pixmapped()) {
    scrollBar.dn_win = XCreateWindow(Xdisplay, scrollBar.win, 0, 0, 1, 1, 0, Xdepth, InputOutput, CopyFromParent,
				     CWOverrideRedirect | CWSaveUnder | CWBackingStore | CWColormap, &Attributes);
    XSelectInput(Xdisplay, scrollBar.dn_win, mask);
    D_SCROLLBAR(("scrollbar_init():  Created scrollbar down arrow window 0x%08x\n", scrollBar.dn_win));
  }
  if (scrollbar_anchor_is_pixmapped()) {
    scrollBar.sa_win = XCreateWindow(Xdisplay, scrollBar.win, 0, 0, 1, 1, 0, Xdepth, InputOutput, CopyFromParent,
				     CWOverrideRedirect | CWSaveUnder | CWBackingStore | CWColormap, &Attributes);
    XSelectInput(Xdisplay, scrollBar.sa_win, mask);
    D_SCROLLBAR(("scrollbar_init():  Created scrollbar anchor window 0x%08x\n", scrollBar.sa_win));
  }
#endif

  if (scrollBar.type == SCROLLBAR_XTERM) {
    scrollbar_set_shadow(0);
  } else {
    scrollbar_set_shadow((Options & Opt_scrollBar_floating) ? 0 : SHADOW);
  }
  D_SCROLLBAR(("scrollbar_init():  Set scrollbar shadow to %d\n", scrollbar_get_shadow()));
  event_register_dispatcher(scrollbar_dispatch_event, scrollbar_event_init_dispatcher);

}

void
scrollbar_event_init_dispatcher(void)
{

  MEMSET(&scrollbar_event_data, 0, sizeof(event_dispatcher_data_t));

#if 0
  EVENT_DATA_ADD_HANDLER(scrollbar_event_data, ConfigureNotify, sb_handle_configure_notify);
#endif
  EVENT_DATA_ADD_HANDLER(scrollbar_event_data, EnterNotify, sb_handle_enter_notify);
  EVENT_DATA_ADD_HANDLER(scrollbar_event_data, LeaveNotify, sb_handle_leave_notify);
  EVENT_DATA_ADD_HANDLER(scrollbar_event_data, FocusIn, sb_handle_focus_in);
  EVENT_DATA_ADD_HANDLER(scrollbar_event_data, FocusOut, sb_handle_focus_out);
  EVENT_DATA_ADD_HANDLER(scrollbar_event_data, GraphicsExpose, sb_handle_expose);
  EVENT_DATA_ADD_HANDLER(scrollbar_event_data, Expose, sb_handle_expose);
  EVENT_DATA_ADD_HANDLER(scrollbar_event_data, ButtonPress, sb_handle_button_press);
  EVENT_DATA_ADD_HANDLER(scrollbar_event_data, ButtonRelease, sb_handle_button_release);
  EVENT_DATA_ADD_HANDLER(scrollbar_event_data, MotionNotify, sb_handle_motion_notify);

  event_data_add_mywin(&scrollbar_event_data, scrollBar.win);
#ifdef PIXMAP_SCROLLBAR
  if (scrollbar_is_pixmapped()) {
    event_data_add_mywin(&scrollbar_event_data, scrollBar.up_win);
    event_data_add_mywin(&scrollbar_event_data, scrollBar.dn_win);
    event_data_add_mywin(&scrollbar_event_data, scrollBar.sa_win);
  }
#endif

  event_data_add_parent(&scrollbar_event_data, TermWin.vt);
  event_data_add_parent(&scrollbar_event_data, TermWin.parent);

}

#if 0
unsigned char
sb_handle_configure_notify(event_t * ev)
{

  D_EVENTS(("sb_handle_configure_notify(ev [%8p] on window 0x%08x)\n", ev, ev->xany.window));

  REQUIRE_RVAL(XEVENT_IS_PARENT(ev, &scrollbar_event_data), 0);

  redraw_image(image_sb);
  return 1;
}
#endif

unsigned char
sb_handle_enter_notify(event_t * ev)
{

  D_EVENTS(("sb_handle_enter_notify(ev [%8p] on window 0x%08x)\n", ev, ev->xany.window));

  REQUIRE_RVAL(XEVENT_IS_MYWIN(ev, &scrollbar_event_data), 0);

  if (scrollbar_uparrow_is_pixmapped() && scrollbar_win_is_uparrow(ev->xany.window)) {
    images[image_up].current = images[image_up].selected;
    render_simage(images[image_up].current, scrollbar_get_uparrow_win(), scrollbar_arrow_width(), scrollbar_arrow_width(), image_up, 0);
  }
  if (scrollbar_downarrow_is_pixmapped() && scrollbar_win_is_downarrow(ev->xany.window)) {
    images[image_down].current = images[image_down].selected;
    render_simage(images[image_down].current, scrollbar_get_downarrow_win(), scrollbar_arrow_width(), scrollbar_arrow_width(), image_down, 0);
  }
  if (scrollbar_anchor_is_pixmapped() && scrollbar_win_is_anchor(ev->xany.window)) {
    images[image_sa].current = images[image_sa].selected;
    render_simage(images[image_sa].current, scrollbar_get_anchor_win(), scrollbar_anchor_width(), scrollbar_anchor_height(), image_sa, 0);
  }
  if (scrollbar_is_pixmapped() && scrollbar_win_is_scrollbar(ev->xany.window)) {
    images[image_sb].current = images[image_sb].selected;
    render_simage(images[image_sb].current, scrollbar_get_win(), scrollbar_trough_width(), scrollbar_trough_height(), image_sb, 0);
  }
  return 1;
}

unsigned char
sb_handle_leave_notify(event_t * ev)
{

  D_EVENTS(("sb_handle_leave_notify(ev [%8p] on window 0x%08x)\n", ev, ev->xany.window));

  REQUIRE_RVAL(XEVENT_IS_MYWIN(ev, &scrollbar_event_data), 0);

  if (scrollbar_uparrow_is_pixmapped() && scrollbar_win_is_uparrow(ev->xany.window)) {
    images[image_up].current = images[image_up].norm;
    render_simage(images[image_up].current, scrollbar_get_uparrow_win(), scrollbar_arrow_width(), scrollbar_arrow_width(), image_up, 0);
  }
  if (scrollbar_downarrow_is_pixmapped() && scrollbar_win_is_downarrow(ev->xany.window)) {
    images[image_down].current = images[image_down].norm;
    render_simage(images[image_down].current, scrollbar_get_downarrow_win(), scrollbar_arrow_width(), scrollbar_arrow_width(), image_down, 0);
  }
  if (scrollbar_anchor_is_pixmapped() && scrollbar_win_is_anchor(ev->xany.window)) {
    images[image_sa].current = images[image_sa].norm;
    render_simage(images[image_sa].current, scrollbar_get_anchor_win(), scrollbar_anchor_width(), scrollbar_anchor_height(), image_sa, 0);
  }
  if (scrollbar_is_pixmapped() && scrollbar_win_is_scrollbar(ev->xany.window)) {
    images[image_sb].current = images[image_sb].norm;
    render_simage(images[image_sb].current, scrollbar_get_win(), scrollbar_trough_width(), scrollbar_trough_height(), image_sb, 0);
  }
  return 1;
}

unsigned char
sb_handle_focus_in(event_t * ev)
{

  D_EVENTS(("sb_handle_focus_in(ev [%8p] on window 0x%08x)\n", ev, ev->xany.window));

  REQUIRE_RVAL(XEVENT_IS_MYWIN(ev, &scrollbar_event_data), 0);

  return 1;
}

unsigned char
sb_handle_focus_out(event_t * ev)
{

  D_EVENTS(("sb_handle_focus_out(ev [%8p] on window 0x%08x)\n", ev, ev->xany.window));

  REQUIRE_RVAL(XEVENT_IS_MYWIN(ev, &scrollbar_event_data), 0);

  return 1;
}

unsigned char
sb_handle_expose(event_t * ev)
{

  XEvent unused_xevent;

  D_EVENTS(("sb_handle_expose(ev [%8p] on window 0x%08x)\n", ev, ev->xany.window));

  REQUIRE_RVAL(XEVENT_IS_MYWIN(ev, &scrollbar_event_data), 0);

  while (XCheckTypedWindowEvent(Xdisplay, ev->xany.window, Expose, &unused_xevent));
  while (XCheckTypedWindowEvent(Xdisplay, ev->xany.window, GraphicsExpose, &unused_xevent));
  return 1;
}

unsigned char
sb_handle_button_press(event_t * ev)
{

  D_EVENTS(("sb_handle_button_press(ev [%8p] on window 0x%08x)\n", ev, ev->xany.window));

  REQUIRE_RVAL(XEVENT_IS_MYWIN(ev, &scrollbar_event_data), 0);

  button_state.bypass_keystate = (ev->xbutton.state & (Mod1Mask | ShiftMask));
  button_state.report_mode = (button_state.bypass_keystate ? 0 : ((PrivateModes & PrivMode_mouse_report) ? 1 : 0));

  scrollbar_setNone();
#ifndef NO_SCROLLBAR_REPORT
  if (button_state.report_mode) {
    /* Mouse report disabled scrollbar.  Arrows send cursor key up/down, trough sends pageup/pagedown */
    if (scrollbar_upButton(ev->xany.window, ev->xbutton.y))
      tt_printf((unsigned char *) "\033[A");
    else if (scrollbar_dnButton(ev->xany.window, ev->xbutton.y))
      tt_printf((unsigned char *) "\033[B");
    else
      switch (ev->xbutton.button) {
	case Button2:
	  tt_printf((unsigned char *) "\014");
	  break;
	case Button1:
	  tt_printf((unsigned char *) "\033[6~");
	  break;
	case Button3:
	  tt_printf((unsigned char *) "\033[5~");
	  break;
      }
  } else
#endif /* NO_SCROLLBAR_REPORT */
  {
    D_EVENTS(("ButtonPress event for window 0x%08x at %d, %d\n", ev->xany.window, ev->xbutton.x, ev->xbutton.y));
    D_EVENTS(("  up [0x%08x], down [0x%08x], anchor [0x%08x], trough [0x%08x]\n", scrollBar.up_win, scrollBar.dn_win, scrollBar.sa_win, scrollBar.win));

    if (scrollbar_upButton(ev->xany.window, ev->xbutton.y)) {

      if (scrollbar_uparrow_is_pixmapped() && images[image_up].current != images[image_up].clicked) {
	images[image_up].current = images[image_up].clicked;
	render_simage(images[image_up].current, scrollbar_get_uparrow_win(), scrollbar_arrow_width(), scrollbar_arrow_width(), image_up, 0);
      }
#ifdef SCROLLBAR_BUTTON_CONTINUAL_SCROLLING
      scroll_arrow_delay = SCROLLBAR_INITIAL_DELAY;
#endif
      if (scr_page(UP, 1)) {
	scrollbar_setUp();
      }
    } else if (scrollbar_dnButton(ev->xany.window, ev->xbutton.y)) {

      if (scrollbar_downarrow_is_pixmapped() && images[image_down].current != images[image_down].clicked) {
	images[image_down].current = images[image_down].clicked;
	render_simage(images[image_down].current, scrollbar_get_downarrow_win(), scrollbar_arrow_width(), scrollbar_arrow_width(), image_down, 0);
      }
#ifdef SCROLLBAR_BUTTON_CONTINUAL_SCROLLING
      scroll_arrow_delay = SCROLLBAR_INITIAL_DELAY;
#endif
      if (scr_page(DN, 1)) {
	scrollbar_setDn();
      }
    } else {
      if (scrollbar_anchor_is_pixmapped() && images[image_sa].current != images[image_sa].clicked) {
        images[image_sa].current = images[image_sa].clicked;
        render_simage(images[image_sa].current, scrollbar_get_anchor_win(), scrollbar_anchor_width(), scrollbar_anchor_height(), image_sa, 0);
      }
      switch (ev->xbutton.button) {
	case Button2:
	  button_state.mouse_offset = scrollbar_anchor_height() / 2;	/* Align to center */
	  if (scrollbar_is_above_anchor(ev->xany.window, ev->xbutton.y)
	      || scrollbar_is_below_anchor(ev->xany.window, ev->xbutton.y)
	      || scrollBar.type == SCROLLBAR_XTERM) {
	    scr_move_to(scrollbar_position(ev->xbutton.y) - button_state.mouse_offset, scrollbar_scrollarea_height());
	  }
	  scrollbar_setMotion();
	  break;

	case Button1:
	  button_state.mouse_offset = ev->xbutton.y - (scrollbar_anchor_is_pixmapped()? 0 : scrollBar.top);
	  MAX_IT(button_state.mouse_offset, 1);
	  /* drop */
	case Button3:
#if defined(MOTIF_SCROLLBAR) || defined(NEXT_SCROLLBAR)
	  if (scrollBar.type == SCROLLBAR_MOTIF || scrollBar.type == SCROLLBAR_NEXT) {
	    if (scrollbar_is_above_anchor(ev->xany.window, ev->xbutton.y)) {
	      if (images[image_sb].current != images[image_sb].clicked) {
		images[image_sb].current = images[image_sb].clicked;
		render_simage(images[image_sb].current, scrollbar_get_win(), scrollbar_trough_width(), scrollbar_trough_height(), image_sb, 0);
	      }
	      scr_page(UP, TermWin.nrow - 1);
	    } else if (scrollbar_is_below_anchor(ev->xany.window, ev->xbutton.y)) {
	      if (images[image_sb].current != images[image_sb].clicked) {
		images[image_sb].current = images[image_sb].clicked;
		render_simage(images[image_sb].current, scrollbar_get_win(), scrollbar_trough_width(), scrollbar_trough_height(), image_sb, 0);
	      }
	      scr_page(DN, TermWin.nrow - 1);
	    } else {
	      if (scrollbar_anchor_is_pixmapped() && images[image_sa].current != images[image_sa].clicked) {
		images[image_sa].current = images[image_sa].clicked;
		render_simage(images[image_sa].current, scrollbar_get_anchor_win(), scrollbar_anchor_width(), scrollbar_anchor_height(), image_sa, 0);
	      }
	      scrollbar_setMotion();
	    }
	  }
#endif /* MOTIF_SCROLLBAR || NEXT_SCROLLBAR */

#ifdef XTERM_SCROLLBAR
	  if (scrollBar.type == SCROLLBAR_XTERM) {
	    scr_page((ev->xbutton.button == Button1 ? DN : UP), (TermWin.nrow * scrollbar_position(ev->xbutton.y) / scrollbar_scrollarea_height()));
	  }
#endif /* XTERM_SCROLLBAR */
	  break;
      }
    }
  }

  return 1;
}

unsigned char
sb_handle_button_release(event_t * ev)
{

  Window root, child;
  int root_x, root_y, win_x, win_y;
  unsigned int mask;

  D_EVENTS(("sb_handle_button_release(ev [%8p] on window 0x%08x)\n", ev, ev->xany.window));

  REQUIRE_RVAL(XEVENT_IS_MYWIN(ev, &scrollbar_event_data), 0);
  button_state.mouse_offset = 0;
  button_state.report_mode = (button_state.bypass_keystate ? 0 : ((PrivateModes & PrivMode_mouse_report) ? 1 : 0));

#ifdef PIXMAP_SCROLLBAR
  XQueryPointer(Xdisplay, scrollbar_get_win(), &root, &child, &root_x, &root_y, &win_x, &win_y, &mask);
  if (scrollbar_uparrow_is_pixmapped()) {
    if (scrollbar_win_is_uparrow(child)) {
      images[image_up].current = images[image_up].selected;
      render_simage(images[image_up].current, scrollbar_get_uparrow_win(), scrollbar_arrow_width(), scrollbar_arrow_width(), image_up, 0);
    } else if (images[image_up].current != images[image_up].norm) {
      images[image_up].current = images[image_up].norm;
      render_simage(images[image_up].current, scrollbar_get_uparrow_win(), scrollbar_arrow_width(), scrollbar_arrow_width(), image_up, 0);
    }
  }
  if (scrollbar_downarrow_is_pixmapped()) {
    if (scrollbar_win_is_downarrow(child)) {
      images[image_down].current = images[image_down].selected;
      render_simage(images[image_down].current, scrollbar_get_downarrow_win(), scrollbar_arrow_width(), scrollbar_arrow_width(), image_down, 0);
    } else if (images[image_down].current != images[image_down].norm) {
      images[image_down].current = images[image_down].norm;
      render_simage(images[image_down].current, scrollbar_get_downarrow_win(), scrollbar_arrow_width(), scrollbar_arrow_width(), image_down, 0);
    }
  }
  if (scrollbar_anchor_is_pixmapped()) {
    if (scrollbar_win_is_anchor(child)) {
      images[image_sa].current = images[image_sa].selected;
      render_simage(images[image_sa].current, scrollbar_get_anchor_win(), scrollbar_anchor_width(), scrollbar_anchor_height(), image_sa, 0);
    } else if (images[image_sa].current != images[image_sa].norm) {
      images[image_sa].current = images[image_sa].norm;
      render_simage(images[image_sa].current, scrollbar_get_anchor_win(), scrollbar_anchor_width(), scrollbar_anchor_height(), image_sa, 0);
    }
  }
  if (scrollbar_is_pixmapped()) {
    if (scrollbar_win_is_scrollbar(child)) {
      images[image_sb].current = images[image_sb].selected;
      render_simage(images[image_sb].current, scrollbar_get_win(), scrollbar_trough_width(), scrollbar_trough_height(), image_sb, 0);
    } else if (images[image_sb].current != images[image_sb].norm) {
      images[image_sb].current = images[image_sb].norm;
      render_simage(images[image_sb].current, scrollbar_get_win(), scrollbar_trough_width(), scrollbar_trough_height(), image_sb, 0);
    }
  }
#endif

  if (scrollbar_isUpDn()) {
    scrollbar_setNone();
    scrollbar_show(0);
  }
  return 1;
}

unsigned char
sb_handle_motion_notify(event_t * ev)
{

  D_EVENTS(("sb_handle_motion_notify(ev [%8p] on window 0x%08x)\n", ev, ev->xany.window));

  REQUIRE_RVAL(XEVENT_IS_MYWIN(ev, &scrollbar_event_data), 0);
  if ((PrivateModes & PrivMode_mouse_report) && !(button_state.bypass_keystate))
    return 1;

  D_EVENTS(("MotionNotify event for window 0x%08x\n", ev->xany.window));
  D_EVENTS(("  up [0x%08x], down [0x%08x], anchor [0x%08x], trough [0x%08x]\n", scrollBar.up_win, scrollBar.dn_win, scrollBar.sa_win, scrollBar.win));

  if ((scrollbar_win_is_scrollbar(ev->xany.window)
#ifdef PIXMAP_SCROLLBAR
       || (scrollbar_anchor_is_pixmapped() && scrollbar_win_is_anchor(ev->xany.window))
#endif
      ) && scrollbar_isMotion()) {

    Window unused_root, unused_child;
    int unused_root_x, unused_root_y;
    unsigned int unused_mask;

    while (XCheckTypedWindowEvent(Xdisplay, scrollBar.win, MotionNotify, ev));
    XQueryPointer(Xdisplay, scrollBar.win, &unused_root, &unused_child, &unused_root_x, &unused_root_y, &(ev->xbutton.x), &(ev->xbutton.y), &unused_mask);
    scr_move_to(scrollbar_position(ev->xbutton.y) - button_state.mouse_offset, scrollbar_scrollarea_height());
    refresh_count = refresh_limit = 0;
    scr_refresh(refresh_type);
    scrollbar_show(button_state.mouse_offset);
  }
  return 1;
}

unsigned char
scrollbar_dispatch_event(event_t * ev)
{
  if (scrollbar_event_data.handlers[ev->type] != NULL) {
    return ((scrollbar_event_data.handlers[ev->type]) (ev));
  }
  return (0);
}

unsigned char
scrollbar_mapping(unsigned char show)
{

  unsigned char change = 0;

  D_SCROLLBAR(("scrollbar_mapping(%d)\n", show));

  if (show && !scrollbar_visible()) {
    D_SCROLLBAR((" -> Mapping all scrollbar windows.  Returning 1.\n"));
    scrollBar.state = 1;
    XMapWindow(Xdisplay, scrollBar.win);
#ifdef PIXMAP_SCROLLBAR
    if (scrollbar_uparrow_is_pixmapped()) {
      XMapWindow(Xdisplay, scrollBar.up_win);
    }
    if (scrollbar_downarrow_is_pixmapped()) {
      XMapWindow(Xdisplay, scrollBar.dn_win);
    }
    if (scrollbar_anchor_is_pixmapped()) {
      XMapWindow(Xdisplay, scrollBar.sa_win);
    }
#endif
    change = 1;
  } else if (!show && scrollbar_visible()) {
    D_SCROLLBAR((" -> Unmapping all scrollbar windows.  Returning 1.\n"));
    scrollBar.state = 0;
#ifdef PIXMAP_SCROLLBAR
    if (scrollbar_uparrow_is_pixmapped()) {
      XUnmapWindow(Xdisplay, scrollBar.up_win);
    }
    if (scrollbar_downarrow_is_pixmapped()) {
      XUnmapWindow(Xdisplay, scrollBar.dn_win);
    }
    if (scrollbar_anchor_is_pixmapped()) {
      XUnmapWindow(Xdisplay, scrollBar.sa_win);
    }
#endif
    XUnmapWindow(Xdisplay, scrollBar.win);
    change = 1;
  } else {
    D_SCROLLBAR((" -> No action required.  Returning 0.\n"));
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
#if defined(MOTIF_SCROLLBAR) || defined(NEXT_SCROLLBAR)
  if (topShadowGC != None) {
    XFreeGC(Xdisplay, topShadowGC);
    topShadowGC = None;
  }
  if (botShadowGC != None) {
    XFreeGC(Xdisplay, botShadowGC);
    botShadowGC = None;
  }
#endif
#ifdef XTERM_SCROLLBAR
  if (shadowGC != None) {
    XFreeGC(Xdisplay, shadowGC);
    shadowGC = None;
  }
#endif
  last_top = last_bot = 0;
  scrollBar.init = 0;
}

void
scrollbar_resize(int width, int height)
{

  if (!scrollbar_visible()) {
    return;
  }

  D_SCROLLBAR(("scrollbar_resize(%d, %d)\n", width, height));
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
    scrollBar.end -= (scrollBar.width * 2 + (scrollbar_get_shadow() ? scrollbar_get_shadow() : 1) + 2);
  }
#endif
  width -= scrollbar_trough_width();
  XMoveResizeWindow(Xdisplay, scrollBar.win, ((Options & Opt_scrollBar_right) ? (width) : (0)), 0, scrollbar_trough_width(), height);
  D_X11((" -> New scrollbar width/height == %lux%lu\n", scrollbar_trough_width(), height));
  scrollBar.init = 0;
  scrollbar_show(0);
}

unsigned char
scrollbar_show(short mouseoffset)
{

  unsigned char xsb = 0, force_update = 0;
  static int focus = -1;

  if (!scrollbar_visible()) {
    return 0;
  }

  D_SCROLLBAR(("scrollbar_show(%d)\n", mouseoffset));

  if (scrollbarGC == None) {

    XGCValues gcvalue;

#ifdef XTERM_SCROLLBAR
    if (scrollBar.type == SCROLLBAR_XTERM) {
      gcvalue.stipple = XCreateBitmapFromData(Xdisplay, scrollBar.win, (char *) xterm_sb_bits, 12, 2);
      if (!gcvalue.stipple) {
	print_error("Unable to create xterm scrollbar bitmap.  Reverting to default scrollbar.");
	scrollBar.type = SCROLLBAR_MOTIF;
      } else {
	gcvalue.fill_style = FillOpaqueStippled;
	gcvalue.foreground = PixColors[fgColor];
	gcvalue.background = PixColors[bgColor];
	scrollbarGC = XCreateGC(Xdisplay, scrollBar.win, GCForeground | GCBackground | GCFillStyle | GCStipple, &gcvalue);
	gcvalue.foreground = PixColors[borderColor];
	shadowGC = XCreateGC(Xdisplay, scrollBar.win, GCForeground, &gcvalue);
      }
    }
#endif /* XTERM_SCROLLBAR */

#if defined(MOTIF_SCROLLBAR) || defined(NEXT_SCROLLBAR)
    if (scrollBar.type == SCROLLBAR_MOTIF || scrollBar.type == SCROLLBAR_NEXT) {

      gcvalue.foreground = (Xdepth <= 2 ? PixColors[fgColor] : PixColors[scrollColor]);
      scrollbarGC = XCreateGC(Xdisplay, scrollBar.win, GCForeground, &gcvalue);

      if ((Options & Opt_scrollBar_floating) && !scrollbar_is_pixmapped() && !background_is_pixmap()) {
        XSetWindowBackground(Xdisplay, scrollBar.win, PixColors[bgColor]);
      } else {
	render_simage(images[image_sb].current, scrollBar.win, scrollbar_trough_width(), scrollbar_trough_height(), image_sb, 0);
        if (image_mode_is(image_sb, MODE_AUTO)) {
          enl_ipc_sync();
          XClearWindow(Xdisplay, scrollBar.win);
        }
      }
      gcvalue.foreground = PixColors[topShadowColor];
      topShadowGC = XCreateGC(Xdisplay, scrollBar.win, GCForeground, &gcvalue);

      gcvalue.foreground = PixColors[bottomShadowColor];
      botShadowGC = XCreateGC(Xdisplay, scrollBar.win, GCForeground, &gcvalue);
    }
#endif /* MOTIF_SCROLLBAR || NEXT_SCROLLBAR */
  }
#if defined(MOTIF_SCROLLBAR) || defined(NEXT_SCROLLBAR)
  else if (image_mode_is(image_sb, (MODE_TRANS | MODE_VIEWPORT))) {
    render_simage(images[image_sb].current, scrollBar.win, scrollbar_trough_width(), scrollbar_trough_height(), image_sb, 0);
  }
  if (scrollBar.type == SCROLLBAR_MOTIF || scrollBar.type == SCROLLBAR_NEXT) {
    if (focus != TermWin.focus) {
      XGCValues gcvalue;

      focus = TermWin.focus;
      gcvalue.foreground = PixColors[focus ? scrollColor : unfocusedScrollColor];
      XChangeGC(Xdisplay, scrollbarGC, GCForeground, &gcvalue);

      gcvalue.foreground = PixColors[focus ? topShadowColor : unfocusedTopShadowColor];
      XChangeGC(Xdisplay, topShadowGC, GCForeground, &gcvalue);

      gcvalue.foreground = PixColors[focus ? bottomShadowColor : unfocusedBottomShadowColor];
      XChangeGC(Xdisplay, botShadowGC, GCForeground, &gcvalue);
      force_update = 1;
    }
  }
#endif /* MOTIF_SCROLLBAR || NEXT_SCROLLBAR */

  if (mouseoffset) {
    int top = (TermWin.nscrolled - TermWin.view_start);
    int bot = top + (TermWin.nrow - 1);
    int len = max((TermWin.nscrolled + (TermWin.nrow - 1)), 1);

    scrollBar.top = (scrollBar.beg + (top * scrollbar_scrollarea_height()) / len);
    scrollBar.bot = (scrollBar.beg + (bot * scrollbar_scrollarea_height()) / len);

    if (rs_min_anchor_size && scrollBar.type != SCROLLBAR_XTERM) {
      if ((scrollbar_scrollarea_height() > rs_min_anchor_size) && (scrollbar_anchor_height() < rs_min_anchor_size)) {

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
	  scr_move_to(scrollBar.end, scrollbar_scrollarea_height());
	  scr_refresh(DEFAULT_REFRESH);
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
    XClearArea(Xdisplay, scrollBar.win, scrollbar_get_shadow() + xsb, last_top, scrollBar.width, (scrollBar.top - last_top), False);
  }
  if (scrollBar.bot < last_bot) {
    XClearArea(Xdisplay, scrollBar.win, scrollbar_get_shadow() + xsb, scrollBar.bot, scrollBar.width, (last_bot - scrollBar.bot), False);
  }
#ifdef PIXMAP_SCROLLBAR
  if (scrollbar_anchor_is_pixmapped()) {
    if ((last_top != scrollBar.top) || (scrollBar.bot != last_bot)) {
      XMoveResizeWindow(Xdisplay, scrollBar.sa_win, scrollbar_get_shadow(), scrollBar.top, scrollBar.width, scrollbar_anchor_height());
    }
    if (scrollbar_anchor_height() > 1) {
      render_simage(images[image_sa].current, scrollBar.sa_win, scrollbar_anchor_width(), scrollbar_anchor_height(), image_sa, 0);
    }
  }
#endif
  last_top = scrollBar.top;
  last_bot = scrollBar.bot;

  /* scrollbar anchor */
#ifdef XTERM_SCROLLBAR
  if (scrollBar.type == SCROLLBAR_XTERM) {
    XFillRectangle(Xdisplay, scrollBar.win, scrollbarGC, xsb + 1, scrollBar.top, scrollBar.width - 2, scrollbar_anchor_height());
    XDrawLine(Xdisplay, scrollBar.win, shadowGC, xsb ? 0 : scrollBar.width, scrollBar.beg, xsb ? 0 : scrollBar.width, scrollBar.end);
  }
#endif /* XTERM_SCROLLBAR */

#if defined(MOTIF_SCROLLBAR) || defined(NEXT_SCROLLBAR)
  if (scrollBar.type == SCROLLBAR_MOTIF || scrollBar.type == SCROLLBAR_NEXT) {
    if (!scrollbar_anchor_is_pixmapped()) {
      /* Draw the scrollbar anchor in if it's not pixmapped. */
      if (scrollbar_is_pixmapped()) {
	XSetTSOrigin(Xdisplay, scrollbarGC, scrollbar_get_shadow(), scrollBar.top);
	XFillRectangle(Xdisplay, scrollBar.win, scrollbarGC, scrollbar_get_shadow(), scrollBar.top, scrollBar.width, scrollbar_anchor_height());
	XSetTSOrigin(Xdisplay, scrollbarGC, 0, 0);
      } else {
	XFillRectangle(Xdisplay, scrollBar.win, scrollbarGC, scrollbar_get_shadow(), scrollBar.top, scrollBar.width, scrollbar_anchor_height());
      }
    }
    /* Draw trough shadow */
    if (scrollbar_get_shadow() && !scrollbar_is_pixmapped()) {
      Draw_Shadow(scrollBar.win, botShadowGC, topShadowGC, 0, 0, scrollbar_trough_width(), scrollbar_trough_height());
    }
    /* Draw anchor shadow */
    if (!scrollbar_anchor_is_pixmapped()) {
      Draw_Shadow(scrollBar.win, topShadowGC, botShadowGC, scrollbar_get_shadow(), scrollBar.top, scrollBar.width, scrollbar_anchor_height());
    }
    /* Draw scrollbar arrows */
    if (scrollbar_uparrow_is_pixmapped()) {
      if (scrollBar.init == 0) {
        XMoveResizeWindow(Xdisplay, scrollBar.up_win, scrollbar_get_shadow(), scrollbar_up_loc(), scrollbar_arrow_width(), scrollbar_arrow_height());
        render_simage(images[image_up].current, scrollBar.up_win, scrollbar_arrow_width(), scrollbar_arrow_width(), image_up, 0);
      }
    } else {
      Draw_up_button(scrollbar_get_shadow(), scrollbar_up_loc(), (scrollbar_isUp()? -1 : +1));
    }
    if (scrollbar_downarrow_is_pixmapped()) {
      if (scrollBar.init == 0) {
        XMoveResizeWindow(Xdisplay, scrollBar.dn_win, scrollbar_get_shadow(), scrollbar_dn_loc(), scrollbar_arrow_width(), scrollbar_arrow_height());
        render_simage(images[image_down].current, scrollBar.dn_win, scrollbar_arrow_width(), scrollbar_arrow_width(), image_down, 0);
      }
    } else {
      Draw_dn_button(scrollbar_get_shadow(), scrollbar_dn_loc(), (scrollbar_isDn()? -1 : +1));
    }
  }
  if (image_mode_is(image_up, MODE_AUTO) || image_mode_is(image_down, MODE_AUTO) || image_mode_is(image_sb, MODE_AUTO) || image_mode_is(image_sa, MODE_AUTO)) {
    //    enl_ipc_sync();
  }
#endif /* MOTIF_SCROLLBAR || NEXT_SCROLLBAR */

  scrollBar.init = 1;
  return 1;
}
