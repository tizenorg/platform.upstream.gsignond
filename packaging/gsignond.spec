Name: gsignond
Summary: GLib based Single Sign-On daemon
Version: 0.0.0
Release: 8
Group: System/Daemons
License: LGPL
Source: %{name}-%{version}.tar.bz2
Requires: dbus-1
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires: pkgconfig(gtk-doc)
BuildRequires: pkgconfig(glib-2.0) >= 2.30
BuildRequires: pkgconfig(gobject-2.0)
BuildRequires: pkgconfig(gio-2.0)
BuildRequires: pkgconfig(gio-unix-2.0)
BuildRequires: pkgconfig(gmodule-2.0)
BuildRequires: pkgconfig(sqlite3)


%description
%{summary}.


%package devel
Summary:    Development files for %{name}
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
%{summary}.


%prep
%setup -q -n %{name}-%{version}
#gtkdocize
# for repository snapshot packages
aclocal
autoheader
libtoolize --copy --force
autoconf
automake --add-missing --copy
autoreconf --install --force
# fore release source packages
#autoreconf -f -i


%build
%configure --enable-dbus-type=session
make %{?_smp_mflags}


%install
rm -rf %{buildroot}
%make_install


%post
/sbin/ldconfig
chmod u+s %{_bindir}/%{name}


%postun -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
%doc AUTHORS COPYING.LIB INSTALL NEWS README
%{_bindir}/%{name}
%{_bindir}/%{name}-plugind
%{_libdir}/lib%{name}-*.so.*
%{_libdir}/%{name}/extensions/*.so*
%{_libdir}/%{name}/plugins/*.so*
%{_datadir}/dbus-1/services/*SingleSignOn*.service
%exclude %{_libdir}/gsignond/extensions/*.la
%exclude %{_libdir}/gsignond/plugins/*.la


%files devel
%defattr(-,root,root,-)
#%{_includedir}/%{name}/*.h
%{_libdir}/lib%{name}-*.so
%{_libdir}/lib%{name}-*.la
%{_libdir}/pkgconfig/%{name}.pc
%{_datadir}/dbus-1/interfaces/*SingleSignOn*.xml
%{_datadir}/dbus-1/interfaces/*SSO*.xml


%changelog
* Thu Feb 08 2013 Jussi Laako <jussi.laako@linux.intel.com>
- Initial RPM packaging

