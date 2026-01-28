/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2009.03.31: Corrected the operation of timer-queue.
            2017.01.26: Fixed the IPA warnings.
            2017.07.19: Deleted TMO cast when calling _kernel_enqtim.
***************************************************************************/

#include "uC3sys.h"
#include "string.h"


static BOOL _kernel_almfunc(T_ALM *alm);
static ER _kernel_crealm_1(ID almid, T_CALM *pk_calm);
static ER _kernel_crealm(ID almid, T_CALM *pk_calm);

/***********************************
    Create Alarm Handler
 ***********************************/

static BOOL _kernel_almfunc(T_ALM *alm)
{
    if ((alm->stat.msts & TALM_STA) != 0U) {
        alm->stat.msts &= ~TALM_STA;
        _kernel_timfunc(alm->stat, (UH)alm->almid, alm->almhdr, alm->exinf);
    }
    alm->stat.msts &= ~TALM_PND;
    if ((alm->stat.msts & TALM_RST) != 0U) {
        alm->stat.msts = TALM_STA;
        _kernel_enqtim((T_TCB *)alm, &_kernel_systbl.systim, alm->stime.ltime);
    }
    return FALSE;
}

static ER _kernel_crealm_1(ID almid, T_CALM *pk_calm)
{
    T_ALM *alm;
    ER ercd;

    if (almid == 0) {
        if (_kernel_systbl.qalm.inf->limit == _kernel_systbl.qalm.inf->usedc) {
            ercd = E_NOID;
        } else {
            for(almid = (ID)_kernel_systbl.qalm.inf->limit; almid >= TMIN_OBJ; almid--) {
                if (_kernel_systbl.qalm.alm[almid] == 0) {
                    break;
                }
            }
            ercd = (ER)almid;
        }
    } else {
        if (_kernel_systbl.qalm.alm[almid] != 0) {
            ercd = E_OBJ;
        } else {
            ercd = E_OK;
        }
    }

    if (ercd >= E_OK) {
        alm = (T_ALM *)_kernel_getmem(&_kernel_systbl.free_sys, sizeof(T_ALM));
        if (alm == 0) {
            ercd = E_NOMEM;
        } else {
            memset((void *)alm, 0x00, sizeof(T_ALM));
            alm->name = pk_calm->name;
            alm->stat.catr = TA_ALM;
            alm->stat.oatr = (UB)pk_calm->almatr;
            alm->almid = almid;
            alm->exinf = pk_calm->exinf;
            alm->almhdr = pk_calm->almhdr;
            alm->sysfunc = (FP)&_kernel_almfunc;
            _kernel_systbl.qalm.inf->usedc++;
            _kernel_systbl.qalm.alm[almid] = alm;
        }
    }
    return ercd;
}

static ER _kernel_crealm(ID almid, T_CALM *pk_calm)
{
    T_SSB *cssb;
    ER ercd;

    cssb = _kernel_systbl.cssb;
    if (cssb == 0) {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        ercd = _kernel_crealm_1(almid, pk_calm);
        _kernel_relrun();
    } else if (cssb->stat.catr == TA_INI) {
        ercd = _kernel_crealm_1(almid, pk_calm);
    } else {
        ercd = E_CTX;
    }
    return ercd;
}

ER cre_alm(ID almid, T_CALM *pk_calm)
{
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qalm.inf == 0) ||
               ((almid < TMIN_OBJ) || (almid > (ID)_kernel_systbl.qalm.inf->limit))) {
        ercd = E_ID;
    } else {
        ercd = _kernel_crealm(almid, pk_calm);
    }
    return ercd;
}

ER_ID acre_alm(T_CALM *pk_calm)
{
    ER_ID ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (_kernel_systbl.qalm.inf == 0) {
        ercd = E_NOID;
    } else {
        ercd = (ER_ID)_kernel_crealm(0, pk_calm);
    }
    return ercd;
}
