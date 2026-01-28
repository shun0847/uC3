/**
 * @file    ampext_shmem_spinlock.h
 * @brief   Lock functions for Sheard Memory Library
 * @date    2019.09.20
 * @author  Copyright (c) 2019, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2019.09.20) nozaki
 *            Initial version.
 ****************************************************************************
 */
#ifndef AMPEXT_SHMEM_SPINLOCK_H_
#define AMPEXT_SHMEM_SPINLOCK_H_

#include <metal/atomic.h>
#include <stdbool.h>
#include <stdint.h>

typedef void* ampext_spinlock_global_handler;

struct ampext_spinlock{
    volatile atomic_int v;
};

ampext_spinlock_global_handler ampext_spinlock_global_init(void);

void ampext_spinlock_global_deinit(ampext_spinlock_global_handler *hd);

void ampext_spinlock_init(struct ampext_spinlock *slock);

void ampext_spinlock_release(ampext_spinlock_global_handler *hd, struct ampext_spinlock *slock);

void ampext_spinlock_acquire(ampext_spinlock_global_handler *hd, struct ampext_spinlock *slock, uint8_t v);

#endif /* AMPEXT_SHMEM_SPINLOCK_H_ */
