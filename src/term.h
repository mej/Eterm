/* term.h -- Eterm terminal emulation module header file
 *
 * This file is original work by Michael Jennings <mej@eterm.org> and
 * Tuomo Venalainen <vendu@cc.hut.fi>.  This file, and any other file
 * bearing this same message or a similar one, is distributed under
 * the GNU Public License (GPL) as outlined in the COPYING file.
 *
 * Copyright (C) 1997-2000, Michael Jennings and Tuomo Venalainen
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
# define XTerm_logfile	46
# define XTerm_font	50

/* rxvt/Eterm extensions of XTerm escape sequences: ESC ] Ps;Pt BEL */
# define XTerm_Takeover     5     /* Steal keyboard focus and raise window */
# define XTerm_EtermSeq     6     /* Eterm proprietary escape sequences */
# define XTerm_EtermIPC     7     /* Eterm escape code/text IPC interface */
# define XTerm_Pixmap	   20     /* new bg pixmap */
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
# ifndef NO_BRIGHTCOLOR
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
# else
    WhiteColor = maxColor,
# endif
# ifndef NO_CURSORCOLOR
    cursorColor,
    cursorColor2,
# endif
    pointerColor,
    borderColor,
# ifndef NO_BOLDUNDERLINE
    colorBD,
    colorUL,
# endif
    menuTextColor,
    scrollColor,
    menuColor,
    unfocusedScrollColor,
    unfocusedMenuColor,
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

# define NSHADOWCOLORS		(TOTAL_COLORS - NRS_COLORS)

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

/************ Variables ************/
#ifdef META8_OPTION
extern unsigned char meta_char;	/* Alt-key prefix */
#endif
extern unsigned long PrivateModes;
extern unsigned long SavedModes;
extern char *def_colorName[];
extern char *rs_color[NRS_COLORS];
extern Pixel PixColors[NRS_COLORS + NSHADOWCOLORS];
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
extern void color_aliases(int);
#ifndef NO_BRIGHTCOLOR
extern void set_colorfgbg(void);
#else
# define set_colorfgbg() ((void)0)
#endif /* NO_BRIGHTCOLOR */
extern void xterm_seq(int, const char *);

_XFUNCPROTOEND

#endif	/* _TERM_H_ */
