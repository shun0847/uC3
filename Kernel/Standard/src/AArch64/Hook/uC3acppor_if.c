/**
 * @file    uC3acppor_if.c
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
 * Accept Rendezvous                            *
 ************************************************/
ER _kernel_tacp_por(ID porid, RDVPTN acpptn, RDVNO *p_rdvno, VP msg, TMO tmout) {
    INT code = (tmout == TMO_FEVR)?((INT)((UINT)TFN_ACP_POR << 8) + 4)
                                 :((tmout == TMO_POL)?((INT)((UINT)TFN_PACP_POR << 8) + 4)
                                                     :((INT)((UINT)TFN_TACP_POR << 8) + 5));
    return _kernel_systrace_p5_entry((FP)tacp_por, code, (UD)porid, (UD)acpptn, (UD)p_rdvno, (UD)msg, (UD)tmout);
}

