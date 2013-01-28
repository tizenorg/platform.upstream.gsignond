include $(top_srcdir)/common.mk

SUBDIRS = db 
NULL=

lib_LTLIBRARIES = libgsignond-common.la

libgsignond_common_la_CPPFLAGS = \
    -I$(top_srcdir) \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/include \
    $(GSIGNOND_CFLAGS) \
    -DGSIGNOND_PLUGINS_DIR='"$(pluginsdir)"' \
    -DGSIGNOND_EXTENSIONS_DIR='"$(extensionsdir)"' \
    $(NULL)

libgsignond_common_la_DEPENDENCIES = \
    $(top_builddir)/src/common/db/libgsignond-common-db.la \
    $(NULL)

libgsignond_common_la_LIBADD = \
    $(GSIGNOND_LIBS) \
    $(top_builddir)/src/common/db/libgsignond-common-db.la \
    $(NULL)

libgsignond_common_la_SOURCES = \
    gsignond-security-context.c \
    gsignond-access-control-manager.c \
    gsignond-extension-interface.c \
    gsignond-storage-manager.c \
    gsignond-identity-info.c \
    gsignond-credentials.c \
    gsignond-config.c \
    gsignond-error.c \
    gsignond-plugin-interface.c \
    gsignond-dictionary.c \
    gsignond-session-data.c \
    $(NULL)

CLEANFILES = 

