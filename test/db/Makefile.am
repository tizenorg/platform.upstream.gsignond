TESTS = dbtest
TESTS_ENVIRONMENT=
VALGRIND_TESTS_DISABLE=
include $(top_srcdir)/test/valgrind_common.mk

check_PROGRAMS = dbtest
dbtest_SOURCES = dbtest.c
dbtest_CFLAGS = \
    $(GSIGNOND_CFLAGS) \
    $(CHECK_CFLAGS) \
    -I$(top_srcdir)/src/ \
    -I$(top_srcdir)/include/ 

dbtest_LDADD = \
    $(top_builddir)/src/common/libgsignond-common.la \
    $(top_builddir)/src/daemon/db/libgsignond-db.la \
    $(GSIGNOND_LIBS) \
    $(CHECK_LIBS)
