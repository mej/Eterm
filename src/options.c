/*  options.c -- Eterm options module
 *            -- 25 July 1997, mej
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <X11/keysym.h>

#include "../libmej/debug.h"
#include "../libmej/mem.h"
#include "../libmej/strings.h"
#include "debug.h"
#include "actions.h"
#include "command.h"
#include "events.h"
#include "font.h"
#include "grkelot.h"
#include "startup.h"
#include "menus.h"
#include "options.h"
#include "pixmap.h"
#include "scrollbar.h"
#include "screen.h"
#include "system.h"
#include "term.h"
#include "windows.h"

/* local functions referenced */
char *chomp(char *);
void parse_main(char *);
void parse_color(char *);
void parse_attributes(char *);
void parse_toggles(char *);
void parse_keyboard(char *);
void parse_misc(char *);
void parse_imageclasses(char *);
void parse_image(char *);
void parse_actions(char *);
void parse_menu(char *);
void parse_menuitem(char *);
void parse_xim(char *);
void parse_multichar(char *);
void parse_undef(char *);
char *builtin_random(char *);
char *builtin_exec(char *);
char *builtin_version(char *);
char *builtin_appname(char *);

const char *true_vals[] =
{"1", "on", "true", "yes"};
const char *false_vals[] =
{"0", "off", "false", "no"};
/* This structure defines a context and its attributes */
struct context_struct {
  const unsigned char id;
  const char *description;
  void (*ctx_handler) (char *);
  const unsigned char valid_sub_contexts[CTX_MAX];
} contexts[] = {

  {
    CTX_NULL, "none", NULL, {
      CTX_MAIN, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }
  },
  {
    CTX_MAIN, "main", parse_main, {
      CTX_COLOR, CTX_ATTRIBUTES, CTX_TOGGLES, CTX_KEYBOARD,
      CTX_MISC, CTX_IMAGECLASSES, CTX_ACTIONS,
      CTX_MENU, CTX_XIM, CTX_MULTI_CHARSET, 0, 0, 0
    }
  },
  {
    CTX_COLOR, "color", parse_color, {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }
  },
  {
    CTX_ATTRIBUTES, "attributes", parse_attributes, {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }
  },
  {
    CTX_TOGGLES, "toggles", parse_toggles, {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }
  },
  {
    CTX_KEYBOARD, "keyboard", parse_keyboard, {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }
  },
  {
    CTX_MISC, "misc", parse_misc, {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }
  },
  {
    CTX_IMAGECLASSES, "imageclasses", parse_imageclasses, {
      CTX_IMAGE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }
  },
  {
    CTX_IMAGE, "image", parse_image, {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }
  },
  {
    CTX_ACTIONS, "actions", parse_actions, {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }
  },
  {
    CTX_MENU, "menu", parse_menu, {
      CTX_MENUITEM, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }
  },
  {
    CTX_MENUITEM, "menuitem", parse_menuitem, {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }
  },
  {
    CTX_XIM, "xim", parse_xim, {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }
  },
  {
    CTX_MULTI_CHARSET, "multichar", parse_multichar, {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }
  },
  {
    CTX_UNDEF, NULL, parse_undef, {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    }
  }

};
unsigned char id_stack[MAX_CTX_DEPTH];
unsigned short cur_ctx = 0;
file_state file_stack[MAX_FILE_DEPTH];
short cur_file = -1;
eterm_func builtins[] =
{
  {"random", builtin_random, -1},
  {"exec", builtin_exec, -1},
  {"version", builtin_version, 0},
  {"appname", builtin_appname, 0},
  {(char *) NULL, (eterm_function_ptr) NULL, 0}
};
unsigned long Options = (Opt_scrollBar), image_toggles = 0;
static menu_t *curmenu;
char *theme_dir = NULL, *user_dir = NULL;
char **rs_execArgs = NULL;	/* Args to exec (-e or --exec) */
char *rs_title = NULL;		/* Window title */
char *rs_iconName = NULL;	/* Icon name */
char *rs_geometry = NULL;	/* Geometry string */
int rs_desktop = -1;
char *rs_path = NULL;
int rs_saveLines = SAVELINES;	/* Lines in the scrollback buffer */
#ifdef USE_XIM
char *rs_inputMethod = NULL;
char *rs_preeditType = NULL;
#endif
char *rs_name = NULL;
#ifndef NO_BOLDFONT
char *rs_boldFont = NULL;
#endif
char *rs_font[NFONTS];
#ifdef MULTI_CHARSET
char *rs_mfont[NFONTS];
#endif
#ifdef PRINTPIPE
char *rs_print_pipe = NULL;
#endif
char *rs_cutchars = NULL;
unsigned short rs_min_anchor_size = 0;
char *rs_scrollbar_type = NULL;
unsigned long rs_scrollbar_width = 0;
char *rs_term_name = NULL;
#if defined (HOTKEY_CTRL) || defined (HOTKEY_META)
static char *rs_bigfont_key = NULL;
static char *rs_smallfont_key = NULL;
#endif
#ifdef MULTI_CHARSET
static char *rs_multchar_encoding = NULL;
#endif
#ifdef GREEK_SUPPORT
static char *rs_greek_keyboard = NULL;
#endif
#ifdef PIXMAP_SUPPORT
char *rs_pixmapScale = NULL;
const char *rs_backing_store = NULL;
char *rs_icon = NULL;
char *rs_cmod_image = NULL;
char *rs_cmod_red = NULL;
char *rs_cmod_green = NULL;
char *rs_cmod_blue = NULL;
# ifdef BACKGROUND_CYCLING_SUPPORT
char *rs_anim_pixmap_list = NULL;
char **rs_anim_pixmaps = NULL;
time_t rs_anim_delay = 0;
# endif
static char *rs_pixmaps[image_max];
#endif
char *rs_theme = NULL;
char *rs_config_file = NULL;
unsigned int rs_line_space = 0;
#ifdef KEYSYM_ATTRIBUTE
unsigned char *KeySym_map[256];	/* probably mostly empty */
#endif
#if defined (HOTKEY_CTRL) || defined (HOTKEY_META)
/* recognized when combined with HOTKEY */
KeySym ks_bigfont = XK_greater;
KeySym ks_smallfont = XK_less;
#endif

/* Options structure */
static const struct {
  char short_opt;
  char *long_opt;
  const char *const description;
  unsigned short flag;
  const void *pval;
  unsigned long *maskvar;
  int mask;
} optList[] = {

  OPT_STR('t', "theme", "select a theme", &rs_theme),
      OPT_STR('X', "config-file", "choose an alternate config file", &rs_config_file),
      OPT_STR('d', "display", "X server to connect to", &display_name),
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
      OPT_BLONG("install", "install a private colormap", &Options, Opt_install),

      OPT_BOOL('h', "help", "display usage information", NULL, 0),
      OPT_BLONG("version", "display version and configuration information", NULL, 0),

/* =======[ Color options ]======= */
      OPT_BOOL('r', "reverse-video", "reverse video", &Options, Opt_reverseVideo),
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
      OPT_LONG("menur-color", "menu color", &rs_color[menuColor]),
      OPT_LONG("menu-text-color", "menu text color", &rs_color[menuTextColor]),
      OPT_STR('S', "scrollbar-color", "scrollbar color", &rs_color[scrollColor]),
      OPT_LONG("unfocused-menu-color", "unfocused menu color", &rs_color[unfocusedMenuColor]),
      OPT_LONG("unfocused-scrollbar-color", "unfocused scrollbar color", &rs_color[unfocusedScrollColor]),
      OPT_LONG("pointer-color", "mouse pointer color", &rs_color[pointerColor]),
#ifndef NO_CURSORCOLOR
      OPT_STR('c', "cursor-color", "cursor color", &rs_color[cursorColor]),
      OPT_LONG("cursor-text-color", "cursor text color", &rs_color[cursorColor2]),
#endif /* NO_CURSORCOLOR */

/* =======[ X11 options ]======= */
      OPT_STR('g', "geometry", "WxH+X+Y = size and position", &rs_geometry),
      OPT_BOOL('i', "iconic", "start iconified", &Options, Opt_iconic),
      OPT_STR('n', "name", "client instance, icon, and title strings", &rs_name),
      OPT_STR('T', "title", "title string", &rs_title),
      OPT_LONG("icon-name", "icon name", &rs_iconName),
      OPT_STR('B', "scrollbar-type", "choose the scrollbar type (motif, next, xterm)", &rs_scrollbar_type),
      OPT_ILONG("scrollbar-width", "choose the width (in pixels) of the scrollbar", &rs_scrollbar_width),
      OPT_INT('D', "desktop", "desktop to start on (requires GNOME-compliant window manager)", &rs_desktop),
      OPT_ILONG("line-space", "number of extra dots between lines", &rs_line_space),
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
      OPT_STR('P', "background-pixmap", "background pixmap [scaling optional]", &rs_pixmaps[image_bg]),
      OPT_STR('I', "icon", "icon pixmap", &rs_icon),
      OPT_LONG("up-arrow-pixmap", "up arrow pixmap [scaling optional]", &rs_pixmaps[image_up]),
      OPT_LONG("down-arrow-pixmap", "down arrow pixmap [scaling optional]", &rs_pixmaps[image_down]),
      OPT_LONG("trough-pixmap", "scrollbar background (trough) pixmap [scaling optional]", &rs_pixmaps[image_sb]),
      OPT_LONG("anchor-pixmap", "scrollbar anchor pixmap [scaling optional]", &rs_pixmaps[image_sa]),
      OPT_LONG("menu-pixmap", "menu pixmap [scaling optional]", &rs_pixmaps[image_menu]),
      OPT_BOOL('O', "trans", "creates a pseudo-transparent Eterm", &image_toggles, IMOPT_TRANS),
      OPT_BLONG("viewport-mode", "use viewport mode for the background image", &image_toggles, IMOPT_VIEWPORT),
      OPT_LONG("cmod", "image color modifier (\"brightness contrast gamma\")", &rs_cmod_image),
      OPT_LONG("cmod-red", "red-only color modifier (\"brightness contrast gamma\")", &rs_cmod_red),
      OPT_LONG("cmod-green", "green-only color modifier (\"brightness contrast gamma\")", &rs_cmod_green),
      OPT_LONG("cmod-blue", "blue-only color modifier (\"brightness contrast gamma\")", &rs_cmod_blue),
      OPT_STR('p', "path", "pixmap file search path", &rs_path),
# ifdef BACKGROUND_CYCLING_SUPPORT
      OPT_STR('N', "anim", "a delay and list of pixmaps for cycling", &rs_anim_pixmap_list),
# endif				/* BACKGROUND_CYCLING_SUPPORT */
#endif /* PIXMAP_SUPPORT */

/* =======[ Kanji options ]======= */
#ifdef MULTI_CHARSET
      OPT_STR('M', "mfont", "normal text multichar font", &rs_mfont[0]),
      OPT_LONG("mfont1", "multichar font 1", &rs_mfont[1]),
      OPT_LONG("mfont2", "multichar font 2", &rs_mfont[2]),
      OPT_LONG("mfont3", "multichar font 3", &rs_mfont[3]),
      OPT_LONG("mfont4", "multichar font 4", &rs_mfont[4]),
      OPT_LONG("mencoding", "multichar encoding mode (eucj or sjis or euckr)",
	       &rs_multchar_encoding),
#endif /* MULTI_CHARSET */
#ifdef USE_XIM
      OPT_LONG("input-method", "XIM input method", &rs_inputMethod),
      OPT_LONG("preedit-type", "XIM preedit type", &rs_preeditType),
#endif

/* =======[ Toggles ]======= */
      OPT_BOOL('l', "login-shell", "login shell, prepend - to shell name", &Options, Opt_loginShell),
      OPT_BOOL('s', "scrollbar", "display scrollbar", &Options, Opt_scrollBar),
      OPT_BOOL('u', "utmp-logging", "make a utmp entry", &Options, Opt_utmpLogging),
      OPT_BOOL('v', "visual-bell", "visual bell", &Options, Opt_visualBell),
      OPT_BOOL('H', "home-on-echo", "jump to bottom on output", &Options, Opt_homeOnEcho),
      OPT_BLONG("home-on-input", "jump to bottom on input", &Options, Opt_homeOnInput),
      OPT_BOOL('E', "home-on-refresh", "jump to bottom on refresh (^L)", &Options, Opt_homeOnRefresh),
      OPT_BLONG("scrollbar-right", "display the scrollbar on the right", &Options, Opt_scrollBar_right),
      OPT_BLONG("scrollbar-floating", "display the scrollbar with no trough", &Options, Opt_scrollBar_floating),
      OPT_BLONG("scrollbar-popup", "popup the scrollbar only when focused", &Options, Opt_scrollbar_popup),
      OPT_BOOL('x', "borderless", "force Eterm to have no borders", &Options, Opt_borderless),
#ifndef NO_MAPALERT
# ifdef MAPALERT_OPTION
      OPT_BOOL('m', "map-alert", "uniconify on beep", &Options, Opt_mapAlert),
# endif
#endif
#ifdef META8_OPTION
      OPT_BOOL('8', "meta-8", "Meta key toggles 8-bit", &Options, Opt_meta8),
#endif
      OPT_BLONG("backing-store", "use backing store", &Options, Opt_backing_store),
      OPT_BLONG("no-cursor", "disable the text cursor", &Options, Opt_noCursor),
      OPT_BLONG("pause", "pause for a keypress after the child process exits", &Options, Opt_pause),
      OPT_BLONG("xterm-select", "duplicate xterm's broken selection behavior", &Options, Opt_xterm_select),
      OPT_BLONG("select-line", "triple-click selects whole line", &Options, Opt_select_whole_line),
      OPT_BLONG("select-trailing-spaces", "do not skip trailing spaces when selecting", &Options, Opt_select_trailing_spaces),
      OPT_BLONG("report-as-keysyms", "report special keys as keysyms", &Options, Opt_report_as_keysyms),

/* =======[ Keyboard options ]======= */
#if defined (HOTKEY_CTRL) || defined (HOTKEY_META)
      OPT_LONG("big-font-key", "keysym for font size increase", &rs_bigfont_key),
      OPT_LONG("small-font-key", "keysym for font size decrease", &rs_smallfont_key),
#endif
#ifdef GREEK_SUPPORT
      OPT_LONG("greek-keyboard", "greek keyboard mapping (iso or ibm)", &rs_greek_keyboard),
#endif
      OPT_BLONG("app-keypad", "application keypad mode", &PrivateModes, PrivMode_aplKP),
      OPT_BLONG("app-cursor", "application cursor key mode", &PrivateModes, PrivMode_aplCUR),

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
      OPT_LONG("term-name", "value to use for setting $TERM", &rs_term_name),
      OPT_BOOL('C', "console", "grab console messages", &Options, Opt_console),
      OPT_ARGS('e', "exec", "execute a command rather than a shell", &rs_execArgs)
};

/* Print usage information */
#define INDENT "5"
static void
usage(void)
{

  unsigned short i, col;

  printf("Eterm Enlightened Terminal Emulator for X Windows\n");
  printf("Copyright (c) 1997-1999, " AUTHORS "\n\n");
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
#if DEBUG >= DEBUG_MENU
  printf(" +DEBUG_MENU");
#endif
#if DEBUG >= DEBUG_TTYMODE
  printf(" +DEBUG_TTYMODE");
#endif
#if DEBUG >= DEBUG_COLORS
  printf(" +DEBUG_COLORS");
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
#ifdef BACKING_STORE
  printf(" +BACKING_STORE");
#else
  printf(" -BACKING_STORE");
#endif
#ifdef USE_EFFECTS
  printf(" +USE_EFFECTS");
#else
  printf(" -USE_EFFECTS");
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
#ifdef USE_XIM
  printf(" +XIM");
#else
  printf(" -XIM");
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
#ifdef SCROLLBAR_BUTTON_CONTINUAL_SCROLLING
  printf(" +SCROLLBAR_BUTTON_CONTINUAL_SCROLLING");
#else
  printf(" -SCROLLBAR_BUTTON_CONTINUAL_SCROLLING");
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
#ifdef MULTI_CHARSET
  printf(" +MULTI_CHARSET");
#else
  printf(" -MULTI_CHARSET");
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
#ifdef THEME_CFG
  printf(" THEME_CFG=\"%s\"\n", THEME_CFG);
#else
  printf(" -THEME_CFG\n");
#endif
#ifdef USER_CFG
  printf(" USER_CFG=\"%s\"\n", USER_CFG);
#else
  printf(" -USER_CFG\n");
#endif

  printf("\n");
  exit(EXIT_SUCCESS);
}

void
get_options(int argc, char *argv[])
{

  register unsigned long i, j, l;
  unsigned char bad_opts = 0;

  for (i = 1; i < (unsigned long) argc; i++) {

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
	    rs_execArgs[k] = StrDup(argv[k + i]);
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
	    } else if (BOOL_OPT_ISFALSE(val_ptr)) {
	      D_OPTIONS(("\"%s\" == FALSE\n", val_ptr));
	      if (optList[j].maskvar) {
		*(optList[j].maskvar) &= ~(optList[j].mask);
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
	  }
	} else if (optList[j].flag & OPT_INTEGER) {	/* Integer value */
	  D_OPTIONS(("Integer option detected\n"));
	  *((int *) optList[j].pval) = strtol(val_ptr, (char **) NULL, 0);
	} else {		/* String value */
	  D_OPTIONS(("String option detected\n"));
	  if (val_ptr && optList[j].pval) {
	    *((const char **) optList[j].pval) = StrDup(val_ptr);
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
	    rs_execArgs[0] = StrDup((char *) (val_ptr));
	    D_OPTIONS(("rs_execArgs[0] == %s\n", rs_execArgs[0]));
	    k++;
	  } else {
	    rs_execArgs[0] = StrDup(argv[k - 1]);
	    D_OPTIONS(("rs_execArgs[0] == %s\n", rs_execArgs[0]));
	  }
	  for (; k < argc; k++) {
	    rs_execArgs[k - i] = StrDup(argv[k]);
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
	  } else if (optList[j].flag & OPT_INTEGER) {	/* Integer value */
	    D_OPTIONS(("Integer option detected\n"));
	    *((int *) optList[j].pval) = strtol(val_ptr, (char **) NULL, 0);
	    D_OPTIONS(("Got value %d\n", *((int *) optList[j].pval)));
	  } else {		/* String value */
	    D_OPTIONS(("String option detected\n"));
	    if (optList[j].pval) {
	      *((const char **) optList[j].pval) = StrDup(val_ptr);
	    }
	  }			/* End if value type */
	}			/* End if option type */
      }				/* End if (exec or help or other) */
    }				/* End if (islong) */
  }				/* End main for loop */
}

void
get_initial_options(int argc, char *argv[])
{

  register unsigned long i, j;

  for (i = 1; i < (unsigned long) argc; i++) {

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
      } else if (!BEG_STRCASECMP(opt, "display")) {
	j = 2;
      } else if (!BEG_STRCASECMP(opt, "debug")) {
	j = 3;
      } else if (!BEG_STRCASECMP(opt, "install")) {
	j = 4;
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
      if (val_ptr == NULL && j != 4) {
	print_error("long option --%s requires a%s value", opt, (j == 3 ? "n integer" : " string"));
	continue;
      }
      if (j == 3) {
	D_OPTIONS(("Integer option detected\n"));
	*((int *) optList[j].pval) = strtol(val_ptr, (char **) NULL, 0);
      } else if (j == 4) {
	if (val_ptr) {
	  if (BOOL_OPT_ISTRUE(val_ptr)) {
	    D_OPTIONS(("\"%s\" == TRUE\n", val_ptr));
	    if (optList[j].maskvar) {
	      *(optList[j].maskvar) |= optList[j].mask;
	    }
	  } else if (BOOL_OPT_ISFALSE(val_ptr)) {
	    D_OPTIONS(("\"%s\" == FALSE\n", val_ptr));
	    if (optList[j].maskvar) {
	      *(optList[j].maskvar) &= ~(optList[j].mask);
	    }
	  }
	} else {		/* No value, so force it on */
	  D_OPTIONS(("Forcing option --%s to TRUE\n", opt));
	  if (optList[j].maskvar) {
	    *(optList[j].maskvar) |= optList[j].mask;
	  }
	}
      } else {
	D_OPTIONS(("String option detected\n"));
	if (val_ptr && optList[j].pval) {
	  *((const char **) optList[j].pval) = StrDup(val_ptr);
	}
      }
    } else {			/* It's a POSIX option */

      register unsigned short pos;
      unsigned char done = 0;

      for (pos = 1; opt[pos] && !done; pos++) {
	if (opt[pos] == 't') {
	  j = 0;
	} else if (opt[pos] == 'X') {
	  j = 1;
	} else if (opt[pos] == 'd') {
	  j = 2;
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
	  *((const char **) optList[j].pval) = StrDup(val_ptr);
	}
      }				/* End for loop */
    }				/* End if (islong) */
  }				/* End main for loop */
}

/***** The Config File Section *****/
char *
builtin_random(char *param)
{

  unsigned long n, index;
  static unsigned int rseed = 0;

  D_PARSE(("builtin_random(%s) called\n", param));

  if (rseed == 0) {
    rseed = (unsigned int) (getpid() * time(NULL) % ((unsigned int) -1));
    srand(rseed);
  }
  n = NumWords(param);
  index = (int) (n * ((float) rand()) / (RAND_MAX + 1.0)) + 1;
  D_PARSE(("random index == %lu\n", index));

  return (Word(index, param));
}

char *
builtin_exec(char *param)
{

  unsigned long fsize;
  char *Command, *Output = NULL, *OutFile;
  FILE *fp;

  D_PARSE(("builtin_exec(%s) called\n", param));

  Command = (char *) MALLOC(CONFIG_BUFF);
  OutFile = tmpnam(NULL);
  if (strlen(param) + strlen(OutFile) + 8 > CONFIG_BUFF) {
    print_error("Parse error in file %s, line %lu:  Cannot execute command, line too long",
                file_peek_path(), file_peek_line());
    return ((char *) NULL);
  }
  strcpy(Command, param);
  strcat(Command, " >");
  strcat(Command, OutFile);
  system(Command);
  if ((fp = fopen(OutFile, "rb")) != NULL) {
    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
    rewind(fp);
    if (fsize) {
      Output = (char *) MALLOC(fsize + 1);
      fread(Output, fsize, 1, fp);
      Output[fsize] = 0;
      fclose(fp);
      remove(OutFile);
      Output = CondenseWhitespace(Output);
    } else {
      print_warning("Command at line %lu of file %s returned no output.", file_peek_line(), file_peek_path());
    }
  } else {
    print_warning("Output file %s could not be created.  (line %lu of file %s)", NONULL(OutFile),
                  file_peek_line(), file_peek_path());
  }
  FREE(Command);

  return (Output);
}

char *
builtin_version(char *param)
{

  if (param) {
    D_PARSE(("builtin_version(%s) called\n", param));
  }

  return (StrDup(VERSION));
}

char *
builtin_appname(char *param)
{

  if (param) {
    D_PARSE(("builtin_appname(%s) called\n", param));
  }

  return (StrDup(APL_NAME "-" VERSION));
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
  register char *pbuff = s, *tmp1;
  register unsigned long j, k, l = 0;
  char new[CONFIG_BUFF];
  unsigned char eval_escape = 1, eval_var = 1, eval_exec = 1, eval_func = 1, in_single = 0, in_double = 0;
  unsigned long cnt1 = 0, cnt2 = 0;
  const unsigned long max = CONFIG_BUFF - 1;
  char *Command, *Output, *EnvVar;

  ASSERT_RVAL(s != NULL, (char *) NULL);

#if 0
  new = (char *) MALLOC(CONFIG_BUFF);
#endif

  for (j = 0; *pbuff && j < max; pbuff++, j++) {
    switch (*pbuff) {
      case '~':
	D_OPTIONS(("Tilde detected.\n"));
	if (eval_var) {
	  strncpy(new + j, getenv("HOME"), max - j);
	  cnt1 = strlen(getenv("HOME")) - 1;
          cnt2 = max - j - 1;
          j += MIN(cnt1, cnt2);
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
	  D_PARSE(("Checking for function %%%s, pbuff == \"%s\"\n", builtins[k].name, pbuff));
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
	    strncpy(new + j, Output, max - j);
            cnt2 = max - j - 1;
	    j += MIN(l, cnt2);
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
	  for (pbuff++; *pbuff && *pbuff != '`' && l < max; pbuff++, l++) {
            Command[l] = *pbuff;
	  }
          ASSERT(l < CONFIG_BUFF);
	  Command[l] = 0;
          Command = shell_expand(Command);
          Output = builtin_exec(Command);
	  FREE(Command);
	  if (Output && *Output) {
	    l = strlen(Output) - 1;
	    strncpy(new + j, Output, max - j);
            cnt2 = max - j - 1;
	    j += MIN(l, cnt2);
	    FREE(Output);
	  } else {
	    j--;
	  }
	} else {
	  new[j] = *pbuff;
	}
#else
	print_warning("Backquote execution support not compiled in, ignoring");
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
	    strncpy(new, tmp, max - j);
            cnt1 = strlen(tmp) - 1;
            cnt2 = max - j - 1;
	    j += MIN(cnt1, cnt2);
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
  ASSERT(j < CONFIG_BUFF);
  new[j] = 0;

  D_PARSE(("shell_expand(%s) returning \"%s\"\n", s, new));
  D_PARSE((" strlen(s) == %lu, strlen(new) == %lu, j == %lu\n", strlen(s), strlen(new), j));

  strcpy(s, new);
#if 0
  FREE(new);
#endif
  return (s);
}

/* The config file parsers.  Each function handles a given context. */

void
parse_main(char *buff)
{

  ASSERT(buff != NULL);

  print_error("Parse error in file %s, line %lu:  Attribute "
	      "\"%s\" is not valid within context main\n",
	      file_peek_path(), file_peek_line(), buff);
}

void
parse_color(char *buff)
{

  ASSERT(buff != NULL);

  if (!BEG_STRCASECMP(buff, "foreground ")) {
    RESET_AND_ASSIGN(rs_color[fgColor], Word(2, buff));
  } else if (!BEG_STRCASECMP(buff, "background ")) {
    RESET_AND_ASSIGN(rs_color[bgColor], Word(2, buff));

  } else if (!BEG_STRCASECMP(buff, "cursor ")) {

#ifndef NO_CURSORCOLOR
    RESET_AND_ASSIGN(rs_color[cursorColor], Word(2, buff));
#else
    print_warning("Support for the cursor attribute was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "cursor_text ")) {
#ifndef NO_CURSORCOLOR
    RESET_AND_ASSIGN(rs_color[cursorColor2], Word(2, buff));
#else
    print_warning("Support for the cursor_text attribute was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "menu ")) {
    RESET_AND_ASSIGN(rs_color[menuColor], Word(2, buff));

  } else if (!BEG_STRCASECMP(buff, "menu_text ")) {
    RESET_AND_ASSIGN(rs_color[menuTextColor], Word(2, buff));

  } else if (!BEG_STRCASECMP(buff, "scrollbar ")) {
    RESET_AND_ASSIGN(rs_color[scrollColor], Word(2, buff));

  } else if (!BEG_STRCASECMP(buff, "unfocused_menu ")) {
    RESET_AND_ASSIGN(rs_color[unfocusedMenuColor], Word(2, buff));

  } else if (!BEG_STRCASECMP(buff, "unfocused_scrollbar ")) {
    RESET_AND_ASSIGN(rs_color[unfocusedScrollColor], Word(2, buff));

  } else if (!BEG_STRCASECMP(buff, "pointer ")) {
    RESET_AND_ASSIGN(rs_color[pointerColor], Word(2, buff));

  } else if (!BEG_STRCASECMP(buff, "video ")) {

    char *tmp = PWord(2, buff);

    if (!BEG_STRCASECMP(tmp, "reverse")) {
      Options |= Opt_reverseVideo;
    } else if (BEG_STRCASECMP(tmp, "normal")) {
      print_error("Parse error in file %s, line %lu:  Invalid value \"%s\" for attribute video",
		  file_peek_path(), file_peek_line(), tmp);
    }
  } else if (!BEG_STRCASECMP(buff, "color ")) {

    char *tmp = 0, *r1, *g1, *b1;
    unsigned int n, r, g, b, index = 0;

    n = NumWords(buff);
    if (n < 3) {
      print_error("Parse error in file %s, line %lu:  Invalid parameter list \"%s\" for "
		  "attribute color", file_peek_path(), file_peek_line(), (tmp ? tmp : ""));
      return;
    }
    tmp = PWord(2, buff);
    r1 = PWord(3, buff);
    if (!isdigit(*r1)) {
      if (isdigit(*tmp)) {
	n = strtoul(tmp, (char **) NULL, 0);
	if (n <= 7) {
	  index = minColor + n;
	} else if (n >= 8 && n <= 15) {
	  index = minBright + n - 8;
	}
	RESET_AND_ASSIGN(rs_color[index], Word(1, r1));
	return;
      } else {
	if (!BEG_STRCASECMP(tmp, "bd ")) {
#ifndef NO_BOLDUNDERLINE
	  RESET_AND_ASSIGN(rs_color[colorBD], Word(1, r1));
#else
	  print_warning("Support for the color bd attribute was not compiled in, ignoring");
#endif
	  return;
	} else if (!BEG_STRCASECMP(tmp, "ul ")) {
#ifndef NO_BOLDUNDERLINE
	  RESET_AND_ASSIGN(rs_color[colorUL], Word(1, r1));
#else
	  print_warning("Support for the color ul attribute was not compiled in, ignoring");
#endif
	  return;
	} else {
	  tmp = Word(1, tmp);
	  print_error("Parse error in file %s, line %lu:  Invalid color index \"%s\"",
		      file_peek_path(), file_peek_line(), (tmp ? tmp : ""));
	  FREE(tmp);
	}
      }
    }
    if (n != 5) {
      print_error("Parse error in file %s, line %lu:  Invalid parameter list \"%s\" for "
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
      if (n <= 7) {
	index = minColor + n;
	RESET_AND_ASSIGN(rs_color[index], MALLOC(14));
	sprintf(rs_color[index], "#%02x%02x%02x", r, g, b);
      } else if (n >= 8 && n <= 15) {
	index = minBright + n - 8;
	RESET_AND_ASSIGN(rs_color[index], MALLOC(14));
	sprintf(rs_color[index], "#%02x%02x%02x", r, g, b);
      } else {
	print_error("Parse error in file %s, line %lu:  Invalid color index %lu",
		    file_peek_path(), file_peek_line(), n);
      }

    } else if (!BEG_STRCASECMP(tmp, "bd ")) {
#ifndef NO_BOLDUNDERLINE
      RESET_AND_ASSIGN(rs_color[colorBD], MALLOC(14));
      r = strtoul(r1, (char **) NULL, 0);
      g = strtoul(g1, (char **) NULL, 0);
      b = strtoul(b1, (char **) NULL, 0);
      sprintf(rs_color[colorBD], "#%02x%02x%02x", r, g, b);
#else
      print_warning("Support for the color bd attribute was not compiled in, ignoring");
#endif

    } else if (!BEG_STRCASECMP(tmp, "ul ")) {
#ifndef NO_BOLDUNDERLINE
      RESET_AND_ASSIGN(rs_color[colorUL], MALLOC(14));
      r = strtoul(r1, (char **) NULL, 0);
      g = strtoul(g1, (char **) NULL, 0);
      b = strtoul(b1, (char **) NULL, 0);
      sprintf(rs_color[colorUL], "#%02x%02x%02x", r, g, b);
#else
      print_warning("Support for the color ul attribute was not compiled in, ignoring");
#endif

    } else {
      tmp = Word(1, tmp);
      print_error("Parse error in file %s, line %lu:  Invalid color index \"%s\"",
		  file_peek_path(), file_peek_line(), (tmp ? tmp : ""));
      FREE(tmp);
    }
  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid "
		"within context color", file_peek_path(), file_peek_line(), buff);
  }
}

void
parse_attributes(char *buff)
{

  ASSERT(buff != NULL);

  if (!BEG_STRCASECMP(buff, "geometry ")) {
    RESET_AND_ASSIGN(rs_geometry, Word(2, buff));

  } else if (!BEG_STRCASECMP(buff, "title ")) {
    RESET_AND_ASSIGN(rs_title, Word(2, buff));

  } else if (!BEG_STRCASECMP(buff, "name ")) {
    RESET_AND_ASSIGN(rs_name, Word(2, buff));

  } else if (!BEG_STRCASECMP(buff, "iconname ")) {
    RESET_AND_ASSIGN(rs_iconName, Word(2, buff));

  } else if (!BEG_STRCASECMP(buff, "desktop ")) {
    rs_desktop = (int) strtol(buff, (char **) NULL, 0);

  } else if (!BEG_STRCASECMP(buff, "scrollbar_type ")) {
    RESET_AND_ASSIGN(rs_scrollbar_type, Word(2, buff));

  } else if (!BEG_STRCASECMP(buff, "scrollbar_width ")) {
    rs_scrollbar_width = strtoul(PWord(2, buff), (char **) NULL, 0);

  } else if (!BEG_STRCASECMP(buff, "font ")) {

    char *tmp = PWord(2, buff);
    unsigned char n;

    if (NumWords(buff) != 3) {
      print_error("Parse error in file %s, line %lu:  Invalid parameter list \"%s\" for "
		  "attribute font", file_peek_path(), file_peek_line(), (tmp ? tmp : ""));
      return;
    }
    if (isdigit(*tmp)) {
      n = (unsigned char) strtoul(tmp, (char **) NULL, 0);
      if (n <= 4) {
	RESET_AND_ASSIGN(rs_font[n], Word(2, tmp));
      } else {
	print_error("Parse error in file %s, line %lu:  Invalid font index %d",
		    file_peek_path(), file_peek_line(), n);
      }
    } else if (!BEG_STRCASECMP(tmp, "bold ")) {
#ifndef NO_BOLDFONT
      RESET_AND_ASSIGN(rs_boldFont, Word(2, tmp));
#else
      print_warning("Support for the bold font attribute was not compiled in, ignoring");
#endif

    } else {
      tmp = Word(1, tmp);
      print_error("Parse error in file %s, line %lu:  Invalid font index \"%s\"",
		  file_peek_path(), file_peek_line(), (tmp ? tmp : ""));
      FREE(tmp);
    }

  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid "
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
    print_error("Parse error in file %s, line %lu:  Missing boolean value in context toggles", file_peek_path(), file_peek_line());
    return;
  }
  if (BOOL_OPT_ISTRUE(tmp)) {
    bool_val = 1;
  } else if (BOOL_OPT_ISFALSE(tmp)) {
    bool_val = 0;
  } else {
    print_error("Parse error in file %s, line %lu:  Invalid boolean value \"%s\" in context toggles",
		file_peek_path(), file_peek_line(), tmp);
    return;
  }

  if (!BEG_STRCASECMP(buff, "map_alert ")) {
#if !defined(NO_MAPALERT) && defined(MAPALERT_OPTION)
    if (bool_val) {
      Options |= Opt_mapAlert;
    } else {
      Options &= ~(Opt_mapAlert);
    }
#else
    print_warning("Support for the map_alert attribute was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "visual_bell ")) {
    if (bool_val) {
      Options |= Opt_visualBell;
    } else {
      Options &= ~(Opt_visualBell);
    }
  } else if (!BEG_STRCASECMP(buff, "login_shell ")) {
    if (bool_val) {
      Options |= Opt_loginShell;
    } else {
      Options &= ~(Opt_loginShell);
    }
  } else if (!BEG_STRCASECMP(buff, "scrollbar ")) {
    if (bool_val) {
      Options |= Opt_scrollBar;
    } else {
      Options &= ~(Opt_scrollBar);
    }

  } else if (!BEG_STRCASECMP(buff, "utmp_logging ")) {
#ifdef UTMP_SUPPORT
    if (bool_val) {
      Options |= Opt_utmpLogging;
    } else {
      Options &= ~(Opt_utmpLogging);
    }
#else
    print_warning("Support for the utmp_logging attribute was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "meta8 ")) {
#ifdef META8_OPTION
    if (bool_val) {
      Options |= Opt_meta8;
    } else {
      Options &= ~(Opt_meta8);
    }
#else
    print_warning("Support for the meta8 attribute was not compiled in, ignoring");
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
    } else {
      Options &= ~(Opt_homeOnEcho);
    }

  } else if (!BEG_STRCASECMP(buff, "home_on_input ")) {
    if (bool_val) {
      Options |= Opt_homeOnInput;
    } else {
      Options &= ~(Opt_homeOnInput);
    }

  } else if (!BEG_STRCASECMP(buff, "home_on_refresh ")) {
    if (bool_val) {
      Options |= Opt_homeOnRefresh;
    } else {
      Options &= ~(Opt_homeOnRefresh);
    }

  } else if (!BEG_STRCASECMP(buff, "scrollbar_floating ")) {
    if (bool_val) {
      Options |= Opt_scrollBar_floating;
    } else {
      Options &= ~(Opt_scrollBar_floating);
    }

  } else if (!BEG_STRCASECMP(buff, "scrollbar_right ")) {
    if (bool_val) {
      Options |= Opt_scrollBar_right;
    } else {
      Options &= ~(Opt_scrollBar_right);
    }
  } else if (!BEG_STRCASECMP(buff, "scrollbar_popup ")) {
    if (bool_val) {
      Options |= Opt_scrollbar_popup;
    } else {
      Options &= ~(Opt_scrollbar_popup);
    }
  } else if (!BEG_STRCASECMP(buff, "borderless ")) {
    if (bool_val) {
      Options |= Opt_borderless;
    } else {
      Options &= ~(Opt_borderless);
    }
  } else if (!BEG_STRCASECMP(buff, "backing_store ")) {
    if (bool_val) {
      Options |= Opt_backing_store;
    } else {
      Options &= ~(Opt_backing_store);
    }

  } else if (!BEG_STRCASECMP(buff, "no_cursor ")) {
    if (bool_val) {
      Options |= Opt_noCursor;
    } else {
      Options &= ~(Opt_noCursor);
    }

  } else if (!BEG_STRCASECMP(buff, "pause ")) {
    if (bool_val) {
      Options |= Opt_pause;
    } else {
      Options &= ~(Opt_pause);
    }

  } else if (!BEG_STRCASECMP(buff, "xterm_select ")) {
    if (bool_val) {
      Options |= Opt_xterm_select;
    } else {
      Options &= ~(Opt_xterm_select);
    }

  } else if (!BEG_STRCASECMP(buff, "select_line ")) {
    if (bool_val) {
      Options |= Opt_select_whole_line;
    } else {
      Options &= ~(Opt_select_whole_line);
    }

  } else if (!BEG_STRCASECMP(buff, "select_trailing_spaces ")) {
    if (bool_val) {
      Options |= Opt_select_trailing_spaces;
    } else {
      Options &= ~(Opt_select_trailing_spaces);
    }

  } else if (!BEG_STRCASECMP(buff, "report_as_keysyms ")) {
    if (bool_val) {
      Options |= Opt_report_as_keysyms;
    } else {
      Options &= ~(Opt_report_as_keysyms);
    }

  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid within context toggles", file_peek_path(), file_peek_line(), buff);
  }
}

void
parse_keyboard(char *buff)
{

  ASSERT(buff != NULL);

  if (!BEG_STRCASECMP(buff, "smallfont_key ")) {
#if defined (HOTKEY_CTRL) || defined (HOTKEY_META)
    RESET_AND_ASSIGN(rs_smallfont_key, Word(2, buff));
    to_keysym(&ks_smallfont, rs_smallfont_key);
#else
    print_warning("Support for the smallfont_key attribute was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "bigfont_key ")) {
#if defined (HOTKEY_CTRL) || defined (HOTKEY_META)
    RESET_AND_ASSIGN(rs_bigfont_key, Word(2, buff));
    to_keysym(&ks_bigfont, rs_bigfont_key);
#else
    print_warning("Support for the bigfont_key attribute was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "keysym ")) {
#ifdef KEYSYM_ATTRIBUTE

    int sym, len;
    char *str = buff + 7, *s;

    sym = (int) strtol(str, (char **) NULL, 0);
    if (sym != (int) 2147483647L) {

      if (sym >= 0xff00)
	sym -= 0xff00;
      if (sym < 0 || sym > 0xff) {
	print_error("Parse error in file %s, line %lu:  Keysym 0x%x out of range 0xff00-0xffff",
		    file_peek_path(), file_peek_line(), sym + 0xff00);
	return;
      }
      s = Word(3, buff);
      str = (char *) MALLOC(strlen(s) + 2);
      strcpy(str, s);
      FREE(s);
      chomp(str);
      len = parse_escaped_string(str);
      if (len > 255)
	len = 255;		/* We can only handle lengths that will fit in a char */
      if (len && KeySym_map[sym] == NULL) {

	char *p = malloc(len + 1);

	*p = len;
	strncpy(p + 1, str, len);
	KeySym_map[sym] = (unsigned char *) p;
      }
    }
#else
    print_warning("Support for the keysym attributes was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "greek ")) {
#ifdef GREEK_SUPPORT

    char *tmp = PWord(2, buff);

    if (!tmp) {
      print_error("Parse error in file %s, line %lu:  Missing boolean value for attribute greek",
		  file_peek_path(), file_peek_line());
      return;
    }
    if (BOOL_OPT_ISTRUE(tmp)) {
      RESET_AND_ASSIGN(rs_greek_keyboard, Word(3, buff));
      if (BEG_STRCASECMP(rs_greek_keyboard, "iso")) {
	greek_setmode(GREEK_ELOT928);
      } else if (BEG_STRCASECMP(rs_greek_keyboard, "ibm")) {
	greek_setmode(GREEK_IBM437);
      } else {
	print_error("Parse error in file %s, line %lu:  Invalid greek keyboard mode \"%s\"",
		    file_peek_path(), file_peek_line(), (rs_greek_keyboard ? rs_greek_keyboard : ""));
      }
    } else if (BOOL_OPT_ISFALSE(tmp)) {
      /* This space intentionally left no longer blank =^) */
    } else {
      print_error("Parse error in file %s, line %lu:  Invalid boolean value \"%s\" for attribute %s",
		  file_peek_path(), file_peek_line(), tmp, buff);
      return;
    }
#else
    print_warning("Support for the greek attribute was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "app_keypad ")) {

    char *tmp = PWord(2, buff);

    if (!tmp) {
      print_error("Parse error in file %s, line %lu:  Missing boolean value for attribute app_keypad",
		  file_peek_path(), file_peek_line());
      return;
    }
    if (BOOL_OPT_ISTRUE(tmp)) {
      PrivateModes |= PrivMode_aplKP;
    } else if (BOOL_OPT_ISFALSE(tmp)) {
      PrivateModes &= ~(PrivMode_aplKP);
    } else {
      print_error("Parse error in file %s, line %lu:  Invalid boolean value \"%s\" for "
		  "attribute app_keypad", file_peek_path(), file_peek_line(), tmp);
      return;
    }

  } else if (!BEG_STRCASECMP(buff, "app_cursor ")) {

    char *tmp = PWord(2, buff);

    if (!tmp) {
      print_error("Parse error in file %s, line %lu:  Missing boolean value for attribute app_cursor",
		  file_peek_path(), file_peek_line());
      return;
    }
    if (BOOL_OPT_ISTRUE(tmp)) {
      PrivateModes |= PrivMode_aplCUR;
    } else if (BOOL_OPT_ISFALSE(tmp)) {
      PrivateModes &= ~(PrivMode_aplCUR);
    } else {
      print_error("Parse error in file %s, line %lu:  Invalid boolean value \"%s\" for "
		  "attribute app_cursor", file_peek_path(), file_peek_line(), tmp);
      return;
    }

  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid "
		"within context keyboard", file_peek_path(), file_peek_line(), buff);
  }
}

void
parse_misc(char *buff)
{

  ASSERT(buff != NULL);

  if (!BEG_STRCASECMP(buff, "print_pipe ")) {
#ifdef PRINTPIPE
    RESET_AND_ASSIGN(rs_print_pipe, StrDup(PWord(2, buff)));
    chomp(rs_print_pipe);
#else
    print_warning("Support for the print_pipe attribute was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "save_lines ")) {
    rs_saveLines = strtol(PWord(2, buff), (char **) NULL, 0);

  } else if (!BEG_STRCASECMP(buff, "min_anchor_size ")) {
    rs_min_anchor_size = strtol(PWord(2, buff), (char **) NULL, 0);

  } else if (!BEG_STRCASECMP(buff, "border_width ")) {
#ifdef BORDER_WIDTH_OPTION
    TermWin.internalBorder = (short) strtol(PWord(2, buff), (char **) NULL, 0);
#else
    print_warning("Support for the border_width attribute was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "line_space ")) {
    rs_line_space = strtol(PWord(2, buff), (char **) NULL, 0);

  } else if (!BEG_STRCASECMP(buff, "term_name ")) {
    RESET_AND_ASSIGN(rs_term_name, Word(2, buff));

  } else if (!BEG_STRCASECMP(buff, "debug ")) {
    debug_level = (unsigned int) strtoul(PWord(2, buff), (char **) NULL, 0);

  } else if (!BEG_STRCASECMP(buff, "exec ")) {

    register unsigned short k, n;

    Options |= Opt_exec;

    RESET_AND_ASSIGN(rs_execArgs, (char **) malloc(sizeof(char *) * ((n = NumWords(PWord(2, buff))) + 1)));

    for (k = 0; k < n; k++) {
      rs_execArgs[k] = Word(k + 2, buff);
      D_OPTIONS(("rs_execArgs[%d] == %s\n", k, rs_execArgs[k]));
    }
    rs_execArgs[n] = (char *) NULL;

  } else if (!BEG_STRCASECMP(buff, "cut_chars ")) {
#ifdef CUTCHAR_OPTION
    RESET_AND_ASSIGN(rs_cutchars, Word(2, buff));
    chomp(rs_cutchars);
#else
    print_warning("Support for the cut_chars attribute was not compiled in, ignoring");
#endif

  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid "
		"within context misc", file_peek_path(), file_peek_line(), buff);
  }
}

void
parse_imageclasses(char *buff)
{

#ifdef PIXMAP_SUPPORT
  ASSERT(buff != NULL);

  if (!BEG_STRCASECMP(buff, "icon ")) {
    RESET_AND_ASSIGN(rs_icon, Word(2, buff));

  } else if (!BEG_STRCASECMP(buff, "path ")) {
    RESET_AND_ASSIGN(rs_path, Word(2, buff));

  } else if (!BEG_STRCASECMP(buff, "anim ")) {
# ifdef BACKGROUND_CYCLING_SUPPORT
    char *tmp = PWord(2, buff);

    if (tmp) {
      rs_anim_pixmap_list = StrDup(tmp);
    } else {
      print_error("Parse error in file %s, line %lu:  Invalid parameter list \"\" for attribute anim", file_peek_path(), file_peek_line());
    }
# else
    print_warning("Support for the anim attribute was not compiled in, ignoring");
# endif

  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid "
		"within context imageclasses", file_peek_path(), file_peek_line(), buff);
  }
#else
  print_warning("Pixmap support was not compiled in, ignoring entire context");
  file_poke_skip(1);
#endif
}

void
parse_image(char *buff)
{
  static short idx = -1;

#ifdef PIXMAP_SUPPORT
  ASSERT(buff != NULL);

  if (!BEG_STRCASECMP(buff, "type ")) {
    char *type = PWord(2, buff);

    if (!type) {
      print_error("Parse error in file %s, line %lu:  Missing image type", file_peek_path(), file_peek_line());
      return;
    }
    if (!strcasecmp(type, "background")) {
      idx = image_bg;
    } else if (!strcasecmp(type, "trough")) {
      idx = image_sb;
    } else if (!strcasecmp(type, "anchor")) {
      idx = image_sa;
    } else if (!strcasecmp(type, "up_arrow")) {
      idx = image_up;
    } else if (!strcasecmp(type, "down_arrow")) {
      idx = image_down;
    } else if (!strcasecmp(type, "left_arrow")) {
      idx = image_left;
    } else if (!strcasecmp(type, "right_arrow")) {
      idx = image_right;
    } else if (!strcasecmp(type, "menu")) {
      idx = image_menu;
    } else if (!strcasecmp(type, "submenu")) {
      idx = image_submenu;
    } else {
      print_error("Parse error in file %s, line %lu:  Invalid image type \"%s\"", file_peek_path(), file_peek_line(), type);
      return;
    }

  } else if (!BEG_STRCASECMP(buff, "mode ")) {
    char *mode = PWord(2, buff);
    char *allow_list = PWord(4, buff);

    if (idx < 0) {
      print_error("Parse error in file %s, line %lu:  Encountered \"mode\" with no image type defined", file_peek_path(), file_peek_line());
      return;
    }
    if (!mode) {
      print_error("Parse error in file %s, line %lu:  Missing parameters for mode line", file_peek_path(), file_peek_line());
      return;
    }
    if (!BEG_STRCASECMP(mode, "image")) {
      images[idx].mode = (MODE_IMAGE | ALLOW_IMAGE);
    } else if (!BEG_STRCASECMP(mode, "trans")) {
      images[idx].mode = (MODE_TRANS | ALLOW_TRANS);
    } else if (!BEG_STRCASECMP(mode, "viewport")) {
      images[idx].mode = (MODE_VIEWPORT | ALLOW_VIEWPORT);
    } else if (!BEG_STRCASECMP(mode, "auto")) {
      images[idx].mode = (MODE_AUTO | ALLOW_AUTO);
    } else if (!BEG_STRCASECMP(mode, "solid")) {
      images[idx].mode = MODE_SOLID;
    } else {
      print_error("Parse error in file %s, line %lu:  Invalid mode \"%s\"", file_peek_path(), file_peek_line(), mode);
    }
    if (allow_list) {
      char *allow;

      for (; (allow = (char *) strsep(&allow_list, " ")) != NULL;) {
	if (!BEG_STRCASECMP("image", allow)) {
	  images[idx].mode |= ALLOW_IMAGE;
	} else if (!BEG_STRCASECMP("transparent", allow)) {
	  images[idx].mode |= ALLOW_TRANS;
	} else if (!BEG_STRCASECMP("viewport", allow)) {
	  images[idx].mode |= ALLOW_VIEWPORT;
	} else if (!BEG_STRCASECMP("auto", allow)) {
	  images[idx].mode |= ALLOW_AUTO;
	} else if (!BEG_STRCASECMP("solid", allow)) {
	} else {
	  print_error("Parse error in file %s, line %lu:  Invalid mode \"%s\"", file_peek_path(), file_peek_line(), allow);
	}
      }
    }
  } else if (!BEG_STRCASECMP(buff, "state ")) {
    char *state = PWord(2, buff), new = 0;

    if (!state) {
      print_error("Parse error in file %s, line %lu:  Missing state", file_peek_path(), file_peek_line());
      return;
    }
    if (idx < 0) {
      print_error("Parse error in file %s, line %lu:  Encountered \"state\" with no image type defined", file_peek_path(), file_peek_line());
      return;
    }
    if (!strcasecmp(state, "normal")) {
      if (images[idx].norm == NULL) {
	images[idx].norm = (simage_t *) MALLOC(sizeof(simage_t));
	new = 1;
      }
      images[idx].current = images[idx].norm;
    } else if (!strcasecmp(state, "selected")) {
      if (images[idx].selected == NULL) {
	images[idx].selected = (simage_t *) MALLOC(sizeof(simage_t));
	new = 1;
      }
      images[idx].current = images[idx].selected;
    } else if (!strcasecmp(state, "clicked")) {
      if (images[idx].clicked == NULL) {
	images[idx].clicked = (simage_t *) MALLOC(sizeof(simage_t));
	new = 1;
      }
      images[idx].current = images[idx].clicked;
    } else {
      print_error("Parse error in file %s, line %lu:  Invalid state \"%s\"", file_peek_path(), file_peek_line(), state);
      return;
    }
    if (new) {
      images[idx].current->pmap = (pixmap_t *) MALLOC(sizeof(pixmap_t));
      images[idx].current->iml = (imlib_t *) MALLOC(sizeof(imlib_t));
      MEMSET(images[idx].current->pmap, 0, sizeof(pixmap_t));
      MEMSET(images[idx].current->iml, 0, sizeof(imlib_t));
    }
  } else if (!BEG_STRCASECMP(buff, "file ")) {
    char *filename = PWord(2, buff);

    if (idx < 0) {
      print_error("Parse error in file %s, line %lu:  Encountered \"file\" with no image type defined", file_peek_path(), file_peek_line());
      return;
    }
    if (!filename) {
      print_error("Parse error in file %s, line %lu:  Missing filename", file_peek_path(), file_peek_line());
      return;
    }
    load_image(filename, idx);

  } else if (!BEG_STRCASECMP(buff, "geom ")) {
    char *geom = PWord(2, buff);

    if (idx < 0) {
      print_error("Parse error in file %s, line %lu:  Encountered \"geom\" with no image type defined", file_peek_path(), file_peek_line());
      return;
    }
    if (!geom) {
      print_error("Parse error in file %s, line %lu:  Missing geometry string", file_peek_path(), file_peek_line());
      return;
    }
    set_pixmap_scale(geom, images[idx].current->pmap);

  } else if (!BEG_STRCASECMP(buff, "cmod ") || !BEG_STRCASECMP(buff, "colormod ")) {
    char *color = PWord(2, buff);
    char *mods = PWord(3, buff);
    unsigned char n;
    imlib_t *iml = images[idx].current->iml;

    if (idx < 0) {
      print_error("Parse error in file %s, line %lu:  Encountered color modifier with no image type defined", file_peek_path(), file_peek_line());
      return;
    }
    if (!color) {
      print_error("Parse error in file %s, line %lu:  Missing color name", file_peek_path(), file_peek_line());
      return;
    }
    if (!mods) {
      print_error("Parse error in file %s, line %lu:  Missing modifier(s)", file_peek_path(), file_peek_line());
      return;
    }
    n = NumWords(mods);

    if (!BEG_STRCASECMP(color, "image ")) {
      RESET_AND_ASSIGN(iml->mod, (ImlibColorModifier *) MALLOC(sizeof(ImlibColorModifier)));
      iml->mod->contrast = iml->mod->gamma = 0xff;
      iml->mod->brightness = (int) strtol(mods, (char **) NULL, 0);
      if (n > 1) {
        iml->mod->contrast = (int) strtol(PWord(2, mods), (char **) NULL, 0);
      }
      if (n > 2) {
        iml->rmod->gamma = (int) strtol(PWord(3, mods), (char **) NULL, 0);
      }
    } else if (!BEG_STRCASECMP(color, "red ")) {
      RESET_AND_ASSIGN(iml->rmod, (ImlibColorModifier *) MALLOC(sizeof(ImlibColorModifier)));
      iml->rmod->contrast = iml->rmod->gamma = 0xff;
      iml->rmod->brightness = (int) strtol(mods, (char **) NULL, 0);
      if (n > 1) {
        iml->rmod->contrast = (int) strtol(PWord(2, mods), (char **) NULL, 0);
      }
      if (n > 2) {
        iml->rmod->gamma = (int) strtol(PWord(3, mods), (char **) NULL, 0);
      }
    } else if (!BEG_STRCASECMP(color, "green ")) {
      RESET_AND_ASSIGN(iml->gmod, (ImlibColorModifier *) MALLOC(sizeof(ImlibColorModifier)));
      iml->gmod->contrast = iml->gmod->gamma = 0xff;
      iml->gmod->brightness = (int) strtol(mods, (char **) NULL, 0);
      if (n > 1) {
        iml->gmod->contrast = (int) strtol(PWord(2, mods), (char **) NULL, 0);
      }
      if (n > 2) {
        iml->gmod->gamma = (int) strtol(PWord(3, mods), (char **) NULL, 0);
      }
    } else if (!BEG_STRCASECMP(color, "blue ")) {
      RESET_AND_ASSIGN(iml->bmod, (ImlibColorModifier *) MALLOC(sizeof(ImlibColorModifier)));
      iml->bmod->contrast = iml->bmod->gamma = 0xff;
      iml->bmod->brightness = (int) strtol(mods, (char **) NULL, 0);
      if (n > 1) {
        iml->bmod->contrast = (int) strtol(PWord(2, mods), (char **) NULL, 0);
      }
      if (n > 2) {
        iml->bmod->gamma = (int) strtol(PWord(3, mods), (char **) NULL, 0);
      }
    } else {
      print_error("Parse error in file %s, line %lu:  Color must be either \"image\", \"red\", \"green\", or \"blue\"", file_peek_path(), file_peek_line());
      return;
    }

  } else if (!BEG_STRCASECMP(buff, "border ")) {
    if (idx < 0) {
      print_error("Parse error in file %s, line %lu:  Encountered \"border\" with no image type defined", file_peek_path(), file_peek_line());
      return;
    }
    if (NumWords(buff + 7) < 4) {
      print_error("Parse error in file %s, line %lu:  Invalid parameter list for attribute \"border\"", file_peek_path(), file_peek_line());
      return;
    }
    RESET_AND_ASSIGN(images[idx].current->iml->border, (ImlibBorder *) MALLOC(sizeof(ImlibBorder)));

    images[idx].current->iml->border->left = (unsigned short) strtoul(PWord(2, buff), (char **) NULL, 0);
    images[idx].current->iml->border->right = (unsigned short) strtoul(PWord(3, buff), (char **) NULL, 0);
    images[idx].current->iml->border->top = (unsigned short) strtoul(PWord(4, buff), (char **) NULL, 0);
    images[idx].current->iml->border->bottom = (unsigned short) strtoul(PWord(5, buff), (char **) NULL, 0);

    if ((images[idx].current->iml->border->left == 0) && (images[idx].current->iml->border->right == 0)
	&& (images[idx].current->iml->border->top == 0) && (images[idx].current->iml->border->bottom == 0)) {
      FREE(images[idx].current->iml->border);
      images[idx].current->iml->border = (ImlibBorder *) NULL;	/* No sense in wasting CPU time and memory if there are no borders */
    }
  } else if (!BEG_STRCASECMP(buff, "bevel ")) {
    if (idx < 0) {
      print_error("Parse error in file %s, line %lu:  Encountered \"bevel\" with no image type defined", file_peek_path(), file_peek_line());
      return;
    }
    if (NumWords(buff + 6) < 5) {
      print_error("Parse error in file %s, line %lu:  Invalid parameter list for attribute \"bevel\"", file_peek_path(), file_peek_line());
      return;
    }
    if (images[idx].current->iml->bevel != NULL) {
      FREE(images[idx].current->iml->bevel->edges);
      FREE(images[idx].current->iml->bevel);
    }
    images[idx].current->iml->bevel = (bevel_t *) MALLOC(sizeof(bevel_t));
    images[idx].current->iml->bevel->edges = (ImlibBorder *) MALLOC(sizeof(ImlibBorder));

    if (!BEG_STRCASECMP(PWord(2, buff), "down")) {
      images[idx].current->iml->bevel->up = 0;
    } else {
      images[idx].current->iml->bevel->up = 1;
    }
    images[idx].current->iml->bevel->edges->left = (unsigned short) strtoul(PWord(3, buff), (char **) NULL, 0);
    images[idx].current->iml->bevel->edges->right = (unsigned short) strtoul(PWord(4, buff), (char **) NULL, 0);
    images[idx].current->iml->bevel->edges->top = (unsigned short) strtoul(PWord(5, buff), (char **) NULL, 0);
    images[idx].current->iml->bevel->edges->bottom = (unsigned short) strtoul(PWord(6, buff), (char **) NULL, 0);

    if ((images[idx].current->iml->bevel->edges->left == 0) && (images[idx].current->iml->bevel->edges->right == 0)
	&& (images[idx].current->iml->bevel->edges->top == 0) && (images[idx].current->iml->bevel->edges->bottom == 0)) {
      FREE(images[idx].current->iml->bevel->edges);
      images[idx].current->iml->bevel->edges = (ImlibBorder *) NULL;
      FREE(images[idx].current->iml->bevel);
      images[idx].current->iml->bevel = (bevel_t *) NULL;
    }
  } else if (!BEG_STRCASECMP(buff, "padding ")) {
    if (idx < 0) {
      print_error("Parse error in file %s, line %lu:  Encountered \"padding\" with no image type defined", file_peek_path(), file_peek_line());
      return;
    }
    if (NumWords(buff + 8) < 4) {
      print_error("Parse error in file %s, line %lu:  Invalid parameter list for attribute \"padding\"", file_peek_path(), file_peek_line());
      return;
    }
    RESET_AND_ASSIGN(images[idx].current->iml->pad, (ImlibBorder *) MALLOC(sizeof(ImlibBorder)));

    images[idx].current->iml->pad->left = (unsigned short) strtoul(PWord(2, buff), (char **) NULL, 0);
    images[idx].current->iml->pad->right = (unsigned short) strtoul(PWord(3, buff), (char **) NULL, 0);
    images[idx].current->iml->pad->top = (unsigned short) strtoul(PWord(4, buff), (char **) NULL, 0);
    images[idx].current->iml->pad->bottom = (unsigned short) strtoul(PWord(5, buff), (char **) NULL, 0);

    if ((images[idx].current->iml->pad->left == 0) && (images[idx].current->iml->pad->right == 0)
	&& (images[idx].current->iml->pad->top == 0) && (images[idx].current->iml->pad->bottom == 0)) {
      FREE(images[idx].current->iml->pad);
      images[idx].current->iml->pad = (ImlibBorder *) NULL;
    }
  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid "
		"within context image", file_peek_path(), file_peek_line(), buff);
  }
#else
  print_warning("Pixmap support was not compiled in, ignoring entire context");
  file_poke_skip(1);
#endif
}

void
parse_actions(char *buff)
{
  unsigned short mod = MOD_NONE;
  unsigned char button = BUTTON_NONE;
  KeySym keysym = 0;
  char *str;
  unsigned short i;

  ASSERT(buff != NULL);

  if (!BEG_STRCASECMP(buff, "bind ")) {
    for (i = 2; (str = Word(i, buff)) && strcasecmp(str, "to"); i++) {
      if (!BEG_STRCASECMP(str, "anymod")) {
        mod = MOD_ANY;
      } else if (!BEG_STRCASECMP(str, "ctrl")) {
        mod |= MOD_CTRL;
      } else if (!BEG_STRCASECMP(str, "shift")) {
        mod |= MOD_SHIFT;
      } else if (!BEG_STRCASECMP(str, "lock")) {
        mod |= MOD_LOCK;
      } else if (!BEG_STRCASECMP(str, "mod1") || !BEG_STRCASECMP(str, "alt") || !BEG_STRCASECMP(str, "meta")) {
        mod |= MOD_MOD1;
      } else if (!BEG_STRCASECMP(str, "mod2")) {
        mod |= MOD_MOD2;
      } else if (!BEG_STRCASECMP(str, "mod3")) {
        mod |= MOD_MOD3;
      } else if (!BEG_STRCASECMP(str, "mod4")) {
        mod |= MOD_MOD4;
      } else if (!BEG_STRCASECMP(str, "mod5")) {
        mod |= MOD_MOD5;
      } else if (!BEG_STRCASECMP(str, "button")) {
        switch (*(str+6)) {
        case '1': button = Button1; break;
        case '2': button = Button2; break;
        case '3': button = Button3; break;
        case '4': button = Button4; break;
        case '5': button = Button5; break;
        default: break;
        }
      } else if (isdigit(*str)) {
        keysym = (KeySym) strtoul(str, (char **) NULL, 0);
      } else {
        keysym = XStringToKeysym(str);
      }
      FREE(str);
    }
    if (!str) {
      print_error("Parse error in file %s, line %lu:  Syntax error (\"to\" not found)", file_peek_path(), file_peek_line());
      return;
    }
    FREE(str);
    if ((button == BUTTON_NONE) && (keysym == 0)) {
      print_error("Parse error in file %s, line %lu:  No valid button/keysym found for action", file_peek_path(), file_peek_line());
      return;
    }
    i++;
    str = PWord(i, buff);
    if (!BEG_STRCASECMP(str, "string")) {
      str = Word(i+1, buff);
      action_add(mod, button, keysym, ACTION_STRING, (void *) str);
      FREE(str);
    } else if (!BEG_STRCASECMP(str, "echo")) {
      str = Word(i+1, buff);
      action_add(mod, button, keysym, ACTION_ECHO, (void *) str);
      FREE(str);
    } else if (!BEG_STRCASECMP(str, "menu")) {
      menu_t *menu;

      str = Word(i+1, buff);
      menu = find_menu_by_title(menu_list, str);
      action_add(mod, button, keysym, ACTION_MENU, (void *) menu);
      FREE(str);
    } else {
      print_error("Parse error in file %s, line %lu:  Syntax error (\"to\" not found)", file_peek_path(), file_peek_line());
      return;
    }

  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid "
		"within context action", file_peek_path(), file_peek_line(), buff);
  }
}

void
parse_menu(char *buff)
{

  ASSERT(buff != NULL);

  if (!BEG_STRCASECMP(buff, "title ")) {
    char *title = Word(2, buff);

    curmenu = menu_create(title);
    menu_list = menulist_add_menu(menu_list, curmenu);
    FREE(title);

  } else if (!BEG_STRCASECMP(buff, "font ")) {
    char *name = Word(2, buff);

    if (!curmenu) {
      print_error("Parse error in file %s, line %lu:  You must set the menu title before choosing a font.", file_peek_path(), file_peek_line());
      return;
    }
    if (!name) {
      print_error("Parse error in file %s, line %lu:  Missing font name.", file_peek_path(), file_peek_line());
      return;
    }
    menu_set_font(curmenu, name);
    FREE(name);

  } else if (!BEG_STRCASECMP(buff, "sep") || !BEG_STRCASECMP(buff, "-")) {
    menuitem_t *item;

    item = menuitem_create((char *) NULL);
    menu_add_item(curmenu, item);
    menuitem_set_action(item, MENUITEM_SEP, (char *) NULL);

  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid "
		"within context menu", file_peek_path(), file_peek_line(), buff);
  }
}

void
parse_menuitem(char *buff)
{

  static menuitem_t *curitem;

  ASSERT(buff != NULL);

  if (!BEG_STRCASECMP(buff, "text ")) {
    char *text = Word(2, buff);

    curitem = menuitem_create(text);
    menu_add_item(curmenu, curitem);
    FREE(text);

  } else if (!BEG_STRCASECMP(buff, "rtext ")) {
    char *rtext = Word(2, buff);

    menuitem_set_rtext(curitem, rtext);
    FREE(rtext);

  } else if (!BEG_STRCASECMP(buff, "icon ")) {

  } else if (!BEG_STRCASECMP(buff, "action ")) {
    char *type = PWord(2, buff);
    char *action = Word(3, buff);

    if (!BEG_STRCASECMP(type, "submenu ")) {
      menuitem_set_action(curitem, MENUITEM_SUBMENU, action);

    } else if (!BEG_STRCASECMP(type, "string ")) {
      menuitem_set_action(curitem, MENUITEM_STRING, action);

    } else if (!BEG_STRCASECMP(type, "echo ")) {
      menuitem_set_action(curitem, MENUITEM_ECHO, action);

    } else if (!BEG_STRCASECMP(type, "separator")) {
      menuitem_set_action(curitem, MENUITEM_SEP, action);

    } else {
      print_error("Parse error in file %s, line %lu:  Invalid menu item action \"%s\"", file_peek_path(), file_peek_line(), type);
    }
    FREE(action);

  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid "
		"within context menu", file_peek_path(), file_peek_line(), buff);
  }
}

void
parse_xim(char *buff)
{

  ASSERT(buff != NULL);

#ifdef USE_XIM
  if (!BEG_STRCASECMP(buff, "input_method ")) {
    RESET_AND_ASSIGN(rs_inputMethod, Word(2, buff));
  } else if (!BEG_STRCASECMP(buff, "preedit_type ")) {
    RESET_AND_ASSIGN(rs_preeditType, Word(2, buff));
  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid within context xim",
		file_peek_path(), file_peek_line(), buff);
  }
#else
  print_warning("XIM support was not compiled in, ignoring entire context");
  file_poke_skip(1);
#endif
}

void
parse_multichar(char *buff)
{

  ASSERT(buff != NULL);

#ifdef MULTI_CHARSET
  if (!BEG_STRCASECMP(buff, "encoding ")) {
    RESET_AND_ASSIGN(rs_multchar_encoding, Word(2, buff));
    if (rs_multchar_encoding != NULL) {
      if (BEG_STRCASECMP(rs_multchar_encoding, "eucj")
	  && BEG_STRCASECMP(rs_multchar_encoding, "sjis")
	  && BEG_STRCASECMP(rs_multchar_encoding, "euckr")) {
	print_error("Parse error in file %s, line %lu:  Invalid multichar encoding mode \"%s\"",
		    file_peek_path(), file_peek_line(), rs_multchar_encoding);
	return;
      }
      set_multichar_encoding(rs_multchar_encoding);
    } else {
      print_error("Parse error in file %s, line %lu:  Invalid parameter list \"\" for attribute encoding",
		  file_peek_path(), file_peek_line());
    }
  } else if (!BEG_STRCASECMP(buff, "font ")) {

    char *tmp = PWord(2, buff);
    unsigned char n;

    if (NumWords(buff) != 3) {
      print_error("Parse error in file %s, line %lu:  Invalid parameter list \"%s\" for attribute font",
		  file_peek_path(), file_peek_line(), (tmp ? tmp : ""));
      return;
    }
    if (isdigit(*tmp)) {
      n = (unsigned char) strtoul(tmp, (char **) NULL, 0);
      if (n <= 4) {
	RESET_AND_ASSIGN(rs_mfont[n], Word(2, tmp));
      } else {
	print_error("Parse error in file %s, line %lu:  Invalid font index %d",
		    file_peek_path(), file_peek_line(), n);
      }
    } else {
      tmp = Word(1, tmp);
      print_error("Parse error in file %s, line %lu:  Invalid font index \"%s\"",
		  file_peek_path(), file_peek_line(), NONULL(tmp));
      FREE(tmp);
    }
  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid within context multichar",
		file_peek_path(), file_peek_line(), buff);
  }
#else
  print_warning("Multichar support was not compiled in, ignoring entire context");
  file_poke_skip(1);
#endif
}

void
parse_undef(char *buff)
{

  ASSERT(buff != NULL);

  print_error("Parse error in file %s, line %lu:  Undefined subcontext \"%s\" within context %s",
	      file_peek_path(), file_peek_line(), PWord(2, buff),
	      ctx_id_to_name(ctx_peek_last()));
  file_poke_skip(1);

}

/* The config file reader.  This looks for the config file by searching CONFIG_SEARCH_PATH and PATH_ENV.
   If it can't find a config file, it displays a warning but continues. -- mej */

char *
find_theme(char *path, char *theme_name, char *conf_name)
{

  register char *search_path;
  register char *cur_path;
  char working_path[PATH_MAX + 1];
  char *theme_dir = NULL;
  struct stat fst;
  unsigned long len;

  ASSERT(path != NULL && conf_name != NULL);

  search_path = StrDup(path);
  if (theme_name) {
    D_OPTIONS(("Searching for config file %s in theme %s\n", conf_name, theme_name));
  } else {
    D_OPTIONS(("Searching for config file %s (no theme)\n", conf_name));
  }

  for (cur_path = strtok(search_path, ":"); cur_path; cur_path = strtok((char *) NULL, ":")) {

    if (!BEG_STRCASECMP(cur_path, "~/")) {
      snprintf(working_path, PATH_MAX, "%s/%s/%s%s%s", getenv("HOME"), cur_path + 2, (theme_name ? theme_name : ""), (theme_name ? "/" : ""), conf_name);
    } else {
      snprintf(working_path, PATH_MAX, "%s/%s%s%s", cur_path, (theme_name ? theme_name : ""), (theme_name ? "/" : ""), conf_name);
    }
    D_OPTIONS(("Checking for a valid config file at \"%s\"\n", working_path));

    if (!access(working_path, R_OK)) {
      stat(working_path, &fst);
      if (!S_ISDIR(fst.st_mode)) {
	if (open_config_file(working_path)) {
	  len = strlen(working_path) - strlen(conf_name);
	  theme_dir = (char *) MALLOC(len);
	  snprintf(theme_dir, len, "%s", working_path);
	  if (theme_name) {
	    D_OPTIONS(("Found config file %s in theme %s at %s\n", conf_name, theme_name, theme_dir));
	  } else {
	    D_OPTIONS(("Found config file %s at %s\n", conf_name, theme_dir));
	  }
	  break;
	} else {
	  D_OPTIONS((" -> Not a valid config file.\n"));
	}
      } else {
	D_OPTIONS((" -> That is a directory.\n"));
      }
    } else {
      D_OPTIONS((" -> Read access forbidden -- %s\n", strerror(errno)));
    }
  }
  FREE(search_path);
  return (theme_dir);
}

unsigned char
open_config_file(char *name)
{

  FILE *fp;
  int ver;
  char buff[256], *end_ptr;

  ASSERT(name != NULL);

  fp = fopen(name, "rt");
  if (fp != NULL) {
    fgets(buff, 256, fp);
    D_OPTIONS(("Magic line \"%s\" [%s]  VERSION == \"%s\"  size == %lu\n", buff, buff + 7, VERSION, sizeof(VERSION) - 1));
    if (BEG_STRCASECMP(buff, "<Eterm-")) {
      fclose(fp);
      fp = NULL;
    } else {
      if ((end_ptr = strchr(buff, '>')) != NULL) {
	*end_ptr = 0;
      }
      if ((ver = BEG_STRCASECMP(buff + 7, VERSION)) > 0) {
	print_warning("Config file is designed for a newer version of Eterm");
#ifdef WARN_OLDER
      } else if (ver < 0) {
	print_warning("Config file is designed for an older version of Eterm");
#endif
      }
    }
  }
  file_poke_fp(fp);
  return ((unsigned char) (fp == NULL ? 0 : 1));
}

void
read_config(char *conf_name)
{

  register char *search_path;
  char *path_env, *desc, *outfile;
  char buff[CONFIG_BUFF];
  char *root = NULL;
  register unsigned long i = 0;
  static char first_time = 1;
  unsigned char id = CTX_UNDEF;
  file_state fs =
  {NULL, NULL, NULL, 1, 0};

  D_OPTIONS(("read_config(conf_name [%s]) called.\n", conf_name));
  path_env = getenv(PATH_ENV);
  if (path_env)
    i += strlen(path_env);
  i += sizeof(CONFIG_SEARCH_PATH) + 3;
  search_path = (char *) MALLOC(i);
  snprintf(search_path, i, "%s%s%s", CONFIG_SEARCH_PATH, (path_env ? ":" : ""), (path_env ? path_env : ""));
  file_push(fs);

  if (rs_theme) {
    root = find_theme(search_path, rs_theme, conf_name);
  }
  if (root == NULL) {
    root = find_theme(search_path, "Eterm", conf_name);
    if (root == NULL) {
      root = find_theme(search_path, "DEFAULT", conf_name);
      if (root == NULL) {
	root = find_theme(search_path, (char *) NULL, conf_name);
	if (rs_theme != NULL) {
	  FREE(rs_theme);
	  rs_theme = NULL;
	}
      }
    }
  }
  FREE(search_path);
  if (root == NULL) {
    D_OPTIONS((" -> Config file search failed.\n"));
    if (first_time) {
      print_error("Unable to find/open config file %s.  Continuing with defaults.", conf_name);
    }
    return;
  }
  D_OPTIONS((" -> Config file search succeeded with root == %s\n", root));
  if (first_time) {
    theme_dir = root;
    root = (char *) MALLOC(strlen(theme_dir) + 17 + 1);
    sprintf(root, "ETERM_THEME_ROOT=%s", theme_dir);
    putenv(root);
    chdir(theme_dir);
  } else {
    user_dir = root;
    root = (char *) MALLOC(strlen(theme_dir) + 16 + 1);
    sprintf(root, "ETERM_USER_ROOT=%s", theme_dir);
    putenv(root);
    chdir(user_dir);
  }
  first_time = 0;

  file_poke_path(conf_name);
  ctx_poke(0);
  for (; cur_file >= 0;) {
    for (; fgets(buff, CONFIG_BUFF, file_peek_fp());) {
      file_inc_line();
      if (!strchr(buff, '\n')) {
	print_error("Parse error in file %s, line %lu:  line too long", file_peek_path(), file_peek_line());
	for (; fgets(buff, CONFIG_BUFF, file_peek_fp()) && !strrchr(buff, '\n'););
	continue;
      }
      if (!(*buff) || *buff == '\n') {
	continue;
      }
      chomp(buff);
      switch (*buff) {
	case '#':
	case '<':
	  break;
	case '%':
	  D_OPTIONS(("read_config():  Parsing line #%lu of file %s\n", file_peek_line(), file_peek_path()));
	  if (!BEG_STRCASECMP(PWord(1, buff + 1), "include ")) {
	    shell_expand(buff);
	    fs.path = Word(2, buff + 1);
	    fs.outfile = (char *) NULL;
	    fs.line = 1;
	    fs.flags = 0;
	    file_push(fs);
	    if (!open_config_file(fs.path)) {
	      print_error("Error in file %s, line %lu:  Unable to locate %%included config file %s "
			  "(%s), continuing", file_peek_path(), file_peek_line(), fs.path,
			  strerror(errno));
	      file_pop();
	    }
	  } else if (!BEG_STRCASECMP(PWord(1, buff + 1), "preproc ")) {
	    char cmd[PATH_MAX];
	    FILE *fp;

	    if (file_peek_preproc()) {
	      continue;
	    }
	    outfile = tmpnam(NULL);
	    snprintf(cmd, PATH_MAX, "%s < %s > %s", PWord(2, buff), file_peek_path(), outfile);
	    system(cmd);
	    fp = fopen(outfile, "rt");
	    if (fp != NULL) {
	      fclose(file_peek_fp());
	      file_poke_fp(fp);
	      file_poke_preproc(1);
	      file_poke_outfile(outfile);
	    }
	  } else {
	    print_error("Parse error in file %s, line %lu:  Undefined macro \"%s\"", file_peek_path(), file_peek_line(), buff);
	  }
	  break;
	case 'b':
	  D_OPTIONS(("read_config():  Parsing line #%lu of file %s\n", file_peek_line(), file_peek_path()));
	  if (file_peek_skip())
	    continue;
	  if (!BEG_STRCASECMP(buff, "begin ")) {
	    desc = PWord(2, buff);
	    ctx_name_to_id(id, desc, i);
	    if (id == CTX_UNDEF) {
	      ctx_push(id);
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
		    print_error("Parse error in file %s, line %lu:  subcontext %s is not valid "
				"within context %s", file_peek_path(), file_peek_line(),
				contexts[id].description, contexts[ctx_peek()].description);
		    break;
		  }
		}
	      } else if (id != CTX_MAIN) {
		print_error("Parse error in file %s, line %lu:  subcontext %s is not valid "
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
	  D_OPTIONS(("read_config():  Parsing line #%lu of file %s\n", file_peek_line(), file_peek_path()));
	  if (!BEG_STRCASECMP(buff, "end ") || !strcasecmp(buff, "end")) {
	    if (ctx_get_depth()) {
	      (void) ctx_pop();
	      file_poke_skip(0);
	    }
	    break;
	  }
	default:
	  D_OPTIONS(("read_config():  Parsing line #%lu of file %s\n", file_peek_line(), file_peek_path()));
	  if (file_peek_skip())
	    continue;
	  if ((id = ctx_peek())) {
	    shell_expand(buff);
	    (*ctx_id_to_func(id)) (buff);
	  } else {
	    print_error("Parse error in file %s, line %lu:  No established context to parse \"%s\"",
			file_peek_path(), file_peek_line(), buff);
	  }
      }
    }
    fclose(file_peek_fp());
    if (file_peek_preproc()) {
      remove(file_peek_outfile());
    }
    file_pop();
  }
}

/* Initialize the default values for everything */
void
init_defaults(void)
{

  rs_name = StrDup(APL_NAME " " VERSION);

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
    scrollbar_set_shadow(0);
  } else {
    scrollbar_set_shadow((Options & Opt_scrollBar_floating) ? 0 : SHADOW);
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
    if (!rs_font[i]) {
      rs_font[i] = StrDup(def_fontName[i]);
    }
#ifdef MULTI_CHARSET
    if (!rs_mfont[i]) {
      rs_mfont[i] = StrDup(def_mfontName[i]);
    }
#endif
  }

  /* Clean up image stuff */
  for (i = 0; i < image_max; i++) {
    if (!(images[i].norm)) {
      images[i].norm = (simage_t *) MALLOC(sizeof(simage_t));
      images[i].norm->pmap = (pixmap_t *) MALLOC(sizeof(pixmap_t));
      images[i].norm->iml = (imlib_t *) MALLOC(sizeof(imlib_t));
      MEMSET(images[i].norm->pmap, 0, sizeof(pixmap_t));
      MEMSET(images[i].norm->iml, 0, sizeof(imlib_t));
      images[i].mode = MODE_IMAGE & ALLOW_IMAGE;
    }
    images[i].current = images[i].norm;
    if (rs_pixmaps[i]) {
      reset_simage(images[i].norm, RESET_ALL);
      load_image(rs_pixmaps[i], i);
      FREE(rs_pixmaps[i]);	/* These are created by StrDup() */
    }
    if (!(images[i].selected)) {
      images[i].selected = images[i].norm;
    }
    if (!(images[i].clicked)) {
      images[i].clicked = images[i].selected;
    }
    if ((image_toggles & IMOPT_TRANS) && (image_mode_is(i, ALLOW_TRANS))) {
      D_PIXMAP(("Detected transparency option.  Enabling transparency on image %s\n", get_image_type(i)));
      image_set_mode(i, MODE_TRANS);
    } else if ((image_toggles & IMOPT_VIEWPORT) && (image_mode_is(i, ALLOW_VIEWPORT))) {
      D_PIXMAP(("Detected viewport option.  Enabling viewport mode on image %s\n", get_image_type(i)));
      image_set_mode(i, MODE_VIEWPORT);
    }
  }

  if (rs_cmod_image) {
    unsigned char n = NumWords(rs_cmod_image);
    imlib_t *iml = images[image_bg].norm->iml;

    RESET_AND_ASSIGN(iml->mod, (ImlibColorModifier *) MALLOC(sizeof(ImlibColorModifier)));
    iml->mod->contrast = iml->mod->gamma = 0xff;
    iml->mod->brightness = (int) strtol(rs_cmod_image, (char **) NULL, 0);
    if (n > 1) {
      iml->mod->contrast = (int) strtol(PWord(2, rs_cmod_image), (char **) NULL, 0);
    }
    if (n > 2) {
      iml->mod->gamma = (int) strtol(PWord(3, rs_cmod_image), (char **) NULL, 0);
    }
    FREE(rs_cmod_image);
  }
  if (rs_cmod_red) {
    unsigned char n = NumWords(rs_cmod_red);
    imlib_t *iml = images[image_bg].norm->iml;

    RESET_AND_ASSIGN(iml->rmod, (ImlibColorModifier *) MALLOC(sizeof(ImlibColorModifier)));
    iml->rmod->contrast = iml->rmod->gamma = 0xff;
    iml->rmod->brightness = (int) strtol(rs_cmod_red, (char **) NULL, 0);
    if (n > 1) {
      iml->rmod->contrast = (int) strtol(PWord(2, rs_cmod_red), (char **) NULL, 0);
    }
    if (n > 2) {
      iml->rmod->gamma = (int) strtol(PWord(3, rs_cmod_red), (char **) NULL, 0);
    }
    FREE(rs_cmod_red);
  }
  if (rs_cmod_green) {
    unsigned char n = NumWords(rs_cmod_green);
    imlib_t *iml = images[image_bg].norm->iml;

    RESET_AND_ASSIGN(iml->gmod, (ImlibColorModifier *) MALLOC(sizeof(ImlibColorModifier)));
    iml->gmod->contrast = iml->gmod->gamma = 0xff;
    iml->gmod->brightness = (int) strtol(rs_cmod_green, (char **) NULL, 0);
    if (n > 1) {
      iml->gmod->contrast = (int) strtol(PWord(2, rs_cmod_green), (char **) NULL, 0);
    }
    if (n > 2) {
      iml->gmod->gamma = (int) strtol(PWord(3, rs_cmod_green), (char **) NULL, 0);
    }
    FREE(rs_cmod_green);
  }
  if (rs_cmod_blue) {
    unsigned char n = NumWords(rs_cmod_blue);
    imlib_t *iml = images[image_bg].norm->iml;

    RESET_AND_ASSIGN(iml->bmod, (ImlibColorModifier *) MALLOC(sizeof(ImlibColorModifier)));
    iml->bmod->contrast = iml->bmod->gamma = 0xff;
    iml->bmod->brightness = (int) strtol(rs_cmod_blue, (char **) NULL, 0);
    if (n > 1) {
      iml->bmod->contrast = (int) strtol(PWord(2, rs_cmod_blue), (char **) NULL, 0);
    }
    if (n > 2) {
      iml->bmod->gamma = (int) strtol(PWord(3, rs_cmod_blue), (char **) NULL, 0);
    }
    FREE(rs_cmod_blue);
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

#ifdef BACKGROUND_CYCLING_SUPPORT
  if (rs_anim_pixmap_list != NULL) {
    rs_anim_delay = strtoul(rs_anim_pixmap_list, (char **) NULL, 0);
    fflush(stdout);
    if (rs_anim_delay == 0) {
      FREE(rs_anim_pixmap_list);
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
	    strcat(rs_anim_pixmaps[i], "@100x100");
	  } else {
	    rs_anim_pixmaps[i] = Word(3, temp);
	    rs_anim_pixmaps[i] = (char *) realloc(rs_anim_pixmaps[i], strlen(rs_anim_pixmaps[i]) + 5);
	    strcat(rs_anim_pixmaps[i], "@0x0");
	  }
	  FREE(temp);
	}
      }
      rs_anim_pixmaps[count] = NULL;
      FREE(rs_anim_pixmap_list);
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
  simage_t *simg;
  action_t *action;

  cur_tm = localtime(&cur_time);

  if (!path) {
    path = (char *) MALLOC(PATH_MAX + 1);
    snprintf(path, PATH_MAX, "%s/%s", getenv("ETERM_THEME_ROOT"), (rs_config_file ? rs_config_file : USER_CFG));
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
  strftime(dt_stamp, 50, "%Y/%m/%d at %X", cur_tm);
  fprintf(fp, "<" APL_NAME "-" VERSION ">\n");
  fprintf(fp, "# Eterm Configuration File\n");
  fprintf(fp, "# Automatically generated by " APL_NAME "-" VERSION " on %s\n", dt_stamp);

  fprintf(fp, "begin main\n\n");

  fprintf(fp, "  begin color\n");
  fprintf(fp, "    foreground %s\n", rs_color[fgColor]);
  fprintf(fp, "    background %s\n", rs_color[bgColor]);
  fprintf(fp, "    cursor %s\n", rs_color[cursorColor]);
  fprintf(fp, "    cursor_text %s\n", rs_color[cursorColor2]);
  if (rs_color[menuColor]) {
    fprintf(fp, "    menu %s\n", rs_color[menuColor]);
  }
  if (rs_color[unfocusedMenuColor]) {
    fprintf(fp, "    unfocused_menu %s\n", rs_color[unfocusedMenuColor]);
  }
  if (rs_color[menuTextColor]) {
    fprintf(fp, "    menu_text %s\n", rs_color[menuTextColor]);
  }
  if (rs_color[scrollColor]) {
    fprintf(fp, "    scrollbar %s\n", rs_color[scrollColor]);
  }
  if (rs_color[unfocusedScrollColor]) {
    fprintf(fp, "    unfocused_scrollbar %s\n", rs_color[unfocusedScrollColor]);
  }
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
  fprintf(fp, "    scrollbar_width %d\n", scrollbar_anchor_width());
  for (i = 0; i < 5; i++) {
    fprintf(fp, "    font %d %s\n", i, rs_font[i]);
  }
#ifndef NO_BOLDFONT
  if (rs_boldFont) {
    fprintf(fp, "    font bold %s\n", rs_boldFont);
  }
#endif
  fprintf(fp, "  end attributes\n\n");

  fprintf(fp, "  begin imageclasses\n");
  fprintf(fp, "    path \"%s\"\n", rs_path);
  if (rs_icon != NULL) {
    fprintf(fp, "    icon %s\n", rs_icon);
  }
  if (rs_anim_delay) {
    /* FIXME:  Do something here! */
  }
  for (i = 0; i < image_max; i++) {
    fprintf(fp, "    begin image\n");
    switch (i) {
      case image_bg:      fprintf(fp, "      type background\n"); break;
      case image_sb:      fprintf(fp, "      type trough\n"); break;
      case image_sa:      fprintf(fp, "      type anchor\n"); break;
      case image_up:      fprintf(fp, "      type up_arrow\n"); break;
      case image_down:    fprintf(fp, "      type down_arrow\n"); break;
      case image_left:    fprintf(fp, "      type left_arrow\n"); break;
      case image_right:   fprintf(fp, "      type right_arrow\n"); break;
      case image_menu:    fprintf(fp, "      type menu\n"); break;
      case image_submenu: fprintf(fp, "      type submenu\n"); break;
    }
    fprintf(fp, "      mode ");
    switch (images[i].mode & MODE_MASK) {
      case MODE_IMAGE:     fprintf(fp, "image"); break;
      case MODE_TRANS:     fprintf(fp, "trans"); break;
      case MODE_VIEWPORT:  fprintf(fp, "viewport"); break;
      case MODE_AUTO:      fprintf(fp, "auto"); break;
      default:             fprintf(fp, "solid"); break;
    }
    if (images[i].mode & ALLOW_MASK) {
      fprintf(fp, " allow");
      if (image_mode_is(i, ALLOW_IMAGE)) {
        fprintf(fp, " image");
      }
      if (image_mode_is(i, ALLOW_TRANS)) {
        fprintf(fp, " trans");
      }
      if (image_mode_is(i, ALLOW_VIEWPORT)) {
        fprintf(fp, " viewport");
      }
      if (image_mode_is(i, ALLOW_AUTO)) {
        fprintf(fp, " auto");
      }
    }
    fprintf(fp, "\n");

    /* Now save each state. */
    simg = images[i].norm;
    fprintf(fp, "      state normal\n");
    if (simg->iml->im) {
      fprintf(fp, "      file %s\n", NONULL(simg->iml->im->filename));
    }
    fprintf(fp, "      geom %hdx%hd+%hd+%hd", simg->pmap->w, simg->pmap->h, simg->pmap->x, simg->pmap->y);
    if (simg->pmap->op & OP_TILE) {
      fprintf(fp, ":tiled");
    }
    if ((simg->pmap->op & OP_SCALE) || ((simg->pmap->op & OP_HSCALE) && (simg->pmap->op & OP_VSCALE))) {
      fprintf(fp, ":scaled");
    } else if (simg->pmap->op & OP_HSCALE) {
      fprintf(fp, ":hscaled");
    } else if (simg->pmap->op & OP_VSCALE) {
      fprintf(fp, ":vscaled");
    }
    if (simg->pmap->op & OP_PROPSCALE) {
      fprintf(fp, ":propscaled");
    }
    fprintf(fp, "\n");
    if (simg->iml->mod) {
      fprintf(fp, "      colormod image 0x%02x 0x%02x 0x%02x\n", simg->iml->mod->brightness, simg->iml->mod->contrast, simg->iml->mod->gamma);
    }
    if (simg->iml->rmod) {
      fprintf(fp, "      colormod red 0x%02x 0x%02x 0x%02x\n", simg->iml->rmod->brightness, simg->iml->rmod->contrast, simg->iml->rmod->gamma);
    }
    if (simg->iml->gmod) {
      fprintf(fp, "      colormod green 0x%02x 0x%02x 0x%02x\n", simg->iml->gmod->brightness, simg->iml->gmod->contrast, simg->iml->gmod->gamma);
    }
    if (simg->iml->bmod) {
      fprintf(fp, "      colormod blue 0x%02x 0x%02x 0x%02x\n", simg->iml->bmod->brightness, simg->iml->bmod->contrast, simg->iml->bmod->gamma);
    }
    if (simg->iml->border) {
      fprintf(fp, "      border %hu %hu %hu %hu\n", simg->iml->border->left, simg->iml->border->right, simg->iml->border->top, simg->iml->border->bottom);
    }
    if (simg->iml->bevel) {
      fprintf(fp, "      bevel %s %hu %hu %hu %hu\n", ((simg->iml->bevel->up) ? "up" : "down"), simg->iml->bevel->edges->left, simg->iml->bevel->edges->right, simg->iml->bevel->edges->top, simg->iml->bevel->edges->bottom);
    }
    if (simg->iml->pad) {
      fprintf(fp, "      padding %hu %hu %hu %hu\n", simg->iml->pad->left, simg->iml->pad->right, simg->iml->pad->top, simg->iml->pad->bottom);
    }

    /* Selected state */
    if (images[i].selected != images[i].norm) {
      simg = images[i].selected;
      fprintf(fp, "      state selected\n");
      if (simg->iml->im) {
        fprintf(fp, "      file %s\n", NONULL(simg->iml->im->filename));
      }
      fprintf(fp, "      geom %hdx%hd+%hd+%hd", simg->pmap->w, simg->pmap->h, simg->pmap->x, simg->pmap->y);
      if (simg->pmap->op & OP_TILE) {
        fprintf(fp, ":tiled");
      }
      if ((simg->pmap->op & OP_SCALE) || ((simg->pmap->op & OP_HSCALE) && (simg->pmap->op & OP_VSCALE))) {
        fprintf(fp, ":scaled");
      } else if (simg->pmap->op & OP_HSCALE) {
        fprintf(fp, ":hscaled");
      } else if (simg->pmap->op & OP_VSCALE) {
        fprintf(fp, ":vscaled");
      }
      if (simg->pmap->op & OP_PROPSCALE) {
        fprintf(fp, ":propscaled");
      }
      fprintf(fp, "\n");
      if (simg->iml->mod) {
        fprintf(fp, "      colormod image 0x%02x 0x%02x 0x%02x\n", simg->iml->mod->brightness, simg->iml->mod->contrast, simg->iml->mod->gamma);
      }
      if (simg->iml->rmod) {
        fprintf(fp, "      colormod red 0x%02x 0x%02x 0x%02x\n", simg->iml->rmod->brightness, simg->iml->rmod->contrast, simg->iml->rmod->gamma);
      }
      if (simg->iml->gmod) {
        fprintf(fp, "      colormod green 0x%02x 0x%02x 0x%02x\n", simg->iml->gmod->brightness, simg->iml->gmod->contrast, simg->iml->gmod->gamma);
      }
      if (simg->iml->bmod) {
        fprintf(fp, "      colormod blue 0x%02x 0x%02x 0x%02x\n", simg->iml->bmod->brightness, simg->iml->bmod->contrast, simg->iml->bmod->gamma);
      }
      if (simg->iml->border) {
        fprintf(fp, "      border %hu %hu %hu %hu\n", simg->iml->border->left, simg->iml->border->right, simg->iml->border->top, simg->iml->border->bottom);
      }
      if (simg->iml->bevel) {
        fprintf(fp, "      bevel %s %hu %hu %hu %hu\n", ((simg->iml->bevel->up) ? "up" : "down"), simg->iml->bevel->edges->left, simg->iml->bevel->edges->right, simg->iml->bevel->edges->top, simg->iml->bevel->edges->bottom);
      }
      if (simg->iml->pad) {
        fprintf(fp, "      padding %hu %hu %hu %hu\n", simg->iml->pad->left, simg->iml->pad->right, simg->iml->pad->top, simg->iml->pad->bottom);
      }
    }

    /* Clicked state */
    if (images[i].clicked != images[i].norm) {
      simg = images[i].clicked;
      fprintf(fp, "      state clicked\n");
      if (simg->iml->im) {
        fprintf(fp, "      file %s\n", NONULL(simg->iml->im->filename));
      }
      fprintf(fp, "      geom %hdx%hd+%hd+%hd", simg->pmap->w, simg->pmap->h, simg->pmap->x, simg->pmap->y);
      if (simg->pmap->op & OP_TILE) {
        fprintf(fp, ":tiled");
      }
      if ((simg->pmap->op & OP_SCALE) || ((simg->pmap->op & OP_HSCALE) && (simg->pmap->op & OP_VSCALE))) {
        fprintf(fp, ":scaled");
      } else if (simg->pmap->op & OP_HSCALE) {
        fprintf(fp, ":hscaled");
      } else if (simg->pmap->op & OP_VSCALE) {
        fprintf(fp, ":vscaled");
      }
      if (simg->pmap->op & OP_PROPSCALE) {
        fprintf(fp, ":propscaled");
      }
      fprintf(fp, "\n");
      if (simg->iml->mod) {
        fprintf(fp, "      colormod image 0x%02x 0x%02x 0x%02x\n", simg->iml->mod->brightness, simg->iml->mod->contrast, simg->iml->mod->gamma);
      }
      if (simg->iml->rmod) {
        fprintf(fp, "      colormod red 0x%02x 0x%02x 0x%02x\n", simg->iml->rmod->brightness, simg->iml->rmod->contrast, simg->iml->rmod->gamma);
      }
      if (simg->iml->gmod) {
        fprintf(fp, "      colormod green 0x%02x 0x%02x 0x%02x\n", simg->iml->gmod->brightness, simg->iml->gmod->contrast, simg->iml->gmod->gamma);
      }
      if (simg->iml->bmod) {
        fprintf(fp, "      colormod blue 0x%02x 0x%02x 0x%02x\n", simg->iml->bmod->brightness, simg->iml->bmod->contrast, simg->iml->bmod->gamma);
      }
      if (simg->iml->border) {
        fprintf(fp, "      border %hu %hu %hu %hu\n", simg->iml->border->left, simg->iml->border->right, simg->iml->border->top, simg->iml->border->bottom);
      }
      if (simg->iml->bevel) {
        fprintf(fp, "      bevel %s %hu %hu %hu %hu\n", ((simg->iml->bevel->up) ? "up" : "down"), simg->iml->bevel->edges->left, simg->iml->bevel->edges->right, simg->iml->bevel->edges->top, simg->iml->bevel->edges->bottom);
      }
      if (simg->iml->pad) {
        fprintf(fp, "      padding %hu %hu %hu %hu\n", simg->iml->pad->left, simg->iml->pad->right, simg->iml->pad->top, simg->iml->pad->bottom);
      }
    }
    fprintf(fp, "    end image\n");
  }
  fprintf(fp, "  end imageclasses\n\n");

  for (i = 0; i < menu_list->nummenus; i++) {
    menu_t *menu = menu_list->menus[i];
    unsigned short j;

    fprintf(fp, "  begin menu\n");
    fprintf(fp, "    title \"%s\"\n", menu->title);
    if (menu->font) {
      unsigned long tmp;

      if ((XGetFontProperty(menu->font, XA_FONT_NAME, &tmp)) == True) {
        fprintf(fp, "    font \"%s\"\n", ((char *) tmp));
      }
    }
    for (j = 0; j < menu->numitems; j++) {
      menuitem_t *item = menu->items[j];

      if (item->type == MENUITEM_SEP) {
        fprintf(fp, "    -\n");
      } else {
        fprintf(fp, "    begin menuitem\n");
        fprintf(fp, "      text \"%s\"\n", item->text);
        if (item->rtext) {
          fprintf(fp, "      rtext \"%s\"\n", item->rtext);
        }
        fprintf(fp, "      action ");
        if (item->type == MENUITEM_STRING) {
          fprintf(fp, "string \"%s\"\n", item->action.string);
        } else if (item->type == MENUITEM_ECHO) {
          fprintf(fp, "echo \"%s\"\n", item->action.string);
        } else if (item->type == MENUITEM_SUBMENU) {
          fprintf(fp, "submenu \"%s\"\n", (item->action.submenu)->title);
        }
        fprintf(fp, "    end\n");
      }
    }
    fprintf(fp, "  end menu\n");
  }
  fprintf(fp, "\n");

  fprintf(fp, "  begin actions\n");
  for (action = action_list; action; action = action->next) {
    fprintf(fp, "    bind ");
    if (action->mod != MOD_NONE) {
      if (action->mod & MOD_ANY) {
        fprintf(fp, "anymod ");
      }
      if (action->mod & MOD_CTRL) {
        fprintf(fp, "ctrl ");
      }
      if (action->mod & MOD_SHIFT) {
        fprintf(fp, "shift ");
      }
      if (action->mod & MOD_LOCK) {
        fprintf(fp, "lock ");
      }
      if (action->mod & MOD_MOD1) {
        fprintf(fp, "mod1 ");
      }
      if (action->mod & MOD_MOD2) {
        fprintf(fp, "mod2 ");
      }
      if (action->mod & MOD_MOD3) {
        fprintf(fp, "mod3 ");
      }
      if (action->mod & MOD_MOD4) {
        fprintf(fp, "mod4 ");
      }
      if (action->mod & MOD_MOD5) {
        fprintf(fp, "mod5 ");
      }
      if (action->keysym) {
        fprintf(fp, "0x%04x", (unsigned int) action->keysym);
      } else {
        fprintf(fp, "button");
        if (action->button == Button5) {
          fprintf(fp, "5");
        } else if (action->button == Button4) {
          fprintf(fp, "4");
        } else if (action->button == Button3) {
          fprintf(fp, "3");
        } else if (action->button == Button2) {
          fprintf(fp, "2");
        } else {
          fprintf(fp, "1");
        }
      }
      fprintf(fp, " to ");
      if (action->type == ACTION_STRING) {
        fprintf(fp, "string \"%s\"\n", action->param.string);
      } else if (action->type == ACTION_ECHO) {
        fprintf(fp, "echo \"%s\"\n", action->param.string);
      } else if (action->type == ACTION_MENU) {
        fprintf(fp, "menu \"%s\"\n", (action->param.menu)->title);
      }
    }
  }
  fprintf(fp, "  end actions\n\n");

#ifdef MULTI_CHARSET
  fprintf(fp, "  begin multichar\n");
  fprintf(fp, "    encoding %s\n", rs_multchar_encoding);
  for (i = 0; i < 5; i++) {
    fprintf(fp, "    font %d %s\n", i, rs_mfont[i]);
  }
  fprintf(fp, "  end multichar\n\n");
#endif

#ifdef USE_XIM
  fprintf(fp, "  begin xim\n");
  fprintf(fp, "    input_method %s\n", rs_input_method);
  fprintf(fp, "    preedit_type %s\n", rs_preedit_type);
  fprintf(fp, "  end xim\n\n");
#endif

  fprintf(fp, "  begin toggles\n");
  fprintf(fp, "    map_alert %d\n", (Options & Opt_mapAlert ? 1 : 0));
  fprintf(fp, "    visual_bell %d\n", (Options & Opt_visualBell ? 1 : 0));
  fprintf(fp, "    login_shell %d\n", (Options & Opt_loginShell ? 1 : 0));
  fprintf(fp, "    scrollbar %d\n", (Options & Opt_scrollBar ? 1 : 0));
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
  fprintf(fp, "    backing_store %d\n", (Options & Opt_backing_store ? 1 : 0));
  fprintf(fp, "    no_cursor %d\n", (Options & Opt_noCursor ? 1 : 0));
  fprintf(fp, "    pause %d\n", (Options & Opt_pause ? 1 : 0));
  fprintf(fp, "    xterm_select %d\n", (Options & Opt_xterm_select ? 1 : 0));
  fprintf(fp, "    select_line %d\n", (Options & Opt_select_whole_line ? 1 : 0));
  fprintf(fp, "    select_trailing_spaces %d\n", (Options & Opt_select_trailing_spaces ? 1 : 0));
  fprintf(fp, "    report_as_keysyms %d\n", (Options & Opt_report_as_keysyms ? 1 : 0));
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
