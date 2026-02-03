/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK
    BSD socket wrapper/Loopback interface
    Copyright (c) 2023, eForce Co., Ltd. All rights reserved.

    Version Information
      2014.04.28: Created
      2015.12.02: The section definition added for task
      2020.03.23: Support 64bit processor
      2023.06.23: Synchronize with the notification to TX process.
 ***************************************************************************/

#include "kernel.h"
#include "net_hdr.h"
#include "net_def.h"

/* kernel resources */
#if (defined(NET_HW_OS) || defined(NET_C_OS))
#include "kernel_id.h"
#else

#if (_kernel_SIZE_SIZE==8)
#define     LOIF_TASK_SIZE      (512*4)
#else
#define     LOIF_TASK_SIZE     	(512)
#endif

/* Local Stack allocate */
#ifdef __CC_ARM
long long _loif_tsk_stk[LOIF_TASK_SIZE/sizeof(long long)] __attribute__ ((section ("LOCALSTKMEM"), zero_init));
#elif __IAR_SYSTEMS_ICC__
#pragma section = "LOCALSTKMEM"
long long _loif_tsk_stk[LOIF_TASK_SIZE/sizeof(long long)] @ "LOCALSTKMEM";
#endif

static void lo_if_tsk(VP_INT exinf);
#if (defined(__CC_ARM) || defined(__IAR_SYSTEMS_ICC__))
static const T_CTSK c_lo_if_tsk  = { TA_HLNG | TA_FPU, NULL, (FP)lo_if_tsk,  4,  LOIF_TASK_SIZE, (VP)_loif_tsk_stk, "lo_if_tsk"};
#else
static const T_CTSK c_lo_if_tsk  = { TA_HLNG | TA_FPU, NULL, (FP)lo_if_tsk,  4,  LOIF_TASK_SIZE, 0, "lo_if_tsk"};
#endif
static const T_CMBX c_lo_if_mbx  = { TA_TFIFO|TA_MFIFO, 0, NULL, "lo_if_mbx"};
ID ID_LO_IF_TSK;
ID ID_LO_IF_MBX;
#endif

ER lo_net_ini(UH dev_num)
{
    ER ercd;
#ifdef NET_S_OS
    ercd = acre_tsk((T_CTSK*)&c_lo_if_tsk);
    if (ercd <= 0) {
        return ercd;
    }
    ID_LO_IF_TSK = ercd;
    ercd = acre_mbx((T_CMBX *)&c_lo_if_mbx);
    if (ercd <= 0) {
        ter_tsk(ID_LO_IF_TSK);
        del_tsk(ID_LO_IF_TSK);
        return ercd;
    }
    ID_LO_IF_MBX = ercd;
#endif

    ercd = sta_tsk(ID_LO_IF_TSK, 0);
    return ercd;
}

ER lo_net_cls(UH dev_num)
{
    ter_tsk(ID_LO_IF_TSK);
#ifdef NET_S_OS
    del_tsk(ID_LO_IF_TSK);
    del_mbx(ID_LO_IF_MBX);
#endif
    return E_OK;
}

ER lo_net_ctl(UH dev_num, UH opt, VP val)
{
    return E_OK;
}

ER lo_net_sts(UH dev_num, UH opt, VP val)
{
    return E_OK;
}

ER lo_net_rcv(UH dev_num, T_NET_BUF* pkt)
{
    net_pkt_rcv(pkt);
    return E_OK;
}

ER lo_net_snd(UH dev_num, T_NET_BUF* pkt)
{
    snd_mbx(ID_LO_IF_MBX, (T_MSG *)pkt);
    return E_WBLK;
}

void lo_if_tsk(VP_INT exinf)
{
    T_NET_BUF *txpkt, *rxpkt;
    T_NET_DEV *dev;
    UH len;
    ER ercd;

    while (1) {
        ercd = rcv_mbx(ID_LO_IF_MBX, (T_MSG **)&txpkt);
        if (ercd != E_OK) {
            break;
        }

        len = txpkt->hdr_len;
        ercd = net_buf_get(&rxpkt, len, TMO_POL);
        if (ercd != E_OK) {
            txpkt->ercd = E_NOMEM;
            net_buf_ret(txpkt); /* notify error to TX process */
            continue;
        }
        dev = txpkt->net->dev;  /* get origin device */

        rxpkt->hdr     = rxpkt->buf + dev->hhdrofs;
        rxpkt->hdr_len = dev->hhdrsz;
        rxpkt->dat     = rxpkt->hdr + rxpkt->hdr_len;
        rxpkt->dat_len = len - rxpkt->hdr_len;
        rxpkt->dev     = dev;
        net_memcpy(rxpkt->hdr, txpkt->hdr, len);

        /* skip checksum for reception */
        rxpkt->flg    |= (HW_CS_RX_IPH4 | HW_CS_RX_DATA);

        /* notify succeess to TX process */
        loc_tcp();
        net_buf_ret(txpkt);
        ulc_tcp();

        /* Protocol stack process */
        lo_net_rcv(0, rxpkt);
    }
}
