/* eterm_imlib.h - An include file for porting Eterm to use Imlib 1.0
 * in addition to Imlib 0.x.
 * Feb 15 1998, vendu
 */

#ifndef _ETERM_IMLIB_H
# define _ETERM_IMLIB_H

# ifdef NEW_IMLIB
#  include <Imlib.h>
# else if defined(OLD_IMLIB)
#  include <X11/imlib.h>
# endif

# ifdef NEW_IMLIB
#  define ImColor ImlibColor
#  define Image ImlibImage
#  define ImlibInit(d) Imlib_init(d)
#  define ImlibLoadImage(id,f,c) Imlib_load_image(id,f)
#  define ImlibFreePixmap(id,p) Imlib_free_pixmap(id,p)
#  define ImlibRender(id,i,w,h) Imlib_render(id,i,w,h)
#  define ImlibCopyImageToPixmap(id,i) Imlib_copy_image(id,i)
#  define ImlibDestroyImage(id,i) Imlib_destroy_image(id,i)
# endif

#endif /* _ETERM_IMLIB_H */
