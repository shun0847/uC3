/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK
    BSD socket wrapper/Internal definitions
    Copyright (c) 2014-2020, eForce Co., Ltd. All rights reserved.

    Version Information
      2014.04.28: Created
      2014.07.24: Fixed the problem of cancel message
      2015.03.20: Modified to support uNet3/Compact.
      2016.05.20  Add include file for HWOS
      2020.09.07: Supported IPv6.
 ***************************************************************************/

#ifndef     _UNET3_WRAP_H
#define     _UNET3_WRAP_H

#include "kernel.h"
#include "net_sup.h"

/* messge direction */
#define     API_MSG_NONE                    0
#define     API_MSG_APPLICATION             1
#define     API_MSG_CALLBCK                 2

/* API Type */
#define     API_TYPE_NOT_IO                 -1

#define     API_TYPE_CON                    0    /* connect, accept */
#define     API_TYPE_SND                    1    /* send, sendto    */
#define     API_TYPE_RCV                    2    /* recv, recvfrom  */
#define     API_TYPE_CLS                    3    /* shutdown, close */
#define     API_TYPE_START                  API_TYPE_CON
#define     API_TYPE_END                    API_TYPE_CLS    /* must be 'close' id */

#define     API_TYPE_SRDY                   4    /* select */
#define     API_TYPE_RRDY                   5    /* select */

#define     TCP_CON_NOTCON                  0    /* not connected yet*/
#define     TCP_CON_CONNECT                 1    /* connecting */
#define     TCP_CON_DISCON_RX               2    /* rx stream reach to EOF(FIN/RST) */

#define     E_WRAPPER                       -100 /* error in BSD wrapper */

/* API ID */
typedef enum api_id {
    API_ID_SOCKET = 0,
    API_ID_BIND,
    API_ID_LISTEN,
    API_ID_ACCEPT,
    API_ID_CONNECT,
    API_ID_SEND,
    API_ID_SENDTO,
    API_ID_RECV,
    API_ID_RECVFROM,
    API_ID_SHUTDOWN,
    API_ID_CLOSE,
    API_ID_SELECT,
#if (SYS_PLATFORM)
    API_ID_SELECT_U,
#endif
    API_ID_GETSOCKNAME,
    API_ID_GETPEERNAME,
    API_ID_GETSOCKOPT,
    API_ID_SETSOCKOPT,
    API_ID_IOCTL,

    API_ID_uNET3_CALLBAK
}API_ID;

/* Message Type */
#define     MSG_TYPE_REQUEST                 1
#define     MSG_TYPE_CANCEL                  2
#define     MSG_TYPE_ABORT                   3

#define     SIN_NODE(sin, node)   (node)->ipa=ntohl((sin)->sin_addr.s_addr);\
                                  (node)->port=ntohs((sin)->sin_port);\
                                  (node)->ver=(sin)->sin_family
#define     SA_NODE(sa, node)     SIN_NODE((unet3_sockaddr_in*)sa, node)
#define     SIN6_NODE(sin6, node) (node)->ip6a[0]=ntohl((sin6)->sin6_addr.unet3_s6_addr32[0]);\
                                  (node)->ip6a[1]=ntohl((sin6)->sin6_addr.unet3_s6_addr32[1]);\
                                  (node)->ip6a[2]=ntohl((sin6)->sin6_addr.unet3_s6_addr32[2]);\
                                  (node)->ip6a[3]=ntohl((sin6)->sin6_addr.unet3_s6_addr32[3]);\
                                  (node)->port=ntohs((sin6)->sin6_port);\
                                  (node)->ver=(sin6)->sin6_family
#define     NODE_SIN(node, sin)   (sin)->sin_addr.s_addr=htonl((node)->ipa);\
                                  (sin)->sin_port=htons((node)->port);\
                                  (sin)->sin_family=(node)->ver
#define     NODE_SA(node, sa)     NODE_SIN(node, (unet3_sockaddr_in*)sa)
#define     NODE_SIN6(node, sin6) (sin6)->sin6_addr.unet3_s6_addr32[0]=htonl((node)->ip6a[0]);\
                                  (sin6)->sin6_addr.unet3_s6_addr32[1]=htonl((node)->ip6a[1]);\
                                  (sin6)->sin6_addr.unet3_s6_addr32[2]=htonl((node)->ip6a[2]);\
                                  (sin6)->sin6_addr.unet3_s6_addr32[3]=htonl((node)->ip6a[3]);\
                                  (sin6)->sin6_port=htons((node)->port);\
                                  (sin6)->sin6_family=(node)->ver


#define     TIME_VALUE(tmo)       ((tmo)->tv_sec*1000+((tmo)->tv_usec+999)/1000)
#define     TIME_FOREVER           0x7FFFFFFF/1000
#define     TIME_FOREVER_SEC       -1L
#define     TIME_FOREVER_USEC      999000L
#define     TIME_MAX_SEC           TIME_FOREVER
#define     TIME_MAX_SEC_USEC      0

#define     ADDRESS_LEN            16 /*sizeof(sockaddr_in)*/
#define     SIN6_LEN               28 /*sizeof(sockaddr_in6)*/ 


typedef struct t_unet3_api_msg {
    struct t_unet3_api_msg *next;
    struct t_unet3_api_msg *cancel;
    ER                      ercd;
    UB                      from;
    B                       apitype;
    API_ID                  apiid;
    ID                      aptskid;  /* origin task id */

    UB                      msgtype;

    int                     sockfd;
    T_PARAM                 par;

}T_UNET3_API_MSG;


/* kernel resources */
extern ID      TSK_BSD_API;
extern ID      MBX_BSD_REQ;
extern ID      MPF_BSD_MSG;

#ifndef NET_C_OS
extern const   T_CTSK c_bsd_tsk;
extern const   T_CMBX c_bsd_mbx;
#endif
extern const   T_CMPF c_bsd_mpf;


extern T_UNET3_BSD_SOC  gNET_BSD_SOC[];
extern UW tsk_errno[];


#define VALID_FD(fd)       (0 < (fd) && (fd) <= NET_SOC_MAX)
#define VALID_SOC(fd)      (VALID_FD(fd) && gNET_BSD_SOC[(fd)-1].sockfd)
#define LISTEN_SOC(fd)     (gNET_BSD_SOC[(fd)-1].parentfd)
#define ACTIVE_SOC(fd)     ((VALID_SOC(fd) && !LISTEN_SOC(fd)) ? &gNET_BSD_SOC[(fd)-1] : NULL)
#define ACTIVE_SOC_ALL(fd) (VALID_SOC(fd) ? &gNET_BSD_SOC[(fd)-1] : NULL)    /* include listen q */

ER msg_buf_get(T_UNET3_API_MSG **msg);
ER msg_buf_wai_get(T_UNET3_API_MSG **msg);
ER msg_buf_ret(T_UNET3_API_MSG *msg);
UB can_read(int sockfd);
UB can_write(int sockfd);
void loc_bsd(void);
void ulc_bsd(void);
void wrap_socketoption(T_UNET3_API_MSG *msg);
int get_devid_addr(UW ip);
#ifdef IPV6_SUP
int get_devid_ip6addr(UW *ip6addr);
#endif
int get_devid_soc(int sockfd);
void set_devid_soc(int sockfd, int devid);
int get_devid_name(const char *name);
void set_errno(ID tid, int err, int sockfd);
int cnvrt_err(ER ercd);

ER lo_net_ini(UH dev_num);

#ifdef NET_C_OS
void bsd_wrapper_ini(UW bsd_tcp_top, UW bsd_tcp_max, UW bsd_udp_top, UW bsd_udp_max);
#endif

#endif
