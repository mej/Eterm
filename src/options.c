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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <X11/keysym.h>

#include "../libmej/debug.h"
#include "../libmej/mem.h"
#include "../libmej/strings.h"
#include "debug.h"
#include "actions.h"
#include "buttons.h"
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

static void *parse_color(char *, void *);
static void *parse_attributes(char *, void *);
static void *parse_toggles(char *, void *);
static void *parse_keyboard(char *, void *);
static void *parse_misc(char *, void *);
static void *parse_imageclasses(char *, void *);
static void *parse_image(char *, void *);
static void *parse_actions(char *, void *);
static void *parse_menu(char *, void *);
static void *parse_menuitem(char *, void *);
static void *parse_bbar(char *, void *);
static void *parse_xim(char *, void *);
static void *parse_multichar(char *, void *);
static conf_var_t *conf_new_var(void);
static void conf_free_var(conf_var_t *);
static char *conf_get_var(const char *);
static void conf_put_var(char *, char *);
static char *builtin_random(char *);
static char *builtin_exec(char *);
static char *builtin_get(char *);
static char *builtin_put(char *);
static char *builtin_dirscan(char *);
static char *builtin_version(char *);
static char *builtin_appname(char *);
static void *parse_null(char *, void *);

static ctx_t *context;
static ctx_state_t *ctx_state;
static eterm_func_t *builtins;
static unsigned char ctx_cnt, ctx_idx, ctx_state_idx, ctx_state_cnt, fstate_cnt, builtin_cnt, builtin_idx;
static conf_var_t *conf_vars = NULL;
#if defined (HOTKEY_CTRL) || defined (HOTKEY_META)
static char *rs_bigfont_key = NULL;
static char *rs_smallfont_key = NULL;
#endif
#ifdef MULTI_CHARSET
static char *rs_multichar_encoding = NULL;
#endif
#ifdef GREEK_SUPPORT
static char *rs_greek_keyboard = NULL;
#endif

const char *true_vals[] =
{"1", "on", "true", "yes"};
const char *false_vals[] =
{"0", "off", "false", "no"};

fstate_t *fstate;
unsigned char fstate_idx;
unsigned long Options = (Opt_scrollbar), image_toggles = 0;
char *theme_dir = NULL, *user_dir = NULL;
char **rs_execArgs = NULL;	/* Args to exec (-e or --exec) */
char *rs_title = NULL;		/* Window title */
char *rs_iconName = NULL;	/* Icon name */
char *rs_geometry = NULL;	/* Geometry string */
int rs_desktop = -1;
char *rs_path = NULL;
int rs_saveLines = SAVELINES;	/* Lines in the scrollback buffer */
#ifdef USE_XIM
char *rs_input_method = NULL;
char *rs_preedit_type = NULL;
#endif
char *rs_name = NULL;
#ifndef NO_BOLDFONT
char *rs_boldFont = NULL;
#endif
#ifdef PRINTPIPE
char *rs_print_pipe = NULL;
#endif
char *rs_cutchars = NULL;
unsigned short rs_min_anchor_size = 0;
char *rs_scrollbar_type = NULL;
unsigned long rs_scrollbar_width = 0;
char *rs_term_name = NULL;
#ifdef PIXMAP_SUPPORT
char *rs_pixmapScale = NULL;
char *rs_backing_store = NULL;
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
unsigned int rs_meta_mod = 0, rs_alt_mod = 0, rs_numlock_mod = 0;
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
      OPT_LONG("menu-color", "menu color", &rs_color[menuColor]),
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
      OPT_ILONG("default-font-index", "set the index of the default font", &def_font_idx),
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
	       &rs_multichar_encoding),
#endif /* MULTI_CHARSET */
#ifdef USE_XIM
      OPT_LONG("input-method", "XIM input method", &rs_input_method),
      OPT_LONG("preedit-type", "XIM preedit type", &rs_preedit_type),
#endif

/* =======[ Toggles ]======= */
      OPT_BOOL('l', "login-shell", "login shell, prepend - to shell name", &Options, Opt_loginShell),
      OPT_BOOL('s', "scrollbar", "display scrollbar", &Options, Opt_scrollbar),
      OPT_BOOL('u', "utmp-logging", "make a utmp entry", &Options, Opt_utmpLogging),
      OPT_BOOL('v', "visual-bell", "visual bell", &Options, Opt_visualBell),
      OPT_BOOL('H', "home-on-echo", "jump to bottom on output", &Options, Opt_home_on_output),
      OPT_BLONG("home-on-input", "jump to bottom on input", &Options, Opt_home_on_input),
      OPT_BLONG("scrollbar-right", "display the scrollbar on the right", &Options, Opt_scrollbar_right),
      OPT_BLONG("scrollbar-floating", "display the scrollbar with no trough", &Options, Opt_scrollbar_floating),
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
      OPT_BLONG("double-buffer", "use double-buffering to reduce exposes (uses more memory)", &Options, Opt_double_buffer),
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
      OPT_ILONG("meta-mod", "modifier to interpret as the Meta key", &rs_meta_mod),
      OPT_ILONG("alt-mod", "modifier to interpret as the Alt key", &rs_alt_mod),
      OPT_ILONG("numlock-mod", "modifier to interpret as the NumLock key", &rs_numlock_mod),
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
  printf("Copyright (c) 1997-2000, " AUTHORS "\n\n");
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
  printf("Copyright (c) 1997-2000, " AUTHORS "\n\n");

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
#ifdef PTY_GRP_NAME
  printf(" PTY_GRP_NAME=\"%s\"\n", PTY_GRP_NAME);
#else
  printf(" -PTY_GRP_NAME\n");
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
	  if (*argv[i + 1] != '-' || StrCaseStr(optList[j].long_opt, "font") || StrCaseStr(optList[j].long_opt, "geometry")) {
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

	  rs_execArgs = (char **) MALLOC(sizeof(char *) * (argc - i + 1));

	  for (k = 0; k < len; k++) {
	    rs_execArgs[k] = StrDup(argv[k + i]);
	    D_OPTIONS(("rs_execArgs[%d] == %s\n", k, rs_execArgs[k]));
	  }
	  rs_execArgs[k] = (char *) NULL;
	  return;
	} else {

	  register unsigned short k;

	  rs_execArgs = (char **) MALLOC(sizeof(char *) * (NumWords(val_ptr) + 1));

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
	  if ((val_ptr == NULL) || ((*val_ptr == '-') && (optList[j].short_opt != 'F') && (optList[j].short_opt != 'g'))) {
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
	  rs_execArgs = (char **) MALLOC(sizeof(char *) * len);

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
      } else if (!BEG_STRCASECMP(opt, "exec") && (*(opt+4) != '=')) {
        break;
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
	} else if (opt[pos] == 'e') {
	  break;
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
/* This function must be called before any other conf_*() function.
   Otherwise you will be bitten by dragons.  That's life. */
void
conf_init_subsystem(void) {

  /* Initialize the context list and establish a catch-all "null" context */
  ctx_cnt = 20;
  ctx_idx = 0;
  context = (ctx_t *) malloc(sizeof(ctx_t) * ctx_cnt);
  MEMSET(context, 0, sizeof(ctx_t) * ctx_cnt);
  context[0].name = "null";
  context[0].handler = parse_null;

  /* Initialize the context state stack and set the current context to "null" */
  ctx_state_cnt = 20;
  ctx_state_idx = 0;
  ctx_state = (ctx_state_t *) malloc(sizeof(ctx_state_t) * ctx_state_cnt);
  MEMSET(ctx_state, 0, sizeof(ctx_state_t) * ctx_state_cnt);

  /* Initialize the file state stack */
  fstate_cnt = 10;
  fstate_idx = 0;
  fstate = (fstate_t *) malloc(sizeof(fstate_t) * fstate_cnt);
  MEMSET(fstate, 0, sizeof(fstate_t) * fstate_cnt);

  /* Initialize the builtin function table */
  builtin_cnt = 10;
  builtin_idx = 0;
  builtins = (eterm_func_t *) malloc(sizeof(eterm_func_t) * builtin_cnt);
  MEMSET(builtins, 0, sizeof(eterm_func_t) * builtin_cnt);

  /* Register the omni-present builtin functions */
  conf_register_builtin("appname", builtin_appname);
  conf_register_builtin("version", builtin_version);
  conf_register_builtin("exec", builtin_exec);
  conf_register_builtin("random", builtin_random);
  conf_register_builtin("get", builtin_get);
  conf_register_builtin("put", builtin_put);
  conf_register_builtin("dirscan", builtin_dirscan);

  /* For expediency, Eterm registers its contexts here.  If you want to use
     this parser in another program, remove these lines. */
  conf_register_context("color", parse_color);
  conf_register_context("attributes", parse_attributes);
  conf_register_context("toggles", parse_toggles);
  conf_register_context("keyboard", parse_keyboard);
  conf_register_context("misc", parse_misc);
  conf_register_context("imageclasses", parse_imageclasses);
  conf_register_context("image", parse_image);
  conf_register_context("actions", parse_actions);
  conf_register_context("menu", parse_menu);
  conf_register_context("menuitem", parse_menuitem);
  conf_register_context("button_bar", parse_bbar);
  conf_register_context("xim", parse_xim);
  conf_register_context("multichar", parse_multichar);
}

/* Register a new config file context */
unsigned char
conf_register_context(char *name, ctx_handler_t handler) {

  if (++ctx_idx == ctx_cnt) {
    ctx_cnt *= 2;
    context = (ctx_t *) realloc(context, sizeof(ctx_t) * ctx_cnt);
  }
  context[ctx_idx].name = strdup(name);
  context[ctx_idx].handler = handler;
  D_OPTIONS(("conf_register_context():  Added context \"%s\" with ID %d and handler 0x%08x\n",
     context[ctx_idx].name, ctx_idx, context[ctx_idx].handler));
  return (ctx_idx);
}

/* Register a new file state structure */
unsigned char
conf_register_fstate(FILE *fp, char *path, char *outfile, unsigned long line, unsigned char flags) {

  if (++fstate_idx == fstate_cnt) {
    fstate_cnt *= 2;
    fstate = (fstate_t *) realloc(fstate, sizeof(fstate_t) * fstate_cnt);
  }
  fstate[fstate_idx].fp = fp;
  fstate[fstate_idx].path = path;
  fstate[fstate_idx].outfile = outfile;
  fstate[fstate_idx].line = line;
  fstate[fstate_idx].flags = flags;
  return (fstate_idx);
}

/* Register a new builtin function */
unsigned char
conf_register_builtin(char *name, eterm_func_ptr_t ptr) {

  builtins[builtin_idx].name = strdup(name);
  builtins[builtin_idx].ptr = ptr;
  if (++builtin_idx == builtin_cnt) {
    builtin_cnt *= 2;
    builtins = (eterm_func_t *) realloc(builtins, sizeof(eterm_func_t) * builtin_cnt);
  }
  return (builtin_idx - 1);
}

/* Register a new config file context */
unsigned char
conf_register_context_state(unsigned char ctx_id) {

  if (++ctx_state_idx == ctx_state_cnt) {
    ctx_state_cnt *= 2;
    ctx_state = (ctx_state_t *) realloc(ctx_state, sizeof(ctx_state_t) * ctx_state_cnt);
  }
  ctx_state[ctx_state_idx].ctx_id = ctx_id;
  ctx_state[ctx_state_idx].state = NULL;
  return (ctx_state_idx);
}

static conf_var_t *
conf_new_var(void)
{
  conf_var_t *v;

  v = (conf_var_t *) MALLOC(sizeof(conf_var_t));
  MEMSET(v, 0, sizeof(conf_var_t));
  return v;
}

static void
conf_free_var(conf_var_t *v)
{
  if (v->var) {
    FREE(v->var);
  }
  if (v->value) {
    FREE(v->value);
  }
  FREE(v);
}

static char *
conf_get_var(const char *var)
{
  conf_var_t *v;

  D_OPTIONS(("var == \"%s\"\n", var));
  for (v = conf_vars; v; v = v->next) {
    if (!strcmp(v->var, var)) {
      D_OPTIONS(("Found it at %8p:  \"%s\" == \"%s\"\n", v, v->var, v->value));
      return (v->value);
    }
  }
  D_OPTIONS(("Not found.\n"));
  return NULL;
}

static void
conf_put_var(char *var, char *val)
{
  conf_var_t *v, *loc = NULL, *tmp;

  ASSERT(var != NULL);
  D_OPTIONS(("var == \"%s\", val == \"%s\"\n", var, val));

  for (v = conf_vars; v; loc = v, v = v->next) {
    int n;

    n = strcmp(var, v->var);
    D_OPTIONS(("Comparing at %8p:  \"%s\" -> \"%s\", n == %d\n", v, v->var, v->value, n));
    if (n == 0) {
      FREE(v->value);
      if (val) {
        v->value = val;
        D_OPTIONS(("Variable already defined.  Replacing its value with \"%s\"\n", v->value));
      } else {
        D_OPTIONS(("Variable already defined.  Deleting it.\n"));
        if (loc) {
          loc->next = v->next;
        } else {
          conf_vars = v->next;
        }
        conf_free_var(v);
      }
      return;
    } else if (n < 0) {
      break;
    }
  }
  if (!val) {
    D_OPTIONS(("Empty value given for non-existant variable \"%s\".  Aborting.\n", var));
    return;
  }
  D_OPTIONS(("Inserting new var/val pair between \"%s\" and \"%s\"\n", ((loc) ? loc->var : "-beginning-"), ((v) ? v->var : "-end-")));
  tmp = conf_new_var();
  if (loc == NULL) {
    tmp->next = conf_vars;
    conf_vars = tmp;
  } else {
    tmp->next = loc->next;
    loc->next = tmp;
  }
  tmp->var = var;
  tmp->value = val;
}

static char *
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

static char *
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

static char *
builtin_get(char *param)
{
  char *s, *f, *v;
  unsigned short n;

  if (!param || ((n = NumWords(param)) > 2)) {
    print_error("Parse error in file %s, line %lu:  Invalid syntax for %get().  Syntax is:  %get(variable)", file_peek_path(), file_peek_line());
    return NULL;
  }

  D_PARSE(("builtin_get(%s) called\n", param));
  s = Word(1, param);
  if (n == 2) {
    f = Word(2, param);
  } else {
    f = NULL;
  }
  v = conf_get_var(s);
  FREE(s);
  if (v) {
    if (f) {
      FREE(f);
    }
    return (StrDup(v));
  } else if (f) {
    return f;
  } else {
    return NULL;
  }
}

static char *
builtin_put(char *param)
{
  char *var, *val;

  if (!param || (NumWords(param) != 2)) {
    print_error("Parse error in file %s, line %lu:  Invalid syntax for %put().  Syntax is:  %put(variable value)", file_peek_path(), file_peek_line());
    return NULL;
  }

  D_PARSE(("builtin_put(%s) called\n", param));
  var = Word(1, param);
  val = Word(2, param);
  conf_put_var(var, val);
  return NULL;
}

static char *
builtin_dirscan(char *param)
{
  int i;
  unsigned long n;
  DIR *dirp;
  struct dirent *dp;
  struct stat filestat;
  char *dir, *buff;

  if (!param || (NumWords(param) != 1)) {
    print_error("Parse error in file %s, line %lu:  Invalid syntax for %dirscan().  Syntax is:  %dirscan(directory)", file_peek_path(), file_peek_line());
    return NULL;
  }
  D_PARSE(("builtin_dirscan(%s)\n", param));
  dir = Word(1, param);
  dirp = opendir(dir);
  if (!dirp) {
    return NULL;
  }
  buff = (char *) MALLOC(CONFIG_BUFF);
  *buff = 0;
  n = CONFIG_BUFF;

  for (i = 0; (dp = readdir(dirp)) != NULL;) {
    char fullname[PATH_MAX];

    snprintf(fullname, sizeof(fullname), "%s/%s", dir, dp->d_name);
    if (stat(fullname, &filestat)) {
      D_PARSE((" -> Couldn't stat() file %s -- %s\n", fullname, strerror(errno)));
    } else {
      if (S_ISREG(filestat.st_mode)) {
        unsigned long len;

        len = strlen(dp->d_name);
        if (len < n) {
          strcat(buff, dp->d_name);
          strcat(buff, " ");
          n -= len + 1;
        }
      }
    }
    if (n < 2) {
      break;
    }
  }
  closedir(dirp);
  return buff;
}

static char *
builtin_version(char *param)
{

  if (param) {
    D_PARSE(("builtin_version(%s) called\n", param));
  }

  return (StrDup(VERSION));
}

static char *
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
              new[j] = '\b';
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
	  if (Output) {
            if (*Output) {
              l = strlen(Output) - 1;
              strncpy(new + j, Output, max - j);
              cnt2 = max - j - 1;
              j += MIN(l, cnt2);
            } else {
              j--;
            }
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
	  if (Output) {
            if (*Output) {
              l = strlen(Output) - 1;
              strncpy(new + j, Output, max - j);
              cnt2 = max - j - 1;
              j += MIN(l, cnt2);
            } else {
              j--;
            }
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
static void *
parse_color(char *buff, void *state)
{
  if ((*buff == CONF_BEGIN_CHAR) || (*buff == CONF_END_CHAR)) {
    return NULL;
  }
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
		  "attribute color", file_peek_path(), file_peek_line(), NONULL(tmp));
      return NULL;
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
	return NULL;
      } else {
	if (!BEG_STRCASECMP(tmp, "bd ")) {
#ifndef NO_BOLDUNDERLINE
	  RESET_AND_ASSIGN(rs_color[colorBD], Word(1, r1));
#else
	  print_warning("Support for the color bd attribute was not compiled in, ignoring");
#endif
	  return NULL;
	} else if (!BEG_STRCASECMP(tmp, "ul ")) {
#ifndef NO_BOLDUNDERLINE
	  RESET_AND_ASSIGN(rs_color[colorUL], Word(1, r1));
#else
	  print_warning("Support for the color ul attribute was not compiled in, ignoring");
#endif
	  return NULL;
	} else {
	  tmp = Word(1, tmp);
	  print_error("Parse error in file %s, line %lu:  Invalid color index \"%s\"",
		      file_peek_path(), file_peek_line(), NONULL(tmp));
	  FREE(tmp);
	}
      }
    }
    if (n != 5) {
      print_error("Parse error in file %s, line %lu:  Invalid parameter list \"%s\" for "
		  "attribute color", file_peek_path(), file_peek_line(), NONULL(tmp));
      return NULL;
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
		  file_peek_path(), file_peek_line(), NONULL(tmp));
      FREE(tmp);
    }
  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid "
		"within context color", file_peek_path(), file_peek_line(), buff);
  }
  return state;
}

static void *
parse_attributes(char *buff, void *state)
{
  if ((*buff == CONF_BEGIN_CHAR) || (*buff == CONF_END_CHAR)) {
    return NULL;
  }
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
		  "attribute font", file_peek_path(), file_peek_line(), NONULL(tmp));
      return NULL;
    }
    if (isdigit(*tmp)) {
      n = (unsigned char) strtoul(tmp, (char **) NULL, 0);
      if (n <= 255) {
        eterm_font_add(&etfonts, PWord(2, tmp), n);
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

    } else if (!BEG_STRCASECMP(tmp, "default ")) {
      def_font_idx = strtoul(PWord(2, tmp), (char **) NULL, 0);

    } else {
      tmp = Word(1, tmp);
      print_error("Parse error in file %s, line %lu:  Invalid font index \"%s\"",
		  file_peek_path(), file_peek_line(), NONULL(tmp));
      FREE(tmp);
    }

  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid "
		"within context attributes", file_peek_path(), file_peek_line(), (buff ? buff : ""));
  }
  return state;
}

static void *
parse_toggles(char *buff, void *state)
{
  char *tmp;
  unsigned char bool_val;

  if ((*buff == CONF_BEGIN_CHAR) || (*buff == CONF_END_CHAR)) {
    return NULL;
  }
  if (!(tmp = PWord(2, buff))) {
    print_error("Parse error in file %s, line %lu:  Missing boolean value in context toggles", file_peek_path(), file_peek_line());
    return NULL;
  }
  if (BOOL_OPT_ISTRUE(tmp)) {
    bool_val = 1;
  } else if (BOOL_OPT_ISFALSE(tmp)) {
    bool_val = 0;
  } else {
    print_error("Parse error in file %s, line %lu:  Invalid boolean value \"%s\" in context toggles",
		file_peek_path(), file_peek_line(), tmp);
    return NULL;
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
      Options |= Opt_scrollbar;
    } else {
      Options &= ~(Opt_scrollbar);
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

  } else if (!BEG_STRCASECMP(buff, "home_on_output ")) {
    if (bool_val) {
      Options |= Opt_home_on_output;
    } else {
      Options &= ~(Opt_home_on_output);
    }

  } else if (!BEG_STRCASECMP(buff, "home_on_input ")) {
    if (bool_val) {
      Options |= Opt_home_on_input;
    } else {
      Options &= ~(Opt_home_on_input);
    }

  } else if (!BEG_STRCASECMP(buff, "scrollbar_floating ")) {
    if (bool_val) {
      Options |= Opt_scrollbar_floating;
    } else {
      Options &= ~(Opt_scrollbar_floating);
    }

  } else if (!BEG_STRCASECMP(buff, "scrollbar_right ")) {
    if (bool_val) {
      Options |= Opt_scrollbar_right;
    } else {
      Options &= ~(Opt_scrollbar_right);
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

  } else if (!BEG_STRCASECMP(buff, "double_buffer ")) {
    if (bool_val) {
      Options |= Opt_double_buffer;
    } else {
      Options &= ~(Opt_double_buffer);
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
  return state;
}

static void *
parse_keyboard(char *buff, void *state)
{
  if ((*buff == CONF_BEGIN_CHAR) || (*buff == CONF_END_CHAR)) {
    return NULL;
  }
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
	return NULL;
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

	char *p = MALLOC(len + 1);

	*p = len;
	strncpy(p + 1, str, len);
	KeySym_map[sym] = (unsigned char *) p;
      }
    }
#else
    print_warning("Support for the keysym attributes was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "meta_mod ")) {
    char *tmp = PWord(2, buff);

    if (!tmp) {
      print_error("Parse error in file %s, line %lu:  Missing modifier value for attribute meta_mod",
		  file_peek_path(), file_peek_line());
      return NULL;
    }
    rs_meta_mod = (unsigned int) strtoul(tmp, (char **) NULL, 0);
    
  } else if (!BEG_STRCASECMP(buff, "alt_mod ")) {
    char *tmp = PWord(2, buff);

    if (!tmp) {
      print_error("Parse error in file %s, line %lu:  Missing modifier value for attribute alt_mod",
		  file_peek_path(), file_peek_line());
      return NULL;
    }
    rs_alt_mod = (unsigned int) strtoul(tmp, (char **) NULL, 0);
    
  } else if (!BEG_STRCASECMP(buff, "numlock_mod ")) {
    char *tmp = PWord(2, buff);

    if (!tmp) {
      print_error("Parse error in file %s, line %lu:  Missing modifier value for attribute numlock_mod",
		  file_peek_path(), file_peek_line());
      return NULL;
    }
    rs_numlock_mod = (unsigned int) strtoul(tmp, (char **) NULL, 0);
    
  } else if (!BEG_STRCASECMP(buff, "greek ")) {
#ifdef GREEK_SUPPORT

    char *tmp = PWord(2, buff);

    if (!tmp) {
      print_error("Parse error in file %s, line %lu:  Missing boolean value for attribute greek",
		  file_peek_path(), file_peek_line());
      return NULL;
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
      return NULL;
    }
#else
    print_warning("Support for the greek attribute was not compiled in, ignoring");
#endif

  } else if (!BEG_STRCASECMP(buff, "app_keypad ")) {

    char *tmp = PWord(2, buff);

    if (!tmp) {
      print_error("Parse error in file %s, line %lu:  Missing boolean value for attribute app_keypad",
		  file_peek_path(), file_peek_line());
      return NULL;
    }
    if (BOOL_OPT_ISTRUE(tmp)) {
      PrivateModes |= PrivMode_aplKP;
    } else if (BOOL_OPT_ISFALSE(tmp)) {
      PrivateModes &= ~(PrivMode_aplKP);
    } else {
      print_error("Parse error in file %s, line %lu:  Invalid boolean value \"%s\" for "
		  "attribute app_keypad", file_peek_path(), file_peek_line(), tmp);
      return NULL;
    }

  } else if (!BEG_STRCASECMP(buff, "app_cursor ")) {

    char *tmp = PWord(2, buff);

    if (!tmp) {
      print_error("Parse error in file %s, line %lu:  Missing boolean value for attribute app_cursor",
		  file_peek_path(), file_peek_line());
      return NULL;
    }
    if (BOOL_OPT_ISTRUE(tmp)) {
      PrivateModes |= PrivMode_aplCUR;
    } else if (BOOL_OPT_ISFALSE(tmp)) {
      PrivateModes &= ~(PrivMode_aplCUR);
    } else {
      print_error("Parse error in file %s, line %lu:  Invalid boolean value \"%s\" for "
		  "attribute app_cursor", file_peek_path(), file_peek_line(), tmp);
      return NULL;
    }

  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid "
		"within context keyboard", file_peek_path(), file_peek_line(), buff);
  }
  return state;
}

static void *
parse_misc(char *buff, void *state)
{
  if ((*buff == CONF_BEGIN_CHAR) || (*buff == CONF_END_CHAR)) {
    return NULL;
  }
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

    RESET_AND_ASSIGN(rs_execArgs, (char **) MALLOC(sizeof(char *) * ((n = NumWords(PWord(2, buff))) + 1)));

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
  return state;
}

static void *
parse_imageclasses(char *buff, void *state)
{
  if ((*buff == CONF_BEGIN_CHAR) || (*buff == CONF_END_CHAR)) {
    return NULL;
  }

#ifdef PIXMAP_SUPPORT
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
  return state;
}

static void *
parse_image(char *buff, void *state)
{
  int idx;

#ifdef PIXMAP_SUPPORT
  if (*buff == CONF_BEGIN_CHAR) {
    int *tmp;

    tmp = (int *) MALLOC(sizeof(int));
    return ((void *) tmp);
  }
  ASSERT_RVAL(state != NULL, (file_skip_to_end(), NULL));
  if (*buff == CONF_END_CHAR) {
    int *tmp;

    tmp = (int *) state;
    FREE(tmp);
    return NULL;
  }
  idx = *((int *) state);
  if (!BEG_STRCASECMP(buff, "type ")) {
    char *type = PWord(2, buff);

    if (!type) {
      print_error("Parse error in file %s, line %lu:  Missing image type", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (!strcasecmp(type, "background")) {
      idx = image_bg;
    } else if (!strcasecmp(type, "trough")) {
      idx = image_sb;
    } else if (!strcasecmp(type, "anchor")) {
      idx = image_sa;
    } else if (!strcasecmp(type, "thumb")) {
      idx = image_st;
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
    } else if (!strcasecmp(type, "menuitem")) {
      idx = image_menuitem;
    } else if (!strcasecmp(type, "submenu")) {
      idx = image_submenu;
    } else if (!strcasecmp(type, "button")) {
      idx = image_button;
    } else if (!strcasecmp(type, "button_bar") || !strcasecmp(type, "buttonbar")) {
      idx = image_bbar;
    } else if (!strcasecmp(type, "grab_bar")) {
      idx = image_gbar;
    } else if (!strcasecmp(type, "dialog_box")) {
      idx = image_dialog;
    } else {
      print_error("Parse error in file %s, line %lu:  Invalid image type \"%s\"", file_peek_path(), file_peek_line(), type);
      return NULL;
    }
    *((int *) state) = idx;

  } else if (!BEG_STRCASECMP(buff, "mode ")) {
    char *mode = PWord(2, buff);
    char *allow_list = PWord(4, buff);

    if (idx < 0) {
      print_error("Parse error in file %s, line %lu:  Encountered \"mode\" with no image type defined", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (!mode) {
      print_error("Parse error in file %s, line %lu:  Missing parameters for mode line", file_peek_path(), file_peek_line());
      return NULL;
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
      return NULL;
    }
    if (idx < 0) {
      print_error("Parse error in file %s, line %lu:  Encountered \"state\" with no image type defined", file_peek_path(), file_peek_line());
      return NULL;
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
    } else if (!strcasecmp(state, "disabled")) {
      if (images[idx].disabled == NULL) {
	images[idx].disabled = (simage_t *) MALLOC(sizeof(simage_t));
	new = 1;
      }
      images[idx].current = images[idx].disabled;
    } else {
      print_error("Parse error in file %s, line %lu:  Invalid state \"%s\"", file_peek_path(), file_peek_line(), state);
      return NULL;
    }
    if (new) {
      MEMSET(images[idx].current, 0, sizeof(simage_t));
      images[idx].current->pmap = (pixmap_t *) MALLOC(sizeof(pixmap_t));
      images[idx].current->iml = (imlib_t *) MALLOC(sizeof(imlib_t));
      MEMSET(images[idx].current->pmap, 0, sizeof(pixmap_t));
      MEMSET(images[idx].current->iml, 0, sizeof(imlib_t));
    }
  } else if (!BEG_STRCASECMP(buff, "color ")) {
    char *fg = Word(2, buff), *bg = Word(3, buff);

    if (idx < 0) {
      print_error("Parse error in file %s, line %lu:  Encountered \"color\" with no image type defined", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (images[idx].current == NULL) {
      print_error("Parse error in file %s, line %lu:  Encountered \"color\" with no image state defined", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (!fg || !bg) {
      print_error("Parse error in file %s, line %lu:  Foreground and background colors must be specified with \"color\"", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (!BEG_STRCASECMP(fg, "0x")) {
      images[idx].current->fg = get_color_by_pixel((Pixel) strtoul(fg, (char **) NULL, 0), WhitePixel(Xdisplay, Xscreen));
    } else {
      images[idx].current->fg = get_color_by_name(fg, "white");
    }
    if (!BEG_STRCASECMP(bg, "0x")) {
      images[idx].current->bg = get_color_by_pixel((Pixel) strtoul(bg, (char **) NULL, 0), BlackPixel(Xdisplay, Xscreen));
    } else {
      images[idx].current->bg = get_color_by_name(bg, "black");
    }
    FREE(fg);
    FREE(bg);

  } else if (!BEG_STRCASECMP(buff, "file ")) {
    char *filename = PWord(2, buff);

    if (idx < 0) {
      print_error("Parse error in file %s, line %lu:  Encountered \"file\" with no image type defined", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (images[idx].current == NULL) {
      print_error("Parse error in file %s, line %lu:  Encountered \"file\" with no image state defined", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (!filename) {
      print_error("Parse error in file %s, line %lu:  Missing filename", file_peek_path(), file_peek_line());
      return NULL;
    }
    load_image(filename, images[idx].current);

  } else if (!BEG_STRCASECMP(buff, "geom ")) {
    char *geom = PWord(2, buff);

    if (idx < 0) {
      print_error("Parse error in file %s, line %lu:  Encountered \"geom\" with no image type defined", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (images[idx].current == NULL) {
      print_error("Parse error in file %s, line %lu:  Encountered \"geom\" with no image state defined", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (!geom) {
      print_error("Parse error in file %s, line %lu:  Missing geometry string", file_peek_path(), file_peek_line());
      return NULL;
    }
    set_pixmap_scale(geom, images[idx].current->pmap);

  } else if (!BEG_STRCASECMP(buff, "cmod ") || !BEG_STRCASECMP(buff, "colormod ")) {
    char *color = PWord(2, buff);
    char *mods = PWord(3, buff);
    unsigned char n;
    imlib_t *iml = images[idx].current->iml;

    if (idx < 0) {
      print_error("Parse error in file %s, line %lu:  Encountered color modifier with no image type defined", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (images[idx].current == NULL) {
      print_error("Parse error in file %s, line %lu:  Encountered color modifier with no image state defined", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (!color) {
      print_error("Parse error in file %s, line %lu:  Missing color name", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (!mods) {
      print_error("Parse error in file %s, line %lu:  Missing modifier(s)", file_peek_path(), file_peek_line());
      return NULL;
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
        iml->mod->gamma = (int) strtol(PWord(3, mods), (char **) NULL, 0);
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
      return NULL;
    }

  } else if (!BEG_STRCASECMP(buff, "border ")) {
    if (idx < 0) {
      print_error("Parse error in file %s, line %lu:  Encountered \"border\" with no image type defined", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (NumWords(buff + 7) < 4) {
      print_error("Parse error in file %s, line %lu:  Invalid parameter list for attribute \"border\"", file_peek_path(), file_peek_line());
      return NULL;
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
      return NULL;
    }
    if (images[idx].current == NULL) {
      print_error("Parse error in file %s, line %lu:  Encountered \"bevel\" with no image state defined", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (NumWords(buff + 6) < 5) {
      print_error("Parse error in file %s, line %lu:  Invalid parameter list for attribute \"bevel\"", file_peek_path(), file_peek_line());
      return NULL;
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
      return NULL;
    }
    if (images[idx].current == NULL) {
      print_error("Parse error in file %s, line %lu:  Encountered \"padding\" with no image state defined", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (NumWords(buff + 8) < 4) {
      print_error("Parse error in file %s, line %lu:  Invalid parameter list for attribute \"padding\"", file_peek_path(), file_peek_line());
      return NULL;
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
  return ((void *) state);
#else
  print_warning("Pixmap support was not compiled in, ignoring entire context");
  file_poke_skip(1);
  return NULL;
#endif
}

static void *
parse_actions(char *buff, void *state)
{
  unsigned short mod = MOD_NONE;
  unsigned char button = BUTTON_NONE;
  KeySym keysym = 0;
  char *str;
  unsigned short i;

  if ((*buff == CONF_BEGIN_CHAR) || (*buff == CONF_END_CHAR)) {
    return NULL;
  }

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
      } else if (!BEG_STRCASECMP(str, "meta")) {
        mod |= MOD_META;
      } else if (!BEG_STRCASECMP(str, "alt")) {
        mod |= MOD_ALT;
      } else if (!BEG_STRCASECMP(str, "mod1")) {
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
      return NULL;
    }
    FREE(str);
    if ((button == BUTTON_NONE) && (keysym == 0)) {
      print_error("Parse error in file %s, line %lu:  No valid button/keysym found for action", file_peek_path(), file_peek_line());
      return NULL;
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
      return NULL;
    }

  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid "
		"within context action", file_peek_path(), file_peek_line(), buff);
  }
  return state;
}

static void *
parse_menu(char *buff, void *state)
{
  menu_t *menu;

  if (*buff == CONF_BEGIN_CHAR) {
    char *title = PWord(2, buff + 6);

    menu = menu_create(title);
    return ((void *) menu);
  }
  ASSERT_RVAL(state != NULL, (file_skip_to_end(), NULL));
  menu = (menu_t *) state;
  if (*buff == CONF_END_CHAR) {
    if (!(*(menu->title))) {
      char tmp[20];

      sprintf(tmp, "Eterm_Menu_%u", menu_list->nummenus);
      menu_set_title(menu, tmp);
      print_error("Parse error in file %s, line %lu:  Menu context ended without giving a title.  Defaulted to \"%s\".", file_peek_path(), file_peek_line(), tmp);
    }
    menu_list = menulist_add_menu(menu_list, menu);
    return NULL;
  }
  if (!BEG_STRCASECMP(buff, "title ")) {
    char *title = Word(2, buff);

    menu_set_title(menu, title);
    FREE(title);

  } else if (!BEG_STRCASECMP(buff, "font ")) {
    char *name = Word(2, buff);

    if (!name) {
      print_error("Parse error in file %s, line %lu:  Missing font name.", file_peek_path(), file_peek_line());
      return ((void *) menu);
    }
    menu_set_font(menu, name);
    FREE(name);

  } else if (!BEG_STRCASECMP(buff, "sep") || !BEG_STRCASECMP(buff, "-")) {
    menuitem_t *item;

    item = menuitem_create((char *) NULL);
    menu_add_item(menu, item);
    menuitem_set_action(item, MENUITEM_SEP, (char *) NULL);

  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid within context menu", file_peek_path(), file_peek_line(), buff);
  }
  return ((void *) menu);
}

static void *
parse_menuitem(char *buff, void *state)
{
  static menu_t *menu;
  menuitem_t *curitem;

  ASSERT_RVAL(state != NULL, (file_skip_to_end(), NULL));
  if (*buff == CONF_BEGIN_CHAR) {
    menu = (menu_t *) state;
    curitem = menuitem_create(NULL);
    return ((void *) curitem);
  }
  curitem = (menuitem_t *) state;
  ASSERT_RVAL(menu != NULL, state);
  if (*buff == CONF_END_CHAR) {
    if (!(curitem->text)) {
      print_error("Parse error in file %s, line %lu:  Menuitem context ended with no text given.  Discarding this entry.", file_peek_path(), file_peek_line());
      FREE(curitem);
    } else {
      menu_add_item(menu, curitem);
    }
    return ((void *) menu);
  }
  if (!BEG_STRCASECMP(buff, "text ")) {
    char *text = Word(2, buff);

    if (!text) {
      print_error("Parse error in file %s, line %lu:  Missing menuitem text.", file_peek_path(), file_peek_line());
      return ((void *) curitem);
    }
    menuitem_set_text(curitem, text);
    FREE(text);

  } else if (!BEG_STRCASECMP(buff, "rtext ")) {
    char *rtext = Word(2, buff);

    if (!rtext) {
      print_error("Parse error in file %s, line %lu:  Missing menuitem right-justified text.", file_peek_path(), file_peek_line());
      return ((void *) curitem);
    }
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
      print_error("Parse error in file %s, line %lu:  Invalid menu item action \"%s\"", file_peek_path(), file_peek_line(), NONULL(type));
    }
    FREE(action);

  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid within context menu", file_peek_path(), file_peek_line(), buff);
  }
  return ((void *) curitem);
}

static void *
parse_bbar(char *buff, void *state)
{
  buttonbar_t *bbar;

  if (*buff == CONF_BEGIN_CHAR) {
    bbar = bbar_create();
    return ((void *) bbar);
  }
  ASSERT_RVAL(state != NULL, (file_skip_to_end(), NULL));
  bbar = (buttonbar_t *) state;
  if (*buff == CONF_END_CHAR) {
    bbar_add(bbar);
    return NULL;
  }
  if (!BEG_STRCASECMP(buff, "font ")) {
    char *font = Word(2, buff);

    bbar_set_font(bbar, font);
    FREE(font);

  } else if (!BEG_STRCASECMP(buff, "dock ")) {
    char *where = PWord(2, buff);

    if (!where) {
      print_error("Parse error in file %s, line %lu:  Attribute dock requires a parameter", file_peek_path(), file_peek_line());
    } else if (!BEG_STRCASECMP(where, "top")) {
      bbar_set_docked(bbar, BBAR_DOCKED_TOP);
    } else if (!BEG_STRCASECMP(where, "bot")) {  /* "bot" or "bottom" */
      bbar_set_docked(bbar, BBAR_DOCKED_BOTTOM);
    } else if (!BEG_STRCASECMP(where, "no")) {  /* "no" or "none" */
      bbar_set_docked(bbar, BBAR_UNDOCKED);
    } else {
      print_error("Parse error in file %s, line %lu:  Invalid parameter \"%s\" to attribute dock", file_peek_path(), file_peek_line(), where);
    }

  } else if (!BEG_STRCASECMP(buff, "visible ")) {
    char *tmp = PWord(2, buff);

    if (BOOL_OPT_ISTRUE(tmp)) {
      bbar_set_visible(bbar, 1);
    } else if (BOOL_OPT_ISFALSE(tmp)) {
      bbar_set_visible(bbar, 0);
    } else {
      print_error("Parse error in file %s, line %lu:  Invalid boolean value \"%s\" in context button_bar", file_peek_path(), file_peek_line(), tmp);
    }

  } else if (!BEG_STRCASECMP(buff, "button ") || !BEG_STRCASECMP(buff, "rbutton ")) {
    char *text = PWord(2, buff);
    char *icon = StrCaseStr(buff, "icon ");
    char *action = StrCaseStr(buff, "action ");
    button_t *button;

    if (text == icon) {
      text = NULL;
    } else {
      text = Word(1, text);
    }
    if (!text && !icon) {
      print_error("Parse error in file %s, line %lu:  Missing button specifications", file_peek_path(), file_peek_line());
      return ((void *) bbar);
    }

    button = button_create(text);
    if (icon) {
      simage_t *simg;

      icon = Word(2, icon);
      simg = create_simage();
      if (load_image(icon, simg)) {
        button_set_icon(button, simg);
      } else {
        free_simage(simg);
      }
      FREE(icon);
    }
    if (action) {
      char *type = PWord(2, action);

      action = Word(2, type);
      if (!BEG_STRCASECMP(type, "menu ")) {
        button_set_action(button, ACTION_MENU, action);
      } else if (!BEG_STRCASECMP(type, "string ")) {
        button_set_action(button, ACTION_STRING, action);
      } else if (!BEG_STRCASECMP(type, "echo ")) {
        button_set_action(button, ACTION_ECHO, action);
      } else {
        print_error("Parse error in file %s, line %lu:  Invalid button action \"%s\"", file_peek_path(), file_peek_line(), type);
        FREE(action);
        FREE(button);
        return ((void *) bbar);
      }
      FREE(action);
    } else {
      print_error("Parse error in file %s, line %lu:  Missing button action", file_peek_path(), file_peek_line());
      FREE(button);
      return ((void *) bbar);
    }
    if (tolower(*buff) == 'r') {
      bbar_add_rbutton(bbar, button);
    } else {
      bbar_add_button(bbar, button);
    }
  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid "
		"within context menu", file_peek_path(), file_peek_line(), buff);
  }
  return ((void *) bbar);
}

static void *
parse_xim(char *buff, void *state)
{
#ifdef USE_XIM
  if ((*buff == CONF_BEGIN_CHAR) || (*buff == CONF_END_CHAR)) {
    return NULL;
  }
  if (!BEG_STRCASECMP(buff, "input_method ")) {
    RESET_AND_ASSIGN(rs_input_method, Word(2, buff));
  } else if (!BEG_STRCASECMP(buff, "preedit_type ")) {
    RESET_AND_ASSIGN(rs_preedit_type, Word(2, buff));
  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid within context xim",
		file_peek_path(), file_peek_line(), buff);
  }
#else
  print_warning("XIM support was not compiled in, ignoring entire context");
  file_poke_skip(1);
#endif
  return state;
  buff = NULL;
}

static void *
parse_multichar(char *buff, void *state)
{
#ifdef MULTI_CHARSET
  if ((*buff == CONF_BEGIN_CHAR) || (*buff == CONF_END_CHAR)) {
    return NULL;
  }
  if (!BEG_STRCASECMP(buff, "encoding ")) {
    RESET_AND_ASSIGN(rs_multichar_encoding, Word(2, buff));
    if (rs_multichar_encoding != NULL) {
      if (BEG_STRCASECMP(rs_multichar_encoding, "eucj")
	  && BEG_STRCASECMP(rs_multichar_encoding, "sjis")
	  && BEG_STRCASECMP(rs_multichar_encoding, "euckr")) {
	print_error("Parse error in file %s, line %lu:  Invalid multichar encoding mode \"%s\"",
		    file_peek_path(), file_peek_line(), rs_multichar_encoding);
	return NULL;
      }
      set_multichar_encoding(rs_multichar_encoding);
    } else {
      print_error("Parse error in file %s, line %lu:  Invalid parameter list \"\" for attribute encoding",
		  file_peek_path(), file_peek_line());
    }
  } else if (!BEG_STRCASECMP(buff, "font ")) {

    char *tmp = PWord(2, buff);
    unsigned char n;

    if (NumWords(buff) != 3) {
      print_error("Parse error in file %s, line %lu:  Invalid parameter list \"%s\" for "
		  "attribute font", file_peek_path(), file_peek_line(), NONULL(tmp));
      return NULL;
    }
    if (isdigit(*tmp)) {
      n = (unsigned char) strtoul(tmp, (char **) NULL, 0);
      if (n <= 255) {
        eterm_font_add(&etmfonts, PWord(2, tmp), n);
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
  return state;
  buff = NULL;
}

/* The config file reader.  This looks for the config file by searching CONFIG_SEARCH_PATH.
   If it can't find a config file, it displays a warning but continues. -- mej */

char *
conf_find_file(const char *file, const char *dir, const char *pathlist) {

  static char name[PATH_MAX], full_path[PATH_MAX];
  const char *path;
  char *p;
  short maxpathlen;
  unsigned short len;
  struct stat fst;

  REQUIRE_RVAL(file != NULL, NULL);

  getcwd(name, PATH_MAX);
  D_OPTIONS(("conf_find_file(\"%s\", \"%s\", \"%s\") called from directory \"%s\".\n", file, NONULL(dir), NONULL(pathlist), name));

  if (dir) {
    strcpy(name, dir);
    strcat(name, "/");
    strcat(name, file);
  } else {
    strcpy(name, file);
  }
  len = strlen(name);
  D_OPTIONS(("Checking for file \"%s\"\n", name));
  if ((!access(name, R_OK)) && (!stat(name, &fst)) && (!S_ISDIR(fst.st_mode))) {
    D_OPTIONS(("Found \"%s\"\n", name));
    return ((char *) name);
  }

  /* maxpathlen is the longest possible path we can stuff into name[].  The - 2 saves room for
     an additional / and the trailing null. */
  if ((maxpathlen = sizeof(name) - len - 2) <= 0) {
    D_OPTIONS(("Too big.  I lose. :(\n", name));
    return ((char *) NULL);
  }

  for (path = pathlist; path != NULL && *path != '\0'; path = p) {
    short n;

    /* Calculate the length of the next directory in the path */
    if ((p = strchr(path, ':')) != NULL) {
      n = p++ - path;
    } else {
      n = strlen(path);
    }

    /* Don't try if it's too long */
    if (n > 0 && n <= maxpathlen) {
      /* Compose the /path/file combo */
      strncpy(full_path, path, n);
      if (full_path[n - 1] != '/') {
	full_path[n++] = '/';
      }
      full_path[n] = '\0';
      strcat(full_path, name);

      D_OPTIONS(("Checking for file \"%s\"\n", full_path));
      if ((!access(full_path, R_OK)) && (!stat(full_path, &fst)) && (!S_ISDIR(fst.st_mode))) {
        D_OPTIONS(("Found \"%s\"\n", full_path));
        return ((char *) full_path);
      }
    }
  }
  D_OPTIONS(("conf_find_file():  File \"%s\" not found in path.\n", name));
  return ((char *) NULL);
}

FILE *
open_config_file(char *name)
{

  FILE *fp;
  int ver;
  char buff[256], *begin_ptr, *end_ptr;

  ASSERT(name != NULL);

  fp = fopen(name, "rt");
  if (fp != NULL) {
    fgets(buff, 256, fp);
    if (BEG_STRCASECMP(buff, "<" PACKAGE "-")) {
      fclose(fp);
      fp = NULL;
    } else {
      begin_ptr = strchr(buff, '-') + 1;
      if ((end_ptr = strchr(buff, '>')) != NULL) {
	*end_ptr = 0;
      }
      if ((ver = BEG_STRCASECMP(begin_ptr, VERSION)) > 0) {
	print_warning("Config file is designed for a newer version of " PACKAGE);
      }
    }
  }
  return (fp);
}

char *
conf_parse(char *conf_name, const char *dir, const char *path) {

  FILE *fp;
  char *name = NULL, *outfile, *p = ".";
  char buff[CONFIG_BUFF], orig_dir[PATH_MAX];
  register unsigned long i = 0;
  unsigned char id = 0;
  void *state = NULL;

  REQUIRE_RVAL(conf_name != NULL, 0);

  *orig_dir = 0;
  if (path) {
    if ((name = conf_find_file(conf_name, dir, path)) != NULL) {
      if ((p = strrchr(name, '/')) != NULL) {
        getcwd(orig_dir, PATH_MAX);
        *p = 0;
        p = name;
        chdir(name);
      } else {
        p = ".";
      }
    } else {
      return NULL;
    }
  }
  if ((fp = open_config_file(conf_name)) == NULL) {
    return NULL;
  }
  file_push(fp, conf_name, NULL, 1, 0);  /* Line count starts at 1 because open_config_file() parses the first line */

  for (; fstate_idx > 0;) {
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
            char *path;
            FILE *fp;

	    shell_expand(buff);
	    path = Word(2, buff + 1);
	    if ((fp = open_config_file(path)) == NULL) {
	      print_error("Error in file %s, line %lu:  Unable to locate %%included config file %s (%s), continuing", file_peek_path(), file_peek_line(),
                          path, strerror(errno));
	    } else {
              file_push(fp, path, NULL, 1, 0);
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
            D_OPTIONS(("read_config():  Parsing line #%lu of file %s\n", file_peek_line(), file_peek_path()));
            if (file_peek_skip()) {
              continue;
            }
            shell_expand(buff);
	  }
	  break;
	case 'b':
	  D_OPTIONS(("read_config():  Parsing line #%lu of file %s\n", file_peek_line(), file_peek_path()));
	  if (file_peek_skip()) {
	    continue;
          }
	  if (!BEG_STRCASECMP(buff, "begin ")) {
	    name = PWord(2, buff);
	    ctx_name_to_id(id, name, i);
            ctx_push(id);
            *buff = CONF_BEGIN_CHAR;
            state = (*ctx_id_to_func(id))(buff, ctx_peek_last_state());
            ctx_poke_state(state);
            break;
          }
          /* Intentional pass-through */
	case 'e':
          if (*buff != 'b') {
            D_OPTIONS(("read_config():  Parsing line #%lu of file %s\n", file_peek_line(), file_peek_path()));
          }
	  if (!BEG_STRCASECMP(buff, "end ") || !strcasecmp(buff, "end")) {
	    if (ctx_get_depth()) {
              *buff = CONF_END_CHAR;
              state = (*ctx_id_to_func(id))(buff, ctx_peek_state());
              ctx_poke_state(NULL);
	      ctx_pop();
              id = ctx_peek_id();
              ctx_poke_state(state);
	      file_poke_skip(0);
	    }
	    break;
	  }
          /* Intentional pass-through */
	default:
          if (*buff != 'b' && *buff != 'e') {
            D_OPTIONS(("read_config():  Parsing line #%lu of file %s\n", file_peek_line(), file_peek_path()));
          }
	  if (file_peek_skip()) {
	    continue;
          }
          shell_expand(buff);
          ctx_poke_state((*ctx_id_to_func(id))(buff, ctx_peek_state()));
      }
    }
    fclose(file_peek_fp());
    if (file_peek_preproc()) {
      remove(file_peek_outfile());
    }
    file_pop();
  }
  if (*orig_dir) {
    chdir(orig_dir);
  }
  D_OPTIONS(("Returning \"%s\"\n", p));
  return (StrDup(p));
}

char *
conf_parse_theme(char *theme, char *conf_name, unsigned char fallback)
{
  static char path[CONFIG_BUFF];
  char *ret = NULL;

  if (!(*path)) {
    char *path_env;

    path_env = getenv(PATH_ENV);
    if (path_env) {
      strcpy(path, CONFIG_SEARCH_PATH ":");
      strcat(path, path_env);
    } else {
      strcpy(path, CONFIG_SEARCH_PATH);
    }
    shell_expand(path);
  }
  if (!theme || (ret = conf_parse(conf_name, rs_theme, path)) == NULL) {
    if (fallback) {
      RESET_AND_ASSIGN(rs_theme, StrDup(PACKAGE));
      if ((ret = conf_parse(conf_name, rs_theme, path)) == NULL) {
        RESET_AND_ASSIGN(rs_theme, NULL);
        ret = conf_parse(conf_name, rs_theme, path);
      }
    }
  }
  return ret;
}

static void *
parse_null(char *buff, void *state) {

  if (*buff == CONF_BEGIN_CHAR) {
    return (NULL);
  } else if (*buff == CONF_END_CHAR) {
    return (NULL);
  } else {
    print_error("Parse error in file %s, line %lu:  Not allowed in \"null\" context:  \"%s\"", file_peek_path(), file_peek_line(), buff);
    return (state);
  }
}

/* Initialize the default values for everything */
void
init_defaults(void)
{
  unsigned char i;

#if DEBUG >= DEBUG_MALLOC
  if (debug_level >= DEBUG_MALLOC) {
    memrec_init();
  }
#endif

  rs_name = StrDup(APL_NAME " " VERSION);
  Options = (Opt_scrollbar | Opt_select_trailing_spaces);
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
  MEMSET(rs_font, 0, sizeof(char *) * NFONTS);
  for (i = 0; i < NFONTS; i++) {
    eterm_font_add(&etfonts, def_fontName[i], i);
#ifdef MULTI_CHARSET
    eterm_font_add(&etmfonts, def_mfontName[i], i);
#endif
  }
#ifdef MULTI_CHARSET
  rs_multichar_encoding = StrDup(MULTICHAR_ENCODING);
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
      scrollbar_set_type(SCROLLBAR_XTERM);
#else
      print_error("Support for xterm scrollbars was not compiled in.  Sorry.");
#endif
    } else if (!strcasecmp(rs_scrollbar_type, "next")) {
#ifdef NEXT_SCROLLBAR
      scrollbar_set_type(SCROLLBAR_NEXT);
#else
      print_error("Support for NeXT scrollbars was not compiled in.  Sorry.");
#endif
    } else if (!strcasecmp(rs_scrollbar_type, "motif")) {
#ifdef MOTIF_SCROLLBAR
      scrollbar_set_type(SCROLLBAR_MOTIF);
#else
      print_error("Support for motif scrollbars was not compiled in.  Sorry.");
#endif
    } else {
      print_error("Unrecognized scrollbar type \"%s\".", rs_scrollbar_type);
    }
  }
  if (rs_scrollbar_width) {
    scrollbar_set_width(rs_scrollbar_width);
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
    Options &= ~Opt_scrollbar;

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
    if (rs_font[i]) {
      if (def_font_idx == 0) {
        eterm_font_add(&etfonts, rs_font[i], i);
        RESET_AND_ASSIGN(rs_font[i], NULL);
      } else {
        eterm_font_add(&etfonts, rs_font[i], ((i == 0) ? def_font_idx : ((i <= def_font_idx) ? (i - 1) : i)));
        RESET_AND_ASSIGN(rs_font[i], NULL);
      }
    }
#ifdef MULTI_CHARSET
    if (rs_mfont[i]) {
      if (def_font_idx == 0) {
        eterm_font_add(&etmfonts, rs_mfont[i], i);
        RESET_AND_ASSIGN(rs_mfont[i], NULL);
      } else {
        eterm_font_add(&etmfonts, rs_mfont[i], ((i == 0) ? def_font_idx : ((i <= def_font_idx) ? (i - 1) : i)));
        RESET_AND_ASSIGN(rs_mfont[i], NULL);
      }
    }
#endif
  }

  /* Clean up image stuff */
  for (i = 0; i < image_max; i++) {
    if (images[i].norm) {
      /* If we have a bevel but no border, use the bevel as a border. */
      if (images[i].norm->iml->bevel && !(images[i].norm->iml->border)) {
        images[i].norm->iml->border = images[i].norm->iml->bevel->edges;
      }
      images[i].userdef = 1;
    } else {
      images[i].norm = (simage_t *) MALLOC(sizeof(simage_t));
      images[i].norm->pmap = (pixmap_t *) MALLOC(sizeof(pixmap_t));
      images[i].norm->iml = (imlib_t *) MALLOC(sizeof(imlib_t));
      images[i].norm->fg = WhitePixel(Xdisplay, Xscreen);
      images[i].norm->bg = BlackPixel(Xdisplay, Xscreen);
      MEMSET(images[i].norm->pmap, 0, sizeof(pixmap_t));
      MEMSET(images[i].norm->iml, 0, sizeof(imlib_t));
      images[i].mode = MODE_IMAGE & ALLOW_IMAGE;
    }
    images[i].current = images[i].norm;
    if (rs_pixmaps[i]) {
      reset_simage(images[i].norm, RESET_ALL_SIMG);
      load_image(rs_pixmaps[i], images[i].norm);
      FREE(rs_pixmaps[i]);	/* These are created by StrDup() */
    }
    if (images[i].selected) {
      /* If we have a bevel but no border, use the bevel as a border. */
      if (images[i].selected->iml->bevel && !(images[i].selected->iml->border)) {
        images[i].selected->iml->border = images[i].selected->iml->bevel->edges;
      }
      /* If normal has an image but we don't, copy it. */
      if (!(images[i].selected->iml->im) && (images[i].norm->iml->im)) {
        images[i].selected->iml->im = images[i].norm->iml->im;
        *(images[i].selected->pmap) = *(images[i].norm->pmap);
      }
      if (images[i].selected->fg == 0 && images[i].selected->bg == 0) {
        images[i].selected->fg = images[i].norm->fg;
        images[i].selected->bg = images[i].norm->bg;
      }
    } else {
      D_PIXMAP(("No \"selected\" state for image %s.  Setting fallback to the normal state.\n", get_image_type(i)));
      images[i].selected = images[i].norm;
    }
    if (images[i].clicked) {
      /* If we have a bevel but no border, use the bevel as a border. */
      if (images[i].clicked->iml->bevel && !(images[i].clicked->iml->border)) {
        images[i].clicked->iml->border = images[i].clicked->iml->bevel->edges;
      }
      /* If normal has an image but we don't, copy it. */
      if (!(images[i].clicked->iml->im) && (images[i].norm->iml->im)) {
        images[i].clicked->iml->im = images[i].norm->iml->im;
        *(images[i].clicked->pmap) = *(images[i].norm->pmap);
      }
      if (images[i].clicked->fg == 0 && images[i].clicked->bg == 0) {
        images[i].clicked->fg = images[i].norm->fg;
        images[i].clicked->bg = images[i].norm->bg;
      }
    } else {
      D_PIXMAP(("No \"clicked\" state for image %s.  Setting fallback to the selected state.\n", get_image_type(i)));
      images[i].clicked = images[i].selected;
    }
    if (images[i].disabled) {
      /* If we have a bevel but no border, use the bevel as a border. */
      if (images[i].disabled->iml->bevel && !(images[i].disabled->iml->border)) {
        images[i].disabled->iml->border = images[i].disabled->iml->bevel->edges;
      }
      /* If normal has an image but we don't, copy it. */
      if (!(images[i].disabled->iml->im) && (images[i].norm->iml->im)) {
        images[i].disabled->iml->im = images[i].norm->iml->im;
        *(images[i].disabled->pmap) = *(images[i].norm->pmap);
      }
      if (images[i].disabled->fg == 0 && images[i].disabled->bg == 0) {
        images[i].disabled->fg = images[i].norm->fg;
        images[i].disabled->bg = images[i].norm->bg;
      }
    } else {
      D_PIXMAP(("No \"disabled\" state for image %s.  Setting fallback to the normal state.\n", get_image_type(i)));
      images[i].disabled = images[i].norm;
    }
    if ((image_toggles & IMOPT_TRANS) && (image_mode_is(i, ALLOW_TRANS))) {
      D_PIXMAP(("Detected transparency option.  Enabling transparency on image %s\n", get_image_type(i)));
      image_set_mode(i, MODE_TRANS);
    } else if ((image_toggles & IMOPT_VIEWPORT) && (image_mode_is(i, ALLOW_VIEWPORT))) {
      D_PIXMAP(("Detected viewport option.  Enabling viewport mode on image %s\n", get_image_type(i)));
      image_set_mode(i, MODE_VIEWPORT);
    }
  }
  if (images[image_bg].norm->fg || images[image_bg].norm->bg) {
    /* They specified their colors here, so copy them to the right place. */
    PixColors[fgColor] = images[image_bg].norm->fg;
    PixColors[bgColor] = images[image_bg].norm->bg;
  }

  /* Update buttonbar sizes based on new imageclass info. */
  bbar_resize_all(-1);

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

  if (rs_meta_mod) {
    MetaMask = modmasks[rs_meta_mod - 1];
  }
  if (rs_alt_mod) {
    AltMask = modmasks[rs_alt_mod - 1];
  }
  if (rs_numlock_mod) {
    NumLockMask = modmasks[rs_numlock_mod - 1];
  }

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
	    rs_anim_pixmaps[i] = (char *) REALLOC(rs_anim_pixmaps[i], strlen(rs_anim_pixmaps[i]) + 9);
	    strcat(rs_anim_pixmaps[i], "@100x100");
	  } else {
	    rs_anim_pixmaps[i] = Word(3, temp);
	    rs_anim_pixmaps[i] = (char *) REALLOC(rs_anim_pixmaps[i], strlen(rs_anim_pixmaps[i]) + 5);
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
  fprintf(fp, "    scrollbar_type %s\n", (scrollbar_get_type() == SCROLLBAR_XTERM ? "xterm" : (scrollbar_get_type() == SCROLLBAR_MOTIF ? "motif" : "next")));
  fprintf(fp, "    scrollbar_width %d\n", scrollbar_anchor_width());
  fprintf(fp, "    font default %u\n", (unsigned int) font_idx);
  for (i = 0; i < font_cnt; i++) {
    if (etfonts[i]) {
      fprintf(fp, "    font %d %s\n", i, etfonts[i]);
    }
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
      case image_bg:        fprintf(fp, "      type background\n"); break;
      case image_sb:        fprintf(fp, "      type trough\n"); break;
      case image_sa:        fprintf(fp, "      type anchor\n"); break;
      case image_st:        fprintf(fp, "      type thumb\n"); break;
      case image_up:        fprintf(fp, "      type up_arrow\n"); break;
      case image_down:      fprintf(fp, "      type down_arrow\n"); break;
      case image_left:      fprintf(fp, "      type left_arrow\n"); break;
      case image_right:     fprintf(fp, "      type right_arrow\n"); break;
      case image_menu:      fprintf(fp, "      type menu\n"); break;
      case image_menuitem:  fprintf(fp, "      type menuitem\n"); break;
      case image_submenu:   fprintf(fp, "      type submenu\n"); break;
      case image_button:    fprintf(fp, "      type button\n"); break;
      case image_bbar:      fprintf(fp, "      type button_bar\n"); break;
      case image_gbar:      fprintf(fp, "      type grab_bar\n"); break;
      case image_dialog:    fprintf(fp, "      type dialog\n"); break;
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
    if (simg->fg || simg->bg) {
      fprintf(fp, "      color 0x%08x 0x%08x\n", (unsigned int) simg->fg, (unsigned int) simg->bg);
    }
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
      if (simg->fg || simg->bg) {
        fprintf(fp, "      color 0x%08x 0x%08x\n", (unsigned int) simg->fg, (unsigned int) simg->bg);
      }
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
      if (simg->fg || simg->bg) {
        fprintf(fp, "      color 0x%08x 0x%08x\n", (unsigned int) simg->fg, (unsigned int) simg->bg);
      }
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

    /* Disabled state */
    if (images[i].disabled != images[i].norm) {
      simg = images[i].disabled;
      fprintf(fp, "      state disabled\n");
      if (simg->fg || simg->bg) {
        fprintf(fp, "      color 0x%08x 0x%08x\n", (unsigned int) simg->fg, (unsigned int) simg->bg);
      }
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
      if (action->mod & MOD_META) {
        fprintf(fp, "meta ");
      }
      if (action->mod & MOD_ALT) {
        fprintf(fp, "alt ");
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
  if (rs_multichar_encoding) {
    fprintf(fp, "    encoding %s\n", rs_multichar_encoding);
  }
  for (i = 0; i < font_cnt; i++) {
    if (etmfonts[i]) {
      fprintf(fp, "    font %d %s\n", i, etmfonts[i]);
    }
  }
  fprintf(fp, "  end multichar\n\n");
#endif

#ifdef USE_XIM
  fprintf(fp, "  begin xim\n");
  if (rs_input_method) {
    fprintf(fp, "    input_method %s\n", rs_input_method);
  }
  if (rs_preedit_type) {
    fprintf(fp, "    preedit_type %s\n", rs_preedit_type);
  }
  fprintf(fp, "  end xim\n\n");
#endif

  fprintf(fp, "  begin toggles\n");
  fprintf(fp, "    map_alert %d\n", (Options & Opt_mapAlert ? 1 : 0));
  fprintf(fp, "    visual_bell %d\n", (Options & Opt_visualBell ? 1 : 0));
  fprintf(fp, "    login_shell %d\n", (Options & Opt_loginShell ? 1 : 0));
  fprintf(fp, "    scrollbar %d\n", (Options & Opt_scrollbar ? 1 : 0));
  fprintf(fp, "    utmp_logging %d\n", (Options & Opt_utmpLogging ? 1 : 0));
  fprintf(fp, "    meta8 %d\n", (Options & Opt_meta8 ? 1 : 0));
  fprintf(fp, "    iconic %d\n", (Options & Opt_iconic ? 1 : 0));
  fprintf(fp, "    home_on_output %d\n", (Options & Opt_home_on_output ? 1 : 0));
  fprintf(fp, "    home_on_input %d\n", (Options & Opt_home_on_input ? 1 : 0));
  fprintf(fp, "    scrollbar_floating %d\n", (Options & Opt_scrollbar_floating ? 1 : 0));
  fprintf(fp, "    scrollbar_right %d\n", (Options & Opt_scrollbar_right ? 1 : 0));
  fprintf(fp, "    scrollbar_popup %d\n", (Options & Opt_scrollbar_popup ? 1 : 0));
  fprintf(fp, "    borderless %d\n", (Options & Opt_borderless ? 1 : 0));
  fprintf(fp, "    backing_store %d\n", (Options & Opt_backing_store ? 1 : 0));
  fprintf(fp, "    double_buffer %d\n", (Options & Opt_double_buffer ? 1 : 0));
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
    fprintf(fp, "    bigfont_key %s\n", tmp_str);
  }
  if (rs_meta_mod) {
    fprintf(fp, "    meta_mod %d\n", rs_meta_mod);
  }
  if (rs_alt_mod) {
    fprintf(fp, "    alt_mod %d\n", rs_alt_mod);
  }
  if (rs_numlock_mod) {
    fprintf(fp, "    numlock_mod %d\n", rs_numlock_mod);
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
