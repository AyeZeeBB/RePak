//=============================================================================//
//
// xxHash3 wrapper for RePak
//
// Provides access to XXH3_128bits from the zstd-bundled xxhash.
// The zstd copy uses ZSTD_ namespace prefix via macros, so the standard
// XXH3_128bits_withSeed function name is automatically remapped.
//
//=============================================================================//
#pragma once

#include <thirdparty/zstd/common/xxhash.h>

// XXH3_128bits_withSeed is automatically remapped to ZSTD_XXH3_128bits_withSeed
// by the XXH_NAMESPACE macro in xxhash.h, so no additional work needed here.
// Just include this header and use XXH3_128bits_withSeed and XXH128_hash_t directly.
