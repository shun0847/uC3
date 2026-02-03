/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2021, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.03.02: Fixed the IPA warnings.
            2021.01.27: Fixed C++test warnings.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_reftsk(T_TCB *tcb, T_RTSK *pk_rtsk);
static BOOL _kernel_reftsk_1(T_SSB *par, BOOL retcd);

/***********************************
    Reference Task State
 ***********************************/

static const STAT ttw_table[] = {   TTW_SLP , TTW_DLY , TTW_RDV , TTW_SEM , TTW_FLG ,
                                    TTW_SDTQ, TTW_RDTQ, TTW_MBX , TTW_MTX , TTW_SMBF,
                                    TTW_RMBF, TTW_CAL , TTW_ACP , TTW_MPF , TTW_MPL };

static ER _kernel_reftsk(T_TCB *tcb, T_RTSK *pk_rtsk)
{
    TMO lefttmo;

    if (tcb == _kernel_systbl.ctcb) {
        pk_rtsk->tskstat = (STAT)TTS_RUN;
    } else {
        pk_rtsk->tskstat = (STAT)tcb->stat.msts & (TTS_RDY|TTS_WAI|TTS_SUS|TTS_DMT);
    }
    if ((tcb->stat.msts & TTS_WAI) != 0U) {
        pk_rtsk->tskwait = ttw_table[tcb->stat.wsts - 1U];
        pk_rtsk->wobjid = (ID)tcb->wobjid;
        if ((tcb->stat.msts & TTS_TMR) != 0U) {
            lefttmo = (TMO)(tcb->stime.ltime - _kernel_systbl.systim.ltime - 1U);
            pk_rtsk->lefttmo = lefttmo - (lefttmo % (TMO)_kernel_systbl.tick);
        } else {
            pk_rtsk->lefttmo = TMO_FEVR;
        }
    }
    pk_rtsk->tskpri = (PRI)tcb->cpri;
    pk_rtsk->tskbpri = (PRI)tcb->bpri;
    pk_rtsk->actcnt = (UINT)tcb->act;
    pk_rtsk->wupcnt = (UINT)tcb->wup;
    pk_rtsk->suscnt = (UINT)tcb->sus;
    return E_OK;
}

static BOOL _kernel_reftsk_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_reftsk((T_TCB *)par->p1, (T_RTSK *)par->p2);
    return retcd;
}

ER ref_tsk(ID tskid, T_RTSK *pk_rtsk)
{
    T_TCB *tcb;
    T_TCB *ttcb;
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if ((tskid < TSK_SELF) || (tskid > (ID)_kernel_systbl.qtcb.inf->limit)) {
        ercd = E_ID;

    } else if (_kernel_systbl.cssb == 0) {
        tcb = _kernel_systbl.ctcb;
        if (tskid == TSK_SELF) {
            tskid = (ID)tcb->tskid;
        }
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        ttcb = _kernel_systbl.qtcb.tcb[tskid];
        if (ttcb == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            tcb->p1 = (VP_INT)ttcb;
            tcb->p2 = (VP_INT)pk_rtsk;
            tcb->sysfunc = (FP)&_kernel_reftsk_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    } else {
        if (tskid < TMIN_TSK) {
            ercd = E_ID;
        } else {
            ttcb = _kernel_systbl.qtcb.tcb[tskid];
            if (ttcb == 0) {
                ercd = E_NOEXS;
            } else {
                ercd = _kernel_reftsk(ttcb, pk_rtsk);
            }
        }
    }
    return ercd;
}
