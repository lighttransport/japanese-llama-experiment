// SPDX-License-Identifier: Apache 2.0
// Copyright 2024 - Present, Light Transport Entertainment, Inc.
#pragma once

#include <fstream>
#include <iostream>
#include <map>
#include <unordered_map>

#include "hat-trie/include/tsl/htrie_map.h"

namespace nanotokenizer {

// hat-trie version of Tokenizer.
// Up to 65535 vocab id
// - token id 0 is reserved for empty(zero)
// - token ids in [127, 256] are reserved for UTF-8 byte fallback(+1'ed)
class HatTrieTokenizer {
 public:
  bool load_vocab(const std::map<std::string, int> &str_to_id_map, std::string &err) {

    int max_id{0};
    for (const auto &it : str_to_id_map) {
      if (it.second == 0) {
        err += "vocab with id 0 is not allowed.\n";
        return false;
      }
      // reserved for UTF-8 byte fallback
      if ((it.second >= 127) && (it.second <= 256)) {
        continue;
      }
      _str_to_id_map[it.first] = it.second;
      _id_to_str_map[it.second] = it.first;
      max_id = (std::max)(max_id, it.second);
    }

    for (const auto &it : str_to_id_map) {
      _trie_map[it.first] = it.second;
    }

    if (max_id > 65535) {
      err += "Max vocab id exceeds 65535\n";
      return false;
    }
    _utf8_id_offset = 1;  // ASCII character is +1'ed in RWKV world vocab

    return true;
  }

  bool encode(const std::string &_input_str, std::vector<int> &output_ids) {
    std::vector<int> dst;

    const size_t s_len = _input_str.size();

    if (s_len == 0) {
      // empty input
      return false;
    }

    size_t char_idx = 0;
    int prev_id = -1;  // Track previously matched result.
    size_t key_size = 0;

    // Find match for each UTF-8 character,
    // Since `longest_prefix` is quite slow for larger input string.

    while ((char_idx + key_size) < s_len) {
      // Extract UTF-8 char.
      uint32_t charlen = utf8_len(_input_str[char_idx]);
      if (charlen == 0) {
        // Found invalid UTF-8 string.
        return false;
      }

      key_size += charlen;

      auto it = _trie_map.find_ks(&_input_str[char_idx], key_size);
      if (it == _trie_map.cend()) {
        if (prev_id > 0) {
          // prev_id = id of longest matched key
          dst.push_back(prev_id);

          // pop current UTF-8 character.
          key_size -= charlen;

        } else {
          // UTF-8 byte fallback
          // Should be single UTF-8 character
          if (key_size != charlen) {
            // This should not happen. Just in case.
            return false;
          }

          for (size_t i = 0; i < charlen; i++) {
            dst.push_back(int(uint8_t(_input_str[char_idx + i])) +
                          _utf8_id_offset);
          }
        }

        prev_id = -1;

        char_idx += key_size;
        key_size = 0;
      } else {
        prev_id = *(it);

        // Continue search
      }
    }

    // Remainder
    if (prev_id) {
      dst.push_back(prev_id);
    }

    output_ids = dst;
    return true;
  }

  bool decode(const std::vector<int> input_ids, std::string &output_str) {
    std::string dst;

    for (size_t i = 0; i < input_ids.size(); i++) {
      if ((input_ids[i] > 0) && (input_ids[i] < (256 + _utf8_id_offset))) {
        std::string u8char;
        if (!utf8_char_from_ids(input_ids.data(), i, input_ids.size(),
                                u8char, _utf8_id_offset)) {
          std::cerr << "utf8 reconstruct failed.\n";
          return false;
        }

        i += u8char.size() - 1;

        dst += u8char;

        continue;
      }

      if (!_id_to_str_map.count(input_ids[i])) {
        std::cerr << "id not found: " << input_ids[i] << "\n";
        return false;
      }

      dst += _id_to_str_map[input_ids[i]];
    }

    output_str = dst;

    return true;
  }

 private:
  // We can use uint16_t as value type.
  tsl::htrie_map<char, int> _trie_map;

  std::unordered_map<std::string, int> _str_to_id_map;
  std::unordered_map<int, std::string> _id_to_str_map;

  int _utf8_id_offset{1};  // ASCII character is +1'ed in RWKV world vocab

  inline uint32_t utf8_len(const uint8_t c) {
    if (c <= 127) {
      // ascii
      return 1;
    } else if ((c & 0xE0) == 0xC0) {
      return 2;
    } else if ((c & 0xF0) == 0xE0) {
      return 3;
    } else if ((c & 0xF8) == 0xF0) {
      return 4;
    }

    // invalid
    return 0;
  }

  // Reconstruct UTF-8 bytes from int sequence(UTF-8 encoded)
  inline bool utf8_char_from_ids(const int *addr, size_t loc, size_t n,
                                 std::string &str, int id_offset = 1) {
    if (loc >= n) {
      return false;
    }

    int start_c = addr[loc] - id_offset;
    if ((start_c < 0) || (start_c > 255)) {
      return false;
    }

    uint32_t len = utf8_len(uint8_t(start_c));

    if (len == 0) {
      return false;
    }

    if ((loc + len) > n) {
      return false;
    }

    str = "";
    std::vector<uint8_t> buf;
    for (size_t i = 0; i < len; i++) {
      int ic = addr[loc + i] - id_offset;
      if ((ic < 0) || (ic > 255)) {
        return false;
      }
      buf.push_back(uint8_t(ic));
    }

    str = std::string(reinterpret_cast<const char *>(buf.data()), buf.size());

    return true;
  }

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
};

} // namespace nanotokenizer
