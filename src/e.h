/* e.h -- Eterm Enlightenment support module header file
 *     -- 6 June 1999, mej
 *
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

#ifndef _E_H_
#define _E_H_
/* includes */
#include <X11/Xfuncproto.h>
#include <X11/Intrinsic.h>	/* Xlib, Xutil, Xresource, Xfuncproto */
#include "pixmap.h"  /* For simage_t */

/************ Macros and Definitions ************/
#define IPC_TIMEOUT    ((char *) 1)

/************ Variables ************/
extern Window ipc_win;
extern Atom ipc_atom;

/************ Function Prototypes ************/
_XFUNCPROTOBEGIN

extern unsigned char check_for_enlightenment(void);
extern Window enl_ipc_get_win(void);
extern void enl_ipc_send(char *);
extern char *enl_wait_for_reply(void);
extern char *enl_ipc_get(const char *);
extern void enl_query_for_image(unsigned char);
extern void eterm_ipc_parse(char *);
extern void eterm_ipc_send(char *);
extern char *eterm_ipc_get(void);
extern void eterm_handle_winop(char *);

_XFUNCPROTOEND

#endif	/* _E_H_ */
