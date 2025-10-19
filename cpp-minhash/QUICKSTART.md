# Quick Start Guide

## Build and Run (CMake)

```bash
cd cpp-minhash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
./basic_example
```

## Build and Run (Meson)

```bash
cd cpp-minhash
meson setup build --buildtype=release
ninja -C build
./build/basic_example
```

## Minimal Usage Example

```cpp
#include <minhash.hpp>
#include <iostream>
#include <vector>
#include <string>

int main() {
    // Create two MinHash objects with the same seed
    minhash::MinHash64 mh1(42);
    minhash::MinHash64 mh2(42);

    // Add elements (e.g., n-grams from documents)
    std::vector<std::string> doc1_ngrams = {"the", "qui", "uic", "ick"};
    std::vector<std::string> doc2_ngrams = {"the", "qui", "uic", "ick"};

    mh1.update_all(doc1_ngrams);
    mh2.update_all(doc2_ngrams);

    // Estimate similarity
    double similarity = mh1.jaccard(mh2);
    std::cout << "Jaccard similarity: " << similarity << "\n";

    // Check for duplicates using LSH
    auto bands1 = minhash::LSH64::compute_bands(mh1.signature());
    auto bands2 = minhash::LSH64::compute_bands(mh2.signature());

    if (minhash::LSH64::has_matching_band(bands1, bands2)) {
        std::cout << "Potential duplicate!\n";
    }

    return 0;
}
```

## Common Use Cases

### Document Deduplication

```cpp
#include <minhash.hpp>
#include <unordered_map>
#include <vector>
#include <string>

struct BandHash {
    std::size_t operator()(const std::array<uint64_t, 16>& bands) const {
        std::size_t h = 0;
        for (auto b : bands) h ^= b;
        return h;
    }
};

void deduplicate_documents(const std::vector<std::vector<std::string>>& docs) {
    std::unordered_map<std::array<uint64_t, 16>, std::vector<size_t>, BandHash> index;

    for (size_t i = 0; i < docs.size(); ++i) {
        minhash::MinHash64 mh(42);
        mh.update_all(docs[i]);
        auto bands = minhash::LSH64::compute_bands(mh.signature());

        // Find documents with matching bands
        if (index.count(bands)) {
            std::cout << "Document " << i << " is similar to: ";
            for (auto j : index[bands]) {
                std::cout << j << " ";
            }
            std::cout << "\n";
        }

        index[bands].push_back(i);
    }
}
```

### Clustering Similar Items

```cpp
#include <minhash.hpp>
#include <vector>

// Group items by similarity threshold
std::vector<std::vector<size_t>> cluster_by_similarity(
    const std::vector<std::vector<std::string>>& items,
    double threshold = 0.7)
{
    std::vector<minhash::MinHash64> signatures;
    for (const auto& item : items) {
        minhash::MinHash64 mh(42);
        mh.update_all(item);
        signatures.push_back(mh);
    }

    std::vector<std::vector<size_t>> clusters;
    std::vector<bool> assigned(items.size(), false);

    for (size_t i = 0; i < items.size(); ++i) {
        if (assigned[i]) continue;

        std::vector<size_t> cluster = {i};
        assigned[i] = true;

        for (size_t j = i + 1; j < items.size(); ++j) {
            if (!assigned[j] && signatures[i].jaccard(signatures[j]) >= threshold) {
                cluster.push_back(j);
                assigned[j] = true;
            }
        }

        clusters.push_back(cluster);
    }

    return clusters;
}
```

## Tips

1. **Always use the same seed** for MinHash objects you want to compare
2. **Use 64-bit hashes** by default (good balance of speed and accuracy)
3. **Adjust LSH bands** based on your similarity threshold:
   - Higher threshold → use more rows per band (fewer bands)
   - Lower threshold → use more bands (fewer rows per band)
4. **Profile first** before switching to 128-bit hashes (often unnecessary)
5. **Generate good n-grams**: Usually 3-5 character n-grams work well for text

## Build Options

### CMake Options

- `-DBUILD_EXAMPLES=ON/OFF` - Build examples (default: ON)
- `-DBUILD_SHARED_LIBS=ON/OFF` - Build shared library (default: ON)
- `-DCMAKE_BUILD_TYPE=Release/Debug` - Build type

### Meson Options

- `-Dbuild_examples=true/false` - Build examples (default: true)
- `--buildtype=release/debug` - Build type

## Installation

```bash
# CMake
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
sudo make install

# Meson
meson setup build --buildtype=release
ninja -C build
sudo ninja -C build install
```

## Troubleshooting

**Q: Compile error about C++20 not supported**

A: Use a newer compiler (GCC 10+, Clang 11+, MSVC 2019+)

**Q: Different similarity results each run**

A: Make sure you're using the same seed for MinHash objects being compared

**Q: Low accuracy for small sets**

A: MinHash works best with sets of 100+ elements. For small sets, use exact Jaccard.

**Q: Too many false positives with LSH**

A: Increase rows per band (decrease number of bands) for higher precision
