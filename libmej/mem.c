
/***************************************************************
 * MEM.C -- Memory allocation handlers                         *
 *       -- Michael Jennings                                   *
 *       -- 07 January 1997                                    *
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

static const char cvs_ident[] = "$Id$";

#include "config.h"
#include "../src/feature.h"

#include "global.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>

#include "debug.h"
#include "mem.h"

/* 
 * These're added for a pretty obvious reason -- they're implemented towards
 * The beginning of each one's respective function. (The ones with capitalized
 * letters. I'm not sure that they'll be useful outside of gdb. Maybe. 
 */
#ifdef MALLOC_CALL_DEBUG
static int malloc_count = 0;
static int calloc_count = 0;
static int realloc_count = 0;
static int free_count = 0;
#endif

MemRec memrec =
{0, 0, 0, (void **) NULL, (size_t *) NULL};

char *
SafeStr(register char *str, unsigned short len)
{
  register unsigned short i;

  for (i = 0; i < len; i++) {
    if (iscntrl(str[i])) {
      str[i] = '.';
    }
  }

  return (str);
}

void
memrec_init(void)
{
  memrec.Count = 0;
  D_MALLOC(("Constructing memrec\n"));
  memrec.Ptrs = (void **) malloc(sizeof(void *));

  memrec.Size = (size_t *) malloc(sizeof(size_t));
  memrec.init = 1;
}

void
memrec_add_var(void *ptr, size_t size)
{
  memrec.Count++;
  if ((memrec.Ptrs = (void **) realloc(memrec.Ptrs, sizeof(void *) * memrec.Count)) == NULL) {
    D_MALLOC(("Unable to reallocate pointer list -- %s\n", strerror(errno)));
  }
  if ((memrec.Size = (size_t *) realloc(memrec.Size, sizeof(size_t) * memrec.Count)) == NULL) {
    D_MALLOC(("Unable to reallocate pointer size list -- %s\n", strerror(errno)));
  }
  D_MALLOC(("Adding variable of size %lu at %8p\n", size, ptr));
  memrec.Ptrs[memrec.Count - 1] = ptr;
  memrec.Size[memrec.Count - 1] = size;
}

void
memrec_rem_var(const char *filename, unsigned long line, void *ptr)
{

  unsigned long i;

  for (i = 0; i < memrec.Count; i++)
    if (memrec.Ptrs[i] == ptr)
      break;
  if (i == memrec.Count && memrec.Ptrs[i] != ptr) {
    D_MALLOC(("ERROR:  File %s, line %d attempted to free a pointer not allocated with Malloc/Realloc:  %8p\n", filename, line, ptr));
    return;
  }
  memrec.Count--;
  D_MALLOC(("Removing variable of size %lu at %8p\n", memrec.Size[i], memrec.Ptrs[i]));
  memmove(memrec.Ptrs + i, memrec.Ptrs + i + 1, sizeof(void *) * (memrec.Count - i));

  memmove(memrec.Size + i, memrec.Size + i + 1, sizeof(size_t) * (memrec.Count - i));
  memrec.Ptrs = (void **) realloc(memrec.Ptrs, sizeof(void *) * memrec.Count);

  memrec.Size = (size_t *) realloc(memrec.Size, sizeof(size_t) * memrec.Count);
}

void
memrec_chg_var(const char *filename, unsigned long line, void *oldp, void *newp, size_t size)
{

  unsigned long i;

  for (i = 0; i < memrec.Count; i++)
    if (memrec.Ptrs[i] == oldp)
      break;
  if (i == memrec.Count && memrec.Ptrs[i] != oldp) {
    D_MALLOC(("ERROR:  File %s, line %d attempted to realloc a pointer not allocated with Malloc/Realloc:  %8p\n", filename, line, oldp));
    return;
  }
  D_MALLOC(("Changing variable of %lu bytes at %8p to one of %lu bytes at %8p\n", memrec.Size[i], memrec.Ptrs[i], size, newp));
  memrec.Ptrs[i] = newp;
  memrec.Size[i] = size;
}

void
memrec_dump(void)
{

  unsigned long i, j, k, l, total = 0;
  unsigned long len1, len2;
  unsigned char *ptr;
  unsigned char buff[9];

  fprintf(stderr, "DUMP :: %lu pointers stored.\n", memrec.Count);
  fprintf(stderr, "DUMP ::  Pointer |  Address |  Size  | Offset  | 00 01 02 03 04 05 06 07 |  ASCII  \n");
  fflush(stderr);
  fprintf(stderr, "DUMP :: ---------+----------+--------+---------+-------------------------+---------\n");
  fflush(stderr);
  len1 = sizeof(void *) * memrec.Count;

  len2 = sizeof(size_t) * memrec.Count;
  for (ptr = (unsigned char *) memrec.Ptrs, j = 0; j < len1; j += 8) {
    fprintf(stderr, "DUMP ::  %07lu | %8p | %06lu | %07x | ", (unsigned long) 0, memrec.Ptrs, (unsigned long) (sizeof(void *) * memrec.Count), (unsigned int) j);

    l = ((len1 - j < 8) ? (len1 - j) : (8));
    memset(buff, 0, 9);
    memcpy(buff, ptr + j, l);
    for (k = 0; k < l; k++) {
      fprintf(stderr, "%02x ", buff[k]);
    }
    for (; k < 8; k++) {
      fprintf(stderr, "   ");
    }
    fprintf(stderr, "| %-8s\n", SafeStr((char *) buff, l));
    fflush(stderr);
  }
  for (ptr = (unsigned char *) memrec.Size, j = 0; j < len2; j += 8) {
    fprintf(stderr, "DUMP ::  %07lu | %8p | %06lu | %07x | ", (unsigned long) 0, memrec.Size, sizeof(size_t) * memrec.Count, (unsigned int) j);
    l = ((len2 - j < 8) ? (len2 - j) : (8));
    memset(buff, 0, 9);
    memcpy(buff, ptr + j, l);
    for (k = 0; k < l; k++) {
      fprintf(stderr, "%02x ", buff[k]);
    }
    for (; k < 8; k++) {
      fprintf(stderr, "   ");
    }
    fprintf(stderr, "| %-8s\n", SafeStr((char *) buff, l));
    fflush(stderr);
  }
  for (i = 0; i < memrec.Count; i++) {
    total += memrec.Size[i];
    for (ptr = (unsigned char *) memrec.Ptrs[i], j = 0; j < memrec.Size[i]; j += 8) {
      fprintf(stderr, "DUMP ::  %07lu | %8p | %06lu | %07x | ", i + 1, memrec.Ptrs[i], (unsigned long) memrec.Size[i], (unsigned int) j);
      l = ((memrec.Size[i] - j < 8) ? (memrec.Size[i] - j) : (8));
      memset(buff, 0, 9);
      memcpy(buff, ptr + j, l);
      for (k = 0; k < l; k++) {
	fprintf(stderr, "%02x ", buff[k]);
      }
      for (; k < 8; k++) {
	fprintf(stderr, "   ");
      }
      fprintf(stderr, "| %-8s\n", SafeStr((char *) buff, l));
      fflush(stderr);
    }
  }
  fprintf(stderr, "DUMP :: Total allocated memory: %10lu bytes\n\n", total);
  fflush(stderr);
}

/************* Function prototypes ****************/

void *
Malloc(const char *filename, unsigned long line, size_t size)
{

  void *temp;

#ifdef MALLOC_CALL_DEBUG
  ++malloc_count;
  if (!(malloc_count % MALLOC_MOD)) {
    fprintf(stderr, "Calls to malloc(): %d\n", malloc_count);
  }
#endif

  D_MALLOC(("Malloc(%lu) called at %s:%lu\n", size, filename, line));
  if (!memrec.init) {
    D_MALLOC(("WARNING:  You must call memrec_init() before using the libmej memory management calls.\n"));
    memrec_init();
  }

  temp = (void *) malloc(size);
  if (!temp)
    return NULL;
  if (debug_level >= DEBUG_MALLOC) {
    memrec_add_var(temp, size);
  }
  return (temp);
}

void *
Realloc(const char *var, const char *filename, unsigned long line, void *ptr, size_t size)
{

  void *temp;

#ifdef MALLOC_CALL_DEBUG
  ++realloc_count;
  if (!(realloc_count % REALLOC_MOD)) {
    fprintf(stderr, "Calls to realloc(): %d\n", realloc_count);
  }
#endif

  if (!memrec.init) {
    D_MALLOC(("WARNING:  You must call memrec_init() before using the libmej memory management calls.\n"));
    memrec_init();
  }

  D_MALLOC(("Realloc(%lu) called for variable %s (%8p) at %s:%lu\n", size, var, ptr, filename, line));
  if (ptr == NULL) {
    temp = (void *) Malloc(__FILE__, __LINE__, size);
  } else {
    temp = (void *) realloc(ptr, size);
    if (debug_level >= DEBUG_MALLOC) {
      memrec_chg_var(filename, line, ptr, temp, size);
    }
  }
  return (temp);
}

void *
Calloc(const char *filename, unsigned long line, size_t count, size_t size)
{

  char *temp;

#ifdef MALLOC_CALL_DEBUG
  ++calloc_count;
  if (!(calloc_count % CALLOC_MOD)) {
    fprintf(stderr, "Calls to calloc(): %d\n", calloc_count);
  }
#endif

  if (!memrec.init) {
    D_MALLOC(("WARNING:  You must call memrec_init() before using the libmej memory management calls.\n"));
    memrec_init();
  }

  D_MALLOC(("Calloc(%lu, %lu) called at %s:%lu\n", count, size, filename, line));
  temp = (char *) calloc(count, size);
  if (debug_level >= DEBUG_MALLOC) {
    memrec_add_var(temp, size * count);
  }
  return (temp);
}

void
Free(const char *var, const char *filename, unsigned long line, void *ptr)
{

#ifdef MALLOC_CALL_DEBUG
  ++free_count;
  if (!(free_count % FREE_MOD)) {
    fprintf(stderr, "Calls to free(): %d\n", free_count);
  }
#endif

  if (!memrec.init) {
    D_MALLOC(("WARNING:  You must call memrec_init() before using the libmej memory management calls.\n"));
    memrec_init();
  }

  D_MALLOC(("Free() called for variable %s (%8p) at %s:%lu\n", var, ptr, filename, line));
  if (ptr) {
    if (debug_level >= DEBUG_MALLOC) {
      memrec_rem_var(filename, line, ptr);
    }
    free(ptr);
  } else {
    D_MALLOC(("ERROR:  Caught attempt to free NULL pointer\n"));
  }
}

void
HandleSigSegv(int sig)
{

#if DEBUG >= DEBUG_MALLOC
  fprintf(stderr, "Fatal memory fault (%d)!  Dumping memory table.\n", sig);
  memrec_dump();
#endif
  exit(EXIT_FAILURE);
}
