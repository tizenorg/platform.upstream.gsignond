TESTS = daemontest

TESTS_ENVIRONMENT= SSO_GPLUGINS_DIR=$(top_builddir)/src/plugins/.libs \
	SSO_BIN_DIR=$(top_builddir)/src/daemon/.libs \
	LD_LIBRARY_PATH="$(top_builddir)/src/common/.libs:$(top_builddir)/src/common/db/.libs:$(top_builddir)/src/daemon/.libs:$(top_builddir)/src/daemon/db/.libs:$(top_builddir)/src/daemon/dbus/.libs:$(top_builddir)/src/daemon/plugins/.libs"

VALGRIND_TESTS_DISABLE=

check_PROGRAMS = daemontest
include $(top_srcdir)/test/valgrind_common.mk

daemontest_SOURCES = daemon-test.c

daemontest_CFLAGS = \
    $(GSIGNOND_CFLAGS) \
    $(CHECK_CFLAGS) \
    -U G_LOG_DOMAIN \
    -DG_LOG_DOMAIN=\"gsignond-test-server\" \
    -I$(top_builddir) \
    -I$(top_builddir)/src/ \
    -I$(top_srcdir)/src/ \
    -I$(top_srcdir)/include/

if USE_GTESTDBUS
daemontest_CFLAGS += \
    -DGSIGNOND_TEST_DBUS_SERVICE_DIR=\""$(abs_top_builddir)/test/daemon/services"\"
endif

daemontest_LDADD = \
    $(top_builddir)/src/common/libgsignond-common.la \
    $(top_builddir)/src/daemon/dbus/libgsignond-dbus-glue.la \
    $(GSIGNOND_LIBS) \
    $(CHECK_LIBS)

