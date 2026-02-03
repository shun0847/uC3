/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2021, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.14: Created.
            2008.12.17: Corrected the problem of calling 0.
            2009.03.31: Corrected the problem of return value.
            2009.06.29: Corrected the problem of 0 size buffer.
            2010.02.05: Corrected the checking of error code.
            2017.01.26: Fixed the IPA warnings.
            2017.07.19: Added the following checking code.
                         - If the tmout is -2 or less, an E_PAR error.
                         - If msg is NULL, an E_PAR error.
            2017.09.01: Fixed the C++test StaticAnalysis warnings.
            2021.01.27: Fixed C++test warnings.
 ***************************************************************************/

#include "uC3sys.h"


static void _kernel_getmbf(T_MBF *mbf, UB *msg, SIZE msgsz);
static ER_UINT _kernel_rcvmbf(T_MBF *mbf, VP msg, TMO tmout, T_TCB *tcb);
static BOOL _kernel_rcvmbf_1(T_SSB *par, BOOL retcd);

/***********************************
    Receive from Message Buffer
 ***********************************/

static void _kernel_getmbf(T_MBF *mbf, UB *msg, SIZE msgsz)
{
    UB *from_msg;
    SIZE msz;

    msz = mbf->mbfsz - mbf->getp;
    if (msz < msgsz) {
        msgsz -= msz;
        from_msg = &mbf->mbf[mbf->getp];
        mbf->getp += msz;
        mbf->frsz += msz;
        if (((SIZE)mbf->getp + msz) >= mbf->mbfsz) {
            mbf->getp = 0UL;
        } else {
            mbf->getp += msz;
        }
        for(; msz != 0U; msz--) {
            *msg++ = *from_msg++;
        }
    }
    if (msgsz != 0U) {
        from_msg = &mbf->mbf[mbf->getp];
        mbf->getp += msgsz;
        mbf->frsz += msgsz;
        if (mbf->getp >= mbf->mbfsz) {
            mbf->getp = 0UL;
        }
        for(; msgsz != 0U; msgsz--) {
            *msg++ = *from_msg++;
        }
    }
}

static ER_UINT _kernel_rcvmbf(T_MBF *mbf, VP msg, TMO tmout, T_TCB *tcb)
{
    void (*mbffunc)(ID);
    T_TCB *ttcb;
    T_WTCB *wtcb;
    UB *from_msg;
    UB *to_msg;
    SIZE msgsz;
    PRI primax;
    ER_UINT ercd;
    UB data;

    if ((tmout == TMO_POL) ||
        (_kernel_systbl.stat.dspint == 0UL)) {
        if ((mbf->mbfatr & TA_TPRI) != 0U) {
            primax = (PRI)_kernel_systbl.qrdq.inf->limit;
            wtcb = mbf->sque.mwait;
        } else {
            primax = 0;
            wtcb = &mbf->sque.wait;
        }
        if (mbf->frsz != mbf->mbfsz) {
            if (mbf->maxmsz == 1U) {
                _kernel_getmbf(mbf, (UB *)msg, (SIZE)1);
                msgsz = 1U;
            } else {
                _kernel_getmbf(mbf, &data, (SIZE)1);
                msgsz = data;
                if (mbf->maxmsz >= 256U) {
                    _kernel_getmbf(mbf, &data, (SIZE)1);
                    msgsz += (SIZE)data << 8;
                }
                _kernel_getmbf(mbf, (UB *)msg, msgsz);
            }
            mbf->cnt--;
            mbffunc = (void (*)(ID))_kernel_systbl.mbffunc;
            if (mbffunc != 0) {
                mbffunc((ID)mbf->mbfid);
            }
            ercd = (ER_UINT)msgsz;
        } else {
            ttcb = _kernel_gettcb(wtcb, primax);
            if (ttcb != 0) {
                to_msg = (UB *)msg;
                from_msg = (UB *)ttcb->p2;
                ercd = (ER_UINT)ttcb->p3;
                for(msgsz = (SIZE)ttcb->p3; msgsz != 0U; msgsz--) {
                    *to_msg++ = *from_msg++;
                }
                _kernel_deqtcb(&ttcb->wtcb);
                if ((ttcb->stat.msts & TTS_TMR) != 0U) {
                    ttcb->sprev->snext = (T_SSB *)ttcb->snext;
                    if (ttcb->snext != 0) {
                        ttcb->snext->sprev = (T_SSB *)ttcb->sprev;
                    }
                    ttcb->stat.msts &= (UB)~TTS_TMR;
                }
                _kernel_enqrdq(ttcb);
            } else {
                if (tmout == TMO_POL) {
                    ercd = E_TMOUT;
                } else {
                    _kernel_deqtcb(&tcb->wtcb);
                    tcb->stat.msts = TTS_WAI | TTS_FIFO;
                    tcb->stat.wsts = TS_RMBF;
                    tcb->wobjid = mbf->mbfid;
                    tcb->wque = &mbf->wque;
                    _kernel_enqtcb(&tcb->wque[0], &tcb->wtcb);
                    if (tmout != TMO_FEVR) {
                        _kernel_enqtim(tcb, &_kernel_systbl.systim, (RELTIM)tmout);
                        tcb->stat.msts |= TTS_TMR;
                    }
                    _kernel_systbl.ctcb = 0;
                    ercd = E_OK;
                }
            }
        }
    } else {
        ercd = E_CTX;
    }
    return ercd;
}

static BOOL _kernel_rcvmbf_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_rcvmbf((T_MBF *)par->p1, (VP)par->p2,
                             (TMO)par->p3, (T_TCB *)par->p4);
    return retcd;
}

ER_UINT trcv_mbf(ID mbfid, VP msg, TMO tmout)
{
    T_TCB *tcb;
    T_MBF *mbf;
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qmbf.inf == 0) ||
               ((mbfid < TMIN_OBJ) || (mbfid > (ID)_kernel_systbl.qmbf.inf->limit))) {
        ercd = E_ID;
    } else if ((tmout != TMO_FEVR) && (tmout < 0)) {
        ercd = E_PAR;
    } else if (msg == 0) {
        ercd = E_PAR;

    } else if (_kernel_systbl.cssb == 0) {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        mbf = _kernel_systbl.qmbf.mbf[mbfid];
        if (mbf == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)mbf;
            tcb->p2 = (VP_INT)msg;
            tcb->p3 = (VP_INT)tmout;
            tcb->p4 = (VP_INT)tcb;
            tcb->sysfunc = (FP)&_kernel_rcvmbf_1;
            _kernel_entsys(FALSE);
            ercd = (ER_UINT)tcb->p1;
        }
    } else {
        if (tmout != TMO_POL) {
            ercd = E_CTX;
        } else {
            mbf = _kernel_systbl.qmbf.mbf[mbfid];
            if (mbf == 0) {
                ercd = E_NOEXS;
            } else {
                ercd = _kernel_rcvmbf(mbf, msg, tmout, 0);
            }
        }
    }
    return ercd;
}
