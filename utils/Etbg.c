/* Eterm background switching helper
 * by keebler 3/26/99
 *
 * This program is original work by Brian McFee <keebler@themes.org>.
 * This program is distributed under the GNU Public License (GPL) as
 * outlined in the COPYING file.
 *
 * Copyright (C) 1999, Brian McFee.
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

#include <stdio.h>
#include <string.h>

int 
main(int argc, char **argv)
{
  enum {
    FALSE, TRUE
  } scale = FALSE, trans = FALSE;
  unsigned int i, pic = 0, tint = 0, shade = 0;

  for (i = 1; i < argc; i++) {
    if (strcasecmp(argv[i], "-scale") == 0) {
      scale = TRUE;
    } else if (strcasecmp(argv[i], "-trans") == 0) {
      trans = TRUE;
    } else if (strcasecmp(argv[i], "-tint") == 0 && i < argc - 1) {
      tint = ++i;
    } else if (strcasecmp(argv[i], "-shade") == 0 && i < argc - 1) {
      shade = ++i;
    } else if (strncasecmp(argv[i], "-h", 2) == 0) {
      printf("Usage: %s [[-scale] file] [-trans] [-tint 0xACOLOR] [-shade nn%]\n", argv[0]);
      return 0;
    } else {
      pic = i;
    }
  }

  if (pic && argv[pic]) {
    printf("\033]6;0;0\a");
    printf("\033]20;%s%s\a", argv[pic], scale ? ";100x100+0+0" : ";0");
  }
  if (trans) {
    printf("\033]6;0;1\a");
  }
  if (tint && argv[tint]) {
    printf("\033]6;2;%s\a", argv[tint]);
  }
  if (shade && argv[shade]) {
    printf("\033]6;1;%s\a", argv[shade]);
  }
  return 0;
}
