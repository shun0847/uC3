/*
 * Copyright (c) 2016, Xilinx Inc. and Contributors. All rights reserved.
 * Copyright (c) 2017-2022, eForce. Co. Ltd. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * @file	uc3/irq.c
 * @brief	uc3 libmetal irq definitions.
 */

#include <metal/errno.h>
#include <metal/irq_controller.h>
#include <metal/irq.h>
#include <metal/sys.h>
#include <metal/log.h>
#include <metal/mutex.h>
#include <metal/list.h>
#include <metal/utilities.h>
#include <metal/alloc.h>
#include "kernel.h"

#ifndef NUM_IRQS
#define NUM_IRQS 1
#endif

static struct metal_irq irqs[NUM_IRQS];
static void metal_uc3_irq_isr(void *arg);

static void metal_uc3_irq_set_enable(struct metal_irq_controller *irq_cntr,
				     int irq, unsigned int state)
{
	if (irq < irq_cntr->irq_base ||
	    irq >= irq_cntr->irq_base + irq_cntr->irq_num) {
		metal_log(METAL_LOG_ERROR, "%s: invalid irq %d\n",
			  __func__, irq);
		return;
	} else if (state == METAL_IRQ_ENABLE) {
		sys_irq_enable((unsigned int)irq);
	} else {
		sys_irq_disable((unsigned int)irq);
	}
}

static int metal_uc3_irq_register(struct metal_irq_controller *irq_cntr,
					int irq, metal_irq_handler hd,
					void *arg)
{
	int ret, vector;
	T_CISR cisr;
	struct metal_irq *data;
    
	if (irq < irq_cntr->irq_base) {
		return -EINVAL;
	}

 	vector = irq - irq_cntr->irq_base;
	data = &irq_cntr->irqs[vector];

	if (hd) { /* register irq */
		data->hd  = hd;
		data->arg = arg;

		cisr.isratr = TA_HLNG;
		cisr.exinf  = (VP_INT)irq;
		cisr.intno  = irq;
		cisr.isr    = (FP)metal_uc3_irq_isr;
		cisr.imask  = 0xA0U;

		ret = acre_isr(&cisr);

	} else { /* unregister irq */

		data->hd = NULL; 
		data->arg = NULL;

		ret = del_isr(irq);
	}

	switch (ret) {
		case E_PAR:
		case E_ID:
			return -EINVAL;
		case E_NOMEM:
			return -ENOMEM;
		case E_CTX:
			return -EPERM;
		case E_NOEXS:
			return -ENXIO;
		case E_NOID:
			return -ENOSPC;
	}

	return 0;
}

/**< uC3 common platform IRQ controller */
static METAL_IRQ_CONTROLLER_DECLARE(uc3_irq_cntr, /* _irq_controller */
				BASE_IRQ, /* .irq_base */
				NUM_IRQS, /* .irq_num */
				NULL, /* .arg */
				metal_uc3_irq_set_enable, /* .irq_set_enable */
				metal_uc3_irq_register, /* .irq_register */
				irqs /* .irqs */ );

/**
 * @brief default handler
 */
static void metal_uc3_irq_isr(void *arg)
{
	unsigned int vector, tmp;

	vector = (uintptr_t)arg;
	tmp = vector - uc3_irq_cntr.irq_base;

	(void)metal_irq_handle(&irqs[tmp], (int)vector);
}

int metal_uc3_irq_init(void)
{
	int ret;

	ret =  metal_irq_register_controller(&uc3_irq_cntr);
	if (ret < 0) {
		metal_log(METAL_LOG_ERROR, "%s: register irq controller failed.\n",
			  __func__);
	}

	return ret;
}

unsigned int metal_irq_save_disable(void)
{
	sys_irq_save_disable();

	return 0;
}

void metal_irq_restore_enable(unsigned int flags)
{
	(void)flags;

	sys_irq_restore_enable();
}
