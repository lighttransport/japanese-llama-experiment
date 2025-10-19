// SPDX-License-Identifier: MIT
// SAIS Suffix Array Example and Test

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <random>
#include "sais.hpp"

// Naive suffix array construction for verification
std::vector<uint32_t> naive_suffix_array(const std::string& text) {
    uint32_t n = static_cast<uint32_t>(text.size());
    std::vector<uint32_t> SA(n);

    for (uint32_t i = 0; i < n; ++i) {
        SA[i] = i;
    }

    std::sort(SA.begin(), SA.end(), [&](uint32_t a, uint32_t b) {
        return text.substr(a) < text.substr(b);
    });

    return SA;
}

// Verify suffix array is correct
bool verify_suffix_array(const std::string& text, const std::vector<uint32_t>& SA) {
    uint32_t n = static_cast<uint32_t>(text.size());

    if (SA.size() != n) {
        std::cerr << "SA size mismatch\n";
        return false;
    }

    // Check all indices are present
    std::vector<bool> present(n, false);
    for (uint32_t i = 0; i < n; ++i) {
        if (SA[i] >= n) {
            std::cerr << "Invalid index: " << SA[i] << "\n";
            return false;
        }
        present[SA[i]] = true;
    }

    for (uint32_t i = 0; i < n; ++i) {
        if (!present[i]) {
            std::cerr << "Missing index: " << i << "\n";
            return false;
        }
    }

    // Check ordering
    for (uint32_t i = 1; i < n; ++i) {
        std::string suffix1 = text.substr(SA[i - 1]);
        std::string suffix2 = text.substr(SA[i]);

        if (suffix1 > suffix2) {
            std::cerr << "Order violation at " << i << "\n";
            std::cerr << "  SA[" << (i-1) << "] = " << SA[i-1] << " -> \"" << suffix1.substr(0, 10) << "...\"\n";
            std::cerr << "  SA[" << i << "] = " << SA[i] << " -> \"" << suffix2.substr(0, 10) << "...\"\n";
            return false;
        }
    }

    return true;
}

// Print suffix array (first few entries)
void print_suffix_array(const std::string& text, const std::vector<uint32_t>& SA, int max_print = 20) {
    std::cout << "Suffix Array (first " << std::min(max_print, (int)SA.size()) << " entries):\n";
    std::cout << std::setw(6) << "i" << std::setw(10) << "SA[i]" << "  Suffix\n";
    std::cout << std::string(50, '-') << "\n";

    for (int i = 0; i < std::min(max_print, (int)SA.size()); ++i) {
        std::string suffix = text.substr(SA[i]);
        if (suffix.length() > 30) {
            suffix = suffix.substr(0, 30) + "...";
        }
        std::cout << std::setw(6) << i
                  << std::setw(10) << SA[i]
                  << "  \"" << suffix << "\"\n";
    }

    if (SA.size() > (size_t)max_print) {
        std::cout << "... (" << (SA.size() - max_print) << " more entries)\n";
    }
    std::cout << "\n";
}

// Generate random string
std::string generate_random_string(size_t length, int alphabet_size = 4) {
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist('a', 'a' + alphabet_size - 1);

    std::string result;
    result.reserve(length);

    for (size_t i = 0; i < length; ++i) {
        result += static_cast<char>(dist(rng));
    }

    return result;
}

// Benchmark suffix array construction
void benchmark_sais(size_t length, int alphabet_size) {
    std::cout << "=== Benchmark: n=" << length << ", alphabet=" << alphabet_size << " ===\n";

    auto text = generate_random_string(length, alphabet_size);

    // Benchmark SAIS
    auto start = std::chrono::high_resolution_clock::now();
    auto SA = sais::build_suffix_array(text);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "SAIS construction time: " << duration.count() << " ms\n";
    std::cout << "Throughput: " << (length / 1000.0 / (duration.count() / 1000.0)) << " K chars/sec\n";

    // Verify
    bool valid = verify_suffix_array(text, SA);
    std::cout << "Verification: " << (valid ? "PASSED" : "FAILED") << "\n\n";

    if (!valid) {
        std::cerr << "ERROR: Suffix array is invalid!\n";
    }
}

int main() {
    std::cout << "SAIS Suffix Array Implementation\n";
    std::cout << "=================================\n\n";

    // Display SIMD capabilities
    std::cout << "SIMD Instruction Set: " << sais::get_simd_name() << "\n";
    std::cout << "Memory Efficiency: Full 32-bit range (4GB) vs libsais 2GB\n\n";

    // Example 1: Simple string
    {
        std::cout << "=== Example 1: Simple String ===\n";
        std::string text = "banana";
        std::cout << "Text: \"" << text << "\"\n\n";

        auto SA = sais::build_suffix_array(text);

        print_suffix_array(text, SA);

        // Verify
        bool valid = verify_suffix_array(text, SA);
        std::cout << "Verification: " << (valid ? "PASSED" : "FAILED") << "\n\n";
    }

    // Example 2: Repeated characters
    {
        std::cout << "=== Example 2: Repeated Characters ===\n";
        std::string text = "mississippi";
        std::cout << "Text: \"" << text << "\"\n\n";

        auto SA = sais::build_suffix_array(text);

        print_suffix_array(text, SA);

        bool valid = verify_suffix_array(text, SA);
        std::cout << "Verification: " << (valid ? "PASSED" : "FAILED") << "\n\n";
    }

    // Example 3: Compare with naive algorithm
    {
        std::cout << "=== Example 3: Comparison with Naive Algorithm ===\n";
        std::string text = "abracadabra";
        std::cout << "Text: \"" << text << "\"\n\n";

        auto SA_sais = sais::build_suffix_array(text);
        auto SA_naive = naive_suffix_array(text);

        std::cout << "SAIS result:\n";
        print_suffix_array(text, SA_sais, 15);

        std::cout << "Naive result:\n";
        print_suffix_array(text, SA_naive, 15);

        bool match = (SA_sais == SA_naive);
        std::cout << "Results match: " << (match ? "YES" : "NO") << "\n\n";
    }

    // Example 4: Memory efficiency demonstration
    {
        std::cout << "=== Example 4: Memory Efficiency ===\n";
        std::cout << "Using UINT32_MAX as sentinel allows full 32-bit range:\n";
        std::cout << "  Maximum text length: " << (UINT32_MAX - 1) << " bytes (~4GB)\n";
        std::cout << "  libsais (MSB bit): " << (INT32_MAX) << " bytes (~2GB)\n";
        std::cout << "  Advantage: " << ((UINT32_MAX - 1.0) / INT32_MAX) << "x range\n\n";
    }

    // Performance benchmarks
    std::cout << "=== Performance Benchmarks ===\n\n";

    benchmark_sais(10000, 4);      // 10K, small alphabet
    benchmark_sais(100000, 4);     // 100K, small alphabet
    benchmark_sais(1000000, 4);    // 1M, small alphabet

    benchmark_sais(10000, 26);     // 10K, larger alphabet
    benchmark_sais(100000, 26);    // 100K, larger alphabet
    benchmark_sais(1000000, 26);   // 1M, larger alphabet

    // Example 5: Finding patterns using suffix array
    {
        std::cout << "=== Example 5: Pattern Matching with Suffix Array ===\n";
        std::string text = "the quick brown fox jumps over the lazy dog";
        std::string pattern = "the";

        std::cout << "Text: \"" << text << "\"\n";
        std::cout << "Pattern: \"" << pattern << "\"\n\n";

        auto SA = sais::build_suffix_array(text);

        // Binary search for pattern
        std::vector<uint32_t> matches;

        auto lower = std::lower_bound(SA.begin(), SA.end(), 0, [&](uint32_t sa_pos, int) {
            return text.substr(sa_pos).compare(0, pattern.size(), pattern) < 0;
        });

        auto upper = std::upper_bound(SA.begin(), SA.end(), 0, [&](int, uint32_t sa_pos) {
            return pattern.compare(text.substr(sa_pos, pattern.size())) < 0;
        });

        std::cout << "Found " << (upper - lower) << " occurrences:\n";
        for (auto it = lower; it != upper; ++it) {
            std::cout << "  Position " << *it << ": \"" << text.substr(*it, 20) << "...\"\n";
        }
        std::cout << "\n";
    }

    // Example 6: LCP array could be built from SA (future work)
    {
        std::cout << "=== Example 6: Future Work ===\n";
        std::cout << "The suffix array can be used to build:\n";
        std::cout << "  - LCP (Longest Common Prefix) array\n";
        std::cout << "  - Burrows-Wheeler Transform (BWT)\n";
        std::cout << "  - FM-index for pattern matching\n";
        std::cout << "  - Compressed suffix trees\n\n";
    }

    std::cout << "All examples completed successfully!\n";

    return 0;
}
