/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 *
 * Copyright (C) 2015 Xilinx, Inc.  All rights reserved.
 * Copyright (c) 2020-2025, eForce Co., Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef RSC_TABLE_H_
#define RSC_TABLE_H_

#include <stddef.h>
#include "openamp/open_amp.h"
#include "platform_info.h"

/* Place resource table in special ELF section */
#if defined(__CC_ARM) || defined(__GNUC__)
#define __section_t(S)          __attribute__((__section__(#S)))
#define __resource              __section_t(.resource_table)
#else
#define __resource
#endif

#if defined (USE_UC3_SINGLE_SAMPLE) || defined (USE_UC3_DUAL_SAMPLE)
#define NUM_RSCTBLS             (2U)
#elif defined (USE_UC3_TRIPLE_SAMPLE)
#define NUM_RSCTBLS             (3U)
#else
#error "Unknown uC3 sample type definition specified!"
#endif

#define RPMSG_IPU_C0_FEATURES   (1U)

/* VirtIO rpmsg device id */
#define VIRTIO_ID_RPMSG_        (7U)

#define NUM_VRINGS              (2U)
#define NUM_TABLE_ENTRIES       (1U)
#define NO_RESOURCE_ENTRIES     (1U)

/* Resource table UIO device */
#define CFG_RSCTBL_DEV_NAME     "98800000.rsctbl"
#define CFG_RSCTBL_MEM_PA       (0x98800000U)
#define CFG_RSCTBL_MAP_SIZE     (0x00001000U)


#if defined __cplusplus
extern "C" {
#endif
  
#if defined(__CC_ARM) || defined(__GNUC__)
/* Resource table for the given remote */
METAL_PACKED_BEGIN
struct remote_resource_table {
    unsigned int version;
    unsigned int num;
    unsigned int reserved[2];
    unsigned int offset[NO_RESOURCE_ENTRIES];
    /* rpmsg vdev entry */
    struct fw_rsc_vdev rpmsg_vdev;
    struct fw_rsc_vdev_vring rpmsg_vring0;
    struct fw_rsc_vdev_vring rpmsg_vring1;
} METAL_PACKED_END;

#elif __ICCARM__
/* Workaround for IAR compiler
 * IAR compiler does not allow array of zero length in struct definition
 * so the definition of fw_rsc_vdev must be overriden.
 */
METAL_PACKED_BEGIN
struct my_fw_rsc_vdev {
    uint32_t type;
    uint32_t id;
    uint32_t notifyid;
    uint32_t dfeatures;
    uint32_t gfeatures;
    uint32_t config_len;
    uint8_t status;
    uint8_t num_of_vrings;
    uint8_t reserved[2];
    struct fw_rsc_vdev_vring vring[NUM_VRINGS];
} METAL_PACKED_END;

/* Resource table for the given remote */
METAL_PACKED_BEGIN
struct remote_resource_table {
    unsigned int version;
    unsigned int num;
    unsigned int reserved[2];
    unsigned int offset[NO_RESOURCE_ENTRIES];
    /* rpmsg vdev entry */
    struct my_fw_rsc_vdev rpmsg_vdev;
} METAL_PACKED_END;
#endif


#if defined __cplusplus
}
#endif

#ifndef __linux__ /* uC3 */
extern void resource_table_init(void);
extern void *get_resource_table (int rsc_id, size_t *len);
#endif

#endif /* RSC_TABLE_H_ */
