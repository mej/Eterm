/****************************************************************************
 * scream::screamcfg.h
 * user-tunable parameters for the routines to connect to screen and/or
 * scream daemons.
 * GNU Public Licence applies.
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

#define NS_SSH_CALL        "ssh"
#define NS_SSH_OPTS        "-t"
#define NS_SCREAM_CALL     "scream"
#define NS_SCREAM_OPTS     "-xRR"
#define NS_SCREEN_CALL     "screen"
#define NS_SCREEN_OPTS     "-c /dev/null -xRR"
#define NS_SCREEM_CALL     "\"" NS_SCREAM_CALL " " NS_SCREAM_OPTS " 2>/dev/null || " NS_SCREEN_CALL " " NS_SCREEN_OPTS "\""

/* this should never change. the escape-char for an out-of-the-box "screen".
   don't change this just because you set something else in your .screenrc */
#define NS_SCREEN_ESCAPE   '\x01'
#define NS_SCREEN_LITERAL  'a'

/* the following must use the char defined in NS_SCREEN_ESCAPE. if something
   else is used in the session, libscream will convert it on the fly. */
/* DO NOT use \005Lw for your status, it breaks older screens!! */
#define NS_SCREEN_UPDATE   "\x01w"
#define NS_SCREEN_INIT     "\x01:hardstatus lastline\r\x01:defhstatus \"\\005w\"\r\x01:hstatus \"\\005w\"\r" NS_SCREEN_UPDATE

#define NS_DFLT_SSH_PORT   22
#define NS_MAX_PORT        65535

#define NS_MAX_DISPS       512

#define NS_SCREEN_FLAGS    "*-$!@L&Z"

/* if >0, force an update every NS_SCREEN_UPD_FREQ seconds.
   a bit of a last resort. */
#define NS_SCREEN_UPD_FREQ 0



/***************************************************************************/
