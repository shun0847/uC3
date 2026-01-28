/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


/***********************************
    Enable Dispatching
 ***********************************/

ER ena_dsp(void)
{
    ER ercd = E_OK;

    if ((_kernel_systbl.icnt != 0U) ||
        (_kernel_systbl.cssb != 0)) {
        ercd = E_CTX;
    } else if (_kernel_systbl.stat.s.dsp == (UB)TRUE) {
        _kernel_systbl.stat.s.dsp = (UB)FALSE;
        _kernel_relrun();
    } else {
        /* Do Nothing */
    }
    return ercd;
}
