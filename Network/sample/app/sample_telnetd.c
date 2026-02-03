/***************************************************************************
    TELNETd Sample (Operating as command in shell)
    Copyright (c)  2020, eForce Co., Ltd. All rights reserved.

    2020-03-23: Created.
 ***************************************************************************/
#include "kernel.h"
#include "sample_netapp_cfg.h"      /* include Sample Application Settings */

#if SAMPLE_ENA_TELNETd
#include "telnet_server.h"

#if (0 == SAMPLE_USE_GENSRC)    /* no use configurator */
/* Task */
static ID net_id_tsk_telnets = 0;
static ID net_id_tsk_shell = 0;
void net_telnets_tsk(VP_INT);
void net_shell_tsk(VP_INT);
#if EXEC_CPU_64BIT
static const T_CTSK net_ctsk_telnets = {
    TA_HLNG | TA_FPU, (VP_INT)0, (FP)net_telnets_tsk, 6, 0xC00, 0, "TelnetServer"
};
static const T_CTSK net_ctsk_shell = {
    TA_HLNG | TA_FPU, (VP_INT)0, (FP)net_shell_tsk, 6, 0x1800, 0, "Shell"
};
#else
static const T_CTSK net_ctsk_telnets = {
    TA_HLNG | TA_FPU, (VP_INT)0, (FP)net_telnets_tsk, 6, 0x400, 0, "TelnetServer"
};
static const T_CTSK net_ctsk_shell = {
    TA_HLNG | TA_FPU, (VP_INT)0, (FP)net_shell_tsk, 6, 0x800, 0, "Shell"
};
#endif

/* OS Resources */
static const T_CMPF net_cmpf_telnets = {TA_TFIFO, 6, 144, 0, "TelnetServer"};
static const T_CMBX net_cmbx_telnets = {TA_TFIFO | TA_MFIFO, 0, 0, "TelnetServer"};
static const T_CFLG net_cflg_telnets = {TA_TFIFO | TA_WMUL, 0x00000000, "TelnetServer"};

/* Control Block */
static T_TELNET_SERVER net_ctl_telnets;
static T_SHELL_CTL net_ctl_shell;

/*******************************
    Telnet Server Task
 *******************************/
void net_telnets_tsk(VP_INT exinf)
{
    net_ctl_telnets.dev_num = 0;
    net_ctl_telnets.port = INADDR_ANY;
    net_ctl_telnets.shell_tid = net_id_tsk_shell;

    telnet_server(&net_ctl_telnets);
}

/***********************************
    Shell Task for Telnet Server
 ***********************************/
void net_shell_tsk(VP_INT exinf)
{
    net_memset(&net_ctl_shell, 0, sizeof(T_SHELL_CTL));

    net_ctl_shell.typ = TYP_TELNET;
    net_ctl_shell.pcb = &net_ctl_telnets;

    shell_sta(&net_ctl_shell);
}

ER sample_telnetd_ini()
{
    ER ercd;
    T_NODE node;

    /* Create Resources for Telnet Server */
    net_memset(&net_ctl_telnets, 0, sizeof(T_TELNET_SERVER));
    ercd = acre_mpf((T_CMPF*)&net_cmpf_telnets);
    if (ercd <= E_OK) {
        return ercd;
    }
    net_ctl_telnets.mpf_id = (ID)ercd;

    ercd = acre_flg((T_CFLG*)&net_cflg_telnets);
    if (ercd <= E_OK) {
        return ercd;
    }
    net_ctl_telnets.flg_id = (ID)ercd;

    ercd = acre_mbx((T_CMBX*)&net_cmbx_telnets);
    if (ercd <= E_OK) {
        return ercd;
    }
    net_ctl_telnets.mbx_id = (ID)ercd;

    node.num  = SAMPLE_SOCDEV_SER;
    node.ipa  = INADDR_ANY;
    node.port = 23;
    node.ver  = IP_VER4;
    ercd = cre_soc(IP_PROTO_TCP, &node);
    if (ercd <= E_OK) {
        return ercd;
    }
    net_ctl_telnets.sid = (ID)ercd;

    /* Telnet Server Task */
    ercd = acre_tsk((T_CTSK*)&net_ctsk_telnets);
    if (ercd <= E_OK) {
        return ercd;
    }
    net_id_tsk_telnets = (ID)ercd;

    /* Shell Task */
    ercd = acre_tsk((T_CTSK*)&net_ctsk_shell);
    if (ercd <= E_OK) {
        return ercd;
    }
    net_id_tsk_shell = (ID)ercd;

    /* Start telnet server task */
    return sta_tsk(net_id_tsk_telnets, 0x00);
}
#else
ER sample_telnetd_ini()
{
    return sta_tsk(ID_TELNET_TSK1, 0x00);
}
#endif
#endif /* SAMPLE_ENA_TELNETd */

