lib_LTLIBRARIES = libgsignond-plugins.la

libgsignond_plugins_la_CPPFLAGS = \
    -I$(top_srcdir) \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/include \
    $(GSIGNOND_CFLAGS)

libgsignond_plugins_la_LIBADD =    \
        $(top_srcdir)/src/common/libgsignond-common.la \
        $(GSIGNOND_LIBS)

libgsignond_plugins_la_SOURCES = \
   gsignond-plugin-proxy.c \
   gsignond-plugin-proxy-factory.c

CLEANFILES = 
