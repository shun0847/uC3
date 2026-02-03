/**
 * @file    uC3relmpl_if.c
 * @brief   Micro C Cube Standard, KERNEL
 *          API Hook routine header (ARMv8A AArch64)
 * @date    2021.01.25
 * @author  Copyright (c) 2018-2021, eForce Co., Ltd. All rights reserved.
 *
 ******************************************************************************
 * @par     History
 *          - rev 1.0 (2018.05.25) yokota
 *            Initial version.
 *          - rev 1.1 (2021.01.25) Imada
 *            Fixed C++test warnings.
 ******************************************************************************/
#include "uC3hook.h"

/************************************************
;* Release Variable-Sized Memory Block          *
 ************************************************/
ER _kernel_rel_mpl(ID mplid, VP p_blk) {
    return _kernel_systrace_p2_entry((FP)rel_mpl, ((INT)((UINT)TFN_REL_MPL << 8) + 2), (UD)mplid, (UD)p_blk);
}

