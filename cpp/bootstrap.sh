rm -rf build
mkdir build

CXX=clang++ CC=clang cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -Bbuild -S.
