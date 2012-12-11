lib_LTLIBRARIES = libgsignond-db.la

libgsignond_db_la_CPPFLAGS = \
    -I$(top_srcdir) \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/include \
    $(GSIGNOND_CFLAGS)

libgsignond_db_la_LIBS = \
    $(top_srcdir)/src/common/libgsignond-common.la \
    $(GSIGNOND_LIBS)

libgsignond_db_la_SOURCES = \
   gsignond-db-error.c \
   gsignond-db-error.h 

CLEANFILES = 
