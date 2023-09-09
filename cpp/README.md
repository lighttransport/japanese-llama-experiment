## Build

```
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
$ make
```

### With KenLM

```
$ git submodule update --init --recursive

$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCPP_PROC_WITH_KENLM=1 ..
$ make
```

### single-file zstd

zstd.c was created using https://github.com/facebook/zstd/tree/dev/build/single_file_libs

### Third party libraries

* fastbase64 : https://github.com/lemire/fastbase64
* murmurhash3. public domain: https://github.com/aappleby/smhasher/issues/86
* glob: MIT license: https://github.com/p-ranav/glob
* libsais: Apache 2.0
* nlohmann/json: MIT license https://github.com/nlohmann/json
* stack_container.h: BSD license
