#!/bin/sh
#
git archive --format=tar --prefix=gsignond-0.0.2/ -o ../gsignond-0.0.2.tar daemon
bzip2 ../gsignond-0.0.2.tar
mv ../gsignond-0.0.2.tar.bz2 ~/rpmbuild/SOURCES/

