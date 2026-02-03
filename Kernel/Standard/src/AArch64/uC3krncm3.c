/**
 * @file    uC3krncm3.c
 * @brief   Micro C Cube Standard, KERNEL
 *          AArch64 dependent function
 * @date    2018.06.12
 * @author  Copyright (c) 2016-2018, eForce Co., Ltd. All rights reserved.
 *
 ******************************************************************************
 * @par     History
 *          - rev 1.0 (2016.09.21) y-kim
 *            Created based on uC3krncm3.c of ARMv7-A.
 *          - rev 1.1 (2018.06.12) yokota
 *            Fix _kernel_id_init_stack.
 ******************************************************************************
 */
#include "uC3sys.h"

extern void _kernel_init_stack(INT *stk, SIZE sz, UINT fill);

/*******************************************
    Definition for version information
 *******************************************/

T_RVER const _kernel_ver = {
        TKERNEL_MAKER,  /* メーカーコード */
        TKERNEL_PRID,   /* 識別番号（eForce level 2 operating system）*/
        TKERNEL_SPVER,  /* 仕様のバージョン番号 */
        TKERNEL_PRVER,  /* 製品のバージョン番号 */
        0x0000,         /* 製品の管理情報 */
        0x0000,         /* 製品の管理情報 */
        0x0000,         /* 製品の管理情報 */
        0x0000};        /* 製品の管理情報 */

/*******************************************
    Dummy Definition for Debug
 *******************************************/

volatile char kernel_ver_1_8_0 = 0;
void * const _kernel_debug_dummy[] = {(void *)&_kernel_systbl, (void *)&_kernel_ver};

/*******************************************
    Initialize Stack Area
 *******************************************/

void _kernel_zero_init_stack(INT *stk, SIZE sz, ID tskid)
{
    (void)tskid;
    _kernel_init_stack(stk, sz, (UINT)0);
}

void _kernel_id_init_stack(INT *stk, SIZE sz, ID tskid)
{
#if (TMAX_TSK < 256)
    _kernel_init_stack(stk, sz, ((UINT)tskid<<24) | ((UINT)tskid<<16) | ((UINT)tskid<<8) | (UINT)tskid);
#else
    _kernel_init_stack(stk, sz, ((UINT)tskid<<16) | (UINT)tskid);
#endif
}
