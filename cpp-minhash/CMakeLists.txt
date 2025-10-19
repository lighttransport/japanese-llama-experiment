cmake_minimum_required(VERSION 3.16)
project(cpp-minhash VERSION 1.0.0 LANGUAGES CXX)

# C++20 required
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(BUILD_EXAMPLES "Build example programs" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# Warning flags
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Library sources
set(MINHASH_SOURCES
    src/murmurhash3.cpp
)

# Library headers
set(MINHASH_HEADERS
    include/minhash.hpp
    include/murmurhash3.hpp
    include/fnv1a.hpp
    include/hash_function.hpp
    include/jaccard.hpp
    include/simd_jaccard.hpp
    include/sais.hpp
)

# Create library
add_library(minhash ${MINHASH_SOURCES})
target_include_directories(minhash
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Set library properties
set_target_properties(minhash PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "${MINHASH_HEADERS}"
)

# Examples
if(BUILD_EXAMPLES)
    add_executable(basic_example examples/basic_example.cpp)
    target_link_libraries(basic_example PRIVATE minhash)

    add_executable(jaccard_simd_example examples/jaccard_simd_example.cpp)
    target_link_libraries(jaccard_simd_example PRIVATE minhash)

    add_executable(hash_benchmark examples/hash_benchmark.cpp)
    target_link_libraries(hash_benchmark PRIVATE minhash)

    # Also build FNV-1a version of hash_benchmark
    add_executable(hash_benchmark_fnv1a examples/hash_benchmark.cpp)
    target_link_libraries(hash_benchmark_fnv1a PRIVATE minhash)
    target_compile_definitions(hash_benchmark_fnv1a PRIVATE MINHASH_USE_FNV1A)

    add_executable(sais_example examples/sais_example.cpp)
    target_link_libraries(sais_example PRIVATE minhash)
endif()

# Installation
include(GNUInstallDirs)

install(TARGETS minhash
    EXPORT minhash-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/minhash
)

install(EXPORT minhash-targets
    FILE minhash-targets.cmake
    NAMESPACE minhash::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/minhash
)

# Create config file
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/minhash-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/minhash-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/minhash
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/minhash-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/minhash-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/minhash-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/minhash
)

# Summary
message(STATUS "")
message(STATUS "cpp-minhash configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Build examples: ${BUILD_EXAMPLES}")
message(STATUS "  Build shared libs: ${BUILD_SHARED_LIBS}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
