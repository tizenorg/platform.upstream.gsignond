#!/bin/sh
#
git archive --format=tar --prefix=gsignond-0.0.0/ -o ../gsignond-0.0.0.tar daemon
bzip2 ../gsignond-0.0.0.tar
mv ../gsignond-0.0.0.tar.bz2 ~/rpmbuild/SOURCES/

