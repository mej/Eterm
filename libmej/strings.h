/***************************************************************
 * STRINGS.H -- String manipulation routines                   *
 *           -- Michael Jennings                               *
 *           -- 08 January 1997                                *
 ***************************************************************/
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

#ifndef _STRINGS_H_

#define _STRINGS_H_

#include "global.h"
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>

#ifndef TRUE
#  define TRUE    ((unsigned char)(1))
#  define FALSE   ((unsigned char)(0))
#endif

#ifndef swap
#  define swap(a, b) (((int)(b)) ^= ((int)(a)) ^= ((int)(b)) ^= ((int)(a)))
#endif

#ifndef cswap
#  define cswap(a, b) ((b) ^= (a) ^= (b) ^= (a))
#endif

#define DEFAULT_DELIM " \r\n\f\t\v"

#define CONST_STRLEN(x)            (sizeof(x) - 1)
#define BEG_STRCASECMP(s, constr)  (strncasecmp(s, constr, CONST_STRLEN(constr)))

#ifdef IRIX
#  define regex_t char
#  define NBRA 9
   extern char     *braslist[NBRA];
   extern char     *braelist[NBRA];
   extern int nbra, regerrno, reglength;
   extern char *loc1, *loc2, *locs;

   extern "C" int step(const char *, const char *); 
   extern "C" int advance(const char *, char *);
   extern "C" char *compile(const char *, char *, char *);
#elif defined(HAVE_REGEX_H)
#  include <regex.h>
#endif

extern char *LeftStr(const char *, unsigned long);
extern char *MidStr(const char *, unsigned long, unsigned long);
extern char *RightStr(const char *, unsigned long);
#if defined(HAVE_REGEX_H) || defined(IRIX)
extern unsigned char Match(const char *, const char *);
#endif
extern char *Word(unsigned long, const char *);
extern char *PWord(unsigned long, char *);
extern unsigned long NumWords(const char *);
extern char *StripWhitespace(char *);
extern char *LowerStr(char *);
extern char *UpStr(char *);
extern char *StrCaseStr(char *, const char *);
extern char *StrCaseChr(char *, char);
extern char *StrCasePBrk(char *, char *);
extern char *StrRev(char *);
extern char *StrDup(const char *);
#if !(HAVE_STRSEP)
extern char *strsep(char **, char *);
#endif
extern char *SafeStr(char *, unsigned short);
extern char *GarbageCollect(char *, size_t);
extern char *FGarbageCollect(char *, size_t);
extern char *CondenseWhitespace(char *);
extern void HexDump(void *, size_t);
#ifndef HAVE_MEMMEM
extern void *memmem(void *, size_t, void *, size_t);
#endif
#ifndef HAVE_USLEEP
extern void usleep(unsigned long);
#endif
#ifndef HAVE_SNPRINTF
# ifdef HAVE_STDARG_H
#  include <stdarg.h>
# endif
extern int vsnprintf(char *str, size_t count, const char *fmt, va_list args);
extern int snprintf(char *str, size_t count, const char *fmt, ...);
#endif
/*
#ifndef HAVE_NANOSLEEP
extern void nanosleep(unsigned long);
#endif
*/

#endif /* _STRINGS_H_ */
