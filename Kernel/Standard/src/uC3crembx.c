/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created.
            2009.07.08: Suppressed warning of the RVDS compiler.
            2017.01.26: Fixed the IPA warnings.
****************************************************************************/

#include "uC3sys.h"
#include "string.h"


static ER _kernel_crembx_1(ID mbxid, T_CMBX *pk_cmbx);
static ER _kernel_crembx(ID mbxid, T_CMBX *pk_cmbx);

/***********************************
    Create Mailbox
 ***********************************/

static ER _kernel_crembx_1(ID mbxid, T_CMBX *pk_cmbx)
{
    T_MBX *mbx;
    T_WTCB *que = 0;
    T_MHD *mhd;
    ER ercd;

    if (mbxid == 0) {
        if (_kernel_systbl.qmbx.inf->limit == _kernel_systbl.qmbx.inf->usedc) {
            ercd = E_NOID;
        } else {
            for(mbxid = (ID)_kernel_systbl.qmbx.inf->limit; mbxid >= TMIN_OBJ; mbxid--) {
                if (_kernel_systbl.qmbx.mbx[mbxid] == 0) {
                    break;
                }
            }
            ercd = mbxid;
        }
    } else {
        if (_kernel_systbl.qmbx.mbx[mbxid] != 0) {
            ercd = E_OBJ;
        } else {
            ercd = E_OK;
        }
    }

    if (ercd >= E_OK) {
        mbx = (T_MBX *)_kernel_getmem(&_kernel_systbl.free_sys, sizeof(T_MBX));
        if (mbx == 0) {
            ercd = E_NOMEM;
        } else {
            memset((void *)mbx, 0x00, sizeof(T_MBX));
            mbx->name = pk_cmbx->name;
            mbx->mbxatr = (UH)(pk_cmbx->mbxatr & (TA_TPRI|TA_MPRI));
            mbx->mbxid = (UH)mbxid;

            if ((mbx->mbxatr & TA_TPRI) != 0U) {
                que = (T_WTCB *)_kernel_getmem(&_kernel_systbl.free_sys,
                           _kernel_systbl.qrdq.inf->limit * sizeof(T_WTCB));
                if (que == 0) {
                    (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)mbx, sizeof(T_MBX));
                    ercd = E_NOMEM;
                } else {
                    _kernel_inique(que, (ID)_kernel_systbl.qrdq.inf->limit);
                    mbx->que.mwait = que-1;
                }
            } else {
                mbx->que.wait.next = &mbx->que.wait;
                mbx->que.wait.prev = &mbx->que.wait;
            }

            if (ercd >= E_OK) {
                if ((mbx->mbxatr & TA_MPRI) != 0U) {
                    mhd = (T_MHD *)pk_cmbx->mprihd;
                    if (mhd != 0) {
                        mbx->mbxatr |= (UH)TA_UBUF;
                    } else {
                        mhd = (T_MHD *)_kernel_getmem(&_kernel_systbl.free_sys,
                                   (SIZE)pk_cmbx->maxmpri * (SIZE)sizeof(T_MHD));
                        if (mhd == 0) {
                            (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)que,
                                            _kernel_systbl.qrdq.inf->limit * sizeof(T_WTCB));
                            (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)mbx, sizeof(T_MBX));
                            ercd = E_NOMEM;
                        }
                    }
                    if (ercd >= E_OK) {
                        memset((void *)mhd, 0x00, (SIZE)pk_cmbx->maxmpri * (SIZE)sizeof(T_MHD));
                        mbx->mprihd.mult.mque = mhd-1;
                        mbx->mprihd.mult.maxmpri = pk_cmbx->maxmpri;
                    }
                }
            }

            if (ercd >= E_OK) {
                _kernel_systbl.qmbx.inf->usedc++;
                _kernel_systbl.qmbx.mbx[mbxid] = mbx;
            }
        }
    }
    return ercd;
}

static ER _kernel_crembx(ID mbxid, T_CMBX *pk_cmbx)
{
    T_SSB *cssb;
    ER ercd;

    if ((pk_cmbx->mbxatr & ~(TA_TPRI|TA_TFIFO|TA_MPRI|TA_MFIFO)) != 0U) {
        ercd = E_RSATR;
    } else if (((pk_cmbx->mbxatr & TA_MPRI) != 0U) &&
               ((pk_cmbx->maxmpri < TMIN_MPRI) ||(pk_cmbx->maxmpri > TMAX_MPRI))) {
        ercd = E_PAR;

    } else {
        cssb = _kernel_systbl.cssb;
        if (cssb == 0) {
            _kernel_systbl.stat.s.dlock = (UB)TRUE;
            ercd = _kernel_crembx_1(mbxid, pk_cmbx);
            _kernel_relrun();
        } else if (cssb->stat.catr == TA_INI) {
            ercd = _kernel_crembx_1(mbxid, pk_cmbx);
        } else {
            ercd = E_CTX;
        }
    }
    return ercd;
}

ER cre_mbx(ID mbxid, T_CMBX *pk_cmbx)
{
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qmbx.inf == 0)||
               ((mbxid < TMIN_OBJ) || (mbxid > (ID)_kernel_systbl.qmbx.inf->limit))) {
        ercd = E_ID;
    } else {
        ercd = _kernel_crembx(mbxid, pk_cmbx);
    }
    return ercd;
}

ER_ID acre_mbx(T_CMBX *pk_cmbx)
{
    ER_ID ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (_kernel_systbl.qmbx.inf == 0) {
        ercd = E_NOID;
    } else {
        ercd = (ER_ID)_kernel_crembx(0, pk_cmbx);
    }
    return ercd;
}
