/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2018, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created.
            2009.07.08: Suppressed warning of the RVDS compiler.
            2017.01.26: Fixed the IPA warnings.
            2018.03.27: Add check dtq attribute.
 ***************************************************************************/

#include "uC3sys.h"
#include "string.h"


static ER _kernel_credtq_1(ID dtqid, T_CDTQ *pk_cdtq);
static ER _kernel_credtq(ID dtqid, T_CDTQ *pk_cdtq);

/***********************************
    Create Data Queue
 ***********************************/

static ER _kernel_credtq_1(ID dtqid, T_CDTQ *pk_cdtq)
{
    T_DTQ *dtq;
    T_WTCB *que;
    T_MEM **mem_base = 0;
    ER ercd;

    if (dtqid == 0) {
        if (_kernel_systbl.qdtq.inf->limit == _kernel_systbl.qdtq.inf->usedc) {
            ercd = E_NOID;
        } else {
            for(dtqid = (ID)_kernel_systbl.qdtq.inf->limit; dtqid >= TMIN_OBJ; dtqid--) {
                if (_kernel_systbl.qdtq.dtq[dtqid] == 0) {
                    break;
                }
            }
            ercd = dtqid;
        }
    } else {
        if (_kernel_systbl.qdtq.dtq[dtqid] != 0) {
            ercd = E_OBJ;
        } else {
            ercd = E_OK;
        }
    }

    if (ercd >= E_OK) {
        dtq = (T_DTQ *)_kernel_getmem(&_kernel_systbl.free_sys, sizeof(T_DTQ));
        if (dtq == 0) {
            ercd = E_NOMEM;
        } else {
            memset((void *)dtq, 0x00, sizeof(T_DTQ));
            dtq->name = pk_cdtq->name;
            dtq->dtqatr = (UH)(pk_cdtq->dtqatr & (TA_TFIFO|TA_TPRI));
            dtq->dtqid = (UH)dtqid;
            dtq->cnt = 0U;
            dtq->put = 0U;
            dtq->que.next = &dtq->que;
            dtq->que.prev = &dtq->que;
            dtq->dtqcnt = (UH)pk_cdtq->dtqcnt;
            if (dtq->dtqcnt != 0U) {
                if (pk_cdtq->dtq == 0) {
                    if (_kernel_systbl.free_mpl != (T_MEM *)1) {
                        mem_base = &_kernel_systbl.free_mpl;
                    } else if (_kernel_systbl.free_stk != (T_MEM *)1) {
                        mem_base = &_kernel_systbl.free_stk;
                    } else {
                        mem_base = &_kernel_systbl.free_sys;
                    }
                    dtq->dtq = (VP)_kernel_getmem(mem_base, dtq->dtqcnt*sizeof(VP_INT));
                    if (dtq->dtq == 0) {
                        (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)dtq, sizeof(T_DTQ));
                        ercd = E_NOMEM;
                    }
                } else {
                    dtq->dtqatr |= TA_UBUF;
                    dtq->dtq = pk_cdtq->dtq;
                }
            }

            if (ercd >= E_OK) {
                if ((dtq->dtqatr & TA_TPRI) != 0U) {
                    que = (T_WTCB *)_kernel_getmem(&_kernel_systbl.free_sys,
                               _kernel_systbl.qrdq.inf->limit * sizeof(T_WTCB));
                    if (que == 0) {
                        if ((dtq->dtq != 0) && ((dtq->dtqatr & TA_UBUF) == 0U)) {
                            (void)_kernel_relmem(mem_base, (T_MEM *)dtq->dtq, dtq->dtqcnt*sizeof(VP_INT));
                        }
                        (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)dtq, sizeof(T_DTQ));
                        ercd = E_NOMEM;
                    } else {
                        _kernel_inique(que, (ID)_kernel_systbl.qrdq.inf->limit);
                        dtq->sque.mwait = que-1;
                    }
                } else {
                    dtq->sque.wait.next = &dtq->sque.wait;
                    dtq->sque.wait.prev = &dtq->sque.wait;
                }
            }

            if (ercd >= E_OK) {
                _kernel_systbl.qdtq.inf->usedc++;
                _kernel_systbl.qdtq.dtq[dtqid] = dtq;
            }
        }
    }
    return ercd;
}

static ER _kernel_credtq(ID dtqid, T_CDTQ *pk_cdtq)
{
    T_SSB *cssb;
    ER ercd;

    if ((pk_cdtq->dtqatr & ~(TA_TPRI|TA_TFIFO)) != 0U) {
        ercd = E_RSATR;
    } else if (pk_cdtq->dtqcnt > 0xFFFFU) {
        ercd = E_PAR;

    } else {
        cssb = _kernel_systbl.cssb;
        if (cssb == 0) {
            _kernel_systbl.stat.s.dlock = (UB)TRUE;
            ercd = _kernel_credtq_1(dtqid, pk_cdtq);
            _kernel_relrun();
        } else if (cssb->stat.catr == TA_INI) {
            ercd = _kernel_credtq_1(dtqid, pk_cdtq);
        } else {
            ercd = E_CTX;
        }
    }
    return ercd;
}

ER cre_dtq(ID dtqid, T_CDTQ *pk_cdtq)
{
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qdtq.inf == 0) ||
               ((dtqid < TMIN_OBJ) || (dtqid > (ID)_kernel_systbl.qdtq.inf->limit))) {
        ercd = E_ID;
    } else {
        ercd = _kernel_credtq(dtqid, pk_cdtq);
    }
    return ercd;
}

ER_ID acre_dtq(T_CDTQ *pk_cdtq)
{
    ER_ID ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (_kernel_systbl.qdtq.inf == 0) {
        ercd = E_NOID;
    } else {
        ercd = (ER_ID)_kernel_credtq(0, pk_cdtq);
    }
    return ercd;
}
