noinst_LTLIBRARIES = libgsignond-common-db.la
NULL=

libgsignond_common_db_la_CPPFLAGS = \
    -I$(top_builddir) \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/include \
    $(GSIGNOND_CFLAGS)

libgsignond_common_db_la_LIBS =    \
	$(GSIGNOND_LIBS)

libgsignond_common_db_la_SOURCES = \
    gsignond-db-defines.h \
    gsignond-db-error.h \
    gsignond-db-error.c \
    gsignond-db-sql-database.h \
    gsignond-db-sql-database.c \
    gsignond-db-secret-database.h \
    gsignond-db-secret-database.c \
    gsignond-secret-storage.c \
    gsignond-db-sql-database-private.h \
	$(NULL)


