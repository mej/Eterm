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

#ifndef _TERM_H_
#define _TERM_H_

#include <stdio.h>
#include <X11/Xfuncproto.h>
#include <X11/Intrinsic.h>	/* Xlib, Xutil, Xresource, Xfuncproto */

/************ Macros and Definitions ************/
/* Macros to make parsing escape sequences slightly more readable.... <G> */
#define OPT_SET_OR_TOGGLE(s, mask, bit) do { \
        if (!(s) || !(*(s))) { \
	  if ((mask) & (bit)) { \
	    (mask) &= ~(bit); \
          } else { \
	    (mask) |= (bit); \
          } \
        } else if (BOOL_OPT_ISTRUE(s)) { \
	  if ((mask) & (bit)) return; \
	  (mask) |= (bit); \
	} else if (BOOL_OPT_ISFALSE(s)) { \
	  if (!((mask) & (bit))) return; \
	  (mask) &= ~(bit); \
	} \
      } while (0)
/* The macro below forces bit to the opposite state from what we want, so that the
   code that follows will set it right.  Hackish, but saves space. :)  Use this
   if you need to do some processing other than just setting the flag right. */
#define OPT_SET_OR_TOGGLE_NEG(s, mask, bit) do { if (s) { \
	if (BOOL_OPT_ISTRUE(s)) { \
	  if ((mask) & (bit)) return; \
	  (mask) &= ~(bit); \
	} else if (BOOL_OPT_ISFALSE(s)) { \
	  if (!((mask) & (bit))) return; \
	  (mask) |= (bit); \
	} \
      } } while (0)

/* XTerm escape sequences: ESC ] Ps;Pt BEL */
# define XTerm_name	0
# define XTerm_iconName	1
# define XTerm_title	2
# define XTerm_prop	3
# define XTerm_ccolor	12
# define XTerm_logfile	46
# define XTerm_font	50

/* rxvt/Eterm extensions of XTerm escape sequences: ESC ] Ps;Pt BEL */
# define XTerm_Takeover     5     /* Steal keyboard focus and raise window */
# define XTerm_EtermSeq     6     /* Eterm proprietary escape sequences */
# define XTerm_Pixmap	   20     /* new bg pixmap */
# define XTerm_DumpScreen  30     /* Dump contents of scrollback to a file */
# define XTerm_restoreFG   39     /* change default fg color */
# define XTerm_restoreBG   49     /* change default bg color */

# define restoreFG	39	/* restore default fg color */
# define restoreBG	49	/* restore default bg color */

enum color_list {
    fgColor,
    bgColor,
    minColor,				/* 2 */
    BlackColor = minColor,
    Red3Color,
    Green3Color,
    Yellow3Color,
    Blue3Color,
    Magenta3Color,
    Cyan3Color,
    maxColor,				/* minColor + 7 */
# ifdef NO_BRIGHTCOLOR
    WhiteColor = maxColor,
    maxBright = maxColor,
# else
    AntiqueWhiteColor = maxColor,
    minBright,				/* maxColor + 1 */
    Grey25Color = minBright,
    RedColor,
    GreenColor,
    YellowColor,
    BlueColor,
    MagentaColor,
    CyanColor,
    maxBright,				/* minBright + 7 */
    WhiteColor = maxBright,
# endif
# ifndef NO_CURSORCOLOR
    cursorColor,
    cursorColor2,
# endif
# ifndef NO_BOLDUNDERLINE
    colorBD,
    colorUL,
# endif
#ifdef ESCREEN
    ES_COLOR_CURRENT,
    ES_COLOR_ACTIVE,
#endif
    pointerColor,
    borderColor,
    NRS_COLORS,				/* */
    topShadowColor = NRS_COLORS,
    bottomShadowColor,
    unfocusedTopShadowColor,
    unfocusedBottomShadowColor,
    menuTopShadowColor,
    menuBottomShadowColor,
    unfocusedMenuTopShadowColor,
    unfocusedMenuBottomShadowColor,
    TOTAL_COLORS			/* */
};

# define EXTRA_COLORS		(TOTAL_COLORS - NRS_COLORS)

#ifdef HOTKEY_CTRL
# define HOTKEY ctrl
#elif defined(HOTKEY_META)
# define HOTKEY meta
#endif

#define PrivCases(bit)	do {if (mode == 't') state = !(PrivateModes & bit); else state = mode; \
                            switch (state) { \
                              case 's': SavedModes |= (PrivateModes & bit); continue; break; \
                              case 'r': state = (SavedModes & bit) ? 1 : 0; \
                              default:  PrivMode(state, bit); break; \
                            }} while (0)

#define COLOR_NAME(c)   ((rs_color[c]) ? (rs_color[c]) : (def_colorName[c]))

/************ Variables ************/
#ifdef META8_OPTION
extern unsigned char meta_char;	/* Alt-key prefix */
#endif
extern unsigned long PrivateModes;
extern unsigned long SavedModes;
extern char *def_colorName[];
extern char *rs_color[NRS_COLORS];
extern Pixel PixColors[NRS_COLORS + EXTRA_COLORS];
extern unsigned int MetaMask, AltMask, NumLockMask;
extern unsigned int modmasks[];

/************ Function Prototypes ************/
_XFUNCPROTOBEGIN

extern void get_modifiers(void);
extern void lookup_key(XEvent *);
#ifdef PRINTPIPE
extern FILE *popen_printer(void);
extern int pclose_printer(FILE *);
extern void process_print_pipe(void);
#endif
extern void process_escape_seq(void);
extern void process_csi_seq(void);
extern void process_xterm_seq(void);
extern void process_window_mode(unsigned int, int []);
extern void process_terminal_mode(int, int, unsigned int, int []);
extern void process_sgr_mode(unsigned int, int []);
#ifndef NO_BRIGHTCOLOR
extern void set_colorfgbg(void);
#else
# define set_colorfgbg() ((void)0)
#endif /* NO_BRIGHTCOLOR */
extern void set_title(const char *);
extern void set_icon_name(const char *);
extern void append_to_title(const char *);
extern void append_to_icon_name(const char *);
extern void xterm_seq(int, const char *);

_XFUNCPROTOEND

#endif	/* _TERM_H_ */
