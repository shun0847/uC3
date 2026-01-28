/*
 * Copyright (c) 2015, Xilinx Inc. and Contributors. All rights reserved.
 * Copyright (c) 2017-2025, eForce Co Ltd. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	uc3/sys.h
 * @brief	uC3 system primitives for libmetal.
 */

#ifndef __METAL_SYS__H__
#error "Include metal/sys.h instead of metal/uc3/sys.h"
#endif

#ifndef __METAL_UC3_SYS__H__
#define __METAL_UC3_SYS__H__

#include <metal/errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#ifdef __AM65XX__
#include "./am65xx/sys.h"
#elif __STM32MP1__
#include "./stm32mp1/sys.h"
#elif __ZYNQ7000__
#include "./zynq7/sys.h"
#elif __ZYNQMP__
#include "./zynqmp/sys.h"
#elif __IMX8MMINI__
#include "./imx8mmini/sys.h"
#elif __IMX8MQ__
#include "./imx8mq/sys.h"
#elif __RZG2_A5X__
#include "./rzg2_a5x/sys.h"
#elif __IMX8_A5X__
#include "./imx8_a5x/sys.h"
#else
#error "Please define a macro to specify a SoC type"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef METAL_MAX_DEVICE_REGIONS
#define METAL_MAX_DEVICE_REGIONS 1
#endif

/** Structure for uc3 libmetal runtime state. */
struct metal_state {

	/** Common (system independent) data. */
	struct metal_common_state common;
};

#ifdef METAL_INTERNAL

/**
 * @brief restore interrupts to state before disable_global_interrupt()
 */
void sys_irq_restore_enable(void);

/**
 * @brief disable all interrupts
 */
void sys_irq_save_disable(void);

#endif /* METAL_INTERNAL */

#ifdef __cplusplus
}
#endif

#endif /* __METAL_UC3_SYS__H__ */
