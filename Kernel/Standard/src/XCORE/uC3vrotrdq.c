/***************************************************************************
    Micro C Cube Standard, KERNEL AMP HeteroCore Extention

    Copyright (c)  2020-2021, eForce Co.,Ltd.  All rights reserved.

    Version Information
            2020.05.08: Created.
            2021.10.13: Fixed data synchronization
 ***************************************************************************/

#define _UC3VROTRDQ_C_

#include "uC3sys.h"
#include "uC3xcext.h"


/***********************************
    Rotate Task Precedence
 ***********************************/

ER _kernel_vrotrdq_3(T_CMDBUF *buffer)
{
    ER ercd;

    if (_kernel_systbl.systrace != 0) {
        _kernel_systrace(buffer, 2U);
    }

    ercd = rot_rdq((ID)buffer->para[1]);

    if (_kernel_systbl.rettrace != 0) {
        _kernel_rettrace(buffer, ercd);
    }

    return ercd;
}

void _kernel_vrotrdq_2(T_CMDBUF *buffer)
{
    ER ercd;

    ercd = _kernel_vrotrdq_3(buffer);

    if (ercd != E_OK) {
        if (_kernel_systbl.errhandler != 0) {
            _kernel_xerror_handler(buffer, ercd);
        }
    }
}

void _kernel_vrotrdq_1(T_SYNCTBL *syncbuf)
{
    syncbuf->error = _kernel_vrotrdq_3(&syncbuf->cmdbuf);
    _kernel_synch_cache();
    syncbuf->fin = 1U;
    _kernel_synch_cache();
}

ER _kernel_vrotrdq_h(ID coreid, PRI tskpri, UW seqno, FP ret_pc)
{
    T_TCB *tcb;
    T_SSB *cssb;
    XPARAM para[3];
    ER ercd;

    if ((coreid & DOMAIN_MASK) == 0U) {
        coreid |= (get_domain_id() << 24);
    }

    if ((ID)get_hcid() == coreid) {
        ercd = rot_rdq(tskpri);
    } else {
        para[0] = (XPARAM)2;
        para[1] = (XPARAM)coreid;
        para[2] = (XPARAM)tskpri;
        if (_kernel_systbl.icnt != 0U) {
            /* Asynchronization system call */
            ercd = _kernel_async_sys((FN)TFN_VROT_RDQ, (ID)_kernel_systbl.ctxid_s, seqno, (XPTR)((ADDR)ret_pc), XC_FUNC_IDX_VROTRDQ_2, para);
        } else if (_kernel_systbl.cssb == 0) {
            /* Synchronization system call */
            if (_kernel_xcore_ext.sync_syscall_mode == SYNCCALL_ASYNC_MODE) {
                ercd = _kernel_async_sys((FN)TFN_VROT_RDQ, (ID)_kernel_systbl.ctxid_s, seqno, (XPTR)((ADDR)ret_pc), XC_FUNC_IDX_VROTRDQ_2, para);
            } else {
                tcb = _kernel_systbl.ctcb;
                _kernel_systbl.stat.s.dlock = (UB)TRUE;
                tcb->p1 = (VP_INT)TFN_VROT_RDQ;
                tcb->p2 = (VP_INT)_kernel_systbl.ctxid_s;
                tcb->p3 = (VP_INT)seqno;
                tcb->p4 = (VP_INT)ret_pc;
                tcb->p5 = (VP_INT)XC_FUNC_IDX_VROTRDQ_1;
                tcb->p6 = (VP_INT)para;
                tcb->sysfunc = (FP)&_kernel_sync_sys_1;
                _kernel_entsys(FALSE);
                ercd = (ER)tcb->p1;
            }
        } else {
            cssb = _kernel_systbl.cssb;
            if (cssb->stat.catr == TA_INI) {
                ercd = E_CTX;
            } else {
                /* Synchronization system call */
                if (_kernel_xcore_ext.sync_syscall_mode == SYNCCALL_ASYNC_MODE) {
                    ercd = _kernel_async_sys((FN)TFN_VROT_RDQ, (ID)_kernel_systbl.ctxid_s, seqno, (XPTR)((ADDR)ret_pc), XC_FUNC_IDX_VROTRDQ_2, para);
                } else {
                    ercd = _kernel_sync_sys((FN)TFN_VROT_RDQ, (ID)_kernel_systbl.ctxid_s, seqno, (XPTR)((ADDR)ret_pc), XC_FUNC_IDX_VROTRDQ_1, para);
                }
            }
        }
    }
    return ercd;
}

ER vrot_rdq(ID coreid, PRI tskpri)
{
    return _kernel_vrotrdq_h(coreid, tskpri, 0U, 0);
}
