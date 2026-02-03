/**
 * @file    uC3krncm2.c
 * @brief   Micro C Cube Standard, KERNEL
 *          AArch64 dependent function
 * @date    2019.04.01
 * @author  Copyright (c) 2016-2019, eForce Co., Ltd. All rights reserved.
 *
 ******************************************************************************
 * @par     History
 *          - rev 1.0 (2016.09.21) y-kim
 *            Created based on uC3krncm2.c of ARMv7-A.
 *          - rev 1.1 (2017.04.25) yokota
 *            Fixed the IPA warnings.
 *          - rev 1.2 (2018.06.04) yokota
 *            Fixed trace func type at _kernel_blktrace.
 *          - rev 1.3 (2019.04.01) i-cho
 *            Fixed buffer overflow at _kernel_iniint.
 ******************************************************************************
 */
#include "uC3sys.h"
#include <string.h>

/***********************************
    Internel functions
 ***********************************/

void _kernel_iniint(void)
{
    T_VINFTBL * vinftbl = &_kernel_vinftbl_begin()[0];
    UW i;
    for (i = 0U; &vinftbl[i] < &_kernel_vinftbl_end()[0]; i++) {
        vinftbl[i].next = 0;
        vinftbl[i].prev = 0;
        vinftbl[i].intinfo = 0U;
        vinftbl[i].intfunc = &int_abort;
    }
}

void _kernel_setctx(T_TCB *tcb, VP_INT stacd)
{
#ifdef _KERNEL_FPU_VFP
    tcb->sp -= 6;
    tcb->sp[0] = ((tcb->stat.oatr & TA_FPU) == 0U)?0ULL:0x00300000ULL;
    tcb->sp[1] = _kernel_systbl.cpudep.fpscr;
    tcb->sp[2] = 0UL;
    tcb->sp[3] = (T_REG)&ext_tsk;
    tcb->sp[4] = (T_REG)stacd;
    tcb->sp[5] = (T_REG)tcb->task;
#else
    tcb->sp -= 4;
    tcb->sp[0] = 0UL;
    tcb->sp[1] = (T_REG)&ext_tsk;
    tcb->sp[2] = (T_REG)stacd;
    tcb->sp[3] = (T_REG)tcb->task;
#endif
    tcb->stat.catr = TA_STA;
}

void _kernel_timfunc(T_CSTS stat, UH objid, FP timfunc, VP_INT exinf)
{
    IMASK imask;
    UW old_ctxid;
    UW new_ctxid;

    new_ctxid = ((UW)stat.catr << 16) | ((UW)objid);
    old_ctxid = _kernel_systbl.ctxid_c;
    if (_kernel_systbl.ctxtrace3 != 0) {
        UW microtim;
        imask = _kernel_set_imask(1U);
        microtim = _kernel_micro_systim();
        ((void (*)(UW,UW,UW,UD))_kernel_systbl.ctxtrace3)(old_ctxid, new_ctxid, microtim, (UD)&timfunc);
    } else {
        imask = _kernel_set_imask(1U);
    }
    _kernel_systbl.ctxid_c = new_ctxid;
    _kernel_systbl.ctxid_s = new_ctxid;
    (void)_kernel_set_imask(imask);
#ifdef _KERNEL_FPU_VFP
    if ((stat.oatr & TA_FPU) != 0U) {
        vena_vfp();
        vset_fpscr((UW)_kernel_systbl.cpudep.fpscr);
    }
#endif
    ((void (*)(VP_INT))timfunc)(exinf);
#ifdef _KERNEL_FPU_VFP
    vdis_vfp();
#endif
}

void _kernel_ovrfunc(ATR atr, FP ovrfunc, T_TCB *tcb)
{
    IMASK imask;
    UW old_ctxid;
    UW new_ctxid;

    new_ctxid = ((UW)TA_OVR << 16) | ((UW)tcb->tskid & 0xFFFFU);
    old_ctxid = _kernel_systbl.ctxid_c;
    if (_kernel_systbl.ctxtrace3 != 0) {
        UW microtim;
        imask = _kernel_set_imask(1U);
        microtim = _kernel_micro_systim();
        ((void (*)(UW,UW,UW,UW))_kernel_systbl.ctxtrace3)(old_ctxid, new_ctxid, microtim, (UW)&ovrfunc);
    } else {
        imask = _kernel_set_imask(1U);
    }
    _kernel_systbl.ctxid_c = new_ctxid;
    _kernel_systbl.ctxid_s = new_ctxid;
    (void)_kernel_set_imask(imask);
#ifdef _KERNEL_FPU_VFP
    if ((atr & TA_FPU) != 0U) {
        vena_vfp();
        vset_fpscr((UW)_kernel_systbl.cpudep.fpscr);
    }
#endif
    ((void (*)(ID,VP_INT))ovrfunc)((ID)tcb->tskid, tcb->exinf);
#ifdef _KERNEL_FPU_VFP
    vdis_vfp();
#endif
}

void _kernel_agentfunc(FP agentfunc)
{
    _kernel_systbl.ctxid_s = ((UW)TA_AGT << 16);
#ifdef _KERNEL_FPU_VFP
    vena_vfp();
    vset_fpscr((UW)_kernel_systbl.cpudep.fpscr);
#endif
    agentfunc();
    _kernel_systbl.ctxid_s = _kernel_systbl.ctxid_c;
#ifdef _KERNEL_FPU_VFP
    vdis_vfp();
#endif
}

void _kernel_blktrace(UW ctxid, FN fncode, ID objid, VP blk, SIZE blksz)
{
    UW microtim;
    ADDR pc;

    microtim = _kernel_micro_systim();
    if ((ctxid & 0xFFFF0000U) == 0U) {
        pc = (ADDR)_kernel_systbl.qtcb.tcb[ctxid & 0x0000FFFFU]->pc;
    } else {
        pc = 0UL;
    }
    ((void (*)(UW,FN,UW,ADDR,ID,ADDR,SIZE))_kernel_systbl.ctxtrace4)(ctxid, fncode,
                             microtim, pc, objid, (ADDR)blk, blksz);
}

void _kernel_error_handler(FN func_code, ER error_code, VP_INT *para)
{
    UW old_ctxid;
    UW new_ctxid;

    new_ctxid = 0x00600000U;
    old_ctxid = _kernel_systbl.ctxid_c;
    if (new_ctxid != old_ctxid) {
        IMASK imask = _kernel_set_imask(1U);
        if (_kernel_systbl.ctxtrace3 != 0) {
            UW microtim = _kernel_micro_systim();
            ((void (*)(UW,UW,UW,UD))_kernel_systbl.ctxtrace3)(
                old_ctxid, new_ctxid, microtim, (UD)_kernel_systbl.errhandler);
        }
        _kernel_systbl.ctxid_c = new_ctxid;
        _kernel_systbl.ctxid_s = new_ctxid;
        (void)_kernel_set_imask(imask);
    }
#ifdef _KERNEL_FPU_VFP
    if ((_kernel_systbl.atrhandler & TA_FPU) != 0U) {
        vena_vfp();
        vset_fpscr((UW)_kernel_systbl.cpudep.fpscr);
    }
#endif
    ((void (*)(FN,ER,VP_INT*))_kernel_systbl.errhandler)(
                func_code, error_code, para);
#ifdef _KERNEL_FPU_VFP
    vdis_vfp();
#endif
}

BOOL _kernel_inihdr(T_SSB *ssb)
{
    _kernel_systbl.ctxid_c = (UW)TA_INI << 16;
    _kernel_systbl.ctxid_s = (UW)TA_INI << 16;
    if (_kernel_systbl.ctxtrace3 != 0) {
        UW microtim = _kernel_micro_systim();
        ((void (*)(UW,UW,UW,UD))_kernel_systbl.ctxtrace3)(0x00000000, ((UW)TA_INI << 16), microtim, (UD)ssb->p1);
    }
#ifdef _KERNEL_FPU_VFP
    vena_vfp();
    _kernel_systbl.cpudep.fpscr = vget_fpscr();
#endif
    ((void (*)(void))(ADDR)ssb->p1)();
#ifdef _KERNEL_FPU_VFP
    vdis_vfp();
#endif
    _KERNEL_START_MULTI_TASK();
    ssb->p1 = (VP_INT)E_OK;
    return TRUE;
}

void _kernel_entisr(T_ISR *isr)
{
    for(;isr != 0;isr = isr->next) {
#ifdef _KERNEL_FPU_VFP
        if ((isr->isratr & TA_FPU) != 0U) {
            vena_vfp();
            vset_fpscr((UW)_kernel_systbl.cpudep.fpscr);
        } else {
            vdis_vfp();
        }
#endif
        ((void (*)(VP_INT))isr->isr)(isr->exinf);
#ifdef _KERNEL_FPU_VFP
        vdis_vfp();
#endif
    }
}

BOOL _kernel_snsloc(void)
{
    return (BOOL)((_kernel_get_imask() == 1U)? TRUE : FALSE);
}

IMASK _kernel_getims(void)
{
    return _kernel_get_imask();
}

IMASK _kernel_setims(IMASK imask)
{
    return _kernel_set_imask(imask);
}
