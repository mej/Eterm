/* profile.h for Eterm.
 * 25 Mar 1998, vendu.
 */

#ifndef _PROFILE_H
# define _PROFILE_H

/* included for a possible #define PROFILE */
# include <stdio.h>
# include <sys/time.h>
# include <unistd.h>

/* NOTE: if PROFILE is not defined, all macros in this file will
 * be set to (void)0 so they won't get compiled into binaries.
 */
#define PROFILE
# ifdef PROFILE

/* Data structures */

typedef struct {
    long long total;
    struct timeval start;
    struct timeval stop;
} P_counter_t;

/* Profiling macros */

/* Sets tv to current time.
 * struct timeval tv;
 * Usage: P_SETTIMEVAL(struct timeval tv);
 */
#  define P_SETTIMEVAL(tv) gettimeofday(&(tv), NULL)

/* NOT FINISHED YET */
#  define P_UPDATETOTAL(cnt) { \
    cnt.total += P_CMPTIMEVALS_USEC(cnt.start, cnt.stop); \
}

/* Declare cnt and initialize by setting to zero.
 * P_counter_t cnt;
 * Usage: P_INITCOUNTER(counter);
 * NOTES: cnt will be declared. This means that you'll
 * probably need to localize a block where to use this; see
 * the definition of P_CALL() elsewhere in this file.
 */
#  define P_INITCOUNTER(cnt) \
    P_counter_t cnt = { 0, { 0, 0 }, { 0, 0 } }
/* Time from start to stop in microseconds
 * struct timeval start, stop;
 */
#  define P_CMPTIMEVALS_USEC(start, stop) \
    ((stop.tv_sec - start.tv_sec)*1000000 \
     + (stop.tv_usec - start.tv_usec))

/* Counts the time spent in the call f and outputs
 * str: <time spent in f in microseconds> to stderr.
 * NOTE: f can be any function call, for example sqrt(5).
 */
#  define P_CALL(f, str) { \
    P_INITCOUNTER(cnt); \
    P_SETTIMEVAL(cnt.start); \
    f; \
    P_SETTIMEVAL(cnt.stop); \
    fprintf(stderr, "%s: %ld\n", str, \
            P_CMPTIMEVALS_USEC(cnt.start, cnt.stop)); \
}

# else /* PROFILE */

#  define P_SETTIMEVAL(tv) ((void)0)
#  define P_UPDATETOTAL(cnt) ((void)0)
#  define P_INITCOUNTER(cnt) ((void)0)
#  define P_CMPTIMEVALS_USEC(start, stop) ((void)0)
#  define P_CALL(f, str) f

# endif /* PROFILE */

#endif /* _PROFILE_H */
