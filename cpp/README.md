## Build

```
# checkout submodules(e.g. simdjson)
$ git submodule update --init --recursive
```

```
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
$ make
```

### single-file zstd

zstd.c was created using https://github.com/facebook/zstd/tree/dev/build/single_file_libs

