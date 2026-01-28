/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created.
            2009.03.09: Corrected the E_QOVR error check.
            2009.09.12: Modified for the erro-code handler.
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_sigsem(T_SEM *sem);
static BOOL _kernel_sigsem_1(T_SSB *par, BOOL retcd);

/***************************************
    Release Semaphore Resource
 ***************************************/

static ER _kernel_sigsem(T_SEM *sem)
{
    T_TCB *tcb;
    T_WTCB *wtcb;
    PRI primax;
    ER ercd;

    if ((sem->sematr & TA_TPRI) != 0U) {
        primax = (PRI)_kernel_systbl.qrdq.inf->limit;
        wtcb = sem->que.mwait;
    } else {
        primax = 0;
        wtcb = &sem->que.wait;
    }
    tcb = _kernel_gettcb(wtcb, primax);
    if (tcb != 0) {
        _kernel_deqtcb(&tcb->wtcb);
        if ((tcb->stat.msts & TTS_TMR) != 0U) {
            tcb->sprev->snext = (T_SSB *)tcb->snext;
            if (tcb->snext != 0) {
                tcb->snext->sprev = (T_SSB *)tcb->sprev;
            }
            tcb->stat.msts &= ~TTS_TMR;
        }
        _kernel_enqrdq(tcb);
        ercd = E_OK;
    } else if (sem->semcnt < sem->maxsem) {
        sem->semcnt++;
        ercd = E_OK;
    } else {
        ercd = E_QOVR;
    }
    return ercd;
}

static BOOL _kernel_sigsem_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_sigsem((T_SEM *)par->p1);
    return retcd;
}

ER sig_sem(ID semid)
{
    T_SSB *ssb;
    T_TCB *tcb;
    T_SEM *sem;
    ER ercd;

    if (( _kernel_systbl.qsem.inf == 0) ||
        ((semid < TMIN_OBJ) || (semid > (ID)_kernel_systbl.qsem.inf->limit))) {
        ercd = E_ID;

    } else if (_kernel_systbl.icnt != 0U) {
        sem = _kernel_systbl.qsem.sem[semid];
        if (sem == 0) {
            ercd = E_NOEXS;
        } else {
            ssb = _kernel_getssb();
            if (ssb == 0) {
                ercd = E_NOMEM;
            } else {
                ssb->stat.catr = TA_SSB;
                ssb->p1 = (VP_INT)sem;
                ssb->p3 = (VP_INT)TFN_SIG_SEM;
                ssb->p4 = (VP_INT)semid;
                ssb->sysfunc = (FP)&_kernel_sigsem_1;
                _kernel_enqssb(ssb);
                ercd = E_OK;
            }
        }
    } else if (_kernel_systbl.cssb == 0) {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        sem = _kernel_systbl.qsem.sem[semid];
        if (sem == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)sem;
            tcb->sysfunc = (FP)&_kernel_sigsem_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    } else {
        sem = _kernel_systbl.qsem.sem[semid];
        if (sem == 0) {
            ercd = E_NOEXS;
        } else {
            ercd = _kernel_sigsem(sem);
        }
    }
    return ercd;
}
