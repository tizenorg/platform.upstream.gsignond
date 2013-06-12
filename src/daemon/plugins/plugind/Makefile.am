SUBDIRS=
NULL=

lib_LTLIBRARIES = libgsignond-plugind-daemon.la

libgsignond_plugind_daemon_la_CPPFLAGS = \
    -I$(top_builddir) \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/include \
    -I$(top_builddir)/src \
    $(GSIGNOND_CFLAGS)

libgsignond_plugind_daemon_la_LIBADD =    \
        $(top_builddir)/src/common/libgsignond-common.la \
        $(top_builddir)/src/daemon/dbus/libgsignond-dbus-glue.la \
        $(GSIGNOND_LIBS)

libgsignond_plugind_daemon_la_SOURCES = \
   gsignond-plugin-daemon.h \
   gsignond-plugin-daemon.c

bin_PROGRAMS = gsignond-plugind

gsignond_plugind_SOURCES = \
    main.c \
    $(NULL)

gsignond_plugind_CFLAGS = \
    -I$(top_builddir) \
    -I$(top_srcdir)/include/ \
    -I$(top_srcdir)/src/ \
    $(GSIGNOND_CFLAGS) \
    $(NULL)

gsignond_plugind_LDADD = \
    libgsignond-plugind-daemon.la \
    $(GSIGNOND_LIBS) \
    $(NULL)

all-local: slink

slink:
	$(MKDIR_P) $(abs_top_builddir)/src/daemon/.libs
	ln -sf $(abs_top_builddir)/src/daemon/plugins/plugind/.libs/gsignond-plugind $(abs_top_builddir)/src/daemon/.libs/gsignond-plugind
