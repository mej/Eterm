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
/* includes */
#include <X11/Xfuncproto.h>
#include "main.h"

/* defines */
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
extern void privileges(int);

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
#ifdef KANJI
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

/* macros */
/* how to build & extract colors and attributes */
#define GET_FGCOLOR(r)	(((r) & RS_fgMask)>>8)
#define GET_BGCOLOR(r)	(((r) & RS_bgMask)>>16)
#define GET_ATTR(r)	(((r) & RS_attrMask))
#define GET_BGATTR(r)	(((r) & (RS_attrMask | RS_bgMask)))

#define SET_FGCOLOR(r,fg)	(((r) & ~RS_fgMask)  | ((fg)<<8))
#define SET_BGCOLOR(r,bg)	(((r) & ~RS_bgMask)  | ((bg)<<16))
#define SET_ATTR(r,a)		(((r) & ~RS_attrMask)| (a))
#define DEFAULT_RSTYLE		(RS_None | (fgColor<<8) | (bgColor<<16))

/* extern variables */
#ifndef NO_BRIGHTCOLOR
extern unsigned int colorfgbg;
#endif

/* types */
typedef unsigned char text_t;
typedef unsigned int rend_t;
typedef struct {
    int             row, col;
} row_col_t;

/* screen_t flags */
#define Screen_Relative		(1<<0)	/* relative origin mode flag         */
#define Screen_VisibleCursor	(1<<1)	/* cursor visible?                   */
#define Screen_Autowrap		(1<<2)	/* auto-wrap flag                    */
#define Screen_Insert		(1<<3)	/* insert mode (vs. overstrike)      */
#define Screen_WrapNext		(1<<4)	/* need to wrap for next char?       */
#define Screen_DefaultFlags	(Screen_VisibleCursor|Screen_Autowrap)

/*
 * screen accounting:
 * screen_t elements
 *   text:      Contains all the text information including the scrollback
 *              buffer.  Each line is length (TermWin.ncol + 1)
 *              The final character is either the _length_ of the line or
 *              for wrapped lines: (MAX_COLS + 1) 
 *   rend:      Contains rendition information: font, bold, colour, etc.
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
	SELECTION_DONE,		/* selection put in CUT_BUFFER0              */
    } op;			/* current operation                         */
    short           screen;	/* screen being used                         */
    short           clicks;	/* number of clicks                          */
    row_col_t       beg, mark, end;
} selection_t;

#ifdef USE_ACTIVE_TAGS
extern screen_t screen;
#endif

/* prototypes: */
_XFUNCPROTOBEGIN

void blank_dline(text_t * et, rend_t * er, int width, rend_t efs);
void blank_sline(text_t * et, rend_t * er, int width);
void make_screen_mem(text_t ** tp, rend_t ** rp, int row);
void scr_reset(void);
void scr_release(void);
void scr_poweron(void);
void scr_cursor(int mode);
int scr_change_screen(int scrn);
void scr_color(unsigned int color, unsigned int Intensity);
void scr_rendition(int set, int style);
int scroll_text(int row1, int row2, int count, int spec);
void scr_add_lines(const unsigned char *str, int nlines, int len);
void scr_backspace(void);
void scr_tab(int count);
void scr_gotorc(int row, int col, int relative);
void scr_index(int direction);
void scr_erase_line(int mode);
void scr_erase_screen(int mode);
void scr_E(void);
void scr_insdel_lines(int count, int insdel);
void scr_insdel_chars(int count, int insdel);
void scr_scroll_region(int top, int bot);
void scr_cursor_visible(int mode);
void scr_autowrap(int mode);
void scr_relative_origin(int mode);
void scr_insert_mode(int mode);
void scr_set_tab(int mode);
void scr_rvideo_mode(int mode);
void scr_report_position(void);
void set_font_style(void);
void scr_charset_choose(int set);
void scr_charset_set(int set, unsigned int ch);
void eucj2jis(unsigned char *str, int len);
void sjis2jis(unsigned char *str, int len);
void set_kanji_encoding(const char *str);
int scr_get_fgcolor(void);
int scr_get_bgcolor(void);
void scr_expose(int x, int y, int width, int height);
void scr_touch(void);
int scr_move_to(int y, int len);
int scr_page(int direction, int nlines);
void scr_bell(void);
void scr_printscreen(int fullhist);
void scr_refresh(int type);
void selection_check(void);
void PasteIt(unsigned char *data, unsigned int nitems);
void selection_paste(Window win, unsigned prop, int Delete);
void selection_request(Time tm, int x, int y);
void selection_reset(void);
void selection_clear(void);
void selection_setclr(int set, int startr, int startc, int endr, int endc);
void selection_start(int x, int y);
void selection_start_colrow(int col, int row);
void selection_make(Time tm);
void selection_click(int clicks, int x, int y);
void selection_delimit_word(int col, int row, row_col_t * beg, row_col_t * end);
void selection_extend(int x, int y, int flag);
void selection_extend_colrow(int col, int row, int flag, int cont);
void selection_rotate(int x, int y);
void selection_send(XSelectionRequestEvent * rq);
void mouse_report(XButtonEvent * ev);
void mouse_tracking(int report, int x, int y, int firstrow, int lastrow);
void debug_PasteIt(unsigned char *data, int nitems);
int debug_selection(void);
void debug_colors(void);

_XFUNCPROTOEND

#endif	/* whole file */
/*----------------------- end-of-file (C header) -----------------------*/
