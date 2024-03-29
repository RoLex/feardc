#!/bin/sh
set -e

# run from cygwin or msys console
# ./build_cygwin.sh {x86|x64}

uname_id="$(uname -s)"
echo "[build_cygwin.sh] Proceeding using $uname_id"

if [ "$1" = "x86" ] || [ "$1" = "x64" ]
then
	echo "[build_cygwin.sh] Proceeding with $1 build"

else # unknown
	echo "[build_cygwin.sh] Usage: ./build_cygwin.sh {x86|x64}"
	exit 1
fi

if [ -x "$(command -v perl)" ]
then
	echo "[build_cygwin.sh] Found Perl, proceeding"

else
	echo "[build_cygwin.sh] Missing Perl, exiting"
	exit 1
fi

if [ -x "$(command -v patch)" ]
then
	echo "[build_cygwin.sh] Found Patch, proceeding"

else
	echo "[build_cygwin.sh] Missing Patch, exiting"
	exit 1
fi

if [ -x "$(command -v tar)" ]
then
	echo "[build_cygwin.sh] Found Tar, proceeding"

else
	echo "[build_cygwin.sh] Missing Tar, exiting"
	exit 1
fi

if [ -x "$(command -v make)" ]
then
	echo "[build_cygwin.sh] Found Make, proceeding"

else
	echo "[build_cygwin.sh] Missing Make, exiting"
	exit 1
fi

if [ -e ../include/openssl/opensslconf.h ]
then
	echo "[build_cygwin.sh] Found opensslconf.h, proceeding"

else
	echo "[build_cygwin.sh] Missing opensslconf.h, exiting"
	exit 1
fi

if [ -e ../openssl-*.tar.gz ]
then
	mv ../openssl-*.tar.gz ./
fi

if [ -e ./openssl-*.tar.gz ]
then
	echo "[build_cygwin.sh] Found $(ls openssl-*.tar.gz), proceeding"

else
	echo "[build_cygwin.sh] Missing OpenSSL archive, exiting"
	exit 1
fi

if [ -e ../include/openssl/opensslconf-mingw-x86.h ]
then
	mv ../include/openssl/opensslconf-mingw-x86.h ./
fi

if [ -e ../include/openssl/opensslconf-mingw-x64.h ]
then
	mv ../include/openssl/opensslconf-mingw-x64.h ./
fi

mv ../include/openssl/opensslconf.h ./
rm -f ../include/openssl/*.h
mv ./opensslconf.h ../include/openssl/

if [ -e ./opensslconf-mingw-x86.h ]
then
	mv ./opensslconf-mingw-x86.h ../include/openssl/
fi

if [ -e ./opensslconf-mingw-x64.h ]
then
	mv ./opensslconf-mingw-x64.h ../include/openssl/
fi

rm -rf ./openssl-*/
tar -xzf ./openssl-*.tar.gz
mv ./openssl-*.tar.gz ../
cd ./openssl-*
patch -p1 -i ../patches/mingw-flags.patch
cw_pref_32=""
cw_pref_64=""

if [ "$(expr substr $uname_id 1 6)" = "CYGWIN" ]
then
	cw_pref_32="--cross-compile-prefix=i686-w64-mingw32-"
	cw_pref_64="--cross-compile-prefix=x86_64-w64-mingw32-"
	echo "[build_cygwin.sh] Proceeding with Cygwin prefix"
fi

if [ "$1" = "x86" ]
then
	./Configure mingw no-shared $cw_pref_32
	make
	mv ./include/openssl/opensslconf.h ../../include/openssl/opensslconf-mingw-x86.h
	rm -f ../../lib/*.*
	mv ./libcrypto.a ../../lib/
	mv ./libssl.a ../../lib/

elif [ "$1" = "x64" ]
then
	./Configure mingw64 no-shared $cw_pref_64
	make
	mv ./include/openssl/opensslconf.h ../../include/openssl/opensslconf-mingw-x64.h
	rm -f ../../lib/x64/*.*
	mv ./libcrypto.a ../../lib/x64/
	mv ./libssl.a ../../lib/x64/
fi

mv ./include/openssl/*.h ../../include/openssl/
cd ../
rm -rf ./openssl-*/
mv ../openssl-*.tar.gz ./
echo "[build_cygwin.sh] Done building OpenSSL for $1"
exit 0

# end of file