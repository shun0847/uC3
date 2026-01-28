/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common, HeteroCore Extention

    Copyright (c)  2009-2020, eForce Co.,Ltd.  All rights reserved.

    Version Information
            2020.05.08: Created.
 ***************************************************************************/

#define _UC3VSIGTIM_C_

#include "uC3sys.h"
#include "uC3xcext.h"

/**********************************
     ivsig_tim()
 **********************************/

ER ivsig_tim(ID coreid)
{
    ER ercd;
    XPARAM para[2];

    if ((coreid & DOMAIN_MASK) == 0U) {
        coreid |= (get_domain_id() << 24);
    }

    if ((ID)get_hcid() == coreid) {
        ercd = isig_tim();
    } else {
        if (_kernel_systbl.icnt == 0U) {
            ercd = E_CTX;
        } else {
            /* Asynchronization system call */
            para[0] = 0;
            para[1] = (XPARAM)coreid;
            ercd = _kernel_async_sys(0, (ID)_kernel_systbl.ctxid_s, 0U, 0ULL, XC_FUNC_IDX_SIGTIM, para);
        }
    }
    return ercd;
}

