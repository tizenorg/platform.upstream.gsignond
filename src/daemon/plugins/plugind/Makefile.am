SUBDIRS=
NULL=

bin_PROGRAMS = gsignond-plugind

gsignond_plugind_SOURCES = \
    main.c \
    gsignond-plugin-daemon.c \
    $(NULL)

gsignond_plugind_CFLAGS = \
    -I$(top_srcdir) \
    -I$(top_srcdir)/include/ \
    -I$(top_srcdir)/src/ \
    $(GSIGNOND_CFLAGS) \
    $(NULL)

gsignond_plugind_LDADD = \
    $(top_srcdir)/src/common/libgsignond-common.la \
    $(top_srcdir)/src/daemon/dbus/libgsignond-dbus-glue.la \
    $(GSIGNOND_LIBS) \
    $(NULL)

