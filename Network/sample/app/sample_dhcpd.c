/***************************************************************************
    DHCPd Sample
 ***************************************************************************/
#include "kernel.h"
#include "sample_netapp_cfg.h"      /* include Sample Application Settings */

#if SAMPLE_ENA_DHCPd
#include "dhcp_server.h"

/* printf like function */
extern void shell_printf(VP ctrl, const char *fmt, ...) ;

#if (0 == SAMPLE_USE_GENSRC)    /* no use configurator */
/* Task */
SID ID_SOC_DHCPS1;
ID ID_DHCPS_TSK1;
ID ID_DHCPS_RCV_TSK1;
ID ID_DHCP_MSG_MPF1;
ID ID_DHCPS_MBX1;

void dhcp_svr_tsk1(VP_INT exinf);
void dhcp_rcv_tsk(VP_INT exinf);
void sample_dhcpd_cbk(ID type, T_DHCP_RESOURCE * dat);

#if EXEC_CPU_64BIT
#define TSKSZ_DHCPD         (1024*3)
#define TSKSZ_DHCPD_RCV     (1024*3)
#else
#define TSKSZ_DHCPD         1024
#define TSKSZ_DHCPD_RCV     1024
#endif
const T_CTSK net_ctsk_dhcps = {TA_HLNG | TA_ACT | TA_FPU, (VP_INT)0, (FP)dhcp_svr_tsk1,   6,
                               TSKSZ_DHCPD,           0,         "DhcpServerTask"};
const T_CTSK net_ctsk_dhcps_rcv = {TA_HLNG | TA_ACT | TA_FPU, (VP_INT)0, (FP)dhcp_rcv_tsk,   6,
                               TSKSZ_DHCPD_RCV,       0,         "DhcpServerTask"};

/* OS Resources */
static const T_CMPF net_cmpf_dhcps = {TA_TFIFO, 5, 552, 0, "DhcpServer"};
static const T_CMBX net_cmbx_dhcps = {TA_TFIFO | TA_MFIFO, 0, 0, "DhcpServer"};
static const T_CFLG net_cflg_telnets = {TA_TFIFO | TA_WMUL, 0x00000000, "TelnetServer"};


T_DHCP_SERVER dhcp_server1;
#ifdef DHCP_AGENT_PORT
T_DHCP_SCOPE dhcp_scope1;
#endif
static UB reserv_mac1[][6] = {
    {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC},
};
void dhcp_svr_tsk1(VP_INT exinf)
{
    net_memset((char* )&dhcp_server1, 0, sizeof(dhcp_server1));
    dhcp_server1.dev_num = ID_NETIF_DEV1;
    dhcp_server1.sid = ID_SOC_DHCPS1;
    dhcp_server1.tsk_id = ID_DHCPS_RCV_TSK1;
    dhcp_server1.mpf_id = ID_DHCP_MSG_MPF1;
    dhcp_server1.mbx_id = ID_DHCPS_MBX1;
    dhcp_server1.alt = sample_dhcpd_cbk;
#ifdef DHCP_AGENT_PORT
    net_memset((char* )&dhcp_scope1, 0, sizeof(dhcp_scope1));
    dhcp_scope1.reserve_mac = (UB**)reserv_mac1;
    dhcp_scope1.reserve_num = 1;
    dhcp_scope1.starting_addr = SPL_DHCPD_STA_ADDR;
    dhcp_scope1.lease_period = SPL_DHCPD_LEASE_PERIOD;
    dhcp_scope1.lease_num = SPL_DHCPD_LEASE_NUM;
    dhcp_scope1.subnet = SPL_DHCPD_SUBNETMASK;
    dhcp_scope1.gateway = SPL_DHCPD_GATEWAY;
    dhcp_scope1.other_opt = NULL;
    dhcp_server1.scope = &dhcp_scope1;
#else
    dhcp_server1.reserve_mac = (UB**)reserv_mac1;
    dhcp_server1.reserve_num = 1;
    dhcp_server1.starting_addr = SPL_DHCPD_STA_ADDR;
    dhcp_server1.lease_period = SPL_DHCPD_LEASE_PERIOD;
    dhcp_server1.lease_num = SPL_DHCPD_LEASE_NUM;
    dhcp_server1.subnet = SPL_DHCPD_SUBNETMASK;
    dhcp_server1.gateway = SPL_DHCPD_GATEWAY;
    dhcp_server1.other_opt = NULL;
#endif

    dhcp_server(&dhcp_server1);
}

ER sample_dhcpd_ini()
{
    ER ercd;
    T_NODE lo_host = {0};

    lo_host.num = SAMPLE_SOCDEV_SER;
    lo_host.ver = IP_VER4;
    lo_host.ipa = INADDR_ANY;
    lo_host.port = 67U;

    ercd         = cre_soc(IP_PROTO_UDP, &lo_host);
    if (0 >= ercd) {
        return ercd;
    }
    ID_SOC_DHCPS1 = (SID)ercd;

    ercd = acre_mpf((T_CMPF*)&net_cmpf_dhcps);
    if (0 >= ercd) {
        return ercd;
    }
    ID_DHCP_MSG_MPF1 = ercd;

    ercd = acre_mbx((T_CMBX*)&net_cmbx_dhcps);
    if (0 >= ercd) {
        return ercd;
    }
    ID_DHCPS_MBX1 = ercd;
    
    
    /* Starting Server Task  */
    ercd       = acre_tsk((T_CTSK*)&net_ctsk_dhcps_rcv);
    if (ercd <= E_OK) {
        return ercd;
    }
    ID_DHCPS_RCV_TSK1 = ercd;
    
    ercd       = acre_tsk((T_CTSK*)&net_ctsk_dhcps);
    if (ercd <= E_OK) {
        return ercd;
    }
    ID_DHCPS_TSK1 = ercd;
    (void)act_tsk((ID)ercd);

    return E_OK;
}
#else
extern T_DHCP_SERVER dhcp_server1;
ER sample_dhcpd_ini()
{
    return sta_tsk(ID_DHCPS_TSK1, 0x00);
}
#endif


/* Command 'dhcpd_start' */
ER shell_usr_cmd_dhcpd_sta(VP ctrl, INT argc, VB *argv[])
{
    ER ercd;
    T_RTST tskinfo;
    
    /* get task status */
    ercd = ref_tst( ID_DHCPS_TSK1, &tskinfo );
    if ( ercd != E_OK ) {
        return ercd;
    }

    if ( tskinfo.tskstat == TTS_DMT ) {
        /* Start DHCPd Task */
        sta_tsk(ID_DHCPS_TSK1, 0);
        shell_puts(ctrl, SPL_LF"DHCP server start.");
    } else {
        shell_puts(ctrl, SPL_LF"DHCP server already in startup.");
    }
    
    return ercd;
}

/* Command 'dhcpd_stop' */
ER shell_usr_cmd_dhcpd_stp(VP ctrl, INT argc, VB *argv[])
{
    ER ercd;
    T_RTST tskinfo;
    
    /* get task status */
    ercd = ref_tst( ID_DHCPS_TSK1, &tskinfo );
    if ( ercd != E_OK ) {
        return ercd;
    }

    if ( tskinfo.tskstat != TTS_DMT ) {
        /* Stop DHCPd Task */
        dhcp_server_stop( ID_NETIF_DEV1, 10 );
        shell_puts(ctrl, SPL_LF"DHCP server stop.");
    } else {
        shell_puts(ctrl, SPL_LF"DHCP server already in stop.");
    }
    
    return ercd;
}


/* Command 'dhcpd_stat' */
UW get_net_sec(void);
static VP gCTRL = NULL;
ER shell_usr_cmd_dhcpd_sts(VP ctrl, INT argc, VB *argv[])
{
#ifdef DHCP_AGENT_PORT
    T_DHCP_SCOPE *sp = dhcp_server1.scope;
#else
    T_DHCP_SERVER *sp = &dhcp_server1;
#endif    
    T_DHCP_RESOURCE *pool;
    VB sipa[16];
    UW i;
    UW e;
    UB *mac;
    
    ip_ntoa(sipa, sp->starting_addr);
    
    shell_printf(ctrl, SPL_LF"DHCP server status"); 
    shell_printf(ctrl, SPL_LF"  Start Address     : %s", sipa); 
    shell_printf(ctrl, SPL_LF"  Lease Period(sec) : %d", sp->lease_period);
    shell_printf(ctrl, SPL_LF"  Lease Num         : %d", sp->lease_num); 
    shell_printf(ctrl, SPL_LF"  Reserved Num      : %d", sp->reserve_num); 
    shell_printf(ctrl, SPL_LF"  Elapsed Time      : %d", get_net_sec());
    
    shell_printf(ctrl, SPL_LF"  Lease Info (ipa, mac, expiration)");
    e = 0;
    for (i = 0; i < sp->lease_num; i++) {
        pool = &sp->pool[i];
        mac = pool->mac;
        if ((0 == pool->expiration) && (0x00 == mac[0]))	continue;
//        if (0 == pool->expiration)	continue;
        ip_ntoa(sipa, pool->ipaddr);
        shell_printf(ctrl, SPL_LF"    %3d: %s, %02x-%02x-%02x-%02x-%02x-%02x, %d", 
            i, sipa, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], pool->expiration);
        e++;
    }
    if (0 == e) {
        shell_printf(ctrl, SPL_LF"    No entry.");
    }
    
    gCTRL = ctrl;
    
    return E_OK;
}

void sample_dhcpd_cbk(ID type, T_DHCP_RESOURCE * dat)
{
    VB *stype;
    VB sipa[16];
    UB *mac;
    
    if (!gCTRL)     return;
    
    switch (type) {
    case DHCP_SERVER_ALT_ENTRYPOOL:     stype = "ENTRYPOOL";    break;
    case DHCP_SERVER_ALT_FULLPOOL:      stype = "FULLPOOL";     break;
    case DHCP_SERVER_ALT_EXPIREPOOL:    stype = "EXPIREPOOL";   break;
    case DHCP_SERVER_ALT_RELPOOL:       stype = "RELPOOL";      break;
    case DHCP_SERVER_ALT_DECLINE:       stype = "DECLINE";      break;
    case DHCP_SERVER_ALT_OTHER:         stype = "OTHER";        break;
    default:                            stype = "Unknown Type"; break;
    }
    
    ip_ntoa(sipa, dat->ipaddr);
    mac = dat->mac;
    shell_printf(gCTRL, SPL_LF"type=%s, ipa=%s, mac=%02x-%02x-%02x-%02x-%02x-%02x", 
                    stype, sipa, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

#endif  /* #if SAMPLE_ENA_DHCPd */
