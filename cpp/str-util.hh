// SPDX-License-Identifier: MIT
// Copyright 2023 - Present, Light Transport Entertainment Inc.

#pragma once

#include <vector>
#include <string>
#include <array>
#include <cassert>
#include <algorithm>

#include "stack_container.h"

namespace strutil {

inline std::string extract_utf8_char(const std::string &str, uint32_t start_i,
                                     int &len) {
  len = 0;

  if ((start_i + 1) > str.size()) {
    len = 0;
    return std::string();
  }

  unsigned char c = static_cast<unsigned char>(str[start_i]);

  if (c <= 127) {
    // ascii
    len = 1;
    return str.substr(start_i, 1);
  } else if ((c & 0xE0) == 0xC0) {
    if ((start_i + 2) > str.size()) {
      len = 0;
      return std::string();
    }
    len = 2;
    return str.substr(start_i, 2);
  } else if ((c & 0xF0) == 0xE0) {
    if ((start_i + 3) > str.size()) {
      len = 0;
      return std::string();
    }
    len = 3;
    return str.substr(start_i, 3);
  } else if ((c & 0xF8) == 0xF0) {
    if ((start_i + 4) > str.size()) {
      len = 0;
      return std::string();
    }
    len = 4;
    return str.substr(start_i, 4);
  } else {
    // invalid utf8
    len = 0;
    return std::string();
  }
}

inline std::vector<std::string> to_utf8_chars(const std::string &str) {
  uint64_t sz = str.size();
  std::vector<std::string> utf8_chars;

  for (size_t i = 0; i <= sz;) {
    int len = 0;
    std::string s = strutil::extract_utf8_char(str, uint32_t(i), len);
    if (len == 0) {
      // invalid char
      break;
    }

    i += uint64_t(len);
    utf8_chars.push_back(s);
  }

  return utf8_chars;
}

//using UTF8Char = StackVector<char, 4>; // Usually less than 4 bytes.

// TODO: Use UTFChar type.
using NGram = StackVector<std::string, 32>;

//
// Build N-gram
//
// vector of (utf-8 char x N)
//
inline std::vector<NGram> build_ngram(
    const std::string &str, const uint32_t N) {
  std::vector<NGram> ret;

  std::vector<std::string> utf8_chars = to_utf8_chars(str);

  if (utf8_chars.size() < N) {
    // empty
    return ret;
  }

  size_t nitems = utf8_chars.size() - N + 1;

  for (size_t i = 0; i < nitems; i++) {
    assert((i + N) <= utf8_chars.size());

    size_t iend = std::min(i + N, utf8_chars.size());
    size_t nchars = iend - i;

    NGram gram;

    for (size_t k = 0; k < N; k++) {
      gram->push_back(utf8_chars[i + k]);
    }

    ret.emplace_back(std::move(gram));
  }

  return ret;
}

inline std::vector<uint8_t> ngram_to_bytes(const NGram &ngram) {
  std::vector<uint8_t> data;

  for (size_t i = 0; i < ngram->size(); i++) {
    std::transform(ngram[i].begin(), ngram[i].end(), std::back_inserter(data), [](const char &a) {
      return uint8_t(a);
    });
  }

  return data;
}

}  // namespace strutil


