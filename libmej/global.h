/***************************************************************
 * GLOBAL.H -- Compile-time options header file                *
 *          -- Michael Jennings                                *
 *          -- 16 January 1997                                 *
 ***************************************************************/
/*
 * This file is original work by Michael Jennings <mej@eterm.org>.
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

#ifndef _GLOBAL_H_

#define _GLOBAL_H_

/* Other compile-time defines */
#ifdef LINUX
#  define	_GNU_SOURCE
#  define	__USE_GNU
#  define	_BSD_SOURCE
#elif defined(IRIX)
#  define	_MODERN_C_
#  define	_BSD_TYPES
#  define	_SGI_SOURCE
#elif defined(HP_UX)

#endif

#endif /* _GLOBAL_H_ */
