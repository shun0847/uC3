/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"
#include "string.h"


static ER _kernel_cresem_1(ID semid, T_CSEM *pk_csem);
static ER _kernel_cresem(ID semid, T_CSEM *pk_csem);

/***********************************
    Create Semaphore
 ***********************************/

static ER _kernel_cresem_1(ID semid, T_CSEM *pk_csem)
{
    T_SEM *sem;
    T_WTCB *que;
    ER ercd;

    if (semid == 0) {
        if (_kernel_systbl.qsem.inf->limit == _kernel_systbl.qsem.inf->usedc) {
            ercd = E_NOID;
        } else {
            for(semid = (ID)_kernel_systbl.qsem.inf->limit; semid >= TMIN_OBJ; semid--) {
                if (_kernel_systbl.qsem.sem[semid] == 0) {
                    break;
                }
            }
            ercd = semid;
        }
    } else {
        if (_kernel_systbl.qsem.sem[semid] != 0) {
            ercd = E_OBJ;
        } else {
            ercd = E_OK;
        }
    }

    if (ercd >= E_OK) {
        sem = (T_SEM *)_kernel_getmem(&_kernel_systbl.free_sys, sizeof(T_SEM));
        if (sem == 0) {
            ercd =  E_NOMEM;
        } else {
            memset((void *)sem, 0x00, sizeof(T_SEM));
            sem->name = pk_csem->name;
            sem->sematr = (UH)pk_csem->sematr;
            sem->semid = (UH)semid;
            sem->semcnt = (UH)pk_csem->isemcnt;
            sem->maxsem = (UH)pk_csem->maxsem;

            if ((sem->sematr & TA_TPRI) != 0U) {
                que = (T_WTCB *)_kernel_getmem(&_kernel_systbl.free_sys,
                           _kernel_systbl.qrdq.inf->limit * sizeof(T_WTCB));
                if (que == 0) {
                    (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)sem, sizeof(T_SEM));
                    ercd = E_NOMEM;
                } else {
                    _kernel_inique(que, (ID)_kernel_systbl.qrdq.inf->limit);
                    sem->que.mwait = que-1;
                }
            } else {
                sem->que.wait.next = &sem->que.wait;
                sem->que.wait.prev = &sem->que.wait;
            }

            if (ercd >= E_OK) {
                _kernel_systbl.qsem.inf->usedc++;
                _kernel_systbl.qsem.sem[semid] = sem;
            }
        }
    }
    return ercd;
}

static ER _kernel_cresem(ID semid, T_CSEM *pk_csem)
{
    T_SSB *cssb;
    ER ercd;

    if ((pk_csem->sematr & ~(TA_TPRI|TA_TFIFO)) != 0U) {
        ercd = E_RSATR;
    } else if (( pk_csem->isemcnt > pk_csem->maxsem) ||
               ((pk_csem->maxsem == 0U) ||
                (pk_csem->maxsem > TMAX_MAXSEM))) {
        ercd = E_PAR;

    } else {
        cssb = _kernel_systbl.cssb;
        if (cssb == 0) {
            _kernel_systbl.stat.s.dlock = (UB)TRUE;
            ercd = _kernel_cresem_1(semid, pk_csem);
            _kernel_relrun();
        } else if (cssb->stat.catr == TA_INI) {
            ercd = _kernel_cresem_1(semid, pk_csem);
        } else {
            ercd = E_CTX;
        }
    }
    return ercd;
}

ER cre_sem(ID semid, T_CSEM *pk_csem)
{
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qsem.inf == 0)||
               ((semid < TMIN_OBJ) || (semid > (ID)_kernel_systbl.qsem.inf->limit))) {
        ercd = E_ID;
    } else {
        ercd = _kernel_cresem(semid, pk_csem);
    }
    return ercd;
}

ER_ID acre_sem(T_CSEM *pk_csem)
{
    ER_ID ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (_kernel_systbl.qsem.inf == 0) {
        ercd = E_NOID;
    } else {
        ercd = (ER_ID)_kernel_cresem(0, pk_csem);
    }
    return ercd;
}
