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

#ifndef _LIBMEJ_H_
#define _LIBMEJ_H_

/* This GNU goop has to go before the system headers */
#ifdef __GNUC__
# ifndef __USE_GNU
#  define __USE_GNU
# endif
# ifndef _GNU_SOURCE
#  define _GNU_SOURCE
# endif
# ifndef _BSD_SOURCE
#  define _BSD_SOURCE
# endif
# ifndef inline
#  define inline __inline__
# endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#ifdef HAVE_REGEX_H
# include <regex.h>
#endif
#ifdef HAVE_STDARG_H
# include <stdarg.h>
#endif
#ifdef WITH_DMALLOC
# include <dmalloc.h>
#elif defined(HAVE_MALLOC_H)
# include <malloc.h>
#endif

/******************************* GENERIC GOOP *********************************/
#ifndef TRUE
# define TRUE    ((unsigned char)(1))
# define FALSE   ((unsigned char)(0))
#endif

#ifndef PACKAGE
# define PACKAGE "libmej"
#endif



/****************************** DEBUGGING GOOP ********************************/
#ifndef LIBMEJ_DEBUG_FD
# define LIBMEJ_DEBUG_FD  (stderr)
#endif
#ifndef DEBUG
# define DEBUG 0
#endif

#define DEBUG_LEVEL       (libmej_debug_level)
#define DEBUG_FLAGS       (libmej_debug_flags)

/* A NOP.  Does nothing. */
#define NOP ((void)0)

/* A macro and an #define to FIXME-ize individual calls or entire code blocks. */
#define FIXME_NOP(x)
#define FIXME_BLOCK 0

/* The basic debugging output leader. */
#if defined(__FILE__) && defined(__LINE__)
# ifdef __GNUC__
#  define __DEBUG()  fprintf(LIBMEJ_DEBUG_FD, "[%lu] %12s | %4d: %s(): ", (unsigned long) time(NULL), __FILE__, __LINE__, __FUNCTION__)
# else
#  define __DEBUG()  fprintf(LIBMEJ_DEBUG_FD, "[%lu] %12s | %4d: ", (unsigned long) time(NULL), __FILE__, __LINE__)
# endif
#else
# define __DEBUG()   NOP
#endif

/* A quick and dirty macro to say, "Hi!  I got here without crashing!" */
#define MOO()  do { __DEBUG(); fprintf(LIBMEJ_DEBUG_FD, "Moo.\n"); fflush(LIBMEJ_DEBUG_FD); } while (0)

/* Assertion/abort macros which are quite a bit more useful than assert() and abort(). */
#if defined(__FILE__) && defined(__LINE__)
# ifdef __GNUC__
#  define ASSERT(x)  do {if (!(x)) {if (DEBUG_LEVEL>=1) {fatal_error("ASSERT failed in %s() at %s:%d:  %s", __FUNCTION__, __FILE__, __LINE__, #x);} \
                                                   else {print_warning("ASSERT failed in %s() at %s:%d:  %s", __FUNCTION__, __FILE__, __LINE__, #x);}}} while (0)
#  define ASSERT_RVAL(x, val)  do {if (!(x)) {if (DEBUG_LEVEL>=1) {fatal_error("ASSERT failed in %s() at %s:%d:  %s", __FUNCTION__, __FILE__, __LINE__, #x);} \
                                                             else {print_warning("ASSERT failed in %s() at %s:%d:  %s", __FUNCTION__, __FILE__, __LINE__, #x);} \
                                              return (val);}} while (0)
#  define ASSERT_NOTREACHED()  do {if (DEBUG_LEVEL>=1) {fatal_error("ASSERT failed in %s() at %s:%d:  This code should not be reached.", __FUNCTION__, __FILE__, __LINE__);} \
                                                  else {print_warning("ASSERT failed in %s() at %s:%d:  This code should not be reached.", __FUNCTION__, __FILE__, __LINE__);} \
                                   } while (0)
#  define ASSERT_NOTREACHED_RVAL(val)  do {if (DEBUG_LEVEL>=1) {fatal_error("ASSERT failed in %s() at %s:%d:  This code should not be reached.", __FUNCTION__, __FILE__, __LINE__);} \
                                                          else {print_warning("ASSERT failed in %s() at %s:%d:  This code should not be reached.", __FUNCTION__, __FILE__, __LINE__);} \
                                           return (val);} while (0)
#  define ABORT() fatal_error("Aborting in %s() at %s:%d.", __FUNCTION__, __FILE__, __LINE__)
# else
#  define ASSERT(x)  do {if (!(x)) {if (DEBUG_LEVEL>=1) {fatal_error("ASSERT failed at %s:%d:  %s", __FILE__, __LINE__, #x);} \
                                                   else {print_warning("ASSERT failed at %s:%d:  %s", __FILE__, __LINE__, #x);}}} while (0)
#  define ASSERT_RVAL(x, val)  do {if (!(x)) {if (DEBUG_LEVEL>=1) {fatal_error("ASSERT failed at %s:%d:  %s", __FILE__, __LINE__, #x);} \
                                                             else {print_warning("ASSERT failed at %s:%d:  %s", __FILE__, __LINE__, #x);} \
                                              return (val);}} while (0)
#  define ASSERT_NOTREACHED()  do {if (DEBUG_LEVEL>=1) {fatal_error("ASSERT failed at %s:%d:  This code should not be reached.", __FILE__, __LINE__);} \
                                                  else {print_warning("ASSERT failed at %s:%d:  This code should not be reached.", __FILE__, __LINE__);} \
                                   } while (0)
#  define ASSERT_NOTREACHED_RVAL(val)  do {if (DEBUG_LEVEL>=1) {fatal_error("ASSERT failed at %s:%d:  This code should not be reached.", __FILE__, __LINE__);} \
                                                          else {print_warning("ASSERT failed at %s:%d:  This code should not be reached.", __FILE__, __LINE__);} \
                                           return (val);} while (0)
#  define ABORT() fatal_error("Aborting at %s:%d.", __FILE__, __LINE__)
# endif
#else
# define ASSERT(x)  do {if (!(x)) {if (DEBUG_LEVEL>=1) {fatal_error("ASSERT failed:  %s", #x);} \
                                                  else {print_warning("ASSERT failed:  %s", #x);}}} while (0)
# define ASSERT_RVAL(x, val)  do {if (!(x)) {if (DEBUG_LEVEL>=1) {fatal_error("ASSERT failed:  %s", #x);} \
                                                            else {print_warning("ASSERT failed:  %s", #x);} return (val);}} while (0)
# define ASSERT_NOTREACHED()       return
# define ASSERT_NOTREACHED_RVAL(x) return (x)
# define ABORT() fatal_error("Aborting.\n")
#endif

#define REQUIRE(x) do {if (!(x)) {if (DEBUG_LEVEL>=1) {__DEBUG(); libmej_dprintf("REQUIRE failed:  %s\n", #x);} return;}} while (0)
#define REQUIRE_RVAL(x, v) do {if (!(x)) {if (DEBUG_LEVEL>=1) {__DEBUG(); libmej_dprintf("REQUIRE failed:  %s\n", #x);} return (v);}} while (0)
#define NONULL(x) ((x) ? (x) : ("<null>"))

/* Macros for printing debugging messages */
#if DEBUG >= 1
# ifndef DPRINTF
#  define DPRINTF(x)           do { __DEBUG(); libmej_dprintf x; } while (0)
# endif
# define DPRINTF1(x)           do { if (DEBUG_LEVEL >= 1) {__DEBUG(); libmej_dprintf x;} } while (0)
# define DPRINTF2(x)           do { if (DEBUG_LEVEL >= 2) {__DEBUG(); libmej_dprintf x;} } while (0)
# define DPRINTF3(x)           do { if (DEBUG_LEVEL >= 3) {__DEBUG(); libmej_dprintf x;} } while (0)
# define DPRINTF4(x)           do { if (DEBUG_LEVEL >= 4) {__DEBUG(); libmej_dprintf x;} } while (0)
# define DPRINTF5(x)           do { if (DEBUG_LEVEL >= 5) {__DEBUG(); libmej_dprintf x;} } while (0)
# define DPRINTF6(x)           do { if (DEBUG_LEVEL >= 6) {__DEBUG(); libmej_dprintf x;} } while (0)
# define DPRINTF7(x)           do { if (DEBUG_LEVEL >= 7) {__DEBUG(); libmej_dprintf x;} } while (0)
# define DPRINTF8(x)           do { if (DEBUG_LEVEL >= 8) {__DEBUG(); libmej_dprintf x;} } while (0)
# define DPRINTF9(x)           do { if (DEBUG_LEVEL >= 9) {__DEBUG(); libmej_dprintf x;} } while (0)
#else
# ifndef DPRINTF
#  define DPRINTF(x)           NOP
# endif
# define DPRINTF1(x)           NOP
# define DPRINTF2(x)           NOP
# define DPRINTF3(x)           NOP
# define DPRINTF4(x)           NOP
# define DPRINTF5(x)           NOP
# define DPRINTF6(x)           NOP
# define DPRINTF7(x)           NOP
# define DPRINTF8(x)           NOP
# define DPRINTF9(x)           NOP
#endif

/* Use this for stuff that you only want turned on in dire situations */
#define D_NEVER(x)             NOP

#define DEBUG_MEM              5
#define D_MEM(x)               DPRINTF5(x)
#define DEBUG_STRINGS          9999
#define D_STRINGS(x)           D_NEVER(x)



/********************************* MEM GOOP ***********************************/
typedef struct ptr_struct {
  void *ptr;
  size_t size;
} ptr_t;
typedef struct memrec_struct {
  unsigned char init;
  unsigned long cnt;
  ptr_t *ptrs;
} memrec_t;

#if (DEBUG >= DEBUG_MEM)
# define MALLOC(sz)             libmej_malloc(__FILE__, __LINE__, (sz))
# define CALLOC(type,n)         libmej_calloc(__FILE__, __LINE__, (n), (sizeof(type)))
# define REALLOC(mem,sz)        libmej_realloc(#mem, __FILE__, __LINE__, (mem), (sz))
# define FREE(ptr)              do { libmej_free(#ptr, __FILE__, __LINE__, (ptr)); (ptr) = NULL; } while (0)
# define STRDUP(s)              libmej_strdup(#s, __FILE__, __LINE__, (s))
# define MALLOC_MOD 25
# define REALLOC_MOD 25
# define CALLOC_MOD 25
# define FREE_MOD 25
#else
# define MALLOC(sz)             malloc(sz)
# define CALLOC(type,n)         calloc((n),(sizeof(type)))
# define REALLOC(mem,sz)        ((sz) ? ((mem) ? (realloc((mem), (sz))) : (malloc(sz))) : ((mem) ? (free(mem), NULL) : (NULL)))
# define FREE(ptr)              do { free(ptr); (ptr) = NULL; } while (0)
# define STRDUP(s)              strdup(s)
#endif

/* Fast memset() macro contributed by vendu */
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
    if (((unsigned long) count) >= 4 * sizeof(long)) { \
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



/******************************* STRINGS GOOP *********************************/

#ifdef __GNUC__
# define SWAP(a, b)  __extension__ ({__typeof__(a) tmp = (a); (a) = (b); (b) = tmp;})
#else
# define SWAP(a, b)  do {void *tmp = ((void *)(a)); (a) = (b); (b) = tmp;} while (0)
#endif

#define CONST_STRLEN(x)            (sizeof(x) - 1)
#define BEG_STRCASECMP(s, constr)  (strncasecmp(s, constr, CONST_STRLEN(constr)))



/******************************** PROTOTYPES **********************************/

/* msgs.c */
extern int libmej_dprintf(const char *, ...);
extern void print_error(const char *fmt, ...);
extern void print_warning(const char *fmt, ...);
extern void fatal_error(const char *fmt, ...);

/* debug.c */
extern unsigned int DEBUG_LEVEL;

/* mem.c */
extern void memrec_init(void);
extern void memrec_add_var(void *, size_t);
extern void memrec_rem_var(const char *, const char *, unsigned long, void *);
extern void memrec_chg_var(const char *, const char *, unsigned long, void *, void *, size_t);
extern void memrec_dump(void);
extern void *libmej_malloc(const char *, unsigned long, size_t);
extern void *libmej_realloc(const char *, const char *, unsigned long, void *, size_t);
extern void *libmej_calloc(const char *, unsigned long, size_t, size_t);
extern void libmej_free(const char *, const char *, unsigned long, void *);
extern char *libmej_strdup(const char *, const char *, unsigned long, const char *);
extern void libmej_handle_sigsegv(int);

/* strings.c */
extern char *left_str(const char *, unsigned long);
extern char *mid_str(const char *, unsigned long, unsigned long);
extern char *right_str(const char *, unsigned long);
#ifdef HAVE_REGEX_H
extern unsigned char regexp_match(const char *, const char *);
#endif
extern char *get_word(unsigned long, const char *);
extern char *get_pword(unsigned long, const char *);
extern unsigned long num_words(const char *);
extern char *strip_whitespace(char *);
extern char *downcase_str(char *);
extern char *upcase_str(char *);
#ifndef HAVE_STRCASESTR
extern char *strcasestr(char *, const char *);
#endif
#ifndef HAVE_STRCASECHR
extern char *strcasechr(char *, char);
#endif
#ifndef HAVE_STRCASEPBRK
extern char *strcasepbrk(char *, char *);
#endif
#ifndef HAVE_STRREV
extern char *strrev(char *);
#endif
#if !(HAVE_STRSEP)
extern char *strsep(char **, char *);
#endif
extern char *safe_str(char *, unsigned short);
extern char *garbage_collect(char *, size_t);
extern char *file_garbage_collect(char *, size_t);
extern char *condense_whitespace(char *);
extern void hex_dump(void *, size_t);
#ifndef HAVE_MEMMEM
extern void *memmem(void *, size_t, void *, size_t);
#endif
#ifndef HAVE_USLEEP
extern void usleep(unsigned long);
#endif
#ifndef HAVE_SNPRINTF
extern int vsnprintf(char *str, size_t count, const char *fmt, va_list args);
extern int snprintf(char *str, size_t count, const char *fmt, ...);
#endif

#endif /* _LIBMEJ_H_ */
