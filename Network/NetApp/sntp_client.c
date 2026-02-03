/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    SNTP Client
    Copyright (c)  2012-2023, eForce Co., Ltd. All rights reserved.

    Version Information
      2012.06.07: Created
      2012.10.02: Modify to avoid use of string libraries.
      2014.04.04: Remove line of "#include <stdlib.h>".
      2014.04.11: Modified to determine return value of con_soc
      2015.12.14: The socket ID replaced SID types
      2016.07.06: Execute static analysis tool to this source.
      2017.04.12: Changed "stratum" field to be available from user-app.
      2017.07.27: Support 64bit processor
      2021.02.01: Enabled to refer to the value of Flags field with sntp_client().
      2023.06.21: Changed not to receive packets after API processing.
 ***************************************************************************/

#include "kernel.h"
#include "net_hdr.h"
#include "sntp_client.h"

/** Definition for SNTP Packet **/
#define SNTP_LI_MASK        0xC0U
#define SNTP_LI_NON         0x00U
#define SNTP_VN_MASK        0x38U
#define SNTP_VN_VER1        0x08U
#define SNTP_VN_VER2        0x10U
#define SNTP_VN_VER3        0x18U
#define SNTP_VN_VER4        0x20U
#define SNTP_MODE_MASK      0x07U
#define SNTP_MODE_CLI       0x03U
#define SNTP_MODE_SER       0x04U

#define SNTP_VN_USE         (SNTP_VN_VER3)
#define DUMMY_LEN           1U      /* dummy length (use frame check) */

typedef struct t_sntp_pkt_c {
    UB flg;
    UB stt;
    UB poll;
    UB prec;
    UW rtdly;                       /* Root Delay   */
    UW rtdsp;                       /* Root Dispersion */
    UW refid;                       /* Refarence Clock ID */
    UW reftim[2];                   /* Refarence Clock Update Time */
    UW orgtim[2];                   /* Originate Time Stamp */
    UW rcvtim[2];                   /* Receive Time Stamp */
    UW tratim[2];                   /* Transmit Timestamp */
}T_SNTP_PKT_C;

static ER clear_rcvq(SID sid, UB *buf, UH len)
{
    ER ercd = E_OK;

    (void)cfg_soc(sid, SOC_TMO_RCV, (VP)0);
    while (ercd != E_TMOUT) {
        ercd = rcv_soc(sid, buf, len);
    }
    return E_OK;
}


ER sntp_client(T_SNTP_CLIENT *sc, UW *sec, UW *fra)
{
    T_NET_BUF *buf;
    T_SNTP_PKT_C *pkt;
    T_NODE host;
    ER ercd;

    ercd = E_OK;
    if ((!sc) || (!sec) || (!fra)) {
        ercd = E_PAR;
    }
    else if (sc->ipa == 0U) {
        ercd = E_PAR;
    }
    else if ((sc->sid == 0U) || (sc->sid > (SID)NET_SOC_MAX)) {
        ercd = E_PAR;
    }
    else if (sc->devnum > (UH)NET_DEV_MAX) {
        ercd = E_PAR;
    }
    if (E_OK != ercd) {
        return ercd;
    }

    if (sc->devnum == DEV_ANY) {
        sc->devnum++;
    }
    if (sc->port == PORT_ANY) {
        sc->port = SNTP_PORT;
    }
    if (sc->tmo == 0) {
        (void)ref_soc(sc->sid, SOC_TMO_RCV, (VP)&sc->tmo);
        if (sc->tmo == 0) {
            sc->tmo = SNTP_TIMEOUT;
        }
    }

    net_memset(&host, 0, sizeof(host));
    host.num = (UB)sc->devnum;
    host.ver = sc->ipv;
    host.ipa = INADDR_ANY;
    host.port = PORT_ANY;

    do {
        buf = 0;
        ercd = net_buf_get(&buf, sizeof(T_SNTP_PKT_C) + DUMMY_LEN, TMO_POL);
        if ((ercd != E_OK) || (buf == 0)) {
            ercd = E_NOMEM;
            break;      /* exit sntp process */
        }
        
        /* Enable the receive queue to allow packet receive. */
        (void)cfg_soc(sc->sid, SOC_UDP_RQSZ, (VP)1);        
        
        pkt = (T_SNTP_PKT_C*)buf->hdr;
        (void)clear_rcvq(sc->sid, (UB*)pkt, sizeof(T_SNTP_PKT_C));

        /* Set Reception Timeout                */
        (void)cfg_soc(sc->sid, SOC_TMO_RCV, (VP)(ADDR)sc->tmo);

        /* Set remote host as SNTP Server */
        host.port = sc->port;
        host.ipa = sc->ipa;
        ercd = con_soc(sc->sid, &host, (UB)SOC_CLI);
        if (ercd != E_OK) {
            break;      /* exit sntp process */
        }

        /* Set SNTP Client message */
        net_memset(pkt, 0, sizeof(T_SNTP_PKT_C));
        pkt->flg = SNTP_LI_NON | SNTP_VN_USE | SNTP_MODE_CLI;
        ercd = snd_soc(sc->sid, pkt, sizeof(T_SNTP_PKT_C));
        if ((UW)ercd != sizeof(T_SNTP_PKT_C)) {
            break;      /* exit sntp process */
        }

        ercd = rcv_soc(sc->sid, pkt, sizeof(T_SNTP_PKT_C) + DUMMY_LEN);
        if (ercd <= E_OK) {
            break;      /* exit sntp process */
        }
        if ((UW)ercd != sizeof(T_SNTP_PKT_C)) {
            /* Protocol Error */
            ercd = E_OBJ;
            break;      /* exit sntp process */
        }
        if ((SNTP_VN_USE| SNTP_MODE_SER) != (pkt->flg & (SNTP_VN_MASK | SNTP_MODE_MASK))) {
            /* Protocol Error */
            ercd = E_OBJ;
            break;      /* exit sntp process */
        }
        sc->flg = pkt->flg;
        sc->stt = pkt->stt;
        *sec = ntohl(pkt->tratim[0]);
        *fra = ntohl(pkt->tratim[1]);
        ercd = E_OK;
    } while (0);
    
    (void)cfg_soc(sc->sid, SOC_UDP_RQSZ, (VP)0);

    if (0 != buf) {
        net_buf_ret(buf);
    }
    return ercd;
}

