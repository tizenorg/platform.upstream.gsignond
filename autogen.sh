#!/bin/sh -e

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

gtkdocize || exit 1
aclocal #-I m4 
autoheader 
libtoolize --copy --force 
autoconf 
automake --add-missing --copy
autoreconf --install --force
. $srcdir/configure "$@"

