/**
 * @file    uC3krncm5.c
 * @brief   Micro C Cube Standard, KERNEL
 *          AArch64 and GCC dependent function
 * @date    2016.09.21
 * @author  Copyright (c) 2016, eForce Co., Ltd. All rights reserved.
 *
 ******************************************************************************
 * @par     History
 *          - rev 1.0 (2016.09.21) y-kim
 *            Initial version.
 ******************************************************************************
 */
#include "uC3sys.h"

extern T_VINFTBL __vinftbl_begin[];
extern T_VINFTBL __vinftbl_end[];

/***********************************
    Reserve System Memory
 ***********************************/

T_SYSTBL _kernel_systbl __attribute__ ((section (".sys")));

/*
 * start address of VINFTBL
 */
T_VINFTBL * _kernel_vinftbl_begin(void)
{
    return __vinftbl_begin;
}

/*
 * end address of VINFTBL
 */
T_VINFTBL * _kernel_vinftbl_end(void)
{
    return __vinftbl_end;
}

/*
 * length of VINFTBL
 */
SIZE _kernel_vinftbl_length(void)
{
    return (SIZE)__vinftbl_end - (SIZE)__vinftbl_begin;
}
