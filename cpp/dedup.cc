#include <algorithm>
#include <clocale>
#include <cstdint>
#include <set>
#include <vector>

#include <thread>
#include <mutex>
#include <mutex>

#include "dedup.hh"

static uint32_t cpu_count() {
  return (std::max)(1u, std::thread::hardware_concurrency());
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


bool dedup_stream(
  const std::vector<std::vector<uint8_t>> &lshs,
  std::set<std::vector<uint8_t>> &hash_store /* inout */) {

  bool duplicated{false};

  for (size_t i = 0; i < lshs.size(); i++) {
    const std::vector<uint8_t> &lsh = lshs[i];

    if (hash_store.count(lsh)) {
      duplicated = true;
    } else {
      hash_store.insert(lsh);
    }
  }

  return duplicated;

}

