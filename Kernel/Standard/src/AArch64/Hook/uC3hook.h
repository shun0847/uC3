/**
 * @file    uC3hook.h
 * @brief   Micro C Cube Standard, KERNEL
 *          Hook Routine Entry
 * @date    2018.05.22
 * @author  Copyright (c) 2018, eForce Co., Ltd. All rights reserved.
 *
 ******************************************************************************
 * @par     History
 *          - rev 1.0 (2018.05.22) yokota
 *            Initial version.
 ******************************************************************************
 */
#define  DISABLE_HOOK
#include "kernel.h"

extern ER _kernel_systrace_p0_entry(FP func, INT code);
extern ER _kernel_systrace_p1_entry(FP func, INT code, UD p1);
extern ER _kernel_systrace_p2_entry(FP func, INT code, UD p1, UD p2);
extern ER _kernel_systrace_p3_entry(FP func, INT code, UD p1, UD p2, UD p3);
extern ER _kernel_systrace_p4_entry(FP func, INT code, UD p1, UD p2, UD p3, UD p4);
extern ER _kernel_systrace_p5_entry(FP func, INT code, UD p1, UD p2, UD p3, UD p4, UD p5);
