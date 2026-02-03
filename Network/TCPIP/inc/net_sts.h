/***************************************************************************
    MICRO C CUBE / COMPACT/STANDARD, NETWORK
    Network status header, internal structure
    Copyright (c) 2014-2023, eForce Co., Ltd. All rights reserved.
    
    Version Information
      2014.03.06: Created
      2023.05.25: Fixed interface MIB is invalid in 64bit environment.
 **************************************************************************/

#ifndef NETSTS_H
#define NETSTS_H
#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
    Status
***************************************************************************/
/* Interface status */
typedef struct t_net_sts_ifs {
    INT index;              /* Index [1..4] */
    UW descr;               /* Descripter (ex. net0) */
    INT type;               /* Type (Ethernet CSMACD(6), PPP(23)) */
    INT mtu;                /* Maximum transmission unit(MTU) */
    UW speed;               /* Speed (bps) */
    UW phy_addr;            /* MAC address */
    INT admin_sts;          /* User request for link of the interface */
    INT oper_sts;           /* Current link status (Link up(1), down(2)) */
    UW reserve;             /* Reserved */
    UW in_octet;            /* Number of octets received (in) */
    UW in_ucast_pkt;        /* Number of unicast packets (in) */
    UW in_nucast_pkt;       /* Number of broadcast or multicast packets (in) */
    UW in_discard;          /* Number of discarded packets even no errors (in) */
    UW in_err;              /* Number of error packets (in) */
    UW in_unknown_proto;    /* Number of unknown or unsupported packets (in) */
    UW out_octet;           /* Number of octets transmitted (out) */
    UW out_ucast_pkt;       /* Number of unicast packets (out) */
    UW out_nucast_pkt;      /* Number of broadcast or multicast packets (out) */
    UW out_discard;         /* Number of discarded packets even no errors (out) */
    UW out_err;             /* Number of error packets (out) */
    UW out_qlen;            /* Length of output packet queue */
    
#if (_kernel_SIZE_SIZE==8)
    UW descr_h;             /* upper32bit - Descripter (ex. net0) */
    UW phy_addr_h;          /* upper32bit - MAC address */
#endif
} T_NET_STS_IFS;

/* IP status */
typedef struct t_net_sts_ip {
    INT forward;            /* Forwarding (Forwarding(1), Not forwarding(2))*/
    INT def_ttl;            /* Default TTL(Time-To-Live) */
    UW in_rcv;              /* Total number of input datagrams (in) */
    UW in_hdr_err;          /* Number of datagrams discarded errors in header (in) */
    UW in_addr_err;         /* Number of datagrams discarded invalid addresses (in) */
    UW in_forw_datagram;    /* Number of datagrams forwarded (in) */
    UW in_unknown_prot;     /* Number of datagrams discarded unknown unsupported protocol (in) */
    UW in_discard;          /* Number of datagrams discarded for no problems (in) */
    UW in_deliver;          /* Number of input datagrams successfully delivered (in) */
    UW out_req;             /* Total number of datagrams requests for transmission (out) */
    UW out_discard;         /* Number of datagrams discarded for no problems (out) */
    UW out_no_route;        /* Number of datagrams discarded no route (out) */
    INT reasm_timeout;      /* Maximum number of seconds held for reassemble fragments */
    UW reasm_req;           /* Number of fragments received needed to be reassembled */
    UW reasm_ok;            /* Number of datagrams successfully reassembled */
    UW reasm_fail;          /* Number of failures detected by the reassembly */
    UW frag_ok;             /* Number of datagrams fragmented successfully */
    UW frag_fail;           /* Number of datagrams discarded needed to be fragmented */
    UW frag_create;         /* Number of datagram fragments generated */
    UW routing_discard;     /* Number of routing entries discarded even they are valid */
} T_NET_STS_IP;

/* ICMP status */
typedef struct t_net_sts_icmp {
    UW in_msg;              /* Total number of ICMP messages (in) */
    UW in_err;              /* Number of error messages (in) */
    UW in_dest_unreach;     /* Number of destination unreachable messages (in) */
    UW in_time_excd;        /* Number of time exceeded messages (in) */
    UW in_parm_prob;        /* Number of parameter problem messages (in) */
    UW in_src_quench;       /* Number of source quench messages (in) */
    UW in_redirect;         /* Number of redirect messages (in) */
    UW in_echo;             /* Number of echo messages (in) */
    UW in_echo_rep;         /* Number of echo reply messages (in) */
    UW in_time_stamp;       /* Number of timestamp messages (in) */
    UW in_time_stamp_rep;   /* Number of timestamp reply messages (in) */
    UW in_addr_mask;        /* Number of address mask messages (in) */
    UW in_addr_mask_rep;    /* Number of address mask reply messages (in) */
    UW out_msg;             /* Total number of ICMP (out) */
    UW out_err;             /* Number of error messages (out) */
    UW out_dest_unreach;    /* Number of destination unreachable messages (out) */
    UW out_time_excd;       /* Number of time exceeded messages (out) */
    UW out_parm_prob;       /* Number of parameter problem messages (out) */
    UW out_src_quench;      /* Number of source quench messages (out) */
    UW out_redirect;        /* Number of redirect messages (out) */
    UW out_echo;            /* Number of echo messages (out) */
    UW out_echo_rep;        /* Number of echo reply messages (out) */
    UW out_time_stamp;      /* Number of timestamp messages (out) */
    UW out_time_stamp_rep;  /* Number of timestamp reply messages (out) */
    UW out_addr_mask;       /* Number of address mask messages (out) */
    UW out_addr_mask_rep;   /* Number of address mask reply messages (out) */
} T_NET_STS_ICMP;

/* TCP status */
typedef struct t_net_sts_tcp {
    INT rto_algo;           /* RTO algorithm (Other(1)) */
    INT rto_min;            /* Minimum value of RTO */
    INT rto_max;            /* Maximum value of RTO */
    INT max_conn;           /* Maximum number of connections */
    UW active_open;         /* Number of connections */
    UW passive_open;        /* Number of connections (passive) */
    UW attempt_fail;        /* Number of connections to failed */
    UW estab_reset;         /* Number of connections to reseted */
    UW curr_estab;          /* Number of connections to established or close-wait */
    UW in_seg;              /* Total number of segments (in) */
    UW out_seg;             /* Total number of segments (out) */
    UW retrans_seg;         /* Number of segments retransmitted */
    UW in_err;              /* Number of segments received in error (in) */
    UW out_rst;             /* Number of segments sent containing the reset flag (out) */
} T_NET_STS_TCP;

/* UDP status */
typedef struct t_net_sts_udp {
    UW in_datagram;         /* Total number of datagrams (in) */
    UW no_port;             /* Number of datagrams no port (in) */
    UW in_err;              /* Number of datagrams in error (in) */
    UW out_datagram;        /* Total number of datagrams (out) */
} T_NET_STS_UDP;

/***************************************************************************
    Internal
***************************************************************************/
/* Network device status */
typedef struct t_net_sts_dev {
    UH sts;         /* Status */
} T_NET_STS_DEV;

/* ARP status */
typedef struct t_net_sts_arp {
    UW ipaddr;      /* Resolving IP address */
    UH dev_num;     /* Device number */
    UB mac[6];      /* Valid MAC */
    UB sts;         /* Status */
} T_NET_STS_ARP;

/* Socket status */
typedef struct t_net_sts_soc {
    UW raddr;       /* Remote address */
    UH rport;       /* Remote port */
    UH lport;       /* Local port */
    UH dev_num;     /* Device number */
    UH state;       /* Status */
    UB proto;       /* Protocol */
    UB bsd;
} T_NET_STS_SOC;

/* Status block */
typedef struct t_net_sts {
    T_NET_STS_IFS* ifs;             /* Interface table */
    T_NET_STS_IP ip;                /* IP */
    T_NET_STS_ICMP icmp;            /* ICMP */
    T_NET_STS_TCP tcp;              /* TCP */
    T_NET_STS_UDP udp;              /* UDP */
} T_NET_STS;

/* Configuration block */
typedef struct t_net_sts_cfg {
    T_NET_STS_DEV* dev;             /* Network device */
    T_NET_STS_IFS* ifs;             /* Interface table */
    T_NET_STS_IFS* ifs_tmp;         /* Interface table (Temporary) */
    T_NET_STS_ARP* arp;             /* ARP status */
    T_NET_STS_SOC* soc;             /* Socket status */
    VP* sts_ptr;                    /* Status table */
    VP* sts_ptr_tmp;                /* Status table (Temporary) */
    void (*cbk_snmp)(UH, UH, VP);   /* Callback function for SNMP */
} T_NET_STS_CFG;

#ifdef __cplusplus
}
#endif
#endif

