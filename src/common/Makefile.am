SUBDIRS = db

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
    gsignond-access-control-manager.c \
    gsignond-extension-interface.c \
    gsignond-storage-manager.c \
    gsignond-credentials.c \
    gsignond-config.c \
    gsignond-error.c \
    db/gsignond-db-error.c \
    db/gsignond-db-sql-database.c \
    db/gsignond-db-secret-database.c \
    db/gsignond-db-secret-storage.c \
    $(NULL)

CLEANFILES = 
