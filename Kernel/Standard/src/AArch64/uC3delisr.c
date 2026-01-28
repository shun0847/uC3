/**
 * @file    uC3delisr.c
 * @brief   Micro C Cube Standard, KERNEL
 *          AArch64 dependent function
 * @date    2017.04.25
 * @author  Copyright (c) 2016-2017, eForce Co., Ltd. All rights reserved.
 *
 ******************************************************************************
 * @par     History
 *          - rev 1.0 (2016.09.21) y-kim
 *            Created based on uC3delisr.c of ARMv7-A.
 *          - rev 1.1 (2017.04.25) yokota
 *            Fixed the IPA warnings.
 ******************************************************************************
 */
#include "uC3sys.h"

static ER _kernel_delisr(ID isrid);

/***************************************
    Delete Interrupt Service Routine
 ***************************************/

static ER _kernel_delisr(ID isrid)
{
    ER ercd = E_OK;
    T_ISR *isr = _kernel_systbl.qisr.isr[isrid];

    if (isr == 0) {
        ercd = E_NOEXS;
    } else {
        SIZE maxhdr = _kernel_vinftbl_length() / sizeof(T_VINFTBL);
        if (maxhdr == 0ULL) {
            ercd = E_PAR;
        } else {
            T_VINFTBL *vinftbl = &_kernel_vinftbl_begin()[isr->intno];
            if ((!vinftbl) ||
            	(vinftbl->intfunc != (FP)&_kernel_entisr)) {
                ercd = E_PAR;
            } else {
                T_ISR *isr1 = (T_ISR *)&vinftbl->next;
                while ((isr1 != 0) && (isr1->next != isr))
                {
                    isr1 = isr1->next;
                }
                _kernel_lock();
                isr1->next = isr->next;
                if (isr1->next == 0) {
                    vinftbl->prev = isr1;
                }
                if (vinftbl->next == 0) {
                    vinftbl->next = 0;
                    vinftbl->prev = 0;
                    vinftbl->intinfo = 0U;
                    vinftbl->intfunc = &int_abort;
                }
                _kernel_unlock();
                _kernel_systbl.qisr.isr[isrid] = 0;
                _kernel_systbl.qisr.inf->usedc--;
                (void)_kernel_relmem(&_kernel_systbl.free_sys, (T_MEM *)isr, sizeof(T_ISR));
            }
        }
    }
    return ercd;
}

ER del_isr(ID isrid)
{
    ER ercd;

    if ((_kernel_systbl.icnt != 0U) ||
        (_kernel_systbl.cssb != 0)) {
        ercd = E_CTX;
    } else if (( _kernel_systbl.qisr.inf == 0) ||
               (isrid < TMIN_OBJ) ||
               (isrid > (ID)_kernel_systbl.qisr.inf->limit)) {
        ercd = E_ID;
    } else {
        _kernel_systbl.stat.s.dlock = 1U;
        ercd = _kernel_delisr(isrid);
        _kernel_relrun();
    }
    return ercd;
}
