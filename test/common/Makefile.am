TESTS = commontest
TESTS_ENVIRONMENT= SSO_GPLUGINS_DIR=$(top_builddir)/src/plugins/.libs \
    SSO_BIN_DIR=$(top_builddir)/src/daemon/.libs \
    SSO_STORAGE_PATH=/tmp/gsignond \
    LD_LIBRARY_PATH="$(top_builddir)/src/common/.libs:$(top_builddir)/src/common/db/.libs:$(top_builddir)/src/daemon/.libs:$(top_builddir)/src/daemon/db/.libs:$(top_builddir)/src/daemon/dbus/.libs:$(top_builddir)/src/daemon/plugins/.libs"

VALGRIND_TESTS_DISABLE=

check_PROGRAMS = commontest
include $(top_srcdir)/test/valgrind_common.mk
commontest_SOURCES = commontest.c
commontest_CFLAGS = \
    $(GSIGNOND_CFLAGS) \
    $(CHECK_CFLAGS) \
    -I$(top_builddir) \
    -I$(top_srcdir)/include/ \
    -I$(top_srcdir)/src/

commontest_LDADD = \
    $(top_builddir)/src/common/libgsignond-common.la \
    $(GSIGNOND_LIBS) \
    $(CHECK_LIBS)
