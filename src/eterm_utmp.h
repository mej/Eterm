/*  eterm_utmp.h -- Eterm utmp module header file
 *
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

#ifndef ETERM_UTMP_H_
#define ETERM_UTMP_H_

#include <X11/Xfuncproto.h>
#include <X11/Intrinsic.h>	/* Xlib, Xutil, Xresource, Xfuncproto */

#ifdef UTMP_SUPPORT
# ifdef HAVE_UTEMPTER
#  include <utempter.h>
#  define add_utmp_entry(p, h, f)  addToUtmp(p, h, f)
#  define remove_utmp_entry()      removeFromUtmp()
# endif

/************ Macros and Definitions ************/
# ifndef UTMP_FILENAME
#   ifdef UTMP_FILE
#     define UTMP_FILENAME UTMP_FILE
#   elif defined(_PATH_UTMP)
#     define UTMP_FILENAME _PATH_UTMP
#   else
#     define UTMP_FILENAME "/etc/utmp"
#   endif
# endif

# ifndef LASTLOG_FILENAME
#  ifdef _PATH_LASTLOG
#   define LASTLOG_FILENAME _PATH_LASTLOG
#  else
#   define LASTLOG_FILENAME "/usr/adm/lastlog"	/* only on BSD systems */
#  endif
# endif

# ifndef WTMP_FILENAME
#   ifdef WTMP_FILE
#     define WTMP_FILENAME WTMP_FILE
#   elif defined(_PATH_WTMP)
#     define WTMP_FILENAME _PATH_WTMP
#   elif defined(SYSV)
#     define WTMP_FILENAME "/etc/wtmp"
#   else
#     define WTMP_FILENAME "/usr/adm/wtmp"
#   endif
# endif

# ifndef TTYTAB_FILENAME
#   ifdef TTYTAB
#     define TTYTAB_FILENAME TTYTAB_FILENAME
#   else
#     define TTYTAB_FILENAME "/etc/ttytab"
#   endif
# endif

# ifndef USER_PROCESS
#   define USER_PROCESS 7
# endif
# ifndef DEAD_PROCESS
#   define DEAD_PROCESS 8
# endif

/************ Function Prototypes ************/
_XFUNCPROTOBEGIN

# ifndef HAVE_UTEMPTER
extern void add_utmp_entry(const char *, const char *, int);
extern void remove_utmp_entry(void);
# endif

_XFUNCPROTOEND

#else /* UTMP_SUPPORT */
# define add_utmp_entry(p, h, f)  NOP
# define remove_utmp_entry()      NOP
#endif

#endif	/* ETERM_UTMP_H_ */
