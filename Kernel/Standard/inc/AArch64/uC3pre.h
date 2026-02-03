/**
 * @file    uC3pre.h
 * @brief   Micro C Cube Standard, KERNEL
 *          AArch64 dependent internal predefinitions
 * @date    2018.04.04
 * @author  Copyright (c) 2016-2018, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2016.09.21) y-kim
 *            Created based on uC3pre.h of ARMv7-A.
 *          - rev 1.1 (2017.04.25) yokota
 *            Fixed the IPA warnings.
 *          - rev 1.2 (2018.04.04) yokota
 *            Added "C" linkage macro.
 ****************************************************************************
 */
#ifndef _UC3PRE_H_
#define _UC3PRE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef UD  T_REG;

typedef struct t_cpudep {
    FP      synhdr;
    FP      serhdr;
    T_REG   fpscr;
} T_CPUDEP;

#define CPU_DEPENDEND T_CPUDEP  cpudep;
#define _KERNEL_START_MULTI_TASK()  (_kernel_start_multi_task())

#ifdef __cplusplus
}
#endif

#endif
