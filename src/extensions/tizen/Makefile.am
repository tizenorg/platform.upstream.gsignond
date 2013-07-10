include $(top_srcdir)/common.mk
extensions_LTLIBRARIES = libextension-tizen.la

libextension_tizen_la_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir)/include \
	$(GSIGNOND_CFLAGS) \
	$(LIBSMACK_CFLAGS) \
	$(LIBECRYPTFS_CFLAGS)

libextension_tizen_la_LIBADD = \
	$(top_builddir)/src/common/libgsignond-common.la \
	$(GSIGNOND_LIBS) \
	$(LIBSMACK_LIBS) \
	$(LIBECRYPTFS_LIBS)

libextension_tizen_la_SOURCES = \
    tizen-access-control-manager.h \
    tizen-access-control-manager.c \
    tizen-extension.h \
    tizen-extension.c \
    tizen-secret-storage.h \
    tizen-secret-storage.c \
    tizen-storage-manager.h \
    tizen-storage-manager.c

