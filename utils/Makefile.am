# $Id$

INCLUDES = -I. -I../libmej -I.. -I$(includedir) -I$(prefix)/include $(X_CFLAGS)

bin_PROGRAMS = Esetroot Etbg
bin_SCRIPTS = Etcolors.sh Etmenu.sh

Esetroot_SOURCES = Esetroot.c
Esetroot_LDFLAGS = -rpath $(libdir):$(pkglibdir)
Esetroot_LDADD = -L$(DIST_ROOT)/libmej -L$(libdir) -L$(prefix)/lib $(LIBS) $(GRLIBS) $(X_LIBS)

Etbg_SOURCES = Etbg.c

EXTRA_DIST = Etbg.c Esetroot.c Makefile.am Makefile.in Etcolors.sh Etmenu.sh

