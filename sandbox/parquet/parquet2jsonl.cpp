// SPDX-License-Identifier: MIT
// parquet2jsonl - Convert Parquet files to JSONL with optional zstd compression
//
// Usage: parquet2jsonl <input.parquet> [output.jsonl.zstd]
//        If output is omitted, uses input basename + .jsonl.zstd

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>

// Include zstd before minparquet to make compression available
#include "../../cpp/zstd.h"

// Disable GZIP support (most parquet files use ZSTD or uncompressed)
#define MINPQ_NO_GZIP

// Now include minparquet with ZSTD enabled
#include "minparquet.h"

// Include nlohmann json for JSON serialization
#include "../../cpp/json.hpp"

using json = nlohmann::json;

// Helper to get output filename from input
static std::string get_output_filename(const std::string& input) {
    std::string base = input;
    // Remove .parquet extension if present
    size_t pos = base.rfind(".parquet");
    if (pos != std::string::npos && pos == base.size() - 8) {
        base = base.substr(0, pos);
    }
    return base + ".jsonl.zstd";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input.parquet> [output.jsonl.zstd]\n";
        std::cerr << "\nConverts a Parquet file to JSONL format with zstd compression.\n";
        std::cerr << "If output is omitted, uses input basename + .jsonl.zstd\n";
        return 1;
    }

    const char* input_file = argv[1];
    std::string output_file = (argc >= 3) ? argv[2] : get_output_filename(input_file);

    // Check if output should be uncompressed
    bool use_compression = true;
    if (output_file.size() >= 5 && output_file.substr(output_file.size() - 5) == ".zstd") {
        use_compression = true;
    } else if (output_file.size() >= 6 && output_file.substr(output_file.size() - 6) == ".jsonl") {
        use_compression = false;
    }

    // Open parquet file
    minpq::ParquetReader reader;
    if (!reader.open(input_file)) {
        std::cerr << "Error opening parquet file: " << reader.error() << "\n";
        return 1;
    }

    std::cerr << "Input: " << input_file << "\n";
    std::cerr << "Output: " << output_file << (use_compression ? " (zstd)" : "") << "\n";
    std::cerr << "Rows: " << reader.num_rows() << "\n";
    std::cerr << "Columns: " << reader.num_columns() << "\n";
    std::cerr << "Row groups: " << reader.num_row_groups() << "\n";

    // Get column names
    size_t num_cols = reader.num_columns();
    std::vector<std::string> col_names(num_cols);
    for (size_t i = 0; i < num_cols; i++) {
        col_names[i] = std::string(reader.column_name(i));
    }

    // Print column info
    std::cerr << "\nColumns:\n";
    for (size_t i = 0; i < num_cols; i++) {
        std::cerr << "  " << i << ": " << col_names[i];
        auto type = reader.column_type(i);
        if (type) {
            switch (*type) {
                case minpq::Type::BOOLEAN: std::cerr << " (BOOLEAN)"; break;
                case minpq::Type::INT32: std::cerr << " (INT32)"; break;
                case minpq::Type::INT64: std::cerr << " (INT64)"; break;
                case minpq::Type::INT96: std::cerr << " (INT96)"; break;
                case minpq::Type::FLOAT: std::cerr << " (FLOAT)"; break;
                case minpq::Type::DOUBLE: std::cerr << " (DOUBLE)"; break;
                case minpq::Type::BYTE_ARRAY: std::cerr << " (BYTE_ARRAY)"; break;
                case minpq::Type::FIXED_LEN_BYTE_ARRAY: std::cerr << " (FIXED_LEN_BYTE_ARRAY)"; break;
            }
        }
        std::cerr << "\n";
    }
    std::cerr << "\n";

    // Buffer for JSONL output
    std::string jsonl_buffer;
    jsonl_buffer.reserve(64 * 1024 * 1024);  // 64MB initial buffer

    size_t total_rows = 0;

    // Process each row group
    for (size_t rg = 0; rg < reader.num_row_groups(); rg++) {
        // Read all columns for this row group
        std::vector<std::vector<std::string>> string_cols(num_cols);
        std::vector<std::vector<int32_t>> int32_cols(num_cols);
        std::vector<std::vector<int64_t>> int64_cols(num_cols);
        std::vector<std::vector<float>> float_cols(num_cols);
        std::vector<std::vector<double>> double_cols(num_cols);
        std::vector<std::vector<bool>> bool_cols(num_cols);

        size_t max_rows = 0;

        for (size_t col = 0; col < num_cols; col++) {
            auto col_type = reader.column_type(col);
            if (!col_type) continue;

            switch (*col_type) {
                case minpq::Type::BOOLEAN: {
                    auto vals = reader.read_bool_column(rg, col);
                    if (vals) {
                        bool_cols[col] = std::move(*vals);
                        max_rows = std::max(max_rows, bool_cols[col].size());
                    }
                    break;
                }
                case minpq::Type::INT32: {
                    auto vals = reader.read_int32_column(rg, col);
                    if (vals) {
                        int32_cols[col] = std::move(*vals);
                        max_rows = std::max(max_rows, int32_cols[col].size());
                    }
                    break;
                }
                case minpq::Type::INT64: {
                    auto vals = reader.read_int64_column(rg, col);
                    if (vals) {
                        int64_cols[col] = std::move(*vals);
                        max_rows = std::max(max_rows, int64_cols[col].size());
                    }
                    break;
                }
                case minpq::Type::FLOAT: {
                    auto vals = reader.read_float_column(rg, col);
                    if (vals) {
                        float_cols[col] = std::move(*vals);
                        max_rows = std::max(max_rows, float_cols[col].size());
                    }
                    break;
                }
                case minpq::Type::DOUBLE: {
                    auto vals = reader.read_double_column(rg, col);
                    if (vals) {
                        double_cols[col] = std::move(*vals);
                        max_rows = std::max(max_rows, double_cols[col].size());
                    }
                    break;
                }
                case minpq::Type::BYTE_ARRAY: {
                    auto vals = reader.read_string_column(rg, col);
                    if (vals) {
                        string_cols[col] = std::move(*vals);
                        max_rows = std::max(max_rows, string_cols[col].size());
                    }
                    break;
                }
                default:
                    break;
            }
        }

        // Convert each row to JSON
        for (size_t row = 0; row < max_rows; row++) {
            json obj = json::object();

            for (size_t col = 0; col < num_cols; col++) {
                auto col_type = reader.column_type(col);
                if (!col_type) continue;

                json val;
                switch (*col_type) {
                    case minpq::Type::BOOLEAN:
                        if (row < bool_cols[col].size()) {
                            val = bool_cols[col][row];
                        }
                        break;
                    case minpq::Type::INT32:
                        if (row < int32_cols[col].size()) {
                            val = int32_cols[col][row];
                        }
                        break;
                    case minpq::Type::INT64:
                        if (row < int64_cols[col].size()) {
                            val = int64_cols[col][row];
                        }
                        break;
                    case minpq::Type::FLOAT:
                        if (row < float_cols[col].size()) {
                            val = float_cols[col][row];
                        }
                        break;
                    case minpq::Type::DOUBLE:
                        if (row < double_cols[col].size()) {
                            val = double_cols[col][row];
                        }
                        break;
                    case minpq::Type::BYTE_ARRAY:
                        if (row < string_cols[col].size()) {
                            val = string_cols[col][row];
                        }
                        break;
                    default:
                        break;
                }

                if (!val.is_null()) {
                    obj[col_names[col]] = std::move(val);
                }
            }

            // Append JSON line
            jsonl_buffer += obj.dump(-1, ' ', false, json::error_handler_t::replace);
            jsonl_buffer += '\n';
            total_rows++;
        }

        std::cerr << "Processed row group " << rg + 1 << "/" << reader.num_row_groups()
                  << " (" << max_rows << " rows)\n";
    }

    std::cerr << "Total rows: " << total_rows << "\n";
    std::cerr << "JSONL size: " << jsonl_buffer.size() << " bytes\n";

    // Write output
    if (use_compression) {
        // Compress with zstd
        size_t compressed_bound = ZSTD_compressBound(jsonl_buffer.size());
        std::vector<char> compressed(compressed_bound);

        size_t compressed_size = ZSTD_compress(
            compressed.data(), compressed_bound,
            jsonl_buffer.data(), jsonl_buffer.size(),
            3  // compression level
        );

        if (ZSTD_isError(compressed_size)) {
            std::cerr << "Error compressing data\n";
            return 1;
        }

        std::cerr << "Compressed size: " << compressed_size << " bytes ("
                  << (100.0 * compressed_size / jsonl_buffer.size()) << "%)\n";

        // Write compressed data
        std::ofstream out(output_file, std::ios::binary);
        if (!out) {
            std::cerr << "Error opening output file: " << output_file << "\n";
            return 1;
        }
        out.write(compressed.data(), compressed_size);
        if (!out) {
            std::cerr << "Error writing output file\n";
            return 1;
        }
    } else {
        // Write uncompressed
        std::ofstream out(output_file);
        if (!out) {
            std::cerr << "Error opening output file: " << output_file << "\n";
            return 1;
        }
        out.write(jsonl_buffer.data(), jsonl_buffer.size());
        if (!out) {
            std::cerr << "Error writing output file\n";
            return 1;
        }
    }

    std::cerr << "Done! Output written to: " << output_file << "\n";
    return 0;
}
