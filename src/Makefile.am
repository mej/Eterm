# $Id$

lib_LTLIBRARIES = libEterm.la

libEterm_la_SOURCES = actions.c actions.h buttons.c buttons.h command.c        \
                      command.h debug.h draw.c draw.h e.c e.h eterm_utmp.h     \
                      events.c events.h feature.h font.c font.h grkelot.c      \
                      grkelot.h icon.h menus.c menus.h misc.c misc.h netdisp.c \
                      options.c options.h pixmap.c pixmap.h profile.h screen.c \
                      screen.h scrollbar.c scrollbar.h startup.c startup.h     \
                      system.c system.h term.c term.h timer.c timer.h utmp.c   \
                      windows.c windows.h

libEterm_la_DEPENDENCIES = $(top_builddir)/libmej/libmej.la feature.h
libEterm_la_LDFLAGS = -release $(VERSION)

bin_PROGRAMS = Eterm
Eterm_SOURCES = main.c
Eterm_DEPENDENCIES = libEterm.la
Eterm_LDFLAGS = -rpath $(libdir):$(pkglibdir)

INCLUDES = -I. -I$(top_srcdir) -I$(top_srcdir)/libmej -I.. -I$(includedir) -I$(prefix)/include $(X_CFLAGS)
LDADD = libEterm.la $(top_builddir)/libmej/libmej.la -L$(libdir) -L$(prefix)/lib $(LIBS) $(GRLIBS) $(X_LIBS)
