 /***************************************************************
 * MEM.H -- Header file for mem.c                              *
 *       -- Michael Jennings                                   *
 *       -- 07 January 1997                                    *
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
#  define MALLOC(sz)		malloc(sz)
#  define CALLOC(type,n)	calloc((n),(sizeof(type)))
#  define REALLOC(mem,sz)	realloc((mem), (sz))
#  define FREE(ptr)		free(ptr)
#elif (DEBUG >= DEBUG_MALLOC)
#  define MALLOC(sz)		Malloc(sz)
#  define CALLOC(type,n)	Calloc((n),(sizeof(type)))
#  define REALLOC(mem,sz)	Realloc((mem),(sz))
/* #  define FREE(ptr)		Free(ptr) */
#  define FREE(ptr)		do { Free(ptr); ptr = NULL; } while(0)
#  define MALLOC_MOD 25
#  define REALLOC_MOD 25
#  define CALLOC_MOD 25
#  define FREE_MOD 25
#else
#  define MALLOC(sz)		malloc(sz)
#  define CALLOC(type,n)	calloc((n),(sizeof(type)))
#  define REALLOC(mem,sz)	((sz) ? ((mem) ? (realloc((mem), (sz))) : (malloc(sz))) : ((mem) ? (free(mem)) : (NULL)))
#  define FREE(ptr)		do { free(ptr); ptr = NULL; } while(0)
#endif

extern char *SafeStr(char *, unsigned short);
extern MemRec memrec;
extern void memrec_init(void);
extern void memrec_add_var(void *, size_t);
extern void memrec_rem_var(void *);
extern void memrec_chg_var(void *, void *, size_t);
extern void memrec_dump(void);
extern void *Malloc(size_t);
extern void *Realloc(void *, size_t);
extern void *Calloc(size_t, size_t);
extern void Free(void *);
extern void myalarm(long);
extern void HandleSigSegv(int);
extern char *GarbageCollect(char *, size_t);
extern char *FileGarbageCollect(char *, size_t);
extern void *fixed_realloc(void *, size_t);

#endif /* _MEM_H_ */

