include $(top_srcdir)/common.mk
gplugins_LTLIBRARIES = libssotest.la
NULL=

libssotest_la_CPPFLAGS = \
    -I$(top_builddir) \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/include \
    $(GSIGNOND_CFLAGS) \
    $(NULL)

libssotest_la_LIBADD = \
    $(top_builddir)/src/common/libgsignond-common.la \
    $(GSIGNOND_LIBS) \
    $(NULL)

libssotest_la_SOURCES = \
    gsignond-ssotest-plugin.h \
    gsignond-ssotest-plugin.c \
    $(NULL)

libssotest_la_LDFLAGS = -avoid-version

all-local: slink

slink:
	ln -sf $(abs_top_builddir)/src/plugins/ssotest/.libs/libssotest.so ../.libs/libssotest.so
    

