// SPDX-License-Identifier: MIT
// Copyright (c) 2024 cpp-minhash
// LSH-based Document Deduplication using MinHash
// Efficient near-duplicate detection with configurable similarity threshold

#pragma once

#include "minhash.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>
#include <optional>
#include <algorithm>
#include <functional>
#include <cmath>

namespace minhash {

/**
 * @brief Statistics for deduplication process
 */
struct DeduplicationStats {
    size_t total_documents = 0;
    size_t unique_documents = 0;
    size_t duplicate_documents = 0;
    size_t candidate_pairs = 0;
    size_t verified_duplicates = 0;
    size_t clusters = 0;

    double false_positive_rate() const {
        if (candidate_pairs == 0) return 0.0;
        return static_cast<double>(candidate_pairs - verified_duplicates) / candidate_pairs;
    }

    double deduplication_ratio() const {
        if (total_documents == 0) return 0.0;
        return static_cast<double>(duplicate_documents) / total_documents;
    }
};

/**
 * @brief LSH-based document deduplicator using MinHash
 *
 * Uses Locality-Sensitive Hashing (LSH) with b bands × r rows to efficiently
 * find near-duplicate documents. Documents that match in at least one band
 * are considered candidate duplicates and verified using Jaccard similarity.
 *
 * Algorithm:
 * 1. Compute MinHash signature for each document (k hash functions)
 * 2. Divide signature into b bands of r rows (k = b × r)
 * 3. Hash each band independently
 * 4. Documents matching in ≥1 band are candidate duplicates
 * 5. Verify candidates using actual Jaccard similarity
 *
 * Probability of matching in at least one band:
 *   P(match) = 1 - (1 - s^r)^b
 * where s is the Jaccard similarity
 *
 * Optimal threshold: t ≈ (1/b)^(1/r)
 * For b=16, r=8: t ≈ 0.62 (62% similarity)
 *
 * Example:
 *   Deduplicator<Hash64, 128> dedup(42);  // seed=42, 128 hashes
 *
 *   // Add documents
 *   dedup.add_document(0, "The quick brown fox");
 *   dedup.add_document(1, "The quick brown cat");
 *   dedup.add_document(2, "The quick brown fox");  // duplicate of 0
 *
 *   // Find duplicates
 *   auto clusters = dedup.find_duplicates(0.8);  // 80% similarity threshold
 *
 *   // Get statistics
 *   auto stats = dedup.get_stats();
 *   std::cout << "Duplicates: " << stats.duplicate_documents << "\n";
 */
template <typename HashType = Hash64,
          std::size_t NumHashes = 128,
          std::size_t NumBands = 16>
class Deduplicator {
public:
    using minhash_type = MinHash<HashType, NumHashes>;
    using signature_type = typename minhash_type::signature_type;
    using document_id = uint64_t;

    static_assert(NumHashes % NumBands == 0,
                  "NumHashes must be divisible by NumBands");

    static constexpr std::size_t RowsPerBand = NumHashes / NumBands;

    /**
     * @brief Construct deduplicator with given seed
     */
    explicit Deduplicator(uint32_t seed = 42)
        : seed_(seed), minhash_template_(seed) {}

    /**
     * @brief Add document with pre-computed MinHash signature
     */
    void add_document(document_id id, const signature_type& signature) {
        signatures_[id] = signature;

        // Compute LSH band hashes
        auto bands = compute_bands(signature);

        // Add to band hash tables
        for (size_t band_idx = 0; band_idx < NumBands; ++band_idx) {
            band_tables_[band_idx][bands[band_idx]].push_back(id);
        }

        ++stats_.total_documents;
    }

    /**
     * @brief Add document from text (creates n-grams and computes MinHash)
     */
    template <typename Range>
    void add_document(document_id id, const Range& elements) {
        minhash_type mh(seed_);
        mh.update_all(elements);
        add_document(id, mh.signature());
    }

    /**
     * @brief Add document from n-grams
     */
    void add_document(document_id id, const std::vector<std::string>& ngrams) {
        minhash_type mh(seed_);
        mh.update_all(ngrams);
        add_document(id, mh.signature());
    }

    /**
     * @brief Find duplicate clusters using LSH + verification
     * @param threshold Minimum Jaccard similarity to consider duplicates
     * @return Vector of duplicate clusters (each cluster is a vector of document IDs)
     */
    std::vector<std::vector<document_id>> find_duplicates(double threshold = 0.8) {
        // Reset stats
        stats_.candidate_pairs = 0;
        stats_.verified_duplicates = 0;

        // Find candidate pairs from LSH bands
        std::unordered_set<std::pair<document_id, document_id>, pair_hash> candidates;

        for (size_t band_idx = 0; band_idx < NumBands; ++band_idx) {
            for (const auto& [band_hash, doc_ids] : band_tables_[band_idx]) {
                // All documents with same band hash are candidates
                for (size_t i = 0; i < doc_ids.size(); ++i) {
                    for (size_t j = i + 1; j < doc_ids.size(); ++j) {
                        auto id1 = doc_ids[i];
                        auto id2 = doc_ids[j];
                        if (id1 > id2) std::swap(id1, id2);
                        candidates.insert({id1, id2});
                    }
                }
            }
        }

        stats_.candidate_pairs = candidates.size();

        // Verify candidates using actual Jaccard similarity
        std::unordered_map<document_id, document_id> parent;  // Union-Find

        std::function<document_id(document_id)> find = [&](document_id id) -> document_id {
            if (parent.find(id) == parent.end()) {
                parent[id] = id;
            }
            if (parent[id] != id) {
                parent[id] = find(parent[id]);
            }
            return parent[id];
        };

        auto unite = [&](document_id id1, document_id id2) {
            id1 = find(id1);
            id2 = find(id2);
            if (id1 != id2) {
                parent[id2] = id1;
            }
        };

        // Verify each candidate pair
        for (const auto& [id1, id2] : candidates) {
            double similarity = jaccard(signatures_.at(id1), signatures_.at(id2));

            if (similarity >= threshold) {
                unite(id1, id2);
                ++stats_.verified_duplicates;
            }
        }

        // Build clusters from Union-Find
        std::unordered_map<document_id, std::vector<document_id>> clusters_map;
        for (const auto& [id, _] : signatures_) {
            document_id root = find(id);
            clusters_map[root].push_back(id);
        }

        // Convert to vector and sort
        std::vector<std::vector<document_id>> clusters;
        for (auto& [root, cluster] : clusters_map) {
            std::sort(cluster.begin(), cluster.end());
            clusters.push_back(std::move(cluster));
        }

        // Sort clusters by size (largest first)
        std::sort(clusters.begin(), clusters.end(),
                  [](const auto& a, const auto& b) {
                      return a.size() > b.size();
                  });

        // Update stats
        stats_.clusters = 0;
        stats_.unique_documents = 0;
        stats_.duplicate_documents = 0;

        for (const auto& cluster : clusters) {
            if (cluster.size() > 1) {
                ++stats_.clusters;
                ++stats_.unique_documents;
                stats_.duplicate_documents += cluster.size() - 1;
            } else {
                ++stats_.unique_documents;
            }
        }

        return clusters;
    }

    /**
     * @brief Get deduplication statistics
     */
    const DeduplicationStats& get_stats() const {
        return stats_;
    }

    /**
     * @brief Get number of documents
     */
    size_t size() const {
        return signatures_.size();
    }

    /**
     * @brief Clear all data
     */
    void clear() {
        signatures_.clear();
        for (auto& table : band_tables_) {
            table.clear();
        }
        stats_ = DeduplicationStats{};
    }

    /**
     * @brief Get MinHash signature for document
     */
    std::optional<signature_type> get_signature(document_id id) const {
        auto it = signatures_.find(id);
        if (it != signatures_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /**
     * @brief Get expected probability curve for LSH parameters
     */
    static std::vector<std::pair<double, double>> probability_curve(int steps = 100) {
        std::vector<std::pair<double, double>> curve;

        for (int i = 0; i <= steps; ++i) {
            double s = static_cast<double>(i) / steps;
            double prob = 1.0 - std::pow(1.0 - std::pow(s, RowsPerBand), NumBands);
            curve.push_back({s, prob});
        }

        return curve;
    }

    /**
     * @brief Get optimal threshold for current LSH parameters
     */
    static double optimal_threshold() {
        return std::pow(1.0 / NumBands, 1.0 / RowsPerBand);
    }

private:
    struct pair_hash {
        size_t operator()(const std::pair<document_id, document_id>& p) const {
            return std::hash<document_id>{}(p.first) ^
                   (std::hash<document_id>{}(p.second) << 1);
        }
    };

    // Compute LSH band hashes
    std::array<uint64_t, NumBands> compute_bands(const signature_type& signature) const {
        std::array<uint64_t, NumBands> bands;

        for (size_t band_idx = 0; band_idx < NumBands; ++band_idx) {
            uint64_t band_hash = 0;
            size_t start = band_idx * RowsPerBand;

            // Hash the rows in this band
            for (size_t i = 0; i < RowsPerBand; ++i) {
                uint64_t val = 0;

                if constexpr (std::is_same_v<HashType, Hash32>) {
                    val = signature[start + i];
                } else if constexpr (std::is_same_v<HashType, Hash64>) {
                    val = signature[start + i];
                } else {  // Hash128
                    val = signature[start + i].low;
                }

                // Simple hash combination
                band_hash ^= val + 0x9e3779b9 + (band_hash << 6) + (band_hash >> 2);
            }

            bands[band_idx] = band_hash;
        }

        return bands;
    }

    // Compute Jaccard similarity from signatures
    double jaccard(const signature_type& sig1, const signature_type& sig2) const {
        size_t matches = 0;
        for (size_t i = 0; i < NumHashes; ++i) {
            if (sig1[i] == sig2[i]) {
                ++matches;
            }
        }
        return static_cast<double>(matches) / NumHashes;
    }

    uint32_t seed_;
    minhash_type minhash_template_;

    // Document signatures
    std::unordered_map<document_id, signature_type> signatures_;

    // LSH band hash tables: band_idx -> (band_hash -> [doc_ids])
    std::array<std::unordered_map<uint64_t, std::vector<document_id>>, NumBands> band_tables_;

    DeduplicationStats stats_;
};

// Convenience type aliases
using Deduplicator64 = Deduplicator<Hash64, 128, 16>;
using Deduplicator128 = Deduplicator<Hash128, 128, 16>;

/**
 * @brief Generate n-grams from text
 */
inline std::vector<std::string> generate_ngrams(const std::string& text, size_t n = 3) {
    std::vector<std::string> ngrams;

    if (text.size() < n) {
        if (!text.empty()) {
            ngrams.push_back(text);
        }
        return ngrams;
    }

    ngrams.reserve(text.size() - n + 1);
    for (size_t i = 0; i <= text.size() - n; ++i) {
        ngrams.push_back(text.substr(i, n));
    }

    return ngrams;
}

/**
 * @brief Generate word-level shingles from text
 */
inline std::vector<std::string> generate_word_shingles(const std::string& text, size_t n = 3) {
    std::vector<std::string> words;
    std::string word;

    for (char c : text) {
        if (std::isspace(c)) {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    if (words.size() < n) {
        return words.size() > 0 ? std::vector<std::string>{words} : std::vector<std::string>{};
    }

    std::vector<std::string> shingles;
    shingles.reserve(words.size() - n + 1);

    for (size_t i = 0; i <= words.size() - n; ++i) {
        std::string shingle;
        for (size_t j = 0; j < n; ++j) {
            if (j > 0) shingle += " ";
            shingle += words[i + j];
        }
        shingles.push_back(shingle);
    }

    return shingles;
}

} // namespace minhash
