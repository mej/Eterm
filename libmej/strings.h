/***************************************************************
 * STRINGS.H -- String manipulation routines                   *
 *           -- Michael Jennings                               *
 *           -- 08 January 1997                                *
 ***************************************************************/
/*
 * This file is original work by Michael Jennings <mej@eterm.org>.
 *
 * Copyright (C) 1997, Michael Jennings
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
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
/*
#ifndef HAVE_NANOSLEEP
extern void nanosleep(unsigned long);
#endif
*/

#endif /* _STRINGS_H_ */
