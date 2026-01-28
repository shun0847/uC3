/***************************************************************************
    FTPd Sample
 ***************************************************************************/
#include "kernel.h"
#include "sample_netapp_cfg.h"      /* include Sample Application Settings */

#if SAMPLE_ENA_FTPd
#include "ftp_server.h"


#if (0 == SAMPLE_USE_GENSRC)	/* no use configurator */
/*******************************
    FTP Server Task
 *******************************/
SID          ID_SOC_FTPD_CTL1;
SID          ID_SOC_FTPD_DATA1;
T_FTP_SERVER net_ctl_ftps;

void ftp_tsk(VP_INT exinf)
{
    net_memset(&net_ctl_ftps, 0, sizeof(net_ctl_ftps));

    net_ctl_ftps.dev_num = SAMPLE_SOCDEV_SER;
    net_ctl_ftps.ctl_sid = ID_SOC_FTPD_CTL1;
    net_ctl_ftps.dat_sid = ID_SOC_FTPD_DATA1;

    (void)ftp_server(&net_ctl_ftps);
}

#if EXEC_CPU_64BIT
const T_CTSK net_ctsk_ftps = {TA_HLNG | TA_ACT | TA_FPU, (VP_INT)0U, (FP)ftp_tsk,    6U,
                              0x1800U,           0U,         "FtpServerTask"};
#else
const T_CTSK net_ctsk_ftps = {TA_HLNG | TA_ACT | TA_FPU, (VP_INT)0U, (FP)ftp_tsk,    6U,
                              0x800U,           0U,         "FtpServerTask"};
#endif


ER sample_ftpd_ini()
{
	ER ercd;
    T_NODE lo_host = {0};

    lo_host.num = SAMPLE_SOCDEV_SER;
    lo_host.ver = IP_VER4;
    lo_host.ipa = INADDR_ANY;

    /* FTPd */
    lo_host.port = 21U;
    ercd         = cre_soc(IP_PROTO_TCP, &lo_host);
    if (0 >= ercd) {
        return ercd;
    }
    ID_SOC_FTPD_CTL1 = (SID)ercd;
    (void)cfg_soc((SID)ercd, SOC_TMO_SND, (VP)5000U);
    (void)cfg_soc((SID)ercd, SOC_TMO_RCV, (VP)5000U);
    (void)cfg_soc((SID)ercd, SOC_TMO_CON, (VP)5000U);

    lo_host.port = 20U;
    ercd         = cre_soc(IP_PROTO_TCP, &lo_host);
    if (0 >= ercd) {
        return ercd;
    }
    ID_SOC_FTPD_DATA1 = (SID)ercd;
	
    /* Starting Server Task  */
    ercd = acre_tsk((T_CTSK*)&net_ctsk_ftps);
    if (ercd <= E_OK) {
        return ercd;
    }
    (void)act_tsk((ID)ercd);

    return E_OK;
}
#endif

#endif  /* #if SAMPLE_ENA_FTPd */
