/*--------------------------------*-C-*---------------------------------*
 * File:	activetags.c
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

/*
   This file contains all of the basic active tags functionality.  These
   are the generalized routines which can be plugged into just about anything.
   If I've designed everything properly, which I believe I have, you should
   not have to change anything in this file in order to plug active tags into
   an application.

   See activeeterm.c for the routines which interface these functions with Eterm
 */

static const char cvs_ident[] = "$Id$";

#include "feature.h"
#ifdef USE_ACTIVE_TAGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "activetags.h"
#include "activeeterm.h"

/* ============================ Global Variables =========================== */

/* This is for run-time enabling and disabling of tags.  It is 1 if tags are
   enabled at 0 otherwise. */
int active_tags_enabled = 1;

/* This is the global array of tag configurations.  Each of the tags in the
   configuration file corresponds to a tag in this global array.  These are
   allocated statically just to reduce the complexity of the code.  Increase
   MAX_TAGS in activetags.h if you run out.  */
struct active_tag tag[MAX_TAGS];
int num_tags = 0;

/* The tag environment (e.g. "X", "console", ... */
char *tag_env;

/* The data regarding the last tag highlighted on the screen.  NB: this model
   limits the number of highlighted tags on the screen to one. */
int last_h_tag_index = -1, last_h_tag_begin_row = -1, last_h_tag_end_row = -1, last_h_tag_begin_col = -1, last_h_tag_end_col = -1;
tag_highlight_t old_highlighting[MAX_SEARCH_LINES][MAX_SEARCH_COLS];

char *tag_config_file = NULL;

static char *thingy = "ActiveTags 1.0b4 -- Copyright 1997,1998 Nat Friedman <ndf@mit.edu> -- DINGUS UBER ALLES";

/* ============================== Tag Routines ============================= */

/* This is the tag intialization routine.  It needs to be called upon
   startup. */
void
initialize_tags(void)
{
  /* Parse the config file */
  parse_tag_config(tag_config_file);
}

void
disable_tags(void)
{
  fprintf(stderr, "Active tags are disabled.\n");
  active_tags_enabled = 0;
}

/* check_tag_mode returns true if the mode parameter is one of the modes
   for which the tag indexed by tag_index is active.  Otherwise, it returns
   false. */
int
check_tag_mode(int tag_index, char *mode)
{
  int i;

  /* If no modes are listed for a particular tag, that tag is always active. */
  if (tag[tag_index].num_modes == 0)
    return 1;

  for (i = 0; i < tag[tag_index].num_modes; i++)
    if (!strcmp(mode, tag[tag_index].mode[i]))
      return 1;

  return 0;
}

int
check_tag_env(int tag_index)
{
  int i;

  if (!tag[tag_index].num_envs)
    return 1;

  for (i = 0; i < tag[tag_index].num_envs; i++)
    if (!strcasecmp(tag_env, tag[tag_index].env[i]))
      return 1;

  return 0;
}

/* Check the position specified by (row,col) for a tag.  If there is
   a tag there, set tag_begin and tag_end to the proper offsets into
   screen.text and screen.rend, and set tag_index to the index of the
   tag that was identified.  If no tag is found, return 0.  Otherwise,
   return 1.  If binding_mask is set, then only search tags whose
   binding mask matches the binding_mask passed to the function.
   Tag searching begins at the specified index, tag_begin_index. */
int
find_tag(int row, int col, int *tag_begin_row, int *tag_begin_col,
	 int *tag_end_row, int *tag_end_col, int *tag_index,
	 unsigned int binding_mask, int tag_begin_index)
{
  char *curr_row;
  static char mode[1024];
  static int mode_check_count = 0;
  char compare_region[MAX_SEARCH_CHARS];
  int compare_offset, compare_region_pointer_position;
  int done;
  unsigned int region_size;
  unsigned int last_region_size = 0;

  int start_row, end_row, i, dest_offset;

  char *start_match_p, *end_match_p;

#ifndef ACTIVE_TAGS_SPENCER_REGEXP
  regmatch_t regmatch[5];

#endif

  D_TAGS(("==> find_tag(row=%d, col=%d, ..., binding=%d, begin=%d)\n", row, col, binding_mask, tag_begin_index));
  if (!mode_check_count)
    get_tag_mode(mode);

  mode_check_count++;
  if (mode_check_count == MAX_MODE_CHECK_COUNT)
    mode_check_count = 0;
  for (*tag_index = tag_begin_index; *tag_index < num_tags; (*tag_index)++) {
    D_TAGS(("  ==> tag: %d (sl=%d)\n", *tag_index, tag[*tag_index].search_lines));
    if (((binding_mask == 0) && (!tag[*tag_index].latent)) ||
	(binding_mask && (binding_mask == tag[*tag_index].binding_mask)))
      if (check_tag_mode(*tag_index, mode) && (check_tag_env(*tag_index))) {
	start_row = row - tag[*tag_index].search_lines / 2;
	end_row = row + tag[*tag_index].search_lines / 2;

	if (start_row < tag_min_row())
	  start_row = tag_min_row();

	if (end_row > tag_max_row())
	  end_row = tag_max_row();

	compare_region_pointer_position = ((row - start_row) * row_width()) +
	    col;

	region_size = (row_width()) * (end_row - start_row + 1);

	if (region_size > MAX_SEARCH_CHARS) {
	  fprintf(stderr, "search region too large: reduce number of "
		  "search lines.\n");
	  fprintf(stderr, "row_width: %d end_row: %d start_row: %d\n",
		  row_width(), end_row, start_row);
	  break;
	}
	if (region_size != last_region_size) {
	  D_TAGS(("  ==> region_size == %d\tlast_region_size == %d\n", region_size, last_region_size));
	  i = start_row;
	  dest_offset = 0;
	  while (i <= end_row) {
	    tag_get_row(i, &curr_row);
	    D_TAGS(("Memcpying row into place...\n"));
	    memcpy(compare_region + dest_offset, curr_row, row_width());
	    D_TAGS(("Done\n"));
	    dest_offset += row_width();
	    i++;
	  }
	  compare_region[dest_offset + 1] = '\0';
	}
	last_region_size = region_size;


	done = 0;
	compare_offset = 0;

	while (!done) {
#ifdef ACTIVE_TAGS_SPENCER_REGEXP
	  if (regexec(tag[*tag_index].rx, compare_region + compare_offset))
#else
	  if (!regexec(&tag[*tag_index].rx, compare_region + compare_offset,
		       4, regmatch, REG_NOTBOL | REG_NOTEOL))
#endif

	  {
#ifdef ACTIVE_TAGS_SPENCER_REGEXP
	    start_match_p = tag[*tag_index].rx->startp[0];
	    end_match_p = tag[*tag_index].rx->endp[0];
#else
	    start_match_p = compare_region + compare_offset +
		regmatch[0].rm_so;
	    end_match_p = compare_region + compare_offset +
		regmatch[0].rm_eo;
#endif

	    if ((start_match_p <=
		 (compare_region + compare_region_pointer_position)) &&
		(end_match_p >
		 (compare_region + compare_region_pointer_position))) {
	      *tag_begin_row = ((start_match_p -
				 compare_region) / row_width()) +
		  start_row;
	      *tag_begin_col = (start_match_p -
				compare_region) -
		  ((start_match_p - compare_region) /
		   row_width()) * row_width();

	      *tag_end_row = ((end_match_p -
			       compare_region) / row_width()) +
		  start_row;
	      *tag_end_col = (end_match_p -
			      compare_region) -
		  ((end_match_p - compare_region) /
		   row_width()) * row_width();
	      D_TAGS(("Found tag: begin_row: %d begin_col: %d\nend_row  : %d end_col  : %d\n", *tag_begin_row, *tag_begin_col,
		      *tag_end_row, *tag_end_col));
	      return 1;

	    } else
	      compare_offset = (end_match_p -
				compare_region);
	  } else
	    done = 1;
	}
      }
  }

  return 0;
}

/* tag_scroll -- This is to notify the tag functionality that the screen
   has scrolled nlines lines (positive means scrolled up, negative means
   scrolled down) from start_row to end_row (inclusive) */
int
tag_screen_scroll(int nlines, int start_row, int end_row)
{

  D_TAGS(("tag_scroll(%d, %d, %d)\n", nlines, start_row, end_row));
  D_TAGS(("\tlast_brow: %d last_erow: %d\n", last_h_tag_begin_row, last_h_tag_end_row));
  if (!nlines)
    return 0;

  if (last_h_tag_index == -1)
    return 0;

  /* If the last highlighted tag is not part of the region that scrolled,
     we don't need to do anything. */
  if (last_h_tag_begin_row > end_row)
    return 0;

  if (last_h_tag_end_row < start_row)
    return 0;

  /* Otherwise, update the position of the tag last highlighted */
  last_h_tag_begin_row += nlines;
  last_h_tag_end_row += nlines;

  /* Erase the tag */
  (void) show_tag(last_h_tag_begin_row - nlines, last_h_tag_begin_col + 1);
  return 1;
}

/* This function restores the rendering information for the currently
   highlighted tag to its status before the tag was highlighted. */
void
erase_tag_highlighting(void)
{
  int row, col;
  int final_highlight_col;
  int start_highlight_col;

  if (last_h_tag_index != -1) {
    for (row = last_h_tag_begin_row; row <= last_h_tag_end_row; row++) {
      final_highlight_col = (row == last_h_tag_end_row) ?
	  last_h_tag_end_col : row_width();
      start_highlight_col = (row == last_h_tag_begin_row) ?
	  last_h_tag_begin_col : 0;
      for (col = start_highlight_col; col < final_highlight_col; col++) {
	set_tag_highlight(row, col,
			  old_highlighting[row - last_h_tag_begin_row][col]);
      }
    }
  }
  /* We don't need to keep erasing now that nothing is highlighted */
  last_h_tag_index = -1;
}

/* Highlight a tag if one exists at the location specified in pixels.
   If no tag exists there and a tag is currently highlighted, we need
   to erase that tag's highlighting. */
int
show_tag(int row, int col)
{
  unsigned int tag_begin_col, tag_end_col, tag_begin_row, tag_end_row, tag_index;
  int final_highlight_col;
  int start_highlight_col;
  tag_highlight_t highlight;

  D_TAGS(("==> show_tag(%d,%d)\n", row, col));

  /* If there's no tag there and a tag is currently highlighted, we need
     to erase its highlighting. */
  if (!find_tag(row, col, &tag_begin_row, &tag_begin_col, &tag_end_row,
		&tag_end_col, &tag_index, 0, 0)) {
    D_TAGS(("  ==> no tag, erasing highlighting and leaving.\n"));
    /* Erase the old highlighting */
    tag_hide();

    return 0;
  }
  /* If we've come this far, then we are on a tag, and it needs to be
     highlighted. */

  /* If we're on the same tag as last time, there's no need to do
     anything. */
  if ((tag_index == last_h_tag_index) &&
      (tag_begin_row == last_h_tag_begin_row) &&
      (tag_end_row == last_h_tag_end_row) &&
      (tag_begin_col == last_h_tag_begin_col) &&
      (tag_end_col == last_h_tag_end_col))
    return 1;

  /* Erase the old highlighting */
  tag_hide();

  /* Add the new highlighting */
  for (row = tag_begin_row; row <= tag_end_row; row++) {
    final_highlight_col = (row == tag_end_row) ? tag_end_col :
	row_width();
    start_highlight_col = (row == tag_begin_row) ? tag_begin_col : 0;
    for (col = start_highlight_col; col < final_highlight_col; col++) {
      get_tag_highlight(row, col, &highlight);
      memcpy((void *) &old_highlighting[row - tag_begin_row][col],
	     (void *) &highlight, sizeof(tag_highlight_t));

      set_tag_highlight(row, col, tag[tag_index].highlight);
    }
  }

  /* Store the old values to erase later */
  last_h_tag_index = tag_index;
  last_h_tag_begin_row = tag_begin_row;
  last_h_tag_end_row = tag_end_row;
  last_h_tag_begin_col = tag_begin_col;
  last_h_tag_end_col = tag_end_col;

  return 1;
}

/* Check to see if there's a tag at the location specified by (x,y) (in
   pixels).  If so, execute the corresponding action.  Return 0 if there's
   no tag there, otherwise return 1. */
int
tag_activate(int row, int col, unsigned int binding_mask)
{
  int tag_begin_row, tag_end_row, tag_index, tag_begin_col, tag_end_col;

  D_TAGS(("tag_activate(row==%d, col==%d, ...)\n", row, col));

  /* If there is no tag to be activated here, return. */
  if (!find_tag(row, col, &tag_begin_row, &tag_begin_col, &tag_end_row,
		&tag_end_col, &tag_index, binding_mask, 0))
    return 0;

  /* Otherwise, activate the tag. */
  execute_tag(tag_begin_row, tag_begin_col, tag_end_row, tag_end_col,
	      tag_index);

  return 1;
}

/* Execute the tag specified by tag_index and contained between the
   indices specified by tag_begin and tag_end. */

void
execute_tag(int tag_begin_row, int tag_begin_col,
	    int tag_end_row, int tag_end_col, int tag_index)
{
  char tagstr[MAX_SEARCH_CHARS];
  char cmd[MAX_TAG_COMMAND_LENGTH];
  char *p, *q;
  pid_t pid;
  int dest_offset;
  int i, start_column, end_column;
  char *curr_row;

  printf("==> Activating tag %d:\n   ==> Action: [%s]\n Env: [%s]  ==> Output: %s\n",
	 tag_index, tag[tag_index].action, tag_env,
	 (tag[tag_index].output_type == TAG_OUTPUT_NULL) ? "NULL" :
	 ((tag[tag_index].output_type == TAG_OUTPUT_POPUP) ? "POPUP" :
	  ((tag[tag_index].output_type == TAG_OUTPUT_LOOP) ? "LOOP" :
	   ((tag[tag_index].output_type == TAG_OUTPUT_REPL) ? "REPL" :
	    "UNKNOWN"))));

  /* If the tag's action is TAG_ACTION_RELOAD, then we simply
     relaod the tag config file. */
  if (!strcmp(tag[tag_index].action, TAG_ACTION_RELOAD)) {
    for (i = 0; i < num_tags; i++)
#ifdef ACTIVE_TAGS_SPENCER_REGEXP
      regfree(tag[i].rx);
#else
      regfree(&tag[i].rx);
#endif
    num_tags = 0;
    parse_tag_config(tag_config_file);
    return;
  }
  if (!strcmp(tag[tag_index].action, TAG_ACTION_DISABLE)) {
    disable_tags();
    return;
  }
  /* For debugging */
  if (!strcmp(tag[tag_index].action, TAG_ACTION_MODE)) {
    char mode[1024];

    get_tag_mode(mode);
    fprintf(stderr, "Mode: %s\n", mode);
    return;
  }
  /* Fork off a separate process to execute the new tag. */
  pid = fork();

  if (pid == 0) {		/* child */

    D_TAGS(("Child\n"));

    i = tag_begin_row;
    dest_offset = 0;

    while (i <= tag_end_row) {
      start_column = i == tag_begin_row ? tag_begin_col : 0;
      end_column = i == tag_end_row ? tag_end_col : row_width();
      D_TAGS(("row: %d Start col: %d end_col: %d\n", i, start_column, end_column));
      tag_get_row(i, &curr_row);
      memcpy(tagstr + dest_offset,
	     curr_row + start_column,
	     end_column - start_column);

      dest_offset += end_column - start_column;
      i++;
    }

    tagstr[dest_offset] = '\0';
    D_TAGS(("\t==> tag string: {%s}\n", tagstr));
    /* Initialize the command string */
    *cmd = '\0';

    /* Build the command string from the action string by replacing
       all occurences of ${} in the action string with the tag string. */
    q = p = tag[tag_index].action;
    while ((p = strstr(q, "${}")) != NULL) {
      *p = '\0';
      strcat(cmd, q);

      strcat(cmd, tagstr);

      /* Step over the remaining characters of the ${} */
      q = p + 3;
    }

    strcat(cmd + strlen(cmd), q);

    if (!set_tag_pwd())
      fprintf(stderr, "Active Tags: couldn't set the pwd!\n");


    /* Set the UID appropriately so we don't act as the wrong user */
    if (!set_tag_uid()) {
      fprintf(stderr, "Active Tags: tag action: Couldn't set the uid!\n");
      exit(1);
    }
    /* For a loop action, we connect stdout on the tag process to stdin on
       the terminal's executing process */
    if (tag[tag_index].output_type == TAG_OUTPUT_LOOP)
      /* I wonder if screwing around with Eterm's stdin is a good idea >:)
       * -vendu
       */
      if (!set_tag_stdout()) {
	fprintf(stderr, "Active Tags: tag action: Couldn't set stdout for "
		"a loop action!\n");
	exit(1);
      }
    system(cmd);
#if 0
    exit(1);			/* This might be a bad idea :) Makes Eterm exit, at
				 * least if run from another Eterm. -vendu */
#endif
    return;
  }
}

#endif /* USE_ACTIVE_TAGS */
