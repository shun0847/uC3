/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK
    BSD socket wrapper/BSD socket API
    Copyright (c) 2014-2021, eForce Co., Ltd. All rights reserved.

    Version Information
      2014.04.28: Created
      2014.07.24: Fixed the problem of cancel message
      2014.07.24: Add getifaddrs/freeifaddrs
      2014.11.11: Corrected cancel_api()
      2015.03.20: Modified to support uNet3/Compact.
      2015.07.29: Disable original task-id in cancel_api()
      2015.07.30: Disable release buffer at BSD task side in cancel_api()
      2015.08.12: Calculate in pre-process the task and mpf size depends on
                  max number of socket
      2015.08.12: Corrected if_indextoname()
      2015.08.17: Add EINTR as errno at the forced termination
      2015.09.17: Changed to wait forever if select timeout is NULL
      2015.12.02: The section definition added for task and mpf
      2015.12.14: The socket ID replaced SID types
      2016.02.08: Update for raw socket
      2016.03.18: Update for H/W OS
      2016.05.12: Modify the broadcast options
      2016.06.08: Modify the parameter settings of ioctl.
      2016.08.02: Fixed exclusive control for timeout process
      2016.11.02: Update select_u for POSIX adapter 
      2016.11.16: Fixed memory leak at accept timeout
      2016.12.27: Fixed the wrong length parameter of net_buf_get()
      2017.07.27: Only cancel message guarantees the allocation of buffer
      2018.06.06: Corrected API_MSG_BUF_SIZE value
      2019.03.20: Add USE_APLBUF flags option
      2020.03.23: Support 64bit processor
      2020.04.28: Fixed procedure of buffer allocation in send() and sendto()
                  to avoid leak.
      2020.09.07: Supported IPv6.
      2021.12.13: Updated the value of definition API_MSG_BUF_NUM.
 ***************************************************************************/

#include "kernel.h"
#include "net_hdr.h"
#include "net_def.h"
#include "net_strlib.h"

#include "unet3_socket.h"
#include "bsd_param.h"
#include "unet3_wrap.h"
#include <stdarg.h>
#include "unet3_cfg.h"


/* kernel resources */
#if (defined(NET_HW_OS) || defined(NET_C_OS))
#include "kernel_id.h"
#else

/* Optimum value for kernel resource */
#define     FD_SET_SIZE            ((FD_SET_MAX)*4)

/* Message buffer size   */
#define     API_PARAM_CTL_SIZE     32
#define     API_PARAM_OPT_SIZE     (SOC_OPT_MAXLEN + 12)
#define     API_PARAM_SEL_SIZE     (FD_SET_SIZE*3 + 4)
#define     API_PARAM_VAR_SIZE     (API_PARAM_OPT_SIZE > API_PARAM_SEL_SIZE ? API_PARAM_OPT_SIZE : API_PARAM_SEL_SIZE)
#define     API_MSG_BUF_SIZE       (((sizeof(T_UNET3_API_MSG)+31)/32)*32)
#if defined(CFG_BSD_TCP_MAX) && defined(CFG_BSD_UDP_MAX)
#define     API_MSG_BUF_NUM        ((CFG_BSD_TCP_MAX + CFG_BSD_UDP_MAX)*2)
#else
#define     API_MSG_BUF_NUM        (CFG_NET_SOC_MAX*2)
#endif


/* BSD task size         */
#if (_kernel_SIZE_SIZE==8)
#define     BSD_TASK_SIZE          ((1024 + FD_SET_SIZE*2)*4)
#else
#define     BSD_TASK_SIZE          (1024 + FD_SET_SIZE*2)
#endif

/* BSD Local Stack allocate and Fixed-Sized Memory Pool */
#ifdef __CC_ARM
long long _netbsd_tsk_stk[BSD_TASK_SIZE/sizeof(long long)] __attribute__ ((section ("LOCALSTKMEM"), zero_init));
long long _netbsd_mpf_buf[(API_MSG_BUF_NUM*API_MSG_BUF_SIZE)/sizeof(long long)] __attribute__ ((section ("LOCALMPLMEM"), zero_init));
#elif __IAR_SYSTEMS_ICC__
#pragma section = "LOCALSTKMEM"
long long _netbsd_tsk_stk[BSD_TASK_SIZE/sizeof(long long)] @ "LOCALSTKMEM";
#pragma section = "LOCALMPLMEM"
long long _netbsd_mpf_buf[(API_MSG_BUF_NUM*API_MSG_BUF_SIZE)/sizeof(long long)] @ "LOCALMPLMEM";
#endif

ID TSK_BSD_API;
ID MBX_BSD_REQ;
ID MPF_BSD_MSG;
const T_CMBX c_bsd_mbx = {TA_TFIFO, 0, NULL};
#if (defined(__CC_ARM) || defined(__IAR_SYSTEMS_ICC__))
const T_CTSK c_bsd_tsk = {TA_HLNG, NULL, (FP)bsd_unet3_tsk,  BSD_TASK_PRIORITY, BSD_TASK_SIZE, (VP)_netbsd_tsk_stk, NULL};
const T_CMPF c_bsd_mpf = {TA_TFIFO, API_MSG_BUF_NUM, API_MSG_BUF_SIZE, (VP)_netbsd_mpf_buf, NULL};
#else
const T_CTSK c_bsd_tsk = {TA_HLNG | TA_FPU, NULL, (FP)bsd_unet3_tsk,  BSD_TASK_PRIORITY, BSD_TASK_SIZE, NULL, NULL};
const T_CMPF c_bsd_mpf = {TA_TFIFO, API_MSG_BUF_NUM, API_MSG_BUF_SIZE, NULL, NULL};
#endif
#endif

static void cancel_api(T_UNET3_API_MSG *msg)
{
    T_UNET3_API_MSG *cmsg;
    ID tskid;
    ID memid;

    loc_tcp();
    msg->cancel   = (T_UNET3_API_MSG *)1;  /* this API messeage is canceled */
    ulc_tcp();

    if (net_memget((VP*)&cmsg, sizeof(T_UNET3_API_MSG), TMO_FEVR, &memid) != E_OK) {
        msg->msgtype = MSG_TYPE_CANCEL;
        return;
    }
    get_tid(&tskid);
    cmsg->sockfd   = msg->sockfd;
    cmsg->apitype  = msg->apitype;
    cmsg->apiid    = msg->apiid;
    cmsg->msgtype  = MSG_TYPE_CANCEL;
    cmsg->from     = API_MSG_APPLICATION;
    cmsg->aptskid  = tskid;
    cmsg->cancel   = msg;
    net_memcpy(&cmsg->par, &msg->par, sizeof(cmsg->par));

    can_wup(TSK_SELF);
    snd_mbx(MBX_BSD_REQ, (T_MSG*)cmsg);
    slp_tsk();

    net_memret(cmsg, memid);
}

ER unet3_bsd_init(void)
{
    ER ercd;
    UW i;

 /* uNet3/Standard */
#if !(defined(NET_HW_OS) || defined(NET_C_OS))

    TSK_BSD_API = MBX_BSD_REQ = MPF_BSD_MSG = 0;

    ercd = acre_tsk((T_CTSK *)&c_bsd_tsk);
    if (ercd <= 0) {
        goto SYS_ERROR;
    }
    TSK_BSD_API = ercd;

    ercd = acre_mbx((T_CMBX *)&c_bsd_mbx);
    if (ercd <= 0) {
        goto SYS_ERROR;
    }
    MBX_BSD_REQ = ercd;

    ercd = acre_mpf((T_CMPF *)&c_bsd_mpf);
    if (ercd <= 0) {
        goto SYS_ERROR;
    }
    MPF_BSD_MSG = ercd;

    ercd = sta_tsk(TSK_BSD_API, 0);
    if (ercd != E_OK) {
SYS_ERROR:
        if (TSK_BSD_API) del_tsk(TSK_BSD_API);
        if (MBX_BSD_REQ) del_mbx(MBX_BSD_REQ);
        if (MPF_BSD_MSG) del_mpf(MPF_BSD_MSG);
        ercd = E_SYS;
    }
#else
#ifndef NET_HW_OS
    bsd_wrapper_ini(
        CFG_BSD_TCP_TOP, CFG_BSD_TCP_MAX, 
        CFG_BSD_UDP_TOP, CFG_BSD_UDP_MAX
    );
#endif
    ercd = sta_tsk(TSK_BSD_API, 0);
#endif

    if (ercd != E_OK) {
        return ercd;
    }

    ercd = lo_net_ini(0);

    net_memset(&tsk_errno[0], 0, sizeof(tsk_errno[0])*NUM_OF_TASK_ERRNO);

    /* Default recv for broadcast packet */
    for (i = 0; i < NET_DEV_MAX; i++) {
        net_cfg(i + 1, NET_BCAST_RCV, (VP)1);
    }
    
    return ercd;
}

/*
    int socket(int domain, int type, int protocol);
*/
int unet3_socket(int domain, int type, int protocol)
{
    ER ercd;
    T_UNET3_API_MSG *msg;
    T_PARAM_SOCKET  *par;
    int sockfd;
    ID tskid;

    get_tid(&tskid);
    if ((domain == UNET3_AF_INET || domain == UNET3_AF_INET6) == 0) {
        set_errno(tskid, UNET3_ER_EINVAL, 0);
        return -1;
    }
    if ((type == UNET3_SOCK_STREAM || type == UNET3_SOCK_DGRAM || type == UNET3_SOCK_RAW) == 0) {
        set_errno(tskid, UNET3_ER_EINVAL, 0);
        return -1;
    }
    if (type == UNET3_SOCK_RAW) {
        if (protocol == ETH_TYPE_IP4 || protocol == ETH_TYPE_ARP || protocol == ETH_TYPE_IP6) {
            set_errno(tskid, UNET3_ER_EINVAL, 0);
            return -1;
        }
    }

    ercd = msg_buf_get(&msg);
    if (ercd != E_OK) {
        set_errno(tskid, UNET3_ER_ENOMEM, 0);
        return -1;
    }
    msg->sockfd   = 0;
    msg->apitype  = API_TYPE_NOT_IO;
    msg->apiid    = API_ID_SOCKET;
    msg->from     = API_MSG_APPLICATION;
    msg->msgtype  = MSG_TYPE_REQUEST;
    msg->aptskid  = tskid;

    par = &msg->par.socket;
    par->domain   = domain;
    par->type     = type;
    par->protocol = protocol;

    can_wup(TSK_SELF);
    snd_mbx(MBX_BSD_REQ, (T_MSG*)msg);
    ercd = slp_tsk();
    if (ercd == E_RLWAI) {
        msg_buf_ret(msg);
        set_errno(tskid, UNET3_ER_EINTR, 0);
        return -1;
    }

    /* socket create error */
    if (msg->ercd != E_OK) {
        msg->sockfd = -1;
    }
    sockfd = msg->sockfd;
    msg_buf_ret(msg);
    return sockfd;
}


/*
    int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
*/
int unet3_bind(int sockfd, const struct unet3_sockaddr *addr, unsigned int addrlen)
{
    ER ercd;
    T_UNET3_API_MSG *msg;
    T_PARAM_BIND    *par;
    ID tskid;

    get_tid(&tskid);
    if (addr == NULL || addrlen < ADDRESS_LEN) {
        set_errno(tskid, UNET3_ER_EINVAL, sockfd);
        return -1;
    }
    if (addrlen > SIN6_LEN) {
        addrlen = SIN6_LEN;
    }

    ercd = msg_buf_get(&msg);
    if (ercd != E_OK) {
        set_errno(tskid, UNET3_ER_ENOMEM, sockfd);
        return -1;
    }
    msg->sockfd   = sockfd;
    msg->apitype  = API_TYPE_NOT_IO;
    msg->apiid    = API_ID_BIND;
    msg->msgtype  = MSG_TYPE_REQUEST;
    msg->from     = API_MSG_APPLICATION;
    msg->aptskid  = tskid;

    par = &msg->par.bind;
    net_memcpy(&par->addr, (void*)addr, addrlen);
    par->addrlen  = addrlen;

    can_wup(TSK_SELF);
    snd_mbx(MBX_BSD_REQ, (T_MSG*)msg);
    ercd = slp_tsk();
    if (ercd == E_RLWAI) {
        msg_buf_ret(msg);
        set_errno(tskid, UNET3_ER_EINTR, sockfd);
        return -1;
    }

    ercd = msg->ercd;
    msg_buf_ret(msg);
    return (ercd < 0 ? -1 : 0);
}


/*
    int listen(int sockfd, int backlog);
*/
int unet3_listen(int sockfd, int backlog)
{
    ER ercd;
    T_UNET3_API_MSG *msg;
    T_PARAM_LISTEN  *par;
    ID tskid;

    get_tid(&tskid);
    if (backlog > NET_TCP_MAX - 1) {
        backlog = NET_TCP_MAX - 1;
    }
    ercd = msg_buf_get(&msg);
    if (ercd != E_OK) {
        set_errno(tskid, UNET3_ER_ENOMEM, sockfd);
        return -1;
    }
    msg->sockfd   = sockfd;
    msg->apitype  = API_TYPE_CON;
    msg->apiid    = API_ID_LISTEN;
    msg->msgtype  = MSG_TYPE_REQUEST;
    msg->from     = API_MSG_APPLICATION;
    msg->aptskid  = tskid;

    par = &msg->par.listen;
    par->backlog  = backlog;

    can_wup(TSK_SELF);
    snd_mbx(MBX_BSD_REQ, (T_MSG*)msg);
    ercd = slp_tsk();
    if (ercd == E_RLWAI) {
        msg_buf_ret(msg);
        set_errno(tskid, UNET3_ER_EINTR, sockfd);
        return -1;
    }

    ercd = msg->ercd;
    msg_buf_ret(msg);
    return (ercd < 0 ? -1 : 0);
}


/*
    int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
*/
int unet3_accept(int sockfd, struct unet3_sockaddr *addr, unsigned int *addrlen)
{
    ER ercd;
    T_UNET3_API_MSG *msg;
    T_PARAM_ACCEPT    *par;
    struct unet3_timeval  timeout;
    unsigned int timlen = sizeof(timeout);
    ID tskid;

    get_tid(&tskid);
    if (addr) {
        if (addrlen == NULL) {
            set_errno(tskid, UNET3_ER_EINVAL, sockfd);
            return -1;
        }
        if (*addrlen > SIN6_LEN) {
            *addrlen = SIN6_LEN;
        }
    }

    ercd = msg_buf_get(&msg);
    if (ercd != E_OK) {
        set_errno(tskid, UNET3_ER_ENOMEM, sockfd);
        return -1;
    }

    ercd = unet3_getsockopt(sockfd, UNET3_SOL_SOCKET, UNET3_SO_RCVTIMEO, &timeout, &timlen);
    if (ercd != E_OK) {
        msg_buf_ret(msg);
        return -1;
    }

    msg->sockfd   = sockfd;
    msg->apitype  = API_TYPE_CON;
    msg->apiid    = API_ID_ACCEPT;
    msg->msgtype  = MSG_TYPE_REQUEST;
    msg->from     = API_MSG_APPLICATION;
    msg->aptskid  = tskid;

    can_wup(TSK_SELF);
    snd_mbx(MBX_BSD_REQ, (T_MSG*)msg);
    ercd = tslp_tsk(TIME_VALUE(&timeout));
    if (ercd == E_RLWAI) {
        msg_buf_ret(msg);
        set_errno(tskid, UNET3_ER_EINTR, sockfd);
        return -1;
    }

    if (ercd == E_TMOUT) {

        cancel_api(msg);
        set_errno(tskid, UNET3_ER_ETIMEDOUT, sockfd);
        sockfd = -1;
        goto accept_end;

    }

    /* accept error */
    if (msg->ercd != E_OK) {
        sockfd = -1;
        goto accept_end;
    }

    /* set out parameter */
    if (addr) {
        par = &msg->par.accept;
        if (((unet3_sockaddr*)&par->addr)->sa_family == AF_INET) {
            net_memcpy(addr, &par->addr, par->addrlen);
        } else {
            net_memcpy(addr, &par->addr, *addrlen);
        }
        *addrlen = par->addrlen;
    }
    sockfd = msg->sockfd;

accept_end:
    if (ercd == E_TMOUT || msg->ercd != E_WBLK) {
        msg_buf_ret(msg);
    }
    return sockfd;
}


/*
    int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
*/
int unet3_connect(int sockfd, const struct unet3_sockaddr *addr, unsigned int addrlen)
{
    ER ercd;
    T_UNET3_API_MSG *msg;
    T_PARAM_CONNECT    *par;
    struct unet3_timeval  timeout;
    unsigned int timlen = sizeof(timeout);
    ID tskid;

    get_tid(&tskid);
    if (addr == NULL || addrlen < ADDRESS_LEN) {
        set_errno(tskid, UNET3_ER_EINVAL, sockfd);
        return -1;
    }
    if (addrlen > SIN6_LEN) {
        addrlen = SIN6_LEN;
    }

    ercd = msg_buf_get(&msg);
    if (ercd != E_OK) {
        set_errno(tskid, UNET3_ER_ENOMEM, sockfd);
        return -1;
    }

    ercd = unet3_getsockopt(sockfd, UNET3_SOL_SOCKET, UNET3_SO_SNDTIMEO, &timeout, &timlen);
    if (ercd != E_OK) {
        goto connect_end;
    }

    msg->sockfd   = sockfd;
    msg->apitype  = API_TYPE_CON;
    msg->apiid    = API_ID_CONNECT;
    msg->msgtype  = MSG_TYPE_REQUEST;
    msg->from     = API_MSG_APPLICATION;
    msg->aptskid  = tskid;

    par = &msg->par.connect;
    net_memcpy(&par->addr, (void*)addr, addrlen);
    par->addrlen  = addrlen;

    can_wup(TSK_SELF);
    snd_mbx(MBX_BSD_REQ, (T_MSG*)msg);
    ercd = tslp_tsk(TIME_VALUE(&timeout));
    if (ercd == E_RLWAI) {
        msg_buf_ret(msg);
        set_errno(tskid, UNET3_ER_EINTR, sockfd);
        return -1;
    }

    if (ercd == E_TMOUT) {

        cancel_api(msg);
        set_errno(tskid, UNET3_ER_ETIMEDOUT, sockfd);
        goto connect_end;
   }
    ercd = msg->ercd;

connect_end:
    if (ercd != E_WBLK) {
        msg_buf_ret(msg);
    }
    return (ercd < 0 ? -1 : 0);
}


/*
    ssize_t send(int sockfd, const void *buf, size_t len, int flags);
*/
int unet3_send(int sockfd, const void *buf, unsigned int len, int flags)
{
    ER ercd;
    T_UNET3_API_MSG *msg;
    T_PARAM_SEND    *par;
    T_NET_BUF       *nbuf;
    struct unet3_timeval  timeout;
    unsigned int timlen = sizeof(timeout);
    ID tskid;

    get_tid(&tskid);
    if (buf == NULL) {
        set_errno(tskid, UNET3_ER_EINVAL, sockfd);
        return -1;
    }

    if (len > 0xFFFFU) {
        set_errno(tskid, UNET3_ER_EMSGSIZE, sockfd);
        return -1;
    }

    ercd = msg_buf_get(&msg);
    if (ercd != E_OK) {
        set_errno(tskid, UNET3_ER_ENOMEM, sockfd);
        return -1;
    }

    ercd = unet3_getsockopt(sockfd, UNET3_SOL_SOCKET, UNET3_SO_SNDTIMEO, &timeout, &timlen);
    if (ercd != E_OK) {
        msg_buf_ret(msg);
        return -1;
    }

    if ((flags & USE_APLBUF) != USE_APLBUF) {
        ercd = net_buf_get(&nbuf, len + sizeof(T_NET_BUF), TMO_POL);
        if (ercd != E_OK) {
            msg_buf_ret(msg);
            set_errno(tskid, UNET3_ER_ENOMEM, sockfd);
            return -1;
        }
    }

    msg->sockfd   = sockfd;
    msg->apitype  = API_TYPE_SND;
    msg->apiid    = API_ID_SEND;
    msg->msgtype  = MSG_TYPE_REQUEST;
    msg->from     = API_MSG_APPLICATION;
    msg->aptskid  = tskid;

    par = &msg->par.send;
    if ((flags & USE_APLBUF) != USE_APLBUF) {
        net_memcpy(nbuf->buf, (void*)buf, len);
        par->buf  = nbuf->buf;
    } else {
        par->buf  = (void*)buf;
        nbuf = NULL;
    }
    par->len      = len;
    par->flags    = flags;

    can_wup(TSK_SELF);
    snd_mbx(MBX_BSD_REQ, (T_MSG*)msg);
    ercd = tslp_tsk(TIME_VALUE(&timeout));
    if (ercd == E_RLWAI) {
        msg_buf_ret(msg);
        net_buf_ret(nbuf);
        set_errno(tskid, UNET3_ER_EINTR, sockfd);
        return -1;
    }

    if (ercd == E_TMOUT) {

        cancel_api(msg);
        set_errno(tskid, UNET3_ER_ETIMEDOUT, sockfd);
        goto send_end;
   }
    ercd = msg->ercd;

send_end:
    if (ercd != E_WBLK) {
        msg_buf_ret(msg);
    }
    net_buf_ret(nbuf);
    return (ercd < 0 ? -1 : ercd);
}


/*
    ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
*/
int unet3_sendto(int sockfd, const void *buf, unsigned int len, int flags, const struct unet3_sockaddr *dest_addr, unsigned int addrlen)
{
    ER ercd;
    T_UNET3_API_MSG *msg;
    T_PARAM_SENDTO    *par;
    T_NET_BUF       *nbuf;
    struct unet3_timeval  timeout;
    unsigned int timlen = sizeof(timeout);
    ID tskid;

    get_tid(&tskid);
    if (buf == NULL) {
        set_errno(tskid, UNET3_ER_EINVAL, sockfd);
        return -1;
    }
    if (dest_addr &&  addrlen < ADDRESS_LEN) {
        set_errno(tskid, UNET3_ER_EINVAL, sockfd);
        return -1;
    }
    if (len > 0xFFFFU) {
        set_errno(tskid, UNET3_ER_EMSGSIZE, sockfd);
        return -1;
    }
    if (addrlen > SIN6_LEN) {
        addrlen = SIN6_LEN;
    }

    ercd = msg_buf_get(&msg);
    if (ercd != E_OK) {
        set_errno(tskid, UNET3_ER_ENOMEM, sockfd);
        return -1;
    }

    ercd = unet3_getsockopt(sockfd, UNET3_SOL_SOCKET, UNET3_SO_SNDTIMEO, &timeout, &timlen);
    if (ercd != E_OK) {
        msg_buf_ret(msg);
        return -1;
    }

    if ((flags & USE_APLBUF) != USE_APLBUF) {
        ercd = net_buf_get(&nbuf, len + sizeof(T_NET_BUF), TMO_POL);
        if (ercd != E_OK) {
            msg_buf_ret(msg);
            set_errno(tskid, UNET3_ER_ENOMEM, sockfd);
            return -1;
        }
    }

    msg->sockfd   = sockfd;
    msg->apitype  = API_TYPE_SND;
    msg->apiid    = API_ID_SENDTO;
    msg->msgtype  = MSG_TYPE_REQUEST;
    msg->from     = API_MSG_APPLICATION;
    msg->aptskid  = tskid;

    par = &msg->par.sendto;
    if ((flags & USE_APLBUF) != USE_APLBUF) {
        net_memcpy(nbuf->buf, (void*)buf, len);
        par->buf = nbuf->buf;
    } else {
        par->buf = (void*)buf;
        nbuf = NULL;
    }
    par->len      = len;
    par->flags    = flags;

    if (dest_addr != NULL) {
        net_memcpy(&par->dest_addr, (void*)dest_addr, addrlen);
    } else {
        net_memset(&par->dest_addr, 0, sizeof(par->dest_addr));
    }
    par->addrlen  = addrlen;

    can_wup(TSK_SELF);
    snd_mbx(MBX_BSD_REQ, (T_MSG*)msg);
    ercd = tslp_tsk(TIME_VALUE(&timeout));
    if (ercd == E_RLWAI) {
        msg_buf_ret(msg);
        net_buf_ret(nbuf);
        set_errno(tskid, UNET3_ER_EINTR, sockfd);
        return -1;
    }

    if (ercd == E_TMOUT) {

        cancel_api(msg);
        set_errno(tskid, UNET3_ER_ETIMEDOUT, sockfd);
        goto sendto_end;
   }
    ercd = msg->ercd;

sendto_end:
    if (ercd != E_WBLK) {
        msg_buf_ret(msg);
    }
    net_buf_ret(nbuf);
    return (ercd < 0 ? -1 : ercd);
}


/*
    ssize_t recv(int sockfd, void *buf, size_t len, int flags);
*/
int unet3_recv(int sockfd, void *buf, unsigned int len, int flags)
{
    ER ercd;
    T_UNET3_API_MSG *msg;
    T_PARAM_RECV    *par;
    T_NET_BUF       *nbuf;
    struct unet3_timeval  timeout;
    unsigned int timlen = sizeof(timeout);
    ID tskid;

    get_tid(&tskid);
    if (buf == NULL) {
        set_errno(tskid, UNET3_ER_EINVAL, sockfd);
        return -1;
    }
    if (len > 0xFFFFU) {
        len = 0xFFFFU;
    }

    ercd = msg_buf_get(&msg);
    if (ercd != E_OK) {
        set_errno(tskid, UNET3_ER_ENOMEM, sockfd);
        return -1;
    }

    ercd = unet3_getsockopt(sockfd, UNET3_SOL_SOCKET, UNET3_SO_RCVTIMEO, &timeout, &timlen);
    if (ercd != E_OK) {
        msg_buf_ret(msg);
        return -1;
    }

    if ((flags & USE_APLBUF) != USE_APLBUF) {
        ercd = net_buf_get(&nbuf, len + sizeof(T_NET_BUF), TMO_POL);
        if (ercd != E_OK) {
            msg_buf_ret(msg);
            set_errno(tskid, UNET3_ER_ENOMEM, sockfd);
            return -1;
        }
    }

    msg->sockfd   = sockfd;
    msg->apitype  = API_TYPE_RCV;
    msg->apiid    = API_ID_RECV;
    msg->msgtype  = MSG_TYPE_REQUEST;
    msg->from     = API_MSG_APPLICATION;
    msg->aptskid  = tskid;

    par = &msg->par.recv;
    if ((flags & USE_APLBUF) != USE_APLBUF) {
        par->buf  = nbuf->buf;
    } else {
        par->buf  = buf;
        nbuf = NULL;
    }
    par->len      = len;
    par->flags    = flags;

    can_wup(TSK_SELF);
    snd_mbx(MBX_BSD_REQ, (T_MSG*)msg);
    ercd = tslp_tsk(TIME_VALUE(&timeout));
    if (ercd == E_RLWAI) {
        msg_buf_ret(msg);
        net_buf_ret(nbuf);
        set_errno(tskid, UNET3_ER_EINTR, sockfd);
        return -1;
    }

    if (ercd == E_TMOUT) {

        cancel_api(msg);
        set_errno(tskid, UNET3_ER_ETIMEDOUT, sockfd);
        ercd = -1;
        goto recv_end;
    }

    ercd = msg->ercd;
    if (ercd <= 0) {
        goto recv_end;
    }
    if ((flags & USE_APLBUF) != USE_APLBUF) {
        net_memcpy(buf, nbuf->buf, ercd);
    }

recv_end:
    if (ercd != E_WBLK) {
        msg_buf_ret(msg);
    }
    net_buf_ret(nbuf);
    return (ercd < 0 ? -1 : ercd);
}


/*
    ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
*/
int unet3_recvfrom(int sockfd, void *buf, unsigned int len, int flags, struct unet3_sockaddr *src_addr, unsigned int *addrlen)
{
    ER ercd;
    T_UNET3_API_MSG *msg;
    T_PARAM_RECVFROM *par;
    T_NET_BUF       *nbuf;
    struct unet3_timeval  timeout;
    unsigned int timlen = sizeof(timeout);
    ID tskid;

    get_tid(&tskid);
    if (buf == NULL) {
        set_errno(tskid, UNET3_ER_EINVAL, sockfd);
        return -1;
    }
    if (src_addr) {
        if (addrlen == NULL) {
            set_errno(tskid, UNET3_ER_EINVAL, sockfd);
            return -1;
        }
        if (*addrlen > SIN6_LEN) {
            *addrlen = SIN6_LEN;
        }
    }
    if (len > 0xFFFFU) {
        len = 0xFFFFU;
    }

    ercd = msg_buf_get(&msg);
    if (ercd != E_OK) {
        set_errno(tskid, UNET3_ER_ENOMEM, sockfd);
        return -1;
    }

    ercd = unet3_getsockopt(sockfd, UNET3_SOL_SOCKET, UNET3_SO_RCVTIMEO, &timeout, &timlen);
    if (ercd != E_OK) {
        msg_buf_ret(msg);
        return -1;
    }

    if ((flags & USE_APLBUF) != USE_APLBUF) {
        ercd = net_buf_get(&nbuf, len + sizeof(T_NET_BUF), TMO_POL);
        if (ercd != E_OK) {
            msg_buf_ret(msg);
            set_errno(tskid, UNET3_ER_ENOMEM, sockfd);
            return -1;
        }
    }

    msg->sockfd   = sockfd;
    msg->apitype  = API_TYPE_RCV;
    msg->apiid    = API_ID_RECVFROM;
    msg->msgtype  = MSG_TYPE_REQUEST;
    msg->from     = API_MSG_APPLICATION;
    msg->aptskid  = tskid;

    par = &msg->par.recvfrom;
    if ((flags & USE_APLBUF) != USE_APLBUF) {
        par->buf  = nbuf->buf;
    } else {
        par->buf  = buf;
        nbuf = NULL;
    }
    par->len      = len;
    par->flags    = flags;

    can_wup(TSK_SELF);
    snd_mbx(MBX_BSD_REQ, (T_MSG*)msg);
    ercd = tslp_tsk(TIME_VALUE(&timeout));
    if (ercd == E_RLWAI) {
        msg_buf_ret(msg);
        net_buf_ret(nbuf);
        set_errno(tskid, UNET3_ER_EINTR, sockfd);
        return -1;
    }

    if (ercd == E_TMOUT) {

        cancel_api(msg);
        set_errno(tskid, UNET3_ER_ETIMEDOUT, sockfd);
        ercd = -1;
        goto recvfrom_end;
    }

    ercd = msg->ercd;
    if (ercd <= 0) {
        goto recvfrom_end;
    }
    if ((flags & USE_APLBUF) != USE_APLBUF) {
        net_memcpy(buf, nbuf->buf, ercd);
    }
    if (src_addr) {
        if (((unet3_sockaddr*)&par->src_addr)->sa_family == AF_INET) {
            net_memcpy(src_addr, &par->src_addr, par->addrlen);
        } else {
            net_memcpy(src_addr, &par->src_addr, *addrlen);
        }
        *addrlen = par->addrlen;
    }

recvfrom_end:
    if (ercd != E_WBLK) {
        msg_buf_ret(msg);
    }
    net_buf_ret(nbuf);
    return (ercd < 0 ? -1 : ercd);
}


/*
    int shutdown(int sockfd, int how);  
*/
int unet3_shutdown(int sockfd, int how)
{
    ER ercd;
    T_UNET3_API_MSG *msg;
    T_PARAM_SHUTDOWN *par;
    ID tskid;

    get_tid(&tskid);
    ercd = msg_buf_get(&msg);
    if (ercd != E_OK) {
        set_errno(tskid, UNET3_ER_ENOMEM, sockfd);
        return -1;
    }
    msg->sockfd   = sockfd;
    msg->apitype  = API_TYPE_CLS;
    msg->apiid    = API_ID_SHUTDOWN;
    msg->msgtype  = MSG_TYPE_REQUEST;
    msg->from     = API_MSG_APPLICATION;
    msg->aptskid  = tskid;

    par = &msg->par.shutdown;
    par->how      = how;

    can_wup(TSK_SELF);
    snd_mbx(MBX_BSD_REQ, (T_MSG*)msg);
    ercd = slp_tsk();
    if (ercd == E_RLWAI) {
        msg_buf_ret(msg);
        set_errno(tskid, UNET3_ER_EINTR, sockfd);
        return -1;
    }

    ercd = msg->ercd;
    /* BSD spec */
    /* TCP shutdown does not block process     */
    if (ercd != E_WBLK) {
        msg_buf_ret(msg);
    }
    return ((ercd < 0 && ercd != E_WBLK) ? -1 : 0);
}


/*
    int close(int fd);
*/
int unet3_close(int fd)
{
    ER ercd;
    T_UNET3_API_MSG *msg;
    ID tskid;

    get_tid(&tskid);

    ercd = msg_buf_get(&msg);
    if (ercd != E_OK) {
        set_errno(tskid, UNET3_ER_ENOMEM, fd);
        return -1;
    }
    msg->sockfd   = fd;
    msg->apitype  = API_TYPE_CLS;
    msg->apiid    = API_ID_CLOSE;
    msg->msgtype  = MSG_TYPE_REQUEST;
    msg->from     = API_MSG_APPLICATION;
    msg->aptskid  = tskid;

    can_wup(TSK_SELF);
    snd_mbx(MBX_BSD_REQ, (T_MSG*)msg);
    ercd = slp_tsk();
    if (ercd == E_RLWAI) {
        msg_buf_ret(msg);
        set_errno(tskid, UNET3_ER_EINTR, fd);
        return -1;
    }

    ercd = msg->ercd;
    /* BSD spec */
    /* TCP shutdown does not block process     */
    /* E_WBLK means closing TCP session        */
    if (ercd != E_WBLK) {
        msg_buf_ret(msg);
    }
    return ((ercd < 0 && ercd != E_WBLK) ? -1 : 0);
}


/*
    int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
*/
int unet3_select(int nfds, unet3_fd_set *readfds, unet3_fd_set *writefds, unet3_fd_set *exceptfds, struct unet3_timeval *timeout)
{
    ER ercd;
    T_UNET3_API_MSG *msg;
    T_PARAM_SELECT *par;
    ID tskid;

    get_tid(&tskid);
    ercd = msg_buf_get(&msg);
    if (ercd != E_OK) {
        set_errno(tskid, UNET3_ER_ENOMEM, 0);
        return -1;
    }
    msg->sockfd   = 0;
    msg->apitype  = API_TYPE_NOT_IO;
    msg->apiid    = API_ID_SELECT;
    msg->msgtype  = MSG_TYPE_REQUEST;
    msg->from     = API_MSG_APPLICATION;
    msg->aptskid  = tskid;

    par = &msg->par.select;

    par->nfds     = nfds;
    unet3_FD_ZERO(&par->readfds);
    unet3_FD_ZERO(&par->writefds);
    unet3_FD_ZERO(&par->exceptfds);
    par->r = par->w = par->e = 0;

    if (readfds) {
        par->readfds  = *readfds;
        par->r = 1;
    }
    if (writefds) {
        par->writefds = *writefds;
        par->w = 1;
    }
    if (exceptfds) {
        par->exceptfds = *exceptfds;
    }

    can_wup(TSK_SELF);
    snd_mbx(MBX_BSD_REQ, (T_MSG*)msg);
    /* wait selecting */
    slp_tsk();

    /* selected descriptor */
    if (msg->ercd > 0) {
        goto pickup_event;
    }
    else if (msg->ercd < E_OK) {
        ercd = msg->ercd;
        goto select_end;
    }

    /* wait I/O event */
    msg->aptskid  = tskid;
    ercd = tslp_tsk(timeout ? TIME_VALUE(timeout) : TMO_FEVR);

    if (ercd < E_OK || msg->ercd < 1) {
        /* request to discard the state of waiting event */
        cancel_api(msg);
        ercd = 0;
        goto select_end;
    }

    /* occur I/O event */
pickup_event:
    if (readfds) {
       *readfds = par->readfds;
    }
    if (writefds) {
       *writefds = par->writefds;
    }
    if (exceptfds) {
       *exceptfds = par->exceptfds;
    }
    ercd = msg->ercd;

select_end:
    msg_buf_ret(msg);
    return (ercd == 0) ? 0 : (ercd < 0) ? -1 : ercd ;
}

#if (SYS_PLATFORM)
int unet3_select_u(int sockfd, BOOL r, BOOL w, BOOL e, VP cbk)
{
    ER ercd;
    T_UNET3_API_MSG *msg;
    T_PARAM_SELECT_U  *par;
    ID tskid;

    get_tid(&tskid);
    ercd = msg_buf_get(&msg);
    if (ercd != E_OK) {
        set_errno(tskid, UNET3_ER_ENOMEM, 0);
        return -1;
    }
    msg->sockfd   = sockfd;
    msg->apitype  = API_TYPE_NOT_IO;
    msg->apiid    = API_ID_SELECT_U;
    msg->from     = API_MSG_APPLICATION;
    msg->aptskid  = tskid;

    par = &msg->par.select_u;

    if (cbk) {
        msg->msgtype  = MSG_TYPE_REQUEST;
    } else {
        msg->msgtype  = MSG_TYPE_CANCEL;
    }
    par->r = r;
    par->w = w;
    par->e = e;
    par->cbk = (VP)cbk;
    par->tskid = tskid;

    can_wup(TSK_SELF);
    snd_mbx(MBX_BSD_REQ, (T_MSG*)msg);
    /* wait selecting */
    slp_tsk();

    ercd = msg->ercd;
    if (msg->msgtype == MSG_TYPE_CANCEL) {
        msg_buf_ret(msg);
    }

    return (ercd < 0 ? -1 : ercd);
}
#endif

/*
    int ioctl(int d, int request, ...);  
*/
int unet3_ioctl(int d, int request, ...)
{
    ER ercd;
    T_UNET3_API_MSG *msg;
    T_PARAM_IOCTL   *par;
    va_list argp;
    int *val;
    ID tskid;

    get_tid(&tskid);
    ercd = msg_buf_get(&msg);
    if (ercd != E_OK) {
        set_errno(tskid, UNET3_ER_ENOMEM, d);
        return -1;
    }

    va_start(argp, request);
    val = va_arg(argp, int*);
    va_end(argp);

    if (val == NULL) {
        msg_buf_ret(msg);
        set_errno(tskid, UNET3_ER_EFAULT, d);
        return -1;
    }

    msg->sockfd   = d;
    msg->apitype  = API_TYPE_NOT_IO;
    msg->apiid    = API_ID_IOCTL;
    msg->from     = API_MSG_APPLICATION;
    msg->aptskid  = tskid;

    par = &msg->par.ioctl;
    par->request = request;
    par->val = val;

    can_wup(TSK_SELF);
    snd_mbx(MBX_BSD_REQ, (T_MSG*)msg);
    ercd = slp_tsk();
    if (ercd == E_RLWAI) {
        msg_buf_ret(msg);
        set_errno(tskid, UNET3_ER_EINTR, d);
        return -1;
    }

    ercd = msg->ercd;
    msg_buf_ret(msg);
    return (ercd < 0 ? -1 : ercd);
}


/*
    int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
*/
int unet3_getsockname(int sockfd, struct unet3_sockaddr *addr, unsigned int *addrlen)
{
    ER ercd;
    T_UNET3_API_MSG *msg;
    T_PARAM_GETSOCKNAME *par;
    ID tskid;

    get_tid(&tskid);

    if (addr == NULL || addrlen == NULL || *addrlen < ADDRESS_LEN) {
        set_errno(tskid, UNET3_ER_EINVAL, sockfd);
        return -1;
    }
    if (*addrlen > SIN6_LEN) {
        *addrlen = SIN6_LEN;
    }

    ercd = msg_buf_get(&msg);
    if (ercd != E_OK) {
        set_errno(tskid, UNET3_ER_ENOMEM, sockfd);
        return -1;
    }
    msg->sockfd   = sockfd;
    msg->apitype  = API_TYPE_NOT_IO;
    msg->apiid    = API_ID_GETSOCKNAME;
    msg->from     = API_MSG_APPLICATION;
    msg->aptskid  = tskid;

    par = &msg->par.getsockname;
    par->addrlen  = *addrlen;

    can_wup(TSK_SELF);
    snd_mbx(MBX_BSD_REQ, (T_MSG*)msg);
    ercd = slp_tsk();
    if (ercd == E_RLWAI) {
        msg_buf_ret(msg);
        set_errno(tskid, UNET3_ER_EINTR, sockfd);
        return -1;
    }

    /* set out parameter */
    ercd = msg->ercd;
    if (ercd != E_OK) {
        ercd = -1;
    } else {
        if (((unet3_sockaddr*)&par->addr)->sa_family == AF_INET) {
            net_memcpy(addr, &par->addr, par->addrlen);
        } else {
            net_memcpy(addr, &par->addr, *addrlen);
        }
        *addrlen = par->addrlen;
        ercd = msg->ercd;
    }
    msg_buf_ret(msg);
    return ercd;
}


/*
    int getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen);  
*/
int unet3_getpeername(int sockfd, struct unet3_sockaddr *addr, unsigned int *addrlen)
{
    ER ercd;
    T_UNET3_API_MSG *msg;
    T_PARAM_GETPEERNAME *par;
    ID tskid;

    get_tid(&tskid);

    if (addr == NULL || addrlen == NULL || *addrlen < ADDRESS_LEN) {
        set_errno(tskid, UNET3_ER_EINVAL, sockfd);
        return -1;
    }
    if (*addrlen > SIN6_LEN) {
        *addrlen = SIN6_LEN;
    }

    ercd = msg_buf_get(&msg);
    if (ercd != E_OK) {
        set_errno(tskid, UNET3_ER_ENOMEM, sockfd);
        return -1;
    }
    msg->sockfd   = sockfd;
    msg->apitype  = API_TYPE_NOT_IO;
    msg->apiid    = API_ID_GETPEERNAME;
    msg->from     = API_MSG_APPLICATION;
    msg->aptskid  = tskid;

    par = &msg->par.getpeername;
    par->addrlen  = *addrlen;

    can_wup(TSK_SELF);
    snd_mbx(MBX_BSD_REQ, (T_MSG*)msg);
    ercd = slp_tsk();
    if (ercd == E_RLWAI) {
        msg_buf_ret(msg);
        set_errno(tskid, UNET3_ER_EINTR, sockfd);
        return -1;
    }

    /* set out parameter */
    ercd = msg->ercd;
    if (ercd != E_OK) {
        ercd = -1;
    } else {
        if (((unet3_sockaddr*)&par->addr)->sa_family == AF_INET) {
            net_memcpy(addr, &par->addr, par->addrlen);
        } else {
            net_memcpy(addr, &par->addr, *addrlen);
        }
        *addrlen = par->addrlen;
        ercd = msg->ercd;
    }
    msg_buf_ret(msg);
    return ercd;
}


/*
    int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
*/
int unet3_getsockopt(int sockfd, int level, int optname, void *optval, unsigned int *optlen)
{
    ER ercd;
    T_UNET3_API_MSG *msg;
    T_PARAM_GETSOCKOPT *par;
    ID tskid;

    get_tid(&tskid);

    if (optval == NULL || optlen == NULL || *optlen == 0) {
        set_errno(tskid, UNET3_ER_EINVAL, sockfd);
        return -1;
    }

    ercd = msg_buf_get(&msg);
    if (ercd != E_OK) {
        set_errno(tskid, UNET3_ER_ENOMEM, sockfd);
        return -1;
    }
    msg->sockfd   = sockfd;
    msg->apitype  = API_TYPE_NOT_IO;
    msg->apiid    = API_ID_GETSOCKOPT;
    msg->from     = API_MSG_APPLICATION;
    msg->aptskid  = tskid;

    par = &msg->par.getsockopt;
    par->level    = level;
    par->optname  = optname;

    if (*optlen > SOC_OPT_MAXLEN) {
        *optlen = SOC_OPT_MAXLEN;
    }
    par->optlen = *optlen;

    can_wup(TSK_SELF);
    snd_mbx(MBX_BSD_REQ, (T_MSG*)msg);
    ercd = slp_tsk();
    if (ercd == E_RLWAI) {
        msg_buf_ret(msg);
        set_errno(tskid, UNET3_ER_EINTR, sockfd);
        return -1;
    }

    /* set out parameter */
    ercd = msg->ercd;
    if (ercd == E_OK) {
        net_memcpy(optval, &par->optval, par->optlen);
        *optlen = par->optlen;
    }

    msg_buf_ret(msg);
    return (ercd != E_OK) ? -1 : 0 ;
}


/*
    int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
*/
int unet3_setsockopt(int sockfd, int level, int optname, const void *optval, unsigned int optlen)
{
    ER ercd;
    T_UNET3_API_MSG *msg;
    T_PARAM_SETSOCKOPT *par;
    ID tskid;

    get_tid(&tskid);

    if (optval == NULL || optlen == 0 || optlen > SOC_OPT_MAXLEN) {
        set_errno(tskid, UNET3_ER_EINVAL, sockfd);
        return -1;
    }

    ercd = msg_buf_get(&msg);
    if (ercd != E_OK) {
        set_errno(tskid, UNET3_ER_ENOMEM, sockfd);
        return -1;
    }
    msg->sockfd   = sockfd;
    msg->apitype  = API_TYPE_NOT_IO;
    msg->apiid    = API_ID_SETSOCKOPT;
    msg->from     = API_MSG_APPLICATION;
    msg->aptskid  = tskid;

    par = &msg->par.setsockopt;
    par->level    = level;
    par->optname  = optname;
    net_memcpy(&par->optval, (void*)optval, optlen);
    par->optlen   = optlen;

    can_wup(TSK_SELF);
    snd_mbx(MBX_BSD_REQ, (T_MSG*)msg);
    ercd = slp_tsk();
    if (ercd == E_RLWAI) {
        msg_buf_ret(msg);
        set_errno(tskid, UNET3_ER_EINTR, sockfd);
        return -1;
    }

    ercd = msg->ercd;
    msg_buf_ret(msg);
    return (ercd != E_OK) ? -1 : 0 ;
}


/*
    in_addr_t inet_addr(const char *cp);
*/
unsigned int unet3_inet_addr(const char *cp)
{
    return htonl(ip_aton(cp));
}

/*
    int inet_aton(const char *cp, struct in_addr *inp);
*/
int unet3_inet_aton(const char *cp, struct unet3_in_addr *inp)
{
    /*support decimal notation only*/
    inp->s_addr = htonl(ip_aton(cp));
    if (inp->s_addr == 0) {
        return -1;
    }
    return 0;
}

/*
    char *inet_ntoa(struct in_addr in);
*/
char ntoa_str[16];
char *unet3_inet_ntoa(struct unet3_in_addr in)
{
    net_memset(ntoa_str, 0, sizeof(ntoa_str));
    in.s_addr = ntohl(in.s_addr);
    ip_ntoa(ntoa_str, in.s_addr);
    return ntoa_str;
}

unsigned int if_nametoindex(const char *ifname)
{
    int indx;
    ID tid;

    indx = get_devid_name(ifname);
    if (indx == DEV_ANY) {
        get_tid(&tid);
        set_errno(tid, UNET3_ER_ENXIO, 0);
    }
    return indx;
}

char *if_indextoname(unsigned int ifindex, char *ifname)
{
    ID tid;

    if (ifindex < 1 || NET_DEV_MAX < ifindex) {
        get_tid(&tid);
        set_errno(tid, UNET3_ER_ENXIO, 0);
        return NULL;
    }
    loc_tcp();
    net_strcpy(ifname, (const char*)gNET_DEV[ifindex-1].name);
    ulc_tcp();
    return ifname;
}

int unet3_rresvport(int *port)
{
    UH lport;
    SID sid;

    for (;*port < 1024; (*port)++) {
        for (sid = 0; sid < NET_SOC_MAX; sid++) {
            loc_tcp();
            lport = pNET_SOC[sid].lport;
            ulc_tcp();
            if (*port == lport) {
                return (int)sid;
            }
        }
    }
    return -1;
}

int unet3_getifaddrs(struct unet3_ifaddrs **ifap)
{
    T_NET_ADR adr;
    unet3_sockaddr_in* adr_in;
    unet3_ifaddrs *ifa;
    VP buf;
    int i;
    ID tskid;
#ifdef IPV6_SUP
    unet3_sockaddr_in6* adr_in6;
    T_NET6_ADR *adr6;
    T_PREFIX_LST *prelst;
    UH j, k;
    UH prefix_len;
    UH cnt, lstcnt;
    UB adrscope;
#endif

    get_tid(&tskid);

    if (ifap == NULL) {
        set_errno(tskid, UNET3_ER_EINVAL, 0);
        return -1;
    }

    /* IPv4 */
    *ifap = ifa = NULL;
    for (i = 0; i < NET_DEV_MAX; i++) {

        if (*ifap) {
            ifap = &(*ifap)->ifa_next;
        }
        if (msg_buf_get((T_UNET3_API_MSG**)&buf) != E_OK) {
            unet3_freeifaddrs(*ifap);
            set_errno(tskid, UNET3_ER_ENOMEM, 0);
            return -1;
        }
        if (ifa) {
            ifa->ifa_next = (unet3_ifaddrs*)buf;
            ifa = ifa->ifa_next;
        } else {
            *ifap = ifa = (unet3_ifaddrs*)buf;
        }

        ifa->ifa_ifu.ifu_broadaddr = ifa->ifa_netmask + 1;

        ifa->ifa_next = NULL;
        ifa->ifa_name = (char*)gNET_DEV[i].name;
        ifa->ifa_flags = i+1; /* device number */

        net_ref(i+1, NET_IP4_CFG, &adr);

        ifa->ifa_addr = (unet3_sockaddr*)(ifa+1);
        adr_in = (unet3_sockaddr_in*)ifa->ifa_addr;
        adr_in->sin_addr.s_addr = htonl(adr.ipaddr);
        adr_in->sin_port = 0;
        adr_in->sin_family = UNET3_AF_INET;
        adr_in->sin_len = ADDRESS_LEN;

        ifa->ifa_netmask = (unet3_sockaddr*)((unet3_sockaddr_in*)ifa->ifa_addr + 1);
        adr_in = (unet3_sockaddr_in*)ifa->ifa_netmask;
        adr_in->sin_addr.s_addr = htonl(adr.mask);
        adr_in->sin_port = 0;
        adr_in->sin_family = UNET3_AF_INET;
        adr_in->sin_len = ADDRESS_LEN;

        ifa->ifa_ifu.ifu_broadaddr = (unet3_sockaddr*)((unet3_sockaddr_in*)ifa->ifa_netmask + 1);
        adr_in = (unet3_sockaddr_in*)ifa->ifa_ifu.ifu_broadaddr;
        adr_in->sin_addr.s_addr = htonl((adr.ipaddr | ~adr.mask));
        adr_in->sin_port = 0;
        adr_in->sin_family = UNET3_AF_INET;
        adr_in->sin_len = ADDRESS_LEN;

        ifa->ifa_data = NULL;

    }

#ifdef IPV6_SUP
    /* IPv6 */
    for (i = 0; i < NET_DEV_MAX; i++) {
        adr6 = &gNET6_ADR[i];

        for (cnt = 0; cnt < 3; cnt++) {
            if (adr6->ip6addr_ucast[cnt].state == IP6_ADDR_PREFERRED) {
                if (*ifap) {
                    ifap = &(*ifap)->ifa_next;
                }
                if (msg_buf_get((T_UNET3_API_MSG**)&buf) != E_OK) {
                    unet3_freeifaddrs(*ifap);
                    set_errno(tskid, UNET3_ER_ENOMEM, 0);
                    return -1;
                }
                if (ifa) {
                    ifa->ifa_next = (unet3_ifaddrs*)buf;
                    ifa = ifa->ifa_next;
                } else {
                    *ifap = ifa = (unet3_ifaddrs*)buf;
                }

                ifa->ifa_next = NULL;
                ifa->ifa_name = (char*)gNET_DEV[i].name;
                ifa->ifa_flags = i+1; /* device number */

                /* Address of interface */
                ifa->ifa_addr = (unet3_sockaddr*)(ifa+1);
                adr_in6 = (unet3_sockaddr_in6*)ifa->ifa_addr;
                ip6_addr_hton(adr_in6->sin6_addr.unet3_s6_addr32,
                              adr6->ip6addr_ucast[cnt].addr_uw);
                adr_in6->sin6_port = 0;
                adr_in6->sin6_scope_id = get_devid_ip6addr(adr6->ip6addr_ucast[cnt].addr_uw);
                adr_in6->sin6_family = UNET3_AF_INET6;
                adr_in6->sin6_len = SIN6_LEN;

                /* Netmask of interface */
                ifa->ifa_netmask = (unet3_sockaddr*)((unet3_sockaddr_in6*)ifa->ifa_addr + 1);
                adr_in6 = (unet3_sockaddr_in6*)ifa->ifa_netmask;
                /* default subnetmask (64bit) */
                adr_in6->sin6_addr.unet3_s6_addr32[0] = htonl(0xFFFFFFFF);
                adr_in6->sin6_addr.unet3_s6_addr32[1] = htonl(0xFFFFFFFF);
                adr_in6->sin6_addr.unet3_s6_addr32[2] = htonl(0x00000000);
                adr_in6->sin6_addr.unet3_s6_addr32[3] = htonl(0x00000000);

                adrscope = ip6_addr_scope(adr6->ip6addr_ucast[cnt].addr_uw);
                if (adrscope == IP6_ADDR_GLOBAL) {
                    for (lstcnt = 0; lstcnt < NET_PRFX_LST; lstcnt++) {
                        prelst = &gPRFX_LST[lstcnt];
                        if (prelst->onlink == TRUE) {
                            prefix_len = prelst->prfx_len << 3;

                            if (prefix_len >= 128) {
                                /* subnetmask (128bit) */
                                adr_in6->sin6_addr.unet3_s6_addr32[0] = htonl(0xFFFFFFFF);
                                adr_in6->sin6_addr.unet3_s6_addr32[1] = htonl(0xFFFFFFFF);
                                adr_in6->sin6_addr.unet3_s6_addr32[2] = htonl(0xFFFFFFFF);
                                adr_in6->sin6_addr.unet3_s6_addr32[3] = htonl(0xFFFFFFFF);
                            } else if (prefix_len == 0) {
                                /* subnetmask (0bit) */
                                adr_in6->sin6_addr.unet3_s6_addr32[0] = htonl(0);
                                adr_in6->sin6_addr.unet3_s6_addr32[1] = htonl(0);
                                adr_in6->sin6_addr.unet3_s6_addr32[2] = htonl(0);
                                adr_in6->sin6_addr.unet3_s6_addr32[3] = htonl(0);
                            } else {
                                for (j = 0; j < 4; j++) {
                                    adr_in6->sin6_addr.unet3_s6_addr32[j] = 0;
                                    for (k = 0; k < 32; k++) {
                                        if (prefix_len > 0) {
                                            prefix_len--;
                                            adr_in6->sin6_addr.unet3_s6_addr32[j] |= (0x80000000 >> k);
                                        }
                                    }
                                }
                                ip6_addr_hton(adr_in6->sin6_addr.unet3_s6_addr32,
                                              adr_in6->sin6_addr.unet3_s6_addr32);
                            }
                            break;
                        }
                    }
                }
                adr_in6->sin6_port = 0;
                adr_in6->sin6_scope_id = 0;
                adr_in6->sin6_family = UNET3_AF_INET6;
                adr_in6->sin6_len = SIN6_LEN;

                /* IPv6 is not Broadcast address */
                ifa->ifa_ifu.ifu_broadaddr = NULL;

                /* Address-specific data */
                ifa->ifa_data = NULL;
            }
        }
    }
#endif
    return 0;
}

void unet3_freeifaddrs(struct unet3_ifaddrs *ifa)
{
    struct unet3_ifaddrs *ifa_nxt;

    while (ifa) {
        ifa_nxt = ifa->ifa_next;
        msg_buf_ret((T_UNET3_API_MSG*)ifa);
        ifa = ifa_nxt;
    }
}

/*
    int inet_pton(int af, const char *src, void *dst);
*/
int unet3_inet_pton(int af, const char *src, void *dst)
{
    ID tskid;
    unet3_in_addr addr;
    unet3_in6_addr ip6addr;

    net_memset(&addr, 0, sizeof(addr));
    net_memset(&ip6addr, 0, sizeof(ip6addr));

    get_tid(&tskid);

    if ((af != AF_INET) && (af != AF_INET6)) {
        set_errno(tskid, UNET3_ER_EAFNOSUPPORT, 0);
        return -1;
    }

    if ((src == NULL) || (dst == NULL)) {
        set_errno(tskid, UNET3_ER_EINVAL, 0);
        return -1;
    }

#ifdef IPV6_SUP
    if (af == AF_INET) {
#endif
        addr.s_addr = htonl(ip_aton(src));
        net_memcpy(dst, &addr, sizeof(unet3_in_addr));
#ifdef IPV6_SUP
    } else {
        ip6_aton(src, ip6addr.unet3_s6_addr32);
        /* host to network long */
        ip6_addr_hton(ip6addr.unet3_s6_addr32,
                      ip6addr.unet3_s6_addr32);
        net_memcpy(dst, &ip6addr, sizeof(unet3_in6_addr));
    }
#endif

    return 1;
}

/*
    const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
*/
const char *unet3_inet_ntop(int af, const void *src, char *dst, unsigned int size)
{
    ID tskid;
    UW cpylen;
    unet3_in_addr addr;
    unet3_in6_addr ip6addr;
    char addrstr[46];

    net_memset(&addr, 0, sizeof(addr));
    net_memset(&ip6addr, 0, sizeof(ip6addr));
    net_memset(addrstr, 0, sizeof(addrstr));

    get_tid(&tskid);

    if ((af != AF_INET) && (af != AF_INET6)) {
        set_errno(tskid, UNET3_ER_EAFNOSUPPORT, 0);
        return NULL;
    }

    if ((src == NULL) || (dst == NULL)) {
        set_errno(tskid, UNET3_ER_EINVAL, 0);
        return NULL;
    }

#ifdef IPV6_SUP
    if (af == AF_INET) {
#endif
        addr.s_addr = ntohl(((unet3_in_addr*)src)->s_addr);
        ip_ntoa(addrstr, addr.s_addr);
#ifdef IPV6_SUP
    } else {
        /* network to host long */
        ip6_addr_ntoh(ip6addr.unet3_s6_addr32,
                      ((unet3_in6_addr*)src)->unet3_s6_addr32);
        ip6_ntoa(addrstr, ip6addr.unet3_s6_addr32);
    }
#endif

    cpylen = net_strlen(addrstr) + 1;
    if (size < cpylen) {
        set_errno(tskid, UNET3_ER_ENOSPC, 0);
        return NULL;
    }
    net_memcpy(dst, addrstr, cpylen);
    return dst;
}

