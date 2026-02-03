/*
 * Copyright (c) 2018, Linaro Limited. and Contributors. All rights reserved.
 * Copyright (c) 2018, eForce.Co.Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	uc3/cache.h
 * @brief	uc3 cache operation primitives for libmetal.
 */

#ifndef __METAL_CACHE__H__
#error "Include metal/cache.h instead of metal/uc3/cache.h"
#endif

#ifndef __METAL_UC3_CACHE__H__
#define __METAL_UC3_CACHE__H__

#ifdef __cplusplus
extern "C" {
#endif

extern void metal_machine_cache_flush(void *addr, unsigned int len);
extern void metal_machine_cache_invalidate(void *addr, unsigned int len);

static inline void __metal_cache_flush(void *addr, unsigned int len)
{
	metal_machine_cache_flush(addr, len);
}

static inline void __metal_cache_invalidate(void *addr, unsigned int len)
{
	metal_machine_cache_invalidate(addr, len);
}

#ifdef __cplusplus
}
#endif

#endif /* __METAL_UC3_CACHE__H__ */
