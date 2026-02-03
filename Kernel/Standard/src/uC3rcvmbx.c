/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c) 2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created.
            2009.07.08: Suppressed warning of the RVDS compiler.
            2010.02.05: Corrected the checking of error code.
            2017.01.26: Fixed the IPA warnings.
            2017.07.19: Added the following checking code.
                         - If the tmout is -2 or less, an E_PAR error.
                         - If ppk_msg is NULL, an E_PAR error.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_rcvmbx(T_MBX *mbx, T_MSG **ppk_msg, TMO tmout, T_TCB *tcb);
static BOOL _kernel_rcvmbx_1(T_SSB *par, BOOL retcd);
static T_MSG *_kernel_rcvmbx_2(T_MBX *mbx);

/***********************************
    Receive from Mailbox
 ***********************************/

static T_MSG *_kernel_rcvmbx_2(T_MBX *mbx)
{
    T_MHD *mhd;
    T_MSG *msg = 0;
    PRI mpri;

    if ((mbx->mbxatr & TA_MPRI) != 0U) {
        mhd = mbx->mprihd.mult.mque+1;
        for(mpri=0; mpri<mbx->mprihd.mult.maxmpri; mhd++, mpri++) {
            msg = mhd->top;
            if (msg != 0) {
                mhd->top = msg->msgque;
                break;
            }
        }
    } else {
        mhd = &mbx->mprihd.sngl;
        msg = mhd->top;
        if (msg != 0) {
            mhd->top = msg->msgque;
        }
    }
    return msg;
}

static ER _kernel_rcvmbx(T_MBX *mbx, T_MSG **ppk_msg, TMO tmout, T_TCB *tcb)
{
    T_MSG *msg;
    PRI tskpri;
    ER ercd;

    if (tmout == TMO_POL) {
        msg = _kernel_rcvmbx_2(mbx);
        if (msg == 0) {
            ercd = E_TMOUT;
        } else {
            *ppk_msg = msg;
            ercd = E_OK;
        }
    } else if (_kernel_systbl.stat.dspint == 0UL) {
        msg = _kernel_rcvmbx_2(mbx);
        if (msg == 0) {
            _kernel_deqtcb(&tcb->wtcb);
            tcb->stat.msts = TTS_WAI;
            tcb->stat.wsts = TS_MBX;
            tcb->wobjid = mbx->mbxid;
            if ((mbx->mbxatr & TA_TPRI) != 0U) {
                tskpri = (PRI)tcb->cpri;
                tcb->wque = mbx->que.mwait;
            } else {
                tskpri = 0;
                tcb->stat.msts |= TTS_FIFO;
                tcb->wque = &mbx->que.wait;
            }
            _kernel_enqtcb(&tcb->wque[tskpri], &tcb->wtcb);
            if (tmout != TMO_FEVR) {
                _kernel_enqtim(tcb, &_kernel_systbl.systim, (RELTIM)tmout);
                tcb->stat.msts |= TTS_TMR;
            }
            _kernel_systbl.ctcb = 0;
        } else {
            *ppk_msg = msg;
        }
        ercd = E_OK;
    } else {
        ercd = E_CTX;
    }
    return ercd;
}

static BOOL _kernel_rcvmbx_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_rcvmbx((T_MBX *)par->p1, (T_MSG **)par->p2, (TMO)par->p3, (T_TCB *)par->p4);
    return retcd;
}

ER trcv_mbx(ID mbxid, T_MSG **ppk_msg, TMO tmout)
{
    T_TCB *tcb;
    T_MBX *mbx;
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qmbx.inf == 0) ||
               ((mbxid < TMIN_OBJ) || (mbxid > (ID)_kernel_systbl.qmbx.inf->limit))) {
        ercd = E_ID;
    } else if ((tmout != TMO_FEVR) && (tmout < 0)) {
        ercd = E_PAR;
    } else if (ppk_msg == 0) {
        ercd = E_PAR;

    } else if (_kernel_systbl.cssb == 0) {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        mbx = _kernel_systbl.qmbx.mbx[mbxid];
        if (mbx == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)mbx;
            tcb->p2 = (VP_INT)ppk_msg;
            tcb->p3 = (VP_INT)tmout;
            tcb->p4 = (VP_INT)tcb;
            tcb->sysfunc = (FP)&_kernel_rcvmbx_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    } else {
        if (tmout != TMO_POL) {
            ercd = E_CTX;
        } else {
            mbx = _kernel_systbl.qmbx.mbx[mbxid];
            if (mbx == 0) {
                ercd = E_NOEXS;
            } else {
                ercd = _kernel_rcvmbx(mbx, ppk_msg, tmout, 0);
            }
        }
    }
    return ercd;
}
