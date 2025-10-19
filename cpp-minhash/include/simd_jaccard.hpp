// SPDX-License-Identifier: MIT
// Copyright (c) 2024 cpp-minhash

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

// SIMD intrinsics detection and includes
#if defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2) || defined(_M_X64)
    #define MINHASH_HAS_SSE2 1
    #include <emmintrin.h>
#endif

#if defined(__AVX__)
    #define MINHASH_HAS_AVX 1
    #include <immintrin.h>
#endif

#if defined(__AVX2__)
    #define MINHASH_HAS_AVX2 1
    #include <immintrin.h>
#endif

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    #define MINHASH_HAS_NEON 1
    #include <arm_neon.h>
#endif

namespace minhash {
namespace simd {

/**
 * @brief Count matching elements in two arrays using SIMD when available
 *
 * This function automatically dispatches to the best available SIMD implementation:
 * - AVX2 (256-bit, processes 4 x uint64_t at once)
 * - SSE2 (128-bit, processes 2 x uint64_t at once)
 * - NEON (128-bit, processes 2 x uint64_t at once)
 * - Scalar fallback
 */

#if defined(MINHASH_HAS_AVX2)

/**
 * @brief AVX2 optimized signature comparison for 64-bit hashes
 */
inline std::size_t count_matches_avx2(const uint64_t* a, const uint64_t* b, std::size_t count) {
    std::size_t matches = 0;
    std::size_t i = 0;

    // Process 4 uint64_t at a time with AVX2 (256 bits)
    const std::size_t simd_count = count / 4;
    for (; i < simd_count * 4; i += 4) {
        __m256i va = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(a + i));
        __m256i vb = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(b + i));
        __m256i cmp = _mm256_cmpeq_epi64(va, vb);

        // Extract comparison results
        int mask = _mm256_movemask_pd(_mm256_castsi256_pd(cmp));
        matches += __builtin_popcount(mask);
    }

    // Handle remaining elements
    for (; i < count; ++i) {
        if (a[i] == b[i]) {
            ++matches;
        }
    }

    return matches;
}

/**
 * @brief AVX2 optimized signature comparison for 32-bit hashes
 */
inline std::size_t count_matches_avx2(const uint32_t* a, const uint32_t* b, std::size_t count) {
    std::size_t matches = 0;
    std::size_t i = 0;

    // Process 8 uint32_t at a time with AVX2 (256 bits)
    const std::size_t simd_count = count / 8;
    for (; i < simd_count * 8; i += 8) {
        __m256i va = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(a + i));
        __m256i vb = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(b + i));
        __m256i cmp = _mm256_cmpeq_epi32(va, vb);

        int mask = _mm256_movemask_ps(_mm256_castsi256_ps(cmp));
        matches += __builtin_popcount(mask);
    }

    // Handle remaining elements
    for (; i < count; ++i) {
        if (a[i] == b[i]) {
            ++matches;
        }
    }

    return matches;
}

#endif // MINHASH_HAS_AVX2

#if defined(MINHASH_HAS_SSE2)

/**
 * @brief SSE2 optimized signature comparison for 64-bit hashes
 */
inline std::size_t count_matches_sse2(const uint64_t* a, const uint64_t* b, std::size_t count) {
    std::size_t matches = 0;
    std::size_t i = 0;

    // Process 2 uint64_t at a time with SSE2 (128 bits)
    const std::size_t simd_count = count / 2;
    for (; i < simd_count * 2; i += 2) {
        __m128i va = _mm_loadu_si128(reinterpret_cast<const __m128i*>(a + i));
        __m128i vb = _mm_loadu_si128(reinterpret_cast<const __m128i*>(b + i));

        // Compare for equality (32-bit granularity, need two comparisons for 64-bit)
        __m128i cmp = _mm_cmpeq_epi32(va, vb);

        // For 64-bit equality, both 32-bit halves must match
        // Shuffle and AND to check both parts
        __m128i cmp_shuffled = _mm_shuffle_epi32(cmp, _MM_SHUFFLE(2, 3, 0, 1));
        __m128i cmp64 = _mm_and_si128(cmp, cmp_shuffled);

        int mask = _mm_movemask_pd(_mm_castsi128_pd(cmp64));
        matches += __builtin_popcount(mask);
    }

    // Handle remaining elements
    for (; i < count; ++i) {
        if (a[i] == b[i]) {
            ++matches;
        }
    }

    return matches;
}

/**
 * @brief SSE2 optimized signature comparison for 32-bit hashes
 */
inline std::size_t count_matches_sse2(const uint32_t* a, const uint32_t* b, std::size_t count) {
    std::size_t matches = 0;
    std::size_t i = 0;

    // Process 4 uint32_t at a time with SSE2 (128 bits)
    const std::size_t simd_count = count / 4;
    for (; i < simd_count * 4; i += 4) {
        __m128i va = _mm_loadu_si128(reinterpret_cast<const __m128i*>(a + i));
        __m128i vb = _mm_loadu_si128(reinterpret_cast<const __m128i*>(b + i));
        __m128i cmp = _mm_cmpeq_epi32(va, vb);

        int mask = _mm_movemask_ps(_mm_castsi128_ps(cmp));
        matches += __builtin_popcount(mask);
    }

    // Handle remaining elements
    for (; i < count; ++i) {
        if (a[i] == b[i]) {
            ++matches;
        }
    }

    return matches;
}

#endif // MINHASH_HAS_SSE2

#if defined(MINHASH_HAS_NEON)

/**
 * @brief ARM NEON optimized signature comparison for 64-bit hashes
 */
inline std::size_t count_matches_neon(const uint64_t* a, const uint64_t* b, std::size_t count) {
    std::size_t matches = 0;
    std::size_t i = 0;

    // Process 2 uint64_t at a time with NEON (128 bits)
    const std::size_t simd_count = count / 2;
    for (; i < simd_count * 2; i += 2) {
        uint64x2_t va = vld1q_u64(a + i);
        uint64x2_t vb = vld1q_u64(b + i);
        uint64x2_t cmp = vceqq_u64(va, vb);

        // Count matches - each lane is all 1s if equal, 0 if not
        uint64_t lane0 = vgetq_lane_u64(cmp, 0);
        uint64_t lane1 = vgetq_lane_u64(cmp, 1);

        if (lane0 != 0) ++matches;
        if (lane1 != 0) ++matches;
    }

    // Handle remaining elements
    for (; i < count; ++i) {
        if (a[i] == b[i]) {
            ++matches;
        }
    }

    return matches;
}

/**
 * @brief ARM NEON optimized signature comparison for 32-bit hashes
 */
inline std::size_t count_matches_neon(const uint32_t* a, const uint32_t* b, std::size_t count) {
    std::size_t matches = 0;
    std::size_t i = 0;

    // Process 4 uint32_t at a time with NEON (128 bits)
    const std::size_t simd_count = count / 4;
    for (; i < simd_count * 4; i += 4) {
        uint32x4_t va = vld1q_u32(a + i);
        uint32x4_t vb = vld1q_u32(b + i);
        uint32x4_t cmp = vceqq_u32(va, vb);

        // Count matches
        uint32_t lane0 = vgetq_lane_u32(cmp, 0);
        uint32_t lane1 = vgetq_lane_u32(cmp, 1);
        uint32_t lane2 = vgetq_lane_u32(cmp, 2);
        uint32_t lane3 = vgetq_lane_u32(cmp, 3);

        if (lane0 != 0) ++matches;
        if (lane1 != 0) ++matches;
        if (lane2 != 0) ++matches;
        if (lane3 != 0) ++matches;
    }

    // Handle remaining elements
    for (; i < count; ++i) {
        if (a[i] == b[i]) {
            ++matches;
        }
    }

    return matches;
}

#endif // MINHASH_HAS_NEON

/**
 * @brief Scalar fallback for signature comparison
 */
template <typename T>
inline std::size_t count_matches_scalar(const T* a, const T* b, std::size_t count) {
    std::size_t matches = 0;
    for (std::size_t i = 0; i < count; ++i) {
        if (a[i] == b[i]) {
            ++matches;
        }
    }
    return matches;
}

/**
 * @brief Auto-dispatching signature comparison
 * Selects the best available SIMD implementation at compile time
 */
template <typename T>
inline std::size_t count_matches(const T* a, const T* b, std::size_t count) {
#if defined(MINHASH_HAS_AVX2)
    return count_matches_avx2(a, b, count);
#elif defined(MINHASH_HAS_SSE2)
    return count_matches_sse2(a, b, count);
#elif defined(MINHASH_HAS_NEON)
    return count_matches_neon(a, b, count);
#else
    return count_matches_scalar(a, b, count);
#endif
}

/**
 * @brief Array-based signature comparison (for std::array)
 */
template <typename T, std::size_t N>
inline std::size_t count_matches(const std::array<T, N>& a, const std::array<T, N>& b) {
    return count_matches(a.data(), b.data(), N);
}

/**
 * @brief Compute Jaccard similarity from signature match count
 */
inline double jaccard_from_matches(std::size_t matches, std::size_t total) {
    if (total == 0) {
        return 1.0;
    }
    return static_cast<double>(matches) / static_cast<double>(total);
}

/**
 * @brief Get the name of the SIMD instruction set being used
 */
inline const char* get_simd_name() {
#if defined(MINHASH_HAS_AVX2)
    return "AVX2";
#elif defined(MINHASH_HAS_AVX)
    return "AVX";
#elif defined(MINHASH_HAS_SSE2)
    return "SSE2";
#elif defined(MINHASH_HAS_NEON)
    return "NEON";
#else
    return "Scalar";
#endif
}

} // namespace simd
} // namespace minhash
