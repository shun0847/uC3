/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK Application
    DHCP Server
    Copyright (c) 2010-2022, eForce Co., Ltd. All rights reserved.

    Version Information
      2010.06.01: Created
      2014.04.24: Modified to work with multiple network devices
      2015.03.23: 1. Define new callback ID DHCP_SERVER_ALT_EXPIREPOOL.
                     This ID is used when lease time is expired.
                  2. Define new callback ID DHCP_SERVER_ALT_RELPOOL.
                     This ID is used when DHCP release be received.
                  3. Modify callback ID
                     DCHP_SERVER_ALT_xxxx -> DHCP_SERVER_ALT.
      2015.04.20: 1. Bugfix. endian convert function(str_to_bin).
      2015.06.02: Bugfix. DHCP packet is at least as big as a BOOTP packet.
      2016.05.23: Suppress the GCC warning.
      2016.11.01: Corrected the following problems.
        1. Execute static analysis tool to this source.
        2. Support the receive of DHCPDECLINE.
        3. Improved check processing of the lease address.
        4. Support unicast transmission when request's src-address is specified.
      2016.12.12: Improvement to suppress warning of analysis tool.
      2017.01.17: Fixed a problem of receiving from multiple interfaces.
      2017.10.06: Added the following functions.
        1. Supported multiple DHCP scopes.
        2. Added a mechanism to notify users of some events.
      2018.01.18: Improved the decision rule of the response destination IP address.
      2022.10.14: Fixed incorrect parsing when 0-length DHCP options.
 ***************************************************************************/

#include "kernel.h"
#include "net_hdr.h"
#include "net_cfg.h"
#include "net_strlib.h"
#include "dhcp_server.h"


/* Magic Cookie byte data */
#define MAGICCOOKIE_1   99U
#define MAGICCOOKIE_2   130U
#define MAGICCOOKIE_3   83U
#define MAGICCOOKIE_4   99U
#define MAGICCOOKIE_LEN 4U

/* DHCP option length */
#define DHCP_LEN_PAD            1U
#define DHCP_LEN_SUBNET         4U
#define DHCP_LEN_ROUTER         4U
#define DHCP_LEN_IPLEASE        4U
#define DHCP_LEN_DHCPMSGTYPE    1U
#define DHCP_LEN_SERVERIDENT    4U

/* others */
#define S2B_SRC_STRLEN      -1              /* use source string length */
#define BROAD_CAST_ADDR     0xFFFFFFFFU
#define HOST_ROUTING_MASK   0xFFFFFFFFU

/** Global variable definition **/
T_DHCP_SERVER *gDHCP_SERVER[CFG_NET_DEV_MAX];

static const UB nomac[6] = { 0U, 0U, 0U, 0U, 0U, 0U };


#define FLG_RECV_STOPREQ    0x0001
#define FLG_RECV_STOPACK    0x0002
#define FLG_RECV_STARTREQ   0x0010
#define FLG_RECV_STARTACK   0x0020
#define FLG_RECV_REQ        (FLG_RECV_STOPREQ | FLG_RECV_STARTREQ)

static ER dhcpd_recv_start(T_DHCP_SERVER *dhcp)
{
    ER ercd;
    FLGPTN ptn;

    set_flg(dhcp->flg_id, FLG_RECV_STARTREQ);
    ercd = twai_flg(dhcp->flg_id, FLG_RECV_STARTACK, TWF_ORW, &ptn, DHCP_SERVER_RCVMBX_POLL);
    if (E_OK == ercd) {
        clr_flg(dhcp->flg_id, ~FLG_RECV_STARTACK);
    }

    return ercd;
}

static ER dhcpd_recv_stop(T_DHCP_SERVER *dhcp)
{
    ER ercd;
    FLGPTN ptn;

    set_flg(dhcp->flg_id, FLG_RECV_STOPREQ);
    ercd = twai_flg(dhcp->flg_id, FLG_RECV_STOPACK, TWF_ORW, &ptn, DHCP_SERVER_RCVMBX_POLL);
    if (E_OK == ercd) {
        clr_flg(dhcp->flg_id, ~FLG_RECV_STOPACK);
    }

    return ercd;
}


static UW str_to_bin( UB *dst, UB *src, UW size, UB mode )
{
    /* conv identification table         */
    static const UB *bin_tbl = (UB*)"0123456789ABCDEF";
    static const UB bit_tbl[2][16] = { /* conv binary table                 */
        {
            0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U,
            0x08U, 0x09U, 0x0AU, 0x0BU, 0x0CU, 0x0DU, 0x0EU, 0x0FU
        },
        {
            0x00U, 0x10U, 0x20U, 0x30U, 0x40U, 0x50U, 0x60U, 0x70U,
            0x80U, 0x90U, 0xA0U, 0xB0U, 0xC0U, 0xD0U, 0xE0U, 0xF0U
        }
    };

    UW   i;
    UW   j;
    UW  index;                  /* high, low bit switch              */
    UW  offset;                 /* convert offset                    */
    UB  byte;                   /* temp                              */
    UW  result;                 /* return code                       */

    /*-----------------------------------------------------------------------*/
    /* initialize variable                                                   */
    /*-----------------------------------------------------------------------*/
    byte        = 0x00U;
    index       = 0U;
    offset      = 0U;
    result      = 0U;

    if ( size == (UW)S2B_SRC_STRLEN ) {
        /* get string length                                                 */
        size = net_strlen( (VP)src );
    }
    if ( size == 0U ) {
        goto ERR;
    }

    if ( mode == DHCP_MODE_LITTLE_ENDIAN ) {
        /*-------------------------------------------------------------------*/
        /* little endian convert                                             */
        /*-------------------------------------------------------------------*/
        for ( i = size; i > 0U; i-- ) {
            /* search convert target                                         */
            for ( j = 0U; *(bin_tbl + j) != '\0'; j++ ) {
                /* convert target match?                                     */
                if ( *(src + i - 1U) == *(bin_tbl + j) ) {
                    /* set target binary                                     */
                    byte |= bit_tbl[index][j];
                    /* high byte -> low byte, low byte -> high byte          */
                    index ^= 1U;

                    break;
                }
            }

            /* no match data?                                                */
            if ( *(bin_tbl + j) == '\0' ) {
                /* improper data                                             */
                goto ERR;
            }

            /* set low bit?                                                  */
            if ( index == 0U ) {
                /* set convert byte data                                     */
                *(dst + offset) = byte;
                offset++;
                /* initialize binary data                                    */
                byte = 0x00U;
            }
        }
    }
    else {
        /*-------------------------------------------------------------------*/
        /* big endian convert                                                */
        /*-------------------------------------------------------------------*/
        for ( i = 0U; i < size; i++ ) {
            /* search convert target                                         */
            for ( j = 0U; *(bin_tbl + j) != '\0'; j++ ) {
                /* convert target match?                                     */
                if ( *(src + i) == *(bin_tbl + j) ) {
                    /* set target binary                                     */
                    byte |= bit_tbl[index ^ 1U][j];
                    /* high byte -> low byte, low byte -> high byte          */
                    index ^= 1U;

                    break;
                }
            }

            /* no match data?                                                */
            if ( *(bin_tbl + j) == '\0' ) {
                /* improper data                                             */
                goto ERR;
            }

            /* set low bit?                                                  */
            if ( index == 0U ) {
                /* set convert byte data                                     */
                *(dst + offset) = byte;
                offset++;
                /* initialize binary data                                    */
                byte = 0x00U;
            }
        }
    }

    /* set convert length                                                    */
    result = offset;

ERR:
    return result;
}


static void check_expiration( T_DHCP_SERVER *dhcp )
{
    UW      cnt;                        /* loop counter                      */
    UW      net_sec;                    /* net second                        */
    T_DHCP_SCOPE *scope;

    /* get current net second                                                */
    net_sec = get_net_sec();

    for (scope = dhcp->scope; (scope != NULL); scope = scope->next) {
        /* lease expiration check                                                */
        for ( cnt = 0U; cnt < (UW)scope->lease_num; cnt++ ) {
            /* target is not lease or infinite lease?                            */
            if ((scope->pool[cnt].expiration == DHCP_SERVER_INFINITE_LEASE) ||
                (scope->pool[cnt].expiration == DHCP_SERVER_NOT_LEASE)) {
                continue;
            }

            /* target is not expiration?                                         */
            if ( scope->pool[cnt].expiration >= net_sec ) {
                continue;
            }

            if (dhcp->alt != NULL) {
                dhcp->alt(DHCP_SERVER_ALT_EXPIREPOOL, &scope->pool[cnt]);
            }

            /* not if mac address registered in reserve(lease release)           */
            if ( cnt >= (UW)scope->reserve_num ) {
                /* set empty mac address                                         */
                net_memset( &scope->pool[cnt].mac[0], 0x00, sizeof(nomac) );
            }

            /* set not lease(lease release)                                      */
            scope->pool[cnt].expiration = DHCP_SERVER_NOT_LEASE;
        }
    }


    return;
}

static UB *dhcp_parse_opt_ptr(UB *ptr, UH len, UB opt)
{
    UB *p;
    UB code;
    UB tar;
    
    tar = 0;
    code = 1;
    /* parse target is DHCPv4 option format: [Code(1)][Len(1)][Data(Len)] */
    /* - option end when [Code] is 0xFF.            */
    /* - option padding when [Code] is 0x00.        */
    /* - If [Len] is 0, [Data] does not exist.      */
    for (p = ptr; p < (&ptr[len]); p++) {
        if (code) {     /* check [Code] */
            if (*p == DHCP_OPT_PAD) {           /* padding */
                continue;
            }
            else if (*p == DHCP_OPT_END) {      /* end */
                p = NULL;
                break;
            }
            tar = (*p == opt) ? 1 : 0 ;
            code = 0;
        }
        else {          /* check [Len] or [Data] */
            if (tar) {
                //p = (0 < *p) ? p + 1 : NULL ;   /* put NULL if there is no valid data */
                p++;   /* Data, or if Len is 0, next Code */
                break;
            }
            p += *p;            /* skip data */
            code = 1;           /* next code */
        }
    }
    
    /* If the pointer position exceeds the option size, return NULL. */
    if (p >= (&ptr[len])) {
        p = NULL;
    }
    
    return p;
}


static UH set_other_opt( T_DHCP_SCOPE *scope, T_DHCPD_MSG *snd_msg, UH n )
{
    UW str_len;
    UH i;
    UH j;

    /* set other options                                                     */
    for ( i = 0U; scope->other_opt[i].number != 0U; i++ ) {
        /* set option number                                                 */
        snd_msg->opt[n++] = scope->other_opt[i].number;

        /* set option length                                                 */
        snd_msg->opt[n++] = scope->other_opt[i].len;

        /* set option data                                                   */
        for ( j = 0U; scope->other_opt[i].data[j] != NULL; j++ ) {
            switch ( scope->other_opt[i].kind ) {
            case DHCP_OPT_KIND_ADDRESS: /* set address data                  */
                /* set address data                                          */
                ip_n2byte( (VP)&snd_msg->opt[n],
                           ip_aton( (const char *)scope->other_opt[i].data[j] ) );
                n += sizeof(UW);
                break;

            case DHCP_OPT_KIND_BINARY:  /* set binary data                   */
                /* set binary data                                           */
                n += (UH)str_to_bin( &snd_msg->opt[n],
                                 (UB *)scope->other_opt[i].data[j],
                                 (UW)S2B_SRC_STRLEN,
                                 scope->other_opt[i].conv_mode );
                break;

            default:                    /* set string data                   */
                /* set string data                                           */
                str_len = net_strlen( scope->other_opt[i].data[j] );
                net_memcpy( &snd_msg->opt[n],
                            scope->other_opt[i].data[j], str_len );
                n += (UH)str_len;
                break;
            }
        }
    }


    return n;
}

static UH dhcp_set_hdr(T_DHCP_SERVER *dhcp, UB msg, UB index)
{
    T_DHCPD_MSG  *snd_msg;
    T_DHCPD_MSG  *rcv_msg;
    T_NET_ADR   addr;
    T_DHCP_SCOPE *scope;
    UH n;

    snd_msg = &dhcp->ctl.snd_msg;
    rcv_msg = &dhcp->ctl.rcv_msg;
    scope = dhcp->current_sp;

    net_memset((VP)snd_msg, 0, DHCPD_HDR_LEN);
    snd_msg->op = DHCP_OPC_BOOTREPLY;
    snd_msg->htype = DHCP_ETH_TYPE;
    snd_msg->hlen  = DHCP_ETH_LEN;
    snd_msg->xid   = rcv_msg->xid;
    snd_msg->flags = rcv_msg->flags;
    net_memcpy(snd_msg->chaddr, rcv_msg->chaddr, sizeof(rcv_msg->chaddr));
    snd_msg->ciaddr = rcv_msg->ciaddr;

    if (msg != DHCP_MSG_NAK) {
        ip_n2byte((VP)&snd_msg->yiaddr, scope->pool[index].ipaddr);
    }
    snd_msg->giaddr = rcv_msg->giaddr;

    /* Options */
    n = 0U;

    net_memset( &snd_msg->opt[0], (INT)DHCP_OPT_PAD, sizeof(snd_msg->opt) );

    snd_msg->opt[n++] = MAGICCOOKIE_1;
    snd_msg->opt[n++] = MAGICCOOKIE_2;
    snd_msg->opt[n++] = MAGICCOOKIE_3;
    snd_msg->opt[n++] = MAGICCOOKIE_4;

    snd_msg->opt[n++] = DHCP_OPT_DHCPMSGTYPE;
    snd_msg->opt[n++] = DHCP_LEN_DHCPMSGTYPE;
    snd_msg->opt[n++] = msg;

    if (msg != DHCP_MSG_NAK) {

        snd_msg->opt[n++] = DHCP_OPT_SUBNET;
        snd_msg->opt[n++] = DHCP_LEN_SUBNET;
        ip_n2byte((VP)&snd_msg->opt[n], scope->subnet);
        n += DHCP_LEN_SUBNET;

        snd_msg->opt[n++] = DHCP_OPT_ROUTER;
        snd_msg->opt[n++] = DHCP_LEN_ROUTER;
        ip_n2byte((VP)&snd_msg->opt[n], scope->gateway);
        n += DHCP_LEN_ROUTER;

        snd_msg->opt[n++] = DHCP_OPT_IPLEASE;
        snd_msg->opt[n++] = DHCP_LEN_IPLEASE;
        ip_n2byte( (VP)&snd_msg->opt[n], scope->lease_period );
        n += DHCP_LEN_IPLEASE;

        /* set other options                                                 */
        if ( scope->other_opt != NULL ) {
            n = set_other_opt( scope, snd_msg, n );
        }
    }

    snd_msg->opt[n++] = DHCP_OPT_SERVERIDENT;
    snd_msg->opt[n++] = DHCP_LEN_SERVERIDENT;
    (void)net_ref( (UH)dhcp->dev_num, NET_IP4_CFG, (VP)&addr );
    ip_n2byte((VP)&snd_msg->opt[n], addr.ipaddr);
    n += DHCP_LEN_SERVERIDENT;

    if (dhcp->ctl.agent_info) {
        snd_msg->opt[n++] = dhcp->ctl.agent_info[-2];   /* Option Number (82) */
        snd_msg->opt[n++] = dhcp->ctl.agent_info[-1];   /* Option Length */
        net_memcpy(&snd_msg->opt[n], &dhcp->ctl.agent_info[0], dhcp->ctl.agent_info[-1]);
        n += dhcp->ctl.agent_info[-1];
    }

    snd_msg->opt[n++] = DHCP_OPT_END;

    /* Set the packet destination. */
    if (0U != rcv_msg->giaddr) {        /* relay agent */
        dhcp->ctl.cli_ipa = ntohl(rcv_msg->giaddr);
    }
    else if (msg != DHCP_MSG_NAK) {     /* not NAK */
        if (0U != rcv_msg->ciaddr) {
            dhcp->ctl.cli_ipa = ntohl(rcv_msg->ciaddr);
        }
        else if (ntohl(rcv_msg->flags) & DHCP_FLG_BCAST) {
            dhcp->ctl.cli_ipa = BROAD_CAST_ADDR;
        }
        else if (0U == dhcp->ctl.cli_ipa) {     /* invalid remote host addr */
            dhcp->ctl.cli_ipa = BROAD_CAST_ADDR;
        }
        else {  /* (0U != dhcp->ctl.cli_ipa) */
            ;   /* reply remote host */
        }
    }
    else {  /* (msg == DHCP_MSG_NAK) */
        dhcp->ctl.cli_ipa = BROAD_CAST_ADDR;
    }

    return n;
}

static ER dhcp_snd(T_DHCP_SERVER *dhcp, UW addr, UH len)
{
    T_NODE  host;
    ER ercd;

    /* msg length < minimal BOOTP length                                     */
    if ( len < DHCPD_BOOTP_MIN_LEN ) {
        /* set minimal len for BOOTP                                         */
        len = DHCPD_BOOTP_MIN_LEN;
    }

retry_snd:
    host.num  = dhcp->dev_num;
    host.ipa  = addr;
    host.port = (0 < dhcp->ctl.rcv_msg.giaddr) ? (UH)DHCP_AGENT_PORT : (UH)DHCP_CLIENT_PORT;
    host.ver  = IP_VER4;
    ercd = con_soc(dhcp->sid, &host, 0U);
    if (ercd != E_OK) {
        return ercd;
    }

    ercd = snd_soc(dhcp->sid, (VP)&dhcp->ctl.snd_msg, len);
    if ((E_TMOUT == ercd) && (addr != 0xFFFFFFFF)) {
        addr = 0xFFFFFFFF;
        goto retry_snd;
    }

    return ercd;
}

static ER dhcp_rcv(T_DHCP_SERVER *dhcp)
{
    ER ercd;
    T_DHCPD_MSG     *rcv_msg = &dhcp->ctl.rcv_msg;
    T_DHCP_MBX_MSG *msg;
    FLGPTN ptn;
    UB use_recv;

    use_recv = 1;
    for ( ; dhcp->server_tsk_stat != DHCP_SERVER_STOP ; ) {
        ercd = pol_flg(dhcp->flg_id, FLG_RECV_REQ, TWF_ORW, &ptn);
        if (E_OK == ercd) {
            clr_flg(dhcp->flg_id, ~FLG_RECV_REQ);
            if (ptn & FLG_RECV_STARTREQ) {
                set_flg(dhcp->flg_id, FLG_RECV_STARTACK);
                use_recv = 1;
            }
            else if (ptn & FLG_RECV_STOPREQ) {
                set_flg(dhcp->flg_id, FLG_RECV_STOPACK);
                use_recv = 0;
            }
        }

        ercd = trcv_mbx(dhcp->mbx_id, (T_MSG**)&msg, DHCP_SERVER_RCVMBX_POLL);

        /* timeout?                                                          */
        if ( ercd == E_TMOUT ) {
            /* expiration check                                              */
            check_expiration( dhcp );
            continue;
        }

        if (ercd != E_OK) {
            break;
        }

        if (use_recv) {
            dhcp->ctl.cli_ipa = msg->dhcp_msg.yiaddr;   /* remote host (temporary use) */
            msg->dhcp_msg.yiaddr = 0U;                  /* init zero */
            net_memcpy(rcv_msg, &msg->dhcp_msg, DHCPD_MSG_SZ);

            (void)rel_mpf(dhcp->mpf_id, (VP)msg);
        }
        else {
            (void)rel_mpf(dhcp->mpf_id, (VP)msg);
            continue;
        }

        if (rcv_msg->op != DHCP_OPC_BOOTREQ) {
            /* Ignore invalid message */
            net_memset(rcv_msg, 0, DHCPD_MSG_SZ);
            continue;
        }

        /* set option start address( option address + sizeof(Magic Cookie)) */
        dhcp->ctl.opt_ptr = rcv_msg->opt + MAGICCOOKIE_LEN;
        break;
    }


    /* server stop request?                                                  */
    if ( dhcp->server_tsk_stat == DHCP_SERVER_STOP ) {
        /* all mbx queue freeilast cleanupj                                */
        while (1) {
            ercd = prcv_mbx( dhcp->mbx_id, (T_MSG**)&msg );
            if (ercd == E_TMOUT) {
                break;
            }
            rel_mpf( dhcp->mpf_id, (VP)msg );
        }

        /* set server stopped code                                           */
        ercd = E_RLWAI;
    }

    return ercd;
}

static H dhcp_search(T_DHCP_SERVER *dhcp, T_DHCP_RESOURCE *key, BOOL iskey_mac)
{
    H index;
    H freemac;
    UB cnt;
    T_DHCP_SCOPE *scope;

    if (0 <= dhcp->lease_pcidx) {
        return dhcp->lease_pcidx;
    }

    freemac = -1;
    scope = dhcp->current_sp;
    index = (H)scope->pcidx;

    for (cnt = 0U; cnt < scope->lease_num; cnt++) {
        if (iskey_mac) {

            if (0 == net_memcmp(scope->pool[index].mac, key->mac, sizeof(nomac))) {
                break;
            }
            if (-1 == freemac) {
                if (0 == net_memcmp(scope->pool[index].mac, nomac, sizeof(nomac))) {
                    freemac = index;
                }
            }
        }
        else {

            if (scope->pool[index].ipaddr == key->ipaddr) {
                break;
            }
        }

        if (++index >= (H)scope->lease_num) {
            index = 0;
        }
    }

    if (cnt == scope->lease_num) {
        index = (iskey_mac) ? freemac : (H)-1;
    }
    else {
        scope->pcidx = (UB)index;
    }

    return (H)index;
}

static ER dhcp_offer(T_DHCP_SERVER *dhcp)
{
    UH snd_len;
    UH opt_len;
    UB *ptr;
    UB *opt_ptr;
    H index;
    T_DHCP_RESOURCE key = {0};
    ER ercd;

    opt_ptr = dhcp->ctl.opt_ptr;
    opt_len = dhcp->ctl.opt_len;

    /* DHCP Server */
    ptr = dhcp_parse_opt_ptr(opt_ptr, opt_len, DHCP_OPT_SERVERIDENT);

    /* can't accept DISCOVER with server id */
    if (ptr != NULL) {
        return E_OK;
    }

    /* get search key(mac) */
    net_memcpy(key.mac, dhcp->ctl.rcv_msg.chaddr, sizeof(key.mac));

    /* search chaddr from pool */
    index = dhcp_search(dhcp, &key, TRUE);

    /* lease data is nothing */
    if (index < 0) {

        if (dhcp->alt != NULL) {
            dhcp->alt(DHCP_SERVER_ALT_FULLPOOL, &key);
        }
        return E_OK;
    }

    /* make OFFER */
    snd_len = dhcp_set_hdr(dhcp, DHCP_MSG_OFFER, (UB)index);

    /* send message */
    ercd = dhcp_snd(dhcp, dhcp->ctl.cli_ipa, snd_len + DHCPD_HDR_LEN);

    return ercd;
}


static ER dhcp_ack_nack(T_DHCP_SERVER *dhcp)
{
    T_DHCP_RESOURCE key = {0};
    ER ercd;
    UB *ptr;
    UB *opt_ptr;
    UH snd_len;
    UH opt_len;
    H index;
    UB msg;
    T_DHCP_SCOPE *scope;

    opt_ptr = dhcp->ctl.opt_ptr;
    opt_len = dhcp->ctl.opt_len;
    scope = dhcp->current_sp;

    /* Requested IP address */
    ptr = dhcp_parse_opt_ptr(opt_ptr, opt_len, DHCP_OPT_REQIPADDR);

    /* get search key(ip) */
    if (ptr != NULL) {
        key.ipaddr = ip_byte2n((VP)ptr);
    }

    /* get search key(mac) */
    net_memcpy(key.mac, dhcp->ctl.rcv_msg.chaddr, sizeof(key.mac));

    /* REQUEST is init-reboot/selecting */
    if (key.ipaddr != 0U) {

        index = dhcp_search(dhcp, &key, FALSE);
        if (index < 0) {
            msg = DHCP_MSG_NAK;
        }
        else if (scope->pool[index].ipaddr != key.ipaddr) {
            msg = DHCP_MSG_NAK;
        }
        else if (net_memcmp(scope->pool[index].mac, nomac, sizeof(nomac)) == 0) {
            msg = DHCP_MSG_ACK;
        }
        else if (net_memcmp(scope->pool[index].mac, key.mac, sizeof(nomac)) == 0) {
            msg = DHCP_MSG_ACK;
        }
        else {
            msg = (0 <= dhcp->lease_pcidx) ? DHCP_MSG_ACK : DHCP_MSG_NAK ;
        }
    }
    /* REQUEST is rebind/renew */
    else {

        index = dhcp_search(dhcp, &key, TRUE);
        if (index < 0) {
            msg = DHCP_MSG_NAK;
        }
        else {
            msg = DHCP_MSG_ACK;
        }
    }

    /* case of ack                                                           */
    if ( msg == DHCP_MSG_ACK ) {
        /* set lease period                                                  */
        if ( scope->lease_period == DHCP_SERVER_INFINITE_LEASE ) {
            /* set infinite lease                                            */
            scope->pool[index].expiration = DHCP_SERVER_INFINITE_LEASE;
        }
        else {
            scope->pool[index].expiration = get_net_sec() + scope->lease_period;
        }
    }

    /* make ACK or NAK */
    snd_len = dhcp_set_hdr(dhcp, msg, (UB)index);

    /* send message */
    ercd = dhcp_snd(dhcp, dhcp->ctl.cli_ipa, snd_len + DHCPD_HDR_LEN);

    if ((ercd >= 0) && (msg == DHCP_MSG_ACK)) {
        net_memcpy(scope->pool[index].mac, key.mac, sizeof(nomac));

        if (dhcp->alt != NULL) {
            key.ipaddr = scope->pool[index].ipaddr;
            dhcp->alt(DHCP_SERVER_ALT_ENTRYPOOL, &key);
        }
    }

    return ercd;
}

static void dhcp_release(T_DHCP_SERVER *dhcp)
{
    T_DHCP_RESOURCE key = {0};          /* DHCP search key                   */
    H               index;              /* DHCP resource index               */
    UB              *opt_ptr;           /* parse target opt                  */
    UH              opt_len;            /* opt length                        */
    UB              *ptr;               /* parse opt func result             */
    T_DHCP_SCOPE *scope;

    /* initialize variable                                                   */
    opt_ptr = dhcp->ctl.opt_ptr;
    opt_len = dhcp->ctl.opt_len;

    /* Requested IP address                                                  */
    ptr = dhcp_parse_opt_ptr( opt_ptr, opt_len, DHCP_OPT_REQIPADDR );
    if ( ptr != NULL ) {
        /* get search key(ip)                                                */
        key.ipaddr = ip_byte2n( (VP)ptr );
    }

    /* get search key(mac)                                                   */
    net_memcpy( &key.mac[0], &dhcp->ctl.rcv_msg.chaddr[0], sizeof(key.mac) );

    /* get target dhcp resource                                              */
    if ( key.ipaddr != 0U ) {
        index = dhcp_search( dhcp, &key, FALSE );
    }
    else {
        index = dhcp_search( dhcp, &key, TRUE );
    }
    scope = dhcp->current_sp;

    /* dhcp_search function success?                                         */
    if ( index != -1 ) {

        if (dhcp->alt != NULL) {
            dhcp->alt(DHCP_SERVER_ALT_RELPOOL, &scope->pool[index]);
        }

        /* not if mac address registered in reserve                          */
        if ( (UB)index >= scope->reserve_num ) {
            /* set empty mac address                                         */
            net_memset( &scope->pool[index].mac[0], 0x00, sizeof(scope->pool[index].mac) );
        }
        /* set not lease                                                     */
        scope->pool[index].expiration = DHCP_SERVER_NOT_LEASE;
    }


    return;
}

static void dhcp_decline(T_DHCP_SERVER *dhcp)
{
    T_DHCP_RESOURCE key = {0};          /* DHCP search key                   */
    H               index;              /* DHCP resource index               */
    T_DHCP_SCOPE *scope;

    scope = dhcp->current_sp;

    /* get search key(mac) */
    net_memcpy( &key.mac[0], &dhcp->ctl.rcv_msg.chaddr[0], sizeof(key.mac) );

    index = dhcp_search( dhcp, &key, TRUE );
    if (index != -1 ) {
        if (0 == net_memcmp(scope->pool[index].mac, nomac, sizeof(nomac))) {
            index = -1;     /* exclude freemac */
        }
    }
    if (index != -1 ) {
        if (dhcp->alt != NULL) {
            dhcp->alt(DHCP_SERVER_ALT_DECLINE, &scope->pool[index]);
        }

        net_memset( &scope->pool[index].mac[0], 0x00, sizeof(scope->pool[index].mac) );
        scope->pool[index].expiration = DHCP_SERVER_NOT_LEASE;

        if (index == (H)scope->pcidx) {
            scope->pcidx++;
            if (scope->pcidx >= scope->lease_num) {
                scope->pcidx = 0U;
            }
        }
    }
}


static ER dhcp_proc(T_DHCP_SERVER *dhcp)
{
    ER ercd;
    UB *ptr;

    while (1) {
        net_memset(&dhcp->ctl.snd_msg, 0, sizeof(dhcp->ctl.snd_msg));
        net_memset(&dhcp->ctl.rcv_msg, 0, sizeof(dhcp->ctl.rcv_msg));

        ercd = dhcp_rcv(dhcp);
        if (ercd != E_OK) {
            break;
        }

        /* Clear scope */
        dhcp->current_sp = dhcp->scope;
        dhcp->lease_pcidx = -1;
        if (NULL == dhcp->current_sp) {
            continue;       /* invalidate scope */
        }

        /* set pointer at DHCP message type */
        ptr = dhcp_parse_opt_ptr(dhcp->ctl.opt_ptr, dhcp->ctl.opt_len, DHCP_OPT_DHCPMSGTYPE);
        if (ptr == NULL) {
            continue;
        }

        if (0 < dhcp->ctl.rcv_msg.giaddr) {     /* Receive from Relay agent */
            /* Set pointer at DHCP Agent Information */
            dhcp->ctl.agent_info = dhcp_parse_opt_ptr(dhcp->ctl.opt_ptr, dhcp->ctl.opt_len, DHCP_OPT_AGENTINFO);
        }
        else {  /* Receive from not Relay agent */
            dhcp->ctl.agent_info = NULL;
        }

        /* Notify receive packet */
        if (dhcp->evt) {
            ercd = dhcp->evt(dhcp, DHCP_SERVER_EVT_RECV);
            if (ercd != E_OK) {
                continue;
            }
        }

        if (*ptr == DHCP_MSG_DISCOVER) {
            ercd = dhcp_offer(dhcp);
            if (ercd < 0) {
                break;
            }
            continue;
        }

        if (*ptr == DHCP_MSG_REQUEST) {
            ercd = dhcp_ack_nack(dhcp);
            if (ercd < 0) {
                break;
            }
            continue;
        }

        /* allocated network address release(DHCP RELEASE)                   */
        if (*ptr == DHCP_MSG_RELEASE) {
            dhcp_release(dhcp);
            continue;
        }

        if (*ptr == DHCP_MSG_DECLINE) {
            dhcp_decline(dhcp);
            continue;
        }

        if (dhcp->alt != NULL) {
            T_DHCP_RESOURCE key = {0};
            net_memcpy(key.mac, dhcp->ctl.rcv_msg.chaddr, sizeof(key.mac));
            dhcp->alt(DHCP_SERVER_ALT_UNSUPPORTED, &key);
        }
    }

    return ercd;
}


static ER dhcpd_vld_scope(const T_DHCP_SCOPE *scope)
{
    ER ercd;
    UW host_cnt;                        /* assignable host value             */

    do {
        if (!scope) {
            ercd = E_PAR;
            break;
        }
        if ((scope->starting_addr == 0U) || (scope->lease_num == 0U)
          || (scope->lease_period == 0U)) {
            ercd = E_PAR;
            break;
        }

        /* cidr calc */
        if (scope->subnet == HOST_ROUTING_MASK) {
            /* host value = 1 */
            host_cnt = 1U;
        }
        else {
            /* ~subnet - 1 = pow(2, N) - 2 = assignable host value */
            host_cnt = ~scope->subnet;
            host_cnt -= (~scope->subnet) & scope->starting_addr;
        }

        if (host_cnt < (UW)scope->lease_num) {
            /* over lease number */
            ercd = E_PAR;
            break;
        }
        if (scope->reserve_num > scope->lease_num) {
            ercd = E_PAR;
            break;
        }

        ercd = E_OK;
    } while (0);

    return ercd;
}

ER dhcp_server_ini(T_DHCP_SERVER *dhcp)
{
    T_NODE host = {0};
    ER ercd;
    //UW host_cnt;                        /* assignable host value             */
    UB *rmac;
    UW cnt;
    T_DHCP_SCOPE *scope, *sp2;
    T_DHCP_RESOURCE *pool;

    ercd = E_OK;
    if (dhcp == NULL) {
        ercd = E_PAR;
    }
    else if ((dhcp->mpf_id == 0) || (dhcp->mbx_id == 0) || (dhcp->tsk_id == 0)) {
        ercd = E_PAR;
    }
    else if ((UW)dhcp->dev_num > NET_DEV_MAX) {
        ercd = E_PAR;
    }
    else {
        cnt = 0;
        for (scope = dhcp->scope; (scope != NULL); scope = scope->next) {
            ercd = dhcpd_vld_scope(scope);
            if (E_OK != ercd) {
                break;
            }

            /* multiple scopes range conflict check */
            for (sp2 = dhcp->scope; (sp2 != NULL); sp2 = sp2->next) {
                ercd = dhcpd_cmp_scope(scope, sp2);
                if (E_QOVR == ercd) {
                    ercd = E_OK;
                    continue;
                }
                if (E_OK != ercd) {
                    break;
                }
            }
            if (E_OK != ercd) {
                break;
            }

            cnt += scope->lease_num;
        }
        if (DHCP_LEASE_NUM < cnt) {
            ercd = E_NOMEM;
        }

        if (E_OK == ercd) {
            /* clear recv queue */
            ercd = cls_soc( dhcp->sid, 0U );
            if (E_OK != ercd) {
                ercd = E_PAR;
            }
            else {  /* rebind socket information */
                host.ver = IP_VER4;
                host.ipa = BROAD_CAST_ADDR;
                host.port = (UH)DHCP_CLIENT_PORT;
                ercd = con_soc(dhcp->sid, &host, 0);
            }
        }
    }
    if (ercd != E_OK) {
        goto ERR;
    }

    /* get own task id                                                       */
    ercd = get_tid( &dhcp->server_tsk_id );
    if ( ercd != E_OK ) {
        goto ERR;
    }


    ercd = net_cfg((UH)dhcp->dev_num, NET_BCAST_RCV, (VP)1);
    if (ercd != E_OK) {
        goto ERR;
    }

    pool = dhcp->pool_mst;
    for (scope = dhcp->scope; (scope != NULL); scope = scope->next) {
        if (scope->reserve_mac == NULL) {
            rmac = NULL;
        }
        else {
            rmac = (UB*)(VP)scope->reserve_mac;
        }
        net_memset(pool, 0, sizeof(*pool) * scope->lease_num);
        scope->pool = pool;
        pool += scope->lease_num;
        for (cnt = 0U; cnt < scope->lease_num; cnt++) {
            scope->pool[cnt].ipaddr = scope->starting_addr + (UW)cnt;

            if ((cnt < scope->reserve_num) && (rmac != NULL)) {
                net_memcpy(scope->pool[cnt].mac, rmac, sizeof(nomac));
                rmac += sizeof(nomac);
            }

            /* set not lease                                                     */
            scope->pool[cnt].expiration = DHCP_SERVER_NOT_LEASE;
        }

        scope->pcidx = 0U;   /* Initialize Address Pool Index */
    }

    /* Notify initialize complete */
    if (E_OK == ercd) {
        if (dhcp->evt) {
            ercd = dhcp->evt(dhcp, DHCP_SERVER_EVT_INIT);
        }
    }

ERR:
    return ercd;
}

ER dhcp_server(T_DHCP_SERVER *dhcp)
{
    ER ercd;

    ercd = dhcp_server_ini(dhcp);
    if (E_OK == ercd) {
        gDHCP_SERVER[dhcp->dev_num - 1U] = dhcp;
        /* boot dhcp rcv task                                                    */
        ercd = act_tsk( dhcp->tsk_id );
    }

    if (E_OK == ercd) {
        /* set dhcp server boot status                                           */
        dhcp->server_tsk_stat = DHCP_SERVER_BOOT;

        /* DHCP Server process */
        ercd = dhcp_proc(dhcp);

        /* clear recv queue                                                      */
        (void)cls_soc( dhcp->sid, 0U );
    }

    return ercd;
}

ER dhcp_server_stop( UW dev_num, UW retry )
{
    T_DHCP_SERVER   *dhcp;              /* dhcp server info                  */
    T_RTST          tskinfo;            /* task status info                  */
    UW              i;                  /* loop counter                      */
    ER              ercd;               /* function result                   */

    /*-----------------------------------------------------------------------*/
    /* check param                                                           */
    /*-----------------------------------------------------------------------*/
    if ((dev_num < 1U) || (dev_num > (UW)CFG_NET_DEV_MAX)) {
        ercd = E_PAR;
    }
    else {
        dhcp = gDHCP_SERVER[dev_num - 1U];
        if ( dhcp == NULL ) {
            ercd = E_PAR;
        }
        /* task id check                                                         */
        else if ((dhcp->tsk_id == TSK_SELF) || (dhcp->server_tsk_id == TSK_SELF)) {
            /* if TSK_SELF is set, error                                         */
            ercd = E_PAR;
        }
        else {
            ercd = E_OK;
        }
    }
    if (ercd != E_OK) {
        goto ERR;
    }

    /*-----------------------------------------------------------------------*/
    /* rcv task stop proc                                                    */
    /*-----------------------------------------------------------------------*/
    for (i = 0U; i <= retry; i++) {
        /* all cancel socket proc                                            */
        ercd = abt_soc( dhcp->sid, SOC_ABT_ALL );
        if ( ercd != E_OK ) {
            break;
        }

        /* wait retrying                                                     */
        dly_tsk( DHCP_SERVER_RETRY_WAIT );

        /* get task status                                                   */
        ercd = ref_tst( dhcp->tsk_id, &tskinfo );
        if ( ercd != E_OK ) {
            break;
        }

        /* stopped?                                                          */
        if ( tskinfo.tskstat == TTS_DMT ) {
            break;
        }
    }

    /* retry over check                                                      */
    if (i > retry) {
        ercd = E_TMOUT;
    }
    if (ercd != E_OK) {
        goto ERR;
    }

    /*-----------------------------------------------------------------------*/
    /* dhcp server task stop proc                                            */
    /*-----------------------------------------------------------------------*/
    /* set dhcp server stop flag                                             */
    dhcp->server_tsk_stat = DHCP_SERVER_STOP;
    for (i = 0U; i <= retry; i++) {
        /* wait retrying                                                     */
        (void)dly_tsk( DHCP_SERVER_RETRY_WAIT );

        /* get task status                                                   */
        ercd = ref_tst( dhcp->server_tsk_id, &tskinfo );
        if ( ercd != E_OK ) {
            break;
        }

        /* stopped?                                                          */
        if ( tskinfo.tskstat == TTS_DMT ) {
            break;
        }
    }

    /* retry over check                                                      */
    if (i > retry) {
        ercd = E_TMOUT;
    }
    if (ercd != E_OK) {
        goto ERR;
    }


ERR:
    return ercd;
}

/*******************************
        dhcp_rcv_tsk
 *******************************/
void dhcp_rcv_tsk(VP_INT exinf)
{
    T_DHCP_SERVER *dhcp = gDHCP_SERVER[(INT)exinf - 1];
    ER ercd;
    T_DHCP_MBX_MSG *msg;
    T_NODE host = {0};

    if (NULL == dhcp) {
        /* Before start this task, it must start the DHCP server task */
        return;
    }

    while (1) {

        ercd = get_mpf(dhcp->mpf_id, (VP*)&msg);
        if (ercd != E_OK) {
            return;
        }

        ercd = rcv_soc(dhcp->sid, (VP)&msg->dhcp_msg, DHCPD_MSG_SZ);
        if (ercd <= 0) {
            break;
        }

        if ((UH)ercd <= (DHCPD_HDR_LEN + 4U)) {
            rel_mpf(dhcp->mpf_id, (VP)msg);
            continue;
        }

        /* Get client IP addr for unicast */
        net_memset((char *)&host, 0, sizeof(host));
        (void)ref_soc(dhcp->sid, SOC_IP_REMOTE, (VP)&host);
        msg->dhcp_msg.yiaddr = host.ipa;    /* temporary use */

        /* set option length                                                 */
        /* pack len - (sizeof(msg->dhcp_msg) - sizeof(msg->dhcp_msg.opt) + 4 */
        dhcp->ctl.opt_len = (UH)ercd - (DHCPD_HDR_LEN + MAGICCOOKIE_LEN);

        ercd = snd_mbx(dhcp->mbx_id, (T_MSG*)msg);
        if (ercd != E_OK) {
            break;
        }
    }

    rel_mpf(dhcp->mpf_id, (VP)msg);
}



/*----------------------------------------------------------------------------*/
UB* dhcp_opt_get(T_DHCP_SERVER *dhcp, UB opt)
{
    return dhcp_parse_opt_ptr(dhcp->ctl.opt_ptr, dhcp->ctl.opt_len, opt);
}


T_DHCP_SCOPE *dhcpd_check_scope(T_DHCP_SERVER *dhcp, UW ipa)
{
    T_DHCP_SCOPE *scope = NULL;

    if (dhcp) {
        for (scope = dhcp->scope; (scope != NULL); scope = scope->next) {
            if ((scope->starting_addr <= ipa) && (ipa < (scope->starting_addr + scope->lease_num))) {
                break;
            }
        }
    }
    return scope;
}

/* Check range conflict */
ER dhcpd_cmp_scope(T_DHCP_SCOPE *sp1, T_DHCP_SCOPE *sp2)
{
    UW ipa;
    UB flg;

    if ((!sp1) || (!sp2)) {
        return E_PAR;
    }
    else if (sp1 == sp2) {
        return E_QOVR;      /* same address */
    }
    else if ((sp1->starting_addr == sp2->starting_addr) && (sp1->subnet == sp2->subnet)
      && (sp1->gateway == sp2->gateway) && (sp1->lease_num == sp2->lease_num) && (sp1->lease_period == sp2->lease_period)) {
        return E_NOEXS;     /* same value */
    }

    flg = 0;
    ipa = sp2->starting_addr;
    flg |= ((sp1->starting_addr <= ipa) && (ipa < (sp1->starting_addr + sp1->lease_num))) ? 0x01 : 0 ;
    ipa = sp2->starting_addr + sp2->lease_num - 1;
    flg |= ((sp1->starting_addr <= ipa) && (ipa < (sp1->starting_addr + sp1->lease_num))) ? 0x02 : 0 ;

    ipa = sp1->starting_addr;
    flg |= ((sp2->starting_addr <= ipa) && (ipa < (sp2->starting_addr + sp2->lease_num))) ? 0x04 : 0 ;
    ipa = sp1->starting_addr + sp1->lease_num - 1;
    flg |= ((sp2->starting_addr <= ipa) && (ipa < (sp2->starting_addr + sp2->lease_num))) ? 0x08 : 0 ;

    return (0 < flg) ? E_OBJ : E_OK;
}

ER dhcpd_chk_scope(T_DHCP_SERVER *dhcp, T_DHCP_SCOPE *chk)
{
    ER ercd;
    T_DHCP_SCOPE *scope;
    UH lease_sum;

    ercd = E_OK;
    if (!dhcp) {
        ercd = E_PAR;
    }
    else {
    ercd = dhcpd_vld_scope(chk);
    }
    if (E_OK != ercd) {
        return ercd;
    }

    lease_sum = 0;
    for (scope = dhcp->scope; (scope != NULL); scope = scope->next) {
        ercd = dhcpd_cmp_scope(scope, chk);
        if (E_OK != ercd) {
            break;
        }

        lease_sum += scope->lease_num;      /* total lease num */
    }

    if (E_OK == ercd) {
        if (chk->lease_num <= (DHCP_POOL_SIZE - lease_sum)) {
            ercd = E_OK;
        }
        else {
            ercd = E_NOMEM;
        }
    }

    return ercd;
}

ER dhcpd_add_scope(T_DHCP_SERVER *dhcp, T_DHCP_SCOPE *add, UB chk)
{
    ER ercd;
    T_DHCP_SCOPE *scope, *prvsc;
    UB *rmac;
    UW cnt;

    ercd = E_OK;
    if (0 != chk) {
        ercd = dhcpd_chk_scope(dhcp, add);
    }
    else if ((NULL == dhcp) || (NULL == add)) {
        ercd = E_PAR;
    }
        if (E_OK != ercd) {
            return ercd;
        }

    ercd = 0;
    prvsc = dhcp->scope;
    for (scope = dhcp->scope; (scope != NULL); scope = scope->next) {
        if (scope == add) {
            return E_OK;    /* already regist */
        }
        prvsc = scope;
        ercd += scope->lease_num;       /* lease number */
    }

    if (add->lease_num <= (DHCP_POOL_SIZE - ercd)) {
        add->pool = &dhcp->pool_mst[ercd];
        add->next = NULL;

        if (add->reserve_mac == NULL) {
            rmac = NULL;
        }
        else {
            rmac = (UB*)(VP)add->reserve_mac;
        }
        net_memset(add->pool, 0, sizeof(*add->pool) * add->lease_num);
        for (cnt = 0U; cnt < add->lease_num; cnt++) {
            add->pool[cnt].ipaddr = add->starting_addr + (UW)cnt;

            if ((cnt < add->reserve_num) && (rmac != NULL)) {
                net_memcpy(add->pool[cnt].mac, rmac, sizeof(nomac));
                rmac += sizeof(nomac);
            }

            /* set not lease */
            add->pool[cnt].expiration = DHCP_SERVER_NOT_LEASE;
        }
        add->pcidx = 0U;   /* Initialize Address Pool Index */

        if (prvsc) {
            prvsc->next = add;
        }
        else {
            dhcp->scope = add;
            dhcp->current_sp = dhcp->scope;
        }
        ercd = E_OK;
    }
    else {
        ercd = E_NOMEM;     /* no free space */
    }

    return ercd;
}



ER dhcpd_del_scope(T_DHCP_SERVER *dhcp, T_DHCP_SCOPE *del)
{
    ER ercd;
    T_DHCP_SCOPE *scope, *prvsc;

    ercd = E_OK;
    if ((NULL == dhcp) || (NULL == del)) {
        ercd = E_PAR;
    }
    if (E_OK != ercd) {
        return ercd;
    }

    prvsc = NULL;
    for (scope = dhcp->scope; (scope != NULL); scope = scope->next) {
        if (scope == del) {
            break;
        }
        prvsc = scope;
    }

    if (scope) {
        if (prvsc) {
            prvsc->next = del->next;
        }
        else {
            dhcp->scope = del->next;
        }
        if (scope == dhcp->current_sp) {
            dhcp->current_sp = dhcp->scope;
        }

        ercd = dhcpd_recv_stop(dhcp);   /* recv-data stop */
        /* Optimize DHCPd's pool buffer - skip delete buffer */
        prvsc = del;
        for (scope = prvsc->next; (scope != NULL); scope = scope->next) {
            net_memcpy(prvsc->pool, scope->pool, sizeof(*scope->pool) * scope->lease_num);
            prvsc = scope;
        }
        net_memset(prvsc->pool, 0, sizeof(*prvsc->pool) * prvsc->lease_num);

        /* Optimize DHCPd's pool buffer - reset buffer pointer */
        prvsc = del;
        for (scope = prvsc->next; (scope != NULL); scope = scope->next) {
            scope->pool = prvsc->pool;
            prvsc = scope;
        }
        (void)dhcpd_recv_start(dhcp);   /* recv-data start */

        ercd = E_OK;
    }
    else {
        ercd = E_OBJ;
    }

    return ercd;
}

ER dhcpd_change_scope(T_DHCP_SERVER *dhcp, UW ipa)
{
    ER ercd;
    T_DHCP_SCOPE *scope;

    ercd = E_OK;
    if (NULL == dhcp) {
        ercd = E_PAR;
    }
    else if (0 == ipa) {
        dhcp->current_sp = dhcp->scope;
        dhcp->lease_pcidx = -1;
    }
    else {
    dhcp->lease_pcidx = 0;
    scope = dhcpd_check_scope(dhcp, ipa);
    if (scope == NULL) {
            ercd = E_NOSPT;
    }
    else {
        dhcp->current_sp = scope;
        }
    }

    return ercd;
}

#if 0
UW dhcpd_check_lease_mac(T_DHCP_SERVER *dhcp, UB *mac)
{
    T_DHCP_SCOPE *scope;
    UW ret;
    UW i;

    ret = 0;
    for (scope = dhcp->scope; (scope != NULL); scope = scope->next) {
        for (i = 0; i < scope->lease_num; i++) {
            if (0 == net_memcmp(mac, scope->pool[i].mac)) {
                ret = scope->pool[i].ipaddr;
                break;
            }
        }
    }

    return ret;
}
#endif

ER dhcpd_next_lease(T_DHCP_SERVER *dhcp, UW ipa, UB *mac)
{
    ER ercd;
    T_DHCP_SCOPE *scope;

    if (0 == ipa) {
        ercd = E_PAR;
    }
    else {
    ercd = dhcpd_change_scope(dhcp, ipa);
        if (E_OK == ercd) {     /* Specify IP address */
            scope = dhcp->current_sp;
            dhcp->lease_pcidx = (UB)(ipa - scope->starting_addr);

            /* Check register MAC */
            if ((mac) && (DHCP_SERVER_NOT_LEASE != scope->pool[dhcp->lease_pcidx].expiration)) {
                ercd = net_memcmp(mac, scope->pool[dhcp->lease_pcidx].mac, 6);
                if (0 != ercd) {
                    dhcp->current_sp = dhcp->scope;
                    dhcp->lease_pcidx = -1;
                    ercd = E_OBJ;   /* Already other MAC registered. */
                }
            }
        }
    }

    return ercd;
}

