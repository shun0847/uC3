/*
 * Copyright (c) 2016 Xilinx, Inc. All rights reserved.
 * Copyright (c) 2020-2025, eForce Co., Ltd. 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLATFORM_INFO_H_
#define PLATFORM_INFO_H_

#include <openamp/remoteproc.h>
#include <openamp/virtio.h>
#include <openamp/rpmsg.h>
#include "ampext_shmem_cfg.h"
#include "OpenAMP_RPMsg_cfg.h"
#if defined __cplusplus
extern "C" {
#endif

#ifdef __linux__
#define LPRINTF(format, ...)  printf(format, ##__VA_ARGS__)
#define LPERROR(format, ...)  LPRINTF("ERROR: " format, ##__VA_ARGS__)
#define _rproc_wait()         (sched_yield())
#else
#define LPRINTF(format, ...)
#define LPERROR(format, ...)
#define _rproc_wait()
#endif

#ifdef __linux__
#define DEV_BUS_NAME         "platform"
#define IPI0_INT_NUM         (7U)
#define IPI1_INT_NUM         (9U)
#define IPI2_INT_NUM         (10U)
#else
#define DEV_BUS_NAME         "generic"
#define IPI0_INT_NUM         (7U)
#define IPI1_INT_NUM         (9U)
#define IPI2_INT_NUM         (10U)
#endif

/* Remoteproc private data struct */
struct ipi_info {
    int registered;
    unsigned int intno;
    unsigned int notify_id;
    unsigned int cpu_id;
#ifdef __linux__
    atomic_flag sync;
    int sendsgi_fd;
#else
    ID ipi_sem_id;
#endif
};

struct shm_info {
    const char *name;
    const char *bus_name;
    struct metal_device *dev; /**< pointer to shared memory device */
    struct metal_io_region *io; /**< pointer to shared memory i/o region */
    struct remoteproc_mem mem; /**< shared memory */
};

struct vring_info {
    struct shm_info rsc;
    struct shm_info ctl;
    struct shm_info shm;
};

struct remoteproc_priv {
    struct vring_info *vring;
    struct ipi_info *ipi_info;
};

/**
 * platform_init - initialize the platform
 *
 * It will initialize the platform.
 *
 * @proc_id: processor id
 * @rsc_id: resource id
 * @platform: pointer to store the platform data pointer
 *
 * return 0 for success or negative value for failure
 */
int platform_init(unsigned long proc_id, unsigned long rsc_id, void **platform);

/**
 * platform_create_rpmsg_vdev - create rpmsg vdev
 *
 * It will create rpmsg virtio device, and returns the rpmsg virtio
 * device pointer.
 *
 * @platform: pointer to the private data
 * @vdev_index: index of the virtio device, there can more than one vdev
 *              on the platform.
 * @role: virtio master or virtio slave of the vdev
 * @rst_cb: virtio device reset callback
 * @ns_bind_cb: rpmsg name service bind callback
 *
 * return pointer to the rpmsg virtio device
 */
struct rpmsg_device *
platform_create_rpmsg_vdev(void *platform, unsigned int vdev_index,
               unsigned int role,
               void (*rst_cb)(struct virtio_device *vdev),
               rpmsg_ns_bind_cb ns_bind_cb);

/**
 * platform_poll - platform poll function
 *
 * @platform: pointer to the platform
 *
 * return negative value for errors, otherwise 0.
 */
int platform_poll(void *platform);

/**
 * platform_release_rpmsg_vdev - release rpmsg virtio device
 *
 * @platform: pointer to the platform
 * @rpdev: pointer to the rpmsg device
 */
void platform_release_rpmsg_vdev(void *platform, struct rpmsg_device *rpdev);

/**
 * platform_cleanup - clean up the platform resource
 *
 * @platform: pointer to the platform
 */
void platform_cleanup(void *platform);

/**
 * platform_config - Configure the remote core
 *
 * @platform: pointer to the platform
 */
int platform_config(void *platform);

/**
 * platform_load_fw - Load the remote firmware
 *
 * @platform: pointer to the platform
 * @image: pointer to the firmware image
 * @store_ops: image_store_ops to use
 */
int platform_load_fw(void *platform, void *image, struct image_store_ops *store_ops);

/**
 * platform_start - Start the remote core
 *
 * @platform: pointer to the platform
 */
int platform_start(void *platform);

/**
 * platform_stop - Stop the remote core
 *
 * @platform: pointer to the platform
 */
int platform_stop(void *platform);
/**
 * platform_shutdown - Shutdown the remote core
 *
 * @platform: pointer to the platform
 */
int platform_shutdown(void *platform);

#if defined __cplusplus
}
#endif

#endif /* PLATFORM_INFO_H_ */
