/*  term.c -- Eterm terminal emulation module

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

static const char cvs_ident[] = "$Id$";

#include "config.h"
#include "feature.h"

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#include "../libmej/debug.h"
#include "../libmej/mem.h"
#include "../libmej/strings.h"
#include "debug.h"
#include "actions.h"
#include "command.h"
#include "e.h"
#include "events.h"
#include "main.h"
#include "options.h"
#include "pixmap.h"
#include "screen.h"
#include "scrollbar.h"
#include "term.h"
#include "windows.h"

#ifdef META8_OPTION
unsigned char meta_char = 033;	/* Alt-key prefix */
#endif
unsigned long PrivateModes = PrivMode_Default;
unsigned long SavedModes = PrivMode_Default;
char *def_colorName[] =
{
  "rgb:ff/ff/ff", "rgb:0/0/0",	/* fg/bg */
  "rgb:0/0/0",			/* 0: black             (#000000) */
#ifndef NO_BRIGHTCOLOR
    /* low-intensity colors */
  "rgb:cc/00/00",		/* 1: red     */
  "rgb:00/cc/00",		/* 2: green   */
  "rgb:cc/cc/00",		/* 3: yellow  */
  "rgb:00/00/cc",		/* 4: blue    */
  "rgb:cc/00/cc",		/* 5: magenta */
  "rgb:00/cc/cc",		/* 6: cyan    */
  "rgb:fa/eb/d7",		/* 7: white   */
    /* high-intensity colors */
  "rgb:33/33/33",		/* 8: bright black */
#endif				/* NO_BRIGHTCOLOR */
  "rgb:ff/00/00",		/* 1/9:  bright red     */
  "rgb:00/ff/00",		/* 2/10: bright green   */
  "rgb:ff/ff/00",		/* 3/11: bright yellow  */
  "rgb:00/00/ff",		/* 4/12: bright blue    */
  "rgb:ff/00/ff",		/* 5/13: bright magenta */
  "rgb:00/ff/ff",		/* 6/14: bright cyan    */
  "rgb:ff/ff/ff",		/* 7/15: bright white   */
#ifndef NO_CURSORCOLOR
  NULL, NULL,			/* cursorColor, cursorColor2 */
#endif				/* NO_CURSORCOLOR */
  NULL, NULL			/* pointerColor, borderColor */
#ifndef NO_BOLDUNDERLINE
  ,NULL, NULL			/* colorBD, colorUL */
#endif				/* NO_BOLDUNDERLINE */
  ,"rgb:ff/ff/ff"		/* menuTextColor */
  ,"rgb:b2/b2/b2"		/* scrollColor: match Netscape color */
  ,NULL				/* menuColor: match scrollbar color */
  ,NULL				/* unfocusedscrollColor: somebody chose black? */
  ,NULL				/* unfocusedMenuColor */
};
char *rs_color[NRS_COLORS];
Pixel PixColors[NRS_COLORS + NSHADOWCOLORS];

/* To handle buffer overflows properly, we must malloc a buffer.  Free it when done. */
#ifdef USE_XIM
#  define LK_RET()   do {if (kbuf_alloced) FREE(kbuf); return;} while (0)
#else
#  define LK_RET()   return
#endif
/* Convert the keypress event into a string */
void
lookup_key(XEvent * ev)
{

  static int numlock_state = 0;
  int ctrl, meta, shft, len;
  KeySym keysym;
#ifdef USE_XIM
  int valid_keysym;
  static unsigned char short_buf[256];
  unsigned char *kbuf = short_buf;
  int kbuf_alloced = 0;
#else
  static unsigned char kbuf[KBUFSZ];
#endif
#if DEBUG >= DEBUG_CMD
  static int debug_key = 1;	/* accessible by a debugger only */
#endif
#ifdef GREEK_SUPPORT
  static short greek_mode = 0;
#endif

  /*
   * use Num_Lock to toggle Keypad on/off.  If Num_Lock is off, allow an
   * escape sequence to toggle the Keypad.
   *
   * Always permit `shift' to override the current setting
   */
  shft = (ev->xkey.state & ShiftMask);
  ctrl = (ev->xkey.state & ControlMask);
  meta = (ev->xkey.state & Mod1Mask);
  if (numlock_state || (ev->xkey.state & Mod5Mask)) {
    numlock_state = (ev->xkey.state & Mod5Mask);	/* numlock toggle */
    PrivMode((!numlock_state), PrivMode_aplKP);
  }
#ifdef USE_XIM
  if (Input_Context != NULL) {
    Status status_return;

    kbuf[0] = '\0';
    len = XmbLookupString(Input_Context, &ev->xkey, (char *)kbuf,
                          sizeof(short_buf), &keysym, &status_return);
    if (status_return == XBufferOverflow) {
      kbuf = (unsigned char *) MALLOC(len + 1);
      kbuf_alloced = 1;
      len = XmbLookupString(Input_Context, &ev->xkey, (char *)kbuf, len, &keysym, &status_return);
    }
    valid_keysym = (status_return == XLookupKeySym) || (status_return == XLookupBoth);
  } else {
    len = XLookupString(&ev->xkey, kbuf, sizeof(short_buf), &keysym, NULL);
  }
#else /* USE_XIM */
  len = XLookupString(&ev->xkey, kbuf, sizeof(kbuf), &keysym, NULL);

  /*
   * have unmapped Latin[2-4] entries -> Latin1
   * good for installations  with correct fonts, but without XLOCALE
   */
  if (!len && (keysym >= 0x0100) && (keysym < 0x0400)) {
    len = 1;
    kbuf[0] = (keysym & 0xff);
  }
#endif /* USE_XIM */

#ifdef USE_XIM
  if (valid_keysym) {
#endif

  if (action_dispatch(ev, keysym)) {
    LK_RET();
  } 
  if (len) {
    if (keypress_exit) {
      exit(0);
    } else if (Options & Opt_homeOnInput) {
      TermWin.view_start = 0;
    }
  }

  if ((Options & Opt_report_as_keysyms) && (keysym >= 0xff00)) {
    len = sprintf(kbuf, "\e[k%X;%X~", (unsigned int) (ev->xkey.state & 0xff), (unsigned int) (keysym & 0xff));
    tt_write(kbuf, len);
    LK_RET();
  }
#ifdef HOTKEY
  /* for some backwards compatibility */
  if (HOTKEY) {
    if (keysym == ks_bigfont) {
      change_font(0, FONT_UP);
      LK_RET();
    } else if (keysym == ks_smallfont) {
      change_font(0, FONT_DN);
      LK_RET();
    }
  }
#endif

  if (shft) {
    /* Shift + F1 - F10 generates F11 - F20 */
    if (keysym >= XK_F1 && keysym <= XK_F10) {
      keysym += (XK_F11 - XK_F1);
      shft = 0;			/* turn off Shift */
    } else if (!ctrl && !meta && (PrivateModes & PrivMode_ShiftKeys)) {

      int lnsppg;		/* Lines per page to scroll */

#ifdef PAGING_CONTEXT_LINES
      lnsppg = TermWin.nrow - PAGING_CONTEXT_LINES;
#else
      lnsppg = TermWin.nrow * 4 / 5;
#endif

      switch (keysym) {
	  /* normal XTerm key bindings */
	case XK_Prior:		/* Shift+Prior = scroll back */
	  if (TermWin.saveLines) {
	    scr_page(UP, lnsppg);
	    LK_RET();
	  }
	  break;

	case XK_Next:		/* Shift+Next = scroll forward */
	  if (TermWin.saveLines) {
	    scr_page(DN, lnsppg);
	    LK_RET();
	  }
	  break;

	case XK_Insert:	/* Shift+Insert = paste mouse selection */
	  selection_request(ev->xkey.time, ev->xkey.x, ev->xkey.y);
	  LK_RET();
	  break;

	  /* Eterm extras */
	case XK_KP_Add:	/* Shift+KP_Add = bigger font */
	  change_font(0, FONT_UP);
	  LK_RET();
	  break;

	case XK_KP_Subtract:	/* Shift+KP_Subtract = smaller font */
	  change_font(0, FONT_DN);
	  LK_RET();
	  break;
      }
    }
  }
#ifdef UNSHIFTED_SCROLLKEYS
  else if (!ctrl && !meta) {
    switch (keysym) {
      case XK_Prior:
	if (TermWin.saveLines) {
	  scr_page(UP, TermWin.nrow * 4 / 5);
	  LK_RET();
	}
	break;

      case XK_Next:
	if (TermWin.saveLines) {
	  scr_page(DN, TermWin.nrow * 4 / 5);
	  LK_RET();
	}
	break;
    }
  }
#endif

  switch (keysym) {
    case XK_Print:
#if DEBUG >= DEBUG_SELECTION
      if (debug_level >= DEBUG_SELECTION) {
	debug_selection();
      }
#endif
#ifdef PRINTPIPE
      scr_printscreen(ctrl | shft);
      LK_RET();
#endif
      break;

    case XK_Mode_switch:
#ifdef GREEK_SUPPORT
      greek_mode = !greek_mode;
      if (greek_mode) {
	xterm_seq(XTerm_title, (greek_getmode() == GREEK_ELOT928 ? "[Greek: iso]" : "[Greek: ibm]"));
	greek_reset();
      } else
	xterm_seq(XTerm_title, APL_NAME "-" VERSION);
      LK_RET();
#endif
      break;
  }

  if (keysym >= 0xFF00 && keysym <= 0xFFFF) {
#ifdef KEYSYM_ATTRIBUTE
    if (!(shft | ctrl) && KeySym_map[keysym - 0xFF00] != NULL) {

      const unsigned char *kbuf;
      unsigned int len;

      kbuf = (KeySym_map[keysym - 0xFF00]);
      len = *kbuf++;

      /* escape prefix */
      if (meta
# ifdef META8_OPTION
	  && (meta_char == 033)
# endif
	  ) {
	const unsigned char ch = '\033';

	tt_write(&ch, 1);
      }
      tt_write(kbuf, len);
      LK_RET();
    } else
#endif
      switch (keysym) {
	case XK_BackSpace:
	  len = 1;
#ifdef FORCE_BACKSPACE
	  kbuf[0] = (!(shft | ctrl) ? '\b' : '\177');
#elif defined(FORCE_DELETE)
	  kbuf[0] = ((shft | ctrl) ? '\b' : '\177');
#else
	  kbuf[0] = (((PrivateModes & PrivMode_BackSpace) ? !(shft | ctrl) : (shft | ctrl)) ? '\b' : '\177');
#endif
	  break;

	case XK_Tab:
	  if (shft) {
	    len = 3;
	    strcpy(kbuf, "\033[Z");
	  }
	  break;

#ifdef XK_KP_Home
	case XK_KP_Home:
	  /* allow shift to override */
	  if ((PrivateModes & PrivMode_aplKP) ? !shft : shft) {
	    len = 3;
	    strcpy(kbuf, "\033Ow");
	    break;
	  }
	  /* -> else FALL THROUGH */
#endif

	case XK_Home:
	  len = strlen(strcpy(kbuf, KS_HOME));
	  break;

#ifdef XK_KP_Left
	case XK_KP_Left:	/* \033Ot or standard */
	case XK_KP_Up:		/* \033Ox or standard */
	case XK_KP_Right:	/* \033Ov or standard */
	case XK_KP_Down:	/* \033Ow or standard */
	  if ((PrivateModes && PrivMode_aplKP) ? !shft : shft) {
	    len = 3;
	    strcpy(kbuf, "\033OZ");
	    kbuf[2] = ("txvw"[keysym - XK_KP_Left]);
	    break;
	  } else {
	    /* translate to std. cursor key */
	    keysym = XK_Left + (keysym - XK_KP_Left);
	  }
	  /* FALL THROUGH */
#endif
	case XK_Left:		/* "\033[D" */
	case XK_Up:		/* "\033[A" */
	case XK_Right:		/* "\033[C" */
	case XK_Down:		/* "\033[B" */
	  len = 3;
	  strcpy(kbuf, "\033[@");
	  kbuf[2] = ("DACB"[keysym - XK_Left]);
	  if (PrivateModes & PrivMode_aplCUR) {
	    kbuf[1] = 'O';
	  } else if (shft) {	/* do Shift first */
	    kbuf[2] = ("dacb"[keysym - XK_Left]);
	  } else if (ctrl) {
	    kbuf[1] = 'O';
	    kbuf[2] = ("dacb"[keysym - XK_Left]);
	  }
	  break;
#ifndef UNSHIFTED_SCROLLKEYS
# ifdef XK_KP_Prior
	case XK_KP_Prior:
	  /* allow shift to override */
	  if ((PrivateModes & PrivMode_aplKP) ? !shft : shft) {
	    len = 3;
	    strcpy(kbuf, "\033Oy");
	    break;
	  }
	  /* -> else FALL THROUGH */
# endif				/* XK_KP_Prior */
	case XK_Prior:
	  len = 4;
	  strcpy(kbuf, "\033[5~");
	  break;
# ifdef XK_KP_Next
	case XK_KP_Next:
	  /* allow shift to override */
	  if ((PrivateModes & PrivMode_aplKP) ? !shft : shft) {
	    len = 3;
	    strcpy(kbuf, "\033Os");
	    break;
	  }
	  /* -> else FALL THROUGH */
# endif				/* XK_KP_Next */
	case XK_Next:
	  len = 4;
	  strcpy(kbuf, "\033[6~");
	  break;
#endif /* UNSHIFTED_SCROLLKEYS */
#ifdef XK_KP_End
	case XK_KP_End:
	  /* allow shift to override */
	  if ((PrivateModes & PrivMode_aplKP) ? !shft : shft) {
	    len = 3;
	    strcpy(kbuf, "\033Oq");
	    break;
	  }
	  /* -> else FALL THROUGH */
#endif /* XK_KP_End */
	case XK_End:
	  len = strlen(strcpy(kbuf, KS_END));
	  break;

	case XK_Select:
	  len = 4;
	  strcpy(kbuf, "\033[4~");
	  break;

#ifdef DXK_Remove		/* support for DEC remove like key */
	case DXK_Remove:	/* drop */
#endif
	case XK_Execute:
	  len = 4;
	  strcpy(kbuf, "\033[3~");
	  break;
	case XK_Insert:
	  len = 4;
	  strcpy(kbuf, "\033[2~");
	  break;

	case XK_Menu:
	  len = 5;
	  strcpy(kbuf, "\033[29~");
	  break;
	case XK_Find:
	  len = 4;
	  strcpy(kbuf, "\033[1~");
	  break;
	case XK_Help:
	  len = 5;
	  strcpy(kbuf, "\033[28~");
	  break;

	case XK_KP_Enter:
	  /* allow shift to override */
	  if ((PrivateModes & PrivMode_aplKP) ? !shft : shft) {
	    len = 3;
	    strcpy(kbuf, "\033OM");
	  } else {
	    len = 1;
	    kbuf[0] = '\r';
	  }
	  break;

#ifdef XK_KP_Begin
	case XK_KP_Begin:
	  len = 3;
	  strcpy(kbuf, "\033Ou");
	  break;

	case XK_KP_Insert:
	  len = 3;
	  strcpy(kbuf, "\033Op");
	  break;

	case XK_KP_Delete:
	  len = 3;
	  strcpy(kbuf, "\033On");
	  break;
#endif /* XK_KP_Begin */

	case XK_KP_F1:		/* "\033OP" */
	case XK_KP_F2:		/* "\033OQ" */
	case XK_KP_F3:		/* "\033OR" */
	case XK_KP_F4:		/* "\033OS" */
	  len = 3;
	  strcpy(kbuf, "\033OP");
	  kbuf[2] += (keysym - XK_KP_F1);
	  break;

	case XK_KP_Multiply:	/* "\033Oj" : "*" */
	case XK_KP_Add:	/* "\033Ok" : "+" */
	case XK_KP_Separator:	/* "\033Ol" : "," */
	case XK_KP_Subtract:	/* "\033Om" : "-" */
	case XK_KP_Decimal:	/* "\033On" : "." */
	case XK_KP_Divide:	/* "\033Oo" : "/" */
	case XK_KP_0:		/* "\033Op" : "0" */
	case XK_KP_1:		/* "\033Oq" : "1" */
	case XK_KP_2:		/* "\033Or" : "2" */
	case XK_KP_3:		/* "\033Os" : "3" */
	case XK_KP_4:		/* "\033Ot" : "4" */
	case XK_KP_5:		/* "\033Ou" : "5" */
	case XK_KP_6:		/* "\033Ov" : "6" */
	case XK_KP_7:		/* "\033Ow" : "7" */
	case XK_KP_8:		/* "\033Ox" : "8" */
	case XK_KP_9:		/* "\033Oy" : "9" */
	  /* allow shift to override */
	  if ((PrivateModes & PrivMode_aplKP) ? !shft : shft) {
	    len = 3;
	    strcpy(kbuf, "\033Oj");
	    kbuf[2] += (keysym - XK_KP_Multiply);
	  } else {
	    len = 1;
	    kbuf[0] = ('*' + (keysym - XK_KP_Multiply));
	  }
	  break;

#define FKEY(n,fkey) do { \
len = 5; \
sprintf(kbuf,"\033[%02d~", (int)((n) + (keysym - fkey))); \
} while (0);

	case XK_F1:		/* "\033[11~" */
	case XK_F2:		/* "\033[12~" */
	case XK_F3:		/* "\033[13~" */
	case XK_F4:		/* "\033[14~" */
	case XK_F5:		/* "\033[15~" */
	  FKEY(11, XK_F1);
	  break;

	case XK_F6:		/* "\033[17~" */
	case XK_F7:		/* "\033[18~" */
	case XK_F8:		/* "\033[19~" */
	case XK_F9:		/* "\033[20~" */
	case XK_F10:		/* "\033[21~" */
	  FKEY(17, XK_F6);
	  break;

	case XK_F11:		/* "\033[23~" */
	case XK_F12:		/* "\033[24~" */
	case XK_F13:		/* "\033[25~" */
	case XK_F14:		/* "\033[26~" */
	  FKEY(23, XK_F11);
	  break;

	case XK_F15:		/* "\033[28~" */
	case XK_F16:		/* "\033[29~" */
	  FKEY(28, XK_F15);
	  break;

	case XK_F17:		/* "\033[31~" */
	case XK_F18:		/* "\033[32~" */
	case XK_F19:		/* "\033[33~" */
	case XK_F20:		/* "\033[34~" */
	case XK_F21:		/* "\033[35~" */
	case XK_F22:		/* "\033[36~" */
	case XK_F23:		/* "\033[37~" */
	case XK_F24:		/* "\033[38~" */
	case XK_F25:		/* "\033[39~" */
	case XK_F26:		/* "\033[40~" */
	case XK_F27:		/* "\033[41~" */
	case XK_F28:		/* "\033[42~" */
	case XK_F29:		/* "\033[43~" */
	case XK_F30:		/* "\033[44~" */
	case XK_F31:		/* "\033[45~" */
	case XK_F32:		/* "\033[46~" */
	case XK_F33:		/* "\033[47~" */
	case XK_F34:		/* "\033[48~" */
	case XK_F35:		/* "\033[49~" */
	  FKEY(31, XK_F17);
	  break;
#undef FKEY
#ifdef KS_DELETE
	case XK_Delete:
	  len = strlen(strcpy(kbuf, KS_DELETE));
	  break;
#endif
      }

#ifdef META8_OPTION
    if (meta && (meta_char == 0x80) && len > 0) {
      kbuf[len - 1] |= 0x80;
    }
#endif
  } else if (ctrl && keysym == XK_minus) {
    len = 1;
    kbuf[0] = '\037';		/* Ctrl-Minus generates ^_ (31) */
  } else {
#ifdef META8_OPTION
    /* set 8-bit on */
    if (meta && (meta_char == 0x80)) {

      unsigned char *ch;

      for (ch = kbuf; ch < kbuf + len; ch++)
	*ch |= 0x80;
      meta = 0;
    }
#endif
#ifdef GREEK_SUPPORT
    if (greek_mode)
      len = greek_xlat(kbuf, len);
#endif
  }

#ifdef USE_XIM
  }
#endif

  if (len <= 0)
    LK_RET();			/* not mapped */

  /*
   * these modifications only affect the static keybuffer
   * pass Shift/Control indicators for function keys ending with `~'
   *
   * eg,
   *  Prior = "ESC[5~"
   *  Shift+Prior = "ESC[5~"
   *  Ctrl+Prior = "ESC[5^"
   *  Ctrl+Shift+Prior = "ESC[5@"
   */
  if (kbuf[0] == '\033' && kbuf[1] == '[' && kbuf[len - 1] == '~')
    kbuf[len - 1] = (shft ? (ctrl ? '@' : '$') : (ctrl ? '^' : '~'));

  /* escape prefix */
  if (meta
#ifdef META8_OPTION
      && (meta_char == 033)
#endif
      ) {

    const unsigned char ch = '\033';

    tt_write(&ch, 1);
  }
#if DEBUG >= DEBUG_CMD
  if (debug_level >= DEBUG_CMD && debug_key) {	/* Display keyboard buffer contents */

    char *p;
    int i;

    fprintf(stderr, "key 0x%04X[%d]: `", (unsigned int) keysym, len);
    for (i = 0, p = kbuf; i < len; i++, p++)
      fprintf(stderr, (*p >= ' ' && *p < '\177' ? "%c" : "\\%03o"), *p);
    fprintf(stderr, "'\n");
  }
#endif /* DEBUG_CMD */
  tt_write(kbuf, len);

  LK_RET();
}

/* print pipe */
#ifdef PRINTPIPE
FILE *
popen_printer(void)
{
  FILE *stream = popen(rs_print_pipe, "w");

  if (stream == NULL)
    print_error("can't open printer pipe \"%s\" -- %s", rs_print_pipe, strerror(errno));
  return stream;
}

int
pclose_printer(FILE * stream)
{
  fflush(stream);
  return pclose(stream);
}

/* simulate attached vt100 printer */
void
process_print_pipe(void)
{
  const char *const escape_seq = "\033[4i";
  const char *const rev_escape_seq = "i4[\033";
  int index;
  FILE *fd;

  if ((fd = popen_printer()) != NULL) {
    for (index = 0; index < 4; /* nil */ ) {
      unsigned char ch = cmd_getc();

      if (ch == escape_seq[index])
	index++;
      else if (index)
	for ( /*nil */ ; index > 0; index--)
	  fputc(rev_escape_seq[index - 1], fd);

      if (index == 0)
	fputc(ch, fd);
    }
    pclose_printer(fd);
  }
}
#endif /* PRINTPIPE */

/* process escape sequences */
void
process_escape_seq(void)
{
  unsigned char ch = cmd_getc();

  switch (ch) {
      /* case 1:        do_tek_mode (); break; */
    case '#':
      if (cmd_getc() == '8')
	scr_E();
      break;
    case '(':
      scr_charset_set(0, cmd_getc());
      break;
    case ')':
      scr_charset_set(1, cmd_getc());
      break;
    case '*':
      scr_charset_set(2, cmd_getc());
      break;
    case '+':
      scr_charset_set(3, cmd_getc());
      break;
#ifdef MULTI_CHARSET
    case '$':
      scr_charset_set(-2, cmd_getc());
      break;
#endif
    case '7':
      scr_cursor(SAVE);
      break;
    case '8':
      scr_cursor(RESTORE);
      break;
    case '=':
    case '>':
      PrivMode((ch == '='), PrivMode_aplKP);
      break;
    case '@':
      (void) cmd_getc();
      break;
    case 'D':
      scr_index(UP);
      break;
    case 'E':
      scr_add_lines("\n\r", 1, 2);
      break;
    case 'G':
      process_graphics();
      break;
    case 'H':
      scr_set_tab(1);
      break;
    case 'M':
      scr_index(DN);
      break;
      /*case 'N': scr_single_shift (2);   break; */
      /*case 'O': scr_single_shift (3);   break; */
    case 'Z':
      tt_printf(ESCZ_ANSWER);
      break;			/* steal obsolete ESC [ c */
    case '[':
      process_csi_seq();
      break;
    case ']':
      process_xterm_seq();
      break;
    case 'c':
      scr_poweron();
      break;
    case 'n':
      scr_charset_choose(2);
      break;
    case 'o':
      scr_charset_choose(3);
      break;
  }
}

/* process CSI (code sequence introducer) sequences `ESC[' */
void
process_csi_seq(void)
{

  unsigned char ch, priv;
  unsigned int nargs;
  int arg[ESC_ARGS];

  nargs = 0;
  arg[0] = 0;
  arg[1] = 0;

  priv = 0;
  ch = cmd_getc();
  if (ch >= '<' && ch <= '?') {
    priv = ch;
    ch = cmd_getc();
  }
  /* read any numerical arguments */
  do {
    int n;

    for (n = 0; isdigit(ch); ch = cmd_getc())
      n = n * 10 + (ch - '0');

    if (nargs < ESC_ARGS)
      arg[nargs++] = n;
    if (ch == '\b') {
      scr_backspace();
    } else if (ch == 033) {
      process_escape_seq();
      return;
    } else if (ch < ' ') {
      scr_add_lines(&ch, 0, 1);
      return;
    }
    if (ch < '@')
      ch = cmd_getc();
  }
  while (ch >= ' ' && ch < '@');
  if (ch == 033) {
    process_escape_seq();
    return;
  } else if (ch < ' ')
    return;

  switch (ch) {
#ifdef PRINTPIPE
    case 'i':			/* printing */
      switch (arg[0]) {
	case 0:
	  scr_printscreen(0);
	  break;
	case 5:
	  process_print_pipe();
	  break;
      }
      break;
#endif
    case 'A':
    case 'e':			/* up <n> */
      scr_gotorc((arg[0] ? -arg[0] : -1), 0, RELATIVE);
      break;
    case 'B':			/* down <n> */
      scr_gotorc((arg[0] ? +arg[0] : +1), 0, RELATIVE);
      break;
    case 'C':
    case 'a':			/* right <n> */
      scr_gotorc(0, (arg[0] ? +arg[0] : +1), RELATIVE);
      break;
    case 'D':			/* left <n> */
      scr_gotorc(0, (arg[0] ? -arg[0] : -1), RELATIVE);
      break;
    case 'E':			/* down <n> & to first column */
      scr_gotorc((arg[0] ? +arg[0] : +1), 0, R_RELATIVE);
      break;
    case 'F':			/* up <n> & to first column */
      scr_gotorc((arg[0] ? -arg[0] : -1), 0, R_RELATIVE);
      break;
    case 'G':
    case '`':			/* move to col <n> */
      scr_gotorc(0, (arg[0] ? arg[0] - 1 : +1), R_RELATIVE);
      break;
    case 'd':			/* move to row <n> */
      scr_gotorc((arg[0] ? arg[0] - 1 : +1), 0, C_RELATIVE);
      break;
    case 'H':
    case 'f':			/* position cursor */
      switch (nargs) {
	case 0:
	  scr_gotorc(0, 0, 0);
	  break;
	case 1:
	  scr_gotorc((arg[0] ? arg[0] - 1 : 0), 0, 0);
	  break;
	default:
	  scr_gotorc(arg[0] - 1, arg[1] - 1, 0);
	  break;
      }
      break;
    case 'I':
      scr_tab(arg[0] ? +arg[0] : +1);
      break;
    case 'Z':
      scr_tab(arg[0] ? -arg[0] : -1);
      break;
    case 'J':
      scr_erase_screen(arg[0]);
      break;
    case 'K':
      scr_erase_line(arg[0]);
      break;
    case '@':
      scr_insdel_chars((arg[0] ? arg[0] : 1), INSERT);
      break;
    case 'L':
      scr_insdel_lines((arg[0] ? arg[0] : 1), INSERT);
      break;
    case 'M':
      scr_insdel_lines((arg[0] ? arg[0] : 1), DELETE);
      break;
    case 'X':
      scr_insdel_chars((arg[0] ? arg[0] : 1), ERASE);
      break;
    case 'P':
      scr_insdel_chars((arg[0] ? arg[0] : 1), DELETE);
      break;

    case 'c':
#ifndef NO_VT100_ANS
      tt_printf(VT100_ANS);
#endif
      break;
    case 'm':
      process_sgr_mode(nargs, arg);
      break;
    case 'n':			/* request for information */
      switch (arg[0]) {
	case 5:
	  tt_printf("\033[0n");
	  break;		/* ready */
	case 6:
	  scr_report_position();
	  break;
#if defined (ENABLE_DISPLAY_ANSWER)
	case 7:
	  tt_printf("%s\n", display_name);
	  break;
#endif
	case 8:
	  xterm_seq(XTerm_title, APL_NAME "-" VERSION);
	  break;
	case 9:
#ifdef PIXMAP_OFFSET
	  if (Options & Opt_pixmapTrans) {
	    char tbuff[70];
	    char shading = 0;
	    unsigned long tint = 0xffffff;

	    if (images[image_bg].current->iml->mod) {
	      shading = images[image_bg].current->iml->mod->brightness / 0xff * 100;
	    }
	    if (images[image_bg].current->iml->rmod) {
	      tint = (tint & 0x00ffff) | ((images[image_bg].current->iml->rmod->brightness & 0xff) << 16);
	    }
	    if (images[image_bg].current->iml->gmod) {
	      tint = (tint & 0xff00ff) | ((images[image_bg].current->iml->gmod->brightness & 0xff) << 8);
	    }
	    if (images[image_bg].current->iml->bmod) {
	      tint = (tint & 0xffff00) | (images[image_bg].current->iml->bmod->brightness & 0xff);
	    }
	    snprintf(tbuff, sizeof(tbuff), APL_NAME "-" VERSION ":  Transparent - %d%% shading - 0x%06lx tint mask",
		     shading, tint);
	    xterm_seq(XTerm_title, tbuff);
	  } else
#endif
#ifdef PIXMAP_SUPPORT
	  {
	    char *tbuff;
	    unsigned short len;

	    if (background_is_pixmap()) {
	      char *fname = images[image_bg].current->iml->im->filename;

	      len = strlen(fname) + sizeof(APL_NAME) + sizeof(VERSION) + 5;
	      tbuff = MALLOC(len);
	      snprintf(tbuff, len, APL_NAME "-" VERSION ":  %s", fname);
	      xterm_seq(XTerm_title, tbuff);
	      FREE(tbuff);
	    } else {
	      xterm_seq(XTerm_title, APL_NAME "-" VERSION ":  No Pixmap");
	    }
	  }
#endif /* PIXMAP_SUPPORT */
	  break;
      }
      break;
    case 'r':			/* set top and bottom margins */
      if (priv != '?') {
	if (nargs < 2 || arg[0] >= arg[1])
	  scr_scroll_region(0, 10000);
	else
	  scr_scroll_region(arg[0] - 1, arg[1] - 1);
	break;
      }
      /* drop */
    case 't':
      if (priv != '?') {
	process_window_mode(nargs, arg);
	break;
      }
      /* drop */
    case 's':
      if (ch == 's' && !nargs) {
        scr_cursor(SAVE);
        break;
      }
      /* drop */
    case 'h':
    case 'l':
      process_terminal_mode(ch, priv, nargs, arg);
      break;
    case 'u':
      if (!nargs) {
        scr_cursor(RESTORE);
      }
      break;
    case 'g':
      switch (arg[0]) {
	case 0:
	  scr_set_tab(0);
	  break;		/* delete tab */
	case 3:
	  scr_set_tab(-1);
	  break;		/* clear all tabs */
      }
      break;
    case 'W':
      switch (arg[0]) {
	case 0:
	  scr_set_tab(1);
	  break;		/* = ESC H */
	case 2:
	  scr_set_tab(0);
	  break;		/* = ESC [ 0 g */
	case 5:
	  scr_set_tab(-1);
	  break;		/* = ESC [ 3 g */
      }
      break;
  }
}

/* process xterm text parameters sequences `ESC ] Ps ; Pt BEL' */
void
process_xterm_seq(void)
{
  unsigned char ch, string[STRING_MAX];
  int arg;

  ch = cmd_getc();
  if (isdigit(ch)) {
    for (arg = 0; isdigit(ch); ch = cmd_getc()) {
      arg = arg * 10 + (ch - '0');
    }
  } else if (ch == ';') {
    arg = 0;
  } else {
    arg = ch;
    ch = cmd_getc();
  }
  if (ch == ';') {
    unsigned long n = 0;

    while ((ch = cmd_getc()) != 007) {
      if (ch) {
	if (ch == '\t')
	  ch = ' ';		/* translate '\t' to space */
	else if ((ch < ' ') && !(isspace(ch) && arg == XTerm_EtermIPC))
	  return;		/* control character - exit */

	if (n < sizeof(string) - 1)
	  string[n++] = ch;
      }
    }
    string[n] = '\0';
    xterm_seq(arg, string);

  } else {
    unsigned long n = 0;

    for (; ch != '\e'; ch = cmd_getc()) {
      if (ch) {
	if (ch == '\t')
	  ch = ' ';		/* translate '\t' to space */
	else if (ch < ' ')
	  return;		/* control character - exit */

	if (n < sizeof(string) - 1)
	  string[n++] = ch;
      }
    }
    string[n] = '\0';

    if ((ch = cmd_getc()) != '\\') {
      return;
    }
    switch (arg) {
      case 'l':
	xterm_seq(XTerm_title, string);
	break;
      case 'L':
	xterm_seq(XTerm_iconName, string);
	break;
      case 'I':
	set_icon_pixmap(string, NULL);
	break;
      default:
	break;
    }
  }
}

/* Process window manipulations */
void
process_window_mode(unsigned int nargs, int args[])
{

  register unsigned int i;
  unsigned int x, y;
  Screen *scr;
  Window dummy_child;
  char buff[128], *name;

  if (!nargs)
    return;
  scr = ScreenOfDisplay(Xdisplay, Xscreen);
  if (!scr)
    return;

  for (i = 0; i < nargs; i++) {
    if (args[i] == 14) {
      int dummy_x, dummy_y;
      unsigned int dummy_border, dummy_depth;

      /* Store current width and height in x and y */
      XGetGeometry(Xdisplay, TermWin.parent, &dummy_child, &dummy_x, &dummy_y, &x, &y, &dummy_border, &dummy_depth);
    }
    switch (args[i]) {
      case 1:
	XRaiseWindow(Xdisplay, TermWin.parent);
	break;
      case 2:
	XIconifyWindow(Xdisplay, TermWin.parent, Xscreen);
	break;
      case 3:
	if (i + 2 >= nargs)
	  return;		/* Make sure there are 2 args left */
	x = args[++i];
	y = args[++i];
	if (x > (unsigned long) scr->width || y > (unsigned long) scr->height)
	  return;		/* Don't move off-screen */
	XMoveWindow(Xdisplay, TermWin.parent, x, y);
	break;
      case 4:
	if (i + 2 >= nargs)
	  return;		/* Make sure there are 2 args left */
	y = args[++i];
	x = args[++i];
	XResizeWindow(Xdisplay, TermWin.parent, x, y);
#ifdef USE_XIM
	xim_set_status_position();
#endif
	break;
      case 5:
	XRaiseWindow(Xdisplay, TermWin.parent);
	break;
      case 6:
	XLowerWindow(Xdisplay, TermWin.parent);
	break;
      case 7:
	XClearWindow(Xdisplay, TermWin.vt);
	XSync(Xdisplay, False);
	scr_touch();
	scr_refresh(DEFAULT_REFRESH);
	break;
      case 8:
	if (i + 2 >= nargs)
	  return;		/* Make sure there are 2 args left */
	y = args[++i];
	x = args[++i];
	XResizeWindow(Xdisplay, TermWin.parent,
		      Width2Pixel(x) + 2 * TermWin.internalBorder + (scrollbar_visible()? scrollbar_trough_width() : 0),
		      Height2Pixel(y) + 2 * TermWin.internalBorder);
	break;
      case 11:
	break;
      case 13:
	XTranslateCoordinates(Xdisplay, TermWin.parent, Xroot, 0, 0, &x, &y, &dummy_child);
	snprintf(buff, sizeof(buff), "\e[3;%d;%dt", x, y);
	tt_write(buff, strlen(buff));
	break;
      case 14:
	snprintf(buff, sizeof(buff), "\e[4;%d;%dt", y, x);
	tt_write(buff, strlen(buff));
	break;
      case 18:
	snprintf(buff, sizeof(buff), "\e[8;%d;%dt", TermWin.nrow, TermWin.ncol);
	tt_write(buff, strlen(buff));
	break;
      case 20:
	XGetIconName(Xdisplay, TermWin.parent, &name);
	snprintf(buff, sizeof(buff), "\e]L%s\e\\", name);
	tt_write(buff, strlen(buff));
	XFree(name);
	break;
      case 21:
	XFetchName(Xdisplay, TermWin.parent, &name);
	snprintf(buff, sizeof(buff), "\e]l%s\e\\", name);
	tt_write(buff, strlen(buff));
	XFree(name);
	break;
      default:
	break;
    }
  }
}

/* process DEC private mode sequences `ESC [ ? Ps mode' */
/*
 * mode can only have the following values:
 *      'l' = low
 *      'h' = high
 *      's' = save
 *      'r' = restore
 *      't' = toggle
 */
void
process_terminal_mode(int mode, int priv, unsigned int nargs, int arg[])
{
  unsigned int i;
  int state;

  if (nargs == 0)
    return;

  /* make lo/hi boolean */
  switch (mode) {
    case 'l':
      mode = 0;
      break;
    case 'h':
      mode = 1;
      break;
  }

  switch (priv) {
    case 0:
      if (mode && mode != 1)
	return;			/* only do high/low */
      for (i = 0; i < nargs; i++)
	switch (arg[i]) {
	  case 4:
	    scr_insert_mode(mode);
	    break;
	    /* case 38:  TEK mode */
	}
      break;

    case '?':
      for (i = 0; i < nargs; i++)
	switch (arg[i]) {
	  case 1:		/* application cursor keys */
	    PrivCases(PrivMode_aplCUR);
	    break;

	    /* case 2:   - reset charsets to USASCII */

	  case 3:		/* 80/132 */
	    PrivCases(PrivMode_132);
	    if (PrivateModes & PrivMode_132OK)
	      set_width(state ? 132 : 80);
	    break;

	    /* case 4:   - smooth scrolling */

	  case 5:		/* reverse video */
	    PrivCases(PrivMode_rVideo);
	    scr_rvideo_mode(state);
	    break;

	  case 6:		/* relative/absolute origins  */
	    PrivCases(PrivMode_relOrigin);
	    scr_relative_origin(state);
	    break;

	  case 7:		/* autowrap */
	    PrivCases(PrivMode_Autowrap);
	    scr_autowrap(state);
	    break;

	    /* case 8:   - auto repeat, can't do on a per window basis */

	  case 9:		/* X10 mouse reporting */
	    PrivCases(PrivMode_MouseX10);
	    /* orthogonal */
	    if (PrivateModes & PrivMode_MouseX10)
	      PrivateModes &= ~(PrivMode_MouseX11);
	    break;

#ifdef scrollBar_esc
	  case scrollBar_esc:
	    PrivCases(PrivMode_scrollBar);
	    map_scrollbar(state);
	    break;
#endif
	  case 25:		/* visible/invisible cursor */
	    PrivCases(PrivMode_VisibleCursor);
	    scr_cursor_visible(state);
	    break;

	  case 35:
	    PrivCases(PrivMode_ShiftKeys);
	    break;

	  case 40:		/* 80 <--> 132 mode */
	    PrivCases(PrivMode_132OK);
	    break;

	  case 47:		/* secondary screen */
	    PrivCases(PrivMode_Screen);
	    scr_change_screen(state);
	    break;

	  case 66:		/* application key pad */
	    PrivCases(PrivMode_aplKP);
	    break;

	  case 67:
	    PrivCases(PrivMode_BackSpace);
	    break;

	  case 1000:		/* X11 mouse reporting */
	    PrivCases(PrivMode_MouseX11);
	    /* orthogonal */
	    if (PrivateModes & PrivMode_MouseX11)
	      PrivateModes &= ~(PrivMode_MouseX10);
	    break;

#if 0
	  case 1001:
	    break;		/* X11 mouse highlighting */
#endif
	  case 1010:		/* Scroll to bottom on TTY output */
	    if (Options & Opt_homeOnEcho)
	      Options &= ~Opt_homeOnEcho;
	    else
	      Options |= Opt_homeOnEcho;
	    break;
	  case 1011:		/* scroll to bottom on refresh */
	    if (Options & Opt_homeOnRefresh)
	      Options &= ~Opt_homeOnRefresh;
	    else
	      Options |= Opt_homeOnRefresh;
	    break;
	  case 1012:		/* Scroll to bottom on TTY input */
	    if (Options & Opt_homeOnInput)
	      Options &= ~Opt_homeOnInput;
	    else
	      Options |= Opt_homeOnInput;
	    break;
	}
      break;
  }
}

/* process sgr sequences */
void
process_sgr_mode(unsigned int nargs, int arg[])
{
  unsigned int i;

  if (nargs == 0) {
    scr_rendition(0, ~RS_None);
    return;
  }
  for (i = 0; i < nargs; i++)
    switch (arg[i]) {
      case 0:
	scr_rendition(0, ~RS_None);
	break;
      case 1:
	scr_rendition(1, RS_Bold);
	break;
      case 4:
	scr_rendition(1, RS_Uline);
	break;
      case 5:
	scr_rendition(1, RS_Blink);
	break;
      case 7:
	scr_rendition(1, RS_RVid);
	break;
      case 22:
	scr_rendition(0, RS_Bold);
	break;
      case 24:
	scr_rendition(0, RS_Uline);
	break;
      case 25:
	scr_rendition(0, RS_Blink);
	break;
      case 27:
	scr_rendition(0, RS_RVid);
	break;

      case 30:
      case 31:			/* set fg color */
      case 32:
      case 33:
      case 34:
      case 35:
      case 36:
      case 37:
	scr_color(minColor + (arg[i] - 30), RS_Bold);
	break;
      case 39:			/* default fg */
	scr_color(restoreFG, RS_Bold);
	break;

      case 40:
      case 41:			/* set bg color */
      case 42:
      case 43:
      case 44:
      case 45:
      case 46:
      case 47:
	scr_color(minColor + (arg[i] - 40), RS_Blink);
	break;
      case 49:			/* default bg */
	scr_color(restoreBG, RS_Blink);
	break;
    }
}

/* process Rob Nation's own graphics mode sequences */
void
process_graphics(void)
{
  unsigned char ch, cmd = cmd_getc();

#ifndef RXVT_GRAPHICS
  if (cmd == 'Q') {		/* query graphics */
    tt_printf("\033G0\n");	/* no graphics */
    return;
  }
  /* swallow other graphics sequences until terminating ':' */
  do
    ch = cmd_getc();
  while (ch != ':');
#else
  int nargs;
  int args[NGRX_PTS];
  unsigned char *text = NULL;

  if (cmd == 'Q') {		/* query graphics */
    tt_printf("\033G1\n");	/* yes, graphics (color) */
    return;
  }
  for (nargs = 0; nargs < (sizeof(args) / sizeof(args[0])) - 1; /*nil */ ) {
    int neg;

    ch = cmd_getc();
    neg = (ch == '-');
    if (neg || ch == '+')
      ch = cmd_getc();

    for (args[nargs] = 0; isdigit(ch); ch = cmd_getc())
      args[nargs] = args[nargs] * 10 + (ch - '0');
    if (neg)
      args[nargs] = -args[nargs];

    nargs++;
    args[nargs] = 0;
    if (ch != ';')
      break;
  }

  if ((cmd == 'T') && (nargs >= 5)) {
    int i, len = args[4];

    text = MALLOC((len + 1) * sizeof(char));

    if (text != NULL) {
      for (i = 0; i < len; i++)
	text[i] = cmd_getc();
      text[len] = '\0';
    }
  }
  Gr_do_graphics(cmd, nargs, args, text);
#endif
}

/* color aliases, fg/bg bright-bold */
void
color_aliases(int idx)
{

  if (rs_color[idx] && isdigit(*rs_color[idx])) {

    int i = atoi(rs_color[idx]);

    if (i >= 8 && i <= 15) {	/* bright colors */
      i -= 8;
#ifndef NO_BRIGHTCOLOR
      rs_color[idx] = rs_color[minBright + i];
      return;
#endif
    }
    if (i >= 0 && i <= 7)	/* normal colors */
      rs_color[idx] = rs_color[minColor + i];
  }
}

/* find if fg/bg matches any of the normal (low-intensity) colors */
#ifndef NO_BRIGHTCOLOR
void
set_colorfgbg(void)
{
  unsigned int i;
  static char *colorfgbg_env = NULL;
  char *p;
  int fg = -1, bg = -1;

  if (!colorfgbg_env) {
    colorfgbg_env = (char *) malloc(30);
    strcpy(colorfgbg_env, "COLORFGBG=default;default;bg");
  }
  for (i = BlackColor; i <= WhiteColor; i++) {
    if (PixColors[fgColor] == PixColors[i]) {
      fg = (i - BlackColor);
      break;
    }
  }
  for (i = BlackColor; i <= WhiteColor; i++) {
    if (PixColors[bgColor] == PixColors[i]) {
      bg = (i - BlackColor);
      break;
    }
  }

  p = strchr(colorfgbg_env, '=');
  p++;
  if (fg >= 0)
    sprintf(p, "%d;", fg);
  else
    strcpy(p, "default;");
  p = strchr(p, '\0');
  if (bg >= 0)
    sprintf(p,
# ifdef PIXMAP_SUPPORT
	    "default;"
# endif
	    "%d", bg);
  else
    strcpy(p, "default");
  putenv(colorfgbg_env);

  colorfgbg = DEFAULT_RSTYLE;
  for (i = minColor; i <= maxColor; i++) {
    if (PixColors[fgColor] == PixColors[i]
# ifndef NO_BOLDUNDERLINE
	&& PixColors[fgColor] == PixColors[colorBD]
# endif				/* NO_BOLDUNDERLINE */
    /* if we wanted boldFont to have precedence */
# if 0				/* ifndef NO_BOLDFONT */
	&& TermWin.boldFont == NULL
# endif				/* NO_BOLDFONT */
	)
      colorfgbg = SET_FGCOLOR(colorfgbg, i);
    if (PixColors[bgColor] == PixColors[i])
      colorfgbg = SET_BGCOLOR(colorfgbg, i);
  }
}
#endif /* NO_BRIGHTCOLOR */

/* xterm sequences - title, iconName, color (exptl) */
#ifdef SMART_WINDOW_TITLE
static void
set_title(const char *str)
{

  char *name;

  if (XFetchName(Xdisplay, TermWin.parent, &name))
    name = NULL;
  if (name == NULL || strcmp(name, str))
    XStoreName(Xdisplay, TermWin.parent, str);
  if (name)
    XFree(name);
}
#else
# define set_title(str) XStoreName(Xdisplay, TermWin.parent, str)
#endif

#ifdef SMART_WINDOW_TITLE
static void
set_iconName(const char *str)
{

  char *name;

  if (XGetIconName(Xdisplay, TermWin.parent, &name))
    name = NULL;
  if (name == NULL || strcmp(name, str))
    XSetIconName(Xdisplay, TermWin.parent, str);
  if (name)
    XFree(name);
}
#else
# define set_iconName(str) XSetIconName(Xdisplay, TermWin.parent, str)
#endif

/*
 * XTerm escape sequences: ESC ] Ps;Pt BEL
 *       0 = change iconName/title
 *       1 = change iconName
 *       2 = change title
 *      46 = change logfile (not implemented)
 *      50 = change font
 *
 * rxvt/Eterm extensions:
 *       5 = Hostile takeover (grab focus and raise)
 *       6 = Transparency mode stuff
 *      10 = menu
 *      20 = bg pixmap
 *      39 = change default fg color
 *      49 = change default bg color
 */
void
xterm_seq(int op, const char *str)
{

  XColor xcol;
  char *nstr, *tnstr, *orig_tnstr;
  unsigned char eterm_seq_op;
#ifdef PIXMAP_SUPPORT
  unsigned char changed = 0, scaled = 0;
  char *color, *mod, *valptr;
#endif

  if (!str)
    return;

#ifdef PIXMAP_SUPPORT
  orig_tnstr = tnstr = StrDup(str);
#endif

  switch (op) {
    case XTerm_title:
      set_title(str);
      break;
    case XTerm_name:
      set_title(str);		/* drop */
    case XTerm_iconName:
      set_iconName(str);
      break;
    case XTerm_Takeover:
      XSetInputFocus(Xdisplay, TermWin.parent, RevertToParent, CurrentTime);
      XRaiseWindow(Xdisplay, TermWin.parent);
      break;

    case XTerm_EtermSeq:

      /* Eterm proprietary escape sequences

         Syntax:  ESC ] 6 ; <op> ; <arg> BEL

         where <op> is:  0    Set/toggle transparency
         1    Set color modifiers
         3    Force update of pseudo-transparent background
         10    Set scrollbar type/width
         11    Set/toggle right-side scrollbar
         12    Set/toggle floating scrollbar
         13    Set/toggle popup scrollbar
         20    Set/toggle visual bell
         21    Set/toggle map alert
         22    Set/toggle xterm selection behavior
         23    Set/toggle triple-click line selection
         24    Set/toggle viewport mode
         25    Set/toggle selection of trailing spaces
         30    Do not use
         40    Do not use
         50    Move window to another desktop
         70    Exit Eterm
         71    Save current configuration to a file
         and <arg> is an optional argument, depending
         on the particular sequence being used.  It
         (along with its preceeding semicolon) may or
         may not be needed.
       */

      D_EVENTS(("Got XTerm_EtermSeq sequence\n"));
      nstr = strsep(&tnstr, ";");
      eterm_seq_op = (unsigned char) strtol(nstr, (char **) NULL, 10);
      D_EVENTS(("    XTerm_EtermSeq operation is %d\n", eterm_seq_op));
      /* Yes, there is order to the numbers for this stuff.  And here it is:
         0-9      Transparency Configuration
         10-14    Scrollbar Configuration
         15-19    Menu Configuration
         20-29    Miscellaneous Toggles
         30-39    Foreground/Text Color Configuration
         40-49    Background Color Configuration
         50-69    Window/Window Manager Configuration/Interaction
         70-79    Internal Eterm Operations
       */
      switch (eterm_seq_op) {
#ifdef PIXMAP_OFFSET
	case 0:
	  nstr = strsep(&tnstr, ";");
	  OPT_SET_OR_TOGGLE_NEG(nstr, Options, Opt_pixmapTrans);
	  if (Options & Opt_pixmapTrans) {
	    Options &= ~(Opt_pixmapTrans);
	  } else {
	    Options |= Opt_pixmapTrans;
	    if (images[image_bg].current->pmap->pixmap != None) {
	      Imlib_free_pixmap(imlib_id, images[image_bg].current->pmap->pixmap);
	    }
	    images[image_bg].current->pmap->pixmap = None;
	  }
	  render_simage(images[image_bg].current, TermWin.vt, TermWin_TotalWidth(), TermWin_TotalHeight(), image_bg, 1);
	  scr_touch();
	  render_simage(images[image_sb].current, scrollBar.win, scrollbar_trough_width(), scrollbar_trough_height(), image_sb, 0);
	  scrollbar_show(0);
	  break;
	case 1:
          color = strsep(&tnstr, ";");
          if (!color) {
            break;
          }
          mod = strsep(&tnstr, ";");
          if (!mod) {
            break;
          }
          valptr = strsep(&tnstr, ";");
          if (!valptr) {
            break;
          }
	  D_EVENTS(("Modifying the %s attribute of the %s color modifier to be %s\n", mod, color, valptr));
	  if (Options & Opt_pixmapTrans && desktop_pixmap != None) {
	    free_desktop_pixmap();
	    desktop_pixmap = None;	/* Force the re-read */
	  }
	  if (Options & Opt_viewport_mode && viewport_pixmap != None) {
	    XFreePixmap(Xdisplay, viewport_pixmap);
	    viewport_pixmap = None;	/* Force the re-read */
	  }
          if (!strcasecmp(color, "image")) {
            imlib_t *iml = images[image_bg].current->iml;

            if (iml->mod == NULL) {
              iml->mod = (ImlibColorModifier *) MALLOC(sizeof(ImlibColorModifier));
              iml->mod->brightness = iml->mod->contrast = iml->mod->gamma = 0xff;
            }
            if (!BEG_STRCASECMP("brightness", mod)) {
              iml->mod->brightness = (int) strtol(valptr, (char **) NULL, 0);
            } else if (!BEG_STRCASECMP("contrast", mod)) {
              iml->mod->contrast = (int) strtol(valptr, (char **) NULL, 0);
            } else if (!BEG_STRCASECMP("gamma", mod)) {
              iml->mod->gamma = (int) strtol(valptr, (char **) NULL, 0);
            }

          } else if (!strcasecmp(color, "red")) {
            imlib_t *iml = images[image_bg].current->iml;

            if (iml->rmod == NULL) {
              iml->rmod = (ImlibColorModifier *) MALLOC(sizeof(ImlibColorModifier));
              iml->rmod->brightness = iml->rmod->contrast = iml->rmod->gamma = 0xff;
            }
            if (!BEG_STRCASECMP("brightness", mod)) {
              iml->rmod->brightness = (int) strtol(valptr, (char **) NULL, 0);
            } else if (!BEG_STRCASECMP("contrast", mod)) {
              iml->rmod->contrast = (int) strtol(valptr, (char **) NULL, 0);
            } else if (!BEG_STRCASECMP("gamma", mod)) {
              iml->rmod->gamma = (int) strtol(valptr, (char **) NULL, 0);
            }

          } else if (!strcasecmp(color, "green")) {
            imlib_t *iml = images[image_bg].current->iml;

            if (iml->gmod == NULL) {
              iml->gmod = (ImlibColorModifier *) MALLOC(sizeof(ImlibColorModifier));
              iml->gmod->brightness = iml->gmod->contrast = iml->gmod->gamma = 0xff;
            }
            if (!BEG_STRCASECMP("brightness", mod)) {
              iml->gmod->brightness = (int) strtol(valptr, (char **) NULL, 0);
            } else if (!BEG_STRCASECMP("contrast", mod)) {
              iml->gmod->contrast = (int) strtol(valptr, (char **) NULL, 0);
            } else if (!BEG_STRCASECMP("gamma", mod)) {
              iml->gmod->gamma = (int) strtol(valptr, (char **) NULL, 0);
            }

          } else if (!strcasecmp(color, "blue")) {
            imlib_t *iml = images[image_bg].current->iml;

            if (iml->bmod == NULL) {
              iml->bmod = (ImlibColorModifier *) MALLOC(sizeof(ImlibColorModifier));
              iml->bmod->brightness = iml->bmod->contrast = iml->bmod->gamma = 0xff;
            }
            if (!BEG_STRCASECMP("brightness", mod)) {
              iml->bmod->brightness = (int) strtol(valptr, (char **) NULL, 0);
            } else if (!BEG_STRCASECMP("contrast", mod)) {
              iml->bmod->contrast = (int) strtol(valptr, (char **) NULL, 0);
            } else if (!BEG_STRCASECMP("gamma", mod)) {
              iml->bmod->gamma = (int) strtol(valptr, (char **) NULL, 0);
            }
          }
          
	  render_simage(images[image_bg].current, TermWin.vt, TermWin_TotalWidth(), TermWin_TotalHeight(), image_bg, 1);
	  scr_touch();
	  break;
	case 3:
	  if (Options & Opt_pixmapTrans) {
	    get_desktop_window();
	    if (desktop_pixmap != None) {
	      free_desktop_pixmap();
	    }
	    render_simage(images[image_bg].current, TermWin.vt, TermWin_TotalWidth(), TermWin_TotalHeight(), image_bg, 1);
	    scr_touch();
	  }
	  break;
#endif
	case 10:
	  nstr = strsep(&tnstr, ";");
	  if (nstr && *nstr) {
	    if (!strcasecmp(nstr, "xterm")) {
#ifdef XTERM_SCROLLBAR
	      scrollBar.type = SCROLLBAR_XTERM;
#else
	      print_error("Support for xterm scrollbars was not compiled in.  Sorry.");
#endif
	    } else if (!strcasecmp(nstr, "next")) {
#ifdef NEXT_SCROLLBAR
	      scrollBar.type = SCROLLBAR_NEXT;
#else
	      print_error("Support for NeXT scrollbars was not compiled in.  Sorry.");
#endif
	    } else if (!strcasecmp(nstr, "motif")) {
#ifdef MOTIF_SCROLLBAR
	      scrollBar.type = SCROLLBAR_MOTIF;
#else
	      print_error("Support for motif scrollbars was not compiled in.  Sorry.");
#endif
	    } else {
	      print_error("Unrecognized scrollbar type \"%s\".", nstr);
	    }
	    scrollbar_reset();
	    map_scrollbar(0);
	    map_scrollbar(1);
	    scrollbar_show(0);
	  }
	  nstr = strsep(&tnstr, ";");
	  if (nstr && *nstr) {
	    scrollBar.width = strtoul(nstr, (char **) NULL, 0);
	    if (scrollBar.width == 0) {
	      print_error("Invalid scrollbar length \"%s\".", nstr);
	      scrollBar.width = SB_WIDTH;
	    }
	    scrollbar_reset();
	    map_scrollbar(0);
	    map_scrollbar(1);
	    scrollbar_show(0);
	  }
	  break;
	case 11:
	  nstr = strsep(&tnstr, ";");
	  OPT_SET_OR_TOGGLE(nstr, Options, Opt_scrollBar_right);
	  scrollbar_reset();
	  map_scrollbar(0);
	  map_scrollbar(1);
	  scrollbar_show(0);
	  break;
	case 12:
	  nstr = strsep(&tnstr, ";");
	  OPT_SET_OR_TOGGLE(nstr, Options, Opt_scrollBar_floating);
	  scrollbar_reset();
	  map_scrollbar(0);
	  map_scrollbar(1);
	  scrollbar_show(0);
	  break;
	case 13:
	  nstr = strsep(&tnstr, ";");
	  OPT_SET_OR_TOGGLE(nstr, Options, Opt_scrollbar_popup);
	  break;
	case 20:
	  nstr = strsep(&tnstr, ";");
	  OPT_SET_OR_TOGGLE(nstr, Options, Opt_visualBell);
	  break;
#ifdef MAPALERT_OPTION
	case 21:
	  nstr = strsep(&tnstr, ";");
	  OPT_SET_OR_TOGGLE(nstr, Options, Opt_mapAlert);
	  break;
#endif
	case 22:
	  nstr = strsep(&tnstr, ";");
	  OPT_SET_OR_TOGGLE(nstr, Options, Opt_xterm_select);
	  break;
	case 23:
	  nstr = strsep(&tnstr, ";");
	  OPT_SET_OR_TOGGLE(nstr, Options, Opt_select_whole_line);
	  break;
	case 24:
	  nstr = strsep(&tnstr, ";");
	  OPT_SET_OR_TOGGLE(nstr, Options, Opt_viewport_mode);
	  render_simage(images[image_bg].current, TermWin.vt, TermWin_TotalWidth(), TermWin_TotalHeight(), image_bg, 1);
	  scr_touch();
	  break;
	case 25:
	  nstr = strsep(&tnstr, ";");
	  OPT_SET_OR_TOGGLE(nstr, Options, Opt_select_trailing_spaces);
	  break;
	case 26:
	  nstr = strsep(&tnstr, ";");
	  OPT_SET_OR_TOGGLE(nstr, Options, Opt_report_as_keysyms);
	  break;
	case 30:
	  nstr = strsep(&tnstr, ";");
	  if (nstr) {
	    if (XParseColor(Xdisplay, cmap, nstr, &xcol) && XAllocColor(Xdisplay, cmap, &xcol)) {
	      PixColors[fgColor] = xcol.pixel;
	      scr_refresh(DEFAULT_REFRESH);
	    }
	  }
	  break;
	case 40:
	  nstr = strsep(&tnstr, ";");
	  if (nstr) {
	    if (XParseColor(Xdisplay, cmap, nstr, &xcol) && XAllocColor(Xdisplay, cmap, &xcol)) {
	      PixColors[bgColor] = xcol.pixel;
	      scr_refresh(DEFAULT_REFRESH);
	    }
	  }
	  break;
	case 50:
	  /* Change desktops */
	  nstr = strsep(&tnstr, ";");
	  if (nstr && *nstr) {
	    XClientMessageEvent xev;

	    rs_desktop = (int) strtol(nstr, (char **) NULL, 0);
	    xev.type = ClientMessage;
	    xev.window = TermWin.parent;
	    xev.message_type = XInternAtom(Xdisplay, "_WIN_WORKSPACE", False);
	    xev.format = 32;
	    xev.data.l[0] = rs_desktop;
	    XChangeProperty(Xdisplay, TermWin.parent, xev.message_type, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &rs_desktop, 1);
	    XSendEvent(Xdisplay, Xroot, False, SubstructureNotifyMask, (XEvent *) & xev);
	  }
	  break;
	case 70:
	  /* Exit Eterm */
	  exit(0);
	  break;
	case 71:
	  /* Save current config */
	  nstr = strsep(&tnstr, ";");
	  if (nstr && *nstr) {
	    save_config(nstr);
	  } else {
	    save_config(NULL);
	  }
	  break;
	case 80:
	  /* Set debugging level */
	  nstr = strsep(&tnstr, ";");
	  if (nstr && *nstr) {
	    debug_level = (unsigned int) strtoul(nstr, (char **) NULL, 0);
	  }
	  break;

	default:
	  break;
      }
      break;

    case XTerm_Pixmap:
#ifdef PIXMAP_SUPPORT
# ifdef PIXMAP_OFFSET
      if (Options & Opt_pixmapTrans) {
	Options &= ~(Opt_pixmapTrans);
      }
# endif
      if (!strcmp(str, ";")) {
	load_image("", image_bg);
	bg_needs_update = 1;
      } else {
	nstr = strsep(&tnstr, ";");
	if (nstr) {
	  if (*nstr) {
	    set_pixmap_scale("", images[image_bg].current->pmap);
	    bg_needs_update = 1;
	    load_image(nstr, image_bg);
	  }
	  while ((nstr = strsep(&tnstr, ";")) && *nstr) {
	    changed += set_pixmap_scale(nstr, images[image_bg].current->pmap);
	    scaled = 1;
	  }
	} else {
	  load_image("", image_bg);
	  bg_needs_update = 1;
	}
      }
      if ((changed) || (bg_needs_update)) {
	render_simage(images[image_bg].current, TermWin.vt, TermWin_TotalWidth(), TermWin_TotalHeight(), image_bg, 1);
	scr_touch();
      }
#endif /* PIXMAP_SUPPORT */
      break;

    case XTerm_EtermIPC:
      for (; (nstr = strsep(&tnstr, ";"));) {
	eterm_ipc_parse(nstr);
      }
      break;

    case XTerm_restoreFG:
#ifdef XTERM_COLOR_CHANGE
      set_window_color(fgColor, str);
#endif
      break;
    case XTerm_restoreBG:
#ifdef XTERM_COLOR_CHANGE
      set_window_color(bgColor, str);
#endif
      break;
    case XTerm_logfile:
      break;
    case XTerm_font:
      change_font(0, str);
      break;
#ifdef ETERM_COMMAND_MODE
    case ETerm_command_mode:
      fprintf(stderr, "ETerm_command_mode\n");
      break;
#endif
    default:
      D_CMD(("Unsupported xterm escape sequence operator:  0x%02x\n", op));
      break;
  }
#ifdef PIXMAP_SUPPORT
  FREE(orig_tnstr);
#endif
}
