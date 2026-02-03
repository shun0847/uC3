/**
 * @file    uC3sndmbx_if.c
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
 * Send Mail Box                                *
 ************************************************/
ER _kernel_snd_mbx(ID mbxid, T_MSG *pk_msg) {
    return _kernel_systrace_p2_entry((FP)snd_mbx, ((INT)((UINT)TFN_SND_MBX << 8) + 1), (UD)mbxid, (UD)pk_msg);
}

