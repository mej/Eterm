# $Id$

lib_LTLIBRARIES = libEterm.la
bin_PROGRAMS = Eterm

MMX_SRCS = mmx_cmod.S
MMX_OBJS = $(MMX_SRCS:.S=.lo)

libEterm_la_SOURCES = actions.c actions.h buttons.c buttons.h command.c          \
                      command.h draw.c draw.h e.c e.h eterm_debug.h eterm_utmp.h \
                      events.c events.h feature.h font.c font.h grkelot.c        \
                      grkelot.h icon.h menus.c menus.h misc.c misc.h netdisp.c   \
                      options.c options.h pixmap.c pixmap.h profile.h screen.c   \
                      screen.h scrollbar.c scrollbar.h startup.c startup.h       \
                      system.c system.h term.c term.h timer.c timer.h utmp.c     \
                      windows.c windows.h
libEterm_la_LDFLAGS = -release $(VERSION)
if HAVE_MMX
libEterm_la_DEPENDENCIES = feature.h $(MMX_OBJS)
libEterm_la_LIBADD = $(MMX_OBJS)
else
libEterm_la_DEPENDENCIES = feature.h
endif

Eterm_SOURCES = main.c
Eterm_DEPENDENCIES = libEterm.la
Eterm_LDFLAGS = -rpath $(libdir):$(pkglibdir)
Eterm_LDADD = libEterm.la 

install-exec-hook:
	$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)
	test ! -z "$(GDB_CMD_FILE)" && $(INSTALL_DATA) gdb.scr $(DESTDIR)$(pkgdatadir)/ || :
