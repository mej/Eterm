# $Id$

DIRS = tile scale
EXTRA_DIST =  tile/014.png tile/circuit.png tile/backwave.jpg tile/button1.jpg tile/button5.jpg \
              tile/nebula.jpg scale/fourthday.jpg scale/gaia.jpg scale/galleon.jpg scale/night_of_the_dragon.jpg \
              README.backgrounds
pixmapdir = $(pkgdatadir)/pix
themedir = $(pkgdatadir)/themes

all: Makefile
	@echo "*** Generating pixmap list files from images in tile/ and scale/..."
	@true > pixmaps.list
	@cd tile ; \
	true > tiled-pixmaps.list ; \
	for i in `ls -1 | grep -v CVS | grep -v \.cvsignore | grep -v pixmaps.list` ; do \
	  echo $$i >> tiled-pixmaps.list ; \
	  echo '"'$$i'@0x0+0+0:tile"' >> ../pixmaps.list ; \
	done ; \
	cd .. ; \
	cd scale ; \
	true > scaled-pixmaps.list ; \
	for i in `ls -1 | grep -v CVS | grep -v \.cvsignore | grep -v pixmaps.list` ; do \
	  echo $$i >> scaled-pixmaps.list ; \
	  echo '"'$$i'@100x100"' >> ../pixmaps.list ; \
	done

install-data-hook:
	@if test ! -f pixmaps.list ; then $(MAKE) all ; fi
	$(mkinstalldirs) $(DESTDIR)$(pixmapdir)
	@for j in $(DIRS) ; do echo "Installing pixmaps in $$j" ; cd $$j ; \
	  for i in `ls -1 | grep -v CVS | grep -v \.cvsignore | grep -v pixmaps.list` ; do \
	    echo $(INSTALL_DATA) $$i $(DESTDIR)${pixmapdir} ; \
	    $(INSTALL_DATA) $$i $(DESTDIR)${pixmapdir} ; \
	  done ; cd .. ; \
	done
	@for i in tiled-pixmaps.list scaled-pixmaps.list pixmaps.list ; do \
          if test -f $(DESTDIR)${pixmapdir}/$$i ; then \
	    $(CP) -p $(DESTDIR)${pixmapdir}/$$i $(DESTDIR)${pixmapdir}/$$i.old ; \
	    echo "WARNING:  Backup of existing $$i created." ; \
	  fi ; \
	done
	$(INSTALL_DATA) tile/tiled-pixmaps.list $(DESTDIR)${pixmapdir}
	$(INSTALL_DATA) scale/scaled-pixmaps.list $(DESTDIR)${pixmapdir}
	$(INSTALL_DATA) pixmaps.list $(DESTDIR)${pixmapdir}
	@for i in `ls -1d $(DESTDIR)${themedir}/*` ; do \
	  for j in tiled-pixmaps.list scaled-pixmaps.list pixmaps.list ; do \
	    if test $$i != "." -a $$i != ".." -a ! -f $$i/$$j ; then \
	      echo "$(LN_S) ../../pix/$$j $$i/$$j" ; \
	      $(LN_S) ../../pix/$$j $$i/$$j ; \
	    fi ; \
	  done ; \
	done
