/*--------------------------------*-C-*---------------------------------*
 * File:	command.c
 */
/* notes: */
/*----------------------------------------------------------------------*
 * Copyright 1992 John Bovey, University of Kent at Canterbury.
 *
 * You can do what you like with this source code as long as
 * you don't try to make money out of it and you include an
 * unaltered copy of this message (including the copyright).
 *
 * This module has been very heavily modified by R. Nation
 * <nation@rocket.sanders.lockheed.com>
 * No additional restrictions are applied
 *
 * Additional modification by Garrett D'Amore <garrett@netcom.com> to
 * allow vt100 printing.  No additional restrictions are applied.
 *
 * Integrated modifications by Steven Hirsch <hirsch@emba.uvm.edu> to
 * properly support X11 mouse report mode and support for DEC
 * "private mode" save/restore functions.
 *
 * Integrated key-related changes by Jakub Jelinek <jj@gnu.ai.mit.edu>
 * to handle Shift+function keys properly.
 * Should be used with enclosed termcap / terminfo database.
 *
 * Extensive modifications by mj olesen <olesen@me.QueensU.CA>
 * No additional restrictions.
 *
 * Further modification and cleanups for Solaris 2.x and Linux 1.2.x
 * by Raul Garcia Garcia <rgg@tid.es>. No additional restrictions.
 *
 * As usual, the author accepts no responsibility for anything, nor does
 * he guarantee anything whatsoever.
 *----------------------------------------------------------------------*/

static const char cvs_ident[] = "$Id$";

/* includes: */
#include "feature.h"
#include "config.h"

/* System Headers */
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#if !defined(SIGSYS)
# if defined(SIGUNUSED)
#  define SIGSYS SIGUNUSED
# else
#  define SIGSYS ((int) 0)
# endif
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#include <sys/types.h>
#include <limits.h>

/* X11 Headers */
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>
#include <X11/Xproto.h>
#include <X11/IntrinsicP.h>
#ifdef OFFIX_DND
# define DndFile	2
# define DndDir		5
# define DndLink	7
#endif
#include <X11/keysym.h>
#ifndef NO_XLOCALE
# if (XtVersion < 11005)
#  define NO_XLOCALE
#  include <locale.h>
# else
#  include <X11/Xlocale.h>
# endif
#endif /* NO_XLOCALE */
#ifdef USE_GETGRNAME
# include <grp.h>
#endif
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#if defined (__svr4__)
# include <sys/resource.h>	/* for struct rlimit */
# include <sys/stropts.h>	/* for I_PUSH */
# ifdef HAVE_SYS_STRTIO_H
#  include <sys/strtio.h>
# endif
# ifdef HAVE_BSDTTY_H
#  include <bsdtty.h>
# endif
# define _NEW_TTY_CTRL		/* to get proper defines in <termios.h> */
#endif
#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif
#ifdef HAVE_TERMIOS_H
# include <termios.h>
#else
# include <sgtty.h>
#endif
#if defined(__sun) && defined(__SVR4)
# include <sys/strredir.h>
#endif
#include <sys/wait.h>
#include <sys/stat.h>
#if defined(linux)
# include <linux/tty.h>		/* For N_TTY_BUF_SIZE. */
#endif
#if defined(linux)
# include <string.h>		/* For strsep(). -vendu */
#endif

/* Eterm-specific Headers */
#ifdef USE_ACTIVE_TAGS
# include "activetags.h"
# include "activeeterm.h"
#endif
#include "command.h"
#include "main.h"
#include "../libmej/debug.h"
#include "debug.h"
#include "../libmej/mem.h"
#include "../libmej/strings.h"
#include "string.h"
#include "graphics.h"
#include "grkelot.h"
#include "scrollbar.h"
#include "menubar.h"
#include "screen.h"
#include "options.h"
#include "pixmap.h"
#ifdef USE_POSIX_THREADS
# include "threads.h"
#endif
#ifdef PROFILE
# include "profile.h"
#endif

#ifdef PIXMAP_SCROLLBAR
extern pixmap_t sbPixmap;

#endif
#ifdef PIXMAP_MENUBAR
extern pixmap_t mbPixmap;

#endif


/* terminal mode defines: */
/* ways to deal with getting/setting termios structure */
#ifdef HAVE_TERMIOS_H
typedef struct termios ttymode_t;

# ifdef TCSANOW			/* POSIX */
#  define GET_TERMIOS(fd,tios)	tcgetattr (fd, tios)
#  define SET_TERMIOS(fd,tios)	do {\
cfsetospeed (tios, BAUDRATE);\
cfsetispeed (tios, BAUDRATE);\
tcsetattr (fd, TCSANOW, tios);\
} while (0)
# else
#  ifdef TIOCSETA
#   define GET_TERMIOS(fd,tios)	ioctl (fd, TIOCGETA, tios)
#   define SET_TERMIOS(fd,tios)	do {\
tios->c_cflag |= BAUDRATE;\
ioctl (fd, TIOCSETA, tios);\
} while (0)
#  else
#   define GET_TERMIOS(fd,tios)	ioctl (fd, TCGETS, tios)
#   define SET_TERMIOS(fd,tios)	do {\
tios->c_cflag |= BAUDRATE;\
ioctl (fd, TCSETS, tios);\
} while (0)
#  endif
# endif
# define SET_TTYMODE(fd,tios)		SET_TERMIOS (fd, tios)
#else
/* sgtty interface */
typedef struct {
  struct sgttyb sg;
  struct tchars tc;
  struct ltchars lc;
  int line;
  int local;
} ttymode_t;

# define SET_TTYMODE(fd,tt) \
do {	\
tt->sg.sg_ispeed = tt->sg.sg_ospeed = BAUDRATE;\
ioctl (fd, TIOCSETP, &(tt->sg));\
ioctl (fd, TIOCSETC, &(tt->tc));\
ioctl (fd, TIOCSLTC, &(tt->lc));\
ioctl (fd, TIOCSETD, &(tt->line));\
ioctl (fd, TIOCLSET, &(tt->local));\
} while (0)
#endif /* HAVE_TERMIOS_H */

/* use the fastest baud-rate */
#ifdef B38400
# define BAUDRATE	B38400
#else
# ifdef B19200
#  define BAUDRATE	B19200
# else
#  define BAUDRATE	B9600
# endif
#endif

/* Disable special character functions */
#ifdef _POSIX_VDISABLE
# define VDISABLE	_POSIX_VDISABLE
#else
# define VDISABLE	255
#endif

/*----------------------------------------------------------------------*
 * system default characters if defined and reasonable
 */
#ifndef CINTR
# define CINTR		'\003'	/* ^C */
#endif
#ifndef CQUIT
# define CQUIT		'\034'	/* ^\ */
#endif
#ifndef CERASE
# ifdef linux
#  define CERASE	'\177'	/* ^? */
# else
#  define CERASE	'\010'	/* ^H */
# endif
#endif
#ifndef CKILL
# define CKILL		'\025'	/* ^U */
#endif
#ifndef CEOF
# define CEOF		'\004'	/* ^D */
#endif
#ifndef CSTART
# define CSTART		'\021'	/* ^Q */
#endif
#ifndef CSTOP
# define CSTOP		'\023'	/* ^S */
#endif
#ifndef CSUSP
# define CSUSP		'\032'	/* ^Z */
#endif
#ifndef CDSUSP
# define CDSUSP		'\031'	/* ^Y */
#endif
#ifndef CRPRNT
# define CRPRNT		'\022'	/* ^R */
#endif
#ifndef CFLUSH
# define CFLUSH		'\017'	/* ^O */
#endif
#ifndef CWERASE
# define CWERASE	'\027'	/* ^W */
#endif
#ifndef CLNEXT
# define CLNEXT		'\026'	/* ^V */
#endif

#ifndef VDISCRD
# ifdef VDISCARD
#  define VDISCRD	VDISCARD
# endif
#endif

#ifndef VWERSE
# ifdef VWERASE
#  define VWERSE	VWERASE
# endif
#endif

/* defines: */

#define KBUFSZ		8	/* size of keyboard mapping buffer */
#define STRING_MAX	512	/* max string size for process_xterm_seq() */
#define ESC_ARGS	32	/* max # of args for esc sequences */

/* a large REFRESH_PERIOD causes problems with `cat' */

#ifndef REFRESH_PERIOD
# define REFRESH_PERIOD	3
#endif

#ifndef MULTICLICK_TIME
# define MULTICLICK_TIME		500
#endif
#ifndef SCROLLBAR_INITIAL_DELAY
# define SCROLLBAR_INITIAL_DELAY	40
#endif
#ifndef SCROLLBAR_CONTINUOUS_DELAY
# define SCROLLBAR_CONTINUOUS_DELAY	2
#endif

/* time factor to slow down a `jumpy' mouse */
#define MOUSE_THRESHOLD		50
#define CONSOLE		"/dev/console"	/* console device */

/*
 * key-strings: if only these keys were standardized <sigh>
 */
#ifdef LINUX_KEYS
# define KS_HOME	"\033[1~"	/* Home == Find */
# define KS_END	"\033[4~"	/* End == Select */
#else
# define KS_HOME	"\033[7~"	/* Home */
# define KS_END	"\033[8~"	/* End */
#endif

/* and this one too! */
#ifdef NO_DELETE_KEY
# undef KS_DELETE		/* use X server definition */
#else
# ifndef KS_DELETE
#  define KS_DELETE	"\033[3~"	/* Delete = Execute */
# endif
#endif

/*
 * ESC-Z processing:
 *
 * By stealing a sequence to which other xterms respond, and sending the
 * same number of characters, but having a distinguishable sequence,
 * we can avoid having a timeout (when not under an Eterm) for every login
 * shell to auto-set its DISPLAY.
 *
 * This particular sequence is even explicitly stated as obsolete since
 * about 1985, so only very old software is likely to be confused, a
 * confusion which can likely be remedied through termcap or TERM. Frankly,
 * I doubt anyone will even notice.  We provide a #ifdef just in case they
 * don't care about auto-display setting.  Just in case the ancient
 * software in question is broken enough to be case insensitive to the 'c'
 * character in the answerback string, we make the distinguishing
 * characteristic be capitalization of that character. The length of the
 * two strings should be the same so that identical read(2) calls may be
 * used.
 */
#define VT100_ANS	"\033[?1;2c"	/* vt100 answerback */
#ifndef ESCZ_ANSWER
# define ESCZ_ANSWER	VT100_ANS	/* obsolete ANSI ESC[c */
#endif

/* Global attributes */
extern XWindowAttributes attr;
extern XSetWindowAttributes Attributes;
extern char *orig_argv0;

#ifdef PIXMAP_SUPPORT
extern short bg_needs_update;

#endif

/* extern functions referenced */
extern char *ptsname();

#ifdef DISPLAY_IS_IP
extern char *network_display(const char *display);

#endif

extern void get_initial_options(int, char **);
extern void menubar_read(const char *filename);

#ifdef USE_POSIX_THREADS
extern static void **retval;
extern static int join_value;
extern static pthread_t main_loop_thr;
extern static pthread_attr_t main_loop_attr;

# ifdef MUTEX_SYNCH
extern pthread_mutex_t mutex;

# endif
#endif

#ifdef PIXMAP_SUPPORT
extern void render_pixmap(Window win, imlib_t image, pixmap_t pmap,
			  int which, renderop_t renderop);

# ifdef BACKING_STORE
extern const char *rs_saveUnder;

# endif

extern char *rs_noCursor;

# ifdef USE_IMLIB
extern ImlibData *imlib_id;

# endif
#endif

/* extern variables referenced */
extern int my_ruid, my_rgid, my_euid, my_egid;

#ifdef PIXMAP_OFFSET
extern unsigned int rs_shadePct;
extern unsigned long rs_tintMask;

#endif

#ifdef PIXMAP_OFFSET
extern Pixmap desktop_pixmap, viewport_pixmap;

#endif

/* extern variables declared here */
extern TermWin_t TermWin;
extern Display *Xdisplay;	/* display */

extern char *rs_color[NRS_COLORS];
extern Pixel PixColors[NRS_COLORS + NSHADOWCOLORS];

extern unsigned long Options;

extern const char *display_name;
extern char *rs_name;		/* client instance (resource name) */

#ifndef NO_BOLDFONT
extern const char *rs_boldFont;

#endif
extern const char *rs_font[NFONTS];

#ifdef KANJI
extern const char *rs_kfont[NFONTS];

#endif

#ifdef PRINTPIPE
extern char *rs_print_pipe;

#endif

extern char *rs_cutchars;

/* local variables */
extern Cursor TermWin_cursor;	/* cursor for vt window */
extern unsigned int colorfgbg;
extern menuBar_t menuBar;
unsigned char keypress_exit = 0;

extern XSizeHints szHint;

extern char *def_colorName[];

#ifdef KANJI
/* Kanji font names, roman fonts sized to match */
extern const char *def_kfontName[];

#endif /* KANJI */
extern const char *def_fontName[];

/* extern functions referenced */
#ifdef PIXMAP_SUPPORT
/* the originally loaded pixmap and its scaling */
extern pixmap_t bgPixmap;
extern void set_bgPixmap(const char * /* file */ );

# ifdef USE_IMLIB
extern imlib_t imlib_bg;

# endif
# ifdef PIXMAP_SCROLLBAR
extern pixmap_t sbPixmap;
extern pixmap_t upPixmap, up_clkPixmap;
extern pixmap_t dnPixmap, dn_clkPixmap;
extern pixmap_t saPixmap, sa_clkPixmap;

#  ifdef USE_IMLIB
extern imlib_t imlib_sb, imlib_sa, imlib_saclk;

#  endif
# endif
# ifdef PIXMAP_MENUBAR
extern pixmap_t mbPixmap, mb_selPixmap;

#  ifdef USE_IMLIB
extern imlib_t imlib_mb, imlib_ms;

#  endif
# endif

extern int scale_pixmap(const char *geom, pixmap_t * pmap);

#endif /* PIXMAP_SUPPORT */

/* have we changed the font? Needed to avoid race conditions
 * while window resizing  */
extern int font_change_count;

static void resize(void);

/* extern functions referenced */
#ifdef UTMP_SUPPORT
extern void cleanutent(void);
extern void makeutent(const char *, const char *);

#else
#  define cleanutent() ((void)(0))
#  define makeutent(pty, hostname) ((void)(0))
#endif

/* extern variables referenced */
extern int my_ruid, my_rgid, my_euid, my_egid;

/* extern variables declared here */

/* local variables */
/* static unsigned char segv=0; */
char initial_dir[PATH_MAX + 1];
static char *ptydev = NULL, *ttydev = NULL;	/* pty/tty name */

#ifdef USE_ACTIVE_TAGS
int cmd_fd = -1;		/* file descriptor connected to the command */
pid_t cmd_pid = -1;		/* process id if child */

#else
static int cmd_fd = -1;		/* file descriptor connected to the command */
static pid_t cmd_pid = -1;	/* process id if child */

#endif
static int Xfd = -1;		/* file descriptor of X server connection */
static unsigned int num_fds = 0;	/* number of file descriptors being used */
static struct stat ttyfd_stat;	/* original status of the tty we will use */

#ifdef SCROLLBAR_BUTTON_CONTINUAL_SCROLLING
static int scroll_arrow_delay;

#endif

#ifdef META8_OPTION
static unsigned char meta_char = 033;	/* Alt-key prefix */

#endif

unsigned long PrivateModes = PrivMode_Default;
static unsigned long SavedModes = PrivMode_Default;

#ifndef USE_POSIX_THREADS
static int refresh_count = 0, refresh_limit = 1;

#else
int refresh_count = 0, refresh_limit = 1;

#endif

/* Why? -vendu */
/* static int refresh_type = SLOW_REFRESH; */
static int refresh_type = FAST_REFRESH;

static Atom wmDeleteWindow;

/* OffiX Dnd (drag 'n' drop) support */
#ifdef OFFIX_DND
static Atom DndProtocol, DndSelection;

#endif /* OFFIX_DND */

#ifndef NO_XLOCALE
static char *rs_inputMethod = "";	/* XtNinputMethod */
static char *rs_preeditType = NULL;	/* XtNpreeditType */
static XIC Input_Context;	/* input context */

#endif /* NO_XLOCALE */

/* command input buffering */

#if defined(linux) && defined(N_TTY_BUF_SIZE)
# define CMD_BUF_SIZE N_TTY_BUF_SIZE
#else
# ifndef CMD_BUF_SIZE
#  define CMD_BUF_SIZE 4096
# endif
#endif

#ifndef USE_POSIX_THREADS
static unsigned char cmdbuf_base[CMD_BUF_SIZE], *cmdbuf_ptr, *cmdbuf_endp;

#else
unsigned char cmdbuf_base[CMD_BUF_SIZE], *cmdbuf_ptr, *cmdbuf_endp;

#endif

/* local functions referenced */
void privileges(int mode);

RETSIGTYPE Child_signal(int);
RETSIGTYPE Exit_signal(int);
int get_pty(void);
int get_tty(void);
int run_command(char * /* argv */ []);
unsigned char cmd_getc(void);

#if 0
void lookup_key(XEvent * /* ev */ );

#endif
inline void lookup_key(XEvent * /* ev */ );
void process_x_event(XEvent * /* ev */ );

/*static void   process_string (int); */
#ifdef PRINTPIPE
void process_print_pipe(void);

#endif
void process_escape_seq(void);
void process_csi_seq(void);
void process_xterm_seq(void);
void process_window_mode(unsigned int, int[]);
void process_terminal_mode(int, int, unsigned int, int[]);
void process_sgr_mode(unsigned int, int[]);
void process_graphics(void);

void tt_winsize(int);

#ifndef NO_XLOCALE
void init_xlocale(void);

#else
# define init_xlocale() ((void)0)
#endif

/*for Big Paste Handling */
static int v_doPending(void);
static void v_writeBig(int, char *, int);

/*----------------------------------------------------------------------*/

/* Substitutes for missing system functions */
#ifndef _POSIX_VERSION
# if defined (__svr4__)
int
getdtablesize(void)
{
  struct rlimit rlim;

  getrlimit(RLIMIT_NOFILE, &rlim);
  return rlim.rlim_cur;
}
#  endif
# endif

/* Take care of suid/sgid super-user (root) privileges */
void
privileges(int mode)
{

#ifdef __CYGWIN32__
  return;
#endif

  switch (mode) {
    case IGNORE:
      /* Revoke suid/sgid privs and return to normal uid/gid -- mej */
      D_UTMP(("[%ld]: Before privileges(REVERT): [ %ld, %ld ]  [ %ld, %ld ]\n",
	      getpid(), getuid(), getgid(), geteuid(), getegid()));

#ifdef HAVE_SETRESGID
      setresgid(my_rgid, my_rgid, my_egid);
#elif defined(HAVE_SAVED_UIDS)
      setregid(my_rgid, my_rgid);
#else
      setregid(my_egid, -1);
      setregid(-1, my_rgid);
#endif

#ifdef HAVE_SETRESUID
      setresuid(my_ruid, my_ruid, my_euid);
#elif defined(HAVE_SAVED_UIDS)
      setreuid(my_ruid, my_ruid);
#else
      setreuid(my_euid, -1);
      setreuid(-1, my_ruid);
#endif

      D_UTMP(("[%ld]: After privileges(REVERT): [ %ld, %ld ]  [ %ld, %ld ]\n",
	      getpid(), getuid(), getgid(), geteuid(), getegid()));
      break;

    case SAVE:
      break;

    case RESTORE:
      D_UTMP(("[%ld]: Before privileges(INVOKE): [ %ld, %ld ]  [ %ld, %ld ]\n",
	      getpid(), getuid(), getgid(), geteuid(), getegid()));

#ifdef HAVE_SETRESUID
      setresuid(my_ruid, my_euid, my_euid);
#elif defined(HAVE_SAVED_UIDS)
      setreuid(my_ruid, my_euid);
#else
      setreuid(-1, my_euid);
      setreuid(my_ruid, -1);
#endif

#ifdef HAVE_SETRESGID
      setresgid(my_rgid, my_egid, my_egid);
#elif defined(HAVE_SAVED_UIDS)
      setregid(my_rgid, my_egid);
#else
      setregid(-1, my_egid);
      setregid(my_rgid, -1);
#endif

      D_UTMP(("[%ld]: After privileges(INVOKE): [ %ld, %ld ]  [ %ld, %ld ]\n",
	      getpid(), getuid(), getgid(), geteuid(), getegid()));
      break;
  }
}

char *
sig_to_str(int sig)
{

  /* NOTE: This can't be done with a switch because of possible conflicting
   * conflicting signal types. -vendu
   */

#ifdef SIGHUP
  if (sig == SIGHUP) {
    return ("SIGHUP");
  }
#endif

#ifdef SIGINT
  if (sig == SIGINT) {
    return ("SIGINT");
  }
#endif

#ifdef SIGQUIT
  if (sig == SIGQUIT) {
    return ("SIGQUIT");
  }
#endif

#ifdef SIGILL
  if (sig == SIGILL) {
    return ("SIGILL");
  }
#endif

#ifdef SIGTRAP
  if (sig == SIGTRAP) {
    return ("SIGTRAP");
  }
#endif

#ifdef SIGABRT
  if (sig == SIGABRT) {
    return ("SIGABRT");
  }
#endif

#ifdef SIGIOT
  if (sig == SIGIOT) {
    return ("SIGIOT");
  }
#endif

#ifdef SIGEMT
  if (sig == SIGEMT) {
    return ("SIGEMT");
  }
#endif

#ifdef SIGFPE
  if (sig == SIGFPE) {
    return ("SIGFPE");
  }
#endif

#ifdef SIGKILL
  if (sig == SIGKILL) {
    return ("SIGKILL");
  }
#endif

#ifdef SIGBUS
  if (sig == SIGBUS) {
    return ("SIGBUS");
  }
#endif

#ifdef SIGSEGV
  if (sig == SIGSEGV) {
    return ("SIGSEGV");
  }
#endif

#ifdef SIGSYS
  if (sig == SIGSYS) {
    return ("SIGSYS");
  }
#endif

#ifdef SIGPIPE
  if (sig == SIGPIPE) {
    return ("SIGPIPE");
  }
#endif

#ifdef SIGALRM
  if (sig == SIGALRM) {
    return ("SIGALRM");
  }
#endif

#ifdef SIGTERM
  if (sig == SIGTERM) {
    return ("SIGTERM");
  }
#endif

#ifdef SIGUSR1
  if (sig == SIGUSR1) {
    return ("SIGUSR1");
  }
#endif

#ifdef SIGUSR2
  if (sig == SIGUSR2) {
    return ("SIGUSR2");
  }
#endif

#ifdef SIGCHLD
  if (sig == SIGCHLD) {
    return ("SIGCHLD");
  }
#endif

#ifdef SIGCLD
  if (sig == SIGCLD) {
    return ("SIGCLD");
  }
#endif

#ifdef SIGPWR
  if (sig == SIGPWR) {
    return ("SIGPWR");
  }
#endif

#ifdef SIGVTALRM
  if (sig == SIGVTALRM) {
    return ("SIGVTALRM");
  }
#endif

#ifdef SIGPROF
  if (sig == SIGPROF) {
    return ("SIGPROF");
  }
#endif

#ifdef SIGIO
  if (sig == SIGIO) {
    return ("SIGIO");
  }
#endif

#ifdef SIGPOLL
  if (sig == SIGPOLL) {
    return ("SIGPOLL");
  }
#endif

#ifdef SIGWINCH
  if (sig == SIGWINCH) {
    return ("SIGWINCH");
  }
#endif

#ifdef SIGWINDOW
  if (sig == SIGWINDOW) {
    return ("SIGWINDOW");
  }
#endif

#ifdef SIGSTOP
  if (sig == SIGSTOP) {
    return ("SIGSTOP");
  }
#endif

#ifdef SIGTSTP
  if (sig == SIGTSTP) {
    return ("SIGTSTP");
  }
#endif

#ifdef SIGCONT
  if (sig == SIGCONT) {
    return ("SIGCONT");
  }
#endif

#ifdef SIGTTIN
  if (sig == SIGTTIN) {
    return ("SIGTTIN");
  }
#endif

#ifdef SIGTTOU
  if (sig == SIGTTOU) {
    return ("SIGTTOU");
  }
#endif

#ifdef SIGURG
  if (sig == SIGURG) {
    return ("SIGURG");
  }
#endif

#ifdef SIGLOST
  if (sig == SIGLOST) {
    return ("SIGLOST");
  }
#endif

#ifdef SIGRESERVE
  if (sig == SIGRESERVE) {
    return ("SIGRESERVE");
  }
#endif

#ifdef SIGDIL
  if (sig == SIGDIL) {
    return ("SIGDIL");
  }
#endif

#ifdef SIGXCPU
  if (sig == SIGXCPU) {
    return ("SIGXCPU");
  }
#endif

#ifdef SIGXFSZ
  if (sig == SIGXFSZ) {
    return ("SIGXFSZ");
  }
#endif
  return ("Unknown signal");
}

const char *
event_type_to_name(int type)
{

  if (type == KeyPress) {
    return "KeyPress";
  }
  if (type == KeyRelease) {
    return "KeyRelease";
  }
  if (type == ButtonPress) {
    return "ButtonPress";
  }
  if (type == ButtonRelease) {
    return "ButtonRelease";
  }
  if (type == MotionNotify) {
    return "MotionNotify";
  }
  if (type == EnterNotify) {
    return "EnterNotify";
  }
  if (type == LeaveNotify) {
    return "LeaveNotify";
  }
  if (type == FocusIn) {
    return "FocusIn";
  }
  if (type == FocusOut) {
    return "FocusOut";
  }
  if (type == KeymapNotify) {
    return "KeymapNotify";
  }
  if (type == Expose) {
    return "Expose";
  }
  if (type == GraphicsExpose) {
    return "GraphicsExpose";
  }
  if (type == NoExpose) {
    return "NoExpose";
  }
  if (type == VisibilityNotify) {
    return "VisibilityNotify";
  }
  if (type == CreateNotify) {
    return "CreateNotify";
  }
  if (type == DestroyNotify) {
    return "DestroyNotify";
  }
  if (type == UnmapNotify) {
    return "UnmapNotify";
  }
  if (type == MapNotify) {
    return "MapNotify";
  }
  if (type == MapRequest) {
    return "MapRequest";
  }
  if (type == ReparentNotify) {
    return "ReparentNotify";
  }
  if (type == ConfigureNotify) {
    return "ConfigureNotify";
  }
  if (type == ConfigureRequest) {
    return "ConfigureRequest";
  }
  if (type == GravityNotify) {
    return "GravityNotify";
  }
  if (type == ResizeRequest) {
    return "ResizeRequest";
  }
  if (type == CirculateNotify) {
    return "CirculateNotify";
  }
  if (type == CirculateRequest) {
    return "CirculateRequest";
  }
  if (type == PropertyNotify) {
    return "PropertyNotify";
  }
  if (type == SelectionClear) {
    return "SelectionClear";
  }
  if (type == SelectionRequest) {
    return "SelectionRequest";
  }
  if (type == SelectionNotify) {
    return "SelectionNotify";
  }
  if (type == ColormapNotify) {
    return "ColormapNotify";
  }
  if (type == ClientMessage) {
    return "ClientMessage";
  }
  if (type == MappingNotify) {
    return "MappingNotify";
  }
  return "Bad Event!";
}

const char *
request_code_to_name(int code)
{

  if (code == X_CreateWindow) {
    return "XCreateWindow";
  }
  if (code == X_ChangeWindowAttributes) {
    return "XChangeWindowAttributes";
  }
  if (code == X_GetWindowAttributes) {
    return "XGetWindowAttributes";
  }
  if (code == X_DestroyWindow) {
    return "XDestroyWindow";
  }
  if (code == X_DestroySubwindows) {
    return "XDestroySubwindows";
  }
  if (code == X_ChangeSaveSet) {
    return "XChangeSaveSet";
  }
  if (code == X_ReparentWindow) {
    return "XReparentWindow";
  }
  if (code == X_MapWindow) {
    return "XMapWindow";
  }
  if (code == X_MapSubwindows) {
    return "XMapSubwindows";
  }
  if (code == X_UnmapWindow) {
    return "XUnmapWindow";
  }
  if (code == X_UnmapSubwindows) {
    return "XUnmapSubwindows";
  }
  if (code == X_ConfigureWindow) {
    return "XConfigureWindow";
  }
  if (code == X_CirculateWindow) {
    return "XCirculateWindow";
  }
  if (code == X_GetGeometry) {
    return "XGetGeometry";
  }
  if (code == X_QueryTree) {
    return "XQueryTree";
  }
  if (code == X_InternAtom) {
    return "XInternAtom";
  }
  if (code == X_GetAtomName) {
    return "XGetAtomName";
  }
  if (code == X_ChangeProperty) {
    return "XChangeProperty";
  }
  if (code == X_DeleteProperty) {
    return "XDeleteProperty";
  }
  if (code == X_GetProperty) {
    return "XGetProperty";
  }
  if (code == X_ListProperties) {
    return "XListProperties";
  }
  if (code == X_SetSelectionOwner) {
    return "XSetSelectionOwner";
  }
  if (code == X_GetSelectionOwner) {
    return "XGetSelectionOwner";
  }
  if (code == X_ConvertSelection) {
    return "XConvertSelection";
  }
  if (code == X_SendEvent) {
    return "XSendEvent";
  }
  if (code == X_GrabPointer) {
    return "XGrabPointer";
  }
  if (code == X_UngrabPointer) {
    return "XUngrabPointer";
  }
  if (code == X_GrabButton) {
    return "XGrabButton";
  }
  if (code == X_UngrabButton) {
    return "XUngrabButton";
  }
  if (code == X_ChangeActivePointerGrab) {
    return "XChangeActivePointerGrab";
  }
  if (code == X_GrabKeyboard) {
    return "XGrabKeyboard";
  }
  if (code == X_UngrabKeyboard) {
    return "XUngrabKeyboard";
  }
  if (code == X_GrabKey) {
    return "XGrabKey";
  }
  if (code == X_UngrabKey) {
    return "XUngrabKey";
  }
  if (code == X_AllowEvents) {
    return "XAllowEvents";
  }
  if (code == X_GrabServer) {
    return "XGrabServer";
  }
  if (code == X_UngrabServer) {
    return "XUngrabServer";
  }
  if (code == X_QueryPointer) {
    return "XQueryPointer";
  }
  if (code == X_GetMotionEvents) {
    return "XGetMotionEvents";
  }
  if (code == X_TranslateCoords) {
    return "XTranslateCoords";
  }
  if (code == X_WarpPointer) {
    return "XWarpPointer";
  }
  if (code == X_SetInputFocus) {
    return "XSetInputFocus";
  }
  if (code == X_GetInputFocus) {
    return "XGetInputFocus";
  }
  if (code == X_QueryKeymap) {
    return "XQueryKeymap";
  }
  if (code == X_OpenFont) {
    return "XOpenFont";
  }
  if (code == X_CloseFont) {
    return "XCloseFont";
  }
  if (code == X_QueryFont) {
    return "XQueryFont";
  }
  if (code == X_QueryTextExtents) {
    return "XQueryTextExtents";
  }
  if (code == X_ListFonts) {
    return "XListFonts";
  }
  if (code == X_ListFontsWithInfo) {
    return "XListFontsWithInfo";
  }
  if (code == X_SetFontPath) {
    return "XSetFontPath";
  }
  if (code == X_GetFontPath) {
    return "XGetFontPath";
  }
  if (code == X_CreatePixmap) {
    return "XCreatePixmap";
  }
  if (code == X_FreePixmap) {
    return "XFreePixmap";
  }
  if (code == X_CreateGC) {
    return "XCreateGC";
  }
  if (code == X_ChangeGC) {
    return "XChangeGC";
  }
  if (code == X_CopyGC) {
    return "XCopyGC";
  }
  if (code == X_SetDashes) {
    return "XSetDashes";
  }
  if (code == X_SetClipRectangles) {
    return "XSetClipRectangles";
  }
  if (code == X_FreeGC) {
    return "XFreeGC";
  }
  if (code == X_ClearArea) {
    return "XClearArea";
  }
  if (code == X_CopyArea) {
    return "XCopyArea";
  }
  if (code == X_CopyPlane) {
    return "XCopyPlane";
  }
  if (code == X_PolyPoint) {
    return "XPolyPoint";
  }
  if (code == X_PolyLine) {
    return "XPolyLine";
  }
  if (code == X_PolySegment) {
    return "XPolySegment";
  }
  if (code == X_PolyRectangle) {
    return "XPolyRectangle";
  }
  if (code == X_PolyArc) {
    return "XPolyArc";
  }
  if (code == X_FillPoly) {
    return "XFillPoly";
  }
  if (code == X_PolyFillRectangle) {
    return "XPolyFillRectangle";
  }
  if (code == X_PolyFillArc) {
    return "XPolyFillArc";
  }
  if (code == X_PutImage) {
    return "XPutImage";
  }
  if (code == X_GetImage) {
    return "XGetImage";
  }
  if (code == X_PolyText8) {
    return "XPolyText8";
  }
  if (code == X_PolyText16) {
    return "XPolyText16";
  }
  if (code == X_ImageText8) {
    return "XImageText8";
  }
  if (code == X_ImageText16) {
    return "XImageText16";
  }
  if (code == X_CreateColormap) {
    return "XCreateColormap";
  }
  if (code == X_FreeColormap) {
    return "XFreeColormap";
  }
  if (code == X_CopyColormapAndFree) {
    return "XCopyColormapAndFree";
  }
  if (code == X_InstallColormap) {
    return "XInstallColormap";
  }
  if (code == X_UninstallColormap) {
    return "XUninstallColormap";
  }
  if (code == X_ListInstalledColormaps) {
    return "XListInstalledColormaps";
  }
  if (code == X_AllocColor) {
    return "XAllocColor";
  }
  if (code == X_AllocNamedColor) {
    return "XAllocNamedColor";
  }
  if (code == X_AllocColorCells) {
    return "XAllocColorCells";
  }
  if (code == X_AllocColorPlanes) {
    return "XAllocColorPlanes";
  }
  if (code == X_FreeColors) {
    return "XFreeColors";
  }
  if (code == X_StoreColors) {
    return "XStoreColors";
  }
  if (code == X_StoreNamedColor) {
    return "XStoreNamedColor";
  }
  if (code == X_QueryColors) {
    return "XQueryColors";
  }
  if (code == X_LookupColor) {
    return "XLookupColor";
  }
  if (code == X_CreateCursor) {
    return "XCreateCursor";
  }
  if (code == X_CreateGlyphCursor) {
    return "XCreateGlyphCursor";
  }
  if (code == X_FreeCursor) {
    return "XFreeCursor";
  }
  if (code == X_RecolorCursor) {
    return "XRecolorCursor";
  }
  if (code == X_QueryBestSize) {
    return "XQueryBestSize";
  }
  if (code == X_QueryExtension) {
    return "XQueryExtension";
  }
  if (code == X_ListExtensions) {
    return "XListExtensions";
  }
  if (code == X_ChangeKeyboardMapping) {
    return "XChangeKeyboardMapping";
  }
  if (code == X_GetKeyboardMapping) {
    return "XGetKeyboardMapping";
  }
  if (code == X_ChangeKeyboardControl) {
    return "XChangeKeyboardControl";
  }
  if (code == X_GetKeyboardControl) {
    return "XGetKeyboardControl";
  }
  if (code == X_Bell) {
    return "XBell";
  }
  if (code == X_ChangePointerControl) {
    return "XChangePointerControl";
  }
  if (code == X_GetPointerControl) {
    return "XGetPointerControl";
  }
  if (code == X_SetScreenSaver) {
    return "XSetScreenSaver";
  }
  if (code == X_GetScreenSaver) {
    return "XGetScreenSaver";
  }
  if (code == X_ChangeHosts) {
    return "XChangeHosts";
  }
  if (code == X_ListHosts) {
    return "XListHosts";
  }
  if (code == X_SetAccessControl) {
    return "XSetAccessControl";
  }
  if (code == X_SetCloseDownMode) {
    return "XSetCloseDownMode";
  }
  if (code == X_KillClient) {
    return "XKillClient";
  }
  if (code == X_RotateProperties) {
    return "XRotateProperties";
  }
  if (code == X_ForceScreenSaver) {
    return "XForceScreenSaver";
  }
  if (code == X_SetPointerMapping) {
    return "XSetPointerMapping";
  }
  if (code == X_GetPointerMapping) {
    return "XGetPointerMapping";
  }
  if (code == X_SetModifierMapping) {
    return "XSetModifierMapping";
  }
  if (code == X_GetModifierMapping) {
    return "XGetModifierMapping";
  }
  if (code == X_NoOperation) {
    return "XNoOperation";
  }
  return "Unknown";
}

/* Try to get a stack trace when we croak */
#ifdef HAVE_U_STACK_TRACE
extern void U_STACK_TRACE(void);

#endif

void
dump_stack_trace(void)
{

  char cmd[256];

#ifdef NO_STACK_TRACE
  return;
#endif

  print_error("Attempting to dump a stack trace....\n");
  signal(SIGTSTP, exit);	/* Don't block on tty output, just die */

#ifdef HAVE_U_STACK_TRACE
  U_STACK_TRACE();
  return;
#elif defined(GDB)
  snprintf(cmd, sizeof(cmd), "/bin/echo backtrace | " GDB " " APL_NAME " %d", getpid());
#elif defined(PSTACK)
  snprintf(cmd, sizeof(cmd), PSTACK " %d", getpid());
#elif defined(DBX)
#  ifdef _AIX
  snprintf(cmd, sizeof(cmd), "/bin/echo 'where\ndetach' | " DBX " -a %d", getpid());
#  elif defined(__sgi)
  snprintf(cmd, sizeof(cmd), "/bin/echo 'where\ndetach' | " DBX " -p %d", getpid());
#  else
  snprintf(cmd, sizeof(cmd), "/bin/echo 'where\ndetach' | " DBX " %s %d", orig_argv0, getpid());
#  endif
#else
  print_error("Your system does not support any of the methods Eterm uses.  Exiting.\n");
  return;
#endif
  system(cmd);
}

/* signal handling, exit handler */
/*
 * Catch a SIGCHLD signal and exit if the direct child has died
 */
RETSIGTYPE
Child_signal(int sig)
{

  int pid, save_errno = errno;

  D_CMD(("Received signal %s (%d)\n", sig_to_str(sig), sig));

  do {
    errno = 0;
  } while ((-1 == (pid = waitpid(-1, NULL, WNOHANG))) &&
	   (errno == EINTR));

  D_CMD(("pid == %d, cmd_pid == %d\n", pid, cmd_pid));
  /* If the child that exited is the command we spawned, or if the
     child exited before fork() returned in the parent, it must be
     our immediate child that exited.  We exit gracefully. */
  if (pid == cmd_pid || cmd_pid == -1) {
    if (Options & Opt_pause) {
      const char *message = "\r\nPress any key to exit " APL_NAME "....";

      scr_refresh(SMOOTH_REFRESH);
      scr_add_lines(message, 1, strlen(message));
      scr_refresh(SMOOTH_REFRESH);
      keypress_exit = 1;
      return;
    }
    exit(EXIT_SUCCESS);
  }
  errno = save_errno;

  D_CMD(("Child_signal: installing signal handler\n"));
  signal(SIGCHLD, Child_signal);

#ifdef NEED_EXPLICIT_RETURN
  D_CMD(("FUN FUN FUN. Child_signal returned 0 :)\n"));
  return ((RETSIGTYPE) 0);
#endif
}

/* Handles signals usually sent by a user, like HUP, TERM, INT. */
RETSIGTYPE
Exit_signal(int sig)
{

  print_error("Received terminal signal %s (%d)", sig_to_str(sig), sig);
  signal(sig, SIG_DFL);

#ifdef UTMP_SUPPORT
  privileges(INVOKE);
  cleanutent();
  privileges(REVERT);
#endif

  /* No!  This causes unhandled signal propogation! -- mej */
  /* kill(getpid(), sig); */

  D_CMD(("Exit_signal(): exit(%s)\n", sig_to_str(sig)));
  exit(sig);
}

/* Handles abnormal termination signals -- mej */
static RETSIGTYPE
SegvHandler(int sig)
{

  print_error("Received terminal signal %s (%d)", sig_to_str(sig), sig);
  signal(sig, SIG_DFL);		/* Let the OS handle recursive seg faults */

  /* Lock down security so we don't write any core files as root. */
  privileges(REVERT);
  umask(077);

  /* Make an attempt to dump a stack trace */
  dump_stack_trace();

  /* Exit */
  exit(sig);
}

/*
 * Exit gracefully, clearing the utmp entry and restoring tty attributes
 * TODO:  Also free up X resources, etc., if possible
 */
void
clean_exit(void)
{
  scr_release();
  privileges(INVOKE);

#ifndef __CYGWIN32__
  if (ttydev) {
    D_CMD(("Restoring \"%s\" to mode %03o, uid %d, gid %d\n", ttydev, ttyfd_stat.st_mode,
	   ttyfd_stat.st_uid, ttyfd_stat.st_gid));
    if (chmod(ttydev, ttyfd_stat.st_mode) != 0) {
      D_UTMP(("chmod(\"%s\", %03o) failed:  %s\n", ttydev, ttyfd_stat.st_mode, strerror(errno)));
    }
    if (chown(ttydev, ttyfd_stat.st_uid, ttyfd_stat.st_gid) != 0) {
      D_UTMP(("chown(\"%s\", %d, %d) failed:  %s\n", ttydev, ttyfd_stat.st_uid, ttyfd_stat.st_gid, strerror(errno)));
    }
  }
#endif /* __CYGWIN32__ */

#ifdef UTMP_SUPPORT
  cleanutent();
#endif
  privileges(REVERT);
#ifdef USE_POSIX_THREADS
  /* Get rid of threads if there are any running. Doesn't work yet. */
# if 0
  D_THREADS(("pthread_kill_other_threads_np();\n"));
  pthread_kill_other_threads_np();
  D_THREADS(("pthread_exit();\n"));
# endif
#endif

  /* Work around a nasty Solaris X bug.  If we're not unmapped in
     3 seconds, it ain't gonna happen.  Die anyway. -- mej */
#ifdef HAVE__EXIT
  signal(SIGALRM, _exit);
#else
  signal(SIGALRM, abort);
#endif
  alarm(3);

  /* Close the display connection to the X server. Unmap the windows
   * first.
   */
  D_X11(("XUnmapWindow(Xdisplay, TermWin.parent);\n"));
  XUnmapWindow(Xdisplay, TermWin.parent);
  D_X11(("XSync(Xdisplay, TRUE) - discarding events\n"));
  /* XSync discards all events in the event queue. */
  XSync(Xdisplay, TRUE);
  D_X11(("XCloseDisplay(Xdisplay);\n"));
  XCloseDisplay(Xdisplay);
}
#if (MENUBAR_MAX)
inline void
map_menuBar(int map)
{

  if (delay_menu_drawing) {
    delay_menu_drawing++;
  } else if (menubar_mapping(map)) {
    resize();
  }
  PrivMode(map, PrivMode_menuBar);
}
#endif /* MENUBAR_MAX */

inline void
map_scrollBar(int map)
{

  if (scrollbar_mapping(map)) {
    scr_touch();
    resize();
  }
  PrivMode(map, PrivMode_scrollBar);
}


/* Returns true if running under E, false otherwise */
inline unsigned char
check_for_enlightenment(void)
{
  static char have_e = -1;

  if (have_e == -1) {
    if (XInternAtom(Xdisplay, "ENLIGHTENMENT_COMMS", True) != None) {
      D_X11(("Enlightenment detected.\n"));
      have_e = 1;
    } else {
      D_X11(("Enlightenment not detected.\n"));
      have_e = 0;
    }
  }
  return (have_e);
}

/* Acquire a pseudo-teletype from the system. */
/*
 * On failure, returns -1.
 * On success, returns the file descriptor.
 *
 * If successful, ttydev and ptydev point to the names of the
 * master and slave parts
 */

#ifdef __sgi
inline int
sgi_get_pty(void)
{

  int fd = -1;

  ptydev = ttydev = _getpty(&fd, O_RDWR | O_NDELAY, 0622, 0);
  return (ptydev == NULL ? -1 : fd);

}
#endif

#ifdef _AIX
inline int
aix_get_pty(void)
{

  int fd = -1;

  if ((fd = open("/dev/ptc", O_RDWR)) < 0)
    return (-1);
  else
    ptydev = ttydev = ttyname(fd);
  return (fd);
}
#endif

#ifdef ALL_NUMERIC_PTYS
inline int
sco_get_pty(void)
{

  static char pty_name[] = "/dev/ptyp??\0\0\0";
  static char tty_name[] = "/dev/ttyp??\0\0\0";
  int idx;
  int fd = -1;

  ptydev = pty_name;
  ttydev = tty_name;

  for (idx = 0; idx < 256; idx++) {

    sprintf(ptydev, "%s%d", "/dev/ptyp", idx);
    sprintf(ttydev, "%s%d", "/dev/ttyp", idx);

    if (access(ttydev, F_OK) < 0) {
      idx = 256;
      break;
    }
    if ((fd = open(ptydev, O_RDWR)) >= 0) {
      if (access(ttydev, R_OK | W_OK) == 0)
	return (fd);
      close(fd);
    }
  }
  return (-1);
}
#endif

#if defined (__svr4__) || defined(__CYGWIN32__) || ((__GLIBC__ >= 2) && (__GLIBC_MINOR__ >= 1))
inline int
svr_get_pty(void)
{

  int fd = -1;

  /* open the STREAMS, clone device /dev/ptmx (master pty) */
  if ((fd = open("/dev/ptmx", O_RDWR)) < 0) {
    return (-1);
  } else {
    if (grantpt(fd) != 0) {
      print_error("grantpt(%d) failed:  %s\n", fd, strerror(errno));
      return (-1);
    } else if (unlockpt(fd) != 0) {
      print_error("unlockpt(%d) failed:  %s\n", fd, strerror(errno));
      return (-1);
    } else {
      ptydev = ttydev = ptsname(fd);
      if (ttydev == NULL) {
	print_error("ptsname(%d) failed:  %s\n", fd, strerror(errno));
	return (-1);
      }
    }
  }
  return (fd);
}
#endif

#define PTYCHAR1 "pqrstuvwxyz"
#define PTYCHAR2 "0123456789abcdefghijklmnopqrstuvwxyz"

inline int
gen_get_pty(void)
{

  static char pty_name[] = "/dev/pty??";
  static char tty_name[] = "/dev/tty??";
  int len = sizeof(tty_name);
  char *c1, *c2;
  int fd = -1;

  ptydev = pty_name;
  ttydev = tty_name;

  for (c1 = PTYCHAR1; *c1; c1++) {
    ptydev[len - 3] = ttydev[len - 3] = *c1;
    for (c2 = PTYCHAR2; *c2; c2++) {
      ptydev[len - 2] = ttydev[len - 2] = *c2;
      if ((fd = open(ptydev, O_RDWR)) >= 0) {
	if (access(ttydev, R_OK | W_OK) == 0)
	  return (fd);
	close(fd);
      }
    }
  }
  return (-1);
}

int
get_pty(void)
{

  int fd = -1;

#if defined(__sgi)
  fd = sgi_get_pty();
#elif defined(_AIX)
  fd = aix_get_pty();
#elif defined(__svr4__) || defined(__CYGWIN32__)
  fd = svr_get_pty();
#elif ((__GLIBC__ >= 2) && (__GLIBC_MINOR__ >= 1))
  fd = svr_get_pty();
#elif defined(ALL_NUMERIC_PTYS)	/* SCO OSr5 */
  fd = sco_get_pty();
#endif

  /* Fall back on this method */
  if (fd == -1) {
    fd = gen_get_pty();
  }
  if (fd != -1) {
    fcntl(fd, F_SETFL, O_NDELAY);
    return (fd);
  } else {
    print_error("Can't open pseudo-tty -- %s", strerror(errno));
    return (-1);
  }
}

/* establish a controlling teletype for new session */
/*
 * On some systems this can be done with ioctl() but on others we
 * need to re-open the slave tty.
 */
int
get_tty(void)
{
  int fd;
  pid_t pid;

  /*
   * setsid() [or setpgrp] must be before open of the terminal,
   * otherwise there is no controlling terminal (Solaris 2.4, HP-UX 9)
   */
#ifndef ultrix
# ifdef NO_SETSID
  pid = setpgrp(0, 0);
# else
  pid = setsid();
# endif
  if (pid < 0) {
    D_TTYMODE(("%s: setsid() failed: %s, PID == %d\n", rs_name, strerror(errno), pid));
  }
#endif /* ultrix */

  privileges(INVOKE);
  if (ttydev == NULL) {
    print_error("Slave tty device name is NULL.  Failed to open slave pty.\n");
    exit(EXIT_FAILURE);
  } else if ((fd = open(ttydev, O_RDWR)) < 0) {
    print_error("Can't open slave tty %s -- %s", ttydev, strerror(errno));
    exit(EXIT_FAILURE);
  } else {
    D_TTY(("Opened slave tty %s\n", ttydev));
    privileges(REVERT);
  }

#if defined (__svr4__)
  /*
   * Push STREAMS modules:
   *  ptem: pseudo-terminal hardware emulation module.
   *  ldterm: standard terminal line discipline.
   *  ttcompat: V7, 4BSD and XENIX STREAMS compatibility module.
   */
  ioctl(fd, I_PUSH, "ptem");
  ioctl(fd, I_PUSH, "ldterm");
  ioctl(fd, I_PUSH, "ttcompat");
#else /* __svr4__ */
  {
    /* change ownership of tty to real uid and real group */
    unsigned int mode = 0620;
    gid_t gid = my_rgid;

# ifdef USE_GETGRNAME
    {
      struct group *gr = getgrnam(TTY_GRP_NAME);

      if (gr) {
	/* change ownership of tty to real uid, "tty" gid */
	gid = gr->gr_gid;
	mode = 0620;
      }
    }
# endif				/* USE_GETGRNAME */

    privileges(INVOKE);
# ifndef __CYGWIN32__
    fchown(fd, my_ruid, gid);	/* fail silently */
    fchmod(fd, mode);
# endif
    privileges(REVERT);
  }
#endif /* __svr4__ */

  /*
   * Close all file descriptors.  If only stdin/out/err are closed,
   * child processes remain alive upon deletion of the window.
   */
  {
    int i;

    for (i = 0; i < num_fds; i++) {
      if (i != fd)
	close(i);
    }
  }

  /* Reopen stdin, stdout and stderr over the tty file descriptor */
  dup(fd);			/* 0: stdin */
  dup(fd);			/* 1: stdout */
  dup(fd);			/* 2: stderr */

  if (fd > 2)
    close(fd);

  privileges(INVOKE);

#ifdef ultrix
  if ((fd = open("/dev/tty", O_RDONLY)) >= 0) {
    ioctl(fd, TIOCNOTTY, 0);
    close(fd);
  } else {
# ifdef NO_SETSID
    pid = setpgrp(0, 0);
# else
    pid = setsid();
# endif
    if (pid < 0) {
      D_TTYMODE(("%s: setsid() failed: %s, PID == %d\n", rs_name, strerror(errno), pid));
    }
  }

  /* no error, we could run with no tty to begin with */
#else /* ultrix */

# ifdef TIOCSCTTY
  ioctl(0, TIOCSCTTY, 0);
# endif

  /* set process group */
# if defined (_POSIX_VERSION) || defined (__svr4__)
  tcsetpgrp(0, pid);
# elif defined (TIOCSPGRP)
  ioctl(0, TIOCSPGRP, &pid);
# endif

  /* svr4 problems: reports no tty, no job control */
  /* # if !defined (__svr4__) && defined (TIOCSPGRP) */

  close(open(ttydev, O_RDWR, 0));
  /* # endif */
#endif /* ultrix */

  privileges(REVERT);

  return (fd);
}

/* debug_ttymode() */
#if DEBUG >= DEBUG_TTYMODE && defined(HAVE_TERMIOS_H)

  /* cpp token stringize doesn't work on all machines <sigh> */
#  define SHOW_TTY_FLAG(flag,name) do { \
                                     if ((ttymode->c_iflag) & (flag)) { \
                                       fprintf(stderr, "+%s ", name); \
                                     } else { \
                                       fprintf(stderr, "-%s ", name); \
                                     } \
                                   } while (0)
#  define SHOW_CONT_CHAR(entry, name) fprintf(stderr, "%s=%#3o ", name, ttymode->c_cc[entry])
static void
debug_ttymode(ttymode_t * ttymode)
{

  /* c_iflag bits */
  fprintf(stderr, "Input flags:  ");
  SHOW_TTY_FLAG(IGNBRK, "IGNBRK");
  SHOW_TTY_FLAG(BRKINT, "BRKINT");
  SHOW_TTY_FLAG(IGNPAR, "IGNPAR");
  SHOW_TTY_FLAG(PARMRK, "PARMRK");
  SHOW_TTY_FLAG(INPCK, "INPCK");
  SHOW_TTY_FLAG(ISTRIP, "ISTRIP");
  SHOW_TTY_FLAG(INLCR, "INLCR");
  SHOW_TTY_FLAG(IGNCR, "IGNCR");
  SHOW_TTY_FLAG(ICRNL, "ICRNL");
  SHOW_TTY_FLAG(IXON, "IXON");
  SHOW_TTY_FLAG(IXOFF, "IXOFF");
#  ifdef IUCLC
  SHOW_TTY_FLAG(IUCLC, "IUCLC");
#  endif
#  ifdef IXANY
  SHOW_TTY_FLAG(IXANY, "IXANY");
#  endif
#  ifdef IMAXBEL
  SHOW_TTY_FLAG(IMAXBEL, "IMAXBEL");
#  endif
  fprintf(stderr, "\n");

  fprintf(stderr, "Control character mappings:  ");
  SHOW_CONT_CHAR(VINTR, "VINTR");
  SHOW_CONT_CHAR(VQUIT, "VQUIT");
  SHOW_CONT_CHAR(VERASE, "VERASE");
  SHOW_CONT_CHAR(VKILL, "VKILL");
  SHOW_CONT_CHAR(VEOF, "VEOF");
  SHOW_CONT_CHAR(VEOL, "VEOL");
#  ifdef VEOL2
  SHOW_CONT_CHAR(VEOL2, "VEOL2");
#  endif
#  ifdef VSWTC
  SHOW_CONT_CHAR(VSWTC, "VSWTC");
#  endif
#  ifdef VSWTCH
  SHOW_CONT_CHAR(VSWTCH, "VSWTCH");
#  endif
  SHOW_CONT_CHAR(VSTART, "VSTART");
  SHOW_CONT_CHAR(VSTOP, "VSTOP");
  SHOW_CONT_CHAR(VSUSP, "VSUSP");
#  ifdef VDSUSP
  SHOW_CONT_CHAR(VDSUSP, "VDSUSP");
#  endif
#  ifdef VREPRINT
  SHOW_CONT_CHAR(VREPRINT, "VREPRINT");
#  endif
#  ifdef VDISCRD
  SHOW_CONT_CHAR(VDISCRD, "VDISCRD");
#  endif
#  ifdef VWERSE
  SHOW_CONT_CHAR(VWERSE, "VWERSE");
#  endif
#  ifdef VLNEXT
  SHOW_CONT_CHAR(VLNEXT, "VLNEXT");
#  endif
  fprintf(stderr, "\n\n");
}

#  undef SHOW_TTY_FLAG
#  undef SHOW_CONT_CHAR
#endif /* DEBUG_TTYMODE */

/* get_ttymode() */
static void
get_ttymode(ttymode_t * tio)
{
#ifdef HAVE_TERMIOS_H
  /*
   * standard System V termios interface
   */
  if (GET_TERMIOS(0, tio) < 0) {
    /* return error - use system defaults */
    tio->c_cc[VINTR] = CINTR;
    tio->c_cc[VQUIT] = CQUIT;
    tio->c_cc[VERASE] = CERASE;
    tio->c_cc[VKILL] = CKILL;
    tio->c_cc[VSTART] = CSTART;
    tio->c_cc[VSTOP] = CSTOP;
    tio->c_cc[VSUSP] = CSUSP;
# ifdef VDSUSP
    tio->c_cc[VDSUSP] = CDSUSP;
# endif
# ifdef VREPRINT
    tio->c_cc[VREPRINT] = CRPRNT;
# endif
# ifdef VDISCRD
    tio->c_cc[VDISCRD] = CFLUSH;
# endif
# ifdef VWERSE
    tio->c_cc[VWERSE] = CWERASE;
# endif
# ifdef VLNEXT
    tio->c_cc[VLNEXT] = CLNEXT;
# endif
  }
  tio->c_cc[VEOF] = CEOF;
  tio->c_cc[VEOL] = VDISABLE;
# ifdef VEOL2
  tio->c_cc[VEOL2] = VDISABLE;
# endif
# ifdef VSWTC
  tio->c_cc[VSWTC] = VDISABLE;
# endif
# ifdef VSWTCH
  tio->c_cc[VSWTCH] = VDISABLE;
# endif
# if VMIN != VEOF
  tio->c_cc[VMIN] = 1;
# endif
# if VTIME != VEOL
  tio->c_cc[VTIME] = 0;
# endif

  /* input modes */
  tio->c_iflag = (BRKINT | IGNPAR | ICRNL | IXON
# ifdef IMAXBEL
		  | IMAXBEL
# endif
      );

  /* output modes */
  tio->c_oflag = (OPOST | ONLCR);

  /* control modes */
  tio->c_cflag = (CS8 | CREAD);

  /* line discipline modes */
  tio->c_lflag = (ISIG | ICANON | IEXTEN | ECHO | ECHOE | ECHOK
# if defined (ECHOCTL) && defined (ECHOKE)
		  | ECHOCTL | ECHOKE
# endif
      );

  /*
   * guess an appropriate value for Backspace
   */

#if defined(FORCE_BACKSPACE) && 0
  PrivMode(1, PrivMode_BackSpace);
  tio->c_cc[VERASE] = '\b';	/* force ^H for stty setting...   */
  SET_TERMIOS(0, tio);		/*  ...and make it stick -- casey */
#elif defined(FORCE_DELETE) && 0
  PrivMode(0, PrivMode_BackSpace);
  tio->c_cc[VERASE] = 0x7f;	/* force ^? for stty setting...   */
  SET_TERMIOS(0, tio);		/*  ...and make it stick -- casey */
#else
  PrivMode((tio->c_cc[VERASE] == '\b'), PrivMode_BackSpace);
#endif

#else /* HAVE_TERMIOS_H */

  /*
   * sgtty interface
   */

  /* get parameters -- gtty */
  if (ioctl(0, TIOCGETP, &(tio->sg)) < 0) {
    tio->sg.sg_erase = CERASE;	/* ^H */
    tio->sg.sg_kill = CKILL;	/* ^U */
  }
  tio->sg.sg_flags = (CRMOD | ECHO | EVENP | ODDP);

  /* get special characters */
  if (ioctl(0, TIOCGETC, &(tio->tc)) < 0) {
    tio->tc.t_intrc = CINTR;	/* ^C */
    tio->tc.t_quitc = CQUIT;	/* ^\ */
    tio->tc.t_startc = CSTART;	/* ^Q */
    tio->tc.t_stopc = CSTOP;	/* ^S */
    tio->tc.t_eofc = CEOF;	/* ^D */
    tio->tc.t_brkc = -1;
  }
  /* get local special chars */
  if (ioctl(0, TIOCGLTC, &(tio->lc)) < 0) {
    tio->lc.t_suspc = CSUSP;	/* ^Z */
    tio->lc.t_dsuspc = CDSUSP;	/* ^Y */
    tio->lc.t_rprntc = CRPRNT;	/* ^R */
    tio->lc.t_flushc = CFLUSH;	/* ^O */
    tio->lc.t_werasc = CWERASE;	/* ^W */
    tio->lc.t_lnextc = CLNEXT;	/* ^V */
  }
  /* get line discipline */
  ioctl(0, TIOCGETD, &(tio->line));
# ifdef NTTYDISC
  tio->line = NTTYDISC;
# endif				/* NTTYDISC */
  tio->local = (LCRTBS | LCRTERA | LCTLECH | LPASS8 | LCRTKIL);

  /*
   * guess an appropriate value for Backspace
   */

# ifdef FORCE_BACKSPACE
  PrivMode(1, PrivMode_BackSpace);
  tio->sg.sg_erase = '\b';
  SET_TTYMODE(0, tio);
# elif defined (FORCE_DELETE)
  PrivMode(0, PrivMode_BackSpace);
  tio->sg.sg_erase = 0x7f;
  SET_TTYMODE(0, tio);
# else
  PrivMode((tio->sg.sg_erase == '\b'), PrivMode_BackSpace);
# endif

#endif /* HAVE_TERMIOS_H */
}

/* run_command() */
/*
 * Run the command in a subprocess and return a file descriptor for the
 * master end of the pseudo-teletype pair with the command talking to
 * the slave.
 */
int
run_command(char *argv[])
{

  ttymode_t tio;
  int ptyfd;

  /* Save and then give up any super-user privileges */
  privileges(IGNORE);

  ptyfd = get_pty();
  if (ptyfd < 0)
    return (-1);

  /* store original tty status for restoration clean_exit() -- rgg 04/12/95 */
  lstat(ttydev, &ttyfd_stat);
  D_CMD(("Original settings of %s are mode %o, uid %d, gid %d\n", ttydev, ttyfd_stat.st_mode,
	 ttyfd_stat.st_uid, ttyfd_stat.st_gid));

  /* install exit handler for cleanup */
#ifdef HAVE_ATEXIT
  atexit(clean_exit);
#else
# if defined (__sun__)
  on_exit(clean_exit, NULL);	/* non-ANSI exit handler */
# else
  print_error("no atexit(), UTMP entries can't be cleaned");
# endif
#endif

  /*
   * get tty settings before fork()
   * and make a reasonable guess at the value for BackSpace
   */
  get_ttymode(&tio);
  /* add Backspace value */
  SavedModes |= (PrivateModes & PrivMode_BackSpace);

  /* add value for scrollBar */
  if (scrollbar_visible()) {
    PrivateModes |= PrivMode_scrollBar;
    SavedModes |= PrivMode_scrollBar;
  }
  if (menubar_visible()) {
    PrivateModes |= PrivMode_menuBar;
    SavedModes |= PrivMode_menuBar;
  }
#if DEBUG >= DEBUG_TTYMODE && defined(HAVE_TERMIOS_H)
  if (debug_level >= DEBUG_TTYMODE) {
    debug_ttymode(&tio);
  }
#endif

  /* spin off the command interpreter */
  signal(SIGHUP, Exit_signal);
#ifndef __svr4__
  signal(SIGINT, Exit_signal);
#endif
  signal(SIGQUIT, SegvHandler);
  signal(SIGTERM, Exit_signal);
  signal(SIGCHLD, Child_signal);
  signal(SIGSEGV, SegvHandler);
  signal(SIGBUS, SegvHandler);
  signal(SIGABRT, SegvHandler);
  signal(SIGFPE, SegvHandler);
  signal(SIGILL, SegvHandler);
  signal(SIGSYS, SegvHandler);

  /* need to trap SIGURG for SVR4 (Unixware) rlogin */
  /* signal (SIGURG, SIG_DFL); */

  D_CMD(("run_command(): forking\n"));
  cmd_pid = fork();
  D_CMD(("After fork(), cmd_pid == %d\n", cmd_pid));
  if (cmd_pid < 0) {
    print_error("fork(): %s", strerror(errno));
    return (-1);
  }
  if (cmd_pid == 0) {		/* child */

    /* signal (SIGHUP, Exit_signal); */
    /* signal (SIGINT, Exit_signal); */
#ifdef HAVE_UNSETENV
    /* avoid passing old settings and confusing term size */
    unsetenv("LINES");
    unsetenv("COLUMNS");
    /* avoid passing termcap since terminfo should be okay */
    unsetenv("TERMCAP");
#endif /* HAVE_UNSETENV */
    /* establish a controlling teletype for the new session */
    get_tty();

    /* initialize terminal attributes */
    SET_TTYMODE(0, &tio);

    /* become virtual console, fail silently */
    if (Options & Opt_console) {
      int fd = 1;

      privileges(INVOKE);
#ifdef SRIOCSREDIR
      fd = open(CONSOLE, O_WRONLY);
      if (fd < 0 || ioctl(fd, SRIOCSREDIR, 0) < 0) {
	if (fd >= 0)
	  close(fd);
      }
#elif defined(TIOCCONS)
      ioctl(0, TIOCCONS, &fd);
#endif /* SRIOCSREDIR */
      privileges(REVERT);
    }
    tt_winsize(0);		/* set window size */

    /* Permanently revoke all privileges for the child process.  
       Root shells for everyone are tres uncool.... ;^) -- mej */
#ifdef _HPUX_SOURCE
    setresuid(my_ruid, my_ruid, my_euid);
    setresgid(my_rgid, my_rgid, my_egid);
#else
    /* No special treatment is needed for systems with saved uids/gids,
       because the exec*() calls reset the saved uid/gid to the
       effective uid/gid                               -- mej */
# ifndef __CYGWIN32__
    setregid(my_rgid, my_rgid);
    setreuid(my_ruid, my_ruid);
# endif				/* __CYGWIN32__ */
#endif /* _HPUX_SOURCE */

    D_UTMP(("Child process reset\n"));
    my_euid = my_ruid;
    my_egid = my_rgid;

    /* reset signals and spin off the command interpreter */
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
    signal(SIGSEGV, SIG_DFL);
    signal(SIGBUS, SIG_DFL);
    signal(SIGABRT, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
    signal(SIGILL, SIG_DFL);
    signal(SIGSYS, SIG_DFL);
    signal(SIGALRM, SIG_DFL);

    /*
     * mimick login's behavior by disabling the job control signals
     * a shell that wants them can turn them back on
     */
#ifdef SIGTSTP
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
#endif /* SIGTSTP */

    /* command interpreter path */
    D_CMD(("[%d] About to spawn shell\n", getpid()));
    chdir(initial_dir);
    if (argv != NULL) {
#if DEBUG >= DEBUG_CMD
      if (debug_level >= DEBUG_CMD) {
	int i;

	for (i = 0; argv[i]; i++) {
	  DPRINTF1(("argv[%d] = \"%s\"\n", i, argv[i]));
	}
      }
#endif
      execvp(argv[0], argv);
      print_error("execvp() failed, cannot execute \"%s\": %s", argv[0], strerror(errno));
    } else {

      const char *argv0, *shell;

      if ((shell = getenv("SHELL")) == NULL || *shell == '\0')
	shell = "/bin/sh";

      argv0 = my_basename(shell);
      if (Options & Opt_loginShell) {
	char *p = MALLOC((strlen(argv0) + 2) * sizeof(char));

	p[0] = '-';
	strcpy(&p[1], argv0);
	argv0 = p;
      }
      execlp(shell, argv0, NULL);
      print_error("execlp() failed, cannot execute \"%s\": %s", shell, strerror(errno));
    }
    sleep(3);			/* Sleep to make sure fork() returns in the parent, and so user can read error message */
    exit(EXIT_FAILURE);
  }
#ifdef UTMP_SUPPORT
  privileges(RESTORE);
  if (Options & Opt_utmpLogging)
    makeutent(ttydev, display_name);	/* stamp /etc/utmp */
  privileges(IGNORE);
#endif

#if 0
  D_THREADS(("run_command(): pthread_join(resize_sub_thr)\n"));
  pthread_join(resize_sub_thr, NULL);
#endif

  D_CMD(("run_command() returning\n"));
  return (ptyfd);
}

/* init_command() */
void
init_command(char *argv[])
{

  /* Initialize the command connection.        This should be called after
     the X server connection is established. */

  /* Enable delete window protocol */
  wmDeleteWindow = XInternAtom(Xdisplay, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(Xdisplay, TermWin.parent, &wmDeleteWindow, 1);

#ifdef OFFIX_DND
  /* Enable OffiX Dnd (drag 'n' drop) protocol */
  DndProtocol = XInternAtom(Xdisplay, "DndProtocol", False);
  DndSelection = XInternAtom(Xdisplay, "DndSelection", False);
#endif /* OFFIX_DND */

  init_xlocale();

  /* get number of available file descriptors */
#ifdef _POSIX_VERSION
  num_fds = sysconf(_SC_OPEN_MAX);
#else
  num_fds = getdtablesize();
#endif

#ifdef META8_OPTION
  meta_char = (Options & Opt_meta8 ? 0x80 : 033);
#endif

#ifdef GREEK_SUPPORT
  greek_init();
#endif

  Xfd = XConnectionNumber(Xdisplay);
  D_CMD(("Xfd = %d\n", Xfd));
  cmdbuf_ptr = cmdbuf_endp = cmdbuf_base;

  if ((cmd_fd = run_command(argv)) < 0) {
    print_error("aborting");
    exit(EXIT_FAILURE);
  }
}

/* Xlocale */
#ifndef NO_XLOCALE
inline const char *
get_input_style_flags(XIMStyle style)
{

  static char style_buff[256];

  strcpy(style_buff, "(");
  if (style & XIMPreeditCallbacks) {
    strcat(style_buff, "XIMPreeditCallbacks");
  } else if (style & XIMPreeditPosition) {
    strcat(style_buff, "XIMPreeditPosition");
  } else if (style & XIMPreeditArea) {
    strcat(style_buff, "XIMPreeditArea");
  } else if (style & XIMPreeditNothing) {
    strcat(style_buff, "XIMPreeditNothing");
  } else if (style & XIMPreeditNone) {
    strcat(style_buff, "XIMPreeditNone");
  }
  strcat(style_buff, " | ");
  if (style & XIMStatusCallbacks) {
    strcat(style_buff, "XIMStatusCallbacks");
  } else if (style & XIMStatusArea) {
    strcat(style_buff, "XIMStatusArea");
  } else if (style & XIMStatusNothing) {
    strcat(style_buff, "XIMStatusNothing");
  } else if (style & XIMStatusNone) {
    strcat(style_buff, "XIMStatusNone");
  }
  strcat(style_buff, ")");
  return style_buff;
}

void
init_xlocale(void)
{

  char *p, *s, buf[32], tmp[1024];
  XIM xim = NULL;
  XIMStyle input_style;
  XIMStyles *xim_styles = NULL;
  int found, i, mc;
  XFontSet fontset = 0;
  char *fontname, **ml, *ds;
  XVaNestedList list;
  const char fs_base[] = ",-misc-fixed-*-r-*-*-*-120-*-*-*-*-*-*";

  D_X11(("Initializing X locale and Input Method...\n"));
  Input_Context = NULL;
  if (rs_inputMethod && strlen(rs_inputMethod) >= sizeof(tmp)) {
    print_error("Input Method too long, ignoring.");
    rs_inputMethod = NULL;
  }
# ifdef KANJI
  setlocale(LC_CTYPE, "");
# else
  if (rs_inputMethod && !*rs_inputMethod) {
    rs_inputMethod = NULL;
  }
# endif

  if (rs_inputMethod == NULL) {
    if ((p = XSetLocaleModifiers("@im=none")) != NULL && *p) {
      xim = XOpenIM(Xdisplay, NULL, NULL, NULL);
    }
  } else {
    strcpy(tmp, rs_inputMethod);
    for (s = tmp; *s; /*nil */ ) {

      char *end, *next_s;

      for (; *s && isspace(*s); s++);
      if (!*s)
	break;
      end = s;
      for (; *end && (*end != ','); end++);
      next_s = end--;
      for (; (end >= s) && isspace(*end); end--);
      *(end + 1) = '\0';

      if (*s) {
	snprintf(buf, sizeof(buf), "@im=%s", s);
	if ((p = XSetLocaleModifiers(buf)) != NULL && *p && (xim = XOpenIM(Xdisplay, NULL, NULL, NULL)) != NULL) {
	  break;
	}
      }
      if (!*next_s)
	break;
      s = (next_s + 1);
    }
  }

  if (xim == NULL && (p = XSetLocaleModifiers("")) != NULL && *p) {
    xim = XOpenIM(Xdisplay, NULL, NULL, NULL);
  }
  if (xim == NULL) {
    D_X11(("Error:  Failed to open Input Method\n"));
    return;
  } else {
    D_X11(("Opened X Input Method.  xim == 0x%08x\n", xim));
  }

  if (XGetIMValues(xim, XNQueryInputStyle, &xim_styles, NULL) || !xim_styles) {
    D_X11(("Error:  Input Method doesn't support any style\n"));
    XCloseIM(xim);
    return;
  } else {
    D_X11((" -> Input Method supports %d styles.\n", xim_styles->count_styles));
  }

  /* We only support the Root preedit type */
  input_style = (XIMPreeditNothing | XIMStatusNothing);
  D_X11((" -> input_style == 0x%08x\n", input_style));

  for (i = 0, found = 0; i < xim_styles->count_styles; i++) {
    D_X11((" -> Supported style flags:  0x%08x %s\n", xim_styles->supported_styles[i], get_input_style_flags(xim_styles->supported_styles[i])));
    D_X11(("     -> 0x%08x %s\n", xim_styles->supported_styles[i] & input_style,
	   get_input_style_flags(xim_styles->supported_styles[i] & input_style)));
    if ((xim_styles->supported_styles[i] & input_style) == (xim_styles->supported_styles[i])) {
      input_style = xim_styles->supported_styles[i];
      found = 1;
      break;
    }
  }
  XFree(xim_styles);

  if (!found) {
    D_X11(("Error:  " APL_NAME " " VERSION " only supports the \"Root\" preedit type, which the Input Method does not support.\n"));
    XCloseIM(xim);
    return;
  }
  /* Create Font Set */

#ifdef KANJI
  fontname = MALLOC(strlen(rs_font[0]) + strlen(rs_kfont[0]) + sizeof(fs_base) + 2);
  if (fontname) {
    strcpy(fontname, rs_font[0]);
    strcat(fontname, fs_base);
    strcat(fontname, ",");
    strcat(fontname, rs_kfont[0]);
  }
#else
  fontname = MALLOC(strlen(rs_font[0]) + sizeof(fs_base) + 1);
  if (fontname) {
    strcpy(fontname, rs_font[0]);
    strcat(fontname, fs_base);
  }
#endif
  if (fontname) {
    setlocale(LC_ALL, "");
    fontset = XCreateFontSet(Xdisplay, fontname, &ml, &mc, &ds);
    FREE(fontname);
    if (mc) {
      XFreeStringList(ml);
      fontset = 0;
      return;
    }
  }
  list = XVaCreateNestedList(0, XNFontSet, fontset, NULL);
  Input_Context = XCreateIC(xim, XNInputStyle, input_style, XNClientWindow, TermWin.parent, XNFocusWindow, TermWin.parent,
			    XNPreeditAttributes, list, XNStatusAttributes, list, NULL);
  if (Input_Context == NULL) {
    D_X11(("Error:  Unable to create Input Context\n"));
    XCloseIM(xim);
  }
}
#endif /* NO_XLOCALE */

/* window resizing */
/*
 * Tell the teletype handler what size the window is.
 * Called after a window size change.
 */
void
tt_winsize(int fd)
{

  struct winsize ws;

  if (fd < 0)
    return;

  ws.ws_col = (unsigned short) TermWin.ncol;
  ws.ws_row = (unsigned short) TermWin.nrow;
#ifndef __CYGWIN32__
  ws.ws_xpixel = ws.ws_ypixel = 0;
#endif
  ioctl(fd, TIOCSWINSZ, &ws);
}

void
tt_resize(void)
{
  tt_winsize(cmd_fd);
}

/* Convert the keypress event into a string */
inline void
lookup_key(XEvent * ev)
{

  static int numlock_state = 0;
  static unsigned char kbuf[KBUFSZ];
  int ctrl, meta, shft, len;
  KeySym keysym;

#ifndef NO_XLOCALE
  static Status status_return;

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
#ifndef NO_XLOCALE
  if (Input_Context != NULL) {
    len = XmbLookupString(Input_Context, &ev->xkey, kbuf, sizeof(kbuf), &keysym, &status_return);
    if (status_return == XLookupNone) {
      D_X11(("XmbLookupString() returned len == %d, status_return == XLookupNone\n", len));
      len = 0;
    } else if (status_return == XBufferOverflow) {
      D_X11(("XmbLookupString() returned len == %d, status_return == XBufferOverflow\n", len));
      print_error("XmbLookupString():  Buffer overflow, string too long.");
      XmbResetIC(Input_Context);
    } else if (status_return == XLookupKeySym) {
      D_X11(("XmbLookupString() returned len == %d, status_return == XLookupKeySym\n", len));
      if ((keysym >= 0x0100) && (keysym < 0x0400)) {
	len = 1;
	kbuf[0] = (keysym & 0xff);
      }
    } else if (status_return == XLookupBoth) {
      D_X11(("XmbLookupString() returned len == %d, status_return == XLookupBoth\n", len));
      if ((keysym >= 0x0100) && (keysym < 0x0400)) {
	len = 1;
	kbuf[0] = (keysym & 0xff);
      }
    } else if (status_return == XLookupChars) {
      D_X11(("XmbLookupString() returned len == %d, status_return == XLookupChars\n", len));
      /* Nothing */
    }
  } else {
    len = XLookupString(&ev->xkey, kbuf, sizeof(kbuf), &keysym, NULL);
  }
#else /* NO_XLOCALE */
  len = XLookupString(&ev->xkey, kbuf, sizeof(kbuf), &keysym, NULL);
  /*
   * have unmapped Latin[2-4] entries -> Latin1
   * good for installations  with correct fonts, but without XLOCAL
   */
  if (!len && (keysym >= 0x0100) && (keysym < 0x0400)) {
    len = 1;
    kbuf[0] = (keysym & 0xff);
  }
#endif /* NO_XLOCALE */

  if (len && (Options & Opt_homeOnInput))
    TermWin.view_start = 0;

  /* for some backwards compatibility */
#if defined (HOTKEY_CTRL) || defined (HOTKEY_META)
# ifdef HOTKEY_CTRL
#  define HOTKEY	ctrl
# else
#  ifdef HOTKEY_META
#   define HOTKEY	meta
#  endif
# endif
  if (HOTKEY) {
    if (keysym == ks_bigfont) {
      change_font(0, FONT_UP);
      return;
    } else if (keysym == ks_smallfont) {
      change_font(0, FONT_DN);
      return;
    }
  }
# undef HOTKEY
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
	    return;
	  }
	  break;

	case XK_Next:		/* Shift+Next = scroll forward */
	  if (TermWin.saveLines) {
	    scr_page(DN, lnsppg);
	    return;
	  }
	  break;

	case XK_Insert:	/* Shift+Insert = paste mouse selection */
	  selection_request(ev->xkey.time, ev->xkey.x, ev->xkey.y);
	  return;
	  break;

	  /* Eterm extras */
	case XK_KP_Add:	/* Shift+KP_Add = bigger font */
	  change_font(0, FONT_UP);
	  return;
	  break;

	case XK_KP_Subtract:	/* Shift+KP_Subtract = smaller font */
	  change_font(0, FONT_DN);
	  return;
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
	  return;
	}
	break;

      case XK_Next:
	if (TermWin.saveLines) {
	  scr_page(DN, TermWin.nrow * 4 / 5);
	  return;
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
      return;
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
      return;
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
      return;
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

  if (len <= 0)
    return;			/* not mapped */

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
}

#if (MENUBAR_MAX)

/* attempt to `write' COUNT to the input buffer */
unsigned int
cmd_write(const unsigned char *str, unsigned int count)
{

  int n;

  n = (count - (cmdbuf_ptr - cmdbuf_base));
  /* need to insert more chars that space available in the front */
  if (n > 0) {
    /* try and get more space from the end */
    unsigned char *src, *dst;

    dst = (cmdbuf_base + sizeof(cmdbuf_base) - 1);	/* max pointer */

    if ((cmdbuf_ptr + n) > dst)
      n = (dst - cmdbuf_ptr);	/* max # chars to insert */

    if ((cmdbuf_endp + n) > dst)
      cmdbuf_endp = (dst - n);	/* truncate end if needed */

    /* equiv: memmove ((cmdbuf_ptr+n), cmdbuf_ptr, n); */
    src = cmdbuf_endp;
    dst = src + n;
    /* FIXME: anything special to avoid possible pointer wrap? */
    while (src >= cmdbuf_ptr)
      *dst-- = *src--;

    /* done */
    cmdbuf_ptr += n;
    cmdbuf_endp += n;
  }
  while (count-- && cmdbuf_ptr > cmdbuf_base) {
    /* sneak one in */
    cmdbuf_ptr--;
    *cmdbuf_ptr = str[count];
  }

  return (0);
}
#endif /* MENUBAR_MAX */

#ifdef BACKGROUND_CYCLING_SUPPORT
# if RETSIGTYPE != void
#  define CPC_RETURN(x) return ((RETSIGTYPE) x)
# else
#  define CPC_RETURN(x) return
# endif

RETSIGTYPE
check_pixmap_change(int sig)
{

  static time_t last_update = 0;
  time_t now;
  static unsigned long image_idx = 0;
  void (*old_handler) (int);
  static unsigned char in_cpc = 0;

  if (in_cpc)
    CPC_RETURN(0);
  in_cpc = 1;
  D_PIXMAP(("check_pixmap_change():  rs_anim_delay == %lu seconds, last_update == %lu\n",
	    rs_anim_delay, last_update));
  if (!rs_anim_delay)
    CPC_RETURN(0);
  if (last_update == 0) {
    last_update = time(NULL);
    old_handler = signal(SIGALRM, check_pixmap_change);
    alarm(rs_anim_delay);
    in_cpc = 0;
    CPC_RETURN(0);
  }
  now = time(NULL);
  D_PIXMAP(("now %lu >= %lu (last_update %lu + rs_anim_delay %lu) ?\n", now, last_update + rs_anim_delay, last_update, rs_anim_delay));
  if (now >= last_update + rs_anim_delay || 1) {
    D_PIXMAP(("Time to update pixmap.  now == %lu\n", now));
    Imlib_destroy_image(imlib_id, imlib_bg.im);
    imlib_bg.im = NULL;
    xterm_seq(XTerm_Pixmap, rs_anim_pixmaps[image_idx++]);
    last_update = now;
    old_handler = signal(SIGALRM, check_pixmap_change);
    alarm(rs_anim_delay);
    if (rs_anim_pixmaps[image_idx] == NULL) {
      image_idx = 0;
    }
  }
  in_cpc = 0;
  if (old_handler) {
    CPC_RETURN((*old_handler) (sig));
  } else {
    CPC_RETURN(sig);
  }
}
#endif /* BACKGROUND_CYCLING_SUPPORT */

/* cmd_getc() - Return next input character */
/*
 * Return the next input character after first passing any keyboard input
 * to the command.
 */
#define CHARS_READ() (cmdbuf_ptr < cmdbuf_endp)
#define CHARS_BUFFERED() (count != CMD_BUF_SIZE)
#define RETURN_CHAR() do { refreshed = 0; return (*cmdbuf_ptr++); } while (0)

#ifdef REFRESH_DELAY
# define REFRESH_DELAY_USEC 1000000/25
#endif

unsigned char
cmd_getc(void)
{

#define TIMEOUT_USEC 2500
  static short refreshed = 0;
  fd_set readfds;
  int retval;
  struct timeval value, *delay;

  /* If there has been a lot of new lines, then update the screen
   * What the heck I'll cheat and only refresh less than every page-full.
   * the number of pages between refreshes is refresh_limit, which
   * is incremented here because we must be doing flat-out scrolling.
   *
   * refreshing should be correct for small scrolls, because of the
   * time-out
   */
  if (refresh_count >= (refresh_limit * (TermWin.nrow - 1))) {
    if (refresh_limit < REFRESH_PERIOD)
      refresh_limit++;
    refresh_count = 0;
    refreshed = 1;
    D_CMD(("cmd_getc(): scr_refresh() #1\n"));
#ifdef PROFILE
    P_CALL(scr_refresh(refresh_type), "cmd_getc()->scr_refresh()");
#else
    scr_refresh(refresh_type);
#endif
  }
  /* characters already read in */
  if (CHARS_READ()) {
    RETURN_CHAR();
  }
  for (;;) {

    v_doPending();
    while (XPending(Xdisplay)) {	/* process pending X events */

      XEvent ev;

      refreshed = 0;
      XNextEvent(Xdisplay, &ev);

#ifndef NO_XLOCALE
      if (!XFilterEvent(&ev, ev.xkey.window)) {
	D_X11(("cmd_getc(): process_x_event();\n"));
	process_x_event(&ev);
      }
#else
      D_X11(("cmd_getc(): process_x_event();\n"));
      process_x_event(&ev);
#endif

      /* in case button actions pushed chars to cmdbuf */
      if (CHARS_READ()) {
	RETURN_CHAR();
      }
    }

#ifdef SCROLLBAR_BUTTON_CONTINUAL_SCROLLING
    if (scrollbar_isUp()) {
      if (!scroll_arrow_delay-- && scr_page(UP, 1)) {
	scroll_arrow_delay = SCROLLBAR_CONTINUOUS_DELAY;
	refreshed = 0;
/*              refresh_type |= SMOOTH_REFRESH; */
      }
    } else if (scrollbar_isDn()) {
      if (!scroll_arrow_delay-- && scr_page(DN, 1)) {
	scroll_arrow_delay = SCROLLBAR_CONTINUOUS_DELAY;
	refreshed = 0;
/*              refresh_type |= SMOOTH_REFRESH; */
      }
    }
#endif /* SCROLLBAR_BUTTON_CONTINUAL_SCROLLING */

    /* Nothing to do! */
    FD_ZERO(&readfds);
    FD_SET(cmd_fd, &readfds);
    FD_SET(Xfd, &readfds);
    value.tv_usec = TIMEOUT_USEC;
    value.tv_sec = 0;

    if (refreshed
#ifdef SCROLLBAR_BUTTON_CONTINUAL_SCROLLING
	&& !(scrollbar_isUpDn())
#endif
	) {
      delay = NULL;
    } else {
      delay = &value;
    }
    retval = select(num_fds, &readfds, NULL, NULL, delay);

    /* See if we can read from the application */
    if (FD_ISSET(cmd_fd, &readfds)) {

      /*      unsigned int count = BUFSIZ; */
      register unsigned int count = CMD_BUF_SIZE;

      cmdbuf_ptr = cmdbuf_endp = cmdbuf_base;

      /* while (count > sizeof(cmdbuf_base) / 2) */
      while (count) {

	/*      int n = read(cmd_fd, cmdbuf_endp, count); */
	register int n = read(cmd_fd, cmdbuf_endp, count);

	if (n <= 0)
	  break;
	cmdbuf_endp += n;
	count -= n;
      }
      /* some characters read in */
      if (CHARS_BUFFERED()) {
	RETURN_CHAR();
      }
    }
    /* select statement timed out - better update the screen */

    if (retval == 0) {
      refresh_count = 0;
      refresh_limit = 1;
      if (!refreshed) {
	refreshed = 1;
	D_CMD(("cmd_getc(): scr_refresh() #2\n"));
	scr_refresh(refresh_type);
	if (scrollbar_visible())
	  scrollbar_show(1);
      }
    }
  }

  D_CMD(("cmd_getc() returning\n"));
  return (0);
}

#if MENUBAR_MAX
#  define XEVENT_IS_MYWIN(ev)       (((ev)->xany.window == TermWin.parent) \
				      || ((ev)->xany.window == TermWin.vt) \
				      || ((ev)->xany.window == menuBar.win) \
				      || ((ev)->xany.window == scrollBar.win))
#else
#  define XEVENT_IS_MYWIN(ev)       (((ev)->xany.window == TermWin.parent) \
				      || ((ev)->xany.window == TermWin.vt) \
				      || ((ev)->xany.window == scrollBar.win))
#endif
#define XEVENT_IS_PARENT(ev)        (((ev)->xany.window == TermWin.wm_parent) \
                                      || ((ev)->xany.window == TermWin.wm_grandparent))
#define XEVENT_REQUIRE(bool_test)   do { if (!(bool_test)) return; } while (0)

void
process_x_event(XEvent * ev)
{
  static Time buttonpress_time, lastbutton_press;
  static int clicks = 0;

#define clickOnce() (clicks <= 1)
  static int bypass_keystate = 0;
  int reportmode;
  static int mouseoffset = 0;	/* Mouse pointer offset info scrollbar anchor */

#ifdef COUNT_X_EVENTS
  static long long event_cnt = 0;
  static long long keypress_cnt = 0;
  static long long motion_cnt = 0;
  static long long expose_cnt = 0;

#endif
#ifdef PIXMAP_OFFSET
  Atom type;
  int format;
  unsigned long length, after;
  unsigned char *data;

#endif
#ifdef USE_ACTIVE_TAGS
  static Time activate_time;

#endif
#ifdef PROFILE_X_EVENTS
  struct timeval expose_start, expose_stop, motion_start, motion_stop, keypress_start, keypress_stop;
  static long expose_total = 0;

#endif
#ifdef WATCH_DESKTOP_OPTION
  Window new_desktop_window, last_desktop_window = desktop_window;

#endif

#ifdef COUNT_X_EVENTS
  event_cnt++;
  D_EVENTS(("total number of events: %ld\n", event_cnt));
#endif

  D_EVENTS(("process_x_event(0x%02x):  %s, for window 0x%08x\n",
	    ev->type, event_type_to_name(ev->type), ev->xany.window));

  switch (ev->type) {
    case KeyPress:
      if (keypress_exit)
	exit(EXIT_SUCCESS);
      D_EVENTS(("process_x_event(%s)\n", "KeyPress"));
#ifdef COUNT_X_EVENTS
      keypress_cnt++;
      D_EVENTS(("total number of KeyPress events: %ld\n", keypress_cnt));
#endif
#ifdef PROFILE_X_EVENTS
      P_SETTIMEVAL(keypress_start);
#endif
      XEVENT_REQUIRE(XEVENT_IS_MYWIN(ev));
      lookup_key(ev);
#ifdef PROFILE_X_EVENTS
      P_SETTIMEVAL(keypress_stop);
      fprintf(stderr, "KeyPress: %ld microseconds\n",
	      P_CMPTIMEVALS_USEC(keypress_start, keypress_stop));
#endif
      break;

#ifdef WATCH_DESKTOP_OPTION
    case PropertyNotify:
      D_EVENTS(("process_x_event(%s)\n", "PropertyNotify"));
      if (Options & Opt_pixmapTrans && Options & Opt_watchDesktop) {
	if (desktop_window != None) {
	  XSelectInput(Xdisplay, desktop_window, 0);
	} else {
	  XSelectInput(Xdisplay, Xroot, 0);
	}
	XGetWindowProperty(Xdisplay, Xroot, ev->xproperty.atom, 0L, 1L, False,
			   AnyPropertyType, &type, &format, &length, &after, &data);
	if (type == XA_PIXMAP) {
	  if (desktop_pixmap != None) {
	    XFreePixmap(Xdisplay, desktop_pixmap);
	    desktop_pixmap = None;	/* Force the re-read */
	  }
	  render_pixmap(TermWin.vt, imlib_bg, bgPixmap, 0, 1);
	  scr_expose(0, 0, TermWin_TotalWidth(), TermWin_TotalHeight());
	}
	if (desktop_window != None) {
	  XSelectInput(Xdisplay, desktop_window, PropertyChangeMask);
	} else {
	  XSelectInput(Xdisplay, Xroot, PropertyChangeMask);
	}
      }
      break;

    case ReparentNotify:
      D_EVENTS(("ReparentNotify:  window == 0x%08x, parent == 0x%08x, TermWin.parent == 0x%08x, TermWin.wm_parent == 0x%08x\n",
		ev->xreparent.window, ev->xreparent.parent, TermWin.parent, TermWin.wm_parent));
      if (Options & Opt_watchDesktop) {
	if (ev->xreparent.window == TermWin.parent) {
	  D_EVENTS(("It's TermWin.parent.  Assigning TermWin.wm_parent to 0x%08x\n", ev->xreparent.parent));
	  if (TermWin.wm_parent != None) {
	    XSelectInput(Xdisplay, TermWin.wm_parent, None);
	  }
	  TermWin.wm_parent = ev->xreparent.parent;
	  XSelectInput(Xdisplay, TermWin.wm_parent, (StructureNotifyMask | SubstructureNotifyMask));
	} else if (ev->xreparent.window == TermWin.wm_parent || ev->xreparent.window == TermWin.wm_grandparent) {
	  D_EVENTS(("It's my parent!  last_desktop_window == 0x%08x, desktop_window == 0x%08x\n",
		    last_desktop_window, get_desktop_window()));
	  if (Options & Opt_pixmapTrans && Options & Opt_watchDesktop && get_desktop_window() != last_desktop_window) {
	    D_EVENTS(("Desktop changed!\n"));
	    if (desktop_pixmap != None) {
	      XFreePixmap(Xdisplay, desktop_pixmap);
	      desktop_pixmap = None;	/* Force the re-read */
	    }
	    render_pixmap(TermWin.vt, imlib_bg, bgPixmap, 0, 1);
	    scr_expose(0, 0, TermWin_TotalWidth(), TermWin_TotalHeight());
	  }
	}
      }
      break;
#endif

    case ClientMessage:
      XEVENT_REQUIRE(XEVENT_IS_MYWIN(ev));
      D_EVENTS(("process_x_event(%s)\n", "ClientMessage"));
      if (ev->xclient.format == 32 && ev->xclient.data.l[0] == wmDeleteWindow)
	exit(EXIT_SUCCESS);
#ifdef OFFIX_DND
      /* OffiX Dnd (drag 'n' drop) protocol */
      if (ev->xclient.message_type == DndProtocol &&
	  ((ev->xclient.data.l[0] == DndFile) ||
	   (ev->xclient.data.l[0] == DndDir) ||
	   (ev->xclient.data.l[0] == DndLink))) {
	/* Get Dnd data */
	Atom ActualType;
	int ActualFormat;
	unsigned char *data;
	unsigned long Size, RemainingBytes;

	XGetWindowProperty(Xdisplay, Xroot,
			   DndSelection,
			   0L, 1000000L,
			   False, AnyPropertyType,
			   &ActualType, &ActualFormat,
			   &Size, &RemainingBytes,
			   &data);
	XChangeProperty(Xdisplay, Xroot,
			XA_CUT_BUFFER0, XA_STRING,
			8, PropModeReplace,
			data, strlen(data));
	selection_paste(Xroot, XA_CUT_BUFFER0, True);
	XSetInputFocus(Xdisplay, Xroot, RevertToNone, CurrentTime);
      }
#endif /* OFFIX_DND */
      break;

    case MappingNotify:
      D_EVENTS(("process_x_event(%s)\n", "MappingNotify"));
      XRefreshKeyboardMapping(&(ev->xmapping));
      break;

#ifdef USE_ACTIVE_TAGS
    case LeaveNotify:
      XEVENT_REQUIRE(XEVENT_IS_MYWIN(ev));
      tag_hide();
      break;
#endif

      /* Here's my conclusion:
       * If the window is completely unobscured, use bitblt's
       * to scroll. Even then, they're only used when doing partial
       * screen scrolling. When partially obscured, we have to fill
       * in the GraphicsExpose parts, which means that after each refresh,
       * we need to wait for the graphics expose or Noexpose events,
       * which ought to make things real slow!
       */
    case VisibilityNotify:
      D_EVENTS(("process_x_event(%s)\n", "VisibilityNotify"));
      XEVENT_REQUIRE(XEVENT_IS_MYWIN(ev));
      switch (ev->xvisibility.state) {
	case VisibilityUnobscured:
#ifdef USE_SMOOTH_REFRESH
	  refresh_type = SMOOTH_REFRESH;
#else
	  refresh_type = FAST_REFRESH;
#endif
	  break;

	case VisibilityPartiallyObscured:
	  refresh_type = SLOW_REFRESH;
	  break;

	default:
	  refresh_type = NO_REFRESH;
	  break;
      }
      break;

    case FocusIn:
      D_EVENTS(("process_x_event(%s)\n", "FocusIn"));
      XEVENT_REQUIRE(XEVENT_IS_MYWIN(ev));
      if (!TermWin.focus) {
	TermWin.focus = 1;
#ifdef CHANGE_SCROLLCOLOR_ON_FOCUS
	menubar_expose();
#endif
	if (Options & Opt_scrollbar_popup) {
	  map_scrollBar(Options & Opt_scrollBar);
	}
#ifndef NO_XLOCALE
	if (Input_Context != NULL)
	  XSetICFocus(Input_Context);
#endif
      }
      break;

    case FocusOut:
      D_EVENTS(("process_x_event(%s)\n", "FocusOut"));
      XEVENT_REQUIRE(XEVENT_IS_MYWIN(ev));
      if (TermWin.focus) {
	TermWin.focus = 0;
#ifdef CHANGE_SCROLLCOLOR_ON_FOCUS
	menubar_expose();
#endif
	if (Options & Opt_scrollbar_popup) {
	  map_scrollBar(0);
	}
#ifndef NO_XLOCALE
	if (Input_Context != NULL)
	  XUnsetICFocus(Input_Context);
#endif
      }
      break;

    case ConfigureNotify:
      D_EVENTS(("process_x_event(%s)\n", "ConfigureNotify"));
      XEVENT_REQUIRE(XEVENT_IS_MYWIN(ev));
#ifdef PIXMAP_OFFSET
      if (Options & Opt_pixmapTrans || Options & Opt_viewport_mode) {
	render_pixmap(TermWin.vt, imlib_bg, bgPixmap, 0, 2);
	scr_expose(0, 0, TermWin_TotalWidth(), TermWin_TotalHeight());
      }
#endif
      resize_window();
      menubar_expose();
      break;

    case SelectionClear:
      D_EVENTS(("process_x_event(%s)\n", "SelectionClear"));
      selection_clear();
      break;

    case SelectionNotify:
      D_EVENTS(("process_x_event(%s)\n", "SelectionNotify"));
      selection_paste(ev->xselection.requestor, ev->xselection.property, True);
      break;

    case SelectionRequest:
      D_EVENTS(("process_x_event(%s)\n", "SelectionRequest"));
      selection_send(&(ev->xselectionrequest));
      break;

    case GraphicsExpose:
      D_EVENTS(("process_x_event(%s)\n", "GraphicsExpose"));
    case Expose:
      D_EVENTS(("process_x_event(%s)\n", "Expose"));
#ifdef PROFILE_X_EVENTS
      P_SETTIMEVAL(expose_start);
#endif
      XEVENT_REQUIRE(XEVENT_IS_MYWIN(ev));
      if (ev->xany.window == TermWin.vt) {
	scr_expose(ev->xexpose.x, ev->xexpose.y, ev->xexpose.width, ev->xexpose.height);
      } else {

	XEvent unused_xevent;

	while (XCheckTypedWindowEvent(Xdisplay, ev->xany.window, Expose, &unused_xevent));
	while (XCheckTypedWindowEvent(Xdisplay, ev->xany.window, GraphicsExpose, &unused_xevent));
	if (isScrollbarWindow(ev->xany.window)) {
	  scrollbar_setNone();
	  scrollbar_show(0);
	}
#if (MENUBAR_MAX)
	if (menubar_visible() && isMenuBarWindow(ev->xany.window)) {
	  menubar_expose();
	}
#endif /* MENUBAR_MAX */
	Gr_expose(ev->xany.window);
      }
#ifdef WATCH_DESKTOP_OPTION
      if (Options & Opt_pixmapTrans && Options & Opt_watchDesktop) {
	if (desktop_window != None) {
	  XSelectInput(Xdisplay, desktop_window, PropertyChangeMask);
	} else {
	  XSelectInput(Xdisplay, Xroot, PropertyChangeMask);
	}
      }
#endif
#ifdef PROFILE_X_EVENTS
      P_SETTIMEVAL(expose_stop);
      expose_total += P_CMPTIMEVALS_USEC(expose_start, expose_stop);
      fprintf(stderr, "Expose: %ld(%ld) microseconds\n",
	      P_CMPTIMEVALS_USEC(expose_start, expose_stop),
	      expose_total);
#endif
      break;

    case ButtonPress:
      D_EVENTS(("process_x_event(%s)\n", "ButtonPress"));
      XEVENT_REQUIRE(XEVENT_IS_MYWIN(ev));
      if (Options & Opt_borderless) {
	XSetInputFocus(Xdisplay, Xroot, RevertToNone, CurrentTime);
      }
#if defined(CTRL_CLICK_RAISE) || defined(CTRL_CLICK_SCROLLBAR) || defined(CTRL_CLICK_MENU)
      if ((ev->xbutton.state & ControlMask) && (ev->xany.window == TermWin.vt)) {
	D_EVENTS(("Checking for Ctrl+Button\n"));
	switch (ev->xbutton.button) {
	  case Button1:
	    D_EVENTS(("Ctrl+Button1\n"));
# ifdef CTRL_CLICK_RAISE
	    XSetInputFocus(Xdisplay, TermWin.parent, RevertToParent, CurrentTime);
	    XRaiseWindow(Xdisplay, TermWin.parent);
	    /*XWarpPointer(Xdisplay, None, TermWin.vt, 0, 0, 0, 0, TermWin.width/2, TermWin.height/2); */
# endif
	    break;
	  case Button2:
	    D_EVENTS(("Ctrl+Button2\n"));
# ifdef CTRL_CLICK_SCROLLBAR
	    map_scrollBar(scrollbar_visible()? 0 : 1);
# endif
	    break;
	  case Button3:
	    D_EVENTS(("Ctrl+Button3\n"));
# if defined(CTRL_CLICK_MENU) && (MENUBAR_MAX)
	    map_menuBar(menubar_visible()? 0 : 1);
# endif
	    break;
	  default:
	    break;
	}
	break;
      }
#endif /* CTRL_CLICK_RAISE || CTRL_CLICK_MENU */

      bypass_keystate = (ev->xbutton.state & (Mod1Mask | ShiftMask));
      reportmode = (bypass_keystate ? 0 : (PrivateModes & PrivMode_mouse_report));

      if (ev->xany.window == TermWin.vt) {
	if (ev->xbutton.subwindow != None) {
	  Gr_ButtonPress(ev->xbutton.x, ev->xbutton.y);
	} else {
	  if (reportmode) {
	    if (reportmode & PrivMode_MouseX10) {
	      /* no state info allowed */
	      ev->xbutton.state = 0;
	    }
#ifdef MOUSE_REPORT_DOUBLECLICK
	    if (ev->xbutton.button == Button1) {
	      if (ev->xbutton.time - buttonpress_time < MULTICLICK_TIME)
		clicks++;
	      else
		clicks = 1;
	    }
#else
	    clicks = 1;
#endif /* MOUSE_REPORT_DOUBLECLICK */
	    mouse_report(&(ev->xbutton));
	  } else {
#ifdef USE_ACTIVE_TAGS
	    if (tag_click(ev->xbutton.x, ev->xbutton.y, ev->xbutton.button, ev->xkey.state))
	      activate_time = ev->xbutton.time;
	    else
#endif
	      switch (ev->xbutton.button) {
		case Button1:
		  if (lastbutton_press == 1
		      && (ev->xbutton.time - buttonpress_time < MULTICLICK_TIME))
		    clicks++;
		  else
		    clicks = 1;
		  selection_click(clicks, ev->xbutton.x, ev->xbutton.y);
		  lastbutton_press = 1;
		  break;

		case Button3:
		  if (lastbutton_press == 3
		      && (ev->xbutton.time - buttonpress_time < MULTICLICK_TIME))
		    selection_rotate(ev->xbutton.x, ev->xbutton.y);
		  else
		    selection_extend(ev->xbutton.x, ev->xbutton.y, 1);
		  lastbutton_press = 3;
		  break;
	      }
	  }
	  buttonpress_time = ev->xbutton.time;
	  return;
	}
      }
#ifdef PIXMAP_SCROLLBAR
      if ((isScrollbarWindow(ev->xany.window))
	  || (scrollbar_upButtonWin(ev->xany.window))
	  || (scrollbar_dnButtonWin(ev->xany.window)))
#else
      if (isScrollbarWindow(ev->xany.window))
#endif
      {
	scrollbar_setNone();
	/*
	 * Eterm-style scrollbar:
	 * move up if mouse is above slider
	 * move dn if mouse is below slider
	 *
	 * XTerm-style scrollbar:
	 * Move display proportional to pointer location
	 * pointer near top -> scroll one line
	 * pointer near bot -> scroll full page
	 */
#ifndef NO_SCROLLBAR_REPORT
	if (reportmode) {
	  /*
	   * Mouse report disabled scrollbar:
	   * arrow buttons - send up/down
	   * click on scrollbar - send pageup/down
	   */
#ifdef PIXMAP_SCROLLBAR
	  if ((scrollbar_upButtonWin(ev->xany.window))
	      || (!(scrollbar_is_pixmapped()) &&
		  (scrollbar_upButton(ev->xbutton.y))))
#else
	  if (scrollbar_upButton(ev->xbutton.y))
#endif
	    tt_printf("\033[A");
#ifdef PIXMAP_SCROLLBAR
	  else if ((scrollbar_dnButtonWin(ev->xany.window))
		   || (!(scrollbar_is_pixmapped()) &&
		       (scrollbar_upButton(ev->xbutton.y))))
#else
	  else if (scrollbar_dnButton(ev->xbutton.y))
#endif
	    tt_printf("\033[B");
	  else
	    switch (ev->xbutton.button) {
	      case Button2:
		tt_printf("\014");
		break;
	      case Button1:
		tt_printf("\033[6~");
		break;
	      case Button3:
		tt_printf("\033[5~");
		break;
	    }
	} else
#endif /* NO_SCROLLBAR_REPORT */
	{
#ifdef PIXMAP_SCROLLBAR
	  if ((scrollbar_upButtonWin(ev->xany.window))
	      || (!(scrollbar_is_pixmapped()) &&
		  (scrollbar_upButton(ev->xbutton.y))))
#else
	  if (scrollbar_upButton(ev->xbutton.y))
#endif
	  {
#ifdef SCROLLBAR_BUTTON_CONTINUAL_SCROLLING
	    scroll_arrow_delay = SCROLLBAR_INITIAL_DELAY;
#endif
	    if (scr_page(UP, 1)) {
	      scrollbar_setUp();
	    }
	  } else if
#ifdef PIXMAP_SCROLLBAR
		((scrollbar_dnButtonWin(ev->xany.window))
		 || (!(scrollbar_is_pixmapped())
		     && (scrollbar_dnButton(ev->xbutton.y))))
#else
		(scrollbar_dnButton(ev->xbutton.y))
#endif
	  {
#ifdef SCROLLBAR_BUTTON_CONTINUAL_SCROLLING
	    scroll_arrow_delay = SCROLLBAR_INITIAL_DELAY;
#endif
	    if (scr_page(DN, 1)) {
	      scrollbar_setDn();
	    }
	  }
	  else
	  switch (ev->xbutton.button) {
	    case Button2:
	      mouseoffset = (scrollBar.bot - scrollBar.top) / 2;	/* Align to center */
	      if (scrollbar_above_slider(ev->xbutton.y) || scrollbar_below_slider(ev->xbutton.y)
		  || scrollBar.type == SCROLLBAR_XTERM) {
		scr_move_to(scrollbar_position(ev->xbutton.y) - mouseoffset,
			    scrollbar_size());
	      }
	      scrollbar_setMotion();
	      break;

	    case Button1:
	      mouseoffset = ev->xbutton.y - scrollBar.top;
	      MAX_IT(mouseoffset, 1);
	      /* drop */
	    case Button3:
#if defined(MOTIF_SCROLLBAR) || defined(NEXT_SCROLLBAR)
	      if (scrollBar.type == SCROLLBAR_MOTIF || scrollBar.type == SCROLLBAR_NEXT) {
# ifdef PIXMAP_SCROLLBAR
		if (!(ev->xany.window == scrollBar.sa_win)
		    && (scrollbar_above_slider(ev->xbutton.y)))
# else
		  if (scrollbar_above_slider(ev->xbutton.y))
# endif
		    scr_page(UP, TermWin.nrow - 1);
# ifdef PIXMAP_SCROLLBAR
		  else if (!(ev->xany.window == scrollBar.sa_win)
			   && (scrollbar_below_slider(ev->xbutton.y)))
# else
		    else
		    if (scrollbar_below_slider(ev->xbutton.y))
# endif
		      scr_page(DN, TermWin.nrow - 1);
		    else
		      scrollbar_setMotion();
	      }
#endif /* MOTIF_SCROLLBAR || NEXT_SCROLLBAR */

#ifdef XTERM_SCROLLBAR
	      if (scrollBar.type == SCROLLBAR_XTERM) {
		scr_page((ev->xbutton.button == Button1 ? DN : UP),
			 (TermWin.nrow *
			  scrollbar_position(ev->xbutton.y) /
			  scrollbar_size())
		    );
	      }
#endif /* XTERM_SCROLLBAR */
	      break;
	  }
	}
	return;
      }
#if (MENUBAR_MAX)
      if (isMenuBarWindow(ev->xany.window)) {
	menubar_control(&(ev->xbutton));
	return;
      }
#endif /* MENUBAR_MAX */
      break;

    case ButtonRelease:
      D_EVENTS(("process_x_event(%s)\n", "ButtonRelease"));
      XEVENT_REQUIRE(XEVENT_IS_MYWIN(ev));
      mouseoffset = 0;
      reportmode = (bypass_keystate ?
		    0 : (PrivateModes & PrivMode_mouse_report));

      if (scrollbar_isUpDn()) {
	scrollbar_setNone();
	scrollbar_show(0);
#ifdef SCROLLBAR_BUTTON_CONTINUAL_SCROLLING
/*          refresh_type &= ~SMOOTH_REFRESH; */
#endif
      }
      if (ev->xany.window == TermWin.vt) {
	if (ev->xbutton.subwindow != None)
	  Gr_ButtonRelease(ev->xbutton.x, ev->xbutton.y);
	else {
	  if (reportmode) {
	    switch (reportmode & PrivMode_mouse_report) {
	      case PrivMode_MouseX10:
		break;

	      case PrivMode_MouseX11:
		ev->xbutton.state = bypass_keystate;
		ev->xbutton.button = AnyButton;
		mouse_report(&(ev->xbutton));
		break;
	    }
	    return;
	  }
	  /*
	   * dumb hack to compensate for the failure of click-and-drag
	   * when overriding mouse reporting
	   */
	  if ((PrivateModes & PrivMode_mouse_report) &&
	      (bypass_keystate) &&
	      (ev->xbutton.button == Button1) &&
	      (clickOnce()))
	    selection_extend(ev->xbutton.x, ev->xbutton.y, 0);

	  switch (ev->xbutton.button) {
	    case Button1:
	    case Button3:
#if defined(CTRL_CLICK_RAISE) || defined(CTRL_CLICK_MENU)
	      if (!(ev->xbutton.state & ControlMask))
#endif
		selection_make(ev->xbutton.time);
	      break;

	    case Button2:
#ifdef CTRL_CLICK_SCROLLBAR
	      if (!(ev->xbutton.state & ControlMask))
#endif
		selection_request(ev->xbutton.time, ev->xbutton.x, ev->xbutton.y);
	      break;
	    case Button4:
	      scr_page(UP, (ev->xbutton.state & ShiftMask) ? 1 : 5);
	      break;
	    case Button5:
	      scr_page(DN, (ev->xbutton.state & ShiftMask) ? 1 : 5);
	      break;
	  }
	}
      }
#if (MENUBAR_MAX)
      else if (isMenuBarWindow(ev->xany.window)) {
	menubar_control(&(ev->xbutton));
      }
#endif /* MENUBAR_MAX */
      break;

    case MotionNotify:
      D_EVENTS(("process_x_event(%s)\n", "MotionNotify"));
#ifdef COUNT_X_EVENTS
      motion_cnt++;
      D_EVENTS(("total number of MotionNotify events: %ld\n", motion_cnt));
#endif
#ifdef PROFILE_X_EVENTS
      P_SETTIMEVAL(motion_start);
#endif
      XEVENT_REQUIRE(XEVENT_IS_MYWIN(ev));
#if (MENUBAR_MAX)
      if (isMenuBarWindow(ev->xany.window)) {
	menubar_control(&(ev->xbutton));
	break;
      }
#endif /* MENUBAR_MAX */
      if ((PrivateModes & PrivMode_mouse_report) && !(bypass_keystate))
	break;

      if (ev->xany.window == TermWin.vt) {
	if ((ev->xbutton.state & (Button1Mask | Button3Mask))
#ifdef USE_ACTIVE_TAGS
	    && ((ev->xmotion.time - activate_time) > TAG_DRAG_THRESHHOLD)
#endif
	    ) {
	  Window unused_root, unused_child;
	  int unused_root_x, unused_root_y;
	  unsigned int unused_mask;

	  while (XCheckTypedWindowEvent(Xdisplay, TermWin.vt,
					MotionNotify, ev));
	  XQueryPointer(Xdisplay, TermWin.vt,
			&unused_root, &unused_child,
			&unused_root_x, &unused_root_y,
			&(ev->xbutton.x), &(ev->xbutton.y),
			&unused_mask);
#ifdef MOUSE_THRESHOLD
	  /* deal with a `jumpy' mouse */
	  if ((ev->xmotion.time - buttonpress_time) > MOUSE_THRESHOLD)
#endif
	    selection_extend((ev->xbutton.x), (ev->xbutton.y),
			     (ev->xbutton.state & Button3Mask));
	}
#ifdef USE_ACTIVE_TAGS
	else
	  tag_pointer_new_position(ev->xbutton.x, ev->xbutton.y);
#endif
#ifdef PIXMAP_SCROLLBAR
      } else if ((scrollbar_is_pixmapped()) && (ev->xany.window == scrollBar.sa_win) && scrollbar_isMotion()) {

	Window unused_root, unused_child;
	int unused_root_x, unused_root_y;
	unsigned int unused_mask;

	/* FIXME: I guess pointer or server should be grabbed here
	 * or something like that :) -vendu
	 */

	while (XCheckTypedWindowEvent(Xdisplay, scrollBar.sa_win,
				      MotionNotify, ev));

	XQueryPointer(Xdisplay, scrollBar.sa_win,
		      &unused_root, &unused_child,
		      &unused_root_x, &unused_root_y,
		      &(ev->xbutton.x), &(ev->xbutton.y),
		      &unused_mask);

	scr_move_to(scrollbar_position(ev->xbutton.y) - mouseoffset,
		    scrollbar_size());
	refresh_count = refresh_limit = 0;
	scr_refresh(refresh_type);
	scrollbar_show(mouseoffset);
#endif
      } else if ((ev->xany.window == scrollBar.win) && scrollbar_isMotion()) {
	Window unused_root, unused_child;
	int unused_root_x, unused_root_y;
	unsigned int unused_mask;

	while (XCheckTypedWindowEvent(Xdisplay, scrollBar.win, MotionNotify, ev));
	XQueryPointer(Xdisplay, scrollBar.win,
		      &unused_root, &unused_child,
		      &unused_root_x, &unused_root_y,
		      &(ev->xbutton.x), &(ev->xbutton.y),
		      &unused_mask);
	scr_move_to(scrollbar_position(ev->xbutton.y) - mouseoffset,
		    scrollbar_size());
	refresh_count = refresh_limit = 0;
	scr_refresh(refresh_type);
	scrollbar_show(mouseoffset);
      }
#ifdef PROFILE_X_EVENTS
      P_SETTIMEVAL(motion_stop);
      fprintf(stderr, "MotionNotify: %ld microseconds\n",
	      P_CMPTIMEVALS_USEC(motion_start, motion_stop));
#endif
      break;
  }
}

/* tt_write(), tt_printf() - output to command */
/*
 * Send count characters directly to the command
 */
void
tt_write(const unsigned char *buf, unsigned int count)
{

  v_writeBig(cmd_fd, (char *) buf, count);

#if 0				/* Fixes the bug that hung Eterm when pasting a lot of stuff */
  while (count > 0) {
    int n = write(cmd_fd, buf, count);

    if (n > 0) {
      count -= n;
      buf += n;
    }
  }
#endif
}

/*
 * Send printf() formatted output to the command.
 * Only use for small ammounts of data.
 */
void
tt_printf(const unsigned char *fmt,...)
{
  static unsigned char buf[256];
  va_list arg_ptr;

  va_start(arg_ptr, fmt);
  vsprintf(buf, fmt, arg_ptr);
  va_end(arg_ptr);
  tt_write(buf, strlen(buf));
}


/* print pipe */
/*----------------------------------------------------------------------*/
#ifdef PRINTPIPE
/* PROTO */
FILE *
popen_printer(void)
{
  FILE *stream = popen(rs_print_pipe, "w");

  if (stream == NULL)
    print_error("can't open printer pipe \"%s\" -- %s", rs_print_pipe, strerror(errno));
  return stream;
}

/* PROTO */
int
pclose_printer(FILE * stream)
{
  fflush(stream);
  /* pclose() reported not to work on SunOS 4.1.3 */
# if defined (__sun__)
  /* pclose works provided SIGCHLD handler uses waitpid */
  return pclose(stream);	/* return fclose (stream); */
# else
  return pclose(stream);
# endif
}

/*
 * simulate attached vt100 printer
 */
/* PROTO */
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
/* PROTO */
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
#ifdef KANJI
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
/* PROTO */
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

	    snprintf(tbuff, sizeof(tbuff), APL_NAME "-" VERSION ":  Transparent - %d%% shading - 0x%06x tint mask",
		     rs_shadePct, rs_tintMask);
	    xterm_seq(XTerm_title, tbuff);
	  } else
#endif
#ifdef PIXMAP_SUPPORT
	  {
	    char *tbuff;
	    unsigned short len;

	    if (imlib_bg.im) {
	      len = strlen(imlib_bg.im->filename) + sizeof(APL_NAME) + sizeof(VERSION) + 5;
	      tbuff = MALLOC(len);
	      snprintf(tbuff, len, APL_NAME "-" VERSION ":  %s", imlib_bg.im->filename);
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
    case 'h':
    case 'l':
      process_terminal_mode(ch, priv, nargs, arg);
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
/* PROTO */
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
    int n = 0;

    while ((ch = cmd_getc()) != 007) {
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

    /*
     * menubar_dispatch() violates the constness of the string,
     * so do it here
     */
    if (arg == XTerm_Menu)
      menubar_dispatch(string);
    else
      xterm_seq(arg, string);
  } else {
    int n = 0;

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
	if (x > scr->width || y > scr->height)
	  return;		/* Don't move off-screen */
	XMoveWindow(Xdisplay, TermWin.parent, x, y);
	break;
      case 4:
	if (i + 2 >= nargs)
	  return;		/* Make sure there are 2 args left */
	y = args[++i];
	x = args[++i];
	XResizeWindow(Xdisplay, TermWin.parent, x, y);
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
	scr_refresh(SMOOTH_REFRESH);
	break;
      case 8:
	if (i + 2 >= nargs)
	  return;		/* Make sure there are 2 args left */
	y = args[++i];
	x = args[++i];
	XResizeWindow(Xdisplay, TermWin.parent,
		      Width2Pixel(x) + 2 * TermWin.internalBorder + (scrollbar_visible()? scrollbar_total_width() : 0),
		      Height2Pixel(y) + 2 * TermWin.internalBorder + (menubar_visible()? menuBar_TotalHeight() : 0));
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
 * so no need for fancy checking
 */
/* PROTO */
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

#define PrivCases(bit)	\
if (mode == 't') state = !(PrivateModes & bit); else state = mode;\
switch (state) {\
case 's': SavedModes |= (PrivateModes & bit); continue; break;\
case 'r': state = (SavedModes & bit) ? 1 : 0;/*drop*/\
default:  PrivMode (state, bit); }

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

#if (MENUBAR_MAX)
# ifdef menuBar_esc
	  case menuBar_esc:
	    PrivCases(PrivMode_menuBar);
	    map_menuBar(state);
	    break;
# endif
#endif /* MENUBAR_MAX */

#ifdef scrollBar_esc
	  case scrollBar_esc:
	    PrivCases(PrivMode_scrollBar);
	    map_scrollBar(state);
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
#undef PrivCases
      break;
  }
}

/* process sgr sequences */
/* PROTO */
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
/* PROTO */
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

#ifndef USE_POSIX_THREADS
/* Read and process output from the application */

void
main_loop(void)
{
  /*   int ch; */
  register int ch;

  D_CMD(("[%d] main_loop() called\n", getpid()));

#ifdef BACKGROUND_CYCLING_SUPPORT
  if (rs_anim_delay) {
    check_pixmap_change(0);
  }
#endif
  do {
    while ((ch = cmd_getc()) == 0);	/* wait for something */
    if (ch >= ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
      /* Read a text string from the input buffer */
      int nlines = 0;

      /*           unsigned char * str; */
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
      D_SCREEN(("Adding lines, str == 0x%08x, cmdbuf_ptr == 0x%08x, cmdbuf_endp == 0x%08x\n", str, cmdbuf_ptr,
		cmdbuf_endp));
      D_SCREEN(("Command buffer base == 0x%08x, length %lu, end at 0x%08x\n", cmdbuf_base, CMD_BUF_SIZE,
		cmdbuf_base + CMD_BUF_SIZE - 1));
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
#endif

/* Addresses pasting large amounts of data
 * code pinched from xterm
 */

static char *v_buffer;		/* pointer to physical buffer */
static char *v_bufstr = NULL;	/* beginning of area to write */
static char *v_bufptr;		/* end of area to write */
static char *v_bufend;		/* end of physical buffer */

/* output a burst of any pending data from a paste... */
static int
v_doPending()
{

  if (v_bufstr >= v_bufptr)
    return (0);
  v_writeBig(cmd_fd, NULL, 0);
  return (1);
}

/* Write data to the pty as typed by the user, pasted with the mouse,
 * or generated by us in response to a query ESC sequence.
 * Code stolen from xterm 
 */
static void
v_writeBig(int f, char *d, int len)
{

  int written;
  int c = len;

  if (v_bufstr == NULL && len > 0) {

    v_buffer = malloc(len);
    v_bufstr = v_buffer;
    v_bufptr = v_buffer;
    v_bufend = v_buffer + len;
  }
  /*
   * Append to the block we already have.
   * Always doing this simplifies the code, and
   * isn't too bad, either.  If this is a short
   * block, it isn't too expensive, and if this is
   * a long block, we won't be able to write it all
   * anyway.
   */

  if (len > 0) {
    if (v_bufend < v_bufptr + len) {	/* we've run out of room */
      if (v_bufstr != v_buffer) {
	/* there is unused space, move everything down */
	/* possibly overlapping bcopy here */

	/* bcopy(v_bufstr, v_buffer, v_bufptr - v_bufstr); */
	memcpy(v_buffer, v_bufstr, v_bufptr - v_bufstr);
	v_bufptr -= v_bufstr - v_buffer;
	v_bufstr = v_buffer;
      }
      if (v_bufend < v_bufptr + len) {
	/* still won't fit: get more space */
	/* Don't use XtRealloc because an error is not fatal. */
	int size = v_bufptr - v_buffer;		/* save across realloc */

	v_buffer = realloc(v_buffer, size + len);
	if (v_buffer) {
	  v_bufstr = v_buffer;
	  v_bufptr = v_buffer + size;
	  v_bufend = v_bufptr + len;
	} else {
	  /* no memory: ignore entire write request */
	  print_error("cannot allocate buffer space\n");
	  v_buffer = v_bufstr;	/* restore clobbered pointer */
	  c = 0;
	}
      }
    }
    if (v_bufend >= v_bufptr + len) {	/* new stuff will fit */
      memcpy(v_bufptr, d, len);	/* bcopy(d, v_bufptr, len); */
      v_bufptr += len;
    }
  }
  /*
   * Write out as much of the buffer as we can.
   * Be careful not to overflow the pty's input silo.
   * We are conservative here and only write
   * a small amount at a time.
   *
   * If we can't push all the data into the pty yet, we expect write
   * to return a non-negative number less than the length requested
   * (if some data written) or -1 and set errno to EAGAIN,
   * EWOULDBLOCK, or EINTR (if no data written).
   *
   * (Not all systems do this, sigh, so the code is actually
   * a little more forgiving.)
   */

#if defined(linux)
# ifdef PTY_BUF_SIZE		/* From <linux/tty.h> */
#  define MAX_PTY_WRITE PTY_BUF_SIZE
# endif
#endif

/* NOTE: _POSIX_MAX_INPUT is defined _through_ <limits.h> at least for
 * the following systems: HP-UX 10.20, AIX (no idea about the version),
 * OSF1/alpha 4.0, Linux (probably any Linux system).
 */
#ifndef MAX_PTY_WRITE
# ifdef _POSIX_VERSION
#  ifdef _POSIX_MAX_INPUT
#   define MAX_PTY_WRITE _POSIX_MAX_INPUT
#  else
#   define MAX_PTY_WRITE 255	/* POSIX minimum MAX_INPUT */
#  endif
# endif
#endif

#ifndef MAX_PTY_WRITE
# define MAX_PTY_WRITE 128	/* 1/2 POSIX minimum MAX_INPUT */
#endif

  if (v_bufptr > v_bufstr) {
    written = write(f, v_bufstr, v_bufptr - v_bufstr <= MAX_PTY_WRITE ?
		    v_bufptr - v_bufstr : MAX_PTY_WRITE);
    if (written < 0) {
      written = 0;
    }
    D_TTY(("v_writeBig(): Wrote %d characters\n", written));
    v_bufstr += written;
    if (v_bufstr >= v_bufptr)	/* we wrote it all */
      v_bufstr = v_bufptr = v_buffer;
  }
  /*
   * If we have lots of unused memory allocated, return it
   */
  if (v_bufend - v_bufptr > 1024) {	/* arbitrary hysteresis */
    /* save pointers across realloc */
    int start = v_bufstr - v_buffer;
    int size = v_bufptr - v_buffer;
    int allocsize = size ? size : 1;

    v_buffer = realloc(v_buffer, allocsize);
    if (v_buffer) {
      v_bufstr = v_buffer + start;
      v_bufptr = v_buffer + size;
      v_bufend = v_buffer + allocsize;
    } else {
      /* should we print a warning if couldn't return memory? */
      v_buffer = v_bufstr - start;	/* restore clobbered pointer */
    }
  }
}

#ifndef MAX
# define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

XErrorHandler
xerror_handler(Display * display, XErrorEvent * event)
{

  char err_string[2048];
  extern char *XRequest, *XlibMessage;

  strcpy(err_string, "");
  XGetErrorText(Xdisplay, event->error_code, err_string, sizeof(err_string));
  print_error("XError in function %s (request %d.%d):  %s (error %d)", request_code_to_name(event->request_code),
	      event->request_code, event->minor_code, err_string, event->error_code);
#if DEBUG > DEBUG_X11
  if (debug_level >= DEBUG_X11) {
    dump_stack_trace();
  }
#endif
  print_error("Attempting to continue...");
  return 0;
}

/* color aliases, fg/bg bright-bold */
/*static inline void */
/* inline void */
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

/*
 * find if fg/bg matches any of the normal (low-intensity) colors
 */
#ifndef NO_BRIGHTCOLOR
static inline void
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
#else /* NO_BRIGHTCOLOR */
# define set_colorfgbg() ((void)0)
#endif /* NO_BRIGHTCOLOR */

/* Create_Windows() - Open and map the window */
void
Create_Windows(int argc, char *argv[])
{

  Cursor cursor;
  XClassHint classHint;
  XWMHints wmHint;
  Atom prop;
  CARD32 val;
  int i, x, y, flags;
  unsigned int width, height;
  unsigned int r, g, b;
  MWMHints mwmhints;

/*    char *tmp; */

  if (Options & Opt_borderless) {
    prop = XInternAtom(Xdisplay, "_MOTIF_WM_HINTS", True);
    if (prop == None) {
      print_warning("Window Manager does not support MWM hints.  Bypassing window manager control for borderless window.");
      Attributes.override_redirect = TRUE;
      mwmhints.flags = 0;
    } else {
      mwmhints.flags = MWM_HINTS_DECORATIONS;
      mwmhints.decorations = 0;
    }
  }
  Attributes.save_under = TRUE;
  Attributes.backing_store = WhenMapped;

  /*
   * grab colors before netscape does
   */
  for (i = 0; i < (Xdepth <= 2 ? 2 : NRS_COLORS); i++) {

    XColor xcol;
    unsigned char found_color;

    if (!rs_color[i])
      continue;

    if (!XParseColor(Xdisplay, Xcmap, rs_color[i], &xcol)) {
      print_warning("Unable to resolve \"%s\" as a color name.  Falling back on \"%s\".",
		    rs_color[i], def_colorName[i] ? def_colorName[i] : "(nil)");
      rs_color[i] = def_colorName[i];
      if (!rs_color[i])
	continue;
      if (!XParseColor(Xdisplay, Xcmap, rs_color[i], &xcol)) {
	print_warning("Unable to resolve \"%s\" as a color name.  This should never fail.  Please repair/restore your RGB database.", rs_color[i]);
	found_color = 0;
      } else {
	found_color = 1;
      }
    } else {
      found_color = 1;
    }
    if (found_color) {
      r = xcol.red;
      g = xcol.green;
      b = xcol.blue;
      xcol.pixel = Imlib_best_color_match(imlib_id, &r, &g, &b);
      if (!XAllocColor(Xdisplay, Xcmap, &xcol)) {
	print_warning("Unable to allocate \"%s\" (0x%08x:  0x%04x, 0x%04x, 0x%04x) in the color map.  "
		      "Falling back on \"%s\".",
		      rs_color[i], xcol.pixel, r, g, b, def_colorName[i] ? def_colorName[i] : "(nil)");
	rs_color[i] = def_colorName[i];
	if (!rs_color[i])
	  continue;
	if (!XAllocColor(Xdisplay, Xcmap, &xcol)) {
	  print_warning("Unable to allocate \"%s\" (0x%08x:  0x%04x, 0x%04x, 0x%04x) in the color map.",
			rs_color[i], xcol.pixel, r, g, b);
	  found_color = 0;
	} else {
	  found_color = 1;
	}
      } else {
	found_color = 1;
      }
    }
    if (!found_color) {
      switch (i) {
	case fgColor:
	case bgColor:
	  /* fatal: need bg/fg color */
	  fatal_error("Unable to get foreground/background colors!");
	  break;
#ifndef NO_CURSORCOLOR
	case cursorColor:
	  xcol.pixel = PixColors[bgColor];
	  break;
	case cursorColor2:
	  xcol.pixel = PixColors[fgColor];
	  break;
#endif /* NO_CURSORCOLOR */
	default:
	  xcol.pixel = PixColors[bgColor];	/* None */
	  break;
      }
    }
    PixColors[i] = xcol.pixel;
  }

#ifndef NO_CURSORCOLOR
  if (Xdepth <= 2 || !rs_color[cursorColor])
    PixColors[cursorColor] = PixColors[bgColor];
  if (Xdepth <= 2 || !rs_color[cursorColor2])
    PixColors[cursorColor2] = PixColors[fgColor];
#endif /* NO_CURSORCOLOR */
  if (Xdepth <= 2 || !rs_color[pointerColor])
    PixColors[pointerColor] = PixColors[fgColor];
  if (Xdepth <= 2 || !rs_color[borderColor])
    PixColors[borderColor] = PixColors[bgColor];

#ifndef NO_BOLDUNDERLINE
  if (Xdepth <= 2 || !rs_color[colorBD])
    PixColors[colorBD] = PixColors[fgColor];
  if (Xdepth <= 2 || !rs_color[colorUL])
    PixColors[colorUL] = PixColors[fgColor];
#endif /* NO_BOLDUNDERLINE */

  /*
   * get scrollBar/menuBar shadow colors
   *
   * The calculations of topShadow/bottomShadow values are adapted
   * from the fvwm window manager.
   */
#ifdef KEEP_SCROLLCOLOR
  if (Xdepth <= 2) {		/* Monochrome */
    PixColors[scrollColor] = PixColors[bgColor];
    PixColors[topShadowColor] = PixColors[fgColor];
    PixColors[bottomShadowColor] = PixColors[fgColor];

# ifdef CHANGE_SCROLLCOLOR_ON_FOCUS
    PixColors[unfocusedScrollColor] = PixColors[bgColor];
    PixColors[unfocusedTopShadowColor] = PixColors[fgColor];
    PixColors[unfocusedBottomShadowColor] = PixColors[fgColor];
# endif

  } else {

    XColor xcol, white;

    /* bottomShadowColor */
    xcol.pixel = PixColors[scrollColor];
    XQueryColor(Xdisplay, Xcmap, &xcol);

    xcol.red /= 2;
    xcol.green /= 2;
    xcol.blue /= 2;
    r = xcol.red;
    g = xcol.green;
    b = xcol.blue;
    xcol.pixel = Imlib_best_color_match(imlib_id, &r, &g, &b);

    if (!XAllocColor(Xdisplay, Xcmap, &xcol)) {
      print_error("Unable to allocate \"bottomShadowColor\" (0x%08x:  0x%04x, 0x%04x, 0x%04x) in the color map.",
		  xcol.pixel, r, g, b);
      xcol.pixel = PixColors[minColor];
    }
    PixColors[bottomShadowColor] = xcol.pixel;

#ifdef CHANGE_SCROLLCOLOR_ON_FOCUS
    /* unfocusedBottomShadowColor */
    xcol.pixel = PixColors[unfocusedScrollColor];
    XQueryColor(Xdisplay, Xcmap, &xcol);

    xcol.red /= 2;
    xcol.green /= 2;
    xcol.blue /= 2;
    r = xcol.red;
    g = xcol.green;
    b = xcol.blue;
    xcol.pixel = Imlib_best_color_match(imlib_id, &r, &g, &b);

    if (!XAllocColor(Xdisplay, Xcmap, &xcol)) {
      print_error("Unable to allocate \"unfocusedbottomShadowColor\" (0x%08x:  0x%04x, 0x%04x, 0x%04x) in the color map.",
		  xcol.pixel, r, g, b);
      xcol.pixel = PixColors[minColor];
    }
    PixColors[unfocusedBottomShadowColor] = xcol.pixel;
#endif

    /* topShadowColor */
# ifdef PREFER_24BIT
    white.red = white.green = white.blue = r = g = b = ~0;
    white.pixel = Imlib_best_color_match(imlib_id, &r, &g, &b);
    XAllocColor(Xdisplay, Xcmap, &white);
# else
    white.pixel = WhitePixel(Xdisplay, Xscreen);
    XQueryColor(Xdisplay, Xcmap, &white);
# endif

    xcol.pixel = PixColors[scrollColor];
    XQueryColor(Xdisplay, Xcmap, &xcol);

    xcol.red = max((white.red / 5), xcol.red);
    xcol.green = max((white.green / 5), xcol.green);
    xcol.blue = max((white.blue / 5), xcol.blue);

    xcol.red = min(white.red, (xcol.red * 7) / 5);
    xcol.green = min(white.green, (xcol.green * 7) / 5);
    xcol.blue = min(white.blue, (xcol.blue * 7) / 5);
    r = xcol.red;
    g = xcol.green;
    b = xcol.blue;
    xcol.pixel = Imlib_best_color_match(imlib_id, &r, &g, &b);

    if (!XAllocColor(Xdisplay, Xcmap, &xcol)) {
      print_error("Unable to allocate \"topShadowColor\" (0x%08x:  0x%04x, 0x%04x, 0x%04x) in the color map.",
		  xcol.pixel, r, g, b);
      xcol.pixel = PixColors[WhiteColor];
    }
    PixColors[topShadowColor] = xcol.pixel;

#ifdef CHANGE_SCROLLCOLOR_ON_FOCUS
    /* Do same for unfocusedTopShadowColor */
    xcol.pixel = PixColors[unfocusedScrollColor];
    XQueryColor(Xdisplay, Xcmap, &xcol);

    xcol.red = max((white.red / 5), xcol.red);
    xcol.green = max((white.green / 5), xcol.green);
    xcol.blue = max((white.blue / 5), xcol.blue);

    xcol.red = min(white.red, (xcol.red * 7) / 5);
    xcol.green = min(white.green, (xcol.green * 7) / 5);
    xcol.blue = min(white.blue, (xcol.blue * 7) / 5);
    r = xcol.red;
    g = xcol.green;
    b = xcol.blue;
    xcol.pixel = Imlib_best_color_match(imlib_id, &r, &g, &b);

    if (!XAllocColor(Xdisplay, Xcmap, &xcol)) {
      print_error("Unable to allocate \"unfocusedtopShadowColor\" (0x%08x:  0x%04x, 0x%04x, 0x%04x) in the color map.",
		  xcol.pixel, r, g, b);
      xcol.pixel = PixColors[WhiteColor];
    }
    PixColors[unfocusedTopShadowColor] = xcol.pixel;
#endif

  }
#endif /* KEEP_SCROLLCOLOR */

  szHint.base_width = (2 * TermWin.internalBorder +
		       (Options & Opt_scrollBar ? scrollbar_total_width()
			: 0));
  szHint.base_height = (2 * TermWin.internalBorder);

  flags = (rs_geometry ? XParseGeometry(rs_geometry, &x, &y, &width, &height) : 0);
  D_X11(("XParseGeometry(geom, %d, %d, %d, %d)\n", x, y, width, height));

  if (flags & WidthValue) {
    szHint.width = width;
    szHint.flags |= USSize;
  }
  if (flags & HeightValue) {
    szHint.height = height;
    szHint.flags |= USSize;
  }
  TermWin.ncol = szHint.width;
  TermWin.nrow = szHint.height;

  change_font(1, NULL);
#if (MENUBAR_MAX)
  szHint.base_height += (delay_menu_drawing ? menuBar_TotalHeight() : 0);
#endif
  if (flags & XValue) {
    if (flags & XNegative) {
      if (check_for_enlightenment()) {
	x += (DisplayWidth(Xdisplay, Xscreen));
      } else {
	x += (DisplayWidth(Xdisplay, Xscreen) - (szHint.width + TermWin.internalBorder));
      }
      szHint.win_gravity = NorthEastGravity;
    }
    szHint.x = x;
    szHint.flags |= USPosition;
  }
  if (flags & YValue) {
    if (flags & YNegative) {
      if (check_for_enlightenment()) {
	y += (DisplayHeight(Xdisplay, Xscreen) - (2 * TermWin.internalBorder));
      } else {
	y += (DisplayHeight(Xdisplay, Xscreen) - (szHint.height + TermWin.internalBorder));
      }
      szHint.win_gravity = (szHint.win_gravity == NorthEastGravity ?
			    SouthEastGravity : SouthWestGravity);
    }
    szHint.y = y;
    szHint.flags |= USPosition;
  }
  D_X11(("Geometry values after parsing:  %dx%d%+d%+d\n", width, height, x, y));

  /* parent window - reverse video so we can see placement errors
   * sub-window placement & size in resize_subwindows()
   */

  Attributes.background_pixel = PixColors[bgColor];
  Attributes.border_pixel = PixColors[bgColor];
#ifdef PREFER_24BIT
  Attributes.colormap = Xcmap;
  TermWin.parent = XCreateWindow(Xdisplay, Xroot,
				 szHint.x, szHint.y,
				 szHint.width, szHint.height,
				 0,
				 Xdepth, InputOutput,
				 Xvisual,
				 CWBackPixel | CWBorderPixel | CWColormap | CWOverrideRedirect,
				 &Attributes);
#else
  TermWin.parent = XCreateWindow(Xdisplay, Xroot,
				 szHint.x, szHint.y,
				 szHint.width, szHint.height,
				 0,
				 Xdepth,
				 InputOutput,
				 CopyFromParent,
				 CWBackPixel | CWBorderPixel | CWOverrideRedirect,
				 &Attributes);
#endif

  xterm_seq(XTerm_title, rs_title);
  xterm_seq(XTerm_iconName, rs_iconName);
  classHint.res_name = (char *) rs_name;
  classHint.res_class = APL_NAME;
  wmHint.window_group = TermWin.parent;
  wmHint.input = True;
  wmHint.initial_state = (Options & Opt_iconic ? IconicState : NormalState);
  wmHint.window_group = TermWin.parent;
  wmHint.flags = (InputHint | StateHint | WindowGroupHint);
#ifdef PIXMAP_SUPPORT
  set_icon_pixmap(rs_icon, &wmHint);
#endif

  XSetWMProperties(Xdisplay, TermWin.parent, NULL, NULL, argv, argc, &szHint, &wmHint, &classHint);
  XSelectInput(Xdisplay, TermWin.parent, (KeyPressMask | FocusChangeMask | StructureNotifyMask | VisibilityChangeMask));
  if (mwmhints.flags) {
    XChangeProperty(Xdisplay, TermWin.parent, prop, prop, 32, PropModeReplace, (unsigned char *) &mwmhints, PROP_MWM_HINTS_ELEMENTS);
  }
  /* vt cursor: Black-on-White is standard, but this is more popular */
  TermWin_cursor = XCreateFontCursor(Xdisplay, XC_xterm);
  {

    XColor fg, bg;

    fg.pixel = PixColors[pointerColor];
    XQueryColor(Xdisplay, Xcmap, &fg);
    bg.pixel = PixColors[bgColor];
    XQueryColor(Xdisplay, Xcmap, &bg);
    XRecolorCursor(Xdisplay, TermWin_cursor, &fg, &bg);
  }

  /* cursor (menuBar/scrollBar): Black-on-White */
  cursor = XCreateFontCursor(Xdisplay, XC_left_ptr);

  /* the vt window */

#ifdef BACKING_STORE
  if ((!(Options & Opt_borderless))
      && (Options & Opt_saveUnder)) {
    D_X11(("Creating term window with save_under = TRUE\n"));
    TermWin.vt = XCreateWindow(Xdisplay, TermWin.parent,
			       0, 0,
			       szHint.width, szHint.height,
			       0,
			       Xdepth,
			       InputOutput,
			       CopyFromParent,
			       CWBackPixel | CWBorderPixel | CWOverrideRedirect | CWSaveUnder | CWBackingStore,
			       &Attributes);
    if (!(background_is_pixmap()) && !(Options & Opt_borderless)) {
      XSetWindowBackground(Xdisplay, TermWin.vt, PixColors[bgColor]);
      XClearWindow(Xdisplay, TermWin.vt);
    }
  } else
#endif
  {
    D_X11(("Creating term window with no backing store\n"));
    TermWin.vt = XCreateWindow(Xdisplay, TermWin.parent,
			       0, 0,
			       szHint.width, szHint.height,
			       0,
			       Xdepth,
			       InputOutput,
			       CopyFromParent,
			       CWBackPixel | CWBorderPixel | CWOverrideRedirect,
			       &Attributes);
    if (!(background_is_pixmap()) && !(Options & Opt_borderless)) {
      XSetWindowBackground(Xdisplay, TermWin.vt, PixColors[bgColor]);
      XClearWindow(Xdisplay, TermWin.vt);
    }
  }

  XDefineCursor(Xdisplay, TermWin.vt, TermWin_cursor);
#ifdef USE_ACTIVE_TAGS
  XSelectInput(Xdisplay, TermWin.vt,
	       (ExposureMask | ButtonPressMask | ButtonReleaseMask |
		Button1MotionMask | Button3MotionMask |
		PointerMotionMask | LeaveWindowMask));
#else
  XSelectInput(Xdisplay, TermWin.vt,
	       (ExposureMask | ButtonPressMask | ButtonReleaseMask |
		Button1MotionMask | Button3MotionMask));
#endif

  /* If the user wants a specific desktop, tell the WM that */
  if (rs_desktop != -1) {
    prop = XInternAtom(Xdisplay, "_WIN_WORKSPACE", False);
    val = rs_desktop;
    XChangeProperty(Xdisplay, TermWin.parent, prop, XA_CARDINAL, 32, PropModeReplace, (unsigned char *) &val, 1);
  }
  XMapWindow(Xdisplay, TermWin.vt);
  XMapWindow(Xdisplay, TermWin.parent);

  /* scrollBar: size doesn't matter */
#ifdef KEEP_SCROLLCOLOR
  Attributes.background_pixel = PixColors[scrollColor];
#else
  Attributes.background_pixel = PixColors[fgColor];
#endif
  Attributes.border_pixel = PixColors[bgColor];

  scrollBar.win = XCreateWindow(Xdisplay, TermWin.parent,
				0, 0,
				1, 1,
				0,
				Xdepth,
				InputOutput,
				CopyFromParent,
				CWOverrideRedirect | CWSaveUnder | CWBackPixel | CWBorderPixel,
				&Attributes);

  XDefineCursor(Xdisplay, scrollBar.win, cursor);
  XSelectInput(Xdisplay, scrollBar.win,
	       (ExposureMask | ButtonPressMask | ButtonReleaseMask |
		Button1MotionMask | Button2MotionMask | Button3MotionMask)
      );

#ifdef PIXMAP_SCROLLBAR
  if (scrollbar_is_pixmapped()) {
    scrollBar.up_win = XCreateWindow(Xdisplay, scrollBar.win,
				     0, 0,
				     scrollbar_total_width(),
				     scrollbar_arrow_height(),
				     0,
				     Xdepth,
				     InputOutput,
				     CopyFromParent,
				     CWOverrideRedirect | CWSaveUnder,
				     &Attributes);

    XDefineCursor(Xdisplay, scrollBar.up_win, cursor);
    XSelectInput(Xdisplay, scrollBar.up_win,
		 (ExposureMask | ButtonPressMask | ButtonReleaseMask |
		  Button1MotionMask | Button2MotionMask | Button3MotionMask)
	);

    scrollBar.dn_win = XCreateWindow(Xdisplay, scrollBar.win,
				     0,
				     scrollbar_arrow_height()
				     + scrollbar_anchor_max_height(),
				     scrollbar_total_width(),
				     scrollbar_arrow_height(),
				     0,
				     Xdepth,
				     InputOutput,
				     CopyFromParent,
				     CWOverrideRedirect | CWSaveUnder,
				     &Attributes);

    XDefineCursor(Xdisplay, scrollBar.dn_win, cursor);
    XSelectInput(Xdisplay, scrollBar.dn_win,
		 (ExposureMask | ButtonPressMask | ButtonReleaseMask |
		  Button1MotionMask | Button2MotionMask | Button3MotionMask)
	);
    scrollBar.sa_win = XCreateWindow(Xdisplay, scrollBar.win,
				     0,
				     scrollbar_arrow_height(),
				     scrollbar_total_width(),
				     scrollbar_anchor_max_height(),
				     0,
				     Xdepth,
				     InputOutput,
				     CopyFromParent,
				     CWOverrideRedirect | CWSaveUnder | CWBackingStore,
				     &Attributes);

    XDefineCursor(Xdisplay, scrollBar.sa_win, cursor);
    XSelectInput(Xdisplay, scrollBar.sa_win,
		 (ExposureMask | ButtonPressMask | ButtonReleaseMask |
		  Button1MotionMask | Button2MotionMask | Button3MotionMask)
	);
  }
#endif

#if (MENUBAR_MAX)
  /* menuBar: size doesn't matter */
# ifdef KEEP_SCROLLCOLOR
  Attributes.background_pixel = PixColors[scrollColor];
# else
  Attributes.background_pixel = PixColors[fgColor];
# endif
  Attributes.border_pixel = PixColors[bgColor];
  menuBar.win = XCreateWindow(Xdisplay, TermWin.parent,
			      0, 0,
			      1, 1,
			      0,
			      Xdepth,
			      InputOutput,
			      CopyFromParent,
			      CWOverrideRedirect | CWSaveUnder | CWBackingStore | CWBackPixel | CWBorderPixel,
			      &Attributes);


# ifdef PIXMAP_MENUBAR
  if (menubar_is_pixmapped()) {
    set_Pixmap(rs_pixmaps[pixmap_mb], mbPixmap.pixmap, pixmap_mb);
    XSetWindowBackgroundPixmap(Xdisplay, menuBar.win,
			       mbPixmap.pixmap);
  } else
# endif
  {
# ifdef KEEP_SCROLLCOLOR
    XSetWindowBackground(Xdisplay, menuBar.win, PixColors[scrollColor]);
# else
    XSetWindowBackground(Xdisplay, menuBar.win, PixColors[fgColor]);
# endif
  }

  XClearWindow(Xdisplay, menuBar.win);

  XDefineCursor(Xdisplay, menuBar.win, cursor);
  XSelectInput(Xdisplay, menuBar.win,
	       (ExposureMask | ButtonPressMask | ButtonReleaseMask |
		Button1MotionMask)
      );
#endif /* MENUBAR_MAX */

  XSetWindowBackground(Xdisplay, TermWin.vt, PixColors[bgColor]);
  XClearWindow(Xdisplay, TermWin.vt);

#ifdef PIXMAP_SUPPORT
  if (rs_pixmaps[pixmap_bg] != NULL) {

    char *p = rs_pixmaps[pixmap_bg];

    if ((p = strchr(p, '@')) != NULL) {
      p++;
      scale_pixmap(p, &bgPixmap);
    }
    D_PIXMAP(("set_bgPixmap() call #1\n"));
    set_bgPixmap(rs_pixmaps[pixmap_bg]);
  }
# ifdef PIXMAP_SCROLLBAR
  if (scrollbar_is_pixmapped()) {
    if (rs_pixmaps[pixmap_sb] != NULL) {

      char *p = rs_pixmaps[pixmap_sb];

      if ((p = strchr(p, '@')) != NULL) {
	p++;
	scale_pixmap(p, &sbPixmap);
      }
      fprintf(stderr, "scrollbar sb: %s\n", p);
      D_PIXMAP(("set_Pixmap(rs_pixmaps[pixmap_sb], pixmap_sb)\n"));
      set_Pixmap(rs_pixmaps[pixmap_sb], sbPixmap.pixmap, pixmap_sb);
    }
    if (rs_pixmaps[pixmap_up] != NULL) {

      char *p = rs_pixmaps[pixmap_up];

      if ((p = strchr(p, '@')) != NULL) {
	p++;
	scale_pixmap(p, &upPixmap);
      }
      fprintf(stderr, "scrollbar up: %s\n", p);
      D_PIXMAP(("set_Pixmap(rs_pixmaps[pixmap_up], pixmap_up)\n"));
      set_Pixmap(rs_pixmaps[pixmap_up], upPixmap.pixmap, pixmap_up);
    }
    if (rs_pixmaps[pixmap_upclk] != NULL) {

      char *p = rs_pixmaps[pixmap_upclk];

      if ((p = strchr(p, '@')) != NULL) {
	p++;
	scale_pixmap(p, &up_clkPixmap);
      }
      fprintf(stderr, "scrollbar upclk: %s\n", p);
      D_PIXMAP(("set_Pixmap(rs_pixmaps[pixmap_upclk], pixmap_upclk)\n"));
      set_Pixmap(rs_pixmaps[pixmap_upclk], up_clkPixmap.pixmap, pixmap_upclk);
    }
    if (rs_pixmaps[pixmap_dn] != NULL) {

      char *p = rs_pixmaps[pixmap_dn];

      if ((p = strchr(p, '@')) != NULL) {
	p++;
	scale_pixmap(p, &dnPixmap);
      }
      fprintf(stderr, "scrollbar dn: %s\n", p);
      D_PIXMAP(("set_Pixmap(rs_pixmaps[pixmap_dn], pixmap_dn)\n"));
      set_Pixmap(rs_pixmaps[pixmap_dn], dnPixmap.pixmap, pixmap_dn);
    }
    if (rs_pixmaps[pixmap_dnclk] != NULL) {

      char *p = rs_pixmaps[pixmap_dnclk];

      if ((p = strchr(p, '@')) != NULL) {
	p++;
	scale_pixmap(p, &dn_clkPixmap);
      }
      fprintf(stderr, "scrollbar dnclk: %s\n", p);
      D_PIXMAP(("set_Pixmap(rs_pixmaps[pixmap_dnclk], pixmap_dnclk)\n"));
      set_Pixmap(rs_pixmaps[pixmap_dnclk], dn_clkPixmap.pixmap, pixmap_dnclk);
    }
    if (rs_pixmaps[pixmap_sa] != NULL) {

      char *p = rs_pixmaps[pixmap_sa];

      if ((p = strchr(p, '@')) != NULL) {
	p++;
	scale_pixmap(p, &saPixmap);
      }
      fprintf(stderr, "scrollbar sa: %s\n", p);
      D_PIXMAP(("set_Pixmap(rs_pixmaps[pixmap_sa], pixmap_sa)\n"));
      set_Pixmap(rs_pixmaps[pixmap_sa], saPixmap.pixmap, pixmap_sa);
    }
    if (rs_pixmaps[pixmap_saclk] != NULL) {

      char *p = rs_pixmaps[pixmap_saclk];

      if ((p = strchr(p, '@')) != NULL) {
	p++;
	scale_pixmap(p, &sa_clkPixmap);
      }
      fprintf(stderr, "scrollbar saclk: %s\n", p);
      D_PIXMAP(("set_Pixmap(rs_pixmaps[pixmap_saclk], pixmap_saclk)\n"));
      set_Pixmap(rs_pixmaps[pixmap_saclk], sa_clkPixmap.pixmap, pixmap_saclk);
    }
  }
# endif				/* PIXMAP_SCROLLBAR */

# ifdef PIXMAP_MENUBAR
  if (menubar_is_pixmapped()) {
    if (rs_pixmaps[pixmap_mb] != NULL) {

      char *p = rs_pixmaps[pixmap_mb];

      if ((p = strchr(p, '@')) != NULL) {
	p++;
	scale_pixmap(p, &mbPixmap);
      }
      fprintf(stderr, "menubar mb: %s\n", p);
      set_Pixmap(rs_pixmaps[pixmap_mb], mbPixmap.pixmap, pixmap_mb);
    }
    if (rs_pixmaps[pixmap_ms] != NULL) {

      char *p = rs_pixmaps[pixmap_ms];

      if ((p = strchr(p, '@')) != NULL) {
	p++;
	scale_pixmap(p, &mb_selPixmap);
      }
      fprintf(stderr, "menubar ms: %s\n", p);
      D_PIXMAP(("set_Pixmap(rs_pixmaps[pixmap_ms], pixmap_ms)\n"));
      set_Pixmap(rs_pixmaps[pixmap_ms], mb_selPixmap.pixmap, pixmap_ms);
    }
  }
# endif				/* PIXMAP_MENUBAR */
#else /* PIXMAP_SUPPORT */
  XSetWindowBackground(Xdisplay, TermWin.vt, PixColors[bgColor]);
  XClearWindow(Xdisplay, TermWin.vt);
#endif /* PIXMAP_SUPPORT */

  /* graphics context for the vt window */
  {

    XGCValues gcvalue;

    gcvalue.font = TermWin.font->fid;
    gcvalue.foreground = PixColors[fgColor];
    gcvalue.background = PixColors[bgColor];
    gcvalue.graphics_exposures = 0;
    TermWin.gc = XCreateGC(Xdisplay, TermWin.vt,
			   GCForeground | GCBackground | GCFont | GCGraphicsExposures,
			   &gcvalue);
  }

  if (Options & Opt_noCursor)
    scr_cursor_visible(0);
}

/* window resizing - assuming the parent window is the correct size */
void
resize_subwindows(int width, int height)
{

  int x = 0, y = 0;

#ifdef RXVT_GRAPHICS
  int old_width = TermWin.width;
  int old_height = TermWin.height;

#endif

  D_SCREEN(("resize_subwindows(%d, %d)\n", width, height));
  TermWin.width = TermWin.ncol * TermWin.fwidth;
  TermWin.height = TermWin.nrow * TermWin.fheight;

  /* size and placement */
  if (scrollbar_visible()) {
    scrollBar.beg = 0;
    scrollBar.end = height;
#ifdef MOTIF_SCROLLBAR
    if (scrollBar.type == SCROLLBAR_MOTIF) {
      /* arrows are as high as wide - leave 1 pixel gap */
      scrollBar.beg += scrollbar_arrow_height();
      scrollBar.end -= scrollbar_arrow_height();
    }
#endif
#ifdef NEXT_SCROLLBAR
    if (scrollBar.type == SCROLLBAR_NEXT) {
      scrollBar.beg = sb_shadow;
      scrollBar.end -= (scrollBar.width * 2 + (sb_shadow ? sb_shadow : 1) + 2);
    }
#endif
    width -= scrollbar_total_width();
    XMoveResizeWindow(Xdisplay, scrollBar.win,
		      ((Options & Opt_scrollBar_right) ? (width) : (x)),
		      0, scrollbar_total_width(), height);

    if (!(Options & Opt_scrollBar_right)) {
      x = scrollbar_total_width();
    }
  }
#if (MENUBAR_MAX)
  if (menubar_visible()) {
    y = menuBar_TotalHeight();	/* for placement of vt window */
    XMoveResizeWindow(Xdisplay, menuBar.win, x, 0, width, y);
    if ((!(menubar_is_pixmapped()))
	&& ((Options & Opt_borderless) || (Options & Opt_saveUnder)))
      XSetWindowBackground(Xdisplay, menuBar.win, PixColors[scrollColor]);
  }
#endif /* NO_MENUBAR */

  XMoveResizeWindow(Xdisplay, TermWin.vt, x, y, width, height + 1);

#ifdef RXVT_GRAPHICS
  if (old_width)
    Gr_Resize(old_width, old_height);
#endif
  XClearWindow(Xdisplay, TermWin.vt);
  if (!(background_is_pixmap()))
    XSetWindowBackground(Xdisplay, TermWin.vt, PixColors[bgColor]);

#ifdef PIXMAP_SUPPORT
# ifdef USE_POSIX_THREADS

  D_PIXMAP(("resize_subwindows(): start_bg_thread()\n"));
  pthread_attr_init(&resize_sub_thr_attr);

#  ifdef MUTEX_SYNCH
  if (pthread_mutex_trylock(&mutex) == EBUSY) {
    D_THREADS(("resize_subwindows(): mutex locked, bbl\n"));
  } else {
    D_THREADS(("pthread_mutex_trylock(&mutex): "));
    pthread_mutex_unlock(&mutex);
    D_THREADS(("pthread_mutex_unlock(&mutex)\n"));
  }
#  endif

  if (!(pthread_create(&resize_sub_thr, &resize_sub_thr_attr,
		       (void *) &render_bg_thread, NULL))) {
    /*              bg_set = 0; */
    D_THREADS(("thread created\n"));
  } else {
    D_THREADS(("pthread_create() failed!\n"));
  }

# else
  D_PIXMAP(("resize_subwindows(): render_pixmap(TermWin.vt)\n"));
  render_pixmap(TermWin.vt, imlib_bg, bgPixmap, 0, 1);
  XSync(Xdisplay, 0);
# endif
#endif
}

static void
resize(void)
{
  szHint.base_width = (2 * TermWin.internalBorder);
  szHint.base_height = (2 * TermWin.internalBorder);

  szHint.base_width += (scrollbar_visible()? scrollbar_total_width() : 0);
#if (MENUBAR_MAX)
  szHint.base_height += (menubar_visible()? menuBar_TotalHeight() : 0);
#endif

  szHint.min_width = szHint.base_width + szHint.width_inc;
  szHint.min_height = szHint.base_height + szHint.height_inc;

  szHint.width = szHint.base_width + TermWin.width;
  szHint.height = szHint.base_height + TermWin.height;

  szHint.flags = PMinSize | PResizeInc | PBaseSize | PWinGravity;

  XSetWMNormalHints(Xdisplay, TermWin.parent, &szHint);
  XResizeWindow(Xdisplay, TermWin.parent, szHint.width, szHint.height);

  resize_subwindows(szHint.width, szHint.height);
}

/*
 * Redraw window after exposure or size change
 */
static void
resize_window1(unsigned int width, unsigned int height)
{
  static short first_time = 1;
  int new_ncol = (width - szHint.base_width) / TermWin.fwidth;
  int new_nrow = (height - szHint.base_height) / TermWin.fheight;

  if (first_time ||
      (new_ncol != TermWin.ncol) ||
      (new_nrow != TermWin.nrow)) {
    int curr_screen = -1;

    /* scr_reset only works on the primary screen */
    if (!first_time) {		/* this is not the first time thru */
      selection_clear();
      curr_screen = scr_change_screen(PRIMARY);
    }
    TermWin.ncol = new_ncol;
    TermWin.nrow = new_nrow;

    resize_subwindows(width, height);
    scr_reset();

    if (curr_screen >= 0)	/* this is not the first time thru */
      scr_change_screen(curr_screen);
    first_time = 0;
  } else if (Options & Opt_pixmapTrans) {
    resize_subwindows(width, height);
    scrollbar_show(0);
    scr_expose(0, 0, width, height);
  }
}

/*
 * good for toggling 80/132 columns
 */
void
set_width(unsigned short width)
{
  unsigned short height = TermWin.nrow;

  if (width != TermWin.ncol) {
    width = szHint.base_width + width * TermWin.fwidth;
    height = szHint.base_height + height * TermWin.fheight;

    XResizeWindow(Xdisplay, TermWin.parent, width, height);
    resize_window1(width, height);
  }
}

/*
 * Redraw window after exposure or size change
 */
void
resize_window(void)
{
  Window root;
  XEvent dummy;
  int x, y;
  unsigned int border, depth, width, height;

  while (XCheckTypedWindowEvent(Xdisplay, TermWin.parent,
				ConfigureNotify, &dummy));

  /* do we come from an fontchange? */
  if (font_change_count > 0) {
    font_change_count--;
    return;
  }
  XGetGeometry(Xdisplay, TermWin.parent,
	       &root, &x, &y, &width, &height, &border, &depth);
#if 0
  XGetGeometry(Xdisplay, TermWin.vt,
	       &root, &x, &y, &width, &height, &border, &depth);
#endif

  /* parent already resized */

  resize_window1(width, height);
}

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
# define set_iconName(str) XSetIconName (Xdisplay, TermWin.parent, str)
#endif

#ifdef XTERM_COLOR_CHANGE
static void
set_window_color(int idx, const char *color)
{

  XColor xcol;
  int i;
  unsigned int pixel, r, g, b;

  if (color == NULL || *color == '\0')
    return;

  /* handle color aliases */
  if (isdigit(*color)) {
    i = atoi(color);
    if (i >= 8 && i <= 15) {	/* bright colors */
      i -= 8;
# ifndef NO_BRIGHTCOLOR
      PixColors[idx] = PixColors[minBright + i];
      goto Done;
# endif
    }
    if (i >= 0 && i <= 7) {	/* normal colors */
      PixColors[idx] = PixColors[minColor + i];
      goto Done;
    }
  }
  if (XParseColor(Xdisplay, Xcmap, color, &xcol)) {
    r = xcol.red;
    g = xcol.green;
    b = xcol.blue;
    pixel = Imlib_best_color_match(imlib_id, &r, &g, &b);
    xcol.pixel = pixel;
    if (!XAllocColor(Xdisplay, Xcmap, &xcol)) {
      print_warning("Unable to allocate \"%s\" in the color map.\n", color);
      return;
    }
  } else {
    print_warning("Unable to resolve \"%s\" as a color name.\n", color);
    return;
  }

  /* XStoreColor(Xdisplay, Xcmap, XColor*); */

  /*
   * FIXME: should free colors here, but no idea how to do it so instead,
   * so just keep gobbling up the colormap
   */
# if 0
  for (i = BlackColor; i <= WhiteColor; i++)
    if (PixColors[idx] == PixColors[i])
      break;
  if (i > WhiteColor) {
    /* fprintf (stderr, "XFreeColors: PixColors[%d] = %lu\n", idx, PixColors[idx]); */
    XFreeColors(Xdisplay, Xcmap, (PixColors + idx), 1,
		DisplayPlanes(Xdisplay, Xscreen));
  }
# endif

  PixColors[idx] = xcol.pixel;

  /* XSetWindowAttributes attr; */
  /* Cursor cursor; */
Done:
  if (idx == bgColor)
    XSetWindowBackground(Xdisplay, TermWin.vt, PixColors[bgColor]);

  /* handle colorBD, scrollbar background, etc. */

  set_colorfgbg();
  {

    XColor fg, bg;

    fg.pixel = PixColors[fgColor];
    XQueryColor(Xdisplay, Xcmap, &fg);
    bg.pixel = PixColors[bgColor];
    XQueryColor(Xdisplay, Xcmap, &bg);

    XRecolorCursor(Xdisplay, TermWin_cursor, &fg, &bg);
  }
  /* the only reasonable way to enforce a clean update */
  scr_poweron();
}
#else
# define set_window_color(idx,color) ((void)0)
#endif /* XTERM_COLOR_CHANGE */

/* Macros to make parsing escape sequences slightly more readable.... <G> */
#define OPT_SET_OR_TOGGLE(s, mask, bit) do { \
        if (!(s) || !(*(s))) { \
	  if ((mask) & (bit)) { \
	    (mask) &= ~(bit); \
          } else { \
	    (mask) |= (bit); \
          } \
        } else if (BOOL_OPT_ISTRUE(s)) { \
	  if ((mask) & (bit)) return; \
	  (mask) |= (bit); \
	} else if (BOOL_OPT_ISFALSE(s)) { \
	  if (!((mask) & (bit))) return; \
	  (mask) &= ~(bit); \
	} \
      } while (0)
/* The macro below forces bit to the opposite state from what we want, so that the
   code that follows will set it right.  Hackish, but saves space. :)  Use this
   if you need to do some processing other than just setting the flag right. */
#define OPT_SET_OR_TOGGLE_NEG(s, mask, bit) do { if (s) { \
	if (BOOL_OPT_ISTRUE(s)) { \
	  if ((mask) & (bit)) return; \
	  (mask) &= ~(bit); \
	} else if (BOOL_OPT_ISFALSE(s)) { \
	  if (!((mask) & (bit))) return; \
	  (mask) |= (bit); \
	} \
      } } while (0)

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

#if MENUBAR_MAX
  char *menu_str;

#endif
#ifdef PIXMAP_SUPPORT
  int changed = 0, scaled = 0;

#endif

  if (!str)
    return;

#if MENUBAR_MAX
  menu_str = strdup(str);
#endif
#ifdef PIXMAP_SUPPORT
  orig_tnstr = tnstr = strdup(str);
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
#if MENUBAR_MAX
    case XTerm_Menu:
      menubar_dispatch(menu_str);
      free(menu_str);
      break;
#endif

    case XTerm_EtermSeq:

      /* Eterm proprietary escape sequences

         Syntax:  ESC ] 6 ; <op> ; <arg> BEL

         where <op> is:  0    Set/toggle transparency
         1    Set shade percentage
         2    Set tint mask
         3    Force update of pseudo-transparent background
         4    Set/toggle desktop watching
         10    Set scrollbar type/width
         11    Set/toggle right-side scrollbar
         12    Set/toggle floating scrollbar
         13    Set/toggle popup scrollbar
         15    Set/toggle menubar move
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
         15-19    Menubar Configuration
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
# ifdef IMLIB_TRANS
	    if (imlib_id) {
	      if (imlib_bg.im != NULL) {
		Imlib_kill_image(imlib_id, imlib_bg.im);
		imlib_bg.im = NULL;
	      }
	    }
#endif
	    set_bgPixmap(rs_pixmaps[pixmap_bg]);
	  } else {
	    Options |= Opt_pixmapTrans;
	    if (imlib_id) {
	      ImlibFreePixmap(imlib_id, bgPixmap.pixmap);
	      if (imlib_bg.im != NULL) {
		D_IMLIB(("ImlibDestroyImage()\n"));
		ImlibDestroyImage(imlib_id, imlib_bg.im);
		imlib_bg.im = NULL;
	      }
	      bgPixmap.pixmap = None;
	    }
	    TermWin.pixmap = None;
	  }
	  render_pixmap(TermWin.vt, imlib_bg, bgPixmap, 0, 1);
	  scr_touch();
	  break;
	case 1:
	  nstr = strsep(&tnstr, ";");
	  if (!nstr) {
	    break;
	  }
	  rs_shadePct = strtoul(nstr, (char **) NULL, 0);
	  D_EVENTS(("    XTerm_EtermSeq shade percentage is %d%%\n", rs_shadePct));
	  if (Options & Opt_pixmapTrans && desktop_pixmap != None) {
	    XFreePixmap(Xdisplay, desktop_pixmap);
	    desktop_pixmap = None;	/* Force the re-read */
	  }
	  if (Options & Opt_viewport_mode && viewport_pixmap != None) {
	    XFreePixmap(Xdisplay, viewport_pixmap);
	    viewport_pixmap = None;	/* Force the re-read */
	  }
	  render_pixmap(TermWin.vt, imlib_bg, bgPixmap, 0, 1);
	  scr_touch();
	  break;
	case 2:
	  nstr = strsep(&tnstr, ";");
	  if (!nstr) {
	    break;
	  }
	  if (!BEG_STRCASECMP(nstr, "none")) {
	    rs_tintMask = 0xffffff;
	  } else if (!BEG_STRCASECMP(nstr, "red")) {
	    rs_tintMask = 0xff8080;
	  } else if (!BEG_STRCASECMP(nstr, "green")) {
	    rs_tintMask = 0x80ff80;
	  } else if (!BEG_STRCASECMP(nstr, "blue")) {
	    rs_tintMask = 0x8080ff;
	  } else if (!BEG_STRCASECMP(nstr, "cyan")) {
	    rs_tintMask = 0x80ffff;
	  } else if (!BEG_STRCASECMP(nstr, "magenta")) {
	    rs_tintMask = 0xff80ff;
	  } else if (!BEG_STRCASECMP(nstr, "yellow")) {
	    rs_tintMask = 0xffff80;
	  } else {
	    rs_tintMask = strtoul(nstr, (char **) NULL, 0);
	  }
	  D_EVENTS(("    XTerm_EtermSeq tint mask is 0x%06x\n", rs_tintMask));
	  if (Options & Opt_pixmapTrans && desktop_pixmap != None) {
	    XFreePixmap(Xdisplay, desktop_pixmap);
	    desktop_pixmap = None;	/* Force the re-read */
	  }
	  if (Options & Opt_viewport_mode && viewport_pixmap != None) {
	    XFreePixmap(Xdisplay, viewport_pixmap);
	    viewport_pixmap = None;	/* Force the re-read */
	  }
	  render_pixmap(TermWin.vt, imlib_bg, bgPixmap, 0, 1);
	  scr_touch();
	  break;
	case 3:
	  if (Options & Opt_pixmapTrans) {
	    get_desktop_window();
	    if (desktop_pixmap != None) {
	      XFreePixmap(Xdisplay, desktop_pixmap);
	      desktop_pixmap = None;	/* Force the re-read */
	    }
	    render_pixmap(TermWin.vt, imlib_bg, bgPixmap, 0, 1);
	    scr_expose(0, 0, TermWin_TotalWidth(), TermWin_TotalHeight());
	  }
	  break;
	case 4:
	  nstr = strsep(&tnstr, ";");
	  OPT_SET_OR_TOGGLE(nstr, Options, Opt_watchDesktop);
	  if (Options & Opt_pixmapTrans) {
	    get_desktop_window();
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
	    map_scrollBar(0);
	    map_scrollBar(1);
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
	    map_scrollBar(0);
	    map_scrollBar(1);
	    scrollbar_show(0);
	  }
	  break;
	case 11:
	  nstr = strsep(&tnstr, ";");
	  OPT_SET_OR_TOGGLE(nstr, Options, Opt_scrollBar_right);
	  scrollbar_reset();
	  map_scrollBar(0);
	  map_scrollBar(1);
	  scrollbar_show(0);
	  break;
	case 12:
	  nstr = strsep(&tnstr, ";");
	  OPT_SET_OR_TOGGLE(nstr, Options, Opt_scrollBar_floating);
	  scrollbar_reset();
	  map_scrollBar(0);
	  map_scrollBar(1);
	  scrollbar_show(0);
	  break;
	case 13:
	  nstr = strsep(&tnstr, ";");
	  OPT_SET_OR_TOGGLE(nstr, Options, Opt_scrollbar_popup);
	  break;
	case 15:
	  nstr = strsep(&tnstr, ";");
	  OPT_SET_OR_TOGGLE(nstr, Options, Opt_menubar_move);
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
	  render_pixmap(TermWin.vt, imlib_bg, bgPixmap, 0, 1);
	  scr_touch();
	  break;
	case 25:
	  nstr = strsep(&tnstr, ";");
	  OPT_SET_OR_TOGGLE(nstr, Options, Opt_select_trailing_spaces);
	  break;
	case 30:
	  nstr = strsep(&tnstr, ";");
	  if (nstr) {
	    if (XParseColor(Xdisplay, Xcmap, nstr, &xcol) && XAllocColor(Xdisplay, Xcmap, &xcol)) {
	      PixColors[fgColor] = xcol.pixel;
	      scr_refresh(SMOOTH_REFRESH);
	    }
	  }
	  break;
	case 40:
	  nstr = strsep(&tnstr, ";");
	  if (nstr) {
	    if (XParseColor(Xdisplay, Xcmap, nstr, &xcol) && XAllocColor(Xdisplay, Xcmap, &xcol)) {
	      PixColors[bgColor] = xcol.pixel;
	      scr_refresh(SMOOTH_REFRESH);
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
      rs_shadePct = 0;
      rs_tintMask = 0xffffff;
      if (!strcmp(str, ";")) {
	rs_pixmaps[pixmap_bg] = "";
	set_bgPixmap("");
	return;
      }
      nstr = strsep(&tnstr, ";");
      if (nstr) {
	if (*nstr) {
	  scale_pixmap("", &bgPixmap);
	  D_PIXMAP(("set_bgPixmap() call #2\n"));
	  bg_needs_update = 1;
	  set_bgPixmap(nstr);
	}
	while ((nstr = strsep(&tnstr, ";")) && *nstr) {
	  changed += scale_pixmap(nstr, &bgPixmap);
	  scaled = 1;
	}
	/* FIXME: This used to be && instead of || to avoid unnecessary
	 * rendering under some circumstances... I'll try to look
	 * deeper :) -vendu
	 */
	if ((changed) || (bg_needs_update)) {
	  D_PIXMAP(("XTerm_Pixmap sequence: render_pixmap(TermWin.vt)\n"));
	  render_pixmap(TermWin.vt, imlib_bg, bgPixmap, 0, 1);
	  scr_touch();
	}
      } else {
	D_PIXMAP(("set_bgPixmap() call #3\n"));
	set_bgPixmap("");
      }
#endif /* PIXMAP_SUPPORT */
      break;

    case XTerm_restoreFG:
      set_window_color(fgColor, str);
      break;
    case XTerm_restoreBG:
      set_window_color(bgColor, str);
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
  free(orig_tnstr);
#endif
}

/* change_font() - Switch to a new font */
/*
 * init = 1   - initialize
 *
 * fontname == FONT_UP  - switch to bigger font
 * fontname == FONT_DN  - switch to smaller font
 */
#define ABORT() do { print_error("aborting"); exit(EXIT_FAILURE); } while (0)
void
change_font(int init, const char *fontname)
{
  const char *const msg = "can't load font \"%s\"";
  XFontStruct *xfont;
  static char *newfont[NFONTS];

#ifndef NO_BOLDFONT
  static XFontStruct *boldFont = NULL;

#endif
  static int fnum = FONT0_IDX;	/* logical font number */
  int idx = 0;			/* index into rs_font[] */

#if (FONT0_IDX == 0)
# define IDX2FNUM(i) (i)
# define FNUM2IDX(f) (f)
#else
# define IDX2FNUM(i) (i == 0? FONT0_IDX : (i <= FONT0_IDX? (i-1) : i))
# define FNUM2IDX(f) (f == FONT0_IDX ? 0 : (f < FONT0_IDX ? (f+1) : f))
#endif
#define FNUM_RANGE(i)	(i <= 0 ? 0 : (i >= NFONTS ? (NFONTS-1) : i))

  if (!init) {
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
	if (fontname != NULL) {
	  /* search for existing fontname */
	  for (idx = 0; idx < NFONTS; idx++) {
	    if (!strcmp(rs_font[idx], fontname)) {
	      fnum = IDX2FNUM(idx);
	      fontname = NULL;
	      break;
	    }
	  }
	} else
	  return;
	break;
    }
    /* re-position around the normal font */
    idx = FNUM2IDX(fnum);

    if (fontname != NULL) {
      char *name;

      xfont = XLoadQueryFont(Xdisplay, fontname);
      if (!xfont)
	return;

      name = MALLOC(strlen(fontname + 1) * sizeof(char));

      if (name == NULL) {
	XFreeFont(Xdisplay, xfont);
	return;
      }
      strcpy(name, fontname);
      if (newfont[idx] != NULL)
	FREE(newfont[idx]);
      newfont[idx] = name;
      rs_font[idx] = newfont[idx];
    }
  }
  if (TermWin.font)
    XFreeFont(Xdisplay, TermWin.font);

  /* load font or substitute */
  xfont = XLoadQueryFont(Xdisplay, rs_font[idx]);
  if (!xfont) {
    print_error(msg, rs_font[idx]);
    rs_font[idx] = "fixed";
    xfont = XLoadQueryFont(Xdisplay, rs_font[idx]);
    if (!xfont) {
      print_error(msg, rs_font[idx]);
      ABORT();
    }
  }
  TermWin.font = xfont;

#ifndef NO_BOLDFONT
  /* fail silently */
  if (init && rs_boldFont != NULL)
    boldFont = XLoadQueryFont(Xdisplay, rs_boldFont);
#endif

#ifdef KANJI
  if (TermWin.kanji)
    XFreeFont(Xdisplay, TermWin.kanji);

  /* load font or substitute */
  xfont = XLoadQueryFont(Xdisplay, rs_kfont[idx]);
  if (!xfont) {
    print_error(msg, rs_kfont[idx]);
    rs_kfont[idx] = "k14";
    xfont = XLoadQueryFont(Xdisplay, rs_kfont[idx]);
    if (!xfont) {
      print_error(msg, rs_kfont[idx]);
      ABORT();
    }
  }
  TermWin.kanji = xfont;
#endif /* KANJI */

  /* alter existing GC */
  if (!init) {
    XSetFont(Xdisplay, TermWin.gc, TermWin.font->fid);
#if (MENUBAR_MAX)
    menubar_expose();
#endif /* MENUBAR_MAX */
  }
  /* set the sizes */
  {

    int i, cw, fh, fw = 0;

    fw = TermWin.font->min_bounds.width;
    fh = TermWin.font->ascent + TermWin.font->descent;

    D_X11(("Font information:  Ascent == %hd, Descent == %hd\n", TermWin.font->ascent, TermWin.font->descent));
    if (TermWin.font->min_bounds.width == TermWin.font->max_bounds.width)
      TermWin.fprop = 0;	/* Mono-spaced (fixed width) font */
    else
      TermWin.fprop = 1;	/* Proportional font */
    if (TermWin.fprop == 1)
      for (i = TermWin.font->min_char_or_byte2;
	   i <= TermWin.font->max_char_or_byte2; i++) {
	cw = TermWin.font->per_char[i].width;
	MAX_IT(fw, cw);
      }
    /* not the first time thru and sizes haven't changed */
    if (fw == TermWin.fwidth && fh == TermWin.fheight)
      return;			/* TODO: not return; check KANJI if needed */

    TermWin.fwidth = fw;
    TermWin.fheight = fh;
  }

  /* check that size of boldFont is okay */
#ifndef NO_BOLDFONT
  TermWin.boldFont = NULL;
  if (boldFont != NULL) {
    int i, cw, fh, fw = 0;

    fw = boldFont->min_bounds.width;
    fh = boldFont->ascent + boldFont->descent;
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

    if (fw == TermWin.fwidth && fh == TermWin.fheight)
      TermWin.boldFont = boldFont;
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
#if (MENUBAR_MAX)
  szHint.height += (delay_menu_drawing ? menuBar_TotalHeight() : 0);
#endif

  szHint.flags = PMinSize | PResizeInc | PBaseSize | PWinGravity;

  if (!init) {
    font_change_count++;
    resize();
  }
  return;
#undef IDX2FNUM
#undef FNUM2IDX
#undef FNUM_RANGE
}
