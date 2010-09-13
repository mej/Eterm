%if %{!?compression:1}0
%define compression gz
%endif

%define __os_install_post /usr/lib/rpm/brp-compress
%if %{?optflags:1}0
%define optflags ${RPM_OPT_FLAGS:--O0 -g3}
%endif

Summary: Enlightened terminal emulator
Name: Eterm
Version: 0.10
#Release: 1
Release: 0.%(date '+%Y%m%d')
License: BSD
Group: User Interface/X
URL: http://www.eterm.org/
Source0: http://www.eterm.org/download/%{name}-%{version}.tar.%{compression}
Source1: http://www.eterm.org/download/%{name}-bg-%{version}.tar.%{compression}
#BuildSuggests: xorg-x11-devel, XFree86-devel, xorg-x11-proto-devel, libXext-devel, libXt-devel, freetype-devel
BuildRequires: libast, imlib2-devel
Requires: imlib2, imlib2-loader_jpeg, imlib2-loader_png
BuildRoot: %{?_tmppath}%{!?_tmppath:/var/tmp}/%{name}-%{version}-%{release}-root

%description
Eterm is a color vt102 terminal emulator with enhanced graphical
capabilities.  Eterm is intended to be a replacement for xterm for
Enlightenment window manager users, but it can also be used as a
replacement for xterm by users without Enlightenment.  Eterm supports
various themes and is very configurable, in keeping with the
philosophy of Enlightenment. If you install Eterm, you'll also need to
have the Imlib2 library installed.

%prep
%setup -a 1

%build
CFLAGS="%{optflags}"
export CFLAGS

# When using the configure macro, I also specify all the directory
# macros I use for compatibility with older versions of the macro
%configure --bindir=%{_bindir} --libdir=%{_libdir} --mandir=%{_mandir} \
           --datadir=%{_datadir} --sysconfdir=%{_sysconfdir} \
           --enable-multi-charset --enable-escreen --enable-auto-encoding \
           --with-debugging=9 %{?acflags}
%{__make} %{?mflags}

%install
%{__rm} -rf $RPM_BUILD_ROOT

# If the configure macro is used above (which it is), there
# is NO reason to use the makeinstall macro here, so don't.
%{__make} install DESTDIR=$RPM_BUILD_ROOT %{?mflags_install}

( cd $RPM_BUILD_ROOT
  %{__mv} .%{_bindir}/%{name} .%{_bindir}/%{name}-%{version}
  cd $RPM_BUILD_ROOT%{_bindir}
  %{__ln_s} -f %{name}-%{version} %{name}
  cd $RPM_BUILD_ROOT
  chmod +x .%{_libdir}/lib*so* ||:
)

%{__mkdir_p} $RPM_BUILD_ROOT%{_sysconfdir}/X11/applnk/Utilities
cat > $RPM_BUILD_ROOT%{_sysconfdir}/X11/applnk/Utilities/Eterm.desktop <<EOF
[Desktop Entry]
Name=Eterm
Comment=Eterm
TryExec=Eterm
Exec=Eterm
Icon=gnome-eterm.png
Terminal=0
Type=Application
EOF
chmod 0644 $RPM_BUILD_ROOT%{_sysconfdir}/X11/applnk/Utilities/Eterm.desktop

%post
/sbin/ldconfig || :

if [ -d /usr/share/terminfo -a ! -f /usr/share/terminfo/E/Eterm ]; then
    tic -o/usr/share/terminfo %{_docdir}/%{name}-%{version}/%{name}.ti || :
fi

%postun
/sbin/ldconfig || :

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)
%doc doc/Eterm_reference.html doc/Eterm.1.html doc/Eterm.tcap doc/Eterm.ti doc/README.Escreen
%doc LICENSE README ReleaseNotes ReleaseNotes.1 ChangeLog
%config %{_sysconfdir}/X11/applnk/Utilities/Eterm.desktop
%{_bindir}/*
%{_libdir}/*
%{_mandir}/man1/*
%{_datadir}/%{name}/*

%changelog
