#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.60])
AC_INIT([gsignond], [0.0.3],[],[],[http://01.org/gsso])
AC_CONFIG_SRCDIR([src/daemon/main.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 nostdinc silent-rules subdir-objects tar-pax -Wno-portability])

# Checks for programs.
AC_PROG_CC
AM_PROG_AR
AX_CHECK_GNU_MAKE
if test "x$ifGNUmake" = "x#" ; then
	AC_MSG_ERROR("GNUmake is required")
fi

#libtool
LT_PREREQ([2.2])
LT_INIT([disable-static])

# Checks for libraries.
PKG_CHECK_MODULES([GSIGNOND], 
                  [glib-2.0 >= 2.30
                   gio-2.0
                   gio-unix-2.0
                   gmodule-2.0
                   sqlite3])
AC_SUBST(GSIGNOND_CFLAGS)
AC_SUBST(GSIGNOND_LIBS)


# AM_PATH_CHECK() is deprecated, but check documentation fails to tell that :-/
PKG_CHECK_MODULES([CHECK], [check >= 0.9.4], [have_check=yes], [have_check=no])
AC_SUBST(CHECK_CFLAGS)
AC_SUBST(CHECK_LIBS)

PKG_CHECK_MODULES([LIBSMACK], libsmack >= 1.0, [libsmack=yes], [libsmack=no])
AM_CONDITIONAL(HAVE_LIBSMACK, [test x$libsmack = xyes])
if test "x$libsmack" = xyes; then
    AC_DEFINE(HAVE_LIBSMACK, [1], [Define if libsmack exists.])
fi

PKG_CHECK_MODULES([LIBECRYPTFS], libecryptfs >= 96, [libecryptfs=yes], [libecryptfs=no])
AM_CONDITIONAL(HAVE_LIBECRYPTFS, [test x$libecryptfs = xyes])
if test "x$libecryptfs" = xes; then
    AC_DEFINE(HAVE_LIBECRYPTFS, [1], [Define if libecryptfs exists.])
fi

AC_PATH_PROG(GLIB_MKENUMS, glib-mkenums)

# allow fake installations for "distcheck"
AC_ARG_ENABLE(distcheck,
	      [  --enable-distcheck build for distcheck target],
	      [enable_distcheck=yes], [enable_distcheck=no])
echo -n "running distcheck... "
echo $enable_distcheck

AC_ARG_ENABLE(dbus-type,
              [  --enable-dbus-type=dbus-type specify daemon dbus type:
                          p2p(default): uses peer to peer dbus
                          session: uses session message bus
                          system: uses system message bus],
              [enable_dbus_type=$enableval], [enable_dbus_type=p2p])
echo "----------------------"
echo " DBUS TYPE : '$enable_dbus_type'"
echo "----------------------"
if test "x$enable_dbus_type" = "xp2p" ; then
    AC_DEFINE(USE_P2P, [1], [Use peer to peer dbus])
elif test "x$enable_dbus_type" = "xsession" ; then
    AC_DEFINE(GSIGNOND_BUS_TYPE, [G_BUS_TYPE_SESSION], [Use session bus])
    if test "x$enable_distcheck" = "xyes" ; then
        DBUS_SERVICES_DIR="${datadir}/dbus-1/services"
    else
        DBUS_SERVICES_DIR="`pkg-config --variable session_bus_services_dir dbus-1`"
    fi
    
    PKG_CHECK_MODULES(GTestDBus, gio-2.0 >= 2.34, [gtestdbus=yes], [gtestdbus=no])
    if test "x$gtestdbus" = xyes; then
       AC_DEFINE(HAVE_GTESTDBUS, [1], [Define if GTestDBus exists.])
    fi
elif test "x$enable_dbus_type" = "xsystem" ; then
    AC_DEFINE(GSIGNOND_BUS_TYPE, [G_BUS_TYPE_SYSTEM], [Use system bus])
    if test "x$enable_distcheck" = "xyes" ; then
        DBUS_SERVICES_DIR="${datadir}/dbus-1/system-services"
    else
        DBUS_SERVICES_DIR="`pkg-config --variable system_bus_services_dir dbus-1`"
    fi
fi
if test "x$enable_dbus_type" != "xp2p" ; then
    if test "x$enable_distcheck" = "xyes" ; then
        DBUS_INTERFACES_DIR="${datadir}/dbus-1/interfaces"
    else
        DBUS_INTERFACES_DIR="`pkg-config --variable interfaces_dir dbus-1`"
    fi
fi

AC_ARG_ENABLE(keychain,
              [  --enable-keychain=sysctx
                          enable keychain access with
			  system context "sysctx"],
              [enable_keychain=$enableval])
if test "x$enable_keychain" != "x" ; then
    AC_DEFINE_UNQUOTED(KEYCHAIN_SYSCTX,
		       ["$enable_keychain"],
		       [Keychain system context])
    AC_SUBST(KEYCHAIN_SYSCTX,
	     ["#KeychainSystemContext = $enable_keychain"])
else
    AC_SUBST(KEYCHAIN_SYSCTX,
	     ["#KeychainSystemContext = \"\""])
fi

AC_ARG_ENABLE(storagedir,
	      [  --enable-storagedir=path
	                  enable storage at location "path"
			  instead of default "/var/db"],
              [enable_storagedir=$enableval],
	      [enable_storagedir="/var/db"])
AC_DEFINE_UNQUOTED(BASE_STORAGE_DIR,
		   ["$enable_storagedir"],
		   [Base path for user specific storage directories])
AC_SUBST(BASE_STORAGE_DIR,
	 ["#StoragePath = $enable_storagedir"])

AC_ARG_ENABLE(debug,
	      [  --enable-debug          enable debug features],
	      [enable_debug=yes], [enable_debug=no])
echo -n "enable debug features... "
echo $enable_debug
if test "x$enable_debug" = "xyes" ; then
    AC_DEFINE(ENABLE_DEBUG, [1], [Enable debug features])
    AC_ARG_ENABLE(sql-log,
          [  --enable-sql-log        enable sql log feature],
          [enable_sql_log=yes], [enable_sql_log=no])
    echo -n "enable sql log feature... "
    echo $enable_sql_log
    if test "x$enable_sql_log" = "xyes" ; then
        AC_DEFINE(ENABLE_SQL_LOG, [1], [Enable SQL log feature])
    fi
fi

AC_ARG_ENABLE(acltriggers,
              [  --enable-acltriggers    enable ACL before delete triggers],
              [enable_acl_triggers=yes], [enable_acl_triggers=no])
if test "x$enable_acl_triggers" = "xyes" ; then
    AC_DEFINE(ENABLE_DB_ACL_TRIGGERS, [1], [Enable ACL triggers])
fi

AM_CONDITIONAL(USE_GTESTDBUS, [test x$gtestdbus = xyes])
AC_SUBST(MESSAGE_BUS_TYPE, [$enable_dbus_type])

AM_CONDITIONAL(HAVE_DEBUG, [test x$enable_debug = xyes])
AM_CONDITIONAL(SET_PERMISSIONS, [test x$enable_distcheck != xyes])

AC_SUBST(DBUS_SERVICES_DIR)
AC_SUBST(DBUS_INTERFACES_DIR)

# gtk-doc
GTK_DOC_CHECK([1.18],[--flavour no-tmpl])

# Checks for header files.
AC_CHECK_HEADERS([string.h])

# Enable feature sets
GSIGNOND_CFLAGS="$GSIGNOND_CFLAGS -D_POSIX_C_SOURCE=\\\"200809L\\\" -D_GNU_SOURCE -D_REENTRANT -D_THREAD_SAFE"

# Checks for typedefs, structures, and compiler characteristics.
GSIGNOND_CFLAGS="$GSIGNOND_CFLAGS -Wall -Werror -DG_LOG_DOMAIN=\\\"gsignond\\\""
# GSIGNOND_LIBS="$GSIGNOND_LIBS -lduma"

# Checks for library functions.

AC_OUTPUT([
Makefile
${PACKAGE_NAME}.pc
${PACKAGE_NAME}-uninstalled.pc
gsignond.conf
docs/Makefile
src/Makefile
src/common/Makefile
src/common/db/Makefile
src/daemon/Makefile
src/daemon/db/Makefile
src/daemon/dbus/Makefile
src/daemon/dbus/services/com.google.code.AccountsSSO.gSingleSignOn.service
src/daemon/plugins/Makefile
src/daemon/plugins/plugind/Makefile
src/extensions/Makefile
src/extensions/test/Makefile
src/extensions/tizen/Makefile
src/plugins/Makefile
src/plugins/password/Makefile
src/plugins/ssotest/Makefile
src/plugins/digest/Makefile
test/Makefile
test/common/Makefile
test/db/Makefile
test/daemon/Makefile
test/daemon/gsignond-dbus.conf
test/plugins/Makefile
])

if test x$enable_dbus_type != xp2p; then
    AC_OUTPUT([
    test/daemon/services/com.google.code.AccountsSSO.gSingleSignOn.service
    ])
fi
