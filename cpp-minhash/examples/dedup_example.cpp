// SPDX-License-Identifier: MIT
// Copyright 2025 - Present Light Transport Entertainment Inc.
// Deduplication Example and Benchmark

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <random>
#include <chrono>
#include "deduplicator.hpp"

// Generate random text
std::string generate_random_text(size_t length, std::mt19937& rng) {
    static const char chars[] = "abcdefghijklmnopqrstuvwxyz ";
    std::uniform_int_distribution<int> dist(0, sizeof(chars) - 2);

    std::string result;
    result.reserve(length);

    for (size_t i = 0; i < length; ++i) {
        result += chars[dist(rng)];
    }

    return result;
}

// Create a slightly modified version of text
std::string create_variant(const std::string& text, double modification_rate, std::mt19937& rng) {
    std::string result = text;
    std::uniform_real_distribution<double> prob(0.0, 1.0);
    std::uniform_int_distribution<int> char_dist('a', 'z');

    for (size_t i = 0; i < result.size(); ++i) {
        if (prob(rng) < modification_rate) {
            result[i] = static_cast<char>(char_dist(rng));
        }
    }

    return result;
}

// Print duplicate clusters
void print_clusters(const std::vector<std::vector<uint64_t>>& clusters,
                   const std::vector<std::string>& texts,
                   int max_clusters = 5) {
    std::cout << "\nDuplicate Clusters:\n";
    std::cout << std::string(60, '-') << "\n";

    int shown = 0;
    for (const auto& cluster : clusters) {
        if (cluster.size() <= 1) continue;
        if (shown >= max_clusters) {
            std::cout << "... (" << (clusters.size() - shown) << " more clusters)\n";
            break;
        }

        std::cout << "Cluster " << (shown + 1) << " (" << cluster.size() << " documents):\n";
        for (size_t i = 0; i < std::min(size_t(3), cluster.size()); ++i) {
            uint64_t doc_id = cluster[i];
            std::string preview = texts[doc_id];
            if (preview.length() > 60) {
                preview = preview.substr(0, 60) + "...";
            }
            std::cout << "  Doc " << doc_id << ": \"" << preview << "\"\n";
        }
        if (cluster.size() > 3) {
            std::cout << "  ... (" << (cluster.size() - 3) << " more)\n";
        }
        std::cout << "\n";
        ++shown;
    }
}

// Print statistics
void print_stats(const minhash::DeduplicationStats& stats) {
    std::cout << "\nDeduplication Statistics:\n";
    std::cout << std::string(60, '-') << "\n";
    std::cout << std::setw(30) << "Total documents: " << stats.total_documents << "\n";
    std::cout << std::setw(30) << "Unique documents: " << stats.unique_documents << "\n";
    std::cout << std::setw(30) << "Duplicate documents: " << stats.duplicate_documents << "\n";
    std::cout << std::setw(30) << "Duplicate clusters: " << stats.clusters << "\n";
    std::cout << std::setw(30) << "Candidate pairs (LSH): " << stats.candidate_pairs << "\n";
    std::cout << std::setw(30) << "Verified duplicates: " << stats.verified_duplicates << "\n";
    std::cout << std::setw(30) << "False positive rate: "
              << std::fixed << std::setprecision(2)
              << (stats.false_positive_rate() * 100) << "%\n";
    std::cout << std::setw(30) << "Deduplication ratio: "
              << std::fixed << std::setprecision(2)
              << (stats.deduplication_ratio() * 100) << "%\n";
}

// Example 1: Basic deduplication
void example_basic() {
    std::cout << "=== Example 1: Basic Deduplication ===\n\n";

    minhash::Deduplicator64 dedup(42);

    std::vector<std::string> docs = {
        "The quick brown fox jumps over the lazy dog",
        "The quick brown fox jumps over the lazy cat",  // Similar to doc 0
        "The quick brown fox jumps over the lazy dog",  // Exact duplicate of doc 0
        "Hello world this is a completely different document",
        "Hello world this is a completely different document",  // Duplicate of doc 3
        "The fast brown fox leaps over the sleepy dog",  // Somewhat similar to doc 0
    };

    // Add documents with character-level 3-grams
    for (size_t i = 0; i < docs.size(); ++i) {
        auto ngrams = minhash::generate_ngrams(docs[i], 3);
        dedup.add_document(i, ngrams);
    }

    std::cout << "Added " << docs.size() << " documents\n";
    std::cout << "LSH parameters: 16 bands × 8 rows (128 hashes)\n";
    std::cout << "Optimal threshold: " << std::fixed << std::setprecision(3)
              << minhash::Deduplicator64::optimal_threshold() << "\n";

    // Find duplicates at 80% similarity
    auto clusters = dedup.find_duplicates(0.8);

    print_clusters(clusters, docs, 10);
    print_stats(dedup.get_stats());
}

// Example 2: Word-level shingles
void example_word_shingles() {
    std::cout << "\n=== Example 2: Word-Level Shingles ===\n\n";

    minhash::Deduplicator64 dedup(42);

    std::vector<std::string> docs = {
        "machine learning is a subset of artificial intelligence",
        "artificial intelligence includes machine learning as a subset",
        "deep learning is a type of machine learning algorithm",
        "neural networks are used in deep learning systems",
        "machine learning is a subset of artificial intelligence",  // Duplicate
    };

    // Add documents with word-level 3-shingles
    for (size_t i = 0; i < docs.size(); ++i) {
        auto shingles = minhash::generate_word_shingles(docs[i], 3);
        dedup.add_document(i, shingles);

        std::cout << "Doc " << i << " shingles: " << shingles.size() << "\n";
    }

    auto clusters = dedup.find_duplicates(0.7);

    print_clusters(clusters, docs, 10);
    print_stats(dedup.get_stats());
}

// Example 3: Different similarity thresholds
void example_thresholds() {
    std::cout << "\n=== Example 3: Different Similarity Thresholds ===\n\n";

    std::vector<double> thresholds = {0.5, 0.6, 0.7, 0.8, 0.9};

    std::vector<std::string> docs = {
        "The quick brown fox",
        "The quick brown cat",
        "The fast brown fox",
        "A quick brown fox",
        "The quick brown fox",  // Exact duplicate
    };

    std::cout << "Testing thresholds: ";
    for (double t : thresholds) {
        std::cout << t << " ";
    }
    std::cout << "\n\n";

    std::cout << std::setw(12) << "Threshold"
              << std::setw(12) << "Clusters"
              << std::setw(12) << "Duplicates"
              << std::setw(12) << "FP Rate\n";
    std::cout << std::string(48, '-') << "\n";

    for (double threshold : thresholds) {
        minhash::Deduplicator64 dedup(42);

        for (size_t i = 0; i < docs.size(); ++i) {
            auto ngrams = minhash::generate_ngrams(docs[i], 3);
            dedup.add_document(i, ngrams);
        }

        auto clusters = dedup.find_duplicates(threshold);
        auto stats = dedup.get_stats();

        std::cout << std::fixed << std::setprecision(1)
                  << std::setw(12) << threshold
                  << std::setw(12) << stats.clusters
                  << std::setw(12) << stats.duplicate_documents
                  << std::setw(11) << (stats.false_positive_rate() * 100) << "%\n";
    }
}

// Example 4: Performance benchmark
void example_benchmark() {
    std::cout << "\n=== Example 4: Performance Benchmark ===\n\n";

    std::vector<size_t> doc_counts = {100, 1000, 10000};
    size_t doc_length = 500;
    double duplicate_rate = 0.3;  // 30% duplicates

    std::cout << "Document length: " << doc_length << " chars\n";
    std::cout << "Duplicate rate: " << (duplicate_rate * 100) << "%\n\n";

    std::cout << std::setw(12) << "Documents"
              << std::setw(15) << "Build Time"
              << std::setw(15) << "Dedup Time"
              << std::setw(12) << "Clusters"
              << std::setw(12) << "FP Rate\n";
    std::cout << std::string(66, '-') << "\n";

    for (size_t num_docs : doc_counts) {
        minhash::Deduplicator64 dedup(42);

        std::mt19937 rng(12345);
        std::vector<std::string> docs;

        // Generate documents with some duplicates
        size_t num_unique = static_cast<size_t>(num_docs * (1.0 - duplicate_rate));

        for (size_t i = 0; i < num_unique; ++i) {
            docs.push_back(generate_random_text(doc_length, rng));
        }

        // Add duplicates (with slight modifications)
        std::uniform_int_distribution<size_t> doc_dist(0, num_unique - 1);
        while (docs.size() < num_docs) {
            size_t orig_idx = doc_dist(rng);
            docs.push_back(create_variant(docs[orig_idx], 0.05, rng));
        }

        // Benchmark document addition
        auto start_build = std::chrono::high_resolution_clock::now();

        for (size_t i = 0; i < docs.size(); ++i) {
            auto ngrams = minhash::generate_ngrams(docs[i], 5);
            dedup.add_document(i, ngrams);
        }

        auto end_build = std::chrono::high_resolution_clock::now();

        // Benchmark deduplication
        auto start_dedup = std::chrono::high_resolution_clock::now();
        auto clusters = dedup.find_duplicates(0.8);
        auto end_dedup = std::chrono::high_resolution_clock::now();

        auto build_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_build - start_build).count();
        auto dedup_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_dedup - start_dedup).count();

        auto stats = dedup.get_stats();

        std::cout << std::setw(12) << num_docs
                  << std::setw(13) << build_ms << " ms"
                  << std::setw(13) << dedup_ms << " ms"
                  << std::setw(12) << stats.clusters
                  << std::setw(11) << std::fixed << std::setprecision(1)
                  << (stats.false_positive_rate() * 100) << "%\n";
    }
}

// Example 5: LSH probability curve
void example_probability_curve() {
    std::cout << "\n=== Example 5: LSH Probability Curve ===\n\n";

    using Dedup = minhash::Deduplicator64;

    std::cout << "LSH Parameters:\n";
    std::cout << "  Bands (b): 16\n";
    std::cout << "  Rows per band (r): 8\n";
    std::cout << "  Total hashes (k = b×r): 128\n";
    std::cout << "  Optimal threshold: " << std::fixed << std::setprecision(3)
              << minhash::Deduplicator64::optimal_threshold() << "\n\n";

    auto curve = Dedup::probability_curve(20);

    std::cout << "Probability of being candidate duplicate:\n";
    std::cout << std::setw(15) << "Similarity"
              << std::setw(18) << "P(candidate)\n";
    std::cout << std::string(33, '-') << "\n";

    for (const auto& [similarity, probability] : curve) {
        std::cout << std::setw(14) << std::fixed << std::setprecision(2)
                  << (similarity * 100) << "%"
                  << std::setw(17) << std::fixed << std::setprecision(3)
                  << (probability * 100) << "%\n";
    }

    std::cout << "\nFormula: P(candidate) = 1 - (1 - s^r)^b\n";
    std::cout << "where s is Jaccard similarity\n";
}

int main() {
    std::cout << "MinHash LSH Deduplication Examples\n";
    std::cout << "===================================\n\n";

    example_basic();
    example_word_shingles();
    example_thresholds();
    example_benchmark();
    example_probability_curve();

    std::cout << "\nAll examples completed successfully!\n";

    return 0;
}
