noinst_LTLIBRARIES = libgsignond-db.la

libgsignond_db_la_CPPFLAGS = \
    -I$(top_builddir) \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/include \
    $(GSIGNOND_CFLAGS)

libgsignond_db_la_LIBADD =    \
        $(top_builddir)/src/common/libgsignond-common.la \
        $(GSIGNOND_LIBS)

libgsignond_db_la_SOURCES = \
   gsignond-db-credentials-database.h \
   gsignond-db-credentials-database.c \
   gsignond-db-metadata-database.h \
   gsignond-db-metadata-database.c

CLEANFILES = 
