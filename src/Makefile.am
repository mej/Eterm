# $Id$

lib_LTLIBRARIES = libEterm.la

libEterm_la_SOURCES = actions.c command.c e.c events.c font.c graphics.c grkelot.c menus.c misc.c netdisp.c \
                      options.c pixmap.c screen.c scrollbar.c system.c term.c threads.c timer.c utmp.c windows.c
libEterm_la_DEPENDENCIES = $(top_builddir)/libmej/libmej.la feature.h
libEterm_la_LDFLAGS = -version-info 9:0:9

bin_PROGRAMS = Eterm
Eterm_SOURCES = main.c
Eterm_DEPENDENCIES = libEterm.la
Eterm_LDFLAGS = -rpath $(libdir):$(pkglibdir)

INCLUDES = -I. -I$(top_srcdir) -I$(top_srcdir)/libmej -I.. -I$(includedir) -I$(prefix)/include $(X_CFLAGS)
LDADD = libEterm.la $(top_builddir)/libmej/libmej.la -L$(libdir) -L$(prefix)/lib $(LIBS) $(GRLIBS) $(X_LIBS)

EXTRA_DIST = actions.h command.h debug.h e.h eterm_utmp.h events.h feature.h font.h graphics.h grkelot.h grx.h main.h menus.h misc.h \
             options.h pixmap.h profile.h screen.h scrollbar.h system.h term.h threads.h timer.h windows.h graphics/Makefile.am \
             graphics/Makefile.in graphics/README graphics/data graphics/grxlib.c graphics/grxlib.h graphics/qplot.c Eterm.xpm
