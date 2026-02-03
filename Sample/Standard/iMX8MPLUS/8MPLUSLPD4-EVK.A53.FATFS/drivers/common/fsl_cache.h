/*
 * Minimal cache wrapper for AArch64 CMSIS cache API.
 * Provides DCACHE_* symbols used by drivers/middleware.
 */

#ifndef _FSL_CACHE_H_
#define _FSL_CACHE_H_

#include "core_ca53.h"
#include "cache_armv8a.h"

static inline void DCACHE_CleanByRange(uint32_t addr, uint32_t size)
{
    dcache_clean_range((uintptr_t)addr, (size_t)size);
}

static inline void DCACHE_InvalidateByRange(uint32_t addr, uint32_t size)
{
    dcache_invalidate_range((uintptr_t)addr, (size_t)size);
}

static inline void DCACHE_CleanInvalidateByRange(uint32_t addr, uint32_t size)
{
    dcache_clean_invalidate_range((uintptr_t)addr, (size_t)size);
}

#endif /* _FSL_CACHE_H_ */
