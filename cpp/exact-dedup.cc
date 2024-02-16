// SPDX-License-Identifier: Apache 2.0

#include <iostream>

#include "hat-trie/include/tsl/htrie_map.h"
#include "libsais.h"
#include "libsais16.h"

namespace exact_dedup {

bool build(const uint8_t *addr, const size_t n, std::vector<int32_t> &sa) {

  if (n > (std::numeric_limits<int32_t>::max)()) {
    std::cerr << "Input exceeds 2GB.\n";
    return false;
  }

  sa.resize(n);

  int32_t ret = libsais(addr, sa.data(), n, /* extra space */0, /* symbol freq */nullptr);

  if (ret < 0) {
    std::cerr << "Failed to build suffix array.\n";
    return false;
  }

#if 0
  // dbg
  for (size_t i = 0; i < sa.size(); i++) {
    std::cout << "[" << i << "] = " << sa[i] << ", " << addr[sa[i]] << "\n";
  }
#endif

  return true;
}

bool build_from_tokenized(const uint16_t *addr, const size_t n, std::vector<int32_t> &sa) {

  if (n > (std::numeric_limits<int32_t>::max)()) {
    std::cerr << "Input exceeds 2GB.\n";
    return false;
  }

  sa.resize(n);

  int32_t ret = libsais16(addr, sa.data(), n, /* extra space */0, /* symbol freq */nullptr);

  if (ret < 0) {
    std::cerr << "Failed to build suffix array.\n";
    return false;
  }

#if 0
  // dbg
  for (size_t i = 0; i < sa.size(); i++) {
    std::cout << "[" << i << "] = " << sa[i] << ", " << addr[sa[i]] << "\n";
  }
#endif

  return true;
}

} // namespace exact_dedup

