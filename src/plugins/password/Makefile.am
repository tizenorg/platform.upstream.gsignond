lib_LTLIBRARIES = libpasswordplugin.la
NULL=

libpasswordplugin_la_CPPFLAGS = \
    -I$(top_srcdir) \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/include \
    $(GSIGNOND_CFLAGS) \
    $(NULL)

libpasswordplugin_la_LIBS = \
    $(GSIGNOND_LIBS)
    $(NULL)

libpasswordplugin_la_SOURCES = \
    gsignond-password-plugin.c \
    $(NULL)

CLEANFILES = 
