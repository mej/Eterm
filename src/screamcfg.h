/****************************************************************************
 * scream::screamcfg.h
 * user-tunable parameters for the routines to connect to screen and/or
 * scream daemons.
 * BSD Licence applies.
 * 2002/04/19  Azundris  incept
 ***************************************************************************/



/* NS_PARANOID enables checks for deviant "screen" behaviour.
   libscream is a little more efficient (and still stable on my box)
   without it, but leaving the checks out in the official version
   would turn this into the living support nightmare.  Thus, if you
   undef it, you're on your own. */
#define NS_PARANOID

/* define NS_DEBUG to get debug-info. no support for those who undef this. */
#undef NS_DEBUG

/* debug memory stuff. never actually used this. */
#undef  NS_DEBUG_MEM

#define NS_MAXCMD            512

#define NS_SSH_CALL          "ssh"
#define NS_SSH_OPTS          "-t"
#define NS_SSH_TUNNEL_OPTS   "-N"
#define NS_SCREAM_CALL       "scream %s"
#define NS_SCREAM_OPTS       "-xRR"
#define NS_SCREEN_CALL       "screen %s"
#define NS_SCREEN_OPTS       "-xRR"
#define NS_SCREEN_GREP       "grep escape \"$SCREENRC\" 2>/dev/null || grep escape ~/.screenrc 2>/dev/null || grep escape \"$SYSSCREENRC\" 2>/dev/null || grep escape /etc/screenrc 2>/dev/null || grep escape /usr/local/etc/screenrc 2>/dev/null || echo \"escape ^Aa\"\n"
#define NS_SCREEM_CALL       "%s 2>/dev/null || %s"
#define NS_WRAP_CALL         "export TERM=vt100; %s"
#define NS_SCREEN_RC         ".screenrc"

/* this should never change. the escape-char for an out-of-the-box "screen".
   don't change this just because you set something else in your .screenrc */
#define NS_SCREEN_ESCAPE     '\x01'
#define NS_SCREEN_LITERAL    'a'
#define NS_SCREEN_CMD        ':'
#define NS_SCREEN_RENAME     'A'
#define NS_SCREEN_KILL       'k'
#define NS_SCREEN_DEFSBB     100

/* the following must use the char defined in NS_SCREEN_ESCAPE. if something
   else is used in the session, libscream will convert it on the fly. */
/* DO NOT use \005Lw for your status, it breaks older screens!! */
#define NS_SCREEN_UPDATE     "\x01w"
#define NS_SCREEN_INIT       "\x0c\x01Z\x01:hardstatus lastline\r\x01:defhstatus \"\\005w\"\r\x01:hstatus \"\\005w\"\r\x01:msgminwait 0\r\x01:msgwait 1\r\x01:nethack off\r" NS_SCREEN_UPDATE
#define NS_SCREEN_PRVS_REG   "\x01:focus up\r"

#define NS_DFLT_SSH_PORT     22
#define NS_MIN_PORT          1025
#define NS_MAX_PORT          65535

#define NS_MAX_DISPS         512

#define NS_SCREEN_FLAGS      "*-$!@L&Z"

#define NS_SCREEN_DK_CMD     "unknown command '"
#define NS_SCREEN_VERSION    "scre%s %d.%d.%d %s %s"
#define NS_SCREEN_NO_DEBUG   "Sorry, screen was compiled without -DDEBUG option."

/* if >0, force an update every NS_SCREEN_UPD_FREQ seconds.
   a bit of a last resort. */
#define NS_SCREEN_UPD_FREQ   0

/* should be 1s */
#define NS_INIT_DELAY        1

/* how many seconds to wait for an SSH-tunnel to build when using the
   -Z option (tunnel through firewall).  2 for very fast networks,
   much more for slow connections. */
#define NS_TUNNEL_DELAY      3

/* what to call the menu entry for Escreen */
#define NS_MENU_TITLE        "Escreen"

/* prefix for debug info */
#define NS_PREFIX            "libscream::"



/***************************************************************************/
