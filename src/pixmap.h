/* pixmap.h for Eterm.
 * 17 Feb 1998 vendu
 */

#ifndef _PIXMAP_H
# define _PIXMAP_H

#include <X11/Xatom.h>

# ifdef USE_IMLIB
#  include "eterm_imlib.h"
# endif

typedef struct {
    short w, h, x, y;
    Pixmap pixmap;
} pixmap_t;

# ifdef USE_IMLIB
typedef struct {
    Image *im;
    int last_w,last_h;
} imlib_t;

#  define background_is_pixmap() ((imlib_id != NULL) || Options & Opt_pixmapTrans)
#  define background_is_image() (imlib_bg.im != NULL)

# else

#  define background_is_pixmap() ((int)0)
#  define background_is_image() ((int)0)

# endif

# ifdef USE_IMLIB
extern imlib_t imlib_bg;
extern ImlibData *imlib_id;
#  ifdef PIXMAP_SCROLLBAR
imlib_t imlib_sb, imlib_sa, imlib_saclk;
#  endif
# endif
pixmap_t bgPixmap;
# ifdef PIXMAP_SCROLLBAR
pixmap_t sbPixmap;
pixmap_t upPixmap, up_clkPixmap;
pixmap_t dnPixmap, dn_clkPixmap;
pixmap_t saPixmap, sa_clkPixmap;
# endif

typedef short renderop_t;

# ifdef PIXMAP_OFFSET
enum { FAKE_TRANSPARENCY };
enum { tint_none, tint_red, tint_green, tint_blue,
       tint_cyan, tint_magenta, tint_yellow };

#  ifdef USE_IMLIB
void render_pixmap(Window win, imlib_t image, pixmap_t pmap,
		   int which, renderop_t renderop);
#  endif
# endif

# ifdef USE_POSIX_THREADS
void init_bg_pixmap_thread(void *file);
# endif
void set_bgPixmap(const char *file);
void set_Pixmap(const char *file, Pixmap dest_pmap, int type);
int scale_pixmap(const char *geom, pixmap_t *pmap);
# ifdef USE_IMLIB
void colormod_pixmap(imlib_t, GC, int, int);
# endif

# ifdef PIXMAP_OFFSET
#  ifdef IMLIB_TRANS
void colormod_trans(imlib_t, GC, int, int);
#  else
void colormod_trans(Pixmap, GC, int, int);
#  endif
Window get_desktop_window(void);
Pixmap get_desktop_pixmap(void);
extern Window desktop_window;
# endif
extern void shaped_window_apply_mask(Window, Pixmap);
extern void set_icon_pixmap(char *, XWMHints *);
# ifdef USE_IMLIB
extern ImlibImage *ReadImageViaImlib(Display *, const char *);
# endif

#endif /* _PIXMAP_H */
