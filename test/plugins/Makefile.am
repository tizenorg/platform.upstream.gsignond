TESTS = digestplugintest passwordplugintest pluginproxytest
TESTS_ENVIRONMENT= SSO_PLUGINS_DIR=$(top_builddir)/src/plugins/.libs
VALGRIND_TESTS_DISABLE =
include $(top_srcdir)/test/valgrind_common.mk
    
check_PROGRAMS = digestplugintest passwordplugintest pluginproxytest
digestplugintest_SOURCES = digestplugintest.c
digestplugintest_CFLAGS = \
    $(GSIGNOND_CFLAGS) \
    $(CHECK_CFLAGS) \
    -I$(top_srcdir)/src/plugins/digest \
    -I$(top_srcdir)/include/

digestplugintest_LDADD = \
    $(top_builddir)/src/common/libgsignond-common.la \
    $(top_builddir)/src/plugins/digest/libdigest.la \
    $(GSIGNOND_LIBS) \
    $(CHECK_LIBS)

passwordplugintest_SOURCES = passwordplugintest.c
passwordplugintest_CFLAGS = \
    $(GSIGNOND_CFLAGS) \
    $(CHECK_CFLAGS) \
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
    -I$(top_srcdir)/src/daemon/plugins \
    -I$(top_srcdir)/include/ 

pluginproxytest_LDADD = \
    $(top_builddir)/src/common/libgsignond-common.la \
    $(top_builddir)/src/daemon/plugins/libgsignond-plugins.la \
    $(GSIGNOND_LIBS) \
    $(CHECK_LIBS)



