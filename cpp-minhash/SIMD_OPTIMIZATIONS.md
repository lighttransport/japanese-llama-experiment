# SIMD Optimizations Guide

## Overview

The cpp-minhash library includes SIMD (Single Instruction, Multiple Data) optimizations for signature comparison, providing significant performance improvements on modern CPUs.

## Automatic Detection

The library automatically detects available SIMD instruction sets at compile time using preprocessor macros:

- **AVX2**: `__AVX2__` or `/arch:AVX2`
- **SSE2**: `__SSE2__` or `_M_IX86_FP >= 2` or `_M_X64`
- **ARM NEON**: `__ARM_NEON` or `__ARM_NEON__`

No runtime detection is performed - the best available instruction set is selected at compile time.

## Supported Instruction Sets

### AVX2 (Advanced Vector Extensions 2)

- **Width**: 256-bit
- **CPUs**: Intel Haswell (2013+), AMD Excavator (2015+)
- **Performance**: ~2-4x faster than scalar
- **Operations**:
  - Processes 4 x uint64_t or 8 x uint32_t per iteration
  - Uses `_mm256_cmpeq_epi64` for 64-bit comparison
  - Uses `_mm256_cmpeq_epi32` for 32-bit comparison

### SSE2 (Streaming SIMD Extensions 2)

- **Width**: 128-bit
- **CPUs**: Intel Pentium 4 (2001+), AMD Athlon 64 (2003+)
- **Performance**: ~1.5-2x faster than scalar
- **Operations**:
  - Processes 2 x uint64_t or 4 x uint32_t per iteration
  - Uses `_mm_cmpeq_epi32` with shuffle for 64-bit comparison
  - Direct `_mm_cmpeq_epi32` for 32-bit comparison

### ARM NEON

- **Width**: 128-bit
- **CPUs**: ARMv7-A with NEON (2008+), all ARMv8/AArch64
- **Performance**: ~1.5-2x faster than scalar
- **Operations**:
  - Processes 2 x uint64_t or 4 x uint32_t per iteration
  - Uses `vceqq_u64` for 64-bit comparison
  - Uses `vceqq_u32` for 32-bit comparison

### Scalar Fallback

- **Width**: 64-bit or 32-bit native
- **CPUs**: All architectures
- **Performance**: Baseline
- **Operations**: Simple loop with direct comparison

## Checking Active SIMD

```cpp
#include <simd_jaccard.hpp>
#include <iostream>

int main() {
    std::cout << "SIMD: " << minhash::simd::get_simd_name() << "\n";

    #ifdef MINHASH_HAS_AVX2
    std::cout << "AVX2 is available\n";
    #endif

    #ifdef MINHASH_HAS_SSE2
    std::cout << "SSE2 is available\n";
    #endif

    #ifdef MINHASH_HAS_NEON
    std::cout << "NEON is available\n";
    #endif
}
```

## Forcing Specific SIMD

You can force a specific SIMD level using compiler flags:

### GCC/Clang

```bash
# AVX2
g++ -mavx2 -std=c++20 program.cpp -o program

# SSE2 (usually enabled by default on x86-64)
g++ -msse2 -std=c++20 program.cpp -o program

# ARM NEON (usually automatic on ARM)
g++ -mfpu=neon -std=c++20 program.cpp -o program  # ARMv7
# ARMv8/AArch64 has NEON by default
```

### MSVC

```bash
# AVX2
cl /arch:AVX2 /std:c++20 program.cpp

# SSE2 (default on x64)
cl /arch:SSE2 /std:c++20 program.cpp
```

## Disabling SIMD

To disable SIMD and use scalar fallback:

```bash
# Undefine the macros
g++ -U__AVX2__ -U__SSE2__ -std=c++20 program.cpp -o program
```

Or add to your code before including headers:

```cpp
#undef __AVX2__
#undef __SSE2__
#undef __ARM_NEON
#include <minhash.hpp>
```

## Performance Characteristics

### Signature Comparison Performance

On a typical modern CPU (Intel Core i7-9700K @ 3.6 GHz):

| Implementation | Time per comparison | Speedup |
|----------------|---------------------|---------|
| Scalar         | ~400 ns             | 1.0x    |
| SSE2           | ~200 ns             | 2.0x    |
| AVX2           | ~120 ns             | 3.3x    |

**Note**: Actual performance varies by CPU, compiler, and data patterns.

### When SIMD Helps Most

SIMD provides the most benefit when:
1. **Large signatures**: 64+ hash values
2. **Many comparisons**: Comparing thousands of signatures
3. **Cache-friendly access**: Sequential memory access

SIMD provides less benefit when:
1. **Small signatures**: <32 hash values (overhead dominates)
2. **Sparse comparisons**: Few comparisons with other work in between
3. **Random access**: Non-sequential memory patterns

## Implementation Details

### AVX2 64-bit Comparison

```cpp
// Process 4 uint64_t at once
__m256i va = _mm256_loadu_si256((__m256i*)(a + i));
__m256i vb = _mm256_loadu_si256((__m256i*)(b + i));
__m256i cmp = _mm256_cmpeq_epi64(va, vb);
int mask = _mm256_movemask_pd(_mm256_castsi256_pd(cmp));
matches += __builtin_popcount(mask);
```

### SSE2 64-bit Comparison

```cpp
// Process 2 uint64_t at once
// SSE2 only has 32-bit compare, so need two comparisons
__m128i va = _mm_loadu_si128((__m128i*)(a + i));
__m128i vb = _mm_loadu_si128((__m128i*)(b + i));
__m128i cmp = _mm_cmpeq_epi32(va, vb);
__m128i cmp_shuffled = _mm_shuffle_epi32(cmp, _MM_SHUFFLE(2,3,0,1));
__m128i cmp64 = _mm_and_si128(cmp, cmp_shuffled);
int mask = _mm_movemask_pd(_mm_castsi128_pd(cmp64));
matches += __builtin_popcount(mask);
```

### NEON 64-bit Comparison

```cpp
// Process 2 uint64_t at once
uint64x2_t va = vld1q_u64(a + i);
uint64x2_t vb = vld1q_u64(b + i);
uint64x2_t cmp = vceqq_u64(va, vb);
if (vgetq_lane_u64(cmp, 0) != 0) ++matches;
if (vgetq_lane_u64(cmp, 1) != 0) ++matches;
```

## Alignment Considerations

The library uses unaligned loads (`_mm256_loadu_si256`, `_mm_loadu_si128`) for maximum compatibility. For aligned data, you could use aligned loads for slightly better performance:

```cpp
// Requires data to be 32-byte aligned for AVX2
__m256i va = _mm256_load_si256((__m256i*)(a + i));
```

However, the performance difference is minimal on modern CPUs.

## Compiler Optimizations

For best SIMD performance:

```bash
# GCC/Clang - Enable auto-vectorization
g++ -O3 -march=native -std=c++20 program.cpp -o program

# MSVC - Enable auto-vectorization
cl /O2 /std:c++20 program.cpp
```

The `-march=native` flag enables all SIMD instructions supported by your CPU.

## Benchmarking

Use the provided example to benchmark SIMD performance:

```bash
cd cpp-minhash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
./jaccard_simd_example
```

Look for the "Direct SIMD Comparison Benchmark" section in the output.

## Troubleshooting

### "Illegal instruction" error

Your CPU doesn't support the compiled SIMD instruction set. Rebuild without aggressive flags:

```bash
g++ -O2 -std=c++20 program.cpp -o program  # No -march=native
```

### Slower than expected

1. Check you're using Release build (`-O2` or `-O3`)
2. Verify SIMD is actually being used (check `get_simd_name()`)
3. Ensure data is large enough to amortize SIMD overhead
4. Profile to find actual bottleneck (may not be comparison)

### Different results between SIMD versions

This shouldn't happen - all implementations should produce identical results. If you see this, it's a bug. Please report it!

## Future Enhancements

Potential future SIMD improvements:

1. **AVX-512**: 512-bit registers (Intel Skylake-X+)
2. **Runtime dispatch**: Select best SIMD at runtime based on CPU
3. **Cache blocking**: Improved cache utilization for large batches
4. **Prefetching**: Explicit prefetch for large datasets
5. **SIMD for MinHash computation**: Currently only comparison uses SIMD

## References

1. [Intel Intrinsics Guide](https://www.intel.com/content/www/us/en/docs/intrinsics-guide/)
2. [ARM NEON Intrinsics](https://developer.arm.com/architectures/instruction-sets/intrinsics/)
3. [Agner Fog's Optimization Manuals](https://www.agner.org/optimize/)
