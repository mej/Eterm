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

extern int my_ruid, my_rgid, my_euid, my_egid;

extern int system_wait(char *command);
extern int system_no_wait(char *command);

#endif /* _SYSTEM_H_ */
