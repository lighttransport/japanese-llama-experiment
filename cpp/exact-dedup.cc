// SPDX-License-Identifier: Apache 2.0

#include <iostream>

#include "hat-trie/include/tsl/htrie_map.h"
#include "libsais64.h"

namespace exact_dedup {

bool build(const std::string &str) {

  std::vector<int64_t> sa;
  sa.resize(str.size());
  int64_t n = int64_t(str.size());

  int64_t ret = libsais64(reinterpret_cast<const uint8_t*>(str.c_str()), sa.data(), n, /* extra space */0, /* symbol freq */nullptr);

  if (ret < 0) {
    std::cerr << "Failed to build suffix array.\n";
    return false;
  }

  // dbg
  for (size_t i = 0; i < sa.size(); i++) {
    std::cout << "[" << i << "] = " << sa[i] << ", " << str[sa[i]] << "\n";
  }

  return true;
}

} // namespace exact_dedup

