# $Id$

man_MANS = Eterm.1

all: Makefile Eterm.1.html terminfo

Eterm.1: $(top_srcdir)/doc/Eterm.1.in
	$(SED) -e 's%@''VERSION''@%$(VERSION)%g' -e 's%@''DATE''@%$(DATE)%g' -e 's%@THEMEDIR@%$(pkgdatadir)/themes%g' $(top_srcdir)/doc/$@.in > $@

Eterm.1.html: Eterm.1
	-nroff -man $< | man2html -title Eterm > $@

terminfo: Eterm.ti
	-$(TIC) Eterm.ti

EXTRA_DIST = Eterm.1.in Eterm_reference.html Eterm.tcap Eterm.ti Makefile.am Makefile.in

