/* font.c -- Eterm font module
 *
 * This file is original work by Michael Jennings <mej@eterm.org> and
 * Tuomo Venalainen <vendu@cc.hut.fi>.  This file, and any other file
 * bearing this same message or a similar one, is distributed under
 * the GNU Public License (GPL) as outlined in the COPYING file.
 *
 * Copyright (C) 1997-1999, Michael Jennings and Tuomo Venalainen
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
#include "command.h"
#include "font.h"
#include "startup.h"
#include "options.h"
#include "screen.h"
#include "term.h"
#include "windows.h"

unsigned char font_change_count = 0;
#ifdef MULTI_CHARSET
const char *def_mfontName[] = {MFONT0, MFONT1, MFONT2, MFONT3, MFONT4};
#endif
const char *def_fontName[] = {FONT0, FONT1, FONT2, FONT3, FONT4};

static etfont_t *font_cache = NULL, *cur_font = NULL;
static void font_cache_add(const char *name, unsigned char type, void *info);
static void font_cache_del(const void *info);
static etfont_t *font_cache_find(const char *name, unsigned char type);
static void *font_cache_find_info(const char *name, unsigned char type);

static void
font_cache_add(const char *name, unsigned char type, void *info) {

  etfont_t *font;

  D_FONT(("font_cache_add(%s, %d, 0x%08x) called.\n", NONULL(name), type, (int) info));

  font = (etfont_t *) MALLOC(sizeof(etfont_t));
  font->name = StrDup(name);
  font->type = type;
  font->ref_cnt = 1;
  switch (type) {
  case FONT_TYPE_X: font->fontinfo.xfontinfo = (XFontStruct *) info; break;
  case FONT_TYPE_TTF:  break;
  case FONT_TYPE_FNLIB:  break;
  default:  break;
  }

  if (font_cache == NULL) {
    font_cache = cur_font = font;
    font->next = NULL;
  } else {
    cur_font->next = font;
    font->next = NULL;
    cur_font = font;
  }
}

static void
font_cache_del(const void *info) {

  etfont_t *current, *tmp;

  D_FONT(("font_cache_del(0x%08x) called.\n", (int) info));

  if (font_cache == NULL) {
    return;
  }
  if (((font_cache->type == FONT_TYPE_X) && (font_cache->fontinfo.xfontinfo == (XFontStruct *) info))) {
    D_FONT((" -> Match found at font_cache (0x%08x).  Font name is \"%s\"\n", (int) font_cache, NONULL(font_cache->name)));
    if (--(font_cache->ref_cnt) == 0) {
      D_FONT(("    -> Reference count is now 0.  Deleting from cache.\n"));
      current = font_cache;
      font_cache = current->next;
      XFreeFont(Xdisplay, (XFontStruct *) info);
      FREE(current->name);
      FREE(current);
    } else {
      D_FONT(("    -> Reference count is %d.  Returning.\n", font_cache->ref_cnt));
    }
    return;
  } else if ((font_cache->type == FONT_TYPE_TTF) && (0)) {
  } else if ((font_cache->type == FONT_TYPE_FNLIB) && (0)) {
  } else {
    for (current = font_cache; current->next; current = current->next) {
      if (((current->next->type == FONT_TYPE_X) && (current->next->fontinfo.xfontinfo == (XFontStruct *) info))) {
        D_FONT((" -> Match found at current->next (0x%08x, current == 0x%08x).  Font name is \"%s\"\n", (int) current->next, (int) current, NONULL(current->next->name)));
        if (--(current->next->ref_cnt) == 0) {
          D_FONT(("    -> Reference count is now 0.  Deleting from cache.\n"));
          tmp = current->next;
          current->next = current->next->next;
          XFreeFont(Xdisplay, (XFontStruct *) info);
          FREE(tmp->name);
          FREE(tmp);
        } else {
          D_FONT(("    -> Reference count is %d.  Returning.\n", font_cache->ref_cnt));
        }
        return;
      } else if ((current->next->type == FONT_TYPE_TTF) && (0)) {
      } else if ((current->next->type == FONT_TYPE_FNLIB) && (0)) {
      }
    }
  }
}

static etfont_t *
font_cache_find(const char *name, unsigned char type) {

  etfont_t *current;

  ASSERT_RVAL(name != NULL, NULL);

  D_FONT(("font_cache_find(%s, %d) called.\n", NONULL(name), type));

  for (current = font_cache; current; current = current->next) {
    D_FONT((" -> Checking current (0x%08x), type == %d, name == %s\n", current, current->type, NONULL(current->name)));
    if ((current->type == type) && !strcasecmp(current->name, name)) {
      D_FONT(("    -> Match!\n"));
      return (current);
    }
  }
  D_FONT(("font_cache_find():  No matches found. =(\n"));
  return ((etfont_t *) NULL);
}

static void *
font_cache_find_info(const char *name, unsigned char type) {

  etfont_t *current;

  ASSERT_RVAL(name != NULL, NULL);

  D_FONT(("font_cache_find_info(%s, %d) called.\n", NONULL(name), type));

  for (current = font_cache; current; current = current->next) {
    D_FONT((" -> Checking current (0x%08x), type == %d, name == %s\n", current, current->type, NONULL(current->name)));
    if ((current->type == type) && !strcasecmp(current->name, name)) {
      D_FONT(("    -> Match!\n"));
      switch (type) {
      case FONT_TYPE_X: return ((void *) current->fontinfo.xfontinfo); break;
      case FONT_TYPE_TTF: return (NULL); break;
      case FONT_TYPE_FNLIB: return (NULL); break;
      default: return (NULL); break;
      }
    }
  }
  D_FONT(("font_cache_find_info():  No matches found. =(\n"));
  return (NULL);
}

void *
load_font(const char *name, const char *fallback, unsigned char type)
{

  etfont_t *font;
  XFontStruct *xfont;

  D_FONT(("load_font(%s, %s, %d) called.\n", NONULL(name), NONULL(fallback), type));

  if (type == 0) {
    type = FONT_TYPE_X;
  }
  if (name == NULL) {
    if (fallback) {
      name = fallback;
      fallback = "fixed";
    } else {
      name = "fixed";
      fallback = "-misc-fixed-medium-r-normal--13-120-75-75-c-60-iso8859-1";
    }
  } else if (fallback == NULL) {
    fallback = "fixed";
  }
  D_FONT((" -> Using name == \"%s\" and fallback == \"%s\"\n", name, fallback));

  if ((font = font_cache_find(name, type)) != NULL) {
    font_cache_add_ref(font);
    D_FONT((" -> Font found in cache.  Incrementing reference count to %d and returning existing data.\n", font->ref_cnt));
    switch (type) {
    case FONT_TYPE_X: return ((void *) font->fontinfo.xfontinfo); break;
    case FONT_TYPE_TTF: return (NULL); break;
    case FONT_TYPE_FNLIB: return (NULL); break;
    default: return (NULL); break;
    }
  }
  if (type == FONT_TYPE_X) {
    if ((xfont = XLoadQueryFont(Xdisplay, name)) == NULL) {
      print_error("Unable to load font \"%s\".  Falling back on \"%s\"\n", name, fallback);
      if ((xfont = XLoadQueryFont(Xdisplay, fallback)) == NULL) {
        fatal_error("Couldn't load the fallback font either.  Giving up.");
      } else {
        font_cache_add(fallback, type, (void *) xfont);
      }
    } else {
      font_cache_add(name, type, (void *) xfont);
    }
    return ((void *) xfont);
  } else if (type == FONT_TYPE_TTF) {
    return (NULL);
  } else if (type == FONT_TYPE_FNLIB) {
    return (NULL);
  }
  ASSERT_NOTREACHED_RVAL(NULL);
}

void
free_font(const void *info)
{

  ASSERT(info != NULL);

  font_cache_del(info);
}

void
change_font(int init, const char *fontname)
{
#ifndef NO_BOLDFONT
  static XFontStruct *boldFont = NULL;
#endif
  static short fnum = FONT0_IDX;
  short idx = 0;
  int fh, fw = 0;
  register unsigned long i;
  register int cw;

  if (!init) {
    ASSERT(fontname != NULL);

    switch (fontname[0]) {
      case '\0':
	fnum = FONT0_IDX;
	fontname = NULL;
	break;

	/* special (internal) prefix for font commands */
      case FONT_CMD:
	idx = atoi(fontname + 1);
	switch (fontname[1]) {
	  case '+':		/* corresponds to FONT_UP */
	    fnum += (idx ? idx : 1);
	    fnum = FNUM_RANGE(fnum);
	    break;

	  case '-':		/* corresponds to FONT_DN */
	    fnum += (idx ? idx : -1);
	    fnum = FNUM_RANGE(fnum);
	    break;

	  default:
	    if (fontname[1] != '\0' && !isdigit(fontname[1]))
	      return;
	    if (idx < 0 || idx >= (NFONTS))
	      return;
	    fnum = IDX2FNUM(idx);
	    break;
	}
	fontname = NULL;
	break;

      default:
        for (idx = 0; idx < NFONTS; idx++) {
          if (!strcmp(rs_font[idx], fontname)) {
            fnum = IDX2FNUM(idx);
            fontname = NULL;
            break;
	  }
	}
	break;
    }
    idx = FNUM2IDX(fnum);

    if ((fontname != NULL) && strcasecmp(rs_font[idx], fontname)) {
      RESET_AND_ASSIGN(rs_font[idx], StrDup(fontname));
    }
  }
  if (TermWin.font) {
    if (font_cache_find_info(rs_font[idx], FONT_TYPE_X) != TermWin.font) {
      free_font(TermWin.font);
      TermWin.font = load_font(rs_font[idx], "fixed", FONT_TYPE_X);
    }
  } else {
    TermWin.font = load_font(rs_font[idx], "fixed", FONT_TYPE_X);
  }

#ifndef NO_BOLDFONT
  if (init && rs_boldFont != NULL) {
    boldFont = load_font(rs_boldFont, "-misc-fixed-bold-r-semicondensed--13-120-75-75-c-60-iso8859-1", FONT_TYPE_X);
  }
#endif

#ifdef MULTI_CHARSET
  if (TermWin.mfont) {
    if (font_cache_find_info(rs_mfont[idx], FONT_TYPE_X) != TermWin.mfont) {
      free_font(TermWin.mfont);
      TermWin.mfont = load_font(rs_mfont[idx], "k14", FONT_TYPE_X);
    }
  } else {
    TermWin.mfont = load_font(rs_mfont[idx], "k14", FONT_TYPE_X);
  }
# ifdef USE_XIM
  if (Input_Context) {
    if (TermWin.fontset) {
      XFreeFontSet(Xdisplay, TermWin.fontset);
    }
    TermWin.fontset = create_fontset(rs_font[idx], rs_mfont[idx]);
    xim_set_fontset();
  }
# endif
#endif /* MULTI_CHARSET */

  if (!init) {
    XSetFont(Xdisplay, TermWin.gc, TermWin.font->fid);
  }

  fw = TermWin.font->min_bounds.width;
  fh = TermWin.font->ascent + TermWin.font->descent + rs_line_space;

  D_FONT(("Font information:  Ascent == %hd, Descent == %hd\n", TermWin.font->ascent, TermWin.font->descent));
  if (TermWin.font->min_bounds.width == TermWin.font->max_bounds.width)
    TermWin.fprop = 0;	/* Mono-spaced (fixed width) font */
  else
    TermWin.fprop = 1;	/* Proportional font */
  if (TermWin.fprop == 1)
    for (i = TermWin.font->min_char_or_byte2; i <= TermWin.font->max_char_or_byte2; i++) {
      cw = TermWin.font->per_char[i].width;
      MAX_IT(fw, cw);
    }
  /* not the first time thru and sizes haven't changed */
  if (fw == TermWin.fwidth && fh == TermWin.fheight)
    return;			/* TODO: not return; check MULTI_CHARSET if needed */

  TermWin.fwidth = fw;
  TermWin.fheight = fh;

  /* check that size of boldFont is okay */
#ifndef NO_BOLDFONT
  TermWin.boldFont = NULL;
  if (boldFont != NULL) {

    fw = boldFont->min_bounds.width;
    fh = boldFont->ascent + boldFont->descent + rs_line_space;
    if (TermWin.fprop == 0) {	/* bold font must also be monospaced */
      if (fw != boldFont->max_bounds.width)
	fw = -1;
    } else {
      for (i = 0; i < 256; i++) {
	if (!isprint(i))
	  continue;
	cw = boldFont->per_char[i].width;
	MAX_IT(fw, cw);
      }
    }

    if (fw == TermWin.fwidth && fh == TermWin.fheight) {
      TermWin.boldFont = boldFont;
    }
  }
#endif /* NO_BOLDFONT */

  set_colorfgbg();

  TermWin.width = TermWin.ncol * TermWin.fwidth;
  TermWin.height = TermWin.nrow * TermWin.fheight;

  szHint.width_inc = TermWin.fwidth;
  szHint.height_inc = TermWin.fheight;

  szHint.min_width = szHint.base_width + szHint.width_inc;
  szHint.min_height = szHint.base_height + szHint.height_inc;

  szHint.width = szHint.base_width + TermWin.width;
  szHint.height = szHint.base_height + TermWin.height;

  szHint.flags = PMinSize | PResizeInc | PBaseSize | PWinGravity;

  if (!init) {
    font_change_count++;
    resize();
  }
  return;
}
