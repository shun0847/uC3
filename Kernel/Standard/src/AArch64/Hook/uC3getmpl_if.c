/**
 * @file    uC3getmpl_if.c
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
 * Acquire Variable-Sized Memory Block          *
 ************************************************/
ER _kernel_tget_mpl(ID mplid, UINT blksz, VP *p_blk, TMO tmout) {
    INT code = (tmout == TMO_FEVR)?((INT)((UINT)TFN_GET_MPL << 8) + 3)
                                 : ((tmout == TMO_POL)?((INT)((UINT)TFN_PGET_MPL << 8) + 3)
                                                      :((INT)((UINT)TFN_TGET_MPL << 8) + 4));
    return _kernel_systrace_p4_entry((FP)tget_mpl, code, (UD)mplid, (UD)blksz, (UD)p_blk, (UD)tmout);
}

