/**
 * @brief   Sample program for 8MPLUSLPD4-EVK (i.MX8M Plus, Cortex-A53)
 *          Shared kernel ID definition
 * @date    2025.09.29
 *
 * @copyright (C) 2025, eForce Co., Ltd.
 */
#ifndef GL_KERNEL_ID_H_
#define GL_KERNEL_ID_H_

/**
 * Core0 resources
 */
extern ID MpfID;
extern ID MbxID;
extern ID SndTaskID;
extern ID RcvTaskID;
extern ID WupTaskID;

/**
 * Core1 resources
 */
extern ID Core1TaskID;
extern ID Core1FlagID;

#endif /* GL_KERNEL_ID_H_ */
