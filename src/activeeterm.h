/*--------------------------------*-C-*---------------------------------*
 * File:	activeeterm.h
 *
 * Copyright 1997 Nat Friedman, Massachusetts Institute of Technology
 * <ndf@mit.edu>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 *----------------------------------------------------------------------*/

#ifndef _ACTIVEETERM_H
# define ACTIVEETERM_H

# include "screen.h"
# include "command.h"
# include "main.h" /* for TermWin */
/* #include "rxvtgrx.h" */

/* #define MAX_RXVT_ROWS 1024 */

int tag_click(int x, int y, unsigned int button, unsigned int keystate);
void tag_pointer_new_position(int x, int y);
void tag_init(void);

extern int cmd_fd;
extern screen_t screen;

extern text_t **drawn_text;
extern rend_t **drawn_rend;

# define row_width() (TermWin.ncol)
# define tag_min_row() (0)
# define tag_max_row() (TermWin.nrow - 1)

#endif /*  ACTIVEETERM_H */
