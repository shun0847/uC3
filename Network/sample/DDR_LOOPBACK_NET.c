/*************************************************************************
 * Copyright (C) 2012-2020 by eForce Co., Ltd. All rights reserved.
 * 
 * $Header:     DDR_LOOPBACK_NET.c
 * $Contents:   Loopback device
 *              2012.08.16: Created
 *              2012.10.02  Modify to avoid use of string libraries.
 *              2014.05.14  Correct lo_net_snd() parameter form.
 *              2020.06.15  Support 64-bit architecture
 ************************************************************************/
#include "kernel.h"
#include "net_hdr.h"
#ifdef NET_C_OS
#include "kernel_id.h"
#endif

#if defined(NET3_VER) && ((NET3_VER) >= 3)
#include "net_def.h"
#endif

static UH device_num;

#ifndef NET_C_OS
#if (_kernel_SIZE_SIZE==8)	/* use 64bit */
#define     LODEV_TASK_SIZE      (512*4)
#else                       /* use 32bit */
#define     LODEV_TASK_SIZE      (512)
#endif

static void lo_dev_tsk(VP_INT exinf);
static const T_CTSK c_lo_dev_tsk  = { TA_HLNG, NULL, (FP)lo_dev_tsk, 4, LODEV_TASK_SIZE, 0, "lo_dev_tsk"};
static const T_CMBX c_lo_dev_mbx  = { TA_TFIFO|TA_MFIFO, 0, NULL, "rcv_mbx"};
ID ID_LO_DEV_TSK;
ID ID_LO_DEV_MBX;
#endif

ER lo_net_ini(UH dev_num)
{
    ER ercd;
#ifndef NET_C_OS
    ercd = acre_tsk((T_CTSK*)&c_lo_dev_tsk);
    if (ercd <= 0) {
        return ercd;
    }
    ID_LO_DEV_TSK = ercd;
    ercd = acre_mbx((T_CMBX *)&c_lo_dev_mbx);
    if (ercd <= 0) {
        ter_tsk(ID_LO_DEV_TSK);
        del_tsk(ID_LO_DEV_TSK);
        return ercd;
    }
    ID_LO_DEV_MBX = ercd;
#endif

    if (dev_num < 1 || NET_DEV_MAX < dev_num) {
        return E_ID;
    }
    device_num = dev_num-1;
    ercd = sta_tsk(ID_LO_DEV_TSK, 0);
    return ercd;
}

ER lo_net_cls(UH dev_num)
{
    ter_tsk(ID_LO_DEV_TSK);
#ifndef NET_C_OS
    del_tsk(ID_LO_DEV_TSK);
    del_mbx(ID_LO_DEV_MBX);
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

ER lo_net_snd(UH dev_num, T_NET_BUF* pkt)
{
    snd_mbx(ID_LO_DEV_MBX, (T_MSG *)pkt);
    return E_WBLK;
}

void lo_dev_tsk(VP_INT exinf)
{
    T_NET_BUF *txpkt, *rxpkt;
    UH len;
    ER ercd;

    while (1) {
        ercd = rcv_mbx(ID_LO_DEV_MBX, (T_MSG **)&txpkt);
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
        rxpkt->hdr     =  rxpkt->buf + gNET_DEV[device_num].hhdrofs;
        rxpkt->hdr_len = gNET_DEV[device_num].hhdrsz;
        rxpkt->dat     = rxpkt->hdr + rxpkt->hdr_len;
        rxpkt->dat_len = len - rxpkt->hdr_len;
        rxpkt->dev     = &gNET_DEV[device_num];

        net_memcpy(rxpkt->hdr, txpkt->hdr, len);
        net_buf_ret(txpkt); /* notify succeess to TX process */
        net_pkt_rcv(rxpkt);
    }

    while (1);
}

