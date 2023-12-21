rm -rf build
mkdir build

CXX=clang++ CC=clang cmake -DSANITIZE_ADDRESS=0 -DCMAKE_BUILD_TYPE=RelWithDebInfo -Bbuild -S.
