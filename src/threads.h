/* threads.h for Eterm
 * 25 Feb 1998, vendu
 */

#ifndef _THREADS_H
# define _THREADS_H 

# ifdef USE_POSIX_THREADS

#  ifdef PIXMAP_SUPPORT
#   include "pixmap.h"
#   include <X11/Xthreads.h>
#  endif

short bg_set;
pthread_attr_t resize_sub_thr_attr;
pthread_t resize_sub_thr;

extern void main_thread(void *);
extern void render_bg_thread(void *);
extern void check_bg_pixmap(void);

#  ifdef MUTEX_SYNCH
extern pthread_mutex_t mutex;

extern void prepare(void *dummy);
extern void parent(void *dummy);
extern void child(void *dummy);
#  endif

# endif /* USE_POSIX_THREADS */

#endif /* _THREADS_H */
