# define used dbus type [p2p, session, system]
%define dbus_type p2p
# enable debug features such as control environment variables
# WARNING! do not use for production builds as it will break security
%define debug_build 0

Name: gsignond
Summary: GLib based Single Sign-On daemon
Version: 1.0.3
Release: 0
Group: Security/Accounts
License: LGPL-2.1+ and GPL-2.0+
Source: %{name}-%{version}.tar.gz
URL: https://01.org/gsso
Source1001: %{name}.manifest
Source1002: gsignond-cleandb
Provides: gsignon
%if %{dbus_type} != "p2p"
BuildRequires: dbus-1
%endif
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires: pkgconfig(dbus-1)
BuildRequires: pkgconfig(glib-2.0) >= 2.30
BuildRequires: pkgconfig(gobject-2.0)
BuildRequires: pkgconfig(gio-2.0)
BuildRequires: pkgconfig(gio-unix-2.0)
BuildRequires: pkgconfig(gmodule-2.0)
BuildRequires: pkgconfig(sqlite3)
BuildRequires: pkgconfig(libecryptfs)
BuildRequires: pkgconfig(libsmack)

%description
%{summary} package

%package devel
Summary:    Development files for %{name}
Group:      SDK/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
%{summary} package

%package doc
Summary:    Documentation files for %{name}
Group:      SDK/Documentation
Requires:   %{name} = %{version}-%{release}

%description doc
%{summary} package

%prep
%setup -q -n %{name}-%{version}
cp %{SOURCE1001} .
cp %{SOURCE1002} .

%build
autoreconf -ivf
%if %{debug_build} == 1
%configure --enable-dbus-type=%{dbus_type} --enable-debug
%else
%configure --enable-dbus-type=%{dbus_type}
%endif

%__make %{?_smp_mflags}

%install
rm -rf %{buildroot}
%make_install
install -m 755 -d %{buildroot}%{_unitdir_user}
install -m 644 data/gsignond.service %{buildroot}%{_unitdir_user}/
install -m 755 -d %{buildroot}%{_unitdir_user}/weston.target.wants
ln -sf ../gsignond.service %{buildroot}%{_unitdir_user}/weston.target.wants/gsignond.service
install -m 755 -d %{buildroot}%{_sysconfdir}/gumd/userdel.d/
install -m 755 gsignond-cleandb %{buildroot}%{_sysconfdir}/gumd/userdel.d/

%post
/sbin/ldconfig
chmod u+s %{_bindir}/%{name}
getent group gsignond > /dev/null || /usr/sbin/groupadd -r gsignond

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%manifest %{name}.manifest
%doc AUTHORS COPYING.LIB
%{_bindir}/%{name}
%{_libdir}/lib%{name}-*.so.*
%{_libdir}/%{name}/extensions/*.so*
%{_libdir}/%{name}/gplugins/*.so*
%{_libdir}/%{name}/pluginloaders/%{name}-plugind
%if %{dbus_type} != "p2p"
%{_datadir}/dbus-1/services/*SingleSignOn*.service
%endif
%{_unitdir_user}/gsignond.service
%{_unitdir_user}/weston.target.wants/gsignond.service
%config(noreplace) %{_sysconfdir}/gsignond.conf
%{_sysconfdir}/gumd/userdel.d/

%files devel
%defattr(-,root,root,-)
%{_includedir}/%{name}/*.h
%{_libdir}/lib%{name}-*.so
%{_libdir}/pkgconfig/%{name}.pc
%if %{dbus_type} != "p2p"
%{_datadir}/dbus-1/interfaces/*SSO*.xml
%endif

%files doc
%defattr(-,root,root,-)
%{_datadir}/gtk-doc/html/gsignond/*
