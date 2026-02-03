/*
 * Copyright (c) 2018, eForce.Co.Ltd. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	assert.h
 * @brief	Generic assertion support.
 */

#ifndef __METAL_ASSERT__H__
#error "Include metal/assert.h instead of metal/generic/assert.h"
#endif

#ifndef __METAL_UC3_ASSERT__H__
#define __METAL_UC3_ASSERT__H__

#include <assert.h>

/**
 * @brief Assertion macro for bare-metal applications.
 * @param cond Condition to evaluate.
 */
#if defined(__GNUC__)
/* Some GNU C compilers complain about missing _fstat_r, _getpid_r, _isatty_r,
 * _kill_r and _lseek_r derived from assert() in libc. For GNU C compilers, 
 * replace assert() by {} to remove warnings.
 */
#define metal_sys_assert(cond) {}
#else
#define metal_sys_assert(cond) assert(cond)
#endif

#endif /* __METAL_GENERIC_ASSERT__H__ */

