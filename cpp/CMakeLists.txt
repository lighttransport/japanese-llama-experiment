cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME cpp_proc)
project(${PROJECT_NAME} C CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/sanitizers)
option(CPPPROC_WITH_PYTHON "Embed Python interpreter" OFF)
option(CPPPROC_WITH_IMAGING "Build with image loader/writer(for LLaVa dataset generation)" ON)

find_package(Threads REQUIRED)

# look into <thisrepo>/cmake/Findzstd.cmake
#find_package(zstd REQUIRED)

# look into <thisrepo>/cmake/sanitizers
find_package(Sanitizers) # Supply -DSANITIZE_ADDRESS

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CPPPROC_WITH_PYTHON)

    if(CPPPROC_PREFER_LOCAL_PYTHON_INSTALLATION)
      #message(STATUS "Local Python")
      set(Python3_FIND_FRAMEWORK NEVER) # Do not search framework python
      set(Python3_FIND_STRATEGY LOCATION)
      set(Python3_FIND_REGISTRY NEVER) # Windows only
    else()
      set(Python_FIND_FRAMEWORK LAST
      )# Prefer Brew/Conda to Apple framework python
    endif()

    find_package(
      Python3
      COMPONENTS Interpreter Development.Embed
      REQUIRED)

    #message(STATUS "Python include dirs: " ${Python_INCLUDE_DIRS})
    message(STATUS "Python include dirs: " ${Python3_INCLUDE_DIRS})

    list(APPEND CPPPROC_DEP_LIBRARIES Python3::Python)

endif()


# TODO: add neonbase64.cc or fastavxbase64.c based on CPU ISA.
set(CPP_SOURCES
  main.cc
  jagger.cc
  dedup.cc
  MurmurHash3.cpp
  simdjson.cpp
  safetensors.cc
  # fast scalar base64 encoding/decoding
  chromiumbase64.c
  zstd.c
  utf8proc.c
  libsais64.c
  libsais.c
  sqlite3.c
  lz4.c
  )

if (CPPPROC_WITH_IMAGING)
  list(APPEND CPP_SOURCES fpng.cpp wuffs-unsupported-snapshot.c)
endif()

#add_subdirectory(${PROJECT_SOURCE_DIR}/../third_party/libsais libsais)
#add_subdirectory(${PROJECT_SOURCE_DIR}/../third_party/zstd/build/cmake zstd)

add_executable(${PROJECT_NAME} ${CPP_SOURCES})
target_link_libraries(${PROJECT_NAME} PUBLIC ${CPPPROC_DEP_LIBRARIES} Threads::Threads ${CMAKE_DL_LIBS})
add_sanitizers(${PROJECT_NAME})

