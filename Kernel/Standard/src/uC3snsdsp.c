/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


/***************************************
    Reference Dispatching State
 ***************************************/

BOOL sns_dsp(void)
{
    return (BOOL)_kernel_systbl.stat.s.dsp;
}
