/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2018, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
            2018.02.08: Fix typo.
 ***************************************************************************/

#include "uC3sys.h"


/***********************************
    Disable Dispatching
 ***********************************/

ER dis_dsp(void)
{
    ER ercd;

    if ((_kernel_systbl.icnt != 0U) ||
        (_kernel_systbl.cssb != 0)) {
        ercd = E_CTX;
    } else {
        _kernel_systbl.stat.s.dsp = (UB)TRUE;
        ercd = E_OK;
    }
    return ercd;
}
