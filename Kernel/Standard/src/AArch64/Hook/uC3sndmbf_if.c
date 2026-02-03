/**
 * @file    uC3sndmbf_if.c
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
 * Send Message Buffer                          *
 ************************************************/
ER _kernel_tsnd_mbf(ID mbfid, VP msg, UINT msgsz, TMO tmout) {
    INT code = (tmout == TMO_FEVR)?((INT)((UINT)TFN_SND_MBF << 8) + 2)
                                 :((tmout == TMO_POL)?((INT)((UINT)TFN_PSND_MBF << 8) + 2)
                                                     :((INT)((UINT)TFN_TSND_MBF << 8) + 3));
    return _kernel_systrace_p4_entry((FP)tsnd_mbf, code, (UD)mbfid, (UD)msg, (UD)msgsz, (UD)tmout);
}

