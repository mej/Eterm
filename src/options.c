/*  options.c -- Eterm options module
 *            -- 25 July 1997, mej
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

#include "main.h"
#include "feature.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>
#include <X11/keysym.h>
#include "../libmej/debug.h"
#include "../libmej/strings.h"
#include "debug.h"
#include "mem.h"
#include "strings.h"
#include "command.h"
#include "grkelot.h"
#include "menubar.h"
#include "options.h"
#include "pixmap.h"
#include "scrollbar.h"
#include "screen.h"
#include "system.h"

#if PATH_MAX < 1024
#  undef PATH_MAX
#  define PATH_MAX 1024
#endif

const char *true_vals[] =
{"1", "on", "true", "yes"};
const char *false_vals[] =
{"0", "off", "false", "no"};

char **rs_execArgs = NULL;	/* Args to exec (-e or --exec) */
char *rs_title = NULL;		/* Window title */
char *rs_iconName = NULL;	/* Icon name */
char *rs_geometry = NULL;	/* Geometry string */
int rs_desktop = -1;
char *rs_path = NULL;
int rs_saveLines = SAVELINES;	/* Lines in the scrollback buffer */

#ifdef KEYSYM_ATTRIBUTE
unsigned char *KeySym_map[256];	/* probably mostly empty */

#endif
#if defined (HOTKEY_CTRL) || defined (HOTKEY_META)
/* recognized when combined with HOTKEY */
KeySym ks_bigfont = XK_greater;
KeySym ks_smallfont = XK_less;

#endif
#ifdef PIXMAP_SUPPORT
char *rs_pixmapScale = NULL;
const char *rs_saveUnder = NULL;
char *rs_icon = NULL;

# ifdef BACKGROUND_CYCLING_SUPPORT
char *rs_anim_pixmap_list = NULL;
char **rs_anim_pixmaps = NULL;
time_t rs_anim_delay = 0;

# endif
# ifdef PIXMAP_OFFSET
char *rs_viewport_mode = NULL;
const char *rs_pixmapTrans = NULL;
unsigned long rs_tintMask = 0xffffff;
unsigned int rs_shadePct = 0;

#  ifdef WATCH_DESKTOP_OPTION
const char *rs_watchDesktop = NULL;

#  endif
# endif
char *rs_pixmaps[11] =
{NULL, NULL, NULL, NULL, NULL, NULL, NULL,
 NULL, NULL, NULL, NULL};

#endif
char *rs_noCursor = NULL;

#ifdef USE_THEMES
char *rs_theme = NULL;
char *rs_config_file = NULL;

#endif

/* local functions referenced */
char *chomp(char *);
void parse_main(char *);
void parse_color(char *);
void parse_attributes(char *);
void parse_toggles(char *);
void parse_keyboard(char *);
void parse_misc(char *);
void parse_pixmaps(char *);
void parse_kanji(char *);
void parse_undef(char *);
char *builtin_random(char *);
char *builtin_exec(char *);
char *builtin_version(char *);
char *builtin_appname(char *);

/* local variables */
const char *rs_loginShell = NULL;
const char *rs_utmpLogging = NULL;
const char *rs_scrollBar = NULL;

#if (MENUBAR_MAX)
const char *rs_menubar = NULL;

#endif
const char *rs_app_keypad = NULL;
const char *rs_app_cursor = NULL;
const char *rs_homeOnEcho = NULL;
const char *rs_homeOnInput = NULL;
const char *rs_homeOnRefresh = NULL;
const char *rs_scrollBar_right = NULL;
const char *rs_scrollBar_floating = NULL;
const char *rs_scrollbar_popup = NULL;
const char *rs_borderless = NULL;
const char *rs_pause = NULL;
const char *rs_xterm_select = NULL;
const char *rs_select_whole_line = NULL;
const char *rs_select_trailing_spaces = NULL;
unsigned short rs_min_anchor_size = 0;

char *rs_scrollbar_type = NULL;
unsigned long rs_scrollbar_width = 0;
const char *rs_term_name = NULL;

#if MENUBAR_MAX
const char *rs_menubar_move = NULL;
const char *rs_menu = NULL;

#endif

#if defined (HOTKEY_CTRL) || defined (HOTKEY_META)
static char *rs_bigfont_key = NULL;
static char *rs_smallfont_key = NULL;

#endif

#ifndef NO_MAPALERT
# ifdef MAPALERT_OPTION
static const char *rs_mapAlert = NULL;

# endif
#endif
static const char *rs_visualBell = NULL;
static const char *rs_reverseVideo = NULL;

#ifdef META8_OPTION
static const char *rs_meta8 = NULL;

#endif
#ifdef KANJI
static char *rs_kanji_encoding = NULL;

#endif
#ifdef GREEK_SUPPORT
static char *rs_greek_keyboard = NULL;

#endif

extern char initial_dir[PATH_MAX + 1];

/* Options structure */

#define OPT_BOOLEAN          0x0001
#define OPT_INTEGER          0x0002
#define OPT_STRING           0x0004
#define OPT_ARGUMENT         0x0008

#define OPT_STR(s, l, d, p)         { s, l, "(str)  " d, OPT_STRING,   (const char **)  p, 0, 0 }
#define OPT_INT(s, l, d, p)         { s, l, "(int)  " d, OPT_INTEGER,  (const int *)    p, 0, 0 }
#define OPT_BOOL(s, l, d, p, v, m)  { s, l, "(bool) " d, OPT_BOOLEAN,  (const int *)    p, v, m }
#define OPT_LONG(l, d, p)           { 0, l, "(str)  " d, OPT_STRING,   (const char **)  p, 0, 0 }
#define OPT_ARGS(s, l, d, p)        { s, l, "(str)  " d, OPT_ARGUMENT, (const char ***) p, 0, 0 }
#define OPT_BLONG(l, d, p, v, m)    { 0, l, "(bool) " d, OPT_BOOLEAN,  (const int *)    p, v, m }
#define OPT_ILONG(l, d, p)          { 0, l, "(int)  " d, OPT_INTEGER,  (const int *)    p, 0, 0 }
#define optList_numoptions()    (sizeof(optList)/sizeof(optList[0]))

static const struct {
  char short_opt;
  char *long_opt;
  const char *const description;
  unsigned short flag;
  const void *pval;
  unsigned long *maskvar;
  int mask;
} optList[] = {

#ifdef USE_THEMES		/* These two MUST be first! */
  OPT_STR('t', "theme", "select a theme", &rs_theme),
      OPT_STR('X', "config-file", "choose an alternate config file", &rs_config_file),
#endif /* USE_THEMES */
      OPT_BOOL('h', "help", "display usage information", NULL, NULL, 0),
      OPT_BLONG("version", "display version and configuration information", NULL, NULL, 0),
#if DEBUG <= 0
      OPT_ILONG("debug", "level of debugging information to show (support not compiled in)", &debug_level),
#elif DEBUG == 1
      OPT_ILONG("debug", "level of debugging information to show (0-1)", &debug_level),
#elif DEBUG == 2
      OPT_ILONG("debug", "level of debugging information to show (0-2)", &debug_level),
#elif DEBUG == 3
      OPT_ILONG("debug", "level of debugging information to show (0-3)", &debug_level),
#elif DEBUG == 4
      OPT_ILONG("debug", "level of debugging information to show (0-4)", &debug_level),
#else
      OPT_ILONG("debug", "level of debugging information to show (0-5)", &debug_level),
#endif

/* =======[ Color options ]======= */
      OPT_BOOL('r', "reverse-video", "reverse video", &rs_reverseVideo, &Options, Opt_reverseVideo),
      OPT_STR('b', "background-color", "background color", &rs_color[bgColor]),
      OPT_STR('f', "foreground-color", "foreground color", &rs_color[fgColor]),
      OPT_LONG("color0", "color 0", &rs_color[minColor]),
      OPT_LONG("color1", "color 1", &rs_color[minColor + 1]),
      OPT_LONG("color2", "color 2", &rs_color[minColor + 2]),
      OPT_LONG("color3", "color 3", &rs_color[minColor + 3]),
      OPT_LONG("color4", "color 4", &rs_color[minColor + 4]),
      OPT_LONG("color5", "color 5", &rs_color[minColor + 5]),
      OPT_LONG("color6", "color 6", &rs_color[minColor + 6]),
      OPT_LONG("color7", "color 7", &rs_color[minColor + 7]),
#ifndef NO_BRIGHTCOLOR
      OPT_LONG("color8", "color 8", &rs_color[minBright]),
      OPT_LONG("color9", "color 9", &rs_color[minBright + 1]),
      OPT_LONG("color10", "color 10", &rs_color[minBright + 2]),
      OPT_LONG("color11", "color 11", &rs_color[minBright + 3]),
      OPT_LONG("color12", "color 12", &rs_color[minBright + 4]),
      OPT_LONG("color13", "color 13", &rs_color[minBright + 5]),
      OPT_LONG("color14", "color 14", &rs_color[minBright + 6]),
      OPT_LONG("color15", "color 15", &rs_color[minBright + 7]),
#endif /* NO_BRIGHTCOLOR */
#ifndef NO_BOLDUNDERLINE
      OPT_LONG("colorBD", "bold color", &rs_color[colorBD]),
      OPT_LONG("colorUL", "underline color", &rs_color[colorUL]),
#endif /* NO_BOLDUNDERLINE */
      OPT_LONG("menu-text-color", "menu text color", &rs_color[menuTextColor]),
#ifdef KEEP_SCROLLCOLOR
      OPT_STR('S', "scrollbar-color", "scrollbar color", &rs_color[scrollColor]),
# ifdef CHANGE_SCROLLCOLOR_ON_FOCUS
      OPT_LONG("unfocused-scrollbar-color", "unfocused scrollbar color", &rs_color[unfocusedScrollColor]),
# endif
#endif /* KEEP_SCROLLCOLOR */
      OPT_LONG("pointer-color", "mouse pointer color", &rs_color[pointerColor]),
#ifndef NO_CURSORCOLOR
      OPT_STR('c', "cursor-color", "cursor color", &rs_color[cursorColor]),
      OPT_LONG("cursor-text-color", "cursor text color", &rs_color[cursorColor2]),
#endif /* NO_CURSORCOLOR */

/* =======[ X11 options ]======= */
      OPT_STR('d', "display", "X server to connect to", &display_name),
      OPT_STR('g', "geometry", "WxH+X+Y = size and position", &rs_geometry),
      OPT_BOOL('i', "iconic", "start iconified", NULL, &Options, Opt_iconic),
      OPT_STR('n', "name", "client instance, icon, and title strings", &rs_name),
      OPT_STR('T', "title", "title string", &rs_title),
      OPT_LONG("icon-name", "icon name", &rs_iconName),
      OPT_STR('B', "scrollbar-type", "choose the scrollbar type (motif, next, xterm)", &rs_scrollbar_type),
      OPT_ILONG("scrollbar-width", "choose the width (in pixels) of the scrollbar", &rs_scrollbar_width),
      OPT_INT('D', "desktop", "desktop to start on (requires GNOME-compliant window manager)", &rs_desktop),
#ifndef NO_BOLDFONT
      OPT_LONG("bold-font", "bold text font", &rs_boldFont),
#endif
      OPT_STR('F', "font", "normal text font", &rs_font[0]),
      OPT_LONG("font1", "font 1", &rs_font[1]),
      OPT_LONG("font2", "font 2", &rs_font[2]),
      OPT_LONG("font3", "font 3", &rs_font[3]),
      OPT_LONG("font4", "font 4", &rs_font[4]),

/* =======[ Pixmap options ]======= */
#ifdef PIXMAP_SUPPORT
      OPT_STR('P', "background-pixmap", "background pixmap [scaling optional]", &rs_pixmaps[pixmap_bg]),
      OPT_STR('I', "icon", "icon pixmap", &rs_icon),
      OPT_LONG("up-arrow-pixmap", "up arrow pixmap [scaling optional]", &rs_pixmaps[pixmap_up]),
      OPT_LONG("down-arrow-pixmap", "down arrow pixmap [scaling optional]", &rs_pixmaps[pixmap_dn]),
      OPT_LONG("trough-pixmap", "scrollbar background (trough) pixmap [scaling optional]", &rs_pixmaps[pixmap_sb]),
      OPT_LONG("anchor-pixmap", "scrollbar anchor pixmap [scaling optional]", &rs_pixmaps[pixmap_sa]),
      OPT_BOOL('@', "scale", "scale rather than tile", &rs_pixmapScale, &Options, Opt_pixmapScale),
# ifdef PIXMAP_OFFSET
#  ifdef WATCH_DESKTOP_OPTION
      OPT_BOOL('W', "watch-desktop", "watch the desktop background image for changes", &rs_watchDesktop, &Options, Opt_watchDesktop),
#  endif
      OPT_BOOL('O', "trans", "creates a pseudo-transparent Eterm", &rs_pixmapTrans, &Options, Opt_pixmapTrans),
      OPT_ILONG("shade", "percentage to shade the background in a pseudo-transparent Eterm", &rs_shadePct),
      OPT_ILONG("tint", "RGB brightness mask for color tinting in a pseudo-transparent Eterm", &rs_tintMask),
# endif
      OPT_STR('p', "path", "pixmap file search path", &rs_path),
# ifdef BACKGROUND_CYCLING_SUPPORT
      OPT_STR('N', "anim", "a delay and list of pixmaps for cycling", &rs_anim_pixmap_list),
# endif				/* BACKGROUND_CYCLING_SUPPORT */
#endif /* PIXMAP_SUPPORT */

/* =======[ Kanji options ]======= */
#ifdef KANJI
      OPT_STR('K', "kanji-font", "normal text kanji font", &rs_kfont[0]),
      OPT_LONG("kanji-font1", "kanji font 1", &rs_kfont[1]),
      OPT_LONG("kanji-font2", "kanji font 2", &rs_kfont[2]),
      OPT_LONG("kanji-font3", "kanji font 3", &rs_kfont[3]),
      OPT_LONG("kanji-font4", "kanji font 4", &rs_kfont[4]),
      OPT_LONG("kanji-encoding", "kanji encoding mode (eucj or sjis)", &rs_kanji_encoding),
#endif /* KANJI */

/* =======[ Toggles ]======= */
      OPT_BOOL('l', "login-shell", "login shell, prepend - to shell name", &rs_loginShell, &Options, Opt_loginShell),
      OPT_BOOL('s', "scrollbar", "display scrollbar", &rs_scrollBar, &Options, Opt_scrollBar),
      OPT_BLONG("menubar", "display menubar", &rs_menubar, NULL, 0),
      OPT_BOOL('u', "utmp-logging", "make a utmp entry", &rs_utmpLogging, &Options, Opt_utmpLogging),
      OPT_BOOL('v', "visual-bell", "visual bell", &rs_visualBell, &Options, Opt_visualBell),
      OPT_BOOL('H', "home-on-echo", "jump to bottom on output", &rs_homeOnEcho, &Options, Opt_homeOnEcho),
      OPT_BLONG("home-on-input", "jump to bottom on input", &rs_homeOnInput, &Options, Opt_homeOnInput),
      OPT_BOOL('E', "home-on-refresh", "jump to bottom on refresh (^L)", &rs_homeOnRefresh, &Options, Opt_homeOnRefresh),
      OPT_BLONG("scrollbar-right", "display the scrollbar on the right", &rs_scrollBar_right, &Options, Opt_scrollBar_right),
      OPT_BLONG("scrollbar-floating", "display the scrollbar with no trough", &rs_scrollBar_floating, &Options, Opt_scrollBar_floating),
      OPT_BLONG("scrollbar-popup", "popup the scrollbar only when focused", &rs_scrollbar_popup, &Options, Opt_scrollbar_popup),
      OPT_BOOL('x', "borderless", "force Eterm to have no borders", &rs_borderless, &Options, Opt_borderless),
#ifndef NO_MAPALERT
# ifdef MAPALERT_OPTION
      OPT_BOOL('m', "map-alert", "uniconify on beep", &rs_mapAlert, &Options, Opt_mapAlert),
# endif
#endif
#ifdef META8_OPTION
      OPT_BOOL('8', "meta-8", "Meta key toggles 8-bit", &rs_meta8, &Options, Opt_meta8),
#endif
#ifdef BACKING_STORE
      OPT_BLONG("save-under", "use backing store", &rs_saveUnder, &Options, Opt_saveUnder),
#endif
      OPT_BLONG("no-cursor", "disable the text cursor", &rs_noCursor, &Options, Opt_noCursor),
#if MENUBAR_MAX
      OPT_BOOL('V', "menubar-move", "dragging the menubar will move the window", &rs_menubar_move, &Options, Opt_menubar_move),
#endif
      OPT_BLONG("pause", "pause for a keypress after the child process exits", &rs_pause, &Options, Opt_pause),
      OPT_BLONG("xterm-select", "duplicate xterm's broken selection behavior", &rs_xterm_select, &Options, Opt_xterm_select),
      OPT_BLONG("select-line", "triple-click selects whole line", &rs_select_whole_line, &Options, Opt_select_whole_line),
      OPT_BLONG("select-trailing-spaces", "do not skip trailing spaces when selecting", &rs_select_trailing_spaces, &Options, Opt_select_trailing_spaces),
#ifdef PIXMAP_OFFSET
      OPT_BLONG("viewport-mode", "use viewport mode for the background image", &rs_viewport_mode, &Options, Opt_viewport_mode),
#endif

/* =======[ Keyboard options ]======= */
#if defined (HOTKEY_CTRL) || defined (HOTKEY_META)
      OPT_LONG("big-font-key", "keysym for font size increase", &rs_bigfont_key),
      OPT_LONG("small-font-key", "keysym for font size decrease", &rs_smallfont_key),
#endif
#ifdef GREEK_SUPPORT
      OPT_LONG("greek-keyboard", "greek keyboard mapping (iso or ibm)", &rs_greek_keyboard),
#endif
      OPT_BLONG("app-keypad", "application keypad mode", &rs_app_keypad, &PrivateModes, PrivMode_aplKP),
      OPT_BLONG("app-cursor", "application cursor key mode", &rs_app_cursor, &PrivateModes, PrivMode_aplCUR),

/* =======[ Misc options ]======= */
      OPT_INT('L', "save-lines", "lines to save in scrollback buffer", &rs_saveLines),
      OPT_INT('a', "min-anchor-size", "minimum size of the scrollbar anchor", &rs_min_anchor_size),
#ifdef BORDER_WIDTH_OPTION
      OPT_INT('w', "border-width", "term window border width", &(TermWin.internalBorder)),
#endif
#ifdef PRINTPIPE
      OPT_LONG("print-pipe", "print command", &rs_print_pipe),
#endif
#ifdef CUTCHAR_OPTION
      OPT_LONG("cut-chars", "seperators for double-click selection", &rs_cutchars),
#endif /* CUTCHAR_OPTION */
#if MENUBAR_MAX
      OPT_STR('M', "menu", "Default menubar file", &rs_menu),
#endif
      OPT_LONG("term-name", "value to use for setting $TERM", &rs_term_name),
      OPT_BOOL('C', "console", "grab console messages", NULL, &Options, Opt_console),
      OPT_ARGS('e', "exec", "execute a command rather than a shell", &rs_execArgs)
};

/* Print usage information */
#define INDENT "5"
static void
usage(void)
{

  int i, col;

  printf("Eterm Enlightened Terminal Emulator for X Windows\n");
  printf("Copyright (c) 1997-1999, Tuomo Venalainen and Michael Jennings\n\n");
  printf("Usage for " APL_NAME " " VERSION ":\n\n");
  printf("%7s %17s %40s\n", "POSIX", "GNU", "Description");
  printf("%8s %10s %41s\n", "=======", "===============================",
	 "=========================================");
  for (i = 0; i < optList_numoptions(); i++) {
    printf("%" INDENT "s", " ");
    if (optList[i].short_opt) {
      printf("-%c, ", optList[i].short_opt);
    } else {
      printf("    ");
    }
    printf("--%s", optList[i].long_opt);
    for (col = strlen(optList[i].long_opt); col < 30; col++) {
      printf(" ");
    }
    printf("%s\n", optList[i].description);
  }
  printf("\nOption types:\n");
  printf("  (bool) -- Boolean option ('1', 'on', 'yes', or 'true' to activate, '0', 'off', 'no', or 'false' to deactivate)\n");
  printf("  (int)  -- Integer option (any signed number of reasonable value, usually in decimal/octal/hex)\n");
  printf("  (str)  -- String option (be sure to quote strings if needed to avoid shell expansion)\n\n");

  printf("NOTE:  Long options can be separated from their values by an equal sign ('='), or you can\n");
  printf("       pass the value as the following argument on the command line (e.g., '--scrollbar 0'\n");
  printf("       or '--scrollbar=0').  Short options must have their values passed after them on the\n");
  printf("       command line, and in the case of boolean short options, cannot have values (they\n");
  printf("       default to true) (e.g., '-F shine' or '-s').\n");

  printf("\nPlease consult the Eterm(1) man page for more detailed\n");
  printf("information on command line options.\n\n");
  exit(EXIT_FAILURE);
}

/* Print version and configuration information */
static void
version(void)
{

  int i, col;

  printf("Eterm " VERSION "\n");
  printf("Copyright (c) 1997-1999, Tuomo Venalainen and Michael Jennings\n\n");

  printf("Debugging configuration:  ");
#ifdef DEBUG
  printf("DEBUG=%d", DEBUG);
#else
  printf("-DEBUG");
#endif

#if DEBUG >= DEBUG_SCREEN
  printf(" +DEBUG_SCREEN");
#endif
#if DEBUG >= DEBUG_CMD
  printf(" +DEBUG_CMD");
#endif
#if DEBUG >= DEBUG_TTY
  printf(" +DEBUG_TTY");
#endif
#if DEBUG >= DEBUG_SELECTION
  printf(" +DEBUG_SELECTION");
#endif
#if DEBUG >= DEBUG_UTMP
  printf(" +DEBUG_UTMP");
#endif
#if DEBUG >= DEBUG_OPTIONS
  printf(" +DEBUG_OPTIONS");
#endif
#if DEBUG >= DEBUG_IMLIB
  printf(" +DEBUG_IMLIB");
#endif
#if DEBUG >= DEBUG_PIXMAP
  printf(" +DEBUG_PIXMAP");
#endif
#if DEBUG >= DEBUG_EVENTS
  printf(" +DEBUG_EVENTS");
#endif
#if DEBUG >= DEBUG_MALLOC
  printf(" +DEBUG_MALLOC");
#endif
#if DEBUG >= DEBUG_X11
  printf(" +DEBUG_X11");
#endif
#if DEBUG >= DEBUG_SCROLLBAR
  printf(" +DEBUG_SCROLLBAR");
#endif
#if DEBUG >= DEBUG_THREADS
  printf(" +DEBUG_THREADS");
#endif
#if DEBUG >= DEBUG_TAGS
  printf(" +DEBUG_TAGS");
#endif
#if DEBUG >= DEBUG_MENU
  printf(" +DEBUG_MENU");
#endif
#if DEBUG >= DEBUG_TTYMODE
  printf(" +DEBUG_TTYMODE");
#endif
#if DEBUG >= DEBUG_COLORS
  printf(" +DEBUG_COLORS");
#endif
#if DEBUG >= DEBUG_MENUARROWS
  printf(" +DEBUG_MENUARROWS");
#endif
#if DEBUG >= DEBUG_MENU_LAYOUT
  printf(" +DEBUG_MENU_LAYOUT");
#endif
#if DEBUG >= DEBUG_MENUBAR_STACKING
  printf(" +DEBUG_MENUBAR_STACKING");
#endif
#if DEBUG >= DEBUG_X
  printf(" +DEBUG_X");
#endif

  printf("\n\nCompile-time toggles: ");

#ifdef PROFILE
  printf(" +PROFILE");
#else
  printf(" -PROFILE");
#endif
#ifdef PROFILE_SCREEN
  printf(" +PROFILE_SCREEN");
#else
  printf(" -PROFILE_SCREEN");
#endif
#ifdef PROFILE_X_EVENTS
  printf(" +PROFILE_X_EVENTS");
#else
  printf(" -PROFILE_X_EVENTS");
#endif
#ifdef COUNT_X_EVENTS
  printf(" +COUNT_X_EVENTS");
#else
  printf(" -COUNT_X_EVENTS");
#endif
#ifdef USE_ACTIVE_TAGS
  printf(" +USE_ACTIVE_TAGS");
#else
  printf(" -USE_ACTIVE_TAGS");
#endif
#ifdef OPTIMIZE_HACKS
  printf(" +OPTIMIZE_HACKS");
#else
  printf(" -OPTIMIZE_HACKS");
#endif
#ifdef PIXMAP_SUPPORT
  printf(" +PIXMAP_SUPPORT");
#else
  printf(" -PIXMAP_SUPPORT");
#endif
#ifdef USE_POSIX_THREADS
  printf(" +USE_POSIX_THREADS");
#else
  printf(" -USE_POSIX_THREADS");
#endif
#ifdef MUTEX_SYNCH
  printf(" +MUTEX_SYNCH");
#else
  printf(" -MUTEX_SYNCH");
#endif
#ifdef PIXMAP_OFFSET
  printf(" +PIXMAP_OFFSET");
#else
  printf(" -PIXMAP_OFFSET");
#endif
#ifdef IMLIB_TRANS
  printf(" +IMLIB_TRANS");
#else
  printf(" -IMLIB_TRANS");
#endif
#ifdef BACKGROUND_CYCLING_SUPPORT
  printf(" +BACKGROUND_CYCLING_SUPPORT");
#else
  printf(" -BACKGROUND_CYCLING_SUPPORT");
#endif
#ifdef PIXMAP_SCROLLBAR
  printf(" +PIXMAP_SCROLLBAR");
#else
  printf(" -PIXMAP_SCROLLBAR");
#endif
#ifdef PIXMAP_MENUBAR
  printf(" +PIXMAP_MENUBAR");
#else
  printf(" -PIXMAP_MENUBAR");
#endif
#ifdef BACKING_STORE
  printf(" +BACKING_STORE");
#else
  printf(" -BACKING_STORE");
#endif
#ifdef USE_IMLIB
#  ifdef OLD_IMLIB
  printf(" +USE_IMLIB (OLD_IMLIB)");
#  elif defined(NEW_IMLIB)
  printf(" +USE_IMLIB (NEW_IMLIB)");
#  endif
#else
  printf(" -USE_IMLIB");
#endif
#ifdef USE_THEMES
  printf(" +USE_THEMES");
#else
  printf(" -USE_THEMES");
#endif
#ifdef USE_EFFECTS
  printf(" +USE_EFFECTS");
#else
  printf(" -USE_EFFECTS");
#endif
#ifdef WATCH_DESKTOP_OPTION
  printf(" +WATCH_DESKTOP_OPTION");
#else
  printf(" -WATCH_DESKTOP_OPTION");
#endif
#ifdef NO_CURSORCOLOR
  printf(" +NO_CURSORCOLOR");
#else
  printf(" -NO_CURSORCOLOR");
#endif
#ifdef NO_BRIGHTCOLOR
  printf(" +NO_BRIGHTCOLOR");
#else
  printf(" -NO_BRIGHTCOLOR");
#endif
#ifdef NO_BOLDUNDERLINE
  printf(" +NO_BOLDUNDERLINE");
#else
  printf(" -NO_BOLDUNDERLINE");
#endif
#ifdef NO_BOLDOVERSTRIKE
  printf(" +NO_BOLDOVERSTRIKE");
#else
  printf(" -NO_BOLDOVERSTRIKE");
#endif
#ifdef NO_BOLDFONT
  printf(" +NO_BOLDFONT");
#else
  printf(" -NO_BOLDFONT");
#endif
#ifdef NO_SECONDARY_SCREEN
  printf(" +NO_SECONDARY_SCREEN");
#else
  printf(" -NO_SECONDARY_SCREEN");
#endif
#ifdef FORCE_CLEAR_CHARS
  printf(" +FORCE_CLEAR_CHARS");
#else
  printf(" -FORCE_CLEAR_CHARS");
#endif
#ifdef RXVT_GRAPHICS
  printf(" +RXVT_GRAPHICS");
#else
  printf(" -RXVT_GRAPHICS");
#endif
#ifdef PREFER_24BIT
  printf(" +PREFER_24BIT");
#else
  printf(" -PREFER_24BIT");
#endif
#ifdef OFFIX_DND
  printf(" +OFFIX_DND");
#else
  printf(" -OFFIX_DND");
#endif
#ifdef BORDER_WIDTH_OPTION
  printf(" +BORDER_WIDTH_OPTION");
#else
  printf(" -BORDER_WIDTH_OPTION");
#endif
#ifdef NO_DELETE_KEY
  printf(" +NO_DELETE_KEY");
#else
  printf(" -NO_DELETE_KEY");
#endif
#ifdef FORCE_BACKSPACE
  printf(" +FORCE_BACKSPACE");
#else
  printf(" -FORCE_BACKSPACE");
#endif
#ifdef FORCE_DELETE
  printf(" +FORCE_DELETE");
#else
  printf(" -FORCE_DELETE");
#endif
#ifdef HOTKEY_CTRL
  printf(" +HOTKEY_CTRL");
#else
  printf(" -HOTKEY_CTRL");
#endif
#ifdef HOTKEY_META
  printf(" +HOTKEY_META");
#else
  printf(" -HOTKEY_META");
#endif
#ifdef LINUX_KEYS
  printf(" +LINUX_KEYS");
#else
  printf(" -LINUX_KEYS");
#endif
#ifdef KEYSYM_ATTRIBUTE
  printf(" +KEYSYM_ATTRIBUTE");
#else
  printf(" -KEYSYM_ATTRIBUTE");
#endif
#ifdef NO_XLOCALE
  printf(" +NO_XLOCALE");
#else
  printf(" -NO_XLOCALE");
#endif
#ifdef UNSHIFTED_SCROLLKEYS
  printf(" +UNSHIFTED_SCROLLKEYS");
#else
  printf(" -UNSHIFTED_SCROLLKEYS");
#endif
#ifdef NO_SCROLLBAR_REPORT
  printf(" +NO_SCROLLBAR_REPORT");
#else
  printf(" -NO_SCROLLBAR_REPORT");
#endif
#ifdef CUTCHAR_OPTION
  printf(" +CUTCHAR_OPTION");
#else
  printf(" -CUTCHAR_OPTION");
#endif
#ifdef MOUSE_REPORT_DOUBLECLICK
  printf(" +MOUSE_REPORT_DOUBLECLICK");
#else
  printf(" -MOUSE_REPORT_DOUBLECLICK");
#endif
#ifdef XTERM_SCROLLBAR
  printf(" +XTERM_SCROLLBAR");
#else
  printf(" -XTERM_SCROLLBAR");
#endif
#ifdef MOTIF_SCROLLBAR
  printf(" +MOTIF_SCROLLBAR");
#else
  printf(" -MOTIF_SCROLLBAR");
#endif
#ifdef NEXT_SCROLLBAR
  printf(" +NEXT_SCROLLBAR");
#else
  printf(" -NEXT_SCROLLBAR");
#endif
#ifdef KEEP_SCROLLCOLOR
  printf(" +KEEP_SCROLLCOLOR");
#else
  printf(" -KEEP_SCROLLCOLOR");
#endif
#ifdef CHANGE_SCROLLCOLOR_ON_FOCUS
  printf(" +CHANGE_SCROLLCOLOR_ON_FOCUS");
#else
  printf(" -CHANGE_SCROLLCOLOR_ON_FOCUS");
#endif
#ifdef SCROLLBAR_BUTTON_CONTINUAL_SCROLLING
  printf(" +SCROLLBAR_BUTTON_CONTINUAL_SCROLLING");
#else
  printf(" -SCROLLBAR_BUTTON_CONTINUAL_SCROLLING");
#endif
#ifdef USE_SMOOTH_REFRESH
  printf(" +USE_SMOOTH_REFRESH");
#else
  printf(" -USE_SMOOTH_REFRESH");
#endif
#ifdef MENUBAR_SHADOW_IN
  printf(" +MENUBAR_SHADOW_IN");
#else
  printf(" -MENUBAR_SHADOW_IN");
#endif
#ifdef MENU_SHADOW_IN
  printf(" +MENU_SHADOW_IN");
#else
  printf(" -MENU_SHADOW_IN");
#endif
#ifdef MENU_TEXT_FLOATING
  printf(" +MENU_TEXT_FLOATING");
#else
  printf(" -MENU_TEXT_FLOATING");
#endif
#ifdef CTRL_CLICK_RAISE
  printf(" +CTRL_CLICK_RAISE");
#else
  printf(" -CTRL_CLICK_RAISE");
#endif
#ifdef META8_OPTION
  printf(" +META8_OPTION");
#else
  printf(" -META8_OPTION");
#endif
#ifdef GREEK_SUPPORT
  printf(" +GREEK_SUPPORT");
#else
  printf(" -GREEK_SUPPORT");
#endif
#ifdef KANJI
  printf(" +KANJI");
#else
  printf(" -KANJI");
#endif
#ifdef DISPLAY_IS_IP
  printf(" +DISPLAY_IS_IP");
#else
  printf(" -DISPLAY_IS_IP");
#endif
#ifdef ENABLE_DISPLAY_ANSWER
  printf(" +ENABLE_DISPLAY_ANSWER");
#else
  printf(" -ENABLE_DISPLAY_ANSWER");
#endif
#ifdef NO_VT100_ANS
  printf(" +NO_VT100_ANS");
#else
  printf(" -NO_VT100_ANS");
#endif
#ifdef SMART_WINDOW_TITLE
  printf(" +SMART_WINDOW_TITLE");
#else
  printf(" -SMART_WINDOW_TITLE");
#endif
#ifdef XTERM_COLOR_CHANGE
  printf(" +XTERM_COLOR_CHANGE");
#else
  printf(" -XTERM_COLOR_CHANGE");
#endif
#ifdef DEFINE_XTERM_COLOR
  printf(" +DEFINE_XTERM_COLOR");
#else
  printf(" -DEFINE_XTERM_COLOR");
#endif
#ifdef NO_MAPALERT
  printf(" +NO_MAPALERT");
#else
  printf(" -NO_MAPALERT");
#endif
#ifdef MAPALERT_OPTION
  printf(" +MAPALERT_OPTION");
#else
  printf(" -MAPALERT_OPTION");
#endif
#ifdef ENABLE_CORE_DUMPS
  printf(" +ENABLE_CORE_DUMPS");
#else
  printf(" -ENABLE_CORE_DUMPS");
#endif
#ifdef UTMP_SUPPORT
  printf(" +UTMP_SUPPORT");
#else
  printf(" -UTMP_SUPPORT");
#endif
#ifdef HAVE_SAVED_UIDS
  printf(" +HAVE_SAVED_UIDS");
#else
  printf(" -HAVE_SAVED_UIDS");
#endif
#ifdef USE_GETGRNAME
  printf(" +USE_GETGRNAME");
#else
  printf(" -USE_GETGRNAME");
#endif
#ifdef ALLOW_BACKQUOTE_EXEC
  printf(" +ALLOW_BACKQUOTE_EXEC");
#else
  printf(" -ALLOW_BACKQUOTE_EXEC");
#endif
#ifdef WARN_OLDER
  printf(" +WARN_OLDER");
#else
  printf(" -WARN_OLDER");
#endif

  printf("\n\nCompile-time definitions:\n");

#ifdef PATH_ENV
  printf(" PATH_ENV=\"%s\"\n", PATH_ENV);
#else
  printf(" -PATH_ENV\n");
#endif
#ifdef REFRESH_PERIOD
  printf(" REFRESH_PERIOD=%d\n", REFRESH_PERIOD);
#else
  printf(" -REFRESH_PERIOD\n");
#endif
#ifdef PRINTPIPE
  printf(" PRINTPIPE=\"%s\"\n", PRINTPIPE);
#else
  printf(" -PRINTPIPE\n");
#endif
#ifdef KS_DELETE
  printf(" KS_DELETE=\"%s\"\n", KS_DELETE);
#else
  printf(" -KS_DELETE\n");
#endif
#ifdef SAVELINES
  printf(" SAVELINES=%d\n", SAVELINES);
#else
  printf(" -SAVELINES\n");
#endif
#ifdef CUTCHARS
  printf(" CUTCHARS=\"%s\"\n", CUTCHARS);
#else
  printf(" -CUTCHARS\n");
#endif
#ifdef MULTICLICK_TIME
  printf(" MULTICLICK_TIME=%d\n", MULTICLICK_TIME);
#else
  printf(" -MULTICLICK_TIME\n");
#endif
#ifdef SCROLLBAR_DEFAULT_TYPE
  printf(" SCROLLBAR_DEFAULT_TYPE=%d\n", SCROLLBAR_DEFAULT_TYPE);
#else
  printf(" -SCROLLBAR_DEFAULT_TYPE\n");
#endif
#ifdef SB_WIDTH
  printf(" SB_WIDTH=%d\n", SB_WIDTH);
#else
  printf(" -SB_WIDTH\n");
#endif
#ifdef SCROLLBAR_INITIAL_DELAY
  printf(" SCROLLBAR_INITIAL_DELAY=%d\n", SCROLLBAR_INITIAL_DELAY);
#else
  printf(" -SCROLLBAR_INITIAL_DELAY\n");
#endif
#ifdef SCROLLBAR_CONTINUOUS_DELAY
  printf(" SCROLLBAR_CONTINUOUS_DELAY=%d\n", SCROLLBAR_CONTINUOUS_DELAY);
#else
  printf(" -SCROLLBAR_CONTINUOUS_DELAY\n");
#endif
#ifdef MENUBAR_MAX
  printf(" MENUBAR_MAX=%d\n", MENUBAR_MAX);
#else
  printf(" -MENUBAR_MAX\n");
#endif
#ifdef ESCZ_ANSWER
  printf(" ESCZ_ANSWER=\"%s\"\n", ESCZ_ANSWER);
#else
  printf(" -ESCZ_ANSWER\n");
#endif
#ifdef TTY_GRP_NAME
  printf(" TTY_GRP_NAME=\"%s\"\n", TTY_GRP_NAME);
#else
  printf(" -TTY_GRP_NAME\n");
#endif
#ifdef CONFIG_SEARCH_PATH
  printf(" CONFIG_SEARCH_PATH=\"%s\"\n", CONFIG_SEARCH_PATH);
#else
  printf(" -CONFIG_SEARCH_PATH\n");
#endif
#ifdef CONFIG_FILE_NAME
  printf(" CONFIG_FILE_NAME=\"%s\"\n", CONFIG_FILE_NAME);
#else
  printf(" -CONFIG_FILE_NAME\n");
#endif

  printf("\n");
  exit(EXIT_SUCCESS);
}

/* This defines how many mistakes to allow before giving up
   and printing the usage                          -- mej   */
#define BAD_THRESHOLD 3
#define CHECK_BAD()  do { \
	               if (++bad_opts >= BAD_THRESHOLD) { \
			 print_error("error threshold exceeded, giving up"); \
			 usage(); \
		       } else { \
			 print_error("attempting to continue, but performance may be unpredictable"); \
		       } \
                     } while(0)

void
get_options(int argc, char *argv[])
{

  register unsigned long i, j, l;
  unsigned char bad_opts = 0;

  for (i = 1; i < argc; i++) {

    register char *opt = argv[i];
    char *val_ptr = NULL;
    unsigned char islong = 0, hasequal = 0;

    D_OPTIONS(("argv[%d] == \"%s\"\n", i, argv[i]));

    if (*opt != '-') {
      print_error("unexpected argument %s -- expected option", opt);
      CHECK_BAD();
      continue;
    }
    if (*(opt + 1) == '-') {
      islong = 1;
      D_OPTIONS(("Long option detected\n"));
    }
    if (islong) {
      opt += 2;
      for (j = 0; j < optList_numoptions(); j++) {
	if (!strncasecmp(optList[j].long_opt, opt, (l = strlen(optList[j].long_opt))) &&
	    (opt[l] == '=' || !opt[l])) {
	  D_OPTIONS(("Match found at %d:  %s == %s\n", j, optList[j].long_opt, opt));
	  break;
	}
      }
      if (j == optList_numoptions()) {
	print_error("unrecognized long option --%s", opt);
	CHECK_BAD();
	continue;
      }
      /* Put option-specific warnings here -- mej */
#if 0				/* No longer needed, since it works :) */
      if (optList[j].short_opt == 'w') {
	print_error("warning:  Use of the -w / --border-width option is discouraged and unsupported.");
      }
#endif

      if ((val_ptr = strchr(opt, '=')) != NULL) {
	*val_ptr = 0;
	val_ptr++;
	hasequal = 1;
      } else {
	if (argv[i + 1]) {
	  if (*argv[i + 1] != '-' || StrCaseStr(optList[j].long_opt, "font")) {
	    val_ptr = argv[++i];
	  }
	}
      }
      D_OPTIONS(("hasequal == %d  val_ptr == %10.8p \"%s\"\n", hasequal, val_ptr, (val_ptr ? val_ptr : "(nil)")));
      if (j == 0 || j == 1) {
	continue;
      }
      if (!(optList[j].flag & OPT_BOOLEAN) && (val_ptr == NULL)) {
	print_error("long option --%s requires a%s value", opt,
		    (optList[j].flag & OPT_INTEGER ? "n integer" : " string"));
	CHECK_BAD();
	continue;
      }
      if (!strcasecmp(opt, "exec")) {
	D_OPTIONS(("--exec option detected\n"));
	Options |= Opt_exec;
	if (!hasequal) {

	  register unsigned short k, len = argc - i;

	  rs_execArgs = (char **) malloc(sizeof(char *) * (argc - i + 1));

	  for (k = 0; k < len; k++) {
	    rs_execArgs[k] = strdup(argv[k + i]);
	    D_OPTIONS(("rs_execArgs[%d] == %s\n", k, rs_execArgs[k]));
	  }
	  rs_execArgs[k] = (char *) NULL;
	  return;
	} else {

	  register unsigned short k;

	  rs_execArgs = (char **) malloc(sizeof(char *) * (NumWords(val_ptr) + 1));

	  for (k = 0; val_ptr; k++) {
	    rs_execArgs[k] = Word(1, val_ptr);
	    val_ptr = PWord(2, val_ptr);
	    D_OPTIONS(("rs_execArgs[%d] == %s\n", k, rs_execArgs[k]));
	  }
	  rs_execArgs[k] = (char *) NULL;
	}
      } else if (!strcasecmp(opt, "help")) {
	usage();
      } else if (!strcasecmp(opt, "version")) {
	version();
      } else {			/* It's not --exec */
	if (optList[j].flag & OPT_BOOLEAN) {	/* Boolean value */
	  D_OPTIONS(("Boolean option detected\n"));
	  if (val_ptr) {
	    if (BOOL_OPT_ISTRUE(val_ptr)) {
	      D_OPTIONS(("\"%s\" == TRUE\n", val_ptr));
	      if (optList[j].maskvar) {
		*(optList[j].maskvar) |= optList[j].mask;
	      }
	      if (optList[j].pval) {
		*((const char **) optList[j].pval) = *true_vals;
	      }
	    } else if (BOOL_OPT_ISFALSE(val_ptr)) {
	      D_OPTIONS(("\"%s\" == FALSE\n", val_ptr));
	      if (optList[j].maskvar) {
		*(optList[j].maskvar) &= ~(optList[j].mask);
	      }
	      if (optList[j].pval) {
		*((const char **) optList[j].pval) = *false_vals;
	      }
	    } else {
	      print_error("unrecognized boolean value \"%s\" for option --%s",
			  val_ptr, optList[j].long_opt);
	      CHECK_BAD();
	    }
	  } else {		/* No value, so force it on */
	    D_OPTIONS(("Forcing option --%s to TRUE\n", opt));
	    if (optList[j].maskvar) {
	      *(optList[j].maskvar) |= optList[j].mask;
	    }
	    if (optList[j].pval) {
	      *((const char **) optList[j].pval) = *true_vals;
	    }
	  }
	} else if (optList[j].flag & OPT_INTEGER) {	/* Integer value */
	  D_OPTIONS(("Integer option detected\n"));
	  *((int *) optList[j].pval) = strtol(val_ptr, (char **) NULL, 0);
	} else {		/* String value */
	  D_OPTIONS(("String option detected\n"));
	  if (val_ptr && optList[j].pval) {
	    *((const char **) optList[j].pval) = strdup(val_ptr);
	  }
	}
      }
    } else {			/* It's a POSIX option */

      register unsigned short pos;
      unsigned char done = 0;

      for (pos = 1; opt[pos] && !done; pos++) {
	for (j = 0; j < optList_numoptions(); j++) {
	  if (optList[j].short_opt == opt[pos]) {
	    D_OPTIONS(("Match found at %d:  %c == %c\n", j, optList[j].short_opt, opt[pos]));
	    break;
	  }
	}
	if (j == optList_numoptions()) {
	  print_error("unrecognized option -%c", opt[pos]);
	  CHECK_BAD();
	  continue;
	}
	/* Put option-specific warnings here -- mej */
#if 0				/* No longer needed, since it works :) */
	if (optList[j].short_opt == 'w') {
	  print_error("warning:  Use of the -w / --border-width option is discouraged and unsupported.");
	}
#endif

	if (!(optList[j].flag & OPT_BOOLEAN)) {
	  if (opt[pos + 1]) {
	    val_ptr = opt + pos + 1;
	    done = 1;
	  } else if ((val_ptr = argv[++i]) != NULL) {
	    done = 1;
	  }
	  D_OPTIONS(("val_ptr == %s  done == %d\n", val_ptr, done));
	  if (j == 0 || j == 1) {
	    continue;
	  }
	  if ((val_ptr == NULL) || ((*val_ptr == '-') && (optList[j].short_opt != 'F'))) {
	    print_error("option -%c requires a%s value", opt[pos],
			(optList[j].flag & OPT_INTEGER ? "n integer" : " string"));
	    CHECK_BAD();
	    if (val_ptr) {	/* If the "arg" was actually an option, don't skip it */
	      i--;
	    }
	    continue;
	  }
	}
	if (opt[pos] == 'e') {	/* It's an exec */

	  register unsigned short k, len;

	  D_OPTIONS(("-e option detected\n"));
	  Options |= Opt_exec;

	  if (opt[pos + 1]) {
	    len = argc - i + 2;
	    k = i;
	  } else {
	    len = argc - i + 1;
	    k = i + 1;
	  }
	  D_OPTIONS(("len == %d  k == %d\n", len, k));
	  rs_execArgs = (char **) malloc(sizeof(char *) * len);

	  if (k == i) {
	    rs_execArgs[0] = strdup((char *) (val_ptr));
	    D_OPTIONS(("rs_execArgs[0] == %s\n", rs_execArgs[0]));
	    k++;
	  } else {
	    rs_execArgs[0] = strdup(argv[k - 1]);
	    D_OPTIONS(("rs_execArgs[0] == %s\n", rs_execArgs[0]));
	  }
	  for (; k < argc; k++) {
	    rs_execArgs[k - i] = strdup(argv[k]);
	    D_OPTIONS(("rs_execArgs[%d] == %s\n", k - i, rs_execArgs[k - i]));
	  }
	  rs_execArgs[len - 1] = (char *) NULL;
	  return;
	} else if (opt[pos] == 'h') {
	  usage();

	} else {
	  if (optList[j].flag & OPT_BOOLEAN) {	/* Boolean value */
	    D_OPTIONS(("Boolean option detected\n"));
	    if (optList[j].maskvar) {
	      *(optList[j].maskvar) |= optList[j].mask;
	    }
	    if (optList[j].pval) {
	      *((const char **) optList[j].pval) = *true_vals;
	    }
	  } else if (optList[j].flag & OPT_INTEGER) {	/* Integer value */
	    D_OPTIONS(("Integer option detected\n"));
	    *((int *) optList[j].pval) = strtol(val_ptr, (char **) NULL, 0);
	    D_OPTIONS(("Got value %d\n", *((int *) optList[j].pval)));
	  } else {		/* String value */
	    D_OPTIONS(("String option detected\n"));
	    if (optList[j].pval) {
	      *((const char **) optList[j].pval) = strdup(val_ptr);
	    }
	  }			/* End if value type */
	}			/* End if option type */
      }				/* End if (exec or help or other) */
    }				/* End if (islong) */
  }				/* End main for loop */
}

#ifdef USE_THEMES
void
get_initial_options(int argc, char *argv[])
{

  register unsigned long i, j;

  for (i = 1; i < argc; i++) {

    register char *opt = argv[i];
    char *val_ptr = NULL;
    unsigned char islong = 0, hasequal = 0;

    D_OPTIONS(("argv[%d] == \"%s\"\n", i, argv[i]));

    if (*opt != '-') {
      continue;
    }
    if (*(opt + 1) == '-') {
      islong = 1;
      D_OPTIONS(("Long option detected\n"));
    }
    if (islong) {
      opt += 2;
      if (!BEG_STRCASECMP(opt, "theme")) {
	j = 0;
      } else if (!BEG_STRCASECMP(opt, "config-file")) {
	j = 1;
      } else
	continue;

      if ((val_ptr = strchr(opt, '=')) != NULL) {
	*val_ptr = 0;
	val_ptr++;
	hasequal = 1;
      } else {
	if (argv[i + 1]) {
	  if (*argv[i + 1] != '-') {
	    val_ptr = argv[++i];
	  }
	}
      }
      D_OPTIONS(("hasequal == %d  val_ptr == %10.8p \"%s\"\n", hasequal, val_ptr, val_ptr));
      if (val_ptr == NULL) {
	print_error("long option --%s requires a string value", opt);
	continue;
      }
      D_OPTIONS(("String option detected\n"));
      if (val_ptr && optList[j].pval) {
	*((const char **) optList[j].pval) = strdup(val_ptr);
      }
    } else {			/* It's a POSIX option */

      register unsigned short pos;
      unsigned char done = 0;

      for (pos = 1; opt[pos] && !done; pos++) {
	if (opt[pos] == 't') {
	  j = 0;
	} else if (opt[pos] == 'X') {
	  j = 1;
	} else
	  continue;

	if (opt[pos + 1]) {
	  val_ptr = opt + pos + 1;
	  done = 1;
	} else if ((val_ptr = argv[++i]) != NULL) {
	  done = 1;
	}
	D_OPTIONS(("val_ptr == %s  done == %d\n", val_ptr, done));
	if ((val_ptr == NULL) || (*val_ptr == '-')) {
	  print_error("option -%c requires a string value", opt[pos]);
	  if (val_ptr) {	/* If the "arg" was actually an option, don't skip it */
	    i--;
	  }
	  continue;
	}
	D_OPTIONS(("String option detected\n"));
	if (optList[j].pval) {
	  *((const char **) optList[j].pval) = strdup(val_ptr);
	}
      }				/* End for loop */
    }				/* End if (islong) */
  }				/* End main for loop */
}

#endif

/***** The Config File Section *****/

/* Max length of a line in the config file */
#define CONFIG_BUFF 20480

/* The context identifier.  This tells us what section of the config file
   we're in, for syntax checking purposes and the like.            -- mej */

#define CTX_NULL         0
#define CTX_MAIN         1
#define CTX_COLOR        2
#define CTX_ATTRIBUTES   3
#define CTX_TOGGLES      4
#define CTX_KEYBOARD     5
#define CTX_MISC         6
#define CTX_PIXMAPS      7
#define CTX_KANJI        8
#define CTX_UNDEF        ((unsigned char) -1)
#define CTX_MAX          8

/* This structure defines a context and its attributes */

struct context_struct {

  const unsigned char id;
  const char *description;
  void (*ctx_handler) (char *);
  const unsigned char valid_sub_contexts[CTX_MAX];

} contexts[] = {

  {
    CTX_NULL, "none", NULL, {
      CTX_MAIN, 0, 0, 0, 0, 0, 0, 0
    }
  },
  {
    CTX_MAIN, "main", parse_main, {
      CTX_COLOR, CTX_ATTRIBUTES, CTX_TOGGLES,
	  CTX_KEYBOARD, CTX_MISC, CTX_PIXMAPS,
	  CTX_KANJI, 0
    }
  },
  {
    CTX_COLOR, "color", parse_color, {
      0, 0, 0, 0, 0, 0, 0, 0
    }
  },
  {
    CTX_ATTRIBUTES, "attributes", parse_attributes, {
      0, 0, 0, 0, 0, 0, 0, 0
    }
  },
  {
    CTX_TOGGLES, "toggles", parse_toggles, {
      0, 0, 0, 0, 0, 0, 0, 0
    }
  },
  {
    CTX_KEYBOARD, "keyboard", parse_keyboard, {
      0, 0, 0, 0, 0, 0, 0, 0
    }
  },
  {
    CTX_MISC, "misc", parse_misc, {
      0, 0, 0, 0, 0, 0, 0, 0
    }
  },
  {
    CTX_PIXMAPS, "pixmaps", parse_pixmaps, {
      0, 0, 0, 0, 0, 0, 0, 0
    }
  },
  {
    CTX_KANJI, "kanji", parse_kanji, {
      0, 0, 0, 0, 0, 0, 0, 0
    }
  },
  {
    CTX_UNDEF, NULL, parse_undef, {
      0, 0, 0, 0, 0, 0, 0, 0
    }
  }

};

#define ctx_name_to_id(the_id, n, i) do { \
                                       for ((i)=0; (i) <= CTX_MAX; (i)++) { \
                                         if (!strcasecmp((n), contexts[(i)].description)) { \
		                           (the_id) = contexts[(i)].id; \
					   break; \
					 } \
			               } \
                                       if ((i) > CTX_MAX) (the_id) = CTX_UNDEF; \
                                     } while (0)

#define ctx_id_to_name(id) (contexts[(id)].description)
#define ctx_id_to_func(id) (contexts[(id)].ctx_handler)

/* The context stack.  This keeps track of the current context and each
   previous one.  You MUST define MAX_CTX_DEPTH to the absolute maximum
   number of context levels deep your contexts go, or the results can be
   Very Bad.  I recommend erring on the side of caution.          -- mej */

#define MAX_CTX_DEPTH 10
#define ctx_push(ctx) id_stack[++cur_ctx] = (ctx)
#define ctx_pop()  (id_stack[cur_ctx--])
#define ctx_peek() (id_stack[cur_ctx])

unsigned char id_stack[MAX_CTX_DEPTH];
unsigned short cur_ctx = 0;

/* The file state stack.  This keeps track of the file currently being
   parsed.  This allows for %include directives.                  -- mej */

typedef struct file_state_struct {

  FILE *fp;
  char *path;
  unsigned long line;
  unsigned char skip_to_end;

} file_state;

#define MAX_FILE_DEPTH 10
#define file_push(fs) do { \
                        cur_file++; \
                        file_stack[cur_file].fp = (fs).fp; \
                        file_stack[cur_file].path = (fs).path; \
                        file_stack[cur_file].line = (fs).line; \
			file_stack[cur_file].skip_to_end = (fs).skip_to_end; \
                      } while (0)

#define file_pop()    (cur_file--)
#define file_peek(fs) do { \
                        (fs).fp = file_stack[cur_file].fp; \
                        (fs).path = file_stack[cur_file].path; \
                        (fs).line = file_stack[cur_file].line; \
			(fs).skip_to_end = file_stack[cur_file].skip_to_end; \
                      } while (0)
#define file_peek_fp()   (file_stack[cur_file].fp)
#define file_peek_path() (file_stack[cur_file].path)
#define file_peek_line() (file_stack[cur_file].line)
#define file_peek_skip() (file_stack[cur_file].skip_to_end)

#define file_poke_fp(f)    ((file_stack[cur_file].fp) = (f))
#define file_poke_path(p)  ((file_stack[cur_file].path) = (p))
#define file_poke_line(l)  ((file_stack[cur_file].line) = (l))
#define file_poke_skip(s)  ((file_stack[cur_file].skip_to_end) = (s))

#define file_inc_line()     (file_stack[cur_file].line++)

file_state file_stack[MAX_FILE_DEPTH];
short cur_file = -1;

/* The function structures */

typedef char *(*eterm_function_ptr) (char *);
typedef struct eterm_function_struct {

  char *name;
  eterm_function_ptr ptr;
  int params;

} eterm_func;

eterm_func builtins[] =
{
  {"random", builtin_random, -1},
  {"exec", builtin_exec, -1},
  {"version", builtin_version, 0},
  {"appname", builtin_appname, 0},
  {(char *) NULL, (eterm_function_ptr) NULL, 0}
};

char *
builtin_random(char *param)
{

  unsigned long n, index;
  static unsigned int rseed = 0;

  D_OPTIONS(("builtin_random(%s) called\n", param));

  if (rseed == 0) {
    rseed = (unsigned int) (getpid() * time(NULL) % ((unsigned int) -1));
    srand(rseed);
  }
  n = NumWords(param);
  index = (int) (n * ((float) rand()) / (RAND_MAX + 1.0)) + 1;
  D_OPTIONS(("random index == %lu\n", index));

  return (Word(index, param));
}

char *
builtin_exec(char *param)
{

  D_OPTIONS(("builtin_exec(%s) called\n", param));

  return (param);
}

char *
builtin_version(char *param)
{

  D_OPTIONS(("builtin_version(%s) called\n", param));

  return (Word(1, VERSION));
}

char *
builtin_appname(char *param)
{

  D_OPTIONS(("builtin_appname(%s) called\n", param));

  return (Word(1, APL_NAME "-" VERSION));
}

/* chomp() removes leading and trailing whitespace/quotes from a string */
char *
chomp(char *s)
{

  register char *front, *back;

  for (front = s; *front && isspace(*front); front++);
  /*
     if (*front == '\"') front++;
   */
  for (back = s + strlen(s) - 1; *back && isspace(*back) && back > front; back--);
  /*
     if (*back == '\"') back--;
   */

  *(++back) = 0;
  if (front != s)
    memmove(s, front, back - front + 1);
  return (s);
}

/* shell_expand() takes care of shell variable expansion, quote conventions,
   calling of built-in functions, etc.                                -- mej */
char *
shell_expand(char *s)
{

  register char *tmp;
  register char *new;
  register char *pbuff = s, *tmp1;
  register unsigned long j, k, l;
  unsigned char eval_escape = 1, eval_var = 1, eval_exec = 1, eval_func = 1, in_single = 0, in_double = 0;
  unsigned long fsize;
  char *Command, *Output, *EnvVar, *OutFile;
  FILE *fp;

  ASSERT(s != NULL);
  if (!s)
    return ((char *) NULL);

  new = (char *) MALLOC(CONFIG_BUFF);

  for (j = 0; *pbuff && j < CONFIG_BUFF; pbuff++, j++) {
    switch (*pbuff) {
      case '~':
	D_OPTIONS(("Tilde detected.\n"));
	if (eval_var) {
	  strncpy(new + j, getenv("HOME"), CONFIG_BUFF - j);
	  j += strlen(getenv("HOME")) - 1;
	} else {
	  new[j] = *pbuff;
	}
	break;
      case '\\':
	D_OPTIONS(("Escape sequence detected.\n"));
	if (eval_escape || (in_single && *(pbuff + 1) == '\'')) {
	  switch (tolower(*(++pbuff))) {
	    case 'n':
	      new[j] = '\n';
	      break;
	    case 'r':
	      new[j] = '\r';
	      break;
	    case 't':
	      new[j] = '\t';
	      break;
	    case 'b':
	      j -= 2;
	      break;
	    case 'f':
	      new[j] = '\f';
	      break;
	    case 'a':
	      new[j] = '\a';
	      break;
	    case 'v':
	      new[j] = '\v';
	      break;
	    case 'e':
	      new[j] = '\033';
	      break;
	    default:
	      new[j] = *pbuff;
	      break;
	  }
	} else {
	  new[j++] = *(pbuff++);
	  new[j] = *pbuff;
	}
	break;
      case '%':
	D_OPTIONS(("%% detected.\n"));
	for (k = 0, pbuff++; builtins[k].name != NULL; k++) {
	  D_OPTIONS(("Checking for function %%%s, pbuff == \"%s\"\n", builtins[k].name, pbuff));
	  l = strlen(builtins[k].name);
	  if (!strncasecmp(builtins[k].name, pbuff, l) &&
	      ((pbuff[l] == '(') || (pbuff[l] == ' ' && pbuff[l + 1] == ')'))) {
	    break;
	  }
	}
	if (builtins[k].name == NULL) {
	  new[j] = *pbuff;
	} else {
	  D_OPTIONS(("Call to built-in function %s detected.\n", builtins[k].name));
	  Command = (char *) MALLOC(CONFIG_BUFF);
	  pbuff += l;
	  if (*pbuff != '(')
	    pbuff++;
	  for (tmp1 = Command, pbuff++, l = 1; l && *pbuff; pbuff++, tmp1++) {
	    switch (*pbuff) {
	      case '(':
		l++;
		*tmp1 = *pbuff;
		break;
	      case ')':
		l--;
	      default:
		*tmp1 = *pbuff;
		break;
	    }
	  }
	  *(--tmp1) = 0;
	  if (l) {
	    print_error("parse error in file %s, line %lu:  Mismatched parentheses",
			file_peek_path(), file_peek_line());
	    return ((char *) NULL);
	  }
	  Command = shell_expand(Command);
	  Output = (builtins[k].ptr) (Command);
	  FREE(Command);
	  if (Output && *Output) {
	    l = strlen(Output) - 1;
	    strncpy(new + j, Output, CONFIG_BUFF - j);
	    j += l;
	    FREE(Output);
	  } else {
	    j--;
	  }
	  pbuff--;
	}
	break;
      case '`':
#ifdef ALLOW_BACKQUOTE_EXEC
	D_OPTIONS(("Backquotes detected.  Evaluating expression.\n"));
	if (eval_exec) {
	  Command = (char *) MALLOC(CONFIG_BUFF);
	  l = 0;
	  for (pbuff++; *pbuff && *pbuff != '`' && l < CONFIG_BUFF; pbuff++, l++) {
	    switch (*pbuff) {
	      case '$':
		D_OPTIONS(("Environment variable detected.  Evaluating.\n"));
		EnvVar = (char *) MALLOC(128);
		switch (*(++pbuff)) {
		  case '{':
		    for (pbuff++, k = 0; *pbuff != '}' && k < 127; k++, pbuff++)
		      EnvVar[k] = *pbuff;
		    break;
		  case '(':
		    for (pbuff++, k = 0; *pbuff != ')' && k < 127; k++, pbuff++)
		      EnvVar[k] = *pbuff;
		    break;
		  default:
		    for (k = 0; (isalnum(*pbuff) || *pbuff == '_') && k < 127; k++, pbuff++)
		      EnvVar[k] = *pbuff;
		    break;
		}
		EnvVar[k] = 0;
		if ((tmp = getenv(EnvVar))) {
		  strncpy(Command + l, tmp, CONFIG_BUFF - l);
		  l = strlen(Command) - 1;
		}
		pbuff--;
		break;
	      default:
		Command[l] = *pbuff;
	    }
	  }
	  Command[l] = 0;
	  OutFile = tmpnam(NULL);
	  if (l + strlen(OutFile) + 8 > CONFIG_BUFF) {
	    print_error("parse error in file %s, line %lu:  Cannot execute command, line too long",
			file_peek_path(), file_peek_line());
	    return ((char *) NULL);
	  }
	  strcat(Command, " >");
	  strcat(Command, OutFile);
	  system(Command);
	  if ((fp = fopen(OutFile, "rb")) >= 0) {
	    fseek(fp, 0, SEEK_END);
	    fsize = ftell(fp);
	    rewind(fp);
	    if (fsize) {
	      Output = (char *) MALLOC(fsize + 1);
	      fread(Output, fsize, 1, fp);
	      Output[fsize] = 0;
	      D_OPTIONS(("Command returned \"%s\"\n", Output));
	      fclose(fp);
	      remove(OutFile);
	      Output = CondenseWhitespace(Output);
	      strncpy(new + j, Output, CONFIG_BUFF - j);
	      j += strlen(Output) - 1;
	      FREE(Output);
	    } else {
	      print_warning("Command at line %lu of file %s returned no output.", file_peek_line(), file_peek_path());
	    }
	  } else {
	    print_warning("Output file %s could not be created.  (line %lu of file %s)", (OutFile ? OutFile : "(null)"),
			  file_peek_line(), file_peek_path());
	  }
	  FREE(Command);
	} else {
	  new[j] = *pbuff;
	}
#else
	print_error("warning:  backquote execution support not compiled in, ignoring");
	new[j] = *pbuff;
#endif
	break;
      case '$':
	D_OPTIONS(("Environment variable detected.  Evaluating.\n"));
	if (eval_var) {
	  EnvVar = (char *) MALLOC(128);
	  switch (*(++pbuff)) {
	    case '{':
	      for (pbuff++, k = 0; *pbuff != '}' && k < 127; k++, pbuff++)
		EnvVar[k] = *pbuff;
	      break;
	    case '(':
	      for (pbuff++, k = 0; *pbuff != ')' && k < 127; k++, pbuff++)
		EnvVar[k] = *pbuff;
	      break;
	    default:
	      for (k = 0; (isalnum(*pbuff) || *pbuff == '_') && k < 127; k++, pbuff++)
		EnvVar[k] = *pbuff;
	      break;
	  }
	  EnvVar[k] = 0;
	  if ((tmp = getenv(EnvVar))) {
	    strncpy(new, tmp, CONFIG_BUFF - j);
	    j += strlen(tmp) - 1;
	  }
	  pbuff--;
	} else {
	  new[j] = *pbuff;
	}
	break;
      case '\"':
	D_OPTIONS(("Double quotes detected.\n"));
	if (!in_single) {
	  if (in_double) {
	    in_double = 0;
	  } else {
	    in_double = 1;
	  }
	}
	new[j] = *pbuff;
	break;

      case '\'':
	D_OPTIONS(("Single quotes detected.\n"));
	if (in_single) {
	  eval_var = 1;
	  eval_exec = 1;
	  eval_func = 1;
	  eval_escape = 1;
	  in_single = 0;
	} else {
	  eval_var = 0;
	  eval_exec = 0;
	  eval_func = 0;
	  eval_escape = 0;
	  in_single = 1;
	}
	new[j] = *pbuff;
	break;

      default:
	new[j] = *pbuff;
    }
  }
  new[j] = 0;

  D_OPTIONS(("shell_expand(%s) returning \"%s\"\n", s, new));

  strcpy(s, new);
  FREE(new);
  return (s);
}

/* The config file parsers.  Each function handles a given context. */

void
parse_main(char *buff)
{

  ASSERT(buff != NULL);

  print_error("parse error in file %s, line %lu:  Attribute "
	      "\"%s\" is not valid within context main\n",
	      file_peek_path(), file_peek_line(), buff);
}

void
parse_color(char *buff)
{

  ASSERT(buff != NULL);

  if (!BEG_STRCASECMP(buff, "foreground ")) {
    rs_color[fgColor] = Word(2, buff);
  } else if (!BEG_STRCASECMP(buff, "background ")) {
    rs_color[bgColor] = Word(2, buff);

  } else if (!BEG_STRCASECMP(buff, "tint ")) {
#ifdef PIXMAP_OFFSET
    rs_tintMask = strtoul(buff + 5, (char **) NULL, 0);
#else
    print_error("warning:  support for the tint attribute was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "shade ")) {
#ifdef PIXMAP_OFFSET
    rs_shadePct = strtoul(buff + 5, (char **) NULL, 0);
#else
    print_error("warning:  support for the shade attribute was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "cursor ")) {

#ifndef NO_CURSORCOLOR
    rs_color[cursorColor] = Word(2, buff);
#else
    print_error("warning:  support for the cursor attribute was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "cursor_text ")) {
#ifndef NO_CURSORCOLOR
    rs_color[cursorColor2] = Word(2, buff);
#else
    print_error("warning:  support for the cursor_text attribute was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "menu_text ")) {
    rs_color[menuTextColor] = Word(2, buff);

  } else if (!BEG_STRCASECMP(buff, "scrollbar ")) {
#if defined(KEEP_SCROLLCOLOR)
    rs_color[scrollColor] = Word(2, buff);
#else
    print_error("warning:  support for the scrollbar color attribute was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "unfocusedscrollbar ")) {
#if defined(KEEP_SCROLLCOLOR) && defined(CHANGE_SCROLLCOLOR_ON_FOCUS)
    rs_color[unfocusedScrollColor] = Word(2, buff);
#else
    print_error("warning:  support for the unfocusedscrollbar color attribute was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "pointer ")) {
    rs_color[pointerColor] = Word(2, buff);

  } else if (!BEG_STRCASECMP(buff, "video ")) {

    char *tmp = PWord(2, buff);

    if (!BEG_STRCASECMP(tmp, "reverse")) {
      Options |= Opt_reverseVideo;
      rs_reverseVideo = *true_vals;
    } else if (BEG_STRCASECMP(tmp, "normal")) {
      print_error("parse error in file %s, line %lu:  Invalid value \"%s\" for attribute video",
		  file_peek_path(), file_peek_line(), tmp);
    }
  } else if (!BEG_STRCASECMP(buff, "color ")) {

    char *tmp = 0, *r1, *g1, *b1;
    unsigned int n, r, g, b, index = 0;

    n = NumWords(buff);
    if (n < 3) {
      print_error("parse error in file %s, line %lu:  Invalid parameter list \"%s\" for "
		  "attribute color", file_peek_path(), file_peek_line(), (tmp ? tmp : ""));
      return;
    }
    tmp = PWord(2, buff);
    r1 = PWord(3, buff);
    if (!isdigit(*r1)) {
      if (isdigit(*tmp)) {
	n = strtoul(tmp, (char **) NULL, 0);
	if (n >= 0 && n <= 7) {
	  index = minColor + n;
	} else if (n >= 8 && n <= 15) {
	  index = minBright + n - 8;
	}
	rs_color[index] = Word(1, r1);
	return;
      } else {
	if (!BEG_STRCASECMP(tmp, "bd ")) {
#ifndef NO_BOLDUNDERLINE
	  rs_color[colorBD] = Word(1, r1);
#else
	  print_error("warning:  support for the color bd attribute was not compiled in, ignoring");
#endif
	  return;
	} else if (!BEG_STRCASECMP(tmp, "ul ")) {
#ifndef NO_BOLDUNDERLINE
	  rs_color[colorUL] = Word(1, r1);
#else
	  print_error("warning:  support for the color ul attribute was not compiled in, ignoring");
#endif
	  return;
	} else {
	  tmp = Word(1, tmp);
	  print_error("parse error in file %s, line %lu:  Invalid color index \"%s\"",
		      file_peek_path(), file_peek_line(), (tmp ? tmp : ""));
	  free(tmp);
	}
      }
    }
    if (n != 5) {
      print_error("parse error in file %s, line %lu:  Invalid parameter list \"%s\" for "
		  "attribute color", file_peek_path(), file_peek_line(), (tmp ? tmp : ""));
      return;
    }
    g1 = PWord(4, buff);
    b1 = PWord(5, buff);
    if (isdigit(*tmp)) {
      n = strtoul(tmp, (char **) NULL, 0);
      r = strtoul(r1, (char **) NULL, 0);
      g = strtoul(g1, (char **) NULL, 0);
      b = strtoul(b1, (char **) NULL, 0);
      if (n >= 0 && n <= 7) {
	index = minColor + n;
	rs_color[index] = MALLOC(14);
	sprintf(rs_color[index], "#%02x%02x%02x", r, g, b);
      } else if (n >= 8 && n <= 15) {
	index = minBright + n - 8;
	rs_color[index] = MALLOC(14);
	sprintf(rs_color[index], "#%02x%02x%02x", r, g, b);
      } else {
	print_error("parse error in file %s, line %lu:  Invalid color index %lu",
		    file_peek_path(), file_peek_line(), n);
      }

    } else if (!BEG_STRCASECMP(tmp, "bd ")) {
#ifndef NO_BOLDUNDERLINE
      rs_color[colorBD] = MALLOC(14);
      r = strtoul(r1, (char **) NULL, 0);
      g = strtoul(g1, (char **) NULL, 0);
      b = strtoul(b1, (char **) NULL, 0);
      sprintf(rs_color[colorBD], "#%02x%02x%02x", r, g, b);
#else
      print_error("warning:  support for the color bd attribute was not compiled in, ignoring");
#endif

    } else if (!BEG_STRCASECMP(tmp, "ul ")) {
#ifndef NO_BOLDUNDERLINE
      rs_color[colorUL] = MALLOC(14);
      r = strtoul(r1, (char **) NULL, 0);
      g = strtoul(g1, (char **) NULL, 0);
      b = strtoul(b1, (char **) NULL, 0);
      sprintf(rs_color[colorUL], "#%02x%02x%02x", r, g, b);
#else
      print_error("warning:  support for the color ul attribute was not compiled in, ignoring");
#endif

    } else {
      tmp = Word(1, tmp);
      print_error("parse error in file %s, line %lu:  Invalid color index \"%s\"",
		  file_peek_path(), file_peek_line(), (tmp ? tmp : ""));
      free(tmp);
    }
  } else {
    print_error("parse error in file %s, line %lu:  Attribute \"%s\" is not valid "
		"within context color", file_peek_path(), file_peek_line(), buff);
  }
}

void
parse_attributes(char *buff)
{

  ASSERT(buff != NULL);

  if (!BEG_STRCASECMP(buff, "geometry ")) {
    rs_geometry = Word(2, buff);

  } else if (!BEG_STRCASECMP(buff, "title ")) {
    rs_title = Word(2, buff);

  } else if (!BEG_STRCASECMP(buff, "name ")) {
    rs_name = Word(2, buff);

  } else if (!BEG_STRCASECMP(buff, "iconname ")) {
    rs_iconName = Word(2, buff);

  } else if (!BEG_STRCASECMP(buff, "desktop ")) {
    rs_desktop = (int) strtol(buff, (char **) NULL, 0);

  } else if (!BEG_STRCASECMP(buff, "scrollbar_type ")) {
    rs_scrollbar_type = Word(2, buff);

  } else if (!BEG_STRCASECMP(buff, "scrollbar_width ")) {
    rs_scrollbar_width = strtoul(PWord(2, buff), (char **) NULL, 0);

  } else if (!BEG_STRCASECMP(buff, "font ")) {

    char *tmp = PWord(2, buff);
    unsigned char n;

    if (NumWords(buff) != 3) {
      print_error("parse error in file %s, line %lu:  Invalid parameter list \"%s\" for "
		  "attribute font", file_peek_path(), file_peek_line(), (tmp ? tmp : ""));
      return;
    }
    if (isdigit(*tmp)) {
      n = (unsigned char) strtoul(tmp, (char **) NULL, 0);
      if (n <= 4) {
	rs_font[n] = Word(2, tmp);
      } else {
	print_error("parse error in file %s, line %lu:  Invalid font index %d",
		    file_peek_path(), file_peek_line(), n);
      }
    } else if (!BEG_STRCASECMP(tmp, "bold ")) {
#ifndef NO_BOLDFONT
      rs_boldFont = Word(2, tmp);
#else
      print_error("warning:  support for the bold font attribute was not compiled in, ignoring");
#endif

    } else {
      tmp = Word(1, tmp);
      print_error("parse error in file %s, line %lu:  Invalid font index \"%s\"",
		  file_peek_path(), file_peek_line(), (tmp ? tmp : ""));
      free(tmp);
    }

  } else {
    print_error("parse error in file %s, line %lu:  Attribute \"%s\" is not valid "
		"within context attributes", file_peek_path(), file_peek_line(), (buff ? buff : ""));
  }
}

void
parse_toggles(char *buff)
{

  char *tmp = PWord(2, buff);
  unsigned char bool_val;

  ASSERT(buff != NULL);

  if (!tmp) {
    print_error("parse error in file %s, line %lu:  Missing boolean value in context toggles", file_peek_path(), file_peek_line());
    return;
  }
  if (BOOL_OPT_ISTRUE(tmp)) {
    bool_val = 1;
  } else if (BOOL_OPT_ISFALSE(tmp)) {
    bool_val = 0;
  } else {
    print_error("parse error in file %s, line %lu:  Invalid boolean value \"%s\" in context toggles",
		file_peek_path(), file_peek_line(), tmp);
    return;
  }

  if (!BEG_STRCASECMP(buff, "map_alert ")) {
#if !defined(NO_MAPALERT) && defined(MAPALERT_OPTION)
    if (bool_val) {
      Options |= Opt_mapAlert;
      rs_mapAlert = *true_vals;
    } else {
      Options &= ~(Opt_mapAlert);
      rs_mapAlert = *false_vals;
    }
#else
    print_error("warning:  support for the map_alert attribute was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "visual_bell ")) {
    if (bool_val) {
      Options |= Opt_visualBell;
      rs_visualBell = *true_vals;
    } else {
      Options &= ~(Opt_visualBell);
      rs_visualBell = *false_vals;
    }
  } else if (!BEG_STRCASECMP(buff, "login_shell ")) {
    if (bool_val) {
      Options |= Opt_loginShell;
      rs_loginShell = *true_vals;
    } else {
      Options &= ~(Opt_loginShell);
      rs_loginShell = *false_vals;
    }
  } else if (!BEG_STRCASECMP(buff, "scrollbar ")) {
    if (bool_val) {
      Options |= Opt_scrollBar;
      rs_scrollBar = *true_vals;
    } else {
      Options &= ~(Opt_scrollBar);
      rs_scrollBar = *false_vals;
    }
  } else if (!BEG_STRCASECMP(buff, "menubar ")) {
#if (MENUBAR_MAX)
    if (bool_val) {
      rs_menubar = *true_vals;
    } else {
      rs_menubar = *false_vals;
    }
#else
    print_error("warning:  support for the menubar attribute was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "utmp_logging ")) {
#ifdef UTMP_SUPPORT
    if (bool_val) {
      Options |= Opt_utmpLogging;
      rs_utmpLogging = *true_vals;
    } else {
      Options &= ~(Opt_utmpLogging);
      rs_utmpLogging = *false_vals;
    }
#else
    print_error("warning:  support for the utmp_logging attribute was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "meta8 ")) {
#ifdef META8_OPTION
    if (bool_val) {
      Options |= Opt_meta8;
      rs_meta8 = *true_vals;
    } else {
      Options &= ~(Opt_meta8);
      rs_meta8 = *false_vals;
    }
#else
    print_error("warning:  support for the meta8 attribute was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "iconic ")) {
    if (bool_val) {
      Options |= Opt_iconic;
    } else {
      Options &= ~(Opt_iconic);
    }

  } else if (!BEG_STRCASECMP(buff, "home_on_echo ")) {
    if (bool_val) {
      Options |= Opt_homeOnEcho;
      rs_homeOnEcho = *true_vals;
    } else {
      Options &= ~(Opt_homeOnEcho);
      rs_homeOnEcho = *false_vals;
    }

  } else if (!BEG_STRCASECMP(buff, "home_on_input ")) {
    if (bool_val) {
      Options |= Opt_homeOnInput;
      rs_homeOnInput = *true_vals;
    } else {
      Options &= ~(Opt_homeOnInput);
      rs_homeOnInput = *false_vals;
    }

  } else if (!BEG_STRCASECMP(buff, "home_on_refresh ")) {
    if (bool_val) {
      Options |= Opt_homeOnRefresh;
      rs_homeOnRefresh = *true_vals;
    } else {
      Options &= ~(Opt_homeOnRefresh);
      rs_homeOnRefresh = *false_vals;
    }

  } else if (!BEG_STRCASECMP(buff, "scrollbar_floating ")) {
    if (bool_val) {
      Options |= Opt_scrollBar_floating;
      rs_scrollBar_floating = *true_vals;
    } else {
      Options &= ~(Opt_scrollBar_floating);
      rs_scrollBar_floating = *false_vals;
    }

  } else if (!BEG_STRCASECMP(buff, "scrollbar_right ")) {
    if (bool_val) {
      Options |= Opt_scrollBar_right;
      rs_scrollBar_right = *true_vals;
    } else {
      Options &= ~(Opt_scrollBar_right);
      rs_scrollBar_right = *false_vals;
    }
  } else if (!BEG_STRCASECMP(buff, "scrollbar_popup ")) {
    if (bool_val) {
      Options |= Opt_scrollbar_popup;
      rs_scrollbar_popup = *true_vals;
    } else {
      Options &= ~(Opt_scrollbar_popup);
      rs_scrollbar_popup = *false_vals;
    }
  } else if (!BEG_STRCASECMP(buff, "borderless ")) {
    if (bool_val) {
      Options |= Opt_borderless;
      rs_borderless = *true_vals;
    } else {
      Options &= ~(Opt_borderless);
      rs_borderless = *false_vals;
    }
  } else if (!BEG_STRCASECMP(buff, "save_under ")) {
#ifdef BACKING_STORE
    if (bool_val) {
      Options |= Opt_saveUnder;
      rs_saveUnder = *true_vals;
    } else {
      Options &= ~(Opt_saveUnder);
      rs_saveUnder = *false_vals;
    }
#else
    print_error("warning:  support for the save_under attribute was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "trans ")) {
#ifdef PIXMAP_OFFSET
    if (bool_val) {
      Options |= Opt_pixmapTrans;
      rs_pixmapTrans = *true_vals;
    } else {
      Options &= ~(Opt_pixmapTrans);
      rs_pixmapTrans = *false_vals;
    }
#else
    print_error("warning:  support for the trans attribute was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "watch_desktop ")) {
#ifdef WATCH_DESKTOP_OPTION
    if (bool_val) {
      Options |= Opt_watchDesktop;
      rs_watchDesktop = *true_vals;
    } else {
      Options &= ~(Opt_watchDesktop);
      rs_watchDesktop = *false_vals;
    }
#else
    print_error("warning:  support for the watch_desktop attribute was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "no_cursor ")) {
    if (bool_val) {
      Options |= Opt_noCursor;
      rs_noCursor = (char *) true_vals;
    } else {
      Options &= ~(Opt_noCursor);
      rs_noCursor = (char *) false_vals;
    }

  } else if (!BEG_STRCASECMP(buff, "menubar_move ")) {
#if MENUBAR_MAX
    if (bool_val) {
      Options |= Opt_menubar_move;
      rs_menubar_move = (char *) true_vals;
    } else {
      Options &= ~(Opt_menubar_move);
      rs_menubar_move = (char *) false_vals;
    }
#else
    print_error("warning:  support for the menubar_move attribute was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "pause ")) {
    if (bool_val) {
      Options |= Opt_pause;
      rs_pause = (char *) true_vals;
    } else {
      Options &= ~(Opt_pause);
      rs_pause = (char *) false_vals;
    }

  } else if (!BEG_STRCASECMP(buff, "xterm_select ")) {
    if (bool_val) {
      Options |= Opt_xterm_select;
      rs_xterm_select = (char *) true_vals;
    } else {
      Options &= ~(Opt_xterm_select);
      rs_xterm_select = (char *) false_vals;
    }

  } else if (!BEG_STRCASECMP(buff, "select_line ")) {
    if (bool_val) {
      Options |= Opt_select_whole_line;
      rs_select_whole_line = (char *) true_vals;
    } else {
      Options &= ~(Opt_select_whole_line);
      rs_select_whole_line = (char *) false_vals;
    }

  } else if (!BEG_STRCASECMP(buff, "select_trailing_spaces ")) {
    if (bool_val) {
      Options |= Opt_select_trailing_spaces;
      rs_select_trailing_spaces = (char *) true_vals;
    } else {
      Options &= ~(Opt_select_trailing_spaces);
      rs_select_trailing_spaces = (char *) false_vals;
    }

  } else if (!BEG_STRCASECMP(buff, "viewport_mode ")) {
#ifdef PIXMAP_OFFSET
    if (bool_val) {
      Options |= Opt_viewport_mode;
      rs_viewport_mode = (char *) true_vals;
    } else {
      Options &= ~(Opt_viewport_mode);
      rs_viewport_mode = (char *) false_vals;
    }
#else
    print_error("warning:  support for the viewport_mode attribute was not compiled in, ignoring");
#endif

  } else {
    print_error("parse error in file %s, line %lu:  Attribute \"%s\" is not valid within context toggles", file_peek_path(), file_peek_line(), buff);
  }
}

#define to_keysym(p,s) do { KeySym sym; \
if (s && ((sym = XStringToKeysym(s)) != 0)) *p = sym; \
                           } while (0)

void
parse_keyboard(char *buff)
{

  ASSERT(buff != NULL);

  if (!BEG_STRCASECMP(buff, "smallfont_key ")) {
#if defined (HOTKEY_CTRL) || defined (HOTKEY_META)
    rs_smallfont_key = Word(2, buff);
    to_keysym(&ks_smallfont, rs_smallfont_key);
#else
    print_error("warning:  support for the smallfont_key attribute was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "bigfont_key ")) {
#if defined (HOTKEY_CTRL) || defined (HOTKEY_META)
    rs_bigfont_key = Word(2, buff);
    to_keysym(&ks_bigfont, rs_bigfont_key);
#else
    print_error("warning:  support for the bigfont_key attribute was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "keysym ")) {
#ifdef KEYSYM_ATTRIBUTE

    int sym, len;
    char *str = buff + 7;

    sym = (int) strtol(str, (char **) NULL, 0);
    if (sym != (int) 2147483647L) {

      if (sym >= 0xff00)
	sym -= 0xff00;
      if (sym < 0 || sym > 0xff) {
	print_error("parse error in file %s, line %lu:  Keysym 0x%x out of range 0xff00-0xffff",
		    file_peek_path(), file_peek_line(), sym + 0xff00);
	return;
      }
      str = Word(3, buff);
      chomp(str);
      len = parse_escaped_string(str);
      if (len > 255)
	len = 255;		/* We can only handle lengths that will fit in a char */
      if (len && KeySym_map[sym] == NULL) {

	char *p = malloc(len + 1);

	*p = len;
	strncpy(p + 1, str, len);
	KeySym_map[sym] = p;
      }
    }
#else
    print_error("warning:  support for the keysym attributes was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "greek ")) {
#ifdef GREEK_SUPPORT

    char *tmp = PWord(2, buff);

    if (!tmp) {
      print_error("parse error in file %s, line %lu:  Missing boolean value for attribute greek",
		  file_peek_path(), file_peek_line());
      return;
    }
    if (BOOL_OPT_ISTRUE(tmp)) {
      rs_greek_keyboard = Word(3, buff);
      if (BEG_STRCASECMP(rs_greek_keyboard, "iso")) {
	greek_setmode(GREEK_ELOT928);
      } else if (BEG_STRCASECMP(rs_greek_keyboard, "ibm")) {
	greek_setmode(GREEK_IBM437);
      } else {
	print_error("parse error in file %s, line %lu:  Invalid greek keyboard mode \"%s\"",
		    file_peek_path(), file_peek_line(), (rs_greek_keyboard ? rs_greek_keyboard : ""));
      }
    } else if (BOOL_OPT_ISFALSE(tmp)) {
      /* This space intentionally left no longer blank =^) */
    } else {
      print_error("parse error in file %s, line %lu:  Invalid boolean value \"%s\" for attribute %s",
		  file_peek_path(), file_peek_line(), tmp, buff);
      return;
    }
#else
    print_error("warning:  support for the greek attribute was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "app_keypad ")) {

    char *tmp = PWord(2, buff);

    if (!tmp) {
      print_error("parse error in file %s, line %lu:  Missing boolean value for attribute app_keypad",
		  file_peek_path(), file_peek_line());
      return;
    }
    if (BOOL_OPT_ISTRUE(tmp)) {
      PrivateModes |= PrivMode_aplKP;
      rs_app_keypad = (char *) true_vals;
    } else if (BOOL_OPT_ISFALSE(tmp)) {
      PrivateModes &= ~(PrivMode_aplKP);
      rs_app_keypad = (char *) false_vals;
    } else {
      print_error("parse error in file %s, line %lu:  Invalid boolean value \"%s\" for "
		  "attribute app_keypad", file_peek_path(), file_peek_line(), tmp);
      return;
    }

  } else if (!BEG_STRCASECMP(buff, "app_cursor ")) {

    char *tmp = PWord(2, buff);

    if (!tmp) {
      print_error("parse error in file %s, line %lu:  Missing boolean value for attribute app_cursor",
		  file_peek_path(), file_peek_line());
      return;
    }
    if (BOOL_OPT_ISTRUE(tmp)) {
      PrivateModes |= PrivMode_aplCUR;
      rs_app_cursor = (char *) true_vals;
    } else if (BOOL_OPT_ISFALSE(tmp)) {
      PrivateModes &= ~(PrivMode_aplCUR);
      rs_app_cursor = (char *) false_vals;
    } else {
      print_error("parse error in file %s, line %lu:  Invalid boolean value \"%s\" for "
		  "attribute app_cursor", file_peek_path(), file_peek_line(), tmp);
      return;
    }

  } else {
    print_error("parse error in file %s, line %lu:  Attribute \"%s\" is not valid "
		"within context keyboard", file_peek_path(), file_peek_line(), buff);
  }
}
#undef to_keysym

void
parse_misc(char *buff)
{

  ASSERT(buff != NULL);

  if (!BEG_STRCASECMP(buff, "print_pipe ")) {
#ifdef PRINTPIPE
    rs_print_pipe = strdup(PWord(2, buff));
    chomp(rs_print_pipe);
#else
    print_error("warning:  support for the print_pipe attribute was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "save_lines ")) {
    rs_saveLines = strtol(PWord(2, buff), (char **) NULL, 0);

  } else if (!BEG_STRCASECMP(buff, "min_anchor_size ")) {
    rs_min_anchor_size = strtol(PWord(2, buff), (char **) NULL, 0);

  } else if (!BEG_STRCASECMP(buff, "border_width ")) {
#ifdef BORDER_WIDTH_OPTION
    TermWin.internalBorder = (short) strtol(PWord(2, buff), (char **) NULL, 0);
#else
    print_error("warning:  support for the border_width attribute was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "menu ")) {
#if MENUBAR_MAX
    rs_menu = Word(2, buff);
    if (NumWords(buff) > 2) {
      char *btmp = Word(3, buff);

      if (BOOL_OPT_ISTRUE(btmp)) {
	rs_menubar = *true_vals;
      } else if (BOOL_OPT_ISFALSE(btmp)) {
	rs_menubar = *false_vals;
      }
    }
#else
    print_error("warning:  support for menus was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "term_name ")) {
    rs_term_name = Word(2, buff);

  } else if (!BEG_STRCASECMP(buff, "debug ")) {
    debug_level = (unsigned int) strtoul(PWord(2, buff), (char **) NULL, 0);

  } else if (!BEG_STRCASECMP(buff, "exec ")) {

    register unsigned short k, n;

    Options |= Opt_exec;

    rs_execArgs = (char **) malloc(sizeof(char *) * ((n = NumWords(PWord(2, buff))) + 1));

    for (k = 0; k < n; k++) {
      rs_execArgs[k] = Word(k + 2, buff);
      D_OPTIONS(("rs_execArgs[%d] == %s\n", k, rs_execArgs[k]));
    }
    rs_execArgs[n] = (char *) NULL;

  } else if (!BEG_STRCASECMP(buff, "cut_chars ")) {
#ifdef CUTCHAR_OPTION
    rs_cutchars = Word(2, buff);
    chomp(rs_cutchars);
#else
    print_error("warning:  support for the cut_chars attribute was not compiled in, ignoring");
#endif

  } else {
    print_error("parse error in file %s, line %lu:  Attribute \"%s\" is not valid "
		"within context misc", file_peek_path(), file_peek_line(), buff);
  }
}

void
parse_pixmaps(char *buff)
{

  long w, h;
  char *w1, *h1;
  unsigned long n;

#ifdef PIXMAP_SUPPORT
  ASSERT(buff != NULL);

  if (!BEG_STRCASECMP(buff, "background ")) {

    if ((n = NumWords(buff) < 4) || (n > 6)) {
      char *tmp = PWord(2, buff);

      print_error("parse error in file %s, line %lu:  Invalid parameter list \"%s\" for attribute background",
		  file_peek_path(), file_peek_line(), (tmp ? tmp : ""));
      return;
    }
    w1 = PWord(2, buff);
    h1 = PWord(3, buff);
    w = strtol(w1, (char **) NULL, 0);
    h = strtol(h1, (char **) NULL, 0);
    if (w || h) {
      rs_pixmapScale = (char *) true_vals;
      Options |= Opt_pixmapScale;
      rs_pixmaps[pixmap_bg] = Word(4, buff);
      rs_pixmaps[pixmap_bg] = (char *) realloc(rs_pixmaps[pixmap_bg], strlen(rs_pixmaps[pixmap_bg]) + 9);
      strcat(rs_pixmaps[pixmap_bg], "@100x100");
    } else {
      rs_pixmaps[pixmap_bg] = Word(4, buff);
    }

  } else if (!BEG_STRCASECMP(buff, "icon ")) {
    rs_icon = Word(2, buff);

# ifdef PIXMAP_SCROLLBAR
  } else if (!BEG_STRCASECMP(buff, "scroll_up ")) {

    if ((n = NumWords(buff) < 4) || (n > 6)) {
      char *tmp = PWord(2, buff);

      print_error("parse error in file %s, line %lu:  Invalid parameter list \"%s\" for attribute "
		  "scroll_up", file_peek_path(), file_peek_line(), (tmp ? tmp : ""));
      return;
    }
    w1 = PWord(2, buff);
    h1 = PWord(3, buff);
    w = strtol(w1, (char **) NULL, 0);
    h = strtol(h1, (char **) NULL, 0);
    if (w || h) {
      rs_pixmaps[pixmap_up] = Word(4, buff);
      rs_pixmaps[pixmap_up] = (char *) realloc(rs_pixmaps[pixmap_up], strlen(rs_pixmaps[pixmap_up]) + 9);
      strcat(rs_pixmaps[pixmap_up], "@100x100");
    } else {
      rs_pixmaps[pixmap_up] = Word(4, buff);
    }
  } else if (!BEG_STRCASECMP(buff, "scroll_up_clicked ")) {

    if ((n = NumWords(buff) < 4) || (n > 6)) {
      char *tmp = PWord(2, buff);

      print_error("parse error in file %s, line %lu:  Invalid parameter list \"%s\" for attribute "
		  "scroll_up_clicked", file_peek_path(), file_peek_line(), (tmp ? tmp : ""));
      return;
    }
    w1 = PWord(2, buff);
    h1 = PWord(3, buff);
    w = strtol(w1, (char **) NULL, 0);
    h = strtol(h1, (char **) NULL, 0);
    if (w || h) {
      rs_pixmaps[pixmap_upclk] = Word(4, buff);
      rs_pixmaps[pixmap_upclk] = (char *) realloc(rs_pixmaps[pixmap_upclk], strlen(rs_pixmaps[pixmap_upclk]) + 9);
      strcat(rs_pixmaps[pixmap_upclk], "@100x100");
    } else {
      rs_pixmaps[pixmap_upclk] = Word(4, buff);
    }
  } else if (!BEG_STRCASECMP(buff, "scroll_down ")) {

    if ((n = NumWords(buff) < 4) || (n > 6)) {
      char *tmp = PWord(2, buff);

      print_error("parse error in file %s, line %lu:  Invalid parameter list \"%s\" for attribute "
		  "scroll_down", file_peek_path(), file_peek_line(), (tmp ? tmp : ""));
      return;
    }
    w1 = PWord(2, buff);
    h1 = PWord(3, buff);
    w = strtol(w1, (char **) NULL, 0);
    h = strtol(h1, (char **) NULL, 0);
    if (w || h) {
      rs_pixmaps[pixmap_dn] = Word(4, buff);
      rs_pixmaps[pixmap_dn] = (char *) realloc(rs_pixmaps[pixmap_dn], strlen(rs_pixmaps[pixmap_dn]) + 9);
      strcat(rs_pixmaps[pixmap_dn], "@100x100");
    } else {
      rs_pixmaps[pixmap_dn] = Word(4, buff);
    }
  } else if (!BEG_STRCASECMP(buff, "scroll_down_clicked ")) {

    if ((n = NumWords(buff) < 4) || (n > 6)) {
      char *tmp = PWord(2, buff);

      print_error("parse error in file %s, line %lu:  Invalid parameter list \"%s\" for attribute "
		  "scroll_down_clicked", file_peek_path(), file_peek_line(), (tmp ? tmp : ""));
      return;
    }
    w1 = PWord(2, buff);
    h1 = PWord(3, buff);
    w = strtol(w1, (char **) NULL, 0);
    h = strtol(h1, (char **) NULL, 0);
    if (w || h) {
      rs_pixmaps[pixmap_dnclk] = Word(4, buff);
      rs_pixmaps[pixmap_dnclk] = (char *) realloc(rs_pixmaps[pixmap_dnclk], strlen(rs_pixmaps[pixmap_dnclk]) + 9);
      strcat(rs_pixmaps[pixmap_dnclk], "@100x100");
    } else {
      rs_pixmaps[pixmap_dnclk] = Word(4, buff);
    }
  } else if (!BEG_STRCASECMP(buff, "scroll_background ")) {

    if ((n = NumWords(buff) < 4) || (n > 6)) {
      char *tmp = PWord(2, buff);

      print_error("parse error in file %s, line %lu:  Invalid parameter list \"%s\" for attribute "
		  "scroll_background", file_peek_path(), file_peek_line(), (tmp ? tmp : ""));
      return;
    }
    w1 = PWord(2, buff);
    h1 = PWord(3, buff);
    w = strtol(w1, (char **) NULL, 0);
    h = strtol(h1, (char **) NULL, 0);
    if (w || h) {
      rs_pixmaps[pixmap_sb] = Word(4, buff);
      rs_pixmaps[pixmap_sb] = (char *) realloc(rs_pixmaps[pixmap_sb], strlen(rs_pixmaps[pixmap_sb]) + 9);
      strcat(rs_pixmaps[pixmap_sb], "@100x100");
    } else {
      rs_pixmaps[pixmap_sb] = Word(4, buff);
    }
  } else if (!BEG_STRCASECMP(buff, "scroll_anchor ")) {

    if ((n = NumWords(buff) < 4) || (n > 6)) {
      char *tmp = PWord(2, buff);

      print_error("parse error in file %s, line %lu:  Invalid parameter list \"%s\" for attribute "
		  "scroll_anchor", file_peek_path(), file_peek_line(), (tmp ? tmp : ""));
      return;
    }
    w1 = PWord(2, buff);
    h1 = PWord(3, buff);
    w = strtol(w1, (char **) NULL, 0);
    h = strtol(h1, (char **) NULL, 0);
    if (w || h) {
      rs_pixmaps[pixmap_sa] = Word(4, buff);
      rs_pixmaps[pixmap_sa] = (char *) realloc(rs_pixmaps[pixmap_sa], strlen(rs_pixmaps[pixmap_sa]) + 9);
      strcat(rs_pixmaps[pixmap_sa], "@100x100");
    } else {
      rs_pixmaps[pixmap_sa] = Word(4, buff);
    }
  } else if (!BEG_STRCASECMP(buff, "scroll_anchor_clicked ")) {

    if ((n = NumWords(buff) < 4) || (n > 6)) {
      char *tmp = PWord(2, buff);

      print_error("parse error in file %s, line %lu:  Invalid parameter list \"%s\" for attribute "
		  "scroll_anchor_clicked", file_peek_path(), file_peek_line(), (tmp ? tmp : ""));
      return;
    }
    w1 = PWord(2, buff);
    h1 = PWord(3, buff);
    w = strtol(w1, (char **) NULL, 0);
    h = strtol(h1, (char **) NULL, 0);
    if (w || h) {
      rs_pixmaps[pixmap_saclk] = Word(4, buff);
      rs_pixmaps[pixmap_saclk] = (char *) realloc(rs_pixmaps[pixmap_saclk], strlen(rs_pixmaps[pixmap_saclk]) + 9);
      strcat(rs_pixmaps[pixmap_saclk], "@100x100");
    } else {
      rs_pixmaps[pixmap_saclk] = Word(4, buff);
    }
# endif				/* PIXMAP_SCROLLBAR */
# ifdef PIXMAP_MENUBAR
  } else if (!BEG_STRCASECMP(buff, "menu_background ")) {

    if ((n = NumWords(buff) < 4) || (n > 6)) {
      char *tmp = PWord(2, buff);

      print_error("parse error in file %s, line %lu:  Invalid parameter list \"%s\" for attribute "
		  "menu_background", file_peek_path(), file_peek_line(), (tmp ? tmp : ""));
      return;
    }
    w1 = PWord(2, buff);
    h1 = PWord(3, buff);
    w = strtol(w1, (char **) NULL, 0);
    h = strtol(h1, (char **) NULL, 0);
    if (w || h) {
      rs_pixmaps[pixmap_mb] = Word(4, buff);
      rs_pixmaps[pixmap_mb] = (char *) realloc(rs_pixmaps[pixmap_mb], strlen(rs_pixmaps[pixmap_mb]) + 9);
      strcat(rs_pixmaps[pixmap_mb], "@100x100");
    } else {
      rs_pixmaps[pixmap_mb] = Word(4, buff);
    }
  } else if (!BEG_STRCASECMP(buff, "menu_selected ")) {

    if ((n = NumWords(buff) < 4) || (n > 6)) {
      char *tmp = PWord(2, buff);

      print_error("parse error in file %s, line %lu:  Invalid parameter list \"%s\" for attribute "
		  "menu_selected", file_peek_path(), file_peek_line(), (tmp ? tmp : ""));
      return;
    }
    w1 = PWord(2, buff);
    h1 = PWord(3, buff);
    w = strtol(w1, (char **) NULL, 0);
    h = strtol(h1, (char **) NULL, 0);
    if (w || h) {
      rs_pixmaps[pixmap_ms] = Word(4, buff);
      rs_pixmaps[pixmap_ms] = (char *) realloc(rs_pixmaps[pixmap_ms], strlen(rs_pixmaps[pixmap_ms]) + 9);
      strcat(rs_pixmaps[pixmap_ms], "@100x100");
    } else {
      rs_pixmaps[pixmap_ms] = Word(4, buff);
    }
# endif				/* PIXMAP_MENUBAR */

  } else if (!BEG_STRCASECMP(buff, "path ")) {
    rs_path = Word(2, buff);

  } else if (!BEG_STRCASECMP(buff, "anim ")) {
# ifdef BACKGROUND_CYCLING_SUPPORT
    char *tmp = PWord(2, buff);

    if (tmp) {
      rs_anim_pixmap_list = strdup(tmp);
    } else {
      print_error("parse error in file %s, line %lu:  Invalid parameter list \"\" for attribute anim", file_peek_path(), file_peek_line());
    }
# else
    print_error("warning:  support for the anim attribute was not compiled in, ignoring");
# endif

  } else {
    print_error("parse error in file %s, line %lu:  Attribute \"%s\" is not valid "
		"within context pixmaps", file_peek_path(), file_peek_line(), buff);
  }
#else
  print_error("warning:  pixmap support was not compiled in, ignoring entire context");
  file_poke_skip(1);
#endif
}

void
parse_kanji(char *buff)
{

#ifdef KANJI
  ASSERT(buff != NULL);

  if (!BEG_STRCASECMP(buff, "encoding ")) {
    if ((rs_kanji_encoding = Word(2, buff)) != NULL) {
      if (BEG_STRCASECMP(rs_kanji_encoding, "eucj") && BEG_STRCASECMP(rs_kanji_encoding, "sjis")) {
	print_error("parse error in file %s, line %lu:  Invalid kanji encoding mode \"%s\"",
		    file_peek_path(), file_peek_line(), rs_kanji_encoding);
	return;
      }
      set_kanji_encoding(rs_kanji_encoding);
    } else {
      print_error("parse error in file %s, line %lu:  Invalid parameter list \"\" for attribute encoding", file_peek_path(), file_peek_line());
    }
  } else if (!BEG_STRCASECMP(buff, "font ")) {

    char *tmp = PWord(2, buff);
    unsigned char n;

    if (NumWords(buff) != 3) {
      print_error("parse error in file %s, line %lu:  Invalid parameter list \"%s\" for "
		  "attribute font", file_peek_path(), file_peek_line(), (tmp ? tmp : ""));
      return;
    }
    if (isdigit(*tmp)) {
      n = (unsigned char) strtoul(tmp, (char **) NULL, 0);
      if (n <= 4) {
	rs_kfont[n] = Word(2, tmp);
      } else {
	print_error("parse error in file %s, line %lu:  Invalid font index %d",
		    file_peek_path(), file_peek_line(), n);
      }
    } else {
      tmp = Word(1, tmp);
      print_error("parse error in file %s, line %lu:  Invalid font index \"%s\"",
		  file_peek_path(), file_peek_line(), (tmp ? tmp : ""));
      free(tmp);
    }
  } else {
    print_error("parse error in file %s, line %lu:  Attribute \"%s\" is not valid "
		"within context kanji", file_peek_path(), file_peek_line(), buff);
  }
#else
  print_error("warning:  kanji support was not compiled in, ignoring entire context");
  file_poke_skip(1);
#endif
}

void
parse_undef(char *buff)
{

  ASSERT(buff != NULL);

  print_error("parse error in file %s, line %lu:  Undefined subcontext \"%s\" within context %s",
	      file_peek_path(), file_peek_line(), PWord(2, buff),
	      ctx_id_to_name(ctx_peek()));
  file_poke_skip(1);

}

/* The config file reader.  This loads a file called MAIN, which is
   located by searching CONFIG_SEARCH_PATH, PATH_ENV, and $PATH.  If 
   it can't find a config file, it displays a warning but continues.
   -- mej */

#ifdef USE_THEMES
int
find_theme(char *path, char *name)
{

  register char *search_path = strdup(path);
  register char *cur_path, *conf_name;
  int ver;
  char buff[256];
  char *theme_root, *end_ptr;
  char working_dir[PATH_MAX + 1];

  if (!name)
    return (0);

  D_OPTIONS(("Searching for theme %s\n", name));

  for (cur_path = strtok(search_path, ":"); !file_peek_fp() && cur_path;
       cur_path = strtok((char *) NULL, ":")) {

    D_OPTIONS(("cur_path == %s\n", cur_path));
    if (!BEG_STRCASECMP(cur_path, "~/")) {
      chdir(getenv("HOME"));
      cur_path += 2;
    }
    if (chdir(cur_path)) {
      continue;
    }
    getcwd(working_dir, PATH_MAX);
    D_OPTIONS(("cur_path == %s   wd == %s\n", cur_path, working_dir));
    if (chdir(name)) {
      continue;
    }
    if (rs_config_file) {
      conf_name = rs_config_file;
    } else {
      conf_name = CONFIG_FILE_NAME;
    }
    file_poke_fp(fopen(conf_name, "rt"));
    if (file_peek_fp()) {
      fgets(buff, 256, file_peek_fp());
      D_OPTIONS(("Magic line \"%s\" [%s]  VERSION == \"%s\"  size == %lu\n",
		 buff, buff + 7, VERSION, sizeof(VERSION) - 1));
      if (BEG_STRCASECMP(buff, "<Eterm-")) {
	file_poke_fp(NULL);
      } else {
	if ((end_ptr = strchr(buff, '>')) != NULL) {
	  *end_ptr = 0;
	}
	if ((ver = BEG_STRCASECMP(buff + 7, VERSION)) > 0) {
	  print_error("warning:  config file is designed for a newer version of Eterm");
#ifdef WARN_OLDER
	} else if (ver < 0) {
	  print_error("warning:  config file is designed for an older version of Eterm");
#endif
	}
	theme_root = (char *) MALLOC(strlen(working_dir) + strlen(cur_path) + strlen(name) + 16 + 1);
	sprintf(theme_root, "ETERM_THEME_ROOT=%s/%s", working_dir, name);
	putenv(theme_root);
	D_OPTIONS(("%s\n", theme_root));
      }
    }
  }
  return ((int) file_peek_fp());
}

#endif

int
find_config_file(char *path, char *name)
{

  register char *search_path = strdup(path);
  register char *cur_path;
  int ver;
  char buff[256], *end_ptr;

#if DEBUG >= DEBUG_OPTIONS
  char *pwd2;

#endif

  if (!name)
    return (0);

  D_OPTIONS(("Searching for config file %s\n", name));

  for (cur_path = strtok(search_path, ":"); !file_peek_fp() && cur_path;
       cur_path = strtok((char *) NULL, ":")) {

    D_OPTIONS(("cur_path == %s\n", cur_path));
    if (!BEG_STRCASECMP(cur_path, "~/")) {
      chdir(getenv("HOME"));
      cur_path += 2;
    }
    chdir(cur_path);
#if DEBUG >= DEBUG_OPTIONS
    if (debug_level >= DEBUG_OPTIONS) {
      pwd2 = (char *) malloc(2048);
      getcwd(pwd2, 2048);
      DPRINTF(("cur_path == %s   wd == %s\n", cur_path, pwd2));
      free(pwd2);
    }
#endif

    file_poke_fp(fopen(name, "rt"));
    if (file_peek_fp()) {
      fgets(buff, 256, file_peek_fp());
      D_OPTIONS(("Magic line \"%s\" [%s]  VERSION == \"%s\"  size == %lu\n",
		 buff, buff + 7, VERSION, sizeof(VERSION) - 1));
      if (BEG_STRCASECMP(buff, "<Eterm-")) {
	file_poke_fp(NULL);
      } else {
	if ((end_ptr = strchr(buff, '>')) != NULL) {
	  *end_ptr = 0;
	}
	if ((ver = BEG_STRCASECMP(buff + 7, VERSION)) > 0) {
	  print_error("warning:  config file is designed for a newer version of Eterm");
#ifdef WARN_OLDER
	} else if (ver < 0) {
	  print_error("warning:  config file is designed for an older version of Eterm");
#endif
	}
      }
    }
  }
  return ((int) file_peek_fp());
}

void
read_config(void)
{

  register char *search_path;
  char *env_path, *path_env, *desc;
  char buff[CONFIG_BUFF];
  char *conf_name, *end_ptr;
  register unsigned long i = 0;
  register char *loop_path;	/* -nf */
  register char *cur_path;	/* -nf */
  int ver;
  unsigned char id;
  file_state fs =
  {NULL, CONFIG_FILE_NAME, 1, 0};

  env_path = getenv("PATH");
  if (env_path)
    i += strlen(env_path);
  path_env = getenv(PATH_ENV);
  if (path_env)
    i += strlen(path_env);
  i += sizeof(CONFIG_SEARCH_PATH) + 3;
  search_path = (char *) MALLOC(i);
  strcpy(search_path, CONFIG_SEARCH_PATH);
  if (path_env) {
    strcat(search_path, ":");
    strcat(search_path, path_env);
  }
  if (env_path) {
    strcat(search_path, ":");
    strcat(search_path, env_path);
  }
  file_push(fs);

  getcwd(initial_dir, PATH_MAX);

#ifdef USE_THEMES
  if (rs_config_file) {
    conf_name = rs_config_file;
  } else {
    conf_name = CONFIG_FILE_NAME;
  }
  if (!find_theme(search_path, rs_theme)) {
    if (!find_theme(search_path, "Eterm")) {
      if (!find_theme(search_path, "DEFAULT")) {
	find_config_file(search_path, conf_name);
	FREE(rs_theme);
	rs_theme = NULL;
      }
    }
  }
#else
  conf_name = CONFIG_FILE_NAME;
  find_config_file(search_path, conf_name);
#endif

  /* Modifications by NFin8 to print out search path when unable to find the configuration
     file. Done 2/17/98 -nf */
  loop_path = strdup(search_path);
  if (!file_peek_fp()) {
    print_error("unable to find/open config file %s, I looked in:", conf_name);
    for (cur_path = strtok(loop_path, ":"); cur_path; cur_path = strtok((char *) NULL, ":")) {
      fprintf(stderr, "\t%s\n", cur_path);
    }
    print_error("...continuing with defaults");
    FREE(search_path);
    return;
  }
  FREE(search_path);

  file_poke_path(conf_name);
  ctx_peek() = 0;		/* <== This looks weird, but works -- mej */
  for (; cur_file >= 0;) {
    for (; fgets(buff, CONFIG_BUFF, file_peek_fp());) {
      file_inc_line();
      if (!strchr(buff, '\n')) {
	print_error("parse error in file %s, line %lu:  line too long", file_peek_path(), file_peek_line());
	for (; fgets(buff, CONFIG_BUFF, file_peek_fp()) && !strrchr(buff, '\n'););
	continue;
      }
      chomp(buff);
      switch (*buff) {
	case '#':
	case 0:
	  break;
	case '%':
	  if (!BEG_STRCASECMP(PWord(1, buff + 1), "include ")) {
	    shell_expand(buff);
	    fs.path = Word(2, buff + 1);
	    fs.line = 1;
	    fs.skip_to_end = 0;
	    if ((fs.fp = fopen(fs.path, "rt")) == NULL) {
	      print_error("I/O error in file %s, line %lu:  Unable to open %%include file %s "
			  "(%s), continuing", file_peek_path(), file_peek_line(), fs.path,
			  strerror(errno));
	    } else {
	      file_push(fs);
	      fgets(buff, CONFIG_BUFF, file_peek_fp());
	      chomp(buff);
	      D_OPTIONS(("Magic line \"%s\" [%s]  VERSION == \"%s\" "
			 "size == %lu\n", buff, buff + 7, VERSION, sizeof(VERSION) - 1));
	      if ((end_ptr = strchr(buff, '>')) != NULL) {
		*end_ptr = 0;
	      }
	      if (BEG_STRCASECMP(buff, "<Eterm-")) {
		print_error("parse error in file %s:  Not a valid Eterm config file.  "
			    "This file will be ignored.", file_peek_path());
		file_pop();
	      } else {
		if ((end_ptr = strchr(buff, '>')) != NULL) {
		  *end_ptr = 0;
		}
		if ((ver = BEG_STRCASECMP(buff + 7, VERSION)) > 0) {
		  print_error("warning:  %%included file %s is designed for a newer version of Eterm", file_peek_path());
#ifdef WARN_OLDER
		} else if (ver < 0) {
		  print_error("warning:  %%included file %s is designed for an older version of Eterm", file_peek_path());
#endif
		}
	      }
	    }
	  } else {
	    print_error("parse error in file %s, line %lu:  Undefined macro \"%s\"", file_peek_path(), file_peek_line(), buff);
	  }
	  break;
	case 'b':
	  if (file_peek_skip())
	    continue;
	  if (!BEG_STRCASECMP(buff, "begin ")) {
	    desc = PWord(2, buff);
	    ctx_name_to_id(id, desc, i);
	    if (id == CTX_UNDEF) {
	      parse_undef(buff);
	    } else {
	      if (ctx_peek()) {
		for (i = 0; i <= CTX_MAX; i++) {
		  if (contexts[ctx_peek()].valid_sub_contexts[i]) {
		    if (contexts[ctx_peek()].valid_sub_contexts[i] == id) {
		      ctx_push(id);
		      break;
		    }
		  } else {
		    print_error("parse error in file %s, line %lu:  subcontext %s is not valid "
				"within context %s", file_peek_path(), file_peek_line(),
				contexts[id].description, contexts[ctx_peek()].description);
		    break;
		  }
		}
	      } else if (id != CTX_MAIN) {
		print_error("parse error in file %s, line %lu:  subcontext %s is not valid "
			    "outside context main", file_peek_path(), file_peek_line(),
			    contexts[id].description);
		break;
	      } else {
		ctx_push(id);
	      }
	    }
	    break;
	  }
	case 'e':
	  if (!BEG_STRCASECMP(buff, "end ") || !strcasecmp(buff, "end")) {
	    ctx_pop();
	    file_poke_skip(0);
	    break;
	  }
	default:
	  if (file_peek_skip())
	    continue;
	  if ((id = ctx_peek())) {
	    shell_expand(buff);
	    (*ctx_id_to_func(id)) (buff);
	  } else {
	    print_error("parse error in file %s, line %lu:  No established context to parse \"%s\"",
			file_peek_path(), file_peek_line(), buff);
	  }
      }
    }
    file_pop();
  }
}

/* Initialize the default values for everything */
void
init_defaults(void)
{

  rs_name = APL_NAME " " VERSION;

#if DEBUG >= DEBUG_MALLOC
  if (debug_level >= DEBUG_MALLOC) {
    memrec_init();
  }
#endif

  Options = (Opt_scrollBar);
  Xdisplay = NULL;
  display_name = NULL;
  rs_term_name = NULL;
#ifdef CUTCHAR_OPTION
  rs_cutchars = NULL;
#endif
#ifndef NO_BOLDFONT
  rs_boldFont = NULL;
#endif
#ifdef PRINTPIPE
  rs_print_pipe = NULL;
#endif
  rs_title = NULL;		/* title name for window */
  rs_iconName = NULL;		/* icon name for window */
  rs_geometry = NULL;		/* window geometry */

#if (MENUBAR_MAX)
  rs_menu = NULL;
#endif
#ifdef PIXMAP_SUPPORT
  rs_path = NULL;
#endif
#ifndef NO_BRIGHTCOLOR
  colorfgbg = DEFAULT_RSTYLE;
#endif

  TermWin.internalBorder = DEFAULT_BORDER_WIDTH;

}

/* Sync up options with our internal data after parsing options and configs */
void
post_parse(void)
{

  register int i, count;

  if (rs_scrollbar_type) {
    if (!strcasecmp(rs_scrollbar_type, "xterm")) {
#ifdef XTERM_SCROLLBAR
      scrollBar.type = SCROLLBAR_XTERM;
#else
      print_error("Support for xterm scrollbars was not compiled in.  Sorry.");
#endif
    } else if (!strcasecmp(rs_scrollbar_type, "next")) {
#ifdef NEXT_SCROLLBAR
      scrollBar.type = SCROLLBAR_NEXT;
#else
      print_error("Support for NeXT scrollbars was not compiled in.  Sorry.");
#endif
    } else if (!strcasecmp(rs_scrollbar_type, "motif")) {
#ifdef MOTIF_SCROLLBAR
      scrollBar.type = SCROLLBAR_MOTIF;
#else
      print_error("Support for motif scrollbars was not compiled in.  Sorry.");
#endif
    } else {
      print_error("Unrecognized scrollbar type \"%s\".", rs_scrollbar_type);
    }
  }
  if (rs_scrollbar_width) {
    scrollBar.width = rs_scrollbar_width;
  }
  if (scrollBar.type == SCROLLBAR_XTERM) {
    sb_shadow = 0;
  } else {
    sb_shadow = (Options & Opt_scrollBar_floating) ? 0 : SHADOW;
  }

  /* set any defaults not already set */
  if (!rs_title)
    rs_title = rs_name;
  if (!rs_iconName)
    rs_iconName = rs_name;
  if ((TermWin.saveLines = rs_saveLines) < 0) {
    TermWin.saveLines = SAVELINES;
  }
  /* no point having a scrollbar without having any scrollback! */
  if (!TermWin.saveLines)
    Options &= ~Opt_scrollBar;

#ifdef PRINTPIPE
  if (!rs_print_pipe)
    rs_print_pipe = PRINTPIPE;
#endif
#ifdef CUTCHAR_OPTION
  if (!rs_cutchars)
    rs_cutchars = CUTCHARS;
#endif

#ifndef NO_BOLDFONT
  if (rs_font[0] == NULL && rs_boldFont != NULL) {
    rs_font[0] = rs_boldFont;
    rs_boldFont = NULL;
  }
#endif
  for (i = 0; i < NFONTS; i++) {
    if (!rs_font[i])
      rs_font[i] = def_fontName[i];
#ifdef KANJI
    if (!rs_kfont[i]) {
      rs_kfont[i] = def_kfontName[i];
    }
#endif
  }

#ifdef XTERM_REVERSE_VIDEO
  /* this is how xterm implements reverseVideo */
  if (Options & Opt_reverseVideo) {
    if (!rs_color[fgColor])
      rs_color[fgColor] = def_colorName[bgColor];
    if (!rs_color[bgColor])
      rs_color[bgColor] = def_colorName[fgColor];
  }
#endif

  for (i = 0; i < NRS_COLORS; i++) {
    if (!rs_color[i])
      rs_color[i] = def_colorName[i];
  }

#ifdef CHANGE_SCOLLCOLOR_ON_FOCUS
  /* If they don't set the unfocused color, use the scrollbar color to "turn the option off" */
  if (!rs_color[unfocusedScrollColor]) {
    rs_color[unfocusedScrollColor] = rs_color[scrollColor];
  }
#endif

#ifndef XTERM_REVERSE_VIDEO
  /* this is how we implement reverseVideo */
  if (Options & Opt_reverseVideo) {
    char *tmp;

    /* swap foreground/background colors */

    tmp = rs_color[fgColor];
    rs_color[fgColor] = rs_color[bgColor];
    rs_color[bgColor] = tmp;

    tmp = def_colorName[fgColor];
    def_colorName[fgColor] = def_colorName[bgColor];
    def_colorName[bgColor] = tmp;
  }
#endif

  /* convenient aliases for setting fg/bg to colors */
  color_aliases(fgColor);
  color_aliases(bgColor);
#ifndef NO_CURSORCOLOR
  color_aliases(cursorColor);
  color_aliases(cursorColor2);
#endif /* NO_CURSORCOLOR */
#ifndef NO_BOLDUNDERLINE
  color_aliases(colorBD);
  color_aliases(colorUL);
#endif /* NO_BOLDUNDERLINE */
  color_aliases(pointerColor);
  color_aliases(borderColor);

  /* add startup-menu: */
#if (MENUBAR_MAX)
  /* In a borderless Eterm where the user hasn't said otherwise, make the menubar move the window */
  if (Options & Opt_borderless && rs_menubar_move == NULL) {
    Options |= Opt_menubar_move;
  }
  delay_menu_drawing = 1;
  menubar_read(rs_menu);
  delay_menu_drawing--;
  if (rs_menubar == *false_vals) {
    delay_menu_drawing = 0;
  }
#else
  delay_menu_drawing = 0;
#endif

#ifdef BACKGROUND_CYCLING_SUPPORT
  if (rs_anim_pixmap_list != NULL) {
    rs_anim_delay = strtoul(rs_anim_pixmap_list, (char **) NULL, 0);
    fflush(stdout);
    if (rs_anim_delay == 0) {
      free(rs_anim_pixmap_list);
      rs_anim_pixmap_list = NULL;
    } else {
      char *w1, *h1, *temp;
      unsigned long w, h;

      count = NumWords(rs_anim_pixmap_list) - 1;	/* -1 for the delay */
      rs_anim_pixmaps = (char **) MALLOC(sizeof(char *) * (count + 1));

      for (i = 0; i < count; i++) {
	temp = Word(i + 2, rs_anim_pixmap_list);	/* +2 rather than +1 to account for the delay */
	if (temp == NULL)
	  break;
	if (NumWords(temp) != 3) {
	  if (NumWords(temp) == 1) {
	    rs_anim_pixmaps[i] = temp;
	  }
	} else {
	  w1 = PWord(1, temp);
	  h1 = PWord(2, temp);
	  w = strtol(w1, (char **) NULL, 0);
	  h = strtol(h1, (char **) NULL, 0);
	  if (w || h) {
	    rs_anim_pixmaps[i] = Word(3, temp);
	    rs_anim_pixmaps[i] = (char *) realloc(rs_anim_pixmaps[i], strlen(rs_anim_pixmaps[i]) + 9);
	    strcat(rs_anim_pixmaps[i], ";100x100");
	  } else {
	    rs_anim_pixmaps[i] = Word(3, temp);
	    rs_anim_pixmaps[i] = (char *) realloc(rs_anim_pixmaps[i], strlen(rs_anim_pixmaps[i]) + 9);
	    strcat(rs_anim_pixmaps[i], ";0x0");
	  }
	  FREE(temp);
	}
      }
      rs_anim_pixmaps[count] = NULL;
      free(rs_anim_pixmap_list);
    }
  } else {
    rs_anim_delay = 0;
  }
#endif

}

unsigned char
save_config(char *path)
{

  register FILE *fp;
  register short i;
  char *tmp_str, dt_stamp[50];
  time_t cur_time = time(NULL);
  struct tm *cur_tm;
  struct stat fst;

  cur_tm = localtime(&cur_time);

  if (!path) {
    path = (char *) MALLOC(PATH_MAX + 1);
    snprintf(path, PATH_MAX, "%s/MAIN", getenv("ETERM_THEME_ROOT"));
    path[PATH_MAX] = 0;
  }
  if (!lstat(path, &fst)) {
    char bak_path[PATH_MAX], timestamp[16];

    /* File exists.  Make a backup. */
    strftime(timestamp, 16, "%Y%m%d.%H%M%S", cur_tm);
    snprintf(bak_path, PATH_MAX - 1, "%s.%s", path, timestamp);
    link(path, bak_path);
    unlink(path);
  }
  if ((fp = fopen(path, "w")) == NULL) {
    print_error("Unable to save configuration to file \"%s\" -- %s\n", path, strerror(errno));
    return errno;
  }
  strftime(dt_stamp, 50, "%x at %X", cur_tm);
  fprintf(fp, "<" APL_NAME "-" VERSION ">\n");
  fprintf(fp, "# Eterm Configuration File\n");
  fprintf(fp, "# Automatically generated by " APL_NAME "-" VERSION " on %s\n", dt_stamp);

  fprintf(fp, "begin main\n\n");

  fprintf(fp, "  begin color\n");
  fprintf(fp, "    foreground %s\n", rs_color[fgColor]);
  fprintf(fp, "    background %s\n", rs_color[bgColor]);
  fprintf(fp, "    tint 0x%06x\n", rs_tintMask);
  fprintf(fp, "    shade %lu%%\n", rs_shadePct);
  fprintf(fp, "    cursor %s\n", rs_color[cursorColor]);
  fprintf(fp, "    cursor_text %s\n", rs_color[cursorColor2]);
  fprintf(fp, "    menu_text %s\n", rs_color[menuTextColor]);
  fprintf(fp, "    scrollbar %s\n", rs_color[scrollColor]);
  fprintf(fp, "    unfocusedscrollbar %s\n", rs_color[unfocusedScrollColor]);
  fprintf(fp, "    pointer %s\n", rs_color[pointerColor]);
  fprintf(fp, "    video normal\n");
  for (i = 0; i < 16; i++) {
    fprintf(fp, "    color %d %s\n", i, rs_color[minColor + i]);
  }
  if (rs_color[colorBD]) {
    fprintf(fp, "    color bd %s\n", rs_color[colorBD]);
  }
  if (rs_color[colorUL]) {
    fprintf(fp, "    color ul %s\n", rs_color[colorUL]);
  }
  fprintf(fp, "  end color\n\n");

  fprintf(fp, "  begin attributes\n");
  if (rs_geometry) {
    fprintf(fp, "    geometry %s\n", rs_geometry);
  }
  XFetchName(Xdisplay, TermWin.parent, &tmp_str);
  fprintf(fp, "    title %s\n", tmp_str);
  fprintf(fp, "    name %s\n", rs_name);
  XGetIconName(Xdisplay, TermWin.parent, &tmp_str);
  fprintf(fp, "    iconname %s\n", tmp_str);
  if (rs_desktop != -1) {
    fprintf(fp, "    desktop %d\n", rs_desktop);
  }
  fprintf(fp, "    scrollbar_type %s\n", (scrollBar.type == SCROLLBAR_XTERM ? "xterm" : (scrollBar.type == SCROLLBAR_MOTIF ? "motif" : "next")));
  fprintf(fp, "    scrollbar_width %d\n", scrollBar.width);
  for (i = 0; i < 5; i++) {
    fprintf(fp, "    font %d %s\n", i, rs_font[i]);
  }
#ifndef NO_BOLDFONT
  if (rs_boldFont) {
    fprintf(fp, "    font bold %s\n", i, rs_boldFont);
  }
#endif
  fprintf(fp, "  end attributes\n\n");

  fprintf(fp, "  begin pixmaps\n");
  if (rs_pixmaps[pixmap_bg] && *rs_pixmaps[pixmap_bg]) {
    fprintf(fp, "    background %s %s\n", (Options & Opt_pixmapScale ? "-1 -1" : "0 0"), rs_pixmaps[pixmap_bg]);
  }
  if (rs_icon) {
    fprintf(fp, "    icon %s\n", rs_icon);
  }
  if (rs_path) {
    fprintf(fp, "    path \"%s\"\n", rs_path);
  }
  if (rs_anim_delay) {
    fprintf(fp, "    anim %d ", rs_anim_delay);
    for (i = 0; rs_anim_pixmaps[i]; i++) {
      fprintf(fp, "\"%s\" ", rs_anim_pixmaps[i]);
    }
    fprintf(fp, "\n");
  }
  fprintf(fp, "  end pixmaps\n\n");

#ifdef KANJI
  fprintf(fp, "  begin kanji\n");
  fprintf(fp, "    encoding %s\n", rs_kanji_encoding);
  for (i = 0; i < 5; i++) {
    fprintf(fp, "    font %d %s\n", i, rs_kfont[i]);
  }
  fprintf(fp, "  end kanji\n\n");
#endif

  fprintf(fp, "  begin toggles\n");
  fprintf(fp, "    map_alert %d\n", (Options & Opt_mapAlert ? 1 : 0));
  fprintf(fp, "    visual_bell %d\n", (Options & Opt_visualBell ? 1 : 0));
  fprintf(fp, "    login_shell %d\n", (Options & Opt_loginShell ? 1 : 0));
  fprintf(fp, "    scrollbar %d\n", (Options & Opt_scrollBar ? 1 : 0));
  fprintf(fp, "    menubar %d\n", (menuBar.state ? 1 : 0));
  fprintf(fp, "    utmp_logging %d\n", (Options & Opt_utmpLogging ? 1 : 0));
  fprintf(fp, "    meta8 %d\n", (Options & Opt_meta8 ? 1 : 0));
  fprintf(fp, "    iconic %d\n", (Options & Opt_iconic ? 1 : 0));
  fprintf(fp, "    home_on_echo %d\n", (Options & Opt_homeOnEcho ? 1 : 0));
  fprintf(fp, "    home_on_input %d\n", (Options & Opt_homeOnInput ? 1 : 0));
  fprintf(fp, "    home_on_refresh %d\n", (Options & Opt_homeOnRefresh ? 1 : 0));
  fprintf(fp, "    scrollbar_floating %d\n", (Options & Opt_scrollBar_floating ? 1 : 0));
  fprintf(fp, "    scrollbar_right %d\n", (Options & Opt_scrollBar_right ? 1 : 0));
  fprintf(fp, "    scrollbar_popup %d\n", (Options & Opt_scrollbar_popup ? 1 : 0));
  fprintf(fp, "    borderless %d\n", (Options & Opt_borderless ? 1 : 0));
  fprintf(fp, "    save_under %d\n", (Options & Opt_saveUnder ? 1 : 0));
  fprintf(fp, "    trans %d\n", (Options & Opt_pixmapTrans ? 1 : 0));
  fprintf(fp, "    watch_desktop %d\n", (Options & Opt_watchDesktop ? 1 : 0));
  fprintf(fp, "    no_cursor %d\n", (Options & Opt_noCursor ? 1 : 0));
  fprintf(fp, "    menubar_move %d\n", (Options & Opt_menubar_move ? 1 : 0));
  fprintf(fp, "    pause %d\n", (Options & Opt_pause ? 1 : 0));
  fprintf(fp, "    xterm_select %d\n", (Options & Opt_xterm_select ? 1 : 0));
  fprintf(fp, "    select_line %d\n", (Options & Opt_select_whole_line ? 1 : 0));
  fprintf(fp, "    select_trailing_spaces %d\n", (Options & Opt_select_trailing_spaces ? 1 : 0));
  fprintf(fp, "    viewport_mode %d\n", (Options & Opt_viewport_mode ? 1 : 0));
  fprintf(fp, "  end toggles\n\n");

  fprintf(fp, "  begin keyboard\n");
  tmp_str = XKeysymToString(ks_smallfont);
  if (tmp_str) {
    fprintf(fp, "    smallfont_key %s\n", tmp_str);
  }
  tmp_str = XKeysymToString(ks_bigfont);
  if (tmp_str) {
    fprintf(fp, "    bigfont_key %s\n", XKeysymToString(ks_bigfont));
  }
  for (i = 0; i < 256; i++) {
    if (KeySym_map[i]) {
      fprintf(fp, "    keysym 0xff%02x \"%s\"\n", i, (KeySym_map[i] + 1));
    }
  }
#ifdef GREEK_SUPPORT
  if (rs_greek_keyboard) {
    fprintf(fp, "    greek on %s\n", rs_greek_keyboard);
  }
#endif
  fprintf(fp, "    app_keypad %d\n", (PrivateModes & PrivMode_aplKP ? 1 : 0));
  fprintf(fp, "    app_cursor %d\n", (PrivateModes & PrivMode_aplCUR ? 1 : 0));
  fprintf(fp, "  end keyboard\n\n");

  fprintf(fp, "  begin misc\n");
#ifdef PRINTPIPE
  if (rs_print_pipe) {
    fprintf(fp, "    print_pipe \"%s\"\n", rs_print_pipe);
  }
#endif
  fprintf(fp, "    save_lines %d\n", rs_saveLines);
  fprintf(fp, "    min_anchor_size %d\n", rs_min_anchor_size);
  fprintf(fp, "    border_width %d\n", TermWin.internalBorder);
  fprintf(fp, "    menu %s\n", rs_menu);
  fprintf(fp, "    term_name %s\n", getenv("TERM"));
  fprintf(fp, "    debug %d\n", debug_level);
  if (Options & Opt_exec && rs_execArgs) {
    fprintf(fp, "    exec ");
    for (i = 0; rs_execArgs[i]; i++) {
      fprintf(fp, "'%s' ", rs_execArgs[i]);
    }
    fprintf(fp, "\n");
  }
#ifdef CUTCHAR_OPTIONS
  if (rs_cutchars) {
    fprintf(fp, "    cut_chars \"%s\"\n", rs_cutchars);
  }
#endif
  fprintf(fp, "  end misc\n\n");
  fprintf(fp, "end main\n");

  fclose(fp);
  return 0;
}
