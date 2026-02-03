/**
 * @file    uC3dsp.S
 * @brief   Micro C Cube Standard, KERNEL
 *          Scheduler and Dispatcher (AArch64)
 * @date    2025.04.16
 * @author  Copyright (c) 2016-2025, eForce Co., Ltd. All rights reserved.
 *
 ******************************************************************************
 * @par     History
 *          - rev 1.0 (2016.09.21) y-kim
 *            Initial version.
 *          - rev 1.1 (2017.06.28) y-kim
 *            Bugfixed for loc_cpu/unl_cpu/sns_loc.
 *          - rev 1.2 (2018.02.01) yokota
 *            insert isb after cpacr_el1 assignment.
 *          - rev 1.3 (2018.06.13) yokota
 *            fix trace hook routine.
 *          - rev 1.4 (2021.08.25)
 *            insert isb after cpacr_el1 assignment.
 *          - rev 1.5 (2021.09.09)
 *            insert isb after msr instructions.
 *          - rev 1.6 (2024.03.21)
 *            Fixed SP alignment of ISR to prevent unaligned issue
 *            which is rare-case that triggered by multiple interrupts.
 *          - rev 1.7 (2025.02.04)
 *            Since register saving during an IRQ exception is handled by
 *            the sample vector, register saving in _kernel_inthdr is omitted.(#4103).
 *          - rev 1.8 (2025.04.16)
 *            Update vget_cid for Armv8.2-A.
 ******************************************************************************
 */

/* structure position of context member */

#define SYS_DSP             (0x0)
#define SYS_IMS             (0x1)
#define SYS_DLOCK           (0x2)
#define SYS_CIMS            (0x3)
#define SYS_IIMS            (0x4)
#define SYS_ICNT            (0x4+1)
#define SYS_RDSP            (0x4+2)
#define SYS_TID             (0x18)
#define SYS_PRI             (0x18+2)
#define SYS_SSB_CNT         (0x1C)
#define SYS_SSB_MIN         (0x1C+2)
#define SYS_SSB_TOP         (0x28)
#define SYS_SSB_BTM         (0x30)
#define SYS_CSSB            (0x48)
#define SYS_CTCB            (0x50)
#define SYS_RDQ             (0x58)
#define SYS_SSB_FREE        (0x60)

#define SYS_SYSIDL          (0x100)
#define SYS_CTXTRACE0       (0x110)
#define SYS_CTXTRACE1       (0x118)
#define SYS_CTXTRACE2       (0x120)
#define SYS_CTXTRACE3       (0x128)
#define SYS_SYSTRACE        (0x130)
#define SYS_RETTRACE        (0x138)
#define SYS_AGENTMNG        (0x140)
#define SYS_CTXID_C         (0x148)
#define SYS_CTXID_S         (0x14C)
#define SYS_AGENTTIM        (0x150)
#define SYS_ATRHDL          (0x178)
#define SYS_ERRHDL          (0x180)
#define SYS_CTXTRACE4       (0x188)
#define SYS_SYNHDR          (0x190)
#define SYS_SERHDR          (0x198)
#define SYS_FPSCR           (0x1A0)

#define TCB_MNEXT           (0x0)
#define TCB_MPREV           (0x8)
#define TCB_SNEXT           (0x10)
#define TCB_SPREV           (0x18)
#define TCB_STAT            (0x20)
#define TCB_STAT_CATR       (0x20+0)
#define TCB_STAT_OATR       (0x20+1)
#define TCB_STAT_MSTS       (0x20+2)
#define TCB_STAT_WSTS       (0x20+3)
#define TCB_TID             (0x3A)
#define TCB_PRI             (0x44)
#define TCB_SP              (0x50)
#define TCB_P1              (0x80)
#define TCB_PC              (0x90)

#define SSB_P1              (0x0)
#define SSB_P2              (0x8)
#define SSB_SNEXT           (0x10)
#define SSB_SPREV           (0x18)
#define SSB_STAT            (0x20)
#define SSB_STAT_CATR       (0x20+0)
#define SSB_STAT_OATR       (0x20+1)
#define SSB_STAT_MSTS       (0x20+2)
#define SSB_STAT_WSTS       (0x20+3)
#define SSB_FUNC            (0x28)
#define SSB_P3              (0x30)
#define SSB_P4              (0x38)
#define SSB_P5              (0x40)
#define SSB_P6              (0x48)

/* atribute of stat.catr */

#define CTA_SYS             0x00
#define CTA_INT             0x01
#define CTA_STA             0x02
#define CTA_IDL             0x10
#define CTA_SSB             0x20
#define CTA_CYC             0x40
#define CTA_ALM             0x80

#define CTA_INT_OFFSET      0x00
#define CTA_STA_OFFSET      0x01

/* atribute of stat.oatr */

#define OTA_FPU             (0x40 << 8)
#define OTA_FPU_OFFSET      (14)

/* DAIF bit definition of PSTATE */

#define I_BIT               0x2
#define F_BIT               0x1

#define I_BIT_OFFSET        0x1
#define F_BIT_OFFSET        0x0

#define DAIF_OFFSET         0x6

/* definition of CORE ID */

#define CORE0               1
#define CORE1               2
#define CORE2               3
#define CORE3               4

/* definition of CPACR_EL1 (Architectural Feature Access Control Register) */

#define FPEN_OFFSET         20
#define FPEN_BIT            (3 << FPEN_OFFSET)

/* external declarations */

        .globl  _kernel_systbl
        .globl  _kernel_micro_systim
        .globl  _kernel_error_handler

/************************************************
 * Transition to a system level from the task   *
 ************************************************/

/*
 * void _kernel_relrun(void)
 */
        .text
        .string "eForce co.,ltd."
        .balign 4
        .globl  _kernel_relrun
        .type   _kernel_relrun, @function
_kernel_relrun:
/*
        mrs     r0,  cpsr
        tst     r0,  #0x0F
        swieq   0
        mrs     r1,  cpsr
        ldr     r3,  systbl
        orr     r1,  r1,  #I_BIT
        mov     r2,  r0
        msr     cpsr_c,   r1

        tst     r0,  #I_BIT
        bne     relrun_1
        ldrb    r0,  [r3, #SYS_DSP]
        ldrb    r12, [r3, #SYS_RDSP]
        teq     r0,  #0
        bne     relrun_1
        teq     r12, #0
        beq     relrun_1

        mov     r0,  #0
        mov     r12, #0
        cpsie   i
        b       entsys_0
*/
        mrs     x2,  daif
        ldr     x3,  systbl
        msr     daifset, #I_BIT
        isb

        tbnz    x2,  #(I_BIT_OFFSET + DAIF_OFFSET), relrun_1
        ldrb    w0,  [x3, #SYS_DSP]
        ldrb    w12, [x3, #SYS_RDSP]
        cbnz    x0,  relrun_1
        cbz     x12, relrun_1

        mov     x12, xzr
        msr     daifclr, #I_BIT
        isb
        b       entsys_0

relrun_1:
/*
        mov     r0,  #0
        strb    r0,  [r3, #SYS_DLOCK]
        msr     cpsr_c,   r2
        bx      lr
*/
        strb    wzr, [x3, #SYS_DLOCK]
        msr     daif, x2
        isb
        ret


/*
 * _kernel_entsys
 * void _kernel_entsys(BOOL)
 */
        .balign 4
        .globl  _kernel_entsys
        .type   _kernel_entsys, @function
_kernel_entsys:
/*
        mov     r1,  r0
        mrs     r0,  cpsr
        tst     r0,  #0x0F
        ldr     r3,  systbl
        swieq   0
        mov     r2,  r0
        mov     r12, #1
        mov     r0,  r1
*/
        ldr     x3,  systbl
        mov     x12, #1
        mrs     x2,  daif

/*
 * x0 ... Dispatch Lock Flag
 * x12 .. System Shift Flag
 * x2 ... cpsr
 * x3 ... &_kernel_systbl
 * lr ... return address
 */
entsys_0:
/*
        tst     lr,  #0x00000001
        orrne   r2,  r2,  #T_BIT
        stmfd   sp!, {r2,lr}
 IF {FPU} = "VFPv3" :LOR: {FPU} = "VFPv3_D16"
        mov     lr,  r2
;    Save Permanent FPU registers
        fmrx    r1,  fpexc
        tst     r1,  #0x40000000
 IF :LNOT: ({FPU} = "VFPv3_D16")
        fstmfddne sp!, {d16-d31}
 ENDIF
        fstmfddne sp!, {d8-d15}
        fmrxne  r2,  fpscr
        stmfd   sp!, {r1,r2}
        bicne   r1,  r1,  #0x40000000
        fmxrne  fpexc, r1
        mov     r2,  lr
 ENDIF
        mrs     r1,  cpsr
        stmfd   sp!, {r4-r11}
        orr     r4,  r1,  #I_BIT
        and     r2,  r2,  #(I_BIT:OR:F_BIT)
        msr     cpsr_c,   r4
        strb    r0,  [r3, #SYS_DLOCK]
        ldr     r0,  [r3, #SYS_CTCB]
        strb    r2,  [r3, #SYS_IMS]
        str     sp,  [r0, #TCB_SP]
        ldr     sp,  =|Image$$STACK$$ZI$$Limit|
        mov     r2,  #CTA_SYS
        strb    r2,  [r0, #TCB_STAT_CATR]
*/
        stp     x2,  x30, [sp, #-16]!
#ifdef _KERNEL_FPU_VFP
        mrs     x1,  cpacr_el1
        and     x4,  x1,  #FPEN_BIT
        cmp     x4,  #FPEN_BIT
        b.ne    entsys_0_1

        /* store non-temporary registers (q8-q15) */
        stp     q14, q15, [sp, #-32]!
        stp     q12, q13, [sp, #-32]!
        stp     q10, q11, [sp, #-32]!
        stp     q8,  q9,  [sp, #-32]!
        mrs     x4,  fpsr
        mrs     x5,  fpcr
        orr     x4,  x4,  x5
        bic     x5,  x1,  #FPEN_BIT
        msr     cpacr_el1, x5
        isb
entsys_0_1:
        stp     x1,  x4, [sp, #-16]!
#endif /* #ifdef _KERNEL_FPU_VFP */

        /* store non-temporary registers (x19-x29) */
        stp     x29, x30, [sp, #-16]!
        stp     x27, x28, [sp, #-16]!
        stp     x25, x26, [sp, #-16]!
        stp     x23, x24, [sp, #-16]!
        stp     x21, x22, [sp, #-16]!
        stp     x19, x20, [sp, #-16]!

        mov     x1,  x2
        and     x2,  x2,  #(I_BIT << DAIF_OFFSET)
        msr     daifset,  #I_BIT
        isb
        strb    w0,  [x3, #SYS_DLOCK]
        ldr     x0,  [x3, #SYS_CTCB]
        strb    w2,  [x3, #SYS_IMS]
        mov     x2,  sp
        str     x2,  [x0, #TCB_SP]
        ldr     x2,  =_stack
        mov     sp,  x2
        mov     x2,  #CTA_SYS
        strb    w2,  [x0, #TCB_STAT_CATR]

/*
 * x0 ... TCB
 * x1 ... cspr
 * x3 ... _kernel_systbl
 * x12 .. System Shift Flag
 */
/*
        ldr     r2,  [r3, #SYS_DSP]
        teq     r2,  #0
        bne     entsys_1
        ldrb    r2,  [r3, #SYS_RDSP]
        teq     r2,  #0
        movne   r2,  #0
        strneb  r2,  [r3, #SYS_RDSP]
        strne   r2,  [r3, #SYS_CTCB]
entsys_1
        teq     r12, #0
        beq     entsys_3
        ldr     r2,  [r3, #SYS_CSSB]
        teq     r2,  #0
        beq     entsys_2
        ldr     lr, =dispatch
        b       enqssb_0
entsys_2
        str     r0,  [r3, #SYS_CSSB]
entsys_3
        msr     cpsr_c,   r1
        b       dispatch
*/
        ldr     w2,  [x3, #SYS_DSP]
        cbnz    w2,  entsys_1
        ldrb    w2,  [x3, #SYS_RDSP]
        cbz     w2,  entsys_1
        strb    wzr, [x3, #SYS_RDSP]
        str     xzr, [x3, #SYS_CTCB]
entsys_1:
        cbz     x12, entsys_3
        ldr     x2,  [x3, #SYS_CSSB]
        cbz     x2,  entsys_2
        ldr     x30, =dispatch
        b       enqssb_0
entsys_2:
        str     x0,  [x3, #SYS_CSSB]
entsys_3:
        msr     daif,   x1
        isb
        b       dispatch

/************************************************
 * Queuing of SSB                               *
 ************************************************/

/*
 * void _kernel_enqssb(T_SSB *ssb)
 */
        .globl  _kernel_enqssb
        .type   _kernel_enqssb, @function
_kernel_enqssb:
/*
        mrs     r1,  cpsr
        orr     r2,  r1,  #I_BIT
        ldr     r3,  systbl
        msr     cpsr_c,   r2
enqssb_0
        ldr     r2,  [r3, #SYS_SSB_TOP]
        teq     r2,  #0
        mov     r2,  #0
        str     r2,  [r0, #SSB_SNEXT]
        streq   r0,  [r3, #SYS_SSB_TOP]
        ldrne   r2,  [r3, #SYS_SSB_BTM]
        strne   r0,  [r2, #SSB_SNEXT]
        str     r0,  [r3, #SYS_SSB_BTM]
        msr     cpsr_c,   r1
        bx      lr
*/
        mrs     x1,  daif
        ldr     x3,  systbl
        msr     daifset, #I_BIT
        isb
enqssb_0:
        ldr     x2,  [x3, #SYS_SSB_TOP]
        str     xzr, [x0, #SSB_SNEXT]
        cbnz    x2,  enqssb_nz
        str     x0,  [x3, #SYS_SSB_TOP]
        b       enqssb_end
enqssb_nz:
        ldr     x2,  [x3, #SYS_SSB_BTM]
        str     x0,  [x2, #SSB_SNEXT]
enqssb_end:
        str     x0,  [x3, #SYS_SSB_BTM]
        msr     daif, x1
        isb
        ret

/************************************************
 * Dispatch and task scheduling                 *
 ************************************************/

/*
 * void _kernel_dispatch(void)
 */
        .globl  _kernel_dispatch
        .type   _kernel_dispatch, @function
_kernel_dispatch:
/*
        ldr     sp,  =|Image$$STACK$$ZI$$Limit|
        cpsie   if,  #SYS_MOD
*/
        ldr     x4,  =_stack
        mov     sp,  x4
        msr     daifclr, #I_BIT
        isb

dispatch:
/*
        ldr     r4,  systbl

        ldr     r0,  [r4, #SYS_CSSB]
        teq     r0,  #0
        bne     dispatch_1
dispatch_0
        bl      deqssb
        teq     r0,  #0
        beq     dispatch_4
dispatch_1
        clrex
        mov     r3,  r0
        ldr     r2,  [r3, #SSB_FUNC]
        ldr     r1,  [r3, #SSB_STAT_CATR]
        tst     r1,  #(CTA_SSB:OR:CTA_CYC:OR:CTA_ALM)
        addeq   r0,  r3,  #TCB_P1
        and     r1,  r1,  #CTA_SSB
        blx     r2

        teq     r0,  #0
        beq     dispatch_3
        ldr     r5,  [r4, #SYS_CSSB]
        ldr     r1,  [r5, #SSB_P1]
        cmp     r1,  #0
        ldrne   r0,  [r5, #SSB_P3]
        ldrne   r3,  [r4, #SYS_ERRHDL]
        addne   r2,  r5,  #SSB_P4
        cmpne   r3,  #0
        blne    _kernel_error_handler
        bl      relssb
dispatch_3
        mov     r0,  #0
        str     r0,  [r4, #SYS_CSSB]
        b       dispatch_0
*/
        ldr     x19, systbl

        ldr     x0,  [x19, #SYS_CSSB]
        cbnz    x0,  dispatch_1
dispatch_0:
        bl      deqssb
        cbz     x0,  dispatch_4
dispatch_1:
        mov     x3,  x0
        ldr     x2,  [x3, #SSB_FUNC]
        ldrb    w1,  [x3, #SSB_STAT_CATR]
        and     w1,  w1, #(CTA_SSB | CTA_CYC | CTA_ALM)
        cbnz    w1,  dispatch_1_1
        add     x0,  x3,  #TCB_P1
dispatch_1_1:
        and     w1,  w1,  #CTA_SSB
        blr     x2

        cbz     x0,  dispatch_3
        ldr     x20, [x19, #SYS_CSSB]
        ldr     x1,  [x20, #SSB_P1]
        cbz     x1,  dispatch_1_2
        ldr     x3,  [x19, #SYS_ERRHDL]
        cbz     x3,  dispatch_1_2
        ldr     x0,  [x20, #SSB_P3]
        add     x2,  x20,  #SSB_P4
        bl      _kernel_error_handler
dispatch_1_2:
        bl      relssb
dispatch_3:
        str     xzr, [x19, #SYS_CSSB]
        b       dispatch_0

dispatch_4:
/*
        ldr     r0,  [r4, #SYS_CTCB]
        clrex
        teq     r0,  #0
        bne     dispatch_5

        bl      gettcb
        teq     r0,  #0
        beq     dispatch_idle
*/
        ldr     x0,  [x19, #SYS_CTCB]
        cbnz    x0, dispatch_5
        bl      gettcb
        cbz     x0, dispatch_idle

dispatch_5:
/*
        ldr     sp,  [r0, #TCB_SP]
        ldrb    r1,  [r0, #TCB_STAT_CATR]
        tst     r1,  #CTA_STA
        bne     dispatch_7
        tst     r1,  #CTA_INT
        bne     dispatch_6
*/
        ldr     x1,  [x0, #TCB_SP]
        mov     sp,  x1
        ldrb    w1,  [x0, #TCB_STAT_CATR]
        tbnz    w1,  #CTA_STA_OFFSET,  dispatch_7
        tbnz    w1,  #CTA_INT_OFFSET,  dispatch_6

/*
        mov     r3,  r4
                                    ; r0 .... TCB
                                    ; r3 .... &_kernel_systbl
        ldmfd   sp!, {r4-r11}
 IF {FPU} = "VFPv3" :LOR: {FPU} = "VFPv3_D16"
;    Restore Permanent FPU registers
        ldmfd   sp!, {r1,r2}
        tst     r1,  #0x40000000
        fmxrne  fpexc, r1
        fmxrne  fpscr, r2
        fldmfddne sp!, {d8-d15}
 IF :LNOT: ({FPU} = "VFPv3_D16")
        fldmfddne sp!, {d16-d31}
 ENDIF
 ENDIF
        cpsid   i
        ldrh    r1,  [r0, #TCB_PRI]
        ldrh    r2,  [r0, #TCB_TID]
        str     r0,  [r3, #SYS_CTCB]
        strh    r1,  [r3, #SYS_PRI]
        mov     r1,  #0
        strh    r2,  [r3, #SYS_TID]
        str     r1,  [r0, #TCB_SP]
        ldr     r1,  [r3, #SYS_CTXID_C]
        str     r2,  [r3, #SYS_CTXID_S]
        str     r2,  [r3, #SYS_CTXID_C]
*/
        mov     x3,  x19

        /* restore non-temporary registers (x19-x29) */
        ldp     x19, x20, [sp], #16
        ldp     x21, x22, [sp], #16
        ldp     x23, x24, [sp], #16
        ldp     x25, x26, [sp], #16
        ldp     x27, x28, [sp], #16
        ldp     x29, x30, [sp], #16
#ifdef _KERNEL_FPU_VFP
        ldp     x1,  x2,  [sp],  #16
        and     x12, x1,  #FPEN_BIT
        cmp     x12, #FPEN_BIT
        b.ne    dispatch_5_1

        msr     cpacr_el1, x1
        isb
        msr     fpsr, x2
        msr     fpcr, x2
        isb

        /* restore non-temporary registers (q8-q15) */
        ldp     q8,  q9,  [sp],  #32
        ldp     q10, q11, [sp],  #32
        ldp     q12, q13, [sp],  #32
        ldp     q14, q15, [sp],  #32
dispatch_5_1:
#endif /* #ifdef _KERNEL_FPU_VFP */
        msr     daifset,    #I_BIT
        isb
        ldrh    w1,  [x0, #TCB_PRI]
        strh    w1,  [x3, #SYS_PRI]
        ldrh    w2,  [x0, #TCB_TID]
        strh    w2,  [x3, #SYS_TID]
        str     x0,  [x3, #SYS_CTCB]
        str     xzr, [x0, #TCB_SP]
        ldr     w1,  [x3, #SYS_CTXID_C]
        str     w2,  [x3, #SYS_CTXID_S]
        str     w2,  [x3, #SYS_CTXID_C]

/*
 * Hook routine is here
 *           x0 ... TCB
 *           x1 ... Contex ID
 *           x2 ... Task ID (New)
 *           x3 ... &_kernel_systbl
 */
/*
        cmp     r1,  r2
        beq     dispatch_sysret
        ldr     r12, [r3, #SYS_CTXTRACE2]
        teq     r12, #0
        beq     dispatch_sysret

        ldr     r3,  [r0, #TCB_PC]
        stmfd   sp!, {r1-r3,r12}
        bl      _kernel_micro_systim
        mov     r2,  r0
        ldmfd   sp!, {r0-r1,r3,r12}
        blx     r12
*/
        cmp     w1,  w2
        b.eq    dispatch_sysret
        ldr     x12, [x3, #SYS_CTXTRACE2]
        cbz     x12, dispatch_sysret

        ldr     x3,  [x0, #TCB_PC]
        stp     x3,  x12, [sp, #-16]!
        stp     x1,  x2,  [sp, #-16]!
        bl      _kernel_micro_systim
        mov     x2,  x0
        ldp     x0,  x1,  [sp],  #16
        ldp     x3,  x12, [sp],  #16
        blr     x12

dispatch_sysret:
/*
        ldmfd   sp!, {r2-r3}
        msr     cpsr_c,   r2
        bx      r3
*/
        ldp     x2,  x3, [sp],  #16
        msr     daif,   x2
        isb
        br      x3

dispatch_6:
/*
        mov     r3,  r4
                                    ; r0 .... TCB
                                    ; r3 .... &_kernel_systbl
        ldmfd   sp!, {r4-r11}
 IF {FPU} = "VFPv3" :LOR: {FPU} = "VFPv3_D16"
;    Restore Permanent FPU registers
        ldmfd   sp!, {r1,r2}
        tst     r1,  #0x40000000
        fmxrne  fpexc, r1
        fmxrne  fpscr, r2
        fldmfddne sp!, {d0-d7}
 ENDIF
        cpsid   i
        ldrh    r1,  [r0, #TCB_PRI]
        ldrh    r2,  [r0, #TCB_TID]
        str     r0,  [r3, #SYS_CTCB]
        strh    r1,  [r3, #SYS_PRI]
        mov     r1,  #0
        strh    r2,  [r3, #SYS_TID]
        str     r1,  [r0, #TCB_SP]
        ldr     r1,  [r3, #SYS_CTXID_C]
        str     r2,  [r3, #SYS_CTXID_S]
        str     r2,  [r3, #SYS_CTXID_C]
*/
        mov     x3,  x19

        /* restore non-temporary registers (x19-x29) */
        ldp     x19,  x20,  [sp],  #16
        ldp     x21,  x22,  [sp],  #16
        ldp     x23,  x24,  [sp],  #16
        ldp     x25,  x26,  [sp],  #16
        ldp     x27,  x28,  [sp],  #16
        ldp     x29,  x30,  [sp],  #16
#ifdef _KERNEL_FPU_VFP
        ldp     x1,  x2,  [sp],  #16
        and     x12, x1,  #FPEN_BIT
        cmp     x12, #FPEN_BIT
        b.ne    dispatch_6_1

        msr     cpacr_el1, x1
        isb
        msr     fpsr, x2
        msr     fpcr, x2
        isb

        /* restore non-temporary registers (q8-q15) */
        ldp     q8,  q9,  [sp], #32
        ldp     q10, q11, [sp], #32
        ldp     q12, q13, [sp], #32
        ldp     q14, q15, [sp], #32
dispatch_6_1:
#endif /* #ifdef _KERNEL_FPU_VFP */
        msr     daifset,    #I_BIT
        isb
        ldrh    w1,  [x0, #TCB_PRI]
        strh    w1,  [x3, #SYS_PRI]
        ldrh    w2,  [x0, #TCB_TID]
        strh    w2,  [x3, #SYS_TID]
        str     x0,  [x3, #SYS_CTCB]
        str     xzr, [x0, #TCB_SP]
        ldr     w1,  [x3, #SYS_CTXID_C]
        str     w2,  [x3, #SYS_CTXID_S]
        str     w2,  [x3, #SYS_CTXID_C]

/*
 * Hook routine is here
 *           x0 ... TCB
 *           x1 ... Contex ID
 *           x2 ... Task ID (New)
 *           x3 ... &_kernel_systbl
 */
/*
        cmp     r1,  r2
        beq     dispatch_intret
        ldr     r12, [r3, #SYS_CTXTRACE0]
        teq     r12, #0
        beq     dispatch_intret

        ldr     r3,  [sp, #36]
        stmfd   sp!, {r1-r3,r12}
        bl      _kernel_micro_systim
        mov     r2,  r0
        ldmfd   sp!, {r0-r1,r3,r12}
        blx     r12
*/
        cmp     w1,  w2
        b.eq    dispatch_intret
        ldr     x12, [x3, #SYS_CTXTRACE0]
        cbz     x12, dispatch_intret

#ifdef _KERNEL_FPU_VFP
        mrs     x1,  cpacr_el1
        and     x0,  x1,  #FPEN_BIT
        cmp     x0,  #FPEN_BIT
        b.ne    dispatch_6_2
        ldr     x3,  [sp, #((16*24)+8)]
        b       dispatch_6_3
dispatch_6_2:
#endif
        ldr     x3,  [sp, #8]
dispatch_6_3:
        stp     x3,  x12, [sp, #-16]!
        stp     x1,  x2,  [sp, #-16]!
        bl      _kernel_micro_systim
        mov     x2,  x0
        ldp     x0,  x1,  [sp],  #16
        ldp     x3,  x12, [sp],  #16
        blr     x12

dispatch_intret:
/*
        mov     r12, sp
        cps     #IRQ_MOD
 IF {FPU} = "VFPv3" :LOR: {FPU} = "VFPv3_D16"
;    Restore Temporary FPU registers (r12!)
        fmrx    r1,  fpexc
        tst     r1,  #0x40000000
        fldmfddne r12!,{d8-d15}
 IF :LNOT: ({FPU} = "VFPv3_D16")
        fldmfddne r12!,{d16-d31}
 ENDIF
 ENDIF
        ldmfd   r12!,{r0-r3}
        ldmfd   r12, {sp,lr}^
        nop
        add     r12, r12, #3*4
        ldmfd   r12!,{lr}
        msr     spsr_csxf,lr
        ldmfd   r12, {r12,pc}^
*/
#ifdef _KERNEL_FPU_VFP
        mrs     x1,  cpacr_el1
        and     x0,  x1,  #FPEN_BIT
        cmp     x0,  #FPEN_BIT
        b.ne    dispatch_intret_1

        /* restore temporary registers (q0-q7, q16-q31) */
        ldp     q0,  q1,  [sp], #32
        ldp     q2,  q3,  [sp], #32
        ldp     q4,  q5,  [sp], #32
        ldp     q6,  q7,  [sp], #32
        ldp     q16, q17, [sp], #32
        ldp     q18, q19, [sp], #32
        ldp     q20, q21, [sp], #32
        ldp     q22, q23, [sp], #32
        ldp     q24, q25, [sp], #32
        ldp     q26, q27, [sp], #32
        ldp     q28, q29, [sp], #32
        ldp     q30, q31, [sp], #32
dispatch_intret_1:
#endif /* #ifdef _KERNEL_FPU_VFP */
        ldp     x0,  x1,  [sp], #16
        msr     spsr_el1, x0
        msr     elr_el1,  x1
        isb

        /* restore temporary registers (x0-x18, x30) */
        ldp     x0,  x1,  [sp], #16
        ldp     x2,  x3,  [sp], #16
        ldp     x4,  x5,  [sp], #16
        ldp     x6,  x7,  [sp], #16
        ldp     x8,  x9,  [sp], #16
        ldp     x10, x11, [sp], #16
        ldp     x12, x13, [sp], #16
        ldp     x14, x15, [sp], #16
        ldp     x16, x17, [sp], #16
        ldp     x18, x30, [sp], #16
        eret

dispatch_7:
/*
        mov     r3,  #0
        cpsid   i
        str     r3,  [r0, #TCB_SP]
        ldrh    r1,  [r0, #TCB_PRI]
        ldrh    r2,  [r0, #TCB_TID]
        str     r0,  [r4, #SYS_CTCB]
        strh    r1,  [r4, #SYS_PRI]
        mov     r1,  #0
        strh    r2,  [r4, #SYS_TID]
        str     r1,  [r0, #TCB_SP]
        ldr     r1,  [r4, #SYS_CTXID_C]
        str     r2,  [r4, #SYS_CTXID_S]
        str     r2,  [r4, #SYS_CTXID_C]
*/
        msr     daifset, #I_BIT
        isb
        ldrh    w1,  [x0, #TCB_PRI]
        strh    w1,  [x19, #SYS_PRI]
        ldrh    w2,  [x0, #TCB_TID]
        strh    w2,  [x19, #SYS_TID]
        str     x0,  [x19, #SYS_CTCB]
        str     xzr, [x0, #TCB_SP]
        ldr     w1,  [x19, #SYS_CTXID_C]
        str     w2,  [x19, #SYS_CTXID_S]
        str     w2,  [x19, #SYS_CTXID_C]

/*
 * Hook routine is here
 *           x0 ... TCB
 *           x1 ... Contex ID
 *           x2 ... Task ID (New)
 *           x19 .. &_kernel_systbl
 */
/*
        cmp     r1,  r2
        beq     dispatch_inista
        ldr     r12, [r4, #SYS_CTXTRACE1]
        teq     r12, #0
        beq     dispatch_inista

 IF {FPU} = "VFPv3" :LOR: {FPU} = "VFPv3_D16"
        ldr     r3,  [sp, #5*4]
 ELSE
        ldr     r3,  [sp, #3*4]
 ENDIF
        stmfd   sp!, {r1-r3,r12}
        bl      _kernel_micro_systim
        mov     r2,  r0
        ldmfd   sp!, {r0-r1,r3,r12}
        blx     r12
*/
        cmp     w1,  w2
        b.eq    dispatch_inista
        ldr     x12, [x19, #SYS_CTXTRACE1]
        cbz     x12, dispatch_inista

#ifdef _KERNEL_FPU_VFP
        ldr     x3,  [sp, #5*8]
#else /* #ifdef _KERNEL_FPU_VFP */
        ldr     x3,  [sp, #3*8]
#endif /* #ifdef _KERNEL_FPU_VFP */
        stp     x3,  x12, [sp, #-16]!
        stp     x1,  x2,  [sp, #-16]!
        bl      _kernel_micro_systim
        mov     x2,  x0
        ldp     x0,  x1,  [sp],  #16
        ldp     x3,  x12, [sp],  #16
        blr     x12

dispatch_inista:
/*
 IF {FPU} = "VFPv3" :LOR: {FPU} = "VFPv3_D16"
;    Restore System FPU registers
        ldmfd   sp!, {r1,r2}
        tst     r1,  #0x40000000
        fmxr    fpexc, r1
        fmxrne  fpscr, r2
 ENDIF
        ldmfd   sp!, {r1,lr}
        ldmfd   sp!, {r0,r2}
        msr     cpsr_c,   r1
        bx      r2
*/
#ifdef _KERNEL_FPU_VFP
        ldp     x0,  x1,  [sp], #16
        msr     cpacr_el1, x0
        isb
        and     x2,  x0,  #FPEN_BIT
        cmp     x2,  #FPEN_BIT
        b.ne    dispatch_inista_1

        msr     fpsr, x1
        msr     fpcr, x1
        isb
dispatch_inista_1:
#endif /* #ifdef _KERNEL_FPU_VFP */
        ldp     x1,  x30, [sp], #16
        ldp     x0,  x2,  [sp], #16
        msr     daifclr, #I_BIT
        isb
        br      x2

dispatch_idle:
/*
        mrs     r5,  cpsr
        orr     lr,  r5,  #I_BIT
        msr     cpsr_c,   lr
                                    ; r1 .... MAX priority
                                    ; r4 .... &_kernel_systbl
        mov     r3,  #0
        mov     r2,  #CTA_IDL
        str     r3,  [r4, #SYS_CTCB]
        add     r1,  r1,  #1
        mov     r2,  r2,  LSL #16
        strh    r3,  [r4, #SYS_TID]
        strh    r1,  [r4, #SYS_PRI]
        ldr     r1,  [r4, #SYS_CTXID_C]
        str     r2,  [r4, #SYS_CTXID_S]
        str     r2,  [r4, #SYS_CTXID_C]
*/
        mrs     x5,  daif
        msr     daifset, #I_BIT
        isb
                                    /* x1 .... MAX priority    */
                                    /* x19 ... &_kernel_systbl */

        str     xzr, [x19, #SYS_CTCB]
        strh    wzr, [x19, #SYS_TID]
        add     x1,  x1,  #1
        strh    w1,  [x19, #SYS_PRI]
        ldr     w1,  [x19, #SYS_CTXID_C]
        mov     x2,  #(CTA_IDL << 16)
        str     w2,  [x19, #SYS_CTXID_S]
        str     w2,  [x19, #SYS_CTXID_C]

/*
 * Hook routine is here
 *           x19 ... &_kernel_systbl
 */
/*
        cmp     r1,  r2
        beq     dispatch_idle_2
        ldr     r12, [r4, #SYS_CTXTRACE3]
        teq     r12, #0
        beq     dispatch_idle_2

        ldr     r3,  [r4, #SYS_SYSIDL]
        stmfd   sp!, {r1-r3,r12}
        bl      _kernel_micro_systim
        mov     r2,  r0
        ldmfd   sp!, {r0-r1,r3,r12}
        teq     r3,  #0
        bne     dispatch_idle_1
        add     r3,  pc,  #6*4
dispatch_idle_1
        blx     r12
*/
        cmp     w1,  w2
        b.eq    dispatch_idle_2
        ldr     x12, [x19, #SYS_CTXTRACE3]
        cbz     x12, dispatch_idle_2

        ldr     x3,  [x19, #SYS_SYSIDL]
        stp     x3,  x12, [sp, #-16]!
        stp     x1,  x2,  [sp, #-16]!
        bl      _kernel_micro_systim
        mov     x2,  x0
        ldp     x0,  x1,  [sp],  #16
        ldp     x3,  x12, [sp],  #16
        cbnz    x3,  dispatch_idle_1
        ldr     x3,  =dispatch_idle_4
dispatch_idle_1:
        blr     x12

dispatch_idle_2:
/*
        msr     cpsr_c,   r5

dispatch_idle_3
        ldr     r0,  [r4, #SYS_SYSIDL]
        teq     r0,  #0
        beq     dispatch_idle_4
        blx     r0
        b       dispatch_idle_3
dispatch_idle_4
        b       dispatch_idle_4
*/
        msr     daif,   x5
        isb
dispatch_idle_3:
        ldr     x0,  [x19, #SYS_SYSIDL]
        cbz     x0,  dispatch_idle_4
        blr     x0
        b       dispatch_idle_3
dispatch_idle_4:
        b       dispatch_idle_4

/************************************************
 * Entry point of the interrupt                 *
 ************************************************/

        .globl  _kernel_pre_inthdr
        .globl  _kernel_post_inthdr

        .globl  _kernel_entint
        .type   _kernel_entint, @function
_kernel_entint:
/*
        ldr     r3,  systbl
        clrex
        ldrb    r0,  [r3, #SYS_ICNT]
        teq     r0,  #0
        add     r1,  r0,  #1
        ldr     r0,  [r3, #SYS_CSSB]
        strb    r1,  [r3, #SYS_ICNT]
        add     r2,  sp,  #6*4
        bne     entint_1
        teq     r0,  #0
        bne     entint_1

        cps     #SYS_MOD
        ldr     sp,  =|Image$$STACK$$ZI$$Limit|
        cps     #IRQ_MOD
*/
        ldr     x3,  systbl
        ldrb    w0,  [x3, #SYS_ICNT]
        add     w1,  w0,  #1
        strb    w1,  [x3, #SYS_ICNT]
        cbnz    w0,  entint_1
        ldr     x0,  [x3, #SYS_CSSB]
        cbnz    x0,  entint_1

        ldr     x0, =_stack
        msr     sp_el0, x0
        isb

entint_1:
/*
 IF {FPU} = "VFPv3" :LOR: {FPU} = "VFPv3_D16"
;    Save Temporary FPU registers at IRQ Stack
        fmrx    r0,  fpexc
        tst     r0,  #0x40000000
        fstmfddne sp!, {d0-d7}
        fmrxne  r1,  fpscr
        stmfd   sp!, {r0,r1}
        bicne   r0,  r0,  #0x40000000
        fmxrne  fpexc, r0
 ENDIF
;
        ldr     r0,  [r3, #SYS_CTXID_C]
        ldr     r1,  [r3, #SYS_CTXID_S]
        stmfd   sp!, {r0-r1}
;
        ldr     r1,  [r2]
 IF {FPU} = "VFPv3" :LOR: {FPU} = "VFPv3_D16"
        fmrx    r0,  fpexc
        ldr     r3,  [r3, #SYS_FPSCR]
        stmfd   sp!, {r0,r3}
 ENDIF

        mov     r0,  r2
        ldmfd   r1,  {r1-r3}
        stmfd   sp!, {r0-r3}
        mov     r0,  sp
*/
        mov     x0,  sp
#ifdef _KERNEL_FPU_VFP
        mrs     x1,  cpacr_el1
        and     x2,  x1,  #FPEN_BIT
        cmp     x2,  #FPEN_BIT
        b.ne    entint_1_1

        /* store temporary registers (q0-q7, q16-q31) in sp_el1*/
        stp     q30, q31, [sp, #-32]!
        stp     q28, q29, [sp, #-32]!
        stp     q26, q27, [sp, #-32]!
        stp     q24, q25, [sp, #-32]!
        stp     q22, q23, [sp, #-32]!
        stp     q20, q21, [sp, #-32]!
        stp     q18, q19, [sp, #-32]!
        stp     q16, q17, [sp, #-32]!
        stp     q6,  q7,  [sp, #-32]!
        stp     q4,  q5,  [sp, #-32]!
        stp     q2,  q3,  [sp, #-32]!
        stp     q0,  q1,  [sp, #-32]!

        mrs     x2,  fpsr
        mrs     x12, fpcr
        orr     x2,  x2,  x12
        bic     x12, x1,  #FPEN_BIT
        msr     cpacr_el1, x12
        isb
entint_1_1:
        stp     x1,  x2,  [sp, #-16]!
#endif /* #ifdef _KERNEL_FPU_VFP */
        ldr     w1,  [x3, #SYS_CTXID_C]
        ldr     w2,  [x3, #SYS_CTXID_S]
        stp     x1,  x2,  [sp, #-16]!

#ifdef _KERNEL_FPU_VFP
        mrs     x1,  cpacr_el1
        ldr     x2,  [x3, #SYS_FPSCR]
        stp     x1,  x2,  [sp, #-16]!
#endif /* #ifdef _KERNEL_FPU_VFP */
        ldr     x3,  [x0]
        ldp     x1,  x2,  [x3],  #16
        ldr     x3,  [x3]
        stp     x2,  x3,  [sp, #-16]!
        stp     x0,  x1,  [sp, #-16]!
        mov     x0,  sp
/*
 * Interrupt Handler pre-function _kernel_pre_inthdr(T_INTPARA *intpara)
 *       struct t_ipar {
 *           UW      *savedt;
 *           UW      intinfo;
 *           FP      intfunc;
 *           T_ISR   *next;
 *       } T_IPAR;
 */
        bl      _kernel_pre_inthdr
/*
 * Hook routine is here
 */
/*
        str     r0,  [sp]
        ldr     r0,  [sp, #1*4]
        ldr     r3,  systbl
        mov     r1,  #CTA_SSB
        mov     r0,  r0,  LSR #16
        ldr     r12, [r3, #SYS_CTXTRACE3]
        orr     r1,  r0,  r1, LSL #16
        cmp     r12, #0
        str     r1,  [r3, #SYS_CTXID_S]
        str     r1,  [r3, #SYS_CTXID_C]
        beq     entint_2
        stmfd   sp!, {r1,r12}
        bl      _kernel_micro_systim
        mov     r2,  r0
        ldmfd   sp!, {r1,r12}
        ldr     r3,  [sp, #2*4]
 IF {FPU} = "VFPv3" :LOR: {FPU} = "VFPv3_D16"
        ldr     r0,  [sp, #6*4]
 ELSE
        ldr     r0,  [sp, #4*4]
 ENDIF
        blx     r12
*/
        str     x0,  [sp]
        ldr     x0,  [sp, #1*8]
        ldr     x3,  systbl
        mov     x1,  #CTA_SSB
        lsr     x0,  x0,  #16
        lsl     x1,  x1,  #16
        orr     x1,  x0,  x1
        str     w1,  [x3, #SYS_CTXID_S]
        str     w1,  [x3, #SYS_CTXID_C]
        ldr     x12, [x3, #SYS_CTXTRACE3]
        cbz     x12, entint_2
        stp     x1,  x12, [sp, #-16]!
        bl      _kernel_micro_systim
        mov     x2,  x0
        ldp     x1,  x12, [sp], #16
        ldr     x3,  [sp, #2*8]
#ifdef _KERNEL_FPU_VFP
        ldr     x0,  [sp, #6*8]
#else /* #ifdef _KERNEL_FPU_VFP */
        ldr     x0,  [sp, #4*8]
#endif /* #ifdef _KERNEL_FPU_VFP */
        blr     x12

entint_2:
/*
 IF {FPU} = "VFPv3" :LOR: {FPU} = "VFPv3_D16"
        ldmfd   sp!, {r0,r1}
        tst     r1,  #OTA_FPU << 8
        ldmfd   sp!, {r1,r2}
        ldmfd   sp!, {r3,r12}
        orrne   r3,  r3,  #0x40000000
        fmxr    fpexc, r3
        fmxrne  fpscr, r12
 ELSE
        ldr     r0,  [sp]
        add     sp,  sp,  #2*4
        ldmfd   sp!, {r1,r2}
 ENDIF

        mrs     r3,  cpsr
        teq     r0,  #0
        orr     r3,  r3,  #SYS_MOD
        bicne   r3,  r3,  #I_BIT
        msr     cpsr_c,   r3
        mov     r0,  r2
*/
#ifdef _KERNEL_FPU_VFP
        ldp     x2,  x3,  [sp],  #16
        ldp     x1,  x0,  [sp],  #16
        ldp     x4,  x5,  [sp],  #16
        tbz     x3,  #OTA_FPU_OFFSET, entint_2_1
        orr     x4,  x4,  #FPEN_BIT
        msr     cpacr_el1, x4
        isb
        msr     fpsr, x5
        msr     fpcr, x5
        isb
        b       entint_2_2
entint_2_1:
        msr     cpacr_el1, x4
        isb
entint_2_2:
#else /* #ifdef _KERNEL_FPU_VFP */
        ldp     x2,  x3,  [sp],  #16
        ldp     x1,  x0,  [sp],  #16
#endif /* #ifdef _KERNEL_FPU_VFP */

        msr     spsel, #0
        isb
        mov     x10, sp
        and     sp,  x10,  #(~(0x0F))   /* system stack must be aligned with 16bytes */
        cbz     x2,  entint_3
        msr     daifclr,    #I_BIT
        isb

entint_3:

/************************************************
 * Call the interrupt handler body              *
 *      Interrupt handler                       *
 *      Interrupt service routine（x0=&isr）    *
 ************************************************/

/*
        blx     r1
*/
        blr     x1

/************************************************
 * exit of the interrupt                        *
 ************************************************/

retint:
/*
        cpsid   i,   #IRQ_MOD
 IF {FPU} = "VFPv3" :LOR: {FPU} = "VFPv3_D16"
        fmrx    r1,  fpexc
        bic     r1,  r1,  #0x40000000
        fmxr    fpexc, r1
        ldr     r0,  [sp, #2*4]
        tst     r0,  #0x40000000
        ldrne   r0,  [sp, #(16+2+8)*4]
        ldreq   r0,  [sp, #(2+8)*4]
 ELSE
        ldr     r0,  [sp, #8*4]
 ENDIF
*/
        msr     daifset, #I_BIT
        msr     spsel, #1
        isb
#ifdef _KERNEL_FPU_VFP
        mrs     x0,  cpacr_el1
        bic     x0,  x0, #FPEN_BIT
        msr     cpacr_el1, x0
        isb
        ldr     x0,  [sp, #2*8]
        and     x0,  x0,  #FPEN_BIT
        cmp     x0,  #FPEN_BIT
        b.ne    retint_1_1
        ldr     x0,  [sp, #((2*8)+(2*8)+(24*16))]
        b       retint_1_2
retint_1_1:
        ldr     x0,  [sp, #((2*8)+(2*8))]
retint_1_2:
#else /* #ifdef _KERNEL_FPU_VFP */
        ldr     x0,  [sp, #2*8]
#endif /* #ifdef _KERNEL_FPU_VFP */

/* Interrupt Handler post-function _kernel_post_inthdr(UW savedt) */
        bl      _kernel_post_inthdr

/*
        mrs     r2,  cpsr
        ldr     r3,  systbl
        ldrb    r0,  [r3, #SYS_ICNT]
        orr     r1,  r2,  #SYS_MOD
        subs    r2,  r0,  #1
        ldr     r0,  [r3, #SYS_CSSB]
        strb    r2,  [r3, #SYS_ICNT]
        bne     retint_3
        teq     r0,  #0
        bne     retint_3

        ldr     r2,  [r3, #SYS_CTCB]
        teq     r2,  #0
        beq     retint_2
        ldr     r0,  [r2, #TCB_SP]
        teq     r0,  #0
        bne     retint_2
        ldr     r0,  [r3, #SYS_SSB_TOP]
        teq     r0,  #0
        beq     retint_3
*/
        ldr     x3,  systbl
        ldrb    w0,  [x3, #SYS_ICNT]
        sub     w0,  w0,  #1
        strb    w0,  [x3, #SYS_ICNT]
        cbnz    w0,  retint_3
        ldr     x0,  [x3, #SYS_CSSB]
        cbnz    x0,  retint_3

        ldr     x2,  [x3, #SYS_CTCB]
        cbz     x2,  retint_2
        ldr     x0,  [x2, #TCB_SP]
        cbnz    x0,  retint_2
        ldr     x0,  [x3, #SYS_SSB_TOP]
        cbz     x0,  retint_3

/*
        add     sp,  sp,  #2*4
 IF {FPU} = "VFPv3" :LOR: {FPU} = "VFPv3_D16"
        ldmfd   sp!, {r0,r1}
        tst     r0,  #0x40000000
        ldrne   lr,  [sp, #(16+4)*4]
        ldreq   lr,  [sp, #4*4]
 ELSE
        ldr     lr,  [sp, #4*4]
 ENDIF
        bic     lr,  lr,  #0x07
        sub     r12, lr,  #10*4
 IF {FPU} = "VFPv3" :LOR: {FPU} = "VFPv3_D16"
;    Restore Temporary FPU registers
        fmxrne  fpexc, r0
        fmxrne  fpscr, r1
        fldmfddne sp!, {d0-d7}
;    Save all FPU registers at Task Stack
 IF :LNOT: ({FPU} = "VFPv3_D16")
        fstmfddne r12!,{d16-d31}
 ENDIF
        fstmfddne r12!,{d0-d15}
        stmfd   r12!,{r0,r1}
        bicne   r0,  r0,  #0x40000000
        fmxr    fpexc, r0
 ENDIF
        stmfd   r12!,{r4-r11}
        ldmfd   sp!, {r0,r3-r11}
        stmfd   lr,  {r0,r3-r11}
        mov     r0,  #CTA_INT
        cps     #SYS_MOD
        strb    r0,  [r2, #TCB_STAT_CATR]
        str     r12, [r2, #TCB_SP]
        cpsie   i
        nop
        b       dispatch
*/
        add     sp,  sp,  #2*8
#ifdef _KERNEL_FPU_VFP
        ldp     x4,  x5,  [sp], #16
        and     x0,  x4,  #FPEN_BIT
        cmp     x0,  #FPEN_BIT
        b.ne    retint_1_3
        ldr     x30, [sp, #((24*16)+(1*8))]
        b       retint_1_4
retint_1_3:
        ldr     x30, [sp, #(1*8)]
retint_1_4:
#else /* #ifdef _KERNEL_FPU_VFP */
        ldr     x30, [sp, #1*8]
#endif /* #ifdef _KERNEL_FPU_VFP */

        bic     x30, x30, #0x0f
        sub     x3,  x30, #22*8
#ifdef _KERNEL_FPU_VFP
        cmp     x0,  #FPEN_BIT
        b.ne    retint_1_5

        msr     cpacr_el1, x4
        isb
        msr     fpsr, x5
        msr     fpcr, x5
        isb
        /* restore temporary registers (q0-q7, q16-q31) from sp_el1 */
        ldp     q0,  q1,  [sp], #32
        ldp     q2,  q3,  [sp], #32
        ldp     q4,  q5,  [sp], #32
        ldp     q6,  q7,  [sp], #32
        ldp     q16, q17, [sp], #32
        ldp     q18, q19, [sp], #32
        ldp     q20, q21, [sp], #32
        ldp     q22, q23, [sp], #32
        ldp     q24, q25, [sp], #32
        ldp     q26, q27, [sp], #32
        ldp     q28, q29, [sp], #32
        ldp     q30, q31, [sp], #32

        /* store temporary registers (q0-q7. q16-q31) in sp_el0 */
        stp     q30, q31, [x3, #-32]!
        stp     q28, q29, [x3, #-32]!
        stp     q26, q27, [x3, #-32]!
        stp     q24, q25, [x3, #-32]!
        stp     q22, q23, [x3, #-32]!
        stp     q20, q21, [x3, #-32]!
        stp     q18, q19, [x3, #-32]!
        stp     q16, q17, [x3, #-32]!
        stp     q6,  q7,  [x3, #-32]!
        stp     q4,  q5,  [x3, #-32]!
        stp     q2,  q3,  [x3, #-32]!
        stp     q0,  q1,  [x3, #-32]!

        /* store non-temporary registers (q8-q15) in sp_el0 */
        stp     q14, q15, [x3, #-32]!
        stp     q12, q13, [x3, #-32]!
        stp     q10, q11, [x3, #-32]!
        stp     q8,  q9,  [x3, #-32]!

        bic     x0,  x4,  #FPEN_BIT
        msr     cpacr_el1, x0
        isb
retint_1_5:
        stp     x4,  x5,  [x3, #-16]!
#endif /* #ifdef _KERNEL_FPU_VFP */

        /* store non-temporary registers (x19-x29) in sp_el0 */
        stp     x29, x29, [x3, #-16]!
        stp     x27, x28, [x3, #-16]!
        stp     x25, x26, [x3, #-16]!
        stp     x23, x24, [x3, #-16]!
        stp     x21, x22, [x3, #-16]!
        stp     x19, x20, [x3, #-16]!

        /* restore temporary registers (spsr, elr, x0-x18, x30) from sp_el1 */
        add     sp,  sp,  #2*8
        ldp     x25, x26, [sp],  #16
        ldp     x23, x24, [sp],  #16
        ldp     x21, x22, [sp],  #16
        ldp     x19, x20, [sp],  #16
        ldp     x17, x18, [sp],  #16
        ldp     x15, x16, [sp],  #16
        ldp     x13, x14, [sp],  #16
        ldp     x11, x12, [sp],  #16
        ldp     x9,  x10, [sp],  #16
        ldp     x7,  x8,  [sp],  #16
        ldp     x5,  x6,  [sp],  #16

        /* store temporary registers (spsr, elr, x0-x18, x30) in sp_el0 */
        stp     x5,  x6,  [x30, #-16]!
        stp     x7,  x8,  [x30, #-16]!
        stp     x9,  x10, [x30, #-16]!
        stp     x11, x12, [x30, #-16]!
        stp     x13, x14, [x30, #-16]!
        stp     x15, x16, [x30, #-16]!
        stp     x17, x18, [x30, #-16]!
        stp     x19, x20, [x30, #-16]!
        stp     x21, x22, [x30, #-16]!
        stp     x23, x24, [x30, #-16]!
        stp     x25, x26, [x30, #-16]!

        mov     w0,  #CTA_INT
        strb    w0,  [x2, #TCB_STAT_CATR]
        str     x3,  [x2, #TCB_SP]
        msr     spsel, #0
        msr     daifclr, #I_BIT
        isb
        b       dispatch

retint_2:
/*
 IF {FPU} = "VFPv3" :LOR: {FPU} = "VFPv3_D16"
        ldr     r0,  [sp, #2*4]
        tst     r0,  #0x40000000
        bicne   r0,  r0,  #0x40000000
        fmxr    fpexc, r0
        addne   sp,  sp,  #(16+14)*4
        addeq   sp,  sp,  #14*4
 ELSE
        add     sp,  sp,  #12*4
 ENDIF
        cpsie   i,   #SYS_MOD
        b       dispatch
*/
#ifdef _KERNEL_FPU_VFP
        ldr     x0,  [sp, #2*8]
        and     x1,  X0,  #FPEN_BIT
        cmp     x1,  #FPEN_BIT
        b.ne    retint_2_1
        bic     x0,  x0,  #FPEN_BIT
        add     sp,  sp,  #((26*8)+(24*16)+(2*8))
        b       retint_2_2
retint_2_1:
        add     sp,  sp,  #((26*8)+(2*8))
retint_2_2:
        msr     cpacr_el1, x0
        isb
#else /* #ifdef _KERNEL_FPU_VFP */
        add     sp,  sp,  #26*8
#endif /* #ifdef _KERNEL_FPU_VFP */

        msr     spsel, #0
        msr     daifclr, #I_BIT
        isb
        b       dispatch

retint_3:
/*
 * Hook routine is here
 */
/*
        ldr     r12, [x3, #SYS_CTXTRACE3]
        teq     r12, #0
        beq     retint_4

        stmfd   sp!, {r3,r12}
        bl      _kernel_micro_systim
        ldmfd   sp!, {r3,r12}
        mov     r2,  r0
        ldr     r0,  [r3, #SYS_CTXID_C]
        ldr     r1,  [sp]
        blx     r12
        ldr     r3,  systbl
*/
        ldr     x12, [x3, #SYS_CTXTRACE3]
        cbz     x12, retint_4

        stp     x3,  x12, [sp, #-16]!
        bl      _kernel_micro_systim
        ldp     x3,  x12, [sp], #16
        mov     x2,  x0
        ldr     w0,  [x3, #SYS_CTXID_C]
        ldr     w1,  [sp]
        blr     x12
        ldr     x3,  systbl

retint_4:
/*
        ldmfd   sp!, {r0-r1}
        str     r0,  [r3, #SYS_CTXID_C]
        str     r1,  [r3, #SYS_CTXID_S]
 IF {FPU} = "VFPv3" :LOR: {FPU} = "VFPv3_D16"
;    Restore Temporary FPU  registers
        ldmfd   sp!, {r1,r2}
        tst     r1,  #0x40000000
        fmxr    fpexc, r1
        fmxrne  fpscr, r2
        fldmfddne sp!, {d0-d7}
 ENDIF
        ldmfd   sp!, {r0-r3}
        ldmfd   sp,  {sp,lr}^
        add     sp,  sp,  #2*4
        ldmfd   sp!, {r12,lr}
        msr     spsr_csxf,lr
        ldmfd   sp!, {r12,pc}^
*/
        ldp     x0,  x1,  [sp], #16
        str     w0,  [x3, #SYS_CTXID_C]
        str     w1,  [x3, #SYS_CTXID_S]

#ifdef _KERNEL_FPU_VFP
        ldp     x0,  x1,  [sp], #16
        msr     cpacr_el1, x0
        isb
        and     x0,  x0,  #FPEN_BIT
        cmp     x0,  #FPEN_BIT
        b.ne    retint_4_1

        msr     fpsr, x1
        msr     fpcr, x1
        isb

        /* restore temporary registers (q0-q7, q16-q31) from sp_el1 */
        ldp     q0,  q1,  [sp], #32
        ldp     q2,  q3,  [sp], #32
        ldp     q4,  q5,  [sp], #32
        ldp     q6,  q7,  [sp], #32
        ldp     q16, q17, [sp], #32
        ldp     q18, q19, [sp], #32
        ldp     q20, q21, [sp], #32
        ldp     q22, q23, [sp], #32
        ldp     q24, q25, [sp], #32
        ldp     q26, q27, [sp], #32
        ldp     q28, q29, [sp], #32
        ldp     q30, q31, [sp], #32
retint_4_1:
#endif /* #ifdef _KERNEL_FPU_VFP */

        ldp     x0,  x1,  [sp], #16
        msr     sp_el0,  x1
        isb

        ldp     x0,  x1,  [sp], #16
        msr     spsr_el1, x0
        msr     elr_el1,  x1
        isb

        /* restore temporary registers (x0-x18, x30) from sp_el1 */
        ldp     x0,  x1,  [sp], #16
        ldp     x2,  x3,  [sp], #16
        ldp     x4,  x5,  [sp], #16
        ldp     x6,  x7,  [sp], #16
        ldp     x8,  x9,  [sp], #16
        ldp     x10, x11, [sp], #16
        ldp     x12, x13, [sp], #16
        ldp     x14, x15, [sp], #16
        ldp     x16, x17, [sp], #16
        ldp     x18, x30, [sp], #16
        eret

/************************************************
 * lock and unlock kernel                       *
 ************************************************/

/*
 * void _kernel_lock(void);
 */
        .globl  _kernel_lock
        .type   _kernel_lock, @function
_kernel_lock:
/*
        mrs     r3,  cpsr
        ldr     r2,  systbl
        tst     r3,  #0x0F
        swieq   1
        orrne   r12, r3,  #0x80
        msrne   cpsr_c,   r12
        strb    r3,  [r2, #SYS_CIMS]
        bx      lr
*/
        mrs     x0,  daif
        lsr     x0,  x0,  #DAIF_OFFSET
        ldr     x1,  systbl
        msr     daifset, #I_BIT
        isb
        strb    w0,  [x1, #SYS_CIMS]
        ret

/*
 * void _kernel_unlock(void);
 */
        .globl  _kernel_unlock
        .type   _kernel_unlock, @function
_kernel_unlock:
/*
        ldr     r2,  systbl
        mov     r1,  #0
        ldrb    r0,  [r2, #SYS_CIMS]
        strb    r1,  [r2, #SYS_CIMS]
        msr     cpsr_c,   r0
        bx      lr
*/
        ldr     x1,  systbl
        ldrb    w0,  [x1, #SYS_CIMS]
        lsl     x0,  x0,  #DAIF_OFFSET
        strb    wzr, [x1, #SYS_CIMS]
        msr     daif, x0
        isb
        ret

/************************************************
 * Queuing of current SSB                       *
 *      x19 = &systbl                           *
 ************************************************/
deqssb:
/*
        mrs     r3,  cpsr
        cpsid   i
        ldr     r0,  [r4, #SYS_SSB_TOP]
        teq     r0,  #0
        ldrne   r1,  [r0, #SSB_SNEXT]
        strne   r0,  [r4, #SYS_CSSB]
        strne   r1,  [r4, #SYS_SSB_TOP]
        msr     cpsr_c,   r3
        bx      lr
*/
        mrs     x3,  daif
        msr     daifset, #I_BIT
        isb
        ldr     x0,  [x19, #SYS_SSB_TOP]
        cbz     x0,  deqssb_ret
        str     x0,  [x19, #SYS_CSSB]
        ldr     x1,  [x0, #SSB_SNEXT]
        str     x1,  [x19, #SYS_SSB_TOP]
deqssb_ret:
        msr     daif,   x3
        isb
        ret


/************************************************
 * Search task of highest priority              *
 *      x19 = &systbl                           *
 ************************************************/

gettcb:
/*
        ldr     r3,  [r4, #SYS_RDQ]
        ldrh    r2,  [r3]
        mov     r1,  r2
gettcb_1
        add     r3,  r3,  #2*4
        ldr     r0,  [r3, #TCB_MNEXT]
        cmp     r0,  r3
        bxne    lr
        sub     r2,  r2,  #1
        teq     r2,  #0
        bne     gettcb_1

        mov     r0,  #0
        bx      lr
*/
        ldr     x3,  [x19, #SYS_RDQ]
        ldrh    w2,  [x3]
        mov     x1,  x2
gettcb_1:
        add     x3,  x3,  #2*8
        ldr     x0,  [x3, #TCB_MNEXT]
        cmp     x0,  x3
        b.ne    gettcb_ret
        sub     x2,  x2,  #1
        cbnz    x2,  gettcb_1

        mov     x0,  #0
gettcb_ret:
        ret

/************************************************
 * get the free SSB                             *
 ************************************************/

/*
 * T_SSB *_kernel_getssb(void);
 */
        .globl  _kernel_getssb
        .type   _kernel_getssb, @function
_kernel_getssb:
/*
        stmfd   sp!, {r4-r5}
        mrs     r1,  cpsr
        ldr     r4,  systbl
        cpsid   i
        ldr     r0,  [r4, #SYS_SSB_FREE]
        teq     r0,  #0
        beq     getssb_1
        ldr     r2,  [r0, #SSB_SNEXT]
        ldrh    r3,  [r4, #SYS_SSB_CNT]
        ldrh    r5,  [r4, #SYS_SSB_MIN]
        sub     r3,  r3,  #1
        str     r2,  [r4, #SYS_SSB_FREE]
        cmp     r3,  r5
        strh    r3,  [r4, #SYS_SSB_CNT]
        strlth  r3,  [r4, #SYS_SSB_MIN]
getssb_1
        msr     cpsr_c,   r1
        ldmfd   sp!, {r4-r5}
        bx      lr
*/
        mrs     x1,  daif
        ldr     x4,  systbl
        msr     daifset, #I_BIT
        isb
        ldr     x0,  [x4, #SYS_SSB_FREE]
        cbz     x0,  getssb_1
        ldr     x2,  [x0, #SSB_SNEXT]
        str     x2,  [x4, #SYS_SSB_FREE]
        ldrh    w3,  [x4, #SYS_SSB_CNT]
        sub     w3,  w3,  #1
        strh    w3,  [x4, #SYS_SSB_CNT]
        ldrh    w5,  [x4, #SYS_SSB_MIN]
        cmp     w3,  w5
        b.ge    getssb_1
        strh    w3,  [x4, #SYS_SSB_MIN]
getssb_1:
        msr     daif, x1
        isb
        ret

/************************************************
 * release SSB                                  *
 *      x19 = &systbl                           *
 *      x20 = &ssb                              *
 ************************************************/

relssb:
/*
        mrs     r1,  cpsr
        cpsid   i
        ldr     r2,  [r4, #SYS_SSB_FREE]
        ldrh    r3,  [r4, #SYS_SSB_CNT]
        str     r5,  [r4, #SYS_SSB_FREE]
        add     r3,  r3,  #1
        str     r2,  [r5, #SSB_SNEXT]
        strh    r3,  [r4, #SYS_SSB_CNT]
        msr     cpsr_c,   r1
        bx      lr
*/
        mrs     x1,  daif
        msr     daifset, #I_BIT
        isb
        ldr     x2,  [x19, #SYS_SSB_FREE]
        str     x20, [x19, #SYS_SSB_FREE]
        str     x2,  [x20, #SSB_SNEXT]
        ldrh    w3,  [x19, #SYS_SSB_CNT]
        add     x3,  x3,  #1
        strh    w3,  [x19, #SYS_SSB_CNT]
        msr     daif, x1
        isb
        ret

/************************************************
 * store the return address of the system call  *
 *      x0 = return address                     *
 ************************************************/

/*
 * void _kernel_save_pc(void);
 */
        .globl  _kernel_save_pc
        .type   _kernel_save_pc, @function

_kernel_save_pc:
/*
        mov     r1,  r0
        ldr     r0,  systbl
        ldrb    r12, [r0, #SYS_ICNT]
        cmp     r12, #0
        bxne    lr
        ldr     r12, [r0, #SYS_CSSB]
        cmp     r12, #0
        bxne    lr
        ldr     r12, [r0, #SYS_CTCB]
        cmp     r12, #0
        bxeq    lr
        str     r1,  [r12, #TCB_PC]
        bx      lr
*/
        ldr     x1,  systbl
        ldrb    w2,  [x1, #SYS_ICNT]
        cbnz    w2,  _kernel_save_pc_ret
        ldr     x2,  [x1, #SYS_CSSB]
        cbnz    x2,  _kernel_save_pc_ret
        ldr     x2,  [x1, #SYS_CTCB]
        cbz     x2,  _kernel_save_pc_ret
        str     x0,  [x2, #TCB_PC]
_kernel_save_pc_ret:
        ret

/************************************************
 * Get and set the interrupt level              *
 ************************************************/

/*
 * IMASK _kernel_get_imask(void);
 */
        .globl  _kernel_get_imask
        .type   _kernel_get_imask, @function
_kernel_get_imask:
/*
        mrs     r0,  cpsr
        tst     r0,  #I_BIT
        moveq   r0,  #0
        movne   r0,  #1
        bx      lr
*/
        mrs     x0,  daif
        and     x0,  x0,  #(I_BIT << DAIF_OFFSET)
        lsr     x0,  x0,  #(I_BIT_OFFSET + DAIF_OFFSET)
        ret

/*
 * IMASK _kernel_set_imask(IMASK imask);
 */
        .globl  _kernel_set_imask
        .type   _kernel_set_imask, @function
_kernel_set_imask:
/*
        mov     r1,  r0
        mrs     r0,  cpsr
        tst     r0,  #0x0F
        swieq   0
        teq     r1,  #0
        biceq   r2,  r0,  #I_BIT
        orrne   r2,  r0,  #I_BIT
        msr     cpsr_c,   r2
        tst     r0,  #I_BIT
        moveq   r0,  #0
        movne   r0,  #1
        bx      lr
*/
        mrs     x1,  daif
        bic     x2,  x1,  #(I_BIT << DAIF_OFFSET)
        orr     x2,  x2,  x0,  lsl #(I_BIT_OFFSET + DAIF_OFFSET)
        msr     daif, x2
        isb
        and     x1,  x1,  #(I_BIT << DAIF_OFFSET)
        lsr     x0,  x1,  #(I_BIT_OFFSET + DAIF_OFFSET)
        ret

/************************************************
 * read MPIDR                                   *
 ************************************************/

/*
 * UINT vget_cid(void);
 */
        .globl  vget_cid
        .type   vget_cid, @function
vget_cid:
/*
        mrc     p15, 0,  r1,  c0,  c0,  5
        and     r1,  r1,  #0x0f
        mov     r0,  #0
        cmp     r1,  #0
        moveq   r0,  #CORE0
        cmp     r1,  #1
        moveq   r0,  #CORE1
        cmp     r1,  #2
        moveq   r0,  #CORE2
        cmp     r1,  #3
        moveq   r0,  #CORE3
        bx      lr
*/
        mrs     x0,  mpidr_el1
#ifdef _KERNEL_ARMV82A
        and     x0,  x0,  #0xf00
        asr     x0,  x0,  #8
#else
        and     x0,  x0,  #0xff
#endif
        add     x0,  x0,  #1
        ret

#ifdef _KERNEL_FPU_VFP
/************************************************
 * Enable and disable the VFP                   *
 ************************************************/

/*
 * UW vget_fpscr(void);
 */
        .globl  vget_fpscr
        .type   vget_fpscr, @function
vget_fpscr:
/*
        fmrx    r1,  fpexc
        tst     r1,  #0x40000000
        orreq   r2,  r1,  #0x40000000
        fmxreq  fpexc, r2
        fmrx    r0,  fpscr
        fmxreq  fpexc, r1
        bx      lr
*/
        mrs     x1,  cpacr_el1
        orr     x2,  x1,  #FPEN_BIT
        msr     cpacr_el1, x2
        isb
        mrs     x0,  fpsr
        mrs     x2,  fpcr
        orr     x0,  x0,  x2
        msr     cpacr_el1, x1
        isb
        ret

/*
 * void vset_fpscr(UW fpscr);
 */
        .globl  vset_fpscr
        .type   vset_fpscr, @function
vset_fpscr:
/*
        fmrx    r1,  fpexc
        tst     r1,  #0x40000000
        orreq   r2,  r1,  #0x40000000
        fmxreq  fpexc, r2
        fmxr    fpscr, r0
        fmxreq  fpexc, r1
        bx      lr
*/
        mrs     x1,  cpacr_el1
        orr     x2,  x1,  #FPEN_BIT
        msr     cpacr_el1, x2
        isb
        msr     fpsr, x0
        msr     fpcr, x0
        isb
        msr     cpacr_el1, x1
        isb
        ret

/*
 * void vena_vfp(void);
 */
        .globl  vena_vfp
        .type   vena_vfp, @function
vena_vfp:
/*
        fmrx    r0,  fpexc
        tst     r0,  #0x40000000
        ldreq   r2,  systbl
        orreq   r0,  r0,  #0x40000000
        ldreq   r1,  [r2, #SYS_FPSCR]
        fmxreq  fpexc, r0
        fmxreq  fpscr, r1
        bx      lr
*/
        mrs     x0,  cpacr_el1
        and     x1,  x0,  #FPEN_BIT
        cmp     x1,  #FPEN_BIT
        b.eq    vena_vfp_end
        orr     x0,  x0,  #FPEN_BIT
        msr     cpacr_el1, x0
        isb
        ldr     x2,  systbl
        ldr     w1,  [x2, #SYS_FPSCR]
        msr     fpsr, x1
        msr     fpcr, x1
        isb
vena_vfp_end:
        ret

/*
 * void vdis_vfp(void);
 */
        .globl  vdis_vfp
        .type   vdis_vfp, @function
vdis_vfp:
/*
        fmrx    r0,  fpexc
        tst     r0,  #0x40000000
        bicne   r0,  r0,  #0x40000000
        fmxrne  fpexc, r0
        bx      lr
*/
        mrs     x0,  cpacr_el1
        bic     x0,  x0,  #FPEN_BIT
        msr     cpacr_el1, x0
        isb
        ret

#endif /* #ifdef _KERNEL_FPU_VFP */


/************************************************
 * Barriers                                     *
 ************************************************/

/*
 * Data synchronization barrier
 * void vdsb(void);
 */
        .globl  vdsb
        .type   vdsb, @function
vdsb:
        dsb     sy
        ret

/*
 * Data memory barrier
 * void vdmb(void);
 */
        .globl  vdmb
        .type   vdmb, @function
vdmb:
        dmb     sy
        ret

/*
 * Instruction synchronization barrier
 * void visb(void);
 */
        .globl  visb
        .type   visb, @function
visb:
        isb
        ret

/************************************************
 * System Error exception handler               *
 ************************************************/

        .globl  _kernel_serhdr
        .type   _kernel_serhdr, @function
_kernel_serhdr:
        msr     spsel,  #1  /* select sp_el1 */
        isb

        /* store temporary registers (spsr, elr, x0-x18, x30) */
        stp     x18, x30, [sp, #-16]!
        stp     x16, x17, [sp, #-16]!
        stp     x14, x15, [sp, #-16]!
        stp     x12, x13, [sp, #-16]!
        stp     x10, x11, [sp, #-16]!
        stp     x8,  x9,  [sp, #-16]!
        stp     x6,  x7,  [sp, #-16]!
        stp     x4,  x5,  [sp, #-16]!
        stp     x2,  x3,  [sp, #-16]!
        stp     x0,  x1,  [sp, #-16]!

        mrs     x0,  spsr_el1
        mrs     x1,  elr_el1
        stp     x0,  x1,  [sp, #-16]!

        ldr     x3,  systbl
        ldr     x3,  [x3, #SYS_SERHDR]
        cbz     x3,  _kernel_serhdr_ret
        mrs     x0,  esr_el1
        mrs     x1,  far_el1
        mov     x2,  sp
        blr     x3

_kernel_serhdr_ret:
        ldp     x0,  x1,  [sp], #16
        msr     spsr_el1, x0
        msr     elr_el1,  x1
        isb

        ldp     x0,  x1,  [sp], #16
        ldp     x2,  x3,  [sp], #16
        ldp     x4,  x5,  [sp], #16
        ldp     x6,  x7,  [sp], #16
        ldp     x8,  x9,  [sp], #16
        ldp     x10, x11, [sp], #16
        ldp     x12, x13, [sp], #16
        ldp     x14, x15, [sp], #16
        ldp     x16, x17, [sp], #16
        ldp     x18, x30, [sp], #16
        eret

/************************************************
 * Synchronous exception handler                *
 ************************************************/

        .globl  _kernel_synhdr
        .type   _kernel_synhdr, @function
_kernel_synhdr:
        msr     spsel,  #1  /* select sp_el1 */
        isb

        /* store temporary registers (spsr, elr, x0-x18, x30) */
        stp     x18, x30, [sp, #-16]!
        stp     x16, x17, [sp, #-16]!
        stp     x14, x15, [sp, #-16]!
        stp     x12, x13, [sp, #-16]!
        stp     x10, x11, [sp, #-16]!
        stp     x8,  x9,  [sp, #-16]!
        stp     x6,  x7,  [sp, #-16]!
        stp     x4,  x5,  [sp, #-16]!
        stp     x2,  x3,  [sp, #-16]!
        stp     x0,  x1,  [sp, #-16]!

        mrs     x0,  spsr_el1
        mrs     x1,  elr_el1
        stp     x0,  x1,  [sp, #-16]!

        ldr     x3,  systbl
        ldr     x3,  [x3, #SYS_SYNHDR]
        cbz     x3,  _kernel_synhdr_ret
        mrs     x0,  esr_el1
        mrs     x1,  far_el1
        mov     x2,  sp
        blr     x3

_kernel_synhdr_ret:
        ldp     x0,  x1,  [sp], #16
        msr     spsr_el1, x0
        msr     elr_el1,  x1
        isb

        ldp     x0,  x1,  [sp], #16
        ldp     x2,  x3,  [sp], #16
        ldp     x4,  x5,  [sp], #16
        ldp     x6,  x7,  [sp], #16
        ldp     x8,  x9,  [sp], #16
        ldp     x10, x11, [sp], #16
        ldp     x12, x13, [sp], #16
        ldp     x14, x15, [sp], #16
        ldp     x16, x17, [sp], #16
        ldp     x18, x30, [sp], #16
        eret

        .balign 8
systbl:
        .quad   _kernel_systbl

/************************************************
 * templete of the interrupt entry              *
 ************************************************/
        .globl  _kernel_inthdr
        .type   _kernel_inthdr, @function
_kernel_inthdr:
/*
        sub     lr,  lr,  #4
        stmfd   sp!, {r12,lr}
        adr     r12, intinfo
        mrs     lr,  spsr
        stmfd   sp!, {r12,lr}
        ldr     lr,  [r12, #-4]
        mov     r12, sp
        stmfd   r12, {r0-r3,sp,lr}^
        sub     sp,  sp,  #6*4
        bx      lr
        nop
*/
#ifdef _KERNEL_OMIT_SAVING_REGS
#else
        msr     spsel,  #1  /* select sp_el1 */
        isb

        /* store temporary registers (sp_el0, spsr, elr, x0-x18, x30) */
        stp     x18, x30, [sp, #-16]!
        stp     x16, x17, [sp, #-16]!
        stp     x14, x15, [sp, #-16]!
        stp     x12, x13, [sp, #-16]!
        stp     x10, x11, [sp, #-16]!
        stp     x8,  x9,  [sp, #-16]!
        stp     x6,  x7,  [sp, #-16]!
        stp     x4,  x5,  [sp, #-16]!
        stp     x2,  x3,  [sp, #-16]!
        stp     x0,  x1,  [sp, #-16]!
#endif

        mrs     x0,  spsr_el1
        mrs     x1,  elr_el1
        stp     x0,  x1,  [sp, #-16]!

        adr     x0,  intinfo
        mrs     x1,  sp_el0
        stp     x0,  x1,  [sp, #-16]!

        ldr     x0,  [x0, #-8]
        br      x0
        nop

/*
entint          DCD     _kernel_entint
intinfo         DCD     0
intfunc         DCD     0
isrbase         DCD     0
                DCD     0
*/
        .balign 8
entint:
        .quad   _kernel_entint
intinfo:
        .quad   0
intfunc:
        .quad   0
isrbase:
        .quad   0
        .quad   0
