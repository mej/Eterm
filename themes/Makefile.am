# $Id$

THEMES = Eterm auto cEterm chooser emacs irc mutt trans

EXTRA_DIST = Eterm/theme.cfg.in irc/theme.cfg.in chooser/theme.cfg.in emacs/theme.cfg.in mutt/theme.cfg.in trans/theme.cfg.in \
             Eterm/Eterm-menu.cfg irc/irc-menu.cfg chooser/chooser-menu.cfg emacs/emacs-menu.cfg mutt/mutt-menu.cfg trans/trans-menu.cfg \
             cEterm/cEterm-menu.cfg cEterm/theme.cfg.in auto/auto-menu.cfg auto/theme.cfg.in

all: Makefile
	-@for i in $(THEMES) ; do \
	  echo "Generating theme config file for the $$i theme." ; \
	  $(SED) -e 's%@''PREFIX''@%${prefix}%g' -e 's%@''PKGDATADIR''@%${DESTDIR}${pkgdatadir}%g' -e 's%@''THEME''@%'$$i'%g' \
	    $(srcdir)/$$i/theme.cfg.in > $(srcdir)/$$i/theme.cfg ; \
	done

install-data-hook:
	$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/themes
	-@for i in $(THEMES) ; do \
	  if test -d $(DESTDIR)$(pkgdatadir)/themes/$$i -a "@REMOVE_THEMES@" = "yes"; then \
	    echo "Removing $$i theme in $(DESTDIR)$(pkgdatadir)/themes as requested." ; \
	    $(RM) -rf $(DESTDIR)$(pkgdatadir)/themes/$$i ; \
	    if test -d $(DESTDIR)$(pkgdatadir)/themes/$$i ; then \
	      echo "ERROR:  Unable to remove theme." ; \
	    fi ; \
	  fi ; \
	  if test ! -d $(DESTDIR)$(pkgdatadir)/themes/$$i ; then \
	    echo "Installing $$i theme in $(DESTDIR)$(pkgdatadir)/themes" ; \
	    $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/themes/$$i ; \
	    $(CP) $(srcdir)/$$i/???* $(DESTDIR)$(pkgdatadir)/themes/$$i ; \
	    $(CHMOD) 755 $(DESTDIR)$(pkgdatadir)/themes/$$i ; \
	    $(CHMOD) 644 $(DESTDIR)$(pkgdatadir)/themes/$$i/???* ; \
	    rm -f $(DESTDIR)$(pkgdatadir)/themes/$$i/theme.cfg.in ; \
	  else \
	    echo "ALERT!  Not overwriting $$i theme in $(DESTDIR)$(pkgdatadir)/themes.  You will need to update this theme manually." ; \
	  fi ; \
	done
