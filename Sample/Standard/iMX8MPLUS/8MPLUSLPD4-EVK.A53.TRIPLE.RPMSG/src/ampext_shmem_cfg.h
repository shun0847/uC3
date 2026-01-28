/**
 * @file    ampext_shmem_cfg.h
 * @brief   Shared memory API configurations
 * @date    2025.12.19
 * @author  Copyright (c) 2025, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2025.01.20)
 *            Initial version for iMX8MPlus series.
 *          - rev 1.1 (2025.12.19)
 *            Added definitions for channel #2.
 ****************************************************************************
 */

#ifndef AMPEXT_SHMEM_CFG_H_
#define AMPEXT_SHMEM_CFG_H_

// Shared memory API config
#define SHMEM_CTL_SIZE      (0x00100000U)
#define SHMEM_BLK_SIZE      (0x00400000U)
// channel #0
#define CFG_SHMEM_CTL_BASE0     (0x9A000000U)
#define CFG_SHMEM_BLK_BASE0     (0x9A300000U)
#define CFG_SHMEM_CTL_SIZE0     (SHMEM_CTL_SIZE)
#define CFG_SHMEM_BLK_SIZE0     (SHMEM_BLK_SIZE)
#define CFG_SHMEM_CTL_NAME0     "9a000000.shmapi-ctl0"
#define CFG_SHMEM_BLK_NAME0     "9a300000.shmapi-blk0"
// channel #1
#define CFG_SHMEM_CTL_BASE1     (0x9A100000U)
#define CFG_SHMEM_BLK_BASE1     (0x9A700000U)
#define CFG_SHMEM_CTL_SIZE1     (SHMEM_CTL_SIZE)
#define CFG_SHMEM_BLK_SIZE1     (SHMEM_BLK_SIZE)
#define CFG_SHMEM_CTL_NAME1     "9a100000.shmapi-ctl1"
#define CFG_SHMEM_BLK_NAME1     "9a700000.shmapi-blk1"
// channel #2
#define CFG_SHMEM_CTL_BASE2     (0x9A200000U)
#define CFG_SHMEM_BLK_BASE2     (0x9AB00000U)
#define CFG_SHMEM_CTL_SIZE2     (SHMEM_CTL_SIZE)
#define CFG_SHMEM_BLK_SIZE2     (SHMEM_BLK_SIZE)
#define CFG_SHMEM_CTL_NAME2     "9a200000.shmapi-ctl2"
#define CFG_SHMEM_BLK_NAME2     "9ab00000.shmapi-blk2"

#endif /* AMPEXT_SHMEM_CFG_H_ */
