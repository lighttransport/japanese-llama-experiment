#include <cstdlib>
#include <cstdio>
#include <future>
#include <unordered_map>

#include "ghc/filesystem.hpp"

#include "exact-dedup.hh"

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
#include "fuzzy-dedup.hh"

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


int main(int argc, char **argv) {
  return EXIT_SUCCESS;
}
