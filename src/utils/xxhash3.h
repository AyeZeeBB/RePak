//=============================================================================//
//
// xxHash3 wrapper - enables XXH3 from the zstd-bundled xxhash
//
// This header must be included BEFORE any other xxhash includes to properly
// configure XXH3 support. The zstd bundled xxhash disables XXH3 by default.
//
//=============================================================================//
#pragma once

// Undefine the ZSTD restrictions before including xxhash
#ifdef XXH_NO_XXH3
#undef XXH_NO_XXH3
#endif

// Use inline implementation for maximum performance
#define XXH_INLINE_ALL

// Include the full xxhash implementation
#include <thirdparty/zstd/common/xxhash.h>
