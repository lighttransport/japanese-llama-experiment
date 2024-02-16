cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME exact_dedup)
project(${PROJECT_NAME} C CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../cpp/cmake)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../cpp/cmake/sanitizers)

find_package(Threads REQUIRED)

# look into <thisrepo>/cmake/sanitizers
find_package(Sanitizers) # Supply -DSANITIZE_ADDRESS

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# TODO: add neonbase64.cc or fastavxbase64.c based on CPU ISA.
set(EXACTDEDUP_SOURCES
  main.cc
  ../cpp/zstd.c
  ../cpp/exact-dedup.cc
  ../cpp/TaskScheduler.cpp
  ../cpp/libsais.c
  ../cpp/libsais16.c
  ../cpp/lz4.c
  ../cpp/lz4hc.c
  ../cpp/lz4file.c
  ../cpp/lz4frame.c
  ../cpp/xxhash.c
  )

add_executable(${PROJECT_NAME} ${EXACTDEDUP_SOURCES} ${EXACTDEDUP_DEP_SOURCES})
target_link_libraries(${PROJECT_NAME} PUBLIC ${EXACTDEDUP_DEP_LIBRARIES} Threads::Threads ${CMAKE_DL_LIBS})
target_include_directories(${PROJECT_NAME} PRIVATE "../cpp")

add_sanitizers(${PROJECT_NAME})

