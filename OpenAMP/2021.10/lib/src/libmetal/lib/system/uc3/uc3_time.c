/*
 * Copyright (c) 2016, Xilinx Inc. and Contributors. All rights reserved.
 * Copyright (c) 2017-2018, eForce Co. Ltd. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	uc3/time.c
 * @brief	uc3 libmetal time handling.
 */

#include "kernel.h"
#include "metal/time.h"

unsigned long long metal_get_timestamp(void)
{
    unsigned long long ret;
    SYSTIM systim;
    if (get_tim(&systim) == E_OK) {
        ret = ((((unsigned long long)systim.utime) << 32) | (((unsigned long long)systim.ltime)));
    } else {
        ret = 0xFFFFFFFFFFFFFFFFULL;
    }
    return ret;
}

