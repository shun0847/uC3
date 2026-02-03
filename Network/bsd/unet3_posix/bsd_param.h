/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK
    BSD socket wrapper/Parametet definitions
    Copyright (c) 2014-2020, eForce Co., Ltd. All rights reserved.

    Version Information  2014.04.28: Created
                         2015.08.12: Add parameters of select
                         2016.06.08: Modify the parameters of ioctl.
                         2020.09.07: Supported IPv6.
 ***************************************************************************/

#ifndef     _BSD_PAR_H
#define     _BSD_PAR_H

typedef struct t_param_socket {
    int domain;
    int type;
    int protocol;
}T_PARAM_SOCKET;

typedef struct t_param_bind {
    struct unet3_sockaddr_in6 addr;
    unet3_socklen_t addrlen;
}T_PARAM_BIND;

typedef struct t_param_listen {
    int backlog;
}T_PARAM_LISTEN;

typedef struct t_param_accept {
    struct unet3_sockaddr_in6 addr;
    unet3_socklen_t addrlen;
}T_PARAM_ACCEPT;

typedef struct t_param_connect {
    struct unet3_sockaddr_in6 addr;
    unet3_socklen_t addrlen;
}T_PARAM_CONNECT;

typedef struct t_param_send {
    void *buf;
    unsigned int len;
    int flags;
}T_PARAM_SEND;

typedef struct t_param_sendto {
    void *buf;
    unsigned int len;
    int flags;
    struct unet3_sockaddr_in6 dest_addr;
    unet3_socklen_t addrlen;
}T_PARAM_SENDTO;

typedef struct t_param_recv {
    void *buf;
    unsigned int len;
    int flags;
}T_PARAM_RECV;

typedef struct t_param_recvfrom {
    void *buf;
    unsigned int len;
    int flags;
    struct unet3_sockaddr_in6 src_addr;
    unet3_socklen_t addrlen;
}T_PARAM_RECVFROM;

typedef struct t_param_shutdown {
    int how;
}T_PARAM_SHUTDOWN;

typedef struct t_param_close {
    int dummy;
}T_PARAM_CLOSE;

typedef struct t_param_getsockname {
    struct unet3_sockaddr_in6 addr;
    unet3_socklen_t addrlen;
}T_PARAM_GETSOCKNAME;

typedef struct t_param_getpeername {
    struct unet3_sockaddr_in6 addr;
    unet3_socklen_t addrlen;
}T_PARAM_GETPEERNAME;

typedef struct t_param_getsockopt {
    int level;
    int optname;
    VB  optval[SOC_OPT_MAXLEN];
    unet3_socklen_t optlen;
}T_PARAM_GETSOCKOPT, T_PARAM_SETSOCKOPT, T_PARAM_SOCKOPT;

typedef struct t_param_select {
    int nfds;
    unet3_fd_set readfds;
    unet3_fd_set writefds;
    unet3_fd_set exceptfds;
    BOOL       r;
    BOOL       w;
    BOOL       e;
}T_PARAM_SELECT;

typedef struct t_param_select_u {
    BOOL       r;
    BOOL       w;
    BOOL       e;
    VP         cbk;
    ID         tskid;   /* search key, do not use to wup_tsk() for */
}T_PARAM_SELECT_U;

typedef struct t_param_ioctl {
    int request;
    int *val;
}T_PARAM_IOCTL;

typedef union t_pram {
    T_PARAM_SOCKET      socket;
    T_PARAM_BIND        bind;
    T_PARAM_LISTEN      listen;
    T_PARAM_ACCEPT      accept;
    T_PARAM_CONNECT     connect;
    T_PARAM_SEND        send;
    T_PARAM_SENDTO      sendto;
    T_PARAM_RECV        recv;
    T_PARAM_RECVFROM    recvfrom;
    T_PARAM_SHUTDOWN    shutdown;
    T_PARAM_CLOSE       close;
    T_PARAM_GETSOCKNAME getsockname;
    T_PARAM_GETPEERNAME getpeername;
    T_PARAM_GETSOCKOPT  getsockopt;
    T_PARAM_SETSOCKOPT  setsockopt;
    T_PARAM_SOCKOPT     sockopt;
    T_PARAM_SELECT      select;
    T_PARAM_SELECT_U    select_u;
    T_PARAM_IOCTL       ioctl;
}T_PARAM;


#endif
