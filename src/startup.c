/* startup.c -- Eterm startup code
 *           -- 7 October 1999, mej
 *
 * This file is original work by Michael Jennings <mej@eterm.org> and
 * Tuomo Venalainen <vendu@cc.hut.fi>.  This file, and any other file
 * bearing this same message or a similar one, is distributed under
 * the GNU Public License (GPL) as outlined in the COPYING file.
 *
 * Copyright (C) 1997-1999, Michael Jennings and Tuomo Venalainen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
 */

static const char cvs_ident[] = "$Id$";

#include "config.h"
#include "feature.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <ctype.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>

#include "../libmej/debug.h"	/* from libmej */
#include "debug.h"
#include "../libmej/mem.h"
#include "../libmej/strings.h"
#include "startup.h"
#include "actions.h"
#include "command.h"
#include "eterm_utmp.h"
#include "events.h"
#include "options.h"
#include "pixmap.h"
#include "screen.h"
#include "scrollbar.h"
#include "term.h"
#include "windows.h"

char *orig_argv0;

#ifdef PIXMAP_SUPPORT
/* Set to one in case there is no WM, or a lousy one
   that doesn't send the right events (*cough*
   Window Maker *cough*) -- mej */
short bg_needs_update = 1;
#endif
TermWin_t TermWin;
Display *Xdisplay;		/* display */
Colormap cmap;
unsigned int debug_level = 0;	/* Level of debugging information to display */
const char *display_name = NULL;
unsigned int colorfgbg;

int
eterm_bootstrap(int argc, char *argv[])
{

  int i;
  char *val;
  static char windowid_string[20], *display_string, *term_string;	/* "WINDOWID=\0" = 10 chars, UINT_MAX = 10 chars */
  ImlibInitParams params;

  orig_argv0 = argv[0];

  /* Security enhancements -- mej */
  putenv("IFS= \t\n");
  my_ruid = getuid();
  my_euid = geteuid();
  my_rgid = getgid();
  my_egid = getegid();
  privileges(REVERT);

  PABLO_START_TRACING();
  getcwd(initial_dir, PATH_MAX);

  /* Open display, get options/resources and create the window */
  if ((display_name = getenv("DISPLAY")) == NULL)
    display_name = ":0";

  /* This MUST be called before any other Xlib functions */

  get_initial_options(argc, argv);
  init_defaults();

#ifdef NEED_LINUX_HACK
  privileges(INVOKE);		/* xdm in new Linux versions requires ruid != root to open the display -- mej */
#endif
  Xdisplay = XOpenDisplay(display_name);
#ifdef NEED_LINUX_HACK
  privileges(REVERT);
#endif
  if (!Xdisplay) {
    print_error("can't open display %s", display_name);
    exit(EXIT_FAILURE);
  }
#if DEBUG >= DEBUG_X
  if (debug_level >= DEBUG_X) {
    XSetErrorHandler((XErrorHandler) abort);
  } else {
    XSetErrorHandler((XErrorHandler) xerror_handler);
  }
#else
  XSetErrorHandler((XErrorHandler) xerror_handler);
#endif

  if (Options & Opt_install) {
    cmap = XCreateColormap(Xdisplay, Xroot, Xvisual, AllocNone);
    XInstallColormap(Xdisplay, cmap);
#ifdef PIXMAP_SUPPORT
    params.cmap = cmap;
    params.flags = PARAMS_COLORMAP;
#endif
  } else {
    cmap = Xcmap;
#ifdef PIXMAP_SUPPORT
    params.flags = 0;
#endif
  }

  /* Since we always use Imlib now, let's initialize it here. */
#ifdef PIXMAP_SUPPORT
  if (params.flags) {
    imlib_id = Imlib_init_with_params(Xdisplay, &params);
  } else {
    imlib_id = Imlib_init(Xdisplay);
  }
  if (!imlib_id) {
    fatal_error("Unable to initialize Imlib.  Aborting.");
  }
#endif

  get_modifiers();  /* Set up modifier masks before parsing config files. */

  read_config(THEME_CFG);
  read_config((rs_config_file ? rs_config_file : USER_CFG));

#if defined(PIXMAP_SUPPORT)
  if (rs_path || theme_dir || user_dir) {
    register unsigned long len;
    register char *tmp;

    len = strlen(initial_dir);
    if (rs_path) {
      len += strlen(rs_path) + 1;	/* +1 for the colon */
    }
    if (theme_dir) {
      len += strlen(theme_dir) + 1;
    }
    if (user_dir) {
      len += strlen(user_dir) + 1;
    }
    tmp = MALLOC(len + 1);	/* +1 here for the NUL */
    snprintf(tmp, len + 1, "%s%s%s%s%s%s%s", (rs_path ? rs_path : ""), (rs_path ? ":" : ""), initial_dir,
	     (theme_dir ? ":" : ""), (theme_dir ? theme_dir : ""), (user_dir ? ":" : ""), (user_dir ? user_dir : ""));
    tmp[len] = '\0';
    FREE(rs_path);
    rs_path = tmp;
    D_OPTIONS(("New rs_path set to \"%s\"\n", rs_path));
  }
#endif
  get_options(argc, argv);
  D_UTMP(("Saved real uid/gid = [ %d, %d ]  effective uid/gid = [ %d, %d ]\n", my_ruid, my_rgid, my_euid, my_egid));
  D_UTMP(("Now running with real uid/gid = [ %d, %d ]  effective uid/gid = [ %d, %d ]\n", getuid(), getgid(), geteuid(),
	  getegid()));

  post_parse();

#ifdef PREFER_24BIT
  cmap = DefaultColormap(Xdisplay, Xscreen);

  /*
   * If depth is not 24, look for a 24bit visual.
   */
  if (Xdepth != 24) {
    XVisualInfo vinfo;

    if (XMatchVisualInfo(Xdisplay, Xscreen, 24, TrueColor, &vinfo)) {
      Xdepth = 24;
      Xvisual = vinfo.visual;
      cmap = XCreateColormap(Xdisplay, RootWindow(Xdisplay, Xscreen),
			     Xvisual, AllocNone);
    }
  }
#endif

  Create_Windows(argc, argv);
  scr_reset();			/* initialize screen */

  /* Initialize the scrollbar */
  scrollbar_init(szHint.width, szHint.height);
  scrollbar_mapping(Options & Opt_scrollbar);

  /* Initialize the menu subsystem. */
  menu_init();

#if DEBUG >= DEBUG_X
  if (debug_level >= DEBUG_X) {
    XSynchronize(Xdisplay, True);
  }
#endif

#ifdef DISPLAY_IS_IP
  /* Fixup display_name for export over pty to any interested terminal
   * clients via "ESC[7n" (e.g. shells).  Note we use the pure IP number
   * (for the first non-loopback interface) that we get from
   * network_display().  This is more "name-resolution-portable", if you
   * will, and probably allows for faster x-client startup if your name
   * server is beyond a slow link or overloaded at client startup.  Of
   * course that only helps the shell's child processes, not us.
   *
   * Giving out the display_name also affords a potential security hole
   */

  val = display_name = network_display(display_name);
  if (val == NULL)
#endif /* DISPLAY_IS_IP */
    val = XDisplayString(Xdisplay);
  if (display_name == NULL)
    display_name = val;		/* use broken `:0' value */

  i = strlen(val);
  display_string = MALLOC((i + 9));

  sprintf(display_string, "DISPLAY=%s", val);
  sprintf(windowid_string, "WINDOWID=%u", (unsigned int) TermWin.parent);

  /* add entries to the environment:
   * DISPLAY:       X display name
   * WINDOWID:      X windowid of the window
   * COLORTERM:     Terminal supports color
   * COLORTERM_BCE: Terminal supports BCE
   * TERM:          Terminal type for termcap/terminfo
   */
  putenv(display_string);
  putenv(windowid_string);
  if (Xdepth <= 2) {
    putenv("COLORTERM=" COLORTERMENV "-mono");
    putenv("COLORTERM_BCE=" COLORTERMENV "-mono");
    putenv("TERM=" TERMENV);
  } else {
    if (rs_term_name != NULL) {
      i = strlen(rs_term_name);
      term_string = MALLOC((i + 6) * sizeof(char));

      sprintf(term_string, "TERM=%s", rs_term_name);
      putenv(term_string);
    } else {
#ifdef DEFINE_XTERM_COLOR
      if (Xdepth <= 2)
	putenv("TERM=" TERMENV);
      else
	putenv("TERM=" TERMENV "-color");
#else
      putenv("TERM=" TERMENV);
#endif
    }
    putenv("COLORTERM=" COLORTERMENV);
    putenv("COLORTERM_BCE=" COLORTERMENV);
  }
  putenv("ETERM_VERSION=" VERSION);

  D_CMD(("init_command()\n"));
  init_command(rs_execArgs);

  main_loop();

  return (EXIT_SUCCESS);
}
