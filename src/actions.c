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

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include "../libmej/debug.h"
#include "../libmej/strings.h"
#include "debug.h"
#include "startup.h"
#include "mem.h"
#include "strings.h"
#include "actions.h"
#include "command.h"
#include "e.h"
#include "events.h"
#include "menus.h"
#include "options.h"
#include "pixmap.h"
#include "screen.h"
#include "scrollbar.h"
#include "term.h"
#include "windows.h"

action_t *action_list = NULL;

unsigned char
action_handle_string(event_t *ev, action_t *action) {

  REQUIRE_RVAL(action->param.string != NULL, 0);
  cmd_write((unsigned char *) action->param.string, strlen(action->param.string));
  return 1;
  ev = NULL;
}

unsigned char
action_handle_echo(event_t *ev, action_t *action) {

  REQUIRE_RVAL(action->param.string != NULL, 0);
  tt_write((unsigned char *) action->param.string, strlen(action->param.string));
  return 1;
  ev = NULL;
}

unsigned char
action_handle_menu(event_t *ev, action_t *action) {
  REQUIRE_RVAL(action->param.menu != NULL, 0);
  menu_invoke(ev->xbutton.x, ev->xbutton.y, TermWin.parent, action->param.menu, ev->xbutton.time);
  return 1;
}

unsigned char
action_dispatch(event_t *ev, KeySym keysym) {

  action_t *action;
  unsigned int m = (AltMask | MetaMask | NumLockMask);

  ASSERT(ev != NULL);
  D_ACTIONS(("Event %8p:  Button %d, Keysym 0x%08x, Key State 0x%08x\n", ev, ev->xbutton.button, keysym, ev->xkey.state));
  for (action = action_list; action; action = action->next) {
    D_ACTIONS(("Checking action.  mod == 0x%08x, button == %d, keysym == 0x%08x\n", action->mod, action->button, action->keysym));
    if (ev->xany.type == ButtonPress) {
      if ((action->button == BUTTON_NONE) || ((action->button != BUTTON_ANY) && (action->button != ev->xbutton.button))) {
        continue;
      }
    } else if (action->button != BUTTON_NONE) {
      continue;
    }
    D_ACTIONS(("Button passed.\n"));
    if (action->mod != MOD_ANY) {
      if (LOGICAL_XOR((action->mod & MOD_SHIFT), (ev->xkey.state & ShiftMask))) {
        continue;
      }
      if (LOGICAL_XOR((action->mod & MOD_CTRL), (ev->xkey.state & ControlMask))) {
        continue;
      }
      if (LOGICAL_XOR((action->mod & MOD_LOCK), (ev->xkey.state & LockMask))) {
        continue;
      }
      if (LOGICAL_XOR((action->mod & MOD_META), (ev->xkey.state & MetaMask))) {
        continue;
      }
      if (LOGICAL_XOR((action->mod & MOD_ALT), (ev->xkey.state & AltMask))) {
        continue;
      }
      if (((action->mod & MOD_MOD1) && !(ev->xkey.state & Mod1Mask)) || (!(action->mod & MOD_MOD1) && (ev->xkey.state & Mod1Mask) && !(Mod1Mask & m))) {
        continue;                                                                                                                                 
      }                                                                                                                                           
      if (((action->mod & MOD_MOD2) && !(ev->xkey.state & Mod2Mask)) || (!(action->mod & MOD_MOD2) && (ev->xkey.state & Mod2Mask) && !(Mod2Mask & m))) {
        continue;                                                                                                                                 
      }                                                                                                                                           
      if (((action->mod & MOD_MOD3) && !(ev->xkey.state & Mod3Mask)) || (!(action->mod & MOD_MOD3) && (ev->xkey.state & Mod3Mask) && !(Mod3Mask & m))) {
        continue;                                                                                                                                 
      }                                                                                                                                           
      if (((action->mod & MOD_MOD4) && !(ev->xkey.state & Mod4Mask)) || (!(action->mod & MOD_MOD4) && (ev->xkey.state & Mod4Mask) && !(Mod4Mask & m))) {
        continue;                                                                                                                                 
      }                                                                                                                                           
      if (((action->mod & MOD_MOD5) && !(ev->xkey.state & Mod5Mask)) || (!(action->mod & MOD_MOD5) && (ev->xkey.state & Mod5Mask) && !(Mod5Mask & m))) {
        continue;
      }
    }      
    D_ACTIONS(("Modifiers passed.  keysym == 0x%08x, action->keysym == 0x%08x\n", keysym, action->keysym));
    if ((ev->xany.type == KeyPress) && (action->keysym) && (keysym != action->keysym)) {
      continue;
    }
    D_ACTIONS(("Match found.\n"));
    /* If we've passed all the above tests, it's a match.  Dispatch the handler. */
    return ((action->handler)(ev, action));
  }
  return (0);

}

void
action_add(unsigned short mod, unsigned char button, KeySym keysym, action_type_t type, void *param) {

  static action_t *action;

  if (!action_list) {
    action_list = (action_t *) MALLOC(sizeof(action_t));
    action = action_list;
  } else {
    action->next = (action_t *) MALLOC(sizeof(action_t));
    action = action->next;
  }
  action->mod = mod;
  action->button = button;
  action->type = type;
  action->keysym = keysym;
  switch(type) {
    case ACTION_STRING:
      action->handler = (action_handler_t) action_handle_string;
      action->param.string = (char *) MALLOC(strlen((char *) param) + 2);
      strcpy(action->param.string, (char *) param);
      parse_escaped_string(action->param.string);
      break;
    case ACTION_ECHO:
      action->handler = (action_handler_t) action_handle_echo;
      action->param.string = (char *) MALLOC(strlen((char *) param) + 2);
      strcpy(action->param.string, (char *) param);
      parse_escaped_string(action->param.string);
      break;
    case ACTION_MENU:
      action->handler = (action_handler_t) action_handle_menu;
      action->param.menu = (menu_t *) param;
      break;
    default:
      break;
  }
  action->next = NULL;
  D_ACTIONS(("Added action.  mod == 0x%08x, button == %d, keysym == 0x%08x\n", action->mod, action->button, action->keysym));
}

