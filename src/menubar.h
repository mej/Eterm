/*--------------------------------*-C-*---------------------------------*
 * File:	menubar.h
 *
 * Copyright 1996,97
 * mj olesen <olesen@me.QueensU.CA> Queen's Univ at Kingston
 *
 * You can do what you like with this source code provided you don't make
 * money from it and you include an unaltered copy of this message
 * (including the copyright).  As usual, the author accepts no
 * responsibility for anything, nor does he guarantee anything whatsoever.
 *----------------------------------------------------------------------*/
#ifndef _MENUBAR_H
# define _MENUBAR_H
# include <X11/Intrinsic.h>	/* Xlib, Xutil, Xresource, Xfuncproto */
# include <X11/Xfuncproto.h>

typedef struct {
    short           state;
    Window          win;
} menuBar_t;

typedef struct {
    short           type;	/* must not be changed; first element */
    short           len;	/* strlen (str) */
    unsigned char  *str;	/* action to take */
} action_t;

typedef struct {
    short           type;	/* must not be changed; first element */
    struct menu_t  *menu;	/* sub-menu */
} submenu_t;

typedef struct menuitem_t {
    struct menuitem_t *prev;	/* prev menu-item */
    struct menuitem_t *next;	/* next menu-item */
    char           *name;	/* character string displayed */
    char           *name2;	/* character string displayed (right) */
    short           len;	/* strlen (name) */
    short           len2;	/* strlen (name) */
    union {
	short           type;	/* must not be changed; first element */
	action_t        action;
	submenu_t       submenu;
    } entry;
} menuitem_t;

enum menuitem_t_action {
    MenuLabel,
    MenuAction,
    MenuTerminalAction,
    MenuSubMenu
};

typedef struct menu_t {
    struct menu_t  *parent;	/* parent menu */
    struct menu_t  *prev;	/* prev menu */
    struct menu_t  *next;	/* next menu */
    menuitem_t     *head;	/* double-linked list */
    menuitem_t     *tail;	/* double-linked list */
    menuitem_t     *item;	/* current item */
    char           *name;	/* menu name */
    short           len;	/* strlen (name) */
    short           width;	/* maximum menu width [chars] */
    Window          win;	/* window of the menu */
    short           x;		/* x location [pixels] (chars if parent == NULL) */
    short           y;		/* y location [pixels] */
    short           w, h;	/* window width, height [pixels] */
} menu_t;

typedef struct bar_t {
    menu_t         *head, *tail;	/* double-linked list of menus */
    char           *title;	/* title to put in the empty menuBar */
# if (MENUBAR_MAX > 1)
#  define MAXNAME 16
    char            name[MAXNAME];	/* name to use to refer to menubar */
    struct bar_t   *next, *prev;	/* circular linked-list */
# endif				/* (MENUBAR_MAX > 1) */
# define NARROWS	4
    action_t        arrows[NARROWS];
} bar_t;

extern menuBar_t	menuBar;
extern int delay_menu_drawing;

# define menuBar_margin		2		/* margin below text */
# ifdef PIXMAP_MENUBAR
#  define menubar_is_pixmapped() (0)
# else
#  define menubar_is_pixmapped() (0)
# endif
/* macros */
#define menubar_visible()	(menuBar.state)
#define menuBar_height()	(TermWin.fheight + SHADOW)
#define menuBar_TotalHeight()	(menuBar_height() + SHADOW + menuBar_margin)
#define menuBar_TotalWidth()	(2 * TermWin.internalBorder + TermWin.width)
#define isMenuBarWindow(w)	((w) == menuBar.win)

_XFUNCPROTOBEGIN

extern void
menubar_control (XButtonEvent * /* ev */);

extern void
menubar_dispatch (char * /* str */);

extern void
menubar_expose (void);

extern int
menubar_mapping (int /* map */);

_XFUNCPROTOEND

#if !(MENUBAR_MAX)
# define menubar_dispatch(str)	((void)0)
# define menubar_expose()	((void)0)
# define menubar_control(ev)	((void)0)
# define menubar_mapping(map)	(0)
# undef menubar_visible
# define menubar_visible()	(0)
# undef isMenuBarWindow
# define isMenuBarWindow(w)	(0)
#endif

#endif	/* _MENUBAR_H */
/*----------------------- end-of-file (C header) -----------------------*/
