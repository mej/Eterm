/* timer.c -- Eterm timer module
 *         -- 16 August 1999, mej
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
#include "main.h"
#include "mem.h"
#include "command.h"
#include "events.h"
#include "options.h"
#include "pixmap.h"
#include "timer.h"

static timer_t *timers = NULL;

timerhdl_t
timer_add(unsigned long msec, timer_handler_t handler, void *data) {

  static timer_t *timer;
  struct timeval tv;
  static struct timezone tz;

  if (!timers) {
    timers = (timer_t *) MALLOC(sizeof(timer_t));
    timer = timers;
  } else {
    timer->next = (timer_t *) MALLOC(sizeof(timer_t));
    timer = timer->next;
  }
  timer->msec = msec;
  gettimeofday(&tv, &tz);
  timer->time.tv_sec = (msec / 1000) + tv.tv_sec;
  timer->time.tv_usec = ((msec % 1000) * 1000) + tv.tv_usec;
  timer->handler = handler;
  timer->data = data;
  timer->next = NULL;
  D_TIMER(("Added timer.  Timer set to %lu/%lu with handler 0x%08x and data 0x%08x\n", timer->time.tv_sec, timer->time.tv_usec, timer->handler, timer->data));
  return ((timerhdl_t) timer);
}

unsigned char
timer_del(timerhdl_t handle) {

  register timer_t *current;
  timer_t *temp;

  if (timers == handle) {
    timers = handle->next;
    FREE(handle);
    return 1;
  }
  for (current = timers; current->next; current = current->next) {
    if (current->next == handle) {
      break;
    }
  }
  if (!(current->next)) {
    return 0;
  }
  temp = current->next;
  current->next = temp->next;
  FREE(temp);
  return 1;
}

unsigned char
timer_change_delay(timerhdl_t handle, unsigned long msec) {

  struct timeval tv;
  static struct timezone tz;

  handle->msec = msec;
  gettimeofday(&tv, &tz);
  handle->time.tv_sec = (msec / 1000) + tv.tv_sec;
  handle->time.tv_usec = ((msec % 1000) * 1000) + tv.tv_usec;
  return 1;
}

void
timer_check(void) {

  register timer_t *current;
  struct timeval tv;
  static struct timezone tz;

  if (!timers) return;

  gettimeofday(&tv, &tz);
  for (current = timers; current; current = current->next) {
    if ((current->time.tv_sec > tv.tv_sec) || ((current->time.tv_sec == tv.tv_sec) && (current->time.tv_usec >= tv.tv_usec))) {
      if (!((current->handler)(current->data))) {
        timer_del(current);
      } else {
        timer_change_delay(current, current->msec);
      }
    }
  }
}
