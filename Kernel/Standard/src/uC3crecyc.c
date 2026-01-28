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


static BOOL _kernel_cycfunc(T_CYC *cyc);
static ER _kernel_crecyc_3(T_CYC *cyc, RELTIM cycphs);
static BOOL _kernel_crecyc_2(T_SSB *par, BOOL retcd);
static ER _kernel_crecyc_1(ID cycid, T_CCYC *pk_ccyc);
static ER _kernel_crecyc(ID cycid, T_CCYC *pk_ccyc);

/***********************************
    Create Cyclic Handler
 ***********************************/

static BOOL _kernel_cycfunc(T_CYC *cyc)
{
    if ((cyc->stat.msts & TCYC_STA) != 0U) {
        _kernel_timfunc(cyc->stat, (UH)cyc->cycid, cyc->cychdr, cyc->exinf);
    }
    cyc->stat.msts &= ~TCYC_PND;
    if (((cyc->stat.oatr & TA_PHS)   != 0U) ||
        ((cyc->stat.msts & TCYC_STA) != 0U)) {
        _kernel_enqtim((T_TCB *)cyc, &cyc->stime, cyc->cyctim);
    }
    return FALSE;
}

static ER _kernel_crecyc_3(T_CYC *cyc, RELTIM cycphs)
{
    if ((cyc->stat.oatr & TA_STA) != 0U) {
        cyc->stat.msts = TCYC_STA;
    } else {
        cyc->stat.msts = TCYC_STP;
    }
    _kernel_enqtim((T_TCB *)cyc, &_kernel_systbl.systim, cycphs + (RELTIM)_kernel_systbl.tick);
    return E_OK;
}

static BOOL _kernel_crecyc_2(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_crecyc_3((T_CYC *)par->p1, (RELTIM)par->p2);
    return retcd;
}

static ER _kernel_crecyc_1(ID cycid, T_CCYC *pk_ccyc)
{
    T_TCB *tcb;
    T_CYC *cyc;
    ER ercd;
    ER tmp_ercd;

    if (cycid == 0) {
        if (_kernel_systbl.qcyc.inf->limit == _kernel_systbl.qcyc.inf->usedc) {
            ercd = E_NOID;
        } else {
            for(cycid = (ID)_kernel_systbl.qcyc.inf->limit; cycid >= TMIN_OBJ; cycid--) {
                if (_kernel_systbl.qcyc.cyc[cycid] == 0) {
                    break;
                }
            }
            ercd = (ER)cycid;
        }
    } else {
        if (_kernel_systbl.qcyc.cyc[cycid] != 0) {
            ercd = E_OBJ;
        } else {
            ercd = E_OK;
        }
    }

    if (ercd >= E_OK) {
        cyc = (T_CYC *)_kernel_getmem(&_kernel_systbl.free_sys, sizeof(T_CYC));
        if (cyc == 0) {
            ercd = E_NOMEM;
        } else {
            memset((void *)cyc, 0x00, sizeof(T_CYC));
            cyc->name = pk_ccyc->name;
            cyc->stat.catr = TA_CYC;
            cyc->stat.oatr = (UB)pk_ccyc->cycatr;
            cyc->cycid = cycid;
            cyc->exinf = pk_ccyc->exinf;
            cyc->cychdr = pk_ccyc->cychdr;
            cyc->cyctim = pk_ccyc->cyctim;
            cyc->sysfunc = (FP)&_kernel_cycfunc;
            _kernel_systbl.qcyc.inf->usedc++;
            _kernel_systbl.qcyc.cyc[cycid] = cyc;

            if ((cyc->stat.oatr & (TA_STA|TA_PHS)) != 0U) {
                if (_kernel_systbl.cssb == 0) {
                    tcb = _kernel_systbl.ctcb;
                    tcb->p1 = (VP_INT)cyc;
                    tcb->p2 = (VP_INT)pk_ccyc->cycphs;
                    tcb->sysfunc = (FP)&_kernel_crecyc_2;
                    _kernel_entsys(FALSE);
                    tmp_ercd = (ER)tcb->p1;
                } else {
                    tmp_ercd = _kernel_crecyc_3(cyc, pk_ccyc->cycphs);
                }
                if (tmp_ercd < E_OK) {
                    ercd = tmp_ercd;
                }
            }
        }
    }
    return ercd;
}

static ER _kernel_crecyc(ID cycid, T_CCYC *pk_ccyc)
{
    T_SSB *cssb;
    ER ercd;

    if (pk_ccyc->cyctim == 0U) {
        ercd = E_PAR;

    } else {
        cssb = _kernel_systbl.cssb;
        if (cssb == 0) {
            _kernel_systbl.stat.s.dlock = (UB)TRUE;
            ercd = _kernel_crecyc_1(cycid, pk_ccyc);
            _kernel_relrun();
        } else if (cssb->stat.catr == TA_INI) {
            ercd = _kernel_crecyc_1(cycid, pk_ccyc);
        } else {
            ercd = E_CTX;
        }
    }
    return ercd;
}

ER cre_cyc(ID cycid, T_CCYC *pk_ccyc)
{
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qcyc.inf == 0) ||
               ((cycid < TMIN_OBJ) || (cycid > (ID)_kernel_systbl.qcyc.inf->limit))) {
        ercd = E_ID;
    } else {
        ercd = _kernel_crecyc(cycid, pk_ccyc);
    }
    return ercd;
}

ER_ID acre_cyc(T_CCYC *pk_ccyc)
{
    ER_ID ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (_kernel_systbl.qcyc.inf == 0) {
        ercd = E_ID;
    } else {
        ercd = (ER_ID)_kernel_crecyc(0, pk_ccyc);
    }
    return ercd;
}
