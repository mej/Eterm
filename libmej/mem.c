
/***************************************************************
 * MEM.C -- Memory allocation handlers                         *
 *       -- Michael Jennings                                   *
 *       -- 07 January 1997                                    *
 ***************************************************************/
/*
 * This file is original work by Michael Jennings <mej@tcserv.com>.
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

#include "global.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#define MEM_C
#include "debug.h"
#include "mem.h"

/* 
 * These're added for a pretty obvious reason -- they're implemented towards
 * The beginning of each one's respective function. (The ones with capitalized
 * letters. I'm not sure that they'll be useful outside of gdb. Maybe. 
 */
#if DEBUG >= DEBUG_MALLOC
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
  D_MALLOC(("Adding variable of size %lu at 0x%08x\n", size, ptr));
  memrec.Ptrs[memrec.Count - 1] = ptr;
  memrec.Size[memrec.Count - 1] = size;
#if 0
  memrec_dump();
#endif
}

void
memrec_rem_var(void *ptr)
{

  unsigned long i;

  for (i = 0; i < memrec.Count; i++)
    if (memrec.Ptrs[i] == ptr)
      break;
  if (i == memrec.Count && memrec.Ptrs[i] != ptr) {
#if 0
    memrec_dump();
#endif
    D_MALLOC(("Attempt to remove a pointer not allocated with Malloc/Realloc:"
	     "  0x%08x\n", ptr));
    return;
  }
  memrec.Count--;
  D_MALLOC(("Removing variable of size %lu at 0x%08x\n", memrec.Size[i], memrec.Ptrs[i]));
  memmove(memrec.Ptrs + i, memrec.Ptrs + i + 1, sizeof(void *) * (memrec.Count - i));

  memmove(memrec.Size + i, memrec.Size + i + 1, sizeof(size_t) * (memrec.Count - i));
  memrec.Ptrs = (void **) realloc(memrec.Ptrs, sizeof(void *) * memrec.Count);

  memrec.Size = (size_t *) realloc(memrec.Size, sizeof(size_t) * memrec.Count);
#if 0
  memrec_dump();
#endif
}

void
memrec_chg_var(void *oldp, void *newp, size_t size)
{

  unsigned long i;

  for (i = 0; i < memrec.Count; i++)
    if (memrec.Ptrs[i] == oldp)
      break;
  if (i == memrec.Count && memrec.Ptrs[i] != oldp) {
#if 0
    memrec_dump();
#endif
    D_MALLOC(("Attempt to move a pointer not allocated with Malloc/Realloc:"
	     "  0x%08x\n", oldp));
    return;
  }
  D_MALLOC(("Changing variable of %lu bytes at 0x%08x to one "
	   "of %lu bytes at 0x%08x\n", memrec.Size[i], memrec.Ptrs[i], size, newp));
  memrec.Ptrs[i] = newp;
  memrec.Size[i] = size;
#if 0
  memrec_dump();
#endif
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
    fprintf(stderr, "DUMP ::  %07lu | %08X | %06lu | %07X | ", (unsigned long) 0, (unsigned int) memrec.Ptrs, (unsigned long) (sizeof(void *) * memrec.Count), (unsigned int) j);

    l = ((len1 - j < 8) ? (len1 - j) : (8));
    memset(buff, 0, 9);
    memcpy(buff, ptr + j, l);
    for (k = 0; k < l; k++) {
      fprintf(stderr, "%02.2X ", buff[k]);
    }
    for (; k < 8; k++) {
      fprintf(stderr, "   ");
    }
    fprintf(stderr, "| %-8s\n", SafeStr((char *) buff, l));
    fflush(stderr);
  }
  for (ptr = (unsigned char *) memrec.Size, j = 0; j < len2; j += 8) {
    fprintf(stderr, "DUMP ::  %07lu | %08x | %06lu | %07X | ", (unsigned long) 0, (unsigned int) memrec.Size, sizeof(size_t) * memrec.Count, (unsigned int) j);
    l = ((len2 - j < 8) ? (len2 - j) : (8));
    memset(buff, 0, 9);
    memcpy(buff, ptr + j, l);
    for (k = 0; k < l; k++) {
      fprintf(stderr, "%02.2X ", buff[k]);
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
      fprintf(stderr, "DUMP ::  %07lu | %08x | %06lu | %07X | ", i + 1, (unsigned int) memrec.Ptrs[i], (unsigned long) memrec.Size[i], (unsigned int) j);
      l = ((memrec.Size[i] - j < 8) ? (memrec.Size[i] - j) : (8));
      memset(buff, 0, 9);
      memcpy(buff, ptr + j, l);
      for (k = 0; k < l; k++) {
	fprintf(stderr, "%02.2X ", buff[k]);
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

/* Replacements for malloc(), realloc(), calloc(), and free() */
void *Malloc(size_t);
void *Realloc(void *, size_t);
void *Calloc(size_t, size_t);
void Free(void *);

/* A handler for SIGSEGV */
void HandleSigSegv(int);

void *
Malloc(size_t size)
{

#if 0
  char *temp = NULL;

#endif
  void *temp = NULL;

#if DEBUG >= DEBUG_MALLOC
  ++malloc_count;
#ifdef MALLOC_CALL_DEBUG
  if (!(malloc_count % MALLOC_MOD)) {
    fprintf(stderr, "Calls to malloc(): %d\n", malloc_count);
  }
#endif
#endif

#if DEBUG >= DEBUG_MALLOC

  if (!memrec.init) {
    D_MALLOC(("WARNING:  You must call memrec_init() before using the libmej memory management calls.\n"));
    memrec_init();
  }
#endif

#if 0
  temp = (char *) malloc(size);
#endif
  temp = malloc(size);
  if (!temp)
    return NULL;
#if DEBUG >= DEBUG_MALLOC
  memrec_add_var(temp, size);
#endif
  return (temp);
}

void *
Realloc(void *ptr, size_t size)
{

  char *temp = NULL;

#if DEBUG >= DEBUG_MALLOC
  ++realloc_count;
# ifdef MALLOC_CALL_DEBUG
  if (!(realloc_count % REALLOC_MOD)) {
    fprintf(stderr, "Calls to realloc(): %d\n", realloc_count);
  }
# endif
#endif
/* 
 * Redundant. You know the drill :)
 * #if DEBUG >= DEBUG_MALLOC
 */
  if (!memrec.init) {
    D_MALLOC(("WARNING:  You must call memrec_init() before using the libmej memory management calls.\n"));
    memrec_init();
  }
/* #endif */

  if (ptr == NULL) {
    temp = (char *) Malloc(size);
  } else {
    temp = (char *) realloc(ptr, size);
#if DEBUG >= DEBUG_MALLOC
    memrec_chg_var(ptr, temp, size);
#endif
  }
  if (!temp)
    return NULL;
  return (temp);
}

void *
Calloc(size_t count, size_t size)
{

  char *temp = NULL;

#if DEBUG >= DEBUG_MALLOC
  ++calloc_count;
#ifdef MALLOC_CALL_DEBUG
  if (!(calloc_count % CALLOC_MOD)) {
    fprintf(stderr, "Calls to calloc(): %d\n", calloc_count);
  }
#endif
#endif

#if DEBUG >= DEBUG_MALLOC

  if (!memrec.init) {
    D_MALLOC(("WARNING:  You must call memrec_init() before using the libmej memory management calls.\n"));
    memrec_init();
  }
#endif

  temp = (char *) calloc(count, size);
#if DEBUG >= DEBUG_MALLOC
  memrec_add_var(temp, size * count);
#endif
  if (!temp)
    return NULL;
  return (temp);
}

void
Free(void *ptr)
{

#if DEBUG >= DEBUG_MALLOC
  ++free_count;
#ifdef MALLOC_CALL_DEBUG
  if (!(free_count % FREE_MOD)) {
    fprintf(stderr, "Calls to free(): %d\n", free_count);
  }
#endif
#endif
#if DEBUG >= DEBUG_MALLOC
  if (!memrec.init) {
    D_MALLOC(("WARNING:  You must call memrec_init() before using the libmej memory management calls.\n"));
    memrec_init();
  }
#endif

  if (ptr) {
#if DEBUG >= DEBUG_MALLOC
    memrec_rem_var(ptr);
#endif
    free(ptr);
  } else {
    D_MALLOC(("Caught attempt to free NULL pointer\n"));
  }
}

void
HandleSigSegv(int sig)
{

  static unsigned char segv_again = 0;

  /* Reinstate ourselves as the SIGSEGV handler if we're replaced */
  (void) signal(SIGSEGV, HandleSigSegv);

  /* Recursive seg faults are not cool.... */
  if (segv_again) {
    printf("RECURSIVE SEGMENTATION FAULT DETECTED!\n");
    _exit(EXIT_FAILURE);
  }
  segv_again = 1;
#if DEBUG >= DEBUG_MALLOC
  fprintf(stderr, "SEGMENTATION FAULT!  DUMPING MEMORY TABLE\n");
  memrec_dump();
#endif
  exit(EXIT_FAILURE);
}

inline void *
fixed_realloc(void *ptr, size_t size)
{

  if (ptr)
    return (realloc(ptr, size));
  else
    return (malloc(size));

}
