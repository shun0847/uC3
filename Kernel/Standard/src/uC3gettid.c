/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


/***********************************************
    Reference Task ID in the RUNNING State
 ***********************************************/

ER get_tid(ID *p_tskid)
{
    *p_tskid = (ID)_kernel_systbl.tid;
    return E_OK;
}
