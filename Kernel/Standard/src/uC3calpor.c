/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2021, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created.
            2009.07.08: Suppressed warning of the RVDS compiler.
            2017.03.02: Fixed the IPA warnings.
            2017.07.19: Added the following checking code.
                         - If the tmout is -2 or less, an E_PAR error.
            2018.02.20: Added the following checking code.
                         - msg and cmsgsz check and E_PAR error.
            2021.01.27: Fixed C++test warnings.
 ***************************************************************************/

#include "uC3sys.h"


static BOOL _kernel_calpor_1(T_SSB *par, BOOL retcd);

/***********************************
    Call Rendezvous
 ***********************************/

BOOL _kernel_calpor(T_POR *por, RDVPTN calptn, UB *from_msg, UINT cmsgsz, ID tskid)
{
    UB *to_msg;
    T_TCB *atcb;
    RDVNO *p_rdvno;
    BOOL rtcd = FALSE;

    for(atcb = (T_TCB *)por->aque.next; atcb != (T_TCB *)&por->aque; atcb = (T_TCB *)atcb->wtcb.next) {
        if ((calptn & (RDVPTN)atcb->p2) != 0U) {
            _kernel_deqtcb(&atcb->wtcb);
            if ((atcb->stat.msts & TTS_TMR) != 0U) {
                atcb->sprev->snext = (T_SSB *)atcb->snext;
                if (atcb->snext != 0) {
                    atcb->snext->sprev = (T_SSB *)atcb->sprev;
                }
                atcb->stat.msts &= (UB)~TTS_TMR;
            }
            _kernel_enqrdq(atcb);
            atcb->p1 = (VP_INT)cmsgsz;
            to_msg = (UB *)atcb->p4;
            for(; cmsgsz != 0U; cmsgsz--) {
                *to_msg++ = *from_msg++;
            }
            p_rdvno = (RDVNO *)atcb->p3;
            *p_rdvno = ((RDVNO)tskid << 16) | ((RDVNO)_kernel_systbl.rdmcnt);
            rtcd = TRUE;
            break;
        }
    }
    return rtcd;
}

static BOOL _kernel_calpor_1(T_SSB *par, BOOL retcd)
{
    T_POR *por;
    T_TCB *tcb;
    PRI tskpri;
    TMO tmout;
    BOOL porflag;

    if (_kernel_systbl.stat.dspint == 0UL) {
        tcb = (T_TCB *)par->p6;
        por = (T_POR *)par->p1;
        porflag = _kernel_calpor(por, (RDVPTN)par->p2, (UB *)par->p3, (UINT)par->p4, (ID)tcb->tskid);
        _kernel_deqtcb(&tcb->wtcb);
        tmout = (TMO)par->p5;
        tcb->p5 = (VP_INT)(UW)por->maxrmsz;
        tcb->p6 = par->p3;
        tcb->stat.msts = TTS_WAI;
        tcb->wobjid = por->porid;
        if (porflag != FALSE) {
            tcb->stat.wsts = TS_RDV;
            tcb->p4 = (VP_INT)(UW)por->maxrmsz;
        } else {
            tcb->stat.wsts = TS_CAL;
            if ((por->poratr & TA_TPRI) != 0U) {
                tskpri = (PRI)tcb->cpri;
                tcb->wque = por->cque.mwait;
            } else {
                tskpri = 0;
                tcb->wque = &por->cque.wait;
                tcb->stat.msts |= TTS_FIFO;
            }
            _kernel_enqtcb(&tcb->wque[tskpri], &tcb->wtcb);
        }
        if (tmout != TMO_FEVR) {
            _kernel_enqtim(tcb, &_kernel_systbl.systim, (RELTIM)tmout);
            tcb->stat.msts |= TTS_TMR;
        }
        _kernel_systbl.ctcb = 0;
    } else {
        par->p1 = (VP_INT)E_CTX;
    }
    return retcd;
}

ER_UINT tcal_por(ID porid, RDVPTN calptn, VP msg, UINT cmsgsz, TMO tmout)
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
    } else if ((calptn == 0U) || (tmout == TMO_POL)) {
        ercd = E_PAR;
    } else if ((tmout != TMO_FEVR) && (tmout < 0)) {
        ercd = E_PAR;
    } else if ((cmsgsz > 0U) && (msg == 0)) {
        ercd = E_PAR;
    } else {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        por = _kernel_systbl.qpor.por[porid];
        if (por == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else if ((UINT)por->maxcmsz < cmsgsz) {
            _kernel_relrun();
            ercd = E_PAR;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)por;
            tcb->p2 = (VP_INT)calptn;
            tcb->p3 = (VP_INT)msg;
            tcb->p4 = (VP_INT)cmsgsz;
            tcb->p5 = (VP_INT)tmout;
            tcb->p6 = (VP_INT)tcb;
            tcb->sysfunc = (FP)&_kernel_calpor_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    }
    return ercd;
}
