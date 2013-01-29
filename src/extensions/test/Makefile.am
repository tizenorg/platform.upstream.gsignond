include $(top_srcdir)/common.mk
extensions_LTLIBRARIES = libextension-test.la

libextension_test_la_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/include \
	$(GSIGNOND_CFLAGS)

libextension_test_la_LIBS = \
	$(GSIGNOND_LIBS)

libextension_test_la_SOURCES = \
	test-access-control-manager.c \
	test-extension.c \
	test-secret-storage.c \
	test-storage-manager.c

