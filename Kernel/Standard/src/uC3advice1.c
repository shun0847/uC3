/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2013-2018, eForce Co., Ltd. All rights reserved.

    Version Information
            2013.09.30: Created.
            2017.03.02: Fixed the IPA warnings.
            2017.07.21: Fixed that when an interrupt occurred during dispatching,
                        the advice trace log was not output correctly.
            2018.05.29: change pc parameter type from UW to ADDR.
 ***************************************************************************/

#define _UC3ADVICE_C_

#include "uC3sys.h"

extern int _SMT_OsSwitch_Process(unsigned long processid);
extern int _SMT_OsSwitch_ThreadProcess(unsigned long threadid,unsigned long processid);
extern int _SMT_OsSwitch_Process_Name(unsigned long processid,const char *str);
extern int _SMT_OsSwitch_ThreadProcess_Name(unsigned long threadid,unsigned long processid,const char *tname,const char *pname);
extern int _SMT_OsSwitch_Irq_in(unsigned long irqid);
extern int _SMT_OsSwitch_Irq_out(unsigned long irqid);
extern int _SMT_OsSwitch_Idle(void);

static void _advice_swctx(UW pre_ctxid, UW next_ctxid, UW tim, ADDR pc);


/***************************************************
    Trace contex for CSIDE
 ***************************************************/

void _kernel_advice_tarce_1(void)
{
    _kernel_systbl.ctxtrace0 = (FP)_advice_swctx;
    _kernel_systbl.ctxtrace1 = (FP)_advice_swctx;
    _kernel_systbl.ctxtrace2 = (FP)_advice_swctx;
    _kernel_systbl.ctxtrace3 = (FP)_advice_swctx;
    _kernel_systbl.ctxtrace4 = (FP)0;
    _kernel_systbl.systrace = (FP)0;
    _kernel_systbl.rettrace = (FP)0;
}


static UINT save_icnt = 0U;

static void _advice_swctx(UW pre_ctxid, UW next_ctxid, UW tim, ADDR pc)
{
    const VB *str;

    if ((next_ctxid & 0x00FF0000UL) == 0x00200000UL) {
        if ((UINT)_kernel_systbl.icnt > save_icnt) {
            (void)_SMT_OsSwitch_Irq_in(next_ctxid & 0x0000FFFFUL);
        } else if ((UINT)_kernel_systbl.icnt == save_icnt) {
            (void)_SMT_OsSwitch_Irq_out(pre_ctxid & 0x0000FFFFUL);
            (void)_SMT_OsSwitch_Irq_in(next_ctxid & 0x0000FFFFUL);
        } else {
            (void)_SMT_OsSwitch_Irq_out(pre_ctxid & 0x0000FFFFUL);
        }
        save_icnt = _kernel_systbl.icnt;
    } else {
        if ((pre_ctxid & 0x00FF0000UL) == 0x00200000UL) {
            (void)_SMT_OsSwitch_Irq_out(pre_ctxid & 0x0000FFFFUL);
            save_icnt = 0U;
        }
        if ((next_ctxid & 0x00FF0000UL) == 0x00100000UL) {
            (void)_SMT_OsSwitch_Idle();
        } else if ((next_ctxid & 0x00FF0000UL) == 0x00300000UL) {
            (void)_SMT_OsSwitch_Process(next_ctxid);
        } else {
            switch(next_ctxid & 0x00FF0000UL) {
                case 0x00000000UL:
                    str = _kernel_systbl.qtcb.tcb[next_ctxid & 0x0000FFFFUL]->name;
                    break;
                case 0x00400000UL:
                    str = _kernel_systbl.qcyc.cyc[next_ctxid & 0x0000FFFFUL]->name;
                    break;
                case 0x00800000UL:
                    str = _kernel_systbl.qalm.alm[next_ctxid & 0x0000FFFFUL]->name;
                    break;
                default:
                    str = 0;
                    break;
            }
            if (str == 0) {
                (void)_SMT_OsSwitch_Process(next_ctxid);
            } else {
                (void)_SMT_OsSwitch_Process_Name(next_ctxid, str);
            }
        }
    }
}
