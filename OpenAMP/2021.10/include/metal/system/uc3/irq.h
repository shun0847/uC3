/*
 * Copyright (c) 2017-2022, eForce Co.Ltd. and Contributors. All rights reserved.
 * Copyright (c) 2016, Xilinx Inc. and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	uc3/irq.c
 * @brief	uC3 libmetal irq definitions.
 */

#ifndef __METAL_IRQ__H__
#error "Include metal/irq.h instead of metal/uc3/irq.h"
#endif

#ifndef __METAL_UC3_IRQ__H__
#define __METAL_UC3_IRQ__H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief      register irq controller 
 */
int metal_uc3_irq_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __METAL_UC3_IRQ__H__ */
