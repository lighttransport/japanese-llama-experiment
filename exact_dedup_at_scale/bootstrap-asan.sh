rm -rf build_asan
mkdir build_asan

CXX=clang++ CC=clang cmake -DSANITIZE_ADDRESS=1 -DCMAKE_BUILD_TYPE=Debug -Bbuild_asan -S.
