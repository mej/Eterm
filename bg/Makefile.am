# $Id$

DIRS = tile scale
EXTRA_DIST =  tile/014.png tile/circuit.png tile/backwave.jpg tile/button1.jpg tile/button5.jpg \
              tile/nebula.jpg scale/fourthday.jpg scale/gaia.jpg scale/galleon.jpg scale/night_of_the_dragon.jpg \
              README.backgrounds
pixmapdir = $(pkgdatadir)/pix
themedir = $(pkgdatadir)/themes

all: Makefile
	@echo "*** Generating pixmaps.list file from images in tile/ and scale/..."
	@true > pixmaps.list
	@cd tile ; \
	for i in `ls -1 | grep -v CVS | grep -v \.cvsignore` ; do \
	  echo '" 0  0  '$$i'"' >> ../pixmaps.list ; \
	done ; \
	cd .. ; \
	cd scale ; \
	for i in `ls -1 | grep -v CVS | grep -v \.cvsignore` ; do \
	  echo '"-1 -1  '$$i'"' >> ../pixmaps.list ; \
	done

install-data-hook:
	@if test ! -f pixmaps.list ; then $(MAKE) all ; fi
	$(mkinstalldirs) $(DESTDIR)$(pixmapdir)
	@for j in $(DIRS) ; do echo "Installing pixmaps in $$j" ; cd $$j ; \
	  for i in `ls -1 | grep -v CVS | grep -v \.cvsignore` ; do \
	    echo $(INSTALL_DATA) $$i $(DESTDIR)${pixmapdir} ; \
	    $(INSTALL_DATA) $$i $(DESTDIR)${pixmapdir} ; \
	  done ; cd .. ; \
	done
	@if test -f $(DESTDIR)${pixmapdir}/pixmaps.list ; then \
	  $(CP) -p $(DESTDIR)${pixmapdir}/pixmaps.list $(DESTDIR)${pixmapdir}/pixmaps.list.old ; \
	  echo "WARNING:  Backup of existing pixmaps.list created." ; \
	fi
	$(INSTALL_DATA) pixmaps.list $(DESTDIR)${pixmapdir}
	@for i in `ls -1d $(DESTDIR)${themedir}/*` ; do \
	  if test $$i != "." -a $$i != ".." -a ! -f $$i/pixmaps.list ; then \
	    echo "$(LN_S) ../../pix/pixmaps.list $$i/pixmaps.list" ; \
	    $(LN_S) ../../pix/pixmaps.list $$i/pixmaps.list ; \
	  fi ; \
	done
