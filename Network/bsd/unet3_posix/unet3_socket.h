/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK
    BSD socket wrapper/Socket definitions
    Copyright (c) 2014-2020, eForce Co., Ltd. All rights reserved.

    Version Information  2014.04.28: Created
                         2014.07.24: Add getifaddrs/freeifaddrs
                         2016.02.05: Update for MSG_DONTWAIT for send
                         2016.02.08: Update for raw socket
                         2016.05.12: Modify the broadcast options
                         2016.06.09: Add the MSG_PEEK options
                         2016.06.16  Add the UNET3_OPT_REUSEADDR flag
                         2018.12.06  Add rename macro of IP_DONTFRAGMENT
                         2019.03.20: Add USE_APLBUF flags option
                         2020.09.07: Supported IPv6.
 ***************************************************************************/

#ifndef     _UNET_SOCKET_H
#define     _UNET_SOCKET_H

#include "unet3_cfg.h"

/* sys/socket.h */
/* netinet/in.h */
/* arpa/inet.h */

#include "kernel.h"
#include "net_hdr.h"
#include "net_def.h"
#include "unet3_sys.h"

typedef unsigned int    unet3_socklen_t;
typedef unsigned int    unet3_in_addr_t;

/* ipv4 inet.h */
typedef struct unet3_in_addr {
    UW s_addr;
}unet3_in_addr;

typedef struct unet3_sockaddr_in {
    UB sin_len;
    UB sin_family;
    UH sin_port;
    struct unet3_in_addr sin_addr;
    char sin_zero[8];
}unet3_sockaddr_in;

typedef struct unet3_sockaddr {
    UB sa_len;
    UB sa_family;
    char sa_data[14];
}unet3_sockaddr;

typedef struct unet3_ip_mreq {
    struct unet3_in_addr imr_multiaddr;
    struct unet3_in_addr imr_interface;
    int            imr_ifindex;
}unet3_ip_mreq;

typedef struct unet3_ip_mreqn {
    struct unet3_in_addr imr_multiaddr;
    struct unet3_in_addr imr_address;
    int            imr_ifindex;
}unet3_ip_mreqn;

typedef struct unet3_in_pktinfo {
    unsigned int   ipi_ifindex;
    struct unet3_in_addr ipi_spec_dst;
    struct unet3_in_addr ipi_addr;
    int            enable;
}unet3_in_pktinfo;

typedef struct unet3_linger {
    long l_onoff;
    long l_linger;
}unet3_linger;

typedef struct unet3_ifaddrs {
    struct unet3_ifaddrs    *ifa_next;
    char                    *ifa_name;
    unsigned int            ifa_flags;
    struct unet3_sockaddr   *ifa_addr;
    struct unet3_sockaddr   *ifa_netmask;
    union {
        struct unet3_sockaddr *ifu_broadaddr;
        struct unet3_sockaddr *ifu_dstaddr;  /* peer */
    } ifa_ifu;
    void                    *ifa_data;
}unet3_ifaddrs;

/* ipv6 inet.h */
typedef struct unet3_in6_addr {
    union {
        UB            __u6_addr8[16];
        UH            __u6_addr16[8];
        UW            __u6_addr32[4];
    } __in6_u;
    #define unet3_s6_addr   __in6_u.__u6_addr8
    #define unet3_s6_addr16 __in6_u.__u6_addr16
    #define unet3_s6_addr32 __in6_u.__u6_addr32
}unet3_in6_addr;

typedef struct unet3_sockaddr_in6 {
    UB                    sin6_len;
    UB                    sin6_family;
    UH                    sin6_port;
    unsigned int          sin6_flowinfo;
    struct unet3_in6_addr sin6_addr;
    unsigned int          sin6_scope_id;
}unet3_sockaddr_in6;

typedef struct unet3_sockaddr_storage {
    UB ss_len;
    UB ss_family;
    char                 __ss_pad1[2];
    unsigned long int    __ss_align;
    char                 __ss_pad2[120];
    #define __ss_padding __ss_pad2;
}unet3_sockaddr_storage;

typedef struct unet3_ipv6_mreq {
    struct unet3_in6_addr ipv6mr_multiaddr;
    unsigned int          ipv6mr_interface;
}unet3_ipv6_mreq;

/* protocol familiy */
#define UNET3_AF_INET         IP_VER4
#define UNET3_AF_INET6        IP_VER6
#define UNET3_PF_INET         UNET3_AF_INET
#define UNET3_PF_INET6        UNET3_AF_INET6

#define UNET3_AF_UNSPEC       0xFF
#define UNET3_PF_UNSPEC       UNET3_AF_UNSPEC

/* socket type */
#define UNET3_SOCK_STREAM     IP_PROTO_TCP
#define UNET3_SOCK_DGRAM      IP_PROTO_UDP
#define UNET3_SOCK_RAW        IP_PROTO_RAW

/* how to shutdown */
#define UNET3_SHUT_RDWR       SOC_TCP_CLS
#define UNET3_SHUT_WR         SOC_TCP_SHT
#define UNET3_SHUT_RD         3

/* socket option level */
#define UNET3_SOL_SOCKET      0
#define UNET3_IPPROTO_IP      1
#define UNET3_IPPROTO_TCP     IP_PROTO_TCP /* 6 */
#define UNET3_IPPROTO_UDP     IP_PROTO_UDP /* 17 */
#define UNET3_IPPROTO_IPV6    41           /* IPv6 header */

/* socket option name */
#define UNET3_SO_ACCEPTCONN       1
#define UNET3_SO_BINDTODEVICE     2
#define UNET3_SO_BROADCAST        3
#define UNET3_SO_BSDCOMPAT        4
#define UNET3_SO_DEBUG            5
#define UNET3_SO_DOMAIN           6
#define UNET3_SO_ERROR            7
#define UNET3_SO_DONTROUTE        8
#define UNET3_SO_KEEPALIVE        9
#define UNET3_SO_LINGER           10
#define UNET3_SO_MARK             11
#define UNET3_SO_OOBINLINE        12
#define UNET3_SO_PASSCRED         13
#define UNET3_SO_PEEK_OFF         14
#define UNET3_SO_PEERCRED         15
#define UNET3_SO_PRIORITY         16
#define UNET3_SO_PROTOCOL         17
#define UNET3_SO_RCVBUF           18
#define UNET3_SO_RCVBUFFORCE      19
#define UNET3_SO_RCVLOWAT         20
#define UNET3_SO_SNDLOWAT         21
#define UNET3_SO_RCVTIMEO         22
#define UNET3_SO_SNDTIMEO         23
#define UNET3_SO_REUSEADDR        24
#define UNET3_SO_SNDBUF           25
#define UNET3_SO_SNDBUFFORCE      26
#define UNET3_SO_TIMESTAMP        27
#define UNET3_SO_TYPE             28

/* socket option name */
#define UNET3_IP_ADD_MEMBERSHIP           1
#define UNET3_IP_ADD_SOURCE_MEMBERSHIP    2
#define UNET3_IP_BLOCK_SOURCE             3
#define UNET3_IP_DROP_MEMBERSHIP          4
#define UNET3_IP_DROP_SOURCE_MEMBERSHIP   5
#define UNET3_IP_FREEBIND                 6
#define UNET3_IP_HDRINCL                  7
#define UNET3_IP_MSFILTER                 8
#define UNET3_IP_MTU                      9
#define UNET3_IP_MTU_DISCOVER             10
#define UNET3_IP_PMTUDISC_WANT            11
#define UNET3_IP_PMTUDISC_DONT            12
#define UNET3_IP_PMTUDISC_DO              13
#define UNET3_IP_PMTUDISC_PROBE           14
#define UNET3_IP_MULTICAST_ALL            15
#define UNET3_IP_MULTICAST_IF             16
#define UNET3_IP_MULTICAST_LOOP           17
#define UNET3_IP_MULTICAST_TTL            18
#define UNET3_IP_NODEFRAG                 19
#define UNET3_IP_OPTIONS                  20
#define UNET3_IP_PKTINFO                  21
#define UNET3_IP_RECVERR                  22
#define UNET3_IP_RECVOPTS                 23
#define UNET3_IP_RECVORIGDSTADDR          24
#define UNET3_IP_RECVTOS                  25
#define UNET3_IP_RECVTTL                  26
#define UNET3_IP_RETOPTS                  27
#define UNET3_IP_ROUTER_ALERT             28
#define UNET3_IP_TOS                      29
#define UNET3_IP_TRANSPARENT              30
#define UNET3_IP_TTL                      31
#define UNET3_IP_UNBLOCK_SOURCE           32
#define UNET3_IP_DONTFRAGMENT             33

/* socket option name */
#define UNET3_TCP_CORK                    1
#define UNET3_TCP_DEFER_ACCEPT            2
#define UNET3_TCP_INFO                    3
#define UNET3_TCP_KEEPCNT                 4
#define UNET3_TCP_KEEPIDLE                5
#define UNET3_TCP_KEEPINTVL               6
#define UNET3_TCP_LINGER2                 7
#define UNET3_TCP_MAXSEG                  8
#define UNET3_TCP_NODELAY                 9
#define UNET3_TCP_QUICKACK                10
#define UNET3_TCP_SYNCNT                  11
#define UNET3_TCP_WINDOW_CLAMP            12

/* socket option name */
#define UNET3_IPV6_ADDRFORM               1
#define UNET3_IPV6_ADD_MEMBERSHIP         2
#define UNET3_IPV6_DROP_MEMBERSHIP        3
#define UNET3_IPV6_MTU                    4
#define UNET3_IPV6_MTU_DISCOVER           5
#define UNET3_IPV6_MULTICAST_HOPS         6
#define UNET3_IPV6_MULTICAST_IF           7
#define UNET3_IPV6_MULTICAST_LOOP         8
#define UNET3_IPV6_RECVPKTINFO            9
#define UNET3_IPV6_PKTINFO                10
#define UNET3_IPV6_RTHDR                  11
#define UNET3_IPV6_AUTHHDR                12
#define UNET3_IPV6_DSTOPTS                13
#define UNET3_IPV6_HOPOPTS                14
#define UNET3_IPV6_FLOWINFO               15
#define UNET3_IPV6_HOPLIMIT               16
#define UNET3_IPV6_RECVERR                17
#define UNET3_IPV6_ROUTER_ALERT           18
#define UNET3_IPV6_UNICAST_HOPS           19
#define UNET3_IPV6_V6ONLY                 20

/* flags for send, recv */
#define UNET3_MSG_DONTWAIT                0x01
#define UNET3_MSG_PEEK                    0x02
/* uNet3 original */
#define UNET3_USE_APLBUF                  0x0100

/* flags for socket option */
#define UNET3_OPT_REUSEADDR               0x00000001U

/* IPv6 address */
#define UNET3_IN6ADDR_ANY_INIT            { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } } }
#define UNET3_IN6ADDR_LOOPBACK_INIT       { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } } }

/* proto type */
int unet3_socket(int domain, int type, int protocol);
int unet3_bind(int sockfd, const struct unet3_sockaddr *addr, unsigned int addrlen);
int unet3_listen(int sockfd, int backlog);
int unet3_accept(int sockfd, struct unet3_sockaddr *addr, unsigned int *addrlen);
int unet3_connect(int sockfd, const struct unet3_sockaddr *addr, unsigned int addrlen);
int unet3_send(int sockfd, const void *buf, unsigned int len, int flags);
int unet3_sendto(int sockfd, const void *buf, unsigned int len, int flags, const struct unet3_sockaddr *dest_addr, unsigned int addrlen);
int unet3_recv(int sockfd, void *buf, unsigned int len, int flags);
int unet3_recvfrom(int sockfd, void *buf, unsigned int len, int flags, struct unet3_sockaddr *src_addr, unsigned int *addrlen);
int unet3_shutdown(int sockfd, int how);  
int unet3_close(int fd);


int unet3_getsockname(int sockfd, struct unet3_sockaddr *addr, unsigned int *addrlen);
int unet3_getpeername(int sockfd, struct unet3_sockaddr *addr, unsigned int *addrlen);  
int unet3_getsockopt(int sockfd, int level, int optname, void *optval, unsigned int *optlen);
int unet3_setsockopt(int sockfd, int level, int optname, const void *optval, unsigned int optlen);

int unet3_inet_aton(const char *cp, struct unet3_in_addr *inp);
unsigned int unet3_inet_addr(const char *cp);
char *unet3_inet_ntoa(struct unet3_in_addr in);

int unet3_rresvport(int *port);

int unet3_getifaddrs(struct unet3_ifaddrs **ifap);
void unet3_freeifaddrs(struct unet3_ifaddrs *ifa);
int unet3_inet_pton(int af, const char *src, void *dst);
const char *unet3_inet_ntop(int af, const void *src, char *dst, unsigned int size);

#if (SYS_PLATFORM)
/* select_u callback function */
typedef void (*SELECT_CBK)(int sockfd, BOOL r, BOOL w, BOOL e, ER ercd);
int unet3_select_u(int sockfd, BOOL r, BOOL w, BOOL e, VP cbk);
#endif


ER unet3_bsd_init(void);
void bsd_unet3_tsk(VP_INT exinf);

/*---------------- Redefine POSIX name ----------------*/

/* API */
#if !(SYS_PLATFORM)
#define socket        unet3_socket
#define bind          unet3_bind
#define listen        unet3_listen
#define accept        unet3_accept
#define connect       unet3_connect
#define send          unet3_send
#define sendto        unet3_sendto
#define recv          unet3_recv
#define recvfrom      unet3_recvfrom
#define shutdown      unet3_shutdown
#define close         unet3_close
#define getsockname   unet3_getsockname
#define getpeername   unet3_getpeername
#define getsockopt    unet3_getsockopt
#define setsockopt    unet3_setsockopt
#define inet_aton     unet3_inet_aton
#define inet_addr     unet3_inet_addr
#define inet_ntoa     unet3_inet_ntoa
#define rresvport     unet3_rresvport
#define getifaddrs    unet3_getifaddrs
#define freeifaddrs   unet3_freeifaddrs
#define inet_pton     unet3_inet_pton
#define inet_ntop     unet3_inet_ntop

/* struct */
#define in_addr       unet3_in_addr
#define sockaddr_in   unet3_sockaddr_in
#define sockaddr      unet3_sockaddr
#define ip_mreq       unet3_ip_mreq
#define ip_mreqn      unet3_ip_mreqn
#define in_pktinfo    unet3_in_pktinfo
#define linger        unet3_linger
#define iovec         unet3_iovec
#define msghdr        unet3_msghdr
#define cmsghdr       unet3_cmsghdr
#define ifaddrs       unet3_ifaddrs
#define in6_addr      unet3_in6_addr
#define sockaddr_in6  unet3_sockaddr_in6
#define sockaddr_storage  unet3_sockaddr_storage
#define ipv6_mreq     unet3_ipv6_mreq

/* struct in6_addr define */
#define s6_addr       unet3_s6_addr
#define s6_addr16     unet3_s6_addr16
#define s6_addr32     unet3_s6_addr32

/* protocol familiy */
#define AF_INET             UNET3_AF_INET
#define AF_INET6            UNET3_AF_INET6
#define PF_INET             UNET3_PF_INET
#define PF_INET6            UNET3_PF_INET6
#define AF_UNSPEC           UNET3_AF_UNSPEC
#define PF_UNSPEC           UNET3_PF_UNSPEC
/* socket type */
#define SOCK_STREAM         UNET3_SOCK_STREAM
#define SOCK_DGRAM          UNET3_SOCK_DGRAM
#define SOCK_RAW            UNET3_SOCK_RAW
/* how to shutdown */
#define SHUT_RDWR           UNET3_SHUT_RDWR
#define SHUT_WR             UNET3_SHUT_WR
#define SHUT_RD             UNET3_SHUT_RD
/* socket option level */
#define SOL_SOCKET          UNET3_SOL_SOCKET
#define IPPROTO_IP          UNET3_IPPROTO_IP
#define IPPROTO_TCP         UNET3_IPPROTO_TCP
#define IPPROTO_UDP         UNET3_IPPROTO_UDP
#define IPPROTO_IPV6        UNET3_IPPROTO_IPV6
/* socket option name */
#define SO_ACCEPTCONN       UNET3_SO_ACCEPTCONN
#define SO_BINDTODEVICE     UNET3_SO_BINDTODEVICE
#define SO_BROADCAST        UNET3_SO_BROADCAST
#define SO_BSDCOMPAT        UNET3_SO_BSDCOMPAT
#define SO_DEBUG            UNET3_SO_DEBUG
#define SO_DOMAIN           UNET3_SO_DOMAIN
#define SO_ERROR            UNET3_SO_ERROR
#define SO_DONTROUTE        UNET3_SO_DONTROUTE
#define SO_KEEPALIVE        UNET3_SO_KEEPALIVE
#define SO_LINGER           UNET3_SO_LINGER
#define SO_MARK             UNET3_SO_MARK
#define SO_OOBINLINE        UNET3_SO_OOBINLINE
#define SO_PASSCRED         UNET3_SO_PASSCRED
#define SO_PEEK_OFF         UNET3_SO_PEEK_OFF
#define SO_PEERCRED         UNET3_SO_PEERCRED
#define SO_PRIORITY         UNET3_SO_PRIORITY
#define SO_PROTOCOL         UNET3_SO_PROTOCOL
#define SO_RCVBUF           UNET3_SO_RCVBUF
#define SO_RCVBUFFORCE      UNET3_SO_RCVBUFFORCE
#define SO_RCVLOWAT         UNET3_SO_RCVLOWAT
#define SO_SNDLOWAT         UNET3_SO_SNDLOWAT
#define SO_RCVTIMEO         UNET3_SO_RCVTIMEO
#define SO_SNDTIMEO         UNET3_SO_SNDTIMEO
#define SO_REUSEADDR        UNET3_SO_REUSEADDR
#define SO_SNDBUF           UNET3_SO_SNDBUF
#define SO_SNDBUFFORCE      UNET3_SO_SNDBUFFORCE
#define SO_TIMESTAMP        UNET3_SO_TIMESTAMP
#define SO_TYPE             UNET3_SO_TYPE
/* socket option name */
#define IP_ADD_MEMBERSHIP           UNET3_IP_ADD_MEMBERSHIP
#define IP_ADD_SOURCE_MEMBERSHIP    UNET3_IP_ADD_SOURCE_MEMBERSHIP
#define IP_BLOCK_SOURCE             UNET3_IP_BLOCK_SOURCE
#define IP_DROP_MEMBERSHIP          UNET3_IP_DROP_MEMBERSHIP
#define IP_DROP_SOURCE_MEMBERSHIP   UNET3_IP_DROP_SOURCE_MEMBERSHIP
#define IP_FREEBIND                 UNET3_IP_FREEBIND
#define IP_HDRINCL                  UNET3_IP_HDRINCL
#define IP_MSFILTER                 UNET3_IP_MSFILTER
#define IP_MTU                      UNET3_IP_MTU
#define IP_MTU_DISCOVER             UNET3_IP_MTU_DISCOVER
#define IP_PMTUDISC_WANT            UNET3_IP_PMTUDISC_WANT
#define IP_PMTUDISC_DONT            UNET3_IP_PMTUDISC_DONT
#define IP_PMTUDISC_DO              UNET3_IP_PMTUDISC_DO
#define IP_PMTUDISC_PROBE           UNET3_IP_PMTUDISC_PROBE
#define IP_MULTICAST_ALL            UNET3_IP_MULTICAST_ALL
#define IP_MULTICAST_IF             UNET3_IP_MULTICAST_IF
#define IP_MULTICAST_LOOP           UNET3_IP_MULTICAST_LOOP
#define IP_MULTICAST_TTL            UNET3_IP_MULTICAST_TTL
#define IP_NODEFRAG                 UNET3_IP_NODEFRAG
#define IP_OPTIONS                  UNET3_IP_OPTIONS
#define IP_PKTINFO                  UNET3_IP_PKTINFO
#define IP_RECVERR                  UNET3_IP_RECVERR
#define IP_RECVOPTS                 UNET3_IP_RECVOPTS
#define IP_RECVORIGDSTADDR          UNET3_IP_RECVORIGDSTADDR
#define IP_RECVTOS                  UNET3_IP_RECVTOS
#define IP_RECVTTL                  UNET3_IP_RECVTTL
#define IP_RETOPTS                  UNET3_IP_RETOPTS
#define IP_ROUTER_ALERT             UNET3_IP_ROUTER_ALERT
#define IP_TOS                      UNET3_IP_TOS
#define IP_TRANSPARENT              UNET3_IP_TRANSPARENT
#define IP_TTL                      UNET3_IP_TTL
#define IP_UNBLOCK_SOURCE           UNET3_IP_UNBLOCK_SOURCE
#define IP_DONTFRAGMENT             UNET3_IP_DONTFRAGMENT
/* socket option name */
#define TCP_CORK                    UNET3_TCP_CORK
#define TCP_DEFER_ACCEPT            UNET3_TCP_DEFER_ACCEPT
#define TCP_INFO                    UNET3_TCP_INFO
#define TCP_KEEPCNT                 UNET3_TCP_KEEPCNT
#define TCP_KEEPIDLE                UNET3_TCP_KEEPIDLE
#define TCP_KEEPINTVL               UNET3_TCP_KEEPINTVL
#define TCP_LINGER2                 UNET3_TCP_LINGER2
#define TCP_MAXSEG                  UNET3_TCP_MAXSEG
#define TCP_NODELAY                 UNET3_TCP_NODELAY
#define TCP_QUICKACK                UNET3_TCP_QUICKACK
#define TCP_SYNCNT                  UNET3_TCP_SYNCNT
#define TCP_WINDOW_CLAMP            UNET3_TCP_WINDOW_CLAMP
/* socket option name */
#define IPV6_ADDRFORM               UNET3_IPV6_ADDRFORM
#define IPV6_ADD_MEMBERSHIP         UNET3_IPV6_ADD_MEMBERSHIP
#define IPV6_DROP_MEMBERSHIP        UNET3_IPV6_DROP_MEMBERSHIP
#define IPV6_MTU                    UNET3_IPV6_MTU
#define IPV6_MTU_DISCOVER           UNET3_IPV6_MTU_DISCOVER
#define IPV6_MULTICAST_HOPS         UNET3_IPV6_MULTICAST_HOPS
#define IPV6_MULTICAST_IF           UNET3_IPV6_MULTICAST_IF
#define IPV6_MULTICAST_LOOP         UNET3_IPV6_MULTICAST_LOOP
#define IPV6_RECVPKTINFO            UNET3_IPV6_RECVPKTINFO
#define IPV6_PKTINFO                UNET3_IPV6_PKTINFO
#define IPV6_RTHDR                  UNET3_IPV6_RTHDR
#define IPV6_AUTHHDR                UNET3_IPV6_AUTHHDR
#define IPV6_DSTOPTS                UNET3_IPV6_DSTOPTS
#define IPV6_HOPOPTS                UNET3_IPV6_HOPOPTS
#define IPV6_FLOWINFO               UNET3_IPV6_FLOWINFO
#define IPV6_HOPLIMIT               UNET3_IPV6_HOPLIMIT
#define IPV6_RECVERR                UNET3_IPV6_RECVERR
#define IPV6_ROUTER_ALERT           UNET3_IPV6_ROUTER_ALERT
#define IPV6_UNICAST_HOPS           UNET3_IPV6_UNICAST_HOPS
#define IPV6_V6ONLY                 UNET3_IPV6_V6ONLY

/* flags for send, recv */
#define MSG_DONTWAIT                UNET3_MSG_DONTWAIT
#define MSG_PEEK                    UNET3_MSG_PEEK
/* IPv6 address */
#define IN6ADDR_ANY_INIT            UNET3_IN6ADDR_ANY_INIT
#define IN6ADDR_LOOPBACK_INIT       UNET3_IN6ADDR_LOOPBACK_INIT
/* uNet3 original */
#define USE_APLBUF                  UNET3_USE_APLBUF

/* type */
#define socklen_t                   unet3_socklen_t
#define in_addr_t                   unet3_in_addr_t

#endif /* !(SYS_PLATFORM) */

/* control block */

typedef struct t_unet3_que {
    struct t_unet3_que     *next;
}T_UNET3_QUE, *PNETQ;

typedef struct t_unet3_bsd_soc {
    PNETQ                   next;
    /* API que
        API_TYPE_CON  0    connect, accept
        API_TYPE_SND  1    send, sendto
        API_TYPE_RCV  2    recv, recvfrom
        API_TYPE_CLS  3    shutdown, close
    */
    PNETQ                   apique[4];

#if (SYS_PLATFORM)
    /* select_u que */
    PNETQ                   selectque;
#endif

    /* listen que */
    PNETQ                   listenq;
    /* parent socket from listen que */
    int                     parentfd;
    int                     backlog;

    int                     sockfd;

    T_NODE                  defaddr;
    UB                      type;
    UB                      proto;

    /* BSD specific option */
    BOOL                    ioasync;
    BOOL                    broadcast;

    UB                      hdrincl;

    ER                      so_err;    /* SO_ERROR */
    UB                      con_sts;   /* TCP connection auxiliary state*/
    int                     lst_errno; /* The last error number */
    int                     nxt_errno; /* The next error number */
    UB                      delflg;
    UW                      so_flags;  /* Socket option flags */

    struct unet3_timeval          rcvtm;
    struct unet3_timeval          sndtm;
    struct unet3_linger           clstm;

    /* ancillary message */
    unet3_in_pktinfo              pktinf;
    int                     mcif; /* multicast interface */

}T_UNET3_BSD_SOC;


#endif
