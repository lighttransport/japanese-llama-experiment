// SPDX-License-Identifier: MIT
// Copyright 2025 - Present Light Transport Entertainment Inc.
// Hash function abstraction layer - switchable between MurmurHash3 and FNV-1a

#pragma once

#include <cstddef>
#include <cstdint>

// Select hash algorithm based on compile-time flag
// Default: MurmurHash3
// Define MINHASH_USE_FNV1A to use FNV-1a instead
#ifdef MINHASH_USE_FNV1A
    #include "fnv1a.hpp"
    #define MINHASH_HASH_ALGORITHM "FNV-1a"
#else
    #include "murmurhash3.hpp"
    #define MINHASH_HASH_ALGORITHM "MurmurHash3"
#endif

namespace minhash {
namespace hash {

/**
 * @brief Unified 32-bit hash interface
 *
 * Compiles to either MurmurHash3 or FNV-1a based on MINHASH_USE_FNV1A define
 */
inline void hash_x86_32(const void* key, std::size_t len, uint32_t seed, void* out) {
#ifdef MINHASH_USE_FNV1A
    fnv1a::hash_32(key, len, seed, out);
#else
    murmurhash3::hash_x86_32(key, len, seed, out);
#endif
}

/**
 * @brief Unified 128-bit hash interface (x86 version)
 */
inline void hash_x86_128(const void* key, std::size_t len, uint32_t seed, void* out) {
#ifdef MINHASH_USE_FNV1A
    fnv1a::hash_128(key, len, seed, out);
#else
    murmurhash3::hash_x86_128(key, len, seed, out);
#endif
}

/**
 * @brief Unified 128-bit hash interface (x64 version)
 */
inline void hash_x64_128(const void* key, std::size_t len, uint32_t seed, void* out) {
#ifdef MINHASH_USE_FNV1A
    fnv1a::hash_128(key, len, seed, out);
#else
    murmurhash3::hash_x64_128(key, len, seed, out);
#endif
}

/**
 * @brief Get the name of the active hash algorithm
 */
inline constexpr const char* get_algorithm_name() {
    return MINHASH_HASH_ALGORITHM;
}

/**
 * @brief Get hash algorithm information string
 */
inline const char* get_algorithm_info() {
#ifdef MINHASH_USE_FNV1A
    static char info[128];
    snprintf(info, sizeof(info), "FNV-1a (SIMD: %s)", fnv1a::get_simd_support());
    return info;
#else
    return "MurmurHash3";
#endif
}

/**
 * @brief Hash quality information
 *
 * MurmurHash3: Excellent avalanche, good distribution, good for all purposes
 * FNV-1a: Fast, simple, good for hash tables, lower quality than MurmurHash3
 */
struct HashInfo {
    const char* name;
    const char* description;
    bool has_simd_batch;  // Can process multiple items in parallel
    int quality_score;    // 1-10, higher is better
};

inline HashInfo get_hash_info() {
#ifdef MINHASH_USE_FNV1A
    return HashInfo{
        "FNV-1a",
        "Fast, simple hash. Good for hash tables. SIMD batch processing available.",
        true,  // FNV-1a has batch SIMD
        7      // Good quality but not as good as MurmurHash3
    };
#else
    return HashInfo{
        "MurmurHash3",
        "High-quality hash with excellent avalanche. Industry standard.",
        false, // MurmurHash3 doesn't have batch SIMD
        9      // Excellent quality
    };
#endif
}

/**
 * @brief Compile-time hash algorithm selection check
 */
inline constexpr bool is_fnv1a() {
#ifdef MINHASH_USE_FNV1A
    return true;
#else
    return false;
#endif
}

inline constexpr bool is_murmurhash3() {
#ifdef MINHASH_USE_FNV1A
    return false;
#else
    return true;
#endif
}

} // namespace hash
} // namespace minhash
