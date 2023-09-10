#pragma once

#include <cstdint>
#include <set>
#include <vector>
#include "str-util.hh"

struct LSHDedupConfig
{
  uint32_t n_gram{5};
  uint32_t n_buckets{20};
  uint32_t bucket_size{10};
};

template<int N_BUCKETS, int BUCKET_SIZE>
std::vector<std::vector<uint8_t>> compute_lsh_fast(
  const std::vector<strutil::NGram> &ngram_text, LSHDedupConfig &config) {


}

std::vector<std::vector<uint8_t>> compute_lsh(
  const std::vector<strutil::NGram> &ngram_text, const LSHDedupConfig &config);

///
///
/// Store hashes `hash_store`
/// TODO: Use fixed-size bytes for hash store.
///
/// @return true if duplication detected.
///
bool dedup_stream(
  const std::vector<std::vector<uint8_t>> &lshs,
  std::set<std::vector<uint8_t>> &hash_store /* inout */);

#if 0
// Do text dedup with Locally-Sensitive Hash
bool dedup_lsh(const LSHDedupConfig &config);

// Do text dedup by computing Jaccard coefficient.
// Its more accurate than LSH, but takes time.
bool dedup_jaccard();
#endif
