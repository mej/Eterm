/***************************************************************
 * STRPTIME.H -- Header file for strptime()                    *
 *            -- Michael Jennings                              *
 *            -- 2 April 1997                                  *
 ***************************************************************/
/*
 * Copyright (C) 1997-2000, Michael Jennings
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies of the Software, its documentation and marketing & publicity
 * materials, and acknowledgment shall be given in the documentation, materials
 * and software packages that this Software was used.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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

extern char *strptime(char *, const char *, struct tm *);

#endif /* _STRPTIME_H_ */
