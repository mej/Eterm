/* menus.c -- Eterm popup menu module

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
#include "../libmej/strings.h"
#include "command.h"
#include "e.h"
#include "events.h"
#include "font.h"
#include "startup.h"
#include "menus.h"
#include "misc.h"
#include "options.h"
#include "pixmap.h"
#include "screen.h"
#include "term.h"
#include "windows.h"

event_dispatcher_data_t menu_event_data;
menulist_t *menu_list = NULL;
static GC topShadowGC, botShadowGC;
static Time button_press_time;
static menu_t *current_menu;

static inline void grab_pointer(Window win);
static inline void ungrab_pointer(void);
static inline void draw_string(Drawable d, GC gc, int x, int y, char *str, size_t len);
static inline unsigned short center_coords(register unsigned short c1, register unsigned short c2);

static inline void
grab_pointer(Window win)
{

  int success;

  D_EVENTS(("grab_pointer():  Grabbing control of pointer for window 0x%08x.\n", win));
  success = XGrabPointer(Xdisplay, win, False,
			 EnterWindowMask | LeaveWindowMask | PointerMotionMask | ButtonMotionMask | ButtonPressMask | ButtonReleaseMask
			 | Button1MotionMask | Button2MotionMask | Button3MotionMask,
			 GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
  if (success != GrabSuccess) {
    switch (success) {
      case GrabNotViewable:
	D_MENU((" -> Unable to grab pointer -- Grab window is not viewable.\n"));
	break;
      case AlreadyGrabbed:
	D_MENU((" -> Unable to grab pointer -- Pointer is already grabbed by another client.\n"));
	break;
      case GrabFrozen:
	D_MENU((" -> Unable to grab pointer -- Pointer is frozen by another grab.\n"));
	break;
      case GrabInvalidTime:
	D_MENU((" -> Unable to grab pointer -- Invalid grab time.\n"));
	break;
      default:
	break;
    }
  }
}

static inline void
ungrab_pointer(void)
{

  D_EVENTS(("ungrab_pointer():  Releasing pointer grab.\n"));
  XUngrabPointer(Xdisplay, CurrentTime);
}

static inline void
draw_string(Drawable d, GC gc, int x, int y, char *str, size_t len)
{

  /*D_MENU(("draw_string():  Writing string \"%s\" (length %lu) onto drawable 0x%08x at %d, %d\n", str, len, d, x, y)); */

#ifdef MULTI_CHARSET
  if (current_menu && current_menu->fontset)
    XmbDrawString(Xdisplay, d, current_menu->fontset, gc, x, y, str, len);
  else
#endif
    XDrawString(Xdisplay, d, gc, x, y, str, len);
}

static inline unsigned short
center_coords(register unsigned short c1, register unsigned short c2)
{

  return (((c2 - c1) >> 1) + c1);
}

void
menu_init(void)
{

  XGCValues gcvalue;

  if (!menu_list || menu_list->nummenus == 0) {
    return;
  }
  gcvalue.foreground = PixColors[menuTopShadowColor];
  topShadowGC = XCreateGC(Xdisplay, menu_list->menus[0]->win, GCForeground, &gcvalue);
  gcvalue.foreground = PixColors[menuBottomShadowColor];
  botShadowGC = XCreateGC(Xdisplay, menu_list->menus[0]->win, GCForeground, &gcvalue);

  event_register_dispatcher(menu_dispatch_event, menu_event_init_dispatcher);
}

void
menu_event_init_dispatcher(void)
{
  register unsigned char i;

  MEMSET(&menu_event_data, 0, sizeof(event_dispatcher_data_t));

  EVENT_DATA_ADD_HANDLER(menu_event_data, EnterNotify, menu_handle_enter_notify);
  EVENT_DATA_ADD_HANDLER(menu_event_data, LeaveNotify, menu_handle_leave_notify);
  EVENT_DATA_ADD_HANDLER(menu_event_data, GraphicsExpose, menu_handle_expose);
  EVENT_DATA_ADD_HANDLER(menu_event_data, Expose, menu_handle_expose);
  EVENT_DATA_ADD_HANDLER(menu_event_data, ButtonPress, menu_handle_button_press);
  EVENT_DATA_ADD_HANDLER(menu_event_data, ButtonRelease, menu_handle_button_release);
  EVENT_DATA_ADD_HANDLER(menu_event_data, MotionNotify, menu_handle_motion_notify);

  for (i = 0; i < menu_list->nummenus; i++) {
    event_data_add_mywin(&menu_event_data, menu_list->menus[i]->win);
  }

  event_data_add_parent(&menu_event_data, TermWin.vt);
  event_data_add_parent(&menu_event_data, TermWin.parent);

}

unsigned char
menu_handle_enter_notify(event_t * ev)
{

  register menu_t *menu;

  D_EVENTS(("menu_handle_enter_notify(ev [%8p] on window 0x%08x)\n", ev, ev->xany.window));

  REQUIRE_RVAL(XEVENT_IS_MYWIN(ev, &menu_event_data), 0);

  /* Take control of the pointer so we get all events for it, even those outside the menu window */
  menu = find_menu_by_window(menu_list, ev->xany.window);
  if (menu && menu != current_menu) {
    ungrab_pointer();
    if (menu->state & MENU_STATE_IS_MAPPED) {
      grab_pointer(menu->win);
      menu->state |= MENU_STATE_IS_FOCUSED;
      current_menu = menu;
      menu_reset_submenus(menu);
      menuitem_change_current(find_item_by_coords(current_menu, ev->xbutton.x, ev->xbutton.y));
    }
  }
  return 1;
}

unsigned char
menu_handle_leave_notify(event_t * ev)
{

  D_EVENTS(("menu_handle_leave_notify(ev [%8p] on window 0x%08x)\n", ev, ev->xany.window));

  REQUIRE_RVAL(XEVENT_IS_MYWIN(ev, &menu_event_data), 0);

  if (current_menu) {
    current_menu->state &= ~(MENU_STATE_IS_FOCUSED);
  }
  return 0;
}

unsigned char
menu_handle_focus_in(event_t * ev)
{

  D_EVENTS(("menu_handle_focus_in(ev [%8p] on window 0x%08x)\n", ev, ev->xany.window));

  REQUIRE_RVAL(XEVENT_IS_MYWIN(ev, &menu_event_data), 0);

  return 0;
}

unsigned char
menu_handle_focus_out(event_t * ev)
{

  D_EVENTS(("menu_handle_focus_out(ev [%8p] on window 0x%08x)\n", ev, ev->xany.window));

  REQUIRE_RVAL(XEVENT_IS_MYWIN(ev, &menu_event_data), 0);

  return 0;
}

unsigned char
menu_handle_expose(event_t * ev)
{

  XEvent unused_xevent;
  menu_t *menu;

  D_EVENTS(("menu_handle_expose(ev [%8p] on window 0x%08x)\n", ev, ev->xany.window));

  REQUIRE_RVAL(XEVENT_IS_MYWIN(ev, &menu_event_data), 0);

  while (XCheckTypedWindowEvent(Xdisplay, ev->xany.window, Expose, &unused_xevent));
  while (XCheckTypedWindowEvent(Xdisplay, ev->xany.window, GraphicsExpose, &unused_xevent));
  if ((menu = find_menu_by_window(menu_list, ev->xany.window)) != NULL) {
    menu_draw(menu);
  }
  return 1;
}

unsigned char
menu_handle_button_press(event_t * ev)
{

  D_EVENTS(("menu_handle_button_press(ev [%8p] on window 0x%08x)\n", ev, ev->xany.window));

  REQUIRE_RVAL(XEVENT_IS_MYWIN(ev, &menu_event_data), 0);

  D_EVENTS(("ButtonPress at %d, %d\n", ev->xbutton.x, ev->xbutton.y));

  button_press_time = ev->xbutton.time;

  return 1;
}

unsigned char
menu_handle_button_release(event_t * ev)
{
  menuitem_t *item;

  D_EVENTS(("menu_handle_button_release(ev [%8p] on window 0x%08x)\n", ev, ev->xany.window));

  REQUIRE_RVAL(XEVENT_IS_MYWIN(ev, &menu_event_data), 0);

  D_EVENTS(("ButtonRelease at %d, %d\n", ev->xbutton.x, ev->xbutton.y));

  if (current_menu && (current_menu->state & MENU_STATE_IS_DRAGGING)) {

    /* Dragging-and-release mode */
    D_MENU(("Drag-and-release mode, detected release.\n"));
    ungrab_pointer();

    if (button_press_time && (ev->xbutton.time - button_press_time > MENU_CLICK_TIME)) {
      /* Take action here based on the current menu item */
      if (current_menu) {
        if ((item = menuitem_get_current(current_menu)) != NULL) {
          if (item->type == MENUITEM_SUBMENU) {
            menu_display_submenu(current_menu, item);
          } else {
            menu_action(item);
            menuitem_deselect(current_menu);
          }
	}
      }
    }
    /* Reset the state of the menu system. */
    menu_reset_all(menu_list);
    current_menu = NULL;

  } else {

    /* Single-click mode */
    D_MENU(("Single click mode, detected click.\n"));
    if ((ev->xbutton.x >= 0) && (ev->xbutton.y >= 0) && (ev->xbutton.x < current_menu->w) && (ev->xbutton.y < current_menu->h)) {
      /* Click inside the menu window.  Activate the current item. */
      if (current_menu) {
        if ((item = menuitem_get_current(current_menu)) != NULL) {
          if (item->type == MENUITEM_SUBMENU) {
            menu_display_submenu(current_menu, item);
          } else {
            menu_action(item);
            menuitem_deselect(current_menu);
            menu_reset_all(menu_list);
          }
	}
      }
    } else {
      ungrab_pointer();
      /* Reset the state of the menu system. */
      menu_reset_all(menu_list);
      current_menu = NULL;
    }
  }
  button_press_time = 0;

  return 1;
}

unsigned char
menu_handle_motion_notify(event_t * ev)
{

  register menuitem_t *item = NULL;

  D_EVENTS(("menu_handle_motion_notify(ev [%8p] on window 0x%08x)\n", ev, ev->xany.window));

  REQUIRE_RVAL(XEVENT_IS_MYWIN(ev, &menu_event_data), 0);

  while (XCheckTypedWindowEvent(Xdisplay, ev->xany.window, MotionNotify, ev));
  if (!current_menu) {
    return 1;
  }

  if (button_press_time) {
    current_menu->state |= MENU_STATE_IS_DRAGGING;
  }
  if ((ev->xbutton.x >= 0) && (ev->xbutton.y >= 0) && (ev->xbutton.x < current_menu->w) && (ev->xbutton.y < current_menu->h)) {
    /* Motion within the current menu */
    item = find_item_by_coords(current_menu, ev->xbutton.x, ev->xbutton.y);
    menuitem_change_current(item);
  } else {
    /* Motion outside the current menu */
    int dest_x, dest_y;
    Window child;
    menu_t *menu;

    XTranslateCoordinates(Xdisplay, ev->xany.window, Xroot, ev->xbutton.x, ev->xbutton.y, &dest_x, &dest_y, &child);
    menu = find_menu_by_window(menu_list, child);
    if (menu && menu != current_menu) {
      D_MENU(("Mouse is actually over window 0x%08x belonging to menu \"%s\"\n", child, menu->title));
      ungrab_pointer();
      grab_pointer(menu->win);
      current_menu->state &= ~(MENU_STATE_IS_FOCUSED);
      menu->state |= MENU_STATE_IS_FOCUSED;
      if (!menu_is_child(current_menu, menu)) {
        menu_reset_tree(current_menu);
      }
      current_menu = menu;
      XTranslateCoordinates(Xdisplay, ev->xany.window, child, ev->xbutton.x, ev->xbutton.y, &dest_x, &dest_y, &child);
      item = find_item_by_coords(menu, dest_x, dest_y);
      if (!item || item != menuitem_get_current(current_menu)) {
        menu_reset_submenus(current_menu);
      }
      menuitem_change_current(item);
    }
  }

  return 1;
}

unsigned char
menu_dispatch_event(event_t * ev)
{
  if (menu_event_data.handlers[ev->type] != NULL) {
    return ((menu_event_data.handlers[ev->type]) (ev));
  }
  return (0);
}

menulist_t *
menulist_add_menu(menulist_t * list, menu_t * menu)
{

  ASSERT_RVAL(menu != NULL, list);

  if (list) {
    list->nummenus++;
    list->menus = (menu_t **) REALLOC(list->menus, sizeof(menu_t *) * list->nummenus);
  } else {
    list = (menulist_t *) MALLOC(sizeof(menulist_t));
    list->nummenus = 1;
    list->menus = (menu_t **) MALLOC(sizeof(menu_t *));
  }
  list->menus[list->nummenus - 1] = menu;
  return list;
}

menu_t *
menu_create(char *title)
{

  menu_t *menu;
  static Cursor cursor;
  static long mask;
  static XGCValues gcvalue;
  static XSetWindowAttributes xattr;

  ASSERT_RVAL(title != NULL, NULL);

  if (!mask) {
    xattr.border_pixel = BlackPixel(Xdisplay, Xscreen);
    xattr.save_under = TRUE;
    xattr.backing_store = WhenMapped;
    xattr.override_redirect = TRUE;
    xattr.colormap = cmap;

    cursor = XCreateFontCursor(Xdisplay, XC_left_ptr);
    mask = EnterNotify | LeaveNotify | PointerMotionMask | ButtonMotionMask | ButtonPressMask | ButtonReleaseMask
	| Button1MotionMask | Button2MotionMask | Button3MotionMask;
    gcvalue.foreground = PixColors[menuTextColor];
  }
  menu = (menu_t *) MALLOC(sizeof(menu_t));
  MEMSET(menu, 0, sizeof(menu_t));
  menu->title = StrDup(title);

  menu->win = XCreateWindow(Xdisplay, Xroot, 0, 0, 1, 1, 0, Xdepth, InputOutput, CopyFromParent,
			    CWOverrideRedirect | CWSaveUnder | CWBackingStore | CWBorderPixel | CWColormap, &xattr);
  XDefineCursor(Xdisplay, menu->win, cursor);
  XSelectInput(Xdisplay, menu->win, mask);
  XStoreName(Xdisplay, menu->win, menu->title);

  menu->swin = XCreateWindow(Xdisplay, menu->win, 0, 0, 1, 1, 0, Xdepth, InputOutput, CopyFromParent,
			     CWOverrideRedirect | CWSaveUnder | CWBackingStore | CWBorderPixel | CWColormap, &xattr);

  menu->gc = XCreateGC(Xdisplay, menu->win, GCForeground, &gcvalue);
  menuitem_clear_current(menu);

  return menu;
}

unsigned char
menu_set_font(menu_t * menu, const char *fontname)
{

  XFontStruct *font;
  XGCValues gcvalue;

  ASSERT_RVAL(menu != NULL, 0);
  ASSERT_RVAL(fontname != NULL, 0);

  font = (XFontStruct *) load_font(fontname, "fixed", FONT_TYPE_X);
#ifdef MULTI_CHARSET
  menu->fontset = create_fontset(fontname, rs_mfont[0]);
#endif

  menu->font = font;
  menu->fwidth = font->max_bounds.width;
  menu->fheight = font->ascent + font->descent + rs_line_space;

  gcvalue.font = font->fid;
  XChangeGC(Xdisplay, menu->gc, GCFont, &gcvalue);

  return 1;
}

unsigned char
menu_add_item(menu_t * menu, menuitem_t * item)
{

  ASSERT_RVAL(menu != NULL, 0);
  ASSERT_RVAL(item != NULL, 0);

  if (menu->numitems) {
    menu->numitems++;
    menu->items = (menuitem_t **) REALLOC(menu->items, sizeof(menuitem_t *) * menu->numitems);
  } else {
    menu->numitems = 1;
    menu->items = (menuitem_t **) MALLOC(sizeof(menuitem_t *));
  }

  menu->items[menu->numitems - 1] = item;
  return 1;

}

/* Return 1 if submenu is a child of menu, 0 if not. */
unsigned char
menu_is_child(menu_t * menu, menu_t * submenu)
{

  register unsigned char i;
  register menuitem_t *item;

  ASSERT_RVAL(menu != NULL, 0);
  ASSERT_RVAL(submenu != NULL, 0);

  for (i = 0; i < menu->numitems; i++) {
    item = menu->items[i];
    if (item->type == MENUITEM_SUBMENU && item->action.submenu != NULL) {
      if (item->action.submenu == submenu) {
	return 1;
      } else if (menu_is_child(item->action.submenu, submenu)) {
	return 1;
      }
    }
  }
  return 0;
}

menu_t *
find_menu_by_title(menulist_t * list, char *title)
{

  register unsigned char i;

  REQUIRE_RVAL(list != NULL, NULL);

  for (i = 0; i < list->nummenus; i++) {
    if (!strcasecmp(list->menus[i]->title, title)) {
      return (list->menus[i]);
    }
  }
  return NULL;
}

menu_t *
find_menu_by_window(menulist_t * list, Window win)
{

  register unsigned char i;

  REQUIRE_RVAL(list != NULL, NULL);

  for (i = 0; i < list->nummenus; i++) {
    if (list->menus[i]->win == win) {
      return (list->menus[i]);
    }
  }
  return NULL;
}

menuitem_t *
find_item_by_coords(menu_t * menu, int x, int y)
{

  register unsigned char i;
  register menuitem_t *item;

  ASSERT_RVAL(menu != NULL, NULL);

  for (i = 0; i < menu->numitems; i++) {
    item = menu->items[i];
    if ((x > item->x) && (y > item->y) && (x < item->x + item->w) && (y < item->y + item->h) && (item->type != MENUITEM_SEP)) {
      return (item);
    }
  }
  return NULL;
}

unsigned short
find_item_in_menu(menu_t * menu, menuitem_t * item)
{

  register unsigned char i;

  ASSERT_RVAL(menu != NULL, (unsigned short) -1);
  ASSERT_RVAL(item != NULL, (unsigned short) -1);

  for (i = 0; i < menu->numitems; i++) {
    if (item == menu->items[i]) {
      return (i);
    }
  }
  return ((unsigned short) -1);
}

void
menuitem_change_current(menuitem_t *item)
{
  menuitem_t *current;

  ASSERT(current_menu != NULL);

  current = menuitem_get_current(current_menu);
  if (current != item) {
    D_MENU(("menuitem_change_current():  Changing current item in menu \"%s\" from \"%s\" to \"%s\"\n", current_menu->title, (current ? current->text : "(NULL)"), (item ? item->text : "(NULL)")));
    if (current) {
      /* Reset the current item */
      menuitem_deselect(current_menu);
      /* If we're changing from one submenu to another and neither is a child of the other, or if we're changing from a submenu to
         no current item at all, reset the tree for the current submenu */
      if (current->type == MENUITEM_SUBMENU && current->action.submenu != NULL) {
	if ((item && item->type == MENUITEM_SUBMENU && item->action.submenu != NULL
	     && !menu_is_child(current->action.submenu, item->action.submenu)
	     && !menu_is_child(item->action.submenu, current->action.submenu))
	    || (!item)) {
	  menu_reset_tree(current->action.submenu);
	}
      }
    }
    if (item) {
      menuitem_set_current(current_menu, find_item_in_menu(current_menu, item));
      menuitem_select(current_menu);
      if (item->type == MENUITEM_SUBMENU) {
	/* Display the submenu */
	menu_display_submenu(current_menu, item);
      }
    } else {
      menuitem_clear_current(current_menu);
    }
  } else {
    D_MENU(("menuitem_change_current():  Current item in menu \"%s\" does not require changing.\n", current_menu->title));
  }
}

menuitem_t *
menuitem_create(char *text)
{

  menuitem_t *menuitem;

  menuitem = (menuitem_t *) MALLOC(sizeof(menuitem_t));
  MEMSET(menuitem, 0, sizeof(menuitem_t));

  if (text) {
    menuitem->text = StrDup(text);
    menuitem->len = strlen(text);
  }
  return menuitem;
}

unsigned char
menuitem_set_icon(menuitem_t * item, image_t * icon)
{

  ASSERT_RVAL(item != NULL, 0);
  ASSERT_RVAL(icon != NULL, 0);

  item->icon = icon;
  return 1;
}

unsigned char
menuitem_set_action(menuitem_t * item, unsigned char type, char *action)
{

  ASSERT_RVAL(item != NULL, 0);

  item->type = type;
  switch (type) {
    case MENUITEM_SUBMENU:
      item->action.submenu = find_menu_by_title(menu_list, action);
      break;
    case MENUITEM_STRING:
    case MENUITEM_ECHO:
      item->action.string = (char *) MALLOC(strlen(action) + 2);
      strcpy(item->action.string, action);
      parse_escaped_string(item->action.string);
      break;
    default:
      break;
  }
  return 1;
}

unsigned char
menuitem_set_rtext(menuitem_t * item, char *rtext)
{

  ASSERT_RVAL(item != NULL, 0);
  ASSERT_RVAL(rtext != NULL, 0);

  item->rtext = StrDup(rtext);
  item->rlen = strlen(rtext);
  return 1;
}

void
menu_reset(menu_t * menu)
{

  ASSERT(menu != NULL);

  D_MENU(("menu_reset() called for menu \"%s\" (window 0x%08x)\n", menu->title, menu->win));
  if (!(menu->state & MENU_STATE_IS_MAPPED)) {
    return;
  }
  menu->state &= ~(MENU_STATE_IS_CURRENT | MENU_STATE_IS_DRAGGING | MENU_STATE_IS_MAPPED);
  XUnmapWindow(Xdisplay, menu->swin);
  XUnmapWindow(Xdisplay, menu->win);
  menuitem_clear_current(menu);
}

void
menu_reset_all(menulist_t * list)
{

  register unsigned short i;

  ASSERT(list != NULL);

  if (list->nummenus == 0)
    return;

  D_MENU(("menu_reset_all() called\n"));
  if (menuitem_get_current(current_menu) != NULL) {
    menuitem_deselect(current_menu);
  }
  for (i = 0; i < list->nummenus; i++) {
    menu_reset(list->menus[i]);
  }
  current_menu = NULL;
}

void
menu_reset_tree(menu_t * menu)
{

  register unsigned short i;
  register menuitem_t *item;

  ASSERT(menu != NULL);

  D_MENU(("menu_reset_tree() called for menu \"%s\" (window 0x%08x)\n", menu->title, menu->win));
  if (!(menu->state & MENU_STATE_IS_MAPPED)) {
    return;
  }
  for (i = 0; i < menu->numitems; i++) {
    item = menu->items[i];
    if (item->type == MENUITEM_SUBMENU && item->action.submenu != NULL) {
      menu_reset_tree(item->action.submenu);
    }
  }
  menu_reset(menu);
}

void
menu_reset_submenus(menu_t * menu)
{

  register unsigned short i;
  register menuitem_t *item;

  ASSERT(menu != NULL);

  D_MENU(("menu_reset_submenus() called for menu \"%s\" (window 0x%08x)\n", menu->title, menu->win));
  for (i = 0; i < menu->numitems; i++) {
    item = menu->items[i];
    if (item->type == MENUITEM_SUBMENU && item->action.submenu != NULL) {
      menu_reset_tree(item->action.submenu);
    }
  }
}

void
menuitem_select(menu_t * menu)
{
  menuitem_t *item;

  ASSERT(menu != NULL);

  item = menuitem_get_current(menu);
  REQUIRE(item != NULL);
  D_MENU(("menuitem_select():  Selecting new current item \"%s\" within menu \"%s\" (window 0x%08x, selection window 0x%08x)\n",
	  item->text, menu->title, menu->win, menu->swin));
  item->state |= MENU_STATE_IS_CURRENT;
  XMoveWindow(Xdisplay, menu->swin, item->x, item->y);
  XMapWindow(Xdisplay, menu->swin);
  if (item->type == MENUITEM_SUBMENU) {
    paste_simage(images[image_submenu].selected, image_submenu, menu->swin, 0, 0, item->w - MENU_VGAP, item->h);
  } else {
    render_simage(images[image_menu].selected, menu->swin, item->w - MENU_VGAP, item->h, image_menu, 0);
    if (image_mode_is(image_menu, MODE_AUTO)) {
      enl_ipc_sync();
    }
  }
  draw_string(menu->swin, menu->gc, MENU_HGAP, item->h - MENU_VGAP, item->text, item->len);
  if (item->rtext) {
    draw_string(menu->swin, menu->gc, item->w - XTextWidth(menu->font, item->rtext, item->rlen) - 2 * MENU_HGAP, item->h - MENU_VGAP, item->rtext, item->rlen);
  }
}

void
menuitem_deselect(menu_t * menu)
{
  menuitem_t *item;

  ASSERT(menu != NULL);

  item = menuitem_get_current(menu);
  REQUIRE(item != NULL);
  D_MENU(("menuitem_deselect():  Deselecting item \"%s\"\n", item->text));
  item->state &= ~(MENU_STATE_IS_CURRENT);
  XUnmapWindow(Xdisplay, menu->swin);
  if (item->type == MENUITEM_SUBMENU) {
    paste_simage(images[image_submenu].norm, image_submenu, menu->win, item->x, item->y, item->w - MENU_VGAP, item->h);
  }
  draw_string(menu->win, menu->gc, 2 * MENU_HGAP, item->y + item->h - MENU_VGAP, item->text, item->len);
  if (item->rtext) {
    draw_string(menu->win, menu->gc, item->x + item->w - XTextWidth(menu->font, item->rtext, item->rlen) - 2 * MENU_HGAP, item->y + item->h - MENU_VGAP,
                item->rtext, item->rlen);
  }
}

void
menu_display_submenu(menu_t * menu, menuitem_t * item)
{

  menu_t *submenu;

  ASSERT(menu != NULL);
  ASSERT(item != NULL);
  REQUIRE(item->action.submenu != NULL);

  submenu = item->action.submenu;
  D_MENU(("menu_display_submenu():  Displaying submenu \"%s\" (window 0x%08x) of menu \"%s\" (window 0x%08x)\n",
	  submenu->title, submenu->win, menu->title, menu->win));
  menu_invoke(item->x + item->w, item->y, menu->win, submenu, CurrentTime);

  /* Invoking the submenu makes it current.  Undo that behavior. */
  ungrab_pointer();
  grab_pointer(menu->win);
  current_menu->state &= ~(MENU_STATE_IS_CURRENT);
  current_menu = menu;
  menu->state |= MENU_STATE_IS_CURRENT;
}

void
menu_draw(menu_t * menu)
{

  register unsigned short i, len;
  unsigned long width, height;
#if 0
  char *safeaction;
#endif
  unsigned short str_x, str_y;
  XGCValues gcvalue;
  int ascent, descent, direction;
  XCharStruct chars;
  Screen *scr;

  ASSERT(menu != NULL);

  scr = ScreenOfDisplay(Xdisplay, Xscreen);
  if (!menu->font) {
    menu_set_font(menu, rs_font[0]);
  }
  gcvalue.foreground = PixColors[menuTextColor];
  XChangeGC(Xdisplay, menu->gc, GCForeground, &gcvalue);

  if (!menu->w) {
    unsigned short longest;

    len = strlen(menu->title);
    longest = XTextWidth(menu->font, menu->title, len);
    height = menu->fheight + 3 * MENU_VGAP;
    for (i = 0; i < menu->numitems; i++) {
      unsigned short j = menu->items[i]->len;
      menuitem_t *item = menu->items[i];

      width = XTextWidth(menu->font, item->text, j);
      if (item->rtext) {
        width += XTextWidth(menu->font, item->rtext, item->rlen) + (2 * MENU_HGAP);
      }
      longest = (longest > width) ? longest : width;
      height += ((item->type == MENUITEM_SEP) ? (MENU_VGAP) : (menu->fheight)) + MENU_VGAP;
    }
    width = longest + (4 * MENU_HGAP);
    if (images[image_submenu].selected->iml->pad) {
      width += images[image_submenu].selected->iml->pad->left + images[image_submenu].selected->iml->pad->right;
    }
    menu->w = width;
    menu->h = height;

    /* Size and render menu window */
    D_MENU((" -> width %hu, height %hu\n", menu->w, menu->h));
    XResizeWindow(Xdisplay, menu->win, menu->w, menu->h);
    render_simage(images[image_menu].norm, menu->win, menu->w, menu->h, image_menu, 0);
    if (image_mode_is(image_menu, MODE_AUTO)) {
      enl_ipc_sync();
    }

    /* Size and render selected item window */
    XResizeWindow(Xdisplay, menu->swin, menu->w - 2 * MENU_HGAP, menu->fheight + MENU_VGAP);
    render_simage(images[image_menu].selected, menu->swin, menu->w - 2 * MENU_HGAP, menu->fheight + MENU_VGAP, image_menu, 0);
    if (image_mode_is(image_menu, MODE_AUTO)) {
      enl_ipc_sync();
    }
  }
  if (menu->w + menu->x > scr->width) {
    menu->x = scr->width - menu->w;
  }
  if (menu->h + menu->y > scr->height) {
    menu->y = scr->height - menu->h;
  }
  XMoveWindow(Xdisplay, menu->win, menu->x, menu->y);
  XRaiseWindow(Xdisplay, menu->win);

  str_x = 2 * MENU_HGAP;
  if (images[image_menu].selected->iml->pad) {
    str_x += images[image_menu].selected->iml->pad->left;
  }
  str_y = menu->fheight + MENU_VGAP;
  len = strlen(menu->title);
  XTextExtents(menu->font, menu->title, len, &direction, &ascent, &descent, &chars);
  draw_string(menu->win, menu->gc, center_coords(2 * MENU_HGAP, menu->w - 2 * MENU_HGAP) - (chars.width >> 1),
	      str_y - chars.descent - MENU_VGAP / 2, menu->title, len);
  Draw_Shadow(menu->win, topShadowGC, botShadowGC, str_x, str_y - chars.descent - MENU_VGAP / 2 + 1, menu->w - (4 * MENU_HGAP), MENU_VGAP);
  str_y += MENU_VGAP;

  for (i = 0; i < menu->numitems; i++) {
    menuitem_t *item = menu->items[i];

    if (item->type == MENUITEM_SEP) {

      str_y += 2 * MENU_VGAP;
      if (!item->x) {
	item->x = MENU_HGAP;
	item->y = str_y - 2 * MENU_VGAP;
	item->w = menu->w - MENU_HGAP;
	item->h = 2 * MENU_VGAP;
	D_MENU(("   -> Hot Area at %hu, %hu to %hu, %hu (width %hu, height %hu)\n", item->x, item->y, item->x + item->w, item->y + item->h,
		item->w, item->h));
      }
      Draw_Shadow(menu->win, botShadowGC, topShadowGC, str_x, str_y - MENU_VGAP - MENU_VGAP / 2, menu->w - 4 * MENU_HGAP, MENU_VGAP);

    } else {
      str_y += menu->fheight + MENU_VGAP;
      if (!item->x) {
	item->x = MENU_HGAP;
	item->y = str_y - menu->fheight - MENU_VGAP / 2;
	item->w = menu->w - MENU_HGAP;
	item->h = menu->fheight + MENU_VGAP;
	D_MENU(("   -> Hot Area at %hu, %hu to %hu, %hu (width %hu, height %hu)\n", item->x, item->y, item->x + item->w, item->y + item->h,
		item->w, item->h));
      }
      switch (item->type) {
	case MENUITEM_SUBMENU:
          paste_simage(images[image_submenu].norm, image_submenu, menu->win, item->x, item->y, item->w - MENU_VGAP, item->h);
	  break;
	case MENUITEM_STRING:
#if 0
	  safeaction = StrDup(item->action.string);
	  SafeStr(safeaction, strlen(safeaction));
	  D_MENU(("  Item %hu:  %s (string %s)\n", i, item->text, safeaction));
	  FREE(safeaction);
#endif
	  break;
	case MENUITEM_ECHO:
#if 0
	  safeaction = StrDup(item->action.string);
	  SafeStr(safeaction, strlen(safeaction));
	  D_MENU(("  Item %hu:  %s (echo %s)\n", i, item->text, safeaction));
	  FREE(safeaction);
#endif
	  break;
	default:
	  fatal_error("Internal Program Error:  Unknown menuitem type:  %u\n", item->type);
	  break;
      }
      draw_string(menu->win, menu->gc, str_x, str_y - MENU_VGAP / 2, item->text, item->len);
      if (item->rtext) {
        draw_string(menu->win, menu->gc, str_x + item->w - XTextWidth(menu->font, item->rtext, item->rlen) - 3 * MENU_HGAP, str_y - MENU_VGAP / 2,
                    item->rtext, item->rlen);
      }
    }
  }
}

void
menu_display(int x, int y, menu_t * menu)
{

  ASSERT(menu != NULL);

  menu->state |= (MENU_STATE_IS_CURRENT);
  current_menu = menu;

  /* Move, render, and map menu window */
  menu->x = x;
  menu->y = y;
  D_MENU(("Displaying menu \"%s\" (window 0x%08x) at root coordinates %d, %d\n", menu->title, menu->win, menu->x, menu->y));
  XMoveWindow(Xdisplay, menu->win, menu->x, menu->y);
  XUnmapWindow(Xdisplay, menu->swin);
  XMapWindow(Xdisplay, menu->win);
  menu->state |= (MENU_STATE_IS_MAPPED);

  menu_draw(menu);

  /* Take control of the pointer so we get all events for it, even those outside the menu window */
  grab_pointer(menu->win);
}

void
menu_action(menuitem_t * item)
{

  ASSERT(item != NULL);

  D_MENU(("menu_action() called to invoke %s\n", item->text));
  switch (item->type) {
    case MENUITEM_SEP:
      D_MENU(("Internal Program Error:  menu_action() called for a separator.\n"));
      break;
    case MENUITEM_SUBMENU:
      D_MENU(("Internal Program Error:  menu_action() called for a submenu.\n"));
      break;
    case MENUITEM_STRING:
      cmd_write((unsigned char *) item->action.string, strlen(item->action.string));
      break;
    case MENUITEM_ECHO:
      tt_write((unsigned char *) item->action.string, strlen(item->action.string));
      break;
    default:
      fatal_error("Internal Program Error:  Unknown menuitem type:  %u\n", item->type);
      break;
  }
}

void
menu_invoke(int x, int y, Window win, menu_t * menu, Time timestamp)
{

  int root_x, root_y;
  Window unused;

  REQUIRE(menu != NULL);

  if (timestamp != CurrentTime) {
    button_press_time = timestamp;
  }
  if (win != Xroot) {
    XTranslateCoordinates(Xdisplay, win, Xroot, x, y, &root_x, &root_y, &unused);
  }
  menu_display(root_x, root_y, menu);

}

void
menu_invoke_by_title(int x, int y, Window win, char *title, Time timestamp)
{

  menu_t *menu;

  REQUIRE(title != NULL);
  REQUIRE(menu_list != NULL);

  menu = find_menu_by_title(menu_list, title);
  if (!menu) {
    D_MENU(("Menu \"%s\" not found!\n", title));
    return;
  }
  menu_invoke(x, y, win, menu, timestamp);
}
