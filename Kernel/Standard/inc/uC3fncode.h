/**
 * @file    uC3fncode.h
 * @brief   Micro C Cube Standard, KERNEL
 *          Function Code
 * @date    2018.05.23
 * @author  Copyright (c) 2018, eForce Co., Ltd. All rights reserved.
 *
 ******************************************************************************
 * @par     History
 *          - rev 1.0 (2018.05.23) yokota
 *            separate from kernel.h, hook routine.
 ******************************************************************************
 */
#ifndef _UC3_FNCODE_
#define _UC3_FNCODE_

#ifdef __cplusplus
extern "C" {
#endif

#define TFN_ACT_TSK    (-0x07)   /* act_tsk                                  */
#define TFN_IACT_TSK   (-0x71)   /* iact_tsk                                 */
#define TFN_STA_TSK    (-0x09)   /* sta_tsk                                  */
#define TFN_ROT_RDQ    (-0x55)   /* rot_rdq                                  */
#define TFN_IROT_RDQ   (-0x79)   /* irot_rdq                                 */
#define TFN_SET_FLG    (-0x2B)   /* set_flg                                  */
#define TFN_ISET_FLG   (-0x76)   /* iset_flg                                 */
#define TFN_IVSIG_OVR  (-0xF7)   /* ivsig_ovr                                */
#define TFN_SIG_SEM    (-0x23)   /* sig_sem                                  */
#define TFN_ISIG_SEM   (-0x75)   /* isig_sem                                 */
#define TFN_ISIG_TIM   (-0x7D)   /* isig_tim                                 */
#define TFN_PSND_DTQ   (-0x36)   /* psnd_dtq                                 */
#define TFN_FSND_DTQ   (-0x38)   /* fsnd_dtq                                 */
#define TFN_IPSND_DTQ  (-0x77)   /* ipsnd_dtq                                */
#define TFN_IFSND_DTQ  (-0x78)   /* ifsnd_dtq                                */
#define TFN_WUP_TSK    (-0x13)   /* wup_tsk                                  */
#define TFN_IWUP_TSK   (-0x72)   /* iwup_tsk                                 */
#define TFN_REL_WAI    (-0x15)   /* rel_wai                                  */
#define TFN_IREL_WAI   (-0x73)   /* irel_wai                                 */
#define TFN_REL_MPF    (-0x47)   /* rel_mpf                                  */
#define TFN_GET_MPF    (-0x49)   /* get_mpf                                  */
#define TFN_PGET_MPF   (-0x4A)   /* pget_mpf                                 */
#define TFN_TGET_MPF   (-0x4B)   /* tget_mpf                                 */
#define TFN_REL_MPL    (-0xA3)   /* rel_mpl                                  */
#define TFN_GET_MPL    (-0xA5)   /* get_mpl                                  */
#define TFN_PGET_MPL   (-0xA6)   /* pget_mpl                                 */
#define TFN_TGET_MPL   (-0xA7)   /* tget_mpl                                 */

#define TFN_ACP_POR    (-0x99)
#define TFN_PACP_POR   (-0x9A)
#define TFN_TACP_POR   (-0x9B)
#define TFN_CAL_POR    (-0x97)
#define TFN_TCAL_POR   (-0x98)
#define TFN_CAN_ACT    (-0x08)
#define TFN_CAN_WUP    (-0x14)
#define TFN_CHG_IMS    (-0x6B)
#define TFN_CLR_FLG    (-0x2C)
#define TFN_CHG_IMS    (-0x6B)
#define TFN_CHG_PRI    (-0x0D)
#define TFN_CRE_ALM    (-0xA9)
#define TFN_ACRE_ALM   (-0xCC)
#define TFN_CRE_CYC    (-0x4F)
#define TFN_ACRE_CYC   (-0xCB)
#define TFN_CRE_DTQ    (-0x31)
#define TFN_ACRE_DTQ   (-0xC4)
#define TFN_CRE_FLG    (-0x29)
#define TFN_ACRE_FLG   (-0xC3)
#define TFN_CRE_ISR    (-0x66)
#define TFN_ACRE_ISR   (-0xCD)
#define TFN_CRE_MBF    (-0x89)
#define TFN_ACRE_MBF   (-0xC7)
#define TFN_CRE_MBX    (-0x3D)
#define TFN_ACRE_MBX   (-0xC5)
#define TFN_CRE_MPF    (-0x45)
#define TFN_ACRE_MPF   (-0xC9)
#define TFN_CRE_MPL    (-0xA1)
#define TFN_ACRE_MPL   (-0xCA)
#define TFN_CRE_MTX    (-0x81)
#define TFN_ACRE_MTX   (-0xC6)
#define TFN_CRE_POR    (-0x95)
#define TFN_ACRE_POR   (-0xC8)
#define TFN_CRE_SEM    (-0x21)
#define TFN_ACRE_SEM   (-0xC2)
#define TFN_CRE_TSK    (-0x05)
#define TFN_ACRE_TSK   (-0xC1)
#define TFN_DEF_INH    (-0x65)
#define TFN_DEF_OVR    (-0xB1)
#define TFN_DEL_ALM    (-0xAA)
#define TFN_DEL_CYC    (-0x50)
#define TFN_DEL_DTQ    (-0x32)
#define TFN_DEL_FLG    (-0x2A)
#define TFN_DEL_ISR    (-0x67)
#define TFN_DEL_MBF    (-0x8A)
#define TFN_DEL_MBX    (-0x3E)
#define TFN_DEL_MPF    (-0x46)
#define TFN_DEL_MPL    (-0xA2)
#define TFN_DEL_MTX    (-0x82)
#define TFN_DEL_POR    (-0x96)
#define TFN_DEL_SEM    (-0x22)
#define TFN_DEL_TSK    (-0x06)
#define TFN_DIS_DSP    (-0x5B)
#define TFN_DIS_INT    (-0x69)
#define TFN_DLY_TSK    (-0x19)
#define TFN_ENA_DSP    (-0x5C)
#define TFN_ENA_INT    (-0x6A)
#define TFN_EXD_TSK    (-0x0B)
#define TFN_EXT_TSK    (-0x0A)
#define TFN_FWD_POR    (-0x9C)
#define TFN_GET_IMS    (-0x6C)
#define TFN_GET_PRI    (-0x0E)
#define TFN_GET_TID    (-0x56)
#define TFN_GET_TIM    (-0x4E)
#define TFN_LOC_CPU    (-0x59)
#define TFN_LOC_MTX    (-0x85)
#define TFN_PLOC_MTX   (-0x86)
#define TFN_TLOC_MTX   (-0x87)
#define TFN_RCV_DTQ    (-0x39)
#define TFN_PRCV_DTQ   (-0x3A)
#define TFN_TRCV_DTQ   (-0x3B)
#define TFN_RCV_MBF    (-0x91)
#define TFN_PRCV_MBF   (-0x92)
#define TFN_TRCV_MBF   (-0x93)
#define TFN_RCV_MBX    (-0x41)
#define TFN_PRCV_MBX   (-0x42)
#define TFN_TRCV_MBX   (-0x43)
#define TFN_REF_ALM    (-0xAD)
#define TFN_REF_CFG    (-0x6F)
#define TFN_REF_CYC    (-0x53)
#define TFN_REF_DTQ    (-0x3C)
#define TFN_REF_FLG    (-0x30)
#define TFN_REF_ISR    (-0x0F)
#define TFN_REF_MBF    (-0x94)
#define TFN_REF_MBX    (-0x44)
#define TFN_REF_MPF    (-0x4C)
#define TFN_REF_MPL    (-0xA8)
#define TFN_REF_MTX    (-0x88)
#define TFN_REF_OVR    (-0xB4)
#define TFN_REF_POR    (-0x9E)
#define TFN_REF_RDV    (-0x9F)
#define TFN_REF_SEM    (-0x28)
#define TFN_REF_SYS    (-0x61)
#define TFN_REF_TSK    (-0x0F)
#define TFN_REF_TST    (-0x10)
#define TFN_REF_VER    (-0x70)
#define TFN_RPL_RDV    (-0x9D)
#define TFN_RSM_TSK    (-0x17)
#define TFN_FRSM_TSK   (-0x18)
#define TFN_SET_TIM    (-0x4D)
#define TFN_SLP_TSK    (-0x11)
#define TFN_TSLP_TSK   (-0x12)
#define TFN_SND_DTQ    (-0x35)
#define TFN_TSND_DTQ   (-0x37)
#define TFN_SND_MBF    (-0x8D)
#define TFN_PSND_MBF   (-0x8E)
#define TFN_TSND_MBF   (-0x8F)
#define TFN_SND_MBX    (-0x3F)
#define TFN_SNS_CTX    (-0x5D)
#define TFN_SNS_DPN    (-0x60)
#define TFN_SNS_DSP    (-0x5F)
#define TFN_SNS_LOC    (-0x5E)
#define TFN_STA_ALM    (-0xAB)
#define TFN_STA_CYC    (-0x51)
#define TFN_STA_OVR    (-0xB2)
#define TFN_STP_ALM    (-0xAC)
#define TFN_STP_CYC    (-0x52)
#define TFN_STP_OVR    (-0xB3)
#define TFN_SUS_TSK    (-0x16)
#define TFN_TER_TSK    (-0x0C)
#define TFN_UNL_CPU    (-0x5A)
#define TFN_UNL_MTX    (-0x83)
#define TFN_WAI_FLG    (-0x2D)
#define TFN_POL_FLG    (-0x2E)
#define TFN_TWAI_FLG   (-0x2F)
#define TFN_WAI_SEM    (-0x25)
#define TFN_POL_SEM    (-0x26)
#define TFN_TWAI_SEM   (-0x27)

#define TFN_IGET_TID   (-0x7A)
#define TFN_ILOC_CPU   (-0x7B)
#define TFN_IUNL_CPU   (-0x7C)

/*  Multi-Core Extension */
#define TFN_VACT_TSK   (-0xE1)
#define TFN_VSTA_TSK   (-0xE2)
#define TFN_VWUP_TSK   (-0xE3)
#define TFN_VREL_WAI   (-0xE4)
#define TFN_VSIG_SEM   (-0xE5)
#define TFN_VPOL_SEM   (-0xE6)
#define TFN_VSET_FLG   (-0xE7)
#define TFN_VCLR_FLG   (-0xE8)
#define TFN_VPOL_FLG   (-0xE9)
#define TFN_VPSND_DTQ  (-0xEA)
#define TFN_VFSND_DTQ  (-0xEB)
#define TFN_VPRCV_DTQ  (-0xEC)
#define TFN_VROT_RDQ   (-0xED)

#ifdef __cplusplus
}
#endif

#endif /* _UC3_FNCODE_ */
