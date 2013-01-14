NULL=

bin_PROGRAMS = testdb

testdb_SOURCES = \
    test.c \
    $(NULL)

testdb_CFLAGS = \
    -I$(top_srcdir) \
    -I$(top_builddir)/include/ \
    -I$(top_builddir)/src/ \
    $(GSIGNOND_CFLAGS) \
    $(NULL)

testdb_LDADD = \
    $(top_builddir)/src/common/libgsignond-common.la \
    $(top_builddir)/src/daemon/db/libgsignond-db.la \
    $(GSIGNOND_LIBS) \
    $(NULL)

