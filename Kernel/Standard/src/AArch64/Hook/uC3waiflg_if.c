/**
 * @file    uC3waiflg_if.c
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
 * Wait for Eventflag                           *
 ************************************************/
ER _kernel_twai_flg(ID flgid, FLGPTN waiptn, MODE wfmode, FLGPTN *p_flgptn, TMO tmout) {
    INT code = (tmout == TMO_FEVR)?((INT)((UINT)TFN_WAI_FLG << 8) + 4)
                                 :((tmout == TMO_POL)?((INT)((UINT)TFN_POL_FLG << 8) + 4)
                                                     :((INT)((UINT)TFN_TWAI_FLG << 8) + 5));
    return _kernel_systrace_p5_entry((FP)twai_flg, code, (UD)flgid, (UD)waiptn, (UD)wfmode, (UD)p_flgptn, (UD)tmout);
}

