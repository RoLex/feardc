# run from cygwin console: chmod +x build.sh && ./build.sh

mv ../include/openssl/opensslconf.h .
rm -rf ../include ../lib
mkdir -p ../include/openssl ../lib/x64
mv opensslconf.h ../include/openssl

# 32-bit
#rm -rf openssl*/
#tar -xzf openssl-*.tar.gz
#mv openssl-*.tar.gz ..
#cd openssl*
#patch -p1 -i ../patches/mingw-flags.patch
#./Configure mingw no-shared --cross-compile-prefix=i686-w64-mingw32-
#make
#mv include/openssl/opensslconf.h ../../include/openssl/opensslconf-mingw-x86.h
#mv include/openssl/*.h ../../include/openssl
#mv libcrypto.a libssl.a ../../lib
#cd ..

# 64-bit
rm -rf openssl*/
#mv ../openssl-*.tar.gz .
tar -xzf openssl-*.tar.gz
mv openssl-*.tar.gz ..
cd openssl*
patch -p1 -i ../patches/mingw-flags.patch
./Configure mingw64 no-shared --cross-compile-prefix=x86_64-w64-mingw32-
make
mv include/openssl/opensslconf.h ../../include/openssl/opensslconf-mingw-x64.h
mv libcrypto.a libssl.a ../../lib/x64
cd ..
rm -rf openssl*/
rm ../openssl-*.tar.gz
