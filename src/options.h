/*  options.h -- Eterm options module header file
 *            -- 25 July 1997, mej
 *
 * This file is original work by Michael Jennings <mej@eterm.org> and
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

#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include <X11/Xfuncproto.h>
#include <X11/Intrinsic.h>	/* Xlib, Xutil, Xresource, Xfuncproto */

/************ Macros and Definitions ************/
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
#define optList_numoptions()        (sizeof(optList)/sizeof(optList[0]))

#if PATH_MAX < 1024
#  undef PATH_MAX
#  define PATH_MAX 1024
#endif

# define Opt_console			(1LU <<  0)
# define Opt_loginShell			(1LU <<  1)
# define Opt_iconic			(1LU <<  2)
# define Opt_visualBell			(1LU <<  3)
# define Opt_mapAlert			(1LU <<  4)
# define Opt_reverseVideo		(1LU <<  5)
# define Opt_utmpLogging		(1LU <<  6)
# define Opt_scrollBar			(1LU <<  7)
# define Opt_meta8			(1LU <<  8)
# define Opt_pixmapScale		(1LU <<  9)
# define Opt_exec			(1LU << 10)
# define Opt_homeOnEcho			(1LU << 11)
# define Opt_homeOnRefresh		(1LU << 12)
# define Opt_scrollBar_floating		(1LU << 13)
# define Opt_scrollBar_right		(1LU << 14)
# define Opt_borderless			(1LU << 15)
# define Opt_pixmapTrans		(1LU << 16)
# define Opt_backing_store		(1LU << 17)
# define Opt_noCursor			(1LU << 18)
# define Opt_pause			(1LU << 19)
# define Opt_homeOnInput		(1LU << 20)
# define Opt_report_as_keysyms		(1LU << 21)
# define Opt_xterm_select		(1LU << 22)
# define Opt_select_whole_line		(1LU << 23)
# define Opt_viewport_mode		(1LU << 24)
# define Opt_scrollbar_popup		(1LU << 25)
# define Opt_select_trailing_spaces	(1LU << 26)
# define Opt_install                	(1LU << 27)

#define BOOL_OPT_ISTRUE(s)  (!strcasecmp((s), true_vals[0]) || !strcasecmp((s), true_vals[1]) \
                             || !strcasecmp((s), true_vals[2]) || !strcasecmp((s), true_vals[3]))
#define BOOL_OPT_ISFALSE(s) (!strcasecmp((s), false_vals[0]) || !strcasecmp((s), false_vals[1]) \
                             || !strcasecmp((s), false_vals[2]) || !strcasecmp((s), false_vals[3]))

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

/* Max length of a line in the config file */
#define CONFIG_BUFF 20480

/* The context identifier.  This tells us what section of the config file
   we're in, for syntax checking purposes and the like.            -- mej */

enum {
  CTX_NULL,
  CTX_MAIN,
  CTX_COLOR,
  CTX_ATTRIBUTES,
  CTX_TOGGLES,
  CTX_KEYBOARD,
  CTX_MISC,
  CTX_IMAGECLASSES,
  CTX_IMAGE,
  CTX_ACTIONS,
  CTX_MENU,
  CTX_MENUITEM,
  CTX_XIM,
  CTX_MULTI_CHARSET,
  CTX_MAX = CTX_MULTI_CHARSET,
  CTX_UNDEF
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
#define ctx_poke(c) (id_stack[cur_ctx] = (c))
#define ctx_peek_last() (id_stack[(cur_ctx?cur_ctx-1:0)])
#define ctx_get_depth() (cur_ctx)
#define MAX_FILE_DEPTH 10
#define FILE_SKIP_TO_END	(0x01)
#define FILE_PREPROC		(0x02)
#define file_push(fs) do { \
                        cur_file++; \
                        file_stack[cur_file].fp = (fs).fp; \
                        file_stack[cur_file].path = (fs).path; \
                        file_stack[cur_file].line = (fs).line; \
			file_stack[cur_file].flags = (fs).flags; \
                      } while (0)

#define file_pop()    (cur_file--)
#define file_peek(fs) do { \
                        (fs).fp = file_stack[cur_file].fp; \
                        (fs).path = file_stack[cur_file].path; \
                        (fs).line = file_stack[cur_file].line; \
			(fs).flags = file_stack[cur_file].flags; \
                      } while (0)
#define file_peek_fp()      (file_stack[cur_file].fp)
#define file_peek_path()    (file_stack[cur_file].path)
#define file_peek_outfile() (file_stack[cur_file].outfile)
#define file_peek_line()    (file_stack[cur_file].line)
#define file_peek_skip()    (file_stack[cur_file].flags & FILE_SKIP_TO_END)
#define file_peek_preproc() (file_stack[cur_file].flags & FILE_PREPROC)

#define file_poke_fp(f)      ((file_stack[cur_file].fp) = (f))
#define file_poke_path(p)    ((file_stack[cur_file].path) = (p))
#define file_poke_outfile(p) ((file_stack[cur_file].outfile) = (p))
#define file_poke_line(l)    ((file_stack[cur_file].line) = (l))
#define file_poke_skip(s)    do {if (s) {file_stack[cur_file].flags |= FILE_SKIP_TO_END;} else {file_stack[cur_file].flags &= ~(FILE_SKIP_TO_END);} } while (0)
#define file_poke_preproc(s) do {if (s) {file_stack[cur_file].flags |= FILE_PREPROC;} else {file_stack[cur_file].flags &= ~(FILE_PREPROC);} } while (0)

#define file_inc_line()     (file_stack[cur_file].line++)

#define to_keysym(p,s) do { KeySym sym; \
                            if (s && ((sym = XStringToKeysym(s)) != 0)) *p = sym; \
                           } while (0)

#define RESET_AND_ASSIGN(var, val)  do {if ((var) != NULL) FREE(var);  (var) = (val);} while (0)

/************ Structures ************/
/* The file state stack.  This keeps track of the file currently being
   parsed.  This allows for %include directives.                  -- mej */
typedef struct file_state_struct {
  FILE *fp;
  char *path, *outfile;
  unsigned long line;
  unsigned char flags;
} file_state;
typedef char *(*eterm_function_ptr) (char *);
typedef struct eterm_function_struct {

  char *name;
  eterm_function_ptr ptr;
  int params;

} eterm_func;

/************ Variables ************/
extern unsigned long Options;
extern char *theme_dir, *user_dir;
extern       char **rs_execArgs;        /* Args to exec (-e or --exec) */
extern       char  *rs_title;		/* Window title */
extern       char  *rs_iconName;	/* Icon name */
extern       char  *rs_geometry;	/* Geometry string */
extern        int   rs_desktop;         /* Startup desktop */
extern        int   rs_saveLines;	/* Lines in the scrollback buffer */
extern unsigned short rs_min_anchor_size; /* Minimum size, in pixels, of the scrollbar anchor */
extern const char  *rs_scrollBar_right;
extern const char  *rs_scrollBar_floating;
extern const char  *rs_scrollbar_popup;
extern       char  *rs_viewport_mode;
extern       char  *rs_term_name;
extern const char  *rs_menubar;
extern const char  *rs_menu;
extern const char  *rs_menubar_move;
extern const char  *rs_pause;
extern       char  *rs_icon;
extern       char  *rs_scrollbar_type;
extern unsigned long rs_scrollbar_width;
extern       char  *rs_scrollbar_type;
extern       char  *rs_anim_pixmap_list;
extern       char **rs_anim_pixmaps;
extern     time_t   rs_anim_delay;
extern char *rs_path;
extern const char *rs_saveUnder;
extern char *rs_noCursor;
#ifdef USE_XIM
extern char *rs_inputMethod;
extern char *rs_preeditType;
#endif
extern char *rs_name;
extern char *rs_pixmapScale;
extern char *rs_config_file;
extern unsigned int rs_line_space;
#ifndef NO_BOLDFONT
extern const char *rs_boldFont;
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

unsigned long NumWords(const char *str);
extern void get_initial_options(int, char **);
extern void get_options(int, char **);
extern char *chomp(char *);
extern char *shell_expand(char *);
extern char *find_theme(char *, char *, char *);
extern unsigned char open_config_file(char *);
extern void read_config(char *);
extern void init_defaults(void);
extern void post_parse(void);
unsigned char save_config(char *);

_XFUNCPROTOEND

#endif	/* _OPTIONS_H_ */
