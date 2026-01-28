/**
 * @file    uC3snddtq_if.c
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
 * Send Data Queue                              *
 ************************************************/
ER _kernel_tsnd_dtq(ID dtqid, VP_INT data, TMO tmout) {
    INT code = (tmout == TMO_FEVR)?((INT)((UINT)TFN_SND_DTQ << 8) + 2)
                                 :((tmout == TMO_POL)?((INT)((UINT)TFN_PSND_DTQ << 8) + 2)
                                                     :((INT)((UINT)TFN_TSND_DTQ << 8) + 3));
    return _kernel_systrace_p3_entry((FP)tsnd_dtq, code, (UD)dtqid, (UD)data, (UD)tmout);
}

ER _kernel_fsnd_dtq(ID dtqid, VP_INT data) {
    return _kernel_systrace_p2_entry((FP)fsnd_dtq, ((INT)((UINT)TFN_FSND_DTQ << 8) + 2), (UD)dtqid, (UD)data);
}

