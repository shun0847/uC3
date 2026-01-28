/***************************************************************************
    Micro C Cube Standard, KERNEL AMP HeteroCore Extention
    Kernel Common

    Copyright (c) 2009-2020, eForce Co., Ltd. All rights reserved.

    Version Information
            2020.05.08: Created.
 ***************************************************************************/

#define _UC3KRNCM4_C_

#include "uC3sys.h"
#include "uC3xcext.h"

extern void _kernel_error_handler(FN func_code, ER error_code, VP_INT *para);
/***********************************
    Internel functions
 ***********************************/

void _kernel_systrace(T_CMDBUF *buffer, UW para_n)
{
    UW microtim;
    UW i;
    VP_INT para[7] = { 0 }; /* parasoft-suppress BD-PB-VOVR-3 "2020/05/13 Reviewed" */

    _kernel_lock();
    microtim = _kernel_micro_systim();
    if (sizeof(VP_INT) != sizeof(XPARAM)) {
        for (i = 0U; i < para_n; i++) {
            para[i] = (VP_INT)((UW)buffer->para[i]);
        }
        ((void (*)(ID,FN,UW,FP,UW,VP_INT*,UW))_kernel_systbl.systrace)(buffer->ctexid,
                   buffer->func_n, microtim, (FP)((ADDR)buffer->ret_pc), para_n, (VP_INT*)para, buffer->seq_no);
    } else {
        ((void (*)(ID,FN,UW,FP,UW,VP_INT*,UW))_kernel_systbl.systrace)(buffer->ctexid,
                   buffer->func_n, microtim, (FP)((ADDR)buffer->ret_pc), para_n, (VP_INT*)buffer->para, buffer->seq_no);
    }
    _kernel_unlock();
}

void _kernel_rettrace(T_CMDBUF *buffer, ER ercd)
{
    UW microtim;

    _kernel_lock();
    microtim = _kernel_micro_systim();
    ((void (*)(ID,FN,UW,FP,ER,UW))_kernel_systbl.rettrace)(buffer->ctexid,
                   buffer->func_n, microtim, (FP)((ADDR)buffer->ret_pc), ercd, buffer->seq_no);
    _kernel_unlock();
}

void _kernel_xerror_handler(T_CMDBUF *buffer, ER ercd)
{
    UW i;
    VP_INT para[7] = { 0 }; /* parasoft-suppress BD-PB-VOVR-3 "2020/05/13 Reviewed" */
    if (sizeof(VP_INT) != sizeof(XPARAM)) {
        for (i = 0U; i < 7U; i++) {
            para[i] = (VP_INT)((UW)buffer->para[i]);
        }
        _kernel_error_handler(buffer->func_n, ercd, para);
    } else {
        _kernel_error_handler(buffer->func_n, ercd, (VP_INT*)buffer->para);
    }
}
