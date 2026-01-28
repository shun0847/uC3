/**
 * @brief   Micro C Cube Standard, Multi Core Extension
 *          Exclusive Access (AArch64)
 * @date    2018.02.27
 *
 * @copyright (c) 2016-2018, eForce Co., Ltd. All rights reserved.
 */
        .globl  vget_cid

/*
 * spinlock
 * void _kernel_spin_lock(UW * flag);
 */
        .text
        .balign 4
        .globl  _kernel_spin_lock
        .type   _kernel_spin_lock, @function
_kernel_spin_lock:
        stp     x0,  x30, [sp, #-16]!
        bl      vget_cid
        ldp     x2,  x30, [sp], #16
        dmb     sy
        sevl
kernel_spin_lock_wait:
        wfe
kernel_spin_lock_loop:
        ldaxr   w1,  [x2]
        cbnz    w1,  kernel_spin_lock_wait
        stlxr   w1,  w0,  [x2]
        cbnz    w1,  kernel_spin_lock_loop
        dsb     sy
        ret

/*
 * unlock
 * ID _kernel_spin_unlock(UW * flag);
 */
        .globl  _kernel_spin_unlock
        .type   _kernel_spin_unlock, @function
_kernel_spin_unlock:
        dmb     sy
        str     wzr, [x0]
        dsb     sy
        ret

/*
 * if already locked, return owner_id
 * ID _kernel_spin_lock_test(UW * flag);
 */
        .globl  _kernel_spin_lock_test
        .type   _kernel_spin_lock_test, @function
_kernel_spin_lock_test:
        stp     x0,  x30, [sp, #-16]!
        bl      vget_cid
        ldp     x2,  x30, [sp], #16
        dmb     sy
kernel_spin_lock_test_loop:
        ldaxr   w1,  [x2]
        cbnz    w1,  kernel_spin_lock_test_end
        stlxr   w1,  w0,  [x2]
        cbnz    w1,  kernel_spin_lock_test_loop
kernel_spin_lock_test_end:
        dsb     sy
        mov     w0,  w1
        ret

/*
 * read the lock value
 * ID _kernel_spin_locked_test(UW * flag);
 */
        .globl  _kernel_spin_locked_test
        .type   _kernel_spin_locked_test, @function
_kernel_spin_locked_test:
        ldr   w0,  [x0]
        dsb   sy
        ret
