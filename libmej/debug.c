/*********************************************************
 * DEBUG.C -- Standardized debugging output              *
 *         -- Michael Jennings                           *
 *         -- 20 December 1996                           *
 *********************************************************/
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

static const char cvs_ident[] = "$Id$";

#include "config.h"
#include "../src/feature.h"

#include "global.h"
#include <stdio.h>
#include <stdlib.h>
#ifndef WITH_DMALLOC
# include <malloc.h>
#endif
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "debug.h"

int
real_dprintf(const char *format,...)
{

  va_list args;
  int n;

  va_start(args, format);
  n = vfprintf(stderr, format, args);
  va_end(args);
  fflush(stderr);
  return (n);
}
