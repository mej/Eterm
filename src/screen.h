/*--------------------------------*-C-*---------------------------------*
 * File:	screen.h
 *
 * This module is all new by Robert Nation
 * <nation@rocket.sanders.lockheed.com>
 *
 * Additional modifications by mj olesen <olesen@me.QueensU.CA>
 * No additional restrictions are applied.
 *
 * As usual, the author accepts no responsibility for anything, nor does
 * he guarantee anything whatsoever.
 *----------------------------------------------------------------------*/
#ifndef _SCREEN_H
#define _SCREEN_H

#include <X11/Xfuncproto.h>
#include "main.h"

/************ Macros and Definitions ************/
#define WRAP_CHAR		(MAX_COLS + 1)
#define PROP_SIZE       	4096
#define TABSIZE         	8	/* default tab size */

#define ZERO_SCROLLBACK do { \
                          D_SCREEN(("ZERO_SCROLLBACK()\n")); \
                          if (Options & Opt_homeOnEcho) TermWin.view_start = 0; \
                        } while (0)
#define REFRESH_ZERO_SCROLLBACK do { \
                                  D_SCREEN(("REFRESH_ZERO_SCROLLBACK()\n")); \
                                  if (Options & Opt_homeOnRefresh) TermWin.view_start = 0; \
                                } while (0)
#define CHECK_SELECTION	do { \
                          if (selection.op) selection_check(); \
                        } while (0)

/*
 * CLEAR_ROWS : clear <num> rows starting from row <row>
 * CLEAR_CHARS: clear <num> chars starting from pixel position <x,y>
 * ERASE_ROWS : set <num> rows starting from row <row> to the foreground color
 */
#define drawBuffer	(TermWin.vt)
#define CLEAR_ROWS(row, num) do { \
                               XClearArea(Xdisplay, drawBuffer, Col2Pixel(0), Row2Pixel(row), \
                                          TermWin.width, Height2Pixel(num), 0); \
                             } while (0)
#define CLEAR_CHARS(x, y, num) do { \
                                 D_SCREEN(("CLEAR_CHARS(%d, %d, %d)\n", x, y, num)); \
                                 XClearArea(Xdisplay, drawBuffer, x, y, Width2Pixel(num), Height2Pixel(1), 0); \
                               } while (0)
#define FAST_CLEAR_CHARS(x, y, num) do { \
                                      clear_area(Xdisplay, drawBuffer, x, y, Width2Pixel(num), Height2Pixel(1), 0); \
                                    } while (0)
#define ERASE_ROWS(row, num) do { \
                               XFillRectangle(Xdisplay, drawBuffer, TermWin.gc, Col2Pixel(0), Row2Pixel(row), \
                                              TermWin.width, Height2Pixel(num)); \
                             } while (0)

/* Screen refresh methods */
#define NO_REFRESH              0       /* Window not visible at all!        */
#define FAST_REFRESH            (1<<1)  /* Fully exposed window              */
#define SLOW_REFRESH            (1<<2)  /* Partially exposed window          */
#define SMOOTH_REFRESH          (1<<3)  /* Do sync'ing to make it smooth     */

#define IGNORE	0
#define SAVE	's'
#define RESTORE	'r'
#define REVERT IGNORE
#define INVOKE RESTORE

/* flags for scr_gotorc() */
#define C_RELATIVE		1	/* col movement is relative */
#define R_RELATIVE		2	/* row movement is relative */
#define RELATIVE		(R_RELATIVE|C_RELATIVE)

/* modes for scr_insdel_chars(), scr_insdel_lines() */
#define INSERT			-1	/* don't change these values */
#define DELETE			+1
#define ERASE			+2

/* modes for scr_page() - scroll page. used by scrollbar window */
enum {
    UP,
    DN,
    NO_DIR
};

/* arguments for scr_change_screen() */
enum {
    PRIMARY,
    SECONDARY
};

#define RS_None			0		/* Normal */
#define RS_Cursor		0x01000000u	/* cursor location */
#define RS_Select		0x02000000u	/* selected text */
#define RS_RVid			0x04000000u	/* reverse video */
#define RS_Uline		0x08000000u	/* underline */
#define RS_acsFont		0x10000000u	/* ACS graphics character set */
#define RS_ukFont		0x20000000u	/* UK character set */
#define RS_fontMask		(RS_acsFont|RS_ukFont)
#ifdef MULTI_CHARSET
#define RS_multi0		0x40000000u	/* only multibyte characters */
#define RS_multi1		0x80000000u	/* multibyte 1st byte */
#define RS_multi2		(RS_multi0|RS_multi1)	/* multibyte 2nd byte */
#define RS_multiMask		(RS_multi0|RS_multi1)	/* multibyte mask */
#endif
#define RS_fgMask		0x00001F00u	/* 32 colors */
#define RS_Bold			0x00008000u	/* bold */
#define RS_bgMask		0x001F0000u	/* 32 colors */
#define RS_Blink		0x00800000u	/* blink */
#define RS_Dirty		0x00400000u	/* forced update of char */

#define RS_attrMask		(0xFF000000u|RS_Bold|RS_Blink)

/* how to build & extract colors and attributes */
#define GET_FGCOLOR(r)	(((r) & RS_fgMask)>>8)
#define GET_BGCOLOR(r)	(((r) & RS_bgMask)>>16)
#define GET_ATTR(r)	(((r) & RS_attrMask))
#define GET_BGATTR(r)	(((r) & (RS_attrMask | RS_bgMask)))

#define SET_FGCOLOR(r,fg)	(((r) & ~RS_fgMask)  | ((fg)<<8))
#define SET_BGCOLOR(r,bg)	(((r) & ~RS_bgMask)  | ((bg)<<16))
#define SET_ATTR(r,a)		(((r) & ~RS_attrMask)| (a))
#define DEFAULT_RSTYLE		(RS_None | (fgColor<<8) | (bgColor<<16))

/* screen_t flags */
#define Screen_Relative		(1<<0)	/* relative origin mode flag         */
#define Screen_VisibleCursor	(1<<1)	/* cursor visible?                   */
#define Screen_Autowrap		(1<<2)	/* auto-wrap flag                    */
#define Screen_Insert		(1<<3)	/* insert mode (vs. overstrike)      */
#define Screen_WrapNext		(1<<4)	/* need to wrap for next char?       */
#define Screen_DefaultFlags	(Screen_VisibleCursor|Screen_Autowrap)

/************ Structures ************/
typedef unsigned char text_t;
typedef unsigned int rend_t;
typedef struct {
    int             row, col;
} row_col_t;
/*
 * screen accounting:
 * screen_t elements
 *   text:      Contains all the text information including the scrollback
 *              buffer.  Each line is length (TermWin.ncol + 1)
 *              The final character is either the _length_ of the line or
 *              for wrapped lines: (MAX_COLS + 1) 
 *   rend:      Contains rendition information: font, bold, color, etc.
 * * Note: Each line for both text and rend are only allocated on demand, and
 *              text[x] is allocated <=> rend[x] is allocated  for all x.
 *   row:       Cursor row position                   : 0 <= row < TermWin.nrow
 *   col:       Cursor column position                : 0 <= col < TermWin.ncol
 *   tscroll:   Scrolling region top row inclusive    : 0 <= row < TermWin.nrow
 *   bscroll:   Scrolling region bottom row inclusive : 0 <= row < TermWin.nrow
 *
 * selection_t elements
 *   clicks:    1, 2 or 3 clicks - 4 indicates a special condition of 1 where
 *              nothing is selected
 *   beg:       row/column of beginning of selection  : never past mark
 *   mark:      row/column of initial click           : never past end
 *   end:       row/column of end of selection
 * * Note: -TermWin.nscrolled <= beg.row <= mark.row <= end.row < TermWin.nrow
 * * Note: col == -1 ==> we're left of screen
 *
 * TermWin.saveLines:
 *              Maximum number of lines in the scrollback buffer.
 *              This is fixed for each rxvt instance.
 * TermWin.nscrolled:
 *              Actual number of lines we've used of the scrollback buffer
 *              0 <= TermWin.nscrolled <= TermWin.saveLines
 * TermWin.view_start:  
 *              Offset back into the scrollback buffer for out current view
 *              0 <= TermWin.view_start <= TermWin.nscrolled
 *
 * Layout of text/rend information in the screen_t text/rend structures:
 *   Rows [0] ... [TermWin.saveLines - 1]
 *     scrollback region : we're only here if TermWin.view_start != 0
 *   Rows [TermWin.saveLines] ... [TermWin.saveLines + TermWin.nrow - 1]
 *     normal `unscrolled' screen region
 */
typedef struct {
    text_t        **text;	/* _all_ the text                            */
    rend_t        **rend;	/* rendition, uses RS_ flags                 */
    short           row,	/* cursor row on the screen                  */
                    col;	/* cursor column on the screen               */
    short           tscroll,	/* top of settable scroll region             */
                    bscroll;	/* bottom of settable scroll region          */
    short           charset;	/* character set number [0..3]               */
    unsigned int    flags;
} screen_t;
typedef struct {
    short           row,	/* cursor row                                */
                    col,	/* cursor column                             */
                    charset;	/* character set number [0..3]               */
    char            charset_char;
    rend_t          rstyle;	/* rendition style                           */
} save_t;
typedef struct {
    unsigned char  *text;	/* selected text                             */
    int             len;	/* length of selected text                   */
    enum {
	SELECTION_CLEAR = 0,	/* nothing selected                          */
	SELECTION_INIT,		/* marked a point                            */
	SELECTION_BEGIN,	/* started a selection                       */
	SELECTION_CONT,		/* continued selection                       */
	SELECTION_DONE		/* selection put in CUT_BUFFER0              */
    } op;			/* current operation                         */
    short           screen;	/* screen being used                         */
    short           clicks;	/* number of clicks                          */
    row_col_t       beg, mark, end;
} selection_t;

/************ Variables ************/
#ifndef NO_BRIGHTCOLOR
extern unsigned int colorfgbg;
#endif

/************ Function Prototypes ************/
_XFUNCPROTOBEGIN

extern void blank_line(text_t *, rend_t *, int, rend_t);
extern void blank_dline(text_t *, rend_t *, int, rend_t);
extern void blank_sline(text_t *, rend_t *, int);
extern void make_screen_mem(text_t **, rend_t **, int);
extern void scr_reset(void);
extern void scr_release(void);
extern void scr_poweron(void);
extern void scr_cursor(int);
extern int scr_change_screen(int);
extern void scr_color(unsigned int, unsigned int);
extern void scr_rendition(int, int);
extern int scroll_text(int, int, int, int);
extern void scr_add_lines(const unsigned char *, int, int);
extern void scr_backspace(void);
extern void scr_tab(int);
extern void scr_gotorc(int, int, int);
extern void scr_index(int);
extern void scr_erase_line(int);
extern void scr_erase_screen(int);
extern void scr_E(void);
extern void scr_insdel_lines(int, int);
extern void scr_insdel_chars(int, int);
extern void scr_scroll_region(int, int);
extern void scr_cursor_visible(int);
extern void scr_autowrap(int);
extern void scr_relative_origin(int);
extern void scr_insert_mode(int);
extern void scr_set_tab(int);
extern void scr_rvideo_mode(int);
extern void scr_report_position(void);
extern void set_font_style(void);
extern void scr_charset_choose(int);
extern void scr_charset_set(int, unsigned int);
extern void set_multichar_encoding(const char *);
extern int scr_get_fgcolor(void);
extern int scr_get_bgcolor(void);
extern void scr_expose(int, int, int, int);
extern void scr_touch(void);
extern int scr_move_to(int, int);
extern int scr_page(int, int);
extern void scr_bell(void);
extern void scr_printscreen(int);
extern void scr_refresh(int);
extern void selection_check(void);
extern void PasteIt(unsigned char *, unsigned int);
extern void selection_paste(Window, unsigned, int);
extern void selection_request(Time, int, int);
extern void selection_reset(void);
extern void selection_clear(void);
extern void selection_setclr(int, int, int, int, int);
extern void selection_start(int, int);
extern void selection_start_colrow(int, int);
extern void selection_make(Time);
extern void selection_click(int, int, int);
extern void selection_delimit_word(int, int, row_col_t *, row_col_t *);
extern void selection_extend(int, int, int);
extern void selection_extend_colrow(int, int, int, int);
extern void selection_rotate(int, int);
extern void selection_send(XSelectionRequestEvent *);
extern void mouse_report(XButtonEvent *);
extern void mouse_tracking(int, int, int, int, int);
extern void debug_PasteIt(unsigned char *, int);
extern int debug_selection(void);
extern void debug_colors(void);

_XFUNCPROTOEND

#endif
