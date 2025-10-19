// SPDX-License-Identifier: MIT
// Hash Algorithm Comparison Benchmark
// Compile with -DMINHASH_USE_FNV1A to use FNV-1a instead of MurmurHash3

#include <iostream>
#include <string>
#include <vector>
#include <iomanip>
#include <chrono>
#include <random>
#include <cstring>
#include "minhash.hpp"
#include "hash_function.hpp"

// Generate random data for benchmarking
std::vector<std::vector<uint8_t>> generate_random_data(std::size_t count, std::size_t avg_size) {
    std::mt19937 rng(42);
    std::uniform_int_distribution<std::size_t> size_dist(avg_size / 2, avg_size * 2);
    std::uniform_int_distribution<uint8_t> byte_dist(0, 255);

    std::vector<std::vector<uint8_t>> data;
    data.reserve(count);

    for (std::size_t i = 0; i < count; ++i) {
        std::size_t size = size_dist(rng);
        std::vector<uint8_t> item(size);
        for (auto& byte : item) {
            byte = byte_dist(rng);
        }
        data.push_back(std::move(item));
    }

    return data;
}

// Benchmark raw hash performance
void benchmark_raw_hash(const std::vector<std::vector<uint8_t>>& data) {
    std::cout << "=== Raw Hash Performance ===\n";

    const int iterations = 3;
    uint64_t total_bytes = 0;
    for (const auto& item : data) {
        total_bytes += item.size();
    }

    // Warm-up
    for (const auto& item : data) {
        uint32_t hash;
        minhash::hash::hash_x86_32(item.data(), item.size(), 42, &hash);
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int iter = 0; iter < iterations; ++iter) {
        for (const auto& item : data) {
            uint32_t hash;
            minhash::hash::hash_x86_32(item.data(), item.size(), 42, &hash);
        }
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double total_mb = (total_bytes * iterations) / (1024.0 * 1024.0);
    double seconds = duration.count() / 1000000.0;
    double throughput = total_mb / seconds;

    std::cout << "Algorithm: " << minhash::hash::get_algorithm_name() << "\n";
    std::cout << "Items hashed: " << data.size() << " x " << iterations << " iterations\n";
    std::cout << "Total data: " << std::fixed << std::setprecision(2) << total_mb << " MB\n";
    std::cout << "Time: " << duration.count() << " μs\n";
    std::cout << "Throughput: " << throughput << " MB/s\n";
    std::cout << "Per-item: " << (duration.count() / (data.size() * iterations)) << " μs\n\n";
}

// Benchmark MinHash signature generation
void benchmark_minhash(const std::vector<std::vector<uint8_t>>& data) {
    std::cout << "=== MinHash Signature Generation ===\n";

    const int iterations = 3;

    // Convert to string vectors for MinHash
    std::vector<std::vector<std::string>> string_data;
    for (const auto& item : data) {
        std::vector<std::string> ngrams;
        // Generate 4-grams
        if (item.size() >= 4) {
            for (std::size_t i = 0; i <= item.size() - 4; ++i) {
                ngrams.push_back(std::string(reinterpret_cast<const char*>(&item[i]), 4));
            }
        }
        string_data.push_back(std::move(ngrams));
    }

    // Warm-up
    minhash::MinHash64 mh_warmup(42);
    for (const auto& ngrams : string_data) {
        mh_warmup.reset();
        mh_warmup.update_all(ngrams);
    }

    // Benchmark
    auto start = std::chrono::high_resolution_clock::now();
    for (int iter = 0; iter < iterations; ++iter) {
        for (const auto& ngrams : string_data) {
            minhash::MinHash64 mh(42);
            mh.update_all(ngrams);
        }
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Algorithm: " << minhash::hash::get_algorithm_name() << "\n";
    std::cout << "Documents: " << string_data.size() << " x " << iterations << " iterations\n";
    std::cout << "Hash functions: 128\n";
    std::cout << "Time: " << duration.count() << " ms\n";
    std::cout << "Per-document: " << std::fixed << std::setprecision(3)
              << (duration.count() / (double)(string_data.size() * iterations)) << " ms\n\n";
}

// Test hash distribution quality
void test_hash_distribution() {
    std::cout << "=== Hash Distribution Quality ===\n";

    const int num_buckets = 256;
    const int num_items = 100000;

    std::vector<int> buckets(num_buckets, 0);

    // Hash sequential integers
    for (int i = 0; i < num_items; ++i) {
        uint32_t hash;
        minhash::hash::hash_x86_32(&i, sizeof(i), 42, &hash);
        buckets[hash % num_buckets]++;
    }

    // Calculate statistics
    double mean = num_items / (double)num_buckets;
    double variance = 0.0;
    int min_count = buckets[0];
    int max_count = buckets[0];

    for (int count : buckets) {
        variance += (count - mean) * (count - mean);
        min_count = std::min(min_count, count);
        max_count = std::max(max_count, count);
    }
    variance /= num_buckets;
    double stddev = std::sqrt(variance);

    std::cout << "Algorithm: " << minhash::hash::get_algorithm_name() << "\n";
    std::cout << "Items: " << num_items << "\n";
    std::cout << "Buckets: " << num_buckets << "\n";
    std::cout << "Expected per bucket: " << std::fixed << std::setprecision(2) << mean << "\n";
    std::cout << "Min count: " << min_count << " (" << (min_count / mean * 100) << "%)\n";
    std::cout << "Max count: " << max_count << " (" << (max_count / mean * 100) << "%)\n";
    std::cout << "Std dev: " << stddev << " (" << (stddev / mean * 100) << "%)\n";
    std::cout << "Chi-squared (lower is better): " << (variance * num_buckets / mean) << "\n\n";
}

// Compare Jaccard accuracy between hash algorithms
void test_jaccard_accuracy() {
    std::cout << "=== Jaccard Similarity Accuracy ===\n";

    // Create test sets with known similarity
    std::vector<std::string> set1, set2;

    // 70% overlap
    for (int i = 0; i < 700; ++i) {
        std::string item = "shared_" + std::to_string(i);
        set1.push_back(item);
        set2.push_back(item);
    }

    // 30% unique to each
    for (int i = 0; i < 300; ++i) {
        set1.push_back("unique1_" + std::to_string(i));
        set2.push_back("unique2_" + std::to_string(i));
    }

    // True Jaccard = 700 / (1000 + 1000 - 700) = 700 / 1300 ≈ 0.5385

    minhash::MinHash64 mh1(42);
    minhash::MinHash64 mh2(42);

    mh1.update_all(set1);
    mh2.update_all(set2);

    double estimated = mh1.jaccard(mh2);
    double true_jaccard = 700.0 / 1300.0;

    std::cout << "Algorithm: " << minhash::hash::get_algorithm_name() << "\n";
    std::cout << "Set sizes: " << set1.size() << " and " << set2.size() << "\n";
    std::cout << "Shared elements: 700\n";
    std::cout << "True Jaccard: " << std::fixed << std::setprecision(4) << true_jaccard << "\n";
    std::cout << "Estimated Jaccard: " << estimated << "\n";
    std::cout << "Error: " << std::abs(estimated - true_jaccard) << "\n";
    std::cout << "Relative error: " << (std::abs(estimated - true_jaccard) / true_jaccard * 100) << "%\n\n";
}

int main() {
    std::cout << "Hash Algorithm Benchmark\n";
    std::cout << "========================\n\n";

    // Display active configuration
    std::cout << "=== Configuration ===\n";
    std::cout << "Hash Algorithm: " << minhash::hash::get_algorithm_name() << "\n";
    std::cout << "Algorithm Info: " << minhash::hash::get_algorithm_info() << "\n";

    auto info = minhash::hash::get_hash_info();
    std::cout << "Description: " << info.description << "\n";
    std::cout << "Quality Score: " << info.quality_score << "/10\n";
    std::cout << "Batch SIMD: " << (info.has_simd_batch ? "Yes" : "No") << "\n";
    std::cout << "\n";

#ifdef MINHASH_USE_FNV1A
    std::cout << "Note: Using FNV-1a (compiled with -DMINHASH_USE_FNV1A)\n";
#else
    std::cout << "Note: Using MurmurHash3 (default)\n";
    std::cout << "      Compile with -DMINHASH_USE_FNV1A to use FNV-1a instead\n";
#endif
    std::cout << "\n";

    // Generate test data
    std::cout << "Generating test data...\n";
    auto data = generate_random_data(10000, 100);  // 10K items, ~100 bytes each
    std::cout << "Generated " << data.size() << " items\n\n";

    // Run benchmarks
    benchmark_raw_hash(data);
    benchmark_minhash(data);
    test_hash_distribution();
    test_jaccard_accuracy();

    std::cout << "=== Summary ===\n";
    std::cout << "Hash algorithm: " << minhash::hash::get_algorithm_name() << "\n";
#ifdef MINHASH_USE_FNV1A
    std::cout << "FNV-1a is generally faster but has slightly lower quality than MurmurHash3.\n";
    std::cout << "Best for: High-throughput scenarios where speed > quality.\n";
#else
    std::cout << "MurmurHash3 has excellent distribution and is the recommended default.\n";
    std::cout << "Best for: General-purpose use where quality matters.\n";
#endif
    std::cout << "\n";

    return 0;
}
