#include <cstdlib>
#include <cstdio>
#include <future>
#include <unordered_map>
#include <vector>
#include <future>
#include <iostream>
#include <thread>

#include "exact-dedup.hh"

#define GLOB_USE_GHC_FILESYSTEM
#include "glob.hpp"

#define OPTPARSE_IMPLEMENTATION
#include "optparse.h"

#include "pbar.hpp"
#include "nanotokenizer.hh"
#include "json.hpp"

#include "common.h"  // from zstd example

static uint32_t cpu_count() {
  return (std::max)(1u, std::thread::hardware_concurrency());
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


int main(int argc, char **argv) {

  std::string filename = "../../test_data/bora.jsonl.zst";
  bool tokenize{false};

  struct optparse_long longopts[] = {
        {"tokenize", 't', OPTPARSE_NONE},
        {0}
    };

  int option;
  struct optparse options;
  optparse_init(&options, argv);

  while((option =  optparse_long(&options, longopts, nullptr)) != -1) {
    switch (option) {
      case 't':
        tokenize = true;
        break;
      case '?':
        fprintf(stderr, "%s: %s\n", argv[0], options.errmsg);
        exit(EXIT_FAILURE); 
      }
  }

  char *arg;
  while ((arg = optparse_arg(&options))) {
    printf("%s\n", arg);
  }


  int total = 1;
  pbar::pbar bar(total, /* ncols */100, "[Task]");

  bar.enable_recalc_console_width(1);
  bar.init();

  std::vector<nlohmann::json> js = load_jsonl_zstd(filename);

  ++bar;

  std::cout << std::flush;

  return EXIT_SUCCESS;
}
