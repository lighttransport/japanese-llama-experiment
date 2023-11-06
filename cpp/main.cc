//
// TODO: 
// - [ ] Use fully streaming processing approach to save memory usage
//       (zstd decode, json decode, process task, json encode, zstd encode)
// - [ ] Efficient dedup by creating folder per MSB and use sorting to save memory.
//
#include <algorithm>
#include <atomic>
#include <cassert>
#include <iostream>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>
#include <cstring>

#include "common.h"  // from zstd example
#include "jagger.h"
#include "json.hpp"
#include "simdjson.h"
#include "tinysegmenter.hpp"
#include "utf8proc.h"
#include "./zstd.h"
#include "chromiumbase64.h"

#define GLOB_USE_GHC_FILESYSTEM
#include "glob.hpp"

//
#include "dedup.hh"
#include "str-util.hh"
#include "pbar.hpp"

#define N_GRAM 5
#define B_BYTES 2 // this should be 2
#define N_BUCKETS 20 // this should be 20

#define BUCKET_SIZE 10 // 10 = The Pile. 450 = RefinedWeb

static uint32_t cpu_count() {
  return (std::max)(1u, std::thread::hardware_concurrency());
}

static std::string to_base64(const std::vector<uint8_t> &bytes) {
  size_t len = chromium_base64_encode_len(bytes.size());

  std::vector<char> dst;
  dst.resize(len);

  size_t n = chromium_base64_encode(dst.data(), reinterpret_cast<const char *>(bytes.data()), bytes.size());
  if (n == MODP_B64_ERROR) {
    return std::string();
  }

  return std::string(dst.data(), n);
}

static std::string to_base64(const char *addr, size_t nbytes) {
  size_t len = chromium_base64_encode_len(nbytes);

  std::vector<char> dst;
  dst.resize(len);

  size_t n = chromium_base64_encode(dst.data(), addr, nbytes);
  if (n == MODP_B64_ERROR) {
    return std::string();
  }

  return std::string(dst.data(), n);
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

static bool zstd_compress_to_file(const void *buf, const size_t size,
                                  const char *fname) {
  size_t cBuffSize = ZSTD_compressBound(size);

  /* Compress.
   * If you are doing many compressions, you may want to reuse the context.
   * See the multiple_simple_compression.c example.
   */
  void *const cBuff = malloc_orDie(cBuffSize);

  size_t const cSize =
      ZSTD_compress(cBuff, cBuffSize, buf, size, /* comp_level */ 7);
  CHECK_ZSTD(cSize);

  saveFile_orDie(fname, cBuff, cSize);

  /* success */
  printf("%25s : %6u -> %7u - %s \n", fname, (unsigned)size, (unsigned)cSize,
         fname);

  free(cBuff);

  return true;
}

constexpr size_t kMaxSize = 1024ull * 1024ull * 1024ull * 128ull; // up to 128GB when uncompressed.

static void decompressFile_orDie(const char* fname,
  std::vector<uint8_t> &dst,
  size_t maxSize = kMaxSize)
{
    FILE* const fin  = fopen_orDie(fname, "rb");
    size_t const buffInSize = ZSTD_DStreamInSize();
    void*  const buffIn  = malloc_orDie(buffInSize);
    //FILE* const fout = stdout;
    size_t const buffOutSize = ZSTD_DStreamOutSize();  /* Guarantee to successfully flush at least one complete compressed block in all circumstances. */
    void*  const buffOut = malloc_orDie(buffOutSize);

    ZSTD_DCtx* const dctx = ZSTD_createDCtx();
    CHECK(dctx != NULL, "ZSTD_createDCtx() failed!");

    /* This loop assumes that the input file is one or more concatenated zstd
     * streams. This example won't work if there is trailing non-zstd data at
     * the end, but streaming decompression in general handles this case.
     * ZSTD_decompressStream() returns 0 exactly when the frame is completed,
     * and doesn't consume input after the frame.
     */
    size_t const toRead = buffInSize;
    size_t read;
    size_t lastRet = 0;
    int isEmpty = 1;
    while ( (read = fread_orDie(buffIn, toRead, fin)) ) {
        isEmpty = 0;
        ZSTD_inBuffer input = { buffIn, read, 0 };
        /* Given a valid frame, zstd won't consume the last byte of the frame
         * until it has flushed all of the decompressed data of the frame.
         * Therefore, instead of checking if the return code is 0, we can
         * decompress just check if input.pos < input.size.
         */
        while (input.pos < input.size) {
            ZSTD_outBuffer output = { buffOut, buffOutSize, 0 };
            /* The return code is zero if the frame is complete, but there may
             * be multiple frames concatenated together. Zstd will automatically
             * reset the context when a frame is complete. Still, calling
             * ZSTD_DCtx_reset() can be useful to reset the context to a clean
             * state, for instance if the last decompression call returned an
             * error.
             */
            size_t const ret = ZSTD_decompressStream(dctx, &output , &input);
            CHECK_ZSTD(ret);
            //fwrite_orDie(buffOut, output.pos, fout);
            size_t dst_loc = dst.size();
            if (dst_loc + output.pos >= maxSize) {
              fprintf(stderr, "zstd data too large. exceeds %" PRIu64 ".\n", dst_loc + output.pos);
              
            }
            dst.resize(dst.size() + output.pos);
            memcpy(dst.data() + dst_loc, buffOut, output.pos);
            lastRet = ret;
        }
    }

    if (isEmpty) {
        fprintf(stderr, "input is empty\n");
        exit(1);
    }

    if (lastRet != 0) {
        /* The last return value from ZSTD_decompressStream did not end on a
         * frame, but we reached the end of the file! We assume this is an
         * error, and the input was truncated.
         */
        fprintf(stderr, "EOF before end of stream: %zu\n", lastRet);
        exit(1);
    }

    ZSTD_freeDCtx(dctx);
    fclose_orDie(fin);
    //fclose_orDie(fout);
    free(buffIn);
    free(buffOut);
}

static std::string zstd_decompress_stream(const char *fname) {
  std::vector<uint8_t> data;
  decompressFile_orDie(fname, data, kMaxSize);

  return std::string(reinterpret_cast<char *>(data.data()), data.size());
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
  //CHECK(rSize != ZSTD_CONTENTSIZE_UNKNOWN, "%s: original size unknown!", fname);
  if (rSize == ZSTD_CONTENTSIZE_UNKNOWN) {
    // Use streaming API
    free(cBuff);
    return zstd_decompress_stream(fname);
  } 

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

  if ((s.size() > 0) && (s_begin < s.size())) {
    dst.push_back(s.substr(s_begin, s.size() - s_begin));
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

template<uint32_t N>
void compute_hash(std::vector<nlohmann::json> &jsons,
                  const std::string &text_key) {
  uint32_t nthreads = cpu_count();

  std::vector<std::thread> workers;
  std::atomic<uint64_t> i(0ull);


  for (uint32_t t = 0; t < nthreads; t++) {
    workers.emplace_back(std::thread([&]() {
      uint64_t idx;

      while ((idx = (i++)) < jsons.size()) {
        auto &j = jsons[idx];

        // TODO: apply normalize for dedup.
        // auto lines = split_lines(j[text_key]);

        auto ngram = strutil::build_ngram<N_GRAM>(j[text_key]);
        std::array<MinHashVal<BUCKET_SIZE, B_BYTES>, N_BUCKETS> lshs = compute_lsh<N_GRAM, N_BUCKETS, BUCKET_SIZE>(ngram);

        std::array<std::string, N_BUCKETS> lsh_base64_strs;
        for (size_t i = 0; i < N_BUCKETS; i++) {
          lsh_base64_strs[i] = to_base64(lshs[i].data(), lshs[i].size());
        }
        j["minhashes"] = lsh_base64_strs;
      }
    }));
  }

  for (auto &th : workers) {
    th.join();
  }
}

static bool save_json_zstd(const std::string &filepath,
                           const nlohmann::json &j) {
  // stringify
  std::stringstream ss;

  ss << j;

  std::string s = ss.str();

  return zstd_compress_to_file(reinterpret_cast<const void *>(s.c_str()),
                               s.size(), filepath.c_str());
}

static bool save_jsonl_zstd(const std::string &filepath,
                           const std::vector<nlohmann::json> &js) {
  // stringify
  std::stringstream ss;

  for (size_t i = 0; i < js.size(); i++) {
    const auto &j = js[i];

    if (i > 0) {
      ss << "\n";
    }

    ss << j;
  }

  std::string s = ss.str();

  return zstd_compress_to_file(reinterpret_cast<const void *>(s.c_str()),
                               s.size(), filepath.c_str());
}

template<uint32_t N = 5>
static bool minhash_files(const std::string &filepath,
                          const std::string &output_basedir,
                          const std::string &text_key) {
  std::vector<glob::fs::path> files = glob::glob({filepath + "/*.zstd", filepath + "/*.zst"});
  std::cout << "num files: " << files.size() << "\n";

  size_t n_documents = 0;
  size_t n_dups = 0;

  //LSHDedupConfig conf;
  std::set<std::vector<uint8_t>> hash_store;

  for (const auto &f : files) {
    std::cout << f << "\n";

    glob::fs::path outpath = output_basedir / f.filename();
    std::cout << "output filepath: " << outpath << "\n";

    std::vector<nlohmann::json> jsonl = load_jsonl_zstd(f);

    compute_hash<N>(jsonl, text_key);

    n_documents += jsonl.size();

    //// TODO: threading
    // for (size_t i = 0; i < jsonl.size(); i++) {
    //   const auto &j = jsonl[i];

    //  if (dedup_stream(j["minhashes"], hash_store)) {
    //    n_dups++;
    //  }

    //  n_documents++;
    //}

    std::vector<nlohmann::json> out_jsonl = jsonl;

    std::stringstream ss;

    // strip text
    for (size_t i = 0; i < out_jsonl.size(); i++) {
      auto &j = out_jsonl[i];

      if (i > 0) {
        ss << "\n";
      }
      j.erase(text_key);

      ss << j;
    }

    std::string jsonl_str = ss.str();

    // save
    if (!zstd_compress_to_file(reinterpret_cast<const void *>(jsonl_str.c_str()),
                               jsonl_str.size(), outpath.c_str())) {
      std::cerr << "Failed to compress/write file: " << outpath << "\n";
      return false;
    }

  }

  //std::cout << "duplicated " << n_dups << " documents(total " << n_documents << "). ratio = "
  //          << 100.0 * double(n_dups) / double(n_documents) << " %\n";

  return true;
}

template<uint32_t T_N_BUCKETS, uint32_t T_BUCKET_SIZE = BUCKET_SIZE, uint32_t T_B = B_BYTES>
static std::array<MinHashVal<T_BUCKET_SIZE, T_B>, T_N_BUCKETS> decode_hashval(
  const std::vector<std::string> &minhashes_strs)
{
  std::array<MinHashVal<T_BUCKET_SIZE, T_B>, T_N_BUCKETS> lshs;

  std::vector<uint8_t> buf;

  for (size_t i = 0; i < T_N_BUCKETS; i++) {

    buf.resize(minhashes_strs[i].size());

    size_t n = chromium_base64_decode(reinterpret_cast<char *>(buf.data()), minhashes_strs[i].data(), minhashes_strs[i].size());
    if (n == MODP_B64_ERROR) {
      std::cerr << "failed to decode base64 string\n";
      exit(-1);
    }

    if (n != (BUCKET_SIZE * B_BYTES)) {
      std::cerr << "hashval size mismatch\n";
      exit(-1);
    }

    memcpy(reinterpret_cast<void *>(lshs[i].data()), buf.data(), BUCKET_SIZE * B_BYTES);
  }

  return lshs;
}

static bool dedup_to_files(const std::string &filepath, const std::string &out_basedir)
{
  std::vector<glob::fs::path> files = glob::glob({filepath + "/*.zstd", filepath + "/*.zst"});
  std::cout << "num files: " << files.size() << "\n";

  size_t n_documents = 0;
  size_t n_dups = 0;
  size_t n_processed_files = 0;

//#define BUCKETIZED_DEDUP

#if defined(BUCKETIZED_DEDUP)
#error TODO
  std::array<std::unordered_set<MinHashVal<BUCKET_SIZE, B_BYTES>, MinHashValHasher<BUCKET_SIZE, B_BYTES>, MinHashValEqual<BUCKET_SIZE, B_BYTES>, N_BUCKETS> hash_stores;
#else
  std::unordered_set<MinHashVal<BUCKET_SIZE, B_BYTES>, MinHashValHasher<BUCKET_SIZE, B_BYTES>, MinHashValEqual<BUCKET_SIZE, B_BYTES>> hash_store;
#endif

  for (const auto &f : files) {
    std::cout << f << "\n";

    std::vector<nlohmann::json> jsonl = load_jsonl_zstd(f);

    n_documents += jsonl.size();

    // TODO: threading
    for (size_t i = 0; i < jsonl.size(); i++) {
      auto &j = jsonl[i];

      std::vector<std::string> minhashes_strs = j["minhashes"];

      if (minhashes_strs.size() != N_BUCKETS) {
        std::cerr << "`minhashes` must be an array with length " << N_BUCKETS << ", but got " << minhashes_strs.size() << "\n";
        return false;
      }

      auto lshs = decode_hashval<N_BUCKETS, BUCKET_SIZE, B_BYTES>(minhashes_strs);

#if defined(BUCKETIZED_DEDUP)
      // i = bucket index.
      bool deduped = false;
#error todo
#else
      bool deduped = dedup_stream<N_BUCKETS, BUCKET_SIZE, B_BYTES>(lshs, hash_store);
#endif

      // add "duplicate" flag
      j["duplicate"] = deduped;

      if (deduped) {
        n_dups++;
      }
    }

    n_processed_files++;

    std::cout << "duplicated " << n_dups << " documents(total " << n_documents << "). ratio = "
              << 100.0 * double(n_dups) / double(n_documents) << " %\n";
    std::cout << "  processed files: " << n_processed_files << " / " << files.size() << "\n";
    std::cout << "  hash_store.size: " << hash_store.size() << "\n";

    // save
    glob::fs::path outpath = out_basedir / f.filename();
    if (!save_jsonl_zstd(outpath, jsonl)) {
      std::cerr << "Failed to compress/write file: " << f << "\n";
      return false;
    }

  }

  std::cout << "TOTAL: duplicated " << n_dups << " documents(total " << n_documents << "). ratio = "
            << 100.0 * double(n_dups) / double(n_documents) << " %\n";
  std::cout << "  processed files: " << n_processed_files << " / " << files.size() << "\n";
  std::cout << "  hash_store.size " << hash_store.size() << "\n";

  return true;
}

static int test_dedup() {
  const char *in0 =
      "吾輩は猫である。名前はまだ無い。どこで生まれたかとんと見当がつかぬ。";
  const char *in1 =
      "吾輩は鳥である。名前はまだ無い。どこで生まれたかとんと見当がつかぬ。";
  const char *in2 =
      "東京は晴れ.";

  auto n0 = strutil::build_ngram<N_GRAM>(in0);
  auto n1 = strutil::build_ngram<N_GRAM>(in1);
  auto n2 = strutil::build_ngram<N_GRAM>(in2);

  for (const auto &gram : n0) {
    std::cout << gram.str() << "\n";
  }
  for (const auto &gram : n1) {
    std::cout << gram.str() << "\n";
  }
  for (const auto &gram : n2) {
    std::cout << gram.str() << "\n";
  }


  //LSHDedupConfig conf;

  auto lsh0 = compute_lsh<N_GRAM, N_BUCKETS, BUCKET_SIZE>(n0);
  auto lsh1 = compute_lsh<N_GRAM, N_BUCKETS, BUCKET_SIZE>(n1);
  auto lsh2 = compute_lsh<N_GRAM, N_BUCKETS, BUCKET_SIZE>(n2);


  std::cout << in0 << "\n";
  for (const auto &lsh : lsh0) {
    std::cout << "lsh.sz = " << lsh.size() << "\n";
    std::cout << strutil::byte_to_hex_string(lsh.data(), lsh.size()) << "\n";
  }

  std::cout << in1 << "\n";
  for (const auto &lsh : lsh1) {
    std::cout << strutil::byte_to_hex_string(lsh.data(), lsh.size()) << "\n";
  }

  std::cout << in2 << "\n";
  for (const auto &lsh : lsh2) {
    std::cout << strutil::byte_to_hex_string(lsh.data(), lsh.size()) << "\n";
  }

  std::unordered_set<MinHashVal<BUCKET_SIZE, B_BYTES>, MinHashValHasher<BUCKET_SIZE, B_BYTES>, MinHashValEqual<BUCKET_SIZE, B_BYTES>> hash_store;

  if (dedup_stream<N_BUCKETS, BUCKET_SIZE, B_BYTES>(lsh0, hash_store)) {
    std::cout << in0 << " duplicated!\n";
  }
  if (dedup_stream<N_BUCKETS, BUCKET_SIZE, B_BYTES>(lsh1, hash_store)) {
    std::cout << in1 << " duplicated!\n";
  }
  if (dedup_stream<N_BUCKETS, BUCKET_SIZE, B_BYTES>(lsh2, hash_store)) {
    std::cout << in2 << " duplicated!\n";
  }

  return 0;
}

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cout << "Need cmd ARGS\n";
    std::cout << "  cmd:\n";
    std::cout << "    wakachi input.txt output.txt: Do wakachi-gaki for input "
                 "string\n";
    std::cout << "    normalize input_string : NFKC normalization\n";
    std::cout << "    dedup <folder> [text_key]: Text dedup. Look *.jsonl.zstd "
                 "files in "
                 "<folder>. [text_key] optional. specify text tag in "
                 "JSON(default `text`)\n";
    std::cout
        << "    minhash <folder> <out_folder> [text_key]: Compute minhash and "
           "store minhash JSON to <out_folder>. Look *.zstd files in "
           "<folder>. [text_key] optional. specify text tag in JSON(default "
           "`text`)\n";
    std::cout << "    proc input.jsonl.zstd : proc(WIP)\n";
    std::cout << "    test <test_cmd>: Run tests\n";
    return -1;
  }

  std::string cmd = argv[1];
  if (cmd == "wakachi") {
  } else if (cmd == "normalize") {
    std::string ret = nfkc_normalize(argv[2]);
    std::cout << ret << "\n";

  } else if (cmd == "minhash") {
    if (argc < 4) {
      std::cerr << "Need <folder> <out_folder> [text_key]\n";
      exit(-1);
    }

    std::string out_basedir = argv[3];

    std::string text_key = "text";
    if (argc > 4) {
      text_key = argv[4];
    }

    bool ret = minhash_files(argv[2], out_basedir, text_key);

    if (ret) {
      return 0;
    } else {
      return -1;
    }
  } else if (cmd == "dedup") {

    if (argc < 4) {
      std::cerr << "Need <folder> <out_folder>\n";
      exit(-1);
    }

    bool ret = dedup_to_files(argv[2], argv[3]);

    if (ret) {
      return 0;
    } else {
      return -1;
    }

  } else if (cmd == "test") {
    std::string suite = "text";
    if (argc > 2) {
      suite = argv[2];
    }

    if (suite == "dedup") {
      std::cout << "run dedup test\n";
      return test_dedup();
    } else {
      std::cout << "Unknown test suite: " << suite << "\n";
    }

  } else {
    std::cout << "Unknown command: " << cmd << "\n";
    return -1;
  }

  return 0;
}
