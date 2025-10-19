// SPDX-License-Identifier: MIT
// Copyright 2025 - Present Light Transport Entertainment Inc.

#pragma once

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <ranges>
#include <set>
#include <unordered_set>
#include <vector>

namespace minhash {

/**
 * @brief Exact Jaccard index computation for sets
 *
 * Jaccard(A, B) = |A ∩ B| / |A ∪ B|
 *
 * This is the ground truth similarity measure. MinHash provides an estimate.
 */
namespace jaccard {

/**
 * @brief Compute exact Jaccard similarity between two sorted ranges
 * @param a First sorted range
 * @param b Second sorted range
 * @return Jaccard similarity [0.0, 1.0]
 *
 * Note: Ranges must be sorted for std::set_intersection/union to work.
 * If not sorted, use jaccard_unsorted instead.
 */
template <std::ranges::forward_range R1, std::ranges::forward_range R2>
requires std::equality_comparable_with<std::ranges::range_value_t<R1>,
                                        std::ranges::range_value_t<R2>>
[[nodiscard]] double exact_sorted(const R1& a, const R2& b) {
    using value_type = std::common_type_t<std::ranges::range_value_t<R1>,
                                           std::ranges::range_value_t<R2>>;

    if (std::ranges::empty(a) && std::ranges::empty(b)) {
        return 1.0; // Empty sets are identical
    }

    if (std::ranges::empty(a) || std::ranges::empty(b)) {
        return 0.0; // One empty, one not
    }

    // Count intersection
    std::vector<value_type> intersection;
    std::ranges::set_intersection(a, b, std::back_inserter(intersection));
    const std::size_t intersection_size = intersection.size();

    // Count union
    std::vector<value_type> union_set;
    std::ranges::set_union(a, b, std::back_inserter(union_set));
    const std::size_t union_size = union_set.size();

    if (union_size == 0) {
        return 1.0;
    }

    return static_cast<double>(intersection_size) / static_cast<double>(union_size);
}

/**
 * @brief Compute exact Jaccard similarity between two unsorted ranges
 * @param a First range
 * @param b Second range
 * @return Jaccard similarity [0.0, 1.0]
 *
 * This version handles unsorted data by creating sets internally.
 * Less efficient than exact_sorted but more convenient.
 */
template <std::ranges::forward_range R1, std::ranges::forward_range R2>
requires std::equality_comparable_with<std::ranges::range_value_t<R1>,
                                        std::ranges::range_value_t<R2>>
[[nodiscard]] double exact_unsorted(const R1& a, const R2& b) {
    using value_type = std::common_type_t<std::ranges::range_value_t<R1>,
                                           std::ranges::range_value_t<R2>>;

    std::set<value_type> set_a(std::ranges::begin(a), std::ranges::end(a));
    std::set<value_type> set_b(std::ranges::begin(b), std::ranges::end(b));

    if (set_a.empty() && set_b.empty()) {
        return 1.0;
    }

    if (set_a.empty() || set_b.empty()) {
        return 0.0;
    }

    // Count intersection
    std::size_t intersection_size = 0;
    for (const auto& elem : set_a) {
        if (set_b.contains(elem)) {
            ++intersection_size;
        }
    }

    // Union size = |A| + |B| - |A ∩ B|
    const std::size_t union_size = set_a.size() + set_b.size() - intersection_size;

    if (union_size == 0) {
        return 1.0;
    }

    return static_cast<double>(intersection_size) / static_cast<double>(union_size);
}

/**
 * @brief Compute exact Jaccard similarity using hash sets (faster for large sets)
 * @param a First range
 * @param b Second range
 * @return Jaccard similarity [0.0, 1.0]
 */
template <std::ranges::forward_range R1, std::ranges::forward_range R2>
requires std::equality_comparable_with<std::ranges::range_value_t<R1>,
                                        std::ranges::range_value_t<R2>> &&
         requires(std::ranges::range_value_t<R1> v) {
             { std::hash<std::ranges::range_value_t<R1>>{}(v) } -> std::convertible_to<std::size_t>;
         }
[[nodiscard]] double exact_hash(const R1& a, const R2& b) {
    using value_type = std::common_type_t<std::ranges::range_value_t<R1>,
                                           std::ranges::range_value_t<R2>>;

    std::unordered_set<value_type> set_a(std::ranges::begin(a), std::ranges::end(a));
    std::unordered_set<value_type> set_b(std::ranges::begin(b), std::ranges::end(b));

    if (set_a.empty() && set_b.empty()) {
        return 1.0;
    }

    if (set_a.empty() || set_b.empty()) {
        return 0.0;
    }

    // Count intersection
    std::size_t intersection_size = 0;
    // Iterate over smaller set for efficiency
    const auto& smaller = set_a.size() < set_b.size() ? set_a : set_b;
    const auto& larger = set_a.size() < set_b.size() ? set_b : set_a;

    for (const auto& elem : smaller) {
        if (larger.contains(elem)) {
            ++intersection_size;
        }
    }

    // Union size = |A| + |B| - |A ∩ B|
    const std::size_t union_size = set_a.size() + set_b.size() - intersection_size;

    if (union_size == 0) {
        return 1.0;
    }

    return static_cast<double>(intersection_size) / static_cast<double>(union_size);
}

/**
 * @brief Compute containment of A in B: |A ∩ B| / |A|
 * @param a First set (contained)
 * @param b Second set (container)
 * @return Containment [0.0, 1.0]
 *
 * Note: containment(A, B) != containment(B, A) in general
 */
template <std::ranges::forward_range R1, std::ranges::forward_range R2>
requires std::equality_comparable_with<std::ranges::range_value_t<R1>,
                                        std::ranges::range_value_t<R2>> &&
         requires(std::ranges::range_value_t<R1> v) {
             { std::hash<std::ranges::range_value_t<R1>>{}(v) } -> std::convertible_to<std::size_t>;
         }
[[nodiscard]] double containment(const R1& a, const R2& b) {
    using value_type = std::common_type_t<std::ranges::range_value_t<R1>,
                                           std::ranges::range_value_t<R2>>;

    std::unordered_set<value_type> set_a(std::ranges::begin(a), std::ranges::end(a));
    std::unordered_set<value_type> set_b(std::ranges::begin(b), std::ranges::end(b));

    if (set_a.empty()) {
        return 1.0; // Empty set is contained in everything
    }

    std::size_t intersection_size = 0;
    for (const auto& elem : set_a) {
        if (set_b.contains(elem)) {
            ++intersection_size;
        }
    }

    return static_cast<double>(intersection_size) / static_cast<double>(set_a.size());
}

/**
 * @brief Compute Dice coefficient: 2 * |A ∩ B| / (|A| + |B|)
 * @param a First set
 * @param b Second set
 * @return Dice coefficient [0.0, 1.0]
 *
 * Related to Jaccard: Dice = 2*J / (1+J)
 */
template <std::ranges::forward_range R1, std::ranges::forward_range R2>
requires std::equality_comparable_with<std::ranges::range_value_t<R1>,
                                        std::ranges::range_value_t<R2>> &&
         requires(std::ranges::range_value_t<R1> v) {
             { std::hash<std::ranges::range_value_t<R1>>{}(v) } -> std::convertible_to<std::size_t>;
         }
[[nodiscard]] double dice(const R1& a, const R2& b) {
    using value_type = std::common_type_t<std::ranges::range_value_t<R1>,
                                           std::ranges::range_value_t<R2>>;

    std::unordered_set<value_type> set_a(std::ranges::begin(a), std::ranges::end(a));
    std::unordered_set<value_type> set_b(std::ranges::begin(b), std::ranges::end(b));

    if (set_a.empty() && set_b.empty()) {
        return 1.0;
    }

    const std::size_t sum_size = set_a.size() + set_b.size();
    if (sum_size == 0) {
        return 1.0;
    }

    std::size_t intersection_size = 0;
    const auto& smaller = set_a.size() < set_b.size() ? set_a : set_b;
    const auto& larger = set_a.size() < set_b.size() ? set_b : set_a;

    for (const auto& elem : smaller) {
        if (larger.contains(elem)) {
            ++intersection_size;
        }
    }

    return 2.0 * static_cast<double>(intersection_size) / static_cast<double>(sum_size);
}

} // namespace jaccard

} // namespace minhash
