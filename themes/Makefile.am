# $Id$

THEMES = Eterm chooser emacs irc mutt tn3270 trans

EXTRA_DIST = Eterm/Eterm.menu Eterm/MAIN.in irc/MAIN.in irc/irc.menu active.tags chooser/chooser.menu chooser/MAIN.in emacs/MAIN.in emacs/emacs.menu mutt/MAIN.in mutt/mutt.menu tn3270/MAIN.in trans/MAIN.in trans/trans.menu

install-data-hook:
	-@for i in $(THEMES) ; do \
	  $(SED) -e 's%@''PREFIX''@%${prefix}%g' -e 's%@''DATADIR''@%${datadir}%g' -e 's%@''THEME''@%'$$i'%g' $$i/MAIN.in > $$i/MAIN ; \
	done
	$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/themes
	-@for i in $(THEMES) ; do \
	  if test ! -d $(DESTDIR)$(pkgdatadir)/themes/$$i ; then \
	    echo "Installing $$i theme in $(DESTDIR)$(pkgdatadir)/themes" ; \
	    $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/themes/$$i ; \
	    $(CP) $$i/???* $(DESTDIR)$(pkgdatadir)/themes/$$i ; \
	    $(CHMOD) 755 $(DESTDIR)$(pkgdatadir)/themes/$$i ; \
	    $(CHMOD) 644 $(DESTDIR)$(pkgdatadir)/themes/$$i/???* ; \
	    rm -f $(DESTDIR)$(pkgdatadir)/themes/$$i/MAIN.in ; \
	  else \
	    echo "ALERT!  Not overwriting theme $$i in $(DESTDIR)$(pkgdatadir)/themes.  You will need to update this theme manually." ; \
	  fi ; \
	done
	@if test ! -f $(HOME)/.active.tags ; then $(CP) active.tags $(HOME)/.active.tags ; \
	   else echo "Not installing $(HOME)/.active.tags -- File exists." ; fi
