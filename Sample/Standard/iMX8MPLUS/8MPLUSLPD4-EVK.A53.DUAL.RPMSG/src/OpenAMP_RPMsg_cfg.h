/*
 * @file    OpenAMP_RPMsg_cfg.h
 * @brief   OpenAMP RPMsg configurations
 * @author  Copyright (c) 2025, eForce Co., Ltd. All rights reserved.
 * @license SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef OPENAMP_RPMSG_CFG_H_
#define OPENAMP_RPMSG_CFG_H_

/* RPMsg application config */
#define APP_EPT_ADDR          1024U

#ifdef __linux__
#define RPMSG_DEV_BUS_NAME    "platform"
#else
#define RPMSG_DEV_BUS_NAME    "generic"
#endif

#if defined (USE_UC3_SINGLE_SAMPLE) || defined (USE_UC3_DUAL_SAMPLE)
#define CFG_RPMSG_SVCNO       2U
#elif defined (USE_UC3_TRIPLE_SAMPLE)
#define CFG_RPMSG_SVCNO       3U
#else
#error "Unknown uC3 sample type definition specified!"
#endif

/* RPMSG Shared memory Service 0 */
#define CFG_RPMSG_SVC_NAME0   "rpmsg-openamp-demo-channel-0"
#define CFG_VRING_CTL_NAME0   "99000000.vring-ctl0"
#define CFG_VRING_CTL_BASE0   0x99000000U
#define CFG_VRING_CTL_SIZE0   0x100000U
#define CFG_VRING0_BASE0      (CFG_VRING_CTL_BASE0)
#define CFG_VRING1_BASE0      (CFG_VRING_CTL_BASE0 + 0x4000U)
#define CFG_VRING_SHM_NAME0   "99300000.vring-shm0"
#define CFG_VRING_SHM_BASE0   0x99300000U
#define CFG_VRING_SHM_SIZE0   0x100000U
#define CFG_VRING_ALIGN0      0x1000U
#define CFG_RPMSG_NUM_BUFS0   256U
#define VRING_NOTIFYID0       0U

/* RPMSG Shared memory Service 1 */
#define CFG_RPMSG_SVC_NAME1   "rpmsg-openamp-demo-channel-1"
#define CFG_VRING_CTL_NAME1   "99100000.vring-ctl1"
#define CFG_VRING_CTL_BASE1   0x99100000U
#define CFG_VRING_CTL_SIZE1   0x100000U
#define CFG_VRING0_BASE1      (CFG_VRING_CTL_BASE1)
#define CFG_VRING1_BASE1      (CFG_VRING_CTL_BASE1 + 0x4000U)
#define CFG_VRING_SHM_NAME1   "99400000.vring-shm1"
#define CFG_VRING_SHM_BASE1   0x99400000U
#define CFG_VRING_SHM_SIZE1   0x100000U
#define CFG_VRING_ALIGN1      0x1000U
#define CFG_RPMSG_NUM_BUFS1   256U
#define VRING_NOTIFYID1       1U

/* RPMSG Shared memory Service 2 */
#define CFG_RPMSG_SVC_NAME2   "rpmsg-openamp-demo-channel-2"
#define CFG_VRING_CTL_NAME2   "99200000.vring-ctl2"
#define CFG_VRING_CTL_BASE2   0x99200000U
#define CFG_VRING_CTL_SIZE2   0x100000U
#define CFG_VRING0_BASE2      (CFG_VRING_CTL_BASE2)
#define CFG_VRING1_BASE2      (CFG_VRING_CTL_BASE2 + 0x4000U)
#define CFG_VRING_SHM_NAME2   "99500000.vring-shm2"
#define CFG_VRING_SHM_BASE2   0x99500000U
#define CFG_VRING_SHM_SIZE2   0x100000U
#define CFG_VRING_ALIGN2      0x1000U
#define CFG_RPMSG_NUM_BUFS2   256U
#define VRING_NOTIFYID2       2U

#endif
