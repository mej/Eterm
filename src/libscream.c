/****************************************************************************
 * scream::libscream.c                         Azundris <scream@azundris.com>
 *
 * routines to connect to screen and or scream daemons.
 * libscream is a double-transparency layer -- it abstracts the backend
 * (screen or a replacement, locally or ssh-tunneled) to the front-end
 * (a terminal-emulation such as Eterm) and vice versa.
 *
 * Lesser GNU Public Licence applies.
 * Thread-safe:  untested
 * 2002/04/19  Azundris  incept
 * 2002/05/04  Azundris  support for esoteric screens, thanks to Till
 * 2002/05/12  Azundris  edit display names, send statement, tab completion
 * 2002/05/13  Azundris  ssh tunnel through firewall
 * 2002/05/17  Azundris  supports systemwide screenrc (thanks mej)
 * 2002/05/18  Azundris  remote handling improved (thanks tillsan, tfing)
 ***************************************************************************/

#include "config.h"
#include "src/feature.h"

#include <stdio.h>              /* stderr, fprintf, snprintf() */
#include <string.h>             /* bzero() */
#include <pwd.h>                /* getpwuid() */
#include <sys/types.h>          /* getpwuid() */
#include <sys/stat.h>           /* stat() */
#include <unistd.h>             /* getuid() */
#include <stdlib.h>             /* atoi() */
#include <netdb.h>              /* getservbyname() */
#include <netinet/in.h>         /* ntohs() */
#include <limits.h>             /* PATH_MAX */
#include <ctype.h>              /* isspace() */
#include <errno.h>              /* errno */

#include <libast.h>

#include "scream.h"             /* structs, defs, headers */
#include "screamcfg.h"          /* user-tunables */

#ifndef MAXPATHLEN
#  ifdef PATH_MAX
#    define MAXPATHLEN PATH_MAX
#  elif defined(MAX_PATHLEN)
#    define MAXPATHLEN MAX_PATHLEN
#  endif
#endif



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
#define NS_EFUN_EXISTS(p,s,d,e)  (((p) = ns_get_efuns((s),(d))) && ((p)->e))



/***************************************************************************/
/* module-global vars */
/**********************/



static long err_inhibit = 0;    /* bits. avoid telling same error twice. */
static _ns_sess *sa = NULL;     /* anchor for session list */
static _ns_hop *ha = NULL;      /* anchor for hop list */



/***************************************************************************/
/* constructors/destructors */
/****************************/



/* ns_new_hop.  create and initialize a hop struct.
   lp    local port.  if 0:  if otherwise matching hop exists, reuse that.
                             otherwise, find the first free (as in, not used
                             by us) port, starting with NS_MIN_PORT.
   fw    firewall machine.  numeric or symbolic.
   fp    foreign port. if 0: default to SSH port.
   delay wait n seconds for tunnel to come up before trying to use it
   s     the session to add the hop to
   <-    a matching (existing or newly created) hop structure, or NULL */

_ns_hop *
ns_new_hop(int lp, char *fw, int fp, int delay, _ns_sess * s)
{
    _ns_hop *h = ha;

    if (!fw || !*fw)
        return NULL;

    if (!fp)
        fp = get_ssh_port();    /* remote port defaults to SSH */

    if (s) {
        /* see if we already have a matching hop. */
        while (h && !(((h->localport == lp) || (!lp)) &&
                      (!strcmp(h->fw, fw)) && (h->fwport == fp) && (h->sess->port == s->port) && (!strcmp(h->sess->host, s->host))))
            h = h->next;

        if (h) {
            if (delay)
                h->delay = delay;	/* may change delay! */
            h->refcount++;
            return h;
        }
    }

    h = MALLOC(sizeof(_ns_hop));
    if (h) {
        bzero(h, sizeof(_ns_hop));
        if ((h->fw = strdup(fw))) {
            if (!lp) {
                lp = NS_MIN_PORT;	/* local port defaults to */
                if (ha) {       /* NS_MIN_PORT. if that's */
                    int f;      /* taken, use next free port. */
                    do {        /* free as in, not used by us. */
                        _ns_hop *i = ha;
                        f = 0;
                        while (i)
                            if (i->localport == lp) {
                                f = 1;
                                lp++;
                                i = NULL;
                            } else
                                i = i->next;
                    } while (f);
                }
            }
            h->delay = (delay ? delay : NS_TUNNEL_DELAY);
            h->localport = lp;
            h->fwport = fp;
            h->refcount++;
            h->next = ha;
            h->sess = s;
            ha = h;
        } else {
            FREE(h);
            return NULL;
        }
    }

    return h;
}



/* ns_dst_hop.  deref (and, where necessary, release) a hop struct.
   if sp is provided, additional integrity magic will take place.
   ss  hop to deref/free
   sp  session that the hop used to belong to (NULL for none (as if))
   <-  NULL */

_ns_hop *
ns_dst_hop(_ns_hop ** ss, _ns_sess * sp)
{
    if (ss && *ss) {
        _ns_hop *s = *ss;

#ifdef NS_DEBUG_MEM
        if (s->refcount <= 0) {
            fprintf(stderr, NS_PREFIX "ns_dst_hop: leak alert -- trying to double-free hop...\n");
            return NULL;
        }
#endif

        if (!--(s->refcount)) { /* was last ref to hop => free hop */
            if (s->fw)
                FREE(s->fw);
#ifdef NS_DEBUG_MEM
            bzero(s, sizeof(_ns_hop));
#endif
            if (ha == s)        /* delist */
                ha = s->next;
            else {
                _ns_hop *h = ha;
                while (h && h->next != s)
                    h = h->next;
                if (h)
                    h->next = s->next;
            }
            FREE(s);
        } else if (sp && sp->hop == s) {
            /* hop shouldn't point back at a session that just dereffed it
               as it's probably about to die. fix the back ref to a session
               that's actually valid. */
            _ns_sess *p = sa;
            while (p && ((p == sp) || (p->port != sp->port) || (strcmp(p->host, sp->host))))
                p = p->next;
            if (!p)
                ns_desc_hop(s,
                            NS_PREFIX
                            "ns_dst_sess: Leak alert -- found a hop that is only\n referenced once, but has a refcount > 1. Hop data follow");
            else
                s->sess = p;
        }
        *ss = NULL;
    }
    return NULL;
}



_ns_efuns *
ns_new_efuns(void)
{
    _ns_efuns *s = MALLOC(sizeof(_ns_efuns));
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
        *ss = NULL;
        if (!--(s->refcount)) {
#ifdef NS_DEBUG_MEM
            bzero(s, sizeof(_ns_efuns));
#endif
            FREE(s);
        }
    }
    return NULL;
}



_ns_disp *
ns_new_disp(void)
{
    _ns_disp *s = MALLOC(sizeof(_ns_disp));
    if (s) {
        bzero(s, sizeof(_ns_disp));
    }
    return s;
}

_ns_sess *ns_dst_sess(_ns_sess **);	/* forward, sorry */

_ns_disp *
ns_dst_disp(_ns_disp ** ss)
{
    if (ss && *ss) {
        _ns_disp *s = *ss;
        if (s->name)
            FREE(s->name);
        if (s->efuns)
            ns_dst_efuns(&(s->efuns));
        if (s->child)           /* nested screen? */
            ns_dst_sess(&(s->child));	/* forward, sorry */
        *ss = NULL;
#ifdef NS_DEBUG_MEM
        bzero(s, sizeof(_ns_disp));
#endif
        FREE(s);
    }
    return NULL;
}

_ns_disp *
ns_dst_dsps(_ns_disp ** ss)
{
    if (ss && *ss) {
        _ns_disp *s = *ss, *t;
        *ss = NULL;
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
    _ns_sess *s = MALLOC(sizeof(_ns_sess));
    if (s) {
        bzero(s, sizeof(_ns_sess));
        s->escape = NS_SCREEN_ESCAPE;	/* default setup for the screen program */
        s->literal = NS_SCREEN_LITERAL;
        s->dsbb = NS_SCREEN_DEFSBB;
        s->delay = NS_INIT_DELAY;
        if (sa) {               /* add to end of list */
            _ns_sess *r = sa;
            while (r->next)
                r = r->next;
            r->next = s;
        } else
            sa = s;
    }
    return s;
}

_ns_sess *
ns_dst_sess(_ns_sess ** ss)
{
    if (ss && *ss) {
        _ns_sess *s = *ss;
        ns_dst_dsps(&(s->dsps));
        if (s->hop)
            ns_dst_hop(&(s->hop), s);
        if (s->host)
            FREE(s->host);
        if (s->user)
            FREE(s->user);
        if (s->pass)
            FREE(s->pass);
        if (s->efuns)
            ns_dst_efuns(&(s->efuns));
        if (s->prvs)
            s->prvs->next = s->next;
        else
            sa = s->next;       /* align anchor */
        if (s->next)
            s->next->prvs = s->prvs;
        *ss = NULL;
#ifdef NS_DEBUG_MEM
        bzero(s, sizeof(_ns_sess));
#endif
        FREE(s);
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
    _ns_efuns *efuns;
    char *c;
    int ret = NS_SUCC;

    if (!cmd || !*cmd) {
        return NS_FAIL;
    }

    if (NS_EFUN_EXISTS(efuns, sess, NULL, inp_text)) {
        if ((c = strdup(cmd))) {
            char *p;            /* replace default escape-char with that */

            for (p = c; *p; p++) {	/* actually used in this session */
                if (*p == NS_SCREEN_ESCAPE) {
                    *p = sess->escape;
                }
            }
#ifdef NS_DEBUG
            ns_desc_string(c, "ns_screen_command: xlated string");
#endif
            efuns->inp_text(NULL, sess->fd, c);
            FREE(c);
        } else {
            /* out of memory */
            ret = NS_OOM;
        }
    } else {
        ret = NS_EFUN_NOT_SET;
        fprintf(stderr, NS_PREFIX "ns_screen_command: sess->efuns->inp_text not set!\n");
    }
    return ret;
}



/* send a single command string to screen, adding the equiv of ^A:
   s     the session
   cmd   the command string
   <-    error code */

int
ns_screen_xcommand(_ns_sess * s, char prefix, char *cmd)
{
    char *i;
    int ret = NS_OOM;
    if ((i = MALLOC(strlen(cmd) + 4))) {
        size_t l = strlen(cmd) + 2;
        strcpy(&i[2], cmd);
        i[0] = s->escape;
        i[1] = prefix;
        i[l] = '\n';
        i[++l] = '\0';
        ret = ns_screen_command(s, i);
        FREE(i);
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



/* ns_input_dialog
   open a dialog
   s        the session
  !retstr   where we'll store a pointer to the result (the user's input)
   prompt   the prompt to appear in the dialog box
   <-       msg */


int
ns_input_dialog(_ns_sess * s, char *prompt, int maxlen, char **retstr, int (*inp_tab) (void *, char *, size_t, size_t))
{
    _ns_efuns *efuns;
    char *c;
    int ret = NS_SUCC;

    if (NS_EFUN_EXISTS(efuns, s, NULL, input_dialog)) {
        (void) efuns->input_dialog((void *) s, prompt, maxlen, retstr, inp_tab);
    } else {
        ret = NS_EFUN_NOT_SET;
        fprintf(stderr, NS_PREFIX "ns_screen_command: sess->efuns->input_dialog not set!\n");
    }
    return ret;
}



/***************************************************************************/
/* attach/detach */
/*****************/



/* ns_sess_init
   init an opened session (transmit .screenrc, or whatever)
   sess  the session
   <-    error code */

int
ns_sess_init(_ns_sess * sess)
{
    if ((sess->backend == NS_MODE_NEGOTIATE) || (sess->backend == NS_MODE_SCREEN)) {
        (void) ns_parse_screenrc(sess, sess->sysrc, NS_ESC_SYSSCREENRC);
        return ns_parse_screenrc(sess, sess->home, NS_ESC_SCREENRC);
    }
    return NS_SUCC;
}



/* return port number for service SSH (secure shell).
   <-  a port number -- 22 in all likelihood. */

int
get_ssh_port(void)
{
    static int port = 0;
    struct servent *srv;
    if (port)
        return port;
    /* (fixme) replace with getservbyname_r on systems that have it */
    srv = getservbyname("ssh", "tcp");
    return (port = (srv ? ntohs(srv->s_port) : NS_DFLT_SSH_PORT));
}




/* ns_parse_hop
   parse a hop-string into a hop-struct
   h:   one of NULL lclport:fw:fwport fw:fwport lclport:fw
        if set, describes how to tunnel through a fw to access an URL
        describing a target behind said firewall
   <-   a hop struct, or NULL
*/

_ns_hop *
ns_parse_hop(_ns_sess * s, char *h)
{
    char *p = h, *e, *fw = NULL;
    int f = 0, v, lp = 0, fp = 0, delay = 0;

    if (!h || !*h)
        return NULL;

    if ((e = strrchr(h, ','))) {
        *(e++) = '\0';
        if (*e)
            delay = atoi(e);
    }

    while (*p && *p != ':')
        if (!isdigit(*(p++)))
            f = 1;

    if (!*p)                    /* fw only */
        return ns_new_hop(lp, h, fp, delay, s);

    if (!f) {                   /* lp:fw... */
        if (!(v = atoi(h)))
            return NULL;
        lp = v;
        e = ++p;
        while (*e && *e != ':')
            e++;
        if (*e) {
            *(e++) = '\0';
            if (!(v = atoi(e)))
                return NULL;
            fp = v;
        }
    } else {                    /* fw:fp */
        *(p++) = '\0';
        if (!(v = atoi(p)))
            return NULL;
        fp = v;
        p = h;
    }
    return ns_new_hop(lp, p, fp, delay, s);
}



/* ns_desc_string
   c        the string
   doc      context-info
   !stdout  the string, in human-readable form */

void
ns_desc_string(char *c, char *doc)
{
    char *p = c;

    if (doc)
        fprintf(stderr, NS_PREFIX "%s: ", doc);

    if (!c) {
        fputs("NULL\n", stderr);
        return;
    } else if (!*c) {
        fputs("empty\n", stderr);
        return;
    }

    while (*p) {
        if (*p < ' ')
            fprintf(stderr, "^%c", *p + 'A' - 1);
        else
            fputc(*p, stderr);
        p++;
    }

    fputs("\n", stderr);

    return;
}



/* ns_desc_hop
   print basic info about a hop (tunnel, firewall).  mostly for debugging.
   hop:    a hop struct as generated by (eg) ns_attach_by_URL()
   doc:    info about the context
 ! stderr: info about the hop */

void
ns_desc_hop(_ns_hop * h, char *doc)
{
    if (!h && doc) {
        fprintf(stderr, NS_PREFIX "%s: ns_desc_hop called with broken pointer!\n", doc);
        return;
    }

    if (doc)
        fprintf(stderr, NS_PREFIX "%s:\n", doc);

    fprintf(stderr, NS_PREFIX "tunnel from localhost:%d to %s:%d to %s:%d is %s.  (delay %d, %d ref%s)\n",
            h->localport, h->fw, h->fwport,
            h->sess->host, h->sess->port, h->established ? "up" : "down", h->delay, h->refcount, h->refcount == 1 ? "" : "s");
}



/* ns_desc_sess
   print basic info about a session.  mostly for debugging.
   sess:   a session struct as generated by (eg) ns_attach_by_URL()
   doc:    info about the context
 ! stderr: info about the session */

void
ns_desc_sess(_ns_sess * sess, char *doc)
{
    if (!sess) {
        fprintf(stderr, NS_PREFIX "%s: ns_desc_sess called with broken pointer!\n", doc);
        fflush(stderr);
        return;
    }
    if (sess->where == NS_LCL)
        fprintf(stderr, NS_PREFIX "%s: (efuns@%p)\t (user %s) local %s", doc, sess->efuns, sess->user, sess->proto);
    else {
        fprintf(stderr, NS_PREFIX "%s: (efuns@%p)\t %s://%s%s%s@%s",
                doc, sess->efuns, sess->proto, sess->user, sess->pass ? ":" : "", sess->pass ? sess->pass : "", sess->host);
        if (sess->port != NS_DFLT_SSH_PORT)
            fprintf(stderr, ":%s", sess->port);
    }
    fprintf(stderr, "%c%s\n", sess->where == NS_LCL ? ' ' : '/', sess->rsrc);
    if (sess->hop)
        ns_desc_hop(sess->hop, NULL);
    if (sess->sysrc)
        fprintf(stderr, NS_PREFIX "info: searching for sysrc in %s\n", sess->sysrc);
    if (sess->home)
        fprintf(stderr, NS_PREFIX "info: searching for usrrc in %s\n", sess->home);
    fprintf(stderr, NS_PREFIX "info: escapes set to ^%c-%c\n", sess->escape + 'A' - 1, sess->literal);
    fflush(stderr);
}



/* run a command. uses the terminal's internal run facility.
   converts system/"char *" to exec/"arg **".
   efuns: struct of callbacks into the terminal program.
          ns_run() will fail if no callback to the terminal's "run program"
          (exec) facility is provided.
   cmd:   a string to exec
   <-     whatever the callback returns.  In Eterm, it's a file-descriptor.  */

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
        fprintf(stderr, NS_PREFIX "ns_run: executing \"%s\"...\n", cmd);
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

        if (!(args = MALLOC((n + 2) * sizeof(char *))))
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
        FREE(args);
    return n;

  fail:
    return NS_FAIL;
}



/* create a call line. used in ns_attach_ssh/lcl
   tmpl   the template. should contain one %s
   dflt   the default value
   opt    the user-supplied value (or NULL)
   <-     a new malloc'd string (or NULL) */

char *
ns_make_call_el(char *tmpl, char *dflt, char *opt)
{
    size_t l, r;
    char *p;

    if (tmpl && dflt && *tmpl && strstr(tmpl, "%s")) {
        l = strlen(tmpl) + (opt ? strlen(opt) : strlen(dflt)) - 1L;
        if ((p = MALLOC(l))) {
            r = snprintf(p, l, tmpl, opt ? opt : dflt);
            if ((r >= 0) && (r < l)) {
                return p;
            }
            FREE(p);
        }
    }
    return NULL;
}



char *
ns_make_call(_ns_sess * sess)
{
    char *call, *tmp = NULL, *screen = NULL, *scream = NULL, *screem = NULL;

    /* unless decidedly in other mode... */
    if (sess->backend != NS_MODE_SCREEN)
        tmp = scream = ns_make_call_el(NS_SCREAM_CALL, NS_SCREAM_OPTS, sess->rsrc);
    if (sess->backend != NS_MODE_SCREAM)
        tmp = screen = ns_make_call_el(NS_SCREEN_CALL, NS_SCREEN_OPTS, sess->rsrc);
    if (sess->backend == NS_MODE_NEGOTIATE) {
        size_t r, l = strlen(NS_SCREEM_CALL) + strlen(scream) + strlen(screen) - 3;
        if ((screem = MALLOC(l))) {
            r = snprintf(screem, l, NS_SCREEM_CALL, scream, screen);
#ifdef NS_PARANOID
            if ((r < 0) || (r > l)) {
                FREE(screem);
            }
#endif
        }
        tmp = screem;
    }
    call = ns_make_call_el(NS_WRAP_CALL, tmp, NULL);
    FREE(screen);
    FREE(scream);
    FREE(screem);
    return call;
}



/* attach a local session (using screen/scream)
   sp  the session
   <-  NS_FAIL, or the result of ns_run() */

int
ns_attach_lcl(_ns_sess ** sp)
{
    _ns_sess *sess;
    char *call;
    int ret = -1;

    if (!sp || !*sp)
        return ret;

    sess = *sp;

    if (call = ns_make_call(sess)) {
        char *c2 = ns_make_call_el("/bin/sh -c \"%s\"", call, NULL);
        FREE(call);
        if (c2) {
            ret = ns_run(sess->efuns, c2);
            FREE(c2);
        }
    }
    return ret;
}



/* attach a remote session (using screen/scream via ssh)
   sp  the session
   <-  -1, or the result of ns_run() */

int
ns_attach_ssh(_ns_sess ** sp)
{
    _ns_sess *sess;
    char cmd[NS_MAXCMD + 1];
    char *call;
    int ret;

    if (!sp || !*sp)
        return NS_FAIL;

    sess = *sp;

    call = ns_make_call(sess);

    if (sess->hop) {
        if (sess->hop->established == NS_HOP_DOWN) {	/* the nightmare foe */
            ret = snprintf(cmd, NS_MAXCMD, "%s %s -p %d -L %d:%s:%d %s@%s",
                           NS_SSH_CALL, NS_SSH_TUNNEL_OPTS,
                           sess->hop->fwport, sess->hop->localport, sess->host, sess->port, sess->user, sess->hop->fw);
            if (ret < 0 || ret > NS_MAXCMD)
                return NS_FAIL;
            ns_run(sess->efuns, cmd);
            sleep(sess->hop->delay);
        }
        ret = snprintf(cmd, NS_MAXCMD, "%s %s -p %d %s@localhost \"%s -e^%c%c\"",
                       NS_SSH_CALL, NS_SSH_OPTS, sess->hop->localport, sess->user, call, sess->escape + 'A' - 1, sess->literal);
    } else {
        ret =
            snprintf(cmd, NS_MAXCMD, "%s %s -p %d %s@%s \"%s -e^%c%c\"", NS_SSH_CALL, NS_SSH_OPTS, sess->port, sess->user, sess->host, call,
                     sess->escape + 'A' - 1, sess->literal);
    }
    FREE(call);
#ifdef NS_DEBUG
    fprintf(stderr, "\n\n>>%s\n>>%s\n\n", call, cmd);
    fflush(stderr);
#endif
    return (ret < 0 || ret > NS_MAXCMD) ? NS_FAIL : ns_run(sess->efuns, cmd);
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

    (void) ns_sess_init(sess);

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
    fprintf(stderr, NS_PREFIX "ns_attach_by_sess: screen session-fd is %d, ^%c-%c\n", sess->fd, sess->escape + 'A' - 1, sess->literal);
#endif

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
   hop:    one of NULL lclport:fw:fwport fw:fwport lclport:fw
           if set, describes how to tunnel through a fw to access an URL
           describing a target behind said firewall
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
ns_attach_by_URL(char *url, char *hop, _ns_efuns ** ef, int *err, void *xd)
{
    int err_dummy;
    char *p, *d = NULL;
    _ns_sess *sess = ns_new_sess();
    struct passwd *pwe = getpwuid(getuid());

    if (!err)
        err = &err_dummy;
    *err = NS_OOM;

    if (!sess)
        return NULL;

    if (url && strlen(url)) {
        char *q;

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

        if ((p = strchr(q, '/'))) {
            *(p++) = '\0';
            if (strlen(p)) {
                char *r = p;
                int f;
                while (*r) {
                    if (*r == '+')
                        *(r++) = ' ';
                    else if ((*r == '%') && (strlen(r) > 2)) {
                        long v;
                        char *e;
                        char b[3];
                        b[0] = r[1];
                        b[1] = r[2];
                        b[2] = '\0';
                        v = strtol(b, &e, 16);
                        if (!*e) {
                            *(r++) = (char) (v & 0xff);
                            memmove(r, &r[2], strlen(&r[2]));
                        }
                    } else
                        r++;
                }
                r = p;
                f = 0;
                while (*r) {
                    if (*r == ' ') {	/* Padding between arguments */
                        while (*r == ' ')
                            r++;
                    } else {
                        if (*r == '-') {
                            if (*(++r) == 'e') {	/* set escape */
                                char x = 0, y = 0;
                                while (*(++r) == ' ');
                                if ((x = ns_parse_esc(&r)) && (y = ns_parse_esc(&r))) {
                                    sess->escape = x;
                                    sess->literal = y;
                                    sess->escdef = NS_ESC_CMDLINE;
                                }
                            } else if (*r == 'c') {	/* alt screenrc */
                                char *rc, *rx;
                                while (*(++r) == ' ');
                                if ((rx = strchr(r, ' ')))
                                    *rx = '\0';
                                if (*r != '/')
                                    fprintf(stderr, NS_PREFIX "URL: path for screen's option -c should be absolute (%s)\n", r);
                                if ((rc = strdup(r))) {
                                    if (sess->home)	/* this should never happen */
                                        FREE(sess->home);
#ifdef NS_DEBUG
                                    fprintf(stderr, NS_PREFIX "URL: searching for rc in %s\n", rc);
#endif
                                    sess->home = rc;
                                }
                                if (rx) {
                                    r = rx;
                                    *rx = ' ';
                                }
                            }
                            while (*r && (f || *r != ' ')) {
                                if (*r == '\"')
                                    f = 1 - f;
                                r++;
                            }
                        }
                        while (*r && *r != ' ')	/* proceed to space */
                            r++;
                    }
                }

                if (!(sess->rsrc = strdup(p)))
                    goto fail;
            }
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

        FREE(d);
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
        if (!(pwe = getpwnam(sess->user)) && !sess->host && !sess->port) {
            *err = NS_UNKNOWN_USER;
            goto fail;
        }
    }

    if (getenv("SYSSCREENRC")) {	/* $SYSSCREENRC */
        if (!(sess->sysrc = strdup(getenv("SCREENRC"))))
            goto fail;
    } else {
        char *loc[] = { "/usr/local/etc/screenrc",	/* official */
            "/etc/screenrc",    /* actual (on SuSE) */
            "/usr/etc/screenrc",
            "/opt/etc/screenrc"
        };
        int n, nloc = sizeof(loc) / sizeof(char *);
        for (n = 0; n < nloc; n++)
            if (!access(loc[n], R_OK)) {
                if (!(sess->sysrc = strdup(loc[n])))
                    goto fail;
                n = nloc;
            }
    }

    if (getenv("SCREENRC")) {   /* $SCREENRC */
        sess->home = strdup(getenv("SCREENRC"));
    } else if (pwe && !sess->home) {	/* ~/.screenrc */
        if ((sess->home = MALLOC(strlen(pwe->pw_dir) + strlen(NS_SCREEN_RC) + 2)))
            sprintf(sess->home, "%s/%s", pwe->pw_dir, NS_SCREEN_RC);
    } else
        goto fail;

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
        if (!(sess->proto = strdup("screXX")))
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

    if (hop && strlen(hop)) {
        sess->hop = ns_parse_hop(sess, hop);
        if (sess->hop && (!strcmp(sess->host, sess->hop->fw) || !strcmp(sess->host, "localhost") || !strcmp(sess->host, "127.0.0.1")))
            fprintf(stderr, NS_PREFIX "ns_attach_by_URL: routing in circles...\n");
    }

    *err = NS_SUCC;
    return ns_attach_by_sess(&sess, err);

  fail:
    if (d)
        FREE(d);

    return ns_dst_sess(&sess);
}



/* detach a session and release its memory
   sess  the session
   <-    error code */
int
ns_detach(_ns_sess ** sess)
{
#ifdef NS_DEBUG
    ns_desc_sess(*sess, "ns_detach");
#endif
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



/* function that will open a dialog */
void
ns_register_inp(_ns_efuns * efuns, int (*input_dialog) (void *, char *, int, char **, int (*)(void *, char *, size_t, size_t)))
{
    efuns->input_dialog = input_dialog;
}



/* function that will handle tab-completion in a dialog */
void
ns_register_tab(_ns_efuns * efuns, int (*inp_tab) (void *, char *[], int, char *, size_t, size_t))
{
    efuns->inp_tab = inp_tab;
}



/* function that will do whatever while waiting */
void
ns_register_fun(_ns_efuns * efuns, int (*inp_fun) (void *, int))
{
    efuns->waitstate = inp_fun;
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

    if (!d->sess->curr)         /* note as current on session if first display */
        d->sess->curr = d;

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



/* tab completion for screen-commands
  !b  current entry (changes)
   l  number of characters to compare in current entry
   m  maximum number of characters in entry (size of input buffer)
   <- error code */

int
ns_inp_tab(void *xd, char *b, size_t l, size_t m)
{
    char *sc[] = { "acladd", "addacl", "aclchg", "chacl", "acldel", "aclgrp",
        "aclumask", "umask", "activity",
        "allpartial", "at", "attrcolor", "autonuke", "bce",
        "bell_msg", "bind", "bindkey", "break", "breaktype",
        "bufferfile", "c1", "caption", "charset", "chdir",
        "clear", "compacthist", "console", "copy",
        "crlf", "debug", "defc1", "defautonuke", "defbce",
        "defbreaktype", "defcharset", "defflow", "defgr",
        "defencoding", "deflog", "deflogin", "defmode",
        "defmonitor", "defobuflimit", "defscrollback",
        "defshell", "defsilence", "defslowpast", "defutf8",
        "defwrap", "defwritelock", "defzombie", "detach",
        "dinfo", "displays", "digraph", "dumptermcap",
        "escape", "eval", "exec", "fit", "flow", "focus", "gr",
        "hardcopy", "hardcopy_append", "hardcopydir",
        "height", "help", "history", "ignorecase", "encoding",
        "kill", "license", "lockscreen", "log", "logfile",
        "login", "logtstamp", "mapdefault", "mapnotnext",
        "maptimeout", "markkeys", "meta", "monitor",
        "multiuser", "nethack", "next", "nonblock", "number",
        "obuflimit", "only", "other", "partial", "password",
        "paste", "pastefont", "pow_break", "pow_detach",
        "prev", "printcmd", "process", "quit", "readbuf",
        "readreg", "redisplay", "remove", "removebuf", "reset",
        "resize", "screen", "scrollback", "select",
        "sessionname", "setenv", "setsid", "shell",
        "shelltitle", "silence", "silencewait", "sleep",
        "slowpast", "source", "sorendition", "split", "stuff",
        "su", "suspend", "term", "termcap", "terminfo",
        "termcapinfo", "unsetenv", "utf8", "vbell",
        "vbell_msg", "vbellwait", "verbose", "version",
        "width", "windowlist", "windows", "wrap", "writebuf",
        "writelock", "xoff", "xon", "zombie"
    };

    _ns_efuns *efuns;
    _ns_sess *s = (_ns_sess *) xd;
    int nsc = sizeof(sc) / sizeof(char *);

    if (NS_EFUN_EXISTS(efuns, s, NULL, inp_tab)) {
        return efuns->inp_tab((void *) s, sc, nsc, b, l, m) < 0 ? NS_FAIL : NS_SUCC;
    }

    fprintf(stderr, NS_PREFIX "ns_screen_command: sess->efuns->inp_tab not set!\n");
    return NS_EFUN_NOT_SET;
}



/* parse argument to screen's "escape" statement.
   x   points to the char to process
       screen-manual says this can be one of x ^X \123 or \\ \^ ...
  !x   the pointer is advanced to the next segment (from esc to literal etc.)
   <-  return as char ('\0' -> fail) */

char
ns_parse_esc(char **x)
{
    char r = '\0';

    if (**x == '\\') {
        (*x)++;
        r = **x;
        if (r >= '0' && r <= '7') {	/* octal, otherwise literal */
            char b[4] = "\0\0\0";
            char *e = *x;
            long v;
            size_t l = 0;
            while ((*e >= '0' && *e <= '7') && (l < 3)) {	/* can't use endptr here : ( */
                e++;
                l++;
            }
            *x = &e[-1];
            while (--l)
                b[l] = *(--e);
            r = (char) strtol(b, &e, 8);
        }
    } else if (**x == '^') {
        (*x)++;
        r = **x;
        if (r >= 'A' && r <= 'Z')
            r = 1 + r - 'A';
        else if (r >= 'a' && r <= 'z')
            r = 1 + r - 'a';
        else
            r = '\0';
    } /* malformed */
    else
        r = **x;

    if (**x)
        (*x)++;
    return r;
}



/* ns_parse_screen_cmd
   parse a command the user intends to send to the screen program,
   either via .screenrc or using ^A:
   s       the affected (current) session.  s->current should be set.
   p       the command
   whence  which parsing stage (screenrc, interactive, ...)
   <-  error code */

int
ns_parse_screen_cmd(_ns_sess * s, char *p, int whence)
{
    char *p2;
    long v1 = -1;

    if (!p || !*p)
        return NS_FAIL;

    if ((p2 = strchr(p, ' '))) {	/* first argument */
        char *e;
        while (isspace(*p2))
            p2++;
        v1 = strtol(p2, &e, 0); /* magic conversion mode */
        if ((p2 == e) || (v1 < 0))
            v1 = -1;
    }
#define IS_CMD(b) (strncasecmp(p,b,strlen(b))==0)
    if (!p2) {
        fprintf(stderr, NS_PREFIX "screenrc: ignoring  \"%s\" without an argument...\n", p);
        /* must return success so it's fowarded to screen in interactive mode.
           that way, the user can read the original reply instead of a fake
           one from us. */
        return NS_SUCC;
    } else if (IS_CMD("defescape"))
        fprintf(stderr, NS_PREFIX "screenrc: ignoring  \"defescape\", did you mean \"escape\"?\n");
    else if (IS_CMD("defhstatus") || IS_CMD("hardstatus") || IS_CMD("echo") || IS_CMD("colon") || IS_CMD("wall") ||
#ifdef NS_PARANOID
             IS_CMD("nethack") ||
#endif
             IS_CMD("info") || IS_CMD("time") || IS_CMD("title") || IS_CMD("lastmsg") || IS_CMD("msgwait") || IS_CMD("msgminwait")) {
        fprintf(stderr, NS_PREFIX "screenrc: ignoring  \"%s\", not applicable...\n", p);
        return NS_NOT_ALLOWED;
    } else if (IS_CMD("escape")) {
        char x = 0, y = 0;
        if ((x = ns_parse_esc(&p2)) && (y = ns_parse_esc(&p2))) {
            if (s->escdef == NS_ESC_CMDLINE) {
                fprintf(stderr, NS_PREFIX "screenrc: ignoring  \"escape\"; overridden on command-line...\n", x, y);
                return NS_NOT_ALLOWED;
            } else {
                s->escape = x;
                s->literal = y;
                s->escdef = whence;
                return NS_SUCC;
            }
        } else
            fprintf(stderr, NS_PREFIX "screenrc: ignoring  \"escape\" because of invalid arguments %o %o...\n", x, y);
    } else if (IS_CMD("defscrollback")) {
        if (v1 < NS_SCREEN_DEFSBB)
            fprintf(stderr, NS_PREFIX "screenrc: ignoring  \"%s\" for value < %d...\n", p, NS_SCREEN_DEFSBB);
        else {
            s->dsbb = v1;
            return NS_SUCC;
        }
    } else if (IS_CMD("scrollback")) {
        if (v1 < NS_SCREEN_DEFSBB)
            fprintf(stderr, NS_PREFIX "screenrc: ignoring  \"%s\" for value < %d...\n", p, NS_SCREEN_DEFSBB);
        else {
            if (!s->curr)
                s->curr = s->dsps;
            if (!s->curr)
                fprintf(stderr, NS_PREFIX "screenrc: ignoring  \"%s\", cannot determine current display!?...\n", p);
            else
                s->curr->sbb = v1;
            return NS_SUCC;
        }
    } else {
#ifdef NS_DEBUG
        fprintf(stderr, NS_PREFIX "screenrc: bored now \"%s\"\n", p);
#endif
        return NS_SUCC;
    }
    return NS_FAIL;
}



/* ns_parse_screen_key
   parse and forward a screen-hotkey
   s    the session to forward to
   c    the character following the escape-char.  (when we got here,
        we already got (and threw out) a screen-escape, so we'll have
        to also send one if we ever forward c to the screen program.
   <-   error code */

int
ns_parse_screen_key(_ns_sess * s, char c)
{
    char *i = NULL;
    char b[3];
    int ret = NS_SUCC;
    size_t l;

    b[0] = s->escape;
    b[1] = c;
    b[2] = '\0';

#ifdef NS_DEBUG
    if (c < 27)
        fprintf(stderr, NS_PREFIX "screen_key: ^%c-^%c %d\n", s->escape + 'A' - 1, c + 'A' - 1, c);
    else
        fprintf(stderr, NS_PREFIX "screen_key: ^%c-%c %d\n", s->escape + 'A' - 1, c, c);
#endif

    switch (c) {
      case NS_SCREEN_CMD:      /* send command (statement) to screen server */
          (void) ns_input_dialog((void *) s, "Enter a command to send to the \"screen\" program", 64, &i, ns_inp_tab);
          if (i) {
              if ((ret = ns_parse_screen_cmd(s, i, NS_ESC_INTERACTIVE)) == NS_SUCC) {
                  ret = ns_screen_xcommand(s, c, i);
              } else if (ret == NS_NOT_ALLOWED) {
                  menu_dialog(NULL, "Sorry, David, I cannot allow that.", 0, NULL, NULL);
              }
              FREE(i);
          }
          break;
      case NS_SCREEN_RENAME:   /* rename current display */
          i = s->curr->name;
          l = strlen(i);
          (void) ns_input_dialog(s, "Enter a new name for the current display", 12, &i, NULL);
          if (i && *i) {
              char *n;
              if ((n = MALLOC(strlen(i) + l + 1))) {
                  strcpy(&n[l], i);
                  while (l)
                      n[--l] = '\x08';
                  ret = ns_screen_xcommand(s, c, n);
                  FREE(n);
              }
              FREE(i);
          }
          break;
      default:
          ret = ns_screen_command(s, b);
    }

    return ret;
}



/* ns_parse_screen_interactive
   parse a whole string that may contain screen-escapes that should be
   handled interactively (that should open dialog boxes etc.).
   this will normally be called by menus, buttons etc. that want to send
   input to the add without generating X events for the keystrokes (real
   keystrokes do not come through here; the keyboard-handler should call
   ns_parse_screen_key() directly when it sees the session's escape-char).
   s   the session in question
   c   the string to parse
   <-  error code */

int
ns_parse_screen_interactive(_ns_sess * sess, char *c)
{
    char *s, *p, *o;

    if (!c || !*c)
        return NS_FAIL;
#ifdef NS_PARANOID
    if (!(s = o = strdup(c)))
        return NS_FAIL;
#else
    s = c;
#endif

    p = s;

    while ((p = strchr(s, NS_SCREEN_ESCAPE))) {
        *p = '\0';
        (void) ns_screen_command(sess, s);
        *p = NS_SCREEN_ESCAPE;
        if (*(++p))
            ns_parse_screen_key(sess, *(p++));
        s = p;
    }
    (void) ns_screen_command(sess, s);

#ifdef NS_PARANOID
    FREE(o);
#endif

    return NS_SUCC;
}



/* ns_parse_screenrc -- read the user's screenrc (if we can find it),
   parse it (we need to know if she changes the escapes etc.), and
   send it to the actually screen
   s       the session
   fn      name of the file in question
   whence  which screenrc are we in?
   <-      error code */

int
ns_parse_screenrc(_ns_sess * s, char *fn, int whence)
{
    int fd = -1;
    char *rc = NULL;
    char _e = '\0', _l = '\0', *esc = NULL;

    if (fn) {
        struct stat st;
        ssize_t rd = 0;

        if ((fd = open(fn, 0)) >= 0) {
            if (!fstat(fd, &st)) {
                if ((rc = MALLOC(st.st_size + 1))) {
                    char *p;
                    while (((rd = read(fd, rc, st.st_size)) < 0) && (errno == EINTR));
                    if (rd < 0)
                        goto fail;
                    rc[rd] = '\0';

                    p = rc;
                    while (*p) {
                        char *p2 = p, *n;
                        int f = 0;
                        while (*p2 && *p2 != '\n' && *p2 != '\r')	/* find EOL */
                            p2++;
                        n = p2;
                        while (*n == '\r' || *n == '\n')	/* delete EOL */
                            *(n++) = '\0';
                        while (isspace(*p))
                            p++;

                        p2 = p; /* on first non-white */
                        while (*p2) {
                            if (*p2 == '\\') {
                                p2++;
                                if (*p2)	/* sanity check */
                                    p2++;
                            } else {
                                if (*p2 == '\"')
                                    f = 1 - f;
                                if (!f && *p2 == '#')	/* comment, kill to EOL */
                                    *p2 = '\0';
                                else
                                    p2++;
                            }
                        }

                        if (strlen(p))	/* any commands in line? */
                            ns_parse_screen_cmd(s, p, whence);
                        p = n;  /* done, next line */
                    }
                    FREE(rc);
                    close(fd);
                    return NS_SUCC;
                }
            }
        }
    }

  fail:
    if (fd >= 0)
        close(fd);
    if (rc)
        FREE(rc);
    return NS_FAIL;
}




/* parse a message (not a display-list) set by the "screen" program
   screen   the session associated with that instance of screen,
            as returned by ns_attach_by_URL() and related.
            the session must contain a valid struct of callbacks (efuns),
            as certain functionalities ("add a tab", "show status message")
            may be called from here.
   p        the offending message-line
   <-       returns an error code. */

int
ns_parse_screen_msg(_ns_sess * screen, char *p)
{
    _ns_efuns *efuns;
    _ns_disp *disp;
    char *p2, *p3, *d;
    int ma, mi, mu, n, ret = NS_SUCC, type;

    if (!p)
        return NS_FAIL;

    if (*p == ':')
        p++;
    while (isspace(*p))
        p++;

    type = (strlen(p) > 1) ? NS_SCREEN_STATUS : NS_SCREEN_ST_CLR;

    if (type == NS_SCREEN_ST_CLR) {
        if (NS_EFUN_EXISTS(efuns, screen, NULL, err_msg)) {
            ret = efuns->err_msg(NULL, type, "");
        }
    }
    /* a screen display can disappear because the program in it dies, or
       because we explicitly ask screen to kill the display.  in the latter
       case, screen messages upon success.  rather than explicitly killing
       the disp-struct here, we force a status-line update instead (in which
       the status-line checker will notice the disp has gone, and delete it
       from the struct-list).  this way, we won't need to duplicate the
       delete-logic here. */
    else if (!strncmp(p, "Window ", strlen("Window ")) && (p2 = strrchr(p, ' ')) && !strcmp(p2, " killed.")) {
#ifdef NS_DEBUG
        fprintf(stderr, NS_PREFIX "ns_parse_screen_msg: window kill detected.\n");
#endif
        ret = ns_upd_stat(screen);
        p = NULL;
    } else if (!strcmp(p, "New screen...") || !strncmp(p, "msgwait", strlen("msgwait")) || !strncmp(p, "msgminwait", strlen("msgminwait")))
        p = NULL;
#ifndef NS_PARANOID
    /* FIXME. */
    else if (sscanf(p, NS_SCREEN_VERSION, &p3, &ma, &mi, &mu, &p2, &d) == 6) {
        if (!strcmp("en", p3))
            screen->backend = NS_MODE_SCREEN;
        else if (!strcmp("am", p3))
            screen->backend = NS_MODE_SCREAM;
#  ifdef NS_DEBUG
        fprintf(stderr, NS_PREFIX "ns_parse_screen_msg: scre%s %d.%2d.%2d %s a/o %s\n", p3, ma, mi, mu, p2, d);
#  endif
    }
#endif
    else if (!strcmp(p, NS_SCREEN_NO_DEBUG))
        p = "debug info was not compiled into \"screen\"...";
    else if (!strncmp(p, NS_SCREEN_DK_CMD, strlen(NS_SCREEN_DK_CMD))) {
        p[strlen(p) - 1] = '\0';
        p2 = &p[strlen(NS_SCREEN_DK_CMD)];
        p = "unknown screen statement ignored";
    }
    if (p) {                    /* status. send to status-line or dialog or whatever */
        if (NS_EFUN_EXISTS(efuns, screen, NULL, err_msg)) {
            ret = efuns->err_msg(NULL, type, p);
        }
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
   doing.   2002/05/01  Azundris  <scream@azundris.com>

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
ns_parse_screen(_ns_sess * screen, int force, int width, char *p)
{
    char *p4, *p3, *p2;         /* pointers for parser magic */
    static const char *p5 = NS_SCREEN_FLAGS;
    static size_t l = sizeof(NS_SCREEN_FLAGS);
#if (NS_SCREEN_UPD_FREQ>0)
    time_t t2 = time(NULL);
#endif
    int ret = NS_SUCC, tmp, status_blanks = 0,	/* status-bar overflow? */
     parsed,                    /* no of *visible* elements in status line */
     n,                         /* screen's index (immutable, sparse) */
     r;                         /* real index (r'th element) */
    _ns_efuns *efuns;
    _ns_disp *disp = NULL, *d2 = NULL;

    if (!screen || !p || !width)
        return NS_FAIL;

    if (!force && screen->timestamp)
        return NS_SUCC;

    if (p = strdup(p)) {
        _ns_parse pd[NS_MAX_DISPS];
        p2 = &p[width - 1];
        while (p2 > p && *p2 == ' ') {
            status_blanks++;
            *(p2--) = '\0';
        }                       /* p2 now points behind last item */

#ifdef NS_DEBUG
        fprintf(stderr, NS_PREFIX "parse_screen: screen sends ::%s::\n", p);
#endif

        if (strlen(p) < 2) {    /* special case: display 0 */
            disp = screen->dsps;	/* might not get a status-line in d0! */
            if (disp && !(disp->flags & NS_SCREAM_CURR)) {	/* flags need updating */
                disp->flags |= NS_SCREAM_CURR;	/* set flag to avoid calling inp_text */
                ret = ns_upd_stat(screen);
            } /* more than once */
            else if (!screen->timestamp) {
                screen->timestamp = time(NULL);
                if (screen->delay > 0) {
                    if (NS_EFUN_EXISTS(efuns, screen, NULL, waitstate)) {
                        ret = efuns->waitstate(NULL, screen->delay * 1000);
                    } else {
                        sleep(screen->delay);
                    }
                }
                (void) ns_screen_command(screen, NS_SCREEN_INIT);
            }
            FREE(p);
            return ret;
        }

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

#ifdef NS_DEBUG_
            fputs(NS_PREFIX "parse_screen: found ", stderr);
            for (r = 0; r < parsed; r++)
                if (pd[r].name)
                    fprintf(stderr, "%d(%d/%d,%s)  ", r, pd[r].screen, pd[r].real, pd[r].name);
            fputs("\n\n", stderr);
#endif

            for (r = 0; r < parsed; r++) {
                n = pd[r].screen;
                disp = disp_fetch(screen, n);

                if (!disp) {    /* new display */
                    if (!(disp = disp_fetch_or_make(screen, n)) || !(disp->name = strdup(pd[r].name))) {
                        fprintf(stderr, NS_PREFIX "parse_screen: out of memory in new_display(%d)\n", n);
                        ret = NS_FAIL;
                    } else {
                        if (NS_EFUN_EXISTS(efuns, screen, NULL, ins_disp)) {
                            ret = efuns->ins_disp(screen->userdef, pd[r].real - 1, disp->name);
                        }
                    }
                } else if ((tmp = strcmp(disp->name, pd[r].name)) ||	/* upd display */
                           (disp->flags != pd[r].flags)) {
                    if (tmp) {
                        FREE(disp->name);
                        if (!(disp->name = strdup(pd[r].name))) {
                            FREE(p);
                            return NS_FAIL;
                        }
                    }
                    if (pd[r].flags & NS_SCREAM_CURR)
                        disp->sess->curr = disp;
                    disp->flags = pd[r].flags & NS_SCREAM_MASK;
                    if (NS_EFUN_EXISTS(efuns, screen, NULL, upd_disp)) {
                        ret = efuns->upd_disp(screen->userdef, r, disp->flags, disp->name);
                    }
                }

                /* remove any displays from list that have disappeared
                   from the middle of the status-line */
                if (!d2 || d2->next != disp) {	/* remove expired displays */
                    _ns_disp *d3 = disp->prvs, *d4;
                    while (d3 && d3 != d2) {
#ifdef NS_DEBUG
                        fprintf(stderr, NS_PREFIX "parse_screen: remove expired middle %d \"%s\"...\n", d3->index, d3->name);
#endif
                        d4 = d3->prvs;
                        if (NS_EFUN_EXISTS(efuns, screen, NULL, del_disp)) {
                            ret = efuns->del_disp(screen->userdef, disp_get_real_by_screen(screen, d3->index));
                        }
                        disp_kill(d3);
                        d3 = d4;
                    }
                }
                d2 = disp;
            }



#ifdef NS_PARANOID
            if (!r) {
                if (!(err_inhibit & NS_ERR_WEIRDSCREEN)) {
                    err_inhibit |= NS_ERR_WEIRDSCREEN;
                    fprintf(stderr, NS_PREFIX "parse_screen: !r\n"
                            "This should never happen. It is assumed that you use a\n"
                            "rather unusual configuration for \"screen\".   Please\n"
                            "send the result of 'screen --version' to <scream@azundris.com>\n"
                            "(together with your ~/.screenrc and /etc/screenrc if present).\n"
                            "If at all possible, please also run 'Eterm -e screen' and make\n"
                            "a screenshot of the offending window (and the window only, the\n"
                            "beauty of your desktop is not relevant to this investigation. : ).\n");
                }
                ret = ns_upd_stat(screen);
                FREE(p);
                return NS_FAIL;
            } else
#endif
                /* kill overhang (o/t right) if status-line isn't side-scrolling
                   (as it will if not all the disp names fit in the status-line) */
            if (disp->next && status_blanks > (strlen(disp->next->name) + 6)) {
                _ns_disp *d3 = disp;
                for (disp = disp->next; disp;) {
#ifdef NS_DEBUG
                    fprintf(stderr, NS_PREFIX "parse_screen: remove expired right %d \"%s\"...\n", disp->index, disp->name);
#endif
                    d2 = disp;
                    if (d2->sess->curr == d2)
                        d2->sess->curr = d3;
                    disp = disp->next;
                    if (NS_EFUN_EXISTS(efuns, screen, NULL, del_disp)) {
                        ret = efuns->del_disp(screen->userdef, disp_get_real_by_screen(screen, d2->index));
                    }
                    disp_kill(d2);
                }
                d3->next = NULL;
            }
        }

        else                    /* not a list of displays, but a message. handle separately. */
            ret = ns_parse_screen_msg(screen, p);

        FREE(p);                /* release our (modified) copy of the status-line */
    }

    /* send init string the first time around, just to be on the safe side.
       we could send it before entering this function for the first time,
       but that would break if escapes or screenrc were set from the
       command-line. don't ask. */
#if (NS_SCREEN_UPD_FREQ>0)
    if ((t2 - screen->timestamp) > NS_SCREEN_UPD_FREQ) {
        (void) ns_upd_stat(screen);
        screen->timestamp = t2;
    }
#endif

    return ret;
}



/***************************************************************************/
