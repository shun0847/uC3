/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_reftst(ID tskid, T_RTST *pk_rtst);

/***********************************************
    Reference Task State (Simplified Version)
 ***********************************************/

static const STAT ttw_table[] = {   TTW_SLP , TTW_DLY , TTW_RDV , TTW_SEM , TTW_FLG ,
                                    TTW_SDTQ, TTW_RDTQ, TTW_MBX , TTW_MTX , TTW_SMBF,
                                    TTW_RMBF, TTW_CAL , TTW_ACP , TTW_MPF , TTW_MPL };

static ER _kernel_reftst(ID tskid, T_RTST *pk_rtst)
{
    T_TCB *tcb;
    ER ercd;

    tcb = _kernel_systbl.qtcb.tcb[tskid];
    if (tcb == 0) {
        ercd = E_NOEXS;
    } else {
        if (tcb == _kernel_systbl.ctcb) {
            pk_rtst->tskstat = (STAT)TTS_RUN;
        } else {
            pk_rtst->tskstat = (STAT)tcb->stat.msts & (TTS_RDY|TTS_WAI|TTS_SUS|TTS_DMT);
        }
        if ((tcb->stat.msts & TTS_WAI) != 0U) {
            pk_rtst->tskwait = ttw_table[tcb->stat.wsts - 1U];
        }
        ercd = E_OK;
    }
    return ercd;
}

ER ref_tst(ID tskid, T_RTST *pk_rtst)
{
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if ((tskid < TSK_SELF) || (tskid > (ID)_kernel_systbl.qtcb.inf->limit)) {
        ercd = E_ID;

    } else if (_kernel_systbl.cssb == 0) {
        if (tskid == TSK_SELF) {
            tskid = (ID)_kernel_systbl.ctcb->tskid;
        }
        _kernel_lock();
        ercd = _kernel_reftst(tskid, pk_rtst);
        _kernel_unlock();
    } else {
        if (tskid < TMIN_TSK) {
            ercd = E_ID;
        } else {
            ercd = _kernel_reftst(tskid, pk_rtst);
        }
    }
    return ercd;
}
