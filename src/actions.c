/* actions.c -- Eterm action class module
 *           -- 3 August 1999, mej
 *
 * This file is original work by Michael Jennings <mej@eterm.org> and
 * Tuomo Venalainen <vendu@cc.hut.fi>.  This file, and any other file
 * bearing this same message or a similar one, is distributed under
 * the GNU Public License (GPL) as outlined in the COPYING file.
 *
 * Copyright (C) 1999-1997, Michael Jennings and Tuomo Venalainen
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

  ASSERT(ev != NULL);
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
      if (LOGICAL_XOR((action->mod & MOD_MOD1), (ev->xkey.state & Mod1Mask))) {
        continue;
      }
      if (LOGICAL_XOR((action->mod & MOD_MOD2), (ev->xkey.state & Mod2Mask))) {
        continue;
      }
      if (LOGICAL_XOR((action->mod & MOD_MOD3), (ev->xkey.state & Mod3Mask))) {
        continue;
      }
      if (LOGICAL_XOR((action->mod & MOD_MOD4), (ev->xkey.state & Mod4Mask))) {
        continue;
      }
      if (LOGICAL_XOR((action->mod & MOD_MOD5), (ev->xkey.state & Mod5Mask))) {
        continue;
      }
    }      
    D_ACTIONS(("Modifiers passed.  keysym == 0x%08x, action->keysym == 0x%08x\n", keysym, action->keysym));
    if ((keysym) && (action->keysym) && (keysym != action->keysym)) {
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
  case ACTION_STRING: action->handler = (action_handler_t) action_handle_string; action->param.string = StrDup((char *) param); break;
  case ACTION_ECHO: action->handler = (action_handler_t) action_handle_echo; action->param.string = StrDup((char *) param); break;
  case ACTION_MENU: action->handler = (action_handler_t) action_handle_menu; action->param.menu = (menu_t *) param; break;
  default: break;
  }
  action->next = NULL;
  D_ACTIONS(("Added action.  mod == 0x%08x, button == %d, keysym == 0x%08x\n", action->mod, action->button, action->keysym));
}

