/*
 * Copyright (C) 1997-2000, Michael Jennings
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

#ifndef _DEBUG_H
# define _DEBUG_H

#include <stdlib.h>
#include <time.h>

extern unsigned int debug_level;

/* Assert macros stolen from my work on Ebar.  If these macros break with your cpp, let me know -- mej@eterm.org */
# define NOP ((void)0)

#if defined(__FILE__) && defined(__LINE__)
# ifdef __FUNCTION__
#  define __DEBUG()  fprintf(stderr, "[%lu] %12s | %4d | %30s: ", (unsigned long) time(NULL), __FILE__, __LINE__, __FUNCTION__)
# else
#  define __DEBUG()  fprintf(stderr, "[%lu] %12s | %4d: ", (unsigned long) time(NULL), __FILE__, __LINE__)
# endif
#endif

#if defined(__FILE__) && defined(__LINE__)
# define ASSERT(x)  do {if (!(x)) {if (debug_level>=1) {fatal_error("ASSERT failed at %s:%d:  %s", __FILE__, __LINE__, #x);} \
                                                  else {print_warning("ASSERT failed at %s:%d:  %s", __FILE__, __LINE__, #x);}}} while (0)
# define ASSERT_RVAL(x, val)  do {if (!(x)) {if (debug_level>=1) {fatal_error("ASSERT failed at %s:%d:  %s", __FILE__, __LINE__, #x);} \
                                                            else {print_warning("ASSERT failed at %s:%d:  %s", __FILE__, __LINE__, #x);} \
                                             return (val);}} while (0)
# define ASSERT_NOTREACHED()  do {if (debug_level>=1) {fatal_error("ASSERT failed at %s:%d:  This code should not be reached.", __FILE__, __LINE__);} \
                                                 else {print_warning("ASSERT failed at %s:%d:  This code should not be reached.", __FILE__, __LINE__);} \
                                  } while (0)
# define ASSERT_NOTREACHED_RVAL(val)  do {if (debug_level>=1) {fatal_error("ASSERT failed at %s:%d:  This code should not be reached.", __FILE__, __LINE__);} \
                                                         else {print_warning("ASSERT failed at %s:%d:  This code should not be reached.", __FILE__, __LINE__);} \
                                          return (val);} while (0)
# define ABORT() fatal_error("Aborting at %s:%d.", __FILE__, __LINE__)
#else
# define ASSERT(x)  do {if (!(x)) {if (debug_level>=1) {fatal_error("ASSERT failed:  %s", #x);} \
                                                  else {print_warning("ASSERT failed:  %s", #x);}}} while (0)
# define ASSERT_RVAL(x, val)  do {if (!(x)) {if (debug_level>=1) {fatal_error("ASSERT failed:  %s", #x);} \
                                                            else {print_warning("ASSERT failed:  %s", #x);} return (val);}} while (0)
# define ASSERT_NOTREACHED()       return
# define ASSERT_NOTREACHED_RVAL(x) return (x)
# define ABORT() fatal_error("Aborting.")
#endif

#ifndef __DEBUG
# define __DEBUG()		NOP
#endif

#define REQUIRE(x) do {if (!(x)) {if (debug_level>=1) {__DEBUG(); real_dprintf("REQUIRE failed:  %s\n", #x);} return;}} while (0)
#define REQUIRE_RVAL(x, v) do {if (!(x)) {if (debug_level>=1) {__DEBUG(); real_dprintf("REQUIRE failed:  %s\n", #x);} return (v);}} while (0)
#define NONULL(x) ((x) ? (x) : ("<null>"))

/* Macros for printing debugging messages */
# if DEBUG >= 1
#  ifndef DPRINTF
#    define DPRINTF(x)          do { __DEBUG(); real_dprintf x; } while (0)
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

/* Use this for stuff that you only want turned on in dire situations */
#  define D_NEVER(x)			NOP

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
 
#  define DEBUG_X11			2
#  define D_X11(x)			DPRINTF2(x)
#  define DEBUG_ENL			2
#  define D_ENL(x)			DPRINTF2(x)
#  define DEBUG_SCROLLBAR		2
#  define D_SCROLLBAR(x)		DPRINTF2(x)
#  define DEBUG_TIMER			2
#  define D_TIMER(x)			DPRINTF2(x)
 
#  define DEBUG_MENU			3
#  define D_MENU(x)			DPRINTF3(x)
#  define DEBUG_FONT			3
#  define D_FONT(x)			DPRINTF3(x)
#  define DEBUG_TTYMODE			3
#  define D_TTYMODE(x)			DPRINTF3(x)
#  define DEBUG_COLORS			3
#  define D_COLORS(x)			DPRINTF3(x)
 
#  define DEBUG_MALLOC			4
#  define D_MALLOC(x)			DPRINTF4(x)
#  define DEBUG_ACTIONS			4
#  define D_ACTIONS(x)			DPRINTF4(x)

#  define DEBUG_X			5

#  define DEBUG_PARSE			9999
#  define D_PARSE(x)			D_NEVER(x)
#  define DEBUG_STRINGS			9999
#  define D_STRINGS(x)			D_NEVER(x)

#if (SIZEOF_LONG == 8)
# define MEMSET_LONG() l |= l<<32
#else
# define MEMSET_LONG() ((void)0)
#endif

#define MEMSET(s, c, count) do { \
    char *end = (char *)(s) + (count); \
    long l; \
    long *l_dest = (long *)(s); \
    char *c_dest; \
 \
    /* areas of less than 4 * sizeof(long) are set in 1-byte chunks. */ \
    if ((count) >= 4 * sizeof(long)) { \
        /* fill l with c. */ \
        l = (c) | (c)<<8; \
        l |= l<<16; \
        MEMSET_LONG(); \
 \
        /* fill in 1-byte chunks until boundary of long is reached. */ \
        if ((unsigned long)l_dest & (unsigned long)(sizeof(long) -1)) { \
            c_dest = (char *)l_dest; \
            while ((unsigned long)c_dest & (unsigned long)(sizeof(long) -1)) { \
                *(c_dest++) = (c); \
            } \
            l_dest = (long *)c_dest; \
        } \
 \
        /* fill in long-size chunks as long as possible. */ \
        while (((unsigned long) (end - (char *)l_dest)) >= sizeof(long)) { \
            *(l_dest++) = l; \
        } \
    } \
 \
    /* fill the tail in 1-byte chunks. */ \
    if ((char *)l_dest < end) { \
        c_dest = (char *)l_dest; \
        *(c_dest++) = (c); \
        while (c_dest < end) { \
            *(c_dest++) = (c); \
        } \
    } \
  } while (0)

#endif /* _DEBUG_H */
