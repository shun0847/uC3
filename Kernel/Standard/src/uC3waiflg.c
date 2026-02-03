/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created.
            2010.02.05: Corrected the checking of error code.
            2017.01.26: Fixed the IPA warnings.
            2017.07.19: Added the following checking code.
                         - If the tmout is -2 or less, an E_PAR error.
                         - If p_flgptn is NULL, an E_PAR error.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_waiflg(T_FLG *flg, FLGPTN waiptn, MODE wfmode, FLGPTN *p_flgptn, TMO tmout, T_TCB *tcb);
static BOOL _kernel_waiflg_1(T_SSB *par, BOOL retcd);

/***********************************
    Wait for Eventflag
 ***********************************/

static ER _kernel_waiflg(T_FLG *flg, FLGPTN waiptn, MODE wfmode, FLGPTN *p_flgptn, TMO tmout, T_TCB *tcb)
{
    PRI tskpri;
    ER ercd = E_OK;

    if ((flg->flgatr & (TA_WSGL|TA_WMUL)) == TA_WSGL) {
        if (&flg->que.wait != flg->que.wait.next) {
            ercd = E_ILUSE;
        }
    }

    if (ercd == E_OK) {
        if (tmout == TMO_POL) {
            if (((wfmode == TWF_ORW) && ((flg->flgptn & waiptn) != 0U)) ||
                ((wfmode == TWF_ANDW) && ((flg->flgptn & waiptn) == waiptn))) {
                *p_flgptn = flg->flgptn;
                if ((flg->flgatr & TA_CLR) != 0U) {
                    flg->flgptn = 0U;
                }
            } else {
                ercd = E_TMOUT;
            }
        } else if (_kernel_systbl.stat.dspint == 0UL) {
            if (((wfmode == TWF_ORW) && ((flg->flgptn & waiptn) != 0U)) ||
                ((wfmode == TWF_ANDW) && ((flg->flgptn & waiptn) == waiptn))) {
                *p_flgptn = flg->flgptn;
                if ((flg->flgatr & TA_CLR) != 0U) {
                    flg->flgptn = 0U;
                }
            } else {
                _kernel_deqtcb(&tcb->wtcb);
                tcb->stat.msts = TTS_WAI;
                tcb->stat.wsts = TS_FLG;
                tcb->wobjid = flg->flgid;
                if ((flg->flgatr & (TA_TPRI|TA_WMUL)) == (TA_TPRI|TA_WMUL)) {
                    tskpri = (PRI)tcb->cpri;
                    tcb->wque = flg->que.mwait;
                } else {
                    tskpri = 0;
                    tcb->wque = &flg->que.wait;
                    tcb->stat.msts |= TTS_FIFO;
                }
                _kernel_enqtcb(&tcb->wque[tskpri], &tcb->wtcb);
                if (tmout != TMO_FEVR) {
                    _kernel_enqtim(tcb, &_kernel_systbl.systim, (RELTIM)tmout);
                    tcb->stat.msts |= TTS_TMR;
                }
                _kernel_systbl.ctcb = 0;
            }
        } else {
            ercd = E_CTX;
        }
    }
    return ercd;
}

static BOOL _kernel_waiflg_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_waiflg((T_FLG *)par->p1, (FLGPTN)par->p2,
            (MODE)par->p3, (FLGPTN *)par->p4, (TMO)par->p5, (T_TCB *)par->p6);
    return retcd;
}

ER twai_flg(ID flgid, FLGPTN waiptn, MODE wfmode, FLGPTN *p_flgptn, TMO tmout)
{
    T_TCB *tcb;
    T_FLG *flg;
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qflg.inf == 0) ||
               ((flgid < TMIN_OBJ) || (flgid > (ID)_kernel_systbl.qflg.inf->limit))) {
        ercd = E_ID;
    } else if ((waiptn == 0U) || ((wfmode & ~(TWF_ANDW|TWF_ORW)) != 0U)) {
        ercd = E_PAR;
    } else if ((tmout != TMO_FEVR) && (tmout < 0)) {
        ercd = E_PAR;
    } else if (p_flgptn == 0) {
        ercd = E_PAR;

    } else if (_kernel_systbl.cssb == 0) {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        flg = _kernel_systbl.qflg.flg[flgid];
        if (flg == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)flg;
            tcb->p2 = (VP_INT)waiptn;
            tcb->p3 = (VP_INT)wfmode;
            tcb->p4 = (VP_INT)p_flgptn;
            tcb->p5 = (VP_INT)tmout;
            tcb->p6 = (VP_INT)tcb;
            tcb->sysfunc = (FP)&_kernel_waiflg_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    } else {
        if (tmout != TMO_POL) {
            ercd = E_CTX;
        } else {
            flg = _kernel_systbl.qflg.flg[flgid];
            if (flg == 0) {
                ercd = E_NOEXS;
            } else {
                ercd = _kernel_waiflg(flg, waiptn, wfmode, p_flgptn, tmout, 0);
            }
        }
    }
    return ercd;
}
