/*--------------------------------*-C-*---------------------------------*
 * File:	command.h
 *
 * Copyright 1992 John Bovey, University of Kent at Canterbury.
 *
 * You can do what you like with this source code as long as you don't try
 * to make money out of it and you include an unaltered copy of this
 * message (including the copyright).
 *
 * This module has been heavily modified by R. Nation
 * <nation@rocket.sanders.lockheed.com>
 * No additional restrictions are applied.
 *
 * Additional modifications by mj olesen <olesen@me.QueensU.CA>
 * No additional restrictions are applied.
 *
 * As usual, the author accepts no responsibility for anything, nor does
 * he guarantee anything whatsoever.
 *----------------------------------------------------------------------*/
#ifndef _COMMAND_H_
# define _COMMAND_H_
# include <X11/X.h>
# include <X11/Xfuncproto.h>
# include <X11/Xproto.h>
# include <stdio.h>
# include <limits.h>

# ifdef USE_ACTIVE_TAGS
#  include "activetags.h"
# endif

# define menuBar_esc	10
# define scrollBar_esc	30

/* Motif window hints */
#define MWM_HINTS_FUNCTIONS           (1L << 0)
#define MWM_HINTS_DECORATIONS         (1L << 1)
#define MWM_HINTS_INPUT_MODE          (1L << 2)
#define MWM_HINTS_STATUS              (1L << 3)

/* bit definitions for MwmHints.functions */
#define MWM_FUNC_ALL            (1L << 0)
#define MWM_FUNC_RESIZE         (1L << 1)
#define MWM_FUNC_MOVE           (1L << 2)
#define MWM_FUNC_MINIMIZE       (1L << 3)
#define MWM_FUNC_MAXIMIZE       (1L << 4)
#define MWM_FUNC_CLOSE          (1L << 5)

/* bit definitions for MwmHints.decorations */
#define MWM_DECOR_ALL                 (1L << 0)
#define MWM_DECOR_BORDER              (1L << 1)
#define MWM_DECOR_RESIZEH             (1L << 2)
#define MWM_DECOR_TITLE               (1L << 3)
#define MWM_DECOR_MENU                (1L << 4)
#define MWM_DECOR_MINIMIZE            (1L << 5)
#define MWM_DECOR_MAXIMIZE            (1L << 6)

/* bit definitions for MwmHints.inputMode */
#define MWM_INPUT_MODELESS                  0
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL 1
#define MWM_INPUT_SYSTEM_MODAL              2
#define MWM_INPUT_FULL_APPLICATION_MODAL    3

#define PROP_MWM_HINTS_ELEMENTS             5

/* Motif window hints */
typedef struct _mwmhints {
  CARD32 flags;
  CARD32 functions;
  CARD32 decorations;
  INT32  input_mode;
  CARD32 status;
} MWMHints;

/* DEC private modes */
# define PrivMode_132		(1LU<<0)
# define PrivMode_132OK		(1LU<<1)
# define PrivMode_rVideo	(1LU<<2)
# define PrivMode_relOrigin	(1LU<<3)
# define PrivMode_Screen	(1LU<<4)
# define PrivMode_Autowrap	(1LU<<5)
# define PrivMode_aplCUR	(1LU<<6)
# define PrivMode_aplKP		(1LU<<7)
# define PrivMode_BackSpace	(1LU<<8)
# define PrivMode_ShiftKeys	(1LU<<9)
# define PrivMode_VisibleCursor	(1LU<<10)
# define PrivMode_MouseX10	(1LU<<11)
# define PrivMode_MouseX11	(1LU<<12)
/* too annoying to implement X11 highlight tracking */
/* #define PrivMode_MouseX11Track	(1LU<<13) */
# define PrivMode_scrollBar	(1LU<<14)
# define PrivMode_menuBar	(1LU<<15)

#define PrivMode_mouse_report	(PrivMode_MouseX10|PrivMode_MouseX11)
#define PrivMode(test,bit) do {\
if (test) PrivateModes |= (bit); else PrivateModes &= ~(bit);} while (0)

#define PrivMode_Default (PrivMode_Autowrap|PrivMode_ShiftKeys|PrivMode_VisibleCursor)

extern char initial_dir[PATH_MAX+1];
extern unsigned long PrivateModes;

_XFUNCPROTOBEGIN

# ifdef USE_ACTIVE_TAGS
pid_t cmd_pid;
int cmd_fd;
# endif
extern void init_command(char **);
extern void tt_resize(void);
extern void tt_write(const unsigned char *, unsigned int);
extern void tt_printf(const unsigned char *, ...);
extern unsigned int cmd_write(const unsigned char *, unsigned int);
extern void main_loop(void);
extern FILE *popen_printer(void);
extern int pclose_printer(FILE *);
extern void color_aliases (int idx);

_XFUNCPROTOEND

#endif	/* _COMMAND_H_ */
/*----------------------- end-of-file (C header) -----------------------*/
