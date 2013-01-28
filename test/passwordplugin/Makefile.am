TESTS = passwordplugintest
TESTS_ENVIRONMENT= SSO_PLUGINS_DIR=$(top_builddir)/src/plugins/password/.libs

check_PROGRAMS = passwordplugintest
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
