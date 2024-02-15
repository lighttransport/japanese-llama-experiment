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

  struct optparse_long longopts[] = {
        {"tokenize", 't', OPTPARSE_NONE},
        {0}
    };

  bool tokenize{false};

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

  return EXIT_SUCCESS;
}
