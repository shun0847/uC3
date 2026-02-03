/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created
            2017.01.26: Fixed the IPA warnings.
 ***************************************************************************/

#include "uC3sys.h"
#include <string.h>


static ER _kernel_refsys(T_RSYS *pk_rsys);

/***********************************
    Reference System State
 ***********************************/

static ER _kernel_refsys(T_RSYS *pk_rsys)
{
    SIZE maxsz;

    memset((void *)pk_rsys, 0x00, sizeof(T_RSYS));
    _kernel_getsize(&_kernel_systbl.free_sys, &pk_rsys->fsyssz, &maxsz);
    _kernel_getsize(&_kernel_systbl.free_stk, &pk_rsys->fstksz, &maxsz);
    _kernel_getsize(&_kernel_systbl.free_mpl, &pk_rsys->fmplsz, &maxsz);
    pk_rsys->ssbcnt = _kernel_systbl.ssb_min;

    if (_kernel_systbl.qtcb.inf != 0) {
        pk_rsys->utskid = _kernel_systbl.qtcb.inf->usedc;
    }
    if (_kernel_systbl.qsem.inf != 0) {
        pk_rsys->usemid = _kernel_systbl.qsem.inf->usedc;
    }
    if (_kernel_systbl.qflg.inf != 0) {
        pk_rsys->uflgid = _kernel_systbl.qflg.inf->usedc;
    }
    if (_kernel_systbl.qdtq.inf != 0) {
        pk_rsys->udtqid = _kernel_systbl.qdtq.inf->usedc;
    }
    if (_kernel_systbl.qmbx.inf != 0) {
        pk_rsys->umbxid = _kernel_systbl.qmbx.inf->usedc;
    }
    if (_kernel_systbl.qmtx.inf != 0) {
        pk_rsys->umtxid = _kernel_systbl.qmtx.inf->usedc;
    }
    if (_kernel_systbl.qmbf.inf != 0) {
        pk_rsys->umbfid = _kernel_systbl.qmbf.inf->usedc;
    }
    if (_kernel_systbl.qpor.inf != 0) {
        pk_rsys->uporid = _kernel_systbl.qpor.inf->usedc;
    }
    if (_kernel_systbl.qmpf.inf != 0) {
        pk_rsys->umpfid = _kernel_systbl.qmpf.inf->usedc;
    }
    if (_kernel_systbl.qmpl.inf != 0) {
        pk_rsys->umplid = _kernel_systbl.qmpl.inf->usedc;
    }
    if (_kernel_systbl.qalm.inf != 0) {
        pk_rsys->ualmid = _kernel_systbl.qalm.inf->usedc;
    }
    if (_kernel_systbl.qcyc.inf != 0) {
        pk_rsys->ucycid = _kernel_systbl.qcyc.inf->usedc;
    }
    if (_kernel_systbl.qisr.inf != 0) {
        pk_rsys->uisrid = _kernel_systbl.qisr.inf->usedc;
    }
    return E_OK;
}

ER ref_sys(T_RSYS *pk_rsys)
{
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;
    } else if (_kernel_systbl.cssb == 0) {
        _kernel_systbl.stat.s.dlock = (UB)TRUE;
        ercd = _kernel_refsys(pk_rsys);
        _kernel_relrun();
    } else {
        ercd = _kernel_refsys(pk_rsys);
    }
    return ercd;
}
