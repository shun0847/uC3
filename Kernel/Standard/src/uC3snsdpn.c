/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.03.02: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


/*******************************************
    Reference Dispatch Pending State
 *******************************************/

BOOL sns_dpn(void)
{
    BOOL rtcd;

    if (( _kernel_systbl.stat.s.dsp != 0U) ||
        ((_kernel_systbl.icnt       != 0U) ||
         (_kernel_systbl.cssb       != 0))) {
        rtcd = TRUE;
    } else if (_kernel_snsloc() != FALSE) {
        rtcd = TRUE;
    } else if (_kernel_getims() != 0U) {
        rtcd = TRUE;
    } else {
        rtcd = FALSE;
    }
    return rtcd;
}
