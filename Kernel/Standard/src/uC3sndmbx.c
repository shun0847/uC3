/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_sndmbx(T_MBX *mbx, T_MSG *pk_msg);
static BOOL _kernel_sndmbx_1(T_SSB *par, BOOL retcd);
static ER _kernel_sndmbx_c(T_MBX *mbx, T_MSG_PRI *pk_msg);

/***********************************
    Send to Mailbox
 ***********************************/

static ER _kernel_sndmbx(T_MBX *mbx, T_MSG *pk_msg)
{
    T_TCB *tcb;
    T_WTCB *wtcb;
    T_MSG **ppk_msg;
    PRI primax;

    if ((mbx->mbxatr & TA_TPRI) != 0U) {
        primax = (PRI)_kernel_systbl.qrdq.inf->limit;
        wtcb = mbx->que.mwait;
    } else {
        primax = 0;
        wtcb = &mbx->que.wait;
    }
    tcb = _kernel_gettcb(wtcb, primax);
    if (tcb != 0) {
        ppk_msg = (T_MSG **)tcb->p2;
        *ppk_msg = pk_msg;
        _kernel_deqtcb(&tcb->wtcb);
        if ((tcb->stat.msts & TTS_TMR) != 0U) {
            tcb->sprev->snext = (T_SSB *)tcb->snext;
            if (tcb->snext != 0) {
                tcb->snext->sprev = (T_SSB *)tcb->sprev;
            }
            tcb->stat.msts &= (UB)~TTS_TMR;
        }
        _kernel_enqrdq(tcb);
    } else {
        if ((mbx->mbxatr & TA_MPRI) != 0U) {
            if (mbx->mprihd.mult.mque[((T_MSG_PRI *)pk_msg)->msgpri].top == 0) {
                mbx->mprihd.mult.mque[((T_MSG_PRI *)pk_msg)->msgpri].top = pk_msg;
            } else {
                mbx->mprihd.mult.mque[((T_MSG_PRI *)pk_msg)->msgpri].btm->msgque = pk_msg;
            }
            mbx->mprihd.mult.mque[((T_MSG_PRI *)pk_msg)->msgpri].btm = pk_msg;
        } else {
            if (mbx->mprihd.sngl.top == 0) {
                mbx->mprihd.sngl.top = pk_msg;
            } else {
                mbx->mprihd.sngl.btm->msgque = pk_msg;
            }
            mbx->mprihd.sngl.btm = pk_msg;
        }
        pk_msg->msgque = 0;
    }
    return E_OK;
}

static BOOL _kernel_sndmbx_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_sndmbx((T_MBX *)par->p1, (T_MSG *)par->p2);
    return retcd;
}

static ER _kernel_sndmbx_c(T_MBX *mbx, T_MSG_PRI *pk_msg)
{
    ER ercd = E_OK;

    if ((mbx->mbxatr & TA_MPRI) != 0U) {
        if ((pk_msg->msgpri > mbx->mprihd.mult.maxmpri ) ||
            (pk_msg->msgpri < TMIN_MPRI)) {
            ercd = E_PAR;
        }
    }
    return ercd;
}

ER snd_mbx(ID mbxid, T_MSG *pk_msg)
{
    T_TCB *tcb;
    T_MBX *mbx;
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qmbx.inf == 0) ||
               ((mbxid < TMIN_OBJ) || (mbxid > (ID)_kernel_systbl.qmbx.inf->limit))) {
        ercd = E_ID;

    } else if (_kernel_systbl.cssb == 0) {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        mbx = _kernel_systbl.qmbx.mbx[mbxid];
        if (mbx == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            ercd = _kernel_sndmbx_c(mbx, (T_MSG_PRI *)pk_msg);
            if (ercd != E_OK) {
                _kernel_relrun();
            } else {
                tcb = _kernel_systbl.ctcb;
                tcb->p1 = (VP_INT)mbx;
                tcb->p2 = (VP_INT)pk_msg;
                tcb->sysfunc = (FP)&_kernel_sndmbx_1;
                _kernel_entsys(FALSE);
                ercd = (ER)tcb->p1;
            }
        }
    } else {
        mbx = _kernel_systbl.qmbx.mbx[mbxid];
        if (mbx == 0) {
            ercd = E_NOEXS;
        } else {
            ercd = _kernel_sndmbx_c(mbx, (T_MSG_PRI *)pk_msg);
            if (ercd == E_OK) {
                ercd = _kernel_sndmbx(mbx, pk_msg);
            }
        }
    }
    return ercd;
}
