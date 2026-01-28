/**
 * @file    itron.h
 * @brief   Micro C Cube Standard, KERNEL
 *          AArch64 dependent ITRON macro definitions
 * @date    2025.04.18
 * @author  Copyright (c) 2016-2025, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2016.10.04) y-kim
 *            Created based on itron.h of ARMv7-A.
 *          - rev 1.1 (2017.04.25) yokota
 *            Changed TKERNEL_PRVER to v1.20.0
 *          - rev 1.2 (2017.06.28) y-kim
 *            Changed TKERNEL_PRVER to v1.20.2.
 *          - rev 1.3 (2018.02.01) yokota
 *            Changed TKERNEL_PRVER to v1.31.0.
 *          - rev 1.4 (2018.02.20) yokota
 *            Changed TKERNEL_PRVER to v1.32.0.
 *          - rev 1.5 (2018.03.27) yokota
 *            Changed TKERNEL_PRVER to v1.33.0.
 *          - rev 1.6 (2018.04.04) yokota
 *            Changed TKERNEL_PRVER to v1.33.2.
 *          - rev 1.7 (2018.09.05) yokota
 *            Changed TKERNEL_PRVER to v1.40.0.
 *          - rev 1.9 (2019.04.01) i-cho
 *            Changed TKERNEL_PRVER to v1.41.0.
 *          - rev 2.0 (2020.05.21) yokota
 *            Changed TKERNEL_PRVER to v1.41.2.
 *          - rev 2.1 (2021.02.03) Imada
 *            Added endian check macros for IAR compilers
 *            Fixed C++test warnings
 *          - rev 2.2 (2021.02.19) yokota
 *            Changed TKERNEL_PRVER to v1.42.0.
 *          - rev 2.3 (2021.05.13) yokota
 *            Changed TKERNEL_PRVER to v1.43.0.
 *          - rev 2.4 (2021.08.25) yokota
 *            Changed TKERNEL_PRVER to v1.43.2.
 *          - rev 2.5 (2022.02.10) yokota
 *            Changed TKERNEL_PRVER to v1.44.0
 *            Added _kernel_SIZE_MAX macro.
 *          - rev 2.6 (2024.03.21)
 *            Changed TKERNEL_PRVER to v1.45.0
 *          - rev 2.7 (2025.02.04)
 *            Changed TKERNEL_PRVER to v1.45.2
 *          - rev 2.8 (2025.04.18)
 *            Changed TKERNEL_PRVER to v1.45.4
 ****************************************************************************
 */
#ifndef _ITRON_H_
#define _ITRON_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL    (0)
#else
#define NULL    ((void *)0)
#endif
#endif

#define TRUE    1
#define FALSE   0

#define E_OK    0

#define ID_CORE0            (1U)
#define ID_CORE1            (2U)
#define ID_CORE2            (3U)
#define ID_CORE3            (4U)

#define _kernel_INT_SIZE    4U
#define _kernel_SIZE_SIZE   8U
#define _kernel_SIZE_MAX    (0xFFFFFFFFFFFFFFFFULL)
#define _kernel_ALIGN_SIZE  16U
#define _kernel_INT_BIT     (_kernel_INT_SIZE*8U)

#define TKERNEL_PRID    0x024C
#define TKERNEL_PRVER   0x1454

#define EXC_SYN     1U
#define EXC_SER     2U

/************************************
    Byte Order Type
 ************************************/

#if defined (__ARMCOMPILER_VERSION)     /* for DS-5 armclang Compiler */
#if defined (__ARM_BIG_ENDIAN)
#define _UC3_ENDIAN_BIG
#undef _UC3_ENDIAN_LITTLE
#else
#define _UC3_ENDIAN_LITTLE
#undef _UC3_ENDIAN_BIG
#endif
#elif defined(__GNUC__)                 /* for GNU C */
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define _UC3_ENDIAN_LITTLE
#undef _UC3_ENDIAN_BIG
#else
#define _UC3_ENDIAN_BIG
#undef _UC3_ENDIAN_LITTLE
#endif
#elif defined (__ICCARM__)  /* for IAR Compiler */
#if (__LITTLE_ENDIAN__ == 1)
#define _UC3_ENDIAN_LITTLE
#undef _UC3_ENDIAN_BIG
#else
#define _UC3_ENDIAN_BIG
#undef _UC3_ENDIAN_LITTLE
#endif
#else
#error "Unsupported Compiler."
#endif

/************************************
    Data Types
 ************************************/

typedef signed char B;
typedef signed short H;
typedef signed int W;
typedef unsigned char UB;
typedef unsigned short UH;
typedef unsigned int UW;
typedef char VB;
typedef short VH;
typedef int VW;
typedef void *VP;
typedef void (*FP)(void);

typedef signed long long D;
typedef unsigned long long UD;
typedef long long VD;

typedef unsigned long long SIZE;
typedef unsigned long long ADDR;

typedef int INT;
typedef unsigned int UINT;

typedef VP VP_INT;

typedef INT BOOL;
typedef INT FN;
typedef INT ER;
typedef INT ID;
typedef INT PRI;
typedef INT BOOL_ID;
typedef INT RDVNO;
typedef INT ER_ID;
typedef INT ER_UINT;
typedef UINT TEXPTN;
typedef UINT FLGPTN;
typedef UINT RDVPTN;
typedef UINT INHNO;
typedef UINT INTNO;
typedef UINT IMASK;
typedef UINT EXCNO;

typedef UINT ATR;
typedef UINT STAT;
typedef UINT MODE;

typedef struct t_systim {
    UW utime;
    UW ltime;
} SYSTIM;

typedef W TMO;

typedef UW RELTIM;
typedef UH OVRTIM;

/************************************
    AArch64 dependent definitions
 ************************************/

extern UW vget_fpscr(void);
extern void vset_fpscr(UW fpscr);
extern void vena_vfp(void);
extern void vdis_vfp(void);
extern void vdsb(void);
extern void vdmb(void);
extern void visb(void);
extern UINT vget_cid(void);

#define get_cid                 (vget_cid)
#define _kernel_synch_cache     (vdsb)

#ifdef __cplusplus
}
#endif

#define HOOK_V2

#endif
