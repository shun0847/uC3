/***************************************************************************
    Micro C Cube Standard, KERNEL
    Kernel Common

    Copyright (c)  2013-2018, eForce Co., Ltd. All rights reserved.

    Version Information
            2013.05.21: Created.
            2017.03.02: Fixed the IPA warnings.
            2018.05.29: change pc parameter type from UW to ADDR.
            2018.06.04: change func type to FN, ercd type to ER.
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

typedef enum {
    _SMT_OS_ATTR_CALL = 0,
    _SMT_OS_ATTR_RET
} _SMT_OS_CALL ;

extern int _SMT_OsCall0(int osc,_SMT_OS_CALL attr);
extern int _SMT_OsCall1(int osc,_SMT_OS_CALL attr,unsigned long arg1);
extern int _SMT_OsCall2(int osc,_SMT_OS_CALL attr,unsigned long arg1,unsigned long arg2);
extern int _SMT_OsCall3(int osc,_SMT_OS_CALL attr,unsigned long arg1,unsigned long arg2,unsigned long arg3);
extern int _SMT_OsCall4(int osc,_SMT_OS_CALL attr,unsigned long arg1,unsigned long arg2,unsigned long arg3,unsigned long arg4);
extern int _SMT_OsCall5(int osc,_SMT_OS_CALL attr,unsigned long arg1,unsigned long arg2,unsigned long arg3,unsigned long arg4,unsigned long arg5);

void _advice_swctx(UW pre_ctxid, UW next_ctxid, UW tim, ADDR pc);
void _advice_syscall(UW cur_ctxid, FN func, UW tim, ADDR pc, UH count, UW *para);
void _advice_sysret(UW cur_ctxid, FN func, UW tim, ADDR pc, ER ercd);


/***************************************************
    Trace contex for CSIDE
 ***************************************************/

void _kernel_advice_tarce_2(void)
{
    _kernel_systbl.ctxtrace0 = (FP)_advice_swctx;
    _kernel_systbl.ctxtrace1 = (FP)_advice_swctx;
    _kernel_systbl.ctxtrace2 = (FP)_advice_swctx;
    _kernel_systbl.ctxtrace3 = (FP)_advice_swctx;
    _kernel_systbl.ctxtrace4 = (FP)0;
    _kernel_systbl.systrace = (FP)_advice_syscall;
    _kernel_systbl.rettrace = (FP)_advice_sysret;
}


static UINT save_icnt = 0U;

void _advice_swctx(UW pre_ctxid, UW next_ctxid, UW tim, ADDR pc)
{
    const VB *str;

    if ((next_ctxid & 0x00FF0000UL) == 0x00200000UL) {
        if ((UINT)_kernel_systbl.icnt > save_icnt) {
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

void _advice_syscall(UW cur_ctxid, FN func, UW tim, ADDR pc, UH count, UW *para)
{
    switch(count) {
        case 5U:
            (void)_SMT_OsCall5((INT)func, _SMT_OS_ATTR_CALL, para[0], para[1], para[2], para[3], para[4]);
            break;
        case 4U:
            (void)_SMT_OsCall4((INT)func, _SMT_OS_ATTR_CALL, para[0], para[1], para[2], para[3]);
            break;
        case 3U:
            (void)_SMT_OsCall3((INT)func, _SMT_OS_ATTR_CALL, para[0], para[1], para[2]);
            break;
        case 2U:
            (void)_SMT_OsCall2((INT)func, _SMT_OS_ATTR_CALL, para[0], para[1]);
            break;
        case 1U:
            (void)_SMT_OsCall1((INT)func, _SMT_OS_ATTR_CALL, para[0]);
            break;
        default:
            (void)_SMT_OsCall0((INT)func, _SMT_OS_ATTR_CALL);
            break;
    }
}

void _advice_sysret(UW cur_ctxid, FN func, UW tim, ADDR pc, ER ercd)
{
    (void)_SMT_OsCall1((INT)func, _SMT_OS_ATTR_RET, ercd);
}
