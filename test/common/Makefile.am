TESTS = commontest
TESTS_ENVIRONMENT=
VALGRIND_TESTS_DISABLE=

check_PROGRAMS = commontest
include $(top_srcdir)/test/valgrind_common.mk
commontest_SOURCES = commontest.c
commontest_CFLAGS = \
    $(GSIGNOND_CFLAGS) \
    $(CHECK_CFLAGS) \
    -I$(top_srcdir)/include/ 

commontest_LDADD = \
    $(top_builddir)/src/common/libgsignond-common.la \
    $(GSIGNOND_LIBS) \
    $(CHECK_LIBS)
