#include <vector>
#include <string>
#include <iostream>

#include <zstd.h>
#include "common.h" // from zstd example
#include <simdjson.h>

static std::string decompress(const char* fname)
{
    size_t cSize;
    void* const cBuff = mallocAndLoadFile_orDie(fname, &cSize);
    /* Read the content size from the frame header. For simplicity we require
     * that it is always present. By default, zstd will write the content size
     * in the header when it is known. If you can't guarantee that the frame
     * content size is always written into the header, either use streaming
     * decompression, or ZSTD_decompressBound().
     */
    unsigned long long const rSize = ZSTD_getFrameContentSize(cBuff, cSize);
    CHECK(rSize != ZSTD_CONTENTSIZE_ERROR, "%s: not compressed by zstd!", fname);
    CHECK(rSize != ZSTD_CONTENTSIZE_UNKNOWN, "%s: original size unknown!", fname);

    void* const rBuff = malloc_orDie((size_t)rSize);

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

using namespace simdjson;

int main(int argc, char **argv) {

  if (argc < 3) {
    std::cout << "Need input.jsonl.zstd output.jsonl.zstd\n";
    return -1;
  }

	std::string jsonl = decompress(argv[1]);

  std::vector<std::string> jsons = split_lines(jsonl);


  ondemand::parser parser;
  for (size_t i = 0; i < jsons.size(); i++) {
    padded_string json = padded_string(jsons[i]);
    ondemand::document j = parser.iterate(json);
  }


}
