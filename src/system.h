/*  system.h -- Header file for system.c */

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#define ETERM_SYSTEM_WAIT

#ifdef ETERM_SYSTEM_WAIT
#  define system(c) system_wait((c))
#else
#  define system(c) system_no_wait((c))
#endif

typedef RETSIGTYPE (*sighandler_t)(int);

extern int wait_for_chld(int);
extern int system_wait(char *);
extern int system_no_wait(char *);

#endif /* _SYSTEM_H_ */
