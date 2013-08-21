TESTS = digestplugintest passwordplugintest pluginproxytest pluginremotetest

TESTS_ENVIRONMENT= SSO_PLUGINS_DIR=$(top_builddir)/src/plugins/.libs \
	SSO_BIN_DIR=$(top_builddir)/src/daemon/.libs \
    SSO_STORAGE_PATH=/tmp/gsignond \
    LD_LIBRARY_PATH="$(top_builddir)/src/daemon/plugins/plugind/.libs:$(top_builddir)/src/common/.libs:$(top_builddir)/src/common/db/.libs:$(top_builddir)/src/daemon/.libs:$(top_builddir)/src/daemon/db/.libs:$(top_builddir)/src/daemon/dbus/.libs:$(top_builddir)/src/daemon/plugins/.libs"

VALGRIND_TESTS_DISABLE =
    
check_PROGRAMS = digestplugintest passwordplugintest pluginproxytest pluginremotetest
include $(top_srcdir)/test/valgrind_common.mk

digestplugintest_SOURCES = digestplugintest.c
digestplugintest_CFLAGS = \
    $(GSIGNOND_CFLAGS) \
    $(CHECK_CFLAGS) \
    -I$(top_builddir) \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/src/plugins/digest \
    -I$(top_srcdir)/include

digestplugintest_LDADD = \
    $(top_builddir)/src/common/libgsignond-common.la \
    $(top_builddir)/src/plugins/digest/libdigest.la \
    $(GSIGNOND_LIBS) \
    $(CHECK_LIBS)

passwordplugintest_SOURCES = passwordplugintest.c
passwordplugintest_CFLAGS = \
    $(GSIGNOND_CFLAGS) \
    $(CHECK_CFLAGS) \
    -I$(top_builddir) \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/src/plugins/password \
    -I$(top_srcdir)/include/

passwordplugintest_LDADD = \
    $(top_builddir)/src/common/libgsignond-common.la \
    $(top_builddir)/src/plugins/password/libpassword.la \
    $(GSIGNOND_LIBS) \
    $(CHECK_LIBS)

pluginproxytest_SOURCES = pluginproxytest.c
pluginproxytest_CFLAGS = \
    $(GSIGNOND_CFLAGS) \
    $(CHECK_CFLAGS) \
    -I$(top_builddir) \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/src/daemon/plugins \
    -I$(top_srcdir)/include

pluginproxytest_LDADD = \
    $(top_builddir)/src/common/libgsignond-common.la \
    $(top_builddir)/src/daemon/plugins/libgsignond-plugins.la \
    $(GSIGNOND_LIBS) \
    $(CHECK_LIBS)

pluginremotetest_SOURCES = pluginremotetest.c
pluginremotetest_CFLAGS = \
    $(GSIGNOND_CFLAGS) \
    $(CHECK_CFLAGS) \
    -I$(top_builddir) \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/src/daemon/plugins \
    -I$(top_srcdir)/src/daemon/plugins/plugind \
    -I$(top_srcdir)/include

pluginremotetest_LDADD = \
    $(top_builddir)/src/common/libgsignond-common.la \
    $(top_builddir)/src/daemon/plugins/libgsignond-plugins.la \
    $(top_builddir)/src/daemon/plugins/plugind/libgsignond-plugind-daemon.la \
    $(GSIGNOND_LIBS) \
    $(CHECK_LIBS)
