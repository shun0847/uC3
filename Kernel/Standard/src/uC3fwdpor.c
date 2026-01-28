/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2021, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created.
            2009.07.08: Suppressed warning of the RVDS compiler.
            2017.03.02: Fixed the IPA warnings.
            2018.02.20: Added the following checking code.
                         - msg and cmsgsz check and E_PAR error.
            2021.01.27: Fixed C++test warnings.
 ***************************************************************************/

#include "uC3sys.h"


static BOOL _kernel_fwdpor_1(T_SSB *par, BOOL retcd);

/***********************************
    Forward Rendezvous
 ***********************************/

static BOOL _kernel_fwdpor_1(T_SSB *par, BOOL retcd)
{
    T_POR *por;
    T_TCB *ctcb;
    PRI tskpri;
    BOOL porflag;
    UINT maxrmsz;
    UINT cmsgsz;

    ctcb = (T_TCB *)par->p3;
    if (((ctcb->stat.msts & TTS_WAI) == TTS_WAI) &&
        ( ctcb->stat.wsts == TS_RDV)) {
        maxrmsz = (UINT)ctcb->p5;
        por = (T_POR *)par->p1;
        cmsgsz = (UINT)par->p5;
        if (cmsgsz > maxrmsz) {
            par->p1 = (VP_INT)E_PAR;
        } else if ((UINT)por->maxrmsz > maxrmsz) {
            par->p1 = (VP_INT)E_ILUSE;
        } else {
            ctcb->p2 = par->p2;
            ctcb->p3 = par->p4;
            ctcb->p4 = (VP_INT)cmsgsz;
            porflag = _kernel_calpor(por, (RDVPTN)par->p2, (UB *)par->p4, cmsgsz, (ID)ctcb->tskid);
            if (porflag != FALSE) {
                ctcb->p4 = (VP_INT)(UW)por->maxrmsz;
            } else {
                ctcb->wobjid = por->porid;
                ctcb->stat.wsts = TS_CAL;
                if ((por->poratr & TA_TPRI) != 0U) {
                    tskpri = (PRI)ctcb->cpri;
                    ctcb->wque = por->cque.mwait;
                    ctcb->stat.msts &= TTS_FIFO;
                } else {
                    tskpri = 0;
                    ctcb->wque = &por->cque.wait;
                    ctcb->stat.msts |= TTS_FIFO;
                }
                _kernel_enqtcb(&ctcb->wque[tskpri], &ctcb->wtcb);
            }
            par->p1 = (VP_INT)E_OK;
        }
    } else {
        par->p1 = (VP_INT)E_OBJ;
    }
    return retcd;
}

ER fwd_por(ID porid, RDVPTN calptn, RDVNO rdvno, VP msg, UINT cmsgsz)
{
    T_TCB *ttcb;
    T_TCB *tcb;
    T_POR *por;
    ER ercd;
    ID tskid;

    tskid = (ID)rdvno >> 16;
    if ((_kernel_systbl.icnt != 0U) ||
        (_kernel_systbl.cssb != 0)) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qpor.inf == 0) ||
               ((porid < TMIN_OBJ) || (porid > (ID)_kernel_systbl.qpor.inf->limit))) {
        ercd = E_ID;
    } else if ((tskid > (ID)_kernel_systbl.qtcb.inf->limit) ||
               (tskid < TMIN_TSK)) {
        ercd = E_OBJ;
    } else if (calptn == 0U) {
        ercd = E_PAR;
    } else if ((cmsgsz > 0U) && (msg == 0)) {
        ercd = E_PAR;
    } else {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        por = _kernel_systbl.qpor.por[porid];
        ttcb = _kernel_systbl.qtcb.tcb[tskid];
        if (por == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else if (ttcb == 0) {
            _kernel_relrun();
            ercd = E_OBJ;
        } else if ((UINT)por->maxcmsz < cmsgsz) {
            _kernel_relrun();
            ercd = E_PAR;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)por;
            tcb->p2 = (VP_INT)calptn;
            tcb->p3 = (VP_INT)ttcb;
            tcb->p4 = (VP_INT)msg;
            tcb->p5 = (VP_INT)cmsgsz;
            tcb->sysfunc = (FP)&_kernel_fwdpor_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    }
    return ercd;
}
