// SPDX-License-Identifier: MIT
// Copyright 2025 - Present Light Transport Entertainment Inc.
// Basic MinHash usage example

#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include "minhash.hpp"

// Simple n-gram generator for text
std::vector<std::string> generate_ngrams(const std::string& text, std::size_t n = 3) {
    std::vector<std::string> ngrams;
    if (text.size() < n) {
        return ngrams;
    }

    for (std::size_t i = 0; i <= text.size() - n; ++i) {
        ngrams.push_back(text.substr(i, n));
    }
    return ngrams;
}

int main() {
    std::cout << "MinHash C++20 Example\n";
    std::cout << "=====================\n\n";

    // Example documents
    const std::string doc1 = "The quick brown fox jumps over the lazy dog";
    const std::string doc2 = "The quick brown fox jumps over the lazy cat";
    const std::string doc3 = "A completely different document with other words";

    std::cout << "Document 1: " << doc1 << "\n";
    std::cout << "Document 2: " << doc2 << "\n";
    std::cout << "Document 3: " << doc3 << "\n\n";

    // Generate 3-grams for each document
    auto ngrams1 = generate_ngrams(doc1, 3);
    auto ngrams2 = generate_ngrams(doc2, 3);
    auto ngrams3 = generate_ngrams(doc3, 3);

    std::cout << "Generated " << ngrams1.size() << " 3-grams for doc1\n";
    std::cout << "Generated " << ngrams2.size() << " 3-grams for doc2\n";
    std::cout << "Generated " << ngrams3.size() << " 3-grams for doc3\n\n";

    // Example 1: 32-bit MinHash
    std::cout << "=== 32-bit MinHash (128 permutations) ===\n";
    {
        minhash::MinHash32 mh1(42);
        minhash::MinHash32 mh2(42);
        minhash::MinHash32 mh3(42);

        mh1.update_all(ngrams1);
        mh2.update_all(ngrams2);
        mh3.update_all(ngrams3);

        std::cout << std::fixed << std::setprecision(4);
        std::cout << "Similarity(doc1, doc2): " << mh1.jaccard(mh2) << "\n";
        std::cout << "Similarity(doc1, doc3): " << mh1.jaccard(mh3) << "\n";
        std::cout << "Similarity(doc2, doc3): " << mh2.jaccard(mh3) << "\n\n";
    }

    // Example 2: 64-bit MinHash (more accurate)
    std::cout << "=== 64-bit MinHash (128 permutations) ===\n";
    {
        minhash::MinHash64 mh1(42);
        minhash::MinHash64 mh2(42);
        minhash::MinHash64 mh3(42);

        mh1.update_all(ngrams1);
        mh2.update_all(ngrams2);
        mh3.update_all(ngrams3);

        std::cout << std::fixed << std::setprecision(4);
        std::cout << "Similarity(doc1, doc2): " << mh1.jaccard(mh2) << "\n";
        std::cout << "Similarity(doc1, doc3): " << mh1.jaccard(mh3) << "\n";
        std::cout << "Similarity(doc2, doc3): " << mh2.jaccard(mh3) << "\n\n";
    }

    // Example 3: LSH bands for duplicate detection
    std::cout << "=== LSH Bands (16 bands of 8 rows each) ===\n";
    {
        minhash::MinHash64 mh1(42);
        minhash::MinHash64 mh2(42);
        minhash::MinHash64 mh3(42);

        mh1.update_all(ngrams1);
        mh2.update_all(ngrams2);
        mh3.update_all(ngrams3);

        auto bands1 = minhash::LSH64::compute_bands(mh1.signature());
        auto bands2 = minhash::LSH64::compute_bands(mh2.signature());
        auto bands3 = minhash::LSH64::compute_bands(mh3.signature());

        std::cout << "Bands match (doc1, doc2): "
                  << (minhash::LSH64::has_matching_band(bands1, bands2) ? "YES" : "NO") << "\n";
        std::cout << "Bands match (doc1, doc3): "
                  << (minhash::LSH64::has_matching_band(bands1, bands3) ? "YES" : "NO") << "\n";
        std::cout << "Bands match (doc2, doc3): "
                  << (minhash::LSH64::has_matching_band(bands2, bands3) ? "YES" : "NO") << "\n\n";

        // Show band hashes for doc1
        std::cout << "Band hashes for doc1 (first 5):\n";
        for (std::size_t i = 0; i < 5; ++i) {
            std::cout << "  Band " << i << ": 0x" << std::hex << bands1[i] << std::dec << "\n";
        }
    }

    // Example 4: Merge operation
    std::cout << "\n=== Merge Operation (Union) ===\n";
    {
        minhash::MinHash64 mh1(42);
        minhash::MinHash64 mh2(42);

        // Add first half of ngrams to mh1
        for (std::size_t i = 0; i < ngrams1.size() / 2; ++i) {
            mh1.update(ngrams1[i]);
        }

        // Add second half to mh2
        for (std::size_t i = ngrams1.size() / 2; i < ngrams1.size(); ++i) {
            mh2.update(ngrams1[i]);
        }

        // Create a combined MinHash
        minhash::MinHash64 mh_all(42);
        mh_all.update_all(ngrams1);

        // Merge mh1 and mh2
        minhash::MinHash64 mh_merged = mh1;
        mh_merged.merge(mh2);

        std::cout << "Similarity(merged, full): " << mh_merged.jaccard(mh_all) << "\n";
        std::cout << "(Should be close to 1.0 as they represent the same set)\n";
    }

    return 0;
}
