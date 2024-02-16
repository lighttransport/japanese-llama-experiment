#include <cstdlib>
#include <cstdio>
#include <future>
#include <unordered_map>

#include "fuzzy-dedup.hh"

#include "zstd.h"

struct FileBuffer {
  std::string filename;
  std::vector<uint8_t> data; 
};

class ResourcePool
{
  bool add_file(const std::string &

  std::unordered_map
};

int main(int argc, char **argv) {
  return EXIT_SUCCESS;
}
