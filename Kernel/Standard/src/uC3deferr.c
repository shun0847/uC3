/***************************************************************************
    Micro C Cube Standard, KERNEL
    ARMv6 Architecture dependent function

    Copyright (c) 2009-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2009.09.13: Created.
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


/***********************************
    Define error-code handler
 ***********************************/

ER vdef_err(ATR atr, FP func)
{
    _kernel_lock();
    _kernel_systbl.errhandler = func;
    _kernel_systbl.atrhandler = atr;
    _kernel_unlock();

    return E_OK;
}
