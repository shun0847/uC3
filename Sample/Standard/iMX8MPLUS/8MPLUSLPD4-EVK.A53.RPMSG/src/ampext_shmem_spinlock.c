/**
 * @file    ampext_shmem_spinlock.c
 * @brief   Lock functions for Sheard Memory Library for Linux SMP
 * @date    2020.02.28
 * @author  Copyright (c) 2020, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2020.02.28) Imada
 *            Initial version.
 ****************************************************************************
 */
#include <metal/device.h>
#include <metal/io.h>
#include <metal/irq.h>
#include "ampext_shmem_spinlock.h"
#include "platform_info.h"

#ifdef __cplusplus
extern "C" {
#endif
ampext_spinlock_global_handler ampext_spinlock_global_init(void)
{
    return (ampext_spinlock_global_handler)0x00000001;
}

void ampext_spinlock_global_deinit(ampext_spinlock_global_handler *hd)
{
    (void)hd;
}

void ampext_spinlock_init(struct ampext_spinlock *slock) {
    atomic_store(&slock->v, 0);
    atomic_thread_fence(memory_order_acq_rel);
}

void ampext_spinlock_release(ampext_spinlock_global_handler *hd, struct ampext_spinlock *slock) {
    (void)hd;
    atomic_store(&slock->v, 0U);
    atomic_thread_fence(memory_order_acq_rel);
}

void ampext_spinlock_acquire(ampext_spinlock_global_handler *hd, struct ampext_spinlock *slock, uint8_t v) {
    (void)hd;
    atomic_int tmp;
    while (1) {
        tmp = 0;
        if (atomic_compare_exchange_strong(&slock->v, &tmp, v)) {
            if  (atomic_load(&slock->v) == v) {
                break;
            }
        }
    }
}


#ifdef __cplusplus
}
#endif
