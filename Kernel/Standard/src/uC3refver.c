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
    Reference Version Information
 ***********************************/

ER ref_ver(T_RVER *pk_rver)
{
    pk_rver->maker = _kernel_ver.maker;
    pk_rver->prid  = _kernel_ver.prid;
    pk_rver->spver = _kernel_ver.spver;
    pk_rver->prver = _kernel_ver.prver;
    pk_rver->prno[0] = _kernel_ver.prno[0];
    pk_rver->prno[1] = _kernel_ver.prno[1];
    pk_rver->prno[2] = _kernel_ver.prno[2];
    pk_rver->prno[3] = _kernel_ver.prno[3];
    return E_OK;
}
