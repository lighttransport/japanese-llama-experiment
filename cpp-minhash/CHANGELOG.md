# Changelog

## Version 1.1.0 (2024-10-19)

### New Features

**Switchable Hash Algorithms**
- Added FNV-1a hash algorithm as alternative to MurmurHash3
- **3.4x faster** MinHash generation with FNV-1a
- Compile-time selection via `-DMINHASH_USE_FNV1A`
- Hash abstraction layer for clean switching (`hash_function.hpp`)
- Both algorithms fully compatible with all features

**FNV-1a SIMD Optimizations**
- AVX2 batch processing: 8 items in parallel
- SSE2 batch processing: 4 items in parallel
- ARM NEON batch processing: 4 items in parallel
- Automatic selection of best available implementation

**Benchmark Tool**
- New `hash_benchmark` example comparing algorithms
- Automatically builds both MurmurHash3 and FNV-1a versions
- Tests: raw hash speed, MinHash generation, distribution quality, accuracy
- Comprehensive performance and quality comparison

**New Headers**
- `fnv1a.hpp` - FNV-1a implementation with SIMD
- `hash_function.hpp` - Hash algorithm abstraction layer

### Performance Improvements

FNV-1a vs MurmurHash3 (10K documents benchmark):
- **MinHash Generation**: 1000ms vs 3411ms (3.4x faster)
- **Distribution Quality**: Chi² 0.15 vs 230.57 (better uniformity)
- **Jaccard Accuracy**: 14.4% error vs 4.5% error (MurmurHash3 more accurate)
- **Throughput**: >10000 MB/s vs 2870 MB/s (3.5x faster)

### Documentation

- Added `HASH_ALGORITHMS.md` - Comprehensive hash algorithm guide
- Updated `README.md` with hash selection information
- Added performance comparison tables
- Usage recommendations for each algorithm

### API Changes

No breaking changes. MurmurHash3 remains the default.

New functions in `minhash::hash` namespace:
- `get_algorithm_name()` - Returns "MurmurHash3" or "FNV-1a"
- `get_algorithm_info()` - Returns detailed algorithm information
- `get_hash_info()` - Returns `HashInfo` struct with quality metrics
- `is_fnv1a()` - Compile-time check for FNV-1a
- `is_murmurhash3()` - Compile-time check for MurmurHash3

## Version 1.0.0 (2024-10-19)

### Initial Release

#### Core Features
- Modern C++20 MinHash implementation
- Support for 32-bit, 64-bit, and 128-bit hash values
- LSH (Locality-Sensitive Hashing) banding for duplicate detection
- Header-mostly library design
- Dual build system support (CMake and Meson)

#### New in This Release

**Exact Jaccard Computation** (`include/jaccard.hpp`)
- `exact_sorted()` - For sorted ranges using std::set_intersection/union
- `exact_unsorted()` - For unsorted ranges using std::set
- `exact_hash()` - Fast hash-based using std::unordered_set
- `containment()` - Asymmetric containment measure
- `dice()` - Dice coefficient computation
- All functions use C++20 concepts and ranges

**SIMD Optimizations** (`include/simd_jaccard.hpp`)
- Automatic SIMD detection at compile time
- **AVX2 support**: 256-bit SIMD for x86-64 (Intel Haswell+, AMD Excavator+)
  - Processes 4× uint64_t or 8× uint32_t per iteration
  - ~2-4x speedup over scalar implementation
- **SSE2 support**: 128-bit SIMD for x86/x86-64 (Intel Pentium 4+, AMD Athlon 64+)
  - Processes 2× uint64_t or 4× uint32_t per iteration
  - ~1.5-2x speedup over scalar implementation
- **ARM NEON support**: 128-bit SIMD for ARM (ARMv7-A with NEON, all ARMv8)
  - Processes 2× uint64_t or 4× uint32_t per iteration
  - ~1.5-2x speedup over scalar implementation
- **Scalar fallback**: Portable implementation for all architectures
- `count_matches()` - Auto-dispatching SIMD comparison function
- `get_simd_name()` - Runtime query of active SIMD instruction set

**Enhanced MinHash Class**
- Updated `jaccard()` method to use SIMD optimizations automatically
- Improved performance for signature comparison
- No API changes - existing code gets SIMD benefits automatically

**Examples**
- `basic_example.cpp` - Introduction to MinHash usage
- `jaccard_simd_example.cpp` - Demonstrates exact Jaccard, SIMD, and benchmarking

**Documentation**
- `README.md` - Comprehensive user guide
- `QUICKSTART.md` - Quick start guide with common use cases
- `IMPLEMENTATION.md` - Technical details and comparison with existing code
- `SIMD_OPTIMIZATIONS.md` - Detailed SIMD implementation guide
- `LICENSE` - MIT license

#### Build Systems

**CMake**
- Modern CMake 3.16+ support
- Install targets with proper namespacing
- Package config files for easy integration
- Options: `BUILD_EXAMPLES`, `BUILD_SHARED_LIBS`

**Meson**
- Meson 0.55+ support
- Ninja backend integration
- pkg-config generation
- Options: `build_examples`

#### Performance

Typical performance on modern hardware (Intel Core i7-9700K @ 3.6 GHz):
- **MinHash update**: ~1-2 million n-grams/second per thread
- **Jaccard comparison (SIMD)**: ~120-200 ns (AVX2/SSE2)
- **Jaccard comparison (scalar)**: ~400 ns
- **LSH banding**: ~2-10 microseconds
- **Exact Jaccard** (hash-based): ~2-10 microseconds for 100-1000 elements

#### Dependencies
- C++20 compatible compiler (GCC 10+, Clang 11+, MSVC 2019+)
- CMake 3.16+ or Meson 0.55+ (build time only)
- No runtime dependencies

#### Tested Platforms
- Linux x86-64 (GCC 13.3.0, SSE2/AVX2)
- Expected to work on:
  - macOS x86-64/ARM64
  - Windows x86-64
  - Linux ARM64

#### Known Issues
- None

#### Future Plans
- AVX-512 support
- Runtime CPU detection and dispatch
- SuperMinHash implementation
- Weighted MinHash
- GPU acceleration
- Serialization support
