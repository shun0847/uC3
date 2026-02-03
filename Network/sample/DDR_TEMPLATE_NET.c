/*************************************************************************
 * Copyright (C) 2010-2025 by eForce Co., Ltd. All rights reserved.
 * 
 * $Header:     DDR_TEMPLATE_NET.c
 * $Contents:   Template of network driver with uNet3 interface
 *              2010.12.17: Created
 *              2024.03.18: Several updates.
 *              2025.04.01: Change comment "Get send frame from temp_net_snd()"
  ************************************************************************/
#include "kernel.h"
#include "net_hdr.h"


#ifdef NET_C_OS
#include "kernel_id.h"
#else
static ID NET_MBX_ID;
#endif

static UH net_dev_num;

void send_proc(void)
{
    T_NET_BUF* pkt;
    while (1) {
        /* Get send frame from temp_net_snd() (e.g. rcv_mbx) */
        rcv_mbx(NET_MBX_ID, (T_MSG**)&pkt);

        /* Put send frame to hardware          */

        /* Release network buffer              */
        loc_tcp();
        net_buf_ret(pkt);
        ulc_tcp();
    }
}

void receive_proc(void)
{
    T_NET_BUF* pkt;
    UH net_buff_len;
    UH rx_frame_len;

    while (1) {
        /* Allocate network buffer */
        net_buff_len = 1500;
        net_buf_get(&pkt, net_buff_len, TMO_FEVR);

        /* Set write offset(2 is alignment for IP)*/
        pkt->hdr = pkt->buf + 2;

        /* Get received frame from hardware      */
        /* pkt->hdr must be top of received data */
        /* rx_frame_len is receved data length   */

        /* e.g. UDP with 32byte payload */
        rx_frame_len = ETH_HDR_SZ + IP4_HDR_SZ + UDP_HDR_SZ + 32;

        /* Set frame header/data info  */
        pkt->hdr_len = ETH_HDR_SZ; /*14*/
        pkt->dat = pkt->hdr + pkt->hdr_len;
        pkt->dat_len = (rx_frame_len - pkt->hdr_len);

        /* Set device number */
        pkt->dev = &gNET_DEV[net_dev_num - 1];

        /* uNet3 protocol process */
        net_pkt_rcv(pkt);
    }
}

ER temp_net_ini(UH dev_num)
{
#ifndef NET_C_OS
    T_CMBX net_tx_mbx = {TA_TFIFO | TA_MFIFO, (PRI)0, 0, "NET_TX_MBX"};

    NET_MBX_ID = acre_mbx(&net_tx_mbx);
#endif

    net_dev_num = dev_num;
    return E_OK;
}

ER temp_net_cls(UH dev_num)
{
    return E_OK;
}

ER temp_net_ctl(UH dev_num, UH opt, VP val)
{
    return E_OK;
}

ER temp_net_sts(UH dev_num, UH opt, VP val)
{
    return E_OK;
}

ER temp_net_snd(UH dev_num, T_NET_BUF* pkt)
{
    /* Send pkt to send_proc() (e.g. snd_mbx) */
    snd_mbx(NET_MBX_ID, (T_MSG*)pkt);

    return E_WBLK;
}
