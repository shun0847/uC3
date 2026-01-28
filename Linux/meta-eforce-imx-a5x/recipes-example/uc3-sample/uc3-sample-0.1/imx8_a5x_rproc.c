/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 * Copyright (c) 2017 Xilinx, Inc.
 * Copyright (c) 2020-2025, eForce Co., Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <metal/atomic.h>
#include <metal/assert.h>
#include <metal/device.h>
#include <metal/irq.h>
#include <metal/utilities.h>
#include <openamp/rpmsg_virtio.h>
#include "platform_info.h"

#ifdef __linux__
#define SYSFS_DIR         "/sys/kernel/cpuctl/"
#define TARGET_SENDSGI    (SYSFS_DIR "sendsgi")
#else
#include "DDR_AArch64_GICv3.h"
#endif


static int imx8_a5x_proc_irq_handler(int vect_id, void *data)
{
    struct remoteproc *rproc = data;
    struct remoteproc_priv *prproc;
    (void)vect_id;

    if (!rproc)
        return METAL_IRQ_NOT_HANDLED;

    prproc = rproc->priv;
#ifdef __linux__
    atomic_flag_clear(&prproc->ipi_info->sync);
#else
    if (prproc->ipi_info->ipi_sem_id != E_ID) {
        isig_sem(prproc->ipi_info->ipi_sem_id);
    }
    else
        return METAL_IRQ_NOT_HANDLED;
#endif
    return METAL_IRQ_HANDLED;
}

static int init_memory_device(struct remoteproc *rproc, struct shm_info *info){
    struct metal_device *dev;
    metal_phys_addr_t mem_pa;
    int irq_vect;
    int ret;

    ret = metal_device_open(info->bus_name, info->name, &dev);
    if (ret) {
        LPRINTF("Failed to open uio device %s: %d.\n", info->name, ret);
        goto err;
    }

    info->dev = dev;
    info->io = metal_device_io_region(dev, 0U);
    if (!info->io) {
        goto err;
    }
    /* register IPI */
    if (info->dev->irq_num == 1) {
        irq_vect = (int)((uintptr_t)info->dev->irq_info);
        if (irq_vect > 0) {
            /* Register interrupt handler and enable interrupt */
            ret = metal_irq_register(irq_vect, imx8_a5x_proc_irq_handler, rproc);
            if(ret != 0){
                LPRINTF("metal_irq_register failed, ret=%d\n",ret);
                goto err;
            };
        
            metal_irq_enable((unsigned int)irq_vect);
        }
    }

    mem_pa = metal_io_phys(info->io, 0U);
    remoteproc_init_mem(&info->mem, info->name, mem_pa, mem_pa,
                metal_io_region_size(info->io),
                info->io);
    remoteproc_add_mem(rproc, &info->mem);
    LPRINTF("Successfully added memory device %s.\n", info->name);

    return 0;
err:
    if(info->dev) {
        metal_device_close(info->dev);
        info->dev = NULL;
    }
    return ret;
}

static struct remoteproc *
imx8_a5x_proc_init(struct remoteproc *rproc,
            struct remoteproc_ops *ops, void *arg)
{
    struct remoteproc_priv *prproc = arg;
#ifdef __linux__
    char lbuf[32] = { 0 };
    int len = 0;
    int ret;
#endif

    if ((!rproc) || (!prproc) || (!ops))
        return NULL;

    rproc->priv = prproc;
    rproc->ops = ops;

    if (!prproc->ipi_info->registered) {
#ifdef __linux__
        prproc->ipi_info->sendsgi_fd = open(TARGET_SENDSGI, O_WRONLY|O_SYNC);
        if (prproc->ipi_info->sendsgi_fd < 0) {
            ret = system("modprobe cpuctl > /dev/null 2>&1");
            if (ret < 0) {
                metal_log(METAL_LOG_WARNING, "%s: executing system command 'modprobe cpuctl > /dev/null 2>&1' failed.\n", __func__);
                goto err1;
            }
            prproc->ipi_info->sendsgi_fd = open(TARGET_SENDSGI, O_WRONLY|O_SYNC);
            if (prproc->ipi_info->sendsgi_fd < 0)
                goto err1;
        }
        atomic_flag_test_and_set(&prproc->ipi_info->sync);
#endif
        prproc->ipi_info->registered++;
    }
    /* Get the resource table device */
    if(init_memory_device(rproc, &prproc->vring->rsc))
        goto err1;
    /* Get VRING related devices */
    if(init_memory_device(rproc, &prproc->vring->ctl))
        goto err1;
    if(init_memory_device(rproc, &prproc->vring->shm))
        goto err1;
    return rproc;
err1:
#ifdef __linux__
    if (prproc->ipi_info->sendsgi_fd >= 0) {
        close(prproc->ipi_info->sendsgi_fd);
    }
#endif
    return NULL;
}

static void imx8_a5x_proc_remove(struct remoteproc *rproc)
{
    struct remoteproc_priv *prproc;

    if (!rproc)
        return;
    prproc = rproc->priv;

    /* Release allocated resource */
    if (prproc->ipi_info->registered) {
#ifdef __linux__
        close(prproc->ipi_info->sendsgi_fd);
#endif
        prproc->ipi_info->registered--;
    }
    if(prproc->ipi_info->registered > 0)
        return ;

    (void)metal_irq_unregister((uintptr_t)prproc->vring->ctl.dev->irq_info);
    prproc->ipi_info->registered = 0;
}

static int imx8_a5x_proc_notify(struct remoteproc *rproc, uint32_t id)
{
    struct remoteproc_priv *prproc;
#ifdef __linux__
    char lbuf[32] = { 0 };
    int len = 0;
#endif
    if (!rproc)
        return -1;
    prproc = rproc->priv;
#ifdef __linux__
    len = snprintf(lbuf, 32, "%llx\n", ((prproc->ipi_info->intno << 24) | (1 << prproc->ipi_info->cpu_id))); 
    write(prproc->ipi_info->sendsgi_fd, lbuf, len);
    fsync(prproc->ipi_info->sendsgi_fd);
#else
    ddr_aarch64_gicv3_send_sgi((1U << (prproc->ipi_info->cpu_id+ID_CORE0)), prproc->ipi_info->intno);
#endif
    return 0;
}


#ifdef __linux__
static void *
imx8_a5x_proc_mmap(struct remoteproc *rproc, metal_phys_addr_t *pa,
            metal_phys_addr_t *da, size_t size,
            unsigned int attribute, struct metal_io_region **io)
{
    metal_phys_addr_t lpa, lda;
    struct metal_io_region *tmpio;
    struct remoteproc_priv *prproc;
    (void)attribute;
    (void)size;

    if (!rproc)
        return NULL;
    prproc = rproc->priv;

    lpa = *pa;
    lda = *da;

    if (lpa == METAL_BAD_PHYS && lda == METAL_BAD_PHYS)
        return NULL;
    if (lpa == METAL_BAD_PHYS)
        lpa = lda;
    if (lda == METAL_BAD_PHYS)
        lda = lpa;
    tmpio = prproc->vring->rsc.io; /* We consider the resource table device only */
    if (!tmpio)
        return NULL;

    *pa = lpa;
    *da = lda;
    if (io)
        *io = tmpio;
    return metal_io_phys_to_virt(tmpio, lpa);
}
#endif

/* processor operations from a5x to a5x. It defines
 * notification operation and remote processor managementi operations. */
struct remoteproc_ops imx8_a5x_proc_ops = {
    .init = imx8_a5x_proc_init,
    .remove = imx8_a5x_proc_remove,
#ifdef __linux__
    .mmap = imx8_a5x_proc_mmap,
#else
    .mmap = NULL,
#endif
    .notify = imx8_a5x_proc_notify,
    .start = NULL,
    .stop = NULL,
    .shutdown = NULL,
};
