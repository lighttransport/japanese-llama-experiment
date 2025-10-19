// SPDX-FileCopyrightText: 2025 Light Transport Entertainment Inc.
// SPDX-License-Identifier: MIT

#include "edit_similarity.hpp"

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

// Helper function to print similarity results
void print_similarity(const std::string& label, double similarity) {
  std::cout << "  " << std::setw(30) << std::left << label
            << std::fixed << std::setprecision(4) << similarity
            << " (" << static_cast<int>(similarity * 100) << "%)\n";
}

// Helper function to print distance results
void print_distance(const std::string& label, std::size_t distance) {
  std::cout << "  " << std::setw(30) << std::left << label << distance << "\n";
}

int main() {
  std::cout << "=== Edit Similarity Examples ===\n\n";

  // Example 1: Basic Levenshtein distance
  {
    std::cout << "Example 1: Levenshtein Distance\n";
    std::cout << "--------------------------------\n";

    std::string str1 = "kitten";
    std::string str2 = "sitting";

    std::cout << "String 1: \"" << str1 << "\"\n";
    std::cout << "String 2: \"" << str2 << "\"\n\n";

    auto distance = minhash::edit::levenshtein_distance(str1, str2);
    auto similarity = minhash::edit::similarity(str1, str2);

    print_distance("Levenshtein distance:", distance);
    print_similarity("Edit similarity:", similarity);

    std::cout << "\nExplanation: 'kitten' -> 'sitting' requires 3 edits:\n";
    std::cout << "  1. kitten -> sitten (substitute k->s)\n";
    std::cout << "  2. sitten -> sittin (substitute e->i)\n";
    std::cout << "  3. sittin -> sitting (insert g)\n";
    std::cout << "\n";
  }

  // Example 2: Various string comparisons
  {
    std::cout << "Example 2: Various String Comparisons\n";
    std::cout << "--------------------------------------\n\n";

    struct TestCase {
      std::string str1;
      std::string str2;
      std::string description;
    };

    std::vector<TestCase> test_cases = {
      {"hello", "hello", "Identical strings"},
      {"hello", "hallo", "Single substitution"},
      {"hello", "helo", "Single deletion"},
      {"hello", "helloo", "Single insertion"},
      {"cat", "dog", "Completely different"},
      {"", "", "Both empty"},
      {"abc", "", "One empty"},
      {"saturday", "sunday", "Common example"},
      {"The quick brown fox", "The quick brown dog", "Similar sentences"},
    };

    for (const auto& tc : test_cases) {
      std::cout << tc.description << ":\n";
      std::cout << "  \"" << tc.str1 << "\" vs \"" << tc.str2 << "\"\n";

      auto lev_dist = minhash::edit::levenshtein_distance(tc.str1, tc.str2);
      auto edit_sim = minhash::edit::similarity(tc.str1, tc.str2);

      print_distance("Levenshtein distance:", lev_dist);
      print_similarity("Edit similarity:", edit_sim);
      std::cout << "\n";
    }
  }

  // Example 3: Damerau-Levenshtein distance (with transpositions)
  {
    std::cout << "Example 3: Damerau-Levenshtein Distance\n";
    std::cout << "----------------------------------------\n\n";

    std::string str1 = "CA";
    std::string str2 = "ABC";

    std::cout << "String 1: \"" << str1 << "\"\n";
    std::cout << "String 2: \"" << str2 << "\"\n\n";

    auto lev_dist = minhash::edit::levenshtein_distance(str1, str2);
    auto dam_dist = minhash::edit::damerau_levenshtein_distance(str1, str2);
    auto dam_sim = minhash::edit::damerau_similarity(str1, str2);

    print_distance("Levenshtein distance:", lev_dist);
    print_distance("Damerau-Levenshtein distance:", dam_dist);
    print_similarity("Damerau similarity:", dam_sim);

    std::cout << "\nTransposition example:\n";
    str1 = "abcd";
    str2 = "acbd";

    std::cout << "String 1: \"" << str1 << "\"\n";
    std::cout << "String 2: \"" << str2 << "\"\n\n";

    lev_dist = minhash::edit::levenshtein_distance(str1, str2);
    dam_dist = minhash::edit::damerau_levenshtein_distance(str1, str2);

    print_distance("Levenshtein distance:", lev_dist);
    print_distance("Damerau-Levenshtein distance:", dam_dist);
    std::cout << "\nDamerau-Levenshtein counts transposition of 'bc' as 1 edit\n";
    std::cout << "instead of 2 substitutions.\n\n";
  }

  // Example 4: Longest Common Subsequence
  {
    std::cout << "Example 4: Longest Common Subsequence (LCS)\n";
    std::cout << "--------------------------------------------\n\n";

    std::string str1 = "ABCDGH";
    std::string str2 = "AEDFHR";

    std::cout << "String 1: \"" << str1 << "\"\n";
    std::cout << "String 2: \"" << str2 << "\"\n\n";

    auto lcs_len = minhash::edit::lcs_length(str1, str2);
    auto lcs_sim = minhash::edit::lcs_similarity(str1, str2);

    print_distance("LCS length:", lcs_len);
    print_similarity("LCS similarity:", lcs_sim);
    std::cout << "\nThe LCS is \"ADH\" (length 3)\n\n";

    // Another example
    str1 = "AGGTAB";
    str2 = "GXTXAYB";

    std::cout << "String 1: \"" << str1 << "\"\n";
    std::cout << "String 2: \"" << str2 << "\"\n\n";

    lcs_len = minhash::edit::lcs_length(str1, str2);
    lcs_sim = minhash::edit::lcs_similarity(str1, str2);

    print_distance("LCS length:", lcs_len);
    print_similarity("LCS similarity:", lcs_sim);
    std::cout << "\nThe LCS is \"GTAB\" (length 4)\n\n";
  }

  // Example 5: Hamming distance
  {
    std::cout << "Example 5: Hamming Distance\n";
    std::cout << "----------------------------\n\n";

    std::string str1 = "karolin";
    std::string str2 = "kathrin";

    std::cout << "String 1: \"" << str1 << "\"\n";
    std::cout << "String 2: \"" << str2 << "\"\n\n";

    auto ham_dist = minhash::edit::hamming_distance(str1, str2);
    auto ham_sim = minhash::edit::hamming_similarity(str1, str2);

    print_distance("Hamming distance:", ham_dist);
    print_similarity("Hamming similarity:", ham_sim);

    std::cout << "\nDifferent positions: 3 (positions 1, 3, 4)\n";
    std::cout << "  k a r o l i n\n";
    std::cout << "  k a t h r i n\n";
    std::cout << "    X   X X\n\n";

    // Example with different lengths
    str1 = "hello";
    str2 = "world!";

    std::cout << "Different length example:\n";
    std::cout << "String 1: \"" << str1 << "\" (length " << str1.size() << ")\n";
    std::cout << "String 2: \"" << str2 << "\" (length " << str2.size() << ")\n\n";

    ham_sim = minhash::edit::hamming_similarity(str1, str2);
    std::cout << "  Hamming similarity: " << ham_sim << " (invalid, different lengths)\n\n";
  }

  // Example 6: Generic sequences (not just strings)
  {
    std::cout << "Example 6: Generic Sequences (Integer Vectors)\n";
    std::cout << "-----------------------------------------------\n\n";

    std::vector<int> vec1 = {1, 2, 3, 4, 5};
    std::vector<int> vec2 = {1, 3, 4, 5, 6};

    std::cout << "Vector 1: [";
    for (size_t i = 0; i < vec1.size(); ++i) {
      std::cout << vec1[i] << (i < vec1.size() - 1 ? ", " : "");
    }
    std::cout << "]\n";

    std::cout << "Vector 2: [";
    for (size_t i = 0; i < vec2.size(); ++i) {
      std::cout << vec2[i] << (i < vec2.size() - 1 ? ", " : "");
    }
    std::cout << "]\n\n";

    auto lev_dist = minhash::edit::levenshtein_distance(vec1, vec2);
    auto edit_sim = minhash::edit::similarity(vec1, vec2);
    auto lcs_len = minhash::edit::lcs_length(vec1, vec2);
    auto lcs_sim = minhash::edit::lcs_similarity(vec1, vec2);

    print_distance("Levenshtein distance:", lev_dist);
    print_similarity("Edit similarity:", edit_sim);
    print_distance("LCS length:", lcs_len);
    print_similarity("LCS similarity:", lcs_sim);
    std::cout << "\n";
  }

  // Example 7: Practical use case - spell checking candidates
  {
    std::cout << "Example 7: Spell Checking Candidates\n";
    std::cout << "-------------------------------------\n\n";

    std::string misspelled = "recieve";
    std::vector<std::string> dictionary = {
      "receive",
      "relieve",
      "believe",
      "retrieve",
      "deceive",
      "achieve"
    };

    std::cout << "Misspelled word: \"" << misspelled << "\"\n";
    std::cout << "Finding closest matches in dictionary:\n\n";

    std::vector<std::pair<std::string, double>> candidates;
    for (const auto& word : dictionary) {
      double sim = minhash::edit::similarity(misspelled, word);
      candidates.push_back({word, sim});
    }

    // Sort by similarity (highest first)
    std::sort(candidates.begin(), candidates.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    std::cout << "Ranked suggestions:\n";
    for (size_t i = 0; i < candidates.size(); ++i) {
      std::cout << "  " << (i + 1) << ". "
                << std::setw(12) << std::left << candidates[i].first
                << " (similarity: " << std::fixed << std::setprecision(4)
                << candidates[i].second << ")\n";
    }
    std::cout << "\n";
  }

  // Example 8: Performance comparison
  {
    std::cout << "Example 8: Algorithm Comparison\n";
    std::cout << "--------------------------------\n\n";

    std::string str1 = "The quick brown fox jumps over the lazy dog";
    std::string str2 = "The quick brown cat jumps over the lazy dog";

    std::cout << "String 1: \"" << str1 << "\"\n";
    std::cout << "String 2: \"" << str2 << "\"\n\n";

    std::cout << "Different similarity metrics:\n";
    print_similarity("Levenshtein similarity:", minhash::edit::similarity(str1, str2));
    print_similarity("Damerau-Levenshtein similarity:", minhash::edit::damerau_similarity(str1, str2));
    print_similarity("LCS similarity:", minhash::edit::lcs_similarity(str1, str2));
    print_similarity("Hamming similarity:", minhash::edit::hamming_similarity(str1, str2));

    std::cout << "\nNote: All metrics except Hamming give similar results.\n";
    std::cout << "Hamming requires equal-length strings.\n\n";
  }

  std::cout << "=== All Examples Complete ===\n";

  return 0;
}
