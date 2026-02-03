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
    Reference Contexts
 ***********************************/

BOOL sns_ctx(void)
{
    BOOL rtcd;

    if ((_kernel_systbl.icnt != 0U) ||
        (_kernel_systbl.cssb != 0)) {
        rtcd = TRUE;
    } else {
        rtcd = FALSE;
    }
    return rtcd;
}
