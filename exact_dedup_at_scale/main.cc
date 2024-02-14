#include <cstdlib>
#include <cstdio>
#include <future>
#include <unordered_map>
#include <vector>

#include "exact-dedup.hh"

#define OPTPARSE_IMPLEMENTATION
#include "optparse.h"

struct FileBuffer {
  std::string filename;
  std::vector<uint8_t> data; 
};

class ResourcePool
{
  bool add_file(const std::string &filename);
};

int main(int argc, char **argv) {
  return EXIT_SUCCESS;
}
