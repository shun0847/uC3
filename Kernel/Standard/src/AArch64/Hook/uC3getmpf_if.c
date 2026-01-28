/**
 * @file    uC3getmpf_if.c
 * @brief   Micro C Cube Standard, KERNEL
 *          API Hook routine  (ARMv8A AArch64)
 * @date    2021.01.25
 * @author  Copyright (c) 2018-2021, eForce Co., Ltd. All rights reserved.
 *
 ******************************************************************************
 * @par     History
 *          - rev 1.0 (2018.05.22) yokota
 *            Initial version.
 *          - rev 1.1 (2021.01.25) Imada
 *            Fixed C++test warnings.
 ******************************************************************************/
#include "uC3hook.h"

/************************************************
 * Acquire Fixed-Sized Memory Block             *
 ************************************************/
ER _kernel_tget_mpf(ID mpfid, VP *p_blk, TMO tmout) {
    INT code = (tmout == TMO_FEVR)?((INT)((UINT)TFN_GET_MPF << 8) + 2)
                                 :((tmout == TMO_POL)?((INT)((UINT)TFN_PGET_MPF << 8) + 2)
                                                     :((INT)((UINT)TFN_TGET_MPF << 8) + 3));
    return _kernel_systrace_p3_entry((FP)tget_mpf, code, (UD)mpfid, (UD)p_blk, (UD)tmout);
}
