/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
****************************************************************************/

#include "uC3sys.h"
#include "string.h"


static ER _kernel_creflg_1(ID flgid, T_CFLG *pk_cflg);
static ER _kernel_creflg(ID flgid, T_CFLG *pk_cflg);

/***********************************
    Create Eventflag
 ***********************************/

static ER _kernel_creflg_1(ID flgid, T_CFLG *pk_cflg)
{
    T_FLG *flg;
    T_WTCB *que;
    ER ercd;

    if (flgid == 0) {
        if (_kernel_systbl.qflg.inf->limit == _kernel_systbl.qflg.inf->usedc) {
            ercd = E_NOID;
        } else {
            for(flgid = (ID)_kernel_systbl.qflg.inf->limit; flgid >= TMIN_OBJ; flgid--) {
                if (_kernel_systbl.qflg.flg[flgid] == 0) {
                    break;
                }
            }
            ercd = flgid;
        }
    } else {
        if (_kernel_systbl.qflg.flg[flgid] != 0) {
            ercd = E_OBJ;
        } else {
            ercd = E_OK;
        }
    }

    if (ercd >= E_OK) {
        flg = (T_FLG *)_kernel_getmem(&_kernel_systbl.free_sys, sizeof(T_FLG));
        if (flg == 0) {
            ercd = E_NOMEM;
        } else {
            memset((void *)flg, 0x00, sizeof(T_FLG));
            flg->name = pk_cflg->name;
            flg->flgatr = (UH)pk_cflg->flgatr;
            flg->flgid = (UH)flgid;
            flg->flgptn = pk_cflg->iflgptn;

            if ((flg->flgatr & (TA_TPRI|TA_WMUL)) == (TA_TPRI|TA_WMUL)) {
                que = (T_WTCB *)_kernel_getmem(&_kernel_systbl.free_sys,
                           _kernel_systbl.qrdq.inf->limit * sizeof(T_WTCB));
                if (que == 0) {
                    (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)flg, sizeof(T_FLG));
                    ercd = E_NOMEM;
                } else {
                    _kernel_inique(que, (ID)_kernel_systbl.qrdq.inf->limit);
                    flg->que.mwait = que-1;
                }
            } else {
                flg->que.wait.next = &flg->que.wait;
                flg->que.wait.prev = &flg->que.wait;
            }

            if (ercd >= E_OK) {
                _kernel_systbl.qflg.inf->usedc++;
                _kernel_systbl.qflg.flg[flgid] = flg;
            }
        }
    }
    return ercd;
}

static ER _kernel_creflg(ID flgid, T_CFLG *pk_cflg)
{
    T_SSB *cssb;
    ER ercd;

    if ((pk_cflg->flgatr & ~(TA_TPRI|TA_TFIFO|TA_WSGL|TA_WMUL|TA_CLR)) != 0U) {
        ercd = E_RSATR;

    } else {
        cssb = _kernel_systbl.cssb;
        if (cssb == 0) {
            _kernel_systbl.stat.s.dlock = (UB)TRUE;
            ercd = _kernel_creflg_1(flgid, pk_cflg);
            _kernel_relrun();
        } else if (cssb->stat.catr == TA_INI) {
            ercd = _kernel_creflg_1(flgid, pk_cflg);
        } else {
            ercd = E_CTX;
        }
    }
    return ercd;
}

ER cre_flg(ID flgid, T_CFLG *pk_cflg)
{
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qflg.inf == 0)||
               ((flgid < TMIN_OBJ) || (flgid > (ID)_kernel_systbl.qflg.inf->limit))) {
        ercd = E_ID;
    } else {
        ercd = _kernel_creflg(flgid, pk_cflg);
    }
    return ercd;
}

ER_ID acre_flg(T_CFLG *pk_cflg)
{
    ER_ID ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (_kernel_systbl.qflg.inf == 0) {
        ercd = E_NOID;
    } else {
        ercd = (ER_ID)_kernel_creflg(0, pk_cflg);
    }
    return ercd;
}
