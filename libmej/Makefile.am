# $Id$

lib_LTLIBRARIES = libmej.la
include_HEADERS = libmej.h

libmej_la_SOURCES = debug.c mem.c msgs.c strings.c snprintf.c libmej.h

INCLUDES = -I. -I.. -I$(includedir) -I$(prefix)/include

libmej_la_LDFLAGS = -release $(VERSION)
