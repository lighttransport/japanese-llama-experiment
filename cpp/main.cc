
#include <algorithm>
#include <atomic>
#include <cassert>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "common.h"  // from zstd example
#include "jagger.h"
#include "json.hpp"
#include "simdjson.h"
#include "tinysegmenter.hpp"
#include "utf8proc.h"
#include "zstd.h"

#define GLOB_USE_GHC_FILESYSTEM
#include "glob.hpp"

//
#include "str-util.hh"
#include "dedup.hh"

static uint32_t cpu_count() {
  return (std::max)(1u, std::thread::hardware_concurrency());
}

static std::string wakachi(const std::string &filename) {
  size_t cSize;

  // Assume file is a UTF-8 encoded string
  void *const cBuff = mallocAndLoadFile_orDie(filename.c_str(), &cSize);

  std::string text(reinterpret_cast<const char *>(cBuff), cSize);

  return text;
}

//
// Assume `text` is UTF-8 string
// Return empty string when failed to normalize.
//
static std::string nfkc_normalize(const std::string &text) {
  utf8proc_uint8_t *ret =
      utf8proc_NFKC(reinterpret_cast<const uint8_t *>(text.c_str()));

  if (ret) {
    std::string normalized_text(reinterpret_cast<const char *>(ret));
    free(ret);
    return normalized_text;
  } else {
    return std::string();
  }
}

static std::string zstd_decompress(const char *fname) {
  size_t cSize;
  void *const cBuff = mallocAndLoadFile_orDie(fname, &cSize);
  /* Read the content size from the frame header. For simplicity we require
   * that it is always present. By default, zstd will write the content size
   * in the header when it is known. If you can't guarantee that the frame
   * content size is always written into the header, either use streaming
   * decompression, or ZSTD_decompressBound().
   */
  unsigned long long const rSize = ZSTD_getFrameContentSize(cBuff, cSize);
  CHECK(rSize != ZSTD_CONTENTSIZE_ERROR, "%s: not compressed by zstd!", fname);
  CHECK(rSize != ZSTD_CONTENTSIZE_UNKNOWN, "%s: original size unknown!", fname);

  void *const rBuff = malloc_orDie((size_t)rSize);

  /* Decompress.
   * If you are doing many decompressions, you may want to reuse the context
   * and use ZSTD_decompressDCtx(). If you want to set advanced parameters,
   * use ZSTD_DCtx_setParameter().
   */
  size_t const dSize = ZSTD_decompress(rBuff, rSize, cBuff, cSize);
  CHECK_ZSTD(dSize);
  /* When zstd knows the content size, it will error if it doesn't match. */
  CHECK(dSize == rSize, "Impossible because zstd will check this condition!");

  /* success */
  printf("%25s : %6u -> %7u \n", fname, (unsigned)cSize, (unsigned)rSize);

  // assume decoded data is utf-8 string.
  std::string buf(reinterpret_cast<const char *>(rBuff), dSize);

  free(rBuff);
  free(cBuff);

  return buf;
}

std::vector<std::string> split_lines(const std::string &s) {
  std::vector<std::string> dst;

  size_t s_begin = 0;
  size_t s_end = 0;

  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] == '\n') {
      s_end = i;
      dst.push_back(s.substr(s_begin, s_end - s_begin));
      s_begin = i + 1;
    }
  }

  return dst;
}

std::vector<nlohmann::json> decode_jsonl(std::vector<std::string> &&json_strs) {
  uint32_t nthreads = cpu_count();

  std::vector<std::thread> workers;
  std::atomic<uint64_t> i(0ull);

  std::vector<nlohmann::json> ret;
  ret.resize(json_strs.size());

  for (uint32_t t = 0; t < nthreads; t++) {
    workers.emplace_back(std::thread([&]() {
      uint64_t idx;

      while ((idx = (i++)) < json_strs.size()) {
        // simdjson::ondemand::parser parser;
        // simdjson::padded_string json_str =
        // simdjson::padded_string(jsons[idx]); simdjson::ondemand::document doc
        // = parser.iterate(json_str);

        nlohmann::json j = nlohmann::json::parse(json_strs[idx]);

        ret[idx] = std::move(j);
      }
    }));
  }

  for (auto &th : workers) {
    th.join();
  }

  return ret;
}

static std::vector<nlohmann::json> load_jsonl_zstd(
    const glob::fs::path &filepath) {
  std::string jsonl_data = zstd_decompress(filepath.c_str());

  std::vector<nlohmann::json> jsonl = decode_jsonl(split_lines(jsonl_data));

  return jsonl;
}

static bool dedup_files(const std::string &filepath, const std::string &text_key) {
  std::vector<glob::fs::path> files = glob::glob(filepath + "/*");
  std::cout << "num files: " << files.size() << "\n";

  for (const auto &f : files) {
    std::cout << f << "\n";

    std::vector<nlohmann::json> jsonl = load_jsonl_zstd(f);

    for (size_t i = 0; i < jsonl.size(); i++) {
      const auto &j = jsonl[i];

      auto lines = split_lines(j[text_key]);
      std::cout << lines.size() << "\n";
    }
  }

  return true;
}

static int test_dedup() {
  const char *in0 = "吾輩は猫である。名前はまだ無い。どこで生まれたかとんと見当がつかぬ。";
  const char *in1 = "吾輩は鳥である。名前はまだ無い。どこで生まれたかとんと見当がつかぬ。";


  auto n0 = strutil::build_ngram(in0, 5);
  auto n1 = strutil::build_ngram(in1, 5);

  LSHDedupConfig conf;

  std::vector<std::vector<uint8_t>> lsh0 = compute_lsh(
    n0, conf);

  std::vector<std::vector<uint8_t>> lsh1 = compute_lsh(
    n1, conf);


  //std::cout << strutil::byte_to_he

  return 0;
}

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cout << "Need cmd ARGS\n";
    std::cout << "  cmd:\n";
    std::cout << "    wakachi input.txt output.txt: Do wakachi-gaki for input "
                 "string\n";
    std::cout << "    normalize input_string : NFKC normalization\n";
    std::cout << "    dedup <folder> [text_key]: Text dedup. Look *.jsonl.zstd files in "
                 "<folder>. [text_key] optional. specify text tag in JSON(default `text)\n";
    std::cout << "    proc input.jsonl.zstd : proc(WIP)\n";
    std::cout << "    test <test_cmd>: Run tests\n";
    return -1;
  }

  std::string cmd = argv[1];
  if (cmd == "wakachi") {
  } else if (cmd == "normalize") {
    std::string ret = nfkc_normalize(argv[2]);
    std::cout << ret << "\n";

  } else if (cmd == "dedup") {
    std::string text_key = "text";
    if (argc > 3) {
      text_key = argv[3];
    }

    bool ret = dedup_files(argv[2],text_key);

    if (ret) {
      return 0;
    } else {
      return -1;
    }
  } else if (cmd == "test") {
    std::string suite = "text";
    if (argc > 3) {
      suite = argv[3];
    }

    if (suite == "dedup") {
      return test_dedup();
    } else {

    }

  } else {
    std::cout << "Unknown command: " << cmd << "\n";
    return -1;
  }

  return 0;
}
