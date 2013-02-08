if HAVE_GTESTDBUS
TESTS = daemontest
check_PROGRAMS = $(TESTS)

daemontest_SOURCES = daemon-test.c

daemontest_CFLAGS = \
    $(GSIGNOND_CFLAGS) \
    $(CHECK_CFLAGS) \
    -DG_LOG_DOMAIN=\"gsignond-test-server\" \
    -DGSIGNOND_TEST_DBUS_SERVICE_DIR=\""$(abs_top_builddir)/test/daemon/services"\" \
    -I$(top_srcdir)/src/ \
    -I$(top_srcdir)/include/ 

daemontest_LDADD = \
    $(top_builddir)/src/common/libgsignond-common.la \
    $(top_builddir)/src/daemon/dbus/libgsignond-dbus-glue.la \
    $(GSIGNOND_LIBS) \
    $(CHECK_LIBS)

endif

EXTRA_DIST = "$(top_builddir)/test/daemon/services/com.google.code.AccountsSSO.SingleSignOn.service.in"
