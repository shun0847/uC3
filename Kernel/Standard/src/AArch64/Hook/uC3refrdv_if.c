/**
 * @file    uC3refrdv_if.c
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
 * Reference Rendezvous State                   *
 ************************************************/
ER _kernel_ref_rdv(RDVNO rdvno, T_RRDV *pk_rrdv) {
    return _kernel_systrace_p2_entry((FP)ref_rdv, ((INT)((UINT)TFN_REF_RDV << 8) + 2), (UD)rdvno, (UD)pk_rrdv);
}

