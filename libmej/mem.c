
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

static void memrec_add_var(memrec_t *, void *, size_t);
static void memrec_rem_var(memrec_t *, const char *, const char *, unsigned long, void *);
static void memrec_chg_var(memrec_t *, const char *, const char *, unsigned long, void *, void *, size_t);
static void memrec_dump(memrec_t *);

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

static memrec_t malloc_rec, pixmap_rec, gc_rec;

void
memrec_init(void)
{
  D_MEM(("Constructing memory allocation records\n"));
  malloc_rec.ptrs = (ptr_t *) malloc(sizeof(ptr_t));
  pixmap_rec.ptrs = (ptr_t *) malloc(sizeof(ptr_t));
  gc_rec.ptrs = (ptr_t *) malloc(sizeof(ptr_t));
}

static void
memrec_add_var(memrec_t *memrec, void *ptr, size_t size)
{
  register ptr_t *p;

  ASSERT(memrec != NULL);
  memrec->cnt++;
  if ((memrec->ptrs = (ptr_t *) realloc(memrec->ptrs, sizeof(ptr_t) * memrec->cnt)) == NULL) {
    D_MEM(("Unable to reallocate pointer list -- %s\n", strerror(errno)));
  }
  D_MEM(("Adding variable of size %lu at %8p\n", size, ptr));
  p = memrec->ptrs + memrec->cnt - 1;
  p->ptr = ptr;
  p->size = size;
}

static void
memrec_rem_var(memrec_t *memrec, const char *var, const char *filename, unsigned long line, void *ptr)
{
  register ptr_t *p = NULL;
  register unsigned long i;

  ASSERT(memrec != NULL);
  for (i = 0; i < memrec->cnt; i++) {
    if (memrec->ptrs[i].ptr == ptr) {
      p = memrec->ptrs + i;
      break;
    }
  }
  if (!p) {
    D_MEM(("ERROR:  File %s, line %d attempted to free variable %s (%8p) which was not allocated with MALLOC/REALLOC\n", filename, line, var, ptr));
    return;
  }
  memrec->cnt--;
  D_MEM(("Removing variable of size %lu at %8p\n", p->size, p->ptr));
  memmove(p, p + 1, sizeof(ptr_t) * (memrec->cnt - i));
  memrec->ptrs = (ptr_t *) realloc(memrec->ptrs, sizeof(ptr_t) * memrec->cnt);
}

static void
memrec_chg_var(memrec_t *memrec, const char *var, const char *filename, unsigned long line, void *oldp, void *newp, size_t size)
{
  register ptr_t *p = NULL;
  register unsigned long i;

  ASSERT(memrec != NULL);
  for (i = 0; i < memrec->cnt; i++) {
    if (memrec->ptrs[i].ptr == oldp) {
      p = memrec->ptrs + i;
      break;
    }
  }
  if (i == memrec->cnt) {
    D_MEM(("ERROR:  File %s, line %d attempted to realloc variable %s (%8p) which was not allocated with MALLOC/REALLOC\n", filename, line, var, oldp));
    return;
  }
  D_MEM(("Changing variable of %lu bytes at %8p to one of %lu bytes at %8p\n", p->size, p->ptr, size, newp));
  p->ptr = newp;
  p->size = size;
}

static void
memrec_dump(memrec_t *memrec)
{
  register ptr_t *p;
  unsigned long i, j, k, l, total = 0;
  unsigned long len;
  unsigned char buff[9];

  ASSERT(memrec != NULL);
  fprintf(LIBMEJ_DEBUG_FD, "DUMP :: %lu pointers stored.\n", memrec->cnt);
  fprintf(LIBMEJ_DEBUG_FD, "DUMP ::  Pointer |  Address |  Size  | Offset  | 00 01 02 03 04 05 06 07 |  ASCII  \n");
  fprintf(LIBMEJ_DEBUG_FD, "DUMP :: ---------+----------+--------+---------+-------------------------+---------\n");
  fflush(LIBMEJ_DEBUG_FD);
  len = sizeof(ptr_t) * memrec->cnt;
  memset(buff, 0, sizeof(buff));

  /* First, dump the contents of the memrec->ptrs[] array. */
  for (p = memrec->ptrs, j = 0; j < len; p++, j += 8) {
    fprintf(LIBMEJ_DEBUG_FD, "DUMP ::  %07lu | %8p | %06lu | %07x | ", (unsigned long) 0, memrec->ptrs, (unsigned long) (sizeof(ptr_t) * memrec->cnt), (unsigned int) j);
    /* l is the number of characters we're going to output */
    l = ((len - j < 8) ? (len - j) : (8));
    /* Copy l bytes (up to 8) from memrec->ptrs[] (p) to buffer */
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
  for (p = memrec->ptrs, i = 0; i < memrec->cnt; p++, i++) {
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

/******************** MEMORY ALLOCATION INTERFACE ********************/
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
    memrec_add_var(&malloc_rec, temp, size);
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
      memrec_chg_var(&malloc_rec, var, filename, line, ptr, temp, size);
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
    memrec_add_var(&malloc_rec, temp, size * count);
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
      memrec_rem_var(&malloc_rec, var, filename, line, ptr);
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
libmej_dump_mem_tables(void)
{
  memrec_dump(&malloc_rec);
}



/******************** PIXMAP ALLOCATION INTERFACE ********************/

Pixmap
libmej_x_create_pixmap(const char *filename, unsigned long line, Display *d, Drawable win, unsigned int w, unsigned int h, unsigned int depth)
{
  Pixmap p;

  D_MEM(("Creating %ux%u pixmap of depth %u for window 0x%08x at %s:%lu\n", w, h, depth, win, filename, line));

  p = XCreatePixmap(d, win, w, h, depth);
  ASSERT_RVAL(p != None, None);
  if (DEBUG_LEVEL >= DEBUG_MEM) {
    memrec_add_var(&pixmap_rec, (void *) p, w * h * (depth / 8));
  }
  return (p);
}

void
libmej_x_free_pixmap(const char *var, const char *filename, unsigned long line, Display *d, Pixmap p)
{
  D_MEM(("libmej_x_free_pixmap() called for variable %s (0x%08x) at %s:%lu\n", var, p, filename, line));
  if (p) {
    if (DEBUG_LEVEL >= DEBUG_MEM) {
      memrec_rem_var(&pixmap_rec, var, filename, line, (void *) p);
    }
    XFreePixmap(d, p);
  } else {
    D_MEM(("ERROR:  Caught attempt to free NULL pixmap\n"));
  }
}

void
libmej_dump_pixmap_tables(void)
{
  memrec_dump(&pixmap_rec);
}



/********************** GC ALLOCATION INTERFACE **********************/

GC
libmej_x_create_gc(const char *filename, unsigned long line, Display *d, Drawable win, unsigned long mask, XGCValues *gcv)
{
  GC gc;

  D_MEM(("Creating gc for window 0x%08x at %s:%lu\n", win, filename, line));

  gc = XCreateGC(d, win, mask, gcv);
  ASSERT_RVAL(gc != None, None);
  if (DEBUG_LEVEL >= DEBUG_MEM) {
    memrec_add_var(&gc_rec, (void *) gc, sizeof(XGCValues));
  }
  return (gc);
}

void
libmej_x_free_gc(const char *var, const char *filename, unsigned long line, Display *d, GC gc)
{
  D_MEM(("libmej_x_free_gc() called for variable %s (0x%08x) at %s:%lu\n", var, gc, filename, line));
  if (gc) {
    if (DEBUG_LEVEL >= DEBUG_MEM) {
      memrec_rem_var(&gc_rec, var, filename, line, (void *) gc);
    }
    XFreeGC(d, gc);
  } else {
    D_MEM(("ERROR:  Caught attempt to free NULL GC\n"));
  }
}

void
libmej_dump_gc_tables(void)
{
  memrec_dump(&gc_rec);
}
