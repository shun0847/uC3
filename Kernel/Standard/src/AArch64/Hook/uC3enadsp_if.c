/**
 * @file    uC3enadsp_if.c
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
 * Enable Dsipatching                           *
 ************************************************/
ER _kernel_ena_dsp(void) {
    return _kernel_systrace_p0_entry((FP)ena_dsp, ((INT)((UINT)TFN_ENA_DSP << 8) + 0));
}
