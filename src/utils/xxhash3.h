//=============================================================================//
//
// xxHash3 wrapper for RePak
//
// Provides access to XXH3_128bits from the zstd-bundled xxhash.
// The zstd copy uses ZSTD_ namespace prefix, so we provide aliases.
//
//=============================================================================//
#pragma once

#include <thirdparty/zstd/common/xxhash.h>

// The zstd-bundled xxhash uses ZSTD_ namespace prefix
// Create convenience aliases to the namespaced versions
using XXH128_hash_t = ZSTD_XXH128_hash_t;

inline XXH128_hash_t XXH3_128bits_withSeed(const void* data, size_t len, unsigned long long seed)
{
    return ZSTD_XXH3_128bits_withSeed(data, len, seed);
}
