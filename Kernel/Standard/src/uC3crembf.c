/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2009.07.08: Enabled the use of 0 size message buffer.
                        Suppressed warning of the RVDS compiler.
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"
#include "string.h"


static ER _kernel_crembf_1(ID mbfid, T_CMBF *pk_cmbf);
static ER _kernel_crembf(ID mbfid, T_CMBF *pk_cmbf);

/***********************************
    Create Message Buffer
 ***********************************/

static ER _kernel_crembf_1(ID mbfid, T_CMBF *pk_cmbf)
{
    T_MBF *mbf;
    T_WTCB *que;
    T_MEM **mem_base = 0;
    ER ercd;

    if (mbfid == 0) {
        if (_kernel_systbl.qmbf.inf->limit == _kernel_systbl.qmbf.inf->usedc) {
            ercd = E_NOID;
        } else {
            for(mbfid = (ID)_kernel_systbl.qmbf.inf->limit; mbfid >= TMIN_OBJ; mbfid--) {
                if (_kernel_systbl.qmbf.mbf[mbfid] == 0) {
                    break;
                }
            }
            ercd = mbfid;
        }
    } else {
        if (_kernel_systbl.qmbf.mbf[mbfid] != 0) {
            ercd = E_OBJ;
        } else {
            ercd = E_OK;
        }
    }

    if (ercd >= E_OK) {
        mbf = (T_MBF *)_kernel_getmem(&_kernel_systbl.free_sys, sizeof(T_MBF));
        if (mbf == 0) {
            ercd = E_NOMEM;
        } else {
            memset((void *)mbf, 0x00, sizeof(T_MBF));
            mbf->name = pk_cmbf->name;
            mbf->mbfatr = (UH)(pk_cmbf->mbfatr & (TA_TFIFO|TA_TPRI));
            mbf->mbfid = (UH)mbfid;
            mbf->getp = 0UL;
            mbf->putp = 0UL;
            mbf->cnt = 0U;
            mbf->frsz = pk_cmbf->mbfsz;
            mbf->mbfsz = pk_cmbf->mbfsz;
            mbf->maxmsz = (UH)pk_cmbf->maxmsz;
            mbf->wque.next = &mbf->wque;
            mbf->wque.prev = &mbf->wque;
            if (mbf->mbfsz != 0U) {
                if (pk_cmbf->mbf == 0) {
                    if (_kernel_systbl.free_mpl != (T_MEM *)1) {
                        mem_base = &_kernel_systbl.free_mpl;
                    } else if (_kernel_systbl.free_stk != (T_MEM *)1) {
                        mem_base = &_kernel_systbl.free_stk;
                    } else {
                        mem_base = &_kernel_systbl.free_sys;
                    }
                    mbf->mbf = (UB *)_kernel_getmem(mem_base, mbf->mbfsz);
                    if (mbf->mbf == 0) {
                        (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)mbf, sizeof(T_MBF));
                        ercd = E_NOMEM;
                    }
                } else {
                    mbf->mbfatr |= TA_UBUF;
                    mbf->mbf = (UB *)pk_cmbf->mbf;
                }
            }

            if (ercd >= E_OK) {
                if ((mbf->mbfatr & TA_TPRI) != 0U) {
                    que = (T_WTCB *)_kernel_getmem(&_kernel_systbl.free_sys,
                               _kernel_systbl.qrdq.inf->limit * sizeof(T_WTCB));
                    if (que == 0) {
                        if ((mbf->mbfsz != 0U) && ((mbf->mbfatr & TA_UBUF) == 0U)) {
                            (void)_kernel_relmem(mem_base, (T_MEM *)mbf->mbf, mbf->mbfsz);
                        }
                        (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)mbf, sizeof(T_MBF));
                        ercd = E_NOMEM;
                    } else {
                        _kernel_inique(que, (ID)_kernel_systbl.qrdq.inf->limit);
                        mbf->sque.mwait = que-1;
                    }
                } else {
                    mbf->sque.wait.next = &mbf->sque.wait;
                    mbf->sque.wait.prev = &mbf->sque.wait;
                }
            }

            if (ercd >= E_OK) {
                _kernel_systbl.qmbf.inf->usedc++;
                _kernel_systbl.qmbf.mbf[mbfid] = mbf;
            }
        }
    }
    return ercd;
}

static ER _kernel_crembf(ID mbfid, T_CMBF *pk_cmbf)
{
    T_SSB *cssb;
    ER ercd;

    if ((pk_cmbf->mbfatr & ~(TA_TPRI|TA_TFIFO)) != 0U) {
        ercd = E_RSATR;
    } else if ((pk_cmbf->maxmsz == 0U) || (pk_cmbf->maxmsz > 0xFFFFU)) {
        ercd = E_PAR;

    } else {
        cssb = _kernel_systbl.cssb;
        if (cssb == 0) {
            _kernel_systbl.stat.s.dlock = (UB)TRUE;
            ercd = _kernel_crembf_1(mbfid, pk_cmbf);
            _kernel_relrun();
        } else if (cssb->stat.catr == TA_INI) {
            ercd = _kernel_crembf_1(mbfid, pk_cmbf);
        } else {
            ercd = E_CTX;
        }
    }
    return ercd;
}

ER cre_mbf(ID mbfid, T_CMBF *pk_cmbf)
{
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qmbf.inf == 0) ||
               ((mbfid < TMIN_OBJ) || (mbfid > (ID)_kernel_systbl.qmbf.inf->limit))) {
        ercd = E_ID;
    } else {
        ercd = _kernel_crembf(mbfid, pk_cmbf);
    }
    return ercd;
}

ER_ID acre_mbf(T_CMBF *pk_cmbf)
{
    ER_ID ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (_kernel_systbl.qmbf.inf == 0)  {
        ercd = E_NOID;
    } else {
        ercd = (ER_ID)_kernel_crembf(0, pk_cmbf);
    }
    return ercd;
}
