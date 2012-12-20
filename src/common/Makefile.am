lib_LTLIBRARIES = libgsignond-common.la
NULL=

libgsignond_common_la_CPPFLAGS = \
    -I$(top_srcdir) \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/include \
    $(GSIGNOND_CFLAGS) \
    $(NULL)

libgsignond_common_la_LIBS = \
    $(GSIGNOND_LIBS)
    $(NULL)

libgsignond_common_la_SOURCES = \
    gsignond-security-context.c \
    gsignond-credentials.c \
    gsignond-secret-storage.c \
    gsignond-config.c \
    gsignond-error.c \
    gsignond-error.h \
    gsignond-log.h \
    $(NULL)

CLEANFILES = 
