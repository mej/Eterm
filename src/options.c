/*
 * Copyright (C) 1997-2002, Michael Jennings
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
#include "defaultfont.h"

#ifndef CONFIG_BUFF
# define CONFIG_BUFF 20480
#endif

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

static char *rs_pipe_name = NULL;
#ifdef PIXMAP_SUPPORT
static int rs_shade = 0;
static char *rs_tint = NULL;
#endif
static unsigned long rs_buttonbars = 1;
static char *rs_font_effects = NULL;
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

unsigned long Options = (Opt_scrollbar), image_toggles = 0;
char *theme_dir = NULL, *user_dir = NULL;
char **rs_exec_args = NULL;	/* Args to exec (-e or --exec) */
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
char *rs_finished_title = NULL;
char *rs_finished_text = NULL;
char *rs_term_name = NULL;
#ifdef PIXMAP_SUPPORT
char *rs_pixmapScale = NULL;
char *rs_icon = NULL;
char *rs_cmod_image = NULL;
char *rs_cmod_red = NULL;
char *rs_cmod_green = NULL;
char *rs_cmod_blue = NULL;
unsigned long rs_cache_size = (unsigned long) -1;
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
      OPT_ILONG("debug", "level of debugging information to show (support not compiled in)", &DEBUG_LEVEL),
#elif DEBUG == 1
      OPT_ILONG("debug", "level of debugging information to show (0-1)", &DEBUG_LEVEL),
#elif DEBUG == 2
      OPT_ILONG("debug", "level of debugging information to show (0-2)", &DEBUG_LEVEL),
#elif DEBUG == 3
      OPT_ILONG("debug", "level of debugging information to show (0-3)", &DEBUG_LEVEL),
#elif DEBUG == 4
      OPT_ILONG("debug", "level of debugging information to show (0-4)", &DEBUG_LEVEL),
#else
      OPT_ILONG("debug", "level of debugging information to show (0-5)", &DEBUG_LEVEL),
#endif
      OPT_BLONG("install", "install a private colormap", &Options, Opt_install),

      OPT_BOOL('h', "help", "display usage information", NULL, 0),
      OPT_BLONG("version", "display version and configuration information", NULL, 0),

/* =======[ Color options ]======= */
      OPT_BOOL('r', "reverse-video", "reverse video", &Options, Opt_reverse_video),
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
      OPT_BLONG("proportional", "toggle proportional font optimizations", &Options, Opt_proportional),
      OPT_LONG("font-fx", "specify font effects for the terminal fonts", &rs_font_effects),

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
      OPT_BOOL('0', "itrans", "use immotile-optimized transparency", &image_toggles, IMOPT_ITRANS),
      OPT_BLONG("viewport-mode", "use viewport mode for the background image", &image_toggles, IMOPT_VIEWPORT),
      OPT_ILONG("shade", "old-style shade percentage (deprecated)", &rs_shade),
      OPT_LONG("tint", "old-style tint mask (deprecated)", &rs_tint),
      OPT_LONG("cmod", "image color modifier (\"brightness contrast gamma\")", &rs_cmod_image),
      OPT_LONG("cmod-red", "red-only color modifier (\"brightness contrast gamma\")", &rs_cmod_red),
      OPT_LONG("cmod-green", "green-only color modifier (\"brightness contrast gamma\")", &rs_cmod_green),
      OPT_LONG("cmod-blue", "blue-only color modifier (\"brightness contrast gamma\")", &rs_cmod_blue),
      OPT_STR('p', "path", "pixmap file search path", &rs_path),
      OPT_ILONG("cache", "set Imlib2 image/pixmap cache size", &rs_cache_size),
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
      OPT_LONG("mencoding", "multichar encoding mode (eucj/sjis/euckr/big5/gb)", &rs_multichar_encoding),
#endif /* MULTI_CHARSET */
#ifdef USE_XIM
      OPT_LONG("input-method", "XIM input method", &rs_input_method),
      OPT_LONG("preedit-type", "XIM preedit type", &rs_preedit_type),
#endif

/* =======[ Toggles ]======= */
      OPT_BOOL('l', "login-shell", "login shell, prepend - to shell name", &Options, Opt_login_shell),
      OPT_BOOL('s', "scrollbar", "display scrollbar", &Options, Opt_scrollbar),
      OPT_BOOL('u', "utmp-logging", "make a utmp entry", &Options, Opt_write_utmp),
      OPT_BOOL('v', "visual-bell", "visual bell", &Options, Opt_visual_bell),
      OPT_BOOL('H', "home-on-output", "jump to bottom on output", &Options, Opt_home_on_output),
      OPT_BLONG("home-on-input", "jump to bottom on input", &Options, Opt_home_on_input),
      OPT_BOOL('q', "no-input", "configure for output only", &Options, Opt_no_input),
      OPT_BLONG("scrollbar-right", "display the scrollbar on the right", &Options, Opt_scrollbar_right),
      OPT_BLONG("scrollbar-floating", "display the scrollbar with no trough", &Options, Opt_scrollbar_floating),
      OPT_BLONG("scrollbar-popup", "popup the scrollbar only when focused", &Options, Opt_scrollbar_popup),
      OPT_BOOL('x', "borderless", "force Eterm to have no borders", &Options, Opt_borderless),
#ifndef NO_MAPALERT
# ifdef MAPALERT_OPTION
      OPT_BOOL('m', "map-alert", "uniconify on beep", &Options, Opt_map_alert),
# endif
#endif
#ifdef META8_OPTION
      OPT_BOOL('8', "meta-8", "Meta key toggles 8-bit", &Options, Opt_meta8),
#endif
      OPT_BLONG("double-buffer", "use double-buffering to reduce exposes (uses more memory)", &Options, Opt_double_buffer),
      OPT_BLONG("no-cursor", "disable the text cursor", &Options, Opt_no_cursor),
      OPT_BLONG("pause", "pause after the child process exits", &Options, Opt_pause),
      OPT_BLONG("xterm-select", "duplicate xterm's broken selection behavior", &Options, Opt_xterm_select),
      OPT_BLONG("select-line", "triple-click selects whole line", &Options, Opt_select_whole_line),
      OPT_BLONG("select-trailing-spaces", "do not skip trailing spaces when selecting", &Options, Opt_select_trailing_spaces),
      OPT_BLONG("report-as-keysyms", "report special keys as keysyms", &Options, Opt_report_as_keysyms),
      OPT_BLONG("buttonbar", "toggle the display of all buttonbars", &rs_buttonbars, BBAR_FORCE_TOGGLE),
      OPT_BLONG("resize-gravity", "toggle gravitation to nearest corner on resize", &Options, Opt_resize_gravity),

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
      OPT_ILONG("min-anchor-size", "minimum size of the scrollbar anchor", &rs_min_anchor_size),
#ifdef BORDER_WIDTH_OPTION
      OPT_INT('w', "border-width", "term window border width", &(TermWin.internalBorder)),
#endif
#ifdef PRINTPIPE
      OPT_LONG("print-pipe", "print command", &rs_print_pipe),
#endif
#ifdef CUTCHAR_OPTION
      OPT_LONG("cut-chars", "seperators for double-click selection", &rs_cutchars),
#endif /* CUTCHAR_OPTION */
      OPT_LONG("finished-title", "text to add to window title after program termination", &rs_finished_title),
      OPT_LONG("finished-text", "text to output after program termination", &rs_finished_text),
      OPT_LONG("term-name", "value to use for setting $TERM", &rs_term_name),
      OPT_LONG("pipe-name", "filename of console pipe to emulate -C", &rs_pipe_name),
      OPT_STR('a', "attribute", "parse an attribute in the specified context", NULL),
      OPT_BOOL('C', "console", "grab console messages", &Options, Opt_console),
      OPT_ARGS('e', "exec", "execute a command rather than a shell", &rs_exec_args)
};

/* Print usage information */
#define INDENT "5"
static void
usage(void)
{

  unsigned short i, col;

  printf("Eterm Enlightened Terminal Emulator for the X Window System\n");
  printf("Copyright (c) 1997-2002, " AUTHORS "\n\n");
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
  printf("Copyright (c) 1997-2002, " AUTHORS "\n\n");

  printf("Build info:\n");
  printf("    Built on " BUILD_DATE "\n");
  printf("    " ACTIONS_IDENT "\n"
         "    " BUTTONS_IDENT "\n"
         "    " COMMAND_IDENT "\n"
         "    " DRAW_IDENT "\n"
         "    " E_IDENT "\n"
         "    " EVENTS_IDENT "\n"
         "    " FONT_IDENT "\n"
         "    " GRKELOT_IDENT "\n"
         "    " MAIN_IDENT "\n"
         "    " MENUS_IDENT "\n"
         "    " MISC_IDENT "\n"
         "    " NETDISP_IDENT "\n"
         "    " OPTIONS_IDENT "\n"
         "    " PIXMAP_IDENT "\n"
         "    " SCREEN_IDENT "\n"
         "    " SCROLLBAR_IDENT "\n"
         "    " STARTUP_IDENT "\n"
         "    " SYSTEM_IDENT "\n"
         "    " TERM_IDENT "\n"
         "    " TIMER_IDENT "\n"
         "    " UTMP_IDENT "\n"
         "    " WINDOWS_IDENT "\n"
         "\n");

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
#if DEBUG >= DEBUG_MEM
  printf(" +DEBUG_MEM");
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
#ifdef BACKGROUND_CYCLING_SUPPORT
  printf(" +BACKGROUND_CYCLING_SUPPORT");
#else
  printf(" -BACKGROUND_CYCLING_SUPPORT");
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
  printf(" PATH_ENV=\"%s\"\n", safe_print_string(PATH_ENV, sizeof(PATH_ENV) - 1));
#else
  printf(" -PATH_ENV\n");
#endif
#ifdef REFRESH_PERIOD
  printf(" REFRESH_PERIOD=%d\n", REFRESH_PERIOD);
#else
  printf(" -REFRESH_PERIOD\n");
#endif
#ifdef PRINTPIPE
  printf(" PRINTPIPE=\"%s\"\n", safe_print_string(PRINTPIPE, sizeof(PRINTPIPE) - 1));
#else
  printf(" -PRINTPIPE\n");
#endif
#ifdef KS_DELETE
  printf(" KS_DELETE=\"%s\"\n", safe_print_string(KS_DELETE, sizeof(KS_DELETE) - 1));
#else
  printf(" -KS_DELETE\n");
#endif
#ifdef SAVELINES
  printf(" SAVELINES=%d\n", SAVELINES);
#else
  printf(" -SAVELINES\n");
#endif
#ifdef CUTCHARS
  printf(" CUTCHARS=\"%s\"\n", safe_print_string(CUTCHARS, sizeof(CUTCHARS) - 1));
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
  printf(" ESCZ_ANSWER=\"%s\"\n", safe_print_string(ESCZ_ANSWER, sizeof(ESCZ_ANSWER) - 1));
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
      print_error("unexpected argument %s -- expected option\n", opt);
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
	print_error("unrecognized long option --%s\n", opt);
	CHECK_BAD();
	continue;
      }
      /* Put option-specific warnings here -- mej */
#if 0				/* No longer needed, since it works :) */
      if (optList[j].short_opt == 'w') {
	print_error("warning:  Use of the -w / --border-width option is discouraged and unsupported.\n");
      }
#endif

      if ((val_ptr = strchr(opt, '=')) != NULL) {
	*val_ptr = 0;
	val_ptr++;
	hasequal = 1;
      } else {
	if (argv[i + 1]) {
	  if (*argv[i + 1] != '-' || strcasestr(optList[j].long_opt, "font") || strcasestr(optList[j].long_opt, "geometry")) {
	    val_ptr = argv[++i];
	  }
	}
      }
      D_OPTIONS(("hasequal == %d  val_ptr == %10.8p \"%s\"\n", hasequal, val_ptr, (val_ptr ? val_ptr : "(nil)")));
      if (j == 0 || j == 1) {
	continue;
      }
      if (!(optList[j].flag & OPT_BOOLEAN) && (val_ptr == NULL)) {
	print_error("long option --%s requires a%s value\n", opt,
		    (optList[j].flag & OPT_INTEGER ? "n integer" : " string"));
	CHECK_BAD();
	continue;
      }
      if (!strcasecmp(opt, "exec")) {
	D_OPTIONS(("--exec option detected\n"));
	if (!hasequal) {

	  register unsigned short k, len = argc - i;

	  rs_exec_args = (char **) MALLOC(sizeof(char *) * (argc - i + 1));

	  for (k = 0; k < len; k++) {
	    rs_exec_args[k] = STRDUP(argv[k + i]);
	    D_OPTIONS(("rs_exec_args[%d] == %s\n", k, rs_exec_args[k]));
	  }
	  rs_exec_args[k] = (char *) NULL;
	  return;
	} else {

	  register unsigned short k;

	  rs_exec_args = (char **) MALLOC(sizeof(char *) * (num_words(val_ptr) + 1));

	  for (k = 0; val_ptr; k++) {
	    rs_exec_args[k] = get_word(1, val_ptr);
	    val_ptr = get_pword(2, val_ptr);
	    D_OPTIONS(("rs_exec_args[%d] == %s\n", k, rs_exec_args[k]));
	  }
	  rs_exec_args[k] = (char *) NULL;
	}
      } else if (!strcasecmp(opt, "help")) {
	usage();
      } else if (!strcasecmp(opt, "version")) {
	version();
      } else if (!strcasecmp(opt, "attribute")) {
        conf_parse_line(NULL, val_ptr);
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
	      print_error("unrecognized boolean value \"%s\" for option --%s\n",
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
	    *((const char **) optList[j].pval) = STRDUP(val_ptr);
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
	  print_error("unrecognized option -%c\n", opt[pos]);
	  CHECK_BAD();
	  continue;
	}
	/* Put option-specific warnings here -- mej */
#if 0				/* No longer needed, since it works :) */
	if (optList[j].short_opt == 'w') {
	  print_error("warning:  Use of the -w / --border-width option is discouraged and unsupported.\n");
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
	    print_error("option -%c requires a%s value\n", opt[pos],
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

	  if (opt[pos + 1]) {
	    len = argc - i + 2;
	    k = i;
	  } else {
	    len = argc - i + 1;
	    k = i + 1;
	  }
	  D_OPTIONS(("len == %d  k == %d\n", len, k));
	  rs_exec_args = (char **) MALLOC(sizeof(char *) * len);

	  if (k == i) {
	    rs_exec_args[0] = STRDUP((char *) (val_ptr));
	    D_OPTIONS(("rs_exec_args[0] == %s\n", rs_exec_args[0]));
	    k++;
	  } else {
	    rs_exec_args[0] = STRDUP(argv[k - 1]);
	    D_OPTIONS(("rs_exec_args[0] == %s\n", rs_exec_args[0]));
	  }
	  for (; k < argc; k++) {
	    rs_exec_args[k - i] = STRDUP(argv[k]);
	    D_OPTIONS(("rs_exec_args[%d] == %s\n", k - i, rs_exec_args[k - i]));
	  }
	  rs_exec_args[len - 1] = (char *) NULL;
	  return;
	} else if (opt[pos] == 'h') {
	  usage();
	} else if (opt[pos] == 'a') {
          conf_parse_line(NULL, val_ptr);
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
	      *((const char **) optList[j].pval) = STRDUP(val_ptr);
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
	print_error("long option --%s requires a%s value", opt, (j == 3 ? "n integer" : " string\n"));
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
	  *((const char **) optList[j].pval) = STRDUP(val_ptr);
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
	  print_error("option -%c requires a string value\n", opt[pos]);
	  if (val_ptr) {	/* If the "arg" was actually an option, don't skip it */
	    i--;
	  }
	  continue;
	}
	D_OPTIONS(("String option detected\n"));
	if (optList[j].pval) {
	  *((const char **) optList[j].pval) = STRDUP(val_ptr);
	}
      }				/* End for loop */
    }				/* End if (islong) */
  }				/* End main for loop */
}

/* The config file parsers.  Each function handles a given context. */
static void *
parse_color(char *buff, void *state)
{
  if ((*buff == CONF_BEGIN_CHAR) || (*buff == CONF_END_CHAR)) {
    return NULL;
  }
  if (!BEG_STRCASECMP(buff, "foreground ")) {
    RESET_AND_ASSIGN(rs_color[fgColor], get_word(2, buff));
  } else if (!BEG_STRCASECMP(buff, "background ")) {
    RESET_AND_ASSIGN(rs_color[bgColor], get_word(2, buff));

  } else if (!BEG_STRCASECMP(buff, "cursor ")) {

#ifndef NO_CURSORCOLOR
    RESET_AND_ASSIGN(rs_color[cursorColor], get_word(2, buff));
#else
    print_warning("Support for the cursor attribute was not compiled in, ignoring\n");
#endif

  } else if (!BEG_STRCASECMP(buff, "cursor_text ")) {
#ifndef NO_CURSORCOLOR
    RESET_AND_ASSIGN(rs_color[cursorColor2], get_word(2, buff));
#else
    print_warning("Support for the cursor_text attribute was not compiled in, ignoring\n");
#endif

  } else if (!BEG_STRCASECMP(buff, "pointer ")) {
    RESET_AND_ASSIGN(rs_color[pointerColor], get_word(2, buff));

  } else if (!BEG_STRCASECMP(buff, "video ")) {

    char *tmp = get_pword(2, buff);

    if (!BEG_STRCASECMP(tmp, "reverse")) {
      Options |= Opt_reverse_video;
    } else if (BEG_STRCASECMP(tmp, "normal")) {
      print_error("Parse error in file %s, line %lu:  Invalid value \"%s\" for attribute video\n",
		  file_peek_path(), file_peek_line(), tmp);
    }
  } else if (!BEG_STRCASECMP(buff, "color ")) {

    char *tmp = 0, *r1, *g1, *b1;
    unsigned int n, r, g, b, index = 0;

    n = num_words(buff);
    if (n < 3) {
      print_error("Parse error in file %s, line %lu:  Invalid parameter list \"%s\" for \n"
		  "attribute color", file_peek_path(), file_peek_line(), NONULL(tmp));
      return NULL;
    }
    tmp = get_pword(2, buff);
    r1 = get_pword(3, buff);
    if (!isdigit(*r1)) {
      if (isdigit(*tmp)) {
	n = strtoul(tmp, (char **) NULL, 0);
	if (n <= 7) {
	  index = minColor + n;
	} else if (n >= 8 && n <= 15) {
	  index = minBright + n - 8;
	}
	RESET_AND_ASSIGN(rs_color[index], get_word(1, r1));
	return NULL;
      } else {
	if (!BEG_STRCASECMP(tmp, "bd ")) {
#ifndef NO_BOLDUNDERLINE
	  RESET_AND_ASSIGN(rs_color[colorBD], get_word(1, r1));
#else
	  print_warning("Support for the color bd attribute was not compiled in, ignoring\n");
#endif
	  return NULL;
	} else if (!BEG_STRCASECMP(tmp, "ul ")) {
#ifndef NO_BOLDUNDERLINE
	  RESET_AND_ASSIGN(rs_color[colorUL], get_word(1, r1));
#else
	  print_warning("Support for the color ul attribute was not compiled in, ignoring\n");
#endif
	  return NULL;
	} else {
	  tmp = get_word(1, tmp);
	  print_error("Parse error in file %s, line %lu:  Invalid color index \"%s\"\n",
		      file_peek_path(), file_peek_line(), NONULL(tmp));
	  FREE(tmp);
	}
      }
    }
    if (n != 5) {
      print_error("Parse error in file %s, line %lu:  Invalid parameter list \"%s\" for \n"
		  "attribute color", file_peek_path(), file_peek_line(), NONULL(tmp));
      return NULL;
    }
    g1 = get_pword(4, buff);
    b1 = get_pword(5, buff);
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
	print_error("Parse error in file %s, line %lu:  Invalid color index %lu\n",
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
      print_warning("Support for the color bd attribute was not compiled in, ignoring\n");
#endif

    } else if (!BEG_STRCASECMP(tmp, "ul ")) {
#ifndef NO_BOLDUNDERLINE
      RESET_AND_ASSIGN(rs_color[colorUL], MALLOC(14));
      r = strtoul(r1, (char **) NULL, 0);
      g = strtoul(g1, (char **) NULL, 0);
      b = strtoul(b1, (char **) NULL, 0);
      sprintf(rs_color[colorUL], "#%02x%02x%02x", r, g, b);
#else
      print_warning("Support for the color ul attribute was not compiled in, ignoring\n");
#endif

    } else {
      tmp = get_word(1, tmp);
      print_error("Parse error in file %s, line %lu:  Invalid color index \"%s\"\n",
		  file_peek_path(), file_peek_line(), NONULL(tmp));
      FREE(tmp);
    }
  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid within context color\n",
                file_peek_path(), file_peek_line(), buff);
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
    RESET_AND_ASSIGN(rs_geometry, get_word(2, buff));

  } else if (!BEG_STRCASECMP(buff, "title ")) {
    RESET_AND_ASSIGN(rs_title, get_word(2, buff));

  } else if (!BEG_STRCASECMP(buff, "name ")) {
    RESET_AND_ASSIGN(rs_name, get_word(2, buff));

  } else if (!BEG_STRCASECMP(buff, "iconname ")) {
    RESET_AND_ASSIGN(rs_iconName, get_word(2, buff));

  } else if (!BEG_STRCASECMP(buff, "desktop ")) {
    rs_desktop = (int) strtol(buff, (char **) NULL, 0);

  } else if (!BEG_STRCASECMP(buff, "scrollbar_type ")) {
    RESET_AND_ASSIGN(rs_scrollbar_type, get_word(2, buff));

  } else if (!BEG_STRCASECMP(buff, "scrollbar_width ")) {
    rs_scrollbar_width = strtoul(get_pword(2, buff), (char **) NULL, 0);

  } else if (!BEG_STRCASECMP(buff, "font ")) {

    char *tmp = get_pword(2, buff);
    unsigned char n;

    if (!BEG_STRCASECMP(tmp, "fx ") || !BEG_STRCASECMP(tmp, "effect")) {
      if (parse_font_fx(get_pword(2, tmp)) != 1) {
        print_error("Parse error in file %s, line %lu:  Syntax error in font effects specification\n",
                    file_peek_path(), file_peek_line());
      }
    } else if (!BEG_STRCASECMP(tmp, "prop")) {
      tmp = get_pword(2, tmp);
      if (BOOL_OPT_ISTRUE(tmp)) {
        Options |= Opt_proportional;
      } else if (BOOL_OPT_ISFALSE(tmp)) {
        Options &= ~(Opt_proportional);
      } else {
        print_error("Parse error in file %s, line %lu:  Invalid/missing boolean value for attribute proportional\n",
                    file_peek_path(), file_peek_line());
      }
    } else if (isdigit(*tmp)) {
      n = (unsigned char) strtoul(tmp, (char **) NULL, 0);
      if (n <= 255) {
        eterm_font_add(&etfonts, get_pword(2, tmp), n);
      } else {
	print_error("Parse error in file %s, line %lu:  Invalid font index %d\n",
		    file_peek_path(), file_peek_line(), n);
      }
    } else if (!BEG_STRCASECMP(tmp, "bold ")) {
#ifndef NO_BOLDFONT
      RESET_AND_ASSIGN(rs_boldFont, get_word(2, tmp));
#else
      print_warning("Support for the bold font attribute was not compiled in, ignoring\n");
#endif

    } else if (!BEG_STRCASECMP(tmp, "default ")) {
      def_font_idx = strtoul(get_pword(2, tmp), (char **) NULL, 0);

    } else {
      tmp = get_word(1, tmp);
      print_error("Parse error in file %s, line %lu:  Invalid font index \"%s\"\n",
		  file_peek_path(), file_peek_line(), NONULL(tmp));
      FREE(tmp);
    }

  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid within context attributes\n",
                file_peek_path(), file_peek_line(), (buff ? buff : ""));
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
  if (!(tmp = get_pword(2, buff))) {
    print_error("Parse error in file %s, line %lu:  Missing boolean value in context toggles\n", file_peek_path(), file_peek_line());
    return NULL;
  }
  if (BOOL_OPT_ISTRUE(tmp)) {
    bool_val = 1;
  } else if (BOOL_OPT_ISFALSE(tmp)) {
    bool_val = 0;
  } else {
    print_error("Parse error in file %s, line %lu:  Invalid boolean value \"%s\" in context toggles\n",
		file_peek_path(), file_peek_line(), tmp);
    return NULL;
  }

  if (!BEG_STRCASECMP(buff, "map_alert ")) {
#if !defined(NO_MAPALERT) && defined(MAPALERT_OPTION)
    if (bool_val) {
      Options |= Opt_map_alert;
    } else {
      Options &= ~(Opt_map_alert);
    }
#else
    print_warning("Support for the map_alert attribute was not compiled in, ignoring\n");
#endif

  } else if (!BEG_STRCASECMP(buff, "visual_bell ")) {
    if (bool_val) {
      Options |= Opt_visual_bell;
    } else {
      Options &= ~(Opt_visual_bell);
    }
  } else if (!BEG_STRCASECMP(buff, "login_shell ")) {
    if (bool_val) {
      Options |= Opt_login_shell;
    } else {
      Options &= ~(Opt_login_shell);
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
      Options |= Opt_write_utmp;
    } else {
      Options &= ~(Opt_write_utmp);
    }
#else
    print_warning("Support for the utmp_logging attribute was not compiled in, ignoring\n");
#endif

  } else if (!BEG_STRCASECMP(buff, "meta8 ")) {
#ifdef META8_OPTION
    if (bool_val) {
      Options |= Opt_meta8;
    } else {
      Options &= ~(Opt_meta8);
    }
#else
    print_warning("Support for the meta8 attribute was not compiled in, ignoring\n");
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

  } else if (!BEG_STRCASECMP(buff, "no_input ")) {
    if (bool_val) {
      Options |= Opt_no_input;
    } else {
      Options &= ~(Opt_no_input);
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
  } else if (!BEG_STRCASECMP(buff, "double_buffer ")) {
    if (bool_val) {
      Options |= Opt_double_buffer;
    } else {
      Options &= ~(Opt_double_buffer);
    }

  } else if (!BEG_STRCASECMP(buff, "no_cursor ")) {
    if (bool_val) {
      Options |= Opt_no_cursor;
    } else {
      Options &= ~(Opt_no_cursor);
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

  } else if (!BEG_STRCASECMP(buff, "mbyte_cursor ")) {
    if (bool_val) {
      Options |= Opt_mbyte_cursor;
    } else {
      Options &= ~(Opt_mbyte_cursor);
    }

  } else if (!BEG_STRCASECMP(buff, "itrans ") || !BEG_STRCASECMP(buff, "immotile_trans ")) {
    if (bool_val) {
      image_toggles |= IMOPT_ITRANS;
    } else {
      image_toggles &= ~IMOPT_ITRANS;
    }

  } else if (!BEG_STRCASECMP(buff, "buttonbar")) {
    if (bool_val) {
      FOREACH_BUTTONBAR(bbar_set_visible(bbar, 1););
      rs_buttonbars = 1;  /* Reset for future use. */
    } else {
      FOREACH_BUTTONBAR(bbar_set_visible(bbar, 0););
      rs_buttonbars = 1;  /* Reset for future use. */
    }

  } else if (!BEG_STRCASECMP(buff, "resize_gravity")) {
    if (bool_val) {
      Options |= Opt_resize_gravity;
    } else {
      Options &= ~(Opt_resize_gravity);
    }
  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid within context toggles\n", file_peek_path(), file_peek_line(), buff);
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
    RESET_AND_ASSIGN(rs_smallfont_key, get_word(2, buff));
    to_keysym(&ks_smallfont, rs_smallfont_key);
#else
    print_warning("Support for the smallfont_key attribute was not compiled in, ignoring\n");
#endif

  } else if (!BEG_STRCASECMP(buff, "bigfont_key ")) {
#if defined (HOTKEY_CTRL) || defined (HOTKEY_META)
    RESET_AND_ASSIGN(rs_bigfont_key, get_word(2, buff));
    to_keysym(&ks_bigfont, rs_bigfont_key);
#else
    print_warning("Support for the bigfont_key attribute was not compiled in, ignoring\n");
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
	print_error("Parse error in file %s, line %lu:  Keysym 0x%x out of range 0xff00-0xffff\n",
		    file_peek_path(), file_peek_line(), sym + 0xff00);
	return NULL;
      }
      s = get_word(3, buff);
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
    print_warning("Support for the keysym attributes was not compiled in, ignoring\n");
#endif

  } else if (!BEG_STRCASECMP(buff, "meta_mod ")) {
    char *tmp = get_pword(2, buff);

    if (!tmp) {
      print_error("Parse error in file %s, line %lu:  Missing modifier value for attribute meta_mod\n",
		  file_peek_path(), file_peek_line());
      return NULL;
    }
    rs_meta_mod = (unsigned int) strtoul(tmp, (char **) NULL, 0);

  } else if (!BEG_STRCASECMP(buff, "alt_mod ")) {
    char *tmp = get_pword(2, buff);

    if (!tmp) {
      print_error("Parse error in file %s, line %lu:  Missing modifier value for attribute alt_mod\n",
		  file_peek_path(), file_peek_line());
      return NULL;
    }
    rs_alt_mod = (unsigned int) strtoul(tmp, (char **) NULL, 0);
    
  } else if (!BEG_STRCASECMP(buff, "numlock_mod ")) {
    char *tmp = get_pword(2, buff);

    if (!tmp) {
      print_error("Parse error in file %s, line %lu:  Missing modifier value for attribute numlock_mod\n",
		  file_peek_path(), file_peek_line());
      return NULL;
    }
    rs_numlock_mod = (unsigned int) strtoul(tmp, (char **) NULL, 0);
    
  } else if (!BEG_STRCASECMP(buff, "greek ")) {
#ifdef GREEK_SUPPORT

    char *tmp = get_pword(2, buff);

    if (!tmp) {
      print_error("Parse error in file %s, line %lu:  Missing boolean value for attribute greek\n",
		  file_peek_path(), file_peek_line());
      return NULL;
    }
    if (BOOL_OPT_ISTRUE(tmp)) {
      RESET_AND_ASSIGN(rs_greek_keyboard, get_word(3, buff));
      if (BEG_STRCASECMP(rs_greek_keyboard, "iso")) {
	greek_setmode(GREEK_ELOT928);
      } else if (BEG_STRCASECMP(rs_greek_keyboard, "ibm")) {
	greek_setmode(GREEK_IBM437);
      } else {
	print_error("Parse error in file %s, line %lu:  Invalid greek keyboard mode \"%s\"\n",
		    file_peek_path(), file_peek_line(), (rs_greek_keyboard ? rs_greek_keyboard : ""));
      }
    } else if (BOOL_OPT_ISFALSE(tmp)) {
      /* This space intentionally left no longer blank =^) */
    } else {
      print_error("Parse error in file %s, line %lu:  Invalid boolean value \"%s\" for attribute %s\n",
		  file_peek_path(), file_peek_line(), tmp, buff);
      return NULL;
    }
#else
    print_warning("Support for the greek attribute was not compiled in, ignoring\n");
#endif

  } else if (!BEG_STRCASECMP(buff, "app_keypad ")) {

    char *tmp = get_pword(2, buff);

    if (!tmp) {
      print_error("Parse error in file %s, line %lu:  Missing boolean value for attribute app_keypad\n",
		  file_peek_path(), file_peek_line());
      return NULL;
    }
    if (BOOL_OPT_ISTRUE(tmp)) {
      PrivateModes |= PrivMode_aplKP;
    } else if (BOOL_OPT_ISFALSE(tmp)) {
      PrivateModes &= ~(PrivMode_aplKP);
    } else {
      print_error("Parse error in file %s, line %lu:  Invalid boolean value \"%s\" for attribute app_keypad\n",
                  file_peek_path(), file_peek_line(), tmp);
      return NULL;
    }

  } else if (!BEG_STRCASECMP(buff, "app_cursor ")) {

    char *tmp = get_pword(2, buff);

    if (!tmp) {
      print_error("Parse error in file %s, line %lu:  Missing boolean value for attribute app_cursor\n",
		  file_peek_path(), file_peek_line());
      return NULL;
    }
    if (BOOL_OPT_ISTRUE(tmp)) {
      PrivateModes |= PrivMode_aplCUR;
    } else if (BOOL_OPT_ISFALSE(tmp)) {
      PrivateModes &= ~(PrivMode_aplCUR);
    } else {
      print_error("Parse error in file %s, line %lu:  Invalid boolean value \"%s\" for attribute app_cursor\n",
                  file_peek_path(), file_peek_line(), tmp);
      return NULL;
    }

  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid within context keyboard\n",
                file_peek_path(), file_peek_line(), buff);
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
    RESET_AND_ASSIGN(rs_print_pipe, STRDUP(get_pword(2, buff)));
    chomp(rs_print_pipe);
#else
    print_warning("Support for the print_pipe attribute was not compiled in, ignoring\n");
#endif

  } else if (!BEG_STRCASECMP(buff, "save_lines ")) {
    rs_saveLines = strtol(get_pword(2, buff), (char **) NULL, 0);

  } else if (!BEG_STRCASECMP(buff, "min_anchor_size ")) {
    rs_min_anchor_size = strtol(get_pword(2, buff), (char **) NULL, 0);

  } else if (!BEG_STRCASECMP(buff, "border_width ")) {
#ifdef BORDER_WIDTH_OPTION
    TermWin.internalBorder = (short) strtol(get_pword(2, buff), (char **) NULL, 0);
#else
    print_warning("Support for the border_width attribute was not compiled in, ignoring\n");
#endif

  } else if (!BEG_STRCASECMP(buff, "line_space ")) {
    rs_line_space = strtol(get_pword(2, buff), (char **) NULL, 0);

  } else if (!BEG_STRCASECMP(buff, "finished_title ")) {
    RESET_AND_ASSIGN(rs_finished_title, get_word(2, buff));

  } else if (!BEG_STRCASECMP(buff, "finished_text ")) {
    RESET_AND_ASSIGN(rs_finished_text, get_word(2, buff));

  } else if (!BEG_STRCASECMP(buff, "term_name ")) {
    RESET_AND_ASSIGN(rs_term_name, get_word(2, buff));

  } else if (!BEG_STRCASECMP(buff, "debug ")) {
    DEBUG_LEVEL = (unsigned int) strtoul(get_pword(2, buff), (char **) NULL, 0);

  } else if (!BEG_STRCASECMP(buff, "exec ")) {

    register unsigned short k, n;

    RESET_AND_ASSIGN(rs_exec_args, (char **) MALLOC(sizeof(char *) * ((n = num_words(get_pword(2, buff))) + 1)));

    for (k = 0; k < n; k++) {
      rs_exec_args[k] = get_word(k + 2, buff);
      D_OPTIONS(("rs_exec_args[%d] == %s\n", k, rs_exec_args[k]));
    }
    rs_exec_args[n] = (char *) NULL;

  } else if (!BEG_STRCASECMP(buff, "cut_chars ")) {
#ifdef CUTCHAR_OPTION
    RESET_AND_ASSIGN(rs_cutchars, get_word(2, buff));
    chomp(rs_cutchars);
#else
    print_warning("Support for the cut_chars attribute was not compiled in, ignoring\n");
#endif

  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid within context misc\n",
                file_peek_path(), file_peek_line(), buff);
  }
  return state;
}

static void *
parse_imageclasses(char *buff, void *state)
{
  if ((*buff == CONF_BEGIN_CHAR) || (*buff == CONF_END_CHAR)) {
    return NULL;
  }

  if (!BEG_STRCASECMP(buff, "icon ")) {
#ifdef PIXMAP_SUPPORT
    RESET_AND_ASSIGN(rs_icon, get_word(2, buff));
#else
    print_warning("Pixmap support was not compiled in, ignoring \"icon\" attribute\n");
#endif

  } else if (!BEG_STRCASECMP(buff, "cache")) {
#ifdef PIXMAP_SUPPORT
    rs_cache_size = strtoul(get_pword(2, buff), (char **) NULL, 0);
#else
    print_warning("Pixmap support was not compiled in, ignoring \"cache\" attribute\n");
#endif

  } else if (!BEG_STRCASECMP(buff, "path ")) {
    RESET_AND_ASSIGN(rs_path, get_word(2, buff));

  } else if (!BEG_STRCASECMP(buff, "anim ")) {
#ifdef BACKGROUND_CYCLING_SUPPORT
    char *tmp = get_pword(2, buff);

    if (tmp) {
      rs_anim_pixmap_list = STRDUP(tmp);
    } else {
      print_error("Parse error in file %s, line %lu:  Invalid parameter list \"\" for attribute anim\n", file_peek_path(), file_peek_line());
    }
#else
    print_warning("Support for the anim attribute was not compiled in, ignoring\n");
#endif

  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid within context imageclasses\n",
                file_peek_path(), file_peek_line(), buff);
  }
  return state;
}

static void *
parse_image(char *buff, void *state)
{
  int idx;

  if (*buff == CONF_BEGIN_CHAR) {
    int *tmp;

    tmp = (int *) MALLOC(sizeof(int));
    *tmp = -1;
    return ((void *) tmp);
  }
  ASSERT_RVAL(state != NULL, (void *)(file_skip_to_end(), NULL));
  if (*buff == CONF_END_CHAR) {
    int *tmp;

    tmp = (int *) state;
    FREE(tmp);
    return NULL;
  }
  idx = *((int *) state);
  if (!BEG_STRCASECMP(buff, "type ")) {
    char *type = get_pword(2, buff);

    if (!type) {
      print_error("Parse error in file %s, line %lu:  Missing image type\n", file_peek_path(), file_peek_line());
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
      print_error("Parse error in file %s, line %lu:  Invalid image type \"%s\"\n", file_peek_path(), file_peek_line(), type);
      return NULL;
    }
    *((int *) state) = idx;

  } else if (!BEG_STRCASECMP(buff, "mode ")) {
    char *mode = get_pword(2, buff);
    char *allow_list = get_pword(4, buff);

    if (!CHECK_VALID_INDEX(idx)) {
      print_error("Parse error in file %s, line %lu:  Encountered \"mode\" with no image type defined\n", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (!mode) {
      print_error("Parse error in file %s, line %lu:  Missing parameters for mode line\n", file_peek_path(), file_peek_line());
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
      print_error("Parse error in file %s, line %lu:  Invalid mode \"%s\"\n", file_peek_path(), file_peek_line(), mode);
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
	  print_error("Parse error in file %s, line %lu:  Invalid mode \"%s\"\n", file_peek_path(), file_peek_line(), allow);
	}
      }
    }
  } else if (!BEG_STRCASECMP(buff, "state ")) {
    char *state = get_pword(2, buff), new = 0;

    if (!state) {
      print_error("Parse error in file %s, line %lu:  Missing state\n", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (!CHECK_VALID_INDEX(idx)) {
      print_error("Parse error in file %s, line %lu:  Encountered \"state\" with no image type defined\n", file_peek_path(), file_peek_line());
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
      print_error("Parse error in file %s, line %lu:  Invalid state \"%s\"\n", file_peek_path(), file_peek_line(), state);
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
    char *fg = get_word(2, buff), *bg = get_word(3, buff);

    if (!CHECK_VALID_INDEX(idx)) {
      print_error("Parse error in file %s, line %lu:  Encountered \"color\" with no image type defined\n", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (images[idx].current == NULL) {
      print_error("Parse error in file %s, line %lu:  Encountered \"color\" with no image state defined\n", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (!fg || !bg) {
      print_error("Parse error in file %s, line %lu:  Foreground and background colors must be specified with \"color\"\n", file_peek_path(), file_peek_line());
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

#ifdef PIXMAP_SUPPORT
  } else if (!BEG_STRCASECMP(buff, "file ")) {
    char *filename = get_pword(2, buff);

    if (!CHECK_VALID_INDEX(idx)) {
      print_error("Parse error in file %s, line %lu:  Encountered \"file\" with no image type defined\n", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (images[idx].current == NULL) {
      print_error("Parse error in file %s, line %lu:  Encountered \"file\" with no image state defined\n", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (!filename) {
      print_error("Parse error in file %s, line %lu:  Missing filename\n", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (!load_image(filename, images[idx].current)) {
      images[idx].mode &= ~(MODE_IMAGE | ALLOW_IMAGE);
      D_PIXMAP(("New image mode is 0x%02x, iml->im is 0x%08x\n", images[idx].mode, images[idx].current->iml->im));
    }

  } else if (!BEG_STRCASECMP(buff, "geom ")) {
    char *geom = get_pword(2, buff);

    if (!CHECK_VALID_INDEX(idx)) {
      print_error("Parse error in file %s, line %lu:  Encountered \"geom\" with no image type defined\n", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (images[idx].current == NULL) {
      print_error("Parse error in file %s, line %lu:  Encountered \"geom\" with no image state defined\n", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (!geom) {
      print_error("Parse error in file %s, line %lu:  Missing geometry string\n", file_peek_path(), file_peek_line());
      return NULL;
    }
    set_pixmap_scale(geom, images[idx].current->pmap);

  } else if (!BEG_STRCASECMP(buff, "cmod ") || !BEG_STRCASECMP(buff, "colormod ")) {
    char *color = get_pword(2, buff);
    char *mods = get_pword(3, buff);
    unsigned char n;
    imlib_t *iml = images[idx].current->iml;

    if (!CHECK_VALID_INDEX(idx)) {
      print_error("Parse error in file %s, line %lu:  Encountered color modifier with no image type defined\n", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (images[idx].current == NULL) {
      print_error("Parse error in file %s, line %lu:  Encountered color modifier with no image state defined\n", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (!color) {
      print_error("Parse error in file %s, line %lu:  Missing color name\n", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (!mods) {
      print_error("Parse error in file %s, line %lu:  Missing modifier(s)\n", file_peek_path(), file_peek_line());
      return NULL;
    }
    n = num_words(mods);

    if (!BEG_STRCASECMP(color, "image ")) {
      if (iml->mod) {
        free_colormod(iml->mod);
      }
      iml->mod = create_colormod();
      iml->mod->brightness = (int) strtol(mods, (char **) NULL, 0);
      if (n > 1) {
        iml->mod->contrast = (int) strtol(get_pword(2, mods), (char **) NULL, 0);
      }
      if (n > 2) {
        iml->mod->gamma = (int) strtol(get_pword(3, mods), (char **) NULL, 0);
      }
      update_cmod(iml->mod);
    } else if (!BEG_STRCASECMP(color, "red ")) {
      if (iml->rmod) {
        free_colormod(iml->rmod);
      }
      iml->rmod = create_colormod();
      iml->rmod->brightness = (int) strtol(mods, (char **) NULL, 0);
      if (n > 1) {
        iml->rmod->contrast = (int) strtol(get_pword(2, mods), (char **) NULL, 0);
      }
      if (n > 2) {
        iml->rmod->gamma = (int) strtol(get_pword(3, mods), (char **) NULL, 0);
      }
      update_cmod(iml->rmod);
    } else if (!BEG_STRCASECMP(color, "green ")) {
      if (iml->gmod) {
        free_colormod(iml->gmod);
      }
      iml->gmod = create_colormod();
      iml->gmod->brightness = (int) strtol(mods, (char **) NULL, 0);
      if (n > 1) {
        iml->gmod->contrast = (int) strtol(get_pword(2, mods), (char **) NULL, 0);
      }
      if (n > 2) {
        iml->gmod->gamma = (int) strtol(get_pword(3, mods), (char **) NULL, 0);
      }
      update_cmod(iml->gmod);
    } else if (!BEG_STRCASECMP(color, "blue ")) {
      if (iml->bmod) {
        free_colormod(iml->bmod);
      }
      iml->bmod = create_colormod();
      iml->bmod->brightness = (int) strtol(mods, (char **) NULL, 0);
      if (n > 1) {
        iml->bmod->contrast = (int) strtol(get_pword(2, mods), (char **) NULL, 0);
      }
      if (n > 2) {
        iml->bmod->gamma = (int) strtol(get_pword(3, mods), (char **) NULL, 0);
      }
      update_cmod(iml->bmod);
    } else {
      print_error("Parse error in file %s, line %lu:  Color must be either \"image\", \"red\", \"green\", or \"blue\"\n", file_peek_path(), file_peek_line());
      return NULL;
    }
#endif

  } else if (!BEG_STRCASECMP(buff, "border ")) {
    if (!CHECK_VALID_INDEX(idx)) {
      print_error("Parse error in file %s, line %lu:  Encountered \"border\" with no image type defined\n", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (num_words(buff + 7) < 4) {
      print_error("Parse error in file %s, line %lu:  Invalid parameter list for attribute \"border\"\n", file_peek_path(), file_peek_line());
      return NULL;
    }
    RESET_AND_ASSIGN(images[idx].current->iml->border, (Imlib_Border *) MALLOC(sizeof(Imlib_Border)));

    images[idx].current->iml->border->left = (unsigned short) strtoul(get_pword(2, buff), (char **) NULL, 0);
    images[idx].current->iml->border->right = (unsigned short) strtoul(get_pword(3, buff), (char **) NULL, 0);
    images[idx].current->iml->border->top = (unsigned short) strtoul(get_pword(4, buff), (char **) NULL, 0);
    images[idx].current->iml->border->bottom = (unsigned short) strtoul(get_pword(5, buff), (char **) NULL, 0);

    if ((images[idx].current->iml->border->left == 0) && (images[idx].current->iml->border->right == 0)
	&& (images[idx].current->iml->border->top == 0) && (images[idx].current->iml->border->bottom == 0)) {
      FREE(images[idx].current->iml->border);
      images[idx].current->iml->border = (Imlib_Border *) NULL;	/* No sense in wasting CPU time and memory if there are no borders */
    }
  } else if (!BEG_STRCASECMP(buff, "bevel ")) {
    if (!CHECK_VALID_INDEX(idx)) {
      print_error("Parse error in file %s, line %lu:  Encountered \"bevel\" with no image type defined\n", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (images[idx].current == NULL) {
      print_error("Parse error in file %s, line %lu:  Encountered \"bevel\" with no image state defined\n", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (num_words(buff + 6) < 5) {
      print_error("Parse error in file %s, line %lu:  Invalid parameter list for attribute \"bevel\"\n", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (images[idx].current->iml->bevel != NULL) {
      FREE(images[idx].current->iml->bevel->edges);
      FREE(images[idx].current->iml->bevel);
    }
    images[idx].current->iml->bevel = (bevel_t *) MALLOC(sizeof(bevel_t));
    images[idx].current->iml->bevel->edges = (Imlib_Border *) MALLOC(sizeof(Imlib_Border));

    if (!BEG_STRCASECMP(get_pword(2, buff), "down")) {
      images[idx].current->iml->bevel->up = 0;
    } else {
      images[idx].current->iml->bevel->up = 1;
    }
    images[idx].current->iml->bevel->edges->left = (unsigned short) strtoul(get_pword(3, buff), (char **) NULL, 0);
    images[idx].current->iml->bevel->edges->right = (unsigned short) strtoul(get_pword(4, buff), (char **) NULL, 0);
    images[idx].current->iml->bevel->edges->top = (unsigned short) strtoul(get_pword(5, buff), (char **) NULL, 0);
    images[idx].current->iml->bevel->edges->bottom = (unsigned short) strtoul(get_pword(6, buff), (char **) NULL, 0);

    if ((images[idx].current->iml->bevel->edges->left == 0) && (images[idx].current->iml->bevel->edges->right == 0)
	&& (images[idx].current->iml->bevel->edges->top == 0) && (images[idx].current->iml->bevel->edges->bottom == 0)) {
      FREE(images[idx].current->iml->bevel->edges);
      images[idx].current->iml->bevel->edges = (Imlib_Border *) NULL;
      FREE(images[idx].current->iml->bevel);
      images[idx].current->iml->bevel = (bevel_t *) NULL;
    }
  } else if (!BEG_STRCASECMP(buff, "padding ")) {
    if (!CHECK_VALID_INDEX(idx)) {
      print_error("Parse error in file %s, line %lu:  Encountered \"padding\" with no image type defined\n", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (images[idx].current == NULL) {
      print_error("Parse error in file %s, line %lu:  Encountered \"padding\" with no image state defined\n", file_peek_path(), file_peek_line());
      return NULL;
    }
    if (num_words(buff + 8) < 4) {
      print_error("Parse error in file %s, line %lu:  Invalid parameter list for attribute \"padding\"\n", file_peek_path(), file_peek_line());
      return NULL;
    }
    RESET_AND_ASSIGN(images[idx].current->iml->pad, (Imlib_Border *) MALLOC(sizeof(Imlib_Border)));

    images[idx].current->iml->pad->left = (unsigned short) strtoul(get_pword(2, buff), (char **) NULL, 0);
    images[idx].current->iml->pad->right = (unsigned short) strtoul(get_pword(3, buff), (char **) NULL, 0);
    images[idx].current->iml->pad->top = (unsigned short) strtoul(get_pword(4, buff), (char **) NULL, 0);
    images[idx].current->iml->pad->bottom = (unsigned short) strtoul(get_pword(5, buff), (char **) NULL, 0);

    if ((images[idx].current->iml->pad->left == 0) && (images[idx].current->iml->pad->right == 0)
	&& (images[idx].current->iml->pad->top == 0) && (images[idx].current->iml->pad->bottom == 0)) {
      FREE(images[idx].current->iml->pad);
      images[idx].current->iml->pad = (Imlib_Border *) NULL;
    }
  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid within context image\n",
                file_peek_path(), file_peek_line(), buff);
  }
  return ((void *) state);
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
    for (i = 2; (str = get_word(i, buff)) && strcasecmp(str, "to"); i++) {
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
        button = *(str + 6) - '0';
      } else if (isdigit(*str)) {
        keysym = (KeySym) strtoul(str, (char **) NULL, 0);
      } else {
        keysym = XStringToKeysym(str);
      }
      FREE(str);
    }
    if (!str) {
      print_error("Parse error in file %s, line %lu:  Syntax error (\"to\" not found)\n", file_peek_path(), file_peek_line());
      return NULL;
    }
    FREE(str);
    if ((button == BUTTON_NONE) && (keysym == 0)) {
      print_error("Parse error in file %s, line %lu:  No valid button/keysym found for action\n", file_peek_path(), file_peek_line());
      return NULL;
    }
    i++;
    str = get_pword(i, buff);
    if (!BEG_STRCASECMP(str, "string")) {
      str = get_word(i+1, buff);
      action_add(mod, button, keysym, ACTION_STRING, (void *) str);
      FREE(str);
    } else if (!BEG_STRCASECMP(str, "echo")) {
      str = get_word(i+1, buff);
      action_add(mod, button, keysym, ACTION_ECHO, (void *) str);
      FREE(str);
    } else if (!BEG_STRCASECMP(str, "menu")) {
      menu_t *menu;

      str = get_word(i+1, buff);
      menu = find_menu_by_title(menu_list, str);
      action_add(mod, button, keysym, ACTION_MENU, (void *) menu);
      FREE(str);
    } else if (!BEG_STRCASECMP(str, "script")) {
      str = get_word(i+1, buff);
      action_add(mod, button, keysym, ACTION_SCRIPT, (void *) str);
      FREE(str);
    } else {
      print_error("Parse error in file %s, line %lu:  No valid action type found.  Valid types are \"string,\" \"echo,\" \"menu,\" and \"script.\"\n",
                  file_peek_path(), file_peek_line());
      return NULL;
    }

  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid within context action\n",
                file_peek_path(), file_peek_line(), buff);
  }
  return state;
}

static void *
parse_menu(char *buff, void *state)
{
  menu_t *menu;

  if (*buff == CONF_BEGIN_CHAR) {
    char *title = get_pword(2, buff + 6);

    menu = menu_create(title);
    return ((void *) menu);
  }
  ASSERT_RVAL(state != NULL, (void *)(file_skip_to_end(), NULL));
  menu = (menu_t *) state;
  if (*buff == CONF_END_CHAR) {
    if (!(*(menu->title))) {
      char tmp[20];

      sprintf(tmp, "Eterm_Menu_%u", menu_list->nummenus);
      menu_set_title(menu, tmp);
      print_error("Parse error in file %s, line %lu:  Menu context ended without giving a title.  Defaulted to \"%s\".\n", file_peek_path(), file_peek_line(), tmp);
    }
    menu_list = menulist_add_menu(menu_list, menu);
    return NULL;
  }
  if (!BEG_STRCASECMP(buff, "title ")) {
    char *title = get_word(2, buff);

    menu_set_title(menu, title);
    FREE(title);

  } else if (!BEG_STRCASECMP(buff, "font ")) {
    char *name = get_word(2, buff);

    if (!name) {
      print_error("Parse error in file %s, line %lu:  Missing font name.\n", file_peek_path(), file_peek_line());
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
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid within context menu\n", file_peek_path(), file_peek_line(), buff);
  }
  return ((void *) menu);
}

static void *
parse_menuitem(char *buff, void *state)
{
  static menu_t *menu;
  menuitem_t *curitem;

  ASSERT_RVAL(state != NULL, (void *)(file_skip_to_end(), NULL));
  if (*buff == CONF_BEGIN_CHAR) {
    menu = (menu_t *) state;
    curitem = menuitem_create(NULL);
    return ((void *) curitem);
  }
  curitem = (menuitem_t *) state;
  ASSERT_RVAL(menu != NULL, state);
  if (*buff == CONF_END_CHAR) {
    if (!(curitem->text)) {
      print_error("Parse error in file %s, line %lu:  Menuitem context ended with no text given.  Discarding this entry.\n", file_peek_path(), file_peek_line());
      FREE(curitem);
    } else {
      menu_add_item(menu, curitem);
    }
    return ((void *) menu);
  }
  if (!BEG_STRCASECMP(buff, "text ")) {
    char *text = get_word(2, buff);

    if (!text) {
      print_error("Parse error in file %s, line %lu:  Missing menuitem text.\n", file_peek_path(), file_peek_line());
      return ((void *) curitem);
    }
    menuitem_set_text(curitem, text);
    FREE(text);

  } else if (!BEG_STRCASECMP(buff, "rtext ")) {
    char *rtext = get_word(2, buff);

    if (!rtext) {
      print_error("Parse error in file %s, line %lu:  Missing menuitem right-justified text.\n", file_peek_path(), file_peek_line());
      return ((void *) curitem);
    }
    menuitem_set_rtext(curitem, rtext);
    FREE(rtext);

  } else if (!BEG_STRCASECMP(buff, "icon ")) {

  } else if (!BEG_STRCASECMP(buff, "action ")) {
    char *type = get_pword(2, buff);
    char *action = get_word(3, buff);

    if (!BEG_STRCASECMP(type, "submenu ")) {
      menuitem_set_action(curitem, MENUITEM_SUBMENU, action);

    } else if (!BEG_STRCASECMP(type, "string ")) {
      menuitem_set_action(curitem, MENUITEM_STRING, action);

    } else if (!BEG_STRCASECMP(type, "script ")) {
      menuitem_set_action(curitem, MENUITEM_SCRIPT, action);

    } else if (!BEG_STRCASECMP(type, "echo ")) {
      menuitem_set_action(curitem, MENUITEM_ECHO, action);

    } else if (!BEG_STRCASECMP(type, "separator")) {
      menuitem_set_action(curitem, MENUITEM_SEP, action);

    } else {
      print_error("Parse error in file %s, line %lu:  Invalid menu item action \"%s\"\n", file_peek_path(), file_peek_line(), NONULL(type));
    }
    FREE(action);

  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid within context menu\n", file_peek_path(), file_peek_line(), buff);
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
  ASSERT_RVAL(state != NULL, (void *)(file_skip_to_end(), NULL));
  bbar = (buttonbar_t *) state;
  if (*buff == CONF_END_CHAR) {
    bbar_add(bbar);
    return NULL;
  }
  if (!BEG_STRCASECMP(buff, "font ")) {
    char *font = get_word(2, buff);

    bbar_set_font(bbar, font);
    FREE(font);

  } else if (!BEG_STRCASECMP(buff, "dock ")) {
    char *where = get_pword(2, buff);

    if (!where) {
      print_error("Parse error in file %s, line %lu:  Attribute dock requires a parameter\n", file_peek_path(), file_peek_line());
    } else if (!BEG_STRCASECMP(where, "top")) {
      bbar_set_docked(bbar, BBAR_DOCKED_TOP);
    } else if (!BEG_STRCASECMP(where, "bot")) {  /* "bot" or "bottom" */
      bbar_set_docked(bbar, BBAR_DOCKED_BOTTOM);
    } else if (!BEG_STRCASECMP(where, "no")) {  /* "no" or "none" */
      bbar_set_docked(bbar, BBAR_UNDOCKED);
    } else {
      print_error("Parse error in file %s, line %lu:  Invalid parameter \"%s\" to attribute dock\n", file_peek_path(), file_peek_line(), where);
    }

  } else if (!BEG_STRCASECMP(buff, "visible ")) {
    char *tmp = get_pword(2, buff);

    if (BOOL_OPT_ISTRUE(tmp)) {
      bbar_set_visible(bbar, 1);
    } else if (BOOL_OPT_ISFALSE(tmp)) {
      bbar_set_visible(bbar, 0);
    } else {
      print_error("Parse error in file %s, line %lu:  Invalid boolean value \"%s\" in context button_bar\n", file_peek_path(), file_peek_line(), tmp);
    }

  } else if (!BEG_STRCASECMP(buff, "button ") || !BEG_STRCASECMP(buff, "rbutton ")) {
    char *text = get_pword(2, buff);
    char *icon = strcasestr(buff, "icon ");
    char *action = strcasestr(buff, "action ");
    button_t *button;

    if (text == icon) {
      text = NULL;
    } else {
      text = get_word(1, text);
    }
    if (!text && !icon) {
      print_error("Parse error in file %s, line %lu:  Missing button specifications\n", file_peek_path(), file_peek_line());
      return ((void *) bbar);
    }

    button = button_create(text);
    if (icon) {
      simage_t *simg;

      icon = get_word(2, icon);
      simg = create_simage();
      if (load_image(icon, simg)) {
        button_set_icon(button, simg);
      } else {
        free_simage(simg);
      }
      FREE(icon);
    }
    if (action) {
      char *type = get_pword(2, action);

      action = get_word(2, type);
      if (!BEG_STRCASECMP(type, "menu ")) {
        button_set_action(button, ACTION_MENU, action);
      } else if (!BEG_STRCASECMP(type, "string ")) {
        button_set_action(button, ACTION_STRING, action);
      } else if (!BEG_STRCASECMP(type, "echo ")) {
        button_set_action(button, ACTION_ECHO, action);
      } else if (!BEG_STRCASECMP(type, "script ")) {
        button_set_action(button, ACTION_SCRIPT, action);
      } else {
        print_error("Parse error in file %s, line %lu:  Invalid button action \"%s\"\n", file_peek_path(), file_peek_line(), type);
        FREE(action);
        FREE(button);
        return ((void *) bbar);
      }
      FREE(action);
    } else {
      print_error("Parse error in file %s, line %lu:  Missing button action\n", file_peek_path(), file_peek_line());
      FREE(button);
      return ((void *) bbar);
    }
    if (tolower(*buff) == 'r') {
      bbar_add_rbutton(bbar, button);
    } else {
      bbar_add_button(bbar, button);
    }
  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid within context menu\n",
                file_peek_path(), file_peek_line(), buff);
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
    RESET_AND_ASSIGN(rs_input_method, get_word(2, buff));
  } else if (!BEG_STRCASECMP(buff, "preedit_type ")) {
    RESET_AND_ASSIGN(rs_preedit_type, get_word(2, buff));
  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid within context xim\n",
		file_peek_path(), file_peek_line(), buff);
  }
#else
  print_warning("XIM support was not compiled in, ignoring entire context\n");
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
    RESET_AND_ASSIGN(rs_multichar_encoding, get_word(2, buff));
    if (rs_multichar_encoding != NULL) {
      if (BEG_STRCASECMP(rs_multichar_encoding, "eucj")
	  && BEG_STRCASECMP(rs_multichar_encoding, "sjis")
	  && BEG_STRCASECMP(rs_multichar_encoding, "euckr")
	  && BEG_STRCASECMP(rs_multichar_encoding, "big5")
	  && BEG_STRCASECMP(rs_multichar_encoding, "gb")
	  && BEG_STRCASECMP(rs_multichar_encoding, "iso-10646")
	  && BEG_STRCASECMP(rs_multichar_encoding, "none")) {
	print_error("Parse error in file %s, line %lu:  Invalid multichar encoding mode \"%s\"\n",
		    file_peek_path(), file_peek_line(), rs_multichar_encoding);
        FREE(rs_multichar_encoding);
	return NULL;
      }
    } else {
      print_error("Parse error in file %s, line %lu:  Invalid parameter list \"\" for attribute encoding\n",
		  file_peek_path(), file_peek_line());
    }
  } else if (!BEG_STRCASECMP(buff, "font ")) {

    char *tmp = get_pword(2, buff);
    unsigned char n;

    if (num_words(buff) != 3) {
      print_error("Parse error in file %s, line %lu:  Invalid parameter list \"%s\" for attribute font\n",
                  file_peek_path(), file_peek_line(), NONULL(tmp));
      return NULL;
    }
    if (isdigit(*tmp)) {
      n = (unsigned char) strtoul(tmp, (char **) NULL, 0);
      if (n <= 255) {
        eterm_font_add(&etmfonts, get_pword(2, tmp), n);
      } else {
	print_error("Parse error in file %s, line %lu:  Invalid font index %d\n",
		    file_peek_path(), file_peek_line(), n);
      }
    } else {
      tmp = get_word(1, tmp);
      print_error("Parse error in file %s, line %lu:  Invalid font index \"%s\"\n",
		  file_peek_path(), file_peek_line(), NONULL(tmp));
      FREE(tmp);
    }

  } else {
    print_error("Parse error in file %s, line %lu:  Attribute \"%s\" is not valid within context multichar\n",
		file_peek_path(), file_peek_line(), buff);
  }
#else
  if (*buff == CONF_BEGIN_CHAR) {
    print_warning("Multichar support was not compiled in, ignoring entire context\n");
    file_poke_skip(1);
  }
#endif
  return state;
  buff = NULL;
}

char *
conf_parse_theme(char **theme, char *conf_name, unsigned char fallback)
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
  if (fallback & PARSE_TRY_USER_THEME) {
    if (theme && *theme && (ret = conf_parse(conf_name, *theme, path)) != NULL) {
      return ret;
    }
  }
  if (fallback & PARSE_TRY_DEFAULT_THEME) {
    RESET_AND_ASSIGN(*theme, STRDUP(PACKAGE));
    if ((ret = conf_parse(conf_name, *theme, path)) != NULL) {
      return ret;
    }
  }
  if (fallback & PARSE_TRY_NO_THEME) {
    RESET_AND_ASSIGN(*theme, NULL);
    return (conf_parse(conf_name, *theme, path));
  }
  return NULL;
}

/* Initialize the default values for everything */
void
init_defaults(void)
{
#ifndef AUTO_ENCODING
  unsigned char i;
#endif

#if DEBUG >= DEBUG_MEM
  if (DEBUG_LEVEL >= DEBUG_MEM) {
    memrec_init();
  }
#endif

  Options = (Opt_scrollbar | Opt_select_trailing_spaces);
  Xdisplay = NULL;
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
#if AUTO_ENCODING
#ifdef MULTI_CHARSET
  eterm_default_font_locale(&etfonts, &etmfonts, &rs_multichar_encoding, &def_font_idx);
#else
  eterm_default_font_locale(&etfonts, NULL, NULL, &def_font_idx);
#endif
#else
  for (i = 0; i < NFONTS; i++) {
    eterm_font_add(&etfonts, def_fontName[i], i);
#ifdef MULTI_CHARSET
    eterm_font_add(&etmfonts, def_mfontName[i], i);
#endif
  }
#ifdef MULTI_CHARSET
  rs_multichar_encoding = STRDUP(MULTICHAR_ENCODING);
#endif
#endif
  TermWin.internalBorder = DEFAULT_BORDER_WIDTH;

  /* Initialize the parser */
  conf_init_subsystem();

  /* Register Eterm's context parsers. */
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

/* Sync up options with our internal data after parsing options and configs */
void
post_parse(void)
{
  register int i;

  if (rs_scrollbar_type) {
    if (!strcasecmp(rs_scrollbar_type, "xterm")) {
#ifdef XTERM_SCROLLBAR
      scrollbar_set_type(SCROLLBAR_XTERM);
#else
      print_error("Support for xterm scrollbars was not compiled in.  Sorry.\n");
#endif
    } else if (!strcasecmp(rs_scrollbar_type, "next")) {
#ifdef NEXT_SCROLLBAR
      scrollbar_set_type(SCROLLBAR_NEXT);
#else
      print_error("Support for NeXT scrollbars was not compiled in.  Sorry.\n");
#endif
    } else if (!strcasecmp(rs_scrollbar_type, "motif")) {
#ifdef MOTIF_SCROLLBAR
      scrollbar_set_type(SCROLLBAR_MOTIF);
#else
      print_error("Support for motif scrollbars was not compiled in.  Sorry.\n");
#endif
    } else {
      print_error("Unrecognized scrollbar type \"%s\".\n", rs_scrollbar_type);
    }
  }
  if (rs_scrollbar_width) {
    scrollbar_set_width(rs_scrollbar_width);
  }

  /* set any defaults not already set */
  if (rs_name == NULL) {
    if (rs_exec_args != NULL) {
      rs_name = STRDUP(rs_exec_args[0]);
    } else {
      rs_name = STRDUP(APL_NAME " " VERSION);
    }
  }
  if (!rs_title) {
    rs_title = rs_name;
  }
  if (!rs_iconName) {
    rs_iconName = rs_name;
  }
  if ((TermWin.saveLines = rs_saveLines) < 0) {
    TermWin.saveLines = SAVELINES;
  }
  /* no point having a scrollbar without having any scrollback! */
  if (!TermWin.saveLines) {
    Options &= ~Opt_scrollbar;
  }

#ifdef PRINTPIPE
  if (!rs_print_pipe) {
    rs_print_pipe = STRDUP(PRINTPIPE);
  }
#endif
#ifdef CUTCHAR_OPTION
  if (!rs_cutchars) {
    rs_cutchars = STRDUP(CUTCHARS);
  }
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
    if (rs_multichar_encoding != NULL) {
      set_multichar_encoding(rs_multichar_encoding);
    }
#endif
  }
  if (rs_font_effects) {
    if (parse_font_fx(rs_font_effects) != 1) {
      print_error("Syntax error in the font effects specified on the command line.\n");
    }
    RESET_AND_ASSIGN(rs_font_effects, NULL);
  }

  /* Clean up image stuff */
  for (i = 0; i < image_max; i++) {
    simage_t *simg;
    imlib_t *iml;

    if (images[i].norm) {
      simg = images[i].norm;
      iml = simg->iml;
      /* If we have a bevel but no border, use the bevel as a border. */
      if (iml->bevel && !(iml->border)) {
        iml->border = iml->bevel->edges;
      }
#ifdef PIXMAP_SUPPORT
      if (iml->im) {
        imlib_context_set_image(iml->im);
        update_cmod_tables(iml);
      }
#endif
      images[i].userdef = 1;
    } else {
      simg = images[i].norm = (simage_t *) MALLOC(sizeof(simage_t));
      simg->pmap = (pixmap_t *) MALLOC(sizeof(pixmap_t));
      simg->iml = (imlib_t *) MALLOC(sizeof(imlib_t));
      simg->fg = WhitePixel(Xdisplay, Xscreen);
      simg->bg = BlackPixel(Xdisplay, Xscreen);
      MEMSET(simg->pmap, 0, sizeof(pixmap_t));
      MEMSET(simg->iml, 0, sizeof(imlib_t));
      images[i].mode = MODE_IMAGE & ALLOW_IMAGE;
    }
    images[i].current = simg;
#ifdef PIXMAP_SUPPORT
    if (rs_pixmaps[i]) {
      reset_simage(images[i].norm, RESET_ALL_SIMG);
      load_image(rs_pixmaps[i], images[i].norm);
      FREE(rs_pixmaps[i]);	/* These are created by STRDUP() */
    }
#else
    /* Right now, solid mode is the only thing we can do without pixmap support. */
    images[i].mode = MODE_SOLID & ALLOW_SOLID;
#endif

    if (images[i].selected) {
      simage_t *norm_simg = images[i].norm;

      simg = images[i].selected;
      iml = simg->iml;
      /* If we have a bevel but no border, use the bevel as a border. */
      if (iml->bevel && !(iml->border)) {
        iml->border = iml->bevel->edges;
      }
      /* If normal has an image but we don't, copy it. */
      if (!(simg->iml->im) && (norm_simg->iml->im)) {
        simg->iml->im = norm_simg->iml->im;
        *(simg->pmap) = *(norm_simg->pmap);
      }
      if (simg->fg == 0 && simg->bg == 0) {
        simg->fg = norm_simg->fg;
        simg->bg = norm_simg->bg;
      }
#ifdef PIXMAP_SUPPORT
      if (iml->im) {
        imlib_context_set_image(iml->im);
        update_cmod_tables(iml);
      }
#endif
    } else {
      D_PIXMAP(("No \"selected\" state for image %s.  Setting fallback to the normal state.\n", get_image_type(i)));
      images[i].selected = images[i].norm;
    }
    if (images[i].clicked) {
      simage_t *norm_simg = images[i].norm;

      simg = images[i].clicked;
      iml = simg->iml;
      /* If we have a bevel but no border, use the bevel as a border. */
      if (iml->bevel && !(iml->border)) {
        iml->border = iml->bevel->edges;
      }
      /* If normal has an image but we don't, copy it. */
      if (!(simg->iml->im) && (norm_simg->iml->im)) {
        simg->iml->im = norm_simg->iml->im;
        *(simg->pmap) = *(norm_simg->pmap);
      }
      if (simg->fg == 0 && simg->bg == 0) {
        simg->fg = norm_simg->fg;
        simg->bg = norm_simg->bg;
      }
#ifdef PIXMAP_SUPPORT
      if (iml->im) {
        imlib_context_set_image(iml->im);
        update_cmod_tables(iml);
      }
#endif
    } else {
      D_PIXMAP(("No \"clicked\" state for image %s.  Setting fallback to the selected state.\n", get_image_type(i)));
      images[i].clicked = images[i].selected;
    }
    if (images[i].disabled) {
      simage_t *norm_simg = images[i].norm;

      simg = images[i].disabled;
      iml = simg->iml;
      /* If we have a bevel but no border, use the bevel as a border. */
      if (iml->bevel && !(iml->border)) {
        iml->border = iml->bevel->edges;
      }
      /* If normal has an image but we don't, copy it. */
      if (!(simg->iml->im) && (norm_simg->iml->im)) {
        simg->iml->im = norm_simg->iml->im;
        *(simg->pmap) = *(norm_simg->pmap);
      }
      if (simg->fg == 0 && simg->bg == 0) {
        simg->fg = norm_simg->fg;
        simg->bg = norm_simg->bg;
      }
#ifdef PIXMAP_SUPPORT
      if (iml->im) {
        imlib_context_set_image(iml->im);
        update_cmod_tables(iml);
      }
#endif
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

  /* rs_buttonbars is set to 1.  If it stays 1, the option was never
     specified.  If specified, it will either become 3 (on) or 0 (off). */
  if (rs_buttonbars != 1) {
    if (rs_buttonbars) {
      FOREACH_BUTTONBAR(bbar_set_visible(bbar, 1););
    } else {
      FOREACH_BUTTONBAR(bbar_set_visible(bbar, 0););
    }
    rs_buttonbars = 1;  /* Reset for future use. */
  }
  /* Update buttonbar sizes based on new imageclass info. */
  bbar_resize_all(-1);

#ifdef PIXMAP_SUPPORT
  /* Support the deprecated forms by converting the syntax to the new system */
  if (rs_shade != 0) {
    char buff[10];

    sprintf(buff, "0x%03x", ((100 - rs_shade) << 8) / 100);
    rs_cmod_image = STRDUP(buff);
    D_PIXMAP(("--shade value of %d converted to cmod value of %s\n", rs_shade, rs_cmod_image));
  }
  if (rs_tint) {
    char buff[10];
    unsigned long r, g, b, t;

    if (!isdigit(*rs_tint)) {
      t = get_tint_by_color_name(rs_tint);
    } else {
      t = (unsigned long) strtoul(rs_tint, (char **) NULL, 0);
      D_PIXMAP(("Got numerical tint 0x%06x\n", t));
    }
    if (t != 0xffffff) {
      r = (t & 0xff0000) >> 16;
      if (r != 0xff) {
        sprintf(buff, "0x%03lx", r);
        rs_cmod_red = STRDUP(buff);
      }
      g = (t & 0xff00) >> 8;
      if (g != 0xff) {
        sprintf(buff, "0x%03lx", g);
        rs_cmod_green = STRDUP(buff);
      }
      b = t & 0xff;
      if (b != 0xff) {
        sprintf(buff, "0x%03lx", b);
        rs_cmod_blue = STRDUP(buff);
      }
    }
    FREE(rs_tint);
  }
  if (rs_cmod_image) {
    unsigned char n = num_words(rs_cmod_image);
    imlib_t *iml = images[image_bg].norm->iml;

    if (iml->mod) {
      free_colormod(iml->mod);
    }
    iml->mod = create_colormod();
    iml->mod->brightness = (int) strtol(rs_cmod_image, (char **) NULL, 0);
    if (n > 1) {
      iml->mod->contrast = (int) strtol(get_pword(2, rs_cmod_image), (char **) NULL, 0);
    }
    if (n > 2) {
      iml->mod->gamma = (int) strtol(get_pword(3, rs_cmod_image), (char **) NULL, 0);
    }
    D_PIXMAP(("From image cmod string %s to brightness %d, contrast %d, and gamma %d\n", rs_cmod_image,
              iml->mod->brightness, iml->mod->contrast, iml->mod->gamma));
    FREE(rs_cmod_image);
  }
  if (rs_cmod_red) {
    unsigned char n = num_words(rs_cmod_red);
    imlib_t *iml = images[image_bg].norm->iml;

    if (iml->rmod) {
      free_colormod(iml->rmod);
    }
    iml->rmod = create_colormod();
    iml->rmod->brightness = (int) strtol(rs_cmod_red, (char **) NULL, 0);
    if (n > 1) {
      iml->rmod->contrast = (int) strtol(get_pword(2, rs_cmod_red), (char **) NULL, 0);
    }
    if (n > 2) {
      iml->rmod->gamma = (int) strtol(get_pword(3, rs_cmod_red), (char **) NULL, 0);
    }
    D_PIXMAP(("From red cmod string %s to brightness %d, contrast %d, and gamma %d\n", rs_cmod_red,
              iml->rmod->brightness, iml->rmod->contrast, iml->rmod->gamma));
    FREE(rs_cmod_red);
    update_cmod(iml->rmod);
  }
  if (rs_cmod_green) {
    unsigned char n = num_words(rs_cmod_green);
    imlib_t *iml = images[image_bg].norm->iml;

    if (iml->gmod) {
      free_colormod(iml->gmod);
    }
    iml->gmod = create_colormod();
    iml->gmod->brightness = (int) strtol(rs_cmod_green, (char **) NULL, 0);
    if (n > 1) {
      iml->gmod->contrast = (int) strtol(get_pword(2, rs_cmod_green), (char **) NULL, 0);
    }
    if (n > 2) {
      iml->gmod->gamma = (int) strtol(get_pword(3, rs_cmod_green), (char **) NULL, 0);
    }
    D_PIXMAP(("From green cmod string %s to brightness %d, contrast %d, and gamma %d\n", rs_cmod_green,
              iml->gmod->brightness, iml->gmod->contrast, iml->gmod->gamma));
    FREE(rs_cmod_green);
    update_cmod(iml->gmod);
  }
  if (rs_cmod_blue) {
    unsigned char n = num_words(rs_cmod_blue);
    imlib_t *iml = images[image_bg].norm->iml;

    if (iml->bmod) {
      free_colormod(iml->bmod);
    }
    iml->bmod = create_colormod();
    iml->bmod->brightness = (int) strtol(rs_cmod_blue, (char **) NULL, 0);
    if (n > 1) {
      iml->bmod->contrast = (int) strtol(get_pword(2, rs_cmod_blue), (char **) NULL, 0);
    }
    if (n > 2) {
      iml->bmod->gamma = (int) strtol(get_pword(3, rs_cmod_blue), (char **) NULL, 0);
    }
    D_PIXMAP(("From blue cmod string %s to brightness %d, contrast %d, and gamma %d\n", rs_cmod_blue,
              iml->bmod->brightness, iml->bmod->contrast, iml->bmod->gamma));
    FREE(rs_cmod_blue);
    update_cmod(iml->bmod);
  }
  if (images[image_bg].norm->iml->im) {
    imlib_context_set_image(images[image_bg].norm->iml->im);
    update_cmod_tables(images[image_bg].norm->iml);
  }

  if (rs_cache_size == (unsigned long) -1) {
    imlib_set_cache_size(0);
  } else {
    imlib_set_cache_size(rs_cache_size);
  }
#endif

  if (Options & Opt_reverse_video) {
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
      int count;

      count = num_words(rs_anim_pixmap_list) - 1;	/* -1 for the delay */
      rs_anim_pixmaps = (char **) MALLOC(sizeof(char *) * (count + 1));

      for (i = 0; i < count; i++) {
	temp = get_word(i + 2, rs_anim_pixmap_list);	/* +2 rather than +1 to account for the delay */
	if (temp == NULL)
	  break;
	if (num_words(temp) != 3) {
	  if (num_words(temp) == 1) {
	    rs_anim_pixmaps[i] = temp;
	  }
	} else {
	  w1 = get_pword(1, temp);
	  h1 = get_pword(2, temp);
	  w = strtol(w1, (char **) NULL, 0);
	  h = strtol(h1, (char **) NULL, 0);
	  if (w || h) {
	    rs_anim_pixmaps[i] = get_word(3, temp);
	    rs_anim_pixmaps[i] = (char *) REALLOC(rs_anim_pixmaps[i], strlen(rs_anim_pixmaps[i]) + 9);
	    strcat(rs_anim_pixmaps[i], "@100x100");
	  } else {
	    rs_anim_pixmaps[i] = get_word(3, temp);
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

  if (rs_pipe_name) {
    struct stat fst;

    if (lstat(rs_pipe_name, &fst) != 0) {
      print_error("Unable to stat console pipe \"%s\" -- %s\n", rs_pipe_name, strerror(errno));
    } else {
      if (S_ISREG(fst.st_mode) || S_ISDIR(fst.st_mode)) {
        print_error("Directories and regular files are not valid console pipes.  Sorry.\n");
      } else {
        pipe_fd = open(rs_pipe_name, O_RDONLY | O_NDELAY | O_NOCTTY);
        if (pipe_fd < 0) {
          print_error("Unable to open console pipe -- %s\n", strerror(errno));
        }
      }      
    }
  }
}

unsigned char
save_config(char *path, unsigned char save_theme)
{
  register FILE *fp;
  register short i;
  char *tmp_str, dt_stamp[50];
  time_t cur_time = time(NULL);
  struct tm *cur_tm;
  struct stat fst;
  simage_t *simg;
  action_t *action;

  D_OPTIONS(("Saving %s config to \"%s\"\n", (save_theme ? "theme" : "user"), NONULL(path)));

  cur_tm = localtime(&cur_time);

  if (save_theme) {
    if (!path) {
      path = (char *) MALLOC(PATH_MAX + 1);
      strncpy(path, (theme_dir ? theme_dir : PKGDATADIR "/themes/Eterm"), PATH_MAX - sizeof("/" THEME_CFG));
      path[PATH_MAX] = 0;
      if (stat(path, &fst) || !S_ISDIR(fst.st_mode) || !CAN_WRITE(fst)) {
        char *tmp = NULL;

        D_OPTIONS(("Problem with \"%s\".  S_ISDIR == %d, CAN_WRITE == %d\n", path, S_ISDIR(fst.st_mode), CAN_WRITE(fst)));
        if (theme_dir) {
          tmp = strrchr(theme_dir, '/');
          if (tmp) {
            *tmp++ = 0;
          }
        }
        snprintf(path, PATH_MAX, "%s/.Eterm/themes/%s", getenv("HOME"), (tmp ? tmp : "Eterm"));
        D_OPTIONS(("Trying \"%s\" instead, tmp == \"%s\"\n", path, tmp));
        if (tmp) {
          *(--tmp) = '/';
        }
        if (!mkdirhier(path) || (stat(path, &fst) && !CAN_WRITE(fst))) {
          print_error("I couldn't write to \"%s\" or \"%s\".  I give up.", (theme_dir ? theme_dir : PKGDATADIR "/themes/Eterm\n"), path);
          return errno;
        }
      }
      strcat(path, "/" THEME_CFG);
      D_OPTIONS(("Final path is \"%s\"\n", path));
      path[PATH_MAX] = 0;
    }
  } else {
    if (!path) {
      path = (char *) MALLOC(PATH_MAX + 1);
      strncpy(path, (user_dir ? user_dir : PKGDATADIR "/themes/Eterm"), PATH_MAX - sizeof("/" USER_CFG));
      path[PATH_MAX] = 0;
      if (stat(path, &fst) || !S_ISDIR(fst.st_mode) || !CAN_WRITE(fst)) {
        char *tmp = NULL;

        D_OPTIONS(("Problem with \"%s\".  S_ISDIR == %d, CAN_WRITE == %d\n", path, S_ISDIR(fst.st_mode), CAN_WRITE(fst)));
        if (user_dir) {
          tmp = strrchr(user_dir, '/');
          if (tmp) {
            *tmp++ = 0;
          }
        }
        snprintf(path, PATH_MAX, "%s/.Eterm/themes/%s", getenv("HOME"), (tmp ? tmp : "Eterm"));
        D_OPTIONS(("Trying \"%s\" instead, tmp == \"%s\"\n", path, tmp));
        if (tmp) {
          *(--tmp) = '/';
        }
        if (!mkdirhier(path) || (stat(path, &fst) && !CAN_WRITE(fst))) {
          print_error("I couldn't write to \"%s\" or \"%s\".  I give up.", (user_dir ? user_dir : PKGDATADIR "/themes/Eterm\n"), path);
          return errno;
        }
      }
      strcat(path, "/" USER_CFG);
      D_OPTIONS(("Final path is \"%s\"\n", path));
      path[PATH_MAX] = 0;
    }
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

  fprintf(fp, "begin color\n");
  fprintf(fp, "    foreground %s\n", COLOR_NAME(fgColor));
  fprintf(fp, "    background %s\n", COLOR_NAME(bgColor));
  fprintf(fp, "    cursor %s\n", COLOR_NAME(cursorColor));
  fprintf(fp, "    cursor_text %s\n", COLOR_NAME(cursorColor2));
  fprintf(fp, "    pointer %s\n", COLOR_NAME(pointerColor));
  fprintf(fp, "    video normal\n");
  for (i = 0; i < 16; i++) {
    fprintf(fp, "    color %d %s\n", i, COLOR_NAME(minColor + i));
  }
#ifndef NO_BOLDUNDERLINE
  if (rs_color[colorBD]) {
    fprintf(fp, "    color bd %s\n", COLOR_NAME(colorBD));
  }
  if (rs_color[colorUL]) {
    fprintf(fp, "    color ul %s\n", COLOR_NAME(colorUL));
  }
#endif
  fprintf(fp, "end color\n\n");

  fprintf(fp, "begin attributes\n");
  if (save_theme) {
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
  }
  fprintf(fp, "    scrollbar_type %s\n", (scrollbar_get_type() == SCROLLBAR_XTERM ? "xterm" : (scrollbar_get_type() == SCROLLBAR_MOTIF ? "motif" : "next")));
  fprintf(fp, "    scrollbar_width %d\n", scrollbar_anchor_width());
  fprintf(fp, "    font default %u\n", (unsigned int) font_idx);
  fprintf(fp, "    font proportional %d\n", ((Options & Opt_proportional) ? 1 : 0));
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
  fprintf(fp, "end attributes\n\n");

  if (save_theme) {
    fprintf(fp, "begin imageclasses\n");
    fprintf(fp, "    path \"%s\"\n", rs_path);
#ifdef PIXMAP_SUPPORT
    if (rs_icon != NULL) {
      fprintf(fp, "    icon %s\n", rs_icon);
    }
    if (rs_anim_delay) {
      /* FIXME:  Do something here! */
    }
#endif
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
      case image_dialog:    fprintf(fp, "      type dialog_box\n"); break;
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
#ifdef PIXMAP_SUPPORT
      if (simg->iml->im) {
        imlib_context_set_image(simg->iml->im);
      }
#endif
      fprintf(fp, "      state normal\n");
      if (simg->fg || simg->bg) {
        fprintf(fp, "      color 0x%08x 0x%08x\n", (unsigned int) simg->fg, (unsigned int) simg->bg);
      }
#ifdef PIXMAP_SUPPORT
      if (simg->iml->im) {
        fprintf(fp, "      file %s\n", NONULL(imlib_image_get_filename()));
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
#endif
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
#ifdef PIXMAP_SUPPORT
        if (simg->iml->im) {
          imlib_context_set_image(simg->iml->im);
        }
#endif
        fprintf(fp, "      state selected\n");
        if (simg->fg || simg->bg) {
          fprintf(fp, "      color 0x%08x 0x%08x\n", (unsigned int) simg->fg, (unsigned int) simg->bg);
        }
#ifdef PIXMAP_SUPPORT
        if (simg->iml->im) {
          fprintf(fp, "      file %s\n", NONULL(imlib_image_get_filename()));
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
#endif
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
#ifdef PIXMAP_SUPPORT
        if (simg->iml->im) {
          imlib_context_set_image(simg->iml->im);
        }
#endif
        fprintf(fp, "      state clicked\n");
        if (simg->fg || simg->bg) {
          fprintf(fp, "      color 0x%08x 0x%08x\n", (unsigned int) simg->fg, (unsigned int) simg->bg);
        }
#ifdef PIXMAP_SUPPORT
        if (simg->iml->im) {
          fprintf(fp, "      file %s\n", NONULL(imlib_image_get_filename()));
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
#endif
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
#ifdef PIXMAP_SUPPORT
        if (simg->iml->im) {
          imlib_context_set_image(simg->iml->im);
        }
#endif
        fprintf(fp, "      state disabled\n");
        if (simg->fg || simg->bg) {
          fprintf(fp, "      color 0x%08x 0x%08x\n", (unsigned int) simg->fg, (unsigned int) simg->bg);
        }
#ifdef PIXMAP_SUPPORT
        if (simg->iml->im) {
          fprintf(fp, "      file %s\n", NONULL(imlib_image_get_filename()));
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
#endif
        if (simg->iml->border) {
          fprintf(fp, "      border %hu %hu %hu %hu\n", simg->iml->border->left, simg->iml->border->right, simg->iml->border->top, simg->iml->border->bottom);
        }
        if (simg->iml->bevel) {
          fprintf(fp, "      bevel %s %hu %hu %hu %hu\n", ((simg->iml->bevel->up) ? "up" : "down"), simg->iml->bevel->edges->left, simg->iml->bevel->edges->right,
                  simg->iml->bevel->edges->top, simg->iml->bevel->edges->bottom);
        }
        if (simg->iml->pad) {
          fprintf(fp, "      padding %hu %hu %hu %hu\n", simg->iml->pad->left, simg->iml->pad->right, simg->iml->pad->top, simg->iml->pad->bottom);
        }
      }
      fprintf(fp, "    end image\n");
    }
    fprintf(fp, "end imageclasses\n\n");

    for (i = 0; i < menu_list->nummenus; i++) {
      menu_t *menu = menu_list->menus[i];
      unsigned short j;

      fprintf(fp, "begin menu\n");
      fprintf(fp, "    title \"%s\"\n", menu->title);
      if (menu->font) {
        unsigned long tmp;

        if ((XGetFontProperty(menu->font, XA_FONT_NAME, &tmp)) == True) {
          fprintf(fp, "    font '%s'\n", ((char *) tmp));
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
            fprintf(fp, "string '%s'\n", safe_print_string(item->action.string, -1));
          } else if (item->type == MENUITEM_ECHO) {
            fprintf(fp, "echo '%s'\n", safe_print_string(item->action.string, -1));
          } else if (item->type == MENUITEM_SUBMENU) {
            fprintf(fp, "submenu \"%s\"\n", (item->action.submenu)->title);
          } else if (item->type == MENUITEM_SCRIPT) {
            fprintf(fp, "script '%s'\n", item->action.script);
          }
          fprintf(fp, "    end\n");
        }
      }
      fprintf(fp, "end menu\n");
    }
    fprintf(fp, "\n");
  }

  fprintf(fp, "begin actions\n");
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
    }
    if (action->keysym) {
      fprintf(fp, "0x%04x", (unsigned int) action->keysym);
    } else {
      fprintf(fp, "button%d", (int) action->button);
    }
    fprintf(fp, " to ");
    if (action->type == ACTION_STRING) {
      fprintf(fp, "string '%s'\n", safe_print_string(action->param.string, -1));
    } else if (action->type == ACTION_ECHO) {
      fprintf(fp, "echo '%s'\n", safe_print_string(action->param.string, -1));
    } else if (action->type == ACTION_MENU) {
      fprintf(fp, "menu \"%s\"\n", (action->param.menu)->title);
    } else if (action->type == ACTION_SCRIPT) {
      fprintf(fp, "script '%s'\n", action->param.script);
    }
  }
  fprintf(fp, "end actions\n\n");

#ifdef MULTI_CHARSET
  fprintf(fp, "begin multichar\n");
  if (rs_multichar_encoding) {
    fprintf(fp, "    encoding %s\n", rs_multichar_encoding);
  }
  for (i = 0; i < font_cnt; i++) {
    if (etmfonts[i]) {
      fprintf(fp, "    font %d %s\n", i, etmfonts[i]);
    }
  }
  fprintf(fp, "end multichar\n\n");
#endif

#ifdef USE_XIM
  fprintf(fp, "begin xim\n");
  if (rs_input_method) {
    fprintf(fp, "    input_method %s\n", rs_input_method);
  }
  if (rs_preedit_type) {
    fprintf(fp, "    preedit_type %s\n", rs_preedit_type);
  }
  fprintf(fp, "end xim\n\n");
#endif

  fprintf(fp, "begin toggles\n");
  fprintf(fp, "    map_alert %d\n", (Options & Opt_map_alert ? 1 : 0));
  fprintf(fp, "    visual_bell %d\n", (Options & Opt_visual_bell ? 1 : 0));
  fprintf(fp, "    login_shell %d\n", (Options & Opt_login_shell ? 1 : 0));
  fprintf(fp, "    scrollbar %d\n", (Options & Opt_scrollbar ? 1 : 0));
  fprintf(fp, "    utmp_logging %d\n", (Options & Opt_write_utmp ? 1 : 0));
  fprintf(fp, "    meta8 %d\n", (Options & Opt_meta8 ? 1 : 0));
  fprintf(fp, "    iconic %d\n", (Options & Opt_iconic ? 1 : 0));
  fprintf(fp, "    home_on_output %d\n", (Options & Opt_home_on_output ? 1 : 0));
  fprintf(fp, "    home_on_input %d\n", (Options & Opt_home_on_input ? 1 : 0));
  fprintf(fp, "    no_input %d\n", (Options & Opt_no_input ? 1 : 0));
  fprintf(fp, "    scrollbar_floating %d\n", (Options & Opt_scrollbar_floating ? 1 : 0));
  fprintf(fp, "    scrollbar_right %d\n", (Options & Opt_scrollbar_right ? 1 : 0));
  fprintf(fp, "    scrollbar_popup %d\n", (Options & Opt_scrollbar_popup ? 1 : 0));
  fprintf(fp, "    borderless %d\n", (Options & Opt_borderless ? 1 : 0));
  fprintf(fp, "    double_buffer %d\n", (Options & Opt_double_buffer ? 1 : 0));
  fprintf(fp, "    no_cursor %d\n", (Options & Opt_no_cursor ? 1 : 0));
  fprintf(fp, "    pause %d\n", (Options & Opt_pause ? 1 : 0));
  fprintf(fp, "    xterm_select %d\n", (Options & Opt_xterm_select ? 1 : 0));
  fprintf(fp, "    select_line %d\n", (Options & Opt_select_whole_line ? 1 : 0));
  fprintf(fp, "    select_trailing_spaces %d\n", (Options & Opt_select_trailing_spaces ? 1 : 0));
  fprintf(fp, "    report_as_keysyms %d\n", (Options & Opt_report_as_keysyms ? 1 : 0));
  fprintf(fp, "    itrans %d\n", (image_toggles & IMOPT_ITRANS ? 1 : 0));
  fprintf(fp, "    buttonbar %d\n", ((buttonbar && bbar_is_visible(buttonbar)) ? 1 : 0));
  fprintf(fp, "    resize_gravity %d\n", (Options & Opt_resize_gravity ? 1 : 0));
  fprintf(fp, "end toggles\n\n");

  fprintf(fp, "begin keyboard\n");
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
      fprintf(fp, "    keysym 0xff%02x \'%s\'\n", i, safe_print_string((char *) (KeySym_map[i] + 1), (unsigned long) KeySym_map[i][0]));
    }
  }
#ifdef GREEK_SUPPORT
  if (rs_greek_keyboard) {
    fprintf(fp, "    greek on %s\n", rs_greek_keyboard);
  }
#endif
  fprintf(fp, "    app_keypad %d\n", (PrivateModes & PrivMode_aplKP ? 1 : 0));
  fprintf(fp, "    app_cursor %d\n", (PrivateModes & PrivMode_aplCUR ? 1 : 0));
  fprintf(fp, "end keyboard\n\n");

  fprintf(fp, "begin misc\n");
#ifdef PRINTPIPE
  if (rs_print_pipe) {
    fprintf(fp, "    print_pipe '%s'\n", rs_print_pipe);
  }
#endif
  fprintf(fp, "    save_lines %d\n", rs_saveLines);
  fprintf(fp, "    min_anchor_size %d\n", rs_min_anchor_size);
  fprintf(fp, "    border_width %d\n", TermWin.internalBorder);
  fprintf(fp, "    term_name %s\n", getenv("TERM"));
  fprintf(fp, "    debug %d\n", DEBUG_LEVEL);
  if (save_theme && rs_exec_args) {
    fprintf(fp, "    exec ");
    for (i = 0; rs_exec_args[i]; i++) {
      fprintf(fp, "'%s' ", rs_exec_args[i]);
    }
    fprintf(fp, "\n");
  }
#ifdef CUTCHAR_OPTIONS
  if (rs_cutchars) {
    fprintf(fp, "    cut_chars '%s'\n", rs_cutchars);
  }
#endif
  fprintf(fp, "end misc\n\n");

  fclose(fp);
  return 0;
}
