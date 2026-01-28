/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK
    TCP Protocol
    Copyright (c)  2008-2023, eForce Co., Ltd. All rights reserved.

    Version Information
      2008.11.19: Created
      2010.01.18: ETH_HDR_SZ to dev->hhdrsz
      2010.03.24: SOC_FLG_PRT check
      2010.06.15: Updated for IPv6 support                         
      2010.09.14: Reset snd_una, snd_nxt retransmitting SYN
      2010.10.07: Corrected use of SEQ_LT in tcp_seq_chk
      2010.12.17: Updated for any network interface
      2011.01.06: Correct tcp_pkt_rcv() configurable TOS/TTL
      2011.01.31: Correct source address selection for IPv6
      2011.03.11: Updated for IPv6 multiple interface
      2011.03.30: Updated for TX blocking device
      2011.04.04: Added definition of TCP/MSS for IPv6
      2011.04.21: Correct event received RST at SYN_RECEIVED
      2011.04.27: Choose socket matching IP/Socket version.
      2011.05.20: Set up networkbuffer offset
      2011.11.11: Modified iss to random
      2012.05.21: Reset snd_una, snd_nxt retransmitting SYN
                  in depleted buffer case
      2012.06.22: Corrected DAT/FIN reception in half close
      2012.06.26: Implemented Keep-Alive
      2012.08.09: In transmitting if networkbuffer is absent
                  turn over process to dattimer
      2012.09.18: Corrected RST packet it is sent by return
      2012.10.02  Modify to avoid use of string libraries.
      2012.10.18: Corrected TCP option header length
      2012.11.29: Modified not to use the device setting of
                  TX/RX buffer size in uNet3/Compact.
      2012.12.07: Corrected to choose IPv4 listening socket
                  when IPv6 listening sockets are mixed
      2013.02.13: Correct event received SYN at SYN_RECEIVED
      2013.07.11: Corrected the transmission of duplicate ACK
                  for unexpected seq packet which has no data.
      2013.09.09: Modified to update ISS everytime create soc
      2014.03.06: SNMP option was added
      2014.07.09: Set TCP padding field with 0
      2014.12.02: When the socket is SYN_RECEIVE state, 
                  Correct FIN is not sent in cls_soc().
      2015.12.14: The socket ID replaced SID types
      2016.02.19: Change the size specifying when to get network buffer
      2016.03.09  Devide HW_CS flag into ICMP and TCP/UDP
      2016.05.19  Fixed an issue of ACK field check.
      2016.06.09  Add the process when specify the MSG_PEEK option.
      2016.08.19  Reinstate of ACK field check.
      2016.09.23  Fixed unnecessary retransmission control for ACK
      2017.01.23  Corrected the check range for FIN containing data
      2017.03.29  Support for out of sequence data queue (routque).
      2017.07.06  Call clr_rcv_que() from cls_soc() instead from soc_event().
                  This allows rcv_soc() to read data even in closing state and 
                  until cls_soc() is called.
      2017.08.18  In CLOSED state window update is not transmitted.
      2017.08.30  Duplicate sequences are eliminated when overlapping
                  sequences are received.
      2018.02.01  Fixed an issue of receiving duplicate packets.
      2018.03.01  Changed the condition of retransmission setting
                  when can not get network buffer.
                  Set TCP connection information to rpi.
                  Add link of peer option.
      2019.01.15: Rename tcp_rcv_dat()
      2019.04.03  Fixed discarding of zero window receive data
      2020.05.28: Fixed NULL pointer refer of tcp_con() when IPv6 is enabled.
      2021.12.09: Release the receive queue when TCP resets.
      2022.08.01: In tcp_out_rst() rpkt->soc is NULL, so it is not referenced.
      2023.03.24: Fixed an issue where KeepAliveACK was considered DuplicateACK
      2023.09.12: Fixed recalculation of cwnd during Fast Retransmit.
 ***************************************************************************/

#include <stdio.h>
#include "kernel.h"
#include "net_hdr.h"
#include "net_def.h"
#include "net_sts_id.h"

#ifdef TCP_SUP

/*#define TCP_ACK_MOD*/
#define TCP_DIS_TWS         /* Disable Time Wait State */
#define tcp_rcv_dat         unet3_tcp_rcv_dat   /* avoid the same ITRON function name */

static UW tcp_iss;
static UH tcp_que_data(T_NET_TCP *tcp, T_NET_BUF *pkt, UH len);
static ER tcp_get_data(T_NET_SOC *soc, UB *udat, UH rlen);
static void tcp_set_rpi(T_NET_SOC *soc, T_NET_BUF *pkt);

ER tcp_cre(T_NET_SOC *soc)
{
    T_NET_TCP *tcp;
#ifndef NET_C
    T_NET *net;
#endif

    tcp = &pNET_TCP[soc->sid];

    tcp->bsd = TCP_CLOSED;

#ifndef NET_C
    if (tcp->snd_buf == NULL) {
        return E_SYS;
    }

    net = soc->net;
    if (net != NULL) {
        tcp->sbufsz  = TCP_SND_WND;
        tcp->rbufsz  = TCP_RCV_WND;
    }
#endif
    soc->tcp = tcp;
    tcp->soc = soc;
    tcp_iss  += net_rand();

    return E_OK;
}

ER tcp_del(T_NET_SOC *soc)
{
    T_NET_TCP *tcp;

    tcp = &pNET_TCP[soc->sid];

    if (tcp->bsd == TCP_CLOSED) {
        tcp->bsd = TCP_UNUSED;
        return E_OK;
    }
    else
        return E_OBJ;
}

ER tcp_con(T_NET_SOC *soc, T_NODE *host, UB con_flg)
{
    T_NET     *net;
    T_NET_TCP *tcp;

    if (soc == NULL) {
        /* Error: Resource is not allocated */
        return E_SYS;
    }

#ifdef IPV6_SUP
    if ((host != NULL) && (soc->ver != host->ver)) {
        return E_PAR;
    }
#endif

    tcp = soc->tcp;
    if (tcp == NULL) {
        return E_SYS;
    }

    net = soc->net;
    if (net == NULL) {
        net = &gNET[0]; /* Default Net */
    }

    /* RFC793:P54 */
    if (tcp->bsd != TCP_CLOSED) {
        return E_OBJ;        /* Error: Connection Already exists */
    }

    /* Clean rpi info */
    net_memset(&soc->rpi, 0, sizeof(soc->rpi));

    /* Active Call must set Remote Address */
    if (con_flg == SOC_TCP_CON) {
#ifdef IPV6_SUP
        if(soc->ver == IP_VER6) {
            if ((host == NULL) || (host->port == 0) || (ip6_addr_type(host->ip6a) == IP6_ADDR_UNSPEC)) {
                return E_PAR;   /* error: foreign socket unspecified */
            }
            ip6_addr_cpy(soc->raddr6, host->ip6a);
            set_ip6_src(soc->laddr6, host->ip6a, NULL, net->dev->num);
        }   
        else {
#endif
        if ((host == NULL) || (host->port == 0) || (host->ipa == 0)) {
            return E_PAR;   /* error: foreign socket unspecified */
        }
        soc->raddr = host->ipa;
#ifdef LO_IF_SUP
        if (IS_LBACK_IP(soc->raddr)) {
            soc->raddr = net->adr->ipaddr;
        }
#endif
#ifdef IPV6_SUP
        }
#endif
        soc->rport = host->port;
        soc->flg   &= ~SOC_FLG_SER;
        /* In case of PORT_ANY, a new port value must be used */
        if (soc->flg & SOC_FLG_PRT) {
            soc->lport = get_eport(IP_PROTO_TCP);
        }
    }
    else {
#ifdef IPV6_SUP
        if(soc->ver == IP_VER6) {
            net_memset(soc->raddr6, 0, 16);
            net_memset(soc->laddr6, 0, 16);
        }
        else
#endif
        soc->raddr = 0;
        soc->rport = 0;
        soc->flg   |= SOC_FLG_SER;
    }

    /* Initialize TCB */
    tcp->sflg     = 0;
    tcp->tim_flg  = 0;
    tcp->snd_len  = 0;
    tcp->sputp    = 0;

    /* Initialize send */
    if (tcp->sbufsz == 0) {
        tcp->sbufsz  = TCP_SND_WND;
    }
    soc->sdatque = NULL;
    tcp->snd_wnd = 0;
    tcp->rcv_mss = 0;
    tcp->snd_una = 0;
    tcp->snd_nxt = 0;
    tcp->snd_max = 0;
    tcp->snd_wl1 = 0;
    tcp->snd_wl2 = 0;
    tcp->snd_up  = 0;
    tcp->ret_tmo = 0;

#ifdef KEEPALIVE_SUP
    tcp->kpa_cnt = 0;
    tcp->kpa_tmo = 0;
#endif
    /* Initialize recv */
    if (tcp->rbufsz == 0) {
        tcp->rbufsz  = TCP_RCV_WND;
    }
    tcp->routque = NULL;
    tcp->routque_cnt = 0;
    tcp->rdatque = NULL;
    tcp->rcv_wnd = tcp->rbufsz;
    tcp->rdatsz  = 0;
    tcp->rcv_nxt = 0;
    tcp->irs     = 0;
    tcp->rcv_up  = 0;
    tcp->ack_tmo = 0;
#ifdef IPV6_SUP
    if (soc->ver == IP_VER6)
        tcp->mss = TCP_MSS_IPV6;
    else
#endif
    tcp->mss     = TCP_MSS;

    /* Initialize flow control */
    tcp->cwnd    = tcp->mss * 2;
    tcp->ssth    = tcp->cwnd;
    tcp->recover = 0;
    tcp->dup_cnt = 0;

    /* Initialize RTO           */
    tcp->srtt    = 0;
    tcp->rttvar  = 0;
    tcp->rto     = TCP_RTO_INI;  /* 3 seconds  */
    tcp->rtt_seq = 0;
    tcp->rtt     = 0;

    /* PASSIVE Call */
    if (con_flg == SOC_TCP_ACP) {
        tcp->bsd = TCP_LISTEN;
        tcp->chg_flg = NET_STS_CHG;
        return E_WBLK;
    }

    /* Active Call */

    /* Set Random ISS       */
    tcp->iss    = tcp_iss & 0x0FFFFFFF;
    tcp_iss    += net_rand();

    tcp->snd_una = tcp->iss;
    tcp->snd_nxt = tcp->iss + 1;
    tcp->bsd     = TCP_SYN_SENT;
    tcp->chg_flg = NET_STS_CHG;

    /* Set Timer */
    tcp->tcp_tmo  = NET_TICK + TCP_CON_TMO;  /* Connection Timer */
    tcp->tim_flg |= TCP_SET_CON;
    tcp->ret_tmo  = NET_TICK + tcp->rto;     /* Retransmission   */
    tcp->tim_flg |= TCP_SET_RET;
    tcp->sflg |= TCP_FLG_SYN;

    /* Send SYN: Seq=ISS    */
    tcp_out(tcp, TCP_FLG_SYN, 0);

    return E_WBLK;
}

/*
  Initiate the closing process.
   * Return EV_NET_CLS if already in closing.
   * Send FIN and return EV_NET_OK
*/

ER tcp_sht(T_NET_SOC *soc)
{
    T_NET_TCP *tcp;

    tcp = soc->tcp;

    switch (tcp->bsd) {
        case TCP_CLOSED:
            return E_OBJ;
        case TCP_SYN_RECEIVED:
        case TCP_ESTABLISHED:
        case TCP_CLOSE_WAIT:
            break;
        default:
            return E_CLS;
    }

    if (tcp->sflg & TCP_FLG_FIN) {
        return E_CLS;  /* Closing */
    }

    /* Initiate Close */
    tcp->sflg |= TCP_FLG_FIN;
    tcp_out(tcp, TCP_FLG_FIN, 0);
    return E_OK;
}

/* Close the connection
   - If data not pending to send then Tx FIN
     (shutdown is not used, use close instead)
*/

ER tcp_cls(T_NET_SOC *soc, UB cls_flg)
{
    T_NET     *net;
    T_NET_TCP *tcp;
    ER ercd;

    tcp = soc->tcp;
    if (tcp == NULL) {
        /* Error: Resource is not allocated */
        return E_SYS;
    }

    if (cls_flg == SOC_TCP_SHT) {
        return tcp_sht(soc);
    }

    ercd = 0;
    switch (tcp->bsd) {
        case TCP_CLOSED:
            return E_OBJ; /* error: connection does not exist */
        case TCP_LISTEN:
        case TCP_SYN_SENT:
            tcp_abt(tcp, 2);
            soc_event(soc, EV_SOC_ALL, E_CLS);
            return E_OK;
        case TCP_SYN_RECEIVED:
        case TCP_ESTABLISHED:
        case TCP_CLOSE_WAIT:
            if (!(tcp->sflg & TCP_FLG_FIN)) {
                ercd = 1;   /* need to send fin */
            }
            break;
        default:
            break;
    }

    /* socket is closing! rcv: return E_CLS any pending rcv_soc() call */
    if (soc->wptn & EV_SOC_RCV) {
        soc_event(soc, EV_SOC_RCV, E_CLS);
    }
    
    if (tcp->tim_flg & TCP_SET_CLS) {
        return E_WBLK;
    }

    /* Reset Connection if Receive buffer has data */
    if (tcp->rdatsz) {
        tcp_abt(tcp, 2);
        soc_event(soc, EV_SOC_ALL, E_CLS);
        return E_OK;
    }

    net = soc->net;

    /* Initiate Close */
    tcp->tim_flg |= TCP_SET_CLS;
    if (!(tcp->tim_flg & TCP_SET_DAT)) {
        tcp->tcp_tmo = TCP_CLS_TMO + NET_TICK;
    }

    if (ercd) {
        tcp->sflg |= TCP_FLG_FIN;
        tcp_out(tcp, TCP_FLG_FIN, 0);
    }

    return E_WBLK;
}

/*
    tcp_abt:
        called when Reset is received (code == 0)
        called when Retry total timeout expires (code == 1)
        called when User abort's socket process (code == 2)
    (code==0)
        should not send RST, can move back to LISTEN state.
    (code==1)
        can send RST, can move back to LISTEN state.
    (code==2)
        can send RST, should not move back to LISTEN state.

    RFC 793 p62 ABORT Call
*/
ER tcp_abt(T_NET_TCP *tcp, UB code)
{
    ER ercd;
    T_NET_BUF *pkt;

    if (tcp == NULL)
        return E_OK;

    ercd = E_OK;
    switch (tcp->bsd) {
        case TCP_CLOSED:
            break;
        case TCP_LISTEN:    /* error: connection reset */
            if (code == 2) {
                tcp->bsd = TCP_CLOSED;
            }
            break;
        case TCP_SYN_SENT:
            tcp->bsd = TCP_CLOSED;
            net_sts_inc(NET_STS_TCP, NET_STS_TCP_ATTEMPT_FAIL);
            break;
        case TCP_SYN_RECEIVED:
            /* Return to LISTEN if Passive connection */
            tcp->bsd = TCP_CLOSED;
            net_sts_inc(NET_STS_TCP, NET_STS_TCP_ATTEMPT_FAIL);
            if ((code != 2) && (tcp->soc->flg & SOC_FLG_SER)) {
                ercd = tcp_con(tcp->soc, NULL, SOC_SER);
            }
            break;

        case TCP_CLOSING:
        case TCP_LAST_ACK:
        case TCP_TIME_WAIT:
            tcp->bsd = TCP_CLOSED;
            break;
        default:
            /* Send RST <SEQ=SND.NXT> <CTL=RST>*/
            if (code) {
                tcp_out(tcp, TCP_FLG_RST, 0);
            }
            tcp->bsd = TCP_CLOSED;
            net_sts_inc(NET_STS_TCP, NET_STS_TCP_ESTAB_RESET);
            break;
    }

    if (tcp->bsd == TCP_CLOSED) {
        tcp->sflg = 0;
        tcp->tim_flg = 0;
        tcp->chg_flg = NET_STS_CHG;
        while (tcp->rdatque != NULL) {
            pkt = (T_NET_BUF*)tcp->rdatque;
            rmv4que_top(&tcp->rdatque);
            net_buf_ret(pkt);
        }
        while (tcp->routque != NULL) {
            pkt = tcp->routque;
            rmv4que_top((UW**)&tcp->routque);
            net_buf_ret(pkt);
        }
    }

    return ercd;
}

ER tcp_snd(T_NET_SOC *soc, VP data, UH len)
{
    T_NET_TCP *tcp;
    UW end_len;

    tcp = soc->tcp;

    switch (tcp->bsd) {
        case TCP_CLOSED:
            return E_OBJ; /* error: connection does not exist */
        case TCP_LISTEN:
            /* Not supported move to active connection */
            return E_OBJ;
        case TCP_SYN_SENT:
        case TCP_SYN_RECEIVED:
            /* Not supported queuing */
            return E_OBJ;
        case TCP_ESTABLISHED:
        case TCP_CLOSE_WAIT:
            if (tcp->sflg & TCP_FLG_FIN) {
                return E_CLS;
            }
            break;
        default:
            return E_CLS;
    }

    if (len > (tcp->sbufsz - tcp->snd_len))
        len = (tcp->sbufsz - tcp->snd_len);

    if (len == 0) {
        return E_WBLK;
    }

    /* Copy to TCP Buffer */
    if ((tcp->sputp + len) >= tcp->sbufsz) {
        end_len = tcp->sbufsz - tcp->sputp;
        net_memcpy(tcp->snd_buf + tcp->sputp, (char*)data, end_len);
        tcp->sputp = len - end_len;
        net_memcpy(tcp->snd_buf, (char*)data + end_len, tcp->sputp);
    }
    else {
        net_memcpy(tcp->snd_buf + tcp->sputp, (char*)data, len);
        tcp->sputp += len;
    }

    tcp->snd_len += len;

    /* Issue Send */
    tcp_out(tcp, 0, 2);

    return len;
}

ER tcp_rcv(T_NET_SOC *soc, VP data, UH len)
{
    T_NET_TCP *tcp;
    ER ercd;

    tcp = soc->tcp;

    if (soc->wptn & EV_SOC_CLS) {
        return E_CLS;   /* cls_soc() is called, refuse rcv_soc() */
    }
    /* Data is in buffer? */
    if (tcp->rdatsz != 0) {
        ercd = tcp_get_data(soc, data, len);
        return ercd;
    }

    /* Check whether TCP is in valid Recv state */
    switch (tcp->bsd) {
        case TCP_CLOSED:
            return E_OBJ; /* error: connection does not exist */
        case TCP_LISTEN:
            /* Not supported move to active connection */
            return E_CLS;
        case TCP_SYN_SENT:
        case TCP_SYN_RECEIVED:
            /* Not supported queuing */
            return E_CLS;
        case TCP_ESTABLISHED:
        case TCP_FIN_WAIT1:
        case TCP_FIN_WAIT2:
            break;
        case TCP_CLOSE_WAIT:
            /*if (data) return it;*/
            /*else error: connection closing*/
            return E_OK;    /* E_CLS returned in older version */
        default:
            return E_OK;    /* error: connection closing */
    }

    return E_WBLK;  /* Wait for Data */
}

/*
    1 sec
*/
void tcp_usr_timer(void)
{
    T_NET_TCP *tcp;
    UW ctimval;
    UH i;
#ifdef KEEPALIVE_SUP
    T_NET     *net;
#endif

    ctimval = NET_TICK;

    for (i=0;i<NET_TCP_MAX;i++) {
        tcp = &pNET_TCP[i];
        if (tcp->bsd > TCP_CLOSED) {

            if (tcp->tim_flg & (TCP_SET_CON | TCP_SET_DAT | TCP_SET_CLS)) {
                if (TIM_LEQ(tcp->tcp_tmo,ctimval)) {

                    if (tcp->bsd == TCP_TIME_WAIT) {
                        tcp_abt(tcp, 1);
                        soc_event(tcp->soc, EV_SOC_ALL, E_OK);
                    }
                    else {
                        tcp_abt(tcp, 1);
                        if (tcp->bsd != TCP_LISTEN)
                            soc_event(tcp->soc, EV_SOC_ALL, E_TMOUT);
                    }
                }
            }
#ifdef KEEPALIVE_SUP
            net = tcp->soc->net;
            if (tcp->tim_flg & TCP_SET_KPA) {
                if (TIM_LEQ(tcp->kpa_tmo, ctimval)) {
                    if (tcp->kpa_cnt >= TCP_KPA_CNT) {
                        tcp_abt(tcp, 2);
                        soc_event(tcp->soc, EV_SOC_ALL, E_TMOUT);
                    } else {
                        tcp_ack(tcp, TCP_SET_KPA);
                        tcp->kpa_tmo += TCP_KPA_INT;
                        tcp->kpa_cnt++;
                    }
                }
            }
#endif
        }
    } /*for*/

    tcp_iss += TCP_ISS_INCR;
}

/*
    TCP Fast Timer 200 msec
*/
void tcp_ack_timer(void)
{
    T_NET_TCP *tcp;
    UW ctimval;
    UH i;

    ctimval = NET_TICK;

    for (i=0;i<NET_TCP_MAX;i++) {
        tcp = &pNET_TCP[i];
        if (tcp->bsd > TCP_CLOSED) {
            if (tcp->tim_flg & TCP_SET_ACK) {
                if (TIM_LEQ(tcp->ack_tmo, ctimval)) {
                    tcp_out(tcp, TCP_FLG_ACK, 0);
                }
            }
        }
    } /*for*/
}

/*
    TCP Slow Timer 500 msec
*/
void tcp_dat_timer(void)
{
    T_NET     *net;
    T_NET_TCP *tcp;
    UW ctimval;
    UH i;

    ctimval = NET_TICK;

    for (i=0;i<NET_TCP_MAX;i++) {
        tcp = &pNET_TCP[i];
        if (tcp->bsd > TCP_CLOSED) {

            net = tcp->soc->net;

            if ((tcp->tim_flg & TCP_SET_PRB) || (tcp->tim_flg & TCP_SET_RET)) {

                if (TIM_LEQ(tcp->ret_tmo, ctimval)) {

                    if (!(tcp->tim_flg & TCP_SET_CON)) {
                        if (!(tcp->tim_flg & TCP_SET_DAT)) {
                        tcp->tcp_tmo = NET_TICK + TCP_SND_TMO;
                        tcp->tim_flg |= TCP_SET_DAT;
                        }
                    }

                    if (tcp->tim_flg & TCP_SET_PRB) {
                        tcp_ack(tcp, TCP_SET_PRB);
                    }
                    else {
                        /* Retry Timer Expire
                            cwnd = TCP_MSS;
                            ssth = MAX(flightsize/2,2*smss))
                        */
                        tcp->cwnd     = (tcp->snd_max - tcp->snd_una);
                        tcp->ssth     = 2*tcp->mss;
                        tcp->ssth     = (tcp->cwnd > tcp->ssth) ? tcp->cwnd : tcp->ssth;
                        tcp->cwnd     = tcp->mss;
                        tcp->recover  = tcp->snd_max;
                        tcp->snd_nxt  = tcp->snd_una;
                        tcp->dup_cnt  = 0;
                        tcp_out(tcp, tcp->sflg, 1);
                        tcp->rtt      = 0;
                    }

                    /* Back-off Retry */
                    tcp->rto = tcp->rto << 1;
                    if (tcp->rto > TCP_RTO_MAX)
                        tcp->rto = TCP_RTO_MAX;

                    tcp->ret_tmo  = NET_TICK + tcp->rto;  /* Reset */

                    /* May clear SRTT */
                    tcp->srtt = 0;
                }
            }
            if (tcp->rtt)
                tcp->rtt++;
        }
    } /*for*/
}

/*
    Compute Retransmission timeout
    RFC 2988
        RTO    = 3Sec
    First RTT:
        SRTT   = R
        RTTVAR = R/2
        RTO    = SRTT + MAX(G, 4*RTTVAR)
    Update RTT:
        RTTVAR = (1-1/4) * RTTVAR + 1/4*|SRTT-R|
        SRTT   = (1-1/8) * SRTT   + 1/8 * R
        RTO    = SRTT + MAX(G, 4*RTTVAR)

        RTO < 1sec  ? RTO = 1sec
        RTO > 60sec ? RTO = 60sec

    G = Clock Granularity
*/
static void tcp_rto_update(T_NET_TCP *tcp, UW rtt)
{
    T_NET *net;
    UW g; /*Temp*/

    net = tcp->soc->net;

    rtt = rtt * 50; /* 500msec? */  /* slow timer */

    /* First RTO */
    if (tcp->srtt == 0) {
        tcp->srtt = rtt;
        tcp->rttvar = rtt >> 1;
    }
    else {
        /*(3*rttvar + g)/ 4*/
        if (rtt > tcp->srtt) g = rtt - tcp->srtt;
        else g = tcp->srtt - rtt;
        tcp->rttvar = ((tcp->rttvar*3)+g) >> 2;
        /*(7*SRTT+R)/8*/
        tcp->srtt = (7*tcp->srtt+rtt) >> 3;
    }

    g = tcp->rttvar << 2;
    g = (g  > TCP_RTO_MIN) ? g : TCP_RTO_MIN;

    tcp->rto = tcp->srtt + g;

    if (tcp->rto < TCP_RTO_MIN)
        tcp->rto = TCP_RTO_MIN;
    if (tcp->rto > TCP_RTO_MAX)
        tcp->rto = TCP_RTO_MAX;
}

UH tcp_wnd_adv(T_NET_TCP *tcp)
{
    UW tmp;

    if (tcp->rcv_wnd) {
        /* min(RCV.BUFF/2, Eff.snd.MSS */
        tmp = tcp->rbufsz >> 1;
        tmp = (tmp < tcp->mss) ? tmp : tcp->mss;
        /* Actual Free space greater than tmp? */
        if ((tcp->rbufsz - tcp->rdatsz) > tmp) {
            tcp->rcv_wnd = tcp->rbufsz - tcp->rdatsz;
        }
    }

    return tcp->rcv_wnd;
}

void tcp_wnd_upd(T_NET_TCP *tcp, UH len)
{
    UW  tmp;

    if (tcp->rdatsz >= len)
        tcp->rdatsz -= len;
    else {
        /* This condition should never occur */
        return;
    }

    if (tcp->rcv_wnd) {
        tmp = tcp->rcv_wnd;
        tcp_wnd_adv(tcp);
        /* Send Window Update */
        if (tmp != tcp->rcv_wnd) {
#ifndef TCP_ACK_MOD
            tcp_out(tcp, TCP_FLG_ACK, 0);
#else
            /*  >= 2 * TCP_MSS   */
            /*  >= TCP_RCV_WND/2 */
            tmp = tcp->rcv_wnd - tmp; /* adv */
            if (tmp >= (tcp->mss << 1)) {
                tcp_out(tcp, TCP_FLG_ACK, 0);
            }
            else if (tmp >= (tcp->rbufsz >> 1)) {
                tcp_out(tcp, TCP_FLG_ACK, 0);
            }
#endif
        }
    }
    else {
        /* Zero window update */
        tcp->rcv_wnd = tcp->rbufsz - tcp->rdatsz;
        tcp_out(tcp, TCP_FLG_ACK, 0);
    }

    return;
}

/*
    *rque - queue of data in correct seq
    *rout - out of order data
*/

static UH tcp_que_data(T_NET_TCP *tcp, T_NET_BUF *pkt, UH len)
{
    W buf_spa;

    buf_spa = tcp->rbufsz - tcp->rdatsz;
    if (buf_spa == 0)
        return 0;

    if (len > buf_spa)
        len = buf_spa;

    pkt->dat_len = len;

    /* Queue the packet for user reception */
    add2que_end(&tcp->rdatque, pkt);   /* Add to rque */

    soc_event(tcp->soc, EV_SOC_RCV, (tcp->rdatsz + len));

    return len;
}

/*
    Max rlen 65535?
    passing UB pointer (application buf pointer)

    1. Return 0 if no data is queued
    2. If exisiting data length is less than rlen then return existing data length
    3. else return rlen.
    4. Release the queued packet if the data is fully readed.
*/

static ER tcp_get_data(T_NET_SOC *soc, UB *udat, UH rlen)
{
    T_NET_TCP *tcp;
    T_NET_BUF *pkt;
    UH tot_len = 0;

    tcp = soc->tcp;
    pkt = (T_NET_BUF *)tcp->rdatque;

    /* No Data in Queue */
    while (rlen != 0) {

        if (pkt == NULL)
            break;

        /* Transfer the data to user space */
        if (rlen >= pkt->dat_len) {
            rlen -= pkt->dat_len;
            net_memcpy(udat + tot_len, pkt->dat, pkt->dat_len);
            tot_len += pkt->dat_len;
#ifdef MSG_PEEK_SUP
            if ( (soc->flg & SOC_FLG_MSGPEEK) == 0 ) {
#endif
                rmv4que_top(&tcp->rdatque);
                pkt->soc = NULL;
                net_buf_ret(pkt);
                pkt = (T_NET_BUF *)tcp->rdatque;
#ifdef MSG_PEEK_SUP
            } else {
                pkt = (T_NET_BUF *)pkt->next;
            }
#endif
        }
        else {
            net_memcpy(udat + tot_len, pkt->dat, rlen);
            tot_len += rlen;
#ifdef MSG_PEEK_SUP
            if ( (soc->flg & SOC_FLG_MSGPEEK) == 0 )
#endif
            {
                pkt->dat = pkt->dat + rlen;
                pkt->dat_len = pkt->dat_len - rlen;
            }
            /* Still data is in this packet, don't release it */
            break;
        }
    }

    if (tot_len) {
#ifdef MSG_PEEK_SUP
        if ((soc->flg & SOC_FLG_MSGPEEK) == 0)
#endif
        {
            if (tcp->bsd != TCP_CLOSED) { /* defer wnd updates in CLOSED state */
                tcp_wnd_upd(tcp, tot_len);
            }
        }
    }

#ifdef MSG_PEEK_SUP
    if ( (soc->flg & SOC_FLG_MSGPEEK) != 0 ) {
        /* If the reception is finished, the message peak option auto clear */
        soc->flg &= ~SOC_FLG_MSGPEEK;
    }
#endif

    return tot_len;
}

static UB tcp_seq_chk(T_NET_TCP *tcp, T_TCP_HDR *tcph, UH len)
{
    UW rwin;
    UW rseq;

    rwin = tcp->rcv_wnd;
    rseq = tcph->seq;

    if (len == 0) {
        if (rwin == 0) {
            if (rseq == tcp->rcv_nxt) {
                return 1;   /* Valid Seq */
            }
        }
        else {
            if (SEQ_LEQ(tcp->rcv_nxt,rseq) && SEQ_LT(rseq,(tcp->rcv_nxt + rwin)))
                return 1;   /* Valid Seq */
        }
    }
    else {
        if (rwin == 0) {
            return 0;       /* Invalid Seq */
        }
        else {
            if ((SEQ_LEQ(tcp->rcv_nxt,rseq) && SEQ_LT(rseq,(tcp->rcv_nxt + rwin))) ||
                (SEQ_LEQ(tcp->rcv_nxt,(rseq + len -1)) && SEQ_LT((rseq + len -1),(tcp->rcv_nxt + rwin)))) {
                return 1;   /* Valid Seq */
            }
        }
    }

    return 0;   /* Invalid Sequence */
}

/* trim rx sequence and length to fit in receive window (refer RFC 793 - p70) */
void tcp_seq_trim(T_NET_TCP *tcp, T_NET_BUF *pkt, UH thlen, UH *rlen)
{
    T_TCP_HDR *tcph;
    UH len = 0;
    UH trim_len;
    UH i;

    tcph = (T_TCP_HDR *)pkt->dat;

    trim_len = 0;
    /* segment.seq < rcv.nxt  */
    if (SEQ_LT(tcph->seq, tcp->rcv_nxt)) {
        len = tcp->rcv_nxt - tcph->seq; /* get length to reduce */
        tcph->seq = tcp->rcv_nxt;  /* set segment.seq to rcv_nxt */
        if (*rlen >= len) {
            *rlen -= len;          /* reduce data length */
        }
        trim_len = len;
    }

    /* segment.seq + len > rcv.wnd */
    if (SEQ_GT(tcph->seq + *rlen, tcp->rcv_nxt + tcp->rcv_wnd)) {
        len = (tcph->seq + *rlen) - (tcp->rcv_nxt + tcp->rcv_wnd); /* get length to reduce */
        if (*rlen >= len) {
            *rlen -= len;          /* reduce data length */
        }
    }

    if (trim_len != 0) {
        /* drop duplicate data portion in pkt->dat */
        for (i = 0; i < *rlen; i++) {
            *(pkt->dat + thlen + i) = *(pkt->dat + thlen + trim_len + i);
        }
        pkt->dat_len -= trim_len;
    }
}

static void tcp_mss_option(T_NET_TCP *tcp, UB *opt_data, UH opt_len)
{
    T_NET *net;
    UW i;
    UH mss;
    UB optlen, opt;
    UW MSS;

    net = tcp->soc->net;

#ifdef IPV6_SUP
    if (tcp->soc->ver == IP_VER6)
        MSS = TCP_MSS_IPV6;
    else
#endif
    MSS = TCP_MSS;

    tcp->mss = MSS;
    if (opt_len == 0) {
        return;
    }

    optlen = 0;
    for (i=0;i<opt_len;i += optlen) {
        opt = opt_data[i];
        optlen = 0;
        switch (opt) {
            case 0: /* TCP_OPT_EOL */
                return;
            case 1: /* TCP_OPT_NOP */
                optlen = 1;
                continue;
            case 2: /* TCP_OPT_MSS */
                optlen = opt_data[i+1];
                if (optlen != 4) {
                    /* Broken Header option? */
                    NET_ERR(printf("\r\n broken mss optlen %d", opt, optlen));
                    return;
                }
                mss = opt_data[i+2];
                mss = (mss << 8) | (opt_data[i+3]);
                tcp->mss = (UW)(mss);
                if (tcp->mss > MSS) tcp->mss = MSS;
                /*if (tcp->mss < 64) tcp->mss = 64;*/
                continue;
            default:
                optlen = opt_data[i+1];
                if ((optlen < 2) || (optlen > (opt_len - i))) {
                    /* Broken Header option? */
                    NET_ERR(printf("\r\n broken opt %d optlen %d", opt, optlen));
                    return;
                }
                continue;
        }
    }

    return;
}

/* 'routque' should not queue more than TCP_RCV_OSQ_MAX packets.
 * After process tcp_routseq_chk()/tcp_rcv_dat(), if 'routque' is full
 * then drop the last packet. */
void tcp_routseq_full(T_NET_TCP *tcp)
{
    T_NET *net;
    T_NET_BUF *prev_pkt, *que_pkt;

    net = tcp->soc->net;
    if (!(tcp->routque_cnt > TCP_RCV_OSQ_MAX)) {
        return; /* not full */
    }

    /* queue full? drop the last queued packet */
    prev_pkt = NULL;
    que_pkt = tcp->routque;
    while (que_pkt) {
        if (que_pkt->next == NULL) {
            break;
        }
        prev_pkt = que_pkt;
        que_pkt = (T_NET_BUF *)que_pkt->next;
    }

    if (que_pkt) {
        if (prev_pkt == NULL) {
            tcp->routque = NULL;
        }
        else {
            prev_pkt->next = NULL;
        }
        net_buf_ret(que_pkt);
        tcp->routque_cnt--;
    }
}

/* Insert received out-of-seq packet in 'routque'. if the received packet overlaps with queued packets,
 * drop/trim received or queued packet as required. */
void tcp_routseq_upd(T_NET_TCP *tcp, T_NET_BUF *in_pkt)
{
    T_NET_BUF *prev_pkt, *que_pkt;
    T_TCP_HDR *in_tcphdr, *que_tcphdr;
    UW in_seq, in_end_seq, que_seq, que_end_seq;
    UW in_len, que_len;
    BOOL que_update;
    UB drop_pkt;

    if (tcp->routque == NULL) {
        tcp->routque = in_pkt;
        tcp->routque_cnt++;
        return;
    }

    /* step 1: Traverse queued packet and drop or trim overlap packets. */
    /*         Do not insert the rx packet in queue, will do this in step 2 */
    while (tcp->routque) {

    	que_pkt = tcp->routque;
        prev_pkt = NULL;
        drop_pkt = 0;       /* 1 = drop rx pkt, 2= drop que pkt, 0 - no drop */
        que_update = FALSE; /* change in rx pkt or que, should repeat step 1 */

        in_tcphdr = (T_TCP_HDR *)in_pkt->dat;
        in_seq    = in_tcphdr->seq;
        in_len    = ((((in_tcphdr->flag) & 0xF000) >> 12) * 4); /* TCP header len */
        in_len    = in_pkt->dat_len - in_len; /* TCP data len */
        in_end_seq = in_seq + in_len;

/* overlap pattern for individual queued packet
pattern 1                                  pattern 2
Queued packet  |.....Q-------QE              |.....Q-------QE
Recv start seq |s1...s2                      |.......s1....s2
Recv end seq   |.e1..e2..e3..e4..e5          |.........e1..e2..e3

    e1      e2      e3      e4      e5        e1        e2      e3
s1  que    trim    trim   drop q    drop q   drop rx  drop rx   trim
s2  na    drop rx drop rx drop rx/q drop q    na      drop rx   trim

s2e4 drop queued packet
s1e2 s1e3 trim rx                            s1e3,s2e3 trim queued
*/
        while (que_pkt) {

        	que_tcphdr = (T_TCP_HDR *)que_pkt->dat;
            que_seq    = que_tcphdr->seq;
            que_len    = ((((que_tcphdr->flag) & 0xF000) >> 12) * 4); /* TCP header len */
            que_len    = que_pkt->dat_len - que_len; /* TCP data len */
            que_end_seq = que_seq + que_len;

            /* pattern 1 */
            if (SEQ_LEQ(in_seq, que_seq)) {
                if (SEQ_GEQ(in_end_seq, que_end_seq)) {
                    /* s1e4, s1e5, s2e4, s2e5*/
                    /* drop queued packet */
                    drop_pkt   = 2;
                    que_update = TRUE;  /* repeat step 1 */
                    break;
                }
                if (SEQ_GT(in_end_seq, que_seq)) {
                    if (in_seq == que_seq) {
                        /* s2e2, s2e3 */
                        /* received packet is smaller, drop it */
                        drop_pkt   = 1;
                        break;
                    }
                    /* s1e2, s1e3 */
                    /* trim incoming packet */
                    in_pkt->dat_len -= in_len;
                    in_pkt->dat_len += (que_seq - in_seq);
                    que_update = TRUE;  /* repeat step 1 */
                    break;
                }
                /* whole packet fall before queued packet */
                /* s1e1, s2e1 */
                break;
            }
            /* pattern 2*/
            if (SEQ_LT(in_seq, que_end_seq)) {
                if (SEQ_LEQ(in_end_seq, que_end_seq)) {
                    /* s1e1, s1e2, s2e1, s2e1 */
                    /* drop rx packet */
                    drop_pkt   = 1;
                    break;
                }
                /* s1e3, s2e3 */
                /* trim queued packet (better because we need to adjust only length) */
                que_pkt->dat_len -= que_len;
                que_pkt->dat_len += (in_seq - que_seq);
                que_update = TRUE;  /* repeat step 1 */
                break;
            }

            /* pattern 3 */
            /* whole rx packet falls after queued packet */
            /* continue process */
            prev_pkt = que_pkt;
            que_pkt = (T_NET_BUF *)que_pkt->next;

        } /* end of while 2 */

        if (drop_pkt) {
            if (drop_pkt == 1) {
                /* drop rx packet */
            	in_pkt->next = NULL;
            	net_buf_ret(in_pkt);
                return;
            }
            else {
                /* drop queued packet */
                if (tcp->routque == que_pkt) {
                    tcp->routque = (T_NET_BUF *)que_pkt->next;
                }
                else {
                    prev_pkt->next = que_pkt->next;
                }
                que_pkt->next = NULL;
                net_buf_ret(que_pkt);
                tcp->routque_cnt--;
            }
        }

        if (que_update == FALSE) {
            break;
        }
    } /* end of while 1 */

    /* step 2: insert received packet in the queue */
    if (tcp->routque == NULL) {
        tcp->routque = in_pkt;
        tcp->routque_cnt++;
        return;
    }

    in_tcphdr = (T_TCP_HDR *)in_pkt->dat;
    in_seq    = in_tcphdr->seq;
    in_len    = ((((in_tcphdr->flag) & 0xF000) >> 12) * 4); /* TCP header len */
    in_len    = in_pkt->dat_len - in_len; /* TCP data len */
    in_end_seq = in_seq + in_len;

    que_update = FALSE;
    prev_pkt = NULL;
    que_pkt = tcp->routque;
    while (que_pkt) {
        que_tcphdr = (T_TCP_HDR *)que_pkt->dat;
        if (SEQ_LEQ(in_end_seq, que_tcphdr->seq)) {
            /* insert before */
            if (prev_pkt == NULL) {
                tcp->routque = in_pkt;
            }
            else {
                prev_pkt->next = (UW *)in_pkt;
            }
            in_pkt->next = (UW *)que_pkt;
            que_update = TRUE;
            break;
        }
        prev_pkt = que_pkt;
        que_pkt = (T_NET_BUF *)que_pkt->next;
    }

    if (que_update == FALSE) {
        prev_pkt->next = (UW *)in_pkt;  /* last queued packet */
    }

    tcp->routque_cnt++;
}

/* packet received in sequence. Move it to the 'rdatque' */
void tcp_rcv_dat(T_NET_TCP *tcp, T_NET_BUF *pkt, UH thlen, UH rlen)
{
#if 0 /* this check not required, rlen is trimmed in tcp_seq_trim() */
    if (rlen > tcp->rcv_wnd) {  /* Cannot accept more than buffer size */
        rlen = tcp->rcv_wnd;
    }
#endif
    pkt->dat += thlen;
    pkt->dat_len = rlen;
    rlen = tcp_que_data(tcp, pkt, rlen);   /* nbuf not available any more */

    /* Accept the data */
    tcp->rcv_nxt += rlen;
    tcp->rcv_wnd -= rlen;   /* We never recv more than adv window */
    tcp->rdatsz += rlen;
}

/* Inspect 'routque'. If a queued packet is in expected sequence then
 * move it from 'routque' to 'rdatque'. Point last moved packet tcp header to out_tcphdr
 * and set data length to out_len (these info are used to process Fin).
 * Return True if 'rdatque' is updated otherwise return False. */
BOOL tcp_routseq_chk(T_NET_TCP *tcp, T_TCP_HDR **out_tcphdr, UH *out_len)
{
    T_NET_BUF *que_pkt;
    T_TCP_HDR *que_tcphdr;
    UH thlen, rlen;
    BOOL que_chg;

    que_chg = FALSE;
    *out_tcphdr = NULL;

    que_pkt = tcp->routque;
    while (que_pkt) {
        que_tcphdr = (T_TCP_HDR *)que_pkt->dat;
        thlen = ((((que_tcphdr->flag) & 0xF000) >> 12) * 4);
        rlen  = que_pkt->dat_len - thlen; /* TCP Data len */
        if (tcp->rcv_nxt != que_tcphdr->seq) {
            break;
        }

        /* remove packet from 'routque' */
        tcp->routque = (T_NET_BUF *)que_pkt->next;
        que_pkt->next = NULL;
        tcp->routque_cnt--;

        /* add packet to 'rdatque' */
        tcp_rcv_dat(tcp, que_pkt, thlen, rlen);

        que_pkt = tcp->routque;

        que_chg = TRUE;
        *out_tcphdr = que_tcphdr;
        *out_len = rlen;
    }

    return que_chg;
}

static void tcp_set_rpi(T_NET_SOC *soc, T_NET_BUF *pkt)
{
    T_NET     *net;
    T_TCP_HDR *tcphdr;
    T_IP4_HDR *ip4hdr;
#ifdef IPV6_SUP
    T_IP6_HDR *ip6hdr;
#endif
#ifdef LINK_INF_SUP
    T_NET_DEV *dev;
#endif

    net    = pkt->net;
    tcphdr = (T_TCP_HDR *)pkt->dat;
    soc->rpi.src_port = tcphdr->sp;
    soc->rpi.dst_port = tcphdr->dp;

#ifdef IPV6_SUP
    ip6hdr = (T_IP6_HDR *)pkt->hdr;
    if(soc->ver == IP_VER6) {
        ip6_addr_cpy(soc->rpi.ip6sa, ip6hdr->sa);
        ip6_addr_cpy(soc->rpi.ip6da, ip6hdr->da);
    }
    else {
#endif
    ip4hdr = (T_IP4_HDR *)pkt->hdr;
    soc->rpi.src_ipa  = ip4hdr->sa;
    soc->rpi.dst_ipa  = ip4hdr->da;
    soc->rpi.ttl      = ip4hdr->ttl;
    soc->rpi.tos      = ip4hdr->tos;
#ifdef IPV6_SUP
    }
#endif
    soc->rpi.num      = net->dev->num;
    soc->rpi.ver      = soc->ver;

#ifdef LINK_INF_SUP
    dev = pkt->dev;
    if ((dev != NULL) && (dev->type == NET_DEV_TYPE_ETH)) {
        net_memcpy(soc->rpi.link, &pkt->buf[0] + dev->hhdrofs, sizeof(soc->rpi.link));
    }
#endif

    return;
}

void tcp_pkt_rcv(T_NET_BUF *pkt)
{
    T_TCP_HDR *tcphdr;
    T_IP4_HDR *ip4hdr = NULL;
#ifdef IPV6_SUP
    UB ip_type;
    T_IP6_HDR *ip6hdr = NULL;
#endif
    T_NET_SOC *soc, *tcpsoc;
    T_NET_TCP *tcp;
    T_NET     *net;
    SID sid;
    UH optlen;
    UH rlen, sflg, acked, thlen;
    UH rflag, rwin;
    UW rseq, rack, snd_nxt;
    UB *opt;
    UH flg = 0;

    net_sts_inc(NET_STS_TCP, NET_STS_TCP_IN_SEG);

#ifdef IPV6_SUP
    ip_type = *(pkt->hdr)>>4;
#endif

    /* Validate TCP Header length */

    tcphdr = (T_TCP_HDR *)pkt->dat;
    if (tcphdr == NULL) {
        net_sts_inc(NET_STS_TCP, NET_STS_TCP_IN_ERR);
        goto _tcp_drop;
    }

    thlen = (((ntohs(tcphdr->flag) & 0xF000) >> 12) * 4);
    if ((thlen < TCP_HDR_SZ) || (pkt->dat_len < thlen)) {
        net_sts_inc(NET_STS_TCP, NET_STS_TCP_IN_ERR);
        goto _tcp_drop;
    }

    net    = pkt->net;

    if (!SKIP_TCPCS) {
    if (pkt->flg & HW_CS_RX_TCPUDP) {
        if (pkt->flg & HW_CS_DATA_ERR) {
            net_buf_ret(pkt);
            return;
        }
    }
    else {
#ifdef IPV6_SUP
        if(ip_type == IP_HDR_VER6) {
            if(ip6_cksum((T_IP6_HDR *)pkt->hdr, pkt->dat) != 0) {
                net_sts_inc(NET_STS_TCP, NET_STS_TCP_IN_ERR);
                goto _tcp_drop;
            }
        }
        else
#endif
        /* Validate TCP Checksum */
        if (tcp_csum(pkt) != 0) {
            net_sts_inc(NET_STS_TCP, NET_STS_TCP_IN_ERR);
            goto _tcp_drop;
        }
    }
    } /* skip checksum */

    /* NULL destination port not allowed */
    tcphdr->dp = ntohs(tcphdr->dp);
    if (tcphdr->dp == 0) {
        net_sts_inc(NET_STS_TCP, NET_STS_TCP_IN_ERR);
        goto _tcp_drop;
    }

#ifdef IPV6_SUP
    if(ip_type == IP_HDR_VER6)
        ip6hdr = (T_IP6_HDR *)pkt->hdr;
    else {
#endif
    ip4hdr = (T_IP4_HDR *)pkt->hdr;
    ip4hdr->sa = htonl(ip4hdr->sa);
    ip4hdr->da = htonl(ip4hdr->da);
#ifdef IPV6_SUP
    }
#endif
    tcphdr->sp = ntohs(tcphdr->sp);
    tcphdr->seq = ntohl(tcphdr->seq);
    tcphdr->ack = ntohl(tcphdr->ack);
    tcphdr->flag = ntohs(tcphdr->flag);

    rflag = tcphdr->flag;
    rlen  = pkt->dat_len - thlen; /* TCP Data len */

    opt    = NULL;
    optlen = thlen - TCP_HDR_SZ;
    if (optlen > 0) {
        opt = pkt->dat + TCP_HDR_SZ;
    }

    /* Search for matching Socket */
    soc = tcpsoc = NULL;
    tcp = NULL;

    /* 1. Search for Established Socket */
    for (sid=0;sid<NET_TCP_MAX;sid++) {

        tcp = &pNET_TCP[sid];

        if (tcp->bsd == TCP_UNUSED) {
            continue;
        }

        if ((tcp->bsd == TCP_CLOSED) || (tcp->bsd == TCP_LISTEN)) {
            continue;
        }

        soc = tcp->soc;
#ifdef IPV6_SUP
        if(ip_type == IP_HDR_VER6) {
            if (soc->state == SOC_CREATED &&
                soc->lport == tcphdr->dp  &&
                soc->rport == tcphdr->sp  &&
                soc->ver   == IP_VER6     &&
                ip6_addr_cmp(soc->raddr6, ip6hdr->sa)  &&
                soc->net   == net) {
    
                tcpsoc = soc;
                break;
            }
        }
        else {
#endif
        if (soc->state == SOC_CREATED &&
            soc->lport == tcphdr->dp  &&
            soc->rport == tcphdr->sp  &&
            soc->ver   == IP_VER4     &&
            soc->raddr == ip4hdr->sa  &&
            soc->net   == net) {

            tcpsoc = soc;

            break;
        }
#ifdef IPV6_SUP
        }
#endif
    }


    /* 2. Search for Listening Socket */
    if (tcpsoc == NULL) {

        for (sid=0;sid<NET_TCP_MAX;sid++) {

            tcp = &pNET_TCP[sid];
            if (tcp->bsd != TCP_LISTEN) {
                continue;
            }

            soc = tcp->soc;

#ifdef IPV6_SUP
            if(((ip_type == IP_HDR_VER6) && (soc->ver != IP_VER6))
            || ((ip_type == IP_HDR_VER4) && (soc->ver != IP_VER4)))
                continue;
#endif

            if (soc->state == SOC_CREATED &&
                soc->lport == tcphdr->dp  &&
                ((soc->net == NULL) || (soc->net == net))) {

                tcpsoc = soc;

                break;
            }
        }
    }

    if (tcpsoc == NULL) {
        net_sts_inc(NET_STS_TCP, NET_STS_TCP_IN_ERR);
        goto _tcp_rst;
    }

    /* Process incoming Segment */

    rseq = tcphdr->seq;
    rack = tcphdr->ack;
    rwin = ntohs(tcphdr->win);

    sflg = 0; /* Nothing to Send */

#ifdef KEEPALIVE_SUP
    tcp->kpa_cnt = 0;
    tcp->kpa_tmo = TCP_KPA_TMO + NET_TICK;
#endif

    switch (tcp->bsd) {

        case TCP_CLOSED:
            goto _tcp_rst;

        case TCP_LISTEN:

            if (rflag & TCP_FLG_RST) {
                net_sts_inc(NET_STS_TCP, NET_STS_TCP_IN_ERR);
                goto _tcp_drop;
            }

            if (rflag & TCP_FLG_ACK) {
                net_sts_inc(NET_STS_TCP, NET_STS_TCP_IN_ERR);
                goto _tcp_rst;
            }

            if (!(rflag & TCP_FLG_SYN)) {
                net_sts_inc(NET_STS_TCP, NET_STS_TCP_IN_ERR);
                goto _tcp_drop;
            }

            /* SYN Reception    */

            tcp->irs     = rseq;
            tcp->rcv_nxt = rseq + 1;

            tcp->iss    = tcp_iss & 0x0FFFFFFF;
            tcp_iss    += net_rand();
            tcp->snd_una = tcp->iss;
            tcp->snd_nxt = tcp->iss + 1;

            tcp->snd_wnd = rwin;         /* Update win here? */
            tcp->snd_wl1 = rseq;
            tcp->snd_wl2 = rack;

            tcp->bsd = TCP_SYN_RECEIVED;        /* State Change */
            tcp->chg_flg = NET_STS_CHG;

#ifdef IPV6_SUP
            if(ip_type == IP_HDR_VER6) {
                ip6_addr_cpy(soc->raddr6, ip6hdr->sa);
                ip6_addr_cpy(soc->laddr6, ip6hdr->da);
            }
            else
#endif
            soc->raddr  = ip4hdr->sa;
            soc->rport  = tcphdr->sp;
            if (soc->net == NULL) {
                soc->net = pkt->net;
                if (soc->ttl == DEF_IP4_TTL)
                    soc->ttl = IP4_TTL;
                if (soc->tos == DEF_IP4_TOS)
                    soc->tos = IP4_TOS;
#ifndef NET_C
                tcp->sbufsz  = TCP_SND_WND;
                tcp->rbufsz  = TCP_RCV_WND;
#endif
            }

            tcp_mss_option(tcp, opt, optlen);   /* Set MSS */

            tcp->rto    = TCP_RTO_INI;

            /* RES: SYN-ACK     */
            /* Timer ON         */
            /* return           */
            tcp->tim_flg |= (TCP_SET_CON|TCP_SET_RET);
            tcp->tcp_tmo  = NET_TICK + TCP_CON_TMO;   /* Connection Timer */
            tcp->ret_tmo  = NET_TICK + tcp->rto;      /* Retransmission   */
            tcp->sflg |= TCP_FLG_SYN;
            tcp_out(tcp, TCP_FLG_SYN|TCP_FLG_ACK, 0);

            goto _tcp_drop;

        case TCP_SYN_SENT:
            if (rflag & TCP_FLG_ACK) {

                if (SEQ_LEQ(rack,tcp->iss) || SEQ_GT(rack,tcp->snd_max)) {
                    net_sts_inc(NET_STS_TCP, NET_STS_TCP_IN_ERR);
                    goto _tcp_rst;
                }

                if (!(SEQ_LEQ(tcp->snd_una, rack) && SEQ_LEQ(rack, tcp->snd_max))) {
                    net_sts_inc(NET_STS_TCP, NET_STS_TCP_IN_ERR);
                    goto _tcp_drop;
                }

                if (rflag & TCP_FLG_RST) {
                    tcp_abt(tcp, 1);
                    soc_event(soc, EV_SOC_ALL, E_CLS);
                    goto _tcp_drop;
                }
            }

            if (rflag & TCP_FLG_RST) {
                goto _tcp_drop;
            }

            if (!(rflag & TCP_FLG_SYN)) {
                goto _tcp_drop;
            }

            /* SYN Reception */

            tcp->rcv_nxt = rseq + 1;
            tcp->irs     = rseq;

            tcp_mss_option(tcp, opt, optlen);    /* Set MSS */

            tcp->snd_wnd = rwin;         /* Update win here? */
            tcp->snd_wl1 = rseq;
            tcp->snd_wl2 = rack;

            if (rflag & TCP_FLG_ACK) {
                tcp->snd_una = rack;
                /* clear retry queue if any */
                tcp->sflg &= ~(UH)TCP_FLG_SYN;
                tcp->tim_flg &= ~(UH)TCP_SET_RET;
            }

            if (SEQ_GT(tcp->snd_una,tcp->iss)) {
                tcp->bsd = TCP_ESTABLISHED;     /* State change */
                tcp->chg_flg = NET_STS_CHG;
                /* Initialize Congestion Window */
                tcp->cwnd = tcp->mss * 2;
                tcp->ssth = rwin;

                /* Send ACK */
                sflg |= TCP_FLG_ACK;
                /*tcp_out(tcp, TCP_FLG_ACK, 0);*/

                tcp->tim_flg &= ~(UH)(TCP_SET_CON);

#ifdef KEEPALIVE_SUP
                if (TCP_KPA_CNT) {
                    tcp->tim_flg |= TCP_SET_KPA;
                }
#endif

                /* Set rpi info(active open) */
                tcp_set_rpi(soc, pkt);

                soc_event(soc, EV_SOC_CON, E_OK);
                net_sts_inc(NET_STS_TCP, NET_STS_TCP_ACTIVE_OPEN);

                /* if other controls or data present goto step 6 */
                /* otherwise return */
                goto _tcp_more;     /* continue step 6 */
            }
            else {
                tcp->bsd = TCP_SYN_RECEIVED;
                /* Send SYN|ACK */
                tcp_out(tcp, TCP_FLG_SYN|TCP_FLG_ACK, 0);
                tcp->chg_flg = NET_STS_CHG;
                /* if data present queue it */
            }
            goto _tcp_drop;

        default:
            break;
    }

    /* Step 1: Check sequence number */

    if (!(tcp_seq_chk(tcp, tcphdr, rlen))) {
        if (rflag & TCP_FLG_RST) {
            goto _tcp_drop;
        }
        goto _tcp_ack;
    }
    tcp_seq_trim(tcp, pkt, thlen, &rlen); /* trim data if required */
    rseq = tcphdr->seq;

    /* Step 2: Check for RST */

    if (rflag & TCP_FLG_RST) {
        tcp_abt(tcp, 0);
        if (tcp->bsd != TCP_LISTEN)
            soc_event(soc, EV_SOC_ALL, E_CLS);
        goto _tcp_drop;
    }

    /* Step 3. Check for Security */

    /* Step 4. Check for SYN is in window   */

    if (rflag & TCP_FLG_SYN) {
        tcp_abt(tcp, 0);
        if (tcp->bsd != TCP_LISTEN)
            soc_event(soc, EV_SOC_ALL, E_CLS);
        net_sts_inc(NET_STS_TCP, NET_STS_TCP_IN_ERR);
        goto _tcp_rst;
    }

    /* Step 5. Check for ACK field      */

    if (!(rflag & TCP_FLG_ACK)) {
        net_sts_inc(NET_STS_TCP, NET_STS_TCP_IN_ERR);
        goto _tcp_drop;
    }

    /* Step 5. SYN_RECEIVED State       */
    if (tcp->bsd == TCP_SYN_RECEIVED) {

        if (!(SEQ_LEQ(tcp->snd_una, rack) && SEQ_LEQ(rack, tcp->snd_max))) {
            net_sts_inc(NET_STS_TCP, NET_STS_TCP_IN_ERR);
            goto _tcp_rst;
        }

        tcp->bsd = TCP_ESTABLISHED;
        tcp->sflg &= ~(UH)TCP_FLG_SYN;      /* here clear syn retry ? */
        tcp->tim_flg &= ~(UH)TCP_SET_RET;
        tcp->tim_flg &= ~(UH)TCP_SET_CON;
#ifdef KEEPALIVE_SUP
        if (TCP_KPA_CNT) {
            tcp->tim_flg |= TCP_SET_KPA;
        }
#endif

        /* Set rpi info(passive open) */
        tcp_set_rpi(soc, pkt);

        soc_event(soc, EV_SOC_CON, E_OK);
        net_sts_inc(NET_STS_TCP, NET_STS_TCP_PASSIVE_OPEN);
        tcp->chg_flg = NET_STS_CHG;

        /* initialize congestion */
        tcp->cwnd = tcp->mss * 2;
        tcp->ssth = rwin;
    }

    /* Step 5: ESTABLISHED State */

    /* Invalid ACK */
    if (SEQ_GT(rack, tcp->snd_max)) {
        net_sts_inc(NET_STS_TCP, NET_STS_TCP_IN_ERR);
        goto _tcp_ack;  /* ack & drop */
    }

    if (rwin == 0) {
        /* Start Zero Window Probe Timer */
        tcp->tim_flg |= TCP_SET_PRB;
        tcp->ret_tmo = NET_TICK + tcp->rto;
    }
    else {
        /* Stop Zero Window Probe Timer */
        tcp->tim_flg &= ~(UH)TCP_SET_PRB;
    }

    /* Duplicate ACK */
    if (SEQ_LEQ(rack, tcp->snd_una)) {

        /* Zero window update? */
        if (tcp->snd_wnd != rwin) {

            if (tcp->snd_wnd == 0) {
                tcp->snd_nxt = rack;
            }

            tcp->snd_wnd = rwin;     /* Window Update */
            if (SEQ_LT(tcp->snd_wl1, rseq)) {
                tcp->snd_wl1 = rseq;
                tcp->snd_wl2 = rack;
            }
            tcp->dup_cnt = 0;
            goto _tcp_more;
        }

        if ((rlen != 0) || (rflag & TCP_FLG_FIN)) {
            tcp->dup_cnt = 0;
            goto _tcp_more;
        }

        if (rwin == 0) {
            goto _tcp_drop;
        }

        /* Not duplicate ACK, if send buffer is empty */
        if (tcp->snd_len == 0) {
            goto _tcp_drop;
        }        

#ifndef TCP_DIS_FRET
        /* Do Fast Retransmit Processing */
        tcp->dup_cnt++;
        if (tcp->dup_cnt < TCP_DUP_CNT) {
#ifdef TCP_LIMITED_TX
            /* Limited Transmit                         */
            /* Unsent Data in Queue for Transmission    */
            /* Then, for the first two duplicate ACK's  */
            /* Transmit new data if,                    */
            /* 1.Rx Adv Window allows new transmission  */
            /* 2.FlightSize <= CWND + 2*TCP_MSS         */
            if (tcp->snd_len != 0) {
                snd_nxt = tcp->snd_len - (tcp->snd_nxt - tcp->snd_una);
                if (snd_nxt > 0) {                                  /* Unsent Data length */
                    acked = tcp->snd_max - tcp->snd_una;            /* Flight Size        */
                    if ((acked + (tcp->mss+tcp->mss)) <= tcp->cwnd) {
                      /*tcp_out(tcp, 0, 1);*/
                    }
                }
            }/*snd_nxt, acked are not meaningful name, just temporary variables*/
#endif
            goto _tcp_drop;
        }

        if (tcp->dup_cnt == TCP_DUP_CNT) {
            if (SEQ_GT(tcp->recover,rack)) {
                tcp->dup_cnt = 0;   /* We don't do Fast Retransmit */
                goto _tcp_drop;
            }

            /* Fast Retransmit
                ssth    = MAX(FlightSize/2,2*SMSS)
                recover = Seq Max
                Retransmit one segment
                cwnd    = ssth + 3*SMSS
            */
            snd_nxt      = (tcp->snd_max - tcp->snd_nxt) >> 1; /* FlightSize*/
            tcp->ssth    = (snd_nxt > (tcp->mss << 1)) ? snd_nxt:(tcp->mss<<1);
            tcp->recover = tcp->snd_max;
            snd_nxt      = tcp->snd_nxt;
            tcp->snd_nxt = tcp->snd_una;
            tcp->cwnd    = tcp->mss;
            tcp_out(tcp, 0, 1);    /* Retransmit 1 segment */
            tcp->snd_nxt = snd_nxt;
            tcp->cwnd    = ((tcp->ssth + (3*tcp->mss)) / tcp->mss) * tcp->mss;
            goto _tcp_drop;
        }
        /* Fast Recovery
            cwnd += SMSS
            Transmit new segment if any
        */
        tcp->cwnd = tcp->cwnd + tcp->mss;
        goto _tcp_out;

#else   /* TCP_DIS_FRET */

        goto _tcp_drop;

#endif
    }

    /* New ACK */
    tcp->tim_flg &= ~(UH)TCP_SET_DAT;

    if (SEQ_GT(rack, tcp->snd_nxt))
        tcp->snd_nxt = rack;

    /* Fast Recovery continue */
    if (tcp->dup_cnt >= TCP_DUP_CNT) {
        /* FULL ACK */
        if (SEQ_GT(rack, tcp->recover)) {
            tcp->cwnd = tcp->ssth;
            tcp->dup_cnt = 0;
        }
        else {
            /* Partial ACK
                Retransmit first unack seg.
                cwnd += (Amount of new data acked)
                Reset retry timer for first such ACK
            */
            snd_nxt      = tcp->snd_nxt;
            tcp->snd_nxt = rack;        /*tcp->snd_una;*/
            tcp->cwnd    = tcp->mss;
            tcp_out(tcp, 0, 1); /* Retransmit 1 segment */
            tcp->snd_nxt = snd_nxt;
            acked = rack - tcp->snd_una;
            acked = (acked/tcp->mss)*tcp->mss;
            tcp->cwnd    += acked;
        }
    }
    else {
        tcp->dup_cnt = 0;           /* End of Fast Recovery */
        if (tcp->cwnd < tcp->ssth) {
            tcp->cwnd += tcp->mss;
        }
        else {
            tcp->cwnd += ((tcp->mss*tcp->mss/tcp->cwnd)/tcp->mss);
        }
    }

    /* clear send queue */
    if (tcp->snd_len) {
        acked = rack - tcp->snd_una;
        if (acked > tcp->snd_len)   /* in case, if this ACK for FIN */
            acked = tcp->snd_len;
        tcp->snd_len -= acked;
        soc_event(soc, EV_SOC_SND, tcp->sbufsz - tcp->snd_len);
    }

    tcp->snd_una = rack;
    tcp->snd_wnd = rwin;

    if (tcp->snd_max == tcp->snd_una) {
        if (tcp->tim_flg & TCP_SET_RET)
            tcp->tim_flg &= ~(UH)TCP_SET_RET;
    }
    else {
        if (tcp->tim_flg & TCP_SET_RET) {
            tcp->ret_tmo  = NET_TICK + tcp->rto;
        }
    }

    /* Calculate RTT */
    if (SEQ_GEQ(tcp->snd_una, tcp->rtt_seq)) {
        tcp_rto_update(tcp, tcp->rtt);
        tcp->rtt = 0;
    }

    /* Step 5: Other State */

    switch (tcp->bsd) {
        case TCP_FIN_WAIT1:
            if (tcp->snd_max == rack) {
                tcp->bsd = TCP_FIN_WAIT2;
                tcp->sflg &= ~(UH)TCP_FLG_FIN;      /* clear FIN retry */
                if (tcp->tim_flg & TCP_SET_CLS) {
                    tcp->tcp_tmo = TCP_CLS_TMO + NET_TICK;
                }
                tcp->chg_flg = NET_STS_CHG;
            }
            break; /*?*/
            /* continue processing */
        case TCP_FIN_WAIT2:
            /* Retry queue empty ?  */
            /* USER CLOSE is okay   */
            /* do not delete tcb    */
            tcp->tim_flg &= ~(UH)TCP_SET_RET;
            tcp->tim_flg &= ~(UH)TCP_SET_CLS;
            soc_event(soc, EV_SOC_CLS, E_OK);
            break;
        case TCP_CLOSE_WAIT:
            /* same as ESTABLISHED state */
            break;
        case TCP_CLOSING:
#ifndef TCP_DIS_TWS
            if (tcp->snd_max == rack) {
                tcp->bsd = TCP_TIME_WAIT;
                /* Start 2MSL Timer here? */
                tcp->tim_flg  |= TCP_SET_CON;
                tcp->tcp_tmo  = NET_TICK + TCP_CLW_TMO;
                tcp->chg_flg = NET_STS_CHG;
                /* return? */
            }
            else {
                goto _tcp_out;
            }
            break;
#else
            if (tcp->snd_max != rack) {
                goto _tcp_out;
            }
#endif
        case TCP_LAST_ACK:
            if (tcp->snd_max == rack) {
                tcp->bsd = TCP_CLOSED;
                tcp->sflg &= ~(UH)TCP_FLG_FIN;
                tcp->tim_flg &= ~(UH)TCP_SET_RET;
                tcp->tim_flg &= ~(UH)TCP_SET_CLS;
                soc_event(soc, EV_SOC_CLS, E_OK);
                tcp->chg_flg = NET_STS_CHG;
                goto _tcp_out; /* return */
            }
        case TCP_TIME_WAIT:
            /* Send ACK     */
            sflg |= TCP_FLG_ACK;
            /* Restart 2MSL */
            tcp->tim_flg  |= TCP_SET_CON;
            tcp->tcp_tmo  = NET_TICK + TCP_CLW_TMO;
            goto _tcp_out;
    }

_tcp_more:
    /* step 6: Check the URG bit        */

    /* setp 7: Process the segment text */
    if (rlen != 0) {
        switch (tcp->bsd) {
            case TCP_ESTABLISHED:
            case TCP_FIN_WAIT1:
            case TCP_FIN_WAIT2:

                if (soc->wptn & EV_SOC_CLS) {
                    /* User closed reception        */
                    /* not possible to accept data  */
                    tcp_abt(tcp, 2);
                    soc_event(soc, EV_SOC_ALL, E_CLS);
                    goto _tcp_drop;
                }
                /* rlen not zero and trimmed with tcp->rcv_wnd */
                flg |= TCP_DAT_BUF; /* to avoid releasing 'pkt' in bottom of this function */
                if ((tcp->routque == NULL) && (tcp->rcv_nxt == rseq)) {
                    /* seq in order, consume tcp data */
                    tcp_rcv_dat(tcp, pkt, thlen, rlen); /* add to recv data queue */
                }
                else {
                    NET_ERR(printf("\r\n out-seq 0x%x len %d", tcphdr->seq, rlen));
                    tcp_routseq_upd(tcp, pkt); /* check/add/update out of order queue */
                    //pkt = NULL; //either pkt is added to outque or discarded
                    /* check seq in order, consume data if seq in order */
                    if (tcp_routseq_chk(tcp, &tcphdr, &rlen) == FALSE) { /* True: seq in order */
                        sflg |= TCP_FLG_ACK;
                        tcp_routseq_full(tcp);
                        goto _tcp_out; /* seq not in order, send ACK */
                    }
                    tcp_routseq_full(tcp);
                    /* continue! reinitialize variables required in step 8 */
                    /* note 'pkt' pointer is obsolute */
                    rflag = tcphdr->flag;
                    rseq  = tcphdr->seq;
                    rack  = tcphdr->ack;
                }
                /* Send ACK SEQ=SND.NXT ACK=RCV.NXT CTL=ACK */
#ifdef TCP_ACK_MOD
                if (tcp->tim_flg & TCP_SET_ACK) {
                    sflg |= TCP_FLG_ACK;    /* ACK for every 2nd segment */
                }
#endif
                tcp->tim_flg |= TCP_SET_ACK;
                tcp->ack_tmo = NET_TICK + TCP_ACK_TMO;
                if (tcp->rcv_wnd == 0) {
                    sflg |= TCP_FLG_ACK;    /* ZeroWindowProbe ACK */
                }
                break;
            case TCP_CLOSE_WAIT:
            case TCP_CLOSING:
            case TCP_LAST_ACK:
            case TCP_TIME_WAIT:
                /* Ignore segment text */
                rlen = 0;
                break;
        }
    }

    /* step 8: check the FIN bit        */
    if (rflag & TCP_FLG_FIN) {

        switch (tcp->bsd) {
            case TCP_CLOSED:
            case TCP_LISTEN:
            case TCP_SYN_SENT:
                /* drop the segment */
                goto _tcp_out;  /* any other pending ? check and drop */
            default:
                break;
        }

        if (tcp->rcv_nxt != (rseq + rlen)) { /* last data byte is received ? */
            NET_ERR(printf("\r\n out-seq 0x%x len %d", tcphdr->seq, rlen));
            sflg |= TCP_FLG_ACK;
            goto _tcp_out;
        }

        /* Signal user: connection closing (do this after switch()) */

        /* Advance RCV.NXT for FIN */
        tcp->rcv_nxt += 1;
        /* send ack for fin */
        sflg |= TCP_FLG_ACK;
        /*tcp_ack(tcp, 0);*/

        switch (tcp->bsd) {
            case TCP_SYN_RECEIVED:
            case TCP_ESTABLISHED:
                tcp->bsd = TCP_CLOSE_WAIT;
                tcp->chg_flg = NET_STS_CHG;
                break;
            case TCP_FIN_WAIT1:
                /* fin has been acked? */
                if (rack == tcp->snd_max) {
                    tcp->bsd = TCP_TIME_WAIT;
                    /* start 2MSL */
                    tcp->tim_flg  |= TCP_SET_CON;
                    tcp->tcp_tmo  = NET_TICK + TCP_CLW_TMO;
                    tcp->chg_flg = NET_STS_CHG;
                }
                else {
                    tcp->bsd = TCP_CLOSING;
                    tcp->chg_flg = NET_STS_CHG;
                }
                break;
            case TCP_FIN_WAIT2:
                tcp->bsd = TCP_TIME_WAIT;
                /* start 2MSL */
                /* turn off other timers */
                tcp->tim_flg  = TCP_SET_CON;
                tcp->tcp_tmo  = NET_TICK + TCP_CLW_TMO;
                tcp->chg_flg = NET_STS_CHG;
                break;
            case TCP_TIME_WAIT:
                /* Restart 2MSL */
                tcp->tim_flg |= (TCP_SET_CON);
                tcp->tcp_tmo  = NET_TICK + TCP_CLW_TMO;
                break;
        }
#ifdef TCP_DIS_TWS
        if (tcp->bsd == TCP_TIME_WAIT) {
            tcp->bsd = TCP_CLOSED;
            tcp->sflg &= ~(UH)TCP_FLG_FIN;
            tcp->tim_flg &= ~(UH)TCP_SET_RET;
            tcp->tim_flg &= ~(UH)TCP_SET_CLS;
            /* Signal user: connection closing */
            soc_event(soc, EV_SOC_CLS, E_OK);
            tcp->chg_flg = NET_STS_CHG;
        }
        else
#endif
        {
            soc_event(soc, EV_SOC_RCV, E_OK);
        }
    }

_tcp_out:
    tcp_out(tcp, sflg, 0);
    goto _tcp_drop;

_tcp_ack:
    tcp_ack(tcp, 0);
    goto _tcp_drop;

_tcp_rst:
    tcp_out_rst(pkt, rlen); /* Response with RST */

_tcp_drop:
    if (!(flg & TCP_DAT_BUF)) {
        net_buf_ret(pkt);
    }
    return;
}

void tcp_out(T_NET_TCP *tcp, UH flag, UH rts_flg)
{
    T_NET     *net;
    T_TCP_HDR *tcphdr;
    T_IP4_HDR *iph = NULL;
#ifdef IPV6_SUP
    T_IP6_HDR *ip6h = NULL;
#endif
    T_NET_SOC *soc;
    T_NET_BUF *pkt;
    UB *opt, *pdu_ptr, *dat_ptr;
    UW snd_max;
    UH opt_len, pdu_len, snd_len;
    UH etc_len, cpy_len;
    ER ercd;

    ercd = net_buf_get(&pkt, tcp->mss, TMO_POL);
    if (ercd != E_OK) {
        if (flag & TCP_FLG_SYN) {
            tcp->snd_una = tcp->iss;
            tcp->snd_nxt = tcp->iss + 1;
        }
        if (flag & (TCP_FLG_SYN | TCP_FLG_FIN) || rts_flg == 2) {
            tcp->ret_tmo  = NET_TICK + tcp->rto;
            tcp->tim_flg |= TCP_SET_RET;
        }
        return;
    }

    soc = tcp->soc;
    net = soc->net;

    pkt->hdr = pkt->hdr + net->dev->hhdrsz + net->dev->hhdrofs;
#ifdef IPV6_SUP
    if(soc->ver == IP_VER6)
        pkt->hdr_len = IP6_HDR_SZ;
    else
#endif    
    pkt->hdr_len = IP4_HDR_SZ;
    pkt->dat = pkt->hdr + pkt->hdr_len;     /*Modified to support IPV6*/
    pkt->dat_len = TCP_HDR_SZ;

#ifdef IPV6_SUP
    if(soc->ver == IP_VER6)
        ip6h = (T_IP6_HDR *)pkt->hdr;
    else
#endif
    iph = (T_IP4_HDR *)pkt->hdr;
    tcphdr = (T_TCP_HDR *)pkt->dat;

    pdu_ptr = NULL; /* TCP Data pointer     */
    opt_len = 0;    /* TCP Option Length    */
    pdu_len = 0;    /* TCP Data Length      */
    snd_len = 0;    /* Length of Data in Send Buffer         */
    snd_max = 0;    /* Maximum bytes of data allowed to send */

    /* Pre-Initialize TCP Header fields */
    tcphdr->seq = tcp->snd_max;
    tcphdr->ack = 0;
    tcphdr->flag = 0;

    /* RST */
    ercd = 0;
    if (flag & TCP_FLG_RST) {
        tcphdr->seq = tcp->snd_max;
        tcphdr->ack = tcp->rcv_nxt; /* tcphdr->ack = 0; */
        tcphdr->flag = TCP_FLG_RST|TCP_FLG_ACK;
        tcphdr->win  = 0;
        net_sts_inc(NET_STS_TCP, NET_STS_TCP_OUT_RST);
        goto _tcp_send;
    }

    /* SYN */
    if (flag & TCP_FLG_SYN) {
        /* Set snd_una, snd_nxt here again, because in
           retry timer the snd_nxt is reset. */
        tcp->snd_una = tcp->iss;
        tcp->snd_nxt = tcp->iss + 1;

        tcphdr->seq = tcp->iss;
        tcphdr->flag = TCP_FLG_SYN;
        if (tcp->bsd != TCP_SYN_SENT) {
            flag |= TCP_FLG_ACK; /* Set ACK flag during SYN/ACK retry */
        }
        /* Build TCP Options */
        opt = (UB *)((UH*)&tcphdr->up + 1);
        *opt++ = 2;     /* Maximum Segment Size */
        *opt++ = 4;     /* Total Length of the fields: kind, len, value */
        *((UH *)opt) = htons((UH)tcp->mss); opt += 2;
        *opt++ = 1;     /* No operation */
        *opt++ = 0;     /* End of Option List */
        opt_len = opt - (UB *)((UH*)&tcphdr->up + 1);
        while (opt_len % 4) {
            *opt++ = 0; /* padding with 0*/
            opt_len++;
        }
        ercd = 1;
    }

    /* Unsent data in buffer? */
    snd_len = 0;
    if (tcp->snd_len != 0) {
        snd_len = (tcp->snd_nxt - tcp->snd_una);
        if (snd_len > tcp->snd_len)
            snd_len = 0;
        else
            snd_len = tcp->snd_len - snd_len;
    }

    /*
        New Data cannot be send,
            More than snd_wnd
            More than cwnd
        If all data is transmitted, send pending fin
    */
    if (snd_len && tcp->snd_wnd) {
        snd_max = ((tcp->snd_una + tcp->snd_wnd) - tcp->snd_nxt);
        snd_max = (snd_max > tcp->cwnd) ? tcp->cwnd : snd_max;   /*useable window*/
        snd_max = (snd_max > snd_len) ? snd_len : snd_max;
    }

_snd_more:

    /* snd_max bytes of data we have to send */
    /* if snd_max is zero then check fin can be send */
    pdu_len = 0;

    if (snd_max) {
        dat_ptr = pkt->dat + pkt->dat_len+ opt_len; /* data offset */
        pdu_len = (snd_max > tcp->mss) ? tcp->mss : snd_max;
        pdu_ptr = tcp->snd_buf + ((tcp->snd_nxt - (tcp->iss+1)) % tcp->sbufsz);
        if ((pdu_ptr + pdu_len) > (tcp->snd_buf + tcp->sbufsz)) {
            etc_len = (pdu_ptr + pdu_len) - (tcp->snd_buf + tcp->sbufsz);
            cpy_len = pdu_len - etc_len;
            net_memcpy(dat_ptr, pdu_ptr, cpy_len);
            net_memcpy((dat_ptr + cpy_len), tcp->snd_buf, etc_len);
        }
        else {
            net_memcpy(dat_ptr, pdu_ptr, pdu_len);
        }
        tcphdr->seq = tcp->snd_nxt;
        tcp->snd_nxt += pdu_len;
        snd_len -= pdu_len;
        snd_max -= pdu_len;
        flag |= TCP_FLG_ACK;
        if (snd_max == 0) {
            flag |= TCP_FLG_PSH;    /* Last data in this window     */
        }
        ercd = 1;
    }

    flag &= ~TCP_FLG_FIN;
    /* Check FIN */
    if ((snd_len == 0) && (tcp->sflg & TCP_FLG_FIN)) {

        /* Change State, if not done already */
        switch (tcp->bsd) {
            case TCP_SYN_RECEIVED:
                tcp->sflg &= ~TCP_FLG_SYN;
                tcp->tim_flg &= ~TCP_SET_RET;
                tcp->snd_una += 1;
                tcp->rtt = 0;
                tcp->rto = TCP_RTO_INI;
                /* no break */
                
            case TCP_ESTABLISHED:
                flag |= (TCP_FLG_FIN|TCP_FLG_ACK);
                ercd = 1;
                tcp->snd_nxt += 1;
                tcp->bsd = TCP_FIN_WAIT1;
                tcp->chg_flg = NET_STS_CHG;
                break;
            case TCP_CLOSE_WAIT:
                flag |= (TCP_FLG_FIN|TCP_FLG_ACK);
                ercd = 1;
                tcp->snd_nxt += 1;
                tcp->bsd = TCP_LAST_ACK;
                tcp->chg_flg = NET_STS_CHG;
                break;
            default:
                if ((tcp->snd_nxt + pdu_len + 1) == tcp->snd_max) {
                    flag |= (TCP_FLG_FIN|TCP_FLG_ACK);
                    ercd = 1;
                    tcphdr->seq = tcp->snd_una;  /* Retransmission of FIN? */
                }
                break;
        }
    }

    /* Nothing pending to send */
    if (flag == 0) {
        /* No SYN/FIN/Data */
        net_buf_ret(pkt);
        return;
    }

    /* ACK */
    if (flag & TCP_FLG_ACK) {

        tcphdr->ack = tcp->rcv_nxt;

        /* Clear delay ACK */
        if (tcp->tim_flg & TCP_SET_ACK) {
            tcp->tim_flg &= ~(UH)TCP_SET_ACK;
        }
    }

    if (ercd) {

        /* Set Retransmission Timer if not already set */
        if (!(tcp->tim_flg & TCP_SET_RET)) {
            tcp->ret_tmo  = NET_TICK + tcp->rto;
            tcp->tim_flg |= TCP_SET_RET;
        }

        /* Set RTT */
        if (tcp->rtt == 0) {
            tcp->rtt_seq = tcp->snd_nxt;
            tcp->rtt++;
        }
    }

    tcphdr->flag = flag;

_tcp_send:

    if (SEQ_GT(tcp->snd_nxt, tcp->snd_max)) {
        tcp->snd_max = tcp->snd_nxt;
    }

    pkt->dat_len += opt_len;    /* SYN Options */
    pkt->dat_len += pdu_len;    /* Data length */

    /* Fill TCP Header */
    tcphdr->seq = htonl(tcphdr->seq);
    tcphdr->ack = htonl(tcphdr->ack);
    tcphdr->win = htons(tcp_wnd_adv(tcp));
    tcphdr->sp  = htons(soc->lport);
    tcphdr->dp  = htons(soc->rport);
    tcphdr->cs  = 0;
    tcphdr->up  = 0;

#ifdef IPV6_SUP
    if(soc->ver == IP_VER6) {
        ip6h->pl     = htons(pkt->dat_len);
        ip6h->nh        = IP_PROTO_TCP;
        ip6h->hop       = net->ip6adr->hop;
        ip6_addr_cpy(ip6h->da, soc->raddr6);
        ip6_addr_cpy(ip6h->sa, soc->laddr6);
    }
    else {
#endif
    /* Fill IP Pseudo Header */
    iph->ver     = 0x45;
    iph->tos     = soc->tos;
    iph->tl      = htons(pkt->hdr_len + pkt->dat_len);
    iph->id      = htons(net->ident++);
    iph->fo      = 0;
    iph->ttl     = soc->ttl;
    iph->hc      = 0;
    iph->sa      = htonl(net->adr->ipaddr);
    iph->da      = htonl(soc->raddr);
    iph->prot    = IP_PROTO_TCP;
    if (net->flag & HW_CS_TX_IPH4)
        pkt->flg |= HW_CS_TX_IPH4;
    else
        iph->hc      = ip4_csum(pkt);

#ifdef IPV6_SUP
    }
#endif
    /* Calculate TCP Checksum */
    tcphdr->flag = (((TCP_HDR_SZ + opt_len) + 3)/4 << 12) | tcphdr->flag;
    tcphdr->flag = htons(tcphdr->flag);
    if (net->flag & HW_CS_TX_TCPUDP)
        pkt->flg |= HW_CS_TX_TCPUDP;
    else
#ifdef IPV6_SUP
    {
        if(soc->ver == IP_VER6)
            tcphdr->cs = ntohs(ip6_cksum(ip6h, pkt->dat));
        else
#endif
        tcphdr->cs = tcp_csum(pkt);
#ifdef IPV6_SUP
    }
#endif    
        
    pkt->net = net;
    pkt->soc = soc;
#ifdef IPV6_SUP
    if(soc->ver == IP_VER6) {
        ercd = ip6_snd(pkt);
        if ((ercd != E_WBLK) && (ercd < E_OK)) {
            return;
        }
    }
    else {
#endif
    pkt->dat = NULL;
    pkt->hdr_len = pkt->hdr_len + pkt->dat_len;
    ercd = net->out(pkt);
    if ((ercd != E_WBLK) && (ercd < E_OK)) {
        return;
    }
    if (rts_flg == 0 || rts_flg == 2) {
        net_sts_inc(NET_STS_TCP, NET_STS_TCP_OUT_SEG);
    } else {
        net_sts_inc(NET_STS_TCP, NET_STS_TCP_RETRANS_SEG);
    }
#ifdef IPV6_SUP
    }
#endif
    if (snd_max) {

        ercd = net_buf_get(&pkt, TCP_BUF_SIZE(tcp->mss), TMO_POL);
        if (ercd != E_OK) {
            return;
        }

        pkt->hdr = pkt->hdr + net->dev->hhdrsz + net->dev->hhdrofs;
#ifdef IPV6_SUP
        if(soc->ver == IP_VER6)
            pkt->hdr_len = IP6_HDR_SZ;
        else
#endif     
        pkt->hdr_len = IP4_HDR_SZ;
        pkt->dat = pkt->hdr + pkt->hdr_len;
        pkt->dat_len = TCP_HDR_SZ;

#ifdef IPV6_SUP
        if(soc->ver == IP_VER6)
            ip6h = (T_IP6_HDR *)pkt->hdr;
        else
#endif
        iph = (T_IP4_HDR *)pkt->hdr;
        tcphdr = (T_TCP_HDR *)pkt->dat;

        opt_len = 0;

        goto _snd_more;
    }

    return;
}

ER tcp_ack(T_NET_TCP *tcp, UB flag)
{
    T_NET     *net;
    T_NET_SOC *soc;
    T_NET_BUF *pkt;
    T_TCP_HDR *tcphdr;
    T_IP4_HDR *iph = NULL;
#ifdef IPV6_SUP
    T_IP6_HDR *ip6h = NULL;
#endif
    ER ercd;

    soc = tcp->soc;
    net = soc->net;

    ercd = net_buf_get(&pkt, TCP_BUF_SIZE(0), TMO_POL);
    if (ercd != E_OK) {
        return ercd;
    }

    pkt->hdr = pkt->hdr + net->dev->hhdrsz + net->dev->hhdrofs;

#ifdef IPV6_SUP
    if(soc->ver == IP_VER6)
        pkt->hdr_len = IP6_HDR_SZ;
    else
#endif     
    pkt->hdr_len = IP4_HDR_SZ;
    pkt->dat = pkt->hdr + pkt->hdr_len;
    pkt->dat_len = TCP_HDR_SZ;

#ifdef IPV6_SUP
    if(soc->ver == IP_VER6)
        ip6h    = (T_IP6_HDR *)pkt->hdr;
    else
#endif
    iph    = (T_IP4_HDR *)pkt->hdr;
    tcphdr = (T_TCP_HDR *)pkt->dat;

    if (flag & TCP_SET_PRB) {
        tcphdr->seq = htonl(tcp->snd_una-1);
    }
#ifdef KEEPALIVE_SUP
    else if (flag & TCP_SET_KPA) {
        tcphdr->seq = htonl(tcp->snd_max-1);
    }
#endif
    else {
        tcphdr->seq = htonl(tcp->snd_max);
    }
    tcphdr->ack = htonl(tcp->rcv_nxt);
    tcphdr->win = htons(tcp_wnd_adv(tcp));
    tcphdr->flag = TCP_FLG_ACK;

    tcphdr->sp  = htons(soc->lport);
    tcphdr->dp  = htons(soc->rport);
    tcphdr->cs  = 0;
    tcphdr->up  = 0;


#ifdef IPV6_SUP
    if(soc->ver == IP_VER6) {
        ip6h->pl    = htons(pkt->dat_len);
        ip6h->nh    = IP_PROTO_TCP;
        ip6h->hop       = net->ip6adr->hop;
        ip6_addr_cpy(ip6h->da, soc->raddr6);
        ip6_addr_cpy(ip6h->sa, soc->laddr6);
    }
    else {
#endif
    /* Fill IP Pseudo Header */
    iph->ver     = 0x45;
    iph->tos     = soc->tos;
    iph->tl      = htons(pkt->hdr_len + pkt->dat_len);
    iph->id      = htons(net->ident++);
    iph->fo      = 0;
    iph->ttl     = soc->ttl;
    iph->hc      = 0;
    iph->sa      = htonl(net->adr->ipaddr);
    iph->da      = htonl(soc->raddr);
    iph->prot    = IP_PROTO_TCP;
    if (net->flag & HW_CS_TX_IPH4)
        pkt->flg |= HW_CS_TX_IPH4;
    else
        iph->hc      = ip4_csum(pkt);

#ifdef IPV6_SUP
    }
#endif
    /* Calculate TCP Checksum */
    tcphdr->flag = ((pkt->dat_len + 3)/4 << 12) | tcphdr->flag;
    tcphdr->flag = htons(tcphdr->flag);
    if (net->flag & HW_CS_TX_TCPUDP)
        pkt->flg |= HW_CS_TX_TCPUDP;
    else
#ifdef IPV6_SUP
    {
        if(soc->ver == IP_VER6)
            tcphdr->cs = ntohs(ip6_cksum(ip6h, pkt->dat));
        else
#endif

            tcphdr->cs = tcp_csum(pkt);
#ifdef IPV6_SUP
    }
#endif

    pkt->net = net;
    pkt->soc = NULL;
#ifdef IPV6_SUP
    if(soc->ver == IP_VER6) {
        ercd = ip6_snd(pkt);
    }
    else {
#endif
    pkt->dat = NULL;
    pkt->hdr_len = pkt->hdr_len + pkt->dat_len;
    ercd = net->out(pkt);
#ifdef IPV6_SUP
    }
#endif
    return ercd;
}

ER tcp_out_rst(T_NET_BUF *rpkt, UH len)
{
    T_NET     *net;
    T_NET_BUF *pkt;
    T_TCP_HDR *tcphdr, *rtcphdr;
    T_IP4_HDR *iph = NULL;
    T_IP4_HDR *riph = NULL;
#ifdef IPV6_SUP
    T_IP6_HDR *ip6h = NULL;
    T_IP6_HDR *rip6h = NULL;
    UB ip_type;
#endif
    ER ercd;

#ifdef IPV6_SUP
    ip_type = *(rpkt->hdr)>>4;
    if(ip_type == IP_HDR_VER6)
        rip6h    = (T_IP6_HDR *)rpkt->hdr;
    else
#endif
    riph    = (T_IP4_HDR *)rpkt->hdr;
    rtcphdr = (T_TCP_HDR *)rpkt->dat;
    if (rtcphdr->flag & TCP_FLG_RST) {
        return E_OK;
    }

    /* RST Response for incoming packet */

    net = rpkt->net;

    ercd = net_buf_get(&pkt, TCP_BUF_SIZE(0), TMO_POL);
    if (ercd != E_OK) {
        return ercd;
    }

    pkt->hdr = pkt->hdr + net->dev->hhdrsz + net->dev->hhdrofs;
#ifdef IPV6_SUP
    if(ip_type == IP_HDR_VER6)
        pkt->hdr_len = IP6_HDR_SZ;
    else
#endif     
    pkt->hdr_len = IP4_HDR_SZ;
    pkt->dat = pkt->hdr + pkt->hdr_len;
    pkt->dat_len = TCP_HDR_SZ;

#ifdef IPV6_SUP
    if(ip_type == IP_HDR_VER6)
        ip6h    = (T_IP6_HDR *)pkt->hdr;
    else
#endif
    iph    = (T_IP4_HDR *)pkt->hdr;
    tcphdr = (T_TCP_HDR *)pkt->dat;

    if (rtcphdr->flag & TCP_FLG_ACK) {
        tcphdr->seq = rtcphdr->ack;
        tcphdr->ack = 0;
        tcphdr->flag = TCP_FLG_RST;
    }
    else {
        tcphdr->seq = 0;
        tcphdr->ack = rtcphdr->seq;

        if (rtcphdr->flag & TCP_FLG_SYN)
            tcphdr->ack++;

        if (rtcphdr->flag & TCP_FLG_FIN)
            tcphdr->ack++;

        tcphdr->flag = (TCP_FLG_RST|TCP_FLG_ACK);
    }

    tcphdr->sp = htons(rtcphdr->dp);
    tcphdr->dp = htons(rtcphdr->sp);
    tcphdr->seq = htonl(tcphdr->seq);
    tcphdr->ack = htonl(tcphdr->ack);
    tcphdr->win = rtcphdr->win;
    tcphdr->cs = 0;
    tcphdr->up = 0;

#ifdef IPV6_SUP
    if(ip_type == IP_HDR_VER6) {
        ip6h->pl    = htons(pkt->dat_len);
        ip6h->nh    = IP_PROTO_TCP;
        ip6h->hop       = net->ip6adr->hop;
        ip6_addr_hton(ip6h->da, rip6h->sa);
        ip6_addr_hton(ip6h->sa, rip6h->da);
    }
    else {
#endif
    /* Fill IP Pseudo Header */
    iph->ver     = 0x45;
    iph->tos     = IP4_TOS;
    iph->tl      = htons(pkt->hdr_len + pkt->dat_len);
    iph->id      = htons(net->ident++);
    iph->fo      = 0;
    iph->ttl     = IP4_TTL;
    iph->hc      = 0;
    iph->sa      = htonl(riph->da);
    iph->da      = htonl(riph->sa);
    iph->prot    = IP_PROTO_TCP;
    if (net->flag & HW_CS_TX_IPH4)
        pkt->flg |= HW_CS_TX_IPH4;
    else
        iph->hc   = ip4_csum(pkt);
#ifdef IPV6_SUP
    }
#endif

    /* Calculate TCP Checksum */
    tcphdr->flag = ((5 << 12) | tcphdr->flag);
    tcphdr->flag = htons(tcphdr->flag);
    if (net->flag & HW_CS_TX_TCPUDP)
        pkt->flg |= HW_CS_TX_TCPUDP;
    else
#ifdef IPV6_SUP
    {
        if(ip_type == IP_HDR_VER6)
            tcphdr->cs = ntohs(ip6_cksum(ip6h, pkt->dat));
        else
#endif

        tcphdr->cs = tcp_csum(pkt);
#ifdef IPV6_SUP
    }
#endif 

    pkt->net = net;
    pkt->soc = NULL;
#ifdef IPV6_SUP
    if(ip_type == IP_HDR_VER6) {
        ercd = ip6_snd(pkt);
    }
    else {
#endif
    pkt->dat = NULL;
    pkt->hdr_len = pkt->hdr_len + pkt->dat_len;
    ercd = net->out(pkt);
#ifdef IPV6_SUP
    }
#endif

    if (ercd == E_OK) {
        net_sts_inc(NET_STS_TCP, NET_STS_TCP_OUT_RST);
    }

    return ercd;
}

#endif

/*EOF*/
