/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created.
            2009.07.08: Suppressed warning of the RVDS compiler.
            2017.03.02: Fixed the IPA warnings.
            2017.07.19: Added the following checking code.
                         - If the tmout is -2 or less, an E_PAR error.
 ***************************************************************************/

#include "uC3sys.h"


static BOOL _kernel_acppor(T_POR *por, RDVPTN acpptn, RDVNO *p_rdvno, UB *to_msg, UINT *cmsgsz);
static BOOL _kernel_acppor_1(T_SSB *par, BOOL retcd);

/***********************************
    Accept Rendezvous
 ***********************************/

static BOOL _kernel_acppor(T_POR *por, RDVPTN acpptn, RDVNO *p_rdvno, UB *to_msg, UINT *cmsgsz)
{
    T_WTCB *wtcb;
    T_TCB *ctcb;
    UB *from_msg;
    PRI primax;
    PRI tskpri;
    INT i;
    BOOL rtcd = FALSE;

    if ((por->poratr & TA_TPRI) != 0U) {
        tskpri = 1;
        primax = (PRI)_kernel_systbl.qrdq.inf->limit;
        wtcb = por->cque.mwait;
    } else {
        tskpri = 0;
        primax = 0;
        wtcb = &por->cque.wait;
    }

    for(; tskpri <= primax; tskpri++) {
        for(ctcb = (T_TCB *)wtcb[tskpri].next; ctcb != (T_TCB *)&wtcb[tskpri]; ctcb = (T_TCB *)ctcb->wtcb.next) {
            if ((acpptn & (RDVPTN)ctcb->p2) != 0U) {
                ctcb->stat.wsts = TS_RDV;
                _kernel_deqtcb(&ctcb->wtcb);
                *cmsgsz = (UINT)ctcb->p4;
                ctcb->p4 = (VP_INT)(UW)por->maxrmsz;
                from_msg = (UB *)ctcb->p3;
                for(i = *cmsgsz; i != 0; i--) {
                    *to_msg++ = *from_msg++;
                }
                *p_rdvno = ((RDVNO)ctcb->tskid << 16) | ((RDVNO)_kernel_systbl.rdmcnt);
                rtcd = TRUE;
                break;
            }
        }
        if (rtcd != FALSE) {
            break;
        }
    }
    return rtcd;
}

static BOOL _kernel_acppor_1(T_SSB *par, BOOL retcd)
{
    T_POR *por;
    T_TCB *tcb;
    UINT cmsgsz;
    TMO tmout;
    BOOL porflag;

    if (_kernel_systbl.stat.dspint == 0UL) {
        tcb = (T_TCB *)par->p6;
        por = (T_POR *)par->p1;
        porflag = _kernel_acppor(por, (RDVPTN)par->p2, (RDVNO *)par->p3, (UB *)par->p4, &cmsgsz);
        if (porflag == FALSE) {
            tmout = (TMO)par->p5;
            if (tmout != TMO_POL) {
                _kernel_deqtcb(&tcb->wtcb);
                tcb->wobjid = por->porid;
                tcb->wque = &por->aque;
                tcb->stat.wsts = TS_ACP;
                tcb->stat.msts = (TTS_WAI | TTS_FIFO);
                _kernel_enqtcb(&tcb->wque[0], &tcb->wtcb);
                if (tmout != TMO_FEVR) {
                    _kernel_enqtim(tcb, &_kernel_systbl.systim, (RELTIM)tmout);
                    tcb->stat.msts |= TTS_TMR;
                }
                _kernel_systbl.ctcb = 0;
            } else {
                par->p1 = (VP_INT)E_TMOUT;
            }
        } else {
            par->p1 = (VP_INT)cmsgsz;
        }
    } else {
        par->p1 = (VP_INT)E_CTX;
    }
    return retcd;
}

ER_UINT tacp_por(ID porid, RDVPTN acpptn, RDVNO *p_rdvno, VP msg, TMO tmout)
{
    T_TCB *tcb;
    T_POR *por;
    ER ercd;

    if ((_kernel_systbl.icnt != 0U) ||
        (_kernel_systbl.cssb != 0)) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qpor.inf == 0) ||
               ((porid < TMIN_OBJ) || (porid > (ID)_kernel_systbl.qpor.inf->limit))) {
        ercd = E_ID;
    } else if (acpptn == 0U) {
        ercd = E_PAR;
    } else if ((tmout != TMO_FEVR) && (tmout < 0)) {
        ercd = E_PAR;

    } else {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        por = _kernel_systbl.qpor.por[porid];
        if (por == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)por;
            tcb->p2 = (VP_INT)acpptn;
            tcb->p3 = (VP_INT)p_rdvno;
            tcb->p4 = (VP_INT)msg;
            tcb->p5 = (VP_INT)tmout;
            tcb->p6 = (VP_INT)tcb;
            tcb->sysfunc = (FP)&_kernel_acppor_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    }
    return ercd;
}
