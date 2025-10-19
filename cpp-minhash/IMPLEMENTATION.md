# Implementation Notes

## Overview

This is a modern C++20 implementation of the MinHash algorithm for estimating Jaccard similarity between sets. It was created to replace the existing MinHash implementation in the parent project with a more correct, maintainable, and feature-rich version.

## What Was Wrong With the Existing Implementation

The existing implementation in `../cpp/dedup.hh` and `../progressive_minhashing/fuzzy-dedup.hh` had several issues:

### 1. Uninitialized Minimum Values (Critical)

```cpp
// OLD CODE - PROBLEMATIC
for (uint32_t seed = 0; seed < N_MINHASH; seed++) {
    uint32_t min_hashval;  // Uninitialized!

    for (size_t n = 0; n < ngram_text.size(); n++) {
        uint32_t hashval;
        MurmurHash3_x86_32(..., seed, ...);

        if (n == 0) {
            min_hashval = hashval;
        } else {
            min_hashval = std::min(min_hashval, hashval);
        }
    }
    fingerprints[seed] = min_hashval;
}
```

**Problem**: If `ngram_text.size() == 0`, `min_hashval` is never initialized but is still used. This leads to undefined behavior.

**Solution**: Initialize to `std::numeric_limits<T>::max()` and compute minimum properly:

```cpp
// NEW CODE - CORRECT
for (std::size_t i = 0; i < NumHashes; ++i) {
    const uint32_t hash_seed = seed_ + static_cast<uint32_t>(i);
    const HashType hash_val = compute_hash(data, len, hash_seed);
    signature_[i] = min_hash(signature_[i], hash_val);
}
```

### 2. Only 32-bit Hashes

```cpp
// OLD CODE - Limited to 32-bit
MurmurHash3_x86_32(...);
```

**Problem**: 32-bit hashes have higher collision probability (~1 in 4 billion), which can reduce accuracy for large datasets.

**Solution**: Support 32-bit, 64-bit, and 128-bit hashes via templates:

```cpp
// NEW CODE - Flexible hash sizes
template <typename HashType = Hash64, std::size_t NumHashes = 128>
class MinHash;

using Hash32 = uint32_t;
using Hash64 = uint64_t;
using Hash128 = std::array<uint64_t, 2>;
```

### 3. Hardcoded LSH Banding

```cpp
// OLD CODE - Hardcoded values
constexpr uint32_t N_BUCKETS = 20;
constexpr uint32_t BUCKET_SIZE = 10;
```

**Problem**: LSH band configuration should be tunable based on desired similarity threshold. The hardcoded values work for Jaccard ~0.5 but not optimally for other thresholds.

**Solution**: Template-based configuration with sensible defaults:

```cpp
// NEW CODE - Configurable
template <typename HashType = Hash64,
          std::size_t NumHashes = 128,
          std::size_t NumBands = 16>
class LSHBands;
```

### 4. No Modern C++ Features

The old code used C++11/14 style with manual loops and index arithmetic.

**Solution**: Modern C++20 with:
- Concepts for type safety (`Hashable` concept)
- Ranges for cleaner iteration
- `[[nodiscard]]` for safety
- `constexpr` where applicable
- Better const correctness

### 5. Limited API

The old implementation only supported batch processing of n-grams.

**Solution**: Rich API with:
- Single element updates
- Batch updates
- String updates
- Range-based updates
- Merge operations
- Signature extraction

## Algorithm Correctness

### MinHash Algorithm

The correct MinHash algorithm:

1. Generate k independent hash functions (we simulate using different seeds)
2. For each hash function h_i:
   - Initialize min_i = ∞
   - For each element e in set S:
     - Compute h_i(e)
     - min_i = min(min_i, h_i(e))
3. Signature = [min_1, min_2, ..., min_k]

**Key property**: The probability that min_i(S1) == min_i(S2) equals the Jaccard similarity J(S1, S2).

### LSH Banding

For detecting candidates efficiently:

1. Divide k-length signature into b bands of r rows (k = b × r)
2. Hash each band independently
3. Two sets are candidates if they match in ≥1 band
4. Probability of matching: 1 - (1 - s^r)^b, where s = Jaccard similarity

**Tuning**:
- More bands (smaller r): Catches more duplicates (higher recall), more false positives
- Fewer bands (larger r): Fewer false positives (higher precision), misses some duplicates
- Optimal threshold: t = (1/b)^(1/r)

For our defaults (b=16, r=8): t ≈ 0.62

## Performance Improvements

1. **Fewer memory allocations**: Uses `std::array` for signatures (stack allocation)
2. **Better inlining**: Small functions marked for inlining
3. **Cache-friendly**: Contiguous memory layout
4. **SIMD potential**: Modern compilers can auto-vectorize the loops better

## Testing

Run the example program to verify correctness:

```bash
# CMake build
cd build-cmake
./basic_example

# Meson build
cd build-meson
./basic_example
```

Expected behavior:
- Similar documents (doc1, doc2) have high similarity (~85%)
- Dissimilar documents have low similarity (<5%)
- LSH bands correctly identify similar pairs
- Merge operation produces union (similarity = 1.0)

## Integration with Existing Code

To replace the old implementation:

1. Include the new header: `#include <minhash.hpp>`
2. Replace old code:

```cpp
// OLD
std::array<MinHashVal<BUCKET_SIZE, 2>, N_BUCKETS> lshs = compute_lsh<N_GRAM>(ngrams);

// NEW
minhash::MinHash64 mh(seed);
mh.update_all(ngrams);
auto bands = minhash::LSH64::compute_bands(mh.signature());
```

## Future Enhancements

Potential improvements:

1. **SuperMinHash**: More accurate variant for very large sets
2. **Weighted MinHash**: For sets with weighted elements
3. **GPU acceleration**: CUDA/OpenCL kernels for batch processing
4. **Parallel updates**: Thread-safe version with atomic operations
5. **Serialization**: Save/load signatures to/from disk
6. **B-bit MinHash**: Compress signatures further (1-4 bits per hash)

## References

1. Broder, A. Z. (1997). "On the resemblance and containment of documents"
2. Rajaraman & Ullman (2011). "Mining of Massive Datasets", Chapter 3
3. Leskovec et al. (2020). "Mining of Massive Datasets", 3rd Edition
4. [datasketch library](https://github.com/ekzhu/datasketch) - Python reference implementation
