/*--------------------------------*-C-*---------------------------------*
 * File:	activeconfig.c
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

/* This file contains all the config file parsing functionality */

static const char cvs_ident[] = "$Id$";

#include "feature.h"
#ifdef USE_ACTIVE_TAGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>
#include <pwd.h>

#include "activeconfig.h"

/* The basic design of the config file parsing routines revolves around the
   dispatch table below.  The dispatch table relates file tokens with
   their parsing functions.  In order to add a new config file token
   (for example "Foo=something"), simply define TAG_CONFIG_FOO, write
   parse_config_tag_foo, and then add the following line to the dispatch
   table:

   {TAG_CONFIG_FOO, parse_config_tag_foo}

   and it will magically work. */

/* This is the actual dispatch table.  It should be terminated by a struct
   config_entry whose parser field is NULL. */
struct config_entry config_dispatch[] =
{
  {TAG_CONFIG_LOAD, parse_config_load},
  {TAG_CONFIG_DEFAULT_BINDING, parse_config_default_binding},
  {TAG_CONFIG_DEFAULT_HIGHLIGHT, parse_config_default_highlight},
  {TAG_CONFIG_DEFAULT_SEARCH_LINES, parse_config_default_search_lines},
  {TAG_CONFIG_NEW_TAG, parse_config_tag_begin},
  {TAG_CONFIG_END_TAG, parse_config_tag_end},
  {TAG_CONFIG_LATENT, parse_config_tag_latent},
  {TAG_CONFIG_BINDING, parse_config_tag_binding},
  {TAG_CONFIG_HIGHLIGHT, parse_config_tag_highlight},
  {TAG_CONFIG_MODES, parse_config_tag_modes},
  {TAG_CONFIG_REGEXP, parse_config_tag_regexp},
  {TAG_CONFIG_ACTION, parse_config_tag_action},
  {TAG_CONFIG_OUTPUT, parse_config_tag_output},
  {TAG_CONFIG_SEARCH_LINES, parse_config_tag_search_lines},
  {TAG_CONFIG_CLUE, parse_config_tag_clue},
  {TAG_CONFIG_ENV, parse_config_tag_env},
  {"", NULL}
};

/*
   Dispatch Functions
 */

int
parse_config_tag_env(char *envlist, struct config_info *config_info)
{
  char *p, *q;
  int nenvs = 0;

  p = envlist;
  do {
    printf("Env...\n");
    if ((q = strchr(p, ',')) == NULL)
      q = p + strlen(p);
    strncpy(tag[config_info->curr_tag].env[nenvs], p, q - p);
    tag[config_info->curr_tag].env[nenvs][q - p] = '\0';
    printf("got env: %s\n", tag[config_info->curr_tag].env[nenvs]);
    nenvs++;
    if (*q != ',')
      p = NULL;
    else
      p = q + 1;
  } while (p != NULL);
  tag[config_info->curr_tag].num_envs = nenvs;

  printf("Got envs for tag:\n");
  for (nenvs = 0; nenvs < tag[config_info->curr_tag].num_envs; nenvs++)
    printf("Env %d: (%s)\n", nenvs, tag[config_info->curr_tag].env[nenvs]);
  return 1;
}

int
parse_config_load(char *filename, struct config_info *config_info)
{
  struct config_info new_config_info;

  /* Scope on Defaults is per-file */
  set_config_defaults(&new_config_info);
  new_config_info.curr_tag = config_info->curr_tag;

  /* If it's a relative file name, make it absolute */
  if (*filename != '/') {
    char new_filename[1024];
    char *p, *q;

    if ((p = strrchr(config_info->filename, '/')) == NULL)
      configerror(config_info, "Could not determine path to file!");
    q = config_info->filename;

    strcpy(new_filename, q);
    new_filename[p - q] = '\0';
    strcat(new_filename, "/");
    strcat(new_filename, filename);
    printf("Filename: %s\n", new_filename);
    strcpy(filename, new_filename);
  }
  /* FIXME: This implementation forces pathnames to be absolute.  We should
     allow relative pathnames somehow. */
  parse_tag_file(filename, &new_config_info);

  config_info->curr_tag = new_config_info.curr_tag;

  return 1;
}

int
parse_config_default_binding(char *def,
			     struct config_info *config_info)
{
  config_info->default_binding = string_to_binding_mask(config_info, def);
  if (!(config_info->default_binding & (TAG_BINDING_BUTTON1 | TAG_BINDING_BUTTON2 |
					TAG_BINDING_BUTTON3))) {
    configerror(config_info, "Error reading default binding: binding _must_ "
		"include either Button1, Button2, or Button3.  Reverting to "
		"compiled default.");
    config_info->default_binding = TAG_DEFAULT_BINDING_MASK;
  }
  return 1;
}

int
parse_config_default_highlight(char *def,
			       struct config_info *config_info)
{
  string_to_highlight(config_info, &config_info->default_highlight, def);
  return 1;
}

int
parse_config_default_search_lines(char *def,
				  struct config_info *config_info)
{
  if ((config_info->default_search_lines = atoi(def)) == 0) {
    configerror(config_info, "Invalid default number of search lines.  "
		"Reverting to compiled default.\n");
    config_info->default_search_lines = TAG_DEFAULT_SEARCH_LINES;
  }
  if (config_info->default_search_lines > MAX_SEARCH_LINES) {
    configerror(config_info, "Default number of search lines > maximum.  "
		"Reverting to compiled default.\n");
    config_info->default_search_lines = TAG_DEFAULT_SEARCH_LINES;
  }
  return 1;
}

int
parse_config_tag_begin(char *s, struct config_info *config_info)
{
  fprintf(stderr, "New tag\n");
  if (config_info->in_tag) {
    configerror(config_info, "Open brace inside braces");
    disable_tags();
    return -1;
  }
  if ((config_info->curr_tag + 1) >= MAX_TAGS) {
    configerror(config_info, "Too many tags!  Increase the maximum number "
		"of tags and recompile!\n");
    disable_tags();
    return -1;
  }
  config_info->in_tag = 1;

  /* Initialize the new tag with all the default values. */
  tag[config_info->curr_tag].binding_mask = config_info->default_binding;
  tag[config_info->curr_tag].search_lines = config_info->default_search_lines;
  tag[config_info->curr_tag].highlight.attributes =
      config_info->default_highlight.attributes;
  tag[config_info->curr_tag].highlight.fg_color =
      config_info->default_highlight.fg_color;
  tag[config_info->curr_tag].highlight.bg_color =
      config_info->default_highlight.bg_color;
  tag[config_info->curr_tag].num_modes = 0;
  tag[config_info->curr_tag].latent = 0;
  tag[config_info->curr_tag].output_type = TAG_OUTPUT_NULL;

#ifdef ACTIVE_TAGS_SPENCER_REGEXP
  tag[config_info->curr_tag].rx = NULL;
#endif

  return 1;
}

int
parse_config_tag_end(char *s, struct config_info *config_info)
{
  if (!config_info->in_tag) {
    configerror(config_info, "close brace without open brace");
    disable_tags();
    return -1;
  }
  /* Make sure that the tag was setup properly before advancing to the next
     one in the list */

  /* FIXME: we need a way of figuring this out if we're using POSIX regex */
#ifdef ACTIVE_TAGS_SPENCER_REGEXP
  if (tag[config_info->curr_tag].rx == NULL) {
    configerror("No regular epxression supplied for tag.  Tag ignored.");
    config_info->curr_tag--;
  }
#endif

  config_info->in_tag = 0;
  config_info->curr_tag++;

  return 1;
}

int
parse_config_tag_latent(char *latent, struct config_info *config_info)
{
  if (!strcasecmp(latent, "true"))
    tag[config_info->curr_tag].latent = 1;
  return 1;
}

int
parse_config_tag_search_lines(char *sl, struct config_info *config_info)
{
  if (atoi(sl))
    tag[config_info->curr_tag].search_lines = atoi(sl);

  D_TAGS(("==> Setting tag %d's search lines to %d\n", config_info->curr_tag, atoi(sl)));

  return 1;
}

int
parse_config_tag_binding(char *s, struct config_info *config_info)
{
  tag[config_info->curr_tag].binding_mask = string_to_binding_mask(config_info, s);
  return 1;
}

int
parse_config_tag_highlight(char *s, struct config_info *config_info)
{
  string_to_highlight(config_info, &tag[config_info->curr_tag].highlight, s);
  return 1;
}

int
parse_config_tag_modes(char *s, struct config_info *config_info)
{
  char *mode, *p;

  mode = s;

  while (mode != NULL) {
    if ((p = strchr(mode, ',')) != NULL)
      *p = '\0';
    strcpy(tag[config_info->curr_tag].mode[tag[config_info->curr_tag].num_modes], mode);
    if (p != NULL)
      mode = p + 1;
    else
      mode = NULL;

    tag[config_info->curr_tag].num_modes++;
  }

  return 1;
}

int
parse_config_tag_regexp(char *regexp, struct config_info *config_info)
{
#ifdef ACTIVE_TAGS_SPENCER_REGEXP
  if ((tag[config_info->curr_tag].rx = regcomp(regexp)) == NULL)
#else
  if (regcomp(&tag[config_info->curr_tag].rx, regexp, REG_EXTENDED) != 0)
#endif
  {
    configerror(config_info, "Couldn't compile regular expression");
    return -1;
  }
  return 1;
}

int
parse_config_tag_action(char *action, struct config_info *config_info)
{
  strcpy(tag[config_info->curr_tag].action, action);
  return 1;
}

int
parse_config_tag_output(char *output, struct config_info *config_info)
{
  if (!strcasecmp(output, "null"))
    tag[config_info->curr_tag].output_type = TAG_OUTPUT_NULL;
  else if (!strcasecmp(output, "loop"))
    tag[config_info->curr_tag].output_type = TAG_OUTPUT_LOOP;
  else if (!strcasecmp(output, "replace"))
    tag[config_info->curr_tag].output_type = TAG_OUTPUT_REPL;
  else if (!strcasecmp(output, "popup"))
    tag[config_info->curr_tag].output_type = TAG_OUTPUT_POPUP;
  else {
    configerror(config_info, "Unknown output method; defaulting to NULL");
    tag[config_info->curr_tag].output_type = TAG_OUTPUT_NULL;
  }

  return 1;
}

int
parse_config_tag_clue(char *clue, struct config_info *config_info)
{
  strcpy(tag[config_info->curr_tag].clue, clue);
  return 1;
}



void
set_config_defaults(struct config_info *config_info)
{
  config_info->default_binding = TAG_DEFAULT_BINDING_MASK;
  config_info->default_search_lines = TAG_DEFAULT_SEARCH_LINES;
  config_info->default_highlight.bg_color = TAG_DEFAULT_HIGHLIGHT_BG;
  config_info->default_highlight.fg_color = TAG_DEFAULT_HIGHLIGHT_FG;
  config_info->default_highlight.attributes = TAG_DEFAULT_HIGHLIGHT_ATT;
  config_info->curr_tag = 0;
  config_info->line_num = 0;
  config_info->in_tag = 0;
}

/* parse_tag_config actually reads the file and calls the dispatch functions
   where appropriate */
void
parse_tag_config(char *tag_config_file)
{
  char file_name[1024];
  struct passwd *user;

  struct config_info config_info;

  /* Set the defaults */
  set_config_defaults(&config_info);

  if (tag_config_file != NULL)
    if (!parse_tag_file(tag_config_file, &config_info)) {
      fprintf(stderr, "parse_tag_config: Couldn't open tag config"
	      "file: %s\n", tag_config_file);
      tag_config_file = NULL;
    }
  if (tag_config_file == NULL) {
    user = getpwuid(getuid());
    sprintf(file_name, "%s/%s", user->pw_dir, TAG_CONFIG_USER_FILENAME);
    if (!parse_tag_file(file_name, &config_info)) {
      fprintf(stderr, "parse_tag_config: Couldn't open user tag config "
	      "file: %s\n", file_name);
      fprintf(stderr, "parse_tag_config: Trying system config file\n");

      /* Try the system-wide config file */
      if (!parse_tag_file(TAG_CONFIG_SYSTEM_FILENAME, &config_info)) {
	fprintf(stderr, "parse_tag_config: Error parsing config file: "
		"%s\n", TAG_CONFIG_SYSTEM_FILENAME);
	disable_tags();
	return;
      }
    }
  }
  num_tags = config_info.curr_tag;
  printf("Num tags: %d\n", num_tags);
  {
    int i;

    for (i = 0; i < num_tags; i++)
      printf("Tag action(%d): %s\n", i, tag[i].action);
  }
}

int
parse_tag_file(const char *filename, struct config_info *config_info)
{
  FILE *tag_file;
  char line[1024];
  int i;

  if ((tag_file = fopen(filename, "r")) == NULL)
    return 0;

  strcpy(config_info->filename, filename);

  /* Loop through the config file lines */
  while (!feof(tag_file)) {
    fgets(line, sizeof(line), tag_file);
    config_info->line_num++;

    if (feof(tag_file))
      break;

    if (line[strlen(line) - 1] != '\n') {
      configerror(config_info, "line too long?");
      exit(1);
    }
    line[strlen(line) - 1] = '\0';

    /*  Loop through the config file lines, calling the appropriate
       functions from the dispatch table as we go.  If there is no
       corresponding function, flag a warning and try to continue. */
    if ((line[0] != '#') && (!isspace(line[0])) && (strlen(line) != 0)) {
      for (i = 0; config_dispatch[i].parser != NULL; i++)
	if (TAG_CONFIG(config_dispatch[i].token)) {
	  if ((strchr(line, '=') == NULL) && (*line != '{') &&
	      (*line != '}') && !TAG_CONFIG(TAG_CONFIG_LOAD))
	    configerror(config_info, "'=' not found");
	  else {
	    char *p;

	    p = line + strlen(config_dispatch[i].token) + 1;
	    if (strchr(line, '=') != NULL) {
	      p = strchr(line, '=') + 1;
	      while (isspace(*p))
		p++;
	    }
	    if (!((config_dispatch[i].parser) (p, config_info)))
	      return 0;
	    break;
	  }
	}
      if (config_dispatch[i].parser == NULL)
	configerror(config_info, "Unrecognized token");
    }
  }
  fclose(tag_file);

  return 1;
}

/*
   Internal Functions
 */

/* Use this function to display errors encountered while parsing the config
   file to keep them looking uniform */
void
configerror(struct config_info *config_info, char *message)
{
  fprintf(stderr, "active tags: error on line %d of config file %s: %s\n",
	  config_info->line_num, config_info->filename, message);
}

void
string_to_color(struct config_info *config_info, tag_highlight_t * highlight,
		char *c)
{
#if 0
  int color;

#endif
  int color = 0;
  int bg = 0;
  char *p;

  p = c;

  /* Background colors are prefaced by a '*' */
  if (*p == '*') {
    bg = 1;
    p++;
  }
  if (!strcasecmp(p, "Black"))
    color = TAG_HIGHLIGHT_BLACK;
  else if (!strcasecmp(p, "White"))
    color = TAG_HIGHLIGHT_WHITE;
  else if (!strcasecmp(p, "Red"))
    color = TAG_HIGHLIGHT_RED;
  else if (!strcasecmp(p, "Green"))
    color = TAG_HIGHLIGHT_GREEN;
  else if (!strcasecmp(p, "Yellow"))
    color = TAG_HIGHLIGHT_YELLOW;
  else if (!strcasecmp(p, "Blue"))
    color = TAG_HIGHLIGHT_BLUE;
  else if (!strcasecmp(p, "Magenta"))
    color = TAG_HIGHLIGHT_MAGENTA;
  else if (!strcasecmp(p, "Cyan"))
    color = TAG_HIGHLIGHT_CYAN;
  else if (!strcasecmp(p, "Normal"))
    color = TAG_HIGHLIGHT_NORMAL;
  else
    configerror(config_info, "Unrecognized highlight token");

  if (bg)
    highlight->bg_color = color;
  else
    highlight->fg_color = color;
}

void
string_to_highlight(struct config_info *config_info, tag_highlight_t *
		    highlight, char *s)
{
  char *h_bit;

  /* att_set is 0 if we've set an attribute value and 1 otherwise.  We have to
     keep track of this because setting an attribute value should override the
     default, so we can't blindly OR the new values with whatever was in
     highlight->attribute before.  So, if we've already overriden the value,
     we OR.  If we haven't yet overriden it, then we do so and set att_set to
     1. */
  int att_set = 0;

  h_bit = strtok(s, "&");

  while (h_bit != NULL) {
    if (!strcasecmp(h_bit, "Underline")) {
      if (att_set)
	highlight->attributes |= TAG_HIGHLIGHT_ULINE;
      else {
	highlight->attributes = TAG_HIGHLIGHT_ULINE;
	highlight->fg_color = TAG_HIGHLIGHT_NORMAL;
	highlight->bg_color = TAG_HIGHLIGHT_NORMAL;
	att_set = 1;
      }
    } else if (!strcasecmp(h_bit, "Bold")) {
      if (att_set)
	highlight->attributes |= TAG_HIGHLIGHT_BOLD;
      else {
	highlight->attributes = TAG_HIGHLIGHT_BOLD;
	highlight->fg_color = TAG_HIGHLIGHT_NORMAL;
	highlight->bg_color = TAG_HIGHLIGHT_NORMAL;
	att_set = 1;
      }
    } else if (!strcasecmp(h_bit, "RVid")) {
      if (att_set)
	highlight->attributes |= TAG_HIGHLIGHT_RVID;
      else {
	highlight->attributes = TAG_HIGHLIGHT_RVID;
	highlight->fg_color = TAG_HIGHLIGHT_NORMAL;
	highlight->bg_color = TAG_HIGHLIGHT_NORMAL;
	att_set = 1;
      }
    } else if (!strcasecmp(h_bit, "Blink")) {
      if (att_set)
	highlight->attributes |= TAG_HIGHLIGHT_BLINK;
      else {
	highlight->attributes = TAG_HIGHLIGHT_BLINK;
	highlight->fg_color = TAG_HIGHLIGHT_NORMAL;
	highlight->bg_color = TAG_HIGHLIGHT_NORMAL;
	att_set = 1;
      }
    } else {
      if (att_set)
	string_to_color(config_info, highlight, h_bit);
      else {
	att_set = 1;
	highlight->fg_color = TAG_HIGHLIGHT_NORMAL;
	highlight->bg_color = TAG_HIGHLIGHT_NORMAL;
	highlight->attributes = 0;
	string_to_color(config_info, highlight, h_bit);
      }
    }

    h_bit = strtok(NULL, "&");
  }
}

unsigned int
string_to_binding_mask(struct config_info *config_info, char *s)
{
  char *b_bit;
  unsigned int mask = 0;

  b_bit = strtok(s, "&");
  while (b_bit != NULL) {
    if (!strcasecmp(b_bit, "Button1"))
      mask |= TAG_BINDING_BUTTON1;
    else if (!strcasecmp(b_bit, "Button2"))
      mask |= TAG_BINDING_BUTTON2;
    else if (!strcasecmp(b_bit, "Button3"))
      mask |= TAG_BINDING_BUTTON3;
    else if (!strcasecmp(b_bit, "Shift"))
      mask |= TAG_BINDING_SHIFT;
    else if (!strcasecmp(b_bit, "Control"))
      mask |= TAG_BINDING_CONTROL;
    else if (!strcasecmp(b_bit, "Meta"))
      mask |= TAG_BINDING_META;
    else
      configerror(config_info, "Unknown binding token");

    b_bit = strtok(NULL, "&");
  }

  return mask;
}

#endif /* USE_ACTIVE_TAGS */
