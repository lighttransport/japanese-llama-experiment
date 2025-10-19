// SPDX-License-Identifier: MIT
// Copyright 2025 - Present Light Transport Entertainment Inc.
// MurmurHash3 C++ wrapper
// Based on Austin Appleby's MurmurHash3 (public domain)

#pragma once

#include <cstddef>
#include <cstdint>

namespace murmurhash3 {

/**
 * @brief 32-bit MurmurHash3 (x86 version)
 * @param key Pointer to data to hash
 * @param len Length of data in bytes
 * @param seed Seed value
 * @param out Output hash value
 */
void hash_x86_32(const void* key, std::size_t len, uint32_t seed, void* out);

/**
 * @brief 128-bit MurmurHash3 (x86 version)
 * @param key Pointer to data to hash
 * @param len Length of data in bytes
 * @param seed Seed value
 * @param out Output hash value (16 bytes)
 */
void hash_x86_128(const void* key, std::size_t len, uint32_t seed, void* out);

/**
 * @brief 128-bit MurmurHash3 (x64 version)
 * @param key Pointer to data to hash
 * @param len Length of data in bytes
 * @param seed Seed value
 * @param out Output hash value (16 bytes)
 */
void hash_x64_128(const void* key, std::size_t len, uint32_t seed, void* out);

} // namespace murmurhash3
