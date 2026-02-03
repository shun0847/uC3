/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 * Copyright (c) 2017 Xilinx, Inc.
 * Copyright (c) 2020-2025, eForce Co., Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <openamp/remoteproc.h>
#include <metal/atomic.h>
#include <metal/device.h>
#include <metal/io.h>
#include <metal/irq.h>
#include <metal/sys.h>
#include <metal/utilities.h>
#include <openamp/rpmsg_virtio.h>
#include "platform_info.h"
#include "OpenAMP_RPMsg_cfg.h"
#include "rsc_table.h"

extern struct remoteproc_ops imx8_a5x_proc_ops;
/* RPMsg virtio shared buffer pool */
static struct rpmsg_virtio_shm_pool shpool;

#ifndef __linux__ /* uC3 */
static void start_ipi_task(void *platform);
static void prepare_ipi_sem(void *platform);
#endif

/* Variables */
/* IPI dedicated task */
#ifndef __linux__ /* uC3 */
static ID ipi_tsk_id[CFG_RPMSG_SVCNO] = {0};
#endif

/* IPI information */
extern struct ipi_info ipi[CFG_RPMSG_SVCNO];
/* vring information */
extern struct vring_info vrinfo[CFG_RPMSG_SVCNO];

#ifdef __linux__
static inline void virtio_clear_status(void *rsc_table) {
    struct remote_resource_table *rt = (struct remote_resource_table *)rsc_table;

    rt->rpmsg_vdev.status = 0x0;

    return ;
}
#endif

static struct remoteproc *
platform_create_proc(int proc_index, int rsc_index)
{
    void *rsc_table;
    size_t rsc_size;
    int ret;
    struct remoteproc *rproc_inst;
    struct remoteproc_priv *rproc_priv;
#ifdef __linux__
    metal_phys_addr_t pa;
#endif
    (void)proc_index;
    /* Allocate and initialize remoteproc_priv instance */
    rproc_priv = metal_allocate_memory(sizeof(struct remoteproc_priv));
    if (!rproc_priv) {
        return NULL;
    }
    memset(rproc_priv, 0, sizeof(struct remoteproc_priv));

    rproc_priv->vring = &vrinfo[rsc_index];
    rproc_priv->ipi_info = &ipi[rsc_index];

    /* Allocate remoteproc instance */
    rproc_inst = metal_allocate_memory(sizeof(struct remoteproc));
    if (!rproc_inst) {
        goto err1;
    }
    memset(rproc_inst, 0, sizeof(struct remoteproc));
    rproc_inst->priv = rproc_priv;
    /* remoteproc initialization */
#ifndef __linux__ /* uC3 */
    prepare_ipi_sem(rproc_inst);
#endif
    /* remoteproc initialization */
    if (!remoteproc_init(rproc_inst, &imx8_a5x_proc_ops, rproc_priv)) {
        goto err2;
    }

    /*
     * Mmap shared memories
     * Or shall we constraint that they will be set as carved out
     * in the resource table?
     */
#ifdef __linux__
    rsc_size = sizeof(struct remote_resource_table);
    pa = CFG_RSCTBL_MEM_PA + rsc_index * rsc_size;
    rsc_table = remoteproc_mmap(rproc_inst, &pa,
                NULL, rsc_size,
                0, NULL);
    if (!rsc_table) {
        LPRINTF("Failed to map the resource table.\n");
        goto err2;
    }
#else /* uC3 */
    rsc_table = get_resource_table(rsc_index, &rsc_size);
    rproc_inst->rsc_io = rproc_priv->vring->rsc.io;
#endif

    /* Parse resource table to remoteproc */
    ret = remoteproc_set_rsc_table(rproc_inst, rsc_table, rsc_size);
    if (ret) {
        LPRINTF("Failed to intialize remoteproc\n");
        goto err2;
    }

    LPRINTF("Initialize remoteproc successfully.\n");

    return rproc_inst;
err2:
    (void)remoteproc_remove(rproc_inst);
    metal_free_memory(rproc_inst);
err1:
    metal_free_memory(rproc_priv);
    return NULL;
}

int platform_init(unsigned long proc_id, unsigned long rsc_id, void **platform)
{
    struct remoteproc *rproc;

    if (!platform) {
        LPERROR("Failed to initialize platform,"
               "NULL pointer to store platform data.\n");
        return -EINVAL;
    }

    rproc = platform_create_proc((int)proc_id, (int)rsc_id);
    if (!rproc) {
        LPERROR("Failed to create remoteproc device.\n");
        *platform = NULL;
        return -EINVAL;
    }
    *platform = rproc;
    return 0;
}

struct  rpmsg_device *
platform_create_rpmsg_vdev(void *platform, unsigned int vdev_index,
               unsigned int role,
               void (*rst_cb)(struct virtio_device *vdev),
               rpmsg_ns_bind_cb ns_bind_cb)
{
    struct remoteproc *rproc = platform;
    struct remoteproc_priv *prproc;
    struct rpmsg_virtio_device *rpmsg_vdev;
    struct virtio_device *vdev;
    struct metal_io_region *shbuf_io;
    metal_phys_addr_t pa;
#ifdef __linux__
    void *shbuf;
    size_t len;
#endif
    int ret;

    prproc = rproc->priv;

    rpmsg_vdev = metal_allocate_memory(sizeof(*rpmsg_vdev));
    if (!rpmsg_vdev)
        return NULL;
    memset(rpmsg_vdev, 0, sizeof(*rpmsg_vdev));

    LPRINTF("creating remoteproc virtio\n");
    vdev = remoteproc_create_virtio(rproc, (int)vdev_index, role, rst_cb);
    if (!vdev) {
        LPRINTF("failed remoteproc_create_virtio\n");
        goto err;
    }

    pa = metal_io_phys(prproc->vring->shm.io, 0U);
    shbuf_io = remoteproc_get_io_with_pa(rproc, pa);
    if (!shbuf_io) {
        LPRINTF("failed remoteproc_get_io_with_pa\n");
        goto err;
    }

    /* Only RPMsg virtio master needs to initialize the shared buffers pool */
#ifdef __linux__
    LPRINTF("initializing rpmsg shared buffer pool\n");
    shbuf = metal_io_phys_to_virt(shbuf_io, pa);
    len = metal_io_region_size(prproc->vring->shm.io);
    rpmsg_virtio_init_shm_pool(&shpool, shbuf, len);
#endif

    LPRINTF("initializing rpmsg vdev\n");
    /* RPMsg virtio slave can set shared buffers pool argument to NULL */
    ret =  rpmsg_init_vdev(rpmsg_vdev, vdev, ns_bind_cb,
                   shbuf_io,
                   &shpool);
    if (ret) {
        LPRINTF("failed rpmsg_init_vdev\n");
        goto err;
    }
#ifndef __linux__ /* uC3 */
    start_ipi_task(rproc);
#endif
    return rpmsg_virtio_get_rpmsg_device(rpmsg_vdev);
err:
#ifdef __linux__
    virtio_clear_status(rproc->rsc_table);
#endif
    remoteproc_remove_virtio(rproc, vdev);
    metal_free_memory(rpmsg_vdev);
    return NULL;
}

int platform_poll(void *priv)
{
#ifdef __linux__
    struct remoteproc *rproc = priv;
    struct remoteproc_priv *rpriv = rproc->priv;
    unsigned int flags;
    if (!priv)
        return -1;
    while(1) {
        flags = metal_irq_save_disable();
        if (!(atomic_flag_test_and_set(&rpriv->ipi_info->sync))) {
            metal_irq_restore_enable(flags);
            remoteproc_get_notification(rproc, RSC_NOTIFY_ID_ANY);
            break;
        }
        _rproc_wait();
        metal_irq_restore_enable(flags);
    }
#else
    (void) priv;
#endif
    return 0;
}

void platform_release_rpmsg_vdev(void *platform, struct rpmsg_device *rpdev)
{
    /* Need to free memory regions already allocated but not used anymore? */
    struct remoteproc *rproc = platform;
    struct rpmsg_virtio_device *rpmsg_vdev;
#ifdef __linux__
    virtio_clear_status(rproc->rsc_table);
#endif
    rpmsg_vdev = metal_container_of(rpdev, struct rpmsg_virtio_device, rdev);
    rpmsg_deinit_vdev(rpmsg_vdev);
    remoteproc_remove_virtio(rproc, rpmsg_vdev->vdev);
    metal_free_memory(rpmsg_vdev);
}

void platform_cleanup(void *platform)
{
    struct remoteproc *rproc = platform;
    struct remoteproc_priv *prproc;
    struct metal_device *dev;

    if (rproc){
        (void)remoteproc_remove(rproc);
        prproc = rproc->priv;
        if (rproc->priv) {
            /* Release allocated resource */
            /* Shared memory devices */
            metal_list_del(&(prproc->vring->ctl.mem.node));
            dev = prproc->vring->ctl.dev;
            if (dev)
                metal_device_close(dev);
            metal_list_del(&(prproc->vring->shm.mem.node));
            dev = prproc->vring->shm.dev;
            if (dev)
                metal_device_close(dev);

            /* Resource table device */
            metal_list_del(&(prproc->vring->rsc.mem.node));
            dev = prproc->vring->rsc.dev;
            if (dev)
                metal_device_close(dev);
        }
        
        if (prproc) {
            metal_free_memory(prproc);
        }
        metal_free_memory(rproc);
    }
}

#ifndef __linux__
static void IpiTask(VP_INT exinf)
{
    struct remoteproc *rproc = exinf;
    struct remoteproc_priv *prproc = rproc->priv;

    while (1) {
        wai_sem(prproc->ipi_info->ipi_sem_id);

        /* Ignore a coming interrupt if the virtio layer of a target remoteproc is not yet initialized */
        if (metal_list_is_empty(&rproc->vdevs)) {
            continue;
        }

        (void)remoteproc_get_notification(rproc, RSC_NOTIFY_ID_ANY);
    }
}

static void prepare_ipi_sem(void *platform) {
    struct remoteproc *rproc = platform;
    struct remoteproc_priv *prproc = rproc->priv;
    const T_CSEM csem_ipi = { TA_TFIFO, 0, 1 };
    prproc->ipi_info->ipi_sem_id = acre_sem((T_CSEM*)&csem_ipi);
}

static void start_ipi_task(void *platform)
{
    struct remoteproc *rproc = platform;
    struct remoteproc_priv *prproc = rproc->priv;

    T_CTSK ctsk_ipitask;
    ctsk_ipitask.tskatr = (TA_HLNG | TA_FPU | TA_ACT);
    ctsk_ipitask.name = "metal_ipi_tsk";
    ctsk_ipitask.itskpri = 2;
    ctsk_ipitask.stksz   = 0x1000U;
    ctsk_ipitask.stk     = NULL;
    ctsk_ipitask.task =  (FP)IpiTask;
    ctsk_ipitask.exinf = (VP_INT)platform;
    ipi_tsk_id[prproc->ipi_info->notify_id] = acre_tsk((T_CTSK*)&ctsk_ipitask);

    if (ipi_tsk_id[prproc->ipi_info->notify_id] <= 0) {
        LPRINTF("Failed to register an interrupt service routine.\n");
    }
}
#endif
