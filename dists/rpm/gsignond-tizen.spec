# define used dbus type [p2p, session, system]
%define dbus_type p2p
# enable debug features such as control environment variables
# WARNING! do not use for production builds as it will break security
%define debug_build 0

Name: gsignond
Summary: GLib based Single Sign-On daemon
Version: 1.0.0
Release: 1
Group: System/Daemons
License: LGPL-2.1+
Source: %{name}-%{version}.tar.gz
URL: https://01.org/gsso
Source1001:     %{name}.manifest
Provides: gsignon
%if %{dbus_type} != "p2p"
Requires: dbus-1
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
if [ -f = "gtk-doc.make" ]
then
rm gtk-doc.make
fi
touch gtk-doc.make
autoreconf -f -i


%build
%if %{debug_build} == 1
%configure --enable-dbus-type=%{dbus_type} --enable-debug
%else
%configure --enable-dbus-type=%{dbus_type}
%endif

make %{?_smp_mflags}


%install
rm -rf %{buildroot}
%make_install
cp -a %{SOURCE1001} %{buildroot}%{_datadir}/%{name}.manifest


%post
/sbin/ldconfig
chmod u+s %{_bindir}/%{name}
getent group gsignond > /dev/null || /usr/sbin/groupadd -r gsignond


%postun -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
%manifest %{_datadir}/%{name}.manifest
%doc AUTHORS COPYING.LIB INSTALL NEWS README
%{_bindir}/%{name}
%{_libdir}/lib%{name}-*.so.*
%{_libdir}/%{name}/extensions/*.so*
%{_libdir}/%{name}/gplugins/*.so*
%{_libdir}/%{name}/pluginloaders/%{name}-plugind
%if %{dbus_type} != "p2p"
%{_datadir}/dbus-1/services/*SingleSignOn*.service
%endif
%config(noreplace) %{_sysconfdir}/gsignond.conf


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

