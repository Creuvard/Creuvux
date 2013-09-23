#!/bin/sh

echo "-> aclocal :"
aclocal
echo "-> autoconf :"
autoconf
echo "-> autoheader :"
autoheader
echo "-> automake -a -c :"
automake -a -c
