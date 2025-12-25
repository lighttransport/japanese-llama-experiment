# Makefile for minparquet tools

CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra
CXXFLAGS += -I../../cpp

# Path to cpp directory with dependencies
CPP_DIR = ../../cpp

all: test_parquet_reader parquet2jsonl

test_parquet_reader: test_parquet_reader.cpp minparquet.h
	$(CXX) $(CXXFLAGS) -DMINPQ_NO_GZIP -DMINPQ_NO_ZSTD -o $@ test_parquet_reader.cpp

# parquet2jsonl with zstd support
parquet2jsonl: parquet2jsonl.cpp minparquet.h $(CPP_DIR)/zstd.c $(CPP_DIR)/json.hpp
	$(CXX) $(CXXFLAGS) -o $@ parquet2jsonl.cpp $(CPP_DIR)/zstd.c -lpthread

clean:
	rm -f test_parquet_reader parquet2jsonl

.PHONY: all clean
