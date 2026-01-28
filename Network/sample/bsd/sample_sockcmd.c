/***************************************************************************
    MICRO C CUBE / COMPACT, NETWORK Application
    BSD Socket API command interface Sample
    Copyright (c) 2014-2023, eForce Co., Ltd. All rights reserved.

    Version Information
      2014-05-01: Created
      2021-05-28: Update for IPv6(AF_INET6)
      2023.06.02: Suppressed warning of Zynq GCC compiler.
 ***************************************************************************/

#include <sys/ioctl.h>  // ioctl
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/errno.h>

#include "kernel.h"
#include "net_strlib.h"
#include "shell.h"

#define S_LINE  "\r\n"
#define MAX_CMD_LEN     1024  /* Max Command Length */
char CmdBuf[MAX_CMD_LEN];    /* Command Buffer */

#define  PRM_NONE               0x0000
#define  PRM_SOCFD              0x0001
#define  PRM_FDSET              0x0002
#define  PRM_PROTO              0x0004
#define  PRM_LADDR              0x0008
#define  PRM_RADDR              0x0010
#define  PRM_BUFF               0x0020
#define  PRM_TIME               0x0040
#define  PRM_HOW                0x0080
#define  PRM_OPTION             0x0100
#define  PRM_BACKLOG            0x0200
#define  PRM_IOCTL              0x0400
#define  PRM_LEN_P              0x0800
#define  PRM_FLAGS              0x1000
#define  PRM_BLKNUM             0x8000
#define  PRM_GETADDR            0x10000
#define  PRM_FREEADDR           0x20000
#define  PRM_INETPTON           0x40000
#define  PRM_INETNTOP           0x80000


#define SOT_INT         0
#define SOT_TIME        1
#define SOT_LINGER      2
#define SOT_IP_MREQN    3
#define SOT_IP_PKNFO    4
#define SOT_IPV6_MREQ   5


typedef int (*SOCKET_API)(void*, void*);

typedef struct socket_api_info {
    SOCKET_API  func;
    int         in_param;
    int         out_param;
    const char *api_name;
}SOCKET_API_INFO;

typedef struct t_val_socket {
    int domain;
    int type;
} T_VAL_SOCKET;

typedef struct t_val_option {
    int name;
    int type;
    void *val;
    int level;
    int ival;
    struct linger ling;
    struct timeval time;
    struct ip_mreqn imreqn;
    struct in_pktinfo ipktnfo;
    struct ipv6_mreq v6mreq;
} T_VAL_OPTION;

typedef struct socket_api_param {
    int socfd;
    struct sockaddr_in  laddr, raddr;
    struct sockaddr_in6  laddr6, raddr6;
    struct sockaddr  *laddr_p, *raddr_p;
    int buff; /* length */
    void *buffp; /* pointer */
    struct timeval time;
    int how;
    int backlog;

    socklen_t llen, rlen, *llen_p, *rlen_p;
    socklen_t olen, *olen_p;

    int to_ena;
    int nwfds;
    int nrfds;
    int nefds;
    fd_set wfds;
    fd_set rfds;
    fd_set efds;
    int fds_n;
    int max_fds;

    int ioctlreq;
    int ioctlval;
    int *ioctlval_p;

    T_VAL_SOCKET skt;
    T_VAL_OPTION opt;

    int blknum;
    int flags;
    int family;
    char ipstr[100];
    unsigned int ipval[4];
    char *ipstr_p;
    void *ipval_p;
    struct ifaddrs *ifa;
    struct ifaddrs **ifa_p;
}SOCKET_API_PARAM;

#define MAX_SOCK_NUM    BSD_SOCKET_MAX
unsigned char sock_use[MAX_SOCK_NUM] = {0};

#define FREE_PARAM    /* enable define is manual input mode */

int do_socket(void*, void*);
int do_bind(void*, void*);
int do_connect(void*, void*);
int do_listen(void*, void*);
int do_accept(void*, void*);
int do_send(void*, void*);
int do_sendto(void*, void*);
int do_recv(void*, void*);
int do_recvfrom(void*, void*);
int do_select(void*, void*);
int do_shutdown(void*, void*);
int do_close(void*, void*);
int do_getsockopt(void*, void*);
int do_setsockopt(void*, void*);
int do_getsockname(void*, void*);
int do_getpeername(void*, void*);
int do_ioctl(void*, void*);
int do_getifaddrs(void*, void*);
int do_freeifaddrs(void*, void*);
int do_inet_pton(void*, void*);
int do_inet_ntop(void*, void*);
int do_netstat(void*, void*);


SOCKET_API_INFO socket_api_list[] = {
    /* socket API */
    { do_socket,    PRM_PROTO,                                        PRM_NONE, "socket"},
    { do_bind,      PRM_SOCFD|PRM_LADDR,                              PRM_NONE, "bind"},
    { do_connect,   PRM_SOCFD|PRM_RADDR,                              PRM_NONE, "connect"},
    { do_listen,    PRM_SOCFD|PRM_BACKLOG,                            PRM_NONE, "listen"},
    { do_accept,    PRM_SOCFD|PRM_RADDR|PRM_LEN_P,                    PRM_RADDR|PRM_LEN_P, "accept"},
    { do_send,      PRM_SOCFD|PRM_BUFF|PRM_FLAGS,                     PRM_NONE, "send"},
    { do_sendto,    PRM_SOCFD|PRM_BUFF|PRM_FLAGS|PRM_RADDR,           PRM_NONE, "sendto"},
    { do_recv,      PRM_SOCFD|PRM_BUFF|PRM_FLAGS,                     PRM_BUFF, "recv"},
    { do_recvfrom,  PRM_SOCFD|PRM_BUFF|PRM_FLAGS|PRM_RADDR|PRM_LEN_P, PRM_BUFF|PRM_RADDR|PRM_LEN_P, "recvfrom"},
    { do_select,    PRM_FDSET|PRM_TIME,                               PRM_FDSET,"select"},
    { do_shutdown,  PRM_SOCFD|PRM_HOW,                                PRM_NONE, "shutdown"},
    { do_close,     PRM_SOCFD,                                        PRM_NONE, "close"},
    { do_getsockopt,PRM_SOCFD|PRM_OPTION|PRM_LEN_P,                   PRM_OPTION|PRM_LEN_P,"getsockopt"},
    { do_setsockopt,PRM_SOCFD|PRM_OPTION,                             PRM_NONE, "setsockopt"},
    { do_getsockname,PRM_SOCFD|PRM_LADDR|PRM_LEN_P,                   PRM_LADDR|PRM_LEN_P,"getsockname"},
    { do_getpeername,PRM_SOCFD|PRM_RADDR|PRM_LEN_P,                   PRM_RADDR|PRM_LEN_P,"getpeername"},
    { do_ioctl,     PRM_SOCFD|PRM_IOCTL,                              PRM_NONE, "ioctl"},
    { do_getifaddrs,PRM_GETADDR,                                      PRM_GETADDR, "getifaddrs"},
    { do_freeifaddrs,PRM_FREEADDR,                                    PRM_NONE, "freeifaddrs"},
    { do_inet_pton, PRM_INETPTON,                                     PRM_INETPTON, "inet_pton"},
    { do_inet_ntop, PRM_INETNTOP,                                     PRM_INETNTOP, "inet_ntop"},
    /* contro command */
    { do_netstat,   PRM_NONE,                                         PRM_NONE, "netstat"},
    { NULL,         PRM_NONE,                                         PRM_NONE, "end"},
};


void add_spc(char *buf, int len)
{
    for (; *buf; ++buf) ;
    while (len--)   *(buf++) = ' ';
    *buf = '\0';
}


void output_errno(void *ctrl, int errno)
{
    VB ebuf[8];
    shell_puts(ctrl, net_itoa(errno, ebuf, 10));

#define CASERR_OUT(_e_)     case _e_:   shell_puts(ctrl, " - "#_e_"."); break;
    switch (errno) {
    CASERR_OUT(EPERM);          CASERR_OUT(ENOENT);         CASERR_OUT(ESRCH);          CASERR_OUT(EINTR);
    CASERR_OUT(EIO);            CASERR_OUT(ENXIO);          CASERR_OUT(E2BIG);          CASERR_OUT(ENOEXEC);
    CASERR_OUT(EBADF);          CASERR_OUT(ECHILD);         CASERR_OUT(EDEADLK);        CASERR_OUT(ENOMEM);
    CASERR_OUT(EACCES);         CASERR_OUT(EFAULT);         CASERR_OUT(ENOTBLK);        CASERR_OUT(EBUSY);
    CASERR_OUT(EEXIST);         CASERR_OUT(EXDEV);          CASERR_OUT(ENODEV);         CASERR_OUT(ENOTDIR);
    CASERR_OUT(EISDIR);         CASERR_OUT(EINVAL);         CASERR_OUT(ENFILE);         CASERR_OUT(EMFILE);
    CASERR_OUT(ENOTTY);         CASERR_OUT(ETXTBSY);        CASERR_OUT(EFBIG);          CASERR_OUT(ENOSPC);
    CASERR_OUT(ESPIPE);         CASERR_OUT(EROFS);          CASERR_OUT(EMLINK);         CASERR_OUT(EPIPE);
    CASERR_OUT(EDOM);           CASERR_OUT(ERANGE);         CASERR_OUT(EAGAIN);

    CASERR_OUT(EINPROGRESS);    CASERR_OUT(EALREADY);       CASERR_OUT(ENOTSOCK);       CASERR_OUT(EDESTADDRREQ);
    CASERR_OUT(EMSGSIZE);       CASERR_OUT(EPROTOTYPE);     CASERR_OUT(ENOPROTOOPT);    CASERR_OUT(EPROTONOSUPPORT);
    CASERR_OUT(ESOCKTNOSUPPORT);CASERR_OUT(EOPNOTSUPP);     CASERR_OUT(EPFNOSUPPORT);   CASERR_OUT(EAFNOSUPPORT);
    CASERR_OUT(EADDRINUSE);     CASERR_OUT(EADDRNOTAVAIL);  CASERR_OUT(ENETDOWN);       CASERR_OUT(ENETUNREACH);
    CASERR_OUT(ENETRESET);      CASERR_OUT(ECONNABORTED);   CASERR_OUT(ECONNRESET);     CASERR_OUT(ENOBUFS);
    CASERR_OUT(EISCONN);        CASERR_OUT(ENOTCONN);       CASERR_OUT(ESHUTDOWN);      CASERR_OUT(ETOOMANYREFS);
    CASERR_OUT(ETIMEDOUT);      CASERR_OUT(ECONNREFUSED);   CASERR_OUT(ENAMETOOLONG);   CASERR_OUT(EHOSTDOWN);
    CASERR_OUT(EHOSTUNREACH);   CASERR_OUT(ENOTEMPTY);      CASERR_OUT(EPROCLIM);       CASERR_OUT(EUSERS);
    CASERR_OUT(EDQUOT);         CASERR_OUT(ESTALE);         CASERR_OUT(EREMOTE);        CASERR_OUT(EBADRPC);
    CASERR_OUT(ERPCMISMATCH);   CASERR_OUT(EPROGUNAVAIL);   CASERR_OUT(EPROGMISMATCH);  CASERR_OUT(EPROCUNAVAIL);
    CASERR_OUT(ENOLCK);         CASERR_OUT(ENOSYS);         CASERR_OUT(EFTYPE);         CASERR_OUT(EAUTH);
    CASERR_OUT(ENEEDAUTH);      CASERR_OUT(ERESTARTa);      CASERR_OUT(EJUSTRETURN);

    default:
        shell_puts(ctrl, "Unregist Err."S_LINE);    break;
    }
}



/*****  BSD socket API *****/
/*
    int socket(int domain, int type, int protocol);
*/
int do_socket(void *ctrl, void *p)
{
    SOCKET_API_PARAM *sap = p;
    int res = 0;

    res = socket(sap->skt.domain, sap->skt.type, 0);
    if (-1 != res) {
        sock_use[res] = 1;
    }

    return res;
}
/*
    int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
*/
int do_bind(void *ctrl, void *p)
{
    SOCKET_API_PARAM *sap = p;
    int res = 0;

    res = bind(sap->socfd, sap->laddr_p, sap->llen);

    return res;
}
/*
    int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
*/
int do_connect(void *ctrl, void *p)
{
    SOCKET_API_PARAM *sap = p;
    int res = 0;

    res = connect(sap->socfd, sap->raddr_p, sap->rlen);

    return res;
}
/*
    int listen(int sockfd, int backlog);
*/
int do_listen(void *ctrl, void *p)
{
    SOCKET_API_PARAM *sap = p;
    int res = 0;

    res = listen(sap->socfd, sap->backlog);

    return res;
}
/*
    int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
*/
int do_accept(void *ctrl, void *p)
{
    SOCKET_API_PARAM *sap = p;
    int res = 0;

    res = accept(sap->socfd, sap->raddr_p, sap->rlen_p);
    if (-1 != res) {
        sock_use[res] = 1;
    }
    return res;
}
/*
    ssize_t send(int sockfd, const void *buf, size_t len, int flags);
*/
int do_send(void *ctrl, void *p)
{
    SOCKET_API_PARAM *sap = p;
    int res = 0;

    res = send(sap->socfd, sap->buffp, sap->buff, sap->flags);

    return res;
}
/*
    ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
*/
int do_sendto(void *ctrl, void *p)
{
    SOCKET_API_PARAM *sap = p;
    int res = 0;

    res = sendto(sap->socfd, sap->buffp, sap->buff, sap->flags, sap->raddr_p, sap->rlen);

    return res;
}
/*
    ssize_t recv(int sockfd, void *buf, size_t len, int flags);
*/
int do_recv(void *ctrl, void *p)
{
    SOCKET_API_PARAM *sap = p;
    int res = 0;

    sap->buff = recv(sap->socfd, sap->buffp, sap->buff, sap->flags);
    res = sap->buff;

    return res;
}
/*
    ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
*/
int do_recvfrom(void *ctrl, void *p)
{
    SOCKET_API_PARAM *sap = p;
    int res = 0;

    sap->buff = recvfrom(sap->socfd, sap->buffp, sap->buff, sap->flags, sap->raddr_p, sap->rlen_p);
    res = sap->buff;

    return res;
}
/*
    int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
*/
int do_select(void *ctrl, void *p)
{
    SOCKET_API_PARAM *sap = p;
    int res = 0;

    sap->fds_n = select(sap->max_fds,
            sap->nrfds ?  &sap->rfds : NULL,
            sap->nwfds ?  &sap->wfds : NULL,
            sap->nefds ?  &sap->efds : NULL,
            sap->to_ena ? &sap->time : NULL
    );
    res = sap->fds_n;

    return res;
}
/*
    int shutdown(int sockfd, int how);  
*/
int do_shutdown(void *ctrl, void *p)
{
    SOCKET_API_PARAM *sap = p;
    int res = 0;

    res = shutdown(sap->socfd, sap->how);

    return res;
}
/*
    int close(int fd);
*/
int do_close(void *ctrl, void *p)
{
    SOCKET_API_PARAM *sap = p;
    int res = 0;

    res = close(sap->socfd);
    sock_use[sap->socfd] = 0;

    return res;
}
/*
    int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
*/
int do_getsockopt(void *ctrl, void *p)
{
    SOCKET_API_PARAM *sap = p;
    int res = 0;

    res = getsockopt(sap->socfd, sap->opt.level, sap->opt.name, sap->opt.val, sap->olen_p);

    return res;
}
/*
    int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
*/
int do_setsockopt(void *ctrl, void *p)
{
    SOCKET_API_PARAM *sap = p;
    int res = 0;

    res = setsockopt(sap->socfd, sap->opt.level, sap->opt.name, sap->opt.val, sap->olen);

    return res;
}
/*
    int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
*/
int do_getsockname(void *ctrl, void *p)
{
    SOCKET_API_PARAM *sap = p;
    int res = 0;

    res = getsockname(sap->socfd, sap->laddr_p, sap->llen_p);

    return res;
}
/*
    int getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen);  
*/
int do_getpeername(void *ctrl, void *p)
{
    SOCKET_API_PARAM *sap = p;
    int res = 0;

    res = getpeername(sap->socfd, sap->raddr_p, sap->rlen_p);

    return res;
}
/*
    int ioctl(int d, int request, ...);  
*/
int do_ioctl(void *ctrl, void *p)
{
    SOCKET_API_PARAM *sap = p;
    int res = 0;

    res = ioctl(sap->socfd, sap->ioctlreq, sap->ioctlval_p);

    return res;
}
/*
    int getifaddrs(struct ifaddrs **ifap);
*/
int do_getifaddrs(void *ctrl, void *p)
{
    SOCKET_API_PARAM *sap = p;
    int res = 0;

    res = getifaddrs(sap->ifa_p);

    return res;
}
/*
    void freeifaddrs(struct ifaddrs *ifa);
*/
int do_freeifaddrs(void *ctrl, void *p)
{
    SOCKET_API_PARAM *sap = p;
    int res = 0;

    freeifaddrs(sap->ifa);

    return res;
}
/*
    int inet_pton(int af, const char *src, void *dst);
*/
int do_inet_pton(void *ctrl, void *p)
{
    SOCKET_API_PARAM *sap = p;
    int res = 0;

    res = inet_pton(sap->family, sap->ipstr_p, sap->ipval_p);

    return res;
}
/*
    const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);
*/
int do_inet_ntop(void *ctrl, void *p)
{
    SOCKET_API_PARAM *sap = p;
    const char *pres = 0;

    pres = inet_ntop(sap->family, sap->ipval_p, sap->ipstr_p, sap->llen);

    return (int)(ADDR)pres;
}
int do_netstat(void *ctrl, void *p)
{
    const int len_sfd = sizeof("65536   ") - 1;
    const int len_prt = sizeof("UDPL   ") - 1;
    const int len_ipa = sizeof("255.255.255.255:65536  ") - 1;

    int res = 0;
    int nx, idx;
    int val;
    char *tmp, *item;
    SOCKET_API_PARAM *sap = p;
    struct sockaddr_in6 adr_in6;
    struct sockaddr *sai;
    socklen_t slen;

    net_strcpy(CmdBuf, "sockfd  proto  local-addr             remote-addr            listen" S_LINE);
    shell_puts(ctrl, CmdBuf);

    for (nx = 0; nx < sizeof(sock_use)/sizeof(sock_use[0]); ++nx) {
        if (!sock_use[nx])  continue;
        CmdBuf[0] = '\0';

        /* sockfd */
        item = &CmdBuf[net_strlen(CmdBuf)];
        net_itoa(nx, CmdBuf, 10);
        add_spc(item, len_sfd - net_strlen(item));

        /* Proto */
        item = &CmdBuf[net_strlen(CmdBuf)];
        slen = sizeof(val);
        getsockopt(nx, SOL_SOCKET, SO_TYPE, &val, &slen);
        switch (val) {
          case SOCK_STREAM:     net_strcat(CmdBuf, "TCP");   break;
          case SOCK_DGRAM:      net_strcat(CmdBuf, "UDP");   break;
          case SOCK_RAW:        net_strcat(CmdBuf, "RAW");  break;
          default:
            net_itoa(val, item, 10);
            break;
        }
        add_spc(item, len_prt - net_strlen(item));

        /* Local-Addr */
        item = &CmdBuf[net_strlen(CmdBuf)];
        slen = sizeof(adr_in6);
        sai = (struct sockaddr*)&adr_in6;
        res = getsockname(nx, sai, &slen);
        if (0 == res) {
            if (sai->sa_family != AF_INET6) {
                tmp = inet_ntoa(((struct sockaddr_in*)sai)->sin_addr);
                net_strcat(CmdBuf, tmp);
                net_strcat(CmdBuf, ":");
                idx = net_strlen(CmdBuf);
                net_itoa(ntohs(((struct sockaddr_in*)sai)->sin_port), &CmdBuf[idx], 10);
            } else {
                net_memset(sap->ipstr, 0, sizeof(sap->ipstr));
                inet_ntop(AF_INET6, &((struct sockaddr_in6*)sai)->sin6_addr, sap->ipstr, sizeof(sap->ipstr));
                /* The maximum length of the ipv6 string is 16byte. */
                sap->ipstr[16] = 0;
                net_strcat(CmdBuf, sap->ipstr);
                net_strcat(CmdBuf, ":");
                idx = net_strlen(CmdBuf);
                net_itoa(ntohs(((struct sockaddr_in6*)sai)->sin6_port), &CmdBuf[idx], 10);
            }
        }
        else {
            net_strcat(CmdBuf, "0");
        }
        if (len_ipa > net_strlen(item)) {
            add_spc(item, len_ipa - net_strlen(item));
        } else {
            add_spc(item, 1);
        }

        /* Remote-Addr */
        item = &CmdBuf[net_strlen(CmdBuf)];
        slen = sizeof(adr_in6);
        sai = (struct sockaddr*)&adr_in6;
        res = getpeername(nx, sai, &slen);
        if (0 == res) {
            if (sai->sa_family != AF_INET6) {
                tmp = inet_ntoa(((struct sockaddr_in*)sai)->sin_addr);
                net_strcat(CmdBuf, tmp);
                net_strcat(CmdBuf, ":");
                idx = net_strlen(CmdBuf);
                net_itoa(ntohs(((struct sockaddr_in*)sai)->sin_port), &CmdBuf[idx], 10);
            } else {
                net_memset(sap->ipstr, 0, sizeof(sap->ipstr));
                inet_ntop(AF_INET6, &((struct sockaddr_in6*)sai)->sin6_addr, sap->ipstr, sizeof(sap->ipstr));
                /* The maximum length of the ipv6 string is 16byte. */
                sap->ipstr[16] = 0;
                net_strcat(CmdBuf, sap->ipstr);
                net_strcat(CmdBuf, ":");
                idx = net_strlen(CmdBuf);
                net_itoa(ntohs(((struct sockaddr_in6*)sai)->sin6_port), &CmdBuf[idx], 10);
            }
        }
        else {
            net_strcat(CmdBuf, "0");
        }
        if (len_ipa > net_strlen(item)) {
            add_spc(item, len_ipa - net_strlen(item));
        } else {
            add_spc(item, 1);
        }

        /* TCP-Status */
        slen = sizeof(val);
        getsockopt(nx, SOL_SOCKET, SO_ACCEPTCONN, &val, &slen);
        net_strcat(CmdBuf, (0 != val) ? "o" : "x");

        net_strcat(CmdBuf, S_LINE);
        shell_puts(ctrl, CmdBuf);
    }

    return 0;
}


typedef struct t_eval_api {
    char flag;
    int  val;
    char *api;
} T_EVAL_API;

/* if res is err value, return 1; otherwise return 0 */
int is_error(SOCKET_API_INFO *api, int res)
{
    T_EVAL_API ea[] = {
        {1, -1, "socket"},
        {0, 0, "bind"},
        {0, 0, "connect"},
        {0, 0, "listen"},
        {1, -1, "accept"},
        {1, -1, "send"},
        {1, -1, "sendto"},
        {1, -1, "recv"},
        {1, -1, "recvfrom"},
        {1, -1, "select"},
        {0, 0, "shutdown"},
        {0, 0, "close"},
        {0, 0, "getsockopt"},
        {0, 0, "setsockopt"},
        {0, 0, "getsockname"},
        {0, 0, "getpeername"},
        {1, -1, "ioctl"},
        {0, 0, "getifaddrs"},
        {0, 0, "freeifaddrs"},
        {1, -1, "inet_pton"},
        {1, 0, "inet_ntop"},
        {0, 0, "netstat"},
        {0, 0, "?"},
        {0, 0, "end"},
    };
    int nx;
    int ret;

    for (nx = 0; nx < sizeof(ea)/sizeof(ea[0]); ++nx) {
        if (0 == net_strcmp(ea[nx].api, api->api_name)) {
            break;
        }
    }
    if (nx == sizeof(ea)/sizeof(ea[0])) {
        return 1;
    }

    if (0 == ea[nx].flag) {     // val is normal end
        ret = (ea[nx].val == res) ? 0 : 1 ;
    }
    else {                      // val is error end
        ret = (ea[nx].val == res) ? 1 : 0 ;
    }
    return ret;
}

int ip_core(void *ctrl, const char *s, int rdmode)
{
    int ret;

    shell_puts(ctrl, s);
    shell_puts(ctrl, ": ");
    shell_gets(ctrl, CmdBuf, MAX_CMD_LEN);
    shell_puts(ctrl, S_LINE);

    switch (rdmode) {
    case 1: /* ipaddr */
        if (0 == net_atoi(CmdBuf)) {
            ret = INADDR_ANY;
        }
        else {
            ret = inet_addr(CmdBuf);
        }
        break;

    default:
        ret = net_atoi(CmdBuf);
        break;
    }

    return ret;
}

int ipv6_core(void *ctrl, const char *s, in6_addr *ipv6addr)
{
    int ret;

    shell_puts(ctrl, s);
    shell_puts(ctrl, ": ");
    shell_gets(ctrl, CmdBuf, MAX_CMD_LEN);
    shell_puts(ctrl, S_LINE);
    ret = inet_pton(AF_INET6, CmdBuf, ipv6addr);

    return ret;
}

unsigned long todec(const char str[])
{
    short i = 0;
    short n;
    unsigned int x = 0;

    while (str[i] != '\0') {
        if (('0' <= str[i]) && (str[i] <= '9')) {
            n = str[i] - '0';
        } else if (('a' <= str[i]) && (str[i] <= 'f')) {
            n = str[i] - 'a' + 10;
        } else if (('A' <= str[i]) && (str[i] <= 'F')) {
            n = str[i] - 'A' + 10;
        } else {
            return (0);
        }
        i++;
        x = x *16 + n;
    }
    return (x);
}

int input_param(void *ctrl, SOCKET_API_INFO *api, SOCKET_API_PARAM *param)
{
    const int ip = api->in_param;
    int nx, ny, ret;
    int socfd;
    char buf[128];
    int max;
    char sa_family;
    socklen_t defllen;
#define MAX_VAL(a,b)    ((a > b) ? a : b)

    shell_puts(ctrl, S_LINE);
    ret = 0;
    if (ip == PRM_NONE) return ret;

    if (ip & PRM_SOCFD)     param->socfd    = ip_core(ctrl, "sockfd ", 0);
    if (ip & PRM_FDSET) {
        max = 0;
        param->nrfds = ip_core(ctrl, "- set number of Read fdset ", 0);
        FD_ZERO(&param->rfds);
        for (nx = param->nrfds; nx; --nx) {
            socfd = ip_core(ctrl, "    r-sockfd ", 0);
            FD_SET(socfd, &param->rfds);
            max = MAX_VAL(max, socfd);
        }
        param->nwfds = ip_core(ctrl, "- set number of Write fdset ", 0);
        FD_ZERO(&param->wfds);
        for (nx = param->nwfds; nx; --nx) {
            socfd = ip_core(ctrl, "    w-sockfd ", 0);
            FD_SET(socfd, &param->wfds);
            max = MAX_VAL(max, socfd);
        }
        param->nefds = 0;   /* uNet3/BSD not supported Exception fdset */

#ifdef FREE_PARAM
        param->max_fds = ip_core(ctrl, "- manual input of maxFD+1 enable? (0:no, >0:yes) ", 0);
        if (param->max_fds) {
            param->max_fds = ip_core(ctrl, "  set maxFD+1   ", 0);
        }
        else {
            param->max_fds = max + 1;
        }
#else
        param->max_fds = max + 1;
#endif
    }
    if (ip & PRM_PROTO) {
#ifdef FREE_PARAM
        net_strcpy(buf, "- domain? (");
        nx = net_strlen(buf);   net_itoa(AF_INET, &buf[nx], 10);
        net_strcat(buf, ":AF_INET, ");
        nx = net_strlen(buf);   net_itoa(AF_INET6, &buf[nx], 10);
        net_strcat(buf, ":AF_INET6) ");
        param->skt.domain = ip_core(ctrl, buf , 0);
#else
        param->skt.domain = AF_INET;
#endif

        net_strcpy(buf, "- type? (");
        nx = net_strlen(buf);   net_itoa(SOCK_STREAM, &buf[nx], 10);
        net_strcat(buf, ":SOCK_STREAM, ");
        nx = net_strlen(buf);   net_itoa(SOCK_DGRAM, &buf[nx], 10);
        net_strcat(buf, ":SOCK_DGRAM, ");
        nx = net_strlen(buf);   net_itoa(SOCK_RAW, &buf[nx], 10);
        net_strcat(buf, ":SOCK_RAW) ");
        param->skt.type = ip_core(ctrl, buf, 0);
    }
    if (ip & PRM_LADDR) {
        net_memset(&param->laddr, 0, sizeof(param->laddr));
        net_memset(&param->laddr6, 0, sizeof(param->laddr6));
        param->llen = 0;

        shell_puts(ctrl, "- local-addr." S_LINE);
#ifdef FREE_PARAM
        param->laddr_p = ip_core(ctrl, " enable input addres? ", 0) ? (struct sockaddr*)&param->laddr : NULL;
#else
        param->laddr_p = (struct sockaddr*)&param->laddr;
#endif
        defllen = sizeof(struct sockaddr_in);
        if (param->laddr_p) {
#ifdef FREE_PARAM
            net_strcpy(buf, "    addr_family(");
            nx = net_strlen(buf);   net_itoa(AF_INET, &buf[nx], 10);
            net_strcat(buf, ":AF_INET, ");
            nx = net_strlen(buf);   net_itoa(AF_INET6, &buf[nx], 10);
            net_strcat(buf, ":AF_INET6) ");
            sa_family = ip_core(ctrl, buf, 0);
#else
            sa_family = AF_INET;
#endif
            if (sa_family != AF_INET6) {
                param->laddr.sin_family      = sa_family;
                param->laddr.sin_addr.s_addr = ip_core(ctrl, "    ip (ex: 192.168.1.1) ", 1);
                param->laddr.sin_port        = htons(ip_core(ctrl, "    port                 ", 0));
                net_memset(&param->laddr.sin_zero, 0, sizeof(param->laddr.sin_zero));
            }
            else {
                defllen = sizeof(struct sockaddr_in6);
                param->laddr_p = (struct sockaddr*)&param->laddr6;

                param->laddr6.sin6_family = sa_family;
                ipv6_core(ctrl, "    ipv6 (ex: fe80:::fecf:ffa8) ", &param->laddr6.sin6_addr);
                param->laddr6.sin6_port = htons(ip_core(ctrl, "    port ", 0));
                param->laddr6.sin6_scope_id = ip_core(ctrl, "    scope_id ", 0);
            }
        }

        if (ip & PRM_LEN_P) {
#ifdef FREE_PARAM
            param->llen_p = ip_core(ctrl, " valid laddr_len? ", 0) ? &param->llen : NULL;
#else
            param->llen_p = &param->llen;
#endif
        }
        else {
            param->llen_p = &param->llen;
        }
        if (param->llen_p) {
#ifdef FREE_PARAM
            net_strcpy(buf, "    laddr_len(");
            nx = net_strlen(buf);   net_itoa(defllen, &buf[nx], 10);
            net_strcat(buf, ") ");
            param->llen = ip_core(ctrl, buf, 0);
#else
            param->llen = sizeof(param->laddr);
#endif
        }
    }
    if (ip & PRM_RADDR) {
        net_memset(&param->raddr, 0, sizeof(param->raddr));
        net_memset(&param->raddr6, 0, sizeof(param->raddr6));
        param->rlen = 0;

        shell_puts(ctrl, "- remote-addr." S_LINE);
#ifdef FREE_PARAM
        param->raddr_p = ip_core(ctrl, " enable input addres? ", 0) ? (struct sockaddr*)&param->raddr : NULL;
#else
        param->raddr_p = (struct sockaddr*)&param->raddr;
#endif
        defllen = sizeof(struct sockaddr_in);
        if (param->raddr_p) {
#ifdef FREE_PARAM
            net_strcpy(buf, "    addr_family(");
            nx = net_strlen(buf);   net_itoa(AF_INET, &buf[nx], 10);
            net_strcat(buf, ":AF_INET, ");
            nx = net_strlen(buf);   net_itoa(AF_INET6, &buf[nx], 10);
            net_strcat(buf, ":AF_INET6) ");
            sa_family = ip_core(ctrl, buf, 0);
#else
            sa_family = AF_INET;
#endif
            if (sa_family != AF_INET6) {
                param->raddr.sin_family      = sa_family;
                param->raddr.sin_addr.s_addr = ip_core(ctrl, "    ip (ex: 192.168.1.1) ", 1);
                param->raddr.sin_port        = htons(ip_core(ctrl, "    port                 ", 0));
                net_memset(&param->raddr.sin_zero, 0, sizeof(param->raddr.sin_zero));
            }
            else {
                defllen = sizeof(struct sockaddr_in6);
                param->raddr_p = (struct sockaddr*)&param->raddr6;

                param->raddr6.sin6_family = sa_family;
                ipv6_core(ctrl, "    ipv6 (ex: fe80:::fecf:ffa8) ", &param->raddr6.sin6_addr);
                param->raddr6.sin6_port = htons(ip_core(ctrl, "    port ", 0));
                param->raddr6.sin6_scope_id = ip_core(ctrl, "    scope_id ", 0);
            }
        }
        if (ip & PRM_LEN_P) {
#ifdef FREE_PARAM
            param->rlen_p = ip_core(ctrl, " valid raddr_len? ", 0) ? &param->rlen : NULL;
#else
            param->rlen_p = &param->rlen;
#endif
        }
        else {
            param->rlen_p = &param->rlen;
        }
        if (param->rlen_p) {
#ifdef FREE_PARAM
            net_strcpy(buf, "    raddr_len(");
            nx = net_strlen(buf);   net_itoa(defllen, &buf[nx], 10);
            net_strcat(buf, ") ");
            param->rlen = ip_core(ctrl, buf, 0);
#else
            param->rlen = sizeof(param->raddr);
#endif
        }
    }
    if (ip & PRM_BUFF) {
        net_itoa(MAX_CMD_LEN, CmdBuf, 10);
        shell_puts(ctrl, "- buffer size(int, MAX=");
        shell_puts(ctrl, CmdBuf);
        param->buff     = ip_core(ctrl, ") ", 0);

        if (MAX_CMD_LEN < param->buff) {
            ret = -1;
        }
#ifdef FREE_PARAM
        param->buffp = ip_core(ctrl, " valid buffer? ", 0) ? CmdBuf : NULL;
#else
        param->buffp = CmdBuf;
#endif
    }
    if (ip & PRM_FLAGS) {
        net_strcpy(buf, "- flags(bit, 0x");
        nx = net_strlen(buf);   net_itoa(MSG_DONTWAIT, &buf[nx], 16);
        net_strcat(buf, ":MSG_DONTWAIT, 0x");
        nx = net_strlen(buf);   net_itoa(MSG_PEEK, &buf[nx], 16);
        net_strcat(buf, ":MSG_PEEK) ");
        param->flags = ip_core(ctrl, buf, 0);
    }
    if (ip & PRM_TIME) {
        param->to_ena = ip_core(ctrl, "- timeout enable? (0:no, >0:yes) ", 0);
        if (param->to_ena) {
            param->time.tv_sec          = ip_core(ctrl, "  seconds       ", 0);
            param->time.tv_usec         = ip_core(ctrl, "  micro seconds ", 0);
        }
    }
    if (ip & PRM_HOW) {
        net_strcpy(buf, "- how? (");
        nx = net_strlen(buf);   net_itoa(SHUT_RD, &buf[nx], 10);
        net_strcat(buf, ":SHUT_RD, ");
        nx = net_strlen(buf);   net_itoa(SHUT_WR, &buf[nx], 10);
        net_strcat(buf, ":SHUT_WR, ");
        nx = net_strlen(buf);   net_itoa(SHUT_RDWR, &buf[nx], 10);
        net_strcat(buf, ":SHUT_RDWR) ");
        param->how =ip_core(ctrl, buf, 0);
    }

    if (ip & PRM_IOCTL) {
        net_strcpy(buf, "- ioctl request? (");
        nx = net_strlen(buf);   net_itoa(FIONBIO, &buf[nx], 10);
        net_strcat(buf, ":FIONBIO) ");
        param->ioctlreq = ip_core(ctrl, buf, 0);
#ifdef FREE_PARAM
        param->ioctlval_p = ip_core(ctrl, " valid ioctl_val? ", 0) ? &param->ioctlval : NULL;
#else
        param->ioctlval_p = &param->ioctlval;
#endif
        if (param->ioctlval_p) {
            param->ioctlval = ip_core(ctrl, "    ioctl_val (int)", 0);
        }
    }

    if (ip & PRM_OPTION) {
        net_strcpy(buf, "- option_level? (");
        nx = net_strlen(buf);   net_itoa(SOL_SOCKET, &buf[nx], 10);
        net_strcat(buf, ":SOL_SOCKET, ");
        nx = net_strlen(buf);   net_itoa(IPPROTO_IP, &buf[nx], 10);
        net_strcat(buf, ":IPPROTO_IP, ");
        nx = net_strlen(buf);   net_itoa(IPPROTO_TCP, &buf[nx], 10);
        net_strcat(buf, ":IPPROTO_TCP, ");
        nx = net_strlen(buf);   net_itoa(IPPROTO_IPV6, &buf[nx], 10);
        net_strcat(buf, ":IPPROTO_IPV6) " S_LINE);
        param->opt.level = ip_core(ctrl, buf, 0);

        switch (param->opt.level) {
        case SOL_SOCKET:
            net_strcpy(buf, "- SOL_SOCKET option_name? (");
            nx = net_strlen(buf);   net_itoa(SO_ACCEPTCONN, &buf[nx], 10);
            net_strcat(buf, ":SO_ACCEPTCONN, ");
            nx = net_strlen(buf);   net_itoa(SO_BROADCAST, &buf[nx], 10);
            net_strcat(buf, ":SO_BROADCAST, " S_LINE);
            shell_puts(ctrl, buf);

            net_itoa(SO_DOMAIN, buf, 10);
            net_strcat(buf, ":SO_DOMAIN, ");
            nx = net_strlen(buf);   net_itoa(SO_ERROR, &buf[nx], 10);
            net_strcat(buf, ":SO_ERROR, ");
            nx = net_strlen(buf);   net_itoa(SO_KEEPALIVE, &buf[nx], 10);
            net_strcat(buf, ":SO_KEEPALIVE, ");
            nx = net_strlen(buf);   net_itoa(SO_RCVBUF, &buf[nx], 10);
            net_strcat(buf, ":SO_RCVBUF, ");
            nx = net_strlen(buf);   net_itoa(SO_RCVBUFFORCE, &buf[nx], 10);
            net_strcat(buf, ":SO_RCVBUFFORCE, " S_LINE);
            shell_puts(ctrl, buf);

            net_itoa(SO_RCVTIMEO, buf, 10);
            net_strcat(buf, ":SO_RCVTIMEO, ");
            nx = net_strlen(buf);   net_itoa(SO_SNDTIMEO, &buf[nx], 10);
            net_strcat(buf, ":SO_SNDTIMEO, ");
            nx = net_strlen(buf);   net_itoa(SO_TYPE, &buf[nx], 10);
            net_strcat(buf, ":SO_TYPE, ");
            nx = net_strlen(buf);   net_itoa(SO_REUSEADDR, &buf[nx], 10);
            net_strcat(buf, ":SO_REUSEADDR) ");
            nx =ip_core(ctrl, buf, 0);

            param->opt.name = nx;
            param->opt.type = SOT_INT;
            switch (nx) {
            case SO_RCVTIMEO:   param->opt.type = SOT_TIME;     break;
            case SO_SNDTIMEO:   param->opt.type = SOT_TIME;     break;
            }
            break;

        case IPPROTO_IP:
            net_strcpy(buf, "- IPPROTO_IP option_name? (");
            nx = net_strlen(buf);   net_itoa(IP_ADD_MEMBERSHIP, &buf[nx], 10);
            net_strcat(buf, ":IP_ADD_MEMBERSHIP, ");
            nx = net_strlen(buf);   net_itoa(IP_DROP_MEMBERSHIP, &buf[nx], 10);
            net_strcat(buf, ":IP_DROP_MEMBERSHIP, " S_LINE);
            shell_puts(ctrl, buf);

            net_itoa(IP_HDRINCL, buf, 10);
            net_strcat(buf, ":IP_HDRINCL, ");
            nx = net_strlen(buf);   net_itoa(IP_MTU, &buf[nx], 10);
            net_strcat(buf, ":IP_MTU, ");
            nx = net_strlen(buf);   net_itoa(IP_MULTICAST_TTL, &buf[nx], 10);
            net_strcat(buf, ":IP_MULTICAST_TTL, ");
            nx = net_strlen(buf);   net_itoa(IP_PKTINFO, &buf[nx], 10);
            net_strcat(buf, ":IP_PKTINFO, ");
            nx = net_strlen(buf);   net_itoa(IP_TOS, &buf[nx], 10);
            net_strcat(buf, ":IP_TOS, " S_LINE);
            shell_puts(ctrl, buf);

            net_itoa(IP_TTL, buf, 10);
            net_strcat(buf, ":IP_TTL) ");
            nx = ip_core(ctrl, buf, 0);

            param->opt.name = nx;
            param->opt.type = SOT_INT;
            switch (nx) {
            case IP_ADD_MEMBERSHIP:     param->opt.type = SOT_IP_MREQN; break;
            case IP_DROP_MEMBERSHIP:    param->opt.type = SOT_IP_MREQN; break;
            case IP_PKTINFO:            param->opt.type = SOT_IP_PKNFO; break;
            }
            break;

        case IPPROTO_TCP:
            net_strcpy(buf, "- IPPROTO_TCP option_name? (");
            nx = net_strlen(buf);   net_itoa(TCP_KEEPCNT, &buf[nx], 10);
            net_strcat(buf, ":TCP_KEEPCNT, ");
            nx = net_strlen(buf);   net_itoa(TCP_KEEPIDLE, &buf[nx], 10);
            net_strcat(buf, ":TCP_KEEPIDLE, " S_LINE);
            shell_puts(ctrl, buf);

            net_itoa(TCP_KEEPINTVL, buf, 10);
            net_strcat(buf, ":TCP_KEEPINTVL, ");
            nx = net_strlen(buf);   net_itoa(TCP_MAXSEG, &buf[nx], 10);
            net_strcat(buf, ":TCP_MAXSEG) ");
            nx = ip_core(ctrl, buf, 0);

            param->opt.name = nx;
            param->opt.type = SOT_INT;
            break;

        case IPPROTO_IPV6:
            net_strcpy(buf, "- IPPROTO_IPV6 option_name? (");
            nx = net_strlen(buf);   net_itoa(IPV6_ADD_MEMBERSHIP, &buf[nx], 10);
            net_strcat(buf, ":IPV6_ADD_MEMBERSHIP, ");
            nx = net_strlen(buf);   net_itoa(IPV6_DROP_MEMBERSHIP, &buf[nx], 10);
            net_strcat(buf, ":IPV6_DROP_MEMBERSHIP) ");
            nx = ip_core(ctrl, buf, 0);

            param->opt.name = nx;
            param->opt.type = SOT_INT;
            switch (nx) {
            case IPV6_ADD_MEMBERSHIP:   param->opt.type = SOT_IPV6_MREQ; break;
            case IPV6_DROP_MEMBERSHIP:  param->opt.type = SOT_IPV6_MREQ; break;
            }
            break;

        default:
            param->opt.name = ip_core(ctrl, "- Unknown option_name? (int)", 0);
            param->opt.type = SOT_INT;
            break;
        }

        if (0 == ret) {
#ifdef FREE_PARAM
            nx = ip_core(ctrl, " valid optval? ", 0);
#else
            nx = 1;
#endif
            if (nx) {
                if (0 == (api->out_param & PRM_OPTION)) {   // setparam
                    switch (param->opt.type) {
                    case SOT_INT:
                        param->opt.ival = ip_core(ctrl, "  set_val(int) ", 0);
                        break;
                    case SOT_TIME:
                        shell_puts(ctrl, "  set_val(time) " S_LINE);
                        param->opt.time.tv_sec      = ip_core(ctrl, "  seconds       ", 0);
                        param->opt.time.tv_usec     = ip_core(ctrl, "  micro seconds ", 0);
                        break;
                    case SOT_LINGER:
                        shell_puts(ctrl, "  set_val(linger) " S_LINE);
                        param->opt.ling.l_onoff     = ip_core(ctrl, "  l_onoff  ", 0);
                        param->opt.ling.l_linger    = ip_core(ctrl, "  l_linger ", 0);
                        break;

                    case SOT_IP_MREQN:
                        shell_puts(ctrl, "  set_val(ip_mreqn) " S_LINE);
                        param->opt.imreqn.imr_multiaddr.s_addr
                            = ip_core(ctrl, "  multicast-group-addr (ex: 192.168.1.1) ", 1);
                        param->opt.imreqn.imr_address.s_addr
                            = ip_core(ctrl, "  target ip-addr (ex: 192.168.1.1)       ", 1);
                        param->opt.imreqn.imr_ifindex
                            = ip_core(ctrl, "  interface index(int)                   ", 0);
                        break;

                    case SOT_IP_PKNFO:
                        shell_puts(ctrl, "  set_val(in_pktinfo) " S_LINE);
                        param->opt.ipktnfo.ipi_ifindex
                            = ip_core(ctrl, "  interface index(int)                   ", 0);
                        param->opt.ipktnfo.ipi_spec_dst.s_addr
                            = ip_core(ctrl, "  local ip-addr (ex: 192.168.1.1)        ", 1);
                        param->opt.ipktnfo.ipi_addr.s_addr
                            = ip_core(ctrl, "  header dst-addr (ex: 192.168.1.1)      ", 1);
                        break;

                    case SOT_IPV6_MREQ:
                        shell_puts(ctrl, "  set_val(ipv6_mreq) " S_LINE);
                        ipv6_core(ctrl, "  multicast-group-addr (ex: FF02::1001) ",
                            &param->opt.v6mreq.ipv6mr_multiaddr);
                        param->opt.v6mreq.ipv6mr_interface
                            = ip_core(ctrl, "  interface index(unsigned int)          ", 0);
                        break;
                    }
                }

                switch (param->opt.type) {
                case SOT_INT:       param->opt.val = &param->opt.ival;      break;
                case SOT_TIME:      param->opt.val = &param->opt.time;      break;
                case SOT_LINGER:    param->opt.val = &param->opt.ling;      break;
                case SOT_IP_MREQN:  param->opt.val = &param->opt.imreqn;    break;
                case SOT_IP_PKNFO:  param->opt.val = &param->opt.ipktnfo;   break;
                case SOT_IPV6_MREQ: param->opt.val = &param->opt.v6mreq;    break;
                }
            }
            else {
                param->opt.val = NULL;
            }

            if (ip & PRM_LEN_P) {
#ifdef FREE_PARAM
                param->olen_p = ip_core(ctrl, " valid optlen? ", 0) ? &param->olen : NULL;
#else
                param->olen_p = &param->olen;
#endif
            }
            else {
                param->olen_p = &param->olen;
            }
            ny = 0;
            if (param->olen_p) {
                switch (param->opt.type) {
                case SOT_INT:       ny = sizeof(int);              break;
                case SOT_TIME:      ny = sizeof(struct timeval);   break;
                case SOT_LINGER:    ny = sizeof(struct linger);    break;
                case SOT_IP_MREQN:  ny = sizeof(struct ip_mreqn);  break;
                case SOT_IP_PKNFO:  ny = sizeof(struct in_pktinfo);break;
                case SOT_IPV6_MREQ:  ny = sizeof(struct ipv6_mreq);break;
                }
#ifdef FREE_PARAM
                net_strcpy(buf, "  opt_len (");
                nx = net_strlen(buf);
                net_itoa(ny, &buf[nx], 10);
                net_strcat(buf, ") ");
                param->olen = ip_core(ctrl, buf, 0);
#else
                param->olen = ny;
#endif
            }
        }
    }
    if (ip & PRM_BACKLOG)   param->backlog  = ip_core(ctrl, "- backlog(int) ", 0);

    if (ip & PRM_BLKNUM) {
        param->blknum  = ip_core(ctrl, "- target socket num(int) ", 0);
    }

    if (ip & PRM_GETADDR) {
        shell_puts(ctrl, "- ifap-addr." S_LINE);
        param->ifa_p = ip_core(ctrl, " enable input addres? ", 0) ? &param->ifa : NULL;
    }

    if (ip & PRM_FREEADDR) {
        shell_puts(ctrl, "- ifa-addr." S_LINE);
        param->ifa = ip_core(ctrl, " enable input addres? ", 0) ? (void*)1 : NULL;
        if (param->ifa != NULL) {
            shell_puts(ctrl, "    ifa (ex: 0x8711C800) : 0x");
            shell_gets(ctrl, CmdBuf, MAX_CMD_LEN);
            shell_puts(ctrl, S_LINE);
            param->ifa = (void*)todec(CmdBuf);
        }
    }

    if ((ip & PRM_INETPTON) || (ip & PRM_INETNTOP)) {
        net_strcpy(buf, "- addr_family? (");
        nx = net_strlen(buf);   net_itoa(AF_INET, &buf[nx], 10);
        net_strcat(buf, ":AF_INET, ");
        nx = net_strlen(buf);   net_itoa(AF_INET6, &buf[nx], 10);
        net_strcat(buf, ":AF_INET6) ");
        param->family = ip_core(ctrl, buf , 0);

        net_memset(param->ipstr, 0, sizeof(param->ipstr));
        net_memset(param->ipval, 0, sizeof(param->ipval));

        if (ip & PRM_INETPTON) {
            shell_puts(ctrl, "- src-addr." S_LINE);
            param->ipstr_p = ip_core(ctrl, " enable input addres? ", 0) ? param->ipstr : NULL;
            if (param->ipstr_p != NULL) {
                if (param->family != AF_INET6) {
                    shell_puts(ctrl, "    ip (ex: 192.168.1.1) : ");
                    shell_gets(ctrl, param->ipstr, MAX_CMD_LEN);
                    shell_puts(ctrl, S_LINE);
                } else {
                    shell_puts(ctrl, "    ipv6 (ex: fe80:::fecf:ffa8) : ");
                    shell_gets(ctrl, param->ipstr, MAX_CMD_LEN);
                    shell_puts(ctrl, S_LINE);
                }
            }

            shell_puts(ctrl, "- dst-addr." S_LINE);
            param->ipval_p = ip_core(ctrl, " enable input addres? ", 0) ? &param->ipval : NULL;
        } else {
            shell_puts(ctrl, "- src-addr." S_LINE);
            param->ipval_p = ip_core(ctrl, " enable input addres? ", 0) ? &param->ipval : NULL;
            if (param->ipval_p != NULL) {
                if (param->family != AF_INET6) {
                    shell_puts(ctrl, "    (ex: 0x000000000)" S_LINE);
                    shell_puts(ctrl, "    s_addr : 0x");
                    shell_gets(ctrl, CmdBuf, MAX_CMD_LEN);
                    shell_puts(ctrl, S_LINE);
                    param->ipval[0] = todec(CmdBuf);
                } else {
                    shell_puts(ctrl, "    (ex: 0x00000000)" S_LINE);
                    shell_puts(ctrl, "    s6_addr32[0] : 0x");
                    shell_gets(ctrl, CmdBuf, MAX_CMD_LEN);
                    shell_puts(ctrl, S_LINE);
                    param->ipval[0] = todec(CmdBuf);

                    shell_puts(ctrl, "    s6_addr32[1] : 0x");
                    shell_gets(ctrl, CmdBuf, MAX_CMD_LEN);
                    shell_puts(ctrl, S_LINE);
                    param->ipval[1] = todec(CmdBuf);

                    shell_puts(ctrl, "    s6_addr32[2] : 0x");
                    shell_gets(ctrl, CmdBuf, MAX_CMD_LEN);
                    shell_puts(ctrl, S_LINE);
                    param->ipval[2] = todec(CmdBuf);

                    shell_puts(ctrl, "    s6_addr32[3] : 0x");
                    shell_gets(ctrl, CmdBuf, MAX_CMD_LEN);
                    shell_puts(ctrl, S_LINE);
                    param->ipval[3] = todec(CmdBuf);
                }
            }

            shell_puts(ctrl, "- dst-addr." S_LINE);
            param->ipstr_p = ip_core(ctrl, " enable input addres? ", 0) ? param->ipstr : NULL;

            if (param->family != AF_INET6) {
                net_strcpy(buf, "- size(16)");
            } else {
                net_strcpy(buf, "- size(46)");
            }
            param->llen = ip_core(ctrl, buf, 0);
        }
    }

    return ret;
}

const char *get_optlevel(int level)
{
    const char *opt[] = {
        "SOL_SOCKET", "IPPROTO_IP", "IPPROTO_TCP", "IPPROTO_IPV6"
    };
    int nx;

    nx = -1;
    switch (level) {
    case SOL_SOCKET:    nx = 0;     break;
    case IPPROTO_IP:    nx = 1;     break;
    case IPPROTO_TCP:   nx = 2;     break;
    case IPPROTO_IPV6:  nx = 3;     break;
    }

    return (0 <= nx) ? opt[nx] : "Unknown_Level" ;
}
const char *get_optname(int level, int name)
{
    const char *opt_sock[] = {
        "SO_ACCEPTCONN", "SO_BROADCAST",
        "SO_DOMAIN", "SO_ERROR", "SO_KEEPALIVE", "SO_RCVBUF", "SO_RCVBUFFORCE",
        "SO_RCVTIMEO", "SO_SNDTIMEO", "SO_TYPE", "SO_REUSEADDR"
    };
    const char *opt_ip[] = {
        "IP_ADD_MEMBERSHIP", "IP_DROP_MEMBERSHIP",
        "IP_HDRINCL", "IP_MTU", "IP_MULTICAST_TTL", "IP_PKTINFO", "IP_TOS",
        "IP_TTL"
    };
    const char *opt_tcp[] = {
        "TCP_KEEPCNT", "TCP_KEEPIDLE",
        "TCP_KEEPINTVL", "TCP_MAXSEG"
    };
    const char *opt_ipv6[] = {
        "IPV6_ADD_MEMBERSHIP", "IPV6_DROP_MEMBERSHIP"
    };
    const char **ona = 0;
    int nx;

    nx = -1;
    switch (level) {
    case SOL_SOCKET:
        ona = opt_sock;
        switch (name) {
        case SO_ACCEPTCONN:     nx = 0;     break;
        case SO_BROADCAST:      nx = 1;     break;
        case SO_DOMAIN:         nx = 2;     break;
        case SO_ERROR:          nx = 3;     break;
        case SO_KEEPALIVE:      nx = 4;     break;
        case SO_RCVBUF:         nx = 5;     break;
        case SO_RCVBUFFORCE:    nx = 6;     break;
        case SO_RCVTIMEO:       nx = 7;     break;
        case SO_SNDTIMEO:       nx = 8;     break;
        case SO_TYPE:           nx = 9;     break;
        case SO_REUSEADDR:      nx = 10;     break;
        }
        break;

    case IPPROTO_IP:
        ona = opt_ip;
        switch (name) {
        case IP_ADD_MEMBERSHIP: nx = 0;     break;
        case IP_DROP_MEMBERSHIP:nx = 1;     break;
        case IP_HDRINCL:        nx = 2;     break;
        case IP_MTU:            nx = 3;     break;
        case IP_MULTICAST_TTL:  nx = 4;     break;
        case IP_PKTINFO:        nx = 5;     break;
        case IP_TOS:            nx = 6;     break;
        case IP_TTL:            nx = 7;     break;
        }
        break;

    case IPPROTO_TCP:
        ona = opt_tcp;
        switch (name) {
        case TCP_KEEPCNT:       nx = 0;     break;
        case TCP_KEEPIDLE:      nx = 1;     break;
        case TCP_KEEPINTVL:     nx = 2;     break;
        case TCP_MAXSEG:        nx = 3;     break;
        }
        break;

    case IPPROTO_IPV6:
        ona = opt_ipv6;
        switch (name) {
        case IPV6_ADD_MEMBERSHIP: nx = 0;   break;
        case IPV6_DROP_MEMBERSHIP:nx = 1;   break;
        }
        break;
    }

    return (0 <= nx) ? ona[nx] : "Unknown_Name" ;
}

void output_param(void *ctrl, SOCKET_API_INFO *api, SOCKET_API_PARAM *param)
{
    const int op = api->out_param;
    int nx, ny, len;
    char *tmp;
    char addrstr[256];
    struct ifaddrs *ifa;
    struct sockaddr_in *adr_in;
    struct sockaddr_in6 *adr_in6;

    if (op == PRM_NONE) return;
    shell_puts(ctrl, S_LINE "[output]" S_LINE);

    if (op & PRM_LADDR) {
        if (param->laddr_p) {
            shell_puts(ctrl, "local-addr." S_LINE);
            if (param->laddr_p->sa_family != AF_INET6) {
                tmp = inet_ntoa(param->laddr.sin_addr);
                shell_puts(ctrl, "  ip-addr : ");
                shell_puts(ctrl, tmp);
                shell_puts(ctrl, S_LINE);

                shell_puts(ctrl, "  port    : ");
                shell_puts(ctrl, net_itoa(ntohs(param->laddr.sin_port), CmdBuf, 10));
                shell_puts(ctrl, S_LINE);
            } else {
                inet_ntop(AF_INET6, &param->laddr6.sin6_addr, addrstr, sizeof(addrstr));
                shell_puts(ctrl, "  ipv6-addr : ");
                shell_puts(ctrl, addrstr);
                shell_puts(ctrl, S_LINE);

                shell_puts(ctrl, "  port      : ");
                shell_puts(ctrl, net_itoa(ntohs(param->laddr6.sin6_port), CmdBuf, 10));
                shell_puts(ctrl, S_LINE);

                shell_puts(ctrl, "  scope_id  : ");
                shell_puts(ctrl, net_itoa(ntohs(param->laddr6.sin6_scope_id), CmdBuf, 10));
                shell_puts(ctrl, S_LINE);
            }
        }
        else {
            shell_puts(ctrl, "local-addr : NULL" S_LINE);
        }

        if (op & PRM_LEN_P) {
            if (param->llen_p) {
                tmp = net_itoa(*param->llen_p, CmdBuf, 10);
                shell_puts(ctrl, "laddr_len : ");
                shell_puts(ctrl, tmp);
                shell_puts(ctrl, S_LINE);
            }
            else {
                shell_puts(ctrl, "laddr_len : NULL" S_LINE);
            }
        }
    }
    if (op & PRM_RADDR) {
        if (param->raddr_p) {
            shell_puts(ctrl, "remote-addr." S_LINE);
            if (param->raddr_p->sa_family != AF_INET6) {
                tmp = inet_ntoa(param->raddr.sin_addr);
                shell_puts(ctrl, "  ip-addr : ");
                shell_puts(ctrl, tmp);
                shell_puts(ctrl, S_LINE);

                shell_puts(ctrl, "  port    : ");
                shell_puts(ctrl, net_itoa(ntohs(param->raddr.sin_port), CmdBuf, 10));
                shell_puts(ctrl, S_LINE);
            } else {
                inet_ntop(AF_INET6, &param->raddr6.sin6_addr, addrstr, sizeof(addrstr));
                shell_puts(ctrl, "  ipv6-addr : ");
                shell_puts(ctrl, addrstr);
                shell_puts(ctrl, S_LINE);

                shell_puts(ctrl, "  port      : ");
                shell_puts(ctrl, net_itoa(ntohs(param->raddr6.sin6_port), CmdBuf, 10));
                shell_puts(ctrl, S_LINE);

                shell_puts(ctrl, "  scope_id  : ");
                shell_puts(ctrl, net_itoa(ntohs(param->raddr6.sin6_scope_id), CmdBuf, 10));
                shell_puts(ctrl, S_LINE);
            }
        }
        else {
            shell_puts(ctrl, "remote-addr : NULL" S_LINE);
        }

        if (op & PRM_LEN_P) {
            if (param->rlen_p) {
                tmp = net_itoa(*param->rlen_p, CmdBuf, 10);
                shell_puts(ctrl, "raddr_len : ");
                shell_puts(ctrl, tmp);
                shell_puts(ctrl, S_LINE);
            }
            else {
                shell_puts(ctrl, "raddr_len : NULL" S_LINE);
            }
        }
    }

    if (op & PRM_FDSET) {
        if (0 < param->fds_n) {
            shell_puts(ctrl, "Socket of ");
            net_itoa(param->fds_n, CmdBuf, 10);
            shell_puts(ctrl, CmdBuf);
            shell_puts(ctrl, " response" S_LINE);

            nx = 0;
            for (; nx < MAX_SOCK_NUM; ++nx) {   // param->max_fds ‚ÍŽg—p‚µ‚È‚¢
                ny = 0;
                net_itoa(nx, CmdBuf, 10);
                len = net_strlen(CmdBuf);
                net_strcpy(&CmdBuf[len], ": ");
                len = net_strlen(CmdBuf);


                if ((param->nrfds) && (FD_ISSET(nx, &param->rfds))) {
                    net_strcpy(&CmdBuf[len], "r");
                    ++ny;
                }
                else {
                    net_strcpy(&CmdBuf[len], "-");
                }
                len = net_strlen(CmdBuf);

                if ((param->nwfds) && (FD_ISSET(nx, &param->wfds))) {
                    net_strcpy(&CmdBuf[len], "w");
                    ++ny;
                }
                else {
                    net_strcpy(&CmdBuf[len], "-");
                }
                len = net_strlen(CmdBuf);

                if ((param->nefds) && (FD_ISSET(nx, &param->efds))) {
                    net_strcpy(&CmdBuf[len], "e");
                    ++ny;
                }
                else {
                    net_strcpy(&CmdBuf[len], "-");
                }
                len = net_strlen(CmdBuf);

                if (ny) {
                    shell_puts(ctrl, CmdBuf);
                    shell_puts(ctrl, S_LINE);
                }
            }
        }
    }

    if (op & PRM_OPTION) {
        shell_puts(ctrl, "option_level-name :");
        shell_puts(ctrl, get_optlevel(param->opt.level));
        shell_puts(ctrl, " - ");
        shell_puts(ctrl, get_optname(param->opt.level, param->opt.name));
        shell_puts(ctrl, S_LINE);

        if (param->opt.val) {
            switch (param->opt.type) {
            case SOT_TIME:
                shell_puts(ctrl, "  get_val(time) " S_LINE);
                shell_puts(ctrl, "    seconds       : ");
                shell_puts(ctrl, S_LINE);
                shell_puts(ctrl, net_itoa(param->opt.time.tv_sec, CmdBuf, 10));
                shell_puts(ctrl, "    micro seconds : ");
                shell_puts(ctrl, net_itoa(param->opt.time.tv_usec, CmdBuf, 10));
                shell_puts(ctrl, S_LINE);
                break;
            case SOT_LINGER:
                shell_puts(ctrl, "  get_val(linger) " S_LINE);
                shell_puts(ctrl, "    l_onoff  : ");
                shell_puts(ctrl, net_itoa(param->opt.ling.l_onoff, CmdBuf, 10));
                shell_puts(ctrl, S_LINE);
                shell_puts(ctrl, "    l_linger : ");
                shell_puts(ctrl, net_itoa(param->opt.ling.l_linger, CmdBuf, 10));
                shell_puts(ctrl, S_LINE);
                break;

            case SOT_IP_MREQN:
                shell_puts(ctrl, "  get_val(ip_mreqn) " S_LINE);
                shell_puts(ctrl, "    multicast-group-addr  : ");
                tmp =inet_ntoa(param->opt.imreqn.imr_multiaddr);
                shell_puts(ctrl, tmp);
                shell_puts(ctrl, S_LINE);
                shell_puts(ctrl, "    target-ip-addr        : ");
                tmp =inet_ntoa(param->opt.imreqn.imr_address);
                shell_puts(ctrl, tmp);
                shell_puts(ctrl, S_LINE);
                shell_puts(ctrl, "    interface index       : ");
                shell_puts(ctrl, net_itoa(param->opt.imreqn.imr_ifindex, CmdBuf, 10));
                shell_puts(ctrl, S_LINE);
                break;

            case SOT_IP_PKNFO:
                shell_puts(ctrl, "    interface index : ");
                shell_puts(ctrl, net_itoa(param->opt.ipktnfo.ipi_ifindex, CmdBuf, 10));
                shell_puts(ctrl, S_LINE);
                shell_puts(ctrl, "  get_val(in_pktinfo) " S_LINE);
                shell_puts(ctrl, "    local-ip-addr   : ");
                tmp =inet_ntoa(param->opt.ipktnfo.ipi_addr);
                shell_puts(ctrl, tmp);
                shell_puts(ctrl, S_LINE);
                shell_puts(ctrl, "    header-dst-addr : ");
                tmp =inet_ntoa(param->opt.ipktnfo.ipi_spec_dst);
                shell_puts(ctrl, tmp);
                shell_puts(ctrl, S_LINE);
                    break;

                case SOT_INT:
                default:
                    shell_puts(ctrl, "  get_val(int) : ");
                    shell_puts(ctrl, net_itoa(param->opt.ival, CmdBuf, 10));
                    shell_puts(ctrl, S_LINE);
                break;
            }
        }
        else {
            shell_puts(ctrl, "  opt_val : NULL" S_LINE);
        }

        if (op & PRM_LEN_P) {
            if (param->olen_p) {
                tmp = net_itoa(*param->olen_p, CmdBuf, 10);
                shell_puts(ctrl, "opt_len : ");
                shell_puts(ctrl, tmp);
                shell_puts(ctrl, S_LINE);
            }
            else {
                shell_puts(ctrl, "opt_len : NULL" S_LINE);
            }
        }
    }

    if (op & PRM_GETADDR) {
        for(ifa = *(param->ifa_p); ifa != NULL; ifa=ifa->ifa_next) {
            shell_puts(ctrl, ifa->ifa_name);
            shell_puts(ctrl, S_LINE);

            shell_puts(ctrl, "        ifa              : 0x");
            shell_puts(ctrl, net_itoa((int)(ADDR)ifa, CmdBuf, 16));
            shell_puts(ctrl, S_LINE);
            shell_puts(ctrl, "        ifa_next         : 0x");
            shell_puts(ctrl, net_itoa((int)(ADDR)ifa->ifa_next, CmdBuf, 16));
            shell_puts(ctrl, S_LINE);
            shell_puts(ctrl, "        ifa_flags        : 0x");
            shell_puts(ctrl, net_itoa(ifa->ifa_flags, CmdBuf, 16));
            shell_puts(ctrl, S_LINE);

            shell_puts(ctrl, "        sa_family        : ");
            shell_puts(ctrl, net_itoa(ifa->ifa_addr->sa_family, CmdBuf, 10));
            if (ifa->ifa_addr->sa_family == AF_INET) {
                shell_puts(ctrl, " (AF_INET)");
            } else if (ifa->ifa_addr->sa_family == AF_INET6) {
                shell_puts(ctrl, " (AF_INET6)");
            }
            shell_puts(ctrl, S_LINE);

            if (ifa->ifa_addr->sa_family == AF_INET) {
                adr_in = (struct sockaddr_in*)ifa->ifa_addr;
                inet_ntop(AF_INET, &adr_in->sin_addr, addrstr, sizeof(addrstr));
                shell_puts(ctrl, "        addr             : ");
                shell_puts(ctrl, addrstr);
                shell_puts(ctrl, S_LINE);

                adr_in = (struct sockaddr_in*)ifa->ifa_netmask;
                inet_ntop(AF_INET, &adr_in->sin_addr, addrstr, sizeof(addrstr));
                shell_puts(ctrl, "        netmask          : ");
                shell_puts(ctrl, addrstr);
                shell_puts(ctrl, S_LINE);

                if (ifa->ifa_ifu.ifu_broadaddr != NULL) {
                    adr_in = (struct sockaddr_in*)ifa->ifa_ifu.ifu_broadaddr;
                    inet_ntop(AF_INET, &adr_in->sin_addr, addrstr, sizeof(addrstr));
                    shell_puts(ctrl, "        broadaddr        : ");
                    shell_puts(ctrl, addrstr);
                    shell_puts(ctrl, S_LINE);
                } else {
                    shell_puts(ctrl, "        broadaddr        : NULL");
                    shell_puts(ctrl, S_LINE);
                }
            } else if (ifa->ifa_addr->sa_family == AF_INET6) {
                adr_in6 = (struct sockaddr_in6*)ifa->ifa_addr;
                inet_ntop(AF_INET6, &adr_in6->sin6_addr, addrstr, sizeof(addrstr));
                shell_puts(ctrl, "        addr             : ");
                shell_puts(ctrl, addrstr);
                shell_puts(ctrl, S_LINE);

                shell_puts(ctrl, "        addr scope_id    : ");
                shell_puts(ctrl, net_itoa(adr_in6->sin6_scope_id, CmdBuf, 10));
                shell_puts(ctrl, S_LINE);

                adr_in6 = (struct sockaddr_in6*)ifa->ifa_netmask;
                inet_ntop(AF_INET6, &adr_in6->sin6_addr, addrstr, sizeof(addrstr));
                shell_puts(ctrl, "        netmask          : ");
                shell_puts(ctrl, addrstr);
                shell_puts(ctrl, S_LINE);

                shell_puts(ctrl, "        netmask scope_id : ");
                shell_puts(ctrl, net_itoa(adr_in6->sin6_scope_id, CmdBuf, 10));
                shell_puts(ctrl, S_LINE);

                if (ifa->ifa_ifu.ifu_broadaddr != NULL) {
                    adr_in6 = (struct sockaddr_in6*)ifa->ifa_ifu.ifu_broadaddr;
                    inet_ntop(AF_INET6, &adr_in6->sin6_addr, addrstr, sizeof(addrstr));
                    shell_puts(ctrl, "        broadaddr        : ");
                    shell_puts(ctrl, addrstr);
                  shell_puts(ctrl, S_LINE);
                } else {
                    shell_puts(ctrl, "        broadaddr        : NULL");
                    shell_puts(ctrl, S_LINE);
                }
            }
        }
    }

    if (op & PRM_INETPTON) {
        shell_puts(ctrl, "dst-addr." S_LINE);
        if (param->family != AF_INET6) {
            shell_puts(ctrl, "  ip[0] : 0x");
            shell_puts(ctrl, net_itoa(param->ipval[0], CmdBuf, 16));
            shell_puts(ctrl, S_LINE);
        } else {
            shell_puts(ctrl, "  ip[0] : 0x");
            shell_puts(ctrl, net_itoa(param->ipval[0], CmdBuf, 16));
            shell_puts(ctrl, S_LINE);

            shell_puts(ctrl, "  ip[1] : 0x");
            shell_puts(ctrl, net_itoa(param->ipval[1], CmdBuf, 16));
            shell_puts(ctrl, S_LINE);

            shell_puts(ctrl, "  ip[2] : 0x");
            shell_puts(ctrl, net_itoa(param->ipval[2], CmdBuf, 16));
            shell_puts(ctrl, S_LINE);

            shell_puts(ctrl, "  ip[3] : 0x");
            shell_puts(ctrl, net_itoa(param->ipval[3], CmdBuf, 16));
            shell_puts(ctrl, S_LINE);
        }
    }

    if (op & PRM_INETNTOP) {
        shell_puts(ctrl, "dst-addr." S_LINE);
        shell_puts(ctrl, "  ipstr : ");
        shell_puts(ctrl, param->ipstr);
        shell_puts(ctrl, S_LINE);
    }
}

ER bsd_command(VP ctrl, INT argc, VB *argv[])
{
    SOCKET_API_INFO  *menu = &socket_api_list[0];
    SOCKET_API_PARAM param;
    int res;
    char *cmd = argv[0];
    ER ercd = 0;

    while (menu->func) {
        if (net_strcmp(menu->api_name, cmd) == 0) {
            break;
        }
        menu++;
    }

    if (menu->func) {
        net_memset(&param, 0, sizeof(param));
        res = input_param(ctrl, menu, &param);

        if (0 > res) {
            shell_puts(ctrl, "# input_error ->");
            shell_puts(ctrl, net_itoa(res, CmdBuf, 10));
            return E_PAR;
        }

        res = menu->func(ctrl, &param);
        if (is_error(menu, res)) {
            shell_puts(ctrl, "# func_error ->");
            shell_puts(ctrl, net_itoa(res, CmdBuf, 10));
            shell_puts(ctrl, S_LINE);

            shell_puts(ctrl, "# errno ->");
            output_errno(ctrl, errno);

            ercd = E_OBJ;
        }
        else {
            shell_puts(ctrl, "# func_success ->");
            shell_puts(ctrl, net_itoa(res, CmdBuf, 10));
            output_param(ctrl, menu, &param);

            ercd = E_OK;
        }
    }

    return ercd;
}

