/**
 * @file    DDR_AArch64_GTIMER.h
 * @brief   Micro C Cube Standard, DEVICE DRIVER
 *          ARM Generic Timer
 * @date    2016.09.27
 * @author  Copyright (c) 2016, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2016.09.27) y-kim
 *            Created based on DDR_CortexA_GTIMER.h
 ****************************************************************************
 */
#ifndef _DDR_AARCH64_GTIMER_H_
#define _DDR_AARCH64_GTIMER_H_

#ifdef __cplusplus
extern "C"
{
#endif

extern ER _ddr_aarch64_gtimer_init(UINT tick, UW base_clk);

#ifdef __cplusplus
}
#endif

#endif  /* _DDR_AARCH64_GTIMER_H_ */
