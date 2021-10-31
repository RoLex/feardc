This directory contains a subset of OpenSSL <www.openssl.org>, namely header
files and static libraries compiled with MinGW.

Info on the MinGW compiler is in the "Compile.txt" file at the root of this
repository.
The openssl/build/build.bat script has been used to compile static libraries.

Patches in the openssl/build/patches directory have been applied.

The exact version of OpenSSL can be found in include/openssl/opensslv.h
(OPENSSL_VERSION_NUMBER macro).

See "ThirdPartyLicenses.txt" at the root of this repository for license
details. That file is also included into binary distributions of DC++.
