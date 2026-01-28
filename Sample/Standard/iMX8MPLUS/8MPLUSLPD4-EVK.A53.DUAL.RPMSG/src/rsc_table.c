/*
 * Copyright (c) 2014, Mentor Graphics Corporation
 * All rights reserved.
 * Copyright (c) 2015 Xilinx, Inc. All rights reserved.
 * Copyright (c) 2020-2025, eForce Co., Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "rsc_table.h"

extern struct remote_resource_table rsc_table[NUM_RSCTBLS];

struct remote_resource_table resources[NUM_RSCTBLS] = {
    {
        /* Version */
        1U,

        /* NUmber of table entries */
        NUM_TABLE_ENTRIES,
        /* reserved fields */
        {0U, 0U},

        /* Offsets of rsc entries */
        {
            offsetof(struct remote_resource_table, rpmsg_vdev)
        },
#ifdef __ICCARM__
        /* Virtio device entry */
        {
            RSC_VDEV, VIRTIO_ID_RPMSG_, VRING_NOTIFYID0, RPMSG_IPU_C0_FEATURES, 0U, 0U, 0U,
            NUM_VRINGS, {0U, 0U},
            {
                /* Vring rsc entry - part of vdev rsc entry */
                {CFG_VRING0_BASE0, CFG_VRING_ALIGN0, CFG_RPMSG_NUM_BUFS0, VRING_NOTIFYID0, 0U},
                {CFG_VRING1_BASE0, CFG_VRING_ALIGN0, CFG_RPMSG_NUM_BUFS0, VRING_NOTIFYID0, 0U}
            }
        }
#else
        /* Virtio device entry */
        {
            RSC_VDEV, VIRTIO_ID_RPMSG_, VRING_NOTIFYID0, RPMSG_IPU_C0_FEATURES, 0U, 0U, 0U,
            NUM_VRINGS, {0U, 0U},
        },

        /* Vring rsc entry - part of vdev rsc entry */
        {CFG_VRING0_BASE0, CFG_VRING_ALIGN0, CFG_RPMSG_NUM_BUFS0, VRING_NOTIFYID0, 0U},
        {CFG_VRING1_BASE0, CFG_VRING_ALIGN0, CFG_RPMSG_NUM_BUFS0, VRING_NOTIFYID0, 0U}
#endif
    },
    {
        /* Version */
        1U,

        /* NUmber of table entries */
        NUM_TABLE_ENTRIES,
        /* reserved fields */
        {0U, 0U},

        /* Offsets of rsc entries */
        {
            offsetof(struct remote_resource_table, rpmsg_vdev)
        },
#ifdef __ICCARM__
        /* Virtio device entry */
        {
            RSC_VDEV, VIRTIO_ID_RPMSG_, VRING_NOTIFYID1, RPMSG_IPU_C0_FEATURES, 0U, 0U, 0U,
            NUM_VRINGS, {0U, 0U},
            {
                /* Vring rsc entry - part of vdev rsc entry */
                {CFG_VRING0_BASE1, CFG_VRING_ALIGN1, CFG_RPMSG_NUM_BUFS1, VRING_NOTIFYID1, 0U},
                {CFG_VRING1_BASE1, CFG_VRING_ALIGN1, CFG_RPMSG_NUM_BUFS1, VRING_NOTIFYID1, 0U}
            }
        }
#else
        /* Virtio device entry */
        {
            RSC_VDEV, VIRTIO_ID_RPMSG_, VRING_NOTIFYID1, RPMSG_IPU_C0_FEATURES, 0U, 0U, 0U,
            NUM_VRINGS, {0U, 0U},
        },

        /* Vring rsc entry - part of vdev rsc entry */
        {CFG_VRING0_BASE1, CFG_VRING_ALIGN1, CFG_RPMSG_NUM_BUFS1, VRING_NOTIFYID1, 0U},
        {CFG_VRING1_BASE1, CFG_VRING_ALIGN1, CFG_RPMSG_NUM_BUFS1, VRING_NOTIFYID1, 0U}
#endif
    },
#if (NUM_RSCTBLS >= 3U)
    {
        /* Version */
        1U,

        /* NUmber of table entries */
        NUM_TABLE_ENTRIES,
        /* reserved fields */
        {0U, 0U},

        /* Offsets of rsc entries */
        {
            offsetof(struct remote_resource_table, rpmsg_vdev)
        },
#ifdef __ICCARM__
        /* Virtio device entry */
        {
            RSC_VDEV, VIRTIO_ID_RPMSG_, VRING_NOTIFYID2, RPMSG_IPU_C0_FEATURES, 0U, 0U, 0U,
            NUM_VRINGS, {0U, 0U},
            {
                /* Vring rsc entry - part of vdev rsc entry */
                {CFG_VRING0_BASE2, CFG_VRING_ALIGN2, CFG_RPMSG_NUM_BUFS2, VRING_NOTIFYID2, 0U},
                {CFG_VRING1_BASE2, CFG_VRING_ALIGN2, CFG_RPMSG_NUM_BUFS2, VRING_NOTIFYID2, 0U}
            }
        }
#else
        /* Virtio device entry */
        {
            RSC_VDEV, VIRTIO_ID_RPMSG_, VRING_NOTIFYID2, RPMSG_IPU_C0_FEATURES, 0U, 0U, 0U,
            NUM_VRINGS, {0U, 0U},
        },

        /* Vring rsc entry - part of vdev rsc entry */
        {CFG_VRING0_BASE2, CFG_VRING_ALIGN2, CFG_RPMSG_NUM_BUFS2, VRING_NOTIFYID2, 0U},
        {CFG_VRING1_BASE2, CFG_VRING_ALIGN2, CFG_RPMSG_NUM_BUFS2, VRING_NOTIFYID2, 0U}
#endif
    },
#endif /* (NUM_RSCTBLS >= 3U) */
};

void *get_resource_table (int rsc_id, size_t *len)
{
    *len = sizeof(rsc_table[rsc_id]);
    return (void *)&rsc_table[rsc_id];
}
