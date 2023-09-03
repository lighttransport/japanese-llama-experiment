cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME cpp_proc)
project(${PROJECT_NAME} C CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/sanitizers)

find_package(Threads REQUIRED)

# look into <thisrepo>/cmake/Findzstd.cmake
#find_package(zstd REQUIRED)

# look into <thisrepo>/cmake/sanitizers
find_package(Sanitizers) # Supply -DSANITIZE_ADDRESS

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# TODO: add neonbase64.cc or fastavxbase64.c based on CPU ISA.
set(CPP_SOURCES
  main.cc
  jagger.cc
  MurmurHash3.cpp
  simdjson.cpp
  # fast scalar base64 encoding/decoding
  chromiumbase64.c
  zstd.c
  utf8proc.c
  libsais64.c
  libsais.c
  )

#add_subdirectory(${PROJECT_SOURCE_DIR}/../third_party/libsais libsais)
#add_subdirectory(${PROJECT_SOURCE_DIR}/../third_party/zstd/build/cmake zstd)

add_executable(${PROJECT_NAME} ${CPP_SOURCES})
target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)
add_sanitizers(${PROJECT_NAME})

