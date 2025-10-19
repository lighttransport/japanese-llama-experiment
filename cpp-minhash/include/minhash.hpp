// SPDX-License-Identifier: MIT
// Copyright (c) 2024 MinHash C++20 Implementation

#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <cstdint>
#include <limits>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "hash_function.hpp"
#include "simd_jaccard.hpp"

namespace minhash {

// Concept for hashable types
template <typename T>
concept Hashable = requires(T a) {
    { std::data(a) } -> std::convertible_to<const void*>;
    { std::size(a) } -> std::convertible_to<std::size_t>;
};

// Hash value types
using Hash32 = uint32_t;
using Hash64 = uint64_t;
using Hash128 = std::array<uint64_t, 2>;

/**
 * @brief MinHash signature generator
 *
 * Implements the MinHash algorithm for estimating Jaccard similarity between sets.
 * Uses MurmurHash3 with different seeds to simulate independent hash functions.
 *
 * The algorithm:
 * 1. Generate k hash functions using different seeds
 * 2. For each element in the set, compute k hash values
 * 3. Keep the minimum hash value for each hash function
 * 4. The result is a k-length signature that approximates the set
 *
 * Two sets with high Jaccard similarity will have similar signatures.
 *
 * @tparam HashType The hash value type (Hash32, Hash64, or Hash128)
 * @tparam NumHashes Number of hash functions (permutations)
 */
template <typename HashType = Hash64, std::size_t NumHashes = 128>
class MinHash {
public:
    static constexpr std::size_t num_hashes = NumHashes;
    using hash_type = HashType;
    using signature_type = std::array<HashType, NumHashes>;

    /**
     * @brief Construct a MinHash generator
     * @param seed Base seed for hash function generation
     */
    explicit MinHash(uint32_t seed = 1) : seed_(seed) {
        reset();
    }

    /**
     * @brief Reset the signature to initial state
     */
    void reset() {
        if constexpr (std::is_same_v<HashType, Hash32>) {
            signature_.fill(std::numeric_limits<Hash32>::max());
        } else if constexpr (std::is_same_v<HashType, Hash64>) {
            signature_.fill(std::numeric_limits<Hash64>::max());
        } else if constexpr (std::is_same_v<HashType, Hash128>) {
            signature_.fill(Hash128{std::numeric_limits<uint64_t>::max(),
                                    std::numeric_limits<uint64_t>::max()});
        }
    }

    /**
     * @brief Update signature with a single element
     * @param data Pointer to element data
     * @param len Length of element data in bytes
     */
    void update(const void* data, std::size_t len) {
        for (std::size_t i = 0; i < NumHashes; ++i) {
            const uint32_t hash_seed = seed_ + static_cast<uint32_t>(i);
            const HashType hash_val = compute_hash(data, len, hash_seed);
            signature_[i] = min_hash(signature_[i], hash_val);
        }
    }

    /**
     * @brief Update signature with a hashable element
     * @param element The element to hash
     */
    template <Hashable T>
    void update(const T& element) {
        const void* data = std::data(element);
        const std::size_t len = std::size(element) * sizeof(typename T::value_type);
        update(data, len);
    }

    /**
     * @brief Update signature with a string
     * @param str The string to hash
     */
    void update(std::string_view str) {
        update(str.data(), str.size());
    }

    /**
     * @brief Update signature with multiple elements
     * @param elements Range of elements to hash
     */
    template <std::ranges::range R>
    requires Hashable<std::ranges::range_value_t<R>>
    void update_all(const R& elements) {
        for (const auto& element : elements) {
            update(element);
        }
    }

    /**
     * @brief Get the current signature
     * @return The MinHash signature
     */
    [[nodiscard]] const signature_type& signature() const noexcept {
        return signature_;
    }

    /**
     * @brief Estimate Jaccard similarity with another MinHash
     * @param other The other MinHash to compare with
     * @return Estimated Jaccard similarity [0.0, 1.0]
     *
     * Uses SIMD optimizations when available (AVX2, SSE2, NEON)
     */
    [[nodiscard]] double jaccard(const MinHash& other) const {
        if (seed_ != other.seed_) {
            throw std::invalid_argument("Cannot compare MinHash with different seeds");
        }

        std::size_t matches = 0;

        // Use SIMD for 32-bit and 64-bit hashes
        if constexpr (std::is_same_v<HashType, Hash32> || std::is_same_v<HashType, Hash64>) {
            matches = simd::count_matches(signature_.data(), other.signature_.data(), NumHashes);
        } else if constexpr (std::is_same_v<HashType, Hash128>) {
            // Hash128 needs special handling (compare array of two uint64_t)
            for (std::size_t i = 0; i < NumHashes; ++i) {
                if (equal_hash(signature_[i], other.signature_[i])) {
                    ++matches;
                }
            }
        }

        return simd::jaccard_from_matches(matches, NumHashes);
    }

    /**
     * @brief Merge another MinHash into this one (union operation)
     * @param other The other MinHash to merge
     */
    void merge(const MinHash& other) {
        if (seed_ != other.seed_) {
            throw std::invalid_argument("Cannot merge MinHash with different seeds");
        }

        for (std::size_t i = 0; i < NumHashes; ++i) {
            signature_[i] = min_hash(signature_[i], other.signature_[i]);
        }
    }

    /**
     * @brief Get the seed used for hash generation
     * @return The seed value
     */
    [[nodiscard]] uint32_t seed() const noexcept {
        return seed_;
    }

private:
    uint32_t seed_;
    signature_type signature_;

    // Compute hash based on HashType
    [[nodiscard]] static HashType compute_hash(const void* data, std::size_t len, uint32_t seed) {
        if constexpr (std::is_same_v<HashType, Hash32>) {
            Hash32 result;
            hash::hash_x86_32(data, len, seed, &result);
            return result;
        } else if constexpr (std::is_same_v<HashType, Hash64>) {
            Hash128 temp;
            hash::hash_x64_128(data, len, seed, temp.data());
            return temp[0]; // Use lower 64 bits
        } else if constexpr (std::is_same_v<HashType, Hash128>) {
            Hash128 result;
            hash::hash_x64_128(data, len, seed, result.data());
            return result;
        }
    }

    // Compare hash values
    [[nodiscard]] static bool equal_hash(const HashType& a, const HashType& b) noexcept {
        if constexpr (std::is_same_v<HashType, Hash128>) {
            return a[0] == b[0] && a[1] == b[1];
        } else {
            return a == b;
        }
    }

    // Return minimum hash value
    [[nodiscard]] static HashType min_hash(const HashType& a, const HashType& b) noexcept {
        if constexpr (std::is_same_v<HashType, Hash128>) {
            // Compare 128-bit hashes
            if (a[0] < b[0]) return a;
            if (a[0] > b[0]) return b;
            if (a[1] < b[1]) return a;
            return b;
        } else {
            return std::min(a, b);
        }
    }
};

/**
 * @brief LSH (Locality-Sensitive Hashing) band for duplicate detection
 *
 * Divides a MinHash signature into bands of rows. Documents that match
 * in at least one band are candidate duplicates.
 *
 * For a signature of k hashes divided into b bands of r rows each (k = b * r),
 * two documents with Jaccard similarity s have probability of sharing at least
 * one band: 1 - (1 - s^r)^b
 *
 * @tparam HashType The hash value type
 * @tparam NumHashes Total number of hashes
 * @tparam NumBands Number of bands
 */
template <typename HashType = Hash64, std::size_t NumHashes = 128, std::size_t NumBands = 16>
class LSHBands {
public:
    static constexpr std::size_t num_hashes = NumHashes;
    static constexpr std::size_t num_bands = NumBands;
    static constexpr std::size_t rows_per_band = NumHashes / NumBands;

    static_assert(NumHashes % NumBands == 0,
                  "Number of hashes must be divisible by number of bands");

    using signature_type = std::array<HashType, NumHashes>;
    using band_hash_type = uint64_t;
    using bands_type = std::array<band_hash_type, NumBands>;

    /**
     * @brief Convert a MinHash signature to LSH bands
     * @param signature The MinHash signature
     * @return Array of band hashes
     */
    [[nodiscard]] static bands_type compute_bands(const signature_type& signature) {
        bands_type bands;

        for (std::size_t band_idx = 0; band_idx < NumBands; ++band_idx) {
            // Hash the rows in this band together
            band_hash_type band_hash = 0;

            for (std::size_t row = 0; row < rows_per_band; ++row) {
                const std::size_t sig_idx = band_idx * rows_per_band + row;
                const HashType& hash_val = signature[sig_idx];

                // Combine hash values for this band
                if constexpr (std::is_same_v<HashType, Hash32>) {
                    band_hash ^= static_cast<uint64_t>(hash_val) + 0x9e3779b97f4a7c15ULL +
                                (band_hash << 6) + (band_hash >> 2);
                } else if constexpr (std::is_same_v<HashType, Hash64>) {
                    band_hash ^= hash_val + 0x9e3779b97f4a7c15ULL +
                                (band_hash << 6) + (band_hash >> 2);
                } else if constexpr (std::is_same_v<HashType, Hash128>) {
                    band_hash ^= hash_val[0] + 0x9e3779b97f4a7c15ULL +
                                (band_hash << 6) + (band_hash >> 2);
                    band_hash ^= hash_val[1] + 0x9e3779b97f4a7c15ULL +
                                (band_hash << 6) + (band_hash >> 2);
                }
            }

            bands[band_idx] = band_hash;
        }

        return bands;
    }

    /**
     * @brief Check if two signatures share any band
     * @param bands1 First set of bands
     * @param bands2 Second set of bands
     * @return True if any band matches
     */
    [[nodiscard]] static bool has_matching_band(const bands_type& bands1,
                                                  const bands_type& bands2) noexcept {
        for (std::size_t i = 0; i < NumBands; ++i) {
            if (bands1[i] == bands2[i]) {
                return true;
            }
        }
        return false;
    }
};

// Common type aliases
using MinHash32 = MinHash<Hash32, 128>;
using MinHash64 = MinHash<Hash64, 128>;
using MinHash128 = MinHash<Hash128, 128>;

using LSH64 = LSHBands<Hash64, 128, 16>;
using LSH128 = LSHBands<Hash128, 128, 16>;

} // namespace minhash
