 /***************************************************************
 * MEM.H -- Header file for mem.c                              *
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

#ifndef _MEM_H_

#define _MEM_H_

typedef struct memrec_struct {
  unsigned char init;
  unsigned long Count, TotalSize;
  void **Ptrs;
  size_t *Size;
} MemRec;

#ifdef WITH_DMALLOC
#  include <dmalloc.h>
#endif
#if (DEBUG >= DEBUG_MALLOC)
#  define MALLOC(sz)		Malloc(__FILE__, __LINE__, (sz))
#  define CALLOC(type,n)	Calloc(__FILE__, __LINE__, (n),(sizeof(type)))
#  define REALLOC(mem,sz)	Realloc(#mem, __FILE__, __LINE__, (mem),(sz))
#  define FREE(ptr)		do { Free(#ptr, __FILE__, __LINE__, (ptr)); (ptr) = NULL; } while (0)
#  define MALLOC_MOD 25
#  define REALLOC_MOD 25
#  define CALLOC_MOD 25
#  define FREE_MOD 25
#else
#  define MALLOC(sz)		malloc(sz)
#  define CALLOC(type,n)	calloc((n),(sizeof(type)))
#  define REALLOC(mem,sz)	((sz) ? ((mem) ? (realloc((mem), (sz))) : (malloc(sz))) : ((mem) ? (free(mem), NULL) : (NULL)))
#  define FREE(ptr)		do { free(ptr); (ptr) = NULL; } while (0)
#endif

extern char *SafeStr(char *, unsigned short);
extern MemRec memrec;
extern void memrec_init(void);
extern void memrec_add_var(void *, size_t);
extern void memrec_rem_var(const char *, unsigned long, void *);
extern void memrec_chg_var(const char *, unsigned long, void *, void *, size_t);
extern void memrec_dump(void);
extern void *Malloc(const char *, unsigned long, size_t);
extern void *Realloc(const char *, const char *, unsigned long, void *, size_t);
extern void *Calloc(const char *, unsigned long, size_t, size_t);
extern void Free(const char *, const char *, unsigned long, void *);
extern void HandleSigSegv(int);

#endif /* _MEM_H_ */

