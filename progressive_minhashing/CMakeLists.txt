cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME fuzzy_dedup)
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
set(FUZZYDEDUP_SOURCES
  main.cc
  fuzzy-dedup.cc
  ../cpp/zstd.c
  ../cpp/json.hpp
  )

add_executable(${PROJECT_NAME} ${FUZZYDEDUP_SOURCES} ${FUZZYDEDUP_DEP_SOURCES})
target_link_libraries(${PROJECT_NAME} PUBLIC ${FUZZYDEDUP_DEP_LIBRARIES} Threads::Threads ${CMAKE_DL_LIBS})
target_include_directories(${PROJECT_NAME} PRIVATE "../cpp")

add_sanitizers(${PROJECT_NAME})

