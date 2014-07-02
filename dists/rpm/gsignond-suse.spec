# define used dbus type [p2p, session, system]
%define dbus_type session
# enable debug features such as control environment variables
# WARNING! do not use for production builds as it will break security
%define debug_build 0

Name: gsignond
Summary: GLib based Single Sign-On daemon
Version: 1.0.3
Release: 1
Group: System/Daemons
License: LGPL-2.1+
Source: %{name}-%{version}.tar.gz
URL: https://01.org/gsso
Provides: gsignon
%if %{dbus_type} != "p2p"
Requires: dbus-1
%endif
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires: pkgconfig(dbus-1)
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


%package doc
Summary:    Documentation files for %{name}
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description doc
%{summary}.


%prep
%setup -q -n %{name}-%{version}
# for repository snapshot packages
#gtkdocize
# fore release source packages
autoreconf -f -i


%build
%if %{debug_build} == 1
%configure --enable-dbus-type=%{dbus_type} --enable-debug --enable-gtk-doc
%else
%configure --enable-dbus-type=%{dbus_type}
%endif

make %{?_smp_mflags}


%install
rm -rf %{buildroot}
%make_install


%post
/sbin/ldconfig
chmod u+s %{_bindir}/%{name}
groupadd -f -r gsignond


%postun -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
%doc AUTHORS COPYING.LIB INSTALL NEWS README
%{_bindir}/%{name}
%{_libdir}/lib%{name}-*.so.*
%{_libdir}/%{name}/extensions/*.so*
%{_libdir}/%{name}/gplugins/*.so*
%{_libdir}/%{name}/pluginloaders/%{name}-plugind
%if %{dbus_type} != "p2p"
%{_datadir}/dbus-1/services/*SingleSignOn*.service
%endif
%exclude %{_libdir}/gsignond/extensions/*.la
%exclude %{_libdir}/gsignond/gplugins/*.la
%config(noreplace) %{_sysconfdir}/gsignond.conf


%files devel
%defattr(-,root,root,-)
%{_includedir}/%{name}/*.h
%{_libdir}/lib%{name}-*.so
%{_libdir}/lib%{name}-*.la
%{_libdir}/pkgconfig/%{name}.pc
%if %{dbus_type} != "p2p"
%{_datadir}/dbus-1/interfaces/*SSO*.xml
%endif


%files doc
%defattr(-,root,root,-)
%{_datadir}/gtk-doc/html/gsignond/*


%changelog
* Mon Jun 30 2014 Imran Zaman <imran.zaman@intel.com>
- Release 1.0.3

* Fri Mar 07 2014 Jussi Laako <jussi.laako@linux.intel.com>
- Release 1.0.1

* Thu Mar 06 2014 Imran Zaman <imran.zaman@intel.com>
- Release 1.0.0

* Fri Feb 28 2014 Jussi Laako <jussi.laako@linux.intel.com>
- Release 0.0.4

* Thu Aug 22 2013 Amarnath Valluri <amarnath.valluri@linux.intel.com>
- Release 0.0.3
- Bug fixes in UI interaction
- Documentation support

* Mon Jun 24 2013 Imran Zaman <imran.zaman@intel.com>
- Release 0.0.2 that comprises of bug fixes

* Wed Jun 12 2013 Jussi Laako <jussi.laako@linux.intel.com>
- Prepare for first release

* Thu Feb 08 2013 Jussi Laako <jussi.laako@linux.intel.com>
- Initial RPM packaging

