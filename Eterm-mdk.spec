Summary: Enlightened terminal emulator
Name: Eterm
Version: 0.9.1
Release: 2mdk
Copyright: BSD
Group: Terminals
Source0: ftp://ftp.eterm.org/pub/Eterm/%{name}-%{ver}.tar.bz2
Source1: ftp://ftp.eterm.org/pub/Eterm/%{name}-bg-%{ver}.tar.bz2
Source2: Eterm-48.xpm
Source3: Eterm-32.xpm
Source4: Eterm-16.xpm
URL: http://www.eterm.org/
Packager: Joakim Bodin <joabo552@student.liu.se>
BuildRoot: %{_tmppath}/%{name}-root
Requires: imlib2 => 1.0.0, utempter
BuildRequires: imlib2-devel, utempter-devel

%description
Eterm is a color vt102 terminal emulator with enhanced graphical
capabilities.  Eterm is intended to be a replacement for xterm for
Enlightenment window manager users, but it can also be used as a
replacement for xterm by users without Enlightenment.  Eterm supports
various themes and is very configurable, in keeping with the
philosophy of Enlightenment. If you install Eterm, you'll also need to
have the Imlib2 library installed.

%package backgrounds
Summary: Backgrounds for Eterm
Group: Terminals
Requires: %{name} = %{version}
%description backgrounds
Install this package if you want additional backgrounds for Eterm

%prep
rm -rf $RPM_BUILD_ROOT

%setup -q -a 1

%build

if [ -x configure ]
then
  %configure --with-backspace=bs --with-delete=execute
else
  ./autogen.sh --prefix=%{_prefix} --with-backspace=bs --with-delete=execute
fi

%make

%install
# Install icons
mkdir -p $RPM_BUILD_ROOT/%{_iconsdir}
mkdir -p $RPM_BUILD_ROOT/%{_liconsdir}
mkdir -p $RPM_BUILD_ROOT/%{_miconsdir}
cp %{SOURCE2} $RPM_BUILD_ROOT/%{_liconsdir}/Eterm.xpm
cp %{SOURCE3} $RPM_BUILD_ROOT/%{_iconsdir}/Eterm.xpm
cp %{SOURCE4} $RPM_BUILD_ROOT/%{_miconsdir}/Eterm.xpm

# Make menu entry
mkdir -p $RPM_BUILD_ROOT/%{_menudir}
cat << EOF > $RPM_BUILD_ROOT/%{_menudir}/%{name}
?package(Eterm):\
 needs="x11"\
 section="Terminals"\
 title="Eterm"\
 longtitle="Eterm Enlightened Terminal Emulator for X Window"\
 command="Eterm"\
 icon=Eterm.xpm
EOF

#get the package listing for Eterm-backgrounds
echo "%defattr(-,root,root,0755)" > rpm-Eterm-bg-files
cat > rpm-Eterm-bg-files <<EOF
%defattr(-,root,root,0755)
%{_datadir}/%{name}/pix/tiled-pixmaps.list
%{_datadir}/%{name}/pix/scaled-pixmaps.list
%{_datadir}/%{name}/pix/pixmaps.list
EOF
find ./bg/ -type f -not -name '*.list' -not -name 'Makefile*' \
       -not -name 'README*' -not -name 014.jpg \
        -not -name 40.jpg -not -name blackstone.jpg -not -name giger1.jpg \
        -not -name circuit.jpg -not -name nebula.jpg \
        -not -name fourthday.jpg -not -name gaia.jpg \
        -not -name galleon.jpg -not -name Dragon.jpg \
	-not -name Root -not -name .cvsignore -not -name Entries \
	-not -name Repository  \
        | sed "s#^\./bg/[scati]*le#%{_datadir}/%{name}/pix#g" >> rpm-Eterm-bg-files

%makeinstall

%clean
rm -rf $RPM_BUILD_ROOT

%post
/sbin/ldconfig
%{update_menus}
for i in tiled-pixmaps.list scaled-pixmaps.list pixmaps.list ; do \
  mv -f %{_datadir}/%{name}/pix/$i.base %{_datadir}/%{name}/pix/$i ; done

%postun
/sbin/ldconfig
%{clean_menus}
for i in tiled-pixmaps.list scaled-pixmaps.list pixmaps.list ; do \
  rm -f %{_datadir}/%{name}/pix/$i* ; done

%pre backgrounds
for i in tiled-pixmaps.list scaled-pixmaps.list pixmaps.list ; do \
  mv -f %{_datadir}/%{name}/pix/$i %{_datadir}/%{name}/pix/$i.base ; done

%postun backgrounds
for i in tiled-pixmaps.list scaled-pixmaps.list pixmaps.list ; do \
  mv -f %{_datadir}/%{name}/pix/$i.base %{_datadir}/%{name}/pix/$i ; done

%files
%defattr(-,root,root,0755)
%doc doc/Eterm_reference.html doc/Eterm.1.html
%doc COPYING README ChangeLog
%{_bindir}/*
%{_libdir}/*
%{_mandir}/man1/*
%{_datadir}/%{name}/themes/*
%{_datadir}/%{name}/pix/014.jpg
%{_datadir}/%{name}/pix/40.jpg
%{_datadir}/%{name}/pix/blackstone.jpg
%{_datadir}/%{name}/pix/giger1.jpg
%{_datadir}/%{name}/pix/circuit.jpg
%{_datadir}/%{name}/pix/nebula.jpg
%{_datadir}/%{name}/pix/fourthday.jpg
%{_datadir}/%{name}/pix/gaia.jpg
%{_datadir}/%{name}/pix/galleon.jpg
%{_datadir}/%{name}/pix/Dragon.jpg
%{_datadir}/%{name}/pix/bar_horizontal_[123].png
%{_datadir}/%{name}/pix/bar_vertical_[123].png
%{_datadir}/%{name}/pix/button_arrow_up_[123].png
%{_datadir}/%{name}/pix/button_arrow_down_[123].png
%{_datadir}/%{name}/pix/menu[123].png
%{_liconsdir}/Eterm.xpm
%{_iconsdir}/Eterm.xpm
%{_miconsdir}/Eterm.xpm
#%dir %{_datadir}/%{name}
#%dir %{_datadir}/%{name}/pix

%files backgrounds -f rpm-Eterm-bg-files

%changelog
* Sat Sep 30 2000 Joakim Bodin <joabo552@student.liu.se>
- Put this Eterm 0.9 spec into Eterm 0.9.1 with the only modification being
  that this version requires imlib2 instead of imlib

* Sat Sep 30 2000 Joakim Bodin <joabo552@student.liu.se
- Mandrake:ized spec file
- "Borrowed" Eterm.xpm icons from Eterm-0.8.10 mdk package
- "Borrowed" Menu entry for Eterm from Eterm-0.8.10 mdk package

* Fri Jan 29 2000 Tim Powers <timp@redhat.com>
- rebuilt for 6.2 powertools
- bzipped source to conserve space
- using percent configure so that libtoolize is run, needed for some of the
	newer alphas instead of ./configure --prefix=
- stripping binaries again

* Fri Dec 10 1999 Michael Jennings <mej@eterm.org>
- Added Tim's spec file to CVS as Eterm.spec.in for 0.9

* Wed Dec 8 1999 Tim Powers <timp@redhat.com>
- using unified patch for utempter and themes from Michael Jennings

* Tue Dec 7 1999 Tim Powers <timp@redhat.com>
- added wmconfig entry
- split up into 2 packages, Eterm proper, and Eterm-backgrounds
- thanks to ewt, we no longer have to make Eterm suid root, uses utempter
        instead

* Mon Dec 6 1999 Tim Powers <timp@redhat.com>
- updated to 0.8.10
- patched so that Eterm finds pix/themes in the right place
- new version fixes problems with utmp, conforms to Eterm docs.
- added RedHat.Eterm_suid which includes instructions on how to run Eterm in
        order to have it seen by "w" and "who" as a regular user

* Fri Aug 20 1999 Tim Powers <timp@redhat.com>
- fixed roblem with removing all files when uninstalling Eterm.

* Tue Jul 27 1999 Tim Powers <timp@redhat.com>
- updated version to 0.8.9
- cleaned up spec
- updated patch
- includes new backgrounds
- built for 6.1

* Mon Apr 05 1999 Michael Maher <mike@redhat.com>
- update to 0.8.8

* Fri Oct 23 1998 Jeff Johnson <jbj@redhat.com>
- update to 0.8.7.

* Fri Oct 08 1998 Michael Maher <mike@redhat.com>
- built eterm
