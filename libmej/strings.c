/***************************************************************
 * STRINGS.C -- String manipulation routines                   *
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

static const char cvs_ident[] = "$Id$";

#include "config.h"
#include "../src/feature.h"

#include "global.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#ifndef WITH_DMALLOC
# include <malloc.h>
#endif
#include <ctype.h>
#include "debug.h"
#include "mem.h"
#include "strings.h"

#ifndef HAVE_MEMMEM
/* Find first occurance of bytestring needle of size needlelen in memory region
   haystack of size haystacklen */
void *
memmem(void *haystack, register size_t haystacklen, void *needle, register size_t needlelen)
{

  register char *hs = (char *) haystack;
  register char *n = (char *) needle;
  register unsigned long i;
  register size_t len = haystacklen - needlelen;

  for (i = 0; i < len; i++) {
    if (!memcmp(hs + i, n, needlelen)) {
      return (hs + i);
    }
  }
  return (NULL);
}
#endif

#ifndef HAVE_USLEEP
void
usleep(unsigned long usec)
{

  struct timeval delay;

  delay.tv_sec = 0;
  delay.tv_usec = usec;
  select(0, NULL, NULL, NULL, &delay);

}

#endif

/***** Not needed ******
#ifndef HAVE_NANOSLEEP
inline void
nanosleep(unsigned long nsec) {
    usleep(nsec / 1000);
}
#endif
************************/

/* Return the leftmost cnt characters of str */
char *
LeftStr(const char *str, unsigned long cnt)
{

  char *tmpstr;

  tmpstr = (char *) MALLOC(cnt + 1);
  strncpy(tmpstr, str, cnt);
  tmpstr[cnt] = 0;
  return (tmpstr);
}

/* Return cnt characters from str, starting at position index (from 0) */
char *
MidStr(const char *str, unsigned long index, unsigned long cnt)
{

  char *tmpstr;
  const char *pstr = str;

  tmpstr = (char *) MALLOC(cnt + 1);
  pstr += index;
  strncpy(tmpstr, pstr, cnt);
  tmpstr[cnt] = 0;
  return (tmpstr);
}

/* Return the rightmost characters of str */
char *
RightStr(const char *str, unsigned long cnt)
{

  char *tmpstr;
  const char *pstr = str;

  tmpstr = (char *) MALLOC(cnt + 1);
  pstr += strlen(str);
  pstr -= cnt;
  strcpy(tmpstr, pstr);
  return (tmpstr);
}

/* Returns TRUE if str matches regular expression pattern, FALSE otherwise */
#if defined(HAVE_REGEX_H) || defined(IRIX)
unsigned char
Match(register const char *str, register const char *pattern)
{

  register regex_t *rexp;
  register int result;

#ifndef IRIX
  char errbuf[256];

  rexp = (regex_t *) MALLOC(sizeof(regex_t));
#endif

#ifdef IRIX
  if ((rexp = compile((const char *) pattern, (char *) NULL, (char *) NULL)) == NULL) {
    fprintf(stderr, "Unable to compile regexp %s\n", pattern);
    return (FALSE);
  }
#else
  if ((result = regcomp(rexp, pattern, REG_EXTENDED)) != 0) {
    regerror(result, rexp, errbuf, 256);
    fprintf(stderr, "Unable to compile regexp %s -- %s.\n", pattern, errbuf);
    FREE(rexp);
    return (FALSE);
  }
#endif

#ifdef IRIX
  result = step((const char *) str, rexp);
  FREE(rexp);
  return (result);
#else
  if (((result = regexec(rexp, str, (size_t) 0, (regmatch_t *) NULL, 0))
       != 0) && (result != REG_NOMATCH)) {
    regerror(result, rexp, errbuf, 256);
    fprintf(stderr, "Error testing input string %s -- %s.\n", str, errbuf);
    FREE(rexp);
    return (FALSE);
  }
  FREE(rexp);
  return (!result);
#endif
}
#endif

/* Return malloc'd pointer to index-th word in str.  "..." counts as 1 word. */
char *
Word(unsigned long index, const char *str)
{

  char *tmpstr;
  char *delim = DEFAULT_DELIM;
  register unsigned long i, j, k;

  k = strlen(str) + 1;
  if ((tmpstr = (char *) MALLOC(k)) == NULL) {
    fprintf(stderr, "Word(%lu, %s):  Unable to allocate memory -- %s.\n",
	    index, str, strerror(errno));
    return ((char *) NULL);
  }
  *tmpstr = 0;
  for (i = 0, j = 0; j < index && str[i]; j++) {
    for (; isspace(str[i]); i++);
    switch (str[i]) {
      case '\"':
	delim = "\"";
	i++;
	break;
      case '\'':
	delim = "\'";
	i++;
	break;
      default:
	delim = DEFAULT_DELIM;
    }
    for (k = 0; str[i] && !strchr(delim, str[i]);) {
      if (str[i] == '\\') {
	if (str[i + 1] == '\'' || str[i + 1] == '\"') {
	  i++;
	}
      }
      tmpstr[k++] = str[i++];
    }
    switch (str[i]) {
      case '\"':
      case '\'':
	i++;
	break;
    }
    tmpstr[k] = 0;
  }

  if (j != index) {
    FREE(tmpstr);
    D_STRINGS(("Word(%lu, %s) returning NULL.\n", index, str));
    return ((char *) NULL);
  } else {
    tmpstr = (char *) REALLOC(tmpstr, strlen(tmpstr) + 1);
    D_STRINGS(("Word(%lu, %s) returning \"%s\".\n", index, str, tmpstr));
    return (tmpstr);
  }
}

/* Return pointer into str to index-th word in str.  "..." counts as 1 word. */
char *
PWord(unsigned long index, char *str)
{

  register char *tmpstr = str;
  register unsigned long j;

  if (!str)
    return ((char *) NULL);
  for (; isspace(*tmpstr) && *tmpstr; tmpstr++);
  for (j = 1; j < index && *tmpstr; j++) {
    for (; !isspace(*tmpstr) && *tmpstr; tmpstr++);
    for (; isspace(*tmpstr) && *tmpstr; tmpstr++);
  }

  if (*tmpstr == '\"' || *tmpstr == '\'') {
    tmpstr++;
  }
  if (*tmpstr == '\0') {
    D_STRINGS(("PWord(%lu, %s) returning NULL.\n", index, str));
    return ((char *) NULL);
  } else {
    D_STRINGS(("PWord(%lu, %s) returning \"%s\"\n", index, str, tmpstr));
    return tmpstr;
  }
}

/* Returns the number of words in str, for use with Word() and PWord().  "..." counts as 1 word. */
unsigned long
NumWords(const char *str)
{

  register unsigned long cnt = 0;
  char *delim = DEFAULT_DELIM;
  register unsigned long i;

  for (i = 0; str[i] && strchr(delim, str[i]); i++);
  for (; str[i]; cnt++) {
    switch (str[i]) {
      case '\"':
	delim = "\"";
	i++;
	break;
      case '\'':
	delim = "\'";
	i++;
	break;
      default:
	delim = DEFAULT_DELIM;
    }
    for (; str[i] && !strchr(delim, str[i]); i++);
    switch (str[i]) {
      case '\"':
      case '\'':
	i++;
	break;
    }
    for (; str[i] && isspace(str[i]); i++);
  }

  D_STRINGS(("NumWords() returning %lu\n", cnt));
  return (cnt);
}

char *
StripWhitespace(register char *str)
{

  register unsigned long i, j;

  if ((j = strlen(str))) {
    for (i = j - 1; isspace(*(str + i)); i--);
    str[j = i + 1] = 0;
    for (i = 0; isspace(*(str + i)); i++);
    j -= i;
    memmove(str, str + i, j + 1);
  }
  return (str);
}

char *
LowerStr(char *str)
{

  register char *tmp;

  for (tmp = str; *tmp; tmp++)
    *tmp = tolower(*tmp);
  D_STRINGS(("LowerStr() returning %s\n", str));
  return (str);
}

char *
UpStr(char *str)
{

  register char *tmp;

  for (tmp = str; *tmp; tmp++)
    *tmp = toupper(*tmp);
  D_STRINGS(("UpStr() returning %s\n", str));
  return (str);
}

char *
StrCaseStr(char *haystack, register const char *needle)
{

  register char *t;
  register size_t len = strlen(needle);

  for (t = haystack; t && *t; t++) {
    if (!strncasecmp(t, needle, len)) {
      return (t);
    }
  }
  return (NULL);
}

char *
StrCaseChr(char *haystack, register char needle)
{

  register char *t;

  for (t = haystack; t && *t; t++) {
    if (tolower(*t) == tolower(needle)) {
      return (t);
    }
  }
  return (NULL);
}

char *
StrCasePBrk(char *haystack, register char *needle)
{

  register char *t;

  for (t = haystack; t && *t; t++) {
    if (StrCaseChr(needle, *t)) {
      return (t);
    }
  }
  return (NULL);
}

char *
StrRev(register char *str)
{

  register int i, j;

  i = strlen(str);
  for (j = 0, i--; i > j; i--, j++) {
    cswap(str[j], str[i]);
  }
  return (str);

}

char *
StrDup(register const char *str)
{

  register char *newstr;
  register unsigned long len;

  len = strlen(str) + 1;  /* Copy NUL byte also */
  newstr = (char *) MALLOC(len);
  strcpy(newstr, str);
  return (newstr);
}

#if !(HAVE_STRSEP)
char *
strsep(char **str, register char *sep)
{

  register char *s = *str;
  char *sptr;

  D_STRINGS(("strsep(%s, %s) called.\n", *str, sep));
  sptr = s;
  for (; *s && !strchr(sep, *s); s++);
  if (!*s) {
    if (s != sptr) {
      *str = s;
      D_STRINGS(("Reached end of string with token \"%s\" in buffer\n", sptr));
      return (sptr);
    } else {
      D_STRINGS(("Reached end of string\n"));
      return ((char *) NULL);
    }
  }
  *s = 0;
  *str = s + 1;
  D_STRINGS(("Got token \"%s\", *str == \"%s\"\n", sptr, *str));
  return (sptr);
}
#endif

char *
GarbageCollect(char *buff, size_t len)
{

  register char *tbuff = buff, *pbuff = buff;
  register unsigned long i, j;

  D_STRINGS(("Garbage collecting on %lu bytes at %10.8p\n", len, buff));
  for (i = 0, j = 0; j < len; j++)
    if (pbuff[j])
      tbuff[i++] = pbuff[j];
  tbuff[i++] = '\0';
  D_STRINGS(("Garbage collecting gives: \n%s\n", buff));
  return ((char *) REALLOC(buff, sizeof(char) * i));
}

char *
FGarbageCollect(char *buff, size_t len)
{

  register char *tbuff = buff, *pbuff = buff;
  char *tmp1, *tmp2;
  register unsigned long j;

  D_STRINGS(("File garbage collecting on %lu bytes at %10.8p\n", len, buff));
  for (j = 0; j < len;) {
    switch (pbuff[j]) {
      case '#':
	for (; !strchr("\r\n", pbuff[j]) && j < len; j++)
	  pbuff[j] = '\0';	/* First null out the line up to the CR and/or LF */
	for (; strchr("\r\n", pbuff[j]) && j < len; j++)
	  pbuff[j] = '\0';	/* Then null out the CR and/or LF */
	break;
      case '\r':
      case '\n':
      case '\f':
      case ' ':
      case '\t':
      case '\v':
	for (; isspace(pbuff[j]) && j < len; j++)
	  pbuff[j] = '\0';	/* Null out the whitespace */
	break;
      default:
	/* Find the end of this line and the occurence of the
	   next mid-line comment. */
	tmp1 = strpbrk(pbuff + j, "\r\n");
	tmp2 = strstr(pbuff + j, " #");

	/* If either is null, take the non-null one.  Otherwise,
	   take the lesser of the two. */
	if (!tmp1 || !tmp2) {
	  tbuff = ((tmp1) ? (tmp1) : (tmp2));
	} else {
	  tbuff = ((tmp1 < tmp2) ? (tmp1) : (tmp2));
	}

	/* Now let j catch up so that pbuff+j = tbuff; i.e., let
	   pbuff[j] refer to the same character that tbuff does */
	j += tbuff - (pbuff + j);

	/* Finally, change whatever is at pbuff[j] to a newline.
	   This will accomplish several things at once:
	   o It will change a \r to a \n if that's what's there
	   o If it's a \n, it'll stay the same.  No biggie.
	   o If it's a space, it will end the line there and the
	   next line will begin with a comment, which is handled
	   above. */
	if (j < len)
	  pbuff[j++] = '\n';

    }
  }

  /* Change all occurances of a backslash followed by a newline to nulls
     and null out all whitespace up to the next non-whitespace character.
     This handles support for breaking a string across multiple lines. */
  for (j = 0; j < len; j++) {
    if (pbuff[j] == '\\' && pbuff[j + 1] == '\n') {
      pbuff[j++] = '\0';
      for (; isspace(pbuff[j]) && j < len; j++)
	pbuff[j] = '\0';	/* Null out the whitespace */
    }
  }

  /* And the final step, garbage collect the buffer to condense all
     those nulls we just put in. */
  return (GarbageCollect(buff, len));
}

char *
CondenseWhitespace(char *s)
{

  register unsigned char gotspc = 0;
  register char *pbuff = s, *pbuff2 = s;

  D_STRINGS(("CondenseWhitespace(%s) called.\n", s));
  for (; *pbuff2; pbuff2++) {
    if (isspace(*pbuff2)) {
      if (!gotspc) {
	*pbuff = ' ';
	gotspc = 1;
	pbuff++;
      }
    } else {
      *pbuff = *pbuff2;
      gotspc = 0;
      pbuff++;
    }
  }
  if ((pbuff >= s) && (isspace(*(pbuff - 1))))
    pbuff--;
  *pbuff = 0;
  D_STRINGS(("CondenseWhitespace() returning \"%s\".\n", s));
  return (REALLOC(s, strlen(s) + 1));
}

void
HexDump(void *buff, register size_t count)
{

  register unsigned long j, k, l;
  register unsigned char *ptr;
  unsigned char buffr[9];

  fprintf(stderr, " Address |  Size  | Offset  | 00 01 02 03 04 05 06 07 |  ASCII  \n");
  fprintf(stderr, "---------+--------+---------+-------------------------+---------\n");
  for (ptr = (unsigned char *) buff, j = 0; j < count; j += 8) {
    fprintf(stderr, " %08x | %06lu | %07X | ", (unsigned int) buff,
	    (unsigned long) count, (unsigned int) j);
    l = ((count - j < 8) ? (count - j) : (8));
    memset(buffr, 0, 9);
    memcpy(buffr, ptr + j, l);
    for (k = 0; k < l; k++) {
      fprintf(stderr, "%02.2X ", buffr[k]);
    }
    for (; k < 8; k++) {
      fprintf(stderr, "   ");
    }
    fprintf(stderr, "| %-8s\n", SafeStr((char *) buffr, l));
  }
}
