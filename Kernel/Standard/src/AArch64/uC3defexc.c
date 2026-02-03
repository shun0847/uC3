/**
 * @file    uC3defexc.c
 * @brief   Micro C Cube Standard, KERNEL
 *          AArch64 dependent function
 * @date    2016.09.21
 * @author  Copyright (c) 2016, eForce Co., Ltd. All rights reserved.
 *
 ******************************************************************************
 * @par     History
 *          - rev 1.0 (2016.09.21) y-kim
 *            Created based on uC3defexc.c of ARMv7-A.
 ******************************************************************************
 */
#include "uC3sys.h"

/***********************************
    Define exception handler
 ***********************************/

ER def_exc(EXCNO excno, T_DEXC *pk_dexc)
{
    ER ercd = E_OK;
    switch(excno) {
        case EXC_SYN:
            if (pk_dexc != 0) {
                _kernel_systbl.cpudep.synhdr = pk_dexc->exchdr;
            } else {
                _kernel_systbl.cpudep.synhdr = &int_abort;
            }
            break;
        case EXC_SER:
            if (pk_dexc != 0) {
                _kernel_systbl.cpudep.serhdr = pk_dexc->exchdr;
            } else {
                _kernel_systbl.cpudep.serhdr = &int_abort;
            }
            break;
        default:
            ercd = E_PAR;
            break;
    }
    return ercd;
}
