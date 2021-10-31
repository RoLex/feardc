@echo off
choice /M "%0 - Make sure you have Cygwin - Make sure you have an 'openssl-*.tar.gz' file in this directory - Continue?"
echo on
if errorlevel 2 goto end

set VCDIR=C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC

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

REM ------------------
REM TODO Fix VC builds
rm -rf openssl*/
goto end
REM ------------------

rm -rf openssl*/
tar -xzf openssl-*.tar.gz
cd openssl*
bash --login -i -c "cd '%CD%' && find ../patches -name '*.patch' -exec patch -p1 -i '{}' \;"
call "%VCDIR%\vcvarsall.bat" x86
echo on
perl Configure VC-WIN32 no-shared --prefix=%CD%
call ms\do_nasm
nmake -f ms\nt.mak clean install
if errorlevel 1 goto end
mv inc32/openssl/opensslconf.h ../../include/openssl/opensslconf-msvc-x86.h
mv lib/libeay32.lib lib/ssleay32.lib tmp32/lib.pdb ../../lib
nmake -f ms\ntdebug.mak clean install
if errorlevel 1 goto end
mv lib/libeay32.lib ../../lib/libeay32d.lib
mv lib/ssleay32.lib ../../lib/ssleay32d.lib
mv tmp32.dbg/libd.pdb ../../lib
cd ..

rm -rf openssl*/
tar -xzf openssl-*.tar.gz
cd openssl*
bash --login -i -c "cd '%CD%' && find ../patches -name '*.patch' -exec patch -p1 -i '{}' \;"
call "%VCDIR%\vcvarsall.bat" amd64
echo on
perl Configure VC-WIN64A no-shared --prefix=%CD%
call ms\do_win64a
nmake -f ms\nt.mak clean install
if errorlevel 1 goto end
mv inc32/openssl/opensslconf.h ../../include/openssl/opensslconf-msvc-x64.h
mv lib/libeay32.lib lib/ssleay32.lib tmp32/lib.pdb ../../lib/x64
nmake -f ms\ntdebug.mak clean install
if errorlevel 1 goto end
mv lib/libeay32.lib ../../lib/x64/libeay32d.lib
mv lib/ssleay32.lib ../../lib/x64/ssleay32d.lib
mv tmp32.dbg/libd.pdb ../../lib/x64
cd ..

rm -rf openssl*/
:end
pause
