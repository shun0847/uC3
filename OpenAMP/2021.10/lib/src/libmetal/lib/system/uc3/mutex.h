/*
 * Copyright (c) 2017-2020, eForce, Co. Ltd and Contributors. All rights reserved.
 * Copyright (c) 2016, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	uc3/mutex.h
 * @brief	uC3 mutex primitives for libmetal.
 */

#ifndef __METAL_MUTEX__H__
#error "Include metal/mutex.h instead of metal/uc3/mutex.h"
#endif

#ifndef __METAL_UC3_MUTEX__H__
#define __METAL_UC3_MUTEX__H__

#include <metal/assert.h>
#include "kernel.h"

#include <metal/atomic.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	ID m;
} metal_mutex_t;

/*
 * METAL_MUTEX_INIT - used for initializing an mutex elmenet in a static struct
 * or global
 */
#define METAL_MUTEX_INIT(m) { (ID)0 }
/*
 * METAL_MUTEX_DEFINE - used for defining and initializing a global or
 * static singleton mutex
 */
#define METAL_MUTEX_DEFINE(m) metal_mutex_t m = METAL_MUTEX_INIT(m)

static inline void __metal_mutex_init(metal_mutex_t *mutex)
{
    T_CSEM csem;
	metal_assert(mutex);
	csem.sematr = TA_TFIFO;
	csem.isemcnt = 1U;
	csem.maxsem =  10U;
	csem.name = "env_create_mutex(sem)";
	mutex->m = acre_sem(&csem);
	metal_assert(mutex->m >= 0);
}

static inline void __metal_mutex_deinit(metal_mutex_t *mutex)
{
	metal_assert(mutex && (mutex->m >= 0));
	(void)del_sem(mutex->m);
	mutex->m=E_ID;
}

static inline int __metal_mutex_try_acquire(metal_mutex_t *mutex)
{
	metal_assert(mutex && (mutex->m >= 0));
	return pol_sem(mutex->m);
}

static inline void __metal_mutex_acquire(metal_mutex_t *mutex)
{
	metal_assert(mutex && (mutex->m >= 0));
	wai_sem(mutex->m);
}

static inline void __metal_mutex_release(metal_mutex_t *mutex)
{
	metal_assert(mutex && (mutex->m >= 0));
	sig_sem(mutex->m);
}

static inline int __metal_mutex_is_acquired(metal_mutex_t *mutex)
{
	T_RSEM rsem;
	metal_assert(mutex && (mutex->m >= 0));
	ref_sem(mutex->m, &rsem);
	return (rsem.semcnt != 0) ? 0 : 1;
}

#ifdef __cplusplus
}
#endif

#endif /* __METAL_UC3_MUTEX__H__ */
