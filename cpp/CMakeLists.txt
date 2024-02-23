cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME cpp_proc)
project(${PROJECT_NAME} C CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/sanitizers)
option(CPPPROC_WITH_PYTHON "Embed Python interpreter" OFF)
option(CPPPROC_WITH_IMAGING "Build with image loader/writer(for LLaVa dataset generation)" ON)
option(CPPPROC_WITH_JDEPP "Build with J.DepP(for the dependency parsing of Japanese text)" ON)
option(CPPPROC_WITH_LLAMACPP "Build with llama.cpp(for tokenization)" ON)

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
set(CPPPROC_SOURCES
  main.cc
  jagger.cc
  exact-dedup.cc
  dedup.cc
  MurmurHash3.cpp
  simdjson.cpp
  safetensors.cc
  # fast scalar base64 encoding/decoding
  chromiumbase64.c
  zstd.c
  utf8proc.c
  libsais64.c
  libsais16.c
  libsais.c
  sqlite3.c
  lz4.c
  )

set(CPPPROC_DEP_SOURCES)

if (CPPPROC_WITH_LLAMACPP)

  set(CPPPROC_LLAMACPP_SOURCES
    llamacpp/ggml.c
    llamacpp/ggml-alloc.c
    llamacpp/ggml-quants.c
    llamacpp/ggml-backend.c
    llamacpp/llama.cpp
    llamacpp/common/common.cpp
    llamacpp/common/build-info.cpp
  )

  list(APPEND CPPPROC_DEP_SOURCES ${CPPPROC_LLAMACPP_SOURCES})

  message(STATUS "Build with llama.cpp(for tokenization)")

endif ()

if (CPPPROC_WITH_JDEPP)
  
  set(CPPPROC_JDEPP_SOURCES
    jdepp/classify.cc
    jdepp/kernel.cc
    jdepp/linear.cc
    jdepp/pdep.cc
    jdepp/timer.cc
  )

  list(APPEND CPPPROC_DEP_SOURCES ${CPP_JDEPP_SOURCES})

  message(STATUS "Build with J.DepP(Japanese dependency parser)")

endif()

if (CPPPROC_WITH_IMAGING)
  list(APPEND CPPPROC_SOURCES fpng.cpp wuffs-unsupported-snapshot.c)
endif()

#add_subdirectory(${PROJECT_SOURCE_DIR}/../third_party/libsais libsais)
#add_subdirectory(${PROJECT_SOURCE_DIR}/../third_party/zstd/build/cmake zstd)

add_executable(${PROJECT_NAME} ${CPPPROC_SOURCES} ${CPPPROC_DEP_SOURCES})
target_link_libraries(${PROJECT_NAME} PUBLIC ${CPPPROC_DEP_LIBRARIES} Threads::Threads ${CMAKE_DL_LIBS})

if (CPPPROC_WITH_LLAMACPP)
  target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/llamacpp)
  target_compile_definitions(${PROJECT_NAME} PUBLIC "CPPPROC_USE_LLAMACPP")
endif()

if (CPPPROC_WITH_JDEPP)
  target_compile_definitions(${PROJECT_NAME} PUBLIC "CPPPROC_USE_JDEPP")
endif()

add_sanitizers(${PROJECT_NAME})

