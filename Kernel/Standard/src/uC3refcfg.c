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


/*******************************************
    Reference Configuration Information
 *******************************************/

ER ref_cfg(T_RCFG *pk_rcfg)
{
    memset((void *)pk_rcfg, 0x00, sizeof(T_RCFG));

    pk_rcfg->ssb_cnt = _kernel_systbl.ssb_cnt;
    pk_rcfg->tick = _kernel_systbl.tick;
    pk_rcfg->tskpri_max = _kernel_systbl.qrdq.inf->limit;
    if (_kernel_systbl.qtcb.inf != 0) {
        pk_rcfg->tskid_max = _kernel_systbl.qtcb.inf->limit;
    }
    if (_kernel_systbl.qsem.inf != 0) {
        pk_rcfg->semid_max = _kernel_systbl.qsem.inf->limit;
    }
    if (_kernel_systbl.qflg.inf != 0) {
        pk_rcfg->flgid_max = _kernel_systbl.qflg.inf->limit;
    }
    if (_kernel_systbl.qdtq.inf != 0) {
        pk_rcfg->dtqid_max = _kernel_systbl.qdtq.inf->limit;
    }
    if (_kernel_systbl.qmbx.inf != 0) {
        pk_rcfg->mbxid_max = _kernel_systbl.qmbx.inf->limit;
    }
    if (_kernel_systbl.qmtx.inf != 0) {
        pk_rcfg->mtxid_max = _kernel_systbl.qmtx.inf->limit;
    }
    if (_kernel_systbl.qmbf.inf != 0) {
        pk_rcfg->mbfid_max = _kernel_systbl.qmbf.inf->limit;
    }
    if (_kernel_systbl.qpor.inf != 0) {
        pk_rcfg->porid_max = _kernel_systbl.qpor.inf->limit;
    }
    if (_kernel_systbl.qmpf.inf != 0) {
        pk_rcfg->mpfid_max = _kernel_systbl.qmpf.inf->limit;
    }
    if (_kernel_systbl.qmpl.inf != 0) {
        pk_rcfg->mplid_max = _kernel_systbl.qmpl.inf->limit;
    }
    if (_kernel_systbl.qalm.inf != 0) {
        pk_rcfg->almid_max = _kernel_systbl.qalm.inf->limit;
    }
    if (_kernel_systbl.qcyc.inf != 0) {
        pk_rcfg->cycid_max = _kernel_systbl.qcyc.inf->limit;
    }
    if (_kernel_systbl.qisr.inf != 0) {
        pk_rcfg->isrid_max = _kernel_systbl.qisr.inf->limit;
    }
    return E_OK;
}
