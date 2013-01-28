include $(top_srcdir)/common.mk
plugins_LTLIBRARIES = libpassword.la
NULL=

libpassword_la_CPPFLAGS = \
    -I$(top_srcdir) \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/include \
    $(GSIGNOND_CFLAGS) \
    $(NULL)

libpassword_la_LIBADD = \
    $(top_builddir)/src/common/libgsignond-common.la \
    $(GSIGNOND_LIBS) \
    $(NULL)

libpassword_la_SOURCES = \
    gsignond-password-plugin.c \
    $(NULL)

CLEANFILES = 
