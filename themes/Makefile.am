# $Id$

THEMES = Eterm cEterm chooser emacs irc mutt trans

EXTRA_DIST = Eterm/theme.cfg.in irc/theme.cfg.in chooser/theme.cfg.in emacs/theme.cfg.in mutt/theme.cfg.in \
             trans/theme.cfg.in Eterm/Eterm-menu.cfg irc/irc-menu.cfg chooser/chooser-menu.cfg emacs/emacs-menu.cfg \
             mutt/mutt-menu.cfg trans/trans-menu.cfg cEterm/cEterm-menu.cfg cEterm/theme.cfg.in

install-data-hook:
	-@for i in $(THEMES) ; do \
	  $(SED) -e 's%@''PREFIX''@%${prefix}%g' -e 's%@''PKGDATADIR''@%${DESTDIR}${pkgdatadir}%g' -e 's%@''THEME''@%'$$i'%g' \
	    $(srcdir)/$$i/theme.cfg.in > $(srcdir)/$$i/theme.cfg ; \
	done
	$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/themes
	-@for i in $(THEMES) ; do \
	  if test ! -d $(DESTDIR)$(pkgdatadir)/themes/$$i ; then \
	    echo "Installing $$i theme in $(DESTDIR)$(pkgdatadir)/themes" ; \
	    $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/themes/$$i ; \
	    $(CP) $(srcdir)/$$i/???* $(DESTDIR)$(pkgdatadir)/themes/$$i ; \
	    $(CHMOD) 755 $(DESTDIR)$(pkgdatadir)/themes/$$i ; \
	    $(CHMOD) 644 $(DESTDIR)$(pkgdatadir)/themes/$$i/???* ; \
	    rm -f $(DESTDIR)$(pkgdatadir)/themes/$$i/theme.cfg.in ; \
	  else \
	    echo "ALERT!  Not overwriting theme $$i in $(DESTDIR)$(pkgdatadir)/themes.  You will need to update this theme manually." ; \
	  fi ; \
	done
