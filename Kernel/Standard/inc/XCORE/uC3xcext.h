/**
 * @file    uC3xcext.h
 * @brief   Micro C Cube Standard, HeteroMulti-Core Extension
 *          Synchronization and Asynchronization System Call
 * @date    2021.10.28
 * @author  Copyright (c) 2021, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2020.04.17)
 *            Created.
 *          - rev 1.1 (2020.09.17)
 *            redefine T_XCORE_EXT.sync_domain.
 *          - rev 1.2 (2021.02.19)
 *            add XPARAM_SIZE macro for 32bit support.
 *          - rev 1.3 (2021.10.13)
 *            Added the volatile attribute to member variables in T_SYNCTBL.
 *            Removed the reqevt variable from T_SYNCTBL
 *          - rev 1.4 (2021.10.28)
 *            Added TKERNEL_XCEXT_PRVER for XCORE specific versioning
 *          - rev 1.4 (2021.12.09)
 *            Changed TKERNELXCEXT_PRVER to 1.00.1. 
 ****************************************************************************
 */
#ifndef _UC3XCEXT_H_
#define _UC3XCEXT_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef UW XC_FUNC_IDX;

#if XPARAM_SIZE == 32
typedef UW XPARAM;
typedef UW XPTR;
#else
typedef UD XPARAM;
typedef UD XPTR;
#endif

#define TKERNEL_XCEXT_PRVER 0x1001

#define DOMAIN_MASK  0xFF000000U
#define CORE_MASK    0x00FFFFFFU

#define TO_DOMAINID(coreid)     ((ID)(coreid >> 24))
#define TO_COREID(coreid)       ((ID)(coreid & CORE_MASK))

#define TMIN_DOMAIN   1U
#define TMAX_DOMAIN   4U

#define TMIN_CORE     1U
#define TMAX_CORE     4U

#define SYNCCALL_SYNC_MODE       0UL
#define SYNCCALL_ASYNC_MODE      1UL

#define XC_FUNC_IDX_VACTTSK_1    0UL
#define XC_FUNC_IDX_VACTTSK_2    1UL
#define XC_FUNC_IDX_VCLRFLG_1    2UL
#define XC_FUNC_IDX_VPOLFLG_1    3UL
#define XC_FUNC_IDX_VPOLSEM_1    4UL
#define XC_FUNC_IDX_VRCVDTQ_1    5UL
#define XC_FUNC_IDX_VRELWAI_1    6UL
#define XC_FUNC_IDX_VRELWAI_2    7UL
#define XC_FUNC_IDX_VROTRDQ_1    8UL
#define XC_FUNC_IDX_VROTRDQ_2    9UL
#define XC_FUNC_IDX_VSETFLG_1    10UL
#define XC_FUNC_IDX_VSETFLG_2    11UL
#define XC_FUNC_IDX_VSIGSEM_1    12UL
#define XC_FUNC_IDX_VSIGSEM_2    13UL
#define XC_FUNC_IDX_SIGTIM       14UL
#define XC_FUNC_IDX_VSNDDTQ_1    15UL
#define XC_FUNC_IDX_VSNDDTQ_2    16UL
#define XC_FUNC_IDX_VSTATSK_1    17UL
#define XC_FUNC_IDX_VSTATSK_2    18UL
#define XC_FUNC_IDX_VWUPTSK_1    19UL
#define XC_FUNC_IDX_VWUPTSK_2    20UL

typedef struct t_cmdbuf {
    FN          func_n;
    ID          ctexid;
    UW          seq_no;
    XC_FUNC_IDX func;
    XPTR        ret_pc;
    XPARAM      para[7];
} T_CMDBUF;

typedef struct t_asynctbl {
    UW          lock;
    UW          cnt;
    UW          ptr;
    UW          dummy;
    T_CMDBUF    cmdbuf[1];
} T_ASYNCTBL;

#define DEF_ASYNCTBL(name, bufnum)      \
typedef struct {                        \
    UW          lock;                   \
    UW          cnt;                    \
    UW          ptr;                    \
    UW          dummy;                  \
    T_CMDBUF    cmdbuf[bufnum];         \
} name;

typedef struct t_synctbl {
    UW          lock;
    volatile UW fin;
    volatile ER error;
    XPARAM      ret_v;
    T_CMDBUF    cmdbuf;
} T_SYNCTBL;

typedef struct t_xcore_domain {
    UB          ready[TMAX_CORE];
    UINT        core_max;
    UINT        async_max;
    UINT        sync_core;
    UW          lock;
    UW          dummy;
    XPTR        asynctbl[TMAX_CORE];
    XPTR        synctbl[TMAX_CORE];
} T_XCORE_DOMAIN;

typedef struct t_xcore_ext {
    UB          ready[TMAX_DOMAIN];
    UINT        domain_max;
    union {
        UB      byte[TMAX_DOMAIN];
        UINT    word;
    } sync_domain;
    UW          lock;
    UW          sync_syscall_mode;
    T_XCORE_DOMAIN domain[TMAX_DOMAIN];
    UD          magic;
} T_XCORE_EXT;

typedef struct t_xcore_domain_cfg {
    UINT         core_max;
    UINT         async_max;
    ID           master_core;
    INTNO        sgi_intno;
    IMASK        sgi_ipl;
    FP           sgi_initfunc;
    INTNO        ipi_intno[TMAX_CORE];
    IMASK        ipi_ipl[TMAX_CORE];
    FP           ipi_initfunc[TMAX_CORE];
} T_XCORE_DOMAIN_CFG;

extern T_XCORE_EXT _kernel_xcore_ext;
extern ID get_domain_id(void);
extern ID get_hcid(void);
extern void _kernel_spin_lock(UW *flag);
extern void _kernel_spin_unlock(UW *flag);
extern ID _kernel_spin_lock_test(UW *flag);
extern ID _kernel_spin_locked_test(UW *flag);
extern void _kernel_xcore_enqssb(void);
extern void _kernel_async_sync_event_set(ID coreid);
extern UW _kernel_get_seqno(void);
extern ER _kernel_async_sys(FN func_n, ID coreid, UW seq_no, XPTR ret_pc, XC_FUNC_IDX func, XPTR *para);
extern BOOL _kernel_async_sys_1(T_SSB *par, BOOL retcd);
extern ER _kernel_sync_sys(FN func_n, ID coreid, UW seq_no, XPTR ret_pc, XC_FUNC_IDX func, XPTR *para);
extern BOOL _kernel_sync_sys_1(T_SSB *par, BOOL retcd);
extern void _kernel_systrace(T_CMDBUF *buffer, UW para_n);
extern void _kernel_rettrace(T_CMDBUF *buffer, ER ercd);
extern void _kernel_xerror_handler(T_CMDBUF *buffer, ER ercd);

#ifdef __cplusplus
}
#endif
#endif /* _UC3XCEXT_H_ */

