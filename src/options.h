/*  options.h -- Eterm options module header file
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

#ifndef _OPTIONS_H_
#define _OPTIONS_H_
/* includes */
#include <X11/Xfuncproto.h>
#include <X11/Intrinsic.h>	/* Xlib, Xutil, Xresource, Xfuncproto */
#include "feature.h"

/* prototypes */
unsigned long NumWords(const char *str);

/* extern variables */
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
extern const char  *rs_term_name;
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

extern const char *true_vals[];
extern const char *false_vals[];
#define BOOL_OPT_ISTRUE(s)  (!strcasecmp((s), true_vals[0]) || !strcasecmp((s), true_vals[1]) \
                             || !strcasecmp((s), true_vals[2]) || !strcasecmp((s), true_vals[3]))
#define BOOL_OPT_ISFALSE(s) (!strcasecmp((s), false_vals[0]) || !strcasecmp((s), false_vals[1]) \
                             || !strcasecmp((s), false_vals[2]) || !strcasecmp((s), false_vals[3]))

#ifdef CUTCHAR_OPTION
extern       char  *rs_cutchars;
#endif

#ifdef KEYSYM_ATTRIBUTE
extern unsigned char *KeySym_map[256];
#endif

#if defined (HOTKEY_CTRL) || defined (HOTKEY_META)
extern KeySym ks_bigfont;
extern KeySym ks_smallfont;
#endif

#ifdef PIXMAP_SUPPORT
extern char *rs_pixmaps[];

#define pixmap_bg    0
#define pixmap_sb    1
#define pixmap_up    2
#define pixmap_upclk 3
#define pixmap_dn    4
#define pixmap_dnclk 5
#define pixmap_sa    6
#define pixmap_saclk 7
#define pixmap_mb    8
#define pixmap_ms    9
#endif	/* PIXMAP_SUPPORT */

/* prototypes */
_XFUNCPROTOBEGIN

extern void get_options(int, char **);
extern char *chomp(char *);
extern void read_config(void);

_XFUNCPROTOEND

#endif	/* _OPTIONS_H_ */
