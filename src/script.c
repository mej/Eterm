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
#include "startup.h"
#include "options.h"
#include "pixmap.h"
#include "system.h"
#include "script.h"

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

void
script_parse(char *s)
{
  char **token_list, **param_list;
  register char *pstr;
  register unsigned long i;
  char *func_name, *params, *tmp;
  size_t len;

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
        return;
      }
    } else {
      func_name = STRDUP(pstr);
    }
    if (func_name) {
      chomp(func_name);
    }
    if (params) {
      params++;
      if ((tmp = strchr(params, ')')) != NULL) {
        *tmp = 0;
      } else {
        print_error("Error in script \"%s\":  Missing closing parentheses for \"%s\".\n", s, token_list[i]);
        return;
      }
      param_list = split(", \t", params);
    }
    D_SCRIPT(("Calling function %s with parameters:  %s\n", NONULL(func_name), NONULL(params)));
  }

}
