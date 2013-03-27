TESTS = commontest
TESTS_ENVIRONMENT= SSO_PLUGINS_DIR=$(top_builddir)/src/plugins/password/.libs

check_PROGRAMS = commontest
commontest_SOURCES = commontest.c
commontest_CFLAGS = \
    $(GSIGNOND_CFLAGS) \
    $(CHECK_CFLAGS) \
    -I$(top_srcdir)/include/ 

commontest_LDADD = \
    $(top_builddir)/src/common/libgsignond-common.la \
    $(GSIGNOND_LIBS) \
    $(CHECK_LIBS)