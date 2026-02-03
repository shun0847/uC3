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
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_waisem(T_SEM *sem, TMO tmout, T_TCB *tcb);
static BOOL _kernel_waisem_1(T_SSB *par, BOOL retcd);

/***********************************
    Acquire Semaphore Resouce
 ***********************************/

static ER _kernel_waisem(T_SEM *sem, TMO tmout, T_TCB *tcb)
{
    PRI tskpri;
    ER ercd;

    if (tmout == TMO_POL) {
        if (sem->semcnt == 0U) {
            ercd = E_TMOUT;
        } else {
            sem->semcnt--;
            ercd = E_OK;
        }
    } else if (_kernel_systbl.stat.dspint == 0UL) {
        if (sem->semcnt == 0U) {
            _kernel_deqtcb(&tcb->wtcb);
            tcb->stat.msts = TTS_WAI;
            tcb->stat.wsts = TS_SEM;
            tcb->wobjid = sem->semid;
            if ((sem->sematr & TA_TPRI) != 0U) {
                tskpri = (PRI)tcb->cpri;
                tcb->wque = sem->que.mwait;
            } else {
                tskpri = 0;
                tcb->stat.msts |= TTS_FIFO;
                tcb->wque = &sem->que.wait;
            }
            _kernel_enqtcb(&tcb->wque[tskpri], &tcb->wtcb);
            if (tmout != TMO_FEVR) {
                _kernel_enqtim(tcb, &_kernel_systbl.systim, (RELTIM)tmout);
                tcb->stat.msts |= TTS_TMR;
            }
            _kernel_systbl.ctcb = 0;
        } else {
            sem->semcnt--;
        }
        ercd = E_OK;
    } else {
        ercd = E_CTX;
    }
    return ercd;
}

static BOOL _kernel_waisem_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_waisem((T_SEM *)par->p1, (TMO)par->p2, (T_TCB *)par->p3);
    return retcd;
}

ER twai_sem(ID semid, TMO tmout)
{
    T_TCB *tcb;
    T_SEM *sem;
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qsem.inf == 0) ||
               ((semid < TMIN_OBJ) || (semid > (ID)_kernel_systbl.qsem.inf->limit))) {
        ercd = E_ID;

    } else if ((tmout != TMO_FEVR) && (tmout < 0)) {
        ercd = E_PAR;

    } else if (_kernel_systbl.cssb == 0) {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        sem = _kernel_systbl.qsem.sem[semid];
        if (sem == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)sem;
            tcb->p2 = (VP_INT)tmout;
            tcb->p3 = (VP_INT)tcb;
            tcb->sysfunc = (FP)&_kernel_waisem_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    } else {
        if (tmout != TMO_POL) {
            ercd = E_CTX;
        } else {
            sem = _kernel_systbl.qsem.sem[semid];
            if (sem == 0) {
                ercd = E_NOEXS;
            } else {
                ercd = _kernel_waisem(sem, tmout, 0);
            }
        }
    }
    return ercd;
}
