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

#include "config.h"
#include "feature.h"

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <X11/cursorfont.h>
#include <signal.h>

#include "command.h"
#include "options.h"
#include "pixmap.h"
#include "screen.h"
#include "script.h"
#include "startup.h"
#include "system.h"

static eterm_script_handler_t script_handlers[] =
{
  { "copy",      script_handler_copy },
  { "die",       script_handler_exit },
  { "exec",      script_handler_spawn },
  { "exit",      script_handler_exit },
  { "paste",     script_handler_paste },
  { "quit",      script_handler_exit },
  { "save",      script_handler_save },
  { "search",    script_handler_search },
  { "spawn",     script_handler_spawn },

  { "nop", script_handler_nop }
};
static size_t handler_count = sizeof(script_handlers) / sizeof(eterm_script_handler_t);

#if 0
void
eterm_handle_winop(char *action)
{
  char *winid;
  Window win = 0;

  ASSERT(action != NULL);

  winid = strchr(action, ' ');
  if (winid) {
    win = (Window) strtoul(winid + 1, (char **) NULL, 0);
  }
  if (win == 0) {		/* If no window ID was given, or if the strtoul() call failed */
    win = TermWin.parent;
  }
  if (!BEG_STRCASECMP(action, "raise")) {
    XRaiseWindow(Xdisplay, win);
  } else if (!BEG_STRCASECMP(action, "lower")) {
    XLowerWindow(Xdisplay, win);
  } else if (!BEG_STRCASECMP(action, "map")) {
    XMapWindow(Xdisplay, win);
  } else if (!BEG_STRCASECMP(action, "unmap")) {
    XUnmapWindow(Xdisplay, win);
  } else if (!BEG_STRCASECMP(action, "move")) {
    int x, y, n;
    char *xx, *yy;

    n = num_words(action);
    if (n == 3 || n == 4) {
      if (n == 3) {
        win = TermWin.parent;
      }
      xx = get_pword(n - 1, action);
      yy = get_pword(n, action);
      x = (int) strtol(xx, (char **) NULL, 0);
      y = (int) strtol(yy, (char **) NULL, 0);
      XMoveWindow(Xdisplay, win, x, y);
    }
  } else if (!BEG_STRCASECMP(action, "resize")) {
    int w, h, n;
    char *ww, *hh;

    n = num_words(action);
    if (n == 3 || n == 4) {
      if (n == 3) {
        win = TermWin.parent;
      }
      ww = get_pword(n - 1, action);
      hh = get_pword(n, action);
      w = (int) strtol(ww, (char **) NULL, 0);
      h = (int) strtol(hh, (char **) NULL, 0);
      XResizeWindow(Xdisplay, win, w, h);
    }
  } else if (!BEG_STRCASECMP(action, "kill")) {
    XKillClient(Xdisplay, win);
  } else if (!BEG_STRCASECMP(action, "iconify")) {
    XIconifyWindow(Xdisplay, win, Xscreen);
  } else {
    print_error("IPC Error:  Unrecognized window operation \"%s\"\n", action);
  }
}
#endif

/********* HANDLERS **********/
void
script_handler_copy(char **params)
{
  unsigned char i;
  char *buffer_id;
  Atom sel = XA_PRIMARY;

  if (params) {
    for (i = 0; (buffer_id = params[i]) != NULL; i++) {
      if (*buffer_id) {
        if (*buffer_id >= '0' && *buffer_id <= '7') {
          sel = (Atom) ((int) XA_CUT_BUFFER0 + (int) *buffer_id);
        } else if (!BEG_STRCASECMP(buffer_id, "clipboard")) {
          sel = XA_CLIPBOARD(Xdisplay);
        } else if (!BEG_STRCASECMP(buffer_id, "primary")) {
          sel = XA_PRIMARY;
        } else if (!BEG_STRCASECMP(buffer_id, "secondary")) {
          sel = XA_SECONDARY;
        } else {
          print_error("Invalid parameter to copy():  \"%s\"\n", buffer_id);
        }
      }
    }
  }
  selection_copy(sel);
}

void
script_handler_exit(char **params)
{
  unsigned char code = 0;
  char *tmp;

  if (params && *params) {
    if (isdigit(params[0][0]) || (params[0][0] == '-' && isdigit(params[0][1]))) {
      code = (unsigned char) atoi(params[0]);
    } else {
      tmp = join(" ", params);
      printf("Exiting:  %s\n", tmp);
      FREE(tmp);
    }
  }
  exit(code);
}

void
script_handler_paste(char **params)
{
  unsigned char i;
  char *buffer_id;
  Atom sel = XA_PRIMARY;

  if (params) {
    for (i = 0; (buffer_id = params[i]) != NULL; i++) {
      if (*buffer_id) {
        if (*buffer_id >= '0' && *buffer_id <= '7') {
          sel = (Atom) ((int) XA_CUT_BUFFER0 + (int) *buffer_id);
        } else if (!BEG_STRCASECMP(buffer_id, "clipboard")) {
          sel = XA_CLIPBOARD(Xdisplay);
        } else if (!BEG_STRCASECMP(buffer_id, "primary")) {
          sel = XA_PRIMARY;
        } else if (!BEG_STRCASECMP(buffer_id, "secondary")) {
          sel = XA_SECONDARY;
        } else {
          print_error("Invalid parameter to paste():  \"%s\"\n", buffer_id);
        }
      }
    }
  }
  selection_paste(sel);
}

void
script_handler_save(char **params)
{
  if (params && *params) {
    if (!strcasecmp(params[0], "theme")) {
      save_config(params[1], SAVE_THEME_CONFIG);
    } else {
      save_config(params[0], SAVE_USER_CONFIG);
    }
  } else {
    save_config(NULL, SAVE_USER_CONFIG);
  }
}

void
script_handler_search(char **params)
{
  scr_search_scrollback(params ? params[0] : NULL);
}

void
script_handler_spawn(char **params)
{
  char *tmp;

  if (params && *params) {
    tmp = join(" ", params);
    system_no_wait(tmp);
    FREE(tmp);
  } else {
    system_no_wait("Eterm");
  }
}

void
script_handler_nop(char **params)
{
  USE_VAR(params);
}

/********* ENGINE *********/
eterm_script_handler_t *
script_find_handler(const char *name)
{
  register unsigned long i;

  for (i = 0; i < handler_count; i++) {
    /* Small optimization.  Only call strcasecmp() if the first letter matches. */
    if ((tolower(name[0]) == tolower(script_handlers[i].name[0]))
        && !strcasecmp(name, script_handlers[i].name)) {
      return &script_handlers[i];
    }
  }
  return NULL;
}

void
script_parse(char *s)
{
  char **token_list, **param_list;
  register char *pstr;
  register unsigned long i;
  char *func_name, *params, *tmp;
  size_t len;
  eterm_script_handler_t *func;

  REQUIRE(s != NULL);

  D_SCRIPT(("Parsing:  \"%s\"\n", s));

  token_list = split(";", s);
  if (token_list == NULL) {
    D_SCRIPT(("No tokens found; ignoring script.\n"));
    return;
  }

  for (i = 0; token_list[i]; i++) {
    pstr = token_list[i];
    chomp(pstr);
    if (!(*pstr)) {
      continue;
    }
    if ((params = strchr(pstr, '(')) != NULL) {
      if (params != pstr) {
        len = params - pstr;
        func_name = (char *) MALLOC(len + 1);
        strncpy(func_name, pstr, len);
        func_name[len] = 0;
      } else {
        print_error("Error in script \"%s\":  Missing function name before \"%s\".\n", s, params);
        free_array((void **) token_list, 0);
        return;
      }
    } else {
      func_name = STRDUP(pstr);
    }
    if (!func_name) {
      free_array((void **) token_list, 0);
      return;
    }
    if (params) {
      params++;
      if ((tmp = strrchr(params, ')')) != NULL) {
        *tmp = 0;
      } else {
        print_error("Error in script \"%s\":  Missing closing parentheses for \"%s\".\n", s, token_list[i]);
        free_array((void **) token_list, 0);
        return;
      }
      param_list = split(", \t", params);
    } else {
      param_list = NULL;
    }
    D_SCRIPT(("Calling function %s with parameters:  %s\n", func_name, NONULL(params)));
    if ((func = script_find_handler(func_name)) != NULL) {
      (func->handler)(param_list);
    } else {
      print_error("Error in script \"%s\":  No such function \"%s\".\n", s, func_name);
    }
  }

  if (params) {
    free_array((void **) param_list, 0);
  }
  free_array((void **) token_list, 0);
}
