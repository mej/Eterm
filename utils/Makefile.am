# $Id$

INCLUDES = -I. -I../libmej -I.. -I$(includedir) -I$(prefix)/include $(X_CFLAGS)

bin_PROGRAMS = Esetroot Etbg Ettable
bin_SCRIPTS = Etcolors Etwinop

Esetroot_SOURCES = Esetroot.c
Esetroot_LDFLAGS = -rpath $(libdir):$(pkglibdir)
Esetroot_LDADD = -L$(DIST_ROOT)/libmej -L$(libdir) -L$(prefix)/lib $(LIBS) $(GRLIBS) $(X_LIBS)

Etbg_SOURCES = Etbg.c
Ettable_SOURCES = Ettable.c

EXTRA_DIST = Etbg.c Esetroot.c Ettable.c Makefile.am Makefile.in Etcolors Etwinop

