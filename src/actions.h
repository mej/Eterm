/* actions.h -- Eterm action class module header file
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

#ifndef _ACTIONS_H_
#define _ACTIONS_H_

#include <X11/Xfuncproto.h>
#include <X11/Intrinsic.h>	/* Xlib, Xutil, Xresource, Xfuncproto */

#include "events.h"
#include "menus.h"

/************ Macros and Definitions ************/
typedef enum {
  ACTION_NONE = 0,
  ACTION_STRING,
  ACTION_ECHO,
  ACTION_MENU
} action_type_t;

#define KEYSYM_NONE    (0UL)

#define MOD_NONE       (0UL)
#define MOD_CTRL       (1UL << 0)
#define MOD_SHIFT      (1UL << 1)
#define MOD_LOCK       (1UL << 2)
#define MOD_MOD1       (1UL << 3)
#define MOD_MOD2       (1UL << 4)
#define MOD_MOD3       (1UL << 5)
#define MOD_MOD4       (1UL << 6)
#define MOD_MOD5       (1UL << 7)
#define MOD_ANY        (1UL << 8)

#define BUTTON_NONE    (0)
#define BUTTON_ANY     (0xff)

#define LOGICAL_XOR(a, b)  !(((a) && (b)) || (!(a) && !(b)))

/************ Structures ************/
typedef struct action_struct action_t;
typedef unsigned char (*action_handler_t) (event_t *, action_t *);
struct action_struct {
  unsigned short mod;
  unsigned char button; 
  KeySym keysym;
  action_type_t type;
  action_handler_t handler;
  union {
    char *string;
    menu_t *menu;
  } param;
  struct action_struct *next;
};

/************ Variables ************/
extern action_t *action_list;

/************ Function Prototypes ************/
_XFUNCPROTOBEGIN

extern unsigned char action_handle_string(event_t *ev, action_t *action);
extern unsigned char action_handle_echo(event_t *ev, action_t *action);
extern unsigned char action_handle_menu(event_t *ev, action_t *action);
extern unsigned char action_dispatch(event_t *ev, KeySym keysym);
extern void action_add(unsigned short mod, unsigned char button, KeySym keysym, action_type_t type, void *param);

_XFUNCPROTOEND

#endif	/* _ACTIONS_H_ */
