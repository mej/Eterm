/* threads.c for Eterm
 * Feb 25 1998, vendu
 */

static const char cvs_ident[] = "$Id$";

#include "config.h"
#include "feature.h"

#include <stdio.h>

#ifdef USE_POSIX_THREADS
# include "startup.h"
# include "debug.h"
# include "screen.h"

# ifdef PIXMAP_SUPPORT
#  include "pixmap.h"
# endif

/* extern char *sig_to_str(int sig); */

# ifdef USE_IMLIB
extern imlib_t imlib_bg;

# endif

# include "threads.h"

extern int refresh_count, refresh_limit;
extern unsigned char *cmdbuf_base, *cmdbuf_ptr, *cmdbuf_endp;
extern unsigned char cmd_getc(void);

# ifdef MUTEX_SYNCH
/* pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; */
# endif

short bg_set = 0;
pthread_t resize_sub_thr;
pthread_attr_t resize_sub_thr_attr;

# ifdef MUTEX_SYNCH
void
prepare(void *dummy)
{
  D_THREADS(("prepare(): pthread_mutex_trylock(&mutex)\n"));
  pthread_mutex_trylock(&mutex);
}

void
parent(void *dummy)
{
  D_THREADS(("parent(): pthread_mutex_unlock(&mutex)\n"));
  pthread_mutex_unlock(&mutex);
}

void
child(void *dummy)
{
  D_THREADS(("child(): pthread_mutex_unlock(&mutex)\n"));
  pthread_mutex_unlock(&mutex);
}

# endif

inline void
refresh_termwin(void)
{
  scrollbar_show(0);
  scr_touch();
}

void
render_bg_thread(void *dummy)
{
  void *retval = NULL;

  D_THREADS(("render_bg_thread() entered\n"));

  if (bg_set) {
# ifdef AGGRESSIVE_THREADS
    ;
# else
    D_THREADS(("Background already set\n"));
# endif
  }
# ifdef MUTEX_SYNCH
  if (pthread_mutex_trylock(&mutex) != EBUSY) {
    D_THREADS(("pthread_mutex_trylock(&mutex): "));
  } else {
    D_THREADS(("mutex already locked\n"));
  }
# endif

  D_THREADS(("pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);\n"));
  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

  D_THREADS(("pthread_detach();\n"));
  pthread_detach(resize_sub_thr);

  D_THREADS(("render_bg_thread(): render_pixmap()\n"));
  render_pixmap(TermWin.vt, imlib_bg, bgPixmap, 0, 1);

  D_THREADS(("refresh_termwin()\n"));
  refresh_termwin();

  bg_set = 1;

# ifdef MUTEX_SYNCH
  D_THREADS(("pthread_mutex_unlock(&mutex);\n"));
  pthread_mutex_unlock(&mutex);
# endif

  D_THREADS(("pthread_exit()\n"));
  pthread_exit(retval);
}

/* Read and process output from the application - threaded version. */
void
main_thread(void *ignored)
{
  register int ch;

  D_THREADS(("[%d] main_thread() entered\n", getpid()));
  D_CMD(("bg_set = %d\n", bg_set));

  do {
    while ((ch = cmd_getc()) == 0);	/* wait for something */

    if (ch >= ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
      /* Read a text string from the input buffer */
      int nlines = 0;

      /*     unsigned char * str; */
      register unsigned char *str;

      /*
       * point to the start of the string,
       * decrement first since already did get_com_char ()
       */
      str = --cmdbuf_ptr;
      while (cmdbuf_ptr < cmdbuf_endp) {
	ch = *cmdbuf_ptr++;
	if (ch >= ' ' || ch == '\t' || ch == '\r') {
	  /* nothing */
	} else if (ch == '\n') {
	  nlines++;
	  if (++refresh_count >= (refresh_limit * (TermWin.nrow - 1)))
	    break;
	} else {		/* unprintable */
	  cmdbuf_ptr--;
	  break;
	}
      }

      scr_add_lines(str, nlines, (cmdbuf_ptr - str));
    } else {
      switch (ch) {
# ifdef NO_VT100_ANS
	case 005:
	  break;
# else
	case 005:
	  tt_printf(VT100_ANS);
	  break;		/* terminal Status */
# endif
	case 007:
	  scr_bell();
	  break;		/* bell */
	case '\b':
	  scr_backspace();
	  break;		/* backspace */
	case 013:
	case 014:
	  scr_index(UP);
	  break;		/* vertical tab, form feed */
	case 016:
	  scr_charset_choose(1);
	  break;		/* shift out - acs */
	case 017:
	  scr_charset_choose(0);
	  break;		/* shift in - acs */
	case 033:
	  process_escape_seq();
	  break;
      }
    }
  } while (ch != EOF);
}

inline void
check_bg_pixmap(void)
{
  if (bg_set) {
# ifdef MUTEX_SYNCH
    if (pthread_mutex_trylock(&mutex) == EBUSY) {
      D_THREADS(("cmd_getc(): mutex locked, bbl\n"));
    } else
# endif
    {
# ifdef MUTEX_SYNCH
      D_THREADS(("cmd_getc(): pthread_mutex_trylock(&mutex): "));
# endif

      D_THREADS(("refresh_termwin() "));
      refresh_termwin();

# ifdef MUTEX_SYNCH
      D_THREADS(("pthread_mutex_unlock(&mutex);\n"));
      pthread_mutex_unlock(&mutex);
# endif
      /*                bg_set = 0; */
    }

    bg_set = 0;

    /*      main_thread(NULL); */
  }
}

#endif /* USE_POSIX_THREADS */
