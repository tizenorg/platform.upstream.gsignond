include $(top_srcdir)/common.mk
extensions_LTLIBRARIES = libextension-test.la

libextension_test_la_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir)/include \
	$(GSIGNOND_CFLAGS)

libextension_test_la_LIBS = \
	$(GSIGNOND_LIBS)

libextension_test_la_SOURCES = \
    test-access-control-manager.h \
	test-access-control-manager.c \
    test-extension.h \
    test-extension.c \
    test-secret-storage.h \
    test-secret-storage.c \
    test-storage-manager.h \
    test-storage-manager.c

