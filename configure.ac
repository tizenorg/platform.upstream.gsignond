#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.60])
AC_INIT([gsignond], [0.1],[],[],[http://code.google.com/p/accounts-sso/])
AC_CONFIG_SRCDIR([src/daemon/main.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 nostdinc silent-rules subdir-objects -Wno-portability])

# Checks for programs.
AC_PROG_CC
AM_PROG_AR

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

PKG_CHECK_MODULES(GTestDBus, glib-2.0 >= 2.34, [gtestdbus=yes], [gtestdbus=no])
AM_CONDITIONAL(HAVE_GTESTDBUS, [test x$gtestdbus = xyes])
if test "x$gtestdbus" = xyes; then
    AC_DEFINE(HAVE_GTESTDBUS, [1], [Define if GTestDBus exists.])
fi

AC_PATH_PROG(GLIB_MKENUMS, glib-mkenums)

AC_ARG_ENABLE(dbus-type,
              [  --enable-dbus-type=dbus-type specify daemon dbus type:
                          p2p(default): uses peer to peer dbus
                          session: uses session message bus
                          system: uses system message bus],
              [enable_dbus_type=$enableval], [enable_dbus_type=p2p])

echo "----------------------"
echo " DBUS TYPE : '$enable_dbus_type'"
echo "-----------------------"
if test "x$enable_dbus_type" = "xp2p" ; then
    AC_DEFINE(USE_P2P, [1], [Use peer to peer dbus])
elif test "x$enable_dbus_type" = "xsession" ; then
    AC_DEFINE(GSIGNOND_BUS_TYPE, [G_BUS_TYPE_SESSION], [Use session bus])
    DBUS_SERVICES_DIR="`pkg-config --variable session_bus_services_dir dbus-1`"
elif test "x$enable_dbus_type" = "xsystem" ; then
    AC_DEFINE(GSIGNOND_BUS_TYPE, [G_BUS_TYPE_SYSTEM], [Use system bus])
    DBUS_SERVICES_DIR="`pkg-config --variable system_bus_services_dir dbus-1`"
fi

AC_SUBST(DBUS_SERVICES_DIR)
DBUS_INTERFACES_DIR="`pkg-config --variable interfaces_dir dbus-1`"
AC_SUBST(DBUS_INTERFACES_DIR)

# gtk-doc
GTK_DOC_CHECK

# Checks for header files.
AC_CHECK_HEADERS([string.h])

# Checks for typedefs, structures, and compiler characteristics.
GSIGNOND_CFLAGS="$GSIGNOND_CFLAGS -Wall -Werror -DG_LOG_DOMAIN=\\\"gsignond\\\""
# GSIGNOND_LIBS="$GSIGNOND_LIBS -lduma"

# Checks for library functions.

AC_OUTPUT([
Makefile
${PACKAGE_NAME}.pc
${PACKAGE_NAME}-uninstalled.pc
src/Makefile
src/common/Makefile
src/common/db/Makefile
src/daemon/Makefile
src/daemon/db/Makefile
src/daemon/dbus/Makefile
src/daemon/dbus/services/com.google.code.AccountsSSO.gSingleSignOn.service
src/daemon/plugins/Makefile
src/extensions/Makefile
src/extensions/test/Makefile
src/plugins/Makefile
src/plugins/password/Makefile
src/plugins/ssotest/Makefile
src/plugins/digest/Makefile
test/Makefile
test/common/Makefile
test/db/Makefile
test/daemon/Makefile
test/daemon/services/com.google.code.AccountsSSO.gSingleSignOn.service
test/plugins/Makefile
])
