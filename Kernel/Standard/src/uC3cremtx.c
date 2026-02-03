/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created.
            2008.10.24: Corrected the error in writing.
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"
#include "string.h"


static ER _kernel_cremtx_1(ID mtxid, T_CMTX *pk_cmtx);
static ER _kernel_cremtx(ID mtxid, T_CMTX *pk_cmtx);

/***********************************
    Create Mutex
 ***********************************/

static ER _kernel_cremtx_1(ID mtxid, T_CMTX *pk_cmtx)
{
    T_MTX *mtx;
    T_WTCB *que;
    ER ercd;

    if (mtxid == 0) {
        if (_kernel_systbl.qmtx.inf->limit == _kernel_systbl.qmtx.inf->usedc) {
            ercd = E_NOID;
        } else {
            for(mtxid = (ID)_kernel_systbl.qmtx.inf->limit; mtxid >= TMIN_OBJ; mtxid--) {
                if (_kernel_systbl.qmtx.mtx[mtxid] == 0) {
                    break;
                }
            }
            ercd = mtxid;
        }
    } else {
        if (_kernel_systbl.qmtx.mtx[mtxid] != 0) {
            ercd = E_OBJ;
        } else {
            ercd = E_OK;
        }
    }

    if (ercd >= E_OK) {
        mtx = (T_MTX *)_kernel_getmem(&_kernel_systbl.free_sys, sizeof(T_MTX));
        if (mtx == 0) {
            ercd = E_NOMEM;
        } else {
            memset((void *)mtx, 0x00, sizeof(T_MTX));
            mtx->name = pk_cmtx->name;
            mtx->mtxatr = (UH)pk_cmtx->mtxatr;
            mtx->mtxid = (UH)mtxid;
            if (mtx->mtxatr == TA_CEILING) {
                mtx->pri = (UH)pk_cmtx->ceilpri;
            } else {
                mtx->pri = (UH)TMAX_TPRI;
            }

            if ((mtx->mtxatr & (TA_TPRI|TA_INHERIT|TA_CEILING)) != 0U) {
                que = (T_WTCB *)_kernel_getmem(&_kernel_systbl.free_sys,
                           _kernel_systbl.qrdq.inf->limit * sizeof(T_WTCB));
                if (que == 0) {
                    (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)mtx, sizeof(T_MTX));
                    ercd = E_NOMEM;
                } else {
                    _kernel_inique(que, (ID)_kernel_systbl.qrdq.inf->limit);
                    mtx->que.mwait = que-1;
                }
            } else {
                mtx->que.wait.next = &mtx->que.wait;
                mtx->que.wait.prev = &mtx->que.wait;
            }

            if (ercd >= E_OK) {
                _kernel_systbl.qmtx.inf->usedc++;
                _kernel_systbl.qmtx.mtx[mtxid] = mtx;
            }
        }
    }
    return ercd;
}

static ER _kernel_cremtx(ID mtxid, T_CMTX *pk_cmtx)
{
    T_SSB *cssb;
    ER ercd;

    if ((pk_cmtx->mtxatr & ~(TA_TPRI|TA_TFIFO|TA_INHERIT|TA_CEILING)) != 0U) {
        ercd = E_RSATR;
    } else if (( pk_cmtx->mtxatr == TA_CEILING) &&
               ((pk_cmtx->ceilpri < TMIN_TPRI) ||
                (pk_cmtx->ceilpri > (PRI)_kernel_systbl.qrdq.inf->limit))) {
        ercd = E_PAR;

    } else {
        cssb = _kernel_systbl.cssb;
        if (cssb == 0) {
            _kernel_systbl.stat.s.dlock = (UB)TRUE;
            ercd = _kernel_cremtx_1(mtxid, pk_cmtx);
            _kernel_relrun();
        } else if (cssb->stat.catr == TA_INI) {
            ercd = _kernel_cremtx_1(mtxid, pk_cmtx);
        } else {
            ercd = E_CTX;
        }
    }
    return ercd;
}

ER cre_mtx(ID mtxid, T_CMTX *pk_cmtx)
{
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qmtx.inf == 0)||
               ((mtxid < TMIN_OBJ) || (mtxid > (ID)_kernel_systbl.qmtx.inf->limit))) {
        ercd = E_ID;
    } else {
        ercd = _kernel_cremtx(mtxid, pk_cmtx);
    }
    return ercd;
}

ER_ID acre_mtx(T_CMTX *pk_cmtx)
{
    ER_ID ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (_kernel_systbl.qmtx.inf == 0) {
        ercd = E_NOID;
    } else {
        ercd = (ER_ID)_kernel_cremtx(0, pk_cmtx);
    }
    return ercd;
}
