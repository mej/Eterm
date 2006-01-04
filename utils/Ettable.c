/* Ettable -- Eterm ASCII Table Display Utility

 * This file is original work by Michael Jennings <mej@eterm.org>.
 * This program is distributed under the GNU Public License (GPL) as
 * outlined in the COPYING file.
 *
 * Copyright (C) 1997-2006, Michael Jennings
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
 */

static const char cvs_ident[] = "$Id$";

#include <stdio.h>

const char *lookup[] = {
    "NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL",     /*  0-7  */
    "BS", "HT", "LF", "VT", "FF", "CR", "SO", "SI",     /*  8-15 */
    "DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB",     /* 16-23 */
    "CAN", "EM", "SUB", "ESC", "FS", "GS", "RS", "US"   /* 24-31 */
};

int
main(void)
{

    unsigned short i;

    printf("+-----------+---------+-------------+--------+\n");
    printf("| Character | Decimal | Hexadecimal |  Octal |\n");
    printf("+-----------+---------+-------------+--------+\n");

    for (i = 0; i < 32; i++) {
        printf("|  %3s  ^%c  |   %3d   |    0x%02x     |   %03o  |\n", lookup[i], ('@' + i), i, i, i);
    }
    for (; i < 256; i++) {
        printf("|    '%c'    |   %3d   |    0x%02x     |  %c%03o  |\n", (i == 127 ? ' ' : i), i, i, (i > '\077' ? '0' : ' '), i);
    }
    printf("+-----------+---------+-------------+--------+\n");

    printf("+---------------+---------+-------------+-------+\n");
    printf("| ACS Character | Decimal | Hexadecimal | Octal |\n");
    printf("+---------------+---------+-------------+-------+\n");
    printf("\033)0");

    for (i = 1; i < 32; i++) {
        printf("|    \016%c\017   (%c)    |   %3d   |    0x%02x     |  %03o  |\n", i + 0x5e, i + 0x5e, i, i, i);
    }
    printf("+---------------+---------+-------------+-------+\n");
    return 0;
}
