/*
 * Copyright (c) 2015, Xilinx Inc. and Contributors. All rights reserved.
 * Copyright (c) 2022-2025, eForce. Co. Ltd. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	uc3/init.c
 * @brief	uc3 libmetal initialization.
 */

#include <metal/sys.h>
#include <metal/utilities.h>
#include <metal/device.h>
#include <metal/irq.h>

#include "kernel.h"

#define METAL_MPL_SIZE 0x4000U

struct metal_state _metal;
ID metal_mpl;
static const T_CMPL metal_cmpl = {TA_TFIFO, METAL_MPL_SIZE, NULL, "metal_mpl"};

int metal_sys_init(const struct metal_init_params *params)
{
	metal_unused(params);
	metal_mpl = acre_mpl((T_CMPL *)&metal_cmpl);
	if (metal_mpl < E_OK) { /* Common error cases */
		return metal_mpl;
	} else if (metal_mpl == E_OK) { /* acre_mpl should not return E_OK */
		return E_SYS;
	} else { /* No error cases */
		metal_bus_register(&metal_generic_bus);
		return metal_uc3_irq_init();
	}
}

void metal_sys_finish(void)
{
	if (metal_mpl > E_OK) {
		(void)del_mpl(metal_mpl);
		metal_bus_unregister(&metal_generic_bus);
	}
}
