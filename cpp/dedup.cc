#include "dedup.hh"

#include <algorithm>
#include <clocale>
#include <cstdint>
#include <mutex>
#include <set>
#include <thread>
#include <vector>
#include <unordered_set>

#include "rwkv_world_tokenizer_cedar.hh"

static std::unordered_set<std::string> sUNICODE_PUNCT = {
    "，", "。", "、", "„",  "”",  "“",  "«",  "»",  "１", "」",
    "「", "《", "》", "´",  "∶",  "：", "？", "！", "（", "）",
    "；", "–",  "—",  "．", "～", "’",  "…",  "━",  "〈", "〉",
    "【", "】", "％", "►",  ":",  ";",  "-",  ",",  ".",  "?",
    "!",  ")",  "(",  "<",  ">",  "[",  "]",  "\"", "'"};

std::unordered_set<uint16_t> build_punctuation_tokens(
  nanotokenizer::CedarTrieTokenizer &tokenizer) {

  std::unordered_set<uint16_t> s;

  for (const auto &p : sUNICODE_PUNCT) {
    int tok_id = tokenizer.id_from_str(p);
    if (tok_id > -1) {
      s.insert(uint16_t(tok_id));
    }
  }

  return s;
}

std::vector<uint16_t> normalize_for_dedup(const std::vector<uint16_t> &text,
  nanotokenizer::CedarTrieTokenizer &tokenizer,
  const std::unordered_set<uint16_t> &punct_table) {

  std::vector<uint16_t> dst;

  int _ws_id = tokenizer.id_from_str(" ");
  if (_ws_id < 0) {
    // ???
    std::cerr << "No whitespace token id in the tokenizer.\n";
    return dst;
  }

  uint16_t ws_id = uint16_t(_ws_id);

  // Assume input string(before tokenization) is normalized with NFKC
  //
  // 1. remove whitespaces.
  // 2. remove punctuation.

  for (size_t i = 0; i < text.size(); i++) {
    uint16_t id = i;
    if (id == ws_id) {
      continue;
    }

    if (punct_table.count(id)) {
      continue;
    }

    dst.push_back(id);
  }

  return dst;
}

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

bool dedup_stream(const std::vector<std::vector<uint8_t>> &lshs,
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
