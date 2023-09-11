#pragma once

#include <cstdint>
#include <set>
#include <vector>

#include "str-util.hh"
#include "MurmurHash3.h"

struct LSHDedupConfig
{
  uint32_t n_gram{5};
  uint32_t n_buckets{20};
  uint32_t bucket_size{10}; // 450 for higher accuracy(from RefinedWeb)
};

#if 0
template<int N_BUCKETS, int BUCKET_SIZE>
std::vector<std::vector<uint8_t>> compute_lsh_fast(
  const std::vector<strutil::NGram> &ngram_text, LSHDedupConfig &config) {


}
#endif

template<uint32_t N>
std::vector<std::vector<uint8_t>> compute_lsh(
  const std::vector<strutil::NGram<N>> &ngram_text,
  const LSHDedupConfig &conf) {

  uint32_t n_minhash = conf.n_buckets * conf.bucket_size;

  std::vector<uint32_t> fingerprints; // len = n_minhash

  for (uint32_t seed = 0; seed < n_minhash; seed++) {

    uint32_t min_hashval;

    for (size_t n = 0; n < ngram_text.size(); n++) {

      uint32_t hashval;

      // TODO: Use 64bit or 128bit hash for better accuracy.
      MurmurHash3_x86_32 ( reinterpret_cast<const void *>(ngram_text[n].buffer()), ngram_text[n].n_bytes(), seed, reinterpret_cast<void *>(&hashval));

      if (n == 0) {
        min_hashval = hashval;
      } else {
        min_hashval = std::min(min_hashval, hashval);
      }
    }

    fingerprints.push_back(min_hashval);

  }

  // bucketize
  std::vector<std::vector<uint8_t>> lshs;

  for (size_t bucket_i = 0; bucket_i < conf.n_buckets; bucket_i++) {

    std::vector<uint8_t> lsh;
    // lsh = concat fingerprints by extracting lower 2byte of hash

    for (size_t bucket_s = 0; bucket_s < conf.bucket_size; bucket_s++) {
      // LSB 2 bytes.
      uint16_t f = uint16_t(fingerprints[bucket_i * conf.bucket_size + bucket_s] & 0xffff);

      lsh.push_back(uint8_t((f >> 8) & 0xff));
      lsh.push_back(uint8_t(f & 0xff));
    }

    lshs.push_back(lsh);
  }

  return lshs;
}


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
