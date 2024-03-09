// SPDX-License-Identifier: Apache 2.0
// Copyright 2024 - Present, Light Transport Entertainment, Inc.
#include <fstream>
#include <iostream>
#include <map>
#include <unordered_map>

#include "cedar.h"
#include "ccedar_core.h"

namespace nanotokenizer {

// Trie tokenizer based on ccedar
class CedarTrieTokenizer {
 public:
  static constexpr size_t MAX_KEY_BITS = 16;  // 65536

  using trie_t = cedar::da<int>; // Key = UTF-8 bytes
  using itrie_t = ccedar::da<int, int, MAX_KEY_BITS>; // Key = UTF codepoint(int value)

  CedarTrieTokenizer() = default;
  explicit CedarTrieTokenizer(bool use_codepoint) : _use_codepoint(use_codepoint) {}
  ~CedarTrieTokenizer() {
    // free memory in cedar
    if (_use_codepoint) {
      _ida.clear(/* reuse */false);
    } else {
      _cda.clear(/* reuse */ false);
    }
  }

  bool load_vocab(const std::map<std::string, int> &str_to_id_map, std::string &err) {

    int max_id{0};
    for (const auto &it : str_to_id_map) {
      // ignore empty key(zero-length char).
      if (it.first.empty()) {
        _empty_char_id = it.second;
      } else if (it.second == 0) {
        err += "Vocab ID 0 is not allowed.\n";
        return false;
      } else if ((it.second > 127) && (it.second < 257)) {
        // reserved for UTF-8 byte fallbacl
        continue;
      } else {
        _id_to_str_map[it.second] = it.first;
      }
      max_id = (std::max)(max_id, it.second);

      _str_to_id_map[it.first] = it.second;
    }

    if (max_id > 65535) {
      err += "Vocab ID exceeds 65535\n";
      return false;
    }
    _utf8_id_offset = 1;  // ASCII character is +1'ed in RWKV world vocab

    if (_use_codepoint) {
      for (const auto &it : str_to_id_map) {
        const char *str = it.first.c_str();
        const size_t slen = strlen(str);

        // cedar does not accept empty char(zero-length char).
        if (slen == 0) {
          continue;
        }

        // UTF-8 string to int(unicode) array
        std::vector<int> ikey;

        //std::cout << "str: " << it.first << ", id: " << it.second << "\n";
        int charlen{0};
        for (size_t i = 0; i < slen; i += charlen) {
          int code = to_codepoint(it.first.c_str() + i, charlen);
          ikey.push_back(code);
        }

        _ida.update(ikey.data(), ikey.size(), it.second);
      }
    } else {
      for (const auto &it : str_to_id_map) {
        const char *str = it.first.c_str();
        const size_t slen = strlen(str);

        // cedar does not accept empty char(zero-length char).
        if (slen == 0) {
          continue;
        }

        _cda.update(str, slen, it.second);
      }
    }

    return true;
  }

  bool encode(const std::string &s, std::vector<int> &output_ids) {

    std::vector<int> dst;

    if (s.empty()) {
      return true;
    }

    const size_t s_len = s.size();

    for (size_t i = 0; i < s_len;) {

      uint32_t char_len = utf8_len(s[i]);
      if (char_len == 0) {
        // Found invalid UTF-8 string.
        return false;
      }

      int32_t token_id;
      uint32_t key_size;

      int ret;
      if (_use_codepoint) {
        ret = _ilongestPrefixSearch(s.c_str(), i, s_len, token_id, key_size);
      } else {
        ret = _longestPrefixSearch(s.c_str(), i, s_len, token_id, key_size);
      }

      if (ret) {
        dst.push_back(token_id);
        i += key_size;
      } else {

        // UTF-8 byte fallback
        // Should be single UTF-8 character

        for (size_t c = 0; c < char_len; c++) {
          dst.push_back(int(uint8_t(s[i + c])) +
                        _utf8_id_offset);
        }
        i += char_len;
      }
    }

    output_ids = dst;
    return true;
  }

  bool decode(const std::vector<int> &input_ids, std::string &output_str) const {
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

      dst += _id_to_str_map.at(input_ids[i]);
    }

    output_str = dst;

    return true;
  }

  bool decode(const std::vector<uint16_t> &input_ids, std::string &output_str) const {
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

      dst += _id_to_str_map.at(input_ids[i]);
    }

    output_str = dst;

    return true;
  }


  std::string str_from_id(int id) const {
    if (_id_to_str_map.count(id)) {
      return _id_to_str_map.at(id);
    }
    if (id > 0 && id < 257) {  // ASCII or UTF-8 byte
      return "[[byte]]";
    }
    return std::string();
  }

  int id_from_str(const std::string &s) const {
    if (_str_to_id_map.count(s)) {
      return _str_to_id_map.at(s);
    }
    return -1;
  }

 private:
  itrie_t _ida; // int key
  trie_t _cda; // char key

  bool _longestPrefixSearch(const char *s, const size_t s_offset, const size_t s_len, int &found_id, uint32_t &keylen) {

    size_t prev_from{0};
    int prev_n{-1};

    size_t from{0};
    for (size_t i = s_offset; i < s_len; i++) {
      size_t pos = 0;
      // process 1 char each.
      int n = _cda.traverse(&s[i], from, /* inout */pos, /* len */1);
      if (n == trie_t::CEDAR_NO_VALUE) {
        continue;
      }
      if (n == trie_t::CEDAR_NO_PATH) {
        break;
      }

      prev_n = n;

      if (prev_from == from) {
        // guess exactMatch.
        break;
      }
      prev_from = from;
    }

    if ((prev_n > 0) && _id_to_str_map.count(prev_n)) {
      found_id = prev_n;
      keylen = uint32_t(_id_to_str_map.at(prev_n).size());
      return true;
    }

    return false;
  }

  bool _ilongestPrefixSearch(const char *s, const size_t s_offset, const size_t s_len, int &found_id, uint32_t &keylen) {

    size_t prev_from{0};
    int prev_n{-1};

    size_t from{0};
    int char_len{0};
    for (size_t i = s_offset; i < s_len; i += char_len) {
      size_t pos = 0;

      int code = to_codepoint(&s[i], char_len);
      if (char_len == 0) {
        std::cerr << "charlen is 0.\n";
        // invalid
        return false;
      }

      // process codepoint value each.
      int n = _ida.traverse(&code, from, /* inout */pos, /* len */1);

      if (n == trie_t::CEDAR_NO_VALUE) {
        continue;
      }

      if (n == trie_t::CEDAR_NO_PATH) {
        break;
      }

      prev_n = n;

      if (prev_from == from) {
        // guess exactMatch.
        break;
      }
      prev_from = from;
    }

    if ((prev_n > 0) && _id_to_str_map.count(prev_n)) {
      found_id = prev_n;
      keylen = uint32_t(_id_to_str_map.at(prev_n).size());
      return true;
    }

    return false;
  }

  bool _use_codepoint{false}; // Use Unicode codepoint to represent string instead of UTF-8 byte?
  std::unordered_map<std::string, int> _str_to_id_map;
  std::unordered_map<int, std::string> _id_to_str_map;

  int _utf8_id_offset{1};  // ASCII character is +1'ed in RWKV world vocab
  int _empty_char_id{3319};

  inline uint32_t utf8_len(const uint8_t c) const {
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
                                 std::string &str, int id_offset = 1) const {
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

  // Reconstruct UTF-8 bytes from int sequence(UTF-8 encoded)
  inline bool utf8_char_from_ids(const uint16_t *addr, size_t loc, size_t n,
                                 std::string &str, int id_offset = 1) const {
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

  uint32_t to_codepoint(const char *s, int &len) {
    if (!s) {
      return ~0u;
    }

    uint32_t char_len = utf8_len(*s);

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
};

} // namespace nanotokenizer
