/*
 * Copyright (C) 1997-2009, Michael Jennings
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

#ifndef _ENCODING_H
# define _ENCODING_H

# include <X11/Xfuncproto.h>

/************ Macros and Definitions ************/
#define ENC_US_ASCII         0UL
#define ENC_GREEK_ELOT_928   1UL
#define ENC_GREEK_IBM_437    2UL
#define ENC_CYRILLIC         3UL
#define ENC_USERDEF          255

/************ Structures ************/
typedef unsigned char trans_tbl_t[256];
typedef struct state_switch_struct {
  unsigned char key;
  unsigned char next_state;
  struct state_switch_struct *next;
} state_switch_t;
typedef struct enc_state_struct {
  char *id;
  trans_tbl_t in_tbl, out_tbl;
  state_switch_t *switches;
  unsigned char lifetime:4, life_left:4;
  struct enc_state_struct *next;
} enc_state_t;
typedef struct enc_context_struct {
  char *id;
  enc_state_t *states, *current, *prev;
} enc_context_t;

/************ Variables ************/
extern enc_context_t *encoder;

/************ Function Prototypes ************/
_XFUNCPROTOBEGIN


_XFUNCPROTOEND

#endif	/* _ENCODING_H */
