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

#ifndef _TIMER_H_
#define _TIMER_H_

#include <sys/time.h>
#include <unistd.h>
#include <X11/Xfuncproto.h>
#include <X11/Intrinsic.h>	/* Xlib, Xutil, Xresource, Xfuncproto */

/************ Macros and Definitions ************/
#define find_timer_by_handle(handle)  (handle)
#define timer_change_data(handle, data)  ((handle)->data = (data))
#define timer_change_handler(handle, handler)  ((handle)->handler = (handler))

/************ Structures ************/
typedef unsigned char (*timer_handler_t)(void *);
typedef struct timer_struct timer_t;
typedef timer_t *timerhdl_t;  /* The timer handles are actually pointers to a timer_t struct, but clients shouldn't use them as such. */
struct timer_struct {
  unsigned long msec;
  struct timeval time;
  timer_handler_t handler;
  void *data;
  struct timer_struct *next;
};

/************ Variables ************/

/************ Function Prototypes ************/
_XFUNCPROTOBEGIN

extern timerhdl_t timer_add(unsigned long msec, timer_handler_t handler, void *data);
extern unsigned char timer_del(timerhdl_t handle);
extern unsigned char timer_change_delay(timerhdl_t handle, unsigned long msec);
extern void timer_check(void);

_XFUNCPROTOEND

#endif	/* _TIMER_H_ */
