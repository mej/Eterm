# $Id$

lib_LTLIBRARIES = libmej.la

libmej_la_SOURCES = debug.c mem.c strings.c snprintf.c

INCLUDES = -I. -I.. -I$(includedir) -I$(prefix)/include

libmej_la_LDFLAGS = -release $(VERSION)

EXTRA_DIST = debug.h global.h mem.h strings.h strptime.h strptime.c

