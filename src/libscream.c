/****************************************************************************
 * scream::libscream.c
 * routines to connect to screen and or scream daemons.
 * GNU Public Licence applies.
 * 2002/04/19  Azundris  incept
 * 2002/05/04  Azundris  support for esoteric screens, thanks to Till
 ***************************************************************************/



#include <stdio.h>              /* stderr, fprintf, snprintf() */
#include <string.h>             /* bzero() */
#include <pwd.h>                /* getpwuid() */
#include <sys/types.h>          /* getpwuid() */
#include <unistd.h>             /* getuid() */
#include <stdlib.h>             /* atoi() */
#include <netdb.h>              /* getservbyname() */
#include <netinet/in.h>         /* ntohs() */
#include <limits.h>             /* PATH_MAX */

#include "scream.h"             /* structs, defs, headers */
#include "screamcfg.h"          /* user-tunables */

#ifndef MAXPATHLEN
#  ifdef PATH_MAX
#    define MAXPATHLEN PATH_MAX
#  elif defined(MAX_PATHLEN)
#    define MAXPATHLEN MAX_PATHLEN
#  endif
#endif



long err_inhibit = 0;           /* bits. avoid telling user the same thing twice. */



/***************************************************************************/
/* constructors/destructors */
/****************************/



_ns_efuns *
ns_new_efuns(void)
{
    _ns_efuns *s = malloc(sizeof(_ns_efuns));
    if (s) {
        bzero(s, sizeof(_ns_efuns));
    }
    return s;
}

_ns_efuns *
ns_ref_efuns(_ns_efuns ** ss)
{
    if (ss && *ss) {
        (*ss)->refcount++;
        return *ss;
    }
    return NULL;
}

_ns_efuns *
ns_dst_efuns(_ns_efuns ** ss)
{
    if (ss && *ss) {
        _ns_efuns *s = *ss;
#ifdef NS_DEBUG_MEM
        *ss = NULL;
#endif
        if (!--(s->refcount)) {
#ifdef NS_DEBUG_MEM
            bzero(s, sizeof(_ns_efuns));
#endif
            free(s);
        }
    }
    return NULL;
}



_ns_disp *
ns_new_disp(void)
{
    _ns_disp *s = malloc(sizeof(_ns_disp));
    if (s) {
        bzero(s, sizeof(_ns_disp));
    }
    return s;
}

_ns_disp *
ns_dst_disp(_ns_disp ** ss)
{
    if (ss && *ss) {
        _ns_disp *s = *ss;
        if (s->name)
            free(s->name);
        if (s->efuns)
            ns_dst_efuns(&(s->efuns));
#ifdef NS_DEBUG_MEM
        *ss = NULL;
        bzero(s, sizeof(_ns_disp));
#endif
        free(s);
    }
    return NULL;
}

_ns_disp *
ns_dst_dsps(_ns_disp ** ss)
{
    if (ss && *ss) {
        _ns_disp *s = *ss, *t;
#ifdef NS_DEBUG_MEM
        *ss = NULL;
#endif
        do {
            t = s->next;
            ns_dst_disp(&s);
            s = t;
        } while (s);
        return NULL;
    }
}



_ns_sess *
ns_new_sess(void)
{
    _ns_sess *s = malloc(sizeof(_ns_sess));
    if (s) {
        bzero(s, sizeof(_ns_sess));
        s->escape = NS_SCREEN_ESCAPE;	/* default setup for the screen program */
        s->literal = NS_SCREEN_LITERAL;
    }
    return s;
}

_ns_sess *
ns_dst_sess(_ns_sess ** ss)
{
    if (ss && *ss) {
        _ns_sess *s = *ss;
        ns_dst_dsps(&(s->dsps));
        if (s->host)
            free(s->host);
        if (s->user)
            free(s->user);
        if (s->pass)
            free(s->pass);
        if (s->efuns)
            ns_dst_efuns(&(s->efuns));
#ifdef NS_DEBUG_MEM
        *ss = NULL;
        bzero(s, sizeof(_ns_sess));
#endif
        free(s);
    }
    return NULL;
}




/***************************************************************************/
/* send commands to screen */
/***************************/



/* send a command string to a session, using the appropriate escape-char
   sess  the session
   cmd   the command string.  escapes must be coded as NS_SCREEN_ESCAPE;
         this routine will convert the string to use the escapes actually
         used in the session
   <-    error code */

int
ns_screen_command(_ns_sess * sess, char *cmd)
{
    char *c;
    int ret = NS_SUCC;
    if (sess->efuns->inp_text) {
        if ((c = strdup(cmd))) {
            char *p = c;
            while (*p) {
                if (*p == NS_SCREEN_ESCAPE)	/* replace default escape-char with that */
                    *p = sess->escape;	/* actually used in this session */
                p++;
            }
            sess->efuns->inp_text(NULL, sess->fd, c);
            free(c);
        } else
            ret = NS_OOM;
    } /* out of memory */
    else {
        ret = NS_EFUN_NOT_SET;
        fprintf(stderr, "ns_screen_command: sess->efuns->inp_text not set!\n");
    }
    return ret;
}



/* scroll horizontally to column x (dummy) */
int
ns_scroll2x(_ns_sess * s, int x)
{
    return NS_FAIL;
}

/* scroll vertically so line y of the scrollback buffer is the top line */
int
ns_scroll2y(_ns_sess * s, int y)
{
    return NS_FAIL;
}

/* go to display #d */
int
ns_go2_disp(_ns_sess * s, int d)
{
    return NS_FAIL;
}

/* add a client display with the name "name" after display number #after */
int
ns_add_disp(_ns_sess * s, int after, char *name)
{
    return NS_FAIL;
}

/* resize display #d to w*h */
int
ns_rsz_disp(_ns_sess * s, int d, int w, int h)
{
    return NS_FAIL;
}

/* remove display #d */
int
ns_rem_disp(_ns_sess * s, int d)
{
    return NS_FAIL;
}

/* rename display #d to "name" */
int
ns_ren_disp(_ns_sess * s, int d, char *name)
{
    return NS_FAIL;
}

/* log activity in display #d to file "logfile" */
int
ns_log_disp(_ns_sess * s, int d, char *logfile)
{
    return NS_FAIL;
}

/* force an update of the status line */
int
ns_upd_stat(_ns_sess * s)
{
    return ns_screen_command(s, NS_SCREEN_UPDATE);
}



/***************************************************************************/
/* attach/detach */
/*****************/





/* return port number for service SSH (secure shell).
   <-  a port number -- 22 in all likelihood.
 */
int
get_ssh_port(void)
{
    /* (fixme) replace with getservbyname_r on systems that have it */
    struct servent *srv = getservbyname("ssh", "tcp");
    return srv ? ntohs(srv->s_port) : NS_DFLT_SSH_PORT;
}



/* ns_desc_sess
   print basic info about a session.  mostly for debugging.
   sess:   a session struct as generated by (eg) ns_attach_by_URL()
   doc:    info about the context
 ! stdout: info about the session
*/

void
ns_desc_sess(_ns_sess * sess, char *doc)
{
    if (!sess) {
        fprintf(stderr, "%s: ns_desc_sess called with broken pointer!\n", doc);
        return;
    }
    if (sess->where == NS_LCL)
        fprintf(stderr, "%s: (efuns@%p)\t (user %s) local %s ", doc, sess->efuns, sess->user, sess->proto);
    else {
        fprintf(stderr, "%s: (efuns@%p)\t %s://%s%s%s@%s",
                doc, sess->efuns, sess->proto, sess->user, sess->pass ? ":" : "", sess->pass ? sess->pass : "", sess->host);
        if (sess->port != NS_DFLT_SSH_PORT)
            fprintf(stderr, ":%s", sess->port);
    }
    fprintf(stderr, "/%s\n", sess->rsrc);
}



/* run a command. uses the terminal's internal run facility.
   converts system/"char *" to exec/"arg **".
   efuns: struct of callbacks into the terminal program.
          ns_run() will fail if no callback to the terminal's "run program"
          (exec) facility is provided.
   cmd:   a string to exec
   <-     whatever the callback returns.  In Eterm, it's a file-descriptor.
 */
int
ns_run(_ns_efuns * efuns, char *cmd)
{
    char **args = NULL;
    char *p = cmd;
    int c, n = 0, s = 0;

    if (!efuns || !efuns->execute)
        goto fail;

    if (cmd && *cmd) {          /* count args (if any) */
#ifdef NS_DEBUG
        fprintf(stderr, "ns_run: executing \"%s\"...\n", cmd);
#endif
        do {
            n++;
            while (*p && *p != ' ') {
                if (*p == '\"') {
                    do {
                        p++;
                        if (s)
                            s = 0;
                        else if (*p == '\\')
                            s = 1;
                        else if (*p == '\"')
                            s = 2;
                    }
                    while (*p && s != 2);
                }
                p++;
            }
            while (*p == ' ')
                p++;
        }
        while (*p);

        if (!(args = malloc((n + 2) * sizeof(char *))))
            goto fail;

        for (p = cmd, c = 0; c < n; c++) {
            args[c] = p;
            while (*p && *p != ' ') {
                if (*p == '\"') {	/* leave quoting stuff together as one arg */
                    args[c] = &p[1];	/* but remove the quote signs */
                    do {
                        p++;
                        if (s)
                            s = 0;
                        else if (*p == '\\')
                            s = 1;
                        else if (*p == '\"')
                            s = 2;
                    }
                    while (*p && s != 2);
                    *p = '\0';
                }
                p++;
            }
            while (*p == ' ')
                *(p++) = '\0';
        }
        args[c++] = NULL;
    }

    n = efuns->execute(NULL, args);
    if (args)
        free(args);
    return n;

  fail:
    return NS_FAIL;
}



/* attach a local session (using screen/scream)
   sp  the session
   <-  NS_FAIL, or the result of ns_run() */

int
ns_attach_lcl(_ns_sess ** sp)
{
    _ns_sess *sess;
#define MAXCMD 512
    char cmd[MAXCMD + 1];
    int ret;

    if (!sp || !*sp)
        return NS_FAIL;
    sess = *sp;
    ret = snprintf(cmd, MAXCMD, "%s %s", NS_SCREEN_CALL, NS_SCREEN_OPTS);
    return (ret < 0 || ret > MAXCMD) ? NS_FAIL : ns_run(sess->efuns, cmd);
}



/* attach a remote session (using screen/scream via ssh)
   sp  the session
   <-  NS_FAIL, or the result of ns_run() */

int
ns_attach_ssh(_ns_sess ** sp)
{
    _ns_sess *sess;
    char cmd[MAXCMD + 1];
    int ret;

    if (!sp || !*sp)
        return NS_FAIL;
    sess = *sp;
    ret = snprintf(cmd, MAXCMD, "%s %s -p %d %s@%s %s", NS_SSH_CALL, NS_SSH_OPTS, sess->port, sess->user, sess->host, NS_SCREEM_CALL);
    return (ret < 0 || ret > MAXCMD) ? NS_FAIL : ns_run(sess->efuns, cmd);
}



/* ns_attach_by_sess
   attach/create a scream/screen session, locally or over the net.
   sess:   a session struct as generated by (eg) ns_attach_by_URL()
 ! err:    if non-NULL, variable pointed at will contain an error status
   <-      the requested session, or NULL in case of failure.
           a session thus returned must be detached/closed later.
*/

_ns_sess *
ns_attach_by_sess(_ns_sess ** sp, int *err)
{
    _ns_sess *sess;
    int err_dummy;
    char *p;

    if (!err)
        err = &err_dummy;
    *err = NS_INVALID_SESS;

    if (!sp || !*sp)
        return NULL;
    sess = *sp;

#ifdef NS_DEBUG
    ns_desc_sess(sess, "ns_attach_by_sess()");
#endif

    switch (sess->where) {
      case NS_LCL:
          sess->fd = ns_attach_lcl(&sess);
          break;
      case NS_SU:              /* (fixme) uses ssh, should use su */
          /* local session, but for a different uid. */
          /* FALL-THROUGH */
      case NS_SSH:
          sess->fd = ns_attach_ssh(&sess);
          break;
      default:
          *err = NS_UNKNOWN_LOC;
          goto fail;
    }

#ifdef NS_DEBUG
    fprintf(stderr, "screen session-fd is %d\n", sess->fd);
#endif

    (void) ns_screen_command(sess, NS_SCREEN_INIT);

    return sess;

  fail:
    return ns_dst_sess(sp);
}



/* ns_attach_by_URL
   parse URL into sess struct (with sensible defaults), then pick up/create
   said session using ns_attach_by_sess()
   url:    URL to create/pick up a session at.
           proto://user:password@host.domain:port  (all parts optional)
           NULL/empty string equivalent to
           screen://current_user@localhost/-xRR
   ef:     a struct containing callbacks into client (resize scrollbars etc.)
           while setting those callbacks is optional; omitting the struct
           itself seems unwise.
 ! err:    if non-NULL, variable pointed at will contain an error status
   xd:     pointer to extra-data the terminal app wants to associate with
           a session, or NULL
   <-      the requested session, or NULL in case of failure.
           a session thus returned must be detached/closed later.
*/

_ns_sess *
ns_attach_by_URL(char *url, _ns_efuns ** ef, int *err, void *xd)
{
    int err_dummy;
    char *p;
    _ns_sess *sess = ns_new_sess();
    struct passwd *pwe = getpwuid(getuid());

    if (!err)
        err = &err_dummy;
    *err = NS_OOM;

    if (!sess)
        return NULL;

    if (url && strlen(url)) {
        char *q, *d;

        if (!(d = strdup(url)))
            goto fail;

        if ((q = strstr(d, "://"))) {	/* protocol, if any */
            *q = '\0';
            if (!(sess->proto = strdup(d)))
                goto fail;
            q += 3;
        } else
            q = d;

        if ((p = strchr(q, '@'))) {	/* user, if any */
            char *r;
            if (p != q) {       /* ignore empty user */
                *p = '\0';
                if ((r = strchr(q, ':'))) {	/* password, if any */
                    *(r++) = '\0';
                    if (!(sess->pass = strdup(r)))	/* password may be empty string! */
                        goto fail;
                }
                sess->user = strdup(q);
            }
            q = p + 1;
        }

        if ((p = strchr(q, ':'))) {	/* port, if any */
            *(p++) = '\0';
            if (!*p || !(sess->port = atoi(p)) || sess->port > NS_MAX_PORT) {
                *err = NS_MALFORMED_URL;
                goto fail;
            }
        }

        if (strlen(q) && !(sess->host = strdup(q)))	/* host, if any */
            goto fail;

        free(d);
    }

    sess->where = NS_SSH;

    if (!sess->user) {          /* default user (current user) */
        if (!pwe) {
            *err = NS_UNKNOWN_USER;
            goto fail;
        }
        if (!(sess->user = strdup(pwe->pw_name)))
            goto fail;
    } else if (pwe && strcmp(pwe->pw_name, sess->user)) {	/* user!=current_user */
        sess->where = NS_SU;
    }

    if (!sess->host) {          /* no host */
        if (!(sess->host = strdup("localhost")))
            goto fail;
        if (!sess->port) {      /* no host/port */
            sess->where = NS_LCL;
        }
    } else if ((p = strchr(sess->host, '/')))	/* have host */
        *p = '\0';

    if (!sess->port)            /* no port -> default port (SSH) */
        sess->port = get_ssh_port();

    sess->backend = NS_MODE_NEGOTIATE;
    if (!sess->proto) {
        if (!(sess->proto = strdup("scream")))
            goto fail;
    } else if (!strcmp(sess->proto, "screen"))
        sess->backend = NS_MODE_SCREEN;
    else if (!strcmp(sess->proto, "scream"))
        sess->backend = NS_MODE_SCREAM;
    else {
        *err = NS_UNKNOWN_PROTO;
        goto fail;
    }

    if (!sess->efuns && ef && *ef) {
        sess->efuns = ns_ref_efuns(ef);
    }

    sess->userdef = xd;

    *err = NS_SUCC;
    return ns_attach_by_sess(&sess, err);

  fail:
    return ns_dst_sess(&sess);
}



/* detach a session and release its memory
   sess  the session
   <-    error code */
int
ns_detach(_ns_sess ** sess)
{
    ns_desc_sess(*sess, "ns_detach");
    (void) ns_dst_sess(sess);
    return NS_SUCC;
}



/***************************************************************************/
/* messages to the client */
/* (register callbacks)   */
/**************************/



/* function that moves horizontal scrollbar to x/1000 % of width */
void
ns_register_ssx(_ns_efuns * efuns, int (*set_scroll_x) (void *, int))
{
    efuns->set_scroll_x = set_scroll_x;
}

/* function that moves vertical scrollbar to y/1000 % of height */
void
ns_register_ssy(_ns_efuns * efuns, int (*set_scroll_y) (void *, int))
{
    efuns->set_scroll_y = set_scroll_y;
}

/* function that sets horizontal scrollbar to w/1000 % of width */
void
ns_register_ssw(_ns_efuns * efuns, int (*set_scroll_w) (void *, int))
{
    efuns->set_scroll_w = set_scroll_w;
}

/* function that sets vertical scrollbar to h/1000 % of height */
void
ns_register_ssh(_ns_efuns * efuns, int (*set_scroll_h) (void *, int))
{
    efuns->set_scroll_h = set_scroll_h;
}

/* function that redraws the terminal */
void
ns_register_red(_ns_efuns * efuns, int (*redraw) (void *))
{
    efuns->redraw = redraw;
}

/* function that redraw part of the terminal */
void
ns_register_rda(_ns_efuns * efuns, int (*redraw_xywh) (void *, int, int, int, int))
{
    efuns->redraw_xywh = redraw_xywh;
}

/* function to call when a new client was added ("add tab").
   after denotes the index of the button after which this one should
   be inserted (0..n, 0 denoting "make it the first button") */
void
ns_register_ins(_ns_efuns * efuns, int (*ins_disp) (void *, int, char *))
{
    efuns->ins_disp = ins_disp;
}

/* function to call when a client was closed ("remove tab") */
void
ns_register_del(_ns_efuns * efuns, int (*del_disp) (void *, int))
{
    efuns->del_disp = del_disp;
}

/* function to call when a client's title was changed ("update tab") */
void
ns_register_upd(_ns_efuns * efuns, int (*upd_disp) (void *, int, int, char *))
{
    efuns->upd_disp = upd_disp;
}

/* function to pass status lines to */
void
ns_register_err(_ns_efuns * efuns, int (*err_msg) (void *, int, char *))
{
    efuns->err_msg = err_msg;
}

/* function that will execute client programs (in pseudo-terminal et al) */
void
ns_register_exe(_ns_efuns * efuns, int (*execute) (void *, char **))
{
    efuns->execute = execute;
}

/* function that will hand text as input to the client */
void
ns_register_txt(_ns_efuns * efuns, int (*inp_text) (void *, int, char *))
{
    efuns->inp_text = inp_text;
}



/* get callbacks.  at least one of session and display must be non-NULL.
   s  session, or NULL. if NULL, will be initialized from d->sess
   d  display, or NULL. if NULL, will be initialized from s->curr.
                        if set, will override session callbacks;
                        note that NULL pointers in d->efuns *will*
                        override (disable) non-NULL pointers in s->efuns!
   <- callback-struct */

_ns_efuns *
ns_get_efuns(_ns_sess * s, _ns_disp * d)
{
    if (!s) {
        if (!d || !d->sess)
            return NULL;
        else
            s = d->sess;
    }
    if (!d)
        d = s->curr;
    if (d && d->efuns)
        return d->efuns;
    else
        return s->efuns;
}



/* test if we have a valid callback for function-type "e".
  !p  a variable of the "_ns_efuns *" type.  will contain a pointer to
      an efun struct containing a function pointer to the requested function
      if such a struct exists, or NULL, if it doesn't exist
   s  a variable of the "_ns_sess *" type, or NULL (see ns_get_efuns())
   d  a variable of the "_nd_disp *" type, or NULL (see ns_get_efuns())
   e  the name of an element of "_ns_efuns"
  !<- conditional execution of next (compound-) statement (which would
      normally be (p)->(e)(...), the call of the function e).
 */
#define NS_IF_EFUN_EXISTS(p,s,d,e) if(((p)=ns_get_efuns((s),(d)))&&((p)->e))



/***************************************************************************/
/* display-handling */
/********************/



/* we need a certain display struct (index n in session s).
   give it to us if it exists.
   s  the session the display should be in
   n  the index of the display (>=0).  displays in a session are sorted
      by index, but may be sparse (0, 1, 3, 7)
   <- the requested display */

_ns_disp *
disp_fetch(_ns_sess * s, int n)
{
    _ns_disp *d, *e = NULL, *c;

    for (c = s->dsps; c && (c->index < n); c = c->next)
        e = c;
    if (c && (c->index == n))   /* found it */
        return c;
    return NULL;
}



/* we need a certain display struct (index n in session s).
   give it to us.  if you can't find it, make one up and insert it into
   the list.
   s  the session the display should be in
   n  the index of the display (>=0).  displays in a session are sorted
      by index, but may be sparse (0, 1, 3, 7)
   <- the requested display */

_ns_disp *
disp_fetch_or_make(_ns_sess * s, int n)
{
    _ns_disp *d, *e = NULL, *c;

    for (c = s->dsps; c && (c->index < n); c = c->next)
        e = c;

    if (c && (c->index == n))   /* found it */
        return c;

    if (!(d = ns_new_disp()))   /* not there, create new */
        return NULL;            /* can't create, fail */

    d->index = n;

    if ((d->next = c))          /* if not last element... */
        c->prvs = d;
    if ((d->prvs = e))          /* if not first element */
        e->next = d;
    else                        /* make first */
        s->dsps = d;

    d->sess = s;                /* note session on display */

#if 1
    if (!d->sess->curr)         /* note as current on session if first display */
        d->sess->curr = d;
#endif

    return d;
}



/* get element number from screen-index (latter is sparse, former ain't)
   screen   the session in question
   n        the index screen gave us (sparse)
   <-       the real index (element number in our list of displays) */

int
disp_get_real_by_screen(_ns_sess * screen, int n)
{
    _ns_disp *d2 = screen->dsps;
    int r = 0;
    while (d2 && d2->index != n) {
        d2 = d2->next;
        r++;
    }
#ifdef NS_DEBUG
    if (!d2)
        return -1;
#endif
    return r;
}



/* get screen-index from element number (former is sparse, latter ain't)
   screen   the session in question
   n        the real index (element number in our list of displays)
   <-       the index screen knows (sparse) */

int
disp_get_screen_by_real(_ns_sess * screen, int r)
{
    _ns_disp *d2 = screen->dsps;
    while (d2 && (r-- > 0))
        d2 = d2->next;
#ifdef NS_DEBUG
    if (!d2)
        return -1;
#endif
    return d2->index;
}



/* remove a display from the internal list and release its struct and data
   disp  the display in question */

void
disp_kill(_ns_disp * d3)
{
    if (d3->prvs) {
        d3->prvs->next = d3->next;
        if (d3->sess->curr == d3)
            d3->sess->curr = d3->prvs;
    } else {
        d3->sess->dsps = d3->next;
        if (d3->sess->curr == d3)
            d3->sess->curr = d3->next;
    }
    if (d3->next)
        d3->next->prvs = d3->prvs;
    ns_dst_disp(&d3);
}



/***************************************************************************/
/* parse status lines of the "screen" program */
/**********************************************/



/* parse a message (not a display-list) set by the "screen" program
   screen   the session associated with that instance of screen,
            as returned by ns_attach_by_URL() and related.
            the session must contain a valid struct of callbacks (efuns),
            as certain functionalities ("add a tab", "show status message")
            may be called from here.
   p        the offending message-line
   <-       returns an error code. */

int
parse_screen_msg(_ns_sess * screen, char *p)
{
    _ns_efuns *efuns;
    _ns_disp *disp;
    char *p2;
    int n, ret = NS_SUCC, type = (strlen(p) > 1) ? NS_SCREEN_STATUS : NS_SCREEN_ST_CLR;

    /* a screen display can disappear because the program in it dies, or
       because we explicitly ask screen to kill the display.  in the latter
       case, screen messages upon success.  rather than explicitly killing
       the disp-struct here, we force a status-line update instead (in which
       the status-line checker will notice the disp has gone, and delete it
       from the struct-list).  this way, we won't need to duplicate the
       delete-logic here. */
    if (sscanf(p, "Window %d (%s) killed.", &n, p2) == 2) {
        size_t x = strlen(p2);
        ret = ns_upd_stat(screen);
    } else {                    /* ignoble message */
        NS_IF_EFUN_EXISTS(efuns, screen, NULL, err_msg)
            ret = efuns->err_msg(NULL, type, (type == NS_SCREEN_STATUS) ? p : "");
    }
    return ret;
}



/* parse the "hardstatus"-line of screens.
   this is, and unfortunately has to be, a ton of heuristics.
   I'm pretty sure there will be (esoteric) situations that are not handled
   (correctly) by this code, particularly in connection with more sessions
   than can be enumerated in the status-line (we do have workarounds for
   that case, they're just not very well tested yet).
   do not touch this unless you are absolutely sure you know what you're
   doing.   2002/05/01  Azundris  <hacks@azundris.com>

   screen   the session associated with that instance of screen,
            as returned by ns_attach_by_URL() and related.
            the session must contain a valid struct of callbacks (efuns),
            as certain functionalities ("add a tab", "show status message")
            may be called from here.
   force    the terminal wants us to update.  if it doesn't, we may see
            fit to do so anyway in certain cases.
   width    the terminal's width in columns (== that of the status line)
   p        the pointer to the status line.  may point into the terminal's
            line buffer if that holds plain text data (not interleaved with
            colour- and boldness-data)
   <-       returns an error code. */

int
parse_screen(_ns_sess * screen, int force, int width, char *p)
{
    char *p4, *p3, *p2;         /* pointers for parser magic */
    static const char *p5 = NS_SCREEN_FLAGS;
    static size_t l = sizeof(NS_SCREEN_FLAGS);
#if (NS_SCREEN_UPD_FREQ>0)
    static time_t t = 0;
    time_t t2 = time(NULL);
#endif
    int ret = NS_SUCC, tmp, status_blanks = 0,	/* status-bar overflow? */
     parsed,                    /* no of *visible* elements in status line */
     n,                         /* screen's index (immutable, sparse) */
     r;                         /* real index (r'th element) */
    _ns_efuns *efuns;
    _ns_disp *disp = NULL, *d2 = NULL;

    if (!p)
        return NS_FAIL;

    if (!force)
        return NS_SUCC;

#if (NS_SCREEN_UPD_FREQ>0)
    if ((t2 - t) > NS_SCREEN_UPD_FREQ) {
        (void) ns_upd_stat(screen);
        t = t2;
    }
#endif

    if (p = strdup(p)) {
        _ns_parse pd[NS_MAX_DISPS];
        p2 = &p[width - 1];
        while (p2 > p && *p2 == ' ') {
            status_blanks++;
            *(p2--) = '\0';
        }                       /* p2 now points behind last item */

#ifdef NS_DEBUG
        fprintf(stderr, "::%s::\n", p);
#endif

#ifdef NS_PARANOID_
        if (strlen(p) < 2) {    /* special case: display 0 */
            disp = screen->dsps;	/* might not get a status-line in d0! */
            if (disp && !(disp->flags & NS_SCREAM_CURR)) {	/* flags need updating */
                disp->flags |= NS_SCREAM_CURR;	/* set flag to avoid calling inp_text */
                ret = ns_upd_stat(screen);
            }                   /* more thn once */
            free(p);
            return ret;
        }
#endif

        p3 = p;
        while (isspace(*p3))    /* skip left padding */
            p3++;

        if (isdigit(*p3)) {     /* list of displays */
            parsed = r = 0;
            do {
                n = atoi(p3);
                pd[parsed].name = NULL;
                pd[parsed].screen = n;
                pd[parsed].real = r++;

                while (isdigit(*p3))	/* skip index */
                    p3++;

                pd[parsed].flags = 0;	/* get and skip flags */
                while (*p3 && *p3 != ' ') {
                    for (n = 0; n < l; n++) {
                        if (*p3 == p5[n]) {
                            pd[parsed].flags |= (1 << n);
                            break;
                        }
                    }
                    p3++;
                }

                if (*p3 == ' ') {	/* skip space, read name */
                    *(p3++) = '\0';
                    p4 = p3;
                    while (p3[0] && p3[1] && (p3[0] != ' ' || p3[1] != ' '))
                        p3++;
                    if (p3[0] == ' ') {
                        *(p3++) = '\0';
                        while (isspace(*p3))
                            p3++;
                    }
                    pd[parsed++].name = p4;
                    if (parsed >= NS_MAX_DISPS)
                        p3 = &p3[strlen(p3)];
                } /* out of mem => skip remainder */
                else
                    p3 = &p3[strlen(p3)];	/* weirdness  => skip remainder */
            } while (*p3);

#ifdef NS_DEBUG
            for (r = 0; r < parsed; r++)
                if (pd[r].name)
                    printf("%d(%d/%d,%s)  ", r, pd[r].screen, pd[r].real, pd[r].name);
            puts("\n");
#endif

            for (r = 0; r < parsed; r++) {
                n = pd[r].screen;
                disp = disp_fetch(screen, n);

                if (!disp) {    /* new display */
                    if (!(disp = disp_fetch_or_make(screen, n)) || !(disp->name = strdup(pd[r].name))) {
                        fprintf(stderr, "out of memory in parse_screen::new_display(%d)\n", n);
                        ret = NS_FAIL;
                    } else {
                        NS_IF_EFUN_EXISTS(efuns, screen, NULL, ins_disp)
                            ret = efuns->ins_disp(screen->userdef, pd[r].real - 1, disp->name);
                    }
                } else if ((tmp = strcmp(disp->name, pd[r].name)) ||	/* upd display */
                           (disp->flags != pd[r].flags)) {
                    if (tmp) {
                        free(disp->name);
                        if (!(disp->name = strdup(pd[r].name))) {
                            free(p);
                            return NS_FAIL;
                        }
                    }
                    if (pd[r].flags & NS_SCREAM_CURR)
                        disp->sess->curr = disp;
                    disp->flags = pd[r].flags & NS_SCREAM_MASK;
                    NS_IF_EFUN_EXISTS(efuns, screen, NULL, upd_disp)
                        ret = efuns->upd_disp(screen->userdef, r, disp->flags, disp->name);
                }

                /* remove any displays from list that have disappeared
                   from the middle of the status-line */
                if (!d2 || d2->next != disp) {	/* remove expired displays */
                    _ns_disp *d3 = disp->prvs, *d4;
                    while (d3 && d3 != d2) {
#ifdef NS_DEBUG
                        fprintf(stderr, "remove expired middle %d \"%s\"...\n", d3->index, d3->name);
#endif
                        d4 = d3->prvs;
                        NS_IF_EFUN_EXISTS(efuns, screen, NULL, del_disp)
                            ret = efuns->del_disp(screen->userdef, disp_get_real_by_screen(screen, d3->index));
                        disp_kill(d3);
                        d3 = d4;
                    }
                    if (!d2)
                        ns_upd_stat(screen);
                }
                d2 = disp;
            }



#ifdef NS_PARANOID
            if (!r) {
#  ifdef NS_DEBUG
                if (!(err_inhibit & NS_ERR_WEIRDSCREEN)) {
                    err_inhibit |= NS_ERR_WEIRDSCREEN;
                    fprintf(stderr, "libscream::parse_screen()  d2==NULL\n"
                            "This should never happen. It is assumed that you use a\n"
                            "rather unusual configuration for \"screen\".   Please\n"
                            "send the result of 'screen --version' to <escreen@azundris.com>\n"
                            "(together with your ~/.screenrc and /etc/screenrc if present).\n"
                            "If at all possible, please also run 'Eterm -e screen' and make\n"
                            "a screenshot of the offending window (and the window only, the\n"
                            "beauty of your desktop is not relevant to this investigation. : ).\n");
                }
#  endif
                ret = ns_upd_stat(screen);
                free(p);
                return NS_FAIL;
            } else
#endif
                /* kill overhang (o/t right) if status-line isn't side-scrolling
                   (as it will if not all the disp names fit in the status-line) */
            if (disp->next && status_blanks > (strlen(disp->next->name) + 6)) {
                _ns_disp *d3 = disp;
                for (disp = disp->next; disp;) {
#ifdef NS_DEBUG
                    fprintf(stderr, "remove expired right %d \"%s\"...\n", disp->index, disp->name);
#endif
                    d2 = disp;
                    if (d2->sess->curr == d2)
                        d2->sess->curr = d3;
                    disp = disp->next;
                    NS_IF_EFUN_EXISTS(efuns, screen, NULL, del_disp)
                        ret = efuns->del_disp(screen->userdef, disp_get_real_by_screen(screen, d2->index));
                    disp_kill(d2);
                }
                d3->next = NULL;
            }
        }

        else                    /* not a list of displays, but a message. handle separately. */
            ret = parse_screen_msg(screen, p);

        free(p);
    }
    /* release our (modified) copy of the status-line */
    return ret;
}



/***************************************************************************/
