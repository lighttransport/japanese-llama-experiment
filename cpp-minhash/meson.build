project('cpp-minhash', 'cpp',
  version: '1.0.0',
  default_options: [
    'cpp_std=c++20',
    'warning_level=3',
    'werror=false',
    'buildtype=release'
  ]
)

# Project information
project_description = 'Modern C++20 MinHash implementation for similarity estimation and deduplication'

# Include directories
inc = include_directories('include')

# Source files
murmurhash3_src = files('src/murmurhash3.cpp')

# Library: header-only interface + murmurhash3 implementation
minhash_lib = library('minhash',
  murmurhash3_src,
  include_directories: inc,
  install: true,
  version: meson.project_version()
)

# Declare dependency for other projects
minhash_dep = declare_dependency(
  include_directories: inc,
  link_with: minhash_lib
)

# Install headers
install_headers(
  'include/minhash.hpp',
  'include/murmurhash3.hpp',
  'include/fnv1a.hpp',
  'include/hash_function.hpp',
  'include/jaccard.hpp',
  'include/simd_jaccard.hpp',
  'include/sais.hpp',
  'include/deduplicator.hpp',
  'include/edit_similarity.hpp'
)

# Examples
if get_option('build_examples')
  basic_example = executable('basic_example',
    'examples/basic_example.cpp',
    include_directories: inc,
    link_with: minhash_lib
  )

  jaccard_simd_example = executable('jaccard_simd_example',
    'examples/jaccard_simd_example.cpp',
    include_directories: inc,
    link_with: minhash_lib
  )

  hash_benchmark = executable('hash_benchmark',
    'examples/hash_benchmark.cpp',
    include_directories: inc,
    link_with: minhash_lib
  )

  # FNV-1a version
  hash_benchmark_fnv1a = executable('hash_benchmark_fnv1a',
    'examples/hash_benchmark.cpp',
    include_directories: inc,
    link_with: minhash_lib,
    cpp_args: ['-DMINHASH_USE_FNV1A']
  )

  sais_example = executable('sais_example',
    'examples/sais_example.cpp',
    include_directories: inc,
    link_with: minhash_lib
  )

  dedup_example = executable('dedup_example',
    'examples/dedup_example.cpp',
    include_directories: inc,
    link_with: minhash_lib
  )

  edit_similarity_example = executable('edit_similarity_example',
    'examples/edit_similarity_example.cpp',
    include_directories: inc,
    link_with: minhash_lib
  )
endif

# pkg-config file
pkg = import('pkgconfig')
pkg.generate(
  minhash_lib,
  description: project_description,
  name: 'cpp-minhash'
)

# Summary
summary({
  'prefix': get_option('prefix'),
  'bindir': get_option('bindir'),
  'libdir': get_option('libdir'),
  'includedir': get_option('includedir'),
  'build_examples': get_option('build_examples'),
}, section: 'Directories')

summary({
  'C++ standard': get_option('cpp_std'),
  'Build type': get_option('buildtype'),
}, section: 'Configuration')
