/*
 * File:      screen.c
 *
 */

static const char cvs_ident[] = "$Id$";

#include "config.h"
#include "feature.h"

/* includes */
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#include <X11/Xatom.h>
#include <X11/Xmd.h>		/* CARD32 */

#include "../libmej/debug.h"
#include "command.h"
#include "debug.h"
#include "startup.h"
#include "mem.h"
#include "screen.h"
#include "scrollbar.h"
#include "options.h"
#ifdef PIXMAP_SUPPORT
# include "pixmap.h"
#endif
#ifdef PROFILE_SCREEN
# include "profile.h"
#endif
#include "term.h"

/* This tells what's actually on the screen */
static text_t **drawn_text = NULL;
static rend_t **drawn_rend = NULL;

static text_t **buf_text = NULL;
static rend_t **buf_rend = NULL;

static char *tabs = NULL;	/* a 1 for a location with a tab-stop */

static screen_t screen =
{
  NULL, NULL, 0, 0, 0, 0, 0, Screen_DefaultFlags
};

static screen_t swap =
{
  NULL, NULL, 0, 0, 0, 0, 0, Screen_DefaultFlags
};

static save_t save =
{
  0, 0, 0, 'B', DEFAULT_RSTYLE
};

static selection_t selection =
{
  NULL, 0, SELECTION_CLEAR, PRIMARY, 0,
  {0, 0},
  {0, 0},
  {0, 0}
};

static char charsets[4] =
{
  'B', 'B', 'B', 'B'		/* all ascii */
};

static short current_screen = PRIMARY;
static rend_t rstyle = DEFAULT_RSTYLE;
static short rvideo = 0;	/* reverse video */
int prev_nrow = -1, prev_ncol = -1;	/* moved from scr_reset() to squash seg fault in scroll_text() */

#ifdef MULTI_CHARSET
static short multi_byte = 0;
static enum {
  EUCJ, EUCKR = EUCJ, GB = EUCJ, SJIS, BIG5
} encoding_method = EUCJ;
static enum {
  SBYTE, WBYTE
} chstat = SBYTE;
static short lost_multi = 0;
unsigned char refresh_all = 0;

#define RESET_CHSTAT	if (chstat == WBYTE) chstat = SBYTE, lost_multi = 1
#else
#define RESET_CHSTAT
#endif

/* ------------------------------------------------------------------------- *
 *                        SCREEN `COMMON' ROUTINES                           *
 * ------------------------------------------------------------------------- */
/* Fill part/all of a drawn line with blanks. */
inline void blank_line(text_t *, rend_t *, int, rend_t);

inline void
blank_line(text_t * et, rend_t * er, int width, rend_t efs)
{
/*    int             i = width; */
  register unsigned int i = width;
  rend_t *r = er, fs = efs;

  MEMSET(et, ' ', i);
  for (; i--;)
    *r++ = fs;
}

inline void blank_screen_mem(text_t **, rend_t **, int, rend_t);

inline void
blank_screen_mem(text_t **tp, rend_t **rp, int row, rend_t efs)
{
  register unsigned int i = TermWin.ncol;
  rend_t *r, fs = efs;

  if (tp[row] == NULL) {
    tp[row] = MALLOC(sizeof(text_t) * (TermWin.ncol + 1));
    rp[row] = MALLOC(sizeof(rend_t) * TermWin.ncol);
  }
  MEMSET(tp[row], ' ', i);
  tp[row][i] = 0;
  for (r = rp[row]; i--;)
    *r++ = fs;
}

/* ------------------------------------------------------------------------- */
/* allocate memory for this screen line */
void
make_screen_mem(text_t ** tp, rend_t ** rp, int row)
{
  tp[row] = MALLOC(sizeof(text_t) * (TermWin.ncol + 1));
  rp[row] = MALLOC(sizeof(rend_t) * TermWin.ncol);
}

/* ------------------------------------------------------------------------- *
 *                          SCREEN INITIALISATION                            *
 * ------------------------------------------------------------------------- */
void
scr_reset(void)
{
/*    int             i, j, k, total_rows, prev_total_rows; */
  int total_rows, prev_total_rows;
  register int i, j, k;
  text_t tc;

  D_SCREEN(("scr_reset()\n"));

  TermWin.view_start = 0;
  RESET_CHSTAT;

  if (TermWin.ncol == prev_ncol && TermWin.nrow == prev_nrow)
    return;

  if (TermWin.ncol <= 0)
    TermWin.ncol = 80;
  if (TermWin.nrow <= 0)
    TermWin.nrow = 24;
  if (TermWin.saveLines <= 0)
    TermWin.saveLines = 0;

  total_rows = TermWin.nrow + TermWin.saveLines;
  prev_total_rows = prev_nrow + TermWin.saveLines;

  screen.tscroll = 0;
  screen.bscroll = (TermWin.nrow - 1);
  if (prev_nrow == -1) {
    /*
     * A: first time called so just malloc everything : don't rely on realloc
     *    Note: this is still needed so that all the scrollback lines are NULL
     */
    screen.text = CALLOC(text_t *, total_rows);
    buf_text    = CALLOC(text_t *, total_rows);
    drawn_text  = CALLOC(text_t *, TermWin.nrow);
    swap.text   = CALLOC(text_t *, TermWin.nrow);

    screen.rend = CALLOC(rend_t *, total_rows);
    buf_rend    = CALLOC(rend_t *, total_rows);
    drawn_rend  = CALLOC(rend_t *, TermWin.nrow);
    swap.rend   = CALLOC(rend_t *, TermWin.nrow);

    for (i = 0; i < TermWin.nrow; i++) {
      j = i + TermWin.saveLines;
      blank_screen_mem(screen.text, screen.rend, j, DEFAULT_RSTYLE);
      blank_screen_mem(swap.text, swap.rend, i, DEFAULT_RSTYLE);
      blank_screen_mem(drawn_text, drawn_rend, i, DEFAULT_RSTYLE);
    }
    TermWin.nscrolled = 0;	/* no saved lines */

  } else {
    /*
 * B1: add or delete rows as appropriate
 */
    if (TermWin.nrow < prev_nrow) {
      /* delete rows */
      k = MIN(TermWin.nscrolled, prev_nrow - TermWin.nrow);
      scroll_text(0, prev_nrow - 1, k, 1);
	    
      for (i = TermWin.nrow; i < prev_nrow; i++) {
        j = i + TermWin.saveLines;
        if (screen.text[j])
          FREE(screen.text[j]);
        if (screen.rend[j])
          FREE(screen.rend[j]);
        if (swap.text[i])
          FREE(swap.text[i]);
        if (swap.rend[i])
          FREE(swap.rend[i]);
        if (drawn_text[i])
          FREE(drawn_text[i]);
        if (drawn_rend[i])
          FREE(drawn_rend[i]);
      }
      screen.text = REALLOC(screen.text, total_rows   * sizeof(text_t*));
      buf_text =    REALLOC(buf_text   , total_rows   * sizeof(text_t*));
      drawn_text =  REALLOC(drawn_text , TermWin.nrow * sizeof(text_t*));
      swap.text =   REALLOC(swap.text  , TermWin.nrow * sizeof(text_t*));

      screen.rend = REALLOC(screen.rend, total_rows   * sizeof(rend_t*));
      buf_rend =    REALLOC(buf_rend   , total_rows   * sizeof(rend_t*));
      drawn_rend =  REALLOC(drawn_rend , TermWin.nrow * sizeof(rend_t*));
      swap.rend =   REALLOC(swap.rend  , TermWin.nrow * sizeof(rend_t*));

      /* we have fewer rows so fix up number of scrolled lines */
      MIN_IT(screen.row, TermWin.nrow - 1);

    } else if (TermWin.nrow > prev_nrow) {
      /* add rows */
      screen.text = REALLOC(screen.text, total_rows   * sizeof(text_t*));
      buf_text =    REALLOC(buf_text   , total_rows   * sizeof(text_t*));
      drawn_text =  REALLOC(drawn_text , TermWin.nrow * sizeof(text_t*));
      swap.text =   REALLOC(swap.text  , TermWin.nrow * sizeof(text_t*));
                          
      screen.rend = REALLOC(screen.rend, total_rows   * sizeof(rend_t*));
      buf_rend =    REALLOC(buf_rend   , total_rows   * sizeof(rend_t*));
      drawn_rend =  REALLOC(drawn_rend , TermWin.nrow * sizeof(rend_t*));
      swap.rend =   REALLOC(swap.rend  , TermWin.nrow * sizeof(rend_t*));

      k = MIN(TermWin.nscrolled, TermWin.nrow - prev_nrow);
      for (i = prev_total_rows; i < total_rows - k; i++) {
        screen.text[i] = NULL;
        blank_screen_mem(screen.text, screen.rend, i, DEFAULT_RSTYLE);
      }
      for ( /* i = total_rows - k */ ; i < total_rows; i++) {
        screen.text[i] = NULL;
        screen.rend[i] = NULL;
      }
      for (i = prev_nrow; i < TermWin.nrow; i++) {
        swap.text[i] = NULL;
        drawn_text[i] = NULL;
        blank_screen_mem(swap.text, swap.rend, i, DEFAULT_RSTYLE);
        blank_screen_mem(drawn_text, drawn_rend, i, DEFAULT_RSTYLE);
      }
      if (k > 0) {
        scroll_text(0, TermWin.nrow - 1, -k, 1);
        screen.row += k;
        TermWin.nscrolled -= k;
        for (i = TermWin.saveLines - TermWin.nscrolled; k--; i--) {
          if (screen.text[i] == NULL) {
            blank_screen_mem(screen.text, screen.rend, i, DEFAULT_RSTYLE);
          }
        }
      }
    }
    /* B2: resize columns */
    if (TermWin.ncol != prev_ncol) {
      for (i = 0; i < total_rows; i++) {
        if (screen.text[i]) {
          tc = screen.text[i][prev_ncol];
          screen.text[i] = REALLOC(screen.text[i],
                                   (TermWin.ncol+1)*sizeof(text_t));
          screen.rend[i] = REALLOC(screen.rend[i],
                                   TermWin.ncol*sizeof(rend_t));
          screen.text[i][TermWin.ncol] = min(tc,TermWin.ncol);
          if (TermWin.ncol > prev_ncol)
            blank_line(&(screen.text[i][prev_ncol]),
                       &(screen.rend[i][prev_ncol]),
                       TermWin.ncol - prev_ncol, DEFAULT_RSTYLE);
        }
      }
      for (i = 0; i < TermWin.nrow; i++) {
        drawn_text[i] = REALLOC(drawn_text[i],
                                TermWin.ncol*sizeof(text_t));
        drawn_rend[i] = REALLOC(drawn_rend[i],
                                TermWin.ncol*sizeof(rend_t));
        if (swap.text[i]) {
          tc = swap.text[i][prev_ncol];
          swap.text[i] = REALLOC(swap.text[i],
                                 (TermWin.ncol+1)*sizeof(text_t));
          swap.rend[i] = REALLOC(swap.rend[i],
                                 TermWin.ncol*sizeof(rend_t));
          swap.text[i][TermWin.ncol] = min(tc,TermWin.ncol);
          if (TermWin.ncol > prev_ncol)
            blank_line(&(swap.text[i][prev_ncol]),
                       &(swap.rend[i][prev_ncol]),
                       TermWin.ncol - prev_ncol, DEFAULT_RSTYLE);
        }
        if (TermWin.ncol > prev_ncol)
          blank_line(&(drawn_text[i][prev_ncol]),
                     &(drawn_rend[i][prev_ncol]),
                     TermWin.ncol - prev_ncol, DEFAULT_RSTYLE);
      }
    }
    if (tabs)
      FREE(tabs);
  }
  tabs = MALLOC(TermWin.ncol * sizeof(char));

  for (i = 0; i < TermWin.ncol; i++)
    tabs[i] = (i % TABSIZE == 0) ? 1 : 0;

  prev_nrow = TermWin.nrow;
  prev_ncol = TermWin.ncol;

  tt_resize();
}

/* ------------------------------------------------------------------------- */
/*
 * Free everything.  That way malloc debugging can find leakage.
 */
void
scr_release(void)
{
/*    int             i, total_rows; */
  int total_rows;
  register int i;

  total_rows = TermWin.nrow + TermWin.saveLines;
  for (i = 0; i < total_rows; i++) {
    if (screen.text[i]) {	/* then so is screen.rend[i] */
      FREE(screen.text[i]);
      FREE(screen.rend[i]);
    }
  }
  for (i = 0; i < TermWin.nrow; i++) {
    FREE(drawn_text[i]);
    FREE(drawn_rend[i]);
    FREE(swap.text[i]);
    FREE(swap.rend[i]);
  }
  FREE(screen.text);
  FREE(screen.rend);
  FREE(drawn_text);
  FREE(drawn_rend);
  FREE(swap.text);
  FREE(swap.rend);
  FREE(buf_text);
  FREE(buf_rend);
  FREE(tabs);
}

/* ------------------------------------------------------------------------- */
void
scr_poweron(void)
{
  int last_col;

  D_SCREEN(("scr_poweron()\n"));

  TermWin.nscrolled = 0;	/* xterm doesn't do this */

  last_col = TermWin.ncol - 1;

  MEMSET(charsets, 'B', sizeof(charsets));
  rvideo = 0;
  scr_rendition(0, ~RS_None);
#if NSCREENS
  scr_change_screen(SECONDARY);
  scr_erase_screen(2);
  swap.tscroll = 0;
  swap.bscroll = TermWin.nrow - 1;
  swap.row = swap.col = 0;
  swap.charset = 0;
  swap.flags = Screen_DefaultFlags;
#endif
  scr_change_screen(PRIMARY);
  scr_erase_screen(2);
  screen.row = screen.col = 0;
  screen.charset = 0;
  screen.flags = Screen_DefaultFlags;

  scr_cursor(SAVE);

  scr_reset();
  XClearWindow(Xdisplay, TermWin.vt);
  scr_refresh(SLOW_REFRESH);
}

/* ------------------------------------------------------------------------- *
 *                         PROCESS SCREEN COMMANDS                           *
 * ------------------------------------------------------------------------- */
/*
 * Save and Restore cursor
 * XTERM_SEQ: Save cursor   : ESC 7     
 * XTERM_SEQ: Restore cursor: ESC 8
 */
void
scr_cursor(int mode)
{
  D_SCREEN(("scr_cursor(%s)\n", (mode == SAVE ? "SAVE" : "RESTORE")));

  switch (mode) {
    case SAVE:
      save.row = screen.row;
      save.col = screen.col;
      save.rstyle = rstyle;
      save.charset = screen.charset;
      save.charset_char = charsets[screen.charset];
      break;
    case RESTORE:
      screen.row = save.row;
      screen.col = save.col;
      rstyle = save.rstyle;
      screen.charset = save.charset;
      charsets[screen.charset] = save.charset_char;
      set_font_style();
      break;
  }
}

/* ------------------------------------------------------------------------- */
/*
 * Swap between primary and secondary screens
 * XTERM_SEQ: Primary screen  : ESC [ ? 4 7 h
 * XTERM_SEQ: Secondary screen: ESC [ ? 4 7 l
 */
int
scr_change_screen(int scrn)
{
  int i, offset, tmp;
  text_t *t0;
  rend_t *r0;

  D_SCREEN(("scr_change_screen(%d)\n", scrn));

  TermWin.view_start = 0;
  RESET_CHSTAT;

  if (current_screen == scrn)
    return current_screen;

  SWAP_IT(current_screen, scrn, tmp);
#if NSCREENS
  offset = TermWin.saveLines;
  if (!screen.text || !screen.rend)
    return (current_screen);
  for (i = TermWin.nrow; i--;) {
    SWAP_IT(screen.text[i + offset], swap.text[i], t0);
    SWAP_IT(screen.rend[i + offset], swap.rend[i], r0);
  }
  SWAP_IT(screen.row, swap.row, tmp);
  SWAP_IT(screen.col, swap.col, tmp);
  SWAP_IT(screen.charset, swap.charset, tmp);
  SWAP_IT(screen.flags, swap.flags, tmp);
  screen.flags |= Screen_VisibleCursor;
  swap.flags |= Screen_VisibleCursor;

#else
# ifndef DONT_SCROLL_ME
  if (current_screen == PRIMARY)
    if (current_screen == PRIMARY) {
      scroll_text(0, (TermWin.nrow - 1), TermWin.nrow, 0);
      for (i = TermWin.saveLines; i < TermWin.nrow + TermWin.saveLines; i++)
	if (screen.text[i] == NULL) {
	  blank_screen_mem(screen.text, screen.rend, i, DEFAULT_RSTYLE);
	}
    }
# endif
#endif
  return scrn;
}

/* ------------------------------------------------------------------------- */
/*
 * Change the color for following text
 */
void
scr_color(unsigned int color, unsigned int Intensity)
{

  D_SCREEN(("scr_color(%u, %u) called.\n", color, Intensity));
  if (color == restoreFG)
    color = fgColor;
  else if (color == restoreBG)
    color = bgColor;
  else {
    if (Xdepth <= 2) {		/* Monochrome - ignore color changes */
      switch (Intensity) {
	case RS_Bold:
	  color = fgColor;
	  break;
	case RS_Blink:
	  color = bgColor;
	  break;
      }
    } else {
#ifndef NO_BRIGHTCOLOR
      if ((rstyle & Intensity) && color >= minColor && color <= maxColor)
	color += (minBright - minColor);
      else if (color >= minBright && color <= maxBright) {
	if (rstyle & Intensity)
	  return;
	color -= (minBright - minColor);
      }
#endif
    }
  }
  switch (Intensity) {
    case RS_Bold:
      rstyle = SET_FGCOLOR(rstyle, color);
      break;
    case RS_Blink:
      rstyle = SET_BGCOLOR(rstyle, color);
      break;
  }
}

/* ------------------------------------------------------------------------- */
/*
 * Change the rendition style for following text
 */
void
scr_rendition(int set, int style)
{
  unsigned int color;

  D_SCREEN(("scr_rendition(%d, %d) called.\n", set, style));
  if (set) {
/* A: Set style */
    rstyle |= style;
    switch (style) {
      case RS_RVid:
	if (rvideo)
	  rstyle &= ~RS_RVid;
	break;
#ifndef NO_BRIGHTCOLOR
      case RS_Bold:
	color = GET_FGCOLOR(rstyle);
	scr_color((color == fgColor ? GET_FGCOLOR(colorfgbg) : color),
		  RS_Bold);
	break;
      case RS_Blink:
	color = GET_BGCOLOR(rstyle);
	scr_color((color == bgColor ? GET_BGCOLOR(colorfgbg) : color),
		  RS_Blink);
	break;
#endif
    }
  } else {
/* B: Unset style */
    rstyle &= ~style;

    switch (style) {
      case ~RS_None:		/* default fg/bg colors */
	rstyle = DEFAULT_RSTYLE;
	/* FALLTHROUGH */
      case RS_RVid:
	if (rvideo)
	  rstyle |= RS_RVid;
	break;
#ifndef NO_BRIGHTCOLOR
      case RS_Bold:
	color = GET_FGCOLOR(rstyle);
	if (color >= minBright && color <= maxBright) {
	  scr_color(color, RS_Bold);
	  if ((rstyle & RS_fgMask) == (colorfgbg & RS_fgMask))
	    scr_color(restoreFG, RS_Bold);
	}
	break;
      case RS_Blink:
	color = GET_BGCOLOR(rstyle);
	if (color >= minBright && color <= maxBright) {
	  scr_color(color, RS_Blink);
	  if ((rstyle & RS_bgMask) == (colorfgbg & RS_bgMask))
	    scr_color(restoreBG, RS_Blink);
	}
	break;
#endif
    }
  }
}

/* ------------------------------------------------------------------------- */
/*
 * Scroll text between <row1> and <row2> inclusive, by <count> lines
 */
int
scroll_text(int row1, int row2, int count, int spec)
{
/*    int             i, j; */
  register int i, j;

#ifdef PROFILE_SCREEN
  static long call_cnt = 0;
  static long long total_time = 0;

  P_INITCOUNTER(cnt);
/*    struct timeval start = { 0, 0 }, stop = { 0, 0 }; */

  P_SETTIMEVAL(cnt.start);
#endif

  D_SCREEN(("scroll_text(%d,%d,%d,%d): %s\n", row1, row2, count, spec, (current_screen == PRIMARY) ? "Primary" : "Secondary"));

  if (count == 0 || (row1 > row2))
    return 0;
  if ((count > 0) && (row1 == 0) && (current_screen == PRIMARY)) {
    TermWin.nscrolled += count;
    MIN_IT(TermWin.nscrolled, TermWin.saveLines);
  } else if (!spec)
    row1 += TermWin.saveLines;
  row2 += TermWin.saveLines;

  if (selection.op && current_screen == selection.screen) {
    i = selection.beg.row + TermWin.saveLines;
    j = selection.end.row + TermWin.saveLines;
    if ((i < row1 && j > row1)
        || (i < row2 && j > row2)
        || (i - count < row1 && i >= row1)
        || (i - count > row2 && i <= row2)
        || (j - count < row1 && j >= row1)
        || (j - count > row2 && j <= row2)) {
      CLEAR_ALL_SELECTION;
      selection.op = SELECTION_CLEAR;
    } else if (j >= row1 && j <= row2) {
      /* move selected region too */
      selection.beg.row -= count;
      selection.end.row -= count;
      selection.mark.row -= count;
    }
  }
  CHECK_SELECTION;

  if (count > 0) {

/* A: scroll up */

    MIN_IT(count, row2 - row1 + 1);

/* A1: Copy and blank out lines that will get clobbered by the rotation */
    for (i = 0, j = row1; i < count; i++, j++) {
      buf_text[i] = screen.text[j];
      buf_rend[i] = screen.rend[j];
      if (buf_text[i] == NULL) {
	/* A new ALLOC is done with size ncol and
	   blankline with size prev_ncol -- Sebastien van K */
	buf_text[i] = MALLOC(sizeof(text_t) * (prev_ncol + 1));
	buf_rend[i] = MALLOC(sizeof(rend_t) * prev_ncol);
      }
      blank_line(buf_text[i], buf_rend[i], prev_ncol, DEFAULT_RSTYLE);
      buf_text[i][prev_ncol] = 0;
    }
/* A2: Rotate lines */
    for (j = row1; (j + count) <= row2; j++) {
      screen.text[j] = screen.text[j + count];
      screen.rend[j] = screen.rend[j + count];
    }
/* A3: Resurrect lines */
    for (i = 0; i < count; i++, j++) {
      screen.text[j] = buf_text[i];
      screen.rend[j] = buf_rend[i];
    }
  } else if (count < 0) {
/* B: scroll down */

    count = min(-count, row2 - row1 + 1);
/* B1: Copy and blank out lines that will get clobbered by the rotation */
    for (i = 0, j = row2; i < count; i++, j--) {
      buf_text[i] = screen.text[j];
      buf_rend[i] = screen.rend[j];
      if (buf_text[i] == NULL) {
	/* A new ALLOC is done with size ncol and
	   blankline with size prev_ncol -- Sebastien van K */
	buf_text[i] = MALLOC(sizeof(text_t) * (prev_ncol + 1));
	buf_rend[i] = MALLOC(sizeof(rend_t) * prev_ncol);
      }
      blank_line(buf_text[i], buf_rend[i], prev_ncol, DEFAULT_RSTYLE);
      buf_text[i][prev_ncol] = 0;
    }
/* B2: Rotate lines */
    for (j = row2; (j - count) >= row1; j--) {
      screen.text[j] = screen.text[j - count];
      screen.rend[j] = screen.rend[j - count];
    }
/* B3: Resurrect lines */
    for (i = 0, j = row1; i < count; i++, j++) {
      screen.text[j] = buf_text[i];
      screen.rend[j] = buf_rend[i];
    }
    count = -count;
  }
#ifdef PROFILE_SCREEN
  P_SETTIMEVAL(cnt.stop);
  total_time += P_CMPTIMEVALS_USEC(cnt.start, cnt.stop);
  fprintf(stderr, "scroll_text(%ld): %ld microseconds (%d)\n",
	  ++call_cnt, P_CMPTIMEVALS_USEC(cnt.start, cnt.stop), total_time);
#endif
  return count;
}

/* ------------------------------------------------------------------------- */
/*
 * Add text given in <str> of length <len> to screen struct
 */
void
scr_add_lines(const unsigned char *str, int nlines, int len)
{
/*    char            c; */
  register char c;

/*    int             i, j, row, last_col; */
  int last_col;
  register int i, j, row;
  text_t *stp;
  rend_t *srp;
  row_col_t beg, end;

  if (len <= 0)			/* sanity */
    return;

  last_col = TermWin.ncol;

  D_SCREEN(("scr_add_lines(*,%d,%d)\n", nlines, len));
  ZERO_SCROLLBACK;
  if (nlines > 0) {
    nlines += (screen.row - screen.bscroll);
    D_SCREEN((" -> screen.row == %d, screen.bscroll == %d, new nlines == %d\n", screen.row, screen.bscroll, nlines));
    if ((nlines > 0) && (screen.tscroll == 0) && (screen.bscroll == (TermWin.nrow - 1))) {
      /* _at least_ this many lines need to be scrolled */
      scroll_text(screen.tscroll, screen.bscroll, nlines, 0);
      for (i = nlines, row = screen.bscroll + TermWin.saveLines + 1; row > 0 && i--;) {
	/* Move row-- to beginning of loop to avoid segfault. -- added by Sebastien van K */
	row--;
        blank_screen_mem(screen.text, screen.rend, row, rstyle);
      }
      screen.row -= nlines;
    }
  }
  MIN_IT(screen.col, last_col - 1);
  MIN_IT(screen.row, TermWin.nrow - 1);
  /*MAX_IT(screen.row, 0); */
  MAX_IT(screen.row, -TermWin.nscrolled);

  row = screen.row + TermWin.saveLines;
  if (screen.text[row] == NULL) {
    blank_screen_mem(screen.text, screen.rend, row, DEFAULT_RSTYLE);
  }				/* avoid segfault -- added by Sebastien van K */
  beg.row = screen.row;
  beg.col = screen.col;
  stp = screen.text[row];
  srp = screen.rend[row];

#ifdef MULTI_CHARSET
  if (lost_multi && screen.col > 0
      && ((srp[screen.col - 1] & RS_multiMask) == RS_multi1)
      && *str != '\n' && *str != '\r' && *str != '\t')
    chstat = WBYTE;
#endif

  for (i = 0; i < len;) {
    c = str[i++];
#ifdef MULTI_CHARSET
    if (chstat == WBYTE) {
      rstyle |= RS_multiMask;	/* multibyte 2nd byte */
      chstat = SBYTE;
      if (encoding_method == EUCJ)
	c |= 0x80;		/* maybe overkill, but makes it selectable */
    } else if (chstat == SBYTE)
      if (multi_byte || (c & 0x80)) {	/* multibyte 1st byte */
	rstyle &= ~RS_multiMask;
	rstyle |= RS_multi1;
	chstat = WBYTE;
	if (encoding_method == EUCJ)
	  c |= 0x80;		/* maybe overkill, but makes it selectable */
      } else
#endif
	switch (c) {
	  case 127:
	    continue;		/* ummmm..... */
	  case '\t':
	    scr_tab(1);
	    continue;
	  case '\n':
	    MAX_IT(stp[last_col], screen.col);
	    screen.flags &= ~Screen_WrapNext;
	    if (screen.row == screen.bscroll) {
	      scroll_text(screen.tscroll, screen.bscroll, 1, 0);
	      j = screen.bscroll + TermWin.saveLines;
              blank_screen_mem(screen.text, screen.rend, j, DEFAULT_RSTYLE | ((rstyle & RS_RVid) ? (RS_RVid) : (0)));
	    } else if (screen.row < (TermWin.nrow - 1)) {
	      screen.row++;
	      row = screen.row + TermWin.saveLines;
	    }
	    stp = screen.text[row];	/* _must_ refresh */
	    srp = screen.rend[row];	/* _must_ refresh */
	    continue;
	  case '\r':
	    MAX_IT(stp[last_col], screen.col);
	    screen.flags &= ~Screen_WrapNext;
	    screen.col = 0;
	    continue;
	  default:
#ifdef MULTI_CHARSET
	    rstyle &= ~RS_multiMask;
#endif
	    break;
	}
    if (screen.flags & Screen_WrapNext) {
      stp[last_col] = WRAP_CHAR;
      if (screen.row == screen.bscroll) {
	scroll_text(screen.tscroll, screen.bscroll, 1, 0);
	j = screen.bscroll + TermWin.saveLines;
	/* blank_line(screen.text[j], screen.rend[j], TermWin.ncol,
	   rstyle);    Bug fix from John Ellison - need to reset rstyle */
        blank_screen_mem(screen.text, screen.rend, j, DEFAULT_RSTYLE | ((rstyle & RS_RVid) ? (RS_RVid) : (0)));
      } else if (screen.row < (TermWin.nrow - 1)) {
	screen.row++;
	row = screen.row + TermWin.saveLines;
      }
      stp = screen.text[row];	/* _must_ refresh */
      srp = screen.rend[row];	/* _must_ refresh */
      screen.col = 0;
      screen.flags &= ~Screen_WrapNext;
    }
    if (screen.flags & Screen_Insert)
      scr_insdel_chars(1, INSERT);
    stp[screen.col] = c;
    srp[screen.col] = rstyle;
    if (screen.col < (last_col - 1))
      screen.col++;
    else {
      stp[last_col] = last_col;
      if (screen.flags & Screen_Autowrap)
	screen.flags |= Screen_WrapNext;
      else
	screen.flags &= ~Screen_WrapNext;
    }
  }
  MAX_IT(stp[last_col], screen.col);
  if (screen.col == 0) {
    end.col = last_col - 1;
    end.row = screen.row - 1;
  } else {
    end.col = screen.col - 1;
    end.row = screen.row;
  }
  if (((selection.end.row > beg.row)
       || (selection.end.row == beg.row
	   && selection.end.col >= beg.col))
      && ((selection.beg.row < end.row)
	  || (selection.beg.row == end.row
	      && selection.beg.col <= end.col)))
    selection_reset();
}

/* ------------------------------------------------------------------------- */
/*
 * Process Backspace.  Move back the cursor back a position, wrap if have to
 * XTERM_SEQ: CTRL-H
 */
void
scr_backspace(void)
{

  RESET_CHSTAT;
  if (screen.col == 0 && screen.row > 0) {
    screen.col = TermWin.ncol - 1;
    screen.row--;
  } else if (screen.flags & Screen_WrapNext) {
    screen.flags &= ~Screen_WrapNext;
  } else
    scr_gotorc(0, -1, RELATIVE);
}

/* ------------------------------------------------------------------------- */
/*
 * Process Horizontal Tab
 * count: +ve = forward; -ve = backwards
 * XTERM_SEQ: CTRL-I
 */
void
scr_tab(int count)
{
  int i, x;

  RESET_CHSTAT;
  x = screen.col;
  if (count == 0)
    return;
  else if (count > 0) {
    for (i = x + 1; i < TermWin.ncol; i++) {
      if (tabs[i]) {
	x = i;
	if (!--count)
	  break;
      }
    }
  } else if (count < 0) {
    for (i = x - 1; i >= 0; i--) {
      if (tabs[i]) {
	x = i;
	if (!++count)
	  break;
      }
    }
  }
  if (x != screen.col)
    scr_gotorc(0, x, R_RELATIVE);
}

/* ------------------------------------------------------------------------- */
/*
 * Goto Row/Column
 */
void
scr_gotorc(int row, int col, int relative)
{
  D_SCREEN(("scr_gotorc(r:%d,c:%d,%d): from (r:%d,c:%d)\n", row, col, relative, screen.row, screen.col));

  ZERO_SCROLLBACK;
  RESET_CHSTAT;

  screen.col = ((relative & C_RELATIVE) ? (screen.col + col) : col);
  MAX_IT(screen.col, 0);
  MIN_IT(screen.col, TermWin.ncol - 1);

  if (screen.flags & Screen_WrapNext) {
    screen.flags &= ~Screen_WrapNext;
  }
  if (relative & R_RELATIVE) {
    if (row > 0) {
      if (screen.row <= screen.bscroll
	  && (screen.row + row) > screen.bscroll)
	screen.row = screen.bscroll;
      else
	screen.row += row;
    } else if (row < 0) {
      if (screen.row >= screen.tscroll
	  && (screen.row + row) < screen.tscroll)
	screen.row = screen.tscroll;
      else
	screen.row += row;
    }
  } else {
    if (screen.flags & Screen_Relative) {	/* relative origin mode */
      screen.row = row + screen.tscroll;
      MIN_IT(screen.row, screen.bscroll);
    } else
      screen.row = row;
  }
  MAX_IT(screen.row, 0);
  MIN_IT(screen.row, TermWin.nrow - 1);
}

/* ------------------------------------------------------------------------- */
/*
 * direction  should be UP or DN
 */
void
scr_index(int direction)
{
  int dirn;

  dirn = ((direction == UP) ? 1 : -1);
  D_SCREEN(("scr_index(%d)\n", dirn));

  ZERO_SCROLLBACK;
  RESET_CHSTAT;

  if (screen.flags & Screen_WrapNext) {
    screen.flags &= ~Screen_WrapNext;
  }
  if ((screen.row == screen.bscroll && direction == UP)
      || (screen.row == screen.tscroll && direction == DN)) {
    scroll_text(screen.tscroll, screen.bscroll, dirn, 0);
    if (direction == UP)
      dirn = screen.bscroll + TermWin.saveLines;
    else
      dirn = screen.tscroll + TermWin.saveLines;
    blank_screen_mem(screen.text, screen.rend, dirn, rstyle);
  } else
    screen.row += dirn;
  MAX_IT(screen.row, 0);
  MIN_IT(screen.row, TermWin.nrow - 1);
  CHECK_SELECTION;
}

/* ------------------------------------------------------------------------- */
/*
 * Erase part or whole of a line
 * XTERM_SEQ: Clear line to right: ESC [ 0 K
 * XTERM_SEQ: Clear line to left : ESC [ 1 K
 * XTERM_SEQ: Clear whole line   : ESC [ 2 K
 */
void
scr_erase_line(int mode)
{
  int row, col, num;

  D_SCREEN(("scr_erase_line(%d) at screen row: %d\n", mode, screen.row));
  ZERO_SCROLLBACK;
  RESET_CHSTAT;

  if (screen.flags & Screen_WrapNext)
    screen.flags &= ~Screen_WrapNext;

  row = TermWin.saveLines + screen.row;
  switch (mode) {
    case 0:			/* erase to end of line */
      col = screen.col;
      num = TermWin.ncol - col;
      MIN_IT(screen.text[row][TermWin.ncol], col);
      break;
    case 1:			/* erase to beginning of line */
      col = 0;
      num = screen.col + 1;
      break;
    case 2:			/* erase whole line */
      col = 0;
      num = TermWin.ncol;
      screen.text[row][TermWin.ncol] = 0;
      break;
    default:
      return;
  }
  blank_line(&(screen.text[row][col]), &(screen.rend[row][col]), num,
	     rstyle & ~(RS_RVid | RS_Uline));
}

/* ------------------------------------------------------------------------- */
/*
 * Erase part of whole of the screen
 * XTERM_SEQ: Clear screen after cursor : ESC [ 0 J
 * XTERM_SEQ: Clear screen before cursor: ESC [ 1 J
 * XTERM_SEQ: Clear whole screen        : ESC [ 2 J
 */
void
scr_erase_screen(int mode)
{
  int row, num, row_offset;
  rend_t ren;
  long gcmask;
  XGCValues gcvalue;
  Drawable draw_buffer;
  Pixmap pmap = None;

  D_SCREEN(("scr_erase_screen(%d) at screen row: %d\n", mode, screen.row));
  REFRESH_ZERO_SCROLLBACK;
  RESET_CHSTAT;
  row_offset = TermWin.saveLines;

  if (buffer_pixmap) {
    draw_buffer = buffer_pixmap;
    pmap = images[image_bg].current->pmap->pixmap;
  } else {
    draw_buffer = TermWin.vt;
  }

  switch (mode) {
    case 0:			/* erase to end of screen */
      scr_erase_line(0);
      row = screen.row + 1;	/* possible OOB */
      num = TermWin.nrow - row;
      break;
    case 1:			/* erase to beginning of screen */
      scr_erase_line(1);
      row = 0;			/* possible OOB */
      num = screen.row;
      break;
    case 2:			/* erase whole screen */
      row = 0;
      num = TermWin.nrow;
      break;
    default:
      return;
  }
  if (row >= 0 && row <= TermWin.nrow) {	/* check OOB */
    MIN_IT(num, (TermWin.nrow - row));
    if (rstyle & RS_RVid || rstyle & RS_Uline)
      ren = -1;
    else {
      if (GET_BGCOLOR(rstyle) == bgColor) {
	ren = DEFAULT_RSTYLE;
	CLEAR_ROWS(row, num);
      } else {
	ren = (rstyle & (RS_fgMask | RS_bgMask));
	gcvalue.foreground = PixColors[GET_BGCOLOR(ren)];
	gcmask = GCForeground;
	XChangeGC(Xdisplay, TermWin.gc, gcmask, &gcvalue);
	ERASE_ROWS(row, num);
	gcvalue.foreground = PixColors[fgColor];
	XChangeGC(Xdisplay, TermWin.gc, gcmask, &gcvalue);
      }
    }
    for (; num--; row++) {
      blank_screen_mem(screen.text, screen.rend, row + row_offset, rstyle & ~(RS_RVid | RS_Uline));
      blank_screen_mem(drawn_text, drawn_rend, row, ren);
    }
  }
}

/* ------------------------------------------------------------------------- */
/*
 * Fill the screen with `E's
 * XTERM_SEQ: Screen Alignment Test: ESC # 8
 */
void
scr_E(void)
{
  int i, j;
  text_t *t;
  rend_t *r, fs;

  ZERO_SCROLLBACK;
  RESET_CHSTAT;

  fs = rstyle;
  for (i = TermWin.saveLines; i < TermWin.nrow + TermWin.saveLines; i++) {
    t = screen.text[i];
    r = screen.rend[i];
    for (j = 0; j < TermWin.ncol; j++) {
      *t++ = 'E';
      *r++ = fs;
    }
    *t = '\0';
  }
}

/* ------------------------------------------------------------------------- */
/*
 * Insert/Delete <count> lines
 */
void
scr_insdel_lines(int count, int insdel)
{
  int end;

  ZERO_SCROLLBACK;
  RESET_CHSTAT;

  if (screen.row > screen.bscroll)
    return;

  end = screen.bscroll - screen.row + 1;
  if (count > end) {
    if (insdel == DELETE)
      return;
    else if (insdel == INSERT)
      count = end;
  }
  if (screen.flags & Screen_WrapNext) {
    screen.flags &= ~Screen_WrapNext;
  }
  scroll_text(screen.row, screen.bscroll, insdel * count, 0);

/* fill the inserted or new lines with rstyle. TODO: correct for delete? */
  if (insdel == DELETE) {
    end = screen.bscroll + TermWin.saveLines;
  } else if (insdel == INSERT) {
    end = screen.row + count - 1 + TermWin.saveLines;
  }
  for (; count--; end--) {
    blank_screen_mem(screen.text, screen.rend, end, rstyle);
  }
}

/* ------------------------------------------------------------------------- */
/*
 * Insert/Delete <count> characters from the current position
 */
void
scr_insdel_chars(int count, int insdel)
{
  int col, row;

  ZERO_SCROLLBACK;
  RESET_CHSTAT;

  if (count <= 0)
    return;

  CHECK_SELECTION;
  MIN_IT(count, (TermWin.ncol - screen.col));

  row = screen.row + TermWin.saveLines;
  screen.flags &= ~Screen_WrapNext;

  switch (insdel) {
    case INSERT:
      for (col = TermWin.ncol - 1; (col - count) >= screen.col; col--) {
	screen.text[row][col] = screen.text[row][col - count];
	screen.rend[row][col] = screen.rend[row][col - count];
      }
      screen.text[row][TermWin.ncol] += count;
      MIN_IT(screen.text[row][TermWin.ncol], TermWin.ncol);
      /* FALLTHROUGH */
    case ERASE:
      blank_line(&(screen.text[row][screen.col]),
		 &(screen.rend[row][screen.col]),
		 count, rstyle);
      break;
    case DELETE:
      for (col = screen.col; (col + count) < TermWin.ncol; col++) {
	screen.text[row][col] = screen.text[row][col + count];
	screen.rend[row][col] = screen.rend[row][col + count];
      }
      blank_line(&(screen.text[row][TermWin.ncol - count]),
		 &(screen.rend[row][TermWin.ncol - count]),
		 count, rstyle);
      screen.text[row][TermWin.ncol] -= count;
      if (((int) (char) screen.text[row][TermWin.ncol]) < 0)
	screen.text[row][TermWin.ncol] = 0;
      break;
  }
#ifdef MULTI_CHARSET
  if ((screen.rend[row][0] & RS_multiMask) == RS_multi2) {
    screen.rend[row][0] &= ~RS_multiMask;
    screen.text[row][0] = ' ';
  }
  if ((screen.rend[row][TermWin.ncol - 1] & RS_multiMask) == RS_multi1) {
    screen.rend[row][TermWin.ncol - 1] &= ~RS_multiMask;
    screen.text[row][TermWin.ncol - 1] = ' ';
  }
#endif
}

/* ------------------------------------------------------------------------- */
/*
 * Set the scrolling region
 * XTERM_SEQ: Set region <top> - <bot> inclusive: ESC [ <top> ; <bot> r
 */
void
scr_scroll_region(int top, int bot)
{
  MAX_IT(top, 0);
  MIN_IT(bot, TermWin.nrow - 1);
  if (top > bot)
    return;
  screen.tscroll = top;
  screen.bscroll = bot;
  scr_gotorc(0, 0, 0);
}

/* ------------------------------------------------------------------------- */
/*
 * Make the cursor visible/invisible
 * XTERM_SEQ: Make cursor visible  : ESC [ ? 25 h
 * XTERM_SEQ: Make cursor invisible: ESC [ ? 25 l
 */
void
scr_cursor_visible(int mode)
{
  if (mode)
    screen.flags |= Screen_VisibleCursor;
  else
    screen.flags &= ~Screen_VisibleCursor;
}

/* ------------------------------------------------------------------------- */
/*
 * Set/unset automatic wrapping
 * XTERM_SEQ: Set Wraparound  : ESC [ ? 7 h
 * XTERM_SEQ: Unset Wraparound: ESC [ ? 7 l
 */
void
scr_autowrap(int mode)
{
  if (mode)
    screen.flags |= Screen_Autowrap;
  else
    screen.flags &= ~Screen_Autowrap;
}

/* ------------------------------------------------------------------------- */
/*
 * Set/unset margin origin mode
 * Absolute mode: line numbers are counted relative to top margin of screen
 *      and the cursor can be moved outside the scrolling region.
 * Relative mode: line numbers are relative to top margin of scrolling region
 *      and the cursor cannot be moved outside.
 * XTERM_SEQ: Set Absolute: ESC [ ? 6 h
 * XTERM_SEQ: Set Relative: ESC [ ? 6 l
 */
void
scr_relative_origin(int mode)
{
  if (mode)
    screen.flags |= Screen_Relative;
  else
    screen.flags &= ~Screen_Relative;
  scr_gotorc(0, 0, 0);
}

/* ------------------------------------------------------------------------- */
/*
 * Set insert/replace mode
 * XTERM_SEQ: Set Insert mode : ESC [ ? 4 h
 * XTERM_SEQ: Set Replace mode: ESC [ ? 4 l
 */
void
scr_insert_mode(int mode)
{
  if (mode)
    screen.flags |= Screen_Insert;
  else
    screen.flags &= ~Screen_Insert;
}

/* ------------------------------------------------------------------------- */
/*
 * Set/Unset tabs
 * XTERM_SEQ: Set tab at current column  : ESC H
 * XTERM_SEQ: Clear tab at current column: ESC [ 0 g
 * XTERM_SEQ: Clear all tabs             : ESC [ 3 g
 */
void
scr_set_tab(int mode)
{
  if (mode < 0)
    MEMSET(tabs, 0, (unsigned int) TermWin.ncol);

  else if (screen.col < TermWin.ncol)
    tabs[screen.col] = (mode ? 1 : 0);
}

/* ------------------------------------------------------------------------- */
/*
 * Set reverse/normal video
 * XTERM_SEQ: Reverse video: ESC [ ? 5 h
 * XTERM_SEQ: Normal video : ESC [ ? 5 l
 */
void
scr_rvideo_mode(int mode)
{
  int i, j, maxlines;

  if (rvideo != mode) {
    rvideo = mode;
    rstyle ^= RS_RVid;

    maxlines = TermWin.saveLines + TermWin.nrow;
    for (i = TermWin.saveLines; i < maxlines; i++)
      for (j = 0; j < TermWin.ncol + 1; j++)
	screen.rend[i][j] ^= RS_RVid;
    scr_refresh(SLOW_REFRESH);
  }
}

/* ------------------------------------------------------------------------- */
/*
 * Report current cursor position
 * XTERM_SEQ: Report position: ESC [ 6 n
 */
void
scr_report_position(void)
{
  tt_printf((unsigned char *) "\033[%d;%dR", screen.row + 1, screen.col + 1);
}

/* ------------------------------------------------------------------------- *
 *                                  FONTS                                    * 
 * ------------------------------------------------------------------------- */

/*
 * Set font style
 */
void
set_font_style(void)
{
  rstyle &= ~RS_fontMask;
  switch (charsets[screen.charset]) {
    case '0':			/* DEC Special Character & Line Drawing Set */
      rstyle |= RS_acsFont;
      break;
    case 'A':			/* United Kingdom (UK) */
      rstyle |= RS_ukFont;
      break;
    case 'B':			/* United States (USASCII) */
      break;
    case '<':			/* Multinational character set */
      break;
    case '5':			/* Finnish character set */
      break;
    case 'C':			/* Finnish character set */
      break;
    case 'K':			/* German character set */
      break;
  }
}

/* ------------------------------------------------------------------------- */
/*
 * Choose a font
 * XTERM_SEQ: Invoke G0 character set: CTRL-O
 * XTERM_SEQ: Invoke G1 character set: CTRL-N
 * XTERM_SEQ: Invoke G2 character set: ESC N
 * XTERM_SEQ: Invoke G3 character set: ESC O
 */
void
scr_charset_choose(int set)
{
  screen.charset = set;
  set_font_style();
}

/* ------------------------------------------------------------------------- */
/*
 * Set a font
 * XTERM_SEQ: Set G0 character set: ESC ( <C>
 * XTERM_SEQ: Set G1 character set: ESC ) <C>
 * XTERM_SEQ: Set G2 character set: ESC * <C>
 * XTERM_SEQ: Set G3 character set: ESC + <C>
 * See set_font_style for possible values for <C>
 */
void
scr_charset_set(int set, unsigned int ch)
{
#ifdef MULTI_CHARSET
  multi_byte = (set < 0);
  set = abs(set);
#endif
  charsets[set] = (unsigned char) ch;
  set_font_style();
}

/* ------------------------------------------------------------------------- *
 *             MULTIPLE-CHARACTER SET MANIPULATION FUNCTIONS                 * 
 * ------------------------------------------------------------------------- */
#ifdef MULTI_CHARSET

static void eucj2jis(unsigned char *str, int len);
static void sjis2jis(unsigned char *str, int len);
static void big5dummy(unsigned char *str, int len);

static void (*multichar_decode) (unsigned char *str, int len) = eucj2jis;

static void
eucj2jis(unsigned char *str, int len)
{
  register int i;

  for (i = 0; i < len; i++)
    str[i] &= 0x7F;
}

static void
sjis2jis(unsigned char *str, int len)
{
  register int i;
  unsigned char *high, *low;

  for (i = 0; i < len; i += 2, str += 2) {
    high = str;
    low = str + 1;
    (*high) -= (*high > 0x9F ? 0xB1 : 0x71);
    *high = (*high) * 2 + 1;
    if (*low > 0x9E) {
      *low -= 0x7E;
      (*high)++;
    } else {
      if (*low > 0x7E)
	(*low)--;
      *low -= 0x1F;
    }
  }
}

static void
big5dummy(unsigned char *str, int len)
{
  str = NULL;
  len = 0;
}
#endif

void
set_multichar_encoding(const char *str)
{
#ifdef MULTI_CHARSET
  if (str && *str) {
    if (!strcmp(str, "sjis")) {
      encoding_method = SJIS;
      multichar_decode = sjis2jis;
    } else if (!strcmp(str, "eucj") || !strcmp(str, "euckr")
	       || !strcmp(str, "gb")) {
      encoding_method = EUCJ;
      multichar_decode = eucj2jis;
    } else if (!strcmp(str, "big5")) {
      encoding_method = BIG5;
      multichar_decode = big5dummy;
    }
  }
#else
  return;
  str = NULL;
#endif /* MULTI_CHARSET */
}

/* Refresh an area */
void
scr_expose(int x, int y, int width, int height)
{
  int i;
  rend_t *r;
  row_col_t full_beg, full_end, part_beg, part_end;

  if (drawn_text == NULL)	/* sanity check */
    return;

  part_beg.col = Pixel2Col(x);
  part_beg.row = Pixel2Row(y);
  part_end.col = Pixel2Width(x + width + TermWin.fwidth - 1);
  part_end.row = Pixel2Row(y + height + TermWin.fheight - 1);

  full_beg.col = Pixel2Col(x + TermWin.fwidth - 1);
  full_beg.row = Pixel2Row(y + TermWin.fheight - 1);
  full_end.col = Pixel2Width(x + width);
  full_end.row = Pixel2Row(y + height);

  /* sanity checks */
  MAX_IT(part_beg.col, 0);
  MAX_IT(full_beg.col, 0);
  MAX_IT(part_end.col, 0);
  MAX_IT(full_end.col, 0);
  MAX_IT(part_beg.row, 0);
  MAX_IT(full_beg.row, 0);
  MAX_IT(part_end.row, 0);
  MAX_IT(full_end.row, 0);
  MIN_IT(part_beg.col, TermWin.ncol - 1);
  MIN_IT(full_beg.col, TermWin.ncol - 1);
  MIN_IT(part_end.col, TermWin.ncol - 1);
  MIN_IT(full_end.col, TermWin.ncol - 1);
  MIN_IT(part_beg.row, TermWin.nrow - 1);
  MIN_IT(full_beg.row, TermWin.nrow - 1);
  MIN_IT(part_end.row, TermWin.nrow - 1);
  MIN_IT(full_end.row, TermWin.nrow - 1);

  D_SCREEN(("scr_expose(x:%d, y:%d, w:%d, h:%d) area (c:%d,r:%d)-(c:%d,r:%d)\n", x, y, width, height, part_beg.col, part_beg.row,
	    part_end.col, part_end.row));

  if (full_end.col >= full_beg.col)
    /* set DEFAULT_RSTYLE for totally exposed characters */
    for (i = full_beg.row; i <= full_end.row; i++)
      blank_line(&(drawn_text[i][full_beg.col]),
		 &(drawn_rend[i][full_beg.col]),
		 full_end.col - full_beg.col + 1, DEFAULT_RSTYLE);

/* force an update for partially exposed characters */
  if (part_beg.row != full_beg.row) {
    r = &(drawn_rend[part_beg.row][part_beg.col]);
    for (i = part_end.col - part_beg.col + 1; i--;)
      *r++ = RS_Dirty;
  }
  if (part_end.row != full_end.row) {
    r = &(drawn_rend[part_end.row][part_beg.col]);
    for (i = part_end.col - part_beg.col + 1; i--;)
      *r++ = RS_Dirty;
  }
  if (part_beg.col != full_beg.col)
    for (i = full_beg.row; i <= full_end.row; i++)
      drawn_rend[i][part_beg.col] = RS_Dirty;
  if (part_end.col != full_end.col)
    for (i = full_beg.row; i <= full_end.row; i++)
      drawn_rend[i][part_end.col] = RS_Dirty;

  if (buffer_pixmap) {
    x = Col2Pixel(full_beg.col);
    y = Row2Pixel(full_beg.row);
    XCopyArea(Xdisplay, images[image_bg].current->pmap->pixmap, buffer_pixmap, TermWin.gc, x, y, Width2Pixel(full_end.col - full_beg.col + 1),
              Height2Pixel(full_end.row - full_beg.row + 1), x, y);
  }
}

/* ------------------------------------------------------------------------- */
/*
 * Move the display so that the line represented by scrollbar value Y is at
 * the top of the screen
 */
int
scr_move_to(int y, int len)
{
  int start;

  start = TermWin.view_start;
  TermWin.view_start = ((len - y) * (TermWin.nrow - 1 + TermWin.nscrolled)
			/ (len)) - (TermWin.nrow - 1);
  D_SCREEN(("scr_move_to(%d, %d) view_start:%d\n", y, len, TermWin.view_start));

  MAX_IT(TermWin.view_start, 0);
  MIN_IT(TermWin.view_start, TermWin.nscrolled);

  return (TermWin.view_start - start);
}
/* ------------------------------------------------------------------------- */
/*
 * Page the screen up/down nlines
 * direction  should be UP or DN
 */
int
scr_page(int direction, int nlines)
{
  int start, dirn;

  D_SCREEN(("scr_page(%s, %d) view_start:%d\n", ((direction == UP) ? "UP" : "DN"), nlines, TermWin.view_start));

  dirn = (direction == UP) ? 1 : -1;
  start = TermWin.view_start;
  MAX_IT(nlines, 1);
  MIN_IT(nlines, TermWin.nrow);
  TermWin.view_start += (nlines * dirn);
  MAX_IT(TermWin.view_start, 0);
  MIN_IT(TermWin.view_start, TermWin.nscrolled);

  return (TermWin.view_start - start);
}

/* ------------------------------------------------------------------------- */
void
scr_bell(void)
{
#ifndef NO_MAPALERT
#ifdef MAPALERT_OPTION
  if (Options & Opt_mapAlert)
#endif
    XMapWindow(Xdisplay, TermWin.parent);
#endif
  if (Options & Opt_visualBell) {
    scr_rvideo_mode(!rvideo);	/* scr_refresh() also done */
    scr_rvideo_mode(!rvideo);	/* scr_refresh() also done */
  } else
    XBell(Xdisplay, 0);
}

/* ------------------------------------------------------------------------- */
void
scr_printscreen(int fullhist)
{
#ifdef PRINTPIPE
  int i, r, nrows, row_offset;
  text_t *t;
  FILE *fd;

  if ((fd = popen_printer()) == NULL)
    return;
  nrows = TermWin.nrow;
  if (fullhist) {
    /* Print the entire scrollback buffer.  Always start from the top and go all the way to the bottom. */
    nrows += TermWin.nscrolled;
    row_offset = TermWin.saveLines - TermWin.nscrolled;
  } else {
    /* Just print what's on the screen. */
    row_offset = TermWin.saveLines - TermWin.view_start;
  }

  for (r = 0; r < nrows; r++) {
    t = screen.text[r + row_offset];
    for (i = TermWin.ncol - 1; i >= 0; i--)
      if (!isspace(t[i]))
	break;
    fprintf(fd, "%.*s\n", (i + 1), t);
  }
  pclose_printer(fd);
#endif
}

/* ------------------------------------------------------------------------- */
/*
 * Refresh the screen
 * drawn_text/drawn_rend contain the screen information before the update.
 * screen.text/screen.rend contain what the screen will change to.
 */

void
scr_refresh(int type)
{
  int i,			/* tmp                                       */
   scrrow,			/* screen row offset                         */
   row_offset,			/* basic offset in screen structure          */
   boldlast = 0,		/* last character in some row was bold       */
   len, wlen,			/* text length screen/buffer                 */
   fprop,			/* proportional font used                    */
   is_cursor,			/* cursor this position                      */
   rvid,			/* reverse video this position               */
#if 0
   rend,			/* rendition                                 */
#endif
   fore, back,			/* desired foreground/background             */
   wbyte,			/* we're in multibyte                        */
   xpixel,			/* x offset for start of drawing (font)      */
   ypixel;			/* y offset for start of drawing (font)      */
  register int col, row,	/* column/row we're processing               */
   rend;			/* rendition                                 */
  static int focus = -1;	/* screen in focus?                          */
  long gcmask;			/* Graphics Context mask                     */
  unsigned long ltmp;
  rend_t rt1, rt2,		/* tmp rend values                           */
   lastrend;			/* rend type of last char in drawing set     */
  text_t lasttext;		/* last char being replaced in drawing set   */
  rend_t *drp, *srp;		/* drawn-rend-pointer, screen-rend-pointer   */
  text_t *dtp, *stp;		/* drawn-text-pointer, screen-text-pointer   */
  XGCValues gcvalue;		/* Graphics Context values                   */
  char buf[MAX_COLS + 1];
  register char *buffer = buf;
  Drawable draw_buffer;
  Pixmap pmap = images[image_bg].current->pmap->pixmap;
  int (*draw_string) (), (*draw_image_string) ();
  register int low_x = 99999, low_y = 99999, high_x = 0, high_y = 0;
#ifndef NO_BOLDFONT
  int bfont = 0;		/* we've changed font to bold font           */
#endif
#ifdef OPTIMIZE_HACKS
  register int nrows = TermWin.nrow;
  register int ncols = TermWin.ncol;
#endif
#ifdef PROFILE_SCREEN
  static long call_cnt = 0;
  static long long total_time = 0;

  /*    struct timeval start = { 0, 0 }, stop = { 0, 0 }; */
  P_INITCOUNTER(cnt);
#endif

  switch (type) {
    case NO_REFRESH:
      D_SCREEN(("scr_refresh(NO_REFRESH) called.\n"));
      break;
    case SLOW_REFRESH:
      D_SCREEN(("scr_refresh(SLOW_REFRESH) called.\n"));
      break;
    case FAST_REFRESH:
      D_SCREEN(("scr_refresh(FAST_REFRESH) called.\n"));
      break;
  }
  if (type == NO_REFRESH)
    return;

#ifdef PROFILE_SCREEN
  P_SETTIMEVAL(cnt.start);
#endif

  if (buffer_pixmap) {
    draw_buffer = buffer_pixmap;
  } else {
    draw_buffer = TermWin.vt;
  }

  row_offset = TermWin.saveLines - TermWin.view_start;
  fprop = TermWin.fprop;

  gcvalue.foreground = PixColors[fgColor];
  gcvalue.background = PixColors[bgColor];
/*
 * always go back to the base font - it's much safer
 */
  wbyte = 0;

  XSetFont(Xdisplay, TermWin.gc, TermWin.font->fid);

  draw_string = XDrawString;
  draw_image_string = XDrawImageString;

/*
 * Setup the cursor
 */
  row = screen.row + TermWin.saveLines;
  col = screen.col;
  if (screen.flags & Screen_VisibleCursor) {
    screen.rend[row][col] |= RS_Cursor;
#ifdef MULTI_CHARSET
    srp = &screen.rend[row][col];
    if ((col < ncols - 1) && ((srp[0] & RS_multiMask) == RS_multi1)
	&& ((srp[1] & RS_multiMask) == RS_multi2)) {
      screen.rend[row][col + 1] |= RS_Cursor;
    } else if ((col > 0) && ((srp[0] & RS_multiMask) == RS_multi2)
	       && ((srp[-1] & RS_multiMask) == RS_multi1)) {
      screen.rend[row][col - 1] |= RS_Cursor;
    }
#endif
    if (focus != TermWin.focus) {
      focus = TermWin.focus;
      if ((i = screen.row - TermWin.view_start) >= 0) {
	drawn_rend[i][col] = RS_attrMask;
#ifdef MULTI_CHARSET
	if ((col < ncols - 1) && ((srp[1] & RS_multiMask) == RS_multi2)) {
	  drawn_rend[i][col + 1] = RS_attrMask;
	} else if ((col > 0) && ((srp[-1] & RS_multiMask) == RS_multi1)) {
	  drawn_rend[i][col - 1] = RS_attrMask;
	}
#endif
      }
    }
  }
  /*
   * Go over every single position
   */
  for (row = 0; row < nrows; row++) {

    scrrow = row + row_offset;
    stp = screen.text[scrrow];
    srp = screen.rend[scrrow];
    dtp = drawn_text[row];
    drp = drawn_rend[row];

    for (col = 0; col < ncols; col++) {
      /* compare new text with old - if exactly the same then continue */
      rt1 = srp[col];		/* screen rendition */
      rt2 = drp[col];		/* drawn rendition  */
      if ((stp[col] == dtp[col])	/* must match characters to skip */
	  && ((rt1 == rt2)	/* either rendition the same or  */
	      || ((stp[col] == ' ')	/* space w/ no bg change */
		  &&(GET_BGATTR(rt1) == GET_BGATTR(rt2))
		  && !(rt2 & RS_Dirty)))) {
#ifdef MULTI_CHARSET
	/* if first byte is multibyte then compare second bytes */
	if ((rt1 & RS_multiMask) != RS_multi1)
	  continue;
	else if (stp[col + 1] == dtp[col + 1]) {
	  /* assume no corrupt characters on the screen */
	  col++;
	  continue;
	}
#else
	continue;
#endif
      }
      lasttext = dtp[col];
      lastrend = drp[col];
      /* redraw one or more characters */
      dtp[col] = stp[col];
      rend = drp[col] = srp[col];

      len = 0;
      buffer[len++] = stp[col];
      ypixel = TermWin.font->ascent + Row2Pixel(row);
      xpixel = Col2Pixel(col);
      wlen = 1;

/*
 * Find out the longest string we can write out at once
 */
      if (fprop == 0) {		/* Fixed width font */
#ifdef MULTI_CHARSET
	if (((rend & RS_multiMask) == RS_multi1) && (col < ncols - 1)
	    && ((srp[col + 1]) & RS_multiMask) == RS_multi2) {
	  if (!wbyte) {
	    wbyte = 1;
	    XSetFont(Xdisplay, TermWin.gc, TermWin.mfont->fid);
	    draw_string = XDrawString16;
	    draw_image_string = XDrawImageString16;
	  }
	  /* double stepping - we're in Multibyte mode */
	  for (; ++col < ncols;) {
	    /* XXX: could check sanity on 2nd byte */
	    dtp[col] = stp[col];
	    drp[col] = srp[col];
	    buffer[len++] = stp[col];
	    col++;
	    if ((col == ncols) || (srp[col] != rend))
	      break;
	    if ((stp[col] == dtp[col])
		&& (srp[col] == drp[col])
		&& (stp[col + 1] == dtp[col + 1]))
	      break;
	    if (len == MAX_COLS)
	      break;
	    dtp[col] = stp[col];
	    drp[col] = srp[col];
	    buffer[len++] = stp[col];
	  }
	  col--;
	  if (buffer[0] & 0x80)
	    multichar_decode(buffer, len);
	  wlen = len / 2;
	} else {
	  if ((rend & RS_multiMask) == RS_multi1) {
	    /* XXX : maybe do the same thing for RS_multi2 */
	    /* corrupt character - you're outta there */
	    rend &= ~RS_multiMask;
	    drp[col] = rend;	/* TODO check: may also want */
	    dtp[col] = ' ';	/* to poke into stp/srp      */
	    buffer[0] = ' ';
	  }
	  if (wbyte) {
	    wbyte = 0;
	    XSetFont(Xdisplay, TermWin.gc, TermWin.font->fid);
	    draw_string = XDrawString;
	    draw_image_string = XDrawImageString;
	  }
#endif
	  /* single stepping - `normal' mode */
	  for (; ++col < ncols - 1;) {
	    if ((unsigned int) rend != srp[col])
	      break;
	    if ((stp[col] == dtp[col]) && (srp[col] == drp[col]))
	      break;
	    if (len == MAX_COLS)
	      break;
	    lasttext = dtp[col];
	    lastrend = drp[col];
	    dtp[col] = stp[col];
	    drp[col] = srp[col];
	    buffer[len++] = stp[col];
	  }			/* for (; ++col < TermWin.ncol - 1;) */
	  col--;
	  wlen = len;
#ifdef MULTI_CHARSET
	}
#endif
      }
      buffer[len] = '\0';

/*
 * Determine the attributes for the string
 */
      fore = GET_FGCOLOR(rend);
      back = GET_BGCOLOR(rend);
      rend = GET_ATTR(rend);
      gcmask = 0;
      rvid = (rend & RS_RVid) ? 1 : 0;
      if (rend & RS_Select)
	rvid = !rvid;
      if (rend & RS_Cursor) {
	if (focus) {
	  is_cursor = 2;	/* normal cursor */
	  rvid = !rvid;
	} else {
	  is_cursor = 1;	/* outline cursor */
	  rend &= ~RS_Cursor;
	}
	srp[col] &= ~RS_Cursor;
      } else
	is_cursor = 0;
      switch (rend & RS_fontMask) {
	case RS_acsFont:
	  for (i = 0; i < len; i++)
	    if (buffer[i] == 0x5f)
	      buffer[i] = 0x7f;
	    else if (buffer[i] > 0x5f && buffer[i] < 0x7f)
	      buffer[i] -= 0x5f;
	  break;
	case RS_ukFont:
	  for (i = 0; i < len; i++)
	    if (buffer[i] == '#')
	      buffer[i] = 0x1e;
	  break;
      }
      if (rvid)
	SWAP_IT(fore, back, i);
      if (back != bgColor) {
	gcvalue.background = PixColors[back];
	gcmask |= GCBackground;
      }
      if (fore != fgColor) {
	gcvalue.foreground = PixColors[fore];
	gcmask |= GCForeground;
      }
#ifndef NO_BOLDUNDERLINE
      else if (rend & RS_Bold) {
	if (PixColors[fore] != PixColors[colorBD]
	    && PixColors[back] != PixColors[colorBD]) {
	  gcvalue.foreground = PixColors[colorBD];
	  gcmask |= GCForeground;
	}
      } else if (rend & RS_Uline) {
	if (PixColors[fore] != PixColors[colorUL]
	    && PixColors[back] != PixColors[colorUL]) {
	  gcvalue.foreground = PixColors[colorUL];
	  gcmask |= GCForeground;
	}
      }
#endif
#ifndef NO_CURSORCOLOR
      if (rend & RS_Cursor) {
	if (PixColors[cursorColor] != PixColors[bgColor]) {
	  gcvalue.background = PixColors[cursorColor];
	  back = cursorColor;
	  gcmask |= GCBackground;
	}
	if (PixColors[cursorColor2] != PixColors[fgColor]) {
	  gcvalue.foreground = PixColors[cursorColor2];
	  gcmask |= GCForeground;
	}
      }
#endif
      if (gcmask) {
	XChangeGC(Xdisplay, TermWin.gc, gcmask, &gcvalue);
      }
#ifndef NO_BOLDFONT
      if (!wbyte && MONO_BOLD(rend) && TermWin.boldFont != NULL) {
	XSetFont(Xdisplay, TermWin.gc, TermWin.boldFont->fid);
	bfont = 1;
      } else if (bfont) {
	bfont = 0;
	XSetFont(Xdisplay, TermWin.gc, TermWin.font->fid);
      }
#endif
/*
 * Actually do the drawing of the string here
 */
      if (fprop) {
	if (rvid) {
	  SWAP_IT(gcvalue.foreground, gcvalue.background, ltmp);
	  gcmask |= (GCForeground | GCBackground);
	  XChangeGC(Xdisplay, TermWin.gc, gcmask, &gcvalue);
	  XFillRectangle(Xdisplay, draw_buffer, TermWin.gc, xpixel, ypixel - TermWin.font->ascent, Width2Pixel(1), Height2Pixel(1));
	  SWAP_IT(gcvalue.foreground, gcvalue.background, ltmp);
	  XChangeGC(Xdisplay, TermWin.gc, gcmask, &gcvalue);
	} else {
	  CLEAR_CHARS(xpixel, ypixel - TermWin.font->ascent, 1);
	}
	DRAW_STRING(draw_string, xpixel, ypixel, buffer, 1);
        UPDATE_BOX(xpixel, ypixel - TermWin.font->ascent, xpixel + Width2Pixel(1), ypixel + Height2Pixel(1));
#ifndef NO_BOLDOVERSTRIKE
	if (MONO_BOLD(rend)) {
	  DRAW_STRING(draw_string, xpixel + 1, ypixel, buffer, 1);
          UPDATE_BOX(xpixel + 1, ypixel - TermWin.font->ascent, xpixel + 1 + Width2Pixel(1), ypixel + Height2Pixel(1));
        }
#endif
      } else
#ifdef PIXMAP_SUPPORT
      if (background_is_pixmap() && (back == bgColor)) {
	CLEAR_CHARS(xpixel, ypixel - TermWin.font->ascent, len);
	DRAW_STRING(draw_string, xpixel, ypixel, buffer, wlen);
        UPDATE_BOX(xpixel, ypixel - TermWin.font->ascent, xpixel + Width2Pixel(len), ypixel + Height2Pixel(1));
      } else
#endif
      {
#ifdef FORCE_CLEAR_CHARS
	CLEAR_CHARS(xpixel, ypixel - TermWin.font->ascent, len);
#endif
	DRAW_STRING(draw_image_string, xpixel, ypixel, buffer, wlen);
        UPDATE_BOX(xpixel, ypixel - TermWin.font->ascent, xpixel + Width2Pixel(len), ypixel + Height2Pixel(1));
      }

      /* do the convoluted bold overstrike */
#ifndef NO_BOLDOVERSTRIKE
      if (MONO_BOLD(rend)) {
	DRAW_STRING(draw_string, xpixel + 1, ypixel, buffer, wlen);
        UPDATE_BOX(xpixel + 1, ypixel - TermWin.font->ascent, xpixel + 1 + Width2Pixel(len), ypixel + Height2Pixel(1));
      }
#endif

      if ((rend & RS_Uline) && (TermWin.font->descent > 1)) {
	XDrawLine(Xdisplay, draw_buffer, TermWin.gc, xpixel, ypixel + 1, xpixel + Width2Pixel(len) - 1, ypixel + 1);
        UPDATE_BOX(xpixel, ypixel + 1, xpixel + Width2Pixel(len) - 1, ypixel + 1);
      }
      if (is_cursor == 1) {
#ifndef NO_CURSORCOLOR
	if (PixColors[cursorColor] != PixColors[bgColor]) {
	  gcvalue.foreground = PixColors[cursorColor];
	  gcmask |= GCForeground;
	  XChangeGC(Xdisplay, TermWin.gc, gcmask, &gcvalue);
	}
#endif
	XDrawRectangle(Xdisplay, draw_buffer, TermWin.gc, xpixel, ypixel - TermWin.font->ascent, Width2Pixel(1 + wbyte) - 1, Height2Pixel(1) - 1);
        UPDATE_BOX(xpixel, ypixel - TermWin.font->ascent, Width2Pixel(1 + wbyte) - 1, Height2Pixel(1) - 1);
      }
      if (gcmask) {		/* restore normal colors */
	gcvalue.foreground = PixColors[fgColor];
	gcvalue.background = PixColors[bgColor];
	XChangeGC(Xdisplay, TermWin.gc, gcmask, &gcvalue);
      }
      if (MONO_BOLD(lastrend)) {
	if (col < ncols - 1) {
	  drp[col + 1] |= RS_Dirty;
	  if (wbyte || (TermWin.font->per_char[lasttext - TermWin.font->min_char_or_byte2].width == TermWin.font->per_char[lasttext - TermWin.font->min_char_or_byte2].rbearing))
	    drp[col + 1] |= RS_Dirty;
	} else
	  boldlast = 1;
      }
    }				/* for (col = 0; col < TermWin.ncol; col++) */
  }				/* for (row = 0; row < TermWin.nrow; row++) */

  row = screen.row + TermWin.saveLines;
  col = screen.col;
  if (screen.flags & Screen_VisibleCursor) {
    screen.rend[row][col] &= ~RS_Cursor;
#ifdef MULTI_CHARSET
    /* very low overhead so don't check properly, just wipe it all out */
    if (screen.col < ncols - 1)
      screen.rend[row][col + 1] &= ~RS_Cursor;
    if (screen.col > 0)
      screen.rend[row][col - 1] &= ~RS_Cursor;
#endif
  }
  if (buffer_pixmap) {
    XClearArea(Xdisplay, TermWin.vt, low_x, low_y, high_x - low_x + 1, high_y - low_y + 1, False);
  } else {
    if (boldlast) {
      XClearArea(Xdisplay, TermWin.vt, TermWin_TotalWidth() - 2, 0,
                 1, TermWin_TotalHeight() - 1, 0);
    }
    if (boldlast) {
      XClearArea(Xdisplay, TermWin.vt, TermWin_TotalWidth() - 2, 0,
                 1, TermWin_TotalHeight() - 1, 0);
    }
  }
  if (type == SLOW_REFRESH) {
    XSync(Xdisplay, False);
  }
  D_SCREEN(("scr_refresh() exiting.\n"));

#ifdef PROFILE_SCREEN
  P_SETTIMEVAL(cnt.stop);
  total_time += P_CMPTIMEVALS_USEC(cnt.start, cnt.stop);
  fprintf(stderr, "scr_refresh(%ld): %ld microseconds (%d)\n",
	  ++call_cnt, P_CMPTIMEVALS_USEC(cnt.start, cnt.stop), total_time);
#endif
}
/*    TermWin term_win screen scr foobar */

/* ------------------------------------------------------------------------- *
 *                           CHARACTER SELECTION                             * 
 * ------------------------------------------------------------------------- */

/*
 * If (row,col) is within a selected region of text, remove the selection
 * -TermWin.nscrolled <= (selection row) <= TermWin.nrow - 1
 */
void
selection_check(void)
{
  int c1, c2, r1, r2;

  if (current_screen != selection.screen)
    return;

  if ((selection.mark.row < -TermWin.nscrolled)
      || (selection.mark.row >= TermWin.nrow)
      || (selection.beg.row < -TermWin.nscrolled)
      || (selection.beg.row >= TermWin.nrow)
      || (selection.end.row < -TermWin.nscrolled)
      || (selection.end.row >= TermWin.nrow)) {
    selection_reset();
    return;
  }
  r1 = (screen.row - TermWin.view_start);

  c1 = ((r1 - selection.mark.row) * (r1 - selection.end.row));

/*
 * selection.mark.row > screen.row - TermWin.view_start
 * or
 * selection.end.row > screen.row - TermWin.view_start
 */
  if (c1 < 0)
    selection_reset();
  else if (c1 == 0) {
    if ((selection.mark.row < selection.end.row)
	|| ((selection.mark.row == selection.end.row)
	    && (selection.mark.col < selection.end.col))) {
      r1 = selection.mark.row;
      c1 = selection.mark.col;
      r2 = selection.end.row;
      c2 = selection.end.col;
    } else {
      r1 = selection.end.row;
      c1 = selection.end.col;
      r2 = selection.mark.row;
      c2 = selection.mark.col;
    }
    if ((screen.row == r1) && (screen.row == r2)) {
      if ((screen.col >= c1) && (screen.col <= c2))
	selection_reset();
    } else if (((screen.row == r1) && (screen.col >= c1))
	       || ((screen.row == r2) && (screen.col <= c2)))
      selection_reset();
  }
}

/* ------------------------------------------------------------------------- */
/*
 * Paste a selection direct to the command
 */
void
PasteIt(unsigned char *data, unsigned int nitems)
{
  int num;
  unsigned char *p, cr;

  cr = '\r';
  for (p = data, num = 0; nitems--; p++)
    if (*p != '\n')
      num++;
    else {
      tt_write(data, num);
      tt_write(&cr, 1);
      data += (num + 1);
      num = 0;
    }
  if (num)
    tt_write(data, num);
}

/* ------------------------------------------------------------------------- */
/*
 * Respond to a notification that a primary selection has been sent
 * EXT: SelectionNotify
 */
void
selection_paste(Window win, unsigned prop, int Delete)
{
  long nread;
  unsigned long bytes_after, nitems;
  unsigned char *data;
  Atom actual_type;
  int actual_fmt;

  if (prop == None)
    return;
  for (nread = 0, bytes_after = 1; bytes_after > 0;) {
    if ((XGetWindowProperty(Xdisplay, win, prop, (nread / 4), PROP_SIZE,
			    Delete, AnyPropertyType, &actual_type, &actual_fmt,
			    &nitems, &bytes_after, &data) != Success)) {
      XFree(data);
      return;
    }
    nread += nitems;

    PasteIt(data, nitems);
    XFree(data);
  }
}

/* ------------------------------------------------------------------------- */
/*
 * Request the current primary selection
 * EXT: button 2 release
 */
void
selection_request(Time tm, int x, int y)
{
  Atom prop;

  if (x < 0 || x >= TermWin.width || y < 0 || y >= TermWin.height)
    return;			/* outside window */

  if (selection.text != NULL) {
    PasteIt(selection.text, selection.len);	/* internal selection */
  } else if (XGetSelectionOwner(Xdisplay, XA_PRIMARY) == None) {
    selection_paste(Xroot, XA_CUT_BUFFER0, False);
  } else {
    prop = XInternAtom(Xdisplay, "VT_SELECTION", False);
    XConvertSelection(Xdisplay, XA_PRIMARY, XA_STRING, prop, TermWin.vt,
		      tm);
  }
}

/* ------------------------------------------------------------------------- */
/*
 * Clear the current selection from the screen rendition list
 */
void
selection_reset(void)
{
  int i, j, lrow, lcol;

  D_SELECT(("selection_reset()\n"));

  lrow = TermWin.nrow + TermWin.saveLines;
  lcol = TermWin.ncol;
  selection.op = SELECTION_CLEAR;

  i = (current_screen == PRIMARY) ? 0 : TermWin.saveLines;
  for (; i < lrow; i++)
    if (screen.rend[i])		/* not everything may be malloc()d yet */
      for (j = 0; j < lcol; j++)
	screen.rend[i][j] &= ~RS_Select;
}
/* ------------------------------------------------------------------------- */
/*
 * Clear all selected text
 * EXT:
 */
void
selection_clear(void)
{
  D_SELECT(("selection_clear()\n"));

  if (selection.text)
    FREE(selection.text);
  selection.text = NULL;
  selection.len = 0;
  selection_reset();
}
/* ------------------------------------------------------------------------- */
/*
 * Set or clear between selected points (inclusive)
 */
void
selection_setclr(int set, int startr, int startc, int endr, int endc)
{
  int row, col, last_col;
  rend_t *rend;

  D_SELECT(("selection_setclr(%d) %s (%d,%d)-(%d,%d)\n", set, (set ? "set  " : "clear"), startc, startr, endc, endr));

  if ((startr < -TermWin.nscrolled) || (endr >= TermWin.nrow)) {
    selection_reset();
    return;
  }
  last_col = TermWin.ncol - 1;

  MIN_IT(endc, last_col);
  MIN_IT(startr, TermWin.nrow - 1);
  MAX_IT(startr, -TermWin.nscrolled);
  MIN_IT(endr, TermWin.nrow - 1);
  MAX_IT(endr, -TermWin.nscrolled);
  MAX_IT(startc, 0);

  startr += TermWin.saveLines;
  endr += TermWin.saveLines;

  col = startc;
  if (set) {
    for (row = startr; row < endr; row++) {
      rend = &(screen.rend[row][col]);
      for (; col <= last_col; col++, rend++)
	*rend |= RS_Select;
      col = 0;
    }
    rend = &(screen.rend[row][col]);
    for (; col <= endc; col++, rend++)
      *rend |= RS_Select;
  } else {
    for (row = startr; row < endr; row++) {
      rend = &(screen.rend[row][col]);
      for (; col <= last_col; col++, rend++)
	*rend &= ~RS_Select;
      col = 0;
    }
    rend = &(screen.rend[row][col]);
    for (; col <= endc; col++, rend++)
      *rend &= ~RS_Select;
  }
}

/* ------------------------------------------------------------------------- */
/*
 * Mark a selection at the specified x/y pixel location
 */
void
selection_start(int x, int y)
{
  D_SELECT(("selection_start(%d, %d)\n", x, y));
  selection_start_colrow(Pixel2Col(x), Pixel2Row(y));
}

/* ------------------------------------------------------------------------- */
/*
 * Mark a selection at the specified col/row
 */
void
selection_start_colrow(int col, int row)
{
  int end_col;

  D_SELECT(("selection_start_colrow(%d, %d)\n", col, row));

  if (selection.op) {
    /* clear the old selection */

    if (selection.beg.row < -TermWin.nscrolled)
      selection_reset();
    else
      selection_setclr(0,
		       selection.beg.row, selection.beg.col,
		       selection.end.row, selection.end.col);
  }
  selection.op = SELECTION_INIT;
  MAX_IT(row, 0);
  MIN_IT(row, TermWin.nrow - 1);

  row -= TermWin.view_start;
  end_col = screen.text[row + TermWin.saveLines][TermWin.ncol];
  if (end_col != WRAP_CHAR && col > end_col)
    col = TermWin.ncol;
  selection.mark.col = col;
  selection.mark.row = row;
}

/* ------------------------------------------------------------------------- */
/* 
 * Copy a selection into the cut buffer
 * EXT: button 1 or 3 release
 */
void
selection_make(Time tm)
{
  int i, col, end_col, row, end_row;
  unsigned char *new_selection_text;
  char *str;
  text_t *t;

  D_SELECT(("selection_make(): selection.op=%d, selection.clicks=%d\n", selection.op, selection.clicks));
  switch (selection.op) {
    case SELECTION_CONT:
      break;
    case SELECTION_INIT:
      selection_reset();
      selection.end.row = selection.beg.row = selection.mark.row;
      selection.end.col = selection.beg.col = selection.mark.col;
      /* FALLTHROUGH */
    case SELECTION_BEGIN:
      selection.op = SELECTION_DONE;
      /* FALLTHROUGH */
    default:
      return;
  }
  selection.op = SELECTION_DONE;

  if (selection.clicks == 4)
    return;			/* nothing selected, go away */

  if (selection.beg.row < -TermWin.nscrolled
      || selection.end.row >= TermWin.nrow) {
    selection_reset();
    return;
  }
  i = (selection.end.row - selection.beg.row + 1) * (TermWin.ncol + 1) + 1;
  str = MALLOC(i * sizeof(char));
  new_selection_text = (unsigned char *) str;

  col = max(selection.beg.col, 0);
  row = selection.beg.row + TermWin.saveLines;
  end_row = selection.end.row + TermWin.saveLines;
/*
 * A: rows before end row
 */
  for (; row < end_row; row++) {
    t = &(screen.text[row][col]);
    if ((end_col = screen.text[row][TermWin.ncol]) == WRAP_CHAR)
      end_col = TermWin.ncol;
    for (; col < end_col; col++)
      *str++ = *t++;
    col = 0;
    if (screen.text[row][TermWin.ncol] != WRAP_CHAR) {
      if (!(Options & Opt_select_trailing_spaces)) {
	for (str--; *str == ' ' || *str == '\t'; str--);
	str++;
      }
      *str++ = '\n';
    }
  }
/*
 * B: end row
 */
  t = &(screen.text[row][col]);
  end_col = screen.text[row][TermWin.ncol];
  if (end_col == WRAP_CHAR || selection.end.col <= end_col) {
    i = 0;
    end_col = selection.end.col + 1;
  } else
    i = 1;
  MIN_IT(end_col, TermWin.ncol);
  for (; col < end_col; col++)
    *str++ = *t++;
  if (!(Options & Opt_select_trailing_spaces)) {
    for (str--; *str == ' ' || *str == '\t'; str--);
    str++;
  }
  if (i)
    *str++ = '\n';
  *str = '\0';
  if ((i = strlen((char *) new_selection_text)) == 0) {
    FREE(new_selection_text);
    return;
  }
  selection.len = i;
  if (selection.text)
    FREE(selection.text);
  selection.text = new_selection_text;
  selection.screen = current_screen;

  XSetSelectionOwner(Xdisplay, XA_PRIMARY, TermWin.vt, tm);
  if (XGetSelectionOwner(Xdisplay, XA_PRIMARY) != TermWin.vt)
    print_error("can't get primary selection");
  XChangeProperty(Xdisplay, Xroot, XA_CUT_BUFFER0, XA_STRING, 8,
		  PropModeReplace, selection.text, selection.len);
  D_SELECT(("selection_make(): selection.len=%d\n", selection.len));
}

/* ------------------------------------------------------------------------- */
/*
 * Mark or select text based upon number of clicks: 1, 2, or 3
 * EXT: button 1 press
 */
void
selection_click(int clicks, int x, int y)
{

/*
 *  int             r, c;
 *  row_col_t       ext_beg, ext_end;
 */

  D_SELECT(("selection_click(%d, %d, %d)\n", clicks, x, y));

  clicks = ((clicks - 1) % 3) + 1;
  selection.clicks = clicks;	/* save clicks so extend will work */

  selection_start(x, y);	/* adjusts for scroll offset */
  if (clicks == 2 || clicks == 3)
    selection_extend_colrow(selection.mark.col,
			    selection.mark.row + TermWin.view_start, 0, 1);
}

/* ------------------------------------------------------------------------- */
/*
 * Select text for 2 clicks
 * row is given as a normal selection row value
 * beg.row, end.row are returned as normal selection row values
 */

/* what do we want: spaces/tabs are delimiters or cutchars or non-cutchars */
#ifdef CUTCHAR_OPTION
#  define DELIMIT_TEXT(x) (strchr((rs_cutchars?rs_cutchars:CUTCHARS), (x)) != NULL)
#else
#  define DELIMIT_TEXT(x) (strchr(CUTCHARS, (x)) != NULL)
#endif
#ifdef MULTI_CHARSET
#define DELIMIT_REND(x)	(((x) & RS_multiMask) ? 1 : 0)
#endif

void
selection_delimit_word(int col, int row, row_col_t * beg, row_col_t * end)
{
  int beg_col, beg_row, end_col, end_row, last_col;
  int row_offset, w1;
  text_t *stp, *stp1, t;

#ifdef MULTI_CHARSET
  int w2;
  rend_t *srp, r;

#endif

  if (selection.clicks != 2)	/* We only handle double clicks: go away */
    return;

  if (!screen.text || !screen.rend)
    return;

  last_col = TermWin.ncol - 1;

  if (row >= TermWin.nrow) {
    row = TermWin.nrow - 1;
    col = last_col;
  } else if (row < -TermWin.saveLines) {
    row = -TermWin.saveLines;
    col = 0;
  }
  beg_col = end_col = col;
  beg_row = end_row = row;

  row_offset = TermWin.saveLines;

/* A: find the beginning of the word */

  if (!screen.text[beg_row + row_offset] || !screen.rend[beg_row + row_offset])
    return;
  if (!screen.text[end_row + row_offset] || !screen.rend[end_row + row_offset])
    return;
#if 0
  if (!screen.text[beg_row + row_offset - 1] || !screen.rend[beg_row + row_offset - 1])
    return;
  if (!screen.text[end_row + row_offset + 1] || !screen.rend[end_row + row_offset + 1])
    return;
#endif

  stp1 = stp = &(screen.text[beg_row + row_offset][beg_col]);
  w1 = DELIMIT_TEXT(*stp);
  if (w1 == 2)
    w1 = 0;
#ifdef MULTI_CHARSET
  srp = &(screen.rend[beg_row + row_offset][beg_col]);
  w2 = DELIMIT_REND(*srp);
#endif

  for (;;) {
    for (; beg_col > 0; beg_col--) {
      t = *--stp;
      if (DELIMIT_TEXT(t) != w1 || (w1 && *stp1 != t && Options & Opt_xterm_select))
	break;
#ifdef MULTI_CHARSET
      r = *--srp;
      if (DELIMIT_REND(r) != w2)
	break;
#endif
    }
    if (!(Options & Opt_xterm_select)) {
      if (beg_col == col && beg_col > 0) {
	if (DELIMIT_TEXT(*stp))	/* space or tab or cutchar */
	  break;
#ifdef MULTI_CHARSET
	srp = &(screen.rend[beg_row + row_offset][beg_col - 1]);
#endif
	for (; --beg_col > 0;) {
	  t = *--stp;
	  if (DELIMIT_TEXT(t))
	    break;
#ifdef MULTI_CHARSET
	  r = *--srp;
	  if (DELIMIT_REND(r) != w2)
	    break;
#endif
	}
      }
    }
    if (beg_col == 0 && (beg_row > -TermWin.nscrolled)) {
      stp = &(screen.text[beg_row + row_offset - 1][last_col + 1]);
      if (*stp == WRAP_CHAR) {
	t = *(stp - 1);
#ifdef MULTI_CHARSET
	srp = &(screen.rend[beg_row + row_offset - 1][last_col + 1]);
	r = *(srp - 1);
	if (DELIMIT_TEXT(t) == w1 && (!w1 || *stp == t || !(Options & Opt_xterm_select)) && DELIMIT_REND(r) == w2) {
	  srp--;
#else
	if (DELIMIT_TEXT(t) == w1 && (!w1 || *stp == t || !(Options & Opt_xterm_select))) {
#endif
	  stp--;
	  beg_row--;
	  beg_col = last_col;
	  continue;
	}
      }
    }
    break;
  }

/* B: find the end of the word */

# ifdef OPTIMIZE_HACKS
  stp = stp1;
# else
  stp1 = stp = &(screen.text[end_row + row_offset][end_col]);
# endif

#ifdef MULTI_CHARSET
  srp = &(screen.rend[end_row + row_offset][end_col]);
#endif
  for (;;) {
    for (; end_col < last_col; end_col++) {
      t = *++stp;
      if (DELIMIT_TEXT(t) != w1 || (w1 && *stp1 != t && Options & Opt_xterm_select))
	break;
#ifdef MULTI_CHARSET
      r = *++srp;
      if (DELIMIT_REND(r) != w2)
	break;
#endif
    }
    if (!(Options & Opt_xterm_select)) {
      if (end_col == col && end_col < last_col) {
	if (DELIMIT_TEXT(*stp))	/* space or tab or cutchar */
	  break;
#ifdef MULTI_CHARSET
	srp = &(screen.rend[end_row + row_offset][end_col + 1]);
#endif
	for (; ++end_col < last_col;) {
	  t = *++stp;
	  if (DELIMIT_TEXT(t))
	    break;
#ifdef MULTI_CHARSET
	  r = *++srp;
	  if (DELIMIT_REND(r) != w2)
	    break;
#endif
	}
      }
    }
    if (end_col == last_col
	&& (end_row < (TermWin.nrow - 1))) {
      if (*++stp == WRAP_CHAR) {
	stp = screen.text[end_row + row_offset + 1];
#ifdef MULTI_CHARSET
	srp = screen.rend[end_row + row_offset + 1];
	if (DELIMIT_TEXT(*stp) == w1 && (!w1 || *stp1 == *stp || !(Options & Opt_xterm_select)) && DELIMIT_REND(*srp) == w2) {
#else
	if (DELIMIT_TEXT(*stp) == w1 && (!w1 || *stp1 == *stp || !(Options & Opt_xterm_select))) {
#endif
	  end_row++;
	  end_col = 0;
	  continue;
	}
      }
    }
    break;
  }

  D_SELECT(("selection_delimit_word(%d, %d) says (%d,%d)->(%d,%d)\n", col, row, beg_col, beg_row, end_col, end_row));

/* Poke the values back in */
  beg->col = beg_col;
  beg->row = beg_row;
  end->col = end_col;
  end->row = end_row;
}

/* ------------------------------------------------------------------------- */
/*
 * Extend the selection to the specified x/y pixel location
 * EXT: button 3 press; button 1 or 3 drag
 * flag == 0 ==> button 1
 * flag != 0 ==> button 3
 */
void
selection_extend(int x, int y, int flag)
{
  int col, row;

/*
 * If we're selecting characters (single click) then we must check first
 * if we are at the same place as the original mark.  If we are then
 * select nothing.  Otherwise, if we're to the right of the mark, you have to
 * be _past_ a character for it to be selected.
 */
  col = Pixel2Col(x);
  row = Pixel2Row(y);
  MAX_IT(row, 0);
  MIN_IT(row, TermWin.nrow - 1);
  if (((selection.clicks % 3) == 1) && !flag && (col == selection.mark.col && (row == selection.mark.row + TermWin.view_start))) {
    /* select nothing */
    selection_setclr(0, selection.beg.row, selection.beg.col,
		     selection.end.row, selection.end.col);
    selection.beg.row = selection.end.row = selection.mark.row;
    selection.beg.col = selection.end.col = selection.mark.col;
    selection.clicks = 4;
    D_SELECT(("selection_extend() selection.clicks = 4\n"));
    return;
  }
  if (selection.clicks == 4)
    selection.clicks = 1;
  selection_extend_colrow(col, row, flag, 0);
}

/* ------------------------------------------------------------------------- */
/*
 * Extend the selection to the specified col/row
 */
void
selection_extend_colrow(int col, int row, int flag, int cont)
{
  int old_col, end_col;
  row_col_t new_beg1, new_beg2, new_end1, new_end2, old_beg, old_end;
  enum {
    LEFT, RIGHT
  } closeto = RIGHT;

#ifdef MULTI_CHARSET
  int r;

#endif

  D_SELECT(("selection_extend_colrow(%d, %d, %d, %d) clicks:%d\n", col, row, flag, cont, selection.clicks));

  switch (selection.op) {
    case SELECTION_INIT:
      selection_reset();
      selection.end.col = selection.beg.col = selection.mark.col;
      selection.end.row = selection.beg.row = selection.mark.row;
      selection.op = SELECTION_BEGIN;
      /* FALLTHROUGH */
    case SELECTION_BEGIN:
      break;
    case SELECTION_DONE:
      selection.op = SELECTION_CONT;
      /* FALLTHROUGH */
    case SELECTION_CONT:
      break;
    case SELECTION_CLEAR:
      selection_start_colrow(col, row);
      /* FALLTHROUGH */
    default:
      return;
  }
  if ((selection.beg.row < -TermWin.nscrolled)
      || (selection.end.row < -TermWin.nscrolled)) {
    selection_reset();
    return;
  }
  old_col = col;
  MAX_IT(col, -1);
  MIN_IT(col, TermWin.ncol);
  old_beg.col = selection.beg.col;
  old_beg.row = selection.beg.row;
  old_end.col = selection.end.col;
  old_end.row = selection.end.row;

  if ((selection.op == SELECTION_BEGIN) && (cont || (row != selection.mark.row || col != selection.mark.col)))
    selection.op = SELECTION_CONT;

  row -= TermWin.view_start;	/* adjust for scroll */

  if (flag) {
    if (row < selection.beg.row
	|| (row == selection.beg.row && col < selection.beg.col))
      closeto = LEFT;
    else if (row > selection.end.row
	     || (row == selection.end.row && col >= selection.end.col)) {
      /* */ ;
    } else if (((col - selection.beg.col)
		+ ((row - selection.beg.row) * TermWin.ncol))
	       < ((selection.end.col - col)
		  + ((selection.end.row - row) * TermWin.ncol)))
      closeto = LEFT;
  }
  if (selection.clicks == 1) {
/*
 * A1: extension on single click - selection between points
 */
    if (flag) {			/* button 3 extension */
      if (closeto == LEFT) {
	selection.beg.row = row;
	selection.beg.col = col;
	end_col = screen.text[row + TermWin.saveLines][TermWin.ncol];
	if (end_col != WRAP_CHAR && selection.beg.col > end_col) {
	  if (selection.beg.row < selection.end.row) {
	    selection.beg.col = -1;
	    selection.beg.row++;
	  } else {
	    selection.beg.col = selection.mark.col;
	    selection.beg.row = selection.mark.row;
	  }
	}
      } else {
	selection.end.row = row;
	selection.end.col = col - 1;
	end_col = screen.text[row + TermWin.saveLines][TermWin.ncol];
	if (end_col != WRAP_CHAR && selection.end.col >= end_col)
	  selection.end.col = TermWin.ncol - 1;
      }
    } else if ((row < selection.mark.row)
	       || (row == selection.mark.row && col < selection.mark.col)) {
      /* select left of mark character excluding mark */
      selection.beg.row = row;
      selection.beg.col = col;
      selection.end.row = selection.mark.row;
      selection.end.col = selection.mark.col - 1;
      if (selection.end.col >= 0) {
	end_col = screen.text[row + TermWin.saveLines][TermWin.ncol];
	if (end_col != WRAP_CHAR && selection.beg.col > end_col) {
	  if (selection.beg.row < selection.end.row) {
	    selection.beg.col = -1;
	    selection.beg.row++;
	  } else {
	    selection.beg.col = selection.mark.col;
	    selection.beg.row = selection.mark.row;
	  }
	}
      }
    } else {
      /* select right of mark character including mark */
      selection.beg.row = selection.mark.row;
      selection.beg.col = selection.mark.col;
      selection.end.row = row;
      selection.end.col = col - 1;
      if (old_col >= 0) {
	end_col = screen.text[row + TermWin.saveLines][TermWin.ncol];
	if (end_col != WRAP_CHAR && selection.end.col >= end_col)
	  selection.end.col = TermWin.ncol - 1;
      }
    }
#ifdef MULTI_CHARSET
    if (selection.beg.col > 0) {
      r = selection.beg.row + TermWin.saveLines;
      if (((screen.rend[r][selection.beg.col] & RS_multiMask) == RS_multi2)
	  && ((screen.rend[r][selection.beg.col - 1] & RS_multiMask) == RS_multi1))
	selection.beg.col--;
    }
    if (selection.end.col < TermWin.ncol - 1) {
      r = selection.end.row + TermWin.saveLines;
      if (((screen.rend[r][selection.end.col] & RS_multiMask) == RS_multi1)
	  && ((screen.rend[r][selection.end.col + 1] & RS_multiMask) == RS_multi2))
	selection.end.col++;
    }
#endif
  } else if (selection.clicks == 2) {
/*
 * A2: extension on double click - selection between words
 */
    selection_delimit_word(col, row, &new_beg2, &new_end2);
    if (flag && closeto == LEFT)
      selection_delimit_word(selection.end.col, selection.end.row, &new_beg1, &new_end1);
    else if (flag && closeto == RIGHT)
      selection_delimit_word(selection.beg.col, selection.beg.row, &new_beg1, &new_end1);
    else
      selection_delimit_word(selection.mark.col, selection.mark.row, &new_beg1, &new_end1);
    if ((!flag && (selection.mark.row < row || (selection.mark.row == row && selection.mark.col <= col)))
	|| (flag && closeto == RIGHT)) {
      selection.beg.col = new_beg1.col;
      selection.beg.row = new_beg1.row;
      selection.end.col = new_end2.col;
      selection.end.row = new_end2.row;
    } else {
      selection.beg.col = new_beg2.col;
      selection.beg.row = new_beg2.row;
      selection.end.col = new_end1.col;
      selection.end.row = new_end1.row;
    }
  } else if (selection.clicks == 3) {
/*
 * A3: extension on triple click - selection between lines
 */
    if (flag) {
      if (closeto == LEFT)
	selection.beg.row = row;
      else
	selection.end.row = row;
    } else if (row <= selection.mark.row) {
      selection.beg.row = row;
      selection.end.row = selection.mark.row;
    } else {
      selection.beg.row = selection.mark.row;
      selection.end.row = row;
    }
    if (Options & Opt_select_whole_line) {
      selection.beg.col = 0;
    } else {
      selection.clicks = 2;
      selection_delimit_word(col, row, &new_beg2, &new_end2);
      selection.beg.col = new_beg2.col;
      selection.clicks = 3;
    }
    selection.end.col = TermWin.ncol - 1;
  }
  D_SELECT(("selection_extend_colrow(): (c:%d,r:%d)-(c:%d,r:%d) old (c:%d,r:%d)-(c:%d,r:%d)\n", selection.beg.col, selection.beg.row,
	    selection.end.col, selection.end.row, old_beg.col, old_beg.row, old_end.col, old_end.row));

/* 
 * B1: clear anything before the current selection
 */
  if ((old_beg.row < selection.beg.row) || (old_beg.row == selection.beg.row && old_beg.col < selection.beg.col)) {
    if (selection.beg.col < TermWin.ncol - 1) {
      row = selection.beg.row;
      col = selection.beg.col + 1;
    } else {
      row = selection.beg.row + 1;
      col = 0;
    }
    selection_setclr(0, old_beg.row, old_beg.col, row, col);
  }
/* 
 * B2: clear anything after the current selection
 */
  if ((old_end.row > selection.end.row) || (old_end.row == selection.end.row && old_end.col > selection.end.col)) {
    if (selection.end.col > 0) {
      row = selection.end.row;
      col = selection.end.col - 1;
    } else {
      row = selection.end.row - 1;
      col = TermWin.ncol - 1;
    }
    selection_setclr(0, row, col, old_end.row, old_end.col);
  }
/* 
 * B3: set everything
 */
/* TODO: optimise this */
  selection_setclr(1, selection.beg.row, selection.beg.col, selection.end.row, selection.end.col);
  return;
}

/* ------------------------------------------------------------------------- */
/*
 * Double click on button 3 when already selected
 * EXT: button 3 double click
 */
void
selection_rotate(int x, int y)
{
  int col, row;

  col = Pixel2Col(x);
  row = Pixel2Row(y);
  selection.clicks = selection.clicks % 3 + 1;
  selection_extend_colrow(col, row, 1, 0);
}

/* ------------------------------------------------------------------------- */
/*
 * On some systems, the Atom typedef is 64 bits wide.  We need to have a type
 * that is exactly 32 bits wide, because a format of 64 is not allowed by
 * the X11 protocol.
 */
typedef CARD32 Atom32;

/* ------------------------------------------------------------------------- */
/*
 * Respond to a request for our current selection
 * EXT: SelectionRequest
 */
void
selection_send(XSelectionRequestEvent * rq)
{
  XEvent ev;
  Atom32 target_list[2];
  static Atom xa_targets = None;

  if (xa_targets == None)
    xa_targets = XInternAtom(Xdisplay, "TARGETS", False);

  ev.xselection.type = SelectionNotify;
  ev.xselection.property = None;
  ev.xselection.display = rq->display;
  ev.xselection.requestor = rq->requestor;
  ev.xselection.selection = rq->selection;
  ev.xselection.target = rq->target;
  ev.xselection.time = rq->time;

  if (rq->target == xa_targets) {
    target_list[0] = (Atom32) xa_targets;
    target_list[1] = (Atom32) XA_STRING;
    XChangeProperty(Xdisplay, rq->requestor, rq->property, rq->target,
		    (8 * sizeof(target_list[0])), PropModeReplace,
		    (unsigned char *) target_list,
		    (sizeof(target_list) / sizeof(target_list[0])));
    ev.xselection.property = rq->property;
  } else if (rq->target == XA_STRING) {
    XChangeProperty(Xdisplay, rq->requestor, rq->property, rq->target,
		    8, PropModeReplace, selection.text, selection.len);
    ev.xselection.property = rq->property;
  }
  XSendEvent(Xdisplay, rq->requestor, False, 0, &ev);
}

/* ------------------------------------------------------------------------- *
 *                              MOUSE ROUTINES                               * 
 * ------------------------------------------------------------------------- */

void
mouse_report(XButtonEvent * ev)
{
  int button_number, key_state;

  button_number = ((ev->button == AnyButton) ? 3 : (ev->button - Button1));
  key_state = ((ev->state & (ShiftMask | ControlMask))
	       + ((ev->state & Mod1Mask) ? 2 : 0));
  tt_printf((unsigned char *) "\033[M%c%c%c",
	    (32 + button_number + (key_state << 2)),
	    (32 + Pixel2Col(ev->x) + 1),
	    (32 + Pixel2Row(ev->y) + 1));
}

/* ------------------------------------------------------------------------- */

void
mouse_tracking(int report, int x, int y, int firstrow, int lastrow)
{
  report = 0;
  x = 0;
  y = 0;
  firstrow = 0;
  lastrow = 0;
/* TODO */
}

/* ------------------------------------------------------------------------- *
 *                              DEBUG ROUTINES                               * 
 * ------------------------------------------------------------------------- */
void
debug_PasteIt(unsigned char *data, int nitems)
{
  data = NULL;
  nitems = 0;
/* TODO */
}

/* ------------------------------------------------------------------------- */
int
debug_selection(void)
{
/* TODO */
  return 0;
}

/* ------------------------------------------------------------------------- */
void
debug_colors(void)
{
  int color;
  char *name[] =
  {
    "fg", "bg",
    "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white"
  };

  fprintf(stderr, "Color ( ");
  if (rstyle & RS_RVid)
    fprintf(stderr, "rvid ");
  if (rstyle & RS_Bold)
    fprintf(stderr, "bold ");
  if (rstyle & RS_Blink)
    fprintf(stderr, "blink ");
  if (rstyle & RS_Uline)
    fprintf(stderr, "uline ");
  fprintf(stderr, "): ");

  color = GET_FGCOLOR(rstyle);
#ifndef NO_BRIGHTCOLOR
  if (color >= minBright && color <= maxBright) {
    color -= (minBright - minColor);
    fprintf(stderr, "bright ");
  }
#endif
  fprintf(stderr, "%s on ", name[color]);

  color = GET_BGCOLOR(rstyle);
#ifndef NO_BRIGHTCOLOR
  if (color >= minBright && color <= maxBright) {
    color -= (minBright - minColor);
    fprintf(stderr, "bright ");
  }
#endif
  fprintf(stderr, "%s\n", name[color]);
}

#ifdef USE_XIM
void xim_get_position(XPoint *pos)
{
   pos->x = Col2Pixel(screen.col);
   if (scrollbar_is_visible() && !(Options & Opt_scrollbar_right))
     pos->x += scrollbar_trough_width();
   pos->y = Height2Pixel(screen.row) + TermWin.font->ascent
            + TermWin.internalBorder;
}
#endif
