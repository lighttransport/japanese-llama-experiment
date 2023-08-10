## Build

```
$ sudo apt install libzstd-dev
```


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
