/*  main.c -- Eterm main() function
 *         -- 22 August 1998, mej
 *
 * This file is original work by Michael Jennings <mej@tcserv.com> and
 * Tuomo Venalainen <vendu@cc.hut.fi>.  This file, and any other file
 * bearing this same message or a similar one, is distributed under
 * the GNU Public License (GPL) as outlined in the COPYING file.
 *
 * Copyright (C) 1997, Michael Jennings and Tuomo Venalainen
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

/* includes */
#include "main.h"
#ifdef USE_ACTIVE_TAGS
# include "activetags.h"
# include "activeeterm.h"
#endif

#include <stdio.h>
#include <stdlib.h>
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

#include "command.h"
#include "feature.h"
#include "../libmej/debug.h"	/* from libmej */
#include "debug.h"
#include "../libmej/mem.h"
#include "../libmej/strings.h"
/* For strsep(). -vendu */
#if defined(linux)
# include <string.h>
#endif
#include "string.h"
#include "graphics.h"
#include "scrollbar.h"
#include "menubar.h"
#include "screen.h"
#include "options.h"
#include "pixmap.h"
#ifdef USE_POSIX_THREADS
# include "threads.h"
#endif

/* Global attributes */
XWindowAttributes attr;
XSetWindowAttributes Attributes;
char *orig_argv0;

#ifdef PIXMAP_SUPPORT
/* Set to one in case there is no WM, or a lousy one
   that doesn't send the right events (*cough*
   Window Maker *cough*) -- mej */
short bg_needs_update = 1;

#endif

/* extern functions referenced */
#ifdef DISPLAY_IS_IP
extern char *network_display(const char *display);

#endif

extern void get_initial_options(int, char **);
extern void menubar_read(const char *filename);

#ifdef USE_POSIX_THREADS
static void **retval;
static int join_value;
static pthread_t main_loop_thr;
static pthread_attr_t main_loop_attr;

# ifdef MUTEX_SYNCH
pthread_mutex_t mutex;

# endif
#endif

#ifdef PIXMAP_SUPPORT
extern void render_pixmap(Window win, imlib_t image, pixmap_t pmap,
			  int which, renderop_t renderop);

# ifdef BACKING_STORE
extern const char *rs_saveUnder;

# endif

extern char *rs_noCursor;

# ifdef USE_IMLIB
extern ImlibData *imlib_id;

# endif
#endif

/* extern variables referenced */
extern int my_ruid, my_rgid, my_euid, my_egid;
extern unsigned int rs_shadePct;
extern unsigned long rs_tintMask;

/* extern variables declared here */
TermWin_t TermWin;
Display *Xdisplay;		/* display */

char *rs_color[NRS_COLORS];
Pixel PixColors[NRS_COLORS + NSHADOWCOLORS];

unsigned long Options = (Opt_scrollBar);
unsigned int debug_level = 0;	/* Level of debugging information to display */

const char *display_name = NULL;
char *rs_name = NULL;		/* client instance (resource name) */

#ifndef NO_BOLDFONT
const char *rs_boldFont = NULL;

#endif
const char *rs_font[NFONTS];

#ifdef KANJI
const char *rs_kfont[NFONTS];

#endif

#ifdef PRINTPIPE
char *rs_print_pipe = NULL;

#endif

char *rs_cutchars = NULL;

/* local variables */
Cursor TermWin_cursor;		/* cursor for vt window */
unsigned int colorfgbg;
menuBar_t menuBar;

XSizeHints szHint =
{
  PMinSize | PResizeInc | PBaseSize | PWinGravity,
  0, 0, 80, 24,			/* x, y, width, height */
  1, 1,				/* Min width, height */
  0, 0,				/* Max width, height - unused */
  1, 1,				/* increments: width, height */
  {1, 1},			/* increments: x, y */
  {0, 0},			/* Aspect ratio - unused */
  0, 0,				/* base size: width, height */
  NorthWestGravity		/* gravity */
};

char *def_colorName[] =
{
  "rgb:0/0/0", "rgb:ff/ff/ff",	/* fg/bg */
  "rgb:0/0/0",			/* 0: black             (#000000) */
#ifndef NO_BRIGHTCOLOR
    /* low-intensity colors */
  "rgb:cc/00/00",		/* 1: red     */
  "rgb:00/cc/00",		/* 2: green   */
  "rgb:cc/cc/00",		/* 3: yellow  */
  "rgb:00/00/cc",		/* 4: blue    */
  "rgb:cc/00/cc",		/* 5: magenta */
  "rgb:00/cc/cc",		/* 6: cyan    */
  "rgb:fa/eb/d7",		/* 7: white   */
    /* high-intensity colors */
  "rgb:33/33/33",		/* 8: bright black */
#endif				/* NO_BRIGHTCOLOR */
  "rgb:ff/00/00",		/* 1/9:  bright red     */
  "rgb:00/ff/00",		/* 2/10: bright green   */
  "rgb:ff/ff/00",		/* 3/11: bright yellow  */
  "rgb:00/00/ff",		/* 4/12: bright blue    */
  "rgb:ff/00/ff",		/* 5/13: bright magenta */
  "rgb:00/ff/ff",		/* 6/14: bright cyan    */
  "rgb:ff/ff/ff",		/* 7/15: bright white   */
#ifndef NO_CURSORCOLOR
  NULL, NULL,			/* cursorColor, cursorColor2 */
#endif				/* NO_CURSORCOLOR */
  NULL, NULL			/* pointerColor, borderColor */
#ifndef NO_BOLDUNDERLINE
  ,NULL, NULL			/* colorBD, colorUL */
#endif				/* NO_BOLDUNDERLINE */
  ,"rgb:ff/ff/ff"		/* menuTextColor */
#ifdef KEEP_SCROLLCOLOR
  ,"rgb:b2/b2/b2"		/* scrollColor: match Netscape color */
# ifdef CHANGE_SCROLLCOLOR_ON_FOCUS
  ,NULL				/* unfocusedscrollColor: somebody chose black? */
# endif
#endif
};

#ifdef KANJI
/* Kanji font names, roman fonts sized to match */
const char *def_kfontName[] =
{
  KFONT0, KFONT1, KFONT2, KFONT3, KFONT4
};

#endif /* KANJI */
const char *def_fontName[] =
{
  FONT0, FONT1, FONT2, FONT3, FONT4
};

/* extern functions referenced */
#ifdef PIXMAP_SUPPORT
/* the originally loaded pixmap and its scaling */
extern pixmap_t bgPixmap;
extern void set_bgPixmap(const char * /* file */ );

# ifdef USE_IMLIB
extern imlib_t imlib_bg;

# endif
# ifdef PIXMAP_SCROLLBAR
extern pixmap_t sbPixmap;
extern pixmap_t upPixmap, up_clkPixmap;
extern pixmap_t dnPixmap, dn_clkPixmap;
extern pixmap_t saPixmap, sa_clkPixmap;

#  ifdef USE_IMLIB
extern imlib_t imlib_sb, imlib_sa, imlib_saclk;

#  endif
# endif
# ifdef PIXMAP_MENUBAR
extern pixmap_t mbPixmap, mb_selPixmap;

#  ifdef USE_IMLIB
extern imlib_t imlib_mb, imlib_ms;

#  endif
# endif

extern int scale_pixmap(const char *geom, pixmap_t * pmap);

#endif /* PIXMAP_SUPPORT */

/* have we changed the font? Needed to avoid race conditions
 * while window resizing  */
int font_change_count = 0;

static void resize(void);

extern XErrorHandler xerror_handler(Display *, XErrorEvent *);
extern void Create_Windows(int, char **);

/* main() */
int
main(int argc, char *argv[])
{

  int i, count;
  char *val;
  static char windowid_string[20], *display_string, *term_string;	/* "WINDOWID=\0" = 10 chars, UINT_MAX = 10 chars */

  orig_argv0 = argv[0];

#ifdef USE_POSIX_THREADS
# ifdef MUTEX_SYNCH
  pthread_atfork((void *) &prepare, (void *) &parent, (void *) &child);
# endif
#endif

  /* Security enhancements -- mej */
  my_ruid = getuid();
  my_euid = geteuid();
  my_rgid = getgid();
  my_egid = getegid();
  privileges(REVERT);

  TermWin.wm_parent = None;
  init_defaults();

  /* Open display, get options/resources and create the window */
  if ((display_name = getenv("DISPLAY")) == NULL)
    display_name = ":0";

  /* This MUST be called before any other Xlib functions */

#ifdef USE_POSIX_THREADS
  if (XInitThreads()) {
    D_THREADS(("XInitThreads() succesful\n"));
  } else {
    D_THREADS(("XInitThreads() failed, I'm outta here\n"));
  }
#endif

#ifdef USE_THEMES
  get_initial_options(argc, argv);
#endif
  read_config();
#ifdef PIXMAP_SUPPORT
  if (rs_path) {
    rs_path = REALLOC(rs_path, strlen(rs_path) + strlen(initial_dir) + 2);
    strcat(rs_path, ":");
    strcat(rs_path, initial_dir);
  }
#endif
  get_options(argc, argv);
#ifdef USE_ACTIVE_TAGS
  tag_init();
#endif
  D_UTMP(("Saved real uid/gid = [ %d, %d ]  effective uid/gid = [ %d, %d ]\n", my_ruid, my_rgid, my_euid, my_egid));
  D_UTMP(("Now running with real uid/gid = [ %d, %d ]  effective uid/gid = [ %d, %d ]\n", getuid(), getgid(), geteuid(),
	  getegid()));

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

  /* Since we always use Imlib now, let's initialize it here. */
  imlib_id = Imlib_init(Xdisplay);

  post_parse();

#ifdef PREFER_24BIT
  Xdepth = DefaultDepth(Xdisplay, Xscreen);
  Xcmap = DefaultColormap(Xdisplay, Xscreen);
  Xvisual = DefaultVisual(Xdisplay, Xscreen);

  /*
   * If depth is not 24, look for a 24bit visual.
   */
  if (Xdepth != 24) {
    XVisualInfo vinfo;

    if (XMatchVisualInfo(Xdisplay, Xscreen, 24, TrueColor, &vinfo)) {
      Xdepth = 24;
      Xvisual = vinfo.visual;
      Xcmap = XCreateColormap(Xdisplay, RootWindow(Xdisplay, Xscreen),
			      Xvisual, AllocNone);
    }
  }
#endif

  Create_Windows(argc, argv);
  scr_reset();			/* initialize screen */
  Gr_reset();			/* reset graphics */

  /* add scrollBar, do it directly to avoid resize() */
  scrollbar_mapping(Options & Opt_scrollBar);
  /* we can now add menuBar */
  if (delay_menu_drawing) {
    delay_menu_drawing = 0;
    menubar_mapping(1);
  } else if (rs_menubar == *false_vals) {
    menubar_mapping(0);
  }
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
   * @ DISPLAY:   in case we started with -display
   * @ WINDOWID:  X window id number of the window
   * @ COLORTERM: terminal sub-name and also indicates its color
   * @ TERM:      terminal name
   */
  putenv(display_string);
  putenv(windowid_string);
  if (Xdepth <= 2) {
    putenv("COLORTERM=" COLORTERMENV "-mono");
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
#ifdef PIXMAP_SUPPORT
    putenv("COLORTERM=" COLORTERMENV "-pixmap");
#else
    putenv("COLORTERM=" COLORTERMENV);
#endif
  }

  D_CMD(("init_command()\n"));
  init_command(rs_execArgs);
#ifndef USE_POSIX_THREADS
  if (Options & Opt_borderless) {
    resize_window();
  }
#endif

#ifdef USE_POSIX_THREADS
  D_THREADS(("main_thread:"));
  pthread_attr_init(&main_loop_attr);
  pthread_create(&main_loop_thr, &main_loop_attr,
		 (void *) &main_thread, NULL);
  D_THREADS(("done? :)\n"));
  while (1);
  /*  main_loop(); */
#else
  main_loop();			/* main processing loop */
#endif

  return (EXIT_SUCCESS);
}
/* EOF */
