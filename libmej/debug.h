/**********************************************************
 * DEBUG.H -- Header file for DEBUG.C                     *
 *         -- Michael Jennings                            *
 *         -- 20 December 1996                            *
 **********************************************************/
/*
 * This file is original work by Michael Jennings <mej@tcserv.com>.
 *
 * Copyright (C) 1997, Michael Jennings
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

#if !defined(DEBUG_C) && !defined(DEBUG_CC)
  extern int real_dprintf(const char *, ...);
#endif

#ifndef _LIBMEJ_DEBUG_H
# define _LIBMEJ_DEBUG_H

#include "../src/debug.h"

#endif /* _LIBMEJ_DEBUG_H */
