// SPDX-License-Identifier: Apache 2.0
#pragma once

#include <cinttypes>
#include <vector>
#include <string>

namespace exact_dedup {

///
/// Build suffix array from raw byte representation of string.
///
/// Input must be less than 2GB
///
bool build(const uint8_t *addr, size_t n, std::vector<int32_t> &sa);

///
/// Build suffix array from tokenized string(Assume vocab size < 65535(16bit ))
/// Input must be less than 2G tokens
///
///
bool build_from_tokenized(const uint16_t *addr, size_t n, std::vector<int32_t> &sa);

// TODO
//bool search(const std::string &filename, const std::string &key);
//bool dedup(const std::string &filename, const std::string &key);

} // namespace

