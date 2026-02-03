/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created.
            2009.09.12: Modified for the erro-code handler.
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_setflg(T_FLG *flg, FLGPTN setptn);
static BOOL _kernel_setflg_1(T_SSB *par, BOOL retcd);

/***********************************
    Set Eventflag
 ***********************************/

static ER _kernel_setflg(T_FLG *flg, FLGPTN setptn)
{
    T_WTCB *wque;
    T_TCB *tcb, *tcb1;
    PRI tskpri, primax;
    FLGPTN waiptn, *p_flgptn;
    MODE wfmode;

    if (flg->flgptn != (flg->flgptn | setptn)) {
        flg->flgptn |= setptn;
        if ((flg->flgatr & (TA_TPRI|TA_WMUL)) == (TA_TPRI|TA_WMUL)) {
            primax = (PRI)_kernel_systbl.qrdq.inf->limit;
            wque = flg->que.mwait +1;
        } else {
            primax = 1;
            wque = &flg->que.wait;
        }
        for(tskpri = 0; tskpri < primax; tskpri++, wque++) {
            for(tcb = (T_TCB *)wque->next; tcb != (T_TCB *)wque; tcb = tcb1) {
                tcb1 = (T_TCB *)tcb->wtcb.next;
                waiptn = (FLGPTN)tcb->p2;
                wfmode = (MODE)tcb->p3;
                if (((wfmode == TWF_ORW) && ((flg->flgptn & waiptn) != 0U)) ||
                    ((wfmode == TWF_ANDW) && ((flg->flgptn & waiptn) == waiptn))) {
                    p_flgptn = (FLGPTN *)tcb->p4;
                    *p_flgptn = flg->flgptn;
                    _kernel_deqtcb(&tcb->wtcb);
                    if ((tcb->stat.msts & TTS_TMR) != 0U) {
                        tcb->sprev->snext = (T_SSB *)tcb->snext;
                        if (tcb->snext != 0) {
                            tcb->snext->sprev = (T_SSB *)tcb->sprev;
                        }
                        tcb->stat.msts &= (UB)~TTS_TMR;
                    }
                    _kernel_enqrdq(tcb);
                    if ((flg->flgatr & TA_CLR) != 0U) {
                        flg->flgptn = 0U;
                        break;
                    }
                }
            }
            if (flg->flgptn == 0U) {
                break;
            }
        }
    }
    return E_OK;
}

static BOOL _kernel_setflg_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_setflg((T_FLG *)par->p1, (FLGPTN)par->p2);
    return retcd;
}

ER set_flg(ID flgid, FLGPTN setptn)
{
    T_SSB *ssb;
    T_TCB *tcb;
    T_FLG *flg;
    ER ercd;

    if (( _kernel_systbl.qflg.inf == 0) ||
        ((flgid < TMIN_OBJ) || (flgid > (ID)_kernel_systbl.qflg.inf->limit))) {
        ercd = E_ID;
    } else if (setptn == 0U) {
        ercd = E_PAR;

    } else if (_kernel_systbl.icnt != 0U) {
        flg = _kernel_systbl.qflg.flg[flgid];
        if (flg == 0) {
            ercd = E_NOEXS;
        } else {
            ssb = _kernel_getssb();
            if (ssb == 0) {
                ercd = E_NOMEM;
            } else {
                ssb->stat.catr = TA_SSB;
                ssb->p1 = (VP_INT)flg;
                ssb->p2 = (VP_INT)setptn;
                ssb->p3 = (VP_INT)TFN_SET_FLG;
                ssb->p4 = (VP_INT)flgid;
                ssb->p5 = (VP_INT)setptn;
                ssb->sysfunc = (FP)&_kernel_setflg_1;
                _kernel_enqssb(ssb);
                ercd = E_OK;
            }
        }
    } else if (_kernel_systbl.cssb == 0) {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        flg = _kernel_systbl.qflg.flg[flgid];
        if (flg == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)flg;
            tcb->p2 = (VP_INT)setptn;
            tcb->sysfunc = (FP)&_kernel_setflg_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    } else {
        flg = _kernel_systbl.qflg.flg[flgid];
        if (flg == 0) {
            ercd = E_NOEXS;
        } else {
            ercd = _kernel_setflg(flg, setptn);
        }
    }
    return ercd;
}
