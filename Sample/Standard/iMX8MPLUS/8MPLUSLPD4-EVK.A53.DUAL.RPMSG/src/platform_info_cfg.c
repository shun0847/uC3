/**
 * @brief   Definitions for platform information configuration.
 * @date    2025.12.19
 *
 * @copyright (C) 2025, eForce Co., Ltd.
 */
#include "platform_info.h"
#include "OpenAMP_RPMsg_cfg.h"
#include "rsc_table.h"

#if defined (USE_UC3_SINGLE_SAMPLE)
/* IPI information */
struct ipi_info ipi[CFG_RPMSG_SVCNO] = {
    {
        0,                                  // registered
        IPI0_INT_NUM,                       // intnum
        VRING_NOTIFYID0,                    // notify_id
#ifdef __linux__                            /* linux */
        3U,                                 // cpuid
        ATOMIC_FLAG_INIT,                   // sync
        -1                                  // sendsgi_fd
#else                                       /* uC3 */
        0U,                                 // cpuid
        E_ID,                               // ipi_sem_id
#endif
    },
    {
        0,                                  // registered
        IPI1_INT_NUM,                       // intnum
        VRING_NOTIFYID1,                    // notify_id
#ifdef __linux__                            /* linux */
        3U,                                 // cpuid
        ATOMIC_FLAG_INIT,                   // sync
        -1,                                 // sendsgi_fd
#else                                       /* uC3 */
        0U,                                 // cpuid
        E_ID,                               // ipi_sem_id
#endif
    },
};

/* vring information */
struct vring_info vrinfo[CFG_RPMSG_SVCNO] = {
    { // vinfo[0]
        { // rsc
            CFG_RSCTBL_DEV_NAME, // name
            DEV_BUS_NAME, // bus_name
            NULL, // dev
            NULL, // io
            { 0 }, // mem
        },
        { // ctl
            CFG_VRING_CTL_NAME0, // name
            DEV_BUS_NAME, // bus_name
            NULL, // dev
            NULL, // io
            { 0 }, // mem
        },
        { // shm
            CFG_VRING_SHM_NAME0, // name
            DEV_BUS_NAME, // bus_name
            NULL, // dev
            NULL, // io
            { 0 }, // mem
        },
    },
    { // vinfo[1]
        { // rsc
            CFG_RSCTBL_DEV_NAME, // name
            DEV_BUS_NAME, // bus_name
            NULL, // dev
            NULL, // io
            { 0 }, // mem
        },
        { // ctl
            CFG_VRING_CTL_NAME1, // name
            DEV_BUS_NAME, // bus_name
            NULL, // dev
            NULL, // io
            { 0 }, // mem
        },
        { // shm
            CFG_VRING_SHM_NAME1, // name
            DEV_BUS_NAME, // bus_name
            NULL, // dev
            NULL, // io
            { 0 }, // mem
        },
    },
};
#elif defined (USE_UC3_DUAL_SAMPLE)
/* IPI information */
struct ipi_info ipi[CFG_RPMSG_SVCNO] = {
    {
        0,                                  // registered
        IPI0_INT_NUM,                       // intnum
        VRING_NOTIFYID0,                    // notify_id
#ifdef __linux__                            /* linux */
        3U,                                 // cpuid
        ATOMIC_FLAG_INIT,                   // sync
        -1                                  // sendsgi_fd
#else                                       /* uC3 */
        0U,                                 // cpuid
        E_ID,                               // ipi_sem_id
#endif
    },
    {
        0,                                  // registered
        IPI1_INT_NUM,                       // intnum
        VRING_NOTIFYID1,                    // notify_id
#ifdef __linux__                            /* linux */
        2U,                                 // cpuid
        ATOMIC_FLAG_INIT,                   // sync
        -1,                                 // sendsgi_fd
#else                                       /* uC3 */
        0U,                                 // cpuid
        E_ID,                               // ipi_sem_id
#endif
    },
};

/* vring information */
struct vring_info vrinfo[CFG_RPMSG_SVCNO] = {
    { // vinfo[0]
        { // rsc
            CFG_RSCTBL_DEV_NAME, // name
            DEV_BUS_NAME, // bus_name
            NULL, // dev
            NULL, // io
            { 0 }, // mem
        },
        { // ctl
            CFG_VRING_CTL_NAME0, // name
            DEV_BUS_NAME, // bus_name
            NULL, // dev
            NULL, // io
            { 0 }, // mem
        },
        { // shm
            CFG_VRING_SHM_NAME0, // name
            DEV_BUS_NAME, // bus_name
            NULL, // dev
            NULL, // io
            { 0 }, // mem
        },
    },
    { // vinfo[1]
        { // rsc
            CFG_RSCTBL_DEV_NAME, // name
            DEV_BUS_NAME, // bus_name
            NULL, // dev
            NULL, // io
            { 0 }, // mem
        },
        { // ctl
            CFG_VRING_CTL_NAME1, // name
            DEV_BUS_NAME, // bus_name
            NULL, // dev
            NULL, // io
            { 0 }, // mem
        },
        { // shm
            CFG_VRING_SHM_NAME1, // name
            DEV_BUS_NAME, // bus_name
            NULL, // dev
            NULL, // io
            { 0 }, // mem
        },
    },
};
#elif defined (USE_UC3_TRIPLE_SAMPLE)
/* IPI information */
struct ipi_info ipi[CFG_RPMSG_SVCNO] = {
    {
        0,                                  // registered
        IPI0_INT_NUM,                       // intnum
        VRING_NOTIFYID0,                    // notify_id
#ifdef __linux__                            /* linux */
        3U,                                 // cpuid
        ATOMIC_FLAG_INIT,                   // sync
        -1                                  // sendsgi_fd
#else                                       /* uC3 */
        0U,                                 // cpuid
        E_ID,                               // ipi_sem_id
#endif
    },
    {
        0,                                  // registered
        IPI1_INT_NUM,                       // intnum
        VRING_NOTIFYID1,                    // notify_id
#ifdef __linux__                            /* linux */
        2U,                                 // cpuid
        ATOMIC_FLAG_INIT,                   // sync
        -1,                                 // sendsgi_fd
#else                                       /* uC3 */
        0U,                                 // cpuid
        E_ID,                               // ipi_sem_id
#endif
    },
    {
        0,                                  // registered
        IPI2_INT_NUM,                       // intnum
        VRING_NOTIFYID2,                    // notify_id
#ifdef __linux__                            /* linux */
        1U,                                 // cpuid
        ATOMIC_FLAG_INIT,                   // sync
        -1,                                 // sendsgi_fd
#else                                       /* uC3 */
        0U,                                 // cpuid
        E_ID,                               // ipi_sem_id
#endif
    },
};

/* vring information */
struct vring_info vrinfo[CFG_RPMSG_SVCNO] = {
    { // vinfo[0]
        { // rsc
            CFG_RSCTBL_DEV_NAME, // name
            DEV_BUS_NAME, // bus_name
            NULL, // dev
            NULL, // io
            { 0 }, // mem
        },
        { // ctl
            CFG_VRING_CTL_NAME0, // name
            DEV_BUS_NAME, // bus_name
            NULL, // dev
            NULL, // io
            { 0 }, // mem
        },
        { // shm
            CFG_VRING_SHM_NAME0, // name
            DEV_BUS_NAME, // bus_name
            NULL, // dev
            NULL, // io
            { 0 }, // mem
        },
    },
    { // vinfo[1]
        { // rsc
            CFG_RSCTBL_DEV_NAME, // name
            DEV_BUS_NAME, // bus_name
            NULL, // dev
            NULL, // io
            { 0 }, // mem
        },
        { // ctl
            CFG_VRING_CTL_NAME1, // name
            DEV_BUS_NAME, // bus_name
            NULL, // dev
            NULL, // io
            { 0 }, // mem
        },
        { // shm
            CFG_VRING_SHM_NAME1, // name
            DEV_BUS_NAME, // bus_name
            NULL, // dev
            NULL, // io
            { 0 }, // mem
        },
    },
    { // vinfo[2]
        { // rsc
            CFG_RSCTBL_DEV_NAME, // name
            DEV_BUS_NAME, // bus_name
            NULL, // dev
            NULL, // io
            { 0 }, // mem
        },
        { // ctl
            CFG_VRING_CTL_NAME2, // name
            DEV_BUS_NAME, // bus_name
            NULL, // dev
            NULL, // io
            { 0 }, // mem
        },
        { // shm
            CFG_VRING_SHM_NAME2, // name
            DEV_BUS_NAME, // bus_name
            NULL, // dev
            NULL, // io
            { 0 }, // mem
        },
    },
};
#else
#error "Unknown uC3 sample type definition specified!"
#endif
