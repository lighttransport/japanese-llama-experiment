#include <algorithm>
#include <cstdint>
#include <set>
#include <vector>

#include "dedup.hh"
#include "MurmurHash3.h"

std::vector<std::vector<uint8_t>> compute_lsh(
  const std::vector<strutil::NGram> &ngram_text,
  const LSHDedupConfig &conf) {

  uint32_t n_minhash = conf.n_buckets * conf.bucket_size;

  std::vector<uint32_t> fingerprints; // len = n_minhash

  for (uint32_t seed = 0; seed < n_minhash; seed++) {

    uint32_t min_hashval;

    for (size_t n = 0; n < ngram_text.size(); n++) {

      // to bytes.
      std::vector<uint8_t> text_bytes = strutil::ngram_to_bytes(ngram_text[n]);

      uint32_t hashval;

      // TODO: Use 64bit or 128bit hash for better accuracy.
      MurmurHash3_x86_32 ( reinterpret_cast<const void *>(text_bytes.data()), text_bytes.size(), seed, reinterpret_cast<void *>(&hashval));

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
    // lsh = 1byte(bucket_id) + concat fingerprints by extracting lower 2byte of hash
    lsh.push_back(uint8_t(bucket_i));

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

// ret = intersection(a, b) / union(a, b)
double compute_jaccard(std::vector<uint32_t> &a, std::vector<uint32_t> &b) {
  // inputs must be sorted.
  std::sort(a.begin(), a.end());
  std::sort(b.begin(), b.end());

  std::vector<uint32_t> result_u;
  std::vector<uint32_t> result_i;

  std::set_union(a.begin(), a.end(), b.begin(), b.end(),
                 std::inserter(result_u, std::end(result_u)));
  std::set_intersection(a.begin(), a.end(), b.begin(), b.end(),
                        std::inserter(result_i, std::end(result_i)));

  if (result_u.size() == 0) {
    return 0.0;
  }

  return double(result_i.size()) / double(result_u.size());
}
