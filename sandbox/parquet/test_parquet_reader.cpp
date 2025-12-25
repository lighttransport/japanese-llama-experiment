// SPDX-License-Identifier: MIT
// Test program for minparquet.h

#include <iostream>
#include <iomanip>
#include <cstdlib>

// Disable compression for now (add miniz/zstd if needed)
#define MINPQ_NO_GZIP
#define MINPQ_NO_ZSTD

#include "minparquet.h"

void print_type(minpq::Type type) {
    switch (type) {
        case minpq::Type::BOOLEAN: std::cout << "BOOLEAN"; break;
        case minpq::Type::INT32: std::cout << "INT32"; break;
        case minpq::Type::INT64: std::cout << "INT64"; break;
        case minpq::Type::INT96: std::cout << "INT96"; break;
        case minpq::Type::FLOAT: std::cout << "FLOAT"; break;
        case minpq::Type::DOUBLE: std::cout << "DOUBLE"; break;
        case minpq::Type::BYTE_ARRAY: std::cout << "BYTE_ARRAY"; break;
        case minpq::Type::FIXED_LEN_BYTE_ARRAY: std::cout << "FIXED_LEN_BYTE_ARRAY"; break;
    }
}

void print_converted_type(minpq::ConvertedType ct) {
    switch (ct) {
        case minpq::ConvertedType::NONE: break;
        case minpq::ConvertedType::UTF8: std::cout << " (UTF8)"; break;
        case minpq::ConvertedType::DATE: std::cout << " (DATE)"; break;
        case minpq::ConvertedType::TIMESTAMP_MILLIS: std::cout << " (TIMESTAMP_MILLIS)"; break;
        case minpq::ConvertedType::TIMESTAMP_MICROS: std::cout << " (TIMESTAMP_MICROS)"; break;
        case minpq::ConvertedType::INT_8: std::cout << " (INT8)"; break;
        case minpq::ConvertedType::INT_16: std::cout << " (INT16)"; break;
        case minpq::ConvertedType::INT_32: std::cout << " (INT32)"; break;
        case minpq::ConvertedType::INT_64: std::cout << " (INT64)"; break;
        case minpq::ConvertedType::UINT_8: std::cout << " (UINT8)"; break;
        case minpq::ConvertedType::UINT_16: std::cout << " (UINT16)"; break;
        case minpq::ConvertedType::UINT_32: std::cout << " (UINT32)"; break;
        case minpq::ConvertedType::UINT_64: std::cout << " (UINT64)"; break;
        case minpq::ConvertedType::DECIMAL: std::cout << " (DECIMAL)"; break;
        case minpq::ConvertedType::LIST: std::cout << " (LIST)"; break;
        case minpq::ConvertedType::MAP: std::cout << " (MAP)"; break;
        case minpq::ConvertedType::JSON: std::cout << " (JSON)"; break;
        default: std::cout << " (converted:" << static_cast<int>(ct) << ")"; break;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <parquet-file> [max-rows]\n";
        return 1;
    }

    const char* filename = argv[1];
    size_t max_rows = 10;
    if (argc >= 3) {
        max_rows = std::atoi(argv[2]);
    }

    minpq::ParquetReader reader;
    if (!reader.open(filename)) {
        std::cerr << "Error: " << reader.error() << "\n";
        return 1;
    }

    std::cout << "=== Parquet File: " << filename << " ===\n\n";

    // Print metadata
    const auto* meta = reader.metadata();
    if (meta) {
        std::cout << "Version: " << meta->version << "\n";
        std::cout << "Total rows: " << meta->num_rows << "\n";
        std::cout << "Row groups: " << meta->row_groups.size() << "\n";

        if (meta->created_by) {
            std::cout << "Created by: " << *meta->created_by << "\n";
        }

        if (meta->key_value_metadata) {
            std::cout << "\nKey-Value Metadata:\n";
            for (const auto& kv : *meta->key_value_metadata) {
                std::cout << "  " << kv.key;
                if (kv.value) {
                    // Truncate long values
                    std::string val = *kv.value;
                    if (val.size() > 100) {
                        val = val.substr(0, 100) + "...";
                    }
                    std::cout << " = " << val;
                }
                std::cout << "\n";
            }
        }
    }

    // Print schema
    std::cout << "\n=== Schema ===\n";
    const auto& schema = reader.schema();
    for (size_t i = 0; i < schema.size(); i++) {
        const auto& se = schema[i];
        std::cout << "  [" << i << "] " << se.name;
        if (se.type) {
            std::cout << ": ";
            print_type(*se.type);
            if (se.converted_type) {
                print_converted_type(*se.converted_type);
            }
        }
        if (se.num_children) {
            std::cout << " (children: " << *se.num_children << ")";
        }
        if (se.repetition_type) {
            switch (*se.repetition_type) {
                case minpq::FieldRepetitionType::REQUIRED: std::cout << " REQUIRED"; break;
                case minpq::FieldRepetitionType::OPTIONAL: std::cout << " OPTIONAL"; break;
                case minpq::FieldRepetitionType::REPEATED: std::cout << " REPEATED"; break;
            }
        }
        std::cout << "\n";
    }

    // Print row group info
    std::cout << "\n=== Row Groups ===\n";
    for (size_t rg = 0; rg < reader.num_row_groups(); rg++) {
        std::cout << "Row Group " << rg << ": " << reader.num_rows_in_group(rg) << " rows\n";
    }

    // Print some data
    std::cout << "\n=== Data (first " << max_rows << " rows) ===\n";

    size_t num_cols = reader.num_columns();
    if (num_cols == 0) {
        std::cout << "(no columns)\n";
        return 0;
    }

    // Print column headers
    std::cout << std::left;
    for (size_t col = 0; col < num_cols; col++) {
        std::cout << std::setw(20) << reader.column_name(col);
    }
    std::cout << "\n";
    for (size_t col = 0; col < num_cols; col++) {
        std::cout << std::setw(20) << std::string(18, '-');
    }
    std::cout << "\n";

    // Read and print data from first row group
    size_t rows_printed = 0;
    for (size_t rg = 0; rg < reader.num_row_groups() && rows_printed < max_rows; rg++) {
        // Read all columns for this row group
        std::vector<std::vector<std::string>> col_values(num_cols);

        for (size_t col = 0; col < num_cols; col++) {
            auto col_type = reader.column_type(col);
            if (!col_type) continue;

            switch (*col_type) {
                case minpq::Type::BOOLEAN: {
                    auto vals = reader.read_bool_column(rg, col);
                    if (vals) {
                        for (bool v : *vals) {
                            col_values[col].push_back(v ? "true" : "false");
                        }
                    }
                    break;
                }
                case minpq::Type::INT32: {
                    auto vals = reader.read_int32_column(rg, col);
                    if (vals) {
                        for (int32_t v : *vals) {
                            col_values[col].push_back(std::to_string(v));
                        }
                    }
                    break;
                }
                case minpq::Type::INT64: {
                    auto vals = reader.read_int64_column(rg, col);
                    if (vals) {
                        for (int64_t v : *vals) {
                            col_values[col].push_back(std::to_string(v));
                        }
                    }
                    break;
                }
                case minpq::Type::FLOAT: {
                    auto vals = reader.read_float_column(rg, col);
                    if (vals) {
                        for (float v : *vals) {
                            col_values[col].push_back(std::to_string(v));
                        }
                    }
                    break;
                }
                case minpq::Type::DOUBLE: {
                    auto vals = reader.read_double_column(rg, col);
                    if (vals) {
                        for (double v : *vals) {
                            col_values[col].push_back(std::to_string(v));
                        }
                    }
                    break;
                }
                case minpq::Type::BYTE_ARRAY: {
                    auto vals = reader.read_string_column(rg, col);
                    if (vals) {
                        for (const auto& v : *vals) {
                            std::string s = v;
                            // Truncate long strings
                            if (s.size() > 17) {
                                s = s.substr(0, 14) + "...";
                            }
                            // Escape non-printable
                            for (char& c : s) {
                                if (c < 32 || c > 126) c = '?';
                            }
                            col_values[col].push_back(s);
                        }
                    }
                    break;
                }
                default:
                    break;
            }
        }

        // Find max rows in this row group
        size_t max_col_rows = 0;
        for (const auto& cv : col_values) {
            max_col_rows = std::max(max_col_rows, cv.size());
        }

        // Print rows
        for (size_t row = 0; row < max_col_rows && rows_printed < max_rows; row++, rows_printed++) {
            for (size_t col = 0; col < num_cols; col++) {
                if (row < col_values[col].size()) {
                    std::cout << std::setw(20) << col_values[col][row];
                } else {
                    std::cout << std::setw(20) << "(null)";
                }
            }
            std::cout << "\n";
        }
    }

    std::cout << "\n=== Done ===\n";
    return 0;
}
