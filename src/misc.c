/*--------------------------------*-C-*---------------------------------*
 * File:	misc.c
 *
 * miscellaneous service routines
 *
 * Copyright 1996,97
 * mj olesen <olesen@me.QueensU.CA> Queen's Univ at Kingston
 *
 * You can do what you like with this source code provided you don't make
 * money from it and you include an unaltered copy of this message
 * (including the copyright).  As usual, the author accepts no
 * responsibility for anything, nor does he guarantee anything whatsoever.
 *----------------------------------------------------------------------*/

static const char cvs_ident[] = "$Id$";

#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "command.h"
#include "main.h"
#include "misc.h"
#include "debug.h"
#include "../libmej/debug.h"
#include "../libmej/strings.h"
#include "mem.h"
#include "options.h"

/*----------------------------------------------------------------------*/
const char *
my_basename(const char *str)
{

  const char *base = strrchr(str, '/');

  return (base ? base + 1 : str);

}

/* Print a non-terminal error message */
void
print_error(const char *fmt,...)
{

  va_list arg_ptr;

  va_start(arg_ptr, fmt);
  fprintf(stderr, APL_NAME ":  ");
  vfprintf(stderr, fmt, arg_ptr);
  fprintf(stderr, "\n");
  va_end(arg_ptr);
}

/* Print a simple warning. */
void
print_warning(const char *fmt,...)
{

  va_list arg_ptr;

  va_start(arg_ptr, fmt);
  fprintf(stderr, APL_NAME ":  warning:  ");
  vfprintf(stderr, fmt, arg_ptr);
  fprintf(stderr, "\n");
  va_end(arg_ptr);
}

/* Print a fatal error message and terminate */
void
fatal_error(const char *fmt,...)
{

  va_list arg_ptr;

  va_start(arg_ptr, fmt);
  fprintf(stderr, APL_NAME ":  FATAL:  ");
  vfprintf(stderr, fmt, arg_ptr);
  fprintf(stderr, "\n");
  va_end(arg_ptr);
  exit(-1);
}

/*
 * Compares the first n characters of s1 and s2, where n is strlen(s2)
 * Returns strlen(s2) if they match, 0 if not.
 */
unsigned long
str_leading_match(register const char *s1, register const char *s2)
{

  register unsigned long n;

  if (!s1 || !s2) {
    return (0);
  }
  for (n = 0; *s2; n++, s1++, s2++) {
    if (*s1 != *s2) {
      return 0;
    }
  }

  return n;
}

/* Strip leading and trailing whitespace and quotes from a string */
char *
str_trim(char *str)
{

  register char *tmp = str;
  size_t n;

  if (str && *str) {

    chomp(str);
    n = strlen(str);

    if (!n) {
      *str = 0;
      return str;
    }
    /* strip leading/trailing quotes */
    if (*tmp == '"') {
      tmp++;
      n--;
      if (!n) {
	*str = 0;
	return str;
      } else if (tmp[n - 1] == '"') {
	tmp[--n] = '\0';
      }
    }
    if (tmp != str) {
      memmove(str, tmp, (strlen(tmp)) + 1);
    }
  }
  return str;
}

/*
 * in-place interpretation of string:
 *
 *      backslash-escaped:      "\a\b\E\e\n\r\t", "\octal"
 *      Ctrl chars:     ^@ .. ^_, ^?
 *
 *      Emacs-style:    "M-" prefix
 *
 * Also,
 *      "M-x" prefixed strings, append "\r" if needed
 *      "\E]" prefixed strings (XTerm escape sequence) append "\a" if needed
 *
 * returns the converted string length
 */

#define MAKE_CTRL_CHAR(c) ((c) == '?' ? 127 : ((toupper(c)) - '@'))

int
parse_escaped_string(char *str)
{

  register char *pold, *pnew;
  unsigned char i;

  D_MENUBAR(("parse_escaped_string(\"%s\")\n", str));

  for (pold = pnew = str; *pold; pold++, pnew++) {
    D_MENUBAR(("Looking at \"%s\"\n", pold));
    if (!BEG_STRCASECMP(pold, "m-")) {
      *pold = '\\';
      *(pold + 1) = 'e';
    } else if (!BEG_STRCASECMP(pold, "c-")) {
      *(++pold) = '^';
    }
    D_MENUBAR(("Operating on \'%c\'\n", *pold));
    switch (*pold) {
      case '\\':
	D_MENUBAR(("Backslash + %c\n", *(pold + 1)));
	switch (tolower(*(++pold))) {
	  case '0':
	  case '1':
	  case '2':
	  case '3':
	  case '4':
	  case '5':
	  case '6':
	  case '7':
	    for (i = 0; *pold >= '0' && *pold <= '7'; pold++) {
	      i = (i * 8) + (*pold - '0');
	    }
	    pold--;
	    D_MENUBAR(("Octal number evaluates to %d\n", i));
	    *pnew = i;
	    break;
	  case 'n':
	    *pnew = '\n';
	    break;
	  case 'r':
	    *pnew = '\r';
	    break;
	  case 't':
	    *pnew = '\t';
	    break;
	  case 'b':
	    *pnew = '\b';
	    break;
	  case 'f':
	    *pnew = '\f';
	    break;
	  case 'a':
	    *pnew = '\a';
	    break;
	  case 'v':
	    *pnew = '\v';
	    break;
	  case 'e':
	    *pnew = '\033';
	    break;
	  case 'c':
	    pold++;
	    *pnew = MAKE_CTRL_CHAR(*pold);
	    break;
	  default:
	    *pnew = *pold;
	    break;
	}
	break;
      case '^':
	D_MENUBAR(("Caret + %c\n", *(pold + 1)));
	pold++;
	*pnew = MAKE_CTRL_CHAR(*pold);
	break;
      default:
	*pnew = *pold;
    }
  }

  if (!BEG_STRCASECMP(str, "\033x") && *(pnew - 1) != '\r') {
    D_MENUBAR(("Adding carriage return\n"));
    *(pnew++) = '\r';
  } else if (!BEG_STRCASECMP(str, "\0\e]") && *(pnew - 1) != '\a') {
    D_MENUBAR(("Adding bell character\n"));
    *(pnew++) = '\a';
  }
  *pnew = 0;

#if DEBUG >= DEBUG_MENU
  if (debug_level >= DEBUG_MENU) {
    D_MENUBAR(("New value is:\n\n"));
    HexDump(str, (size_t) (pnew - str));
  }
#endif

  return (pnew - str);
}

const char *
find_file(const char *file, const char *ext)
{

  const char *f;

#if defined(PIXMAP_SUPPORT) || (MENUBAR_MAX)
  if ((f = search_path(rs_path, file, ext)) != NULL) {
    return (f);
  } else
# ifdef PATH_ENV
  if ((f = search_path(getenv(PATH_ENV), file, ext)) != NULL) {
    return (f);
  } else
# endif
  if ((f = search_path(getenv("PATH"), file, ext)) != NULL) {
    return (f);
  } else {
    return (search_path(initial_dir, file, ext));
  }
#else
  return ((const char *) NULL);
#endif

}

/*----------------------------------------------------------------------*
 * miscellaneous drawing routines
 */

/*
 * draw bottomShadow/highlight along top/left sides of the window
 */
void
Draw_tl(Window win, GC gc, int x, int y, int w, int h)
{

  int shadow = SHADOW;

  if (w == 0 || h == 0) {
    shadow = 1;
  }
  w += (x - 1);
  h += (y - 1);

  for (; shadow > 0; shadow--, x++, y++, w--, h--) {
    XDrawLine(Xdisplay, win, gc, x, y, w, y);
    XDrawLine(Xdisplay, win, gc, x, y, x, h);
  }
}

/*
 * draw bottomShadow/highlight along the bottom/right sides of the window
 */
void
Draw_br(Window win, GC gc, int x, int y, int w, int h)
{

  int shadow = SHADOW;

  if (w == 0 || h == 0) {
    shadow = 1;
  }
  w += (x - 1);
  h += (y - 1);
  x++;
  y++;

  for (; shadow > 0; shadow--, x++, y++, w--, h--) {
    XDrawLine(Xdisplay, win, gc, w, h, w, y);
    XDrawLine(Xdisplay, win, gc, w, h, x, h);
  }
}

void
Draw_Shadow(Window win, GC topShadow, GC botShadow, int x, int y, int w, int h)
{

  Draw_tl(win, topShadow, x, y, w, h);
  Draw_br(win, botShadow, x, y, w, h);
}

/* button shapes */
void
Draw_Triangle(Window win, GC topShadow, GC botShadow, int x, int y, int w, int type)
{
  switch (type) {
    case 'r':			/* right triangle */
      XDrawLine(Xdisplay, win, topShadow, x, y, x, y + w);
      XDrawLine(Xdisplay, win, topShadow, x, y, x + w, y + w / 2);
      XDrawLine(Xdisplay, win, botShadow, x, y + w, x + w, y + w / 2);
      break;

    case 'l':			/* left triangle */
      XDrawLine(Xdisplay, win, botShadow, x + w, y + w, x + w, y);
      XDrawLine(Xdisplay, win, botShadow, x + w, y + w, x, y + w / 2);
      XDrawLine(Xdisplay, win, topShadow, x, y + w / 2, x + w, y);
      break;

    case 'd':			/* down triangle */
      XDrawLine(Xdisplay, win, topShadow, x, y, x + w / 2, y + w);
      XDrawLine(Xdisplay, win, topShadow, x, y, x + w, y);
      XDrawLine(Xdisplay, win, botShadow, x + w, y, x + w / 2, y + w);
      break;

    case 'u':			/* up triangle */
      XDrawLine(Xdisplay, win, botShadow, x + w, y + w, x + w / 2, y);
      XDrawLine(Xdisplay, win, botShadow, x + w, y + w, x, y + w);
      XDrawLine(Xdisplay, win, topShadow, x, y + w, x + w / 2, y);
      break;
#if 0
    case 's':			/* square */
      XDrawLine(Xdisplay, win, topShadow, x + w, y, x, y);
      XDrawLine(Xdisplay, win, topShadow, x, y, x, y + w);
      XDrawLine(Xdisplay, win, botShadow, x, y + w, x + w, y + w);
      XDrawLine(Xdisplay, win, botShadow, x + w, y + w, x + w, y);
      break;
#endif
  }
}
/*----------------------- end-of-file (C source) -----------------------*/
