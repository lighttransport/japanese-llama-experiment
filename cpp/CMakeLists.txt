cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME cpp_proc)
project(${PROJECT_NAME} C CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/sanitizers)

# look into <thisrepo>/cmake/Findzstd.cmake
find_package(zstd REQUIRED)

# look into <thisrepo>/cmake/sanitizers
find_package(Sanitizers) # Supply -DSANITIZE_ADDRESS

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CPP_SOURCES
  main.cc
  jagger.cc
  ${PROJECT_SOURCE_DIR}/../third_party/utf8proc/utf8proc.c
  )

add_subdirectory(${PROJECT_SOURCE_DIR}/../third_party/simdjson simdjson)

add_executable(${PROJECT_NAME} ${CPP_SOURCES})
target_link_libraries(${PROJECT_NAME} PUBLIC zstd::zstd simdjson)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/../third_party/utf8proc)
add_sanitizers(${PROJECT_NAME})

