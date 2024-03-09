// SPDX-License-Identifier: MIT
// Copyright 2023 - Present, Light Transport Entertainment Inc.

#pragma once

#include <vector>
#include <string>
#include <array>
#include <cassert>
#include <algorithm>
#include <sstream>
#include <iomanip>

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

inline uint8_t utf8_len(char _c) {
  uint8_t c = uint8_t(_c);
 
  if (c <= 127) {
    return 1;
  } else if ((c & 0xE0) == 0xC0) {
    return 2;
  } else if ((c & 0xF0) == 0xE0) {
    return 3;
  } else if ((c & 0xF8) == 0xF0) {
    return 4;
  } else {
    // invalid code-point
    return 0;
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

// return empty string for invalid codepoint value.
inline std::string codepoint_to_utf8(uint32_t cp) {
    std::string result;
    if (cp <= 0x7f) {
        result.push_back(cp);
    }
    else if (0x80 <= cp && cp <= 0x7ff) {
        result.push_back(0xc0 | ((cp >> 6) & 0x1f));
        result.push_back(0x80 | (cp & 0x3f));
    }
    else if (0x800 <= cp && cp <= 0xffff) {
        result.push_back(0xe0 | ((cp >> 12) & 0x0f));
        result.push_back(0x80 | ((cp >> 6) & 0x3f));
        result.push_back(0x80 | (cp & 0x3f));
    }
    else if (0x10000 <= cp && cp <= 0x10ffff) {
        result.push_back(0xf0 | ((cp >> 18) & 0x07));
        result.push_back(0x80 | ((cp >> 12) & 0x3f));
        result.push_back(0x80 | ((cp >> 6) & 0x3f));
        result.push_back(0x80 | (cp & 0x3f));
    }

    return result;
}

    inline uint32_t to_codepoint(const char *s, uint32_t &len) {
      if (!s) {
        return ~0u;
      }

      uint32_t char_len = utf8_len(uint8_t(*s));

      uint32_t code = 0;
      if (char_len == 1) {
        unsigned char s0 = static_cast<unsigned char>(s[0]);
        if (s0 > 0x7f) {
          len = 0;
          return ~0u;
        }
        code = uint32_t(s0) & 0x7f;
      } else if (char_len == 2) {
        // 11bit: 110y-yyyx 10xx-xxxx
        unsigned char s0 = static_cast<unsigned char>(s[0]);
        unsigned char s1 = static_cast<unsigned char>(s[1]);

        if (((s0 & 0xe0) == 0xc0) && ((s1 & 0xc0) == 0x80)) {
          code = (uint32_t(s0 & 0x1f) << 6) | (s1 & 0x3f);
        } else {
          len = 0;
          return ~0u;
        }
      } else if (char_len == 3) {
        // 16bit: 1110-yyyy 10yx-xxxx 10xx-xxxx
        unsigned char s0 = static_cast<unsigned char>(s[0]);
        unsigned char s1 = static_cast<unsigned char>(s[1]);
        unsigned char s2 = static_cast<unsigned char>(s[2]);
        if (((s0 & 0xf0) == 0xe0) && ((s1 & 0xc0) == 0x80) &&
            ((s2 & 0xc0) == 0x80)) {
          code =
              (uint32_t(s0 & 0xf) << 12) | (uint32_t(s1 & 0x3f) << 6) | (s2 & 0x3f);
        } else {
          len = 0;
          return ~0u;
        }
      } else if (char_len == 4) {
        // 21bit: 1111-0yyy 10yy-xxxx 10xx-xxxx 10xx-xxxx
        unsigned char s0 = static_cast<unsigned char>(s[0]);
        unsigned char s1 = static_cast<unsigned char>(s[1]);
        unsigned char s2 = static_cast<unsigned char>(s[2]);
        unsigned char s3 = static_cast<unsigned char>(s[3]);
        if (((s0 & 0xf8) == 0xf0) && ((s1 & 0xc0) == 0x80) &&
            ((s2 & 0xc0) == 0x80) && ((s2 & 0xc0) == 0x80)) {
          code = (uint32_t(s0 & 0x7) << 18) | (uint32_t(s1 & 0x3f) << 12) |
                 (uint32_t(s2 & 0x3f) << 6) | uint32_t(s3 & 0x3f);
        } else {
          len = 0;
          return ~0u;
        }
      } else {
        len = 0;
        return ~0u;
      }

      len = char_len;
      return code;
    }


inline std::vector<uint32_t> to_utf8_codepoints(const std::string &str) {
  uint64_t sz = str.size();
  std::vector<uint32_t> utf8_chars;

  for (size_t i = 0; i <= sz;) {

    uint32_t len;
    uint32_t codepoint = to_codepoint(&str[i], len);

    i += uint64_t(len);
    utf8_chars.push_back(codepoint);
  }

  return utf8_chars;
}

// N-Gram representation with fixed buffer size.
template<uint32_t N>
struct NGram {

  // Assume UTF-8 char is representable within 4 bytes.
  // TODO: Consider UTF-8 char with 5 or more bytes(e.g. emoji)
  uint8_t charbuf[4 * N] = {}; 
  uint8_t charlen[N] = {}; // 1, 2, 3 or 4.
  uint32_t nchars{0};
  uint32_t nbytes{0};

  bool add_utf8_char(const char *s) {
    if (!s) {
      return false;
    }

    uint32_t len = utf8_len(s[0]);
    memcpy(&charbuf[nbytes], s, len);
    
    nbytes += len;

    return true;
  }

  const uint8_t *buffer() const {
    return &charbuf[0];
  }

  uint32_t n_bytes() const {
    return nbytes;
  }

  std::string str() const {
    std::string ret(reinterpret_cast<const char *>(buffer()), n_bytes());
    return ret;
  }
};


//using UTF8Char = StackVector<char, 4>; // Usually less than 4 bytes.

// TODO: Use UTFChar type.
//using NGram = StackVector<std::string, 32>;

//
// Build N-gram
//
// vector of (utf-8 char x N)
//
template<uint32_t N>
inline std::vector<NGram<N>> build_ngram(
    const std::string &str) {
  std::vector<NGram<N>> ret;

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

    NGram<N> gram;

    for (size_t k = 0; k < nchars; k++) {
      gram.add_utf8_char(utf8_chars[i + k].c_str());
    }

    ret.emplace_back(std::move(gram));
  }

  return ret;
}

#if 0
inline std::vector<uint8_t> ngram_to_bytes(const NGram &ngram) {
  std::vector<uint8_t> data;

  for (size_t i = 0; i < ngram->size(); i++) {
    std::transform(ngram[i].begin(), ngram[i].end(), std::back_inserter(data), [](const char &a) {
      return uint8_t(a);
    });
  }

  return data;
}
#endif

inline std::string byte_to_hex_string(const std::vector<uint8_t> &bytes) {
  std::stringstream ss;

  ss << "0x";

  // TODO: optimize hex print.
  // e.g.: https://github.com/zbjornson/fast-hex/tree/master
  for (auto b : bytes) {
    ss << std::setfill('0') << std::right << std::setw(2) << std::hex << int16_t(b);
  }

  return ss.str();
}

inline std::string byte_to_hex_string(const char *addr, size_t nbytes) {
  std::stringstream ss;

  ss << "0x";

  // TODO: optimize hex print.
  // e.g.: https://github.com/zbjornson/fast-hex/tree/master
  for (size_t i = 0; i < nbytes; i++) {
    ss << std::setfill('0') << std::right << std::setw(2) << std::hex << uint16_t(*reinterpret_cast<const uint8_t *>(&addr[i]));
  }

  return ss.str();
}

}  // namespace strutil


