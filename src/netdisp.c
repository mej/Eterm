/*----------------------------------------------------------------------*
 * support for resolving the actual IP number of the host for remote
 * DISPLAYs.  When the display is local (i.e. :0), we add support for
 * sending the first non-loopback interface IP number as the DISPLAY
 * instead of just sending the incorrect ":0".  This way telnet/rlogin
 * shells can actually get the correct information into DISPLAY for
 * xclients.
  *
  * Copyright 1996	Chuck Blake <cblake@BBN.COM>
  *
+ * Cleaned up somewhat by mj olesen <olesen@me.queensu.ca>
+ *
  * You can do what you like with this source code as long as you don't try
  * to make money out of it and you include an unaltered copy of this
  * message (including the copyright).
  *
  * As usual, the author accepts no responsibility for anything, nor does
  * he guarantee anything whatsoever.
  *----------------------------------------------------------------------*/

static const char cvs_ident[] = "$Id$";

#include "config.h"
#include "feature.h"

#ifdef DISPLAY_IS_IP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/socket.h>
#ifdef HAVE_SYS_SOCKIO_H
# include <sys/sockio.h>
#endif
#ifdef HAVE_SYS_BYTEORDER_H
# include <sys/byteorder.h>
#endif
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#ifdef HAVE_SYS_IOCTL_H
# include <sys/ioctl.h>
#endif
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_arp.h>

#include "startup.h"

/* return a pointer to a static buffer */
char *
network_display(const char *display)
{

    static char ipaddress[32] = "";
    char buffer[1024], *rval = NULL;
    struct ifconf ifc;
    struct ifreq *ifr;
    int i, skfd;

    if (display[0] != ':' && strncmp(display, "unix:", 5))
        return display;         /* nothing to do */

    ifc.ifc_len = sizeof(buffer);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      /* Get names of all ifaces */
    ifc.ifc_buf = buffer;

    if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        return NULL;
    }
    if (ioctl(skfd, SIOCGIFCONF, &ifc) < 0) {
        perror("SIOCGIFCONF");
        close(skfd);
        return NULL;
    }
    for (i = 0, ifr = ifc.ifc_req; i < (ifc.ifc_len / sizeof(struct ifreq)); i++, ifr++) {

        struct ifreq ifr2;

        strcpy(ifr2.ifr_name, ifr->ifr_name);
        if (ioctl(skfd, SIOCGIFADDR, &ifr2) >= 0) {
            unsigned long addr;
            struct sockaddr_in *p_addr;

            p_addr = (struct sockaddr_in *) &(ifr2.ifr_addr);
            addr = htonl((unsigned long) p_addr->sin_addr.s_addr);

            /*
             * not "0.0.0.0" or "127.0.0.1" - so format the address
             */
            if (addr && addr != 0x7F000001) {
                char *colon = strchr(display, ':');

                if (colon == NULL)
                    colon = ":0.0";

                sprintf(ipaddress, "%d.%d.%d.%d%s", (int) ((addr >> 030) & 0xFF), (int) ((addr >> 020) & 0xFF), (int) ((addr >> 010) & 0xFF), (int) (addr & 0xFF), colon);

                rval = ipaddress;
                break;
            }
        }
    }

    close(skfd);
    return rval;
}
#endif /* DISPLAY_IS_IP */
/*----------------------- end-of-file (C source) -----------------------*/
