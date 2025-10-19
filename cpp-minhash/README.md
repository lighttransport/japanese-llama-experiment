# cpp-minhash

A modern C++20 implementation of MinHash for estimating Jaccard similarity and detecting near-duplicate documents, plus memory-efficient suffix array construction.

## Features

### MinHash & Similarity
- **Modern C++20**: Uses concepts, ranges, and other C++20 features
- **Multiple hash sizes**: Support for 32-bit, 64-bit, and 128-bit hash values
- **Switchable hash algorithms**: Choose between MurmurHash3 (default, high quality) and FNV-1a (fast, 3.4x speedup)
- **LSH Banding**: Built-in support for Locality-Sensitive Hashing bands for efficient duplicate detection
- **SIMD Optimizations**: Automatic CPU detection and dispatch for AVX2, SSE2, and ARM NEON
- **Exact Jaccard**: Ground truth similarity computation for validation and small datasets
- **Flexible API**: Supports single updates, batch updates, and merge operations
- **High performance**: Uses MurmurHash3 or FNV-1a for hashing with SIMD-accelerated comparisons

### Document Deduplication
- **LSH-based clustering**: Efficient near-duplicate detection using band hashing
- **Configurable similarity**: Tunable threshold with optimal parameter calculation
- **Fast candidate generation**: Band hash tables for O(1) lookups
- **Duplicate clustering**: Union-Find algorithm for grouping duplicates
- **Character & word n-grams**: Support for both fine and coarse-grained similarity
- **Statistics tracking**: False positive rate, deduplication ratio, cluster analysis
- **Production-ready**: Handles 10K+ documents with <10ms deduplication time

### Suffix Arrays
- **Memory-efficient**: Full 32-bit range (4GB) vs libsais 2GB limit
- **No MSB bit flags**: Allows twice the maximum text size
- **Fast prefix-doubling algorithm**: O(n log² n), practical for most use cases
- **SIMD-accelerated**: AVX2, SSE2, and ARM NEON support for ~2-3x speedup
- **Simple & reliable**: Easy to verify correctness
- **Pattern matching**: Binary search on suffix array for substring queries

### General
- **Header-mostly**: Core algorithms are header-only, only MurmurHash3 needs compilation
- **Dual build systems**: Supports both CMake and Meson+Ninja

## What is MinHash?

MinHash is a probabilistic data structure used to estimate the Jaccard similarity between sets. It's particularly useful for:

- Near-duplicate detection in large document collections
- Clustering similar documents
- Estimating set similarity without storing full sets
- Text deduplication pipelines

### How it works

1. **Shingling**: Break documents into overlapping n-grams (shingles)
2. **Hashing**: Apply k independent hash functions to each shingle
3. **Min selection**: For each hash function, keep the minimum hash value
4. **Signature**: The k minimum values form a compact signature
5. **Comparison**: Jaccard similarity ≈ fraction of matching signature values

### LSH Banding

For efficient duplicate detection at scale, MinHash signatures can be divided into bands:

- Divide k hashes into b bands of r rows each (k = b × r)
- Hash each band independently
- Documents matching in ≥1 band are candidate duplicates
- Probability of matching: 1 - (1 - s^r)^b, where s is Jaccard similarity

## Installation

### Requirements

- C++20 compatible compiler (GCC 10+, Clang 11+, MSVC 2019+)
- CMake 3.16+ or Meson 0.55+
- Ninja (optional, for Meson builds)

### Building with CMake

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
sudo make install
```

Options:
- `-DBUILD_EXAMPLES=ON/OFF` - Build example programs (default: ON)
- `-DBUILD_SHARED_LIBS=ON/OFF` - Build shared libraries (default: ON)

### Building with Meson + Ninja

```bash
meson setup build --buildtype=release
ninja -C build
sudo ninja -C build install
```

Options:
- `-Dbuild_examples=true/false` - Build example programs (default: true)

## Usage

### Basic Example

```cpp
#include <minhash.hpp>
#include <vector>
#include <string>

// Generate 3-grams from text
std::vector<std::string> generate_ngrams(const std::string& text, size_t n = 3);

int main() {
    // Create MinHash with 128 hash functions using 64-bit hashes
    minhash::MinHash64 mh1(42);  // seed = 42
    minhash::MinHash64 mh2(42);  // same seed for comparison

    auto ngrams1 = generate_ngrams("The quick brown fox");
    auto ngrams2 = generate_ngrams("The quick brown cat");

    // Update signatures
    mh1.update_all(ngrams1);
    mh2.update_all(ngrams2);

    // Estimate Jaccard similarity
    double similarity = mh1.jaccard(mh2);
    std::cout << "Similarity: " << similarity << "\n";
}
```

### LSH Banding for Duplicate Detection

```cpp
#include <minhash.hpp>

int main() {
    minhash::MinHash64 mh1(42);
    minhash::MinHash64 mh2(42);

    // ... update with n-grams ...

    // Compute LSH bands (16 bands of 8 rows each)
    auto bands1 = minhash::LSH64::compute_bands(mh1.signature());
    auto bands2 = minhash::LSH64::compute_bands(mh2.signature());

    // Check if any band matches (candidate duplicate)
    if (minhash::LSH64::has_matching_band(bands1, bands2)) {
        std::cout << "Potential duplicate detected!\n";
    }
}
```

### Advanced: Custom Configuration

```cpp
// 256 hash functions with 32-bit hashes
minhash::MinHash<minhash::Hash32, 256> mh(123);

// 128-bit hashes for maximum collision resistance
minhash::MinHash128 mh_large(456);

// Custom LSH: 128 hashes divided into 32 bands
minhash::LSHBands<minhash::Hash64, 128, 32> custom_lsh;
auto bands = custom_lsh::compute_bands(signature);
```

### Merge Operation (Union)

```cpp
minhash::MinHash64 mh1(42);
minhash::MinHash64 mh2(42);

// ... add different elements to each ...

// Merge mh2 into mh1 (union operation)
mh1.merge(mh2);
// mh1 now represents the union of both sets
```

### Exact Jaccard Computation

For validation or when working with small datasets, you can compute exact Jaccard similarity:

```cpp
#include <jaccard.hpp>
#include <vector>
#include <string>

int main() {
    std::vector<std::string> set_a = {"apple", "banana", "cherry"};
    std::vector<std::string> set_b = {"banana", "cherry", "date"};

    // Fast hash-based exact Jaccard
    double exact = minhash::jaccard::exact_hash(set_a, set_b);
    std::cout << "Exact Jaccard: " << exact << "\n";

    // For sorted data (more efficient)
    double exact_sorted = minhash::jaccard::exact_sorted(set_a, set_b);

    // Other metrics
    double dice = minhash::jaccard::dice(set_a, set_b);
    double containment = minhash::jaccard::containment(set_a, set_b);
}
```

### SIMD Optimizations

MinHash automatically uses SIMD instructions when available for signature comparison:

```cpp
#include <simd_jaccard.hpp>

// Check which SIMD instruction set is being used
const char* simd_name = minhash::simd::get_simd_name();
std::cout << "Using: " << simd_name << "\n";
// Output: "AVX2", "SSE2", "NEON", or "Scalar"

// jaccard() method automatically uses best available SIMD
minhash::MinHash64 mh1(42), mh2(42);
// ... update signatures ...
double sim = mh1.jaccard(mh2);  // SIMD-accelerated!
```

**Supported SIMD Instruction Sets:**
- **AVX2**: 256-bit, ~2-4x faster (Intel Haswell+, AMD Excavator+)
- **SSE2**: 128-bit, ~1.5-2x faster (Intel Pentium 4+, AMD Athlon 64+)
- **ARM NEON**: 128-bit, ~1.5-2x faster (ARMv7-A+, all ARMv8)
- **Scalar**: Portable fallback for all architectures

The library automatically detects and uses the best available instruction set at compile time.

### Hash Algorithm Selection

The library supports two hash algorithms, selectable at compile time:

**MurmurHash3 (Default)**
- Industry-standard, high-quality hash
- Excellent avalanche properties
- Better MinHash accuracy (~4.5% error)
- Quality score: 9/10
- **Use for**: General-purpose, accuracy-critical applications

**FNV-1a (Fast Alternative)**
- **3.4x faster** than MurmurHash3
- Excellent distribution uniformity
- Lower MinHash accuracy (~14.4% error)
- SIMD batch processing available
- Quality score: 7/10
- **Use for**: High-throughput, speed-critical applications

```bash
# Default build (MurmurHash3)
cmake -DCMAKE_BUILD_TYPE=Release ..
make

# Fast build (FNV-1a, 3.4x speedup!)
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-DMINHASH_USE_FNV1A" ..
make
```

**Check active algorithm:**
```cpp
#include <hash_function.hpp>
std::cout << minhash::hash::get_algorithm_name() << "\n";  // "MurmurHash3" or "FNV-1a"
```

See [HASH_ALGORITHMS.md](HASH_ALGORITHMS.md) for detailed comparison and benchmarks.

### Document Deduplication

Efficiently detect and cluster near-duplicate documents using LSH banding:

```cpp
#include <deduplicator.hpp>

int main() {
    // Create deduplicator with 128 hashes, 16 bands
    minhash::Deduplicator64 dedup(42);  // seed = 42

    // Sample documents
    std::vector<std::string> docs = {
        "The quick brown fox jumps over the lazy dog",
        "The quick brown fox jumps over the lazy cat",  // Similar
        "The quick brown fox jumps over the lazy dog",  // Duplicate
        "Completely different content here"
    };

    // Add documents with character 3-grams
    for (size_t i = 0; i < docs.size(); ++i) {
        auto ngrams = minhash::generate_ngrams(docs[i], 3);
        dedup.add_document(i, ngrams);
    }

    // Find duplicates at 80% similarity threshold
    auto clusters = dedup.find_duplicates(0.8);

    // Process results
    for (const auto& cluster : clusters) {
        if (cluster.size() > 1) {
            std::cout << "Duplicate group:\n";
            for (auto doc_id : cluster) {
                std::cout << "  Doc " << doc_id << ": "
                          << docs[doc_id] << "\n";
            }
        }
    }

    // Get statistics
    auto stats = dedup.get_stats();
    std::cout << "Duplicates found: " << stats.duplicate_documents << "\n";
    std::cout << "Unique documents: " << stats.unique_documents << "\n";
    std::cout << "False positive rate: " << stats.false_positive_rate() << "\n";
}
```

**Word-level deduplication:**
```cpp
// For phrase-based similarity
auto shingles = minhash::generate_word_shingles(text, 3);
dedup.add_document(id, shingles);
```

**LSH Parameters:**
- Default: 16 bands × 8 rows (128 hashes)
- Optimal threshold: ~0.707 (70.7% similarity)
- Probability of being candidate: P = 1 - (1 - s^r)^b

**Performance:**
- 10K documents: ~5 seconds build, ~9ms deduplication
- False positive rate: 2-5% (tunable)
- Memory efficient: O(n) space

### Suffix Array Construction

Build suffix arrays for pattern matching, text compression, and bioinformatics:

```cpp
#include <sais.hpp>

int main() {
    // Build suffix array from string
    std::string text = "banana";
    auto SA = sais::build_suffix_array(text);

    // SA = [5, 3, 1, 0, 4, 2]
    // Suffixes in lexicographic order:
    // SA[0]=5: "a"
    // SA[1]=3: "ana"
    // SA[2]=1: "anana"
    // SA[3]=0: "banana"
    // SA[4]=4: "na"
    // SA[5]=2: "nana"

    // Pattern matching using binary search
    std::string pattern = "ana";
    auto lower = std::lower_bound(SA.begin(), SA.end(), 0,
        [&](uint32_t sa_pos, int) {
            return text.substr(sa_pos).compare(0, pattern.size(), pattern) < 0;
        });

    auto upper = std::upper_bound(SA.begin(), SA.end(), 0,
        [&](int, uint32_t sa_pos) {
            return pattern.compare(text.substr(sa_pos, pattern.size())) < 0;
        });

    // Found 2 occurrences: positions 1 and 3
    for (auto it = lower; it != upper; ++it) {
        std::cout << "Match at position " << *it << "\n";
    }
}
```

**Memory Efficiency:**
- **Full 32-bit range**: Supports text up to 4GB (UINT32_MAX bytes)
- **libsais limitation**: Uses MSB bit for flags, limiting to 2GB (INT32_MAX bytes)
- **Our advantage**: 2x larger maximum text size with same memory

**SIMD Acceleration:**
```cpp
// Check which SIMD instruction set is being used
std::cout << "SIMD: " << sais::get_simd_name() << "\n";
// Output: "AVX2", "SSE2", "NEON", or "Scalar"
```

The library automatically detects and uses:
- **AVX2**: 8-way parallel, ~2-3x faster (Intel Haswell+, AMD Excavator+)
- **SSE2**: 4-way parallel, ~1.5-2x faster (Intel Pentium 4+, AMD Athlon 64+)
- **ARM NEON**: 4-way parallel, ~1.5-2x faster (ARMv7-A+, all ARMv8)
- **Scalar**: Portable fallback for all architectures

**Performance (O(n log² n) with SIMD optimization):**
- 10K characters: ~2ms
- 100K characters: ~20ms
- 1M characters: ~300ms

**Use Cases:**
- Pattern matching with binary search
- Longest common substring
- Burrows-Wheeler Transform (BWT) for compression
- FM-index construction
- Text indexing and search
- Bioinformatics sequence analysis

## API Reference

### MinHash Class

```cpp
template <typename HashType = Hash64, std::size_t NumHashes = 128>
class MinHash;
```

**Methods:**
- `void update(const void* data, std::size_t len)` - Update with raw data
- `void update(const T& element)` - Update with hashable element
- `void update(std::string_view str)` - Update with string
- `void update_all(const R& elements)` - Update with range of elements
- `double jaccard(const MinHash& other) const` - Estimate Jaccard similarity
- `void merge(const MinHash& other)` - Merge (union) operation
- `void reset()` - Reset to initial state
- `const signature_type& signature() const` - Get signature

**Type Aliases:**
- `MinHash32` - 128 permutations with 32-bit hashes
- `MinHash64` - 128 permutations with 64-bit hashes
- `MinHash128` - 128 permutations with 128-bit hashes

### LSHBands Class

```cpp
template <typename HashType = Hash64, std::size_t NumHashes = 128, std::size_t NumBands = 20>
class LSHBands;
```

**Methods:**
- `static bands_type compute_bands(const signature_type& signature)` - Compute band hashes
- `static bool has_matching_band(const bands_type& b1, const bands_type& b2)` - Check for matches

**Type Aliases:**
- `LSH64` - 128 hashes, 16 bands (8 rows per band)
- `LSH128` - 128-bit hashes, 16 bands (8 rows per band)

## Implementation Notes

### Why Different Seeds Must Match

MinHash objects with different seeds use different hash functions and cannot be meaningfully compared. Always use the same seed for MinHash objects you intend to compare.

### Choosing Hash Size

- **32-bit**: Fastest, good for prototyping, higher collision risk
- **64-bit**: Recommended default, good balance of speed and accuracy
- **128-bit**: Maximum accuracy, slower, use for critical applications

### Choosing Number of Permutations

- More permutations = better accuracy but slower and more memory
- 128 permutations is a good default
- 64 permutations for speed-critical applications
- 256+ for maximum accuracy

### LSH Band Configuration

For b bands of r rows (k = b × r hashes):
- **More bands**: Higher recall (catches more duplicates), lower precision
- **More rows per band**: Higher precision, lower recall
- Default (16 bands × 8 rows) works well for Jaccard similarity ~0.6

Optimal threshold: t = (1/b)^(1/r)
- For b=16, r=8: t ≈ 0.62

## Performance

Typical performance on modern hardware:
- Update: ~1-2 million n-grams/second per thread
- Jaccard comparison: ~1-5 microseconds
- LSH banding: ~2-10 microseconds

## License

MIT License - see LICENSE file for details

MurmurHash3 by Austin Appleby is in the public domain.

## References

1. Broder, A. Z. (1997). "On the resemblance and containment of documents"
2. Rajaraman, A., & Ullman, J. D. (2011). "Mining of Massive Datasets"
3. [MinHash - Wikipedia](https://en.wikipedia.org/wiki/MinHash)
4. [datasketch library](https://github.com/ekzhu/datasketch)
