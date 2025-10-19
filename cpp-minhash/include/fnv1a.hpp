// SPDX-License-Identifier: MIT
// Copyright (c) 2024 cpp-minhash
// FNV-1a Hash Algorithm with SIMD optimizations

#pragma once

#include <cstddef>
#include <cstdint>
#include <array>

// SIMD intrinsics detection
#if defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2) || defined(_M_X64)
    #define FNV1A_HAS_SSE2 1
    #include <emmintrin.h>
#endif

#if defined(__AVX2__)
    #define FNV1A_HAS_AVX2 1
    #include <immintrin.h>
#endif

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    #define FNV1A_HAS_NEON 1
    #include <arm_neon.h>
#endif

namespace fnv1a {

// FNV-1a constants
namespace detail {
    constexpr uint32_t FNV1A_32_OFFSET_BASIS = 2166136261u;
    constexpr uint32_t FNV1A_32_PRIME = 16777619u;

    constexpr uint64_t FNV1A_64_OFFSET_BASIS = 14695981039346656037ull;
    constexpr uint64_t FNV1A_64_PRIME = 1099511628211ull;
}

/**
 * @brief FNV-1a 32-bit hash (scalar version)
 * @param data Pointer to data
 * @param len Length in bytes
 * @param seed Seed value (optional, default uses FNV offset basis)
 * @param out Output hash value
 */
inline void hash_32(const void* data, std::size_t len, uint32_t seed, void* out) {
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    uint32_t hash = (seed == 0) ? detail::FNV1A_32_OFFSET_BASIS : seed;

    for (std::size_t i = 0; i < len; ++i) {
        hash ^= static_cast<uint32_t>(bytes[i]);
        hash *= detail::FNV1A_32_PRIME;
    }

    *static_cast<uint32_t*>(out) = hash;
}

/**
 * @brief FNV-1a 64-bit hash (scalar version)
 * @param data Pointer to data
 * @param len Length in bytes
 * @param seed Seed value (optional, default uses FNV offset basis)
 * @param out Output hash value
 */
inline void hash_64(const void* data, std::size_t len, uint32_t seed, void* out) {
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    uint64_t hash = (seed == 0) ? detail::FNV1A_64_OFFSET_BASIS :
                    (detail::FNV1A_64_OFFSET_BASIS ^ static_cast<uint64_t>(seed));

    for (std::size_t i = 0; i < len; ++i) {
        hash ^= static_cast<uint64_t>(bytes[i]);
        hash *= detail::FNV1A_64_PRIME;
    }

    *static_cast<uint64_t*>(out) = hash;
}

/**
 * @brief FNV-1a 128-bit hash (using two 64-bit hashes with different seeds)
 * @param data Pointer to data
 * @param len Length in bytes
 * @param seed Seed value
 * @param out Output hash value (16 bytes)
 */
inline void hash_128(const void* data, std::size_t len, uint32_t seed, void* out) {
    uint64_t* result = static_cast<uint64_t*>(out);

    // First 64 bits with seed
    hash_64(data, len, seed, &result[0]);

    // Second 64 bits with different seed
    hash_64(data, len, seed ^ 0x9e3779b9u, &result[1]);
}

#if defined(FNV1A_HAS_AVX2)

/**
 * @brief AVX2 optimized FNV-1a 32-bit hash
 * Processes 8 hashes in parallel
 */
inline void hash_32_avx2_batch(const void* const* data_ptrs, const std::size_t* lengths,
                                uint32_t seed, uint32_t* out, std::size_t count) {
    if (count < 8) {
        // Fallback to scalar for small batches
        for (std::size_t i = 0; i < count; ++i) {
            hash_32(data_ptrs[i], lengths[i], seed, &out[i]);
        }
        return;
    }

    const uint32_t offset = (seed == 0) ? detail::FNV1A_32_OFFSET_BASIS : seed;
    const uint32_t prime = detail::FNV1A_32_PRIME;

    // Process 8 at a time
    for (std::size_t batch = 0; batch < count / 8; ++batch) {
        __m256i hashes = _mm256_set1_epi32(offset);
        __m256i primes = _mm256_set1_epi32(prime);

        // Find minimum length
        std::size_t min_len = lengths[batch * 8];
        for (std::size_t j = 1; j < 8; ++j) {
            min_len = std::min(min_len, lengths[batch * 8 + j]);
        }

        // Process common bytes
        for (std::size_t pos = 0; pos < min_len; ++pos) {
            uint32_t bytes[8];
            for (std::size_t j = 0; j < 8; ++j) {
                const uint8_t* ptr = static_cast<const uint8_t*>(data_ptrs[batch * 8 + j]);
                bytes[j] = ptr[pos];
            }

            __m256i byte_vec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(bytes));
            hashes = _mm256_xor_si256(hashes, byte_vec);
            hashes = _mm256_mullo_epi32(hashes, primes);
        }

        // Store results
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out[batch * 8]), hashes);

        // Finish remaining bytes for each individually
        for (std::size_t j = 0; j < 8; ++j) {
            std::size_t idx = batch * 8 + j;
            if (lengths[idx] > min_len) {
                const uint8_t* bytes = static_cast<const uint8_t*>(data_ptrs[idx]);
                uint32_t hash = out[idx];
                for (std::size_t pos = min_len; pos < lengths[idx]; ++pos) {
                    hash ^= bytes[pos];
                    hash *= prime;
                }
                out[idx] = hash;
            }
        }
    }

    // Handle remainder
    std::size_t remainder = count % 8;
    if (remainder > 0) {
        for (std::size_t i = count - remainder; i < count; ++i) {
            hash_32(data_ptrs[i], lengths[i], seed, &out[i]);
        }
    }
}

#endif // FNV1A_HAS_AVX2

#if defined(FNV1A_HAS_SSE2)

/**
 * @brief SSE2 optimized FNV-1a 32-bit hash
 * Processes 4 hashes in parallel
 */
inline void hash_32_sse2_batch(const void* const* data_ptrs, const std::size_t* lengths,
                                uint32_t seed, uint32_t* out, std::size_t count) {
    if (count < 4) {
        for (std::size_t i = 0; i < count; ++i) {
            hash_32(data_ptrs[i], lengths[i], seed, &out[i]);
        }
        return;
    }

    const uint32_t offset = (seed == 0) ? detail::FNV1A_32_OFFSET_BASIS : seed;
    const uint32_t prime = detail::FNV1A_32_PRIME;

    // Process 4 at a time
    for (std::size_t batch = 0; batch < count / 4; ++batch) {
        __m128i hashes = _mm_set1_epi32(offset);
        __m128i primes = _mm_set1_epi32(prime);

        std::size_t min_len = lengths[batch * 4];
        for (std::size_t j = 1; j < 4; ++j) {
            min_len = std::min(min_len, lengths[batch * 4 + j]);
        }

        for (std::size_t pos = 0; pos < min_len; ++pos) {
            alignas(16) uint32_t bytes[4];
            for (std::size_t j = 0; j < 4; ++j) {
                const uint8_t* ptr = static_cast<const uint8_t*>(data_ptrs[batch * 4 + j]);
                bytes[j] = ptr[pos];
            }

            __m128i byte_vec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(bytes));
            hashes = _mm_xor_si128(hashes, byte_vec);

            // Multiply (SSE2 doesn't have 32-bit multiply, use manual approach)
            __m128i lo = _mm_mul_epu32(hashes, primes);
            __m128i hi = _mm_mul_epu32(_mm_srli_si128(hashes, 4), _mm_srli_si128(primes, 4));
            hashes = _mm_unpacklo_epi32(_mm_shuffle_epi32(lo, _MM_SHUFFLE(0,0,2,0)),
                                        _mm_shuffle_epi32(hi, _MM_SHUFFLE(0,0,2,0)));
        }

        _mm_storeu_si128(reinterpret_cast<__m128i*>(&out[batch * 4]), hashes);

        // Finish remaining bytes
        for (std::size_t j = 0; j < 4; ++j) {
            std::size_t idx = batch * 4 + j;
            if (lengths[idx] > min_len) {
                const uint8_t* bytes = static_cast<const uint8_t*>(data_ptrs[idx]);
                uint32_t hash = out[idx];
                for (std::size_t pos = min_len; pos < lengths[idx]; ++pos) {
                    hash ^= bytes[pos];
                    hash *= prime;
                }
                out[idx] = hash;
            }
        }
    }

    std::size_t remainder = count % 4;
    if (remainder > 0) {
        for (std::size_t i = count - remainder; i < count; ++i) {
            hash_32(data_ptrs[i], lengths[i], seed, &out[i]);
        }
    }
}

#endif // FNV1A_HAS_SSE2

#if defined(FNV1A_HAS_NEON)

/**
 * @brief ARM NEON optimized FNV-1a 32-bit hash
 * Processes 4 hashes in parallel
 */
inline void hash_32_neon_batch(const void* const* data_ptrs, const std::size_t* lengths,
                                uint32_t seed, uint32_t* out, std::size_t count) {
    if (count < 4) {
        for (std::size_t i = 0; i < count; ++i) {
            hash_32(data_ptrs[i], lengths[i], seed, &out[i]);
        }
        return;
    }

    const uint32_t offset = (seed == 0) ? detail::FNV1A_32_OFFSET_BASIS : seed;
    const uint32_t prime = detail::FNV1A_32_PRIME;

    for (std::size_t batch = 0; batch < count / 4; ++batch) {
        uint32x4_t hashes = vdupq_n_u32(offset);
        uint32x4_t primes = vdupq_n_u32(prime);

        std::size_t min_len = lengths[batch * 4];
        for (std::size_t j = 1; j < 4; ++j) {
            min_len = std::min(min_len, lengths[batch * 4 + j]);
        }

        for (std::size_t pos = 0; pos < min_len; ++pos) {
            uint32_t bytes[4];
            for (std::size_t j = 0; j < 4; ++j) {
                const uint8_t* ptr = static_cast<const uint8_t*>(data_ptrs[batch * 4 + j]);
                bytes[j] = ptr[pos];
            }

            uint32x4_t byte_vec = vld1q_u32(bytes);
            hashes = veorq_u32(hashes, byte_vec);
            hashes = vmulq_u32(hashes, primes);
        }

        vst1q_u32(&out[batch * 4], hashes);

        // Finish remaining bytes
        for (std::size_t j = 0; j < 4; ++j) {
            std::size_t idx = batch * 4 + j;
            if (lengths[idx] > min_len) {
                const uint8_t* bytes = static_cast<const uint8_t*>(data_ptrs[idx]);
                uint32_t hash = out[idx];
                for (std::size_t pos = min_len; pos < lengths[idx]; ++pos) {
                    hash ^= bytes[pos];
                    hash *= prime;
                }
                out[idx] = hash;
            }
        }
    }

    std::size_t remainder = count % 4;
    if (remainder > 0) {
        for (std::size_t i = count - remainder; i < count; ++i) {
            hash_32(data_ptrs[i], lengths[i], seed, &out[i]);
        }
    }
}

#endif // FNV1A_HAS_NEON

/**
 * @brief Get the name of the hash algorithm
 */
inline const char* get_algorithm_name() {
    return "FNV-1a";
}

/**
 * @brief Check if SIMD is available for FNV-1a
 */
inline const char* get_simd_support() {
#if defined(FNV1A_HAS_AVX2)
    return "AVX2";
#elif defined(FNV1A_HAS_SSE2)
    return "SSE2";
#elif defined(FNV1A_HAS_NEON)
    return "NEON";
#else
    return "Scalar";
#endif
}

} // namespace fnv1a
