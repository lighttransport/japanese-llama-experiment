#include <cstdlib>
#include <cstdio>
#include <future>
#include <unordered_map>
#include <inttypes.h>

#include "ghc/filesystem.hpp"


//
#define GLOB_USE_GHC_FILESYSTEM
#include "glob.hpp"
//

#include "json.hpp"
#include "lz4file.h"
#include "rwkv_world_tokenizer_cedar.hh"

//
#define OPTPARSE_IMPLEMENTATION
#include "optparse.h"
//

#include "common.h"  // from zstd example
#include "pbar.hpp"

//
#define SAFETENSORS_CPP_IMPLEMENTATION
#include "safetensors.hh"
//

//
#include "dedup.hh"
#include "fuzzy-dedup.hh"
#include "jp_normalizer.hh"

namespace fs = ghc::filesystem;

static bool zstd_compress_to_file(const void *buf, const size_t size,
                                  const char *fname, const int comp_level) {
  size_t cBuffSize = ZSTD_compressBound(size);

  /* Compress.
   * If you are doing many compressions, you may want to reuse the context.
   * See the multiple_simple_compression.c example.
   */
  void *const cBuff = malloc_orDie(cBuffSize);

  size_t const cSize =
      ZSTD_compress(cBuff, cBuffSize, buf, size, comp_level);
  CHECK_ZSTD(cSize);

  saveFile_orDie(fname, cBuff, cSize);

  /* success */
  printf("\nzstd compress: %25s : %6u -> %7u - %s \n", fname, (unsigned)size,
         (unsigned)cSize, fname);

  free(cBuff);

  return true;
}

static bool zstd_compress_to_memory(const void *buf, const size_t size,
                                    std::vector<uint8_t> &mem) {
  size_t cBuffSize = ZSTD_compressBound(size);

  /* Compress.
   * If you are doing many compressions, you may want to reuse the context.
   * See the multiple_simple_compression.c example.
   */
  void *const cBuff = malloc_orDie(cBuffSize);

  size_t const cSize =
      ZSTD_compress(cBuff, cBuffSize, buf, size, /* comp_level */ 7);
  CHECK_ZSTD(cSize);

  printf("\nzstd compress: %6u -> %7u \n", (unsigned)size, (unsigned)cSize);

  mem.resize(cSize);
  memcpy(mem.data(), cBuff, cSize);

  free(cBuff);

  return true;
}

bool compute_minhash_tokenized(
  const nanotokenizer::CedarTrieTokenizer &tokenizer,
  const std::string &str,
  uint32_t r,
  uint32_t b) {


  return true;
}

// 32bit hash
bool saveMinhashAsSafetensor(const std::string &input_filename,
                             const std::string &vocab_filename,
                             bool is_tokenized,
                             bool use_codepoint,
                             const std::string &st_filename,
                             const std::vector<int> &document_ids,
                             const std::vector<uint8_t> &document_flags,
                             const int hash_b,
                             const int hash_r,
                             const std::vector<uint32_t> &hashes) {

  size_t num_documents = document_ids.size();

  if (hashes.size() != size_t(hash_b * hash_r) * num_documents) {
    fprintf(stderr, "Invalid hash size.\n");
    exit(-1);
  }

  safetensors::safetensors_t st;


  size_t data_offset = 0;

  // hashes
  {
    // hash values cannot be compressed well, so save hashes as is.
    safetensors::tensor_t tensor;
    tensor.dtype = safetensors::dtype::kUINT32;
    tensor.data_offsets[0] = data_offset;
    tensor.data_offsets[1] = data_offset + hashes.size() * sizeof(int);
    tensor.shape.resize(1);
    tensor.shape[0] = hashes.size();

    st.storage.resize(data_offset + hashes.size() * sizeof(int));
    memcpy(st.storage.data() + data_offset, reinterpret_cast<const void *>(hashes.data()), hashes.size() * sizeof(int));

    data_offset += hashes.size() * sizeof(int);

    st.tensors.insert("minhashes", tensor);
  }

  // document ids
  {
    std::vector<uint8_t> buf;

    if (!zstd_compress_to_memory(reinterpret_cast<const void *>(document_ids.data()), sizeof(int) * document_ids.size(), buf)) {
      std::cerr << "Failed to compress document_ids\n";
      exit(-1);
    }

    safetensors::tensor_t tensor;
    tensor.dtype = safetensors::dtype::kUINT8;
    tensor.data_offsets[0] = data_offset;
    tensor.data_offsets[1] = data_offset + buf.size();
    tensor.shape.resize(1);
    tensor.shape[0] = buf.size();

    st.storage.resize(data_offset + buf.size());
    memcpy(st.storage.data() + data_offset, reinterpret_cast<const void *>(buf.data()), buf.size());

    data_offset += buf.size();

    st.tensors.insert("document_ids", tensor);

  }

  // document flags
  {
    std::vector<uint8_t> buf;

    if (!zstd_compress_to_memory(reinterpret_cast<const void *>(document_flags.data()), document_flags.size(), buf)) {
      std::cerr << "Failed to compress document_flags\n";
      exit(-1);
    }

    safetensors::tensor_t tensor;
    tensor.dtype = safetensors::dtype::kUINT8;
    tensor.data_offsets[0] = data_offset;
    tensor.data_offsets[1] = data_offset + buf.size();
    tensor.shape.resize(1);
    tensor.shape[0] = buf.size();

    st.storage.resize(data_offset + buf.size());
    memcpy(st.storage.data() + data_offset, reinterpret_cast<const void *>(buf.data()), buf.size());

    data_offset += buf.size();

    st.tensors.insert("document_flags", tensor);

  }

  std::string hash_b_str = std::to_string(hash_b);
  std::string hash_r_str = std::to_string(hash_r);

  st.metadata.insert("hash_b", hash_b_str);
  st.metadata.insert("hash_r", hash_r_str);
  st.metadata.insert("input_filename", input_filename);
  st.metadata.insert("compression", "zstd");
  st.metadata.insert("tokenized", is_tokenized ? "true" : "false");
  if (is_tokenized) {
    st.metadata.insert("use_codepoint", use_codepoint ? "true" : "false");
  }
  if (is_tokenized) {
    st.metadata.insert("vocab_filename", vocab_filename);
  }

  std::string warn, err;
  bool ret = safetensors::save_to_file(st, st_filename, &warn, &err);

  if (warn.size()) {
    std::cout << "SaveToSafetensors WARN: " << warn << "\n";
  }

  if (!ret) {
    std::cerr << "Failed to save safetensors: " << err << "\n";
    exit(-1);
  }

  return true;
}

static uint32_t cpu_count() {
  return (std::max)(1u, std::thread::hardware_concurrency());
}

constexpr size_t kMaxSize =
    1024ull * 1024ull * 1024ull * 128ull;  // up to 128GB when uncompressed.

static void decompressFile_orDie(const char *fname, std::vector<uint8_t> &dst,
                                 size_t maxSize = kMaxSize) {
  FILE *const fin = fopen_orDie(fname, "rb");
  size_t const buffInSize = ZSTD_DStreamInSize();
  void *const buffIn = malloc_orDie(buffInSize);
  // FILE* const fout = stdout;
  size_t const buffOutSize =
      ZSTD_DStreamOutSize(); /* Guarantee to successfully flush at least one
                                complete compressed block in all circumstances.
                              */
  void *const buffOut = malloc_orDie(buffOutSize);

  ZSTD_DCtx *const dctx = ZSTD_createDCtx();
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
  while ((read = fread_orDie(buffIn, toRead, fin))) {
    isEmpty = 0;
    ZSTD_inBuffer input = {buffIn, read, 0};
    /* Given a valid frame, zstd won't consume the last byte of the frame
     * until it has flushed all of the decompressed data of the frame.
     * Therefore, instead of checking if the return code is 0, we can
     * decompress just check if input.pos < input.size.
     */
    while (input.pos < input.size) {
      ZSTD_outBuffer output = {buffOut, buffOutSize, 0};
      /* The return code is zero if the frame is complete, but there may
       * be multiple frames concatenated together. Zstd will automatically
       * reset the context when a frame is complete. Still, calling
       * ZSTD_DCtx_reset() can be useful to reset the context to a clean
       * state, for instance if the last decompression call returned an
       * error.
       */
      size_t const ret = ZSTD_decompressStream(dctx, &output, &input);
      CHECK_ZSTD(ret);
      // fwrite_orDie(buffOut, output.pos, fout);
      size_t dst_loc = dst.size();
      if (dst_loc + output.pos >= maxSize) {
        fprintf(stderr, "zstd data too large. exceeds %" PRIu64 ".\n",
                dst_loc + output.pos);
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
  // fclose_orDie(fout);
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
  // CHECK(rSize != ZSTD_CONTENTSIZE_UNKNOWN, "%s: original size unknown!",
  // fname);
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
  // printf("\n%25s : %6u -> %7u \n", fname, (unsigned)cSize, (unsigned)rSize);

  // assume decoded data is utf-8 string.
  std::string buf(reinterpret_cast<const char *>(rBuff), dSize);

  free(rBuff);
  free(cBuff);

  return buf;
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

static std::vector<nlohmann::json> load_jsonl_zstd(
    const glob::fs::path &filepath) {
  std::string jsonl_data = zstd_decompress(filepath.c_str());

  std::vector<nlohmann::json> jsonl = decode_jsonl(split_lines(jsonl_data));

  return jsonl;
}

std::vector<uint8_t> flatten_texts(const std::vector<nlohmann::json> &js,
                                   const std::string &text_key) {
  std::vector<uint8_t> dst;

  for (const auto &j : js) {
    std::string text = j[text_key];
    dst.insert(dst.end(), text.begin(), text.end());

    // Use 3(end-of-text) as delimiter
    dst.push_back(3);
  }

  return dst;
}

bool build_tokenizer(nanotokenizer::CedarTrieTokenizer &tok,
                     const std::string &vocab_filename) {
  std::ifstream ifs(vocab_filename);

  nlohmann::json j = nlohmann::json::parse(ifs);

  std::map<std::string, int> str_to_id_map;

  for (nlohmann::json::iterator it = j.begin(); it != j.end(); ++it) {
    //std::cout << "str " << it.key() << ", v " << int(it.value()) << "\n";
    str_to_id_map[it.key()] = int(it.value());
  }

  std::string err;
  if (!tok.load_vocab(str_to_id_map, err)) {
    fprintf(stderr, "Failed to setup Tokenizer: %s", err.c_str());
    return false;
  }

  return true;
}

bool build_tokenizer_for_dedup(/* inout */nanotokenizer::CedarTrieTokenizer &tok,
                      const std::string &vocab_filename,
                      /* out */std::unordered_set<uint16_t> &punct_table,
                      const std::string &placeholder_str = "0") {
  std::ifstream ifs(vocab_filename);

  nlohmann::json j = nlohmann::json::parse(ifs);

  std::map<std::string, int> str_to_id_map;

  for (nlohmann::json::iterator it = j.begin(); it != j.end(); ++it) {
    //std::cout << "str " << it.key() << ", v " << int(it.value()) << "\n";
    str_to_id_map[it.key()] = int(it.value());
  }

  std::map<std::string, int> filtered_str_to_id_map;
  num_to_placeholder(str_to_id_map, placeholder_str, filtered_str_to_id_map);

  // dbg
  for (const auto &it: filtered_str_to_id_map) {
    std::cout << it.first << ":" << it.second << "\n";
  }

  std::string err;
  if (!tok.load_vocab(filtered_str_to_id_map, err)) {
    fprintf(stderr, "Failed to setup Tokenizer: %s", err.c_str());
    return false;
  }

  std::unordered_set<std::string> punct_strs = jpnormalizer::get_unicode_puncts();

  // Build table of tokenized punctuation string
  for (const auto &p : punct_strs) {
    int id = tok.id_from_str(p);
    if (id > 0) {
      punct_table.insert(uint16_t(id));
    } else {
      std::cout << "Punctuation `" << p << "` is not present in vocab. Ignore this.\n";
    }
  }


  return true;
}

void print_help() {

  std::cout << "exact_dedup OPTIONS input.jsonl.zstd\n";
  std::cout << "\n";
  std::cout << "OPTIONS\n";
  std::cout << "\n";
  std::cout << "--directory(-d) DIR  : Process files in the directory instead of a file\n";
  std::cout << "--outdir(-o) DIR     : Output directory\n";
  std::cout << "--hashconfig(-g)     : 0: b=20 r=450(9000 hashes. Jaccard 0.8), 1: b=20 r=40(800 hashes. Jaccard 0.9), 2: b=20 r=10(200 hashes. Jaccard 0.96)\n";
  std::cout << "--ngram(-n)          : n for N-gram(max 32). default 5.\n";
  std::cout << "--vocab(-b) FILENAME : Specify Vocab JSON file for tokenization\n";
  std::cout << "--codepoint(-c)      : Use codepoint representation of UTF-8 character(faster tokenization).\n";
  std::cout << "--zcomp_level(-z)    : Compression level for ZSTD compression. default 9\n";
  std::cout << "--text_key(-k)       : Specify JSON key for text data(default `text`)\n";
  std::cout << "--test(-s)           : Do tests.\n";
  std::cout << "--help(-h)           : Print this help\n";
}



int main(int argc, char **argv) {

  struct optparse_long longopts[] = {{"indir", 'd', OPTPARSE_REQUIRED},
                                     {"outdir", 'o', OPTPARSE_REQUIRED},
                                     {"codepoint", 'c', OPTPARSE_NONE},
                                     {"vocab", 'b', OPTPARSE_REQUIRED},
                                     {"zcomp_level", 'z', OPTPARSE_REQUIRED},
                                     {"hashconfig", 'g', OPTPARSE_REQUIRED},
                                     {"ngram", 'n', OPTPARSE_REQUIRED},
                                     {"ngram", 'n', OPTPARSE_REQUIRED},
                                     {"test", 's', OPTPARSE_NONE},
                                     {"help", 'h', OPTPARSE_NONE},
                                     {0}};

  int ngram = 5;
  int zcomp_level = 9;
  bool do_test{false};
  int hashconfig = 0; // default: 9000 hashes
  std::string num_placeholder_str = "0";

  // default: Read a file.
  std::string indir;
  std::string outdir{"sa_out"};
  std::string filename = "../../test_data/bora.jsonl.zst";

  bool use_codepoint{false};
  std::string vocab_json_filename{"../../models/rwkv_vocab_v20230424-ja-emo-kao.json"};
  std::string text_key{"text"};

  int option;
  struct optparse options;
  optparse_init(&options, argv);

  while ((option = optparse_long(&options, longopts, nullptr)) != -1) {
    switch (option) {
      case 'g': {
        int val = std::atoi(options.optarg);
        if (val < 0) val = 0;
        if (val > 2) val = 2;
        hashconfig = val;
        break;
      }
      case 'n': {
        int val = std::atoi(options.optarg);
        if (val < 1) val = 1;
        if (val > 32) val = 32;
        ngram = val;
        break;
      }
      case 'k':
        text_key = options.optarg;
        break;
      case 'b':
        vocab_json_filename = options.optarg;
        break;
      case 'c':
        use_codepoint = true;
        break;
      case 'd':
        indir = options.optarg;
        break;
      case 'o':
        outdir = options.optarg;
        break;
      case 's':
        do_test = true;
        break;
      case 'z':
        // zstd itself supports level up to 22, but 15+ requires not prectical to use since it comsumes lots of time for compression
        zcomp_level = (std::max)(1, (std::min)(15, std::atoi(options.optarg)));
        break;
      case 'h':
        print_help();
        exit(-1);
        break;
      case '?':
        fprintf(stderr, "%s: %s\n", argv[0], options.errmsg);
        exit(EXIT_FAILURE);
    }
  }

  char *input_file_arg = optparse_arg(&options);
  if (input_file_arg) {
    filename = std::string(input_file_arg);
  }

  fs::path outdir_path(outdir);

  if (!fs::exists(outdir_path)) {
    if (!fs::create_directory(outdir_path)) {
      std::cerr << "Failed to create directory: " << outdir_path << "\n";
      exit(-1);
    }
  }

  int total = 1;
  pbar::pbar bar(total, /* ncols */ 100, "[Task]");

  bar.enable_recalc_console_width(1);
  bar.init();

  std::vector<nlohmann::json> js = load_jsonl_zstd(filename);
  std::vector<uint8_t> texts = flatten_texts(js, text_key);

  std::vector<int32_t> sa;

  std::string out_filename = "output-sa";

  std::unique_ptr<nanotokenizer::CedarTrieTokenizer> tokenizer(new nanotokenizer::CedarTrieTokenizer(use_codepoint));

  std::unordered_set<uint16_t> punct_table;
  if (!build_tokenizer_for_dedup(*tokenizer, vocab_json_filename, punct_table, num_placeholder_str)) {
    exit(-1);
  }
  out_filename += "-tokenized";

  std::vector<int> input_ids;
  std::string s(texts.begin(), texts.begin() + texts.size());
  if (!tokenizer->encode(s, input_ids)) {
    fprintf(stderr, "tokenize failed.\n");
    exit(-1);
  }

  std::vector<uint16_t> input_ids_u16;
  input_ids_u16.resize(input_ids.size());

  for (size_t i = 0; i < input_ids.size(); i++) {
    if ((input_ids[i] < 0) ||
        (input_ids[i] > (std::numeric_limits<uint16_t>::max)())) {
      fprintf(stderr, "token id must be in range [0, 65535]\n");
      exit(-1);
    }
    input_ids_u16[i] = uint16_t(input_ids[i]);
  }

  //if (do_test) {
  //  test_tokenize(*tokenizer, s, input_ids_u16);
  //}


#if 0
  out_filename += ".safetensors";
  fs::path out_filepath = outdir_path / fs::path(out_filename);
  if (!saveSuffixArraySafetensor(out_filepath, vocab_json_filename, tokenize, use_codepoint, out_filename,
                       reinterpret_cast<const uint8_t *>(sa.data()),
                       sa.size() * sizeof(int32_t))) {
    fprintf(stderr, "Failed to save suffix array.");
    exit(-1);
  }
#endif

  ++bar;

  std::cout << std::flush;
}
