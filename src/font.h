/*
 * Copyright (C) 1997-2009, Michael Jennings
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of the Software, its documentation and marketing & publicity
 * materials, and acknowledgment shall be given in the documentation, materials
 * and software packages that this Software was used.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _FONT_H_
#define _FONT_H_

#include <X11/Xfuncproto.h>
#include <X11/Intrinsic.h>      /* Xlib, Xutil, Xresource, Xfuncproto */
#include <libast.h>

/************  ************/
/************ Generic eterm_font object interface. ************/
/* Cast an arbitrary object pointer to an eterm_font. */
#define SPIF_ETERM_FONT(o)                    (SPIF_CAST(eterm_font) (o))
#define SPIF_ETERM_FONT_CLASS(o)              (SPIF_CAST(eterm_font_class) SPIF_OBJ_CLASS(o))

/* Name of class variable associated with eterm_font interface */
#define SPIF_ETERM_FONT_CLASS_VAR(type)       spif_ ## type ## _eterm_font_class

/* Check if a eterm_font is NULL */
#define SPIF_ETERM_FONT_ISNULL(o)             (SPIF_ETERM_FONT(o) == SPIF_NULL_TYPE(eterm_font))

/* Check if an object is a eterm_font */
#define SPIF_OBJ_IS_ETERM_FONT(o)             SPIF_OBJ_IS_TYPE(o, eterm_font)

/* Call a method on an instance of an implementation class */
#define SPIF_ETERM_FONT_CALL_METHOD(o, meth)  SPIF_ETERM_FONT_CLASS(o)->meth

/* Calls to the basic functions. */
#define SPIF_ETERM_FONT_NEW(type)             SPIF_ETERM_FONT((SPIF_CLASS(SPIF_ETERM_FONT_CLASS_VAR(type)))->noo())
#define SPIF_ETERM_FONT_INIT(o)               SPIF_OBJ_INIT(o)
#define SPIF_ETERM_FONT_DONE(o)               SPIF_OBJ_DONE(o)
#define SPIF_ETERM_FONT_DEL(o)                SPIF_OBJ_DEL(o)
#define SPIF_ETERM_FONT_SHOW(o, b, i)         SPIF_OBJ_SHOW(o, b, i)
#define SPIF_ETERM_FONT_COMP(o1, o2)          SPIF_OBJ_COMP(o1, o2)
#define SPIF_ETERM_FONT_DUP(o)                SPIF_OBJ_DUP(o)
#define SPIF_ETERM_FONT_TYPE(o)               SPIF_OBJ_TYPE(o)

typedef SPIF_TYPE(obj) SPIF_TYPE(eterm_font);

SPIF_DECL_OBJ(eterm_font_class) {
    SPIF_DECL_PARENT_TYPE(class);

};


/************  ************/
#define SPIF_ETERM_FONT_X(obj)                (SPIF_CAST(eterm_font_x) (obj))

#define SPIF_ETERM_FONT_X_ISNULL(o)           (SPIF_ETERM_FONT_X(o) == SPIF_NULL_TYPE(eterm_font_x))
#define SPIF_OBJ_IS_ETERM_FONT_X(o)           (SPIF_OBJ_IS_TYPE((o), eterm_font_x))

SPIF_DECL_OBJ(eterm_font_x) {
    SPIF_DECL_PARENT_TYPE(obj);

};

extern spif_eterm_font_class_t SPIF_ETERM_FONT_CLASS_VAR(eterm_font);
/************  ************/

extern spif_vector_t fonts;
extern spif_class_t SPIF_CLASS_VAR(eterm_font);
extern spif_eterm_font_t spif_eterm_font_new(void);
extern spif_bool_t spif_eterm_font_del(spif_eterm_font_t);
extern spif_bool_t spif_eterm_font_init(spif_eterm_font_t);
extern spif_bool_t spif_eterm_font_done(spif_eterm_font_t);
extern spif_eterm_font_t spif_eterm_font_dup(spif_eterm_font_t);
extern spif_cmp_t spif_eterm_font_comp(spif_eterm_font_t, spif_eterm_font_t);
extern spif_str_t spif_eterm_font_show(spif_eterm_font_t, spif_charptr_t, spif_str_t, size_t);
extern spif_classname_t spif_eterm_font_type(spif_eterm_font_t);


/************ Macros and Definitions ************/
#define FONT_TYPE_X             (0x01)
#define FONT_TYPE_TTF           (0x02)
#define FONT_TYPE_FNLIB         (0x03)

#define font_cache_add_ref(font) ((font)->ref_cnt++)

#define NFONTS 5
#define FONT_CMD       '#'
#define BIGGER_FONT    "#+"
#define SMALLER_FONT   "#-"

/* These are subscripts for the arrays in a fontshadow_t */
#define SHADOW_TOP_LEFT      0
#define SHADOW_TOP_RIGHT     1
#define SHADOW_BOTTOM_LEFT   2
#define SHADOW_BOTTOM_RIGHT  3

/* The macros are used to advance to the next/previous font as with Ctrl-> and Ctrl-< */
#define NEXT_FONT(i)   do { if (font_idx + ((i)?(i):1) >= font_cnt) {font_idx = font_cnt - 1;} else {font_idx += ((i)?(i):1);} \
                            while (!etfonts[font_idx]) {if (font_idx == font_cnt) {font_idx--; break;} font_idx++;} } while (0)
#define PREV_FONT(i)   do { if (font_idx - ((i)?(i):1) < 0) {font_idx = 0;} else {font_idx -= ((i)?(i):1);} \
                            while (!etfonts[font_idx]) {if (font_idx == 0) break; font_idx--;} } while (0)
#define DUMP_FONTS()   do {unsigned char i; D_FONT(("DUMP_FONTS():  Font count is %u\n", (unsigned int) font_cnt)); \
                           for (i = 0; i < font_cnt; i++) {D_FONT(("DUMP_FONTS():  Font %u == \"%s\"\n", (unsigned int) i, NONULL(etfonts[i])));}} while (0)

/************ Structures ************/
typedef struct cachefont_struct {
  char *name;             /* Font name in canonical format */
  unsigned char type;     /* Font type (FONT_TYPE_* from above */
  unsigned char ref_cnt;  /* Reference count */
  union {
    /* This union will eventually have members for TTF/Fnlib fonts */
    XFontStruct *xfontinfo;
  } fontinfo;
  struct cachefont_struct *next;
} cachefont_t;

typedef struct fontshadow_struct {
  Pixel color[4];
  unsigned char shadow[4];
  unsigned char do_shadow;
} fontshadow_t;

/************ Variables ************/
extern unsigned char font_idx, font_cnt, font_chg;
extern int def_font_idx;
extern const char *def_fontName[];
extern char *rs_font[NFONTS];
extern char **etfonts, **etmfonts;
extern fontshadow_t fshadow;
# ifdef MULTI_CHARSET
extern const char *def_mfontName[];
extern char *rs_mfont[NFONTS];
# endif

/************ Function Prototypes ************/
_XFUNCPROTOBEGIN

extern void eterm_font_add(char ***plist, const char *fontname, unsigned char idx);
extern void eterm_font_delete(char **flist, unsigned char idx);
extern void eterm_font_list_clear(void);
extern void font_cache_clear(void);
extern void *load_font(const char *, const char *, unsigned char);
extern void free_font(const void *);
extern void change_font(int, const char *);
extern const char *get_font_name(void *);
extern void set_shadow_color_by_name(unsigned char, const char *);
extern void set_shadow_color_by_pixel(unsigned char, Pixel);
extern unsigned char parse_font_fx(char *line);

_XFUNCPROTOEND

#endif	/* _FONT_H_ */
