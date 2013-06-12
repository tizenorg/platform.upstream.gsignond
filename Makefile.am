ACLOCAL_AMFLAGS = -I m4

AM_DISTCHECK_CONFIGURE_FLAGS = "--enable-debug"

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gsignond.pc

SUBDIRS = src test

valgrind:
	cd test; make valgrind
