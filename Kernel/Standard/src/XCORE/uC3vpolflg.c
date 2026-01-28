/***************************************************************************
    Micro C Cube Standard, KERNEL AMP MultiCore Extention

    Copyright (c)  2009-2021, eForce Co.,Ltd.  All rights reserved.

    Version Information
            2009.09.28: Created.
            2017.04.25: Fixed the IPA warnings.
            2018.07.12: Pass function index as argument of _kernel_sync_sys.
            2021.10.13: Fixed data synchronization
 ***************************************************************************/

#define _UC3VPOLDTQ_C_

#include "uC3sys.h"
#include "uC3xcext.h"


/***********************************
    Wait for Eventflag
 ***********************************/

ER _kernel_vpolflg_3(T_CMDBUF *buffer, XPTR *p_flgptn)
{
    ER ercd;
    FLGPTN flgptn = 0;

    if (_kernel_systbl.systrace != 0) {
        _kernel_systrace(buffer, 3U);
    }

    ercd = pol_flg((ID)buffer->para[1], (FLGPTN)buffer->para[2], (MODE)buffer->para[3], &flgptn);

    if (_kernel_systbl.rettrace != 0) {
        _kernel_rettrace(buffer, ercd);
    }

    *p_flgptn = (UD)flgptn;
    return ercd;
}

void _kernel_vpolflg_1(T_SYNCTBL *syncbuf)
{
    syncbuf->error = _kernel_vpolflg_3(&syncbuf->cmdbuf, &syncbuf->ret_v);
    _kernel_synch_cache();
    syncbuf->fin = 1U;
    _kernel_synch_cache();
}

ER _kernel_vpolflg_h(ID coreid, ID flgid, FLGPTN waiptn, MODE wfmode, FLGPTN *p_flgptn, UW seqno, FP ret_pc)
{
    T_TCB *tcb;
    T_SSB *cssb;
    XPARAM para[6];
    ER ercd;

    if ((coreid & DOMAIN_MASK) == 0U) {
        coreid |= (get_domain_id() << 24);
    }

    if ((ID)get_hcid() == coreid) {
        ercd = pol_flg(flgid, waiptn, wfmode, p_flgptn);
    } else {
        para[0] = (XPARAM)5;
        para[1] = (XPARAM)coreid;
        para[2] = (XPARAM)flgid;
        para[3] = (XPARAM)waiptn;
        para[4] = (XPARAM)wfmode;
        para[5] = (XPARAM)((ADDR)p_flgptn);
        if (_kernel_systbl.icnt != 0U) {
            /* Asynchronization system call */
            ercd = E_CTX;
        } else if (_kernel_systbl.cssb == 0) {
                /* Synchronization system call */
            tcb = _kernel_systbl.ctcb;
            _kernel_systbl.stat.s.dlock = (UB)TRUE;
            tcb->p1 = (VP_INT)TFN_VPOL_FLG;
            tcb->p2 = (VP_INT)_kernel_systbl.ctxid_s;
            tcb->p3 = (VP_INT)seqno;
            tcb->p4 = (VP_INT)ret_pc;
            tcb->p5 = (VP_INT)XC_FUNC_IDX_VPOLFLG_1;
            tcb->p6 = (VP_INT)para;
            tcb->sysfunc = (FP)&_kernel_sync_sys_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
            *p_flgptn = (FLGPTN)para[0];
        } else {
            cssb = _kernel_systbl.cssb;
            if (cssb->stat.catr == TA_INI) {
                ercd = E_CTX;
            } else {
                /* Synchronization system call */
                ercd = _kernel_sync_sys((FN)TFN_VPOL_FLG, (ID)_kernel_systbl.ctxid_s, seqno, (XPTR)((ADDR)ret_pc), XC_FUNC_IDX_VPOLFLG_1, para);
                *p_flgptn = (FLGPTN)para[0];
            }
        }
    }
    return ercd;
}

ER vpol_flg(ID coreid, ID flgid, FLGPTN waiptn, MODE wfmode, FLGPTN *p_flgptn)
{
    return _kernel_vpolflg_h(coreid, flgid, waiptn, wfmode, p_flgptn, 0U, 0);
}
