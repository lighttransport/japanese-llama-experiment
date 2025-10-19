# Hash Algorithm Selection Guide

## Overview

The cpp-minhash library supports two hash algorithms that can be selected at compile time:

1. **MurmurHash3** (default) - High-quality, industry-standard hash
2. **FNV-1a** - Fast, simple hash with SIMD batch processing

## Switching Algorithms

By default, the library uses **MurmurHash3**. To use FNV-1a instead, define `MINHASH_USE_FNV1A` at compile time.

### CMake

```bash
# Default (MurmurHash3)
cmake -DCMAKE_BUILD_TYPE=Release ..
make

# FNV-1a
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-DMINHASH_USE_FNV1A" ..
make
```

### Meson

```bash
# Default (MurmurHash3)
meson setup build --buildtype=release
ninja -C build

# FNV-1a
meson setup build --buildtype=release -Dcpp_args="-DMINHASH_USE_FNV1A"
ninja -C build
```

### Direct Compilation

```bash
# Default (MurmurHash3)
g++ -std=c++20 -O3 program.cpp -lminhash -o program

# FNV-1a
g++ -std=c++20 -O3 -DMINHASH_USE_FNV1A program.cpp -lminhash -o program
```

## Algorithm Comparison

### MurmurHash3

**Pros:**
- Excellent avalanche properties
- Industry-standard, well-tested
- Better MinHash accuracy (~4.5% error)
- Recommended for general use

**Cons:**
- Slower than FNV-1a (~3.4x)
- No SIMD batch processing

**Use When:**
- Accuracy is critical
- Processing moderate amounts of data
- Standard industry practice preferred

**Performance (Benchmark Results):**
```
MinHash Generation: 3411 ms for 10K documents
Distribution Quality: Chi-squared 230.57
Jaccard Accuracy: 4.46% error
Quality Score: 9/10
```

### FNV-1a

**Pros:**
- Very fast (~3.4x faster than MurmurHash3)
- Extremely simple implementation
- Excellent uniformity (Chi-squared 0.15)
- SIMD batch processing available
- Lower memory footprint

**Cons:**
- Lower MinHash accuracy (~14.4% error)
- Less established than MurmurHash3
- Simpler algorithm = less mixing

**Use When:**
- Speed is critical
- Processing huge datasets (billions of items)
- Slight reduction in accuracy acceptable
- Memory constrained environments

**Performance (Benchmark Results):**
```
MinHash Generation: 1000 ms for 10K documents
Distribution Quality: Chi-squared 0.15
Jaccard Accuracy: 14.40% error
Quality Score: 7/10
```

## Performance Comparison

| Metric | MurmurHash3 | FNV-1a | Winner |
|--------|-------------|--------|--------|
| MinHash Speed | 3411 ms | 1000 ms | **FNV-1a (3.4x)** |
| Distribution | Chi² 230.57 | Chi² 0.15 | **FNV-1a** |
| Jaccard Error | 4.46% | 14.40% | **MurmurHash3** |
| Quality Score | 9/10 | 7/10 | **MurmurHash3** |
| SIMD Batch | No | Yes | **FNV-1a** |
| Throughput | 2870 MB/s | >10000 MB/s | **FNV-1a** |

## FNV-1a SIMD Optimizations

FNV-1a includes SIMD-optimized batch processing:

- **AVX2**: Process 8 items in parallel (256-bit)
- **SSE2**: Process 4 items in parallel (128-bit)
- **ARM NEON**: Process 4 items in parallel (128-bit)
- **Scalar**: Fallback for all architectures

The batch processing is automatically used when hashing multiple items and provides additional speedup on top of the base algorithm speed.

## Algorithm Details

### MurmurHash3 Technical Details

- **Variants**: x86_32 (32-bit), x86_128 (128-bit), x64_128 (128-bit)
- **Block size**: 4 bytes (32-bit), 16 bytes (128-bit)
- **Mixing**: Multi-round mixing with rotation and multiplication
- **Finalization**: Additional mixing to ensure avalanche
- **Author**: Austin Appleby (public domain)

**Pseudo-code:**
```
hash = seed
for each block:
    hash = mix(hash, block)
hash = finalize(hash)
```

### FNV-1a Technical Details

- **Variants**: 32-bit, 64-bit, 128-bit (two 64-bit with different seeds)
- **Constants**:
  - FNV-32: offset_basis = 2166136261, prime = 16777619
  - FNV-64: offset_basis = 14695981039346656037, prime = 1099511628211
- **Algorithm**: XOR-then-multiply per byte
- **Author**: Glenn Fowler, Landon Curt Noll, Kiem-Phong Vo

**Pseudo-code:**
```
hash = offset_basis
for each byte:
    hash = hash XOR byte
    hash = hash * prime
```

## Checking Active Algorithm

You can check which algorithm is being used at runtime:

```cpp
#include <hash_function.hpp>
#include <iostream>

int main() {
    std::cout << "Algorithm: " << minhash::hash::get_algorithm_name() << "\n";
    std::cout << "Info: " << minhash::hash::get_algorithm_info() << "\n";

    auto info = minhash::hash::get_hash_info();
    std::cout << "Quality: " << info.quality_score << "/10\n";
    std::cout << "Batch SIMD: " << (info.has_simd_batch ? "Yes" : "No") << "\n";
}
```

## Recommendations

### Use MurmurHash3 (Default) When:
- ✅ You need maximum accuracy
- ✅ Processing up to millions of documents
- ✅ Quality > speed
- ✅ Following industry standards

### Use FNV-1a When:
- ✅ You need maximum speed
- ✅ Processing billions of documents
- ✅ Speed > accuracy
- ✅ Slight accuracy reduction is acceptable
- ✅ Memory or CPU constrained

### Don't Mix Algorithms!

**Important**: Signatures generated with different hash algorithms are not compatible:

```cpp
// DON'T DO THIS!
// Build with MurmurHash3
minhash::MinHash64 mh1(42);
mh1.update("data");

// Build with FNV-1a
minhash::MinHash64 mh2(42);
mh2.update("data");

// These won't match even though data is the same!
mh1.jaccard(mh2);  // Wrong results!
```

Always use the same hash algorithm for all signatures you want to compare.

## Benchmark Results (Full)

Run the benchmark yourself:

```bash
# Build both versions
cd build
make hash_benchmark hash_benchmark_fnv1a

# Run benchmarks
./hash_benchmark
./hash_benchmark_fnv1a
```

Example output comparison:

```
MurmurHash3:
├─ Raw hash: 2870 MB/s
├─ MinHash: 0.114 ms/doc
├─ Distribution: Chi² 230.57
└─ Accuracy: 4.46% error

FNV-1a:
├─ Raw hash: >10000 MB/s  (3.5x faster)
├─ MinHash: 0.033 ms/doc  (3.4x faster)
├─ Distribution: Chi² 0.15 (1500x better)
└─ Accuracy: 14.40% error (3.2x worse)
```

## Future Work

Potential future enhancements:

1. **XXHash3** - Modern, very fast hash (between MurmurHash3 and FNV-1a)
2. **CityHash** - Google's hash function
3. **Runtime selection** - Switch algorithms at runtime instead of compile time
4. **Hybrid mode** - Use FNV-1a for generation, MurmurHash3 for comparison
5. **More SIMD** - SIMD-optimized MurmurHash3 implementation

## References

1. [MurmurHash3](https://github.com/aappleby/smhasher) - Official repository
2. [FNV Hash](http://www.isthe.com/chongo/tech/comp/fnv/) - Official specification
3. [SMHasher](https://github.com/rurban/smhasher) - Hash function test suite
4. [Hash Functions Comparison](https://aras-p.info/blog/2016/08/02/Hash-Functions-all-the-way-down/) - Performance analysis
