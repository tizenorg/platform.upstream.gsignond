ACLOCAL_AMFLAGS = -I m4

# --enable-distcheck is here to make it possible to use
# also --enable-dbus-type=session here
AM_DISTCHECK_CONFIGURE_FLAGS = \
    --enable-debug \
    --enable-distcheck

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gsignond.pc

SUBDIRS = src test

EXTRA_DIST = dist tools

valgrind:
	cd test; make valgrind
