/***************************************************************************
    Micro C Cube Standard, KERNEL AMP HeteroCore Extention

    Copyright (c)  2020-2021, eForce Co.,Ltd.  All rights reserved.

    Version Information
            2020.05.08: Created.
            2021.10.13: Fixed data synchronization
 ***************************************************************************/

#define _UC3VSNDDTQ_C_

#include "uC3sys.h"
#include "uC3xcext.h"


/***********************************
    Send to Data Queue
 ***********************************/

ER _kernel_vsnddtq_3(T_CMDBUF *buffer)
{
    ER ercd;

    if (_kernel_systbl.systrace != 0) {
        _kernel_systrace(buffer, 3U);
    }

    if (buffer->para[3] == (UD)TRUE) {
        ercd = fsnd_dtq((ID)buffer->para[1], (VP_INT)((ADDR)buffer->para[2]));
    } else {
        ercd = psnd_dtq((ID)buffer->para[1], (VP_INT)((ADDR)buffer->para[2]));
    }

    if (_kernel_systbl.rettrace != 0) {
        _kernel_rettrace(buffer, ercd);
    }
    return ercd;
}

void _kernel_vsnddtq_2(T_CMDBUF *buffer)
{
    ER ercd;

    ercd = _kernel_vsnddtq_3(buffer);

    if (ercd != E_OK) {
        if (_kernel_systbl.errhandler != 0) {
            _kernel_xerror_handler(buffer, ercd);
        }
    }
}

void _kernel_vsnddtq_1(T_SYNCTBL *syncbuf)
{
    syncbuf->error = _kernel_vsnddtq_3(&syncbuf->cmdbuf);
    _kernel_synch_cache();
    syncbuf->fin = 1U;
    _kernel_synch_cache();
}

ER _kernel_vsnddtq_h(ID coreid, ID dtqid, VP_INT data, BOOL fmode, UW seqno, FP ret_pc)
{
    T_TCB *tcb;
    T_SSB *cssb;
    XPARAM para[5];
    ER ercd;

    if ((coreid & DOMAIN_MASK) == 0U) {
        coreid |= (get_domain_id() << 24);
    }

    if ((ID)get_hcid() == coreid) {
        if (fmode != FALSE) {
            ercd = fsnd_dtq(dtqid, data);
        } else {
            ercd = psnd_dtq(dtqid, data);
        }
    } else {
        para[0] = (XPARAM)4;
        para[1] = (XPARAM)coreid;
        para[2] = (XPARAM)dtqid;
        para[3] = (XPARAM)((ADDR)data);
        para[4] = (XPARAM)fmode;
        if (_kernel_systbl.icnt != 0U) {
            /* Asynchronization system call */
            ercd = _kernel_async_sys((fmode != FALSE)?(FN)TFN_VFSND_DTQ:(FN)TFN_VPSND_DTQ,
                   (ID)_kernel_systbl.ctxid_s, seqno, (XPTR)((ADDR)ret_pc), XC_FUNC_IDX_VSNDDTQ_2, para);
        } else if (_kernel_systbl.cssb == 0) {
            /* Synchronization system call */
            if (_kernel_xcore_ext.sync_syscall_mode == SYNCCALL_ASYNC_MODE) {
                ercd = _kernel_async_sys((fmode != FALSE)?(FN)TFN_VFSND_DTQ:(FN)TFN_VPSND_DTQ,
                       (ID)_kernel_systbl.ctxid_s, seqno, (XPTR)((ADDR)ret_pc), XC_FUNC_IDX_VSNDDTQ_2, para);
            } else {
                tcb = _kernel_systbl.ctcb;
                _kernel_systbl.stat.s.dlock = (UB)TRUE;
                tcb->p1 = (VP_INT)(fmode != FALSE)?(VP_INT)TFN_VFSND_DTQ:(VP_INT)TFN_VPSND_DTQ;
                tcb->p2 = (VP_INT)_kernel_systbl.ctxid_s;
                tcb->p3 = (VP_INT)seqno;
                tcb->p4 = (VP_INT)ret_pc;
                tcb->p5 = (VP_INT)XC_FUNC_IDX_VSNDDTQ_1;
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
                    ercd = _kernel_async_sys((fmode != FALSE)?(FN)TFN_VFSND_DTQ:(FN)TFN_VPSND_DTQ,
                           (ID)_kernel_systbl.ctxid_s, seqno, (XPTR)((ADDR)ret_pc), XC_FUNC_IDX_VSNDDTQ_2, para);
                } else {
                    ercd = _kernel_sync_sys((fmode != FALSE)?(FN)TFN_VFSND_DTQ:(FN)TFN_VPSND_DTQ,
                           (ID)_kernel_systbl.ctxid_s, seqno, (XPTR)((ADDR)ret_pc), XC_FUNC_IDX_VSNDDTQ_1, para);
                }
            }
        }
    }
    return ercd;
}

ER vpsnd_dtq(ID coreid, ID dtqid, VP_INT data)
{
    return _kernel_vsnddtq_h(coreid, dtqid, data, FALSE, 0U, 0);
}

ER vfsnd_dtq(ID coreid, ID dtqid, VP_INT data)
{
    return _kernel_vsnddtq_h(coreid, dtqid, data, TRUE, 0U, 0);
}
