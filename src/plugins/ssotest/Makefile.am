include $(top_srcdir)/common.mk
plugins_LTLIBRARIES = libssotest.la
NULL=

libssotest_la_CPPFLAGS = \
    -I$(top_srcdir) \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/include \
    $(GSIGNOND_CFLAGS) \
    $(NULL)

libssotest_la_LIBADD = \
    $(top_builddir)/src/common/libgsignond-common.la \
    $(GSIGNOND_LIBS) \
    $(NULL)

libssotest_la_SOURCES = \
    gsignond-ssotest-plugin.c \
    $(NULL)

CLEANFILES = 
