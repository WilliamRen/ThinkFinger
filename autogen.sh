#/bin/sh

rm -f README INSTALL
touch README INSTALL
libtoolize --force
autoheader
aclocal
automake --add-missing
autoconf
