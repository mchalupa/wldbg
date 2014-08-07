#! /bin/sh

# create missing directories
test -d "config" || mkdir "config"
test -d "m4" || mkdir "m4"

test -n "$srcdir" || srcdir=`dirname "$0"`
test -n "$srcdir" || srcdir=.
(
  cd "$srcdir" &&
  autoreconf --force -v --install
) || exit
test -n "$NOCONFIGURE" || "$srcdir/configure" "$@"
