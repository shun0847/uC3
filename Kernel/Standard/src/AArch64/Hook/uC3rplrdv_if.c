/**
 * @file    uC3rplrdv_if.c
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
 * Terminate Rendezvous                         *
 ************************************************/
ER _kernel_rpl_rdv(RDVNO rdvno, VP msg, UINT msgsz) {
    return _kernel_systrace_p3_entry((FP)rpl_rdv, ((INT)((UINT)TFN_RPL_RDV << 8) + 3), (UD)rdvno, (UD)msg, (UD)msgsz);
}

