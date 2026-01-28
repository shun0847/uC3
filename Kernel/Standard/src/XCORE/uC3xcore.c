/***********************************************************************
    Micro C Cube Standard, Hetero Core extension
    Synchronization and Asynchronization System Call

    Copyright (c)  2020-2021, eForce Co., Ltd. All rights reserved.

    Version Information
            2020.05.08: Created.
            2021.10.13: Removed _kernel_synch_cache() after the sysfunc()
                        invoation.
                        (_kernel_synch_cache() should be done in each
                        system call function)
                        Fixed SSB starvation.
            2021.11.04: Removed reqevt check functionality
            2021.12.09: change get_cid to  get_hcid .
 ***********************************************************************
 */
#define _UC3XCORE_C_

#include "uC3sys.h"
#include "uC3xcext.h"


/* External functions --------------------------------------------------------*/

extern void _kernel_vacttsk_1(T_SYNCTBL *syncbuf);
extern void _kernel_vacttsk_2(T_CMDBUF *buffer);
extern void _kernel_vclrflg_1(T_SYNCTBL *syncbuf);
extern void _kernel_vpolflg_1(T_SYNCTBL *syncbuf);
extern void _kernel_vpolsem_1(T_SYNCTBL *syncbuf);
extern void _kernel_vrcvdtq_1(T_SYNCTBL *syncbuf);
extern void _kernel_vrelwai_1(T_SYNCTBL *syncbuf);
extern void _kernel_vrelwai_2(T_CMDBUF *buffer);
extern void _kernel_vrotrdq_1(T_SYNCTBL *syncbuf);
extern void _kernel_vrotrdq_2(T_CMDBUF *buffer);
extern void _kernel_vsetflg_1(T_SYNCTBL *syncbuf);
extern void _kernel_vsetflg_2(T_CMDBUF *buffer);
extern void _kernel_vsigsem_1(T_SYNCTBL *syncbuf);
extern void _kernel_vsigsem_2(T_CMDBUF *buffer);
extern ER   _kernel_sigtim(void);
extern void _kernel_vsnddtq_1(T_SYNCTBL *syncbuf);
extern void _kernel_vsnddtq_2(T_CMDBUF *buffer);
extern void _kernel_vstatsk_1(T_SYNCTBL *syncbuf);
extern void _kernel_vstatsk_2(T_CMDBUF *buffer);
extern void _kernel_vwuptsk_1(T_SYNCTBL *syncbuf);
extern void _kernel_vwuptsk_2(T_CMDBUF *buffer);

/* Private variables ---------------------------------------------------------*/

static FP xc_func_list[] = {
    (FP)&_kernel_vacttsk_1,
    (FP)&_kernel_vacttsk_2,
    (FP)&_kernel_vclrflg_1,
    (FP)&_kernel_vpolflg_1,
    (FP)&_kernel_vpolsem_1,
    (FP)&_kernel_vrcvdtq_1,
    (FP)&_kernel_vrelwai_1,
    (FP)&_kernel_vrelwai_2,
    (FP)&_kernel_vrotrdq_1,
    (FP)&_kernel_vrotrdq_2,
    (FP)&_kernel_vsetflg_1,
    (FP)&_kernel_vsetflg_2,
    (FP)&_kernel_vsigsem_1,
    (FP)&_kernel_vsigsem_2,
    (FP)&_kernel_sigtim,
    (FP)&_kernel_vsnddtq_1,
    (FP)&_kernel_vsnddtq_2,
    (FP)&_kernel_vstatsk_1,
    (FP)&_kernel_vstatsk_2,
    (FP)&_kernel_vwuptsk_1,
    (FP)&_kernel_vwuptsk_2
};

static T_SSB xcore_ssb = { 0 };

/*******************************************************
        Async Syscall
 *******************************************************/

ER _kernel_async_sys(FN func_n, ID ctexid, UW seq_no, XPTR ret_pc, XC_FUNC_IDX func, XPARAM *para)
{
    UINT i;
    ER ercd;
    ID hcoreid;
    ID my_hcoreid;
    ID coreid;
    ID my_coreid;
    ID domainid;
    ID my_domainid;
    T_CMDBUF *buffer;
    T_ASYNCTBL *asynctbl;

    hcoreid = (ID)para[1];
    my_hcoreid = (ID)get_hcid();
    domainid = TO_DOMAINID(hcoreid);
    my_domainid = TO_DOMAINID(my_hcoreid); 
    coreid = TO_COREID(hcoreid) - 1;
    coreid &= (TMAX_CORE - 1);
    my_coreid = TO_COREID(my_hcoreid) - 1;
    my_coreid &= (TMAX_CORE - 1);

    if (coreid >= (ID)_kernel_xcore_ext.domain[domainid-1].core_max) {
        ercd = E_PAR;
    } else if ((_kernel_xcore_ext.domain[domainid-1].ready[coreid]    != 2U) ||
               (_kernel_xcore_ext.domain[my_domainid-1].ready[my_coreid] != 2U)) {
        ercd = E_OBJ;
    } else {
        asynctbl = (T_ASYNCTBL*)((ADDR)_kernel_xcore_ext.domain[domainid-1].asynctbl[coreid]);
        _kernel_spin_lock(&asynctbl->lock);

        if (asynctbl->cnt < (UW)_kernel_xcore_ext.domain[domainid-1].async_max) {
            buffer = &asynctbl->cmdbuf[asynctbl->ptr];
            buffer->func_n = func_n;
            buffer->ctexid = (ID)ctexid | ((ID)get_hcid()<<24);
            buffer->seq_no = seq_no;
            buffer->func = func;
            buffer->ret_pc = ret_pc;
            for (i = 0U; i < (UINT)para[0]; i++) {
                buffer->para[i] = para[i+1U];
            }
            asynctbl->cnt++;
            asynctbl->ptr++;
            if (asynctbl->ptr >= (UW)_kernel_xcore_ext.domain[domainid-1].async_max) {
                asynctbl->ptr = 0U;
            }
            ercd = E_OK;
        } else {
            ercd = E_NOMEM;
        }
        _kernel_synch_cache();
        _kernel_spin_unlock(&asynctbl->lock);
        if (ercd == E_OK) {
            _kernel_async_sync_event_set(hcoreid);
        }
    }
    return ercd;
}

BOOL _kernel_async_sys_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_async_sys((FN)par->p1, (ID)par->p2, (UW)par->p3,
                                       (XPTR)((ADDR)par->p4), (XC_FUNC_IDX)par->p5, (XPARAM*)((ADDR)par->p6));
    return retcd;
}


/*******************************************************
        Sync syscall
 *******************************************************/

ER _kernel_sync_sys(FN func_n, ID ctexid, UW seq_no, XPTR ret_pc, XC_FUNC_IDX func, XPARAM *para)
{
    T_SYNCTBL *synctbl;
    T_SYNCTBL *my_synctbl;
    void (*sysfunc)(T_SYNCTBL*);
    UINT i;
    ID hcoreid;
    ID my_hcoreid;
    ID coreid;
    ID my_coreid;
    ID domainid;
    ID my_domainid;
    ER ercd;

    hcoreid = (ID)para[1];
    my_hcoreid = (ID)get_hcid();
    domainid = TO_DOMAINID(hcoreid);
    my_domainid = TO_DOMAINID(my_hcoreid); 
    coreid = TO_COREID(hcoreid) - 1;
    coreid &= (TMAX_CORE - 1);
    my_coreid = TO_COREID(my_hcoreid) - 1;
    my_coreid &= (TMAX_CORE - 1);

    if (coreid >= (ID)_kernel_xcore_ext.domain[domainid-1].core_max) {
        ercd = E_PAR;
    } else if ((_kernel_xcore_ext.domain[domainid-1].ready[coreid]    != 2U) ||
               (_kernel_xcore_ext.domain[my_domainid-1].ready[my_coreid] != 2U)) {
        ercd = E_OBJ;
    } else {
        synctbl = (T_SYNCTBL*)((ADDR)_kernel_xcore_ext.domain[domainid-1].synctbl[coreid]);
        my_synctbl = (T_SYNCTBL*)((ADDR)_kernel_xcore_ext.domain[my_domainid-1].synctbl[my_coreid]);
        for(;;) {
            if (_kernel_spin_lock_test(&synctbl->lock) == 0) {
                synctbl->fin = 0U;
                synctbl->cmdbuf.ctexid = ctexid | ((ID)get_hcid()<<24);
                synctbl->cmdbuf.seq_no = seq_no;
                synctbl->cmdbuf.func = func;
                synctbl->cmdbuf.ret_pc = ret_pc;
                for (i = 0U; i < (UINT)para[0]; i++) {
                    synctbl->cmdbuf.para[i] = para[i+1U];
                }
                _kernel_synch_cache();
                synctbl->cmdbuf.func_n = func_n;
                _kernel_synch_cache();
                _kernel_async_sync_event_set(hcoreid);
                break;
            } else if (_kernel_spin_locked_test(&my_synctbl->lock) != 0) {
                if ((my_synctbl->fin == 0U) && (my_synctbl->cmdbuf.func_n != 0)) {
                    sysfunc = (void (*)(T_SYNCTBL*))xc_func_list[my_synctbl->cmdbuf.func];
                    sysfunc(my_synctbl);
                }
            } else {
                /* Do Nothing */
            }
        }

        for(;;) {
            if (synctbl->fin != 0U) {
                ercd = synctbl->error;
                para[0] = synctbl->ret_v;
                synctbl->cmdbuf.func_n = 0;
                _kernel_synch_cache();
                synctbl->fin = 0U;
                _kernel_synch_cache();
                _kernel_spin_unlock(&synctbl->lock);
                break;
            } else if (_kernel_spin_locked_test(&my_synctbl->lock) != 0) {
                if ((my_synctbl->fin == 0U) && (my_synctbl->cmdbuf.func_n != 0)) {
                    sysfunc = (void (*)(T_SYNCTBL*))xc_func_list[my_synctbl->cmdbuf.func];
                    sysfunc(my_synctbl);
                }
            } else {
                /* Do Nothing */
            }
        }
    }
    return ercd;
}

BOOL _kernel_sync_sys_1(T_SSB *par, BOOL retcd)
{
    par->p1 = (VP_INT)_kernel_sync_sys((FN)par->p1, (ID)par->p2, (UW)par->p3,
                                       (XPTR)((ADDR)par->p4), (XC_FUNC_IDX)par->p5, (XPARAM*)((ADDR)par->p6));
    return retcd;
}


/*******************************************************
        Sync, Async Syscall Driver
 *******************************************************/

BOOL _ddr_xcore_func_drv(T_SSB *par, BOOL retcd)
{
    (void)retcd;
    void (*asysfunc)(T_CMDBUF*);
    void (*sysfunc)(T_SYNCTBL*);
    T_CMDBUF buffer;
    T_ASYNCTBL *asynctbl;
    T_SYNCTBL *my_synctbl;
    INT ptr;
    ID domainid = (ID)get_domain_id();
    ID cid = get_hcid();
    cid = TO_COREID(cid) - 1U;

    par->p1 = (VP_INT)FALSE;
    asynctbl = (T_ASYNCTBL*)((ADDR)_kernel_xcore_ext.domain[domainid-1].asynctbl[cid]);
    my_synctbl = (T_SYNCTBL*)((ADDR)_kernel_xcore_ext.domain[domainid-1].synctbl[cid]);
    for(;;) {
        if (asynctbl->cnt > 0U) {
            _kernel_spin_lock(&asynctbl->lock);
            ptr = (INT)asynctbl->ptr - (INT)asynctbl->cnt;
            if (ptr < 0) {
                ptr += (INT)_kernel_xcore_ext.domain[domainid-1].async_max;
            }
            buffer.func_n = asynctbl->cmdbuf[ptr].func_n;
            buffer.ctexid = asynctbl->cmdbuf[ptr].ctexid;
            buffer.seq_no = asynctbl->cmdbuf[ptr].seq_no;
            buffer.func = asynctbl->cmdbuf[ptr].func;
            buffer.ret_pc = asynctbl->cmdbuf[ptr].ret_pc;
            buffer.para[0] = asynctbl->cmdbuf[ptr].para[0];
            buffer.para[1] = asynctbl->cmdbuf[ptr].para[1];
            buffer.para[2] = asynctbl->cmdbuf[ptr].para[2];
            buffer.para[3] = asynctbl->cmdbuf[ptr].para[3];
            asynctbl->cnt--;
            _kernel_synch_cache();
            _kernel_spin_unlock(&asynctbl->lock);
            asysfunc = (void (*)(T_CMDBUF*))xc_func_list[buffer.func];
            asysfunc(&buffer);
        } else {
            break;
        }
    }
    if (_kernel_spin_locked_test(&my_synctbl->lock) != 0) {
        if ((my_synctbl->fin == 0U) && (my_synctbl->cmdbuf.func_n != 0)) {
            sysfunc = (void (*)(T_SYNCTBL*))xc_func_list[my_synctbl->cmdbuf.func];
            sysfunc(my_synctbl);
        }
    }
    return FALSE; // Always return 'FALSE' to avoid being registered with the freed-SSB list by rel_ssb()
}

void _kernel_xcore_enqssb(void)
{
    if ((BOOL)((ADDR)xcore_ssb.p1) == FALSE) {
        xcore_ssb.stat.catr = TA_SSB;
        xcore_ssb.p1 = (VP_INT)TRUE;
        xcore_ssb.sysfunc = (FP)&_ddr_xcore_func_drv;
        _kernel_enqssb(&xcore_ssb);
    }
}

