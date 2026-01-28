/**
 * @file    sys_init.c
 * @brief   Metal device definitions
 * @date    2025.12.19
 * @author  Copyright (c) 2025, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2025.01.21)
 *            First release for iMX8MPlus A53
 *          - rev 1.1 (2025.12.19)
 *            Added array for multi core sample.
 ****************************************************************************
 */

#include "metal/io.h"
#include "metal/device.h"
#include "metal/sys.h"
#include "kernel.h"
#include "imx8mplus_uC3.h"
#include "rsc_table.h"
#include "platform_info.h"

#define DEFAULT_PAGE_SHIFT (-1UL)
#define DEFAULT_PAGE_MASK (-1UL)

#if (CFG_RPMSG_SVCNO == 2U)
const metal_phys_addr_t metal_phys[] = {
    CFG_RSCTBL_MEM_PA,
    CFG_VRING0_BASE0,
    CFG_VRING0_BASE1,
    CFG_VRING_SHM_BASE0,
    CFG_VRING_SHM_BASE1,
    CFG_SHMEM_CTL_BASE0,
    CFG_SHMEM_CTL_BASE1,
    CFG_SHMEM_BLK_BASE0,
    CFG_SHMEM_BLK_BASE1
};

struct metal_device metal_dev_table[] = {

    /* Resource table */
    {
        CFG_RSCTBL_DEV_NAME,
        NULL,
        1U,
        {
            {
                (void *)(CFG_RSCTBL_MEM_PA),
                &metal_phys[0],
                CFG_RSCTBL_MAP_SIZE,
                DEFAULT_PAGE_SHIFT,
                DEFAULT_PAGE_MASK,
                0U,
                {NULL},
            }
        },
        {NULL},
        0,
        NULL,
    },
    /* RPMSG (vring) */
    { /* vring #0 CTL */
        CFG_VRING_CTL_NAME0,
        NULL,
        1U,
        {
            {
                (void *)(CFG_VRING0_BASE0),
                &metal_phys[1],
                CFG_VRING_CTL_SIZE0,
                DEFAULT_PAGE_SHIFT,
                DEFAULT_PAGE_MASK,
                0U,
                {NULL},
            }
        },
        {NULL},
        1,
        (void *)IPI0_INT_NUM,
    },
    { /* vring #1 CTL */
        CFG_VRING_CTL_NAME1,
        NULL,
        1U,
        {
            {
                (void *)(CFG_VRING0_BASE1),
                &metal_phys[2],
                CFG_VRING_CTL_SIZE1,
                DEFAULT_PAGE_SHIFT,
                DEFAULT_PAGE_MASK,
                0U,
                {NULL},
            }
        },
        {NULL},
        1,
        (void *)IPI1_INT_NUM,
    },
    { /* vring #0 SHM */
        CFG_VRING_SHM_NAME0,
        NULL,
        1U,
        {
            {
                (void *)(CFG_VRING_SHM_BASE0),
                &metal_phys[3],
                CFG_VRING_SHM_SIZE0,
                DEFAULT_PAGE_SHIFT,
                DEFAULT_PAGE_MASK,
                0U,
                {NULL},
            }
        },
        {NULL},
        0,
        NULL,
    },
    { /* vring #1 SHM */
        CFG_VRING_SHM_NAME1,
        NULL,
        1U,
        {
            {
                (void *)(CFG_VRING_SHM_BASE1),
                &metal_phys[4],
                CFG_VRING_SHM_SIZE1,
                DEFAULT_PAGE_SHIFT,
                DEFAULT_PAGE_MASK,
                0U,
                {NULL},
            }
        },
        {NULL},
        0,
        NULL,
    },
    /* Share memory API */
    { /* Shared memory API #0 CTL */
        CFG_SHMEM_CTL_NAME0,
        NULL,
        1U,
        {
            {
                (void *)(CFG_SHMEM_CTL_BASE0),
                &metal_phys[5],
                CFG_SHMEM_CTL_SIZE0,
                DEFAULT_PAGE_SHIFT,
                DEFAULT_PAGE_MASK,
                0U,
                {NULL},
            }
        },
        {NULL},
        0,
        NULL,
    },
    { /* Shared memory API #1 CTL */
        CFG_SHMEM_CTL_NAME1,
        NULL,
        1U,
        {
            {
                (void *)(CFG_SHMEM_CTL_BASE1),
                &metal_phys[6],
                CFG_SHMEM_CTL_SIZE1,
                DEFAULT_PAGE_SHIFT,
                DEFAULT_PAGE_MASK,
                0U,
                {NULL},
            }
        },
        {NULL},
        0,
        NULL,
    },
    { /* Shared memory API #0 BLK */
        CFG_SHMEM_BLK_NAME0,
        NULL,
        1U,
        {
            {
                (void *)(CFG_SHMEM_BLK_BASE0),
                &metal_phys[7],
                CFG_SHMEM_BLK_SIZE0,
                DEFAULT_PAGE_SHIFT,
                DEFAULT_PAGE_MASK,
                0U,
                {NULL},
            }
        },
        {NULL},
        0,
        NULL,
    },
    { /* Shared memory API #1 BLK */
        CFG_SHMEM_BLK_NAME1,
        NULL,
        1U,
        {
            {
                (void *)(CFG_SHMEM_BLK_BASE1),
                &metal_phys[8],
                CFG_SHMEM_BLK_SIZE1,
                DEFAULT_PAGE_SHIFT,
                DEFAULT_PAGE_MASK,
                0U,
                {NULL},
            }
        },
        {NULL},
        0,
        NULL,
    },
};
#elif (CFG_RPMSG_SVCNO == 3U)
const metal_phys_addr_t metal_phys[] = {
    CFG_RSCTBL_MEM_PA,
    CFG_VRING0_BASE0,
    CFG_VRING0_BASE1,
    CFG_VRING0_BASE2,
    CFG_VRING_SHM_BASE0,
    CFG_VRING_SHM_BASE1,
    CFG_VRING_SHM_BASE2,
    CFG_SHMEM_CTL_BASE0,
    CFG_SHMEM_CTL_BASE1,
    CFG_SHMEM_CTL_BASE2,
    CFG_SHMEM_BLK_BASE0,
    CFG_SHMEM_BLK_BASE1,
    CFG_SHMEM_BLK_BASE2
};

struct metal_device metal_dev_table[] = {

    /* Resource table */
    {
        CFG_RSCTBL_DEV_NAME,
        NULL,
        1U,
        {
            {
                (void *)(CFG_RSCTBL_MEM_PA),
                &metal_phys[0],
                CFG_RSCTBL_MAP_SIZE,
                DEFAULT_PAGE_SHIFT,
                DEFAULT_PAGE_MASK,
                0U,
                {NULL},
            }
        },
        {NULL},
        0,
        NULL,
    },
    /* RPMSG (vring) */
    { /* vring #0 CTL */
        CFG_VRING_CTL_NAME0,
        NULL,
        1U,
        {
            {
                (void *)(CFG_VRING0_BASE0),
                &metal_phys[1],
                CFG_VRING_CTL_SIZE0,
                DEFAULT_PAGE_SHIFT,
                DEFAULT_PAGE_MASK,
                0U,
                {NULL},
            }
        },
        {NULL},
        1,
        (void *)IPI0_INT_NUM,
    },
    { /* vring #1 CTL */
        CFG_VRING_CTL_NAME1,
        NULL,
        1U,
        {
            {
                (void *)(CFG_VRING0_BASE1),
                &metal_phys[2],
                CFG_VRING_CTL_SIZE1,
                DEFAULT_PAGE_SHIFT,
                DEFAULT_PAGE_MASK,
                0U,
                {NULL},
            }
        },
        {NULL},
        1,
        (void *)IPI1_INT_NUM,
    },
    { /* vring #2 CTL */
        CFG_VRING_CTL_NAME2,
        NULL,
        1U,
        {
            {
                (void *)(CFG_VRING0_BASE2),
                &metal_phys[3],
                CFG_VRING_CTL_SIZE2,
                DEFAULT_PAGE_SHIFT,
                DEFAULT_PAGE_MASK,
                0U,
                {NULL},
            }
        },
        {NULL},
        1,
        (void *)IPI2_INT_NUM,
    },
    { /* vring #0 SHM */
        CFG_VRING_SHM_NAME0,
        NULL,
        1U,
        {
            {
                (void *)(CFG_VRING_SHM_BASE0),
                &metal_phys[4],
                CFG_VRING_SHM_SIZE0,
                DEFAULT_PAGE_SHIFT,
                DEFAULT_PAGE_MASK,
                0U,
                {NULL},
            }
        },
        {NULL},
        0,
        NULL,
    },
    { /* vring #1 SHM */
        CFG_VRING_SHM_NAME1,
        NULL,
        1U,
        {
            {
                (void *)(CFG_VRING_SHM_BASE1),
                &metal_phys[5],
                CFG_VRING_SHM_SIZE1,
                DEFAULT_PAGE_SHIFT,
                DEFAULT_PAGE_MASK,
                0U,
                {NULL},
            }
        },
        {NULL},
        0,
        NULL,
    },
    { /* vring #2 SHM */
        CFG_VRING_SHM_NAME2,
        NULL,
        1U,
        {
            {
                (void *)(CFG_VRING_SHM_BASE2),
                &metal_phys[6],
                CFG_VRING_SHM_SIZE2,
                DEFAULT_PAGE_SHIFT,
                DEFAULT_PAGE_MASK,
                0U,
                {NULL},
            }
        },
        {NULL},
        0,
        NULL,
    },
    /* Share memory API */
    { /* Shared memory API #0 CTL */
        CFG_SHMEM_CTL_NAME0,
        NULL,
        1U,
        {
            {
                (void *)(CFG_SHMEM_CTL_BASE0),
                &metal_phys[7],
                CFG_SHMEM_CTL_SIZE0,
                DEFAULT_PAGE_SHIFT,
                DEFAULT_PAGE_MASK,
                0U,
                {NULL},
            }
        },
        {NULL},
        0,
        NULL,
    },
    { /* Shared memory API #1 CTL */
        CFG_SHMEM_CTL_NAME1,
        NULL,
        1U,
        {
            {
                (void *)(CFG_SHMEM_CTL_BASE1),
                &metal_phys[8],
                CFG_SHMEM_CTL_SIZE1,
                DEFAULT_PAGE_SHIFT,
                DEFAULT_PAGE_MASK,
                0U,
                {NULL},
            }
        },
        {NULL},
        0,
        NULL,
    },
    { /* Shared memory API #2 CTL */
        CFG_SHMEM_CTL_NAME2,
        NULL,
        1U,
        {
            {
                (void *)(CFG_SHMEM_CTL_BASE2),
                &metal_phys[9],
                CFG_SHMEM_CTL_SIZE2,
                DEFAULT_PAGE_SHIFT,
                DEFAULT_PAGE_MASK,
                0U,
                {NULL},
            }
        },
        {NULL},
        0,
        NULL,
    },
    { /* Shared memory API #0 BLK */
        CFG_SHMEM_BLK_NAME0,
        NULL,
        1U,
        {
            {
                (void *)(CFG_SHMEM_BLK_BASE0),
                &metal_phys[10],
                CFG_SHMEM_BLK_SIZE0,
                DEFAULT_PAGE_SHIFT,
                DEFAULT_PAGE_MASK,
                0U,
                {NULL},
            }
        },
        {NULL},
        0,
        NULL,
    },
    { /* Shared memory API #1 BLK */
        CFG_SHMEM_BLK_NAME1,
        NULL,
        1U,
        {
            {
                (void *)(CFG_SHMEM_BLK_BASE1),
                &metal_phys[11],
                CFG_SHMEM_BLK_SIZE1,
                DEFAULT_PAGE_SHIFT,
                DEFAULT_PAGE_MASK,
                0U,
                {NULL},
            }
        },
        {NULL},
        0,
        NULL,
    },
    { /* Shared memory API #2 BLK */
        CFG_SHMEM_BLK_NAME2,
        NULL,
        1U,
        {
            {
                (void *)(CFG_SHMEM_BLK_BASE2),
                &metal_phys[12],
                CFG_SHMEM_BLK_SIZE2,
                DEFAULT_PAGE_SHIFT,
                DEFAULT_PAGE_MASK,
                0U,
                {NULL},
            }
        },
        {NULL},
        0,
        NULL,
    },
};
#endif

int init_system (void)
{
    unsigned int i;
    int ret;
    struct metal_init_params metal_param = METAL_INIT_DEFAULTS;
    struct metal_device *dev;

    ret = metal_init(&metal_param);
    if (ret) {
        return ret;
    }

    /* register device */
    for (i = 0U; i < (sizeof(metal_dev_table)/sizeof(struct metal_device)); i++) {
        dev = &metal_dev_table[i];
        ret = metal_register_generic_device(dev);
        if (ret)
            break;
    }
    return ret;
}

void cleanup_system (void)
{
    metal_finish();
}
