/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created.
            2009.03.31: Crrected the return value of rcv_mbf.
            2010.02.05: Corrected the checking of error code.
            2014.02.06: In case ring buffer is not empty, return E_TMOUT.
            2017.03.02: Fixed the IPA warnings.
            2017.07.19: Added the following checking code.
                         - If the tmout is -2 or less, an E_PAR error.
            2017.09.05: Fixed the C++test StaticAnalysis warnings.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_sndmbf_2(T_MBF *mbf, UB *msg, SIZE msgsz, TMO tmout, T_TCB *tcb);
static BOOL _kernel_sndmbf_1(T_SSB *par, BOOL retcd);
static void _kernel_sndmbf_3(T_TCB *tcb, UB *msg, SIZE msgsz);
static void _kernel_chkmbf(ID mbfid);
static void _kernel_putmbf_1(T_MBF *mbf, UB *msg, SIZE msgsz);
static void _kernel_putmbf(T_MBF *mbf, UB *msg, SIZE msgsz);

/***********************************
    Send to Message Buffer
 ***********************************/

static void _kernel_putmbf_1(T_MBF *mbf, UB *msg, SIZE msgsz)
{
    UB *to_msg;
    SIZE msz;

    msz = mbf->mbfsz - mbf->putp;
    if (msz < msgsz) {
        msgsz -= msz;
        to_msg = (UB *)&mbf->mbf[mbf->putp];
        mbf->frsz -= msz;
        mbf->putp = 0UL;
        for(; msz != 0U; msz--) {
            *to_msg++ = *msg++;
        }
    }
    if (msgsz != 0U) {
        to_msg = (UB *)&mbf->mbf[mbf->putp];
        mbf->putp += msgsz;
        mbf->frsz -= msgsz;
        if (mbf->putp >= mbf->mbfsz) {
            mbf->putp = 0UL;
        }
        for(; msgsz != 0U; msgsz--) {
            *to_msg++ = *msg++;
        }
    }
}

static void _kernel_putmbf(T_MBF *mbf, UB *msg, SIZE msgsz)
{
    UB data[2];

    data[0] = msgsz & 0xFFU;
    data[1] = (msgsz >> 8) & 0xFFU;
    if (mbf->maxmsz > 1U) {
        if (mbf->maxmsz < 256U) {
            _kernel_putmbf_1(mbf, data, 1U);
        } else {
            _kernel_putmbf_1(mbf, data, 2U);
        }
    }
    _kernel_putmbf_1(mbf, msg, msgsz);
}

static void _kernel_chkmbf(ID mbfid)
{
    T_TCB *tcb;
    T_MBF *mbf;
    T_WTCB *wtcb;
    PRI primax;
    SIZE msgsz;
    SIZE msz;

    mbf = _kernel_systbl.qmbf.mbf[mbfid];
    if (mbf->maxmsz == 1U) {
        msz = 0U;
    } else if (mbf->maxmsz < 256U) {
        msz = 1U;
    } else {
        msz = 2U;
    }
    if ((mbf->mbfatr & TA_TPRI) != 0U) {
        primax = (PRI)_kernel_systbl.qrdq.inf->limit;
        wtcb = mbf->sque.mwait;
    } else {
        primax = 0;
        wtcb = &mbf->sque.wait;
    }
    for(tcb = _kernel_gettcb(wtcb, primax); tcb != 0; tcb = _kernel_gettcb(wtcb, primax)) {
        msgsz = (UINT)tcb->p3;
        if (mbf->frsz >= (msgsz + msz)) {
            mbf->cnt++;
            _kernel_putmbf(mbf, (UB *)tcb->p2, msgsz);
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
            break;
        }
    }
}

static void _kernel_sndmbf_3(T_TCB *tcb, UB *msg, SIZE msgsz)
{
    UB *to_msg;

    to_msg = (UB *)tcb->p2;
    tcb->p1 = (VP_INT)msgsz;
    for(; msgsz != 0U; msgsz--) {
        *to_msg++ = *msg++;
    }
    _kernel_deqtcb(&tcb->wtcb);
    if ((tcb->stat.msts & TTS_TMR) != 0U) {
        tcb->sprev->snext = (T_SSB *)tcb->snext;
        if (tcb->snext != 0) {
            tcb->snext->sprev = (T_SSB *)tcb->sprev;
        }
        tcb->stat.msts &= (UB)~TTS_TMR;
    }
    _kernel_enqrdq(tcb);
}

static ER _kernel_sndmbf_2(T_MBF *mbf, UB *msg, SIZE msgsz, TMO tmout, T_TCB *tcb)
{
    T_TCB *ttcb;
    SIZE msz;
    ER ercd;
    PRI tskpri;

    if (mbf->maxmsz == 1U) {
        msz = 0U;
    } else if (mbf->maxmsz < 256U) {
        msz = 1U;
    } else {
        msz = 2U;
    }

    if (tmout == TMO_POL) {
        ttcb = _kernel_gettcb(&mbf->wque, 0);
        if (ttcb != 0) {
            _kernel_sndmbf_3(ttcb, msg, msgsz);
            ercd = E_OK;
        } else if (mbf->frsz >= (msgsz + msz)) {
            mbf->cnt++;
            _kernel_putmbf(mbf, msg, msgsz);
            ercd = E_OK;
        } else {
            ercd = E_TMOUT;
        }
    } else if (_kernel_systbl.stat.dspint == 0UL) {
        ttcb = _kernel_gettcb(&mbf->wque, 0);
        if (ttcb != 0) {
            _kernel_sndmbf_3(ttcb, msg, msgsz);
        } else if (mbf->frsz >= (msgsz + msz)) {
            mbf->cnt++;
            _kernel_putmbf(mbf, msg, msgsz);
        } else {
            _kernel_systbl.mbffunc = (FP)&_kernel_chkmbf;
            _kernel_deqtcb(&tcb->wtcb);
            tcb->stat.msts = TTS_WAI;
            tcb->stat.wsts = TS_SMBF;
            tcb->wobjid = mbf->mbfid;
            if ((mbf->mbfatr & TA_TPRI) != 0U) {
                tskpri = (PRI)tcb->cpri;
                tcb->wque = mbf->sque.mwait;
            } else {
                tskpri = 0;
                tcb->stat.msts |= TTS_FIFO;
                tcb->wque = &mbf->sque.wait;
            }
            _kernel_enqtcb(&tcb->wque[tskpri], &tcb->wtcb);
            if (tmout != TMO_FEVR) {
                _kernel_enqtim(tcb, &_kernel_systbl.systim, (RELTIM)tmout);
                tcb->stat.msts |= TTS_TMR;
            }
            _kernel_systbl.ctcb = 0;
        }
        ercd = E_OK;
    } else {
        ercd = E_CTX;
    }
    return ercd;
}

static BOOL _kernel_sndmbf_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_sndmbf_2((T_MBF *)par->p1, (UB *)par->p2,
                             (SIZE)par->p3, (TMO)par->p4, (T_TCB *)par->p5);
    return retcd;
}

ER tsnd_mbf(ID mbfid, VP msg, UINT msgsz, TMO tmout)
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

    } else if (_kernel_systbl.cssb == 0) {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        mbf = _kernel_systbl.qmbf.mbf[mbfid];
        if (mbf == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else if ((msg == 0) || ((msgsz == 0U) || (msgsz > (UINT)mbf->maxmsz))) {
            _kernel_relrun();
            ercd = E_PAR;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)mbf;
            tcb->p2 = (VP_INT)msg;
            tcb->p3 = (VP_INT)msgsz;
            tcb->p4 = (VP_INT)tmout;
            tcb->p5 = (VP_INT)tcb;
            tcb->sysfunc = (FP)&_kernel_sndmbf_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    } else {
        if (tmout != TMO_POL) {
            ercd = E_CTX;
        } else {
            mbf = _kernel_systbl.qmbf.mbf[mbfid];
            if (mbf == 0) {
                ercd = E_NOEXS;
            } else if ((msg == 0) || ((msgsz == 0U) || (msgsz > (UINT)mbf->maxmsz))) {
                ercd = E_PAR;
            } else {
                ercd = _kernel_sndmbf_2(mbf, (UB *)msg, (SIZE)msgsz, tmout, 0);
            }
        }
    }
    return ercd;
}
