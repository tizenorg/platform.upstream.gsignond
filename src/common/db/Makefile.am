noinst_LTLIBRARIES = libgsignond-common-db.la
NULL=

libgsignond_common_db_la_CPPFLAGS = \
    -I$(top_srcdir) \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/include \
    $(GSIGNOND_CFLAGS)

libgsignond_common_db_la_LIBADD =    \
	$(GSIGNOND_LIBS)

libgsignond_common_db_la_SOURCES = \
    gsignond-db-sql-database.c \
    gsignond-secret-storage.c \
    $(NULL)


