/*--------------------------------*-C-*---------------------------------*
 * File:	activetags.h
 *
 * Copyright 1996,1997 Nat Friedman, Massachusetts Institute of Technology
 * <ndf@mit.edu>
 *
 * You can do what you like with this source code as long as
 * you don't try to make money out of it and you include an
 * unaltered copy of this message (including the copyright).
 *
 * The author accepts no responsibility for anything whatsoever, nor does he
 * guarantee anything, nor are any guarantees, promises, or covenants implicit
 * with the use of this software.
 *
 * For information regarding this particular module, please see
 * README.ActiveTags.
 *
 *----------------------------------------------------------------------*/

#ifndef _ACTIVE_TAGS_H
#define _ACTIVE_TAGS_H

#ifndef ACTIVE_TAGS
#define ACTIVE_TAGS
#endif

#ifdef ACTIVE_TAGS_SPENCER_REGEXP
#include "regexp/regexp.h"
#else
#include <sys/types.h>
#include <regex.h>
#endif

#include <sys/types.h>

#define TAG_ACTION_RELOAD  "*reload*"
#define TAG_ACTION_DISABLE "*disable*"

/* Debugging actions */
#define TAG_ACTION_MODE "*mode*"


#define TAG_DRAG_THRESHHOLD 500

/* Maximums */
#define MAX_TAGS 128
#define MAX_SEARCH_CHARS 32767
#define MAX_SEARCH_LINES 25
#define MAX_SEARCH_COLS 300
#define MAX_SCREEN_ROWS 128
#define MAX_TAG_MODES 32
#define MAX_TAG_MODE_LEN 32
#define MAX_TAG_COMMAND_LENGTH 2048
#define MAX_REGEXP_LEN 1024
#define MAX_ACTION_LEN 1024
#define MAX_CLUE_LENGTH 1024
#define MAX_MODE_CHECK_COUNT 25
#define MAX_TAG_ENVS 8
#define MAX_TAG_ENV_LEN 16

/* Output Types */
#define TAG_OUTPUT_POPUP 1
#define TAG_OUTPUT_LOOP  2
#define TAG_OUTPUT_NULL  3
#define TAG_OUTPUT_REPL  4

/* Binding fields */
#define TAG_BINDING_BUTTON1 (1<<0)
#define TAG_BINDING_BUTTON2 (1<<1)
#define TAG_BINDING_BUTTON3 (1<<2)
#define TAG_BINDING_SHIFT   (1<<3)
#define TAG_BINDING_CONTROL (1<<4)
#define TAG_BINDING_META    (1<<5)

/* Highlight fields */
#define TAG_HIGHLIGHT_RVID    (1L<<0)
#define TAG_HIGHLIGHT_BOLD    (1L<<1)
#define TAG_HIGHLIGHT_ULINE   (1L<<2)
#define TAG_HIGHLIGHT_BLINK   (1L<<3)

#define TAG_HIGHLIGHT_BLACK   (1L<<0)
#define TAG_HIGHLIGHT_WHITE   (1L<<1)
#define TAG_HIGHLIGHT_RED     (1L<<2)
#define TAG_HIGHLIGHT_GREEN   (1L<<3)
#define TAG_HIGHLIGHT_YELLOW  (1L<<4)
#define TAG_HIGHLIGHT_BLUE    (1L<<5)
#define TAG_HIGHLIGHT_MAGENTA (1L<<6)
#define TAG_HIGHLIGHT_CYAN    (1L<<7)
#define TAG_HIGHLIGHT_NORMAL  -1

typedef struct tag_highlight {
  /* Underline, Reverse Video, Bold */
  int attributes;

  /* Highlighting colors */
  int fg_color;
  int bg_color;

} tag_highlight_t;

struct active_tag {

  /* rx is the compiled regular expression for the tag */
#ifdef ACTIVE_TAGS_SPENCER_REGEXP
  regexp * rx;
#else
  regex_t rx;
#endif

  /* The action is a program to be executed.  The string ${} in the action
     will be replaced with the tag. */
  char action[MAX_ACTION_LEN];

  tag_highlight_t highlight;

  /* The button/key combo that activates this tag. */
  unsigned int binding_mask;

  /* Number of lines to search for this tag */
  int search_lines;

  char mode[MAX_TAG_MODES][MAX_TAG_MODE_LEN];
  int num_modes;

  char env[MAX_TAG_ENVS][MAX_TAG_ENV_LEN];
  int num_envs;

  /* See TAG_OUTPUT_* */
  int output_type;

  /* Whether or not the tag is latent. */
  int latent;

  char clue[MAX_CLUE_LENGTH];
};


/* Template prototypes */
void get_tag_mode(char * mode);
/* int row_width(void); */
void tag_get_row(int row_num, char ** row);
/* int tag_min_row(void); */
/* int tag_max_row(void); */
void set_tag_highlight(int row, int col, tag_highlight_t highlight);
void get_tag_highlight(int row, int col, tag_highlight_t * highlight);
int  set_tag_stdout(void);
int  set_tag_pwd(void);
int  set_tag_uid(void);

/* Prototypes */
void parse_tag_config(char * tag_config_file);
int find_tag(int row, int col, int * tag_begin_row, int * tag_begin_col,
	     int * tag_end_row, int * tag_end_col,	     
	     int * tag_index, unsigned int binding_mask, int tag_begin_index);
int show_tag(int x, int y);
int tag_activate(int row, int col, unsigned int binding_mask);
void execute_tag(int tag_begin_row, int tag_begin_col, int tag_end_row, int tag_end_col, int tag_index);
void initialize_tags(void);
int tag_screen_scroll(int nlines, int start_row, int end_row);
void reap_tag_process(pid_t pid);
void erase_tag_highlighting(void);
void disable_tags(void);
void tag_hide(void);

/* Externs */
extern int active_tags_enabled;
extern struct active_tag tag[MAX_TAGS];
extern int num_tags;
extern int last_h_tag_begin_row;
extern int last_h_tag_begin_col;
extern int last_h_tag_index;
extern char * tag_env;

#endif /* _ACTIVE_TAGS_H */








