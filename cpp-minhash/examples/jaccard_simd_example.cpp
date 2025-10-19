// SPDX-License-Identifier: MIT
// Copyright 2025 - Present Light Transport Entertainment Inc.
// Exact Jaccard and SIMD optimization example

#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <chrono>
#include "minhash.hpp"
#include "jaccard.hpp"
#include "simd_jaccard.hpp"

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
    std::cout << "MinHash with Exact Jaccard and SIMD Optimizations\n";
    std::cout << "==================================================\n\n";

    // Show which SIMD instruction set is being used
    std::cout << "SIMD Instruction Set: " << minhash::simd::get_simd_name() << "\n\n";

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

    std::cout << "=== Exact Jaccard Similarity (Ground Truth) ===\n";
    {
        // Use hash-based for better performance with large sets
        double j12 = minhash::jaccard::exact_hash(ngrams1, ngrams2);
        double j13 = minhash::jaccard::exact_hash(ngrams1, ngrams3);
        double j23 = minhash::jaccard::exact_hash(ngrams2, ngrams3);

        std::cout << std::fixed << std::setprecision(4);
        std::cout << "Exact Jaccard(doc1, doc2): " << j12 << "\n";
        std::cout << "Exact Jaccard(doc1, doc3): " << j13 << "\n";
        std::cout << "Exact Jaccard(doc2, doc3): " << j23 << "\n\n";
    }

    std::cout << "=== MinHash Estimation (SIMD-optimized) ===\n";
    {
        minhash::MinHash64 mh1(42);
        minhash::MinHash64 mh2(42);
        minhash::MinHash64 mh3(42);

        mh1.update_all(ngrams1);
        mh2.update_all(ngrams2);
        mh3.update_all(ngrams3);

        std::cout << std::fixed << std::setprecision(4);
        std::cout << "MinHash Jaccard(doc1, doc2): " << mh1.jaccard(mh2) << "\n";
        std::cout << "MinHash Jaccard(doc1, doc3): " << mh1.jaccard(mh3) << "\n";
        std::cout << "MinHash Jaccard(doc2, doc3): " << mh2.jaccard(mh3) << "\n\n";
    }

    std::cout << "=== Other Similarity Metrics ===\n";
    {
        // Dice coefficient
        double dice12 = minhash::jaccard::dice(ngrams1, ngrams2);
        std::cout << std::fixed << std::setprecision(4);
        std::cout << "Dice coefficient(doc1, doc2): " << dice12 << "\n";

        // Containment
        double cont12 = minhash::jaccard::containment(ngrams1, ngrams2);
        double cont21 = minhash::jaccard::containment(ngrams2, ngrams1);
        std::cout << "Containment(doc1 in doc2): " << cont12 << "\n";
        std::cout << "Containment(doc2 in doc1): " << cont21 << "\n\n";
    }

    std::cout << "=== Performance Comparison ===\n";
    {
        // Generate larger documents for benchmarking
        std::string large_doc;
        for (int i = 0; i < 1000; ++i) {
            large_doc += "The quick brown fox jumps over the lazy dog. ";
        }

        auto large_ngrams1 = generate_ngrams(large_doc, 5);
        auto large_ngrams2 = generate_ngrams(large_doc + "Extra text", 5);

        std::cout << "Testing with " << large_ngrams1.size() << " 5-grams\n\n";

        // Benchmark exact Jaccard
        auto start = std::chrono::high_resolution_clock::now();
        double exact_sim = minhash::jaccard::exact_hash(large_ngrams1, large_ngrams2);
        auto end = std::chrono::high_resolution_clock::now();
        auto exact_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        std::cout << "Exact Jaccard: " << std::fixed << std::setprecision(4) << exact_sim;
        std::cout << " (took " << exact_duration.count() << " μs)\n";

        // Benchmark MinHash
        minhash::MinHash64 mh1(42);
        minhash::MinHash64 mh2(42);

        start = std::chrono::high_resolution_clock::now();
        mh1.update_all(large_ngrams1);
        mh2.update_all(large_ngrams2);
        double minhash_sim = mh1.jaccard(mh2);
        end = std::chrono::high_resolution_clock::now();
        auto minhash_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        std::cout << "MinHash estimate: " << std::fixed << std::setprecision(4) << minhash_sim;
        std::cout << " (took " << minhash_duration.count() << " μs)\n";

        double speedup = static_cast<double>(exact_duration.count()) /
                        static_cast<double>(minhash_duration.count());
        std::cout << "\nMinHash speedup: " << std::fixed << std::setprecision(2)
                  << speedup << "x\n";
        std::cout << "Estimation error: " << std::fixed << std::setprecision(4)
                  << std::abs(exact_sim - minhash_sim) << "\n\n";
    }

    std::cout << "=== Direct SIMD Comparison Benchmark ===\n";
    {
        // Create two signatures
        minhash::MinHash64 mh1(42);
        minhash::MinHash64 mh2(42);

        // Add some data
        for (int i = 0; i < 100; ++i) {
            std::string data = "element_" + std::to_string(i);
            mh1.update(data);
            mh2.update(data);
        }

        // Change a few elements in mh2
        for (int i = 0; i < 20; ++i) {
            std::string data = "different_" + std::to_string(i);
            mh2.update(data);
        }

        // Benchmark SIMD comparison (done in jaccard method)
        const int iterations = 10000;
        auto start = std::chrono::high_resolution_clock::now();
        double sum = 0.0;
        for (int i = 0; i < iterations; ++i) {
            sum += mh1.jaccard(mh2);
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

        std::cout << "SIMD comparison (" << minhash::simd::get_simd_name() << ")\n";
        std::cout << "Performed " << iterations << " comparisons\n";
        std::cout << "Average time per comparison: "
                  << (duration.count() / iterations) << " ns\n";
        std::cout << "(Sum to prevent optimization: " << sum << ")\n\n";
    }

    std::cout << "=== Comparison Accuracy Test ===\n";
    {
        // Test MinHash accuracy across different similarity levels
        std::cout << std::fixed << std::setprecision(4);
        std::cout << "Similarity | Exact    | MinHash  | Error\n";
        std::cout << "-----------|----------|----------|-------\n";

        for (double target_sim : {0.1, 0.3, 0.5, 0.7, 0.9}) {
            // Create sets with target Jaccard similarity
            std::vector<std::string> set_a;
            std::vector<std::string> set_b;

            const int total_elements = 1000;
            const int shared = static_cast<int>(target_sim * total_elements);

            // Add shared elements
            for (int i = 0; i < shared; ++i) {
                std::string elem = "shared_" + std::to_string(i);
                set_a.push_back(elem);
                set_b.push_back(elem);
            }

            // Add unique elements to each
            for (int i = shared; i < total_elements; ++i) {
                set_a.push_back("a_" + std::to_string(i));
                set_b.push_back("b_" + std::to_string(i));
            }

            // Compute exact
            double exact = minhash::jaccard::exact_hash(set_a, set_b);

            // Compute MinHash estimate
            minhash::MinHash64 mh_a(42);
            minhash::MinHash64 mh_b(42);
            mh_a.update_all(set_a);
            mh_b.update_all(set_b);
            double estimate = mh_a.jaccard(mh_b);

            std::cout << std::setw(10) << std::left << target_sim << " | "
                      << std::setw(8) << exact << " | "
                      << std::setw(8) << estimate << " | "
                      << std::setw(7) << std::abs(exact - estimate) << "\n";
        }
    }

    return 0;
}
