# $Id$

DIRS = tile scale
EXTRA_DIST =  tile/014.jpg tile/circuit.jpg tile/giger1.jpg tile/40.jpg tile/blackstone.jpg \
              tile/nebula.jpg scale/Dragon.jpg \
              README.backgrounds
pixmapdir = $(pkgdatadir)/pix
themedir = $(pkgdatadir)/themes

all: Makefile
	@echo "*** Generating pixmap list files from images in tile/ and scale/..."
	@true > $(srcdir)/pixmaps.list
	@true > $(srcdir)/tile/tiled-pixmaps.list
	@true > $(srcdir)/scale/scaled-pixmaps.list
	@for i in `(cd $(srcdir)/tile && ls -1 | grep -v CVS | grep -v \.cvsignore | grep -v pixmaps.list)` ; do \
	  echo $$i >> $(srcdir)/tile/tiled-pixmaps.list ; \
	  echo '"'$$i'@0x0+0+0:tile"' >> $(srcdir)/pixmaps.list ; \
	done
	@for i in `(cd $(srcdir)/scale && ls -1 | grep -v CVS | grep -v \.cvsignore | grep -v pixmaps.list)` ; do \
	  echo $$i >> $(srcdir)/scale/scaled-pixmaps.list ; \
	  echo '"'$$i'@100x100"' >> $(srcdir)/pixmaps.list ; \
	done

install-data-hook:
	@if test ! -f $(srcdir)/pixmaps.list ; then $(MAKE) all ; fi
	$(mkinstalldirs) $(DESTDIR)$(pixmapdir)
	@(for j in $(DIRS) ; do echo "Installing pixmaps in $$j" ; cd $(srcdir)/$$j ; \
	  for i in `ls -1 | grep -v CVS | grep -v \.cvsignore | grep -v pixmaps.list` ; do \
	    echo $(INSTALL_DATA) $$i $(DESTDIR)${pixmapdir} ; \
	    $(INSTALL_DATA) $$i $(DESTDIR)${pixmapdir} ; \
	  done ; cd .. ; \
	done)
	@for i in tiled-pixmaps.list scaled-pixmaps.list pixmaps.list ; do \
          if test -f $(DESTDIR)${pixmapdir}/$$i ; then \
	    $(CP) -p $(DESTDIR)${pixmapdir}/$$i $(DESTDIR)${pixmapdir}/$$i.old ; \
	    echo "WARNING:  Backup of existing $$i created." ; \
	  fi ; \
	done
	$(INSTALL_DATA) $(srcdir)/tile/tiled-pixmaps.list $(DESTDIR)${pixmapdir}
	$(INSTALL_DATA) $(srcdir)/scale/scaled-pixmaps.list $(DESTDIR)${pixmapdir}
	$(INSTALL_DATA) $(srcdir)/pixmaps.list $(DESTDIR)${pixmapdir}
	@for i in `ls -1d $(DESTDIR)${themedir}/*` ; do \
	  for j in tiled-pixmaps.list scaled-pixmaps.list pixmaps.list ; do \
	    if test $$i != "." -a $$i != ".." -a ! -f $$i/$$j ; then \
	      echo "$(LN_S) ../../pix/$$j $$i/$$j" ; \
	      $(LN_S) ../../pix/$$j $$i/$$j ; \
	    fi ; \
	  done ; \
	done
