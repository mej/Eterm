/* debug.h for Eterm.
 * 21 Feb 1998, vendu.
 */

#ifndef _DEBUG_H
# define _DEBUG_H

extern unsigned int debug_level;

/* Assert macros stolen from my work on Ebar.  If these macros break with your cpp, let me know -- mej@eterm.org */
#  define NOP ((void)0)

#if defined(__FILE__) && defined(__LINE__)
#  define ASSERT(x)  do {if (!(x)) {if (debug_level>=1) {fatal_error("ASSERT failed at %s:%d:  %s", __FILE__, __LINE__, #x);} \
                                                   else {print_warning("ASSERT failed at %s:%d:  %s", __FILE__, __LINE__, #x);}}} while (0);
#else
#  define ASSERT(x)  do {if (!(x)) {if (debug_level>=1) {fatal_error("ASSERT failed:  %s", #x);} \
                                                   else {print_warning("ASSERT failed:  %s", #x);}}} while (0);
#endif

#ifdef __FILE__
#ifdef __LINE__
#define __DEBUG()		fprintf(stderr, "%s, line %d: ", __FILE__, __LINE__);
#endif
#endif

#ifndef __DEBUG
#define __DEBUG()		NOP
#endif

/* Macros for printing debugging messages */
# if DEBUG >= 1
#  ifndef DPRINTF
#    define DPRINTF(x)          do { if (debug_level >= 1) {__DEBUG(); real_dprintf x;} } while (0)
#  endif
#  define DPRINTF1(x)           do { if (debug_level >= 1) {__DEBUG(); real_dprintf x;} } while (0)
#  define DPRINTF2(x)           do { if (debug_level >= 2) {__DEBUG(); real_dprintf x;} } while (0)
#  define DPRINTF3(x)           do { if (debug_level >= 3) {__DEBUG(); real_dprintf x;} } while (0)
#  define DPRINTF4(x)           do { if (debug_level >= 4) {__DEBUG(); real_dprintf x;} } while (0)
#  define DPRINTF5(x)           do { if (debug_level >= 5) {__DEBUG(); real_dprintf x;} } while (0)
# else
#  ifndef DPRINTF
#    define DPRINTF(x)          NOP
#  endif
#  define DPRINTF1(x)           NOP
#  define DPRINTF2(x)           NOP
#  define DPRINTF3(x)           NOP
#  define DPRINTF4(x)           NOP
#  define DPRINTF5(x)           NOP
# endif

/* Debugging macros/defines which set the debugging levels for each output type.
   To change the debugging level at which something appears, change the number in
   both the DEBUG_ definition and the D_ macro (if there is one). -- mej */

#  define DEBUG_SCREEN			1
#  define D_SCREEN(x)			DPRINTF1(x)
#  define DEBUG_CMD			1
#  define D_CMD(x)			DPRINTF1(x)
#  define DEBUG_TTY			1
#  define D_TTY(x)			DPRINTF1(x)
#  define DEBUG_SELECTION		1
#  define D_SELECT(x)			DPRINTF1(x)
#  define DEBUG_UTMP			1
#  define D_UTMP(x)			DPRINTF1(x)
#  define DEBUG_OPTIONS			1
#  define D_OPTIONS(x)			DPRINTF1(x)
#  define DEBUG_IMLIB			1
#  define D_IMLIB(x)			DPRINTF1(x)
#  define DEBUG_PIXMAP			1
#  define D_PIXMAP(x)			DPRINTF1(x)
#  define DEBUG_EVENTS			1
#  define D_EVENTS(x)			DPRINTF1(x)
#  define DEBUG_STRINGS			1
#  define D_STRINGS(x)			DPRINTF1(x)
 
#  define DEBUG_X11			2
#  define D_X11(x)			DPRINTF2(x)
#  define DEBUG_SCROLLBAR		2
#  define D_SCROLLBAR(x)		DPRINTF2(x)
#  define DEBUG_THREADS			2
#  define D_THREADS(x)			DPRINTF2(x)
#  define DEBUG_TAGS			2
#  define D_TAGS(x)			DPRINTF2(x)
 
#  define DEBUG_MENU			3
#  define D_MENUBAR(x)			DPRINTF3(x)
#  define DEBUG_TTYMODE			3
#  define D_TTYMODE(x)			DPRINTF3(x)
#  define DEBUG_COLORS			3
#  define D_COLORS(x)			DPRINTF3(x)
 
#  define DEBUG_MALLOC			4
#  define D_MALLOC(x)			DPRINTF4(x)
#  define DEBUG_MENUARROWS		4
#  define D_MENUARROWS(x)		DPRINTF4(x)
#  define DEBUG_MENU_LAYOUT		4
#  define D_MENU_LAYOUT(x)		DPRINTF4(x)
#  define DEBUG_MENUBAR_STACKING	4
#  define D_MENUBAR_STACKING(x)		DPRINTF4(x)
 
#  define DEBUG_X			5


#endif /* _DEBUG_H */
