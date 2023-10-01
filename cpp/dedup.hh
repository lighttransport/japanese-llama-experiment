#pragma once

#include <cstdint>
#include <set>
#include <unordered_set>
#include <vector>
#include <cstring>
#include <iostream>

#include "str-util.hh"
#include "MurmurHash3.h"

// cityhash
template <class T> inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    const std::size_t kMul = 0x9ddfea08eb382d69ULL;
    std::size_t a = (hasher(v) ^ seed) * kMul;
    a ^= (a >> 47);
    std::size_t b = (seed ^ a) * kMul;
    b ^= (b >> 47);
    seed = b * kMul;
}


// B-byte minhash
// for now, B must 2
template<uint32_t BUCKET_SIZE = 10, uint32_t B = 2>
struct MinHashVal
{
  static_assert(B == 2, "B must be 2 for now.");

  uint32_t vals[BUCKET_SIZE / (sizeof(uint32_t) / B)];

  const char *data() const {
    return reinterpret_cast<const char *>(vals);
  }

  char *data() {
    return reinterpret_cast<char *>(vals);
  }

  constexpr size_t size() const {
    return BUCKET_SIZE * B;
  }

  constexpr size_t nitems() const {
    return sizeof(vals) / sizeof(uint32_t);
  }

};

static_assert(sizeof(size_t) == sizeof(uint64_t), "");

// hasher for unordered_set
// value is already hashed, so only take a combine of them.
template<uint32_t BUCKET_SIZE = 10, uint32_t B = 2>
struct MinHashValHasher
{
  static_assert(B == 2, "B must be 2 for now.");

  size_t operator()(const MinHashVal<BUCKET_SIZE, B> &k) const {
    size_t seed = k.vals[0];
    for (uint32_t i = 1; i < k.nitems(); i++) {
      hash_combine(seed, k.vals[i]);
    }

    return seed;
  }
};

// comparator for unordered_set
template<uint32_t BUCKET_SIZE = 10, uint32_t B = 2>
struct MinHashValEqual
{
  static_assert(B == 2, "B must be 2 for now.");

  constexpr bool operator()(const MinHashVal<BUCKET_SIZE, B> &lhs, const MinHashVal<BUCKET_SIZE, B> &rhs) const {
    for (uint32_t i = 0; i < lhs.nitems(); i++) {
      if (lhs.vals[i] != rhs.vals[i]) {
        return false;
      }
    }

    return true;
  }
};

template<uint32_t BUCKET_SIZE = 10, uint32_t B = 2>
inline bool operator<(const MinHashVal<BUCKET_SIZE, B>& a, const MinHashVal<BUCKET_SIZE, B>& b) {
  for (uint32_t i = 0; i < a.nitems(); i++) {
    if (a.vals[i] < b.vals[i]) {
      return true;
    }
  }
  return false;
}

#if 0
template<int N_BUCKETS, int BUCKET_SIZE>
std::vector<std::vector<uint8_t>> compute_lsh_fast(
  const std::vector<strutil::NGram> &ngram_text, LSHDedupConfig &config) {


}
#endif

template<uint32_t N_GRAM, uint32_t N_BUCKETS = 20, uint32_t BUCKET_SIZE = 10>
std::array<MinHashVal<BUCKET_SIZE, 2>, N_BUCKETS> compute_lsh(
  const std::vector<strutil::NGram<N_GRAM>> &ngram_text)
{
  constexpr uint32_t N_MINHASH = N_BUCKETS * BUCKET_SIZE;

  std::array<uint32_t, N_MINHASH> fingerprints; // len = n_minhash

  for (uint32_t seed = 0; seed < N_MINHASH; seed++) {

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

    fingerprints[seed] = min_hashval;
  }

  // bucketize
  std::array<MinHashVal<BUCKET_SIZE, 2>, N_BUCKETS> lshs;

  for (size_t bucket_i = 0; bucket_i < N_BUCKETS; bucket_i++) {

    MinHashVal<BUCKET_SIZE, 2> lsh;
    // lsh = concat fingerprints by extracting lower 2byte of hash

    for (size_t bucket_s = 0; bucket_s < BUCKET_SIZE; bucket_s++) {
      // extract LSB 2 bytes.
      uint16_t f = uint16_t(fingerprints[bucket_i * BUCKET_SIZE + bucket_s] & 0xffff);

      memcpy(reinterpret_cast<uint8_t *>(&lsh.vals[0]) + 2 * bucket_s,  &f, 2);
    }

    lshs[bucket_i] = lsh;
  }

  return lshs;
}

template<uint32_t N_BUCKETS, uint32_t BUCKET_SIZE = 10, uint32_t B = 2>
bool dedup_stream(
  const std::array<MinHashVal<BUCKET_SIZE, B>, N_BUCKETS> &lshs,
  std::unordered_set<MinHashVal<BUCKET_SIZE, B>, MinHashValHasher<BUCKET_SIZE, B>, MinHashValEqual<BUCKET_SIZE, B>> &hash_store /* inout */) {

  bool duplicated{false};

  for (size_t i = 0; i < N_BUCKETS; i++) {
    const auto &lsh = lshs[i];

    if (hash_store.count(lsh)) {
      duplicated = true;
    } else {
      hash_store.insert(lsh);
    }
  }

  return duplicated;
}

struct DocumentItem
{
  uint32_t bucket_id{0};
  uint64_t document_id{0};
  uint64_t minhash_index{0}; // index to minhash array
};

template<uint32_t N_BUCKETS, uint32_t BUCKET_SIZE = 10, uint32_t B = 2>
bool sort_minhashes(
  const std::vector<std::array<MinHashVal<BUCKET_SIZE, B>, N_BUCKETS>> &lshs,
  std::vector<DocumentItem> &docs) {

  if ((N_BUCKETS * docs.size()) != lshs.size()) {
    return false;
  }

  size_t n = lshs.size();

  // TODO: Use radix sort?
  std::sort(docs.begin(), docs.end(), [&](const DocumentItem &a, const DocumentItem &b) {
    uint64_t a_bucket_id = a.bucket_id;
    uint64_t b_bucket_id = b.bucket_id;

    MinHashVal<BUCKET_SIZE, B> &a_l = lshs[a.minhash_index];
    MinHashVal<BUCKET_SIZE, B> &b_l = lshs[b.minhash_index];

    return (a_l < b_l);
  });

  return true;
}


#if 0
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
#endif

#if 0
// Do text dedup with Locally-Sensitive Hash
bool dedup_lsh(const LSHDedupConfig &config);

// Do text dedup by computing Jaccard coefficient.
// Its more accurate than LSH, but takes time.
bool dedup_jaccard();
#endif
