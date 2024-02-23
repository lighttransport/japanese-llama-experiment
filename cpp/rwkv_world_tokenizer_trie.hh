// SPDX-License-Identifier: Apache 2.0

/*!
 *  Copyright (c) 2023 by Contributors
 *
 *  Modification by (C) 2024 - Present, Light Transport Entertainment Inc.
 * \file rwkv_world_tokenizer.cpp
 * \brief Implementation of llm chat.
 */
#pragma once

#include <map>
#include <unordered_map>
#include <memory>
#include <fstream>
#include <sstream>
#include <vector>

#if defined(RWKV_ENABLE_EXCEPTION)
#include <exception>
#include <stdexcept>
#endif
#include <sstream>
#include <string>

#define STRINGIFY(...) STRINGIFY_(__VA_ARGS__)
#define STRINGIFY_(...) #__VA_ARGS__

#if defined(RWKV_ENABLE_EXCEPTION)

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfor-loop-analysis"
#endif

#define RV_CHECK(...) {      \
  bool _rv_check_status = (__VA_ARGS__); \
  if (!_rv_check_status) {             \
    FRException() << ("Check \"" STRINGIFY(__VA_ARGS__) "\" failed at " + \
                            std::to_string(__LINE__) + " in " __FILE__ "\n  > Error msg: "); \
  } \
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace nanotokenizer {

struct FRException : public std::runtime_error {
  FRException() : std::runtime_error("") {}
  const char* what() const noexcept override { return msg.c_str(); }
  template <typename T>
  FRException& operator<<(const T& s) {
    std::stringstream ss;
    ss << s;
    msg += ss.str();
    return *this;
  }
  std::string msg;
};

} // nanotokenizer
#else

#define RV_CHECK(...)                                                         \
  for (bool _rv_check_status = (__VA_ARGS__); !_rv_check_status;)             \
  _err_ss << ("Check \"" STRINGIFY(__VA_ARGS__) "\" failed at " + \
                          std::to_string(__LINE__) + " in " __FILE__ "\n  > Error msg: ")
#endif


namespace nanotokenizer {

struct TrieTree {
  std::unordered_map<int, std::unique_ptr<TrieTree>> children;
  std::string word;
  int token_id{-1}; // -1 = invalid

  TrieTree(const std::unordered_map<std::string, int>& word2id) {
    for (auto& pair : word2id) {
      add_word(pair.first, pair.second);
    }
  }

  std::pair<std::string, int> find_longest_prefix(const char *s, const size_t s_offset, const size_t s_len) const {
    std::string prefix;
    int token_id = -1;
    const TrieTree* node = this;
    for (int i = s_offset; i < s_len; ++i) {
      auto it = node->children.find(s[i]);
      if (it == node->children.end()) {
        break;
      }
      node = it->second.get();
      if (!node) {
        // invalid
        return {"", -1};
      }
      if (node->token_id >= 0) {
        prefix = node->word;
        token_id = node->token_id;
      }
    }
    if (prefix.empty()) {
      return {"", -1};
    }
    if (token_id == -1) {
      return {"", -1};
    }
    return {prefix, token_id};
  }

 private:
  TrieTree() = default;
  void add_word(const std::string& word, int token_id) { return _add_word(word, token_id, 0); }
  void _add_word(const std::string& word, int token_id, int idx) {
    if (idx == word.size()) {
      this->word = word;
      this->token_id = token_id;
      return;
    }
    auto& child = children[word[idx]];
    if (!child) {
      child = std::unique_ptr<TrieTree>(new TrieTree());
    }
    child->_add_word(word, token_id, idx + 1);
  }
  std::stringstream _err_ss;
};

class TrieTokenizer {
 public:
  TrieTokenizer() = default;

  bool load_vocab(const std::map<std::string, int>& word2idx, std::string &err) {
    for (auto& pair : word2idx) {
      if (pair.first.empty()) {
        _empty_str_id = pair.second;
        continue;
      }

      if ((pair.second > 127) && (pair.second < 257)) {
        // reserved for UTF-8 byte fallback
        continue;
      }

      _word2idx[pair.first] = pair.second;
      _idx2word[pair.second] = pair.first;
    }
    _tree = std::make_unique<TrieTree>(_word2idx);

    return true;
  }

  bool encode(const std::string &str, std::vector<int32_t> &dst) {
    std::vector<int> ids;
    int str_idx = 0;
    size_t str_len = str.size();

    if (str_len == 0) {
      return true;
    }

    while (str_idx < str_len) {
      const auto ret = _tree->find_longest_prefix(str.c_str(), str_idx, str_len);
      if (ret.first.empty()) {
        // UTF-8 byte fallback
        // Should be single UTF-8 character
        
        int char_len = utf8_len(str[str_idx]);
        for (size_t c = 0; c < char_len; c++) {
          ids.push_back(int(uint8_t(str[str_idx + c])) +
                        _utf8_id_offset);
        }
        str_idx += char_len;
      } else {
        ids.push_back(ret.second);
        str_idx += ret.first.size();
      }
    }
    dst = ids;
    return true;
  }

  bool decode(const std::vector<int32_t>& ids, std::string &dst) {
    std::string str;
    for (size_t i = 0; i < ids.size(); i++) {

      if ((ids[i] >= (127 + _utf8_id_offset)) && (ids[i] < (256 + _utf8_id_offset))) {

        std::string u8char;
        if (!utf8_char_from_ids(ids.data(), i, ids.size(),
                                u8char, _utf8_id_offset)) {
          return false;
        }

        i += u8char.size() - 1;
        str += u8char;
        continue;
      }

      std::string s = IdToToken(ids[i]);
      if (s.empty()) {
        return false;
      }
      str += s;
    }
    dst = str;
    return true;
  }

  size_t GetVocabSize() {
    auto size = _idx2word.size();
    RV_CHECK(size > 0);
    return size;
  }

  virtual std::string IdToToken(int32_t token_id) {
    RV_CHECK(_idx2word.size() > 0);
    auto it = _idx2word.find(token_id);
    if (it == _idx2word.end()) {
      return "";
    } else {
      return it->second;
    }
  }

  int32_t TokenToId(const std::string& token) {
    RV_CHECK(_word2idx.size() > 0);
    auto it = _word2idx.find(token);
    if (it == _word2idx.end()) {
      return -1;
    } else {
      return it->second;
    }
  }

 private:
  
  int _empty_str_id{0};

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
  std::stringstream _err_ss;

  // the tokenizer
  std::unordered_map<std::string, int> _word2idx;
  std::unordered_map<int, std::string> _idx2word;
  std::unique_ptr<TrieTree> _tree;

  int _utf8_id_offset{1};

};

}  // namespace nanotokenizer
