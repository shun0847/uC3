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
    Reference Interrupt Mask Level
 ***********************************/

ER get_ims(IMASK *p_imask)
{
    *p_imask = _kernel_getims();
    return E_OK;
}
