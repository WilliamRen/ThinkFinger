#/bin/sh

libtoolize --force
aclocal
automake --add-missing
autoheader
autoconf
