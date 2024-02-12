// SPDX-License-Identifier: MIT

#pragma once

#include <fstream>
#include <iostream>
#include <map>

#include "hat-trie/include/tsl/htrie_map.h"

namespace nanotokenizer {

// Up to 65534 vocabs
class TrieTokenizer
{
 public:

  bool load_vocab(const std::map<std::string, int> &str_to_id_map) {
    _str_to_id_map = str_to_id_map;

    // Ensure 128 ~ 256 is not used(reserved for UTF-8 byte fallback).
    int max_id{0};
    for (const auto &it : str_to_id_map) {
      if ((it.second >= 128) && (it.second <= 256)) {
        return false;
      }
      _id_to_str_map[it.second] = it.first;
      max_id = (std::max)(max_id, it.second);
    }

    for (const auto &it : str_to_id_map) {
      _trie_map[it.first] = it.second;
    }

    _utf8_fallback_token_id = max_id + 1;
    if (_utf8_fallback_token_id > 65535) {
      return false;
    }
    _utf8_id_offset = 1; // ASCII character is +1'ed in RWKV world vocab

    return true;
  }

  bool encode(const std::string &_input_str, std::vector<int> &output_ids) {
    std::vector<int> dst;

    std::string buf = _input_str;

    while (!buf.empty()) {

      auto longest_prefix = _trie_map.longest_prefix(buf);
      // 3319 = empty string.
      if ((longest_prefix != _trie_map.end()) && !longest_prefix.key().empty()) {
        dst.push_back(*longest_prefix);

        buf.erase(0, longest_prefix.key().size());
      } else {
        int u8len{0};
        std::string u8char = extract_utf8_char(buf, 0, u8len);
        if (u8len == 0) {
          std::cerr << "invalid utf8 char found.\n";
          exit(-1);
        }

        dst.push_back(_utf8_fallback_token_id);

        for (size_t i = 0; i < u8char.size(); i++) {
          dst.push_back(int(uint8_t(u8char[i])) + _utf8_id_offset);
        }
        buf.erase(0, u8len);
      }

    }

    output_ids = dst;
    return true;
  }

  bool decode(const std::vector<int> input_ids, std::string &output_str) {

    std::string dst;

    for (size_t i = 0; i < input_ids.size(); i++) {
      if (input_ids[i] == _utf8_fallback_token_id) {
        std::string u8char;
        if (!utf8_char_from_ids(input_ids.data(), i+1, input_ids.size(), u8char, _utf8_id_offset)) {
          std::cerr << "utf8 reconstruct failed.\n";
          return false;
        }

        i += u8char.size();

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

  std::map<std::string, int> _str_to_id_map;
  std::map<int, std::string> _id_to_str_map;

  int _utf8_fallback_token_id{-1};
  int _utf8_id_offset{1}; // ASCII character is +1'ed in RWKV world vocab

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
  inline bool utf8_char_from_ids(const int *addr, size_t loc, size_t n, std::string &str, int id_offset = 1) {
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

#if 0
int main(int argc, char **argv) {
  std::string vocab_json_filename = "rwkv_vocab_v20230424.json";

  if (argc > 1) {
    vocab_json_filename = argv[1];
  }

  std::ifstream ifs(vocab_json_filename);
  if (!ifs) {
    std::cerr << "file not found or open file failed: " << vocab_json_filename
              << "\n";
    return -1;
  }

  std::stringstream buf;
  buf << ifs.rdbuf();
  std::string s = buf.str();

  if (s.size() < 2) {
    std::cerr << "Invalid JSON file or file is invalid(maybe directory or "
                 "special device?): "
              << vocab_json_filename << "\n";
    return -1;
  }

  const char *startp = s.c_str();
  const char *p = s.c_str();

  minijson::value v;
  minijson::error e = minijson::parse(p, v);
  if (e != minijson::no_error) {
    std::cerr << minijson::errstr(e) << " at byte offset (" << int(p - startp)
              << "):" << p << std::endl;
    return -1;
  }

  std::cout << "Read vocab OK: " << vocab_json_filename << "\n";

  std::map<std::string, int> str_to_id_map;

  int max_id = 0;
  // key = string, value = id
  if (const auto *po = v.as<minijson::object>()) {
    // id 0(<endoftext>) is not included in JSON. so add +1
    std::cout << "nvocab = " << po->size() + 1 << "\n";
    for (size_t i = 0; i < po->size(); i++) {
      std::string key = po->keys()[i];

      minijson::value num_v;
      if (!po->at(key, &num_v)) {
        std::cerr << "Invalid JSON. value for `" << key
                  << "` not found: " << vocab_json_filename << "\n";
        return -1;
      }

      if (const auto *pv = num_v.as<minijson::number>()) {
        int id = int(*pv);
        if ((id < 0) || (id > 65535)) {
          std::cerr << "Invalid id value for `" << key
                    << "`. must be in range[0, 65536) but got " << id << "\n";
          return -1;
        }
        str_to_id_map[key] = id;
        max_id = (std::max)(id, max_id);
      } else {
        std::cerr << "Invalid JSON. value is not number type for `" << key
                  << "` : " << vocab_json_filename << "\n";
        return -1;
      }
    }
    std::cout << "max id value = " << max_id << "\n";

  } else {
    std::cerr << "Invalid JSON. Root element must be object: "
              << vocab_json_filename << "\n";
    return -1;
  }

  TrieTokenizer tokenizer;

  if (!tokenizer.load_vocab(str_to_id_map)) {
    std::cerr << "Vocab seems too large(65535 or more): "
              << vocab_json_filename << "\n";
    return -1;
  }

  // encode UTF-8 string
  std::string input_str = u8"吾輩は猫である。🤩";
  std::cout << "input: " << input_str << "\n";

  std::vector<int> output_ids;

  if (!tokenizer.encode(input_str, output_ids)) {
    std::cerr << "encode failed.\n";
    return -1;
  }

  std::cout << "ids = [";
  for (size_t i = 0; i < output_ids.size(); i++) {
    if (i > 0) {
      std::cout << ", ";
    }
    std::cout << output_ids[i];
  }
  std::cout << "]\n";

  std::string output_str;
  if (!tokenizer.decode(output_ids, output_str)) {
    std::cerr << "decode failed.\n";
    return -1;
  }
  std::cout << "decoded: " << output_str << "\n";

  return EXIT_SUCCESS;
}
#endif
