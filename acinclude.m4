dnl acl.m4 -- Written by Duncan Simpson <dps@io.stargate.co.uk>
dnl Posted to BUGTRAQ on 17 June 1999
dnl Used by encouragement. :-)

dnl Check snprintf for overrun potential
AC_DEFUN(dps_snprintf_oflow,
[AC_MSG_CHECKING(whether snprintf ignores n)
AC_CACHE_VAL(dps_cv_snprintf_bug,
[AC_TRY_RUN(
changequote(<<, >>)dnl
<<#include <stdio.h>

#ifndef HAVE_SNPRINTF
#ifdef HAVE_VSNPRINTF
#include "vsnprintf.h"
#else /* not HAVE_VSNPRINTF */
#include "vsnprintf.c"
#endif /* HAVE_VSNPRINTF */
#endif /* HAVE_SNPRINTF */

int main(void)
{
char ovbuf[7];
int i;
for (i=0; i<7; i++) ovbuf[i]='x';
snprintf(ovbuf, 4,"foo%s", "bar");
if (ovbuf[5]!='x') exit(1);
snprintf(ovbuf, 4,"foo%d", 666);
if (ovbuf[5]!='x') exit(1);
exit(0);
} >>
changequote([, ]), dps_cv_snprintf_bug=0, dps_cv_snprintf_bug=1,
dps_cv_snprintf_bug=2)])
if test $dps_cv_snprintf_bug -eq 0; then
  AC_MSG_RESULT([no, snprintf is ok])
else if test $dps_cv_snprint_bug -eq 1; then
  AC_MSG_RESULT([yes, snprintf is broken])
  AC_DEFINE(HAVE_SNPRINTF_BUG,1)
else
  AC_MSG_RESULT([unknown, assuming yes])
  AC_DEFINE(HAVE_SNPRINTF_BUG,1)
fi; fi])

dnl Check vsnprintf for overrun potential
AC_DEFUN(dps_vsnprintf_oflow,
[AC_MSG_CHECKING(whether vsnprintf ignores n)
AC_CACHE_VAL(dps_cv_vsnprintf_bug,
[AC_TRY_RUN(
changequote(<<, >>)dnl
<<#include <stdio.h>
#include <stdarg.h>

#ifndef HAVE_VSNPRINTF
#include "vsnprintf.c"
#endif /* HAVE_VSNPRINTF */

int prnt(char *s, const char *fmt, ...)
{
  va_list argp;
  va_start(argp, fmt);
  vsnprintf(s, 4, fmt, argp);
  va_end(argp);
}

int main(void)
{
  char ovbuf[7];
  int i;
  for (i=0; i<7; i++) ovbuf[i]='x';
  prnt(ovbuf, "foo%s", "bar");
  if (ovbuf[5]!='x') exit(1);
  prnt(ovbuf, "foo%d", 666);
  if (ovbuf[5]!='x') exit(1);
  exit(0);
} >>
changequote([, ]), dps_cv_vsnprintf_bug=0, dps_cv_vsnprintf_bug=1,
dps_cv_vsnprintf_bug=2)])

if test $dps_cv_vsnprintf_bug -eq 0; then
  AC_MSG_RESULT([no, vsnprintf is ok])
else if test $dps_cv_vsnprint_bug -eq 1; then
  AC_MSG_RESULT([yes, vsnprintf is broken])
  AC_DEFINE(HAVE_VSNPRINTF_BUG,1)
else
  AC_MSG_RESULT([unknown, assuming yes])
  AC_DEFINE(HAVE_VSNPRINTF_BUG,1)
fi; fi])

dnl open and symlink interaction bug test
AC_DEFUN(dps_symlink_open_bug,
[AC_MSG_CHECKING(security of interaction between symlink and open)
AC_CACHE_VAL(dps_cv_symlink_open_bug,
[mkdir conftest.d
AC_TRY_RUN(
changequote(<<, >>)dnl
<<#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#else
extern int errno;
#endif

int main(void)
{
  int fd;
  if (chdir("conftest.d")!=0)
    exit(1);
  if (symlink("foo","bar")!=0)
    exit(1);
  if ((fd=open("bar", O_CREAT | O_EXCL | O_WRONLY, 0700))==0)
  {
        write(fd, "If the symlink was to .rhosts you would be unhappy", 50);
	close(fd);
	exit(1);
  }
  if (errno!=EEXIST)
    exit(1);
  exit(0);
} >>
changequote([, ]), cps_cv_symlink_open_bug=0,
[if test -r conftest.d/foo; then
  cps_cv_symlink_open_bug=2
else
  cps_cv_symlink_open_bug=1
fi], cps_cv_symlink_open_buf=3)
rm -rf conftest.d])
case "$cps_cv_symlink_open_bug" in
0) AC_MSG_RESULT(secure) ;;
1) AC_MSG_RESULT(errno wrong but ok)
   AC_DEFINE(HAVE_SYMLINK_OPEN_ERRNO_BUG) ;;
2) AC_MSG_RESULT(insecure)
   AC_DEFINE(HAVE_SYMLINK_OPEN_SECURITY_HOLE)
   AC_DEFINE(HAVE_SYMLINK_OPEN_ERRNO_BUG) ;;
3) AC_MSG_RESULT(assuming insecure)
   AC_DEFINE(HAVE_SYMLINK_OPEN_SECURITY_HOLE)
   AC_DEFINE(HAVE_SYMLINK_OPEN_ERRNO_BUG) ;;
*) AC_MSG_RESULT($cps_cv_symlink_open_bug)
   AC_MSG_ERROR(Impossible value of cps_cv_symlink_open_bug) ;;
esac])

dnl Check to RLIMIT_NPROC resource limit
AC_DEFUN(dps_rlimit_nproc,
[AC_MSG_CHECKING(for working RLIMIT_NPROC resource limit)
AC_CACHE_VAL(dps_cv_rlimit_nproc,
[AC_TRY_RUN(
changequote(<<, >>)dnl
<<
#ifndef HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */
#ifndef HAVE_SIGNAL_H
#include <signal.h>
#endif /* HAVE_SIGNAL_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif /* HAVE_SYS_RESOURCE_H */

int main(void)
{
#ifdef RLIMIT_NPROC
    static const struct rlimit pid_lim={RLIMIT_NPROC, 1};
    pid_t f;

    signal(SIGCHLD, SIG_IGN);
    setrlimit(RLIMIT_NPROC, (struct rlimit *) &pid_lim);
    if ((f=fork())==0)
	exit(0);
    if (f==-1)
	exit(0); /* The fork() failed (the right thing) */
#endif
   exit(1);
} >>
changequote([, ]), cps_cv_rlimit_nproc=0, cps_cv_rlimit_nproc=1,
cps_cv_rlimit_nproc=2)])
if test $cps_cv_rlimit_nproc -eq 0; then
  AC_MSG_RESULT([yes])
  AC_DEFINE(HAVE_RLIMIT_NPROC,1)
else if test $cps_cv_rlimit_nproc -eq 1; then
  AC_MSG_RESULT([no])
else
  AC_MSG_RESULT([unknown, assuming none])
fi; fi])

dnl Check to RLIMIT_MEMLOCK resource limit
AC_DEFUN(cps_rlimit_memlock,
[AC_MSG_CHECKING(for RLIMIT_MEMLOCK resource limit)
AC_CACHE_VAL(cps_cv_rlimit_memlock,
[AC_TRY_RUN(
changequote(<<, >>)dnl
<<
#ifndef HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */
#ifndef HAVE_SIGNAL_H
#include <signal.h>
#endif /* HAVE_SIGNAL_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif /* HAVE_SYS_RESOURCE_H */
#ifdef HAVE_SYS_MMAN
#include <sys/mman.h>
#endif /* HAVE_SYS_MMAN */
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif /* HAVE_ERRNO_H */

int main(void)
{
#ifdef RLIMIT_MEMLOCK
    static const struct rlimit mlock_lim={RLIMIT_MEMLOCK, 0};
    void *memory;

    if (setrlimit(RLIMIT_MEMLOCK, (struct rlimit *) &mlock_lim)!=-1)
	exit(0);
#endif
exit(1);
} >>
changequote([, ]), cps_cv_rlimit_memlock=0, cps_cv_rlimit_memlock=1,
cps_cv_rlimit_memlock=2)])
if test $cps_cv_rlimit_memlock -eq 0; then
  AC_MSG_RESULT([yes])
  AC_DEFINE(HAVE_RLIMIT_MEMLOCK,1)
else if test $cps_cv_rlimit_memlock -eq 1; then
  AC_MSG_RESULT([no])
else
  AC_MSG_RESULT([unknown, assuming none])
fi; fi])

