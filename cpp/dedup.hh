#pragma once

#include <cstdint>
#include "str-util.hh"

struct LSHDedupConfig
{
  uint32_t n_gram{5};
  uint32_t n_buckets{20};
  uint32_t bucket_size{10};
};

std::vector<std::vector<uint8_t>> compute_lsh(
  const std::vector<strutil::NGram> &ngram_text, const LSHDedupConfig &config);

// Do text dedup with Locally-Sensitive Hash
bool dedup_lsh(const LSHDedupConfig &config);

// Do text dedup by computing Jaccard coefficient.
// Its more accurate than LSH, but takes time.
bool dedup_jaccard();
