include $(top_srcdir)/common.mk
extensions_LTLIBRARIES = libextension-tizen.la

libextension_tizen_la_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/include \
	$(GSIGNOND_CFLAGS) \
	$(LIBSMACK_CFLAGS)

libextension_tizen_la_LIBADD = \
	$(GSIGNOND_LIBS) \
	$(LIBSMACK_LIBS)

libextension_tizen_la_SOURCES = \
	tizen-access-control-manager.c \
	tizen-extension.c \
	tizen-secret-storage.c \
	tizen-storage-manager.c

