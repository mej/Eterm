
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "libmej.h"

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

static memrec_t memrec;

void
memrec_init(void)
{
  D_MEM(("Constructing memrec\n"));
  memrec.ptrs = (ptr_t *) malloc(sizeof(ptr_t));
  memrec.init = 1;
}

void
memrec_add_var(void *ptr, size_t size)
{
  register ptr_t *p;

  memrec.cnt++;
  if ((memrec.ptrs = (ptr_t *) realloc(memrec.ptrs, sizeof(ptr_t) * memrec.cnt)) == NULL) {
    D_MEM(("Unable to reallocate pointer list -- %s\n", strerror(errno)));
  }
  D_MEM(("Adding variable of size %lu at %8p\n", size, ptr));
  p = memrec.ptrs + memrec.cnt - 1;
  p->ptr = ptr;
  p->size = size;
}

void
memrec_rem_var(const char *var, const char *filename, unsigned long line, void *ptr)
{
  register ptr_t *p = NULL;
  register unsigned long i;

  for (i = 0; i < memrec.cnt; i++) {
    if (memrec.ptrs[i].ptr == ptr) {
      p = memrec.ptrs + i;
      break;
    }
  }
  if (!p) {
    D_MEM(("ERROR:  File %s, line %d attempted to free variable %s (%8p) which was not allocated with MALLOC/REALLOC\n", filename, line, var, ptr));
    return;
  }
  memrec.cnt--;
  D_MEM(("Removing variable of size %lu at %8p\n", p->size, p->ptr));
  memmove(p, p + 1, sizeof(ptr_t) * (memrec.cnt - i));
  memrec.ptrs = (ptr_t *) realloc(memrec.ptrs, sizeof(ptr_t) * memrec.cnt);
}

void
memrec_chg_var(const char *var, const char *filename, unsigned long line, void *oldp, void *newp, size_t size)
{
  register ptr_t *p = NULL;
  register unsigned long i;

  for (i = 0; i < memrec.cnt; i++) {
    if (memrec.ptrs[i].ptr == oldp) {
      p = memrec.ptrs + i;
      break;
    }
  }
  if (i == memrec.cnt) {
    D_MEM(("ERROR:  File %s, line %d attempted to realloc variable %s (%8p) which was not allocated with MALLOC/REALLOC\n", filename, line, var, oldp));
    return;
  }
  D_MEM(("Changing variable of %lu bytes at %8p to one of %lu bytes at %8p\n", p->size, p->ptr, size, newp));
  p->ptr = newp;
  p->size = size;
}

void
memrec_dump(void)
{
  register ptr_t *p;
  unsigned long i, j, k, l, total = 0;
  unsigned long len;
  unsigned char buff[9];

  fprintf(LIBMEJ_DEBUG_FD, "DUMP :: %lu pointers stored.\n", memrec.cnt);
  fprintf(LIBMEJ_DEBUG_FD, "DUMP ::  Pointer |  Address |  Size  | Offset  | 00 01 02 03 04 05 06 07 |  ASCII  \n");
  fprintf(LIBMEJ_DEBUG_FD, "DUMP :: ---------+----------+--------+---------+-------------------------+---------\n");
  fflush(LIBMEJ_DEBUG_FD);
  len = sizeof(ptr_t) * memrec.cnt;
  memset(buff, 0, sizeof(buff));

  /* First, dump the contents of the memrec.ptrs[] array. */
  for (p = memrec.ptrs, j = 0; j < len; p++, j += 8) {
    fprintf(LIBMEJ_DEBUG_FD, "DUMP ::  %07lu | %8p | %06lu | %07x | ", (unsigned long) 0, memrec.ptrs, (unsigned long) (sizeof(ptr_t) * memrec.cnt), (unsigned int) j);
    /* l is the number of characters we're going to output */
    l = ((len - j < 8) ? (len - j) : (8));
    /* Copy l bytes (up to 8) from memrec.ptrs[] (p) to buffer */
    memcpy(buff, p + j, l);
    for (k = 0; k < l; k++) {
      fprintf(LIBMEJ_DEBUG_FD, "%02x ", buff[k]);
    }
    /* If we printed less than 8 bytes worth, pad with 3 spaces per byte */
    for (; k < 8; k++) {
      fprintf(LIBMEJ_DEBUG_FD, "   ");
    }
    /* Finally, print the printable ASCII string for those l bytes */
    fprintf(LIBMEJ_DEBUG_FD, "| %-8s\n", safe_str((char *) buff, l));
    /* Flush after every line in case we crash */
    fflush(LIBMEJ_DEBUG_FD);
  }

  /* Now print out each pointer and its contents. */
  for (p = memrec.ptrs, i = 0; i < memrec.cnt; p++, i++) {
    /* Add this pointer's size to our total */
    total += p->size;
    for (j = 0; j < p->size; j += 8) {
      fprintf(LIBMEJ_DEBUG_FD, "DUMP ::  %07lu | %8p | %06lu | %07x | ", i + 1, p->ptr, (unsigned long) p->size, (unsigned int) j);
      /* l is the number of characters we're going to output */
      l = ((p->size - j < 8) ? (p->size - j) : (8));
      /* Copy l bytes (up to 8) from p->ptr to buffer */
      memcpy(buff, p->ptr + j, l);
      for (k = 0; k < l; k++) {
	fprintf(LIBMEJ_DEBUG_FD, "%02x ", buff[k]);
      }
      /* If we printed less than 8 bytes worth, pad with 3 spaces per byte */
      for (; k < 8; k++) {
	fprintf(LIBMEJ_DEBUG_FD, "   ");
      }
      /* Finally, print the printable ASCII string for those l bytes */
      fprintf(LIBMEJ_DEBUG_FD, "| %-8s\n", safe_str((char *) buff, l));
      /* Flush after every line in case we crash */
      fflush(LIBMEJ_DEBUG_FD);
    }
  }
  fprintf(LIBMEJ_DEBUG_FD, "DUMP :: Total allocated memory: %10lu bytes\n\n", total);
  fflush(LIBMEJ_DEBUG_FD);
}

void *
libmej_malloc(const char *filename, unsigned long line, size_t size)
{
  void *temp;

#ifdef MALLOC_CALL_DEBUG
  ++malloc_count;
  if (!(malloc_count % MALLOC_MOD)) {
    fprintf(LIBMEJ_DEBUG_FD, "Calls to malloc(): %d\n", malloc_count);
  }
#endif

  D_MEM(("libmej_malloc(%lu) called at %s:%lu\n", size, filename, line));

  temp = (void *) malloc(size);
  ASSERT_RVAL(temp != NULL, NULL);
  if (DEBUG_LEVEL >= DEBUG_MEM) {
    memrec_add_var(temp, size);
  }
  return (temp);
}

void *
libmej_realloc(const char *var, const char *filename, unsigned long line, void *ptr, size_t size)
{
  void *temp;

#ifdef MALLOC_CALL_DEBUG
  ++realloc_count;
  if (!(realloc_count % REALLOC_MOD)) {
    D_MEM(("Calls to realloc(): %d\n", realloc_count));
  }
#endif

  D_MEM(("libmej_realloc(%lu) called for variable %s (%8p) at %s:%lu\n", size, var, ptr, filename, line));
  if (ptr == NULL) {
    temp = (void *) libmej_malloc(__FILE__, __LINE__, size);
  } else {
    temp = (void *) realloc(ptr, size);
    ASSERT_RVAL(temp != NULL, ptr);
    if (DEBUG_LEVEL >= DEBUG_MEM) {
      memrec_chg_var(var, filename, line, ptr, temp, size);
    }
  }
  return (temp);
}

void *
libmej_calloc(const char *filename, unsigned long line, size_t count, size_t size)
{
  void *temp;

#ifdef MALLOC_CALL_DEBUG
  ++calloc_count;
  if (!(calloc_count % CALLOC_MOD)) {
    fprintf(LIBMEJ_DEBUG_FD, "Calls to calloc(): %d\n", calloc_count);
  }
#endif

  D_MEM(("libmej_calloc(%lu, %lu) called at %s:%lu\n", count, size, filename, line));
  temp = (void *) calloc(count, size);
  ASSERT_RVAL(temp != NULL, NULL);
  if (DEBUG_LEVEL >= DEBUG_MEM) {
    memrec_add_var(temp, size * count);
  }
  return (temp);
}

void
libmej_free(const char *var, const char *filename, unsigned long line, void *ptr)
{
#ifdef MALLOC_CALL_DEBUG
  ++free_count;
  if (!(free_count % FREE_MOD)) {
    fprintf(LIBMEJ_DEBUG_FD, "Calls to free(): %d\n", free_count);
  }
#endif

  D_MEM(("libmej_free() called for variable %s (%8p) at %s:%lu\n", var, ptr, filename, line));
  if (ptr) {
    if (DEBUG_LEVEL >= DEBUG_MEM) {
      memrec_rem_var(var, filename, line, ptr);
    }
    free(ptr);
  } else {
    D_MEM(("ERROR:  Caught attempt to free NULL pointer\n"));
  }
}

char *
libmej_strdup(const char *var, const char *filename, unsigned long line, const char *str)
{
  register char *newstr;
  register size_t len;

  D_MEM(("libmej_strdup() called for variable %s (%8p) at %s:%lu\n", var, str, filename, line));

  len = strlen(str) + 1;  /* Copy NUL byte also */
  newstr = (char *) libmej_malloc(filename, line, len);
  strcpy(newstr, str);
  return (newstr);
}

void
libmej_handle_sigsegv(int sig)
{
#if DEBUG >= DEBUG_MEM
  fprintf(LIBMEJ_DEBUG_FD, "Fatal memory fault (%d)!  Dumping memory table.\n", sig);
  memrec_dump();
#endif
  exit(EXIT_FAILURE);
}
