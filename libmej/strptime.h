/***************************************************************
 * STRPTIME.H -- Header file for strptime()                    *
 *            -- Michael Jennings                              *
 *            -- 2 April 1997                                  *
 ***************************************************************/
/*
 * This file is original work by Michael Jennings <mej@tcserv.com>.
 *
 * Copyright (C) 1997, Michael Jennings
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

#ifndef _STRPTIME_H_

#define _STRPTIME_H_

#define sizeofone(s)      (sizeof(s) / sizeof((s)[0]))

#define NUM_MONTHS     12
#define NUM_DAYS        7

typedef struct dtmap_struct {
  char *Months[NUM_MONTHS];
  char *MonthsAbbrev[NUM_MONTHS];
  char *Days[NUM_DAYS];
  char *DaysAbbrev[NUM_DAYS];
  char *DateFormat;
  char *TimeFormat;
  char *DateTimeFormat;
  char *LocaleDateFormat;
  char *AM;
  char *PM;
} DTMap;

static DTMap USMap = {
  { "January", "February", "March", "April",
    "May", "June", "July", "August",
    "September", "October", "November", "December" },
  { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" },
  { "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday" },
  { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" },
  "%m/%d/%y",
  "%H:%M:%S",
  "%a %b %e %T %Z %Y",
  "%A, %B, %e, %Y",
  "AM",
  "PM"
};

#ifndef STRPTIME_C
#  ifdef __cplusplus
extern "C" {
#  else
extern {
#  endif
  extern char *strptime(char *, const char *, struct tm *);
}
#endif

#endif /* _STRPTIME_H_ */
