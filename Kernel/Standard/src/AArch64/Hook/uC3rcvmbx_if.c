/**
 * @file    uC3rcvmbx_if.c
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
 * Receive from Mail Box                        *
 ************************************************/
ER _kernel_trcv_mbx(ID mbxid, T_MSG **ppk_msg, TMO tmout) {
    INT code = (tmout == TMO_FEVR)?((INT)((UINT)TFN_RCV_MBX << 8) + 2)
                                 :((tmout == TMO_POL)?((INT)((UINT)TFN_PRCV_MBX << 8) + 2)
                                                     :((INT)((UINT)TFN_TRCV_MBX << 8) + 3));
    return _kernel_systrace_p3_entry((FP)trcv_mbx, code, (UD)mbxid, (UD)ppk_msg, (UD)tmout);
}

