/*  system.c -- Eterm secure system() replacement
 *           -- 21 August 1997, mej
 */

static const char cvs_ident[] = "$Id$";

#include "../config.h"
#include "feature.h"

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <stdlib.h>
#include <signal.h>
#ifdef HAVE_SYS_WAIT_H
#  include <sys/wait.h>
#endif

#include "strings.h"
#include "options.h"
#include "screen.h"
#include "system.h"
#include "../libmej/debug.h"
#include "debug.h"

int system_wait(char *command);
int system_no_wait(char *command);

int
wait_for_chld(int system_pid)
{

  int pid, status = 0, save_errno = errno, code;

  D_OPTIONS(("wait_for_chld(%ld) called.\n", system_pid));

  while (1) {
    do {
      errno = 0;
    } while (((pid = waitpid(system_pid, &status, WNOHANG)) == -1) &&
	     (errno == EINTR) || !pid);
    /* If the child that exited is the command we spawned, or if the
       child exited before fork() returned in the parent, it must be
       our immediate child that exited.  We exit gracefully. */
    D_OPTIONS(("wait_for_chld():  %ld exited.\n", pid));
    if (pid == system_pid || system_pid == -1) {
      if (WIFEXITED(status)) {
	code = WEXITSTATUS(status);
	D_OPTIONS(("wait_for_chld():  Child process exited with return code %lu\n", code));
      } else if (WIFSIGNALED(status)) {
	code = WTERMSIG(status);
	D_OPTIONS(("wait_for_chld():  Child process was terminated by unhandled signal %lu\n", code));
      }
      return (code);
    }
    errno = save_errno;
  }
  return;
}

/* Replace the system() call with a fork-and-exec that unprivs the child process */

int
system_wait(char *command)
{

  pid_t pid;

  D_OPTIONS(("system_wait(%s) called.\n", command));

  if (!(pid = fork())) {
    setreuid(my_ruid, my_ruid);
    setreuid(my_rgid, my_rgid);
    execl("/bin/sh", "sh", "-c", command, (char *) NULL);
    print_error("system_wait():  execl(%s) failed -- %s", command, strerror(errno));
    exit(EXIT_FAILURE);
  } else {
    D_OPTIONS(("%d:  fork() returned %d\n", getpid(), pid));
    return (wait_for_chld(pid));
  }
}

int
system_no_wait(char *command)
{

  pid_t pid;

  D_OPTIONS(("system_no_wait(%s) called.\n", command));

  if (!(pid = fork())) {
    setreuid(my_ruid, my_ruid);
    setreuid(my_rgid, my_rgid);
    execl("/bin/sh", "sh", "-c", command, (char *) NULL);
    print_error("system_no_wait():  execl(%s) failed -- %s", command, strerror(errno));
    exit(EXIT_FAILURE);
  }
  return (0);
}
