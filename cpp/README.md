## Requirement

C++17 compiler.

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

### Build configuration for J.DepP

* classifier: Linear(fastest but low accuracy)

### single-file zstd

zstd.c was created using https://github.com/facebook/zstd/tree/dev/build/single_file_libs

### Third party libraries

* fastbase64 : https://github.com/lemire/fastbase64
* murmurhash3. public domain: https://github.com/aappleby/smhasher/issues/86
* glob: MIT license: https://github.com/p-ranav/glob
* libsais: Apache 2.0
* nlohmann/json: MIT license https://github.com/nlohmann/json
* stack_container.h: BSD license
* sqlite3: public domain.
* pbar: Apache 2.0 https://github.com/estshorter/pbar
* lz4: 2-clause BSD.
* fpng: unlicense(public domain).
* wuffs: Apache 2.0
* safetensors-cpp: MIT license. https://github.com/syoyo/safetensors-cpp
* ccedar, jagger: We choose BSD license(GPLv2, LGPLv2.1 and BSD triple license) https://www.tkl.iis.u-tokyo.ac.jp/~ynaga/jagger/
* J.DepP: We choose BSD license(GPLv2, LGPLv2.1 and BSD triple license) https://www.tkl.iis.u-tokyo.ac.jp/~ynaga/jdepp/
* hat-trie: MIT license. https://github.com/Tessil/hat-trie
* llama.cpp : MIT license. https://github.com/ggerganov/llama.cpp
* enkiTS: ZLIB license. https://github.com/dougbinks/enkiTS
* optparse: Unlicense. https://github.com/skeeto/optparse
