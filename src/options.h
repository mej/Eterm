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

#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include <X11/Xfuncproto.h>
#include <X11/Intrinsic.h>	/* Xlib, Xutil, Xresource, Xfuncproto */

/************ Macros and Definitions ************/
#define OPT_BOOLEAN                     0x0001
#define OPT_INTEGER                     0x0002
#define OPT_STRING                      0x0004
#define OPT_ARGUMENT                    0x0008

#define OPT_STR(s, l, d, p)             { s, l, "(str)  " d, OPT_STRING,   (const char **)  p, 0, 0 }
#define OPT_INT(s, l, d, p)             { s, l, "(int)  " d, OPT_INTEGER,  (const int *)    p, 0, 0 }
#define OPT_BOOL(s, l, d, v, m)         { s, l, "(bool) " d, OPT_BOOLEAN,                NULL, v, m }
#define OPT_LONG(l, d, p)               { 0, l, "(str)  " d, OPT_STRING,   (const char **)  p, 0, 0 }
#define OPT_ARGS(s, l, d, p)            { s, l, "(str)  " d, OPT_ARGUMENT, (const char ***) p, 0, 0 }
#define OPT_BLONG(l, d, v, m)           { 0, l, "(bool) " d, OPT_BOOLEAN,                NULL, v, m }
#define OPT_ILONG(l, d, p)              { 0, l, "(int)  " d, OPT_INTEGER,  (const int *)    p, 0, 0 }
#define optList_numoptions()            (sizeof(optList)/sizeof(optList[0]))

# define Opt_console                    (1LU <<  0)
# define Opt_login_shell                (1LU <<  1)
# define Opt_iconic                     (1LU <<  2)
# define Opt_visual_bell                (1LU <<  3)
# define Opt_map_alert                  (1LU <<  4)
# define Opt_reverse_video              (1LU <<  5)
# define Opt_write_utmp                 (1LU <<  6)
# define Opt_scrollbar                  (1LU <<  7)
# define Opt_meta8                      (1LU <<  8)
# define Opt_home_on_output             (1LU <<  9)
# define Opt_scrollbar_right            (1LU << 10)
# define Opt_borderless                 (1LU << 11)
# define Opt_no_input                   (1LU << 12)
# define Opt_no_cursor                  (1LU << 13)
# define Opt_pause                      (1LU << 14)
# define Opt_home_on_input              (1LU << 15)
# define Opt_report_as_keysyms          (1LU << 16)
# define Opt_xterm_select               (1LU << 17)
# define Opt_select_whole_line          (1LU << 18)
# define Opt_scrollbar_popup            (1LU << 19)
# define Opt_select_trailing_spaces     (1LU << 20)
# define Opt_install                    (1LU << 21)
# define Opt_scrollbar_floating         (1LU << 22)
# define Opt_double_buffer              (1LU << 23)
# define Opt_mbyte_cursor		(1LU << 24)
# define Opt_proportional               (1LU << 25)
# define Opt_resize_gravity             (1LU << 26)

# define IMOPT_TRANS                    (1U << 0)
# define IMOPT_ITRANS                   (1U << 1)
# define IMOPT_VIEWPORT                 (1U << 2)

# define BBAR_FORCE_TOGGLE              (0x03)

# define SAVE_THEME_CONFIG              ((unsigned char) 1)
# define SAVE_USER_CONFIG               ((unsigned char) 0)

#define PARSE_TRY_USER_THEME            ((unsigned char) 0x01)
#define PARSE_TRY_DEFAULT_THEME         ((unsigned char) 0x02)
#define PARSE_TRY_NO_THEME              ((unsigned char) 0x04)
#define PARSE_TRY_ALL                   ((unsigned char) 0x07)

/* This defines how many mistakes to allow before giving up
   and printing the usage                          -- mej   */
#define BAD_THRESHOLD 3
#define CHECK_BAD()  do { \
	               if (++bad_opts >= BAD_THRESHOLD) { \
			 print_error("Error threshold exceeded, giving up.\n"); \
			 usage(); \
		       } else { \
			 print_error("Attempting to continue, but strange things may happen.\n"); \
		       } \
                     } while(0)

#define to_keysym(p,s)             do { KeySym sym; \
                                     if (s && ((sym = XStringToKeysym(s)) != 0)) *p = sym; \
                                   } while (0)
#define CHECK_VALID_INDEX(i)       (((i) >= image_bg) && ((i) < image_max))

#define RESET_AND_ASSIGN(var, val)  do {if ((var) != NULL) FREE(var);  (var) = (val);} while (0)

/************ Structures ************/

/************ Variables ************/
extern unsigned long Options, image_toggles;
extern char *theme_dir, *user_dir;
extern       char **rs_exec_args;       /* Args to exec (-e or --exec) */
extern       char  *rs_title;		/* Window title */
extern       char  *rs_iconName;	/* Icon name */
extern       char  *rs_geometry;	/* Geometry string */
extern        int   rs_desktop;         /* Startup desktop */
extern        int   rs_saveLines;	/* Lines in the scrollback buffer */
extern unsigned short rs_min_anchor_size; /* Minimum size, in pixels, of the scrollbar anchor */
extern       char  *rs_finished_title;	/* Text added to window title (--pause) */
extern       char  *rs_finished_text;	/* Text added to scrollback (--pause) */
extern       char  *rs_term_name;
extern       char  *rs_icon;
extern       char  *rs_scrollbar_type;
extern unsigned long rs_scrollbar_width;
extern       char  *rs_scrollbar_type;
extern       char  *rs_anim_pixmap_list;
extern       char **rs_anim_pixmaps;
extern     time_t   rs_anim_delay;
extern char *rs_path;
extern char *rs_no_cursor;
#ifdef USE_XIM
extern char *rs_input_method;
extern char *rs_preedit_type;
#endif
extern char *rs_name;
extern char *rs_theme;
extern char *rs_config_file;
extern unsigned int rs_line_space;
extern unsigned int rs_meta_mod, rs_alt_mod, rs_numlock_mod;
#ifndef NO_BOLDFONT
extern char *rs_boldFont;
#endif
#ifdef PRINTPIPE
extern char *rs_print_pipe;
#endif
extern char *rs_cutchars;
#ifdef CUTCHAR_OPTION
extern       char  *rs_cutchars;
#endif
extern const char *true_vals[];
extern const char *false_vals[];
#ifdef KEYSYM_ATTRIBUTE
extern unsigned char *KeySym_map[256];
#endif
#if defined (HOTKEY_CTRL) || defined (HOTKEY_META)
extern KeySym ks_bigfont;
extern KeySym ks_smallfont;
#endif

/************ Function Prototypes ************/
_XFUNCPROTOBEGIN

unsigned long num_words(const char *str);
extern void get_initial_options(int, char **);
extern void get_options(int, char **);
extern char *conf_parse_theme(char **theme, char *conf_name, unsigned char fallback);
extern void init_defaults(void);
extern void post_parse(void);
unsigned char save_config(char *, unsigned char);

_XFUNCPROTOEND

#endif	/* _OPTIONS_H_ */
