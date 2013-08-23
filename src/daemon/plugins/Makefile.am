SUBDIRS = plugind

noinst_LTLIBRARIES = libgsignond-plugins.la

libgsignond_plugins_la_CPPFLAGS = \
    -I$(top_builddir) \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/include \
    -I$(top_builddir)/src \
    -DGSIGNOND_BIN_DIR='"$(bindir)"' \
    -DGSIGNOND_PLUGINS_DIR='"$(pluginsdir)"' \
    $(GSIGNOND_CFLAGS)

libgsignond_plugins_la_LIBADD =    \
        $(top_builddir)/src/common/libgsignond-common.la \
        $(top_builddir)/src/daemon/dbus/libgsignond-dbus-glue.la \
        $(GSIGNOND_LIBS)

libgsignond_plugins_la_SOURCES = \
   gsignond-plugin-proxy.h \
   gsignond-plugin-proxy.c \
   gsignond-plugin-proxy-factory.h \
   gsignond-plugin-proxy-factory.c \
   gsignond-plugin-remote-private.h \
   gsignond-plugin-remote.h \
   gsignond-plugin-remote.c

CLEANFILES = 
