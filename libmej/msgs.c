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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "libmej.h"

int
libmej_dprintf(const char *format, ...)
{
  va_list args;
  int n;

  va_start(args, format);
  n = vfprintf(LIBMEJ_DEBUG_FD, format, args);
  va_end(args);
  fflush(LIBMEJ_DEBUG_FD);
  return (n);
}

/* Print a non-terminal error message */
void
print_error(const char *fmt, ...)
{
  va_list arg_ptr;

  va_start(arg_ptr, fmt);
  fprintf(stderr, PACKAGE ":  Error:  ");
  vfprintf(stderr, fmt, arg_ptr);
  va_end(arg_ptr);
}

/* Print a simple warning */
void
print_warning(const char *fmt, ...)
{
  va_list arg_ptr;

  va_start(arg_ptr, fmt);
  fprintf(stderr, PACKAGE ":  Warning:  ");
  vfprintf(stderr, fmt, arg_ptr);
  va_end(arg_ptr);
}

/* Print a fatal error message and terminate */
void
fatal_error(const char *fmt, ...)
{
  va_list arg_ptr;

  va_start(arg_ptr, fmt);
  fprintf(stderr, PACKAGE ":  FATAL:  ");
  vfprintf(stderr, fmt, arg_ptr);
  va_end(arg_ptr);
  exit(-1);
}

