// SPDX-FileCopyrightText: 2025 Light Transport Entertainment Inc.
// SPDX-License-Identifier: MIT

#pragma once

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace minhash {
namespace edit {

/**
 * @brief Compute Levenshtein (edit) distance between two sequences
 *
 * The Levenshtein distance is the minimum number of single-character edits
 * (insertions, deletions, or substitutions) required to change one sequence
 * into another.
 *
 * Time complexity: O(m * n) where m, n are sequence lengths
 * Space complexity: O(min(m, n)) using space-optimized algorithm
 *
 * @tparam T Element type (must be equality comparable)
 * @param a First sequence
 * @param b Second sequence
 * @return Levenshtein distance (0 = identical, higher = more different)
 */
template <typename T>
  requires std::equality_comparable<T>
std::size_t levenshtein_distance(const std::vector<T>& a, const std::vector<T>& b) {
  const std::size_t m = a.size();
  const std::size_t n = b.size();

  // Handle edge cases
  if (m == 0) return n;
  if (n == 0) return m;

  // Ensure we use the shorter sequence for the column to save memory
  if (m > n) {
    return levenshtein_distance(b, a);
  }

  // Use single row optimization: only need previous and current row
  std::vector<std::size_t> prev_row(m + 1);
  std::vector<std::size_t> curr_row(m + 1);

  // Initialize first row: distance from empty string
  for (std::size_t i = 0; i <= m; ++i) {
    prev_row[i] = i;
  }

  // Fill the matrix row by row
  for (std::size_t j = 1; j <= n; ++j) {
    curr_row[0] = j;  // Distance from empty string

    for (std::size_t i = 1; i <= m; ++i) {
      const std::size_t cost = (a[i - 1] == b[j - 1]) ? 0 : 1;

      curr_row[i] = std::min({
          prev_row[i] + 1,      // Deletion
          curr_row[i - 1] + 1,  // Insertion
          prev_row[i - 1] + cost  // Substitution
      });
    }

    std::swap(prev_row, curr_row);
  }

  return prev_row[m];
}

/**
 * @brief Compute Levenshtein distance between two strings
 *
 * @param a First string
 * @param b Second string
 * @return Levenshtein distance
 */
inline std::size_t levenshtein_distance(std::string_view a, std::string_view b) {
  const std::size_t m = a.size();
  const std::size_t n = b.size();

  // Handle edge cases
  if (m == 0) return n;
  if (n == 0) return m;

  // Ensure we use the shorter string for the column to save memory
  if (m > n) {
    return levenshtein_distance(b, a);
  }

  // Use single row optimization
  std::vector<std::size_t> prev_row(m + 1);
  std::vector<std::size_t> curr_row(m + 1);

  // Initialize first row
  for (std::size_t i = 0; i <= m; ++i) {
    prev_row[i] = i;
  }

  // Fill the matrix row by row
  for (std::size_t j = 1; j <= n; ++j) {
    curr_row[0] = j;

    for (std::size_t i = 1; i <= m; ++i) {
      const std::size_t cost = (a[i - 1] == b[j - 1]) ? 0 : 1;

      curr_row[i] = std::min({
          prev_row[i] + 1,          // Deletion
          curr_row[i - 1] + 1,      // Insertion
          prev_row[i - 1] + cost    // Substitution
      });
    }

    std::swap(prev_row, curr_row);
  }

  return prev_row[m];
}

/**
 * @brief Compute normalized edit similarity between two sequences
 *
 * The edit similarity is defined as:
 *   similarity = 1 - (edit_distance / max_length)
 *
 * @tparam T Element type (must be equality comparable)
 * @param a First sequence
 * @param b Second sequence
 * @return Similarity score in [0.0, 1.0] where 1.0 = identical
 */
template <typename T>
  requires std::equality_comparable<T>
double similarity(const std::vector<T>& a, const std::vector<T>& b) {
  const std::size_t max_len = std::max(a.size(), b.size());

  // Handle edge case: both sequences empty
  if (max_len == 0) {
    return 1.0;
  }

  const std::size_t distance = levenshtein_distance(a, b);
  return 1.0 - (static_cast<double>(distance) / static_cast<double>(max_len));
}

/**
 * @brief Compute normalized edit similarity between two strings
 *
 * @param a First string
 * @param b Second string
 * @return Similarity score in [0.0, 1.0] where 1.0 = identical
 */
inline double similarity(std::string_view a, std::string_view b) {
  const std::size_t max_len = std::max(a.size(), b.size());

  // Handle edge case: both strings empty
  if (max_len == 0) {
    return 1.0;
  }

  const std::size_t distance = levenshtein_distance(a, b);
  return 1.0 - (static_cast<double>(distance) / static_cast<double>(max_len));
}

/**
 * @brief Compute Damerau-Levenshtein distance between two strings
 *
 * The Damerau-Levenshtein distance includes transpositions of two adjacent
 * characters in addition to insertions, deletions, and substitutions.
 *
 * Time complexity: O(m * n)
 * Space complexity: O(m * n)
 *
 * @param a First string
 * @param b Second string
 * @return Damerau-Levenshtein distance
 */
inline std::size_t damerau_levenshtein_distance(std::string_view a, std::string_view b) {
  const std::size_t m = a.size();
  const std::size_t n = b.size();

  // Handle edge cases
  if (m == 0) return n;
  if (n == 0) return m;

  // Allocate matrix (need full matrix for transposition tracking)
  std::vector<std::vector<std::size_t>> matrix(m + 1, std::vector<std::size_t>(n + 1));

  // Initialize first row and column
  for (std::size_t i = 0; i <= m; ++i) {
    matrix[i][0] = i;
  }
  for (std::size_t j = 0; j <= n; ++j) {
    matrix[0][j] = j;
  }

  // Fill the matrix
  for (std::size_t i = 1; i <= m; ++i) {
    for (std::size_t j = 1; j <= n; ++j) {
      const std::size_t cost = (a[i - 1] == b[j - 1]) ? 0 : 1;

      matrix[i][j] = std::min({
          matrix[i - 1][j] + 1,        // Deletion
          matrix[i][j - 1] + 1,        // Insertion
          matrix[i - 1][j - 1] + cost  // Substitution
      });

      // Transposition
      if (i > 1 && j > 1 && a[i - 1] == b[j - 2] && a[i - 2] == b[j - 1]) {
        matrix[i][j] = std::min(matrix[i][j], matrix[i - 2][j - 2] + 1);
      }
    }
  }

  return matrix[m][n];
}

/**
 * @brief Compute normalized Damerau-Levenshtein similarity
 *
 * @param a First string
 * @param b Second string
 * @return Similarity score in [0.0, 1.0] where 1.0 = identical
 */
inline double damerau_similarity(std::string_view a, std::string_view b) {
  const std::size_t max_len = std::max(a.size(), b.size());

  if (max_len == 0) {
    return 1.0;
  }

  const std::size_t distance = damerau_levenshtein_distance(a, b);
  return 1.0 - (static_cast<double>(distance) / static_cast<double>(max_len));
}

/**
 * @brief Compute Longest Common Subsequence (LCS) length
 *
 * The LCS is the longest sequence that can be obtained from both input
 * sequences by deleting some elements (without reordering).
 *
 * Time complexity: O(m * n)
 * Space complexity: O(min(m, n))
 *
 * @tparam T Element type (must be equality comparable)
 * @param a First sequence
 * @param b Second sequence
 * @return Length of LCS
 */
template <typename T>
  requires std::equality_comparable<T>
std::size_t lcs_length(const std::vector<T>& a, const std::vector<T>& b) {
  const std::size_t m = a.size();
  const std::size_t n = b.size();

  if (m == 0 || n == 0) return 0;

  // Ensure we use the shorter sequence for the column
  if (m > n) {
    return lcs_length(b, a);
  }

  // Space-optimized: only need previous and current row
  std::vector<std::size_t> prev_row(m + 1, 0);
  std::vector<std::size_t> curr_row(m + 1, 0);

  for (std::size_t j = 1; j <= n; ++j) {
    for (std::size_t i = 1; i <= m; ++i) {
      if (a[i - 1] == b[j - 1]) {
        curr_row[i] = prev_row[i - 1] + 1;
      } else {
        curr_row[i] = std::max(prev_row[i], curr_row[i - 1]);
      }
    }
    std::swap(prev_row, curr_row);
  }

  return prev_row[m];
}

/**
 * @brief Compute LCS length for strings
 *
 * @param a First string
 * @param b Second string
 * @return Length of LCS
 */
inline std::size_t lcs_length(std::string_view a, std::string_view b) {
  const std::size_t m = a.size();
  const std::size_t n = b.size();

  if (m == 0 || n == 0) return 0;

  if (m > n) {
    return lcs_length(b, a);
  }

  std::vector<std::size_t> prev_row(m + 1, 0);
  std::vector<std::size_t> curr_row(m + 1, 0);

  for (std::size_t j = 1; j <= n; ++j) {
    for (std::size_t i = 1; i <= m; ++i) {
      if (a[i - 1] == b[j - 1]) {
        curr_row[i] = prev_row[i - 1] + 1;
      } else {
        curr_row[i] = std::max(prev_row[i], curr_row[i - 1]);
      }
    }
    std::swap(prev_row, curr_row);
  }

  return prev_row[m];
}

/**
 * @brief Compute LCS-based similarity
 *
 * Similarity is defined as:
 *   similarity = lcs_length / max_length
 *
 * @tparam T Element type (must be equality comparable)
 * @param a First sequence
 * @param b Second sequence
 * @return Similarity score in [0.0, 1.0] where 1.0 = identical
 */
template <typename T>
  requires std::equality_comparable<T>
double lcs_similarity(const std::vector<T>& a, const std::vector<T>& b) {
  const std::size_t max_len = std::max(a.size(), b.size());

  if (max_len == 0) {
    return 1.0;
  }

  const std::size_t lcs_len = lcs_length(a, b);
  return static_cast<double>(lcs_len) / static_cast<double>(max_len);
}

/**
 * @brief Compute LCS-based similarity for strings
 *
 * @param a First string
 * @param b Second string
 * @return Similarity score in [0.0, 1.0] where 1.0 = identical
 */
inline double lcs_similarity(std::string_view a, std::string_view b) {
  const std::size_t max_len = std::max(a.size(), b.size());

  if (max_len == 0) {
    return 1.0;
  }

  const std::size_t lcs_len = lcs_length(a, b);
  return static_cast<double>(lcs_len) / static_cast<double>(max_len);
}

/**
 * @brief Compute Hamming distance between two equal-length sequences
 *
 * The Hamming distance is the number of positions at which the corresponding
 * elements are different. Sequences must have the same length.
 *
 * Time complexity: O(n)
 * Space complexity: O(1)
 *
 * @tparam T Element type (must be equality comparable)
 * @param a First sequence
 * @param b Second sequence
 * @return Hamming distance, or SIZE_MAX if lengths differ
 */
template <typename T>
  requires std::equality_comparable<T>
std::size_t hamming_distance(const std::vector<T>& a, const std::vector<T>& b) {
  if (a.size() != b.size()) {
    return std::numeric_limits<std::size_t>::max();
  }

  std::size_t distance = 0;
  for (std::size_t i = 0; i < a.size(); ++i) {
    if (a[i] != b[i]) {
      ++distance;
    }
  }

  return distance;
}

/**
 * @brief Compute Hamming distance between two equal-length strings
 *
 * @param a First string
 * @param b Second string
 * @return Hamming distance, or SIZE_MAX if lengths differ
 */
inline std::size_t hamming_distance(std::string_view a, std::string_view b) {
  if (a.size() != b.size()) {
    return std::numeric_limits<std::size_t>::max();
  }

  std::size_t distance = 0;
  for (std::size_t i = 0; i < a.size(); ++i) {
    if (a[i] != b[i]) {
      ++distance;
    }
  }

  return distance;
}

/**
 * @brief Compute normalized Hamming similarity
 *
 * @tparam T Element type (must be equality comparable)
 * @param a First sequence
 * @param b Second sequence
 * @return Similarity score in [0.0, 1.0], or -1.0 if lengths differ
 */
template <typename T>
  requires std::equality_comparable<T>
double hamming_similarity(const std::vector<T>& a, const std::vector<T>& b) {
  if (a.size() != b.size()) {
    return -1.0;  // Invalid: different lengths
  }

  if (a.empty()) {
    return 1.0;
  }

  const std::size_t distance = hamming_distance(a, b);
  return 1.0 - (static_cast<double>(distance) / static_cast<double>(a.size()));
}

/**
 * @brief Compute normalized Hamming similarity for strings
 *
 * @param a First string
 * @param b Second string
 * @return Similarity score in [0.0, 1.0], or -1.0 if lengths differ
 */
inline double hamming_similarity(std::string_view a, std::string_view b) {
  if (a.size() != b.size()) {
    return -1.0;  // Invalid: different lengths
  }

  if (a.empty()) {
    return 1.0;
  }

  const std::size_t distance = hamming_distance(a, b);
  return 1.0 - (static_cast<double>(distance) / static_cast<double>(a.size()));
}

}  // namespace edit
}  // namespace minhash
