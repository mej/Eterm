# $Id$

man_MANS = Eterm.1

all: Makefile Eterm.1.html

Eterm.1: $(top_srcdir)/doc/Eterm.1.in
	$(SED) -e 's%@''VERSION''@%$(VERSION)%g' -e 's%@''DATE''@%$(DATE)%g' -e 's%@THEMEDIR@%$(pkgdatadir)/themes%g' $(top_srcdir)/doc/$@.in > $@

Eterm.1.html: Eterm.1
	(nroff -man $< | man2html -title Eterm > $@) || :

install-data-hook:
	$(TIC) Eterm.ti || (mkinstalldirs $(HOME)/.terminfo && $(TIC) -o$(HOME)/.terminfo Eterm.ti) || :
	-infocmp -C -sc Eterm | sed -e '/Reconstructed/d' -e '/\.\.sa=/d' \
	  -e 's!^\([[:space:]]:co#.*\):\\$$!\1:pa#64:Co#8:AF=\\E[3%dm:AB=\\E[4%dm:\\!' \
	  > Eterm.tcap.new && mv Eterm.tcap.new Eterm.tcap || rm -f Eterm.tcap.new
	-if test -f /usr/share/misc/termcap && test -f Eterm.tcap ; then \
	    grep 'Eterm-color' /usr/share/misc/termcap || cat Eterm.tcap >> /usr/share/misc/termcap ; \
	    cap_mkdb termcap ; \
	fi || :

EXTRA_DIST = Eterm.1.in Eterm_reference.html Eterm.tcap Eterm.ti Makefile.am Makefile.in

