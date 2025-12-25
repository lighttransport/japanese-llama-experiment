// SPDX-License-Identifier: MIT
// minparquet.h - Minimal Apache Parquet reader in C++17
// Single-header library with optional GZIP/ZSTD support
//
// Usage:
//   #define MINPQ_IMPLEMENTATION
//   #include "minparquet.h"
//
// Configuration (define before including):
//   #define MINPQ_NO_GZIP    - Disable GZIP decompression
//   #define MINPQ_NO_ZSTD    - Disable ZSTD decompression

#ifndef MINPARQUET_H_
#define MINPARQUET_H_

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <memory>
#include <fstream>
#include <algorithm>
#include <unordered_map>

// Forward declarations for compression libraries
#ifndef MINPQ_NO_GZIP
extern "C" {
    // miniz declarations
    unsigned long mz_compressBound(unsigned long source_len);
    int mz_uncompress(unsigned char *pDest, unsigned long *pDest_len,
                      const unsigned char *pSource, unsigned long source_len);
}
#endif

#ifndef MINPQ_NO_ZSTD
extern "C" {
    size_t ZSTD_decompress(void* dst, size_t dstCapacity,
                           const void* src, size_t compressedSize);
    unsigned ZSTD_isError(size_t code);
    unsigned long long ZSTD_getFrameContentSize(const void *src, size_t srcSize);
}
#endif

namespace minpq {

// ============================================================================
// Parquet Enums (from parquet.thrift)
// ============================================================================

enum class Type : int32_t {
    BOOLEAN = 0,
    INT32 = 1,
    INT64 = 2,
    INT96 = 3,
    FLOAT = 4,
    DOUBLE = 5,
    BYTE_ARRAY = 6,
    FIXED_LEN_BYTE_ARRAY = 7,
};

enum class ConvertedType : int32_t {
    NONE = -1,
    UTF8 = 0,
    MAP = 1,
    MAP_KEY_VALUE = 2,
    LIST = 3,
    ENUM = 4,
    DECIMAL = 5,
    DATE = 6,
    TIME_MILLIS = 7,
    TIME_MICROS = 8,
    TIMESTAMP_MILLIS = 9,
    TIMESTAMP_MICROS = 10,
    UINT_8 = 11,
    UINT_16 = 12,
    UINT_32 = 13,
    UINT_64 = 14,
    INT_8 = 15,
    INT_16 = 16,
    INT_32 = 17,
    INT_64 = 18,
    JSON = 19,
    BSON = 20,
    INTERVAL = 21,
};

enum class FieldRepetitionType : int32_t {
    REQUIRED = 0,
    OPTIONAL = 1,
    REPEATED = 2,
};

enum class Encoding : int32_t {
    PLAIN = 0,
    GROUP_VAR_INT = 1,  // Deprecated
    PLAIN_DICTIONARY = 2,
    RLE = 3,
    BIT_PACKED = 4,  // Deprecated
    DELTA_BINARY_PACKED = 5,
    DELTA_LENGTH_BYTE_ARRAY = 6,
    DELTA_BYTE_ARRAY = 7,
    RLE_DICTIONARY = 8,
    BYTE_STREAM_SPLIT = 9,
};

enum class CompressionCodec : int32_t {
    UNCOMPRESSED = 0,
    SNAPPY = 1,
    GZIP = 2,
    LZO = 3,
    BROTLI = 4,
    LZ4 = 5,
    ZSTD = 6,
    LZ4_RAW = 7,
};

enum class PageType : int32_t {
    DATA_PAGE = 0,
    INDEX_PAGE = 1,
    DICTIONARY_PAGE = 2,
    DATA_PAGE_V2 = 3,
};

// ============================================================================
// Parquet Structures
// ============================================================================

struct SchemaElement {
    std::optional<Type> type;
    std::optional<int32_t> type_length;
    std::optional<FieldRepetitionType> repetition_type;
    std::string name;
    std::optional<int32_t> num_children;
    std::optional<ConvertedType> converted_type;
    std::optional<int32_t> scale;
    std::optional<int32_t> precision;
    std::optional<int32_t> field_id;
};

struct Statistics {
    std::vector<uint8_t> max;
    std::vector<uint8_t> min;
    std::optional<int64_t> null_count;
    std::optional<int64_t> distinct_count;
    std::vector<uint8_t> max_value;
    std::vector<uint8_t> min_value;
};

struct PageEncodingStats {
    PageType page_type;
    Encoding encoding;
    int32_t count;
};

struct ColumnMetaData {
    Type type;
    std::vector<Encoding> encodings;
    std::vector<std::string> path_in_schema;
    CompressionCodec codec;
    int64_t num_values;
    int64_t total_uncompressed_size;
    int64_t total_compressed_size;
    std::optional<std::vector<std::pair<int64_t, int32_t>>> key_value_metadata;
    int64_t data_page_offset;
    std::optional<int64_t> index_page_offset;
    std::optional<int64_t> dictionary_page_offset;
    std::optional<Statistics> statistics;
    std::optional<std::vector<PageEncodingStats>> encoding_stats;
};

struct ColumnChunk {
    std::optional<std::string> file_path;
    int64_t file_offset;
    std::optional<ColumnMetaData> meta_data;
    std::optional<int64_t> offset_index_offset;
    std::optional<int32_t> offset_index_length;
    std::optional<int64_t> column_index_offset;
    std::optional<int32_t> column_index_length;
};

struct RowGroup {
    std::vector<ColumnChunk> columns;
    int64_t total_byte_size;
    int64_t num_rows;
    std::optional<std::vector<std::pair<int64_t, int64_t>>> sorting_columns;
    std::optional<int64_t> file_offset;
    std::optional<int64_t> total_compressed_size;
    std::optional<int16_t> ordinal;
};

struct KeyValue {
    std::string key;
    std::optional<std::string> value;
};

struct FileMetaData {
    int32_t version;
    std::vector<SchemaElement> schema;
    int64_t num_rows;
    std::vector<RowGroup> row_groups;
    std::optional<std::vector<KeyValue>> key_value_metadata;
    std::optional<std::string> created_by;
    std::optional<std::vector<ColumnChunk>> column_orders;
    std::optional<std::string> footer_signing_key_metadata;
};

struct DataPageHeader {
    int32_t num_values;
    Encoding encoding;
    Encoding definition_level_encoding;
    Encoding repetition_level_encoding;
    std::optional<Statistics> statistics;
};

struct DataPageHeaderV2 {
    int32_t num_values;
    int32_t num_nulls;
    int32_t num_rows;
    Encoding encoding;
    int32_t definition_levels_byte_length;
    int32_t repetition_levels_byte_length;
    std::optional<bool> is_compressed;
    std::optional<Statistics> statistics;
};

struct DictionaryPageHeader {
    int32_t num_values;
    Encoding encoding;
    std::optional<bool> is_sorted;
};

struct PageHeader {
    PageType type;
    int32_t uncompressed_page_size;
    int32_t compressed_page_size;
    std::optional<int32_t> crc;
    std::optional<DataPageHeader> data_page_header;
    std::optional<DataPageHeaderV2> data_page_header_v2;
    std::optional<DictionaryPageHeader> dictionary_page_header;
};

// ============================================================================
// Thrift Compact Protocol Decoder
// ============================================================================

class ThriftDecoder {
public:
    ThriftDecoder(const uint8_t* data, size_t size)
        : data_(data), size_(size), pos_(0), last_field_id_(0) {}

    size_t position() const { return pos_; }
    bool has_more() const { return pos_ < size_; }

    // Save current field_id state and reset for nested struct
    int16_t push_field_context() {
        int16_t saved = last_field_id_;
        last_field_id_ = 0;
        return saved;
    }

    // Restore field_id state after nested struct
    void pop_field_context(int16_t saved) {
        last_field_id_ = saved;
    }

    void reset_field_id() { last_field_id_ = 0; }

    // Read unsigned varint
    uint64_t read_varint() {
        uint64_t result = 0;
        int shift = 0;
        while (pos_ < size_) {
            uint8_t b = data_[pos_++];
            result |= static_cast<uint64_t>(b & 0x7F) << shift;
            if ((b & 0x80) == 0) break;
            shift += 7;
        }
        return result;
    }

    // Read zigzag-encoded varint
    int64_t read_zigzag() {
        uint64_t n = read_varint();
        return static_cast<int64_t>((n >> 1) ^ -(static_cast<int64_t>(n) & 1));
    }

    int32_t read_i16() { return static_cast<int16_t>(read_zigzag()); }
    int32_t read_i32() { return static_cast<int32_t>(read_zigzag()); }
    int64_t read_i64() { return read_zigzag(); }

    double read_double() {
        if (pos_ + 8 > size_) return 0.0;
        double result;
        std::memcpy(&result, data_ + pos_, 8);
        pos_ += 8;
        return result;
    }

    float read_float() {
        if (pos_ + 4 > size_) return 0.0f;
        float result;
        std::memcpy(&result, data_ + pos_, 4);
        pos_ += 4;
        return result;
    }

    bool read_bool_value(uint8_t type_byte) {
        // For compact protocol, bool value is encoded in type
        return (type_byte & 0x0F) == 1;  // TRUE=1, FALSE=2
    }

    std::string read_string() {
        uint64_t len = read_varint();
        if (pos_ + len > size_) return "";
        std::string result(reinterpret_cast<const char*>(data_ + pos_), len);
        pos_ += len;
        return result;
    }

    std::vector<uint8_t> read_binary() {
        uint64_t len = read_varint();
        if (pos_ + len > size_) return {};
        std::vector<uint8_t> result(data_ + pos_, data_ + pos_ + len);
        pos_ += len;
        return result;
    }

    void skip_bytes(size_t n) {
        pos_ = std::min(pos_ + n, size_);
    }

    // Read field header, returns (field_id, type) or (0, 0) for STOP
    std::pair<int16_t, uint8_t> read_field_header() {
        if (pos_ >= size_) return {0, 0};

        uint8_t byte = data_[pos_++];
        if (byte == 0) return {0, 0};  // STOP

        uint8_t type = byte & 0x0F;
        int16_t field_id_delta = (byte >> 4) & 0x0F;

        int16_t field_id;
        if (field_id_delta == 0) {
            field_id = static_cast<int16_t>(read_zigzag());
        } else {
            field_id = last_field_id_ + field_id_delta;
        }
        last_field_id_ = field_id;

        return {field_id, type};
    }

    // Skip a field of given type
    void skip_field(uint8_t type) {
        switch (type) {
            case 1: case 2:  // TRUE, FALSE (bool)
                break;
            case 3:  // I8
                pos_++;
                break;
            case 4:  // I16
            case 5:  // I32
            case 6:  // I64
                read_varint();
                break;
            case 7:  // DOUBLE
                pos_ += 8;
                break;
            case 8:  // BINARY/STRING
                skip_bytes(static_cast<size_t>(read_varint()));
                break;
            case 9:  // LIST
            case 10: // SET
                skip_list();
                break;
            case 11: // MAP
                skip_map();
                break;
            case 12: // STRUCT
                skip_struct();
                break;
            default:
                break;
        }
    }

    void skip_struct() {
        int16_t saved = push_field_context();
        while (true) {
            auto [fid, type] = read_field_header();
            if (type == 0) break;
            skip_field(type);
        }
        pop_field_context(saved);
    }

    void skip_list() {
        uint8_t header = data_[pos_++];
        uint8_t elem_type = header & 0x0F;
        uint32_t size = (header >> 4) & 0x0F;
        if (size == 0x0F) {
            size = static_cast<uint32_t>(read_varint());
        }
        for (uint32_t i = 0; i < size; i++) {
            skip_field(elem_type);
        }
    }

    void skip_map() {
        uint32_t size = static_cast<uint32_t>(read_varint());
        if (size == 0) return;
        uint8_t types = data_[pos_++];
        uint8_t key_type = (types >> 4) & 0x0F;
        uint8_t val_type = types & 0x0F;
        for (uint32_t i = 0; i < size; i++) {
            skip_field(key_type);
            skip_field(val_type);
        }
    }

    // List reading helpers
    std::pair<uint8_t, uint32_t> read_list_header() {
        uint8_t header = data_[pos_++];
        uint8_t elem_type = header & 0x0F;
        uint32_t size = (header >> 4) & 0x0F;
        if (size == 0x0F) {
            size = static_cast<uint32_t>(read_varint());
        }
        return {elem_type, size};
    }

private:
    const uint8_t* data_;
    size_t size_;
    size_t pos_;
    int16_t last_field_id_;
};

// ============================================================================
// Parquet Metadata Parser
// ============================================================================

class MetadataParser {
public:
    static std::optional<FileMetaData> parse(const uint8_t* data, size_t size) {
        ThriftDecoder dec(data, size);
        return parse_file_metadata(dec);
    }

private:
    static std::optional<FileMetaData> parse_file_metadata(ThriftDecoder& dec) {
        FileMetaData meta;
        int16_t saved = dec.push_field_context();

        while (true) {
            auto [field_id, type] = dec.read_field_header();
            if (type == 0) break;

            switch (field_id) {
                case 1: meta.version = dec.read_i32(); break;
                case 2: {
                    auto [elem_type, count] = dec.read_list_header();
                    for (uint32_t i = 0; i < count; i++) {
                        if (auto se = parse_schema_element(dec)) {
                            meta.schema.push_back(std::move(*se));
                        }
                    }
                    break;
                }
                case 3: meta.num_rows = dec.read_i64(); break;
                case 4: {
                    auto [elem_type, count] = dec.read_list_header();
                    for (uint32_t i = 0; i < count; i++) {
                        if (auto rg = parse_row_group(dec)) {
                            meta.row_groups.push_back(std::move(*rg));
                        }
                    }
                    break;
                }
                case 5: {
                    auto [elem_type, count] = dec.read_list_header();
                    meta.key_value_metadata = std::vector<KeyValue>();
                    for (uint32_t i = 0; i < count; i++) {
                        if (auto kv = parse_key_value(dec)) {
                            meta.key_value_metadata->push_back(std::move(*kv));
                        }
                    }
                    break;
                }
                case 6: meta.created_by = dec.read_string(); break;
                default: dec.skip_field(type); break;
            }
        }
        dec.pop_field_context(saved);
        return meta;
    }

    static std::optional<SchemaElement> parse_schema_element(ThriftDecoder& dec) {
        SchemaElement se;
        int16_t saved = dec.push_field_context();

        while (true) {
            auto [field_id, type] = dec.read_field_header();
            if (type == 0) break;

            switch (field_id) {
                case 1: se.type = static_cast<Type>(dec.read_i32()); break;
                case 2: se.type_length = dec.read_i32(); break;
                case 3: se.repetition_type = static_cast<FieldRepetitionType>(dec.read_i32()); break;
                case 4: se.name = dec.read_string(); break;
                case 5: se.num_children = dec.read_i32(); break;
                case 6: se.converted_type = static_cast<ConvertedType>(dec.read_i32()); break;
                case 7: se.scale = dec.read_i32(); break;
                case 8: se.precision = dec.read_i32(); break;
                case 9: se.field_id = dec.read_i32(); break;
                default: dec.skip_field(type); break;
            }
        }
        dec.pop_field_context(saved);
        return se;
    }

    static std::optional<RowGroup> parse_row_group(ThriftDecoder& dec) {
        RowGroup rg;
        int16_t saved = dec.push_field_context();

        while (true) {
            auto [field_id, type] = dec.read_field_header();
            if (type == 0) break;

            switch (field_id) {
                case 1: {
                    auto [elem_type, count] = dec.read_list_header();
                    for (uint32_t i = 0; i < count; i++) {
                        if (auto cc = parse_column_chunk(dec)) {
                            rg.columns.push_back(std::move(*cc));
                        }
                    }
                    break;
                }
                case 2: rg.total_byte_size = dec.read_i64(); break;
                case 3: rg.num_rows = dec.read_i64(); break;
                case 5: rg.file_offset = dec.read_i64(); break;
                case 6: rg.total_compressed_size = dec.read_i64(); break;
                case 7: rg.ordinal = static_cast<int16_t>(dec.read_i16()); break;
                default: dec.skip_field(type); break;
            }
        }
        dec.pop_field_context(saved);
        return rg;
    }

    static std::optional<ColumnChunk> parse_column_chunk(ThriftDecoder& dec) {
        ColumnChunk cc;
        int16_t saved = dec.push_field_context();

        while (true) {
            auto [field_id, type] = dec.read_field_header();
            if (type == 0) break;

            switch (field_id) {
                case 1: cc.file_path = dec.read_string(); break;
                case 2: cc.file_offset = dec.read_i64(); break;
                case 3: cc.meta_data = parse_column_metadata(dec); break;
                case 4: cc.offset_index_offset = dec.read_i64(); break;
                case 5: cc.offset_index_length = dec.read_i32(); break;
                case 6: cc.column_index_offset = dec.read_i64(); break;
                case 7: cc.column_index_length = dec.read_i32(); break;
                default: dec.skip_field(type); break;
            }
        }
        dec.pop_field_context(saved);
        return cc;
    }

    static std::optional<ColumnMetaData> parse_column_metadata(ThriftDecoder& dec) {
        ColumnMetaData cm;
        int16_t saved = dec.push_field_context();

        while (true) {
            auto [field_id, type] = dec.read_field_header();
            if (type == 0) break;

            switch (field_id) {
                case 1: cm.type = static_cast<Type>(dec.read_i32()); break;
                case 2: {
                    auto [elem_type, count] = dec.read_list_header();
                    for (uint32_t i = 0; i < count; i++) {
                        cm.encodings.push_back(static_cast<Encoding>(dec.read_i32()));
                    }
                    break;
                }
                case 3: {
                    auto [elem_type, count] = dec.read_list_header();
                    for (uint32_t i = 0; i < count; i++) {
                        cm.path_in_schema.push_back(dec.read_string());
                    }
                    break;
                }
                case 4: cm.codec = static_cast<CompressionCodec>(dec.read_i32()); break;
                case 5: cm.num_values = dec.read_i64(); break;
                case 6: cm.total_uncompressed_size = dec.read_i64(); break;
                case 7: cm.total_compressed_size = dec.read_i64(); break;
                case 9: cm.data_page_offset = dec.read_i64(); break;
                case 10: cm.index_page_offset = dec.read_i64(); break;
                case 11: cm.dictionary_page_offset = dec.read_i64(); break;
                case 12: cm.statistics = parse_statistics(dec); break;
                default: dec.skip_field(type); break;
            }
        }
        dec.pop_field_context(saved);
        return cm;
    }

    static std::optional<Statistics> parse_statistics(ThriftDecoder& dec) {
        Statistics stats;
        int16_t saved = dec.push_field_context();

        while (true) {
            auto [field_id, type] = dec.read_field_header();
            if (type == 0) break;

            switch (field_id) {
                case 1: stats.max = dec.read_binary(); break;
                case 2: stats.min = dec.read_binary(); break;
                case 3: stats.null_count = dec.read_i64(); break;
                case 4: stats.distinct_count = dec.read_i64(); break;
                case 5: stats.max_value = dec.read_binary(); break;
                case 6: stats.min_value = dec.read_binary(); break;
                default: dec.skip_field(type); break;
            }
        }
        dec.pop_field_context(saved);
        return stats;
    }

    static std::optional<KeyValue> parse_key_value(ThriftDecoder& dec) {
        KeyValue kv;
        int16_t saved = dec.push_field_context();

        while (true) {
            auto [field_id, type] = dec.read_field_header();
            if (type == 0) break;

            switch (field_id) {
                case 1: kv.key = dec.read_string(); break;
                case 2: kv.value = dec.read_string(); break;
                default: dec.skip_field(type); break;
            }
        }
        dec.pop_field_context(saved);
        return kv;
    }
};

// ============================================================================
// Page Header Parser
// ============================================================================

class PageHeaderParser {
public:
    static std::optional<PageHeader> parse(const uint8_t* data, size_t size, size_t& bytes_read) {
        ThriftDecoder dec(data, size);
        auto result = parse_page_header(dec);
        bytes_read = dec.position();
        return result;
    }

private:
    static std::optional<PageHeader> parse_page_header(ThriftDecoder& dec) {
        PageHeader ph;
        int16_t saved = dec.push_field_context();

        while (true) {
            auto [field_id, type] = dec.read_field_header();
            if (type == 0) break;

            switch (field_id) {
                case 1: ph.type = static_cast<PageType>(dec.read_i32()); break;
                case 2: ph.uncompressed_page_size = dec.read_i32(); break;
                case 3: ph.compressed_page_size = dec.read_i32(); break;
                case 4: ph.crc = dec.read_i32(); break;
                case 5: ph.data_page_header = parse_data_page_header(dec); break;
                case 6: dec.skip_struct(); break;  // IndexPageHeader
                case 7: ph.dictionary_page_header = parse_dictionary_page_header(dec); break;
                case 8: ph.data_page_header_v2 = parse_data_page_header_v2(dec); break;
                default: dec.skip_field(type); break;
            }
        }
        dec.pop_field_context(saved);
        return ph;
    }

    static std::optional<DataPageHeader> parse_data_page_header(ThriftDecoder& dec) {
        DataPageHeader dph;
        int16_t saved = dec.push_field_context();

        while (true) {
            auto [field_id, type] = dec.read_field_header();
            if (type == 0) break;

            switch (field_id) {
                case 1: dph.num_values = dec.read_i32(); break;
                case 2: dph.encoding = static_cast<Encoding>(dec.read_i32()); break;
                case 3: dph.definition_level_encoding = static_cast<Encoding>(dec.read_i32()); break;
                case 4: dph.repetition_level_encoding = static_cast<Encoding>(dec.read_i32()); break;
                case 5: dph.statistics = parse_statistics(dec); break;
                default: dec.skip_field(type); break;
            }
        }
        dec.pop_field_context(saved);
        return dph;
    }

    static std::optional<DataPageHeaderV2> parse_data_page_header_v2(ThriftDecoder& dec) {
        DataPageHeaderV2 dph;
        int16_t saved = dec.push_field_context();

        while (true) {
            auto [field_id, type] = dec.read_field_header();
            if (type == 0) break;

            switch (field_id) {
                case 1: dph.num_values = dec.read_i32(); break;
                case 2: dph.num_nulls = dec.read_i32(); break;
                case 3: dph.num_rows = dec.read_i32(); break;
                case 4: dph.encoding = static_cast<Encoding>(dec.read_i32()); break;
                case 5: dph.definition_levels_byte_length = dec.read_i32(); break;
                case 6: dph.repetition_levels_byte_length = dec.read_i32(); break;
                case 7: dph.is_compressed = (type == 1); break;  // bool from type
                case 8: dph.statistics = parse_statistics(dec); break;
                default: dec.skip_field(type); break;
            }
        }
        dec.pop_field_context(saved);
        return dph;
    }

    static std::optional<DictionaryPageHeader> parse_dictionary_page_header(ThriftDecoder& dec) {
        DictionaryPageHeader dph;
        int16_t saved = dec.push_field_context();

        while (true) {
            auto [field_id, type] = dec.read_field_header();
            if (type == 0) break;

            switch (field_id) {
                case 1: dph.num_values = dec.read_i32(); break;
                case 2: dph.encoding = static_cast<Encoding>(dec.read_i32()); break;
                case 3: dph.is_sorted = (type == 1); break;
                default: dec.skip_field(type); break;
            }
        }
        dec.pop_field_context(saved);
        return dph;
    }

    static std::optional<Statistics> parse_statistics(ThriftDecoder& dec) {
        Statistics stats;
        int16_t saved = dec.push_field_context();

        while (true) {
            auto [field_id, type] = dec.read_field_header();
            if (type == 0) break;

            switch (field_id) {
                case 1: stats.max = dec.read_binary(); break;
                case 2: stats.min = dec.read_binary(); break;
                case 3: stats.null_count = dec.read_i64(); break;
                case 4: stats.distinct_count = dec.read_i64(); break;
                case 5: stats.max_value = dec.read_binary(); break;
                case 6: stats.min_value = dec.read_binary(); break;
                default: dec.skip_field(type); break;
            }
        }
        dec.pop_field_context(saved);
        return stats;
    }
};

// ============================================================================
// RLE/Bit-Packing Hybrid Decoder
// ============================================================================

class RleBitPackingDecoder {
public:
    RleBitPackingDecoder(const uint8_t* data, size_t size, int bit_width)
        : data_(data), size_(size), pos_(0), bit_width_(bit_width),
          current_count_(0), current_value_(0), is_literal_(false),
          literal_pos_(0), buffer_pos_(0) {
        if (bit_width_ > 0) {
            bytes_per_value_ = (bit_width_ + 7) / 8;
        } else {
            bytes_per_value_ = 0;
        }
    }

    bool get_next(int32_t& value) {
        if (current_count_ == 0) {
            if (!read_next_group()) {
                return false;
            }
        }

        if (is_literal_) {
            value = read_packed_value();
        } else {
            value = current_value_;
        }
        current_count_--;
        return true;
    }

    void get_batch(std::vector<int32_t>& values, size_t count) {
        values.reserve(values.size() + count);
        for (size_t i = 0; i < count; i++) {
            int32_t v;
            if (get_next(v)) {
                values.push_back(v);
            } else {
                break;
            }
        }
    }

private:
    bool read_next_group() {
        if (pos_ >= size_) return false;

        // Read header as varint
        uint32_t header = 0;
        int shift = 0;
        while (pos_ < size_) {
            uint8_t b = data_[pos_++];
            header |= static_cast<uint32_t>(b & 0x7F) << shift;
            if ((b & 0x80) == 0) break;
            shift += 7;
        }

        is_literal_ = (header & 1) != 0;
        if (is_literal_) {
            // Bit-packed run
            current_count_ = ((header >> 1) * 8);
            literal_pos_ = 0;
            buffer_pos_ = 0;
        } else {
            // RLE run
            current_count_ = header >> 1;
            // Read value
            current_value_ = 0;
            for (int i = 0; i < bytes_per_value_ && pos_ < size_; i++) {
                current_value_ |= static_cast<int32_t>(data_[pos_++]) << (i * 8);
            }
        }
        return current_count_ > 0;
    }

    int32_t read_packed_value() {
        // Bit-pack reading
        int32_t value = 0;
        int bits_read = 0;

        while (bits_read < bit_width_) {
            if (buffer_pos_ == 0 && pos_ < size_) {
                current_byte_ = data_[pos_++];
            }

            int bits_available = 8 - buffer_pos_;
            int bits_to_read = std::min(bits_available, bit_width_ - bits_read);

            uint8_t mask = (1 << bits_to_read) - 1;
            value |= static_cast<int32_t>((current_byte_ >> buffer_pos_) & mask) << bits_read;

            bits_read += bits_to_read;
            buffer_pos_ += bits_to_read;

            if (buffer_pos_ >= 8) {
                buffer_pos_ = 0;
            }
        }

        literal_pos_++;
        return value;
    }

    const uint8_t* data_;
    size_t size_;
    size_t pos_;
    int bit_width_;
    int bytes_per_value_;

    uint32_t current_count_;
    int32_t current_value_;
    bool is_literal_;
    size_t literal_pos_;
    int buffer_pos_;
    uint8_t current_byte_;
};

// ============================================================================
// PLAIN Encoding Decoder
// ============================================================================

class PlainDecoder {
public:
    PlainDecoder(const uint8_t* data, size_t size)
        : data_(data), size_(size), pos_(0) {}

    template<typename T>
    bool read(T& value) {
        if (pos_ + sizeof(T) > size_) return false;
        std::memcpy(&value, data_ + pos_, sizeof(T));
        pos_ += sizeof(T);
        return true;
    }

    bool read_bool(bool& value) {
        if (pos_ >= size_) return false;
        value = data_[pos_++] != 0;
        return true;
    }

    bool read_byte_array(std::string& value) {
        if (pos_ + 4 > size_) return false;
        uint32_t len;
        std::memcpy(&len, data_ + pos_, 4);
        pos_ += 4;
        if (pos_ + len > size_) return false;
        value.assign(reinterpret_cast<const char*>(data_ + pos_), len);
        pos_ += len;
        return true;
    }

    bool read_byte_array(std::vector<uint8_t>& value) {
        if (pos_ + 4 > size_) return false;
        uint32_t len;
        std::memcpy(&len, data_ + pos_, 4);
        pos_ += 4;
        if (pos_ + len > size_) return false;
        value.assign(data_ + pos_, data_ + pos_ + len);
        pos_ += len;
        return true;
    }

    bool read_fixed_len_byte_array(std::vector<uint8_t>& value, size_t len) {
        if (pos_ + len > size_) return false;
        value.assign(data_ + pos_, data_ + pos_ + len);
        pos_ += len;
        return true;
    }

    size_t remaining() const { return size_ - pos_; }

private:
    const uint8_t* data_;
    size_t size_;
    size_t pos_;
};

// ============================================================================
// Decompression
// ============================================================================

class Decompressor {
public:
    static bool decompress(CompressionCodec codec,
                          const uint8_t* src, size_t src_size,
                          uint8_t* dst, size_t dst_size) {
        switch (codec) {
            case CompressionCodec::UNCOMPRESSED:
                if (src_size != dst_size) return false;
                std::memcpy(dst, src, src_size);
                return true;

#ifndef MINPQ_NO_GZIP
            case CompressionCodec::GZIP: {
                unsigned long dest_len = static_cast<unsigned long>(dst_size);
                int ret = mz_uncompress(dst, &dest_len, src, static_cast<unsigned long>(src_size));
                return ret == 0;  // MZ_OK
            }
#endif

#ifndef MINPQ_NO_ZSTD
            case CompressionCodec::ZSTD: {
                size_t ret = ZSTD_decompress(dst, dst_size, src, src_size);
                return !ZSTD_isError(ret);
            }
#endif

            default:
                return false;
        }
    }
};

// ============================================================================
// Column Data
// ============================================================================

struct ColumnData {
    std::vector<uint8_t> data;
    std::vector<int16_t> def_levels;
    std::vector<int16_t> rep_levels;
    size_t num_values;
    Type type;
    std::optional<int32_t> type_length;
};

// ============================================================================
// ParquetReader
// ============================================================================

class ParquetReader {
public:
    ParquetReader() = default;

    bool open(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file) {
            error_ = "Failed to open file: " + filename;
            return false;
        }

        size_t file_size = static_cast<size_t>(file.tellg());
        file.seekg(0, std::ios::beg);

        data_.resize(file_size);
        if (!file.read(reinterpret_cast<char*>(data_.data()), file_size)) {
            error_ = "Failed to read file";
            return false;
        }

        return parse();
    }

    bool open(const uint8_t* data, size_t size) {
        data_.assign(data, data + size);
        return parse();
    }

    size_t num_row_groups() const {
        return metadata_ ? metadata_->row_groups.size() : 0;
    }

    size_t num_columns() const {
        if (!metadata_ || metadata_->row_groups.empty()) return 0;
        return metadata_->row_groups[0].columns.size();
    }

    int64_t num_rows() const {
        return metadata_ ? metadata_->num_rows : 0;
    }

    int64_t num_rows_in_group(size_t row_group) const {
        if (!metadata_ || row_group >= metadata_->row_groups.size()) return 0;
        return metadata_->row_groups[row_group].num_rows;
    }

    std::string_view column_name(size_t col) const {
        if (!metadata_) return "";
        // Schema element 0 is root, column elements start at 1
        size_t schema_idx = col + 1;
        if (schema_idx >= metadata_->schema.size()) return "";
        return metadata_->schema[schema_idx].name;
    }

    std::optional<Type> column_type(size_t col) const {
        if (!metadata_) return std::nullopt;
        size_t schema_idx = col + 1;
        if (schema_idx >= metadata_->schema.size()) return std::nullopt;
        return metadata_->schema[schema_idx].type;
    }

    const SchemaElement* column_schema(size_t col) const {
        if (!metadata_) return nullptr;
        size_t schema_idx = col + 1;
        if (schema_idx >= metadata_->schema.size()) return nullptr;
        return &metadata_->schema[schema_idx];
    }

    const std::vector<SchemaElement>& schema() const {
        static std::vector<SchemaElement> empty;
        return metadata_ ? metadata_->schema : empty;
    }

    const FileMetaData* metadata() const {
        return metadata_ ? &*metadata_ : nullptr;
    }

    std::optional<ColumnData> read_column(size_t row_group, size_t column) {
        if (!metadata_) {
            error_ = "No metadata loaded";
            return std::nullopt;
        }
        if (row_group >= metadata_->row_groups.size()) {
            error_ = "Row group out of range";
            return std::nullopt;
        }
        const auto& rg = metadata_->row_groups[row_group];
        if (column >= rg.columns.size()) {
            error_ = "Column out of range";
            return std::nullopt;
        }

        const auto& cc = rg.columns[column];
        if (!cc.meta_data) {
            error_ = "Missing column metadata";
            return std::nullopt;
        }

        return read_column_chunk(cc, column);
    }

    // Typed convenience readers
    std::optional<std::vector<int32_t>> read_int32_column(size_t rg, size_t col) {
        auto data = read_column(rg, col);
        if (!data || data->type != Type::INT32) return std::nullopt;
        return decode_values<int32_t>(*data);
    }

    std::optional<std::vector<int64_t>> read_int64_column(size_t rg, size_t col) {
        auto data = read_column(rg, col);
        if (!data || data->type != Type::INT64) return std::nullopt;
        return decode_values<int64_t>(*data);
    }

    std::optional<std::vector<float>> read_float_column(size_t rg, size_t col) {
        auto data = read_column(rg, col);
        if (!data || data->type != Type::FLOAT) return std::nullopt;
        return decode_values<float>(*data);
    }

    std::optional<std::vector<double>> read_double_column(size_t rg, size_t col) {
        auto data = read_column(rg, col);
        if (!data || data->type != Type::DOUBLE) return std::nullopt;
        return decode_values<double>(*data);
    }

    std::optional<std::vector<std::string>> read_string_column(size_t rg, size_t col) {
        auto data = read_column(rg, col);
        if (!data || data->type != Type::BYTE_ARRAY) return std::nullopt;
        return decode_byte_array_values(*data);
    }

    std::optional<std::vector<bool>> read_bool_column(size_t rg, size_t col) {
        auto data = read_column(rg, col);
        if (!data || data->type != Type::BOOLEAN) return std::nullopt;
        return decode_bool_values(*data);
    }

    const std::string& error() const { return error_; }

private:
    bool parse() {
        if (data_.size() < 12) {
            error_ = "File too small";
            return false;
        }

        // Check magic bytes at start and end
        if (std::memcmp(data_.data(), "PAR1", 4) != 0) {
            error_ = "Invalid magic at start";
            return false;
        }
        if (std::memcmp(data_.data() + data_.size() - 4, "PAR1", 4) != 0) {
            error_ = "Invalid magic at end";
            return false;
        }

        // Read metadata length (4 bytes before final magic)
        uint32_t metadata_len;
        std::memcpy(&metadata_len, data_.data() + data_.size() - 8, 4);

        if (metadata_len + 8 > data_.size()) {
            error_ = "Invalid metadata length";
            return false;
        }

        // Parse metadata
        const uint8_t* metadata_start = data_.data() + data_.size() - 8 - metadata_len;
        metadata_ = MetadataParser::parse(metadata_start, metadata_len);
        if (!metadata_) {
            error_ = "Failed to parse metadata";
            return false;
        }

        return true;
    }

    std::optional<ColumnData> read_column_chunk(const ColumnChunk& cc, size_t col_idx) {
        const auto& meta = *cc.meta_data;
        ColumnData result;
        result.type = meta.type;

        // Get type length for fixed-length byte arrays
        const auto* schema = column_schema(col_idx);
        if (schema && schema->type_length) {
            result.type_length = *schema->type_length;
        }

        // Calculate max definition/repetition levels
        int max_def_level = 0;
        int max_rep_level = 0;
        if (schema) {
            // Simple heuristic: if optional, max_def = 1
            if (schema->repetition_type == FieldRepetitionType::OPTIONAL) {
                max_def_level = 1;
            }
            if (schema->repetition_type == FieldRepetitionType::REPEATED) {
                max_rep_level = 1;
            }
        }

        // Determine start offset
        int64_t offset = meta.dictionary_page_offset.value_or(meta.data_page_offset);
        int64_t end_offset = offset + meta.total_compressed_size;

        // Dictionary for dictionary encoding
        std::vector<uint8_t> dictionary_data;
        size_t dict_num_values = 0;

        // Read pages
        size_t pos = static_cast<size_t>(offset);
        while (pos < static_cast<size_t>(end_offset) && pos < data_.size()) {
            size_t header_size;
            auto page_header = PageHeaderParser::parse(data_.data() + pos,
                                                       data_.size() - pos,
                                                       header_size);
            if (!page_header) {
                error_ = "Failed to parse page header";
                return std::nullopt;
            }

            pos += header_size;

            // Get compressed data
            size_t compressed_size = static_cast<size_t>(page_header->compressed_page_size);
            size_t uncompressed_size = static_cast<size_t>(page_header->uncompressed_page_size);

            if (pos + compressed_size > data_.size()) {
                error_ = "Page data out of bounds";
                return std::nullopt;
            }

            // Decompress if needed
            std::vector<uint8_t> page_data;
            if (meta.codec == CompressionCodec::UNCOMPRESSED ||
                compressed_size == uncompressed_size) {
                page_data.assign(data_.data() + pos, data_.data() + pos + compressed_size);
            } else {
                page_data.resize(uncompressed_size);
                if (!Decompressor::decompress(meta.codec,
                                             data_.data() + pos, compressed_size,
                                             page_data.data(), uncompressed_size)) {
                    error_ = "Decompression failed";
                    return std::nullopt;
                }
            }

            pos += compressed_size;

            // Process page based on type
            if (page_header->type == PageType::DICTIONARY_PAGE) {
                if (page_header->dictionary_page_header) {
                    dict_num_values = static_cast<size_t>(
                        page_header->dictionary_page_header->num_values);
                }
                dictionary_data = std::move(page_data);
            } else if (page_header->type == PageType::DATA_PAGE ||
                       page_header->type == PageType::DATA_PAGE_V2) {
                if (!process_data_page(page_data, *page_header, meta,
                                      dictionary_data, dict_num_values,
                                      max_def_level, max_rep_level,
                                      result)) {
                    return std::nullopt;
                }
            }
        }

        return result;
    }

    bool process_data_page(const std::vector<uint8_t>& page_data,
                          const PageHeader& header,
                          const ColumnMetaData& meta,
                          const std::vector<uint8_t>& dictionary_data,
                          size_t dict_num_values,
                          int max_def_level,
                          int max_rep_level,
                          ColumnData& result) {
        const uint8_t* ptr = page_data.data();
        size_t remaining = page_data.size();
        int32_t num_values = 0;
        Encoding encoding = Encoding::PLAIN;

        if (header.type == PageType::DATA_PAGE_V2 && header.data_page_header_v2) {
            // DATA_PAGE V2: levels stored uncompressed at the start with explicit lengths
            num_values = header.data_page_header_v2->num_values;
            encoding = header.data_page_header_v2->encoding;

            int32_t rep_len = header.data_page_header_v2->repetition_levels_byte_length;
            int32_t def_len = header.data_page_header_v2->definition_levels_byte_length;

            if (max_rep_level > 0 && rep_len > 0) {
                int bit_width = bit_width_for_max(max_rep_level);
                RleBitPackingDecoder rep_decoder(ptr, rep_len, bit_width);
                for (int i = 0; i < num_values; i++) {
                    int32_t level;
                    if (rep_decoder.get_next(level)) {
                        result.rep_levels.push_back(static_cast<int16_t>(level));
                    }
                }
                ptr += rep_len;
                remaining -= rep_len;
            }

            if (max_def_level > 0 && def_len > 0) {
                int bit_width = bit_width_for_max(max_def_level);
                RleBitPackingDecoder def_decoder(ptr, def_len, bit_width);
                for (int i = 0; i < num_values; i++) {
                    int32_t level;
                    if (def_decoder.get_next(level)) {
                        result.def_levels.push_back(static_cast<int16_t>(level));
                    }
                }
                ptr += def_len;
                remaining -= def_len;
            }
        } else if (header.type == PageType::DATA_PAGE && header.data_page_header) {
            // DATA_PAGE V1: levels prefixed with 4-byte length
            num_values = header.data_page_header->num_values;
            encoding = header.data_page_header->encoding;

            if (max_rep_level > 0) {
                if (remaining < 4) return false;
                uint32_t rep_len;
                std::memcpy(&rep_len, ptr, 4);
                ptr += 4;
                remaining -= 4;

                if (remaining < rep_len) return false;
                int bit_width = bit_width_for_max(max_rep_level);
                RleBitPackingDecoder rep_decoder(ptr, rep_len, bit_width);
                for (int i = 0; i < num_values; i++) {
                    int32_t level;
                    if (rep_decoder.get_next(level)) {
                        result.rep_levels.push_back(static_cast<int16_t>(level));
                    }
                }
                ptr += rep_len;
                remaining -= rep_len;
            }

            if (max_def_level > 0) {
                if (remaining < 4) return false;
                uint32_t def_len;
                std::memcpy(&def_len, ptr, 4);
                ptr += 4;
                remaining -= 4;

                if (remaining < def_len) return false;
                int bit_width = bit_width_for_max(max_def_level);
                RleBitPackingDecoder def_decoder(ptr, def_len, bit_width);
                for (int i = 0; i < num_values; i++) {
                    int32_t level;
                    if (def_decoder.get_next(level)) {
                        result.def_levels.push_back(static_cast<int16_t>(level));
                    }
                }
                ptr += def_len;
                remaining -= def_len;
            }
        }

        // Count actual values (non-null)
        size_t actual_values = num_values;
        if (!result.def_levels.empty()) {
            actual_values = 0;
            for (auto dl : result.def_levels) {
                if (dl == max_def_level) actual_values++;
            }
        }

        // Decode values
        if (encoding == Encoding::PLAIN_DICTIONARY || encoding == Encoding::RLE_DICTIONARY) {
            // Dictionary encoding
            if (remaining < 1) return false;
            int bit_width = *ptr++;
            remaining--;

            RleBitPackingDecoder idx_decoder(ptr, remaining, bit_width);
            std::vector<int32_t> indices;
            for (size_t i = 0; i < actual_values; i++) {
                int32_t idx;
                if (idx_decoder.get_next(idx)) {
                    indices.push_back(idx);
                }
            }

            // Decode dictionary values
            decode_dictionary(meta.type, dictionary_data, dict_num_values,
                            indices, result);
        } else if (encoding == Encoding::PLAIN) {
            // Plain encoding - just copy raw bytes
            size_t old_size = result.data.size();
            result.data.resize(old_size + remaining);
            std::memcpy(result.data.data() + old_size, ptr, remaining);
        } else if (encoding == Encoding::RLE && meta.type == Type::BOOLEAN) {
            // Boolean RLE encoding
            if (remaining < 4) return false;
            uint32_t rle_len;
            std::memcpy(&rle_len, ptr, 4);
            ptr += 4;
            remaining -= 4;

            RleBitPackingDecoder bool_decoder(ptr, std::min(remaining, static_cast<size_t>(rle_len)), 1);
            for (size_t i = 0; i < actual_values; i++) {
                int32_t val;
                if (bool_decoder.get_next(val)) {
                    result.data.push_back(val ? 1 : 0);
                }
            }
        }

        result.num_values += actual_values;
        return true;
    }

    void decode_dictionary(Type type,
                          const std::vector<uint8_t>& dict_data,
                          size_t dict_num_values,
                          const std::vector<int32_t>& indices,
                          ColumnData& result) {
        PlainDecoder dict_decoder(dict_data.data(), dict_data.size());

        switch (type) {
            case Type::INT32: {
                std::vector<int32_t> dict_values(dict_num_values);
                for (size_t i = 0; i < dict_num_values; i++) {
                    dict_decoder.read(dict_values[i]);
                }
                for (int32_t idx : indices) {
                    if (idx >= 0 && static_cast<size_t>(idx) < dict_values.size()) {
                        int32_t val = dict_values[idx];
                        size_t old_size = result.data.size();
                        result.data.resize(old_size + sizeof(int32_t));
                        std::memcpy(result.data.data() + old_size, &val, sizeof(int32_t));
                    }
                }
                break;
            }
            case Type::INT64: {
                std::vector<int64_t> dict_values(dict_num_values);
                for (size_t i = 0; i < dict_num_values; i++) {
                    dict_decoder.read(dict_values[i]);
                }
                for (int32_t idx : indices) {
                    if (idx >= 0 && static_cast<size_t>(idx) < dict_values.size()) {
                        int64_t val = dict_values[idx];
                        size_t old_size = result.data.size();
                        result.data.resize(old_size + sizeof(int64_t));
                        std::memcpy(result.data.data() + old_size, &val, sizeof(int64_t));
                    }
                }
                break;
            }
            case Type::FLOAT: {
                std::vector<float> dict_values(dict_num_values);
                for (size_t i = 0; i < dict_num_values; i++) {
                    dict_decoder.read(dict_values[i]);
                }
                for (int32_t idx : indices) {
                    if (idx >= 0 && static_cast<size_t>(idx) < dict_values.size()) {
                        float val = dict_values[idx];
                        size_t old_size = result.data.size();
                        result.data.resize(old_size + sizeof(float));
                        std::memcpy(result.data.data() + old_size, &val, sizeof(float));
                    }
                }
                break;
            }
            case Type::DOUBLE: {
                std::vector<double> dict_values(dict_num_values);
                for (size_t i = 0; i < dict_num_values; i++) {
                    dict_decoder.read(dict_values[i]);
                }
                for (int32_t idx : indices) {
                    if (idx >= 0 && static_cast<size_t>(idx) < dict_values.size()) {
                        double val = dict_values[idx];
                        size_t old_size = result.data.size();
                        result.data.resize(old_size + sizeof(double));
                        std::memcpy(result.data.data() + old_size, &val, sizeof(double));
                    }
                }
                break;
            }
            case Type::BYTE_ARRAY: {
                std::vector<std::string> dict_values(dict_num_values);
                for (size_t i = 0; i < dict_num_values; i++) {
                    dict_decoder.read_byte_array(dict_values[i]);
                }
                for (int32_t idx : indices) {
                    if (idx >= 0 && static_cast<size_t>(idx) < dict_values.size()) {
                        const auto& val = dict_values[idx];
                        uint32_t len = static_cast<uint32_t>(val.size());
                        size_t old_size = result.data.size();
                        result.data.resize(old_size + 4 + len);
                        std::memcpy(result.data.data() + old_size, &len, 4);
                        std::memcpy(result.data.data() + old_size + 4, val.data(), len);
                    }
                }
                break;
            }
            default:
                break;
        }
    }

    template<typename T>
    std::vector<T> decode_values(const ColumnData& data) {
        std::vector<T> result;
        PlainDecoder decoder(data.data.data(), data.data.size());
        T val;
        while (decoder.read(val)) {
            result.push_back(val);
        }
        return result;
    }

    std::vector<std::string> decode_byte_array_values(const ColumnData& data) {
        std::vector<std::string> result;
        PlainDecoder decoder(data.data.data(), data.data.size());
        std::string val;
        while (decoder.read_byte_array(val)) {
            result.push_back(std::move(val));
            val.clear();
        }
        return result;
    }

    std::vector<bool> decode_bool_values(const ColumnData& data) {
        std::vector<bool> result;
        for (uint8_t b : data.data) {
            result.push_back(b != 0);
        }
        return result;
    }

    static int bit_width_for_max(int max_value) {
        if (max_value <= 0) return 0;
        int width = 0;
        while ((1 << width) <= max_value) width++;
        return width;
    }

    std::vector<uint8_t> data_;
    std::optional<FileMetaData> metadata_;
    std::string error_;
};

} // namespace minpq

#endif // MINPARQUET_H_
