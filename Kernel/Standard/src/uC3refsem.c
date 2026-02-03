/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"


static ER _kernel_refsem(T_SEM *sem, T_RSEM *pk_rsem);
static BOOL _kernel_refsem_1(T_SSB *par, BOOL retcd);

/***************************************
    Reference Semaphore State
 ***************************************/

static ER _kernel_refsem(T_SEM *sem, T_RSEM *pk_rsem)
{
    T_TCB *tcb;
    T_WTCB *wtcb;
    PRI primax;

    pk_rsem->semcnt = sem->semcnt;
    if ((sem->sematr & TA_TPRI) != 0U) {
        primax = (PRI)_kernel_systbl.qrdq.inf->limit;
        wtcb = sem->que.mwait;
    } else {
        primax = 0;
        wtcb = &sem->que.wait;
    }
    tcb = _kernel_gettcb(wtcb, primax);
    if (tcb != 0) {
        pk_rsem->wtskid = (ID)tcb->tskid;
    } else {
        pk_rsem->wtskid = TSK_NONE;
    }
    return E_OK;
}

static BOOL _kernel_refsem_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_refsem((T_SEM *)par->p1, (T_RSEM *)par->p2);
    return retcd;
}

ER ref_sem(ID semid, T_RSEM *pk_rsem)
{
    T_TCB *tcb;
    T_SEM *sem;
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qsem.inf == 0)||
               ((semid < TMIN_OBJ) || (semid > (ID)_kernel_systbl.qsem.inf->limit))) {
        ercd = E_ID;

    } else if (_kernel_systbl.cssb == 0) {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        sem = _kernel_systbl.qsem.sem[semid];
        if (sem == 0) {
            _kernel_relrun();
            ercd = E_NOEXS;
        } else {
            tcb = _kernel_systbl.ctcb;
            tcb->p1 = (VP_INT)sem;
            tcb->p2 = (VP_INT)pk_rsem;
            tcb->sysfunc = (FP)&_kernel_refsem_1;
            _kernel_entsys(FALSE);
            ercd = (ER)tcb->p1;
        }
    } else {
        sem = _kernel_systbl.qsem.sem[semid];
        if (sem == 0) {
            ercd = E_NOEXS;
        } else {
            ercd = _kernel_refsem(sem, pk_rsem);
        }
    }
    return ercd;
}
