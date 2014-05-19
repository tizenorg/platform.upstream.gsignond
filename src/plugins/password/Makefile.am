include $(top_srcdir)/common.mk
gplugins_LTLIBRARIES = libpassword.la
NULL=

libpassword_la_CPPFLAGS = \
    -I$(top_builddir) \
    -I$(top_srcdir)/src \
    -I$(top_srcdir)/include \
    $(GSIGNOND_CFLAGS) \
    $(NULL)

libpassword_la_LIBADD = \
    $(top_builddir)/src/common/libgsignond-common.la \
    $(GSIGNOND_LIBS) \
    $(NULL)

libpassword_la_SOURCES = \
    gsignond-password-plugin.h \
    gsignond-password-plugin.c \
    $(NULL)

libpassword_la_LDFLAGS = -avoid-version

all-local: slink

slink:
	ln -sf $(abs_top_builddir)/src/plugins/password/.libs/libpassword.so ../.libs/libpassword.so
