// SPDX-License-Identifier: MIT
// Copyright 2023 Light Transport Entertainment Inc.
//
// Simple huggingface safetensor loader in C++14
// inspired from: https://gist.github.com/Narsil/5d6bf307995158ad2c4994f323967284
// dependency: Nlohmann JSON.hpp
#pragma once

#include <vector>

#include "nlohmann/json.hpp"

namespace huggingface {

namespace safetensors {
    enum dtype {
        kBOOL,
        kUINT8,
        kINT8,
        kINT16,
        kUINT16,
        kFLOAT16,
        kBFLOAT16,
        kINT32,
        kUINT32,
        kFLOAT32,
        kFLOAT64,
        kINT64,
        kUINT64,
    };

    NLOHMANN_JSON_SERIALIZE_ENUM(dtype, {
        { kBOOL, "BOOL" },
        { kUINT8, "U8" },
        { kINT8, "I8" },
        { kINT16, "I16" },
        { kUINT16, "U16" },
        { kFLOAT16, "F16" },
        { kBFLOAT16, "BF16" },
        { kINT32, "I32" },
        { kUINT32, "U32" },
        { kFLOAT32, "F32" },
        { kFLOAT64, "F64" },
        { kINT64, "I64" },
        { kUINT64, "U64" },
    })

    struct metadata_t {
        dtype dtype;
        std::vector<size_t> shape;
        std::pair<size_t, size_t> data_offsets;
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(metadata_t, dtype, shape, data_offsets)

} // namespace safetensors
} // namespace huggingface
