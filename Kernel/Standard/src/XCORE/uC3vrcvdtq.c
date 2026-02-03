/***************************************************************************
    Micro C Cube Standard, KERNEL AMP HeteroCore Extention

    Copyright (c)  2020-2021, eForce Co.,Ltd.  All rights reserved.

    Version Information
            2020.05.08: Created.
            2021.10.13: Fixed data synchronization
 ***************************************************************************/

#define _UC3VRCVDTQ_C_

#include "uC3sys.h"
#include "uC3xcext.h"


/***********************************
    Receive from Data Queue
 ***********************************/

ER _kernel_vrcvdtq_3(T_CMDBUF *buffer, XPTR *p_data)
{
    ER ercd;
    VP_INT data = (VP_INT)0;

    if (_kernel_systbl.systrace != 0) {
        _kernel_systrace(buffer, 3U);
    }

    ercd = prcv_dtq((ID)buffer->para[1], &data);

    if (_kernel_systbl.rettrace != 0) {
        _kernel_rettrace(buffer, ercd);
    }

    *p_data = (UD)((ADDR)data);
    return ercd;
}

void _kernel_vrcvdtq_1(T_SYNCTBL *syncbuf)
{
    syncbuf->error = _kernel_vrcvdtq_3(&syncbuf->cmdbuf, &syncbuf->ret_v);
    _kernel_synch_cache();
    syncbuf->fin = 1U;
    _kernel_synch_cache();
}

ER _kernel_vrcvdtq_h(ID coreid, ID dtqid, VP_INT *p_data, UW seqno, FP ret_pc)
{
    T_TCB *tcb;
    T_SSB *cssb;
    XPARAM para[4];
    ER ercd;

    if ((coreid & DOMAIN_MASK) == 0U) {
        coreid |= (get_domain_id() << 24);
    }

    if ((ID)get_hcid() == coreid) {
        ercd = prcv_dtq(dtqid, p_data);
    } else {
        para[0] = (XPARAM)3;
        para[1] = (XPARAM)coreid;
        para[2] = (XPARAM)dtqid;
        para[3] = (XPARAM)((ADDR)p_data);
        if (_kernel_systbl.icnt != 0U) {
            /* Asynchronization system call */
            ercd = E_CTX;
        } else if (_kernel_systbl.cssb == 0) {
                /* Synchronization system call */
            tcb = _kernel_systbl.ctcb;
            _kernel_systbl.stat.s.dlock = (UB)TRUE;
            tcb->p1 = (VP_INT)TFN_VPRCV_DTQ;
            tcb->p2 = (VP_INT)_kernel_systbl.ctxid_s;
            tcb->p3 = (VP_INT)seqno;
            tcb->p4 = (VP_INT)ret_pc;
            tcb->p5 = (VP_INT)XC_FUNC_IDX_VRCVDTQ_1;
            tcb->p6 = (VP_INT)para;
            tcb->sysfunc = (FP)&_kernel_sync_sys_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
            *p_data = (VP_INT)((ADDR)para[0]);
        } else {
            cssb = _kernel_systbl.cssb;
            if (cssb->stat.catr == TA_INI) {
                ercd = E_CTX;
            } else {
                /* Synchronization system call */
                ercd = _kernel_sync_sys((FN)TFN_VPRCV_DTQ, (ID)_kernel_systbl.ctxid_s, seqno, (XPTR)((ADDR)ret_pc), XC_FUNC_IDX_VRCVDTQ_1, para);
                *p_data = (VP_INT)((ADDR)para[0]);
            }
        }
    }
    return ercd;
}

ER vprcv_dtq(ID coreid, ID dtqid, VP_INT *p_data)
{
    return _kernel_vrcvdtq_h(coreid, dtqid, p_data, 0U, 0);
}
