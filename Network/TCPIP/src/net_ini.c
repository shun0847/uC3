/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK
    Initilization and misc routines
    Copyright (c)  2008-2024, eForce Co., Ltd. All rights reserved.

    Version Information
      2008.11.19: Created
      2010.01.18: Initialize 'hhdrsz' field in T_NET_DEV
      2010.06.02: LITTLE_ENDIAN -> _UC3_ENDIAN_LITTLE
      2010.08.05: ip_aton() Parameter check
      2010.08.10: Added net_rand() & net_rand_seed()
      2010.11.02: Updated for IPv6 support
      2010.11.10: Updated for any network interface
      2011.03.07: Updated for received packet queue
      2011.03.11: Updated for IPv6 multiple interface
      2011.04.04: Changed IPv6 initial process
      2011.05.20: Moved NET_TCP_MAX from lib to application,
                  Set up networkbuffer offset
      2011.06.29: Check net_inftbl[2] in net_ini() for Compact.
      2011.11.14: Implemented Address Conflict Detection
      2011.12.12: Updated to terminate TCP/IP task in net_ext()
      2012.09.18: Corrected initialization size of gNET
      2012.10.02  Modify to avoid use of string libraries.
      2013.04.10  H/W OS supported version
      2013.08.06: Implememted multicast per socket
      2013.08.16: Modify to use low byte of MAC address for
                  random number seed
      2013.12.06  Change to external function to allocate buffer
      2014.03.06: SNMP option was added
      2014.05.16: Check ACD configuration parameter
      2014.08.20: Fixed release leak of semaphore.
                  (When STS_SUP definition is valid.)
      2014.11.14: Fixed release bug correspondence of
                  gcc compiler optimization (-O2).
                  (tcp_csum function.)
      2015.02.27: Modify for RZ/T1.
                  RZ/T1 is comiled with #define NET_S_OS and 
                  #define NET_HW_OS.
                  This modification is no effect at other CPU
      2015.12.14: The socket ID replaced SID types
      2016.01.07: Add MLDv1 functions
      2016.02.29: Change the call position of soc_reset
      2016.03.19  IGMP Version 3 support
      2016.06.24  Check net_sts_ini() result
      2016.10.11: Fixed not to transmit ARP from PPP device
      2017.01.18: Support 64bit processor
      2017.02.10: ip_aton(), ip_ntoa() Parameter check
      2017.07.14: Change the calling condition of soc_reset
      2018.08.23: Support POSIX IP_MULTICAST_LOOP feature
      2018.10.02: Call soc_ext() in net_ext()
      2020.10.13: Added explicit uNet3 control variable initialization.
      2021.03.17: Define the macro for each element number of net_inftbl[]
      2022.04.01: Add parameter "NET_PING_IGNORE" for net_cfg()/net_ref()
      2023.02.03: ip_aton() Parameter check
      2024.03.18: Add NET_IF_INACT for interface inactivate setting.
      2024.03.21: Disabled network interface while net_acd() is processing.
                  Then it is enabled only if net_acd() completes successfully.
 ***************************************************************************/

#include <stdio.h>
#include "kernel.h"
#include "net_hdr.h"
#include "net_def.h"
#include "net_sts_id.h"

#ifdef NET_S_OS
ID ID_NET_TSK=0;
ID ID_NET_SEM=0;
#endif

T_NET_SOC   *pNET_SOC;
T_NET_TCP   *pNET_TCP;

#ifdef NET_C
T_SOC_TABLE *pSOC_TABLE;
#endif

#ifdef IPR_SUP
T_NET_IPR   *pNET_IPR;
#endif
#ifdef MCAST_SUP
T_NET_MGR   *pNET_MGR;
#endif

UW TCP_SND_WND_MAX;

static UH ephemeral_port;


static void net_zero_memory(VP t, UW len)
{
    if (t != NULL) {
       net_memset(t, 0, len); 
    }
}

static void net_variable_ini(void)
{
    net_zero_memory(gNET, sizeof(T_NET) * NET_DEV_MAX);
    net_zero_memory(gNET_ARP, sizeof(T_NET_ARP) * NET_ARP_MAX);
    net_zero_memory(net_inftbl[NETINF_SOC_TBL], sizeof(T_NET_SOC) * NET_SOC_MAX);    /* gNET_SOC */
    net_zero_memory(net_inftbl[NETINF_TCP_TBL], sizeof(T_NET_TCP) * NET_TCP_MAX);    /* gNET_TCP */
    net_zero_memory(net_inftbl[NETINF_IPR_TBL], sizeof(T_NET_IPR) * NET_IPR_MAX);    /* gNET_IPR */
    net_zero_memory(net_inftbl[NETINF_MGR_TBL], sizeof(T_NET_MGR) * NET_MGR_MAX);    /* gNET_MGR */
    
    pNET_SOC = NULL;
    pNET_TCP = NULL;
#ifdef NET_C
    pSOC_TABLE = NULL;
#endif
#ifdef IPR_SUP
    pNET_IPR = NULL;
#endif
#ifdef MCAST_SUP
    pNET_MGR = NULL;
#endif
    
    /* No need for initialization */
    // net_zero_memory(net_inftbl[5], ...);    /* gTCP_SND_BUF */
    // net_inftbl[6] is loopback function pointer.
    // net_inftbl[7] is ipfowarding parameter.
}

ER net_ini(void)
{
    T_NET *net;
    UH num;
    ER ercd;
    SYSTIM rnd_tim;

#if defined(NET_C_OS) || defined(NET_S_OS)
#define rnd_tim rnd_tim.utime
#endif

#ifdef NET_SOC_MAX_LMT
    if (NET_SOC_MAX > NET_SOC_MAX_LMT) {
        return E_PAR;
    }
#endif

    if (NET_DEV_MAX == 0) {
        return E_PAR;
    }
    
    net_variable_ini();

    if (NET_SOC_MAX != 0) {
#ifdef NET_C
        pSOC_TABLE = (T_SOC_TABLE *)net_inftbl[NETINF_SOC_DEF];
        if (pSOC_TABLE == NULL) {
            return E_OBJ;
        }
#endif
        pNET_SOC = (T_NET_SOC *)net_inftbl[NETINF_SOC_TBL];
        if (pNET_SOC == NULL) {
            return E_OBJ;
        }
    }

    if (NET_TCP_MAX != 0) {
        pNET_TCP = (T_NET_TCP *)net_inftbl[NETINF_TCP_TBL];
        if (pNET_TCP == NULL) {
            return E_OBJ;
        }
        if (NET_SOC_MAX < NET_TCP_MAX) {
            return E_OBJ;
        }
    }

#ifdef IPR_SUP
    if (NET_IPR_MAX != 0) {
        pNET_IPR = (T_NET_IPR *)net_inftbl[NETINF_IPR_TBL];
        if (pNET_IPR == NULL) {
            return E_OBJ;
        }
    }
#endif

#ifdef MCAST_SUP
    if (NET_MGR_MAX != 0) {
        pNET_MGR = (T_NET_MGR *)net_inftbl[NETINF_MGR_TBL];
        if (pNET_MGR == NULL) {
            return E_OBJ;
        }
    }
#endif

#ifdef NET_S_OS
    /* Network Timer Task */
    ercd = acre_tsk((T_CTSK *)&c_net_tsk);
    if (ercd <= 0)
        goto ERR;
    ID_NET_TSK = ercd;

    /* Semaphore/Mutex for Network exclusive control */
    ercd = acre_sem((T_CSEM *)&c_net_sem);
    if (ercd <= 0)
        goto ERR;
    ID_NET_SEM = ercd;
#endif

    for (num=0;num<NET_DEV_MAX;num++) {
        if ((gNET_DEV[num].ini == NULL) ||
            (gNET_DEV[num].out == NULL)) {
            ercd = E_OBJ;
            goto ERR;
        }
        gNET_DEV[num].sts = NET_DEV_STS_NON;
    }

    /* Initialize Network buffer */

    ercd = net_buf_ini();
    if (ercd != E_OK) {
        goto ERR;
    }

    /* Initialize Global resources */

    TCP_SND_WND_MAX = 0;
    for (num=0;num<NET_DEV_MAX;num++) {
        net = &gNET[num];
        net_memset((char *)net, 0, sizeof(T_NET));
        if (gNET_CFG[0].use) {
            net->cfg = &gNET_CFG[0];
        }
        else {
            net->cfg = &gNET_CFG[num];
        }
        if (TCP_SND_WND_MAX < TCP_SND_WND) {
            TCP_SND_WND_MAX = TCP_SND_WND;
        }
        net->adr = &gNET_ADR[num];
        net->dev = &gNET_DEV[num];
        if (net->dev->type == NET_DEV_TYPE_ETH) {
            net->out = eth_pkt_out;
            net->dev->hhdrsz = ETH_HDR_SZ;
            net_memcpy((char *)&rnd_tim, (char *)net->dev->cfg.eth.mac+2, sizeof(rnd_tim));
            net_rand_seed(rnd_tim);
#ifdef MAC_RESOLVE
            net->flag |= (IP_RCV_BCAST);
#endif
        }
        else {
            net->out = ppp_pkt_out;
            net->dev->hhdrsz = PPP_HDR_SZ;
        }

        if (net->dev->hhdrofs == 0) {
            net->dev->hhdrofs = DEF_NET_BUF_OFFSET;
        }
        get_tim((SYSTIM*)&rnd_tim);
        net_rand_seed(rnd_tim);
    }

    /* Initialize Socket control block */

    soc_ini();

    /* Initialize ARP Table */

    arp_ini();

#ifdef MCAST_SUP
    /* Intialize IGMP */
    igmp_init();
#endif

    /* Initialize status */

    ercd = net_sts_ini();
    if (ercd != E_OK) {
        goto ERR;
    }

#ifdef IP4_FWD_SUP
    /* Initialize gateway */
    if (net_inftbl[NETINF_GATEWAY] != 0) {
        ercd = net_ip4_fwd_ini(net_inftbl[NETINF_GATEWAY]);
        if (ercd < 0)
            goto ERR;
    }
#endif

    /* Start Timer Task */

    ercd = sta_tsk(ID_NET_TSK, 0);
    if (ercd != E_OK)
        goto ERR;

#ifdef IPV6_SUP
    IPV6_TIMER_ON = 0;
#endif
    return E_OK;

ERR:
#ifdef NET_S_OS
    del_tsk(ID_NET_TSK);
    del_sem(ID_NET_SEM);
#endif
    net_memext();
    return ercd;
}

ER net_ext(void)
{
    UH devnum;
    ter_tsk(ID_NET_TSK);
#ifdef NET_S_OS
    del_tsk(ID_NET_TSK);
    del_sem(ID_NET_SEM);
    net_sts_ext();
#endif

    for (devnum = 1u; devnum <= NET_DEV_MAX; devnum++) {
        soc_ext(devnum);
    }
    net_memext();
    return E_OK;
}

/* Convert ASCII to IP */
/* Validation is not done */
UW ip_aton(const char *str)
{
    UW ip, dec;
    UW ip_cnt =0;
    UW dec_cnt =0;
    char *s;

    s = (char *)str;
    ip = dec = 0;

    if (s == NULL)
        return ip;

    for (;;) {

        if ((*s == '\0') || (*s == '.')) {
            if ((dec_cnt == 0) || (ip_cnt >= 4)) {
                return 0;
            }

            ip <<= 8;
            ip |= dec;
            ip_cnt++;
            dec_cnt = 0;
            dec = 0;
            if (*s == '\0') {
                if (ip_cnt != 4) {
                    return 0;
                }
                break;
            }
        }
        else {              /* Convert to Decimal */
            if (dec_cnt >= 3) {
                return 0;
            }
            dec_cnt++;
            dec *=10;
            dec += (*s - '0');
            if((*s < '0') || ('9' < *s)) {
                return 0;
            }
            if(dec > 255) {
                return 0;
            }
        }

        s++;
    }

    return ip;
}

/* 20090603 AK - Support for Big/Little endian */
void ip_ntoa(const char *s, UW ipaddr)
{
  int i, j;
  UB a, b[3];
  char *str;

    str = (char *)s;
    if (str == NULL) {
        return;
    }

    for (i=3;i>=0;i--) {
        a = (UB)(ipaddr >> (i*8));
        if (a == 0) {
            *str++ = '0';
        }
        else {
            for (j=0;j<3;j++) {
                b[j] = (a % 10) + '0';
                a /= 10;
                if (a == 0) break;
            }
            /* Note: j is in the range of 0 to 2. (j doesn't become 3.) */
            for (;j>=0;j--) {
                *str++ = (char)b[j];
            }
        }
        *str++ = '.';
    }
    *--str = '\0';
}

/* 20090603 AK - Support for Big/Little endian */
UW ip_byte2n(char *s)
{
    UW ip;
    UB b;

    ip = 0;

    if (s == NULL)
        return ip;

    b = s[0]; ip = (ip << 8) + b;
    b = s[1]; ip = (ip << 8) + b;
    b = s[2]; ip = (ip << 8) + b;
    b = s[3]; ip = (ip << 8) + b;

    return ip;
}

void ip_n2byte(char *s, UW ip)
{
    if (s == NULL)
        return;

    if (ip == 0) {
        net_memset(s, 0, 4);
        return;
    }

    *s++ = (char)(0xFF & (ip >> 24));
    *s++ = (char)(0xFF & (ip >> 16));
    *s++ = (char)(0xFF & (ip >> 8));
    *s++ = (char)(0xFF & (ip));

    return;
}

#ifdef _UC3_ENDIAN_LITTLE
/* Host ByteOrder encoding: 16 Bit */
UH htons(UH val)
{
    return (((val & 0xFF) << 8) | ((val & 0xFF00) >> 8));
}

/* Host ByteOrder encoding: 32 Bit */
UW htonl(UW val)
{
    return (((val & 0xFF) << 24) | ((val & 0xFF00) << 8) |
            ((val & 0xFF0000) >> 8) | ((val & 0xFF000000) >> 24));
}
#endif

/* MWC random number */
static UW rnd_z = 362436069;
static UW rnd_w = 521288629; 

void net_rand_seed(UW seed)
{
    rnd_z += seed;
    rnd_w += seed;
}

UW net_rand(void)
{
    rnd_z = 36969*(rnd_z&65535)+(rnd_z>>16);
    rnd_w = 18000*(rnd_w&65535)+(rnd_w>>16);

    return ((rnd_z << 16) + (rnd_w & 65535));
}

/* RFC 1071 - RFC 1624 */
UW net_csum(UH *dat, UH len, UW c)
{
    UW sum = c;

    while (len > 1) {
        sum += *((UH *)dat++);
        len -= 2;
    }

    if (len > 0) {
#ifdef _UC3_ENDIAN_LITTLE
        sum = sum + *((UB *)dat);
#else
        sum += (*dat) & 0xFF00;
#endif
    }

    return sum;
}

UH ip4_csum(T_NET_BUF *pkt)
{
    UW sum;

    sum  = net_csum((UH*)pkt->hdr, pkt->hdr_len, 0);
    while (sum>>16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (UH)~sum;
}

UH icmp_csum(T_NET_BUF *pkt)
{
    UW sum;

    sum  = net_csum((UH*)pkt->dat, pkt->dat_len, 0);
    while (sum>>16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (UH)~sum;
}

UH tcp_csum(T_NET_BUF *pkt)
{
    T_IP4_PSEUDO phdr = { 0 };
    T_IP4_HDR   *iph;
    UW sum;

    iph = (T_IP4_HDR *)pkt->hdr;

    phdr.sa    = iph->sa;
    phdr.da    = iph->da;
    phdr.proto = (UH)iph->prot;
    phdr.proto = htons(phdr.proto);
    phdr.len   = htons(pkt->dat_len);

    sum = net_csum((UH*)&phdr, sizeof(phdr), 0);
    sum = net_csum((UH*)pkt->dat, pkt->dat_len, sum);   /* TCP Header + Data */

    while (sum>>16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (UH)~sum;
}

void loc_tcp(void)
{
    wai_sem(ID_NET_SEM);
}

void ulc_tcp(void)
{
    sig_sem(ID_NET_SEM);
}

/* Get a unused port (EPHEMERAL_PORT to 65535) */

UH get_eport(UB proto)
{
    T_NET_SOC *soc;
    SID sid;

_get_eport:

    ephemeral_port++;

    if (ephemeral_port < EPHEMERAL_PORT) {
        ephemeral_port = EPHEMERAL_PORT;
    }

    for (sid=0;sid<NET_SOC_MAX;sid++) {

        soc = &pNET_SOC[sid];

        if (soc->state != SOC_UNUSED &&
            soc->proto == proto      &&
            soc->lport == ephemeral_port) {
            goto _get_eport;
        }
    }

    return ephemeral_port;
}

ER net_cfg(UH dev_num, UH opt, VP val)
{
    T_NET_ADR *addr;
    T_NET *net;
    ER ercd;

    if (dev_num > NET_DEV_MAX) {
        return E_ID;
    }

    if (dev_num != 0) {
        --dev_num;
    }

    net = get_net_bynum(dev_num);
    if (net == NULL) {
        return E_ID;
    }

    loc_tcp();

    ercd = E_OK;
    switch (opt) {
        case NET_IP4_CFG:
            addr = (T_NET_ADR *)val;
            if (TCP_AUTO_RST) {
                if (!(net->adr->ipaddr == addr->ipaddr &&
                      net->adr->mask == addr->mask)) {
                    soc_reset(net);
                }
            }
            net->adr->ipaddr  = addr->ipaddr;
            net->adr->mask    = addr->mask;
            net->adr->gateway = addr->gateway;
#ifndef ACD_SUP
            if (net->dev->type != NET_DEV_TYPE_PPP) {
                arp_send(net, addr->ipaddr, ARP_GRATUITOUS);  /*UnSolicited ARP*/
            }
#endif
            net_sts_cbk(dev_num + 1, EV_CBK_DEV_CHG_IP, NET_STS_CBK_DIS);
            break;
        case NET_IP4_TTL:
            IP4_TTL    = (UB)((ADDR)val);
            break;
#ifdef MCAST_SUP
#ifndef MCAST_SOC_SUP
        case NET_MCAST_JOIN:
            ercd = igmp_join(net, (ADDR)val);
            break;
        case NET_MCAST_DROP:
            ercd = igmp_leave(net, (ADDR)val);
            break;
#endif
        case NET_MCAST_TTL:
            IP4_MCAST_TTL  = (UB)((ADDR)val);
            break;
#endif
        case NET_BCAST_RCV:
            if ((UB)((ADDR)val) == 0)
                net->flag &= ~(UH)(IP_RCV_BCAST);
            else
                net->flag |= (IP_RCV_BCAST);
            break;
#ifdef ACD_SUP
        case NET_ACD_CBK:
            net->acdcbk = (ACD_HND)((ADDR)val);
            break;
#endif
#ifdef IPV6_SUP
#ifndef MCAST_SOC_SUP
        case NET_MLD_START:
            ercd = mld_add_adr((UW*)val, dev_num+1);
            break;
        case NET_MLD_STOP:
            ercd = mld_rmv_adr((UW*)val, dev_num+1);
            break;
#endif
#endif
        case NET_MCAST_LOOP:
            if (val) {
                net->flag |= MCAST_LOOPBK;
            } else {
                net->flag &= ~MCAST_LOOPBK;
            }
            break;

        case NET_PING_IGNORE:
            if (val) {
                net->cfg->PKT_CTL_FLG |= PKT_CTL_PING_IGNORE;
            } else {
                net->cfg->PKT_CTL_FLG &= ~PKT_CTL_PING_IGNORE;
            }
            break;

        case NET_IF_ACTIVE:
            if (val) {
                net->flag &= ~NET_IF_INACT;
            } else {
                net->flag |= NET_IF_INACT;
            }
            break;

        default:
            ercd = E_NOSPT;
            break;
    }

    ulc_tcp();

    return ercd;
}

ER net_ref(UH dev_num, UH opt, VP val)
{
    T_NET_ADR *addr;
    T_NET *net;
    ER ercd;

    if (dev_num > NET_DEV_MAX) {
        return E_ID;
    }

    if (dev_num != 0) {
        --dev_num;
    }

    net = get_net_bynum(dev_num);
    if (net == NULL) {
        return E_ID;
    }

    loc_tcp();

    ercd = E_OK;
    switch (opt) {
        case NET_IP4_CFG:
            addr         = (T_NET_ADR *)val;
            addr->ipaddr = net->adr->ipaddr;
            addr->mask   = net->adr->mask;
            addr->gateway = net->adr->gateway;
            break;
        case NET_IP4_TTL:
            (*(UB *)val) = IP4_TTL;
            break;
#ifdef MCAST_SUP
        case NET_MCAST_TTL:
            (*(UB *)val) = IP4_MCAST_TTL;
            break;
#endif
        case NET_BCAST_RCV:
            (*(UB *)val) = (net->flag & IP_RCV_BCAST);
            break;
        case NET_MCAST_LOOP:
            (*(UB *)val) = (net->flag & MCAST_LOOPBK);
            break;
        case NET_PING_IGNORE:
            (*(UB *)val) = (PING_IGNORE) ? 1 : 0 ;
            break;
        case NET_IF_ACTIVE:
            (*(UB *)val) = (net->flag & NET_IF_INACT) ? 0 : 1;
            break;
        default:
            ercd = E_NOSPT;
            break;
    }

    ulc_tcp();

    return ercd;
}

void add2que_end(UW **que, T_NET_BUF *pkt)
{
    T_NET_BUF *t;

    pkt->next = NULL;

    if (*que == NULL) {
        *que = (UW*)pkt;
        return;
    }

    /* Add the new packet to last */

    t = (T_NET_BUF *)*que;
    while (t) {
        if (t->next == NULL) {
            t->next = (UW *)pkt;
            break;
        }
        t = (T_NET_BUF *)t->next;
    }

    return;
}

void rmv4que_top(UW **que)
{
    T_NET_BUF *p;

    if (*que == NULL) {
        return;
    }

    p = (T_NET_BUF *)*que;
    *que = p->next;
    p->next = NULL;

    return;
}

#ifdef ACD_SUP
ER net_acd(UH dev_num, T_NET_ACD *acd)
{
    T_NET *net;
    ER ercd;
    int i;
    UW rand;

    if (acd == NULL) {
        return E_PAR;
    }

    if (dev_num > NET_DEV_MAX) {
        return E_ID;
    }

    if (dev_num != 0) {
        --dev_num;
    }

    net = get_net_bynum(dev_num);
    if (net == NULL) {
        return E_ID;
    }

    if (net->adr->ipaddr == 0) {
        return E_OBJ;
    }

    if ((net->prbtskid != 0) || (net->acd != NULL)){
        return E_OBJ;
    }

    loc_tcp();
    net_memset(acd, 0, sizeof(T_NET_ACD));
    net->acd = acd;

    can_wup(TSK_SELF);
    if (ARP_PRB_WAI) {
        rand = net_rand() % ARP_PRB_WAI;
    } else {
        rand = DEF_ARP_PRB_WAI;
    }
    ulc_tcp();
    tslp_tsk(rand);
    loc_tcp();

    get_tid(&net->prbtskid);
    ercd = arp_send(net, net->adr->ipaddr, ARP_PROBE_IP);
    if (ercd != E_OK) {
        net->prbtskid = 0;
        net->acd = NULL;
        ulc_tcp();
        return ercd;
    }

    for (i = 1; i < ARP_PRB_NUM; i++) {
        if (ARP_PRB_MAX - ARP_PRB_MIN > 0) {
            rand = net_rand() % (ARP_PRB_MAX - ARP_PRB_MIN);
        } else {
            rand = 0;
        }
        ulc_tcp();
        tslp_tsk(ARP_PRB_MIN + rand);
        loc_tcp();
        /* detect same address by arp module*/
        if (net->acd->ip != 0) {
            ercd = E_SYS;
            *acd = *net->acd;
            break;
        }

        ercd = arp_send(net, net->adr->ipaddr, ARP_PROBE_IP);
        if (ercd != E_OK) {
            break;
        }
    }

    if (i == ARP_PRB_NUM) {
        ulc_tcp();
        tslp_tsk(ARP_ANC_WAI);
        loc_tcp();
        /* detect same address by arp module*/
        if (net->acd->ip != 0) {
            ercd = E_SYS;
            *acd = *net->acd;
        } else {
        /* The host may begin legitimately using the IP address immediately
           after sending the first of the two ARP Announcements */
            net->prbtskid = 0;
            net->acd = NULL;
            for (i = 0; i < ARP_ANC_NUM; i++) {
                ercd = arp_send(net, net->adr->ipaddr, ARP_ANNOUNCE_IP);
                if (ercd != E_OK) {
                    break;
                }
                ulc_tcp();
                tslp_tsk(ARP_ANC_INT);
                loc_tcp();
            }
        }
    }

    net->prbtskid = 0;
    net->acd = NULL;
    ulc_tcp();
    return ercd;
}
#endif

/*EOF*/
