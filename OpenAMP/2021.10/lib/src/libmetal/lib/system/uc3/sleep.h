/*
 * Copyright (c) 2018, Linaro Limited. and Contributors. All rights reserved.
 * Copyright (c) 2018, eForce Co Ltd. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	uc3/sleep.h
 * @brief	uc3 libmetal sleep handling.
 */

#ifndef __METAL_SLEEP__H__
#error "Include metal/sleep.h instead of metal/uc3/sleep.h"
#endif

#ifndef __METAL_UC3_SLEEP__H__
#define __METAL_UC3_SLEEP__H__

#include <metal/utilities.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int __metal_sleep_usec(unsigned int usec)
{
    (void)dly_tsk(usec/1000U);
    return 0;
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* __METAL_GENERIC_SLEEP__H__ */
