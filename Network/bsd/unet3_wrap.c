/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK
    BSD socket wrapper/uNet3 control function
    Copyright (c) 2014-2024, eForce Co., Ltd. All rights reserved.

    Version Information
      2014.04.28: Created
      2014.05.28: Corected select_u API
      2014.07.24: Fixed the problem of cancel message
      2015.03.20: Modified to support uNet3/Compact.
      2015.03.30: Fixed to bind can issue the same port to multiple FD.
      2015.03.31: Relocation listen queue at being empty
      2015.03.31: Fiexd port variable size at wrap_bind
      2015.04.16: Fixed the RX stream block condition after TCP data reception
      2015.07.30: Disable release buffer at BSD task side in cancel_api()
      2015.08.12: Modified to check validate FD in wrap_select()
      2015.08.17: Clear the number of backlog setting at TCP shutdown
      2015.10.07: Prevent listen the socket other than unconnected
      2015.12.14: The socket ID replaced SID types
      2016.02.05: Update for MSG_DONTWAIT for send
      2016.02.08: Corrected to bind the local port
      2016.02.08: Update for raw socket
      2016.05.11: Fixed an issue where ECONNREFUSED and EHOSTUNREACH is not properly set.
      2016.05.12: Modify the broadcast options
      2016.05.26: Modify Include file name for the HWOS
      2016.06.08: Add FIONREAD option for ioctl.
      2016.06.09: Add the MSG_PEEK option processing.
      2016.08.02: Fixed exclusive control for timeout process
      2016.08.26: Update for H/W OS
      2016.09.02: Fixed the flag control processing problem of MSG_PEEK
      2016.10.11: Fixed to return original device if port setting is error in bind()
      2016.11.02: Update select_u for POSIX adapter 
      2018.06.06: Fixed backlog control of listen queue
      2019.02.21: Update for case of TCP reset and case of losing socket.  
      2019.03.14: Change the errno for case of TCP reset and case of losing socket.
      2019.09.23: Correspondence when uNet3 cancel API is delayed.
      2020.03.23: Suppressed warning of the 64bit GCC compiler.
      2020.03.23: Fixed the omission of cls_soc().
      2020.06.10: Corrected the discrimination method of acceptable TCP socket.
      2020.09.07: Supported IPv6.
      2020.09.07: Fixed the initial value of timeout to be TMO_FEVR of uC3.
      2021.05.31: Added external declaration for R-IN32 memory allocator.
      2023.05.16: Fixed multicast control not working when creating/deleting socket.
      2023.06.23: Fixed multicast control not working when NET_MGR_MAX is 32 or more.
      2024.04.16: Fixed the problem of creating an unavailable socket
                  with socket() when using Compact.
      2024.09.20: Suppressed warning of after the fix on 2024.04.16.
      2024.10.02: Update to set_cre_soc() with net_soc.c update 2024.08.27.
 ***************************************************************************/

#include "kernel.h"
#include "net_hdr.h"
#include "net_def.h"

#include "unet3_socket.h"
#include "bsd_param.h"
#include "unet3_wrap.h"

/* for R-IN32M3 memory allocator */
#ifdef NET_HW_OS
extern UINT hwfnc_shortbuffer_get(UINT);
extern UINT hwfnc_buffer_release(UINT);
#endif

#ifdef NET_C_OS
#include "kernel_id.h"
#define cre_soc(_prto_,_host_)  get_bsd_soc(_prto_,_host_)
#define del_soc(_fd_)           rel_bsd_soc(_fd_)

/*---- Pool for socket management (only uNet3/Compact) ----*/
typedef struct t_bsd_soc_list {
    T_NET_SOC *mine;
    struct t_bsd_soc_list *next;
} T_BSD_SOC_LIST;

typedef struct t_bsd_soc_pool {
    T_BSD_SOC_LIST *use;    /* use socket list */
    T_BSD_SOC_LIST *empty;  /* unuse socket list */
    UW len;                 /* number of management socket */
    UW offset;              /* start offset position from gNET_SOC */
} T_BSD_SOC_POOL;

#if (0 < CFG_BSD_TCP_MAX)
static T_BSD_SOC_LIST gBSD_TCP_LIST[CFG_BSD_TCP_MAX];
static T_BSD_SOC_POOL gBSD_SL_TCP;
#endif
#if (0 < CFG_BSD_UDP_MAX)
static T_BSD_SOC_LIST gBSD_UDP_LIST[CFG_BSD_UDP_MAX];
static T_BSD_SOC_POOL gBSD_SL_UDP;
#endif

extern T_NET_SOC gNET_SOC[];

/* Initialization of BSD socket pool */
void bsd_socpool_ini(T_BSD_SOC_POOL *sl, T_BSD_SOC_LIST *sn, UH len, UH start)
{
    UW i;
    
    if (len) {
    start -= 1;
        for (i = 0; i < len; ++i) {
            sn[i].mine = &gNET_SOC[start + i];
            sn[i].next = ((i + 1) == len) ? NULL : &sn[i + 1];
        }
        sl->use   = NULL;
        sl->empty = sn;
        sl->len   = len;
        sl->offset= start;
    }
    else {
        sl->use   = NULL;
        sl->empty = NULL;
        sl->len   = 0;
        sl->offset= 0;
    }
}

/* Initialization of BSD wrapper */
void bsd_wrapper_ini(UW bsd_tcp_top, UW bsd_tcp_max, UW bsd_udp_top, UW bsd_udp_max)
{
#if (0 < CFG_BSD_TCP_MAX)
    bsd_socpool_ini(&gBSD_SL_TCP, gBSD_TCP_LIST, bsd_tcp_max, bsd_tcp_top);
#endif
#if (0 < CFG_BSD_UDP_MAX)
    bsd_socpool_ini(&gBSD_SL_UDP, gBSD_UDP_LIST, bsd_udp_max, bsd_udp_top);
#endif
}

/* Get socket from BSD socket pool */
ER bsd_socpool_pop(T_BSD_SOC_POOL *sl, T_NET_SOC **soc)
{
    ER ercd;
    T_BSD_SOC_LIST *tmp;
    UW i;
    
    ercd = E_OK;
    if (sl->empty) {
        /* push socket from unuse list */
        tmp         = sl->empty;
        sl->empty   = sl->empty->next;
        tmp->next   = NULL;
        *soc = tmp->mine;
        
        /* pop socket to use list */
        if (sl->use)    tmp->next = sl->use;
        sl->use = tmp;
        
        ercd = sl->offset + sl->len;
        for (i = sl->offset; i < ercd; ++i) {
            if (*soc == &gNET_SOC[i]) {
                break;
            }
        }
        ercd = (ercd == i) ? E_OBJ : (ER)i + 1;
    }
    else {
        ercd = E_NOMEM;
    }
    
    return ercd;
}

/* Release socket to BSD socket pool */
ER bsd_socpool_push(T_BSD_SOC_POOL *sl, T_NET_SOC *soc)
{
    T_BSD_SOC_LIST *tmp, *prev;
    ER ercd;
    
    ercd = E_OK;
    /* push target socket from use list */
    prev = NULL;
    for (tmp = sl->use; tmp != NULL; tmp = tmp->next) {
        if (tmp->mine == soc) {
            break;
        }
        prev = tmp;
    }
    if (tmp) {
        /* pop target socket to unuse list */
        if ((prev == NULL) && (tmp->next)) {    /* Head */
            sl->use = tmp->next;
        }
        else if ((prev) && (tmp->next)) {       /* Center */
            prev->next = tmp->next;
        }
        else if ((prev) && (tmp->next == NULL)) {   /* Foot */
            prev->next = NULL;
        }
        else {                                  /* One left */
                    sl->use = NULL;
        }

        /* pop target socket to unuse list */
        tmp->next = (sl->empty) ? sl->empty : NULL ;
        sl->empty = tmp;
    }
    else {
        ercd = E_OBJ;
    }
    
    return ercd;
}

/* Do cre_soc setting for the target socket. */
ER set_cre_soc(UB proto, T_NODE *host, T_NET_SOC *soc)
{
    T_NET *net;
    SID sid;
    UH local_port;
#ifdef MCAST_SUP
    UW *mgr_idx_list;
#endif    

    /* 1. Validate Parameters */

    /* non check Validate 'proto' */
    soc->proto = proto;     /* Set soc->proto first for release processing. */
    
    /* Validate 'host' */

    if (host == NULL) {
        return E_PAR;
    }

    if (host->ver != IP_VER4) {
#ifdef IPV6_SUP
        if (host->ver != IP_VER6)
#endif
            return E_NOSPT;
    }
    
#ifdef IPV6_SUP
    if(host->ver == IP_VER6) {
        if(ip6_addr_type(host->ip6a) != IP6_ADDR_UNSPEC)
        return E_NOSPT;
    }
    else
    {
#endif

    if (host->ipa != INADDR_ANY) {
        /* Not supported, Multiple address for single interface */
        return E_NOSPT;
    }
#ifdef IPV6_SUP
    }
#endif

    net = NULL;

    /* 2. Validate IP Address, Port */

    /* Local Interface */
    if (host->num != DEV_ANY) {
        net = get_net_bynum(host->num-1);
        if (net == NULL) {
            return E_PAR;
        }
    }

    local_port = host->port;
#ifdef PING_SUP
    if (proto == IP_PROTO_ICMP
#ifdef IPV6_SUP
        || proto == IP_PROTO_ICMPV6
#endif
        ) {
        local_port = 0;
    }
    else
#endif
    if (local_port == PORT_ANY) {
        if (proto != IP_PROTO_RAW) {
            local_port = get_eport(proto); /* Get a random port */
        }
    }
    
    /* not allocate socket */
    /* not call tcp_cre() */
    

    sid = soc->sid;
//    soc->sid   = sid;
    soc->state = SOC_CREATED;
    soc->lport = local_port;
    soc->rport = 0;
    
#ifdef IPV6_SUP
    soc->ver = host->ver;
    if(host->ver == IP_VER6)
        net_memset(soc->raddr6, 0, 16);
    else
#endif
    soc->raddr = 0;
    soc->net   = net;
    soc->usr_net = net;

    if (net) {
        soc->tos = IP4_TOS;
        soc->ttl = IP4_TTL;
        soc->rqsz= PKT_RCV_QUE;
    }
    else {
        soc->tos = DEF_IP4_TOS;
        soc->ttl = DEF_IP4_TTL;
        soc->rqsz= DEF_PKT_RCV_QUE;
    }

    soc->flg     = 0;
    soc->sdatque = NULL;
    soc->rdatque = NULL;

    soc->ctskid = 0;
    soc->stskid = 0;
    soc->rtskid = 0;
    soc->wptn   = 0;
    soc->cptn   = 0;
    soc->cbk    = NULL;

    soc->snd_tmo = TMO_FEVR;
    soc->rcv_tmo = TMO_FEVR;
    soc->con_tmo = TMO_FEVR;
    soc->cls_tmo = TMO_FEVR;
#ifdef SOC_TMO_LOC
    soc->loc_tmo = TMO_FEVR;
#endif
    
    soc->ercd    = E_OK;

    net_memset((char *)&soc->rpi, 0, sizeof(T_RCV_PKT_INF));
#ifdef MCAST_SUP
    if (soc->proto != IP_PROTO_TCP) {
        if (NET_MGR_MAX > 32) {
            if(net_inftbl[NETINF_MGR_INDEX] != NULL) {
            /* if MGR index exceeds 32 replace with extended area instead of mgr_idx */
                mgr_idx_list = (UW*)net_inftbl[NETINF_MGR_INDEX];
                mgr_idx_list += ((NET_MGR_MAX+31)/32) * (sid-NET_TCP_MAX); /* UDP index origin */
                soc->mgr_idx = (UW)mgr_idx_list;
            } else {
                return E_OBJ;
            }
        }
        clr_mgr_all_idx(soc);
    }
#endif
    if (host->port == PORT_ANY) {
        soc->flg |= SOC_FLG_PRT;    /* ANY PORT is specified */
    }

    soc->chg_flg = NET_STS_CHG;

    return (ER)(sid+1);
}

/* Do del_soc setting for the target socket. */
ER set_del_soc(SID sid)
{
    T_NET_SOC *soc;
    T_NET_BUF *pkt;
#ifdef MCAST_SUP
    int i;
#endif

    if ((sid == 0) || (sid > NET_SOC_MAX)) {
        return E_ID;
    }

    sid--;
    soc = &pNET_SOC[sid];

    if (soc->state == SOC_UNUSED) {
        return E_NOEXS;
    }
    
    /* not call tcp_del() */

    soc->state = SOC_UNUSED;

#if defined(UDP_SUP) || defined(PING_SUP)
    if (soc->proto != IP_PROTO_TCP) {

        if (soc->sdatque != NULL) {
            /*net_buf_ret((T_NET_BUF*)soc->sdatque);*/
            soc->sdatque = NULL;
        }

        while (soc->rdatque) {
            pkt = (T_NET_BUF*)soc->rdatque;
            rmv4que_top(&soc->rdatque);
            soc->rqsz++;
            net_buf_ret(pkt);
        }

        if (soc->wptn) {
            if (soc->wptn & EV_SOC_SND) {
                soc->slen = E_CLS;
                if ((soc->cptn & EV_SOC_SND) && (soc->cbk)) {
                    soc->cbk(soc->sid+1, EV_SOC_SND, E_CLS);
                }
                else {
                    wup_tsk(soc->stskid);
                    soc->stskid = 0;
                }
            }
            if (soc->wptn & EV_SOC_RCV) {
                soc->rlen = E_CLS;
                if ((soc->cptn & EV_SOC_RCV) && (soc->cbk)) {
                    soc->cbk(soc->sid+1, EV_SOC_RCV, E_CLS);
                }
                else {
                    wup_tsk(soc->rtskid);
                    soc->rtskid = 0;
                }
            }
        }
#ifdef MCAST_SUP
        if (soc->mgr_idx) {
            for (i = 0; i < NET_MGR_MAX; i++) {
                if (isset_mgr_idx(soc, i)) {
                    igmp_leave(pNET_MGR[i].net, pNET_MGR[i].ga);
                }
            }
        }
#endif
    }
#endif

    soc->chg_flg = NET_STS_CHG;

    return E_OK;
}

ER rel_bsd_soc(int sockfd)
{
    T_NET_SOC *soc = &gNET_SOC[sockfd - 1];
    ER ercd;
    
    ercd = E_OK;
    
    loc_tcp();
    switch (soc->proto) {
#if (0 < CFG_BSD_TCP_MAX)
    case IP_PROTO_TCP:
        ercd = bsd_socpool_push(&gBSD_SL_TCP, soc);
        break;
#endif
#if (0 < CFG_BSD_UDP_MAX)
    case IP_PROTO_UDP:
    case IP_PROTO_RAW:
        ercd = bsd_socpool_push(&gBSD_SL_UDP, soc);
        break;
#endif
        
    default:
        ercd = E_NOSPT;
        break;
    }
    if (ercd == E_OK) {
        set_del_soc(sockfd);
    }
    
    ulc_tcp();
    return ercd;
}

ER get_bsd_soc(UB proto, T_NODE *host)
{
    ER ercd;
    int sockfd;
    T_NET_SOC *soc;
    
    loc_tcp();
    
    switch (proto) {
#if (0 < CFG_BSD_TCP_MAX)
    case IP_PROTO_TCP:
        ercd = bsd_socpool_pop(&gBSD_SL_TCP, &soc);
        break;
#endif
#if (0 < CFG_BSD_UDP_MAX)
    case IP_PROTO_UDP:
    case IP_PROTO_RAW:
        ercd = bsd_socpool_pop(&gBSD_SL_UDP, &soc);
        break;
#endif
        
    default:
        ercd = E_NOSPT;
        break;
    }
    if (0 < ercd) {
        sockfd = ercd;
        ercd = set_cre_soc(proto, host, soc);
        if (0 > ercd) {
            ulc_tcp();
            rel_bsd_soc(sockfd);
            loc_tcp();
        }
    }
    ulc_tcp();
    
    return ercd;
}
#endif


/* select request queue */
static PNETQ  gselect_que;

/* errno */
int unet_errno;

int get_errno(void)
{
    ID tid;
    get_tid(&tid);

    return tsk_errno[tid];

}

void set_errno(ID tid, int err, int sockfd)
{
    T_UNET3_BSD_SOC    *soc;

    soc = ACTIVE_SOC(sockfd);

    /* set socket error */
    if (soc) {
        soc->so_err = err;
    }

    /* set task error */
    tsk_errno[tid] = err;

    /* set global error */
    unet_errno = err;
}

/* uNet3 socket API error -> errno */
int cnvrt_err(ER ercd)
{
    int err;
    switch(ercd) {
        case E_OK:
            err = 0;
            break;
        case E_PAR:
            err = UNET3_ER_EINVAL;
            break;
        case E_NOMEM:
            err = UNET3_ER_ENOMEM;
            break;
        case E_TMOUT:
            err = UNET3_ER_ETIMEDOUT;
            break;
        case E_ID:
        case E_NOEXS:
            err = UNET3_ER_EBADF;
            break;
        case E_WBLK:
        case E_QOVR:
            err = UNET3_ER_EAGAIN;
            break;
        case E_NOSPT:
            err = UNET3_ER_EPROTONOSUPPORT;
            break;
        case E_OBJ:
            err = UNET3_ER_EBADF;
            break;
        case E_CLS:
            err = UNET3_ER_EBADF;
            break;
        case E_SYS:
        default:
            err = UNET3_ER_ENODEV; /* unknown code */
            break;

    }
    return err;
}

static int add_que(T_UNET3_QUE **top, T_UNET3_QUE *obj)
{
    T_UNET3_QUE *tmp;
    int num;

    num = 1;
    obj->next = NULL;
    if (*top == NULL) {
        *top = obj;
        return num;
    }

    tmp = *top;
    while (tmp) {
        num++;
        if (tmp->next == NULL) {
            tmp->next = obj;
            break;
        }
        tmp = tmp->next;
    }
    return num;

}

static void rmv_que(T_UNET3_QUE **top, T_UNET3_QUE *obj)
{
    T_UNET3_QUE *tmp;

    tmp = *top;
    if (tmp == obj) {
        *top = obj->next;
        obj->next = NULL;
    }
    while (tmp != NULL) {
        if (tmp->next == obj) {
            tmp->next = obj->next;
            obj->next = NULL;
            break;
        }
        tmp = tmp->next;
    }
}

static void del_que(T_UNET3_QUE **top)
{
    T_UNET3_QUE *obj;

    if (*top == NULL) {
        return;
    }
    obj = *top;
    *top = obj->next;
    obj->next = NULL;
}

static ER add_apique(int sockfd, T_UNET3_API_MSG *msg)
{
    if (sockfd < 1) {
        return E_PAR;
    }
    if (API_TYPE_START <= msg->apitype && msg->apitype <= API_TYPE_END) {
        return add_que(&gNET_BSD_SOC[sockfd-1].apique[msg->apitype], (T_UNET3_QUE*)msg);
    }
#if (SYS_PLATFORM)
    if (msg->apiid == API_ID_SELECT_U) {
        return add_que(&gNET_BSD_SOC[sockfd-1].selectque, (T_UNET3_QUE*)msg);
    }
#endif
    return E_PAR;
}

static void del_msg(int sockfd, B apitype)
{
    if (sockfd < 1) {
        return;
    }
    if (API_TYPE_START <= apitype && apitype <= API_TYPE_END) {
        del_que(&gNET_BSD_SOC[sockfd-1].apique[apitype]);
    }
}

static T_UNET3_API_MSG *get_api_msg(int sockfd, B apitype)
{
    T_UNET3_API_MSG *apimsg = NULL;
#if !(SYS_PLATFORM)
    unet3_fd_set *fdset;
#endif

    if (sockfd < 1) {
        return NULL;
    }

    if (API_TYPE_START <= apitype && apitype <= API_TYPE_END) {
        apimsg = (T_UNET3_API_MSG*)gNET_BSD_SOC[sockfd-1].apique[apitype];
    }
#if (SYS_PLATFORM)
    else {
        apimsg = (T_UNET3_API_MSG*)gNET_BSD_SOC[sockfd-1].selectque;
    }
#else
    /* if ready event, search FD_SET at select que */
    else if (apitype == API_TYPE_SRDY) {
        apimsg = (T_UNET3_API_MSG*)gselect_que;
        while (apimsg) {
            fdset = (unet3_fd_set*)&apimsg->par.select.writefds;
            if (unet3_FD_ISSET(sockfd, fdset)) {
                apimsg->sockfd = sockfd;
                break;
            }
            apimsg = apimsg->next;
        }
    }
    else if (apitype == API_TYPE_RRDY) {
        /* in listen que*/
        if (LISTEN_SOC(sockfd)) {
            sockfd = gNET_BSD_SOC[sockfd-1].parentfd;
        }
        apimsg = (T_UNET3_API_MSG*)gselect_que;
        while (apimsg) {
            fdset = (unet3_fd_set*)&apimsg->par.select.readfds;
            if (unet3_FD_ISSET(sockfd, fdset)) {
                apimsg->sockfd = sockfd;
                break;
            }
            apimsg = apimsg->next;
        }
    }
#endif
    return apimsg;
}

static void rmv_api_msg(T_UNET3_API_MSG *msg)
{
    if (API_TYPE_START <= msg->apitype && msg->apitype <= API_TYPE_END) {
        del_msg(msg->sockfd, msg->apitype);
    }
    if (msg->apitype == API_TYPE_SRDY || msg->apitype == API_TYPE_RRDY) {
        rmv_que(&gselect_que, (T_UNET3_QUE*)msg);
    }
}

static ER unet3_callback(SID sid, UH event, ER err)
{
    T_UNET3_API_MSG *msg;
    ER ercd;

    ercd = E_OBJ;
    if (event & EV_SOC_CON) {
        ercd = msg_buf_get(&msg);
        if (ercd != E_OK) {
            return ercd;
        }
        msg->sockfd  = sid;
        msg->apitype = API_TYPE_CON;
        msg->apiid   = API_ID_uNET3_CALLBAK;
        msg->ercd  = err;
        msg->from  = API_MSG_CALLBCK;
        snd_mbx(MBX_BSD_REQ, (T_MSG*)msg);
    }

    if (event & EV_SOC_CLS) {
        ercd = msg_buf_get(&msg);
        if (ercd != E_OK) {
            return ercd;
        }
        msg->sockfd  = sid;
        msg->apitype = API_TYPE_CLS;
        msg->apiid   = API_ID_uNET3_CALLBAK;
        msg->ercd  = err;
        msg->from  = API_MSG_CALLBCK;
        snd_mbx(MBX_BSD_REQ, (T_MSG*)msg);
    }

    if (event & EV_SOC_SND) {
        ercd = msg_buf_get(&msg);
        if (ercd != E_OK) {
            return ercd;
        }
        msg->sockfd  = sid;
        msg->apitype = API_TYPE_SND;
        msg->apiid   = API_ID_uNET3_CALLBAK;
        msg->ercd  = err;
        msg->from  = API_MSG_CALLBCK;
        snd_mbx(MBX_BSD_REQ, (T_MSG*)msg);
    }

    if (event & EV_SOC_RCV) {
        ercd = msg_buf_get(&msg);
        if (ercd != E_OK) {
            return ercd;
        }
        msg->sockfd  = sid;
        msg->apitype = API_TYPE_RCV;
        msg->apiid   = API_ID_uNET3_CALLBAK;
        msg->ercd  = err;
        msg->from  = API_MSG_CALLBCK;
        snd_mbx(MBX_BSD_REQ, (T_MSG*)msg);
    }
    if (event & EV_RDY_SND) {
        ercd = msg_buf_get(&msg);
        if (ercd != E_OK) {
            return ercd;
        }
        msg->sockfd  = sid;
        msg->apitype = API_TYPE_SRDY;
        msg->apiid   = API_ID_uNET3_CALLBAK;
        msg->ercd  = err;
        msg->from  = API_MSG_CALLBCK;
        snd_mbx(MBX_BSD_REQ, (T_MSG*)msg);
    }
    if (event & EV_RDY_RCV) {
        ercd = msg_buf_get(&msg);
        if (ercd != E_OK) {
            return ercd;
        }
        msg->sockfd  = sid;
        msg->apitype = API_TYPE_RRDY;
        msg->apiid   = API_ID_uNET3_CALLBAK;
        msg->ercd  = err;
        msg->from  = API_MSG_CALLBCK;
        snd_mbx(MBX_BSD_REQ, (T_MSG*)msg);
    }
    return err;
}

static ER msg_buf_get_tmo(T_UNET3_API_MSG **msg, TMO tmo)
{
    ER ercd = E_OK;
    *msg = NULL;

#ifdef NET_HW_OS
    *msg = (T_UNET3_API_MSG*)hwfnc_shortbuffer_get(sizeof(T_UNET3_API_MSG));
#else
    ercd = tget_mpf(MPF_BSD_MSG, (VP *)msg, tmo);
#endif
    if (*msg == NULL || ercd != E_OK) {
        return E_NOMEM;
    }

    (*msg)->next = NULL;
    (*msg)->from = API_MSG_NONE;
    (*msg)->apitype = -1;
    (*msg)->aptskid = 0;
    (*msg)->msgtype = 0;
    (*msg)->sockfd  = 0;
    (*msg)->ercd = 0;
    (*msg)->cancel = NULL;

    return ercd;
}

/* non-wait */
ER msg_buf_get(T_UNET3_API_MSG **msg)
{
    return msg_buf_get_tmo(msg, TMO_POL);
}

ER msg_buf_wai_get(T_UNET3_API_MSG **msg)
{
    return msg_buf_get_tmo(msg, TMO_FEVR);
}

ER msg_buf_ret(T_UNET3_API_MSG *msg)
{
#ifdef NET_HW_OS
    hwfnc_buffer_release((UINT)msg);
#else
    rel_mpf(MPF_BSD_MSG, msg);
#endif
    return E_OK;
}

static void init_socket(int sockfd)
{
    T_UNET3_BSD_SOC *soc;
    UW evptn;

    soc = &gNET_BSD_SOC[sockfd-1];

    soc->backlog = 0;
    soc->parentfd = 0;
    soc->con_sts = TCP_CON_NOTCON;
    soc->rcvtm.tv_sec = TIME_FOREVER_SEC;
    soc->rcvtm.tv_usec = TIME_FOREVER_USEC;
    soc->sndtm.tv_sec = TIME_FOREVER_SEC;
    soc->sndtm.tv_usec = TIME_FOREVER_USEC;
    soc->clstm.l_linger = TMO_FEVR;
    soc->sockfd = sockfd;
    soc->ioasync = FALSE;
    soc->broadcast = FALSE;

    soc->hdrincl    = 0;
    soc->so_err     = 0;
    soc->lst_errno  = 0;
    soc->nxt_errno  = 0;
    soc->delflg     = 0;
    soc->so_flags   = 0;
    soc->mcif       = 0;
    net_memset(&soc->defaddr, 0, sizeof(soc->defaddr));

    soc->apique[API_TYPE_CON] = NULL;
    soc->apique[API_TYPE_CLS] = NULL;
    soc->apique[API_TYPE_SND] = NULL;
    soc->apique[API_TYPE_RCV] = NULL;
    soc->listenq = NULL;
#if (SYS_PLATFORM)
    soc->selectque = NULL;
#endif

    /* configuration */
    evptn = (EV_SOC_CON|EV_SOC_CLS|EV_SOC_SND|EV_SOC_RCV);
    evptn |= (EV_RDY_SND|EV_RDY_RCV);
    cfg_soc(sockfd, SOC_CBK_HND, (VP)unet3_callback);
    cfg_soc(sockfd, SOC_CBK_FLG, (VP)((ADDR)evptn));
}

static ER new_listener(int parentfd);

/* recursion called */
static void delete_socket(int sockfd)
{
    T_UNET3_BSD_SOC *soc;
    PNETQ que;
    unet3_fd_set *fdset;
    int i;
    ID aptskid;

    soc = ACTIVE_SOC_ALL(sockfd); 
    if (soc == NULL) {
        return;
    }

    /* API message clear without API_TYPE_CLS*/
    for (i = API_TYPE_START; i <= (API_TYPE_END - 1); i++) {
        while (soc->apique[i]) {
            que = soc->apique[i]->next;
            aptskid = ((T_UNET3_API_MSG*)soc->apique[i])->aptskid;
            if (aptskid != 0) {
                rel_wai(aptskid);
            } else {
                msg_buf_ret((T_UNET3_API_MSG*)soc->apique[i]);
            }
            soc->apique[i] = que;
        }
    }

#if (SYS_PLATFORM)
    /* select_u API message clear */
    while (soc->selectque) {
        que = soc->selectque->next;
        msg_buf_ret((T_UNET3_API_MSG*)soc->selectque);
        soc->selectque = que;
    }
#endif

    /* selecting FD clear */
    que = gselect_que;
    while (que) {
        fdset = (unet3_fd_set*)&((T_UNET3_API_MSG*)que)->par.select.readfds;
        unet3_FD_CLR(sockfd, fdset);
        fdset = (unet3_fd_set*)&((T_UNET3_API_MSG*)que)->par.select.writefds;
        unet3_FD_CLR(sockfd, fdset);
        que = que->next;
    }

    /* clean listen queue socket */
    while (soc->listenq) {
        /* abort session */
        abt_soc(((T_UNET3_BSD_SOC*)soc->listenq)->sockfd, SOC_ABT_ALL);
        delete_socket(((T_UNET3_BSD_SOC*)soc->listenq)->sockfd);
        del_que(&soc->listenq);
    }

    /* delete */
    del_soc(sockfd);

    soc->sockfd = soc->parentfd = 0;

}

static ER new_listener(int parentfd)
{
    T_UNET3_BSD_SOC *soc, *atta_soc;
    T_UNET3_API_MSG *msg;
    T_NODE local;
    int listenfd;
    ER ercd;

    soc = &gNET_BSD_SOC[parentfd-1];
    atta_soc = (T_UNET3_BSD_SOC *)soc->listenq;

    /* backlog */
    if (soc->backlog < 1) {
        return E_NOMEM;
    }

   /* take over device, localport, protocol, domain*/
    ref_soc(parentfd, SOC_IP_LOCAL, &local);

    local.ipa = 0; /* can't grant new host address */
#ifdef IPV6_SUP
    net_memset(local.ip6a, 0, sizeof(local.ip6a));
#endif

    ercd = cre_soc(gNET_BSD_SOC[parentfd-1].proto, &local);
    if (ercd <= 0) {
        soc->backlog = 0;
        return ercd;  /* TODO error*/
    }
    listenfd = ercd;

    /* setup */
    init_socket(listenfd);
    atta_soc = &gNET_BSD_SOC[listenfd-1];
    atta_soc->parentfd = parentfd; /* my parent */
    atta_soc->proto = IP_PROTO_TCP;

    ercd = con_soc(listenfd, &local, SOC_SER);
    if (ercd != E_WBLK) {
        /* error */
        del_soc(listenfd);  /* TODO error*/
        soc->backlog = 0;
        return ercd;
    }

    /* dummy API message for request passive open */
    ercd = msg_buf_get(&msg);
    if (ercd != E_OK) {
        del_soc(listenfd);
        soc->backlog = 0;
        return ercd;
    }
    msg->apitype = API_TYPE_CON;
    msg->apiid = API_ID_LISTEN;
    msg->sockfd = listenfd;
    msg->ercd = E_WBLK;   /* waiting SYN */
    msg->from = API_MSG_CALLBCK;

    add_apique(listenfd, msg);

    /* put listenq */
    add_que(&soc->listenq, (T_UNET3_QUE*)atta_soc);
    return listenfd;
}

/* function called  when I/O event occured without API message */
/* and also events that occur after the non-blocking API already completed */
static void event_socket(int sockfd, B apitype, ER code)
{
    UB sts;
    T_UNET3_BSD_SOC *soc, *psoc;
    UH flg;

    soc = ACTIVE_SOC_ALL(sockfd); 
    if (soc == NULL) {
        return;
    }
    if (soc->proto != IP_PROTO_TCP) {
        /* dont care UDP */
        return;
    }

    sts = TCP_CLOSED;
    ref_soc(sockfd, SOC_TCP_STATE, &sts);

    loc_tcp();
    flg = pNET_SOC[soc->sockfd-1].flg;
    ulc_tcp();

    if (apitype == API_TYPE_RRDY) {
        if (code <= 0) {

            /* closed RX stream */
            /* soc->con_sts = TCP_CON_DISCON_RX; */

            /* in listen que && RST by peer */
            psoc = ACTIVE_SOC(soc->parentfd);
            if ((psoc != NULL) && (sts == TCP_CLOSED)) {

                /* remove from listen queue */
                rmv_que(&psoc->listenq, (T_UNET3_QUE*)soc);
                delete_socket(sockfd);
                psoc->backlog++;

                /* alternate socket */
                if (psoc->backlog == 1) {
                    new_listener(psoc->sockfd);
                }

            }
        }
    } else if (apitype == API_TYPE_CON){

        /* active open */
        if ((flg & SOC_FLG_SER) == 0) {
            if (code == E_OK) {
                /* Active open Success is set last errno */
                soc->nxt_errno = soc->lst_errno;
                soc->con_sts = TCP_CON_CONNECT;
            } else {
                if ( code == E_TMOUT ) {
                    /* E_TMOUT is no route */
                    soc->nxt_errno = UNET3_ER_EHOSTUNREACH;
                } else {
                    /* E_CLS is RST */
                    soc->nxt_errno = UNET3_ER_ECONNREFUSED;
                }
                soc->con_sts = TCP_CON_NOTCON;
            }
            /* generate writable event to socket*/
            unet3_callback(sockfd, EV_RDY_SND, E_OK);
        }
    } else if (apitype == API_TYPE_CLS){

        /* close */
        if (soc->delflg) {
            delete_socket(sockfd);
        } 
    }
    return;
}

static int chk_soc_addr(T_UNET3_BSD_SOC *soc, struct unet3_sockaddr *addr, UB sup_family)
{
    ER ercd;
    
    if ((soc == NULL) || (addr == NULL)) {
        ercd = E_OBJ;
    }
    else {
        loc_tcp();
        ercd = (addr->sa_family == pNET_SOC[soc->sockfd-1].ver) ? E_OK : E_NOSPT;
        ulc_tcp();
        if (sup_family) {
            ercd = (addr->sa_family == sup_family) ? E_OK : ercd;
        }
    }

    return ercd;
}

static void wrap_socket(T_UNET3_API_MSG *msg)
{
    T_PARAM_SOCKET  *par;
    T_NODE node;
    int sockfd;

    par = &msg->par.socket;

    node.port = PORT_ANY;
    node.ver = par->domain;
    node.num = DEV_ANY;
    node.ipa = INADDR_ANY;

#ifdef IPV6_SUP
    net_memset(node.ip6a, 0, sizeof(node.ip6a));
#endif
    if (par->type == IP_PROTO_RAW) {
        node.port = par->protocol;
    }
    sockfd = cre_soc(par->type, &node);
    if (sockfd < 0) {
        msg->ercd = sockfd;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), 0);
        return;
    }

    /* setup */
    init_socket(sockfd);
    gNET_BSD_SOC[sockfd-1].proto = par->type;
    msg->sockfd = sockfd;
    msg->ercd = E_OK;
}

static void wrap_bind(T_UNET3_API_MSG *msg)
{
    T_PARAM_BIND       *par;
    int                devid;
    UW                 ip;
    UH                 port;
#ifdef IPV6_SUP
    UW                 ipv6[4];
    UB                 adrscope;
    unet3_in6_addr     anyaddr6 = UNET3_IN6ADDR_ANY_INIT;
#endif

    if (!VALID_SOC(msg->sockfd)) {
        msg->ercd = E_ID;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    devid = get_devid_soc(msg->sockfd);
    if (devid != DEV_ANY) {
        /* already bind */
        msg->ercd = E_PAR;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    par = &msg->par.bind;

    /* check valid address family */
    msg->ercd = chk_soc_addr(ACTIVE_SOC(msg->sockfd), (unet3_sockaddr*)&par->addr, 0);
    if (msg->ercd != E_OK) {
        set_errno(msg->aptskid, UNET3_ER_EAFNOSUPPORT, msg->sockfd);
        return;
    }

#ifdef IPV6_SUP
    if (((unet3_sockaddr*)(&par->addr))->sa_family == AF_INET) {
#endif
        ip = ((unet3_sockaddr_in*)(&par->addr))->sin_addr.s_addr;

        if (ip != INADDR_ANY) {
            devid = get_devid_addr(ntohl(ip));
            if (devid == DEV_ANY) {
                msg->ercd = E_PAR;
                set_errno(msg->aptskid, UNET3_ER_EADDRNOTAVAIL, msg->sockfd);
                return;
            }
            /* set network interface */
            set_devid_soc(msg->sockfd, devid);
        }
#ifdef IPV6_SUP
    } else {
        net_memcpy(ipv6, par->addr.sin6_addr.unet3_s6_addr32, sizeof(ipv6));

        if (ip6_addr_cmp(ipv6, anyaddr6.unet3_s6_addr32) == FALSE) {
            ip6_addr_ntoh(ipv6, ipv6);
            devid = get_devid_ip6addr(ipv6);
            if (devid == DEV_ANY) {
                msg->ercd = E_PAR;
                set_errno(msg->aptskid, UNET3_ER_EADDRNOTAVAIL, msg->sockfd);
                return;
            }
            /* check scope id */
            adrscope = ip6_addr_scope(ipv6);
            if ((adrscope == IP6_ADDR_LINK_LOCAL) && (par->addr.sin6_scope_id != devid)) {
                msg->ercd = E_PAR;
                set_errno(msg->aptskid, UNET3_ER_EADDRNOTAVAIL, msg->sockfd);
                return;
            }
            /* set network interface */
            set_devid_soc(msg->sockfd, devid);
        }
    }
#endif

    if (gNET_BSD_SOC[msg->sockfd - 1].proto == IP_PROTO_RAW) {
        return;
    }

    /* set local port*/
#ifdef IPV6_SUP
    if (((unet3_sockaddr*)(&par->addr))->sa_family == AF_INET) {
#endif
        port = ((unet3_sockaddr_in*)(&par->addr))->sin_port;
#ifdef IPV6_SUP
    } else {
        port = par->addr.sin6_port;
    }
#endif
    port = ntohs(port); /* uNet3 holds port number host byte order */

    if ( (gNET_BSD_SOC[msg->sockfd-1].so_flags & UNET3_OPT_REUSEADDR) == 0UL ) {
        msg->ercd = cfg_soc(msg->sockfd, SOC_PRT_LOCAL, (VP)((ADDR)port));
    } else {
        msg->ercd = cfg_soc(msg->sockfd, SOC_PRT_LOCAL_DUP, (VP)((ADDR)port));
    }
    /* already in use */
    if (msg->ercd == E_PAR) {
        set_errno(msg->aptskid, UNET3_ER_EADDRINUSE, msg->sockfd);
    }

    if (msg->ercd != E_OK) {
        /* rollback */
        loc_tcp();
        pNET_SOC[msg->sockfd-1].usr_net = pNET_SOC[msg->sockfd-1].net = NULL;
        ulc_tcp();
    }
}

static void wrap_connect(T_UNET3_API_MSG *msg)
{
    T_PARAM_CONNECT    *par;
    T_UNET3_BSD_SOC    *soc;
    T_NODE             remote;
    UB                 sts;
    T_NET              *usr_net;

    soc = ACTIVE_SOC(msg->sockfd);
    if (soc == NULL) {
        msg->ercd = E_ID;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    par = &msg->par.connect;

    /* call from uNet3 */
    if (msg->from == API_MSG_CALLBCK) {
        /* callback of TCP/active open indicates process is done */
        if (msg->ercd == E_OK) {
            soc->con_sts = TCP_CON_CONNECT;
        } else {
            soc->con_sts = TCP_CON_NOTCON;
            set_errno(msg->aptskid, UNET3_ER_ECONNREFUSED, msg->sockfd);
        }
        /* generate writable event to socket*/
        unet3_callback(soc->sockfd, EV_RDY_SND, msg->ercd);
        return;
    }

    /* call from application */
    if (msg->msgtype == MSG_TYPE_CANCEL) {
        rmv_que(&soc->apique[API_TYPE_CON], (T_UNET3_QUE*)msg->cancel);
        abt_soc(msg->sockfd, SOC_ABT_CON);
        msg->ercd = E_OK;
        return;
    }

    /* active open can't wait for multiple */
    if (soc->apique[API_TYPE_CON]) {
        msg->ercd = E_OBJ;
        set_errno(msg->aptskid, UNET3_ER_EALREADY, msg->sockfd);
        return;
    }

    /* listen socket can't connect */
    if (soc->listenq) {
        msg->ercd = E_OBJ;
        set_errno(msg->aptskid, UNET3_ER_EISCONN, msg->sockfd);
        return;
    }

    /* check valid address family */
    msg->ercd = chk_soc_addr(soc, (unet3_sockaddr*)&par->addr, UNET3_AF_UNSPEC);
    if (msg->ercd != E_OK) {
        set_errno(msg->aptskid, UNET3_ER_EAFNOSUPPORT, msg->sockfd);
        return;
    }

    net_memset(&remote, 0, sizeof(remote));
#ifdef IPV6_SUP
    if (((unet3_sockaddr*)(&par->addr))->sa_family == AF_INET) {
#endif
        SIN_NODE((unet3_sockaddr_in*)&par->addr, &remote);
#ifdef IPV6_SUP
    } else {
        SIN6_NODE(&par->addr, &remote);
    }
#endif

    /* AF_UNSPEC is destination connect close */
    if ((((unet3_sockaddr*)(&par->addr))->sa_family) == UNET3_AF_UNSPEC) {
        if (soc->proto == IP_PROTO_TCP ) {
            abt_soc(msg->sockfd, SOC_ABT_ALL);
        } else {
            cls_soc(msg->sockfd, SOC_TCP_CLS);
        }
        net_memset(&soc->defaddr, 0, sizeof(soc->defaddr));
        msg->ercd = E_OK;
        return;
    }

    /* destination is set through connect() API for UDP socket */
    if (soc->proto == IP_PROTO_UDP ) {
        if (remote.ipa == 0 && remote.port == 0) {
            net_memset(&soc->defaddr, 0, sizeof(soc->defaddr));
            msg->ercd = E_OK;
            return;
        }
    } else if (soc->proto == IP_PROTO_TCP ) {
        /* If the asynchronous and next errno is set */
        if ( soc->ioasync && soc->nxt_errno != 0 ) {
            set_errno(msg->aptskid, soc->nxt_errno, msg->sockfd);
            if ( soc->con_sts == TCP_CON_NOTCON ) {
                msg->ercd = -1;
            } else {
                msg->ercd = 0;
            }
            soc->lst_errno = 0;
            soc->nxt_errno = 0;
            return;
        }
        
        ref_soc(msg->sockfd, SOC_TCP_STATE, &sts);
        switch ( sts ) {
        case TCP_CLOSED:
            break;
        case TCP_LISTEN:            /* for listen */
        case TCP_SYN_SENT:          /* for connect */
        case TCP_SYN_RECEIVED:      /* for accept */
            msg->ercd = E_OBJ;
            if ( soc->ioasync ) {
                soc->lst_errno = UNET3_ER_EALREADY;
            }
            set_errno(msg->aptskid, UNET3_ER_EALREADY, msg->sockfd);
            return;
        default:                    /* Later ESTABLISHED */
            msg->ercd = E_OBJ;
            set_errno(msg->aptskid, UNET3_ER_EISCONN, msg->sockfd);
            return;
        }
    } else {
        msg->ercd = E_OK;
        soc->defaddr = remote;
        return;
    }

    /* for uNet3 ver4 series specification */
    usr_net = pNET_SOC[msg->sockfd-1].usr_net;
    if (usr_net != NULL) {
        remote.num = usr_net->dev->num;
    }

    msg->ercd = con_soc(msg->sockfd, &remote, SOC_CLI);

    if ( soc->ioasync ) {
        soc->lst_errno = cnvrt_err(msg->ercd);
    }

    set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);

    if ((msg->ercd == E_OK) && (soc->proto == IP_PROTO_UDP)) {
        soc->defaddr = remote;
    }

    if (soc->ioasync && msg->ercd == E_WBLK) {
        msg->ercd = -1;         /* non-block without Add Que */
    }
}

static void wrap_listen(T_UNET3_API_MSG *msg)
{
    T_UNET3_BSD_SOC    *soc;
    T_NODE             node = {0};  /* local, remote*/
    int                sockfd;
    UB              sts;

    soc = ACTIVE_SOC_ALL(msg->sockfd);
    if (soc == NULL) {
        msg->ercd = E_ID;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }
    else if (soc->proto != IP_PROTO_TCP) {
        msg->ercd = E_NOSPT;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    sockfd = msg->sockfd;

    /* call from application */
    if (msg->from == API_MSG_APPLICATION) {

        /* already listening */
        if (soc->listenq) {
            msg->ercd = E_OK;
            return;
        }

        ref_soc(sockfd, SOC_TCP_STATE, &sts);
        if (sts != TCP_CLOSED) {
            msg->ercd = E_PAR;
            set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
            return;
        }

        soc->backlog = msg->par.listen.backlog;
        if (soc->backlog < 1) {
            soc->backlog = 1;
        }
        /* listen is not queued */
        msg->ercd = E_OK; /* listen return value */

    /* call from uNet3 */
    } else {
        /*
            this entry is due to event of the socket in listen queue
        */

        /* SYN/ACK -> RST? */
        if (msg->ercd != E_OK) {
            con_soc(sockfd, &node, SOC_SER);
            return;
        }
        soc->con_sts = TCP_CON_CONNECT;

        /* my parent */
        soc = &gNET_BSD_SOC[soc->parentfd-1];
        soc->backlog--;
        /* get accept message que */
        msg = (T_UNET3_API_MSG*)soc->apique[API_TYPE_CON];
        if (msg) {
            ref_soc(sockfd, SOC_IP_REMOTE, &node);
            /* setup accept() API message */
#ifdef IPV6_SUP
            if (node.ver == AF_INET) {
#endif
                NODE_SIN(&node, (unet3_sockaddr_in*)&msg->par.accept.addr);
                msg->par.accept.addrlen = ADDRESS_LEN;
#ifdef IPV6_SUP
            } else {
                NODE_SIN6(&node, &msg->par.accept.addr);
                msg->par.accept.addrlen = SIN6_LEN;
            }
#endif
            msg->sockfd = sockfd;
            msg->ercd = E_OK;

            del_msg(soc->sockfd, API_TYPE_CON);/* remove accept message */
            del_que(&soc->listenq);         /* remove attached socket */
            gNET_BSD_SOC[sockfd-1].parentfd = 0; /* clear parent */

            soc->backlog++;
            /* wakeup task wating accept */
            wup_tsk(msg->aptskid);
        } else {
            /* generate readable event to listner socket*/
            unet3_callback(soc->sockfd, EV_RDY_RCV, E_OK);
        }
        /* parent socket fd */
        sockfd = soc->sockfd;
    }

    new_listener(sockfd);
}

static void wrap_accept(T_UNET3_API_MSG *msg)
{
    T_PARAM_ACCEPT  *par;
    T_UNET3_BSD_SOC *soc;
    int             sockfd;
    UB              sts;
    T_NODE          remote;

    soc = ACTIVE_SOC(msg->sockfd);
    if (soc == NULL) {
        msg->ercd = E_ID;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    par = &msg->par.accept;
    sockfd = msg->sockfd;

    /* socket is not listener */
    if (soc->listenq == NULL) {
        msg->ercd = E_OBJ;
        set_errno(msg->aptskid, UNET3_ER_EBADF, msg->sockfd);
        return;
    }

    /* cancel request */
    if (msg->msgtype == MSG_TYPE_CANCEL) {
        rmv_que(&soc->apique[API_TYPE_CON], (T_UNET3_QUE*)msg->cancel);
        msg->ercd = E_OK;
        return;
    }

    if (soc->apique[API_TYPE_CON]) {
        if (soc->ioasync) {
            /* not queue ACCEPT */
            msg->ercd = E_QOVR;
        } else {
            msg->ercd = E_WBLK;
        }
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    /* get attached socket */
    sockfd = ((T_UNET3_BSD_SOC *)soc->listenq)->sockfd;
    sts = ((T_UNET3_BSD_SOC *)soc->listenq)->con_sts;

    /* it is not connected yet */
    if (sts < TCP_CON_CONNECT) {
        if (soc->ioasync) {
            /* not queue ACCEPT */
            msg->ercd = E_QOVR;
        } else {
            msg->ercd = E_WBLK;
        }
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    /* TODO close wait socket */
    ref_soc(sockfd, SOC_IP_REMOTE, &remote);
#ifdef IPV6_SUP
    if (remote.ver == AF_INET) {
#endif
        NODE_SIN(&remote, (unet3_sockaddr_in*)&par->addr);
        par->addrlen = ADDRESS_LEN;
#ifdef IPV6_SUP
    } else {
        NODE_SIN6(&remote, &par->addr);
        par->addrlen = SIN6_LEN;
    }
#endif
    del_que(&soc->listenq);
    gNET_BSD_SOC[sockfd-1].parentfd = 0; /* clear parent */

    soc->backlog++;
    msg->sockfd = sockfd;
    msg->ercd = E_OK;

    /* attached socket is not anymore */
    if (soc->backlog == 1) {
        new_listener(soc->sockfd); /* soc is parent */
    }
}

/* common send */
static void wrap_sendto(T_UNET3_API_MSG *msg)
{
    T_PARAM_SENDTO     *par;
    T_UNET3_BSD_SOC    *soc;
    T_NODE             remote;
    T_NET_ADR          adr;
    int                devid;
#ifdef IPV6_SUP
    unet3_in6_addr     anyaddr6 = UNET3_IN6ADDR_ANY_INIT;
#endif
    T_NET *usr_net;

    soc = ACTIVE_SOC(msg->sockfd);
    if (soc == NULL) {
        msg->ercd = E_ID;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    par = &msg->par.sendto;

    /* call from application */
    if (msg->from == API_MSG_APPLICATION) {
        if (msg->msgtype == MSG_TYPE_CANCEL) {
            rmv_que(&soc->apique[API_TYPE_SND], (T_UNET3_QUE*)msg->cancel);
            abt_soc(msg->sockfd, SOC_ABT_SND);
            /*set_errno(msg->aptskid, UNET3_ER_EAGAIN, msg->sockfd);*/
            msg->ercd = E_OK;
            return;
        }
        if (soc->apique[API_TYPE_SND]) {
            msg->ercd = E_WBLK;
            set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
            return;
        }
    /* call from uNet3 */
    } else {
        /* error occurs during processing */
        if (msg->ercd <= 0) {
            set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
            return;
        }
        /* callback of UDP/TX indicates process is done */
        if (soc->proto == IP_PROTO_UDP) {
            return;
        }
        /* callback of TCP/TX indicates available tx buffer */
    }

    /* set destination for UDP */
    if (soc->proto == IP_PROTO_UDP) {
        net_memset(&remote, 0, sizeof(remote));
        /* send */
        if (msg->apiid == API_ID_SEND) {
            remote = soc->defaddr;
        /* sendto */
        } else {
#ifdef IPV6_SUP
            if (((unet3_sockaddr*)(&par->dest_addr))->sa_family == AF_INET) {
#endif
                SIN_NODE((unet3_sockaddr_in*)&par->dest_addr, &remote);
#ifdef IPV6_SUP
            } else {
                SIN6_NODE(&par->dest_addr, &remote);
            }
#endif
        }

#ifdef IPV6_SUP
        if (remote.ver == AF_INET) {
#endif
            if (remote.ipa == INADDR_ANY) {
                msg->ercd = E_PAR;
                set_errno(msg->aptskid, UNET3_ER_EDESTADDRREQ, msg->sockfd);
                return;
            }
            if (IS_MCAST_IP(remote.ipa)) {
                remote.num = (UB)soc->mcif;
            }
            if (soc->broadcast == FALSE) {
                if ( remote.ipa == INADDR_BROADCAST ) {
                    msg->ercd = E_PAR;
                    set_errno(msg->aptskid, UNET3_ER_EACCES, msg->sockfd);
                    return;
                }
                devid = get_devid_soc(msg->sockfd);
                if (devid == DEV_ANY) {
                    /* Case of DEV_ANY, the default device number is 1 */
                    devid = 1;
                }
                net_ref(devid, NET_IP4_CFG, &adr);
                if ( remote.ipa == (~adr.mask | adr.ipaddr) ) {
                    msg->ercd = E_PAR;
                    set_errno(msg->aptskid, UNET3_ER_EACCES, msg->sockfd);
                    return;
                }
            }
#ifdef IPV6_SUP
        } else {
            if (ip6_addr_cmp(remote.ip6a, anyaddr6.unet3_s6_addr32) == TRUE) {
                msg->ercd = E_PAR;
                set_errno(msg->aptskid, UNET3_ER_EDESTADDRREQ, msg->sockfd);
                return;
            }
        }
#endif

        /* early transmit option */
        cfg_soc(msg->sockfd, SOC_UDP_EARLY_TX, (VP)((ADDR)(par->flags & UNET3_MSG_DONTWAIT)));

        /* for uNet3 ver4 series specification */
        usr_net = pNET_SOC[msg->sockfd-1].usr_net;
        if (usr_net != NULL) {
            remote.num = usr_net->dev->num;
        }

        msg->ercd = con_soc(msg->sockfd, &remote, SOC_CLI);
        if (msg->ercd != E_OK) {
            set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
            return;
        }

    /* check connection for TCP */
    } else if (soc->proto == IP_PROTO_TCP) {
        if (soc->con_sts < TCP_CON_CONNECT) {
            msg->ercd = E_OBJ;
            set_errno(msg->aptskid, UNET3_ER_EBADF, msg->sockfd);
            return;
        }

    } else {
        /* raw socket */
        /* nothing to do check */
    }

    msg->ercd = snd_soc(msg->sockfd, par->buf, par->len);

    set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);

    if (soc->ioasync && msg->ercd == E_WBLK) {
        if (soc->proto == IP_PROTO_TCP) {
            abt_soc(msg->sockfd, SOC_ABT_SND);
            msg->ercd = -1;         /* non-block without Add Que */
        } else {
            msg->ercd = par->len;   /* datagram transmission is complete soon */
        }
    }
}

/* common recv */
static void wrap_recvfrom(T_UNET3_API_MSG *msg)
{
    T_PARAM_RECVFROM   *par;
    T_UNET3_BSD_SOC    *soc;
    T_NODE             remote;
    UB sts;

    soc = ACTIVE_SOC(msg->sockfd);
    if (soc == NULL) {
        set_errno(msg->aptskid, UNET3_ER_EBADF, msg->sockfd);
        msg->ercd = E_ID;
        return;
    }

    par = &msg->par.recvfrom;

    /* call from application */
    if (msg->from == API_MSG_APPLICATION) {
        if (msg->msgtype == MSG_TYPE_CANCEL) {
            rmv_que(&soc->apique[API_TYPE_RCV], (T_UNET3_QUE*)msg->cancel);
            abt_soc(msg->sockfd, SOC_ABT_RCV);
            /*set_errno(msg->aptskid, UNET3_ER_EAGAIN, msg->sockfd);*/
            msg->ercd = E_OK;
            return;
        }
        if (soc->apique[API_TYPE_RCV]) {
            if (soc->ioasync) {
                /* not queue recv */
                msg->ercd = E_QOVR;
            } else {
                msg->ercd = E_WBLK;
            }
            set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
            return;
        }
        /* check connection for TCP */
        if (soc->proto == IP_PROTO_TCP && soc->con_sts < TCP_CON_CONNECT) {
            msg->ercd = E_OBJ;
            set_errno(msg->aptskid, UNET3_ER_EBADF, msg->sockfd);
            return;
        }
    /* call from uNet3 */
    } else {
        /* error occurs during processing */
        if (msg->ercd <= 0) {
            if (soc->proto == IP_PROTO_TCP) {
                /* receive FIN case */
                if (msg->ercd == 0) {
                    soc->con_sts = TCP_CON_DISCON_RX;
                /* broken session case */
                } else {
                    /* reset TCP case (by peer or uNet3) */
                    /* close socket case */
                    ref_soc(msg->sockfd, SOC_TCP_STATE, &sts);
                    if (soc->delflg == 1 || sts == TCP_CLOSED) {
                        soc->con_sts = TCP_CON_NOTCON;
                    /* shutdown RX stream case */
                    } else {
                        soc->con_sts = TCP_CON_DISCON_RX;
                        msg->ercd = 0;
                    }
                }
            }
            set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
            return;
        }
    }

    /* BSD spec */
    /* always return 0 after RX stream reach to EOF(FIN) */
    if (soc->con_sts == TCP_CON_DISCON_RX) {
        msg->ercd = 0;
        return;
    }

    if ( (par->flags & UNET3_MSG_PEEK) != 0 ) {
        /* Set MSG_PEEK option */
        cfg_soc( msg->sockfd, SOC_MSG_PEEK, (VP)1 );
    } else {
        /* Set MSG_PEEK option */
        cfg_soc( msg->sockfd, SOC_MSG_PEEK, (VP)0 );
    }

    msg->ercd = rcv_soc(msg->sockfd, par->buf, par->len);

    /* set destination for UDP */
    if (msg->apiid == API_ID_RECVFROM) {
        if (soc->proto == IP_PROTO_UDP && msg->ercd > 0) {
            net_memset(&remote, 0, sizeof(remote));
            ref_soc(msg->sockfd, SOC_IP_REMOTE, &remote);
#ifdef IPV6_SUP
            if (remote.ver == AF_INET) {
#endif
                NODE_SIN(&remote, (unet3_sockaddr_in*)&par->src_addr);
                par->addrlen = ADDRESS_LEN;
#ifdef IPV6_SUP
            } else {
                NODE_SIN6(&remote, &par->src_addr);
                par->addrlen = SIN6_LEN;
            }
#endif
        }
    }

    if (msg->ercd == E_CLS || msg->ercd == 0) {
        soc->con_sts = TCP_CON_DISCON_RX;
        msg->ercd = 0;
    }

    set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);

    if (soc->ioasync && msg->ercd == E_WBLK) {
        abt_soc(msg->sockfd, SOC_ABT_RCV);
        msg->ercd = -1; /* non-block without Add Que */
    }
}

static void wrap_shutdown(T_UNET3_API_MSG *msg)
{
    T_UNET3_BSD_SOC *soc;
    T_PARAM_SHUTDOWN   *par;

    soc = ACTIVE_SOC(msg->sockfd);
    if (soc == NULL) {
        msg->ercd = E_ID;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }
    par = &msg->par.shutdown;

    /* call from application */
    if (msg->from == API_MSG_APPLICATION) {
        if (par->how != UNET3_SHUT_WR && par->how != UNET3_SHUT_RDWR) {
            msg->ercd = E_PAR;
            set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
            return;
        }

        if (soc->listenq == NULL) {
            msg->ercd = cls_soc(msg->sockfd, par->how);
        } else {
           while (soc->listenq) {
                /* abort session */
                abt_soc(((T_UNET3_BSD_SOC*)soc->listenq)->sockfd, SOC_ABT_ALL);
                delete_socket(((T_UNET3_BSD_SOC*)soc->listenq)->sockfd);
                del_que(&soc->listenq);
            }
            msg->ercd = E_OK;
        }
        /* clear number of backlog */
        soc->backlog = 0;

        /* shutdown does not block process */
        if (msg->ercd == E_WBLK) {
            msg->ercd = E_OK;
        }
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
    /* call from uNet3 */
    } else {
        msg->ercd = E_OK;
    }
}

static void wrap_close(T_UNET3_API_MSG *msg)
{
    T_UNET3_BSD_SOC *soc;
    UB sts;

    soc = ACTIVE_SOC(msg->sockfd);
    if (soc == NULL) {
        msg->ercd = E_ID;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }
    msg->ercd = E_OK;

    /* call from application */
    if (msg->from == API_MSG_APPLICATION) {
        /* now closing */
        if (soc->apique[API_TYPE_CLS]) {
            msg->ercd = E_ID;
            set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
            return;
        }

        sts = 0;
        if (soc->proto == IP_PROTO_TCP) {
            ref_soc(msg->sockfd, SOC_TCP_STATE, &sts);
        }
        switch(sts) {
            case TCP_LISTEN:
            case TCP_SYN_SENT:
            case TCP_SYN_RECEIVED:
                msg->ercd = abt_soc(msg->sockfd, SOC_ABT_CON);
                break;
            case TCP_ESTABLISHED:
            case TCP_CLOSE_WAIT:
                msg->ercd = cls_soc(msg->sockfd, SOC_TCP_CLS);
                break;
            default:
                msg->ercd = abt_soc(msg->sockfd, SOC_ABT_ALL);
                cls_soc(msg->sockfd, SOC_TCP_CLS);
                break;
        }
        /* close does not block process */
        if (msg->ercd == E_WBLK) {
            msg->ercd = E_OK;
            soc->delflg = 1;
        } else {
            /* skip TCP cls_soc() case */
            delete_socket(msg->sockfd);
        }
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);

    /* call from uNet3 */
    } else {
        delete_socket(msg->sockfd);
        msg->ercd = E_OK;
    }
}

static void wrap_select(T_UNET3_API_MSG *msg)
{
    int sockfd, maxfd;
    int rdycnt;
    unet3_fd_set *rfds_i, *wfds_i, rfds_o, wfds_o;
    BOOL r, w;

    unet3_FD_ZERO(&rfds_o);
    unet3_FD_ZERO(&wfds_o);

    rfds_i = (unet3_fd_set*)&msg->par.select.readfds;
    wfds_i = (unet3_fd_set*)&msg->par.select.writefds;
    maxfd  = msg->par.select.nfds;
    r      = msg->par.select.r;
    w      = msg->par.select.w;

    /* call from application */
    if (msg->from == API_MSG_APPLICATION) {
        /* cancel request */
        if (msg->msgtype == MSG_TYPE_CANCEL) {
            rmv_que(&gselect_que, (T_UNET3_QUE*)msg->cancel);
            /*set_errno(msg->aptskid, EAGAIN, msg->sockfd);*/   /* not error ?*/
            msg->ercd = E_OK;
            return;
        }

        /* scan target socket  */
        for (sockfd = 1, rdycnt = 0; sockfd < maxfd; sockfd++) {
            if (r && unet3_FD_ISSET(sockfd, rfds_i)) {
                if (!VALID_SOC(sockfd)) {
                    set_errno(msg->aptskid, UNET3_ER_EBADF, msg->sockfd);
                    msg->ercd = E_ID;
                    return;
                }
                if (can_read(sockfd)) {
                    unet3_FD_SET(sockfd, &rfds_o);
                    rdycnt++;
                }
            }
            if (w && unet3_FD_ISSET(sockfd, wfds_i)) {
                if (!VALID_SOC(sockfd)) {
                    set_errno(msg->aptskid, UNET3_ER_EBADF, msg->sockfd);
                    msg->ercd = E_ID;
                    return;
                }
                if (can_write(sockfd)) {
                    unet3_FD_SET(sockfd, &wfds_o);
                    rdycnt++;
                }
            }
        }
    /* call from uNet3 */
    } else {
        /* apitype is determined by occured event */
        if (msg->apitype == API_TYPE_SRDY) {
            unet3_FD_SET(msg->sockfd, &wfds_o);
        } else {
            unet3_FD_SET(msg->sockfd, &rfds_o);
        }
        rdycnt = 1;
    }

    if (rdycnt) {
        msg->ercd = rdycnt;
        *rfds_i = rfds_o;
        *wfds_i = wfds_o;
        unet3_FD_ZERO((unet3_fd_set*)&msg->par.select.exceptfds);
    } else {
        /* rather than API queue put SELECT queue*/
        /* and wakeup task immediately */
        msg->ercd = E_OK;
        add_que(&gselect_que, (T_UNET3_QUE*)msg);
    }
}

#if (SYS_PLATFORM)
/* for single fd select */
static void wrap_select_u(T_UNET3_API_MSG *msg)
{
    T_UNET3_API_MSG  *qmsg;
    T_UNET3_BSD_SOC *soc;
    SELECT_CBK cbk;
    ID aptskid;
    ER ercd;
    B  apitype;

    soc = ACTIVE_SOC(msg->sockfd);
    if (soc == NULL) {
        msg->ercd = E_ID;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    /* call from application */
    if (msg->from == API_MSG_APPLICATION) {

        msg->ercd = E_OK;

        /* cancel request */
        if (msg->msgtype == MSG_TYPE_CANCEL) {
            aptskid = msg->aptskid;
            /* remove from soc->selectque with taskid */
            qmsg = (T_UNET3_API_MSG*)soc->selectque;
            while (qmsg) {
                if (aptskid == qmsg->par.select_u.tskid) {
                    rmv_que(&soc->selectque, (T_UNET3_QUE*)qmsg);
                    msg_buf_ret(qmsg);
                    break;
                }
                qmsg = qmsg->next;
            }
        }
        if (msg->par.select_u.r) {
            if (can_read(msg->sockfd)) {
                msg->ercd |= 0x01;
            }
        }
        if (msg->par.select_u.w) {
            if (can_write(msg->sockfd)) {
                msg->ercd |= 0x02;
            }
        }
        if (msg->par.select_u.e) {
            /* nothing to do */
        }
        if (msg->msgtype != MSG_TYPE_CANCEL) {
            /* select_u always keep msg in selectq */
            add_que(&soc->selectque, (T_UNET3_QUE*)msg);
        }
    /* call from uNet3 */
    } else {

        ercd = msg->ercd;
        apitype = msg->apitype;

        /* remove from soc->selectque with waiting evnet and callback */
        if (apitype == API_TYPE_RRDY) {
            while (msg) {
                if (msg->par.select_u.r) {
                    cbk = (SELECT_CBK)((UW)msg->par.select_u.cbk);
                    qmsg = msg;
                    msg = msg->next;
                    cbk(qmsg->sockfd, 1, 0, 0, ercd);
                    continue;
                }
                msg = msg->next;
            }
        } else {
            while (msg) {
                if (msg->par.select_u.w) {
                    cbk = (SELECT_CBK)((UW)msg->par.select_u.cbk);
                    qmsg = msg;
                    msg = msg->next;
                    cbk(qmsg->sockfd, 0, 1, 0, ercd);
                    continue;
                }
                msg = msg->next;
            }
        }
    }
}
#endif

static void wrap_getsockname(T_UNET3_API_MSG *msg)
{
    T_PARAM_GETSOCKNAME  *par;
    T_NODE local = {0};

    if (!VALID_SOC(msg->sockfd)) {
        msg->ercd = E_ID;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }

    par = &msg->par.getsockname;
    ref_soc(msg->sockfd, SOC_IP_LOCAL, &local);
#ifdef IPV6_SUP
    if (local.ver == AF_INET) {
#endif
        NODE_SIN(&local, (unet3_sockaddr_in*)&par->addr);
        par->addrlen = ADDRESS_LEN;
#ifdef IPV6_SUP
    } else {
        NODE_SIN6(&local, &par->addr);
        par->addrlen = SIN6_LEN;
    }
#endif
    msg->ercd = E_OK;
}

static void wrap_getpeername(T_UNET3_API_MSG *msg)
{
    T_PARAM_GETPEERNAME  *par;
    T_UNET3_BSD_SOC      *soc;
    T_NODE remote = {0};
#ifdef IPV6_SUP
    unet3_in6_addr anyaddr6 = UNET3_IN6ADDR_ANY_INIT;
#endif

    soc = ACTIVE_SOC(msg->sockfd);
    if (soc == NULL) {
        msg->ercd = E_ID;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }
    par = &msg->par.getpeername;

    if (soc->proto == IP_PROTO_TCP && soc->con_sts >= TCP_CON_CONNECT) {
        ref_soc(msg->sockfd, SOC_IP_REMOTE, &remote);
    } else {
        remote = soc->defaddr;
    }

#ifdef IPV6_SUP
    if (remote.ver == AF_INET) {
#endif
        if (remote.ipa) {
            NODE_SIN(&remote, (unet3_sockaddr_in*)&par->addr);
            par->addrlen = ADDRESS_LEN;
            msg->ercd = E_OK;
        } else {
            msg->ercd = E_OBJ;
            set_errno(msg->aptskid, UNET3_ER_ENOTCONN, msg->sockfd);
        }
#ifdef IPV6_SUP
    } else {
        if (ip6_addr_cmp(remote.ip6a, anyaddr6.unet3_s6_addr32) == FALSE) {
            NODE_SIN6(&remote, &par->addr);
            par->addrlen = SIN6_LEN;
            msg->ercd = E_OK;
        } else {
            msg->ercd = E_OBJ;
            set_errno(msg->aptskid, UNET3_ER_ENOTCONN, msg->sockfd);
        }
    }
#endif
}

static void wrap_ioctl(T_UNET3_API_MSG *msg)
{
    T_PARAM_IOCTL      *par;

    if (!VALID_SOC(msg->sockfd)) {
        msg->ercd = E_ID;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
        return;
    }
    par = &msg->par.ioctl;

    if (par->request == UNET3_FIONBIO) {
        /* TODO check socket has API message */
        gNET_BSD_SOC[msg->sockfd-1].ioasync = (*par->val != 0);
        msg->ercd = E_OK;
    } else if (par->request == UNET3_FIONREAD) {
        msg->ercd = ref_soc( msg->sockfd, SOC_RCV_PKT_SIZE, (VP)par->val );
        if ( msg->ercd != E_OK ) {
            set_errno(msg->aptskid, UNET3_ER_EBADF, msg->sockfd);
            msg->ercd = -1;
        }
    } else {
        msg->ercd = E_PAR;
        set_errno(msg->aptskid, cnvrt_err(msg->ercd), msg->sockfd);
    }
}

void (*wrap_api_tbl[])(T_UNET3_API_MSG*) = {
    wrap_socket,
    wrap_bind,
    wrap_listen,
    wrap_accept,
    wrap_connect,
    wrap_sendto,        /* send */
    wrap_sendto,
    wrap_recvfrom,      /* recv */
    wrap_recvfrom,
    wrap_shutdown,
    wrap_close,
    wrap_select,
#if (SYS_PLATFORM)
    wrap_select_u,
#endif
    wrap_getsockname,
    wrap_getpeername,
    wrap_socketoption,   /* getsockopt */
    wrap_socketoption,   /* setsockopt */
    wrap_ioctl,

};

void bsd_unet3_tsk(VP_INT exinf)
{
    ER ercd;
    B apitype;
    T_UNET3_API_MSG *msg;
    int sockfd, i;
    ID aptskid;
    API_ID apiid;

    /* initiate wrapper */
    gselect_que = NULL;
    unet_errno = 0;

    for (i = 0; i < NET_SOC_MAX; i++) {
        net_memset(&gNET_BSD_SOC[i], 0, sizeof(T_UNET3_BSD_SOC));
    }

    while (1) {

        ercd = E_OBJ;
        msg = NULL;
        apitype = 0;

        rcv_mbx(MBX_BSD_REQ, (T_MSG**)&msg);

        /* from application */
        if (msg->from == API_MSG_APPLICATION) {

            /* execute API wrapper */
            wrap_api_tbl[msg->apiid](msg);
#if (SYS_PLATFORM)
            if (msg->apiid == API_ID_SELECT_U) {
                /* select_u always keep msg in selectq */
                wup_tsk(msg->aptskid);
                continue;
            }
#endif
            if (msg->ercd == E_WBLK) {
                add_apique(msg->sockfd, msg);
            } else {
                aptskid = msg->aptskid;
                /* select API does not clear API task ID */
                if (msg->apiid != API_ID_SELECT) {
                    msg->aptskid = 0;
                }
                if (msg->msgtype != MSG_TYPE_CANCEL && msg->cancel != NULL) {
                /* This message should be woken up via cancel message */
                    continue;
                }
                wup_tsk(aptskid);
            }
            continue;
        }

        /* from uNet3*/
        if (msg->from == API_MSG_CALLBCK) {

        /* How to identify BSD API from uNet3 callback
            1) con_soc(), cls_soc(), snd_soc(), rcv_soc()
               search api que with key of socket and event type.
            2) I/O ready event
               search FD_SET in select que with key of socket.
        */
            sockfd  = msg->sockfd;
            ercd    = msg->ercd;
            apitype = msg->apitype;
            msg_buf_ret(msg);

            /* the event that caused by application actions */
            if (ercd == E_RLWAI) {
                continue;
            }

            i = 0;
            while (1) {
                /* get waiting api */
                msg = get_api_msg(sockfd, apitype);
                if (msg == NULL) {
                    /* no waiting api in the first loop */
                    if (i == 0) {
                        event_socket(sockfd, apitype, ercd);
                    }
                    break;
                }
                i++;
                /* override uNet3 result */
                msg->from = API_MSG_CALLBCK;
                msg->ercd = ercd;

                /* for API_TYPE_NOT_IO */
                /*msg->sockfd = sockfd;*/
                msg->apitype = apitype;

                /* execute API wrapper */
                apiid = msg->apiid;
                wrap_api_tbl[apiid](msg);
#if (SYS_PLATFORM)
                if (apiid == API_ID_SELECT_U) {
                    break;
                }
#endif
                if (msg->ercd == E_WBLK) {
                    /* wait callback */
                    /* keep msg */
                    break;
                }

                /* deque */
                rmv_api_msg(msg);
                if (msg->aptskid) {
                    loc_tcp();
                    if (!msg->cancel) {
                        wup_tsk(msg->aptskid);
                    }
                    ulc_tcp();
                } else {
                    msg_buf_ret(msg);
                }
            }
        }
    }
}
