/*--------------------------------*-C-*---------------------------------*
 * File:	activeeterm.c
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

/* This file contains the glue functions to make active tags work in eterm. */

/*
   In order to dingify a program, this plugin file must define the following
   functions with the specified behavior.  Note that functions can be defined
   as macros just fine.

   The functions are broken up into two groups.  The first group of functions
   are the wrapper functions which get called by the main program.  These
   functions call the internal active tags functions.  The second group of
   functions are the internal functions which are called by the active
   tags functions.

   WRAPPER FUNCTIONS

   int tag_click( ... )
   This should call tag_activate after it has properly assembled the
   binding mask and determined the row and column properly.  tag_click()
   is called from inside the main program.

   void tag_pointer_new_position( ... )
   This should call show_tag after it has computed the row and column
   properly and done whatever processing it needs to do.

   void tag_scroll( ... )
   void tag_init ( ... )
   void tag_sig_child ( ... )
   void tag_hide ( ... )

   INTERNAL FUNCTIONS

   void get_tag_mode(char * mode)
   This function stores a string containing the current mode in the
   mode parameter.  This can be very simple (for example
   strcpy(mode, "browser") might be sufficient), or somewhat more complex
   for those programs which can change modes, such as rxvt.

   int row_width(void)
   returns the maximum width of each row.

   const char ** get_rows(void)
   returns the comparison region.  current_row() and current_col() should
   be indexes into this region.

   void set_tag_highlight(int row, int col, tag_highlight_t highlight)
   Highlights the specified character with the specified highlighting.

   tag_highlight_t get_tag_highlight(int row, int col)
   Returns the current highlighting information for the specified
   character.
 */

static const char cvs_ident[] = "$Id$";

#include "feature.h"
#ifdef USE_ACTIVE_TAGS

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "activetags.h"
#include "activeeterm.h"
#include "mem.h"		/* From libmej */

/*
   WRAPPER FUNCTIONS 
 */

void
tag_pointer_new_position(int x, int y)
{
  int row, col;

  if (!active_tags_enabled)
    return;

  col = Pixel2Col(x);
  row = Pixel2Row(y);

#ifdef ACTIVE_TAG_CLICK_CLUES
  if (show_tag(row, col))
    set_click_clue_timer();
  else {
    destroy_click_clue();
    unset_click_clue_timer();
  }
#else
  show_tag(row, col);
#endif
}

void
tag_scroll(int nlines, int row1, int row2)
{
  if (!active_tags_enabled)
    return;

#ifdef ACTIVE_TAG_CLICK_CLUES
  if (tag_screen_scroll(nlines, row1, row2)) {
    destroy_click_clue();
    unset_click_clue_timer();
  }
#else
  (void) tag_screen_scroll(nlines, row1, row2);
#endif
}

void
tag_init(void)
{
  if (!active_tags_enabled)
    return;

  tag_env = "X";

#ifdef ACTIVE_TAG_CLICK_CLUES
  init_click_clues();
#endif

  initialize_tags();
}

#if 0
void
#endif
inline void
tag_hide(void)
{
  if (!active_tags_enabled)
    return;

  erase_tag_highlighting();
}

int
tag_click(int x, int y, unsigned int button, unsigned int keystate)
{
  int binding_mask;
  int row, col;
  int retval;

  if (!active_tags_enabled)
    return 0;

#ifdef ACTIVE_TAG_CLICK_CLUES
  destroy_click_clue();
  unset_click_clue_timer();
#endif

  /* Build the binding mask.  Button3 == 3.  We need it to be 4
     (100 binary) to fit in the binding mask properly. */
  if (button == Button3)
    button = TAG_BINDING_BUTTON3;

  binding_mask = button;

  if (keystate & ShiftMask)
    binding_mask |= TAG_BINDING_SHIFT;
  if (keystate & ControlMask)
    binding_mask |= TAG_BINDING_CONTROL;
  if (keystate & Mod1Mask)
    binding_mask |= TAG_BINDING_META;

  row = Pixel2Row(y);
  col = Pixel2Col(x);
  retval = tag_activate(row, col, binding_mask);
  return retval;
}

/*
   INTERNAL FUNCTIONS 
 */

/* This function finds the current tag mode and stores it in the 'mode'
   parameter.  The current mode is equivalent to argv[0] of the program
   currently controlling the eterm's terminal. */
void
get_tag_mode(char *mode)
{
  char proc_name[1024];
  FILE *f;
  pid_t pid;

  if ((pid = tcgetpgrp(cmd_fd)) == -1) {
    fprintf(stderr, "Couldn't get tag mode!\n");
    strcpy(mode, "");
    return;
  }
  sprintf(proc_name, "/proc/%d/cmdline", pid);
  if ((f = fopen(proc_name, "r")) == NULL) {
    fprintf(stderr, "Couldn't open proc!\n");
    strcpy(mode, "");
    return;
  }
  fscanf(f, "%s", mode);
  fclose(f);
}

/* These were changed to macros and moved into activeeterm.h. -vendu */

#if 0
int
row_width(void)
{
  return TermWin.ncol;
}

int
tag_min_row(void)
{
  return 0;
}

int
tag_max_row(void)
{
  return TermWin.nrow - 1;
}
#endif

void
tag_get_row(int row_num, char **row)
{
/* FIXME: I guess this works :) -vendu */
  *row = drawn_text[row_num];
}

int
tag_eterm_color(int tag_color)
{
  switch (tag_color) {
    case TAG_HIGHLIGHT_BLACK:
      return 2;
    case TAG_HIGHLIGHT_WHITE:
      return 1;
    case TAG_HIGHLIGHT_RED:
      return 3;
    case TAG_HIGHLIGHT_GREEN:
      return 4;
    case TAG_HIGHLIGHT_YELLOW:
      return 5;
    case TAG_HIGHLIGHT_BLUE:
      return 6;
    case TAG_HIGHLIGHT_MAGENTA:
      return 7;
    case TAG_HIGHLIGHT_CYAN:
      return 8;
    default:
      return -1;
  }
}

int
eterm_tag_color(int eterm_color)
{
  switch (eterm_color) {
    case 0:
      return TAG_HIGHLIGHT_NORMAL;
    case 7:
      return TAG_HIGHLIGHT_MAGENTA;
    case 1:
      return TAG_HIGHLIGHT_WHITE;
    case 2:
      return TAG_HIGHLIGHT_BLACK;
    case 3:
      return TAG_HIGHLIGHT_RED;
    case 4:
      return TAG_HIGHLIGHT_GREEN;
    case 5:
      return TAG_HIGHLIGHT_YELLOW;
    case 6:
      return TAG_HIGHLIGHT_BLUE;
    case 8:
      return TAG_HIGHLIGHT_CYAN;
    default:
      return TAG_HIGHLIGHT_NORMAL;
  }
}

void
set_tag_highlight(int row, int col, tag_highlight_t highlight)
{
  unsigned int rend_mask = 0;
  unsigned int back;
  unsigned int fore;

/*  rend_t ** rp = &(screen.rend[row + TermWin.saveLines - TermWin.view_start][col]); */

  if (highlight.attributes & TAG_HIGHLIGHT_RVID)
    rend_mask |= RS_RVid;
  if (highlight.attributes & TAG_HIGHLIGHT_ULINE)
    rend_mask |= RS_Uline;
  if (highlight.attributes & TAG_HIGHLIGHT_BOLD)
    rend_mask |= RS_Bold;

  if (highlight.fg_color == TAG_HIGHLIGHT_NORMAL)
    fore = SET_FGCOLOR(0, fgColor);
  else
    fore = SET_FGCOLOR(0, tag_eterm_color(highlight.fg_color));

  if (highlight.bg_color == TAG_HIGHLIGHT_NORMAL)
    back = SET_BGCOLOR(0, bgColor);
  else
    back = SET_BGCOLOR(0, tag_eterm_color(highlight.bg_color));

  screen.rend[row + TermWin.saveLines - TermWin.view_start][col] =
      rend_mask | fore | back;
}

void
get_tag_highlight(int row, int col, tag_highlight_t * highlight)
{
  unsigned int rend;

  rend = screen.rend[row + TermWin.saveLines - TermWin.view_start][col];

  highlight->attributes = 0;
  if (rend & RS_RVid)
    highlight->attributes |= TAG_HIGHLIGHT_RVID;
  if (rend & RS_Uline)
    highlight->attributes |= TAG_HIGHLIGHT_ULINE;
  if (rend & RS_Bold)
    highlight->attributes |= TAG_HIGHLIGHT_BOLD;
  if (rend & RS_Blink)
    highlight->attributes |= TAG_HIGHLIGHT_BLINK;

  highlight->fg_color = eterm_tag_color(GET_FGCOLOR(rend));
  highlight->bg_color = eterm_tag_color(GET_BGCOLOR(rend));
}

/* Set the UID appropriately */
int
set_tag_uid(void)
{
  return 1;
}

/* Set stdout for loop actions */
int
set_tag_stdout(void)
{
  if (close(1) < 0) {
    perror("close");
    return 0;
  }
  if (dup(cmd_fd) < 0) {
    perror("dup");
    return 0;
  }
  return 1;
}

/* Set the PWD to the pwd of the process eterm is running */
int
set_tag_pwd(void)
{
  char dir[1024];

  sprintf(dir, "/proc/%d/cwd", cmd_pid);
  if (chdir(dir) < 0)
    return 0;

  return 1;
}

#endif /* USE_ACTIVE_TAGS */
