@echo off
choice /M "%0 - Make sure you have Cygwin - Make sure you have an 'openssl-*.tar.gz' file in this directory - Continue?"
echo on
if errorlevel 2 goto end

mv ../include/openssl/opensslconf.h .
rm -rf ../include ../lib
mkdir ..\include\openssl ..\lib\x64 ..\lib\ia64
mv opensslconf.h ../include/openssl

rm -rf openssl*/
tar -xzf openssl-*.tar.gz
cd openssl*
bash --login -i -c "cd '%CD%' && find ../patches -name '*.patch' -exec patch -p1 -i '{}' \;"
bash --login -i -c "cd '%CD%' && ./Configure mingw no-shared --cross-compile-prefix=i686-w64-mingw32- && make"
if errorlevel 1 goto end
mv include/openssl/opensslconf.h ../../include/openssl/opensslconf-mingw-x86.h
mv include/openssl/*.h ../../include/openssl
mv libcrypto.a libssl.a ../../lib
cd ..

rm -rf openssl*/
tar -xzf openssl-*.tar.gz
cd openssl*
bash --login -i -c "cd '%CD%' && find ../patches -name '*.patch' -exec patch -p1 -i '{}' \;"
bash --login -i -c "cd '%CD%' && ./Configure mingw64 no-shared --cross-compile-prefix=x86_64-w64-mingw32- && make"
if errorlevel 1 goto end
mv include/openssl/opensslconf.h ../../include/openssl/opensslconf-mingw-x64.h
mv libcrypto.a libssl.a ../../lib/x64
cd ..

rm -rf openssl*/

:end
pause
