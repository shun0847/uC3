/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2008-2017, eForce Co., Ltd. All rights reserved.

    Version Information
            2008.06.05: Created.
            2009.07.08: Suppressed warning of the RVDS compiler.
            2017.01.26: Fixed the IPA warnings.
            2017.07.19: Fixed the re-definition of overrun handler.
 ***************************************************************************/

#include "uC3sys.h"
#include "string.h"


static ER _kernel_defovr(T_DOVR *pk_dovr);

/***************************************
    Define Overrun Handler
 ***************************************/

static ER _kernel_defovr(T_DOVR *pk_dovr)
{
    T_OVR *ovr = 0;
    T_TCB *tcb;
    ER ercd;
    UH i;

    if (pk_dovr != 0) {
        if (_kernel_systbl.ovr == 0) {
            ovr = (T_OVR *)_kernel_getmem(&_kernel_systbl.free_sys, sizeof(T_OVR));
            if (ovr == 0) {
                ercd = E_NOMEM;
            } else {
                memset((void *)ovr, 0x00, sizeof(T_OVR));
                ovr->ovratr = (UH)pk_dovr->ovratr;
                ovr->ovrhdr = pk_dovr->ovrhdr;
                _kernel_systbl.ovr = ovr;
                ercd = E_OK;
            }
        } else {
            if (_kernel_systbl.cssb == 0) {
                _kernel_lock();
            }
            _kernel_systbl.ovr->ovratr = (UH)pk_dovr->ovratr;
            _kernel_systbl.ovr->ovrhdr = pk_dovr->ovrhdr;
            if (_kernel_systbl.cssb == 0) {
                _kernel_unlock();
            }
            ercd = E_OK;
        }
    } else {
        ovr = _kernel_systbl.ovr;
        _kernel_systbl.ovr = 0;
        for(i = 1U; i <= _kernel_systbl.qtcb.inf->limit; i++) {
            tcb = _kernel_systbl.qtcb.tcb[i];
            if (tcb != 0) {
                tcb->ovrsts = TOVR_STP;
            }
        }
        (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)ovr, sizeof(T_DOVR));
        ercd = E_OK;
    }
    return ercd;
}

ER def_ovr(T_DOVR *pk_dovr)
{
    T_SSB *cssb;
    ER ercd;

    if (_kernel_systbl.icnt != 0U) {
        ercd = E_CTX;

    } else {
        cssb = _kernel_systbl.cssb;
        if (cssb == 0) {
            _kernel_systbl.stat.s.dlock = (UB)TRUE;
            ercd = _kernel_defovr(pk_dovr);
            _kernel_relrun();
        } else if (cssb->stat.catr == TA_INI) {
            ercd = _kernel_defovr(pk_dovr);
        } else {
            ercd = E_CTX;
        }
    }
    return ercd;
}
