include $(top_srcdir)/common.mk
plugins_LTLIBRARIES = libdigest.la
NULL=

libdigest_la_CPPFLAGS = \
    -I$(top_srcdir) \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/include \
    $(GSIGNOND_CFLAGS) \
    $(NULL)

libdigest_la_LIBADD = \
    $(top_builddir)/src/common/libgsignond-common.la \
    $(GSIGNOND_LIBS) \
    $(NULL)

libdigest_la_SOURCES = \
    gsignond-digest-plugin.c \
    $(NULL)

all-local: slink

slink:
	ln -sf $(top_builddir)/src/plugins/digest/.libs/libdigest.so ../.libs/libdigest.so

