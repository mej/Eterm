# $Id$

lib_LTLIBRARIES = libEterm.la

libEterm_la_SOURCES = activeconfig.c activeeterm.c activetags.c command.c \
		graphics.c grkelot.c menubar.c misc.c netdisp.c options.c \
		pixmap.c screen.c scrollbar.c system.c threads.c utmp.c

libEterm_la_LDFLAGS = -version-info 8:9:8

bin_PROGRAMS = Eterm

Eterm_SOURCES = main.c

Eterm_DEPENDENCIES = ../libmej/libmej.la feature.h

INCLUDES = -I. -I../libmej -I.. -I$(includedir) -I$(prefix)/include $(X_CFLAGS)

Eterm_LDFLAGS = -rpath $(libdir):$(pkglibdir)
LDADD = -L$(DIST_ROOT)/src/.libs -lEterm -L$(DIST_ROOT)/libmej/.libs -L$(libdir) -L$(prefix)/lib $(LIBS) -lmej $(GRLIBS) $(X_LIBS)

EXTRA_DIST = activeconfig.h activeeterm.h activetags.h command.h debug.h \
             eterm_imlib.h feature.h.in graphics.h grkelot.h grx.h main.h \
             menubar.h misc.h options.h pixmap.h profile.h screen.h scrollbar.h \
             system.h threads.h graphics/Makefile.am graphics/Makefile.in \
             graphics/README graphics/data graphics/grxlib.c graphics/grxlib.h \
             graphics/qplot.c Eterm.xpm

feature.h: feature.h.in
	$(FEATURE_CMD)
