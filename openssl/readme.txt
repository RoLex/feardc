This directory contains a subset of OpenSSL <www.openssl.org>, namely header
files and static libraries compiled with MinGW and MSVC.

Info on the MinGW and MSVC compilers is in the "Compile.txt" file at the
root of this repository.

The openssl/build/build_mingw.bat script has been used to compile MinGW
static libraries. Alternatively openssl/build/build_cygwin.sh for Cygwin
and Msys users.

Patches in the openssl/build/patches directory have been applied.

The exact version of OpenSSL can be found in include/openssl/opensslv.h
(OPENSSL_VERSION_NUMBER macro).

See "ThirdPartyLicenses.txt" at the root of this repository for license
details. That file is also included into binary distributions of FearDC.
