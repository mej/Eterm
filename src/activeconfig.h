/*--------------------------------*-C-*---------------------------------*
 * File:	activeconfig.h
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

#ifdef ACTIVE_TAGS_SPENCER_REGEXP
#include "regexp/regexp.h"
#else
#include <regex.h>
#endif

#include "activetags.h"

/* The default locations for the config file */
#define TAG_CONFIG_USER_FILENAME ".active.tags"
#define TAG_CONFIG_SYSTEM_FILENAME "/etc/active.tags"

/* Defaults */
#define TAG_DEFAULT_SEARCH_LINES 1
#define TAG_DEFAULT_BINDING_MASK TAG_BINDING_BUTTON3
#define TAG_DEFAULT_HIGHLIGHT_BG TAG_HIGHLIGHT_NORMAL
#define TAG_DEFAULT_HIGHLIGHT_FG TAG_HIGHLIGHT_BLUE
#define TAG_DEFAULT_HIGHLIGHT_ATT 0

/* These are the config file tokens. */
#define TAG_CONFIG_DEFAULT_BINDING "DefaultBinding"
#define TAG_CONFIG_DEFAULT_HIGHLIGHT "DefaultHighlight"
#define TAG_CONFIG_DEFAULT_SEARCH_LINES "DefaultSearchLines"
#define TAG_CONFIG_SEARCH_LINES "SearchLines"
#define TAG_CONFIG_NEW_TAG "{"
#define TAG_CONFIG_END_TAG "}"
#define TAG_CONFIG_OUTPUT "Output"
#define TAG_CONFIG_LATENT  "Latent"
#define TAG_CONFIG_BINDING "Binding"
#define TAG_CONFIG_HIGHLIGHT "Highlight"
#define TAG_CONFIG_MODES "Modes"
#define TAG_CONFIG_REGEXP "Regexp"
#define TAG_CONFIG_ACTION "Action"
#define TAG_CONFIG_LOOPACTION "LoopAction"
#define TAG_CONFIG_CLUE "Clue"
#define TAG_CONFIG_LOAD "Load"
#define TAG_CONFIG_ENV  "Env"

/* Macros for parsing the config file */
#define TAG_CONFIG(x) (!strncmp(line, (x), strlen(x)))

/* The config_info structure holds all the information that each individual
   configuration parsing function needs as it runs */ 
struct config_info {
  int line_num;
  int default_binding;
  tag_highlight_t default_highlight;
  int default_search_lines;
  int in_tag;
  int curr_tag;
  char filename[1024]; 
};

/* Each dispatch table entry is of the following form */
struct config_entry {
  char * token;
  int (*parser)(char *, struct config_info *);
};

/* Configuration dispatch function prototypes */ 
int parse_config_default_binding(char *, struct config_info *);
int parse_config_default_highlight(char *, struct config_info *);
int parse_config_default_search_lines(char *, struct config_info *);
int parse_config_tag_begin(char *, struct config_info *);
int parse_config_tag_end(char *, struct config_info *);
int parse_config_tag_latent(char *, struct config_info *);
int parse_config_tag_binding(char *, struct config_info *);
int parse_config_tag_highlight(char *, struct config_info *);
int parse_config_tag_modes(char *, struct config_info *);
int parse_config_tag_regexp(char *, struct config_info *);
int parse_config_tag_action(char *, struct config_info *);
int parse_config_tag_search_lines(char *, struct config_info *);
int parse_config_tag_output(char *, struct config_info *);
int parse_config_tag_clue(char *, struct config_info *);
int parse_config_load(char * filename, struct config_info * config_info);
int parse_config_tag_env(char * envlist, struct config_info * config_info);

/* Internal helper functin prototypes */
void configerror(struct config_info * config_info, char * message);
void string_to_color(struct config_info * config_info, tag_highlight_t *
		     highlight, char * c);
void string_to_highlight(struct config_info * config_info, tag_highlight_t *
			 highlight, char * s);
unsigned int string_to_binding_mask(struct config_info * config_info,
				    char * s);
void set_config_defaults(struct config_info * config_info);
int parse_tag_file(const char * filename, struct config_info * config_info);
